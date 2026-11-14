/* Dolphin FIFO log (.dff) recorder: writes the GPU command stream of a range of
 * frames, with the memory it reads, in the format of Dolphin's FIFO player
 * (Source/Core/Core/FifoPlayer/FifoDataFile.cpp, version 5), so the same frames
 * can be rendered by Dolphin and compared with ours.
 *
 *   MKDD_DFF=<file>  MKDD_DFF_FRAMES=<start>:<count>
 *
 * A frame is the command stream up to and including a copy to the XFB; frame
 * numbers count XFB copies from boot (0 = the first). Recording starts after
 * XFB copy `start - 1` with a snapshot of the BP/CP/XF registers and TMEM.
 *
 * Addresses: host pointers are written unchanged and the log declares a 64 MB
 * MEM1 (Dolphin's RAM override), so every GX address in the stream (texture
 * and TLUT bases, array bases, display lists, copy destinations) stays valid
 * without rewriting. The memory a frame reads is recorded as memory updates at
 * the position of the top-level command that reads it: display lists, texture
 * images and TLUTs as they are (big-endian), vertex array elements and indexed
 * matrix loads (host order in our RAM) byte-swapped per component to the
 * big-endian layout the GPU expects. A shadow of what was written so far keeps
 * unchanged data out of the log. Texture reads of EFB copy destinations are not
 * recorded (the copy lives in the GPU, as on the console).
 *
 * Play a log with Dolphin (dolphin-emu-nogui -e log.dff) or with our renderer
 * (port/gpu/tools/dffplay.*). With a WebGL backend the XFB of every recorded
 * frame is also written next to the log as <file>.ours-<frame>.ppm. "[dff] XFB
 * copy N" lines in the log help pick frame numbers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpu_internal.h"
#include "texdec.h"

int gpu_fifolog_on;     /* recording the current frame */
int gpu_fifolog_wanted; /* MKDD_DFF set */

char *gpu_js_env_str(const char *name);

#define MEM_SIZE 0x04000000u /* declared MEM1 size */
#define MAX_FRAMES 64

typedef struct Update {
    uint32_t pos, addr, off, size; /* off: into sBlob */
} Update;

typedef struct Frame {
    uint8_t *fifo;
    uint32_t fifoLen, fifoCap;
    Update *upd;
    uint32_t nUpd, capUpd;
} Frame;

static char *sPath;
static uint32_t sStart, sCount, sXfbCopies, sDone;
static Frame sFrames[MAX_FRAMES];
static uint32_t sCur; /* frame being recorded (index into sFrames) */
static uint8_t *sBlob;
static uint32_t sBlobLen, sBlobCap;
static uint8_t *sShadow, *sValid; /* shadow of recorded RAM, 1 bit per byte valid */
static uint32_t sMaxAddr;
static uint32_t sBp[0x100], sCp[0x100], sXfRegs[0x58], sXfMem[0x1000];
static uint32_t sTevColor[8], sTevKonst[8];
static uint8_t *sTmemSnap;

/* EFB copy destinations (texture reads there aren't recorded) */
#define MAX_COPY_RANGES 256
static uint32_t sCopyLo[MAX_COPY_RANGES], sCopyHi[MAX_COPY_RANGES];
static uint32_t sNumCopies;

/* vertex arrays read by the current draw */
static struct {
    uint32_t lo, hi, stride, base, elem;
    uint8_t csize, used;
} sArr[16];

static void *grow(void *p, uint32_t *cap, uint32_t need, uint32_t elem)
{
    uint32_t c;
    if (need <= *cap)
        return p;
    c = *cap ? *cap : 1024;
    while (c < need)
        c *= 2;
    p = realloc(p, (size_t)c * elem);
    if (!p) {
        fprintf(stderr, "[dff] out of memory\n");
        abort();
    }
    *cap = c;
    return p;
}

void gpu_fifolog_init(void)
{
    char *range;
    sPath = gpu_js_env_str("MKDD_DFF");
    if (!sPath)
        return;
    range = gpu_js_env_str("MKDD_DFF_FRAMES");
    sStart = 600;
    sCount = 3;
    if (range) {
        sscanf(range, "%u:%u", &sStart, &sCount);
        free(range);
    }
    if (sCount < 1)
        sCount = 1;
    if (sCount > MAX_FRAMES)
        sCount = MAX_FRAMES;
    gpu_fifolog_wanted = 1;
    gpu_xform_enabled = 1; /* vertex arrays are only read when draws are decoded */
    fprintf(stderr, "[dff] recording frames %u..%u to %s\n", sStart, sStart + sCount - 1, sPath);
}

