/* GPU front end, part 1: the write-gather pipe sink and the command processor.
 *
 * Bytes arrive one store at a time, big-endian, exactly as the hardware FIFO
 * receives them. They are gathered in a buffer; whenever enough bytes for the
 * pending command have arrived, every complete command is executed at once, so
 * the GPU is always idle by the time the CPU looks at it again (a BP draw-done
 * or token write takes effect before the store that carried it returns).
 *
 * Commands handled: NOP, LOAD_CP_REG, LOAD_XF_REG, LOAD_INDX_A-D, CALL_DL,
 * INVALIDATE_VTX_CACHE, LOAD_BP_REG and the draw commands. Display lists run
 * from RAM through the same decoder. BP side effects: the mask register,
 * PE draw done / tokens (interrupts), TLUT loads into TMEM, EFB copies. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dolphin/types.h>
#include "port_sdk.h"
#include "port_hw.h"
#include "port_gxfifo.h"
#include "gpu_internal.h"

GpuRegs gpu_r;
GpuStats gpu_cur;
static GpuStats sPrev;
int gpu_xform_enabled;

static uint8_t sTmem[1 << 20];
static uint32_t sBpMask = 0xFFFFFF;
static int sInited;
static int sStatsEnv;
static uint32_t sPresents;
static uint32_t sUnknownTotal;
static int sXfbCopied; /* the command being executed copied to the XFB (FIFO log frames) */

/* ---- backends ---- */

static void null_draw(void *ctx, const GpuRegs *regs, int prim, const GpuVertex *v, uint32_t count)
{
    (void)ctx; (void)regs; (void)prim; (void)v; (void)count;
}
static void null_copy(void *ctx, const GpuRegs *regs, int toXfb, int clear)
{
    (void)ctx; (void)regs; (void)toXfb; (void)clear;
}
static void null_present(void *ctx, uint32_t xfbAddr) { (void)ctx; (void)xfbAddr; }

static const GpuBackend sNullBackend = {null_draw, null_copy, null_present, NULL};
const GpuBackend *gpu_backend = &sNullBackend;

void gpu_init(void)
{
    const GpuBackend *dump;
    if (sInited)
        return;
    sInited = 1;
    sStatsEnv = port_env("MKDD_GPU_STATS");
    if (port_env("MKDD_GPU_XFORM")) /* profile the front end with the null backend */
        gpu_xform_enabled = 1;
    if (gpu_backend == &sNullBackend && (dump = gpu_dump_backend()) != NULL) {
        gpu_backend = dump;
        gpu_xform_enabled = 1;
    }
    gpu_fifolog_init();
    gpu_fifolog_boot();
}

void gpu_set_backend(const GpuBackend *backend)
{
    gpu_backend = backend ? backend : &sNullBackend;
    gpu_xform_enabled = backend != NULL;
}

const GpuRegs *gpu_regs(void) { return &gpu_r; }
uint8_t *gpu_tmem(void) { return sTmem; }

const GpuStats *gpu_stats(int previousFrame) { return previousFrame ? &sPrev : &gpu_cur; }

int gpu_ram_ok(uint32_t addr, uint32_t size)
{
#ifdef __wasm__
    uint32_t mem = (uint32_t)__builtin_wasm_memory_size(0) * 65536u;
    return addr >= 0x1000 && addr <= mem && size <= mem - addr;
#else
    (void)size;
    return addr != 0;
#endif
}

void gpu_present(uint32_t xfbAddr)
{
    if (!sInited)
        gpu_init();
    gpu_backend->present(gpu_backend->ctx, xfbAddr);
    sPrev = gpu_cur;
    sUnknownTotal += gpu_cur.unknownOpcodes;
    memset(&gpu_cur, 0, sizeof gpu_cur);
    sPresents++;
    if (sStatsEnv && sPresents % 600 == 0)
        fprintf(stderr, "[gpu] frame %u: fifo %u B, %u cmds, %u DLs, %u draws, %u verts, %u copies, %u unknown (%u total)\n",
                sPresents, sPrev.fifoBytes, sPrev.commands, sPrev.displayLists, sPrev.draws, sPrev.vertices,
                sPrev.efbCopies, sPrev.unknownOpcodes, sUnknownTotal);
}

/* ---- register writes ---- */

static void cp_write(uint32_t reg, uint32_t v)
{
    reg &= 0xFF;
    gpu_r.cp[reg] = v;
    if ((reg >= 0x50 && reg < 0x70) || (reg >= 0x70 && reg < 0xA0))
        gpu_vtx_invalidate();
}

static void xf_write(uint32_t addr, uint32_t v)
{
    if (addr < 0x800)
        memcpy(&gpu_r.xfmem[addr], &v, 4);
    else if (addr >= 0x1000 && addr < 0x1000 + sizeof gpu_r.xf / sizeof gpu_r.xf[0])
        gpu_r.xf[addr - 0x1000] = v;
}

