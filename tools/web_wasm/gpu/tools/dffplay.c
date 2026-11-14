/* Dolphin FIFO log (.dff) player for the port's GPU (front end + WebGL2 backend).
 *
 *   tools/web_wasm/gpu/tools/dffplay.sh           builds build/web_wasm/dffplay/
 *   node tools/web_wasm/gpu/tools/dffplay.mjs ... renders logs in headless Chrome
 *
 * Plays the same logs that Dolphin's FIFO player plays (and that the game
 * writes with MKDD_DFF, see gpu_fifolog.c), so both renderers draw exactly the
 * same frames. Memory updates are written to their physical addresses in wasm
 * memory: the player's own data starts at 64 MB (GLOBAL_BASE), leaving
 * [4 KB, 64 MB) to the log. RAM holds big-endian data as on the console, so
 * the front end runs with gpu_ram_be. Every XFB copy of a frame is captured as
 * <out>.ours-<frame>.ppm (frame = index in the log). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include "gpu_internal.h"
#include "webgl_backend.h"

/* ---- stand-ins for the SDK the front end talks to ---- */
uint16_t port_hw_pe[0x100];
void port_raise_interrupt(int interrupt) { (void)interrupt; }
EM_JS(int, port_js_env, (const char *name), {
    var v = (typeof Module !== 'undefined' && Module.env) ? Module.env[UTF8ToString(name)] : undefined;
    return v ? (parseInt(v, 10) || 1) : 0;
});
int port_env(const char *name) { return port_js_env(name); }
void port_gx_w8(uint8_t v);

static uint32_t rd32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; } /* file: little-endian */
static uint64_t rd64(const uint8_t *p) { uint64_t v; memcpy(&v, p, 8); return v; }

static void feed(const uint8_t *p, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++)
        port_gx_w8(p[i]);
}

/* Plays frames [first, last] of the log in `d` (`n` bytes); captures go to
 * <out>.ours-<frame>.ppm. Returns the number of frames played, -1 on error. */
EMSCRIPTEN_KEEPALIVE int dffplay(const uint8_t *d, uint32_t n, int first, int last, const char *out)
{
    static int inited;
    uint32_t frames, i, f;
    uint64_t bpOff, cpOff, xfOff, xfrOff, flOff, texOff;
    if (n < 128 || rd32(d) != 0x0d01f1f0) {
        fprintf(stderr, "[dffplay] not a FIFO log\n");
        return -1;
    }
    if (!inited) {
        const GpuBackend *be;
        inited = 1;
        gpu_init();
        be = gpu_webgl_create();
        if (!be) {
            fprintf(stderr, "[dffplay] no WebGL2\n");
            return -1;
        }
        gpu_set_backend(be);
        gpu_ram_be = 1;
    }
    bpOff = rd64(d + 12);
    cpOff = rd64(d + 24);
    xfOff = rd64(d + 36);
    xfrOff = rd64(d + 48);
    flOff = rd64(d + 60);
    frames = rd32(d + 68);
    texOff = rd64(d + 76);
    if (last < 0 || (uint32_t)last >= frames)
        last = (int)frames - 1;

    /* initial state, as Dolphin's FifoPlayer::LoadRegisters / LoadTextureMemory */
    for (i = 0; i < 0x100; i++) {
        uint32_t v = rd32(d + bpOff + i * 4) & 0xFFFFFF;
        gpu_r.bp[i] = v;
        if (i >= 0xE0 && i < 0xE8) {
            if (v & 0x800000)
                gpu_r.tevKonst[i - 0xE0] = v;
            else
                gpu_r.tevColor[i - 0xE0] = v;
        }
    }
    for (i = 0; i < 0x100; i++)
        gpu_r.cp[i] = rd32(d + cpOff + i * 4);
    gpu_vtx_invalidate();
    memcpy(gpu_r.xfmem, d + xfOff, sizeof gpu_r.xfmem);
    for (i = 0; i < 0x58; i++)
        gpu_r.xf[i] = rd32(d + xfrOff + i * 4);
    /* Dolphin keeps TLUTs at (offset << 9), the port at 0x80000 + (offset << 9) */
    memcpy(gpu_tmem() + 0x80000, d + texOff, 0x80000);

    for (f = 0; f <= (uint32_t)last; f++) {
        const uint8_t *fi = d + flOff + f * 64;
        uint64_t fifoOff = rd64(fi), updOff = rd64(fi + 20);
        uint32_t fifoLen = rd32(fi + 8), nUpd = rd32(fi + 28), pos = 0, u;
        const uint8_t *fifo = d + fifoOff;
        char path[512];
        snprintf(path, sizeof path, "%s", out);
        gpu_fifolog_replay_capture(f >= (uint32_t)first ? path : NULL, f >= (uint32_t)first ? (int)f : -1);
        for (u = 0; u < nUpd; u++) {
            const uint8_t *up = d + updOff + u * 24;
            uint32_t upos = rd32(up), addr = rd32(up + 4), size = rd32(up + 16);
            uint64_t data = rd64(up + 8);
            if (upos > pos) {
                feed(fifo + pos, (upos < fifoLen ? upos : fifoLen) - pos);
                pos = upos < fifoLen ? upos : fifoLen;
            }
            if (addr >= 0x1000 && addr + size <= 0x04000000u)
                memcpy((void *)(uintptr_t)addr, d + data, size);
        }
        feed(fifo + pos, fifoLen - pos);
        gpu_present((gpu_r.bp[0x4B] & 0xFFFFFF) << 5);
    }
    gpu_fifolog_replay_capture(NULL, -1);
    return last + 1;
}

int main(void) { return 0; }