/* ---- memory updates ---- */

static int valid_range(uint32_t addr, uint32_t size)
{
    uint32_t i;
    for (i = addr; i < addr + size; i++)
        if (!(sValid[i >> 3] & (1u << (i & 7))))
            return 0;
    return 1;
}

static void add_update(uint32_t addr, const uint8_t *data, uint32_t size)
{
    Frame *f = &sFrames[sCur];
    Update *u;
    uint32_t i;
    if (!size)
        return;
    if (addr >= MEM_SIZE || size > MEM_SIZE - addr) {
        static int warned;
        if (warned++ < 10)
            fprintf(stderr, "[dff] address %08x+%x outside the %u MB RAM, not recorded\n", addr, size, MEM_SIZE >> 20);
        return;
    }
    if (valid_range(addr, size) && !memcmp(sShadow + addr, data, size))
        return;
    memcpy(sShadow + addr, data, size);
    for (i = addr; i < addr + size; i++)
        sValid[i >> 3] |= (uint8_t)(1u << (i & 7));
    if (addr + size > sMaxAddr)
        sMaxAddr = addr + size;
    sBlob = (uint8_t *)grow(sBlob, &sBlobCap, sBlobLen + size, 1);
    memcpy(sBlob + sBlobLen, data, size);
    f->upd = (Update *)grow(f->upd, &f->capUpd, f->nUpd + 1, sizeof(Update));
    u = &f->upd[f->nUpd++];
    u->pos = f->fifoLen;
    u->addr = addr;
    u->off = sBlobLen;
    u->size = size;
    sBlobLen += size;
}

static void mem_be(uint32_t addr, uint32_t size)
{
    if (size && gpu_ram_ok(addr, size))
        add_update(addr, (const uint8_t *)(uintptr_t)addr, size);
}

/* Host-order data: swap each `csize`-byte component of `elem`-byte elements
 * `stride` apart (gaps between elements are copied as they are). */
static void mem_swapped(uint32_t addr, uint32_t size, uint32_t csize, uint32_t elem, uint32_t stride)
{
    static uint8_t *tmp;
    static uint32_t tmpCap;
    const uint8_t *src;
    uint32_t e, k;
    if (!size || !gpu_ram_ok(addr, size))
        return;
    src = (const uint8_t *)(uintptr_t)addr;
    tmp = (uint8_t *)grow(tmp, &tmpCap, size, 1);
    memcpy(tmp, src, size);
    if (csize > 1) {
        if (!stride)
            stride = elem;
        for (e = 0; e + elem <= size; e += stride)
            for (k = 0; k + csize <= elem; k += csize) {
                uint8_t *p = tmp + e + k;
                if (csize == 2) {
                    uint8_t t = p[0]; p[0] = p[1]; p[1] = t;
                } else {
                    uint8_t t0 = p[0], t1 = p[1];
                    p[0] = p[3]; p[1] = p[2]; p[2] = t1; p[3] = t0;
                }
            }
    }
    add_update(addr, tmp, size);
}

static int in_copy(uint32_t addr, uint32_t size)
{
    uint32_t i;
    for (i = 0; i < sNumCopies; i++)
        if (addr < sCopyHi[i] && addr + size > sCopyLo[i])
            return 1;
    return 0;
}

/* ---- hooks from the front end ---- */

void gpu_fifolog_dl(uint32_t addr, uint32_t size) { mem_be(addr, size); }

void gpu_fifolog_tlut(uint32_t src, uint32_t bytes) { mem_be(src, bytes); }

void gpu_fifolog_indexed(uint32_t src, uint32_t bytes) { mem_swapped(src, bytes, 4, bytes, bytes); }

void gpu_fifolog_array(int array, uint32_t index, uint32_t elem, uint32_t csize)
{
    uint32_t stride = gpu_r.cp[0xB0 + array], base = gpu_r.cp[0xA0 + array];
    if (!sArr[array].used) {
        sArr[array].used = 1;
        sArr[array].lo = sArr[array].hi = index;
        sArr[array].elem = elem;
        sArr[array].csize = (uint8_t)csize;
        sArr[array].stride = stride;
        sArr[array].base = base;
        return;
    }
    if (index < sArr[array].lo)
        sArr[array].lo = index;
    if (index > sArr[array].hi)
        sArr[array].hi = index;
    if (elem > sArr[array].elem)
        sArr[array].elem = elem;
}