static void tlut_load(uint32_t v)
{
    uint32_t src = (gpu_r.bp[0x64] & 0x1FFFFF) << 5;
    uint32_t dst = 0x80000 + ((v & 0x3FF) << 9);
    uint32_t bytes = ((v >> 10) & 0x7FF) * 32;
    if (dst >= sizeof sTmem)
        return;
    if (bytes > sizeof sTmem - dst)
        bytes = sizeof sTmem - dst;
    if (bytes && gpu_ram_ok(src, bytes)) {
        if (gpu_fifolog_on)
            gpu_fifolog_tlut(src, bytes);
        memcpy(sTmem + dst, (const void *)(uintptr_t)src, bytes);
    }
}

static void bp_write(uint32_t word)
{
    uint32_t reg = word >> 24, v = word & 0xFFFFFF;
    if (reg == 0xFE) {
        gpu_r.bp[0xFE] = v;
        sBpMask = v;
        return;
    }
    v = (gpu_r.bp[reg] & ~sBpMask) | (v & sBpMask);
    sBpMask = 0xFFFFFF;
    gpu_r.bp[reg] = v;
    switch (reg) {
    case 0x45: /* PE_DONE: draw done */
        if ((v & 2) && (port_hw_pe[5] & 2)) {
            port_hw_pe[5] |= 8;
            port_raise_interrupt(__OS_INTERRUPT_PI_PE_FINISH);
        }
        break;
    case 0x47: /* PE_TOKEN */
        port_hw_pe[7] = (u16)v;
        break;
    case 0x48: /* PE_TOKEN_INT */
        port_hw_pe[7] = (u16)v;
        if (port_hw_pe[5] & 1) {
            port_hw_pe[5] |= 4;
            port_raise_interrupt(__OS_INTERRUPT_PI_PE_TOKEN);
        }
        break;
    case 0x52: /* PE_COPY_EXECUTE */
        gpu_cur.efbCopies++;
        if (gpu_fifolog_wanted || gpu_fifolog_on) {
            gpu_fifolog_efb_copy(&gpu_r, (v >> 14) & 1);
            sXfbCopied |= (v >> 14) & 1;
        }
        gpu_backend->efb_copy(gpu_backend->ctx, &gpu_r, (v >> 14) & 1, (v >> 11) & 1);
        break;
    case 0x65: /* TLUT load */
        tlut_load(v);
        break;
    default:
        if (reg >= 0xE0 && reg < 0xE8) { /* TEV color registers / konst colors share addresses */
            if (v & 0x800000)
                gpu_r.tevKonst[reg - 0xE0] = v;
            else
                gpu_r.tevColor[reg - 0xE0] = v;
        }
        break;
    }
}

static void load_indexed(int array, uint32_t v)
{
    uint32_t addr = v & 0xFFF, n = ((v >> 12) & 0xF) + 1, index = v >> 16;
    uint32_t src = gpu_r.cp[0xA0 + array] + index * gpu_r.cp[0xB0 + array];
    uint32_t i;
    if (!gpu_ram_ok(src, n * 4))
        return;
    if (gpu_fifolog_on)
        gpu_fifolog_indexed(src, n * 4);
    for (i = 0; i < n; i++) {
        uint32_t w;
        if (gpu_ram_be)
            w = gpu_be32((const uint8_t *)(uintptr_t)src + i * 4);
        else
            memcpy(&w, (const uint8_t *)(uintptr_t)src + i * 4, 4); /* RAM: host byte order */
        xf_write(addr + i, w);
    }
}

/* ---- command decoder ---- */

/* Executes the complete commands in [p, p + n) and returns the bytes consumed.
 * If a command is cut off, *need is set to the bytes it needs in total. */