static void textures(void)
{
    const uint32_t *bp = gpu_r.bp;
    int ntev = (int)((bp[0x00] >> 10) & 15) + 1, nind = (int)((bp[0x00] >> 16) & 7), st, i;
    uint32_t maps = 0;
    for (st = 0; st < ntev; st++) {
        uint32_t o = (bp[0x28 + st / 2] >> ((st & 1) * 12)) & 0xFFF;
        if (o & 0x40)
            maps |= 1u << (o & 7);
    }
    for (i = 0; i < nind && i < 4; i++)
        maps |= 1u << ((bp[0x27] >> (i * 6)) & 7);
    for (i = 0; i < 8; i++) {
        int base = i < 4 ? 0x80 + i : 0xA0 + (i - 4);
        uint32_t mode0, mode1, image0, addr, bytes;
        int w, h, fmt, minf, levels = 1;
        if (!(maps & (1u << i)))
            continue;
        mode0 = bp[base];
        mode1 = bp[base + 4];
        image0 = bp[base + 8];
        addr = (bp[base + 0x14] & 0xFFFFFF) << 5;
        w = (int)(image0 & 0x3FF) + 1;
        h = (int)((image0 >> 10) & 0x3FF) + 1;
        fmt = (int)((image0 >> 20) & 15);
        minf = (int)((mode0 >> 5) & 7);
        if (minf == 1 || minf == 2 || minf == 5 || minf == 6) {
            int maxLevels = 1;
            while ((w >> maxLevels) || (h >> maxLevels))
                maxLevels++;
            levels = (int)((mode1 >> 8) & 0xFF) / 16 + 1;
            if (levels > maxLevels)
                levels = maxLevels;
        }
        bytes = texdec_chain_size(fmt, w, h, levels);
        if (bytes && !in_copy(addr, bytes))
            mem_be(addr, bytes);
    }
}

void gpu_fifolog_draw_begin(void)
{
    memset(sArr, 0, sizeof sArr);
    textures();
}

void gpu_fifolog_draw_end(void)
{
    int a;
    for (a = 0; a < 16; a++) {
        uint32_t stride, lo, size;
        if (!sArr[a].used)
            continue;
        stride = sArr[a].stride;
        lo = sArr[a].base + sArr[a].lo * stride;
        size = (sArr[a].hi - sArr[a].lo) * stride + sArr[a].elem;
        mem_swapped(lo, size, sArr[a].csize, sArr[a].elem, stride);
    }
    memset(sArr, 0, sizeof sArr);
}

void gpu_fifolog_efb_copy(const GpuRegs *r, int toXfb)
{
    uint32_t addr = (r->bp[0x4B] & 0xFFFFFF) << 5, exec = r->bp[0x52], bytes, i;
    int sw = (int)((r->bp[0x4A] & 0x3FF) + 1), sh = (int)(((r->bp[0x4A] >> 10) & 0x3FF) + 1);
    if (toXfb) { /* keep the player's FIFO buffer clear of the XFB */
        bytes = (uint32_t)((r->bp[0x4D] & 0x3FF) << 5) * (uint32_t)sh;
        if (addr + bytes > sMaxAddr && addr + bytes <= MEM_SIZE)
            sMaxAddr = addr + bytes;
        return;
    }
    if ((exec >> 9) & 1) {
        sw = (sw + 1) / 2;
        sh = (sh + 1) / 2;
    }
    bytes = (uint32_t)((r->bp[0x4D] & 0x3FF) << 5) * (uint32_t)((sh + 3) / 4 + 1);
    if (!bytes)
        bytes = (uint32_t)sw * (uint32_t)sh * 4;
    for (i = 0; i < sNumCopies; i++)
        if (sCopyLo[i] == addr)
            break;
    if (i == sNumCopies) {
        if (sNumCopies == MAX_COPY_RANGES)
            return;
        sNumCopies++;
    }
    sCopyLo[i] = addr;
    sCopyHi[i] = addr + bytes;
    if (addr + bytes > sMaxAddr && addr + bytes <= MEM_SIZE)
        sMaxAddr = addr + bytes;
}

/* ---- frames ---- */

static void put8(Frame *f, uint8_t v)
{
    f->fifo = (uint8_t *)grow(f->fifo, &f->fifoCap, f->fifoLen + 1, 1);
    f->fifo[f->fifoLen++] = v;
}

static void put32(Frame *f, uint32_t v)
{
    put8(f, (uint8_t)(v >> 24));
    put8(f, (uint8_t)(v >> 16));
    put8(f, (uint8_t)(v >> 8));
    put8(f, (uint8_t)v);
}

static void begin(void)
{
    uint32_t i;
    Frame *f;
    sShadow = (uint8_t *)calloc(MEM_SIZE, 1);
    sValid = (uint8_t *)calloc(MEM_SIZE / 8, 1);
    sTmemSnap = (uint8_t *)malloc(1 << 20);
    if (!sShadow || !sValid || !sTmemSnap) {
        fprintf(stderr, "[dff] out of memory\n");
        gpu_fifolog_wanted = 0;
        return;
    }
    memcpy(sBp, gpu_r.bp, sizeof sBp);
    memcpy(sCp, gpu_r.cp, sizeof sCp);
    memcpy(sXfRegs, gpu_r.xf, sizeof sXfRegs);
    memset(sXfMem, 0, sizeof sXfMem);
    memcpy(sXfMem, gpu_r.xfmem, sizeof gpu_r.xfmem);
    memcpy(sTevColor, gpu_r.tevColor, sizeof sTevColor);
    memcpy(sTevKonst, gpu_r.tevKonst, sizeof sTevKonst);
    /* Dolphin's TMEM keeps TLUTs at (offset << 9), ours at 0x80000 + (offset << 9) */
    memcpy(sTmemSnap, gpu_tmem(), 1 << 20);
    memcpy(sTmemSnap, gpu_tmem() + 0x80000, 0x80000);
    sCur = 0;
    gpu_fifolog_on = 1;
    /* Both register files behind BP 0xE0-0xE7 (TEV colors and konst colors);
     * the register snapshot can only hold one of them. */
    f = &sFrames[0];
    for (i = 0; i < 8; i++) {
        put8(f, 0x61);
        put32(f, (0xE0u + i) << 24 | (sTevKonst[i] & 0xFFFFFF) | 0x800000);
        put8(f, 0x61);
        put32(f, (0xE0u + i) << 24 | (sTevColor[i] & 0x7FFFFF));
    }
    fprintf(stderr, "[dff] recording started\n");
}

static void w32(FILE *fp, uint32_t v) { fwrite(&v, 4, 1, fp); } /* little-endian host */
static void w64(FILE *fp, uint64_t v) { fwrite(&v, 8, 1, fp); }