static uint32_t run(const uint8_t *p, uint32_t n, uint32_t *need, int depth)
{
    uint32_t i = 0, len = 1;
    while (i < n) {
        const uint8_t *c = p + i;
        uint32_t left = n - i;
        uint8_t op = c[0];
        switch (op) {
        case 0x00: /* NOP */
            len = 1;
            break;
        case 0x08: /* LOAD_CP_REG */
            len = 6;
            if (left < len)
                goto partial;
            cp_write(c[1], gpu_be32(c + 2));
            break;
        case 0x10: { /* LOAD_XF_REG */
            uint32_t h, cnt, k;
            len = 5;
            if (left < len)
                goto partial;
            h = gpu_be32(c + 1);
            cnt = ((h >> 16) & 0xF) + 1;
            len = 5 + cnt * 4;
            if (left < len)
                goto partial;
            for (k = 0; k < cnt; k++)
                xf_write((h & 0xFFFF) + k, gpu_be32(c + 5 + k * 4));
            break;
        }
        case 0x20: case 0x28: case 0x30: case 0x38: /* LOAD_INDX_A..D */
            len = 5;
            if (left < len)
                goto partial;
            load_indexed(12 + ((op - 0x20) >> 3), gpu_be32(c + 1));
            break;
        case 0x40: { /* CALL_DL */
            uint32_t addr, size, dlNeed, used;
            len = 9;
            if (left < len)
                goto partial;
            addr = gpu_be32(c + 1);
            size = gpu_be32(c + 5);
            gpu_cur.displayLists++;
            if (depth < 8 && size && gpu_ram_ok(addr, size)) {
                if (gpu_fifolog_on)
                    gpu_fifolog_dl(addr, size);
                used = run((const uint8_t *)(uintptr_t)addr, size, &dlNeed, depth + 1);
                if (used != size)
                    gpu_cur.unknownOpcodes++; /* display list ends inside a command */
            } else {
                gpu_cur.unknownOpcodes++;
            }
            break;
        }
        case 0x48: /* INVALIDATE_VTX_CACHE */
            len = 1;
            break;
        case 0x61: /* LOAD_BP_REG */
            len = 5;
            if (left < len)
                goto partial;
            bp_write(gpu_be32(c + 1));
            break;
        default:
            if (op >= 0x80 && op < 0xC0) { /* draw: prim in bits 3-5, vertex format in bits 0-2 */
                uint32_t cnt, vsize;
                len = 3;
                if (left < len)
                    goto partial;
                cnt = gpu_be16(c + 1);
                vsize = gpu_vtx_size(op & 7);
                len = 3 + cnt * vsize;
                if (left < len)
                    goto partial;
                gpu_cur.draws++;
                gpu_cur.vertices += cnt;
                if (cnt && gpu_xform_enabled) {
                    if (gpu_fifolog_on)
                        gpu_fifolog_draw_begin();
                    gpu_vtx_draw((op >> 3) & 7, op & 7, cnt, c + 3);
                    if (gpu_fifolog_on)
                        gpu_fifolog_draw_end();
                }
            } else {
                gpu_cur.unknownOpcodes++; /* desync: skip a byte and try again */
                len = 1;
            }
            break;
        }
        gpu_cur.commands += op != 0;
        if (depth == 0 && (gpu_fifolog_on || sXfbCopied)) {
            if (gpu_fifolog_on)
                gpu_fifolog_cmd(c, len);
            if (sXfbCopied) {
                sXfbCopied = 0;
                gpu_fifolog_frame_end();
            }
        }
        i += len;
    }
    *need = 1;
    return i;
partial:
    *need = len;
    return i;
}

/* ---- the FIFO ---- */

static uint8_t *sBuf;
static uint32_t sCap, sRd, sWr, sNeed = 1;

static void pump(void)
{
    uint32_t need;
    sRd += run(sBuf + sRd, sWr - sRd, &need, 0);
    if (sRd == sWr) {
        sRd = sWr = 0;
        sNeed = 1;
    } else {
        sNeed = need;
    }
}

static void make_room(void)
{
    if (sRd) {
        memmove(sBuf, sBuf + sRd, sWr - sRd);
        sWr -= sRd;
        sRd = 0;
    }
    if (sWr == sCap || sNeed > sCap) {
        uint32_t cap = sCap ? sCap * 2 : 1 << 16;
        while (cap < sNeed)
            cap *= 2;
        sBuf = (uint8_t *)realloc(sBuf, cap);
        if (!sBuf) {
            fprintf(stderr, "[gpu] out of memory for the FIFO\n");
            abort();
        }
        sCap = cap;
    }
}

static inline void fifo_put(u8 v)
{
    if (sWr == sCap)
        make_room();
    sBuf[sWr++] = v;
    if (sWr - sRd >= sNeed)
        pump();
}

void port_gx_w8(u8 v)
{
    gpu_cur.fifoBytes += 1;
    fifo_put(v);
}

void port_gx_w16(u16 v)
{
    gpu_cur.fifoBytes += 2;
    fifo_put((u8)(v >> 8));
    fifo_put((u8)v);
}

void port_gx_w32(u32 v)
{
    gpu_cur.fifoBytes += 4;
    fifo_put((u8)(v >> 24));
    fifo_put((u8)(v >> 16));
    fifo_put((u8)(v >> 8));
    fifo_put((u8)v);
}

void port_gx_wf32(f32 v)
{
    u32 u;
    memcpy(&u, &v, 4);
    port_gx_w32(u);
}

/* C++ code refers to the pipe object; it carries no state. */
char port_gx_wgpipe[8];