static void write_file(void)
{
    FILE *fp = fopen(sPath, "wb");
    uint32_t i, k, nFrames = sCur;
    uint64_t frameList = 128, bpOff, cpOff, xfOff, xfrOff, texOff, pos;
    uint64_t *fifoOff, *updOff;
    uint32_t fifoStart, fifoEnd;
    static const uint8_t zero[64];
    if (!fp) {
        fprintf(stderr, "[dff] cannot write %s\n", sPath);
        return;
    }
    /* the player streams the commands through a FIFO in emulated RAM: put it
     * above everything recorded */
    fifoStart = (sMaxAddr + 0xFFFFu) & ~0xFFFFu;
    if (fifoStart + 0x100000u > MEM_SIZE)
        fifoStart = 0x1000; /* below the wasm data segment (GLOBAL_BASE 64 KB) */
    fifoEnd = fifoStart == 0x1000 ? 0xF000 : fifoStart + 0x100000u;

    bpOff = frameList + (uint64_t)nFrames * 64;
    cpOff = bpOff + 0x100 * 4;
    xfOff = cpOff + 0x100 * 4;
    xfrOff = xfOff + 0x1000 * 4;
    texOff = xfrOff + 0x58 * 4;
    pos = texOff + (1 << 20);
    fifoOff = (uint64_t *)calloc(nFrames ? nFrames : 1, 8);
    updOff = (uint64_t *)calloc(nFrames ? nFrames : 1, 8);
    for (i = 0; i < nFrames; i++) {
        fifoOff[i] = pos;
        pos += sFrames[i].fifoLen;
        updOff[i] = pos;
        pos += (uint64_t)sFrames[i].nUpd * 24;
    }
    /* header */
    w32(fp, 0x0d01f1f0);
    w32(fp, 5);
    w32(fp, 5); /* min loader version: RAM override */
    w64(fp, bpOff); w32(fp, 0x100);
    w64(fp, cpOff); w32(fp, 0x100);
    w64(fp, xfOff); w32(fp, 0x1000);
    w64(fp, xfrOff); w32(fp, 0x58);
    w64(fp, frameList); w32(fp, nFrames);
    w32(fp, 0); /* flags: GameCube */
    w64(fp, texOff); w32(fp, 1 << 20);
    w32(fp, MEM_SIZE);
    w32(fp, 0x04000000u); /* MEM2 (unused on GameCube) */
    fwrite(zero, 1, 32, fp);
    /* frame list */
    for (i = 0; i < nFrames; i++) {
        w64(fp, fifoOff[i]);
        w32(fp, sFrames[i].fifoLen);
        w32(fp, fifoStart);
        w32(fp, fifoEnd);
        w64(fp, updOff[i]);
        w32(fp, sFrames[i].nUpd);
        fwrite(zero, 1, 32, fp);
    }
    for (i = 0; i < 0x100; i++) w32(fp, sBp[i]);
    for (i = 0; i < 0x100; i++) w32(fp, sCp[i]);
    for (i = 0; i < 0x1000; i++) w32(fp, sXfMem[i]);
    for (i = 0; i < 0x58; i++) w32(fp, sXfRegs[i]);
    fwrite(sTmemSnap, 1, 1 << 20, fp);
    /* per frame: commands, update list; then the update data */
    for (i = 0; i < nFrames; i++) {
        fwrite(sFrames[i].fifo, 1, sFrames[i].fifoLen, fp);
        for (k = 0; k < sFrames[i].nUpd; k++) {
            const Update *u = &sFrames[i].upd[k];
            w32(fp, u->pos);
            w32(fp, u->addr);
            w64(fp, pos + u->off);
            w32(fp, u->size);
            fputc(0, fp); fputc(0, fp); fputc(0, fp); fputc(0, fp);
        }
    }
    fwrite(sBlob, 1, sBlobLen, fp);
    fclose(fp);
    free(fifoOff);
    free(updOff);
    {
        uint32_t nu = 0, fb = 0;
        for (i = 0; i < nFrames; i++) {
            nu += sFrames[i].nUpd;
            fb += sFrames[i].fifoLen;
        }
        fprintf(stderr, "[dff] wrote %s: %u frames, %u FIFO bytes, %u memory updates (%u bytes), highest address %08x\n",
                sPath, nFrames, fb, nu, sBlobLen, sMaxAddr);
    }
}

/* A top-level command [c, c + len) was executed. */
void gpu_fifolog_cmd(const uint8_t *c, uint32_t len)
{
    Frame *f = &sFrames[sCur];
    f->fifo = (uint8_t *)grow(f->fifo, &f->fifoCap, f->fifoLen + len, 1);
    memcpy(f->fifo + f->fifoLen, c, len);
    f->fifoLen += len;
}

/* Called after every top-level command that contained a copy to the XFB. */
void gpu_fifolog_frame_end(void)
{
    sXfbCopies++;
    if (sXfbCopies % 500 == 0)
        fprintf(stderr, "[dff] XFB copy %u\n", sXfbCopies);
    if (gpu_fifolog_on) {
        sCur++;
        if (sCur == sCount) {
            gpu_fifolog_on = 0;
            gpu_fifolog_wanted = 0;
            sDone = 1;
            write_file();
        }
    }
    if (gpu_fifolog_wanted && !gpu_fifolog_on && !sDone && sXfbCopies == sStart)
        begin();
}

/* The XFB copy index being executed and whether ours should be captured
 * (frames of the recorded range). */
static int sReplayFrame = -1;

void gpu_fifolog_replay_capture(const char *path, int frame)
{
    free(sPath);
    sPath = path ? strdup(path) : NULL;
    sReplayFrame = frame;
}

int gpu_fifolog_capture_index(void)
{
    if (sReplayFrame >= 0)
        return sReplayFrame;
    if (gpu_fifolog_on)
        return (int)(sStart + sCur);
    return -1;
}

const char *gpu_fifolog_path(void) { return sPath; }

void gpu_fifolog_boot(void)
{
    if (gpu_fifolog_wanted && sStart == 0 && !gpu_fifolog_on && !sDone)
        begin();
}
