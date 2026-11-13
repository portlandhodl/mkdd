/* Emulated GameCube GPU (Flipper) for the host port: the contract between the
 * front end (command processor, vertex loader, transform/lighting/texgen) and a
 * rendering backend (WebGL2 in the browser, null under node).
 *
 *   GX library -> port_gx_w* (big-endian FIFO bytes) -> front end -> backend
 *
 * Memory model: physical address == host pointer. Data in main RAM is host
 * byte order, EXCEPT display lists, texture images and TLUT (palette) data,
 * which are big-endian exactly as on hardware. The FIFO stream is big-endian.
 *
 * Division of work:
 *  - front end: parses the stream (incl. display lists), keeps CP/XF/BP register
 *    state, decodes vertices (direct data BE, indexed array data host order,
 *    indexed matrix loads from RAM host-order floats), runs transform, lighting
 *    and texture-coordinate generation in software, converts primitives to
 *    triangle/line/point lists, handles TLUT loads into TMEM, PE tokens and
 *    draw-done interrupts (port_raise_interrupt), and forwards draws, EFB copies
 *    and clears to the backend.
 *  - backend: rasterisation state from BP registers (TEV stages -> shaders,
 *    textures via texdec.h, alpha test, blending/logic op, depth, fog, scissor,
 *    culling already applied or passed), the EFB render target, EFB copies to
 *    textures and to the XFB (display), and presenting frames.
 */
#ifndef PORT_GPU_H
#define PORT_GPU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- register state (raw, as the hardware holds it) ---- */

typedef struct GpuRegs {
    uint32_t bp[0x100];     /* BP registers, 24-bit values (TEV, PE, texture, copy, scissor...) */
    uint32_t cp[0x100];     /* CP registers (VCD, VAT, array bases/strides, matrix indices) */
    uint32_t xf[0x58];      /* XF registers 0x1000.. (viewport, projection, texgen, channel ctrl...) */
    float xfmem[0x800];     /* XF memory: pos/tex matrices 0x000-0x3FF, normal 0x400-0x45F,
                               post-tex 0x500-0x5FF, lights 0x600-0x67F (as floats/raw bits) */
    /* BP 0xE0-0xE7 hold two register files at the same addresses: writes with
     * bit 23 clear set TEV color registers (GXSetTevColor), writes with bit 23
     * set the konst colors (GXSetTevKColor). bp[] keeps the last raw write;
     * these keep each file separately (24-bit values, as written). */
    uint32_t tevColor[8];
    uint32_t tevKonst[8];
} GpuRegs;

/* ---- transformed vertices handed to the backend ---- */

#define GPU_MAX_TEXCOORDS 8

typedef struct GpuVertex {
    float pos[4];                      /* clip space (x, y, z, w), GL conventions: z in [-w, w].
                                          GX clip z (near -w .. far 0) is remapped as z_gl = 2 z_gx + w,
                                          so near -> -1 and far -> +1 after the divide. x/y are GX clip
                                          x/y unchanged (y up). The window transform is the backend's:
                                          XF 0x101A-0x101F (scale x/y/z, offset x/y/z as floats) give
                                          x_win = offX - 342 + scaleX * x_ndc, y_win (down) =
                                          offY - 342 + scaleY * y_ndc (scaleY < 0), and the depth range
                                          near = (offZ - scaleZ) / 16777215, far = offZ / 16777215. */
    float color[2][4];                 /* lit color channels 0/1, RGBA in [0,1] */
    float tex[GPU_MAX_TEXCOORDS][3];   /* generated texcoords (s, t, q) before the perspective divide;
                                          not yet scaled by texture size (normalised GX coordinates) */
} GpuVertex;

enum GpuPrim { GPU_TRIANGLES = 0, GPU_LINES = 1, GPU_POINTS = 2 };

/* ---- backend interface ---- */

typedef struct GpuBackend {
    /* A batch of primitives. `regs` is the full current state; the front end has
     * already applied back-face culling (GEN_MODE) for triangles. */
    void (*draw)(void *ctx, const GpuRegs *regs, int prim, const GpuVertex *v, uint32_t count);
    /* EFB copy triggered by BP 0x52 (PE_COPY_EXECUTE), with the BP copy registers
     * in `regs` (0x49 src, 0x4A size, 0x4B dest addr, 0x4D stride, 0x4E y scale,
     * 0x4F/0x50 clear color, 0x51 clear z). `toXfb` = copy to the external
     * framebuffer (display) rather than to a texture; `clear` = clear the EFB
     * region afterwards. */
    void (*efb_copy)(void *ctx, const GpuRegs *regs, int toXfb, int clear);
    /* The video interface switched to the XFB at `addr` (VISetNextFrameBuffer +
     * retrace): present the most recent copy made to that address. */
    void (*present)(void *ctx, uint32_t xfbAddr); /* 0 = the display is blanked (VISetBlack) */
    void *ctx;
} GpuBackend;

/* ---- front end API ---- */

void gpu_init(void);
void gpu_set_backend(const GpuBackend *backend); /* NULL = null backend (counts only) */
const GpuRegs *gpu_regs(void);
uint8_t *gpu_tmem(void); /* 1 MB texture memory, indexed by TMEM address. A TLUT load (BP 0x64
                            source, BP 0x65) lands at 0x80000 + ((bp65 & 0x3FF) << 9), so a texture's
                            palette (BP 0x98-0x9B / 0xB8-0xBB, offset in bits 0-9) is at
                            0x80000 + ((reg & 0x3FF) << 9); entries are big-endian u16. */
void gpu_present(uint32_t xfbAddr); /* called by the VI layer at every retrace (0 when blanked) */

/* statistics for the current and previous frame (headless validation) */
typedef struct GpuStats {
    uint32_t fifoBytes, commands, displayLists, draws, vertices, efbCopies, unknownOpcodes;
} GpuStats;
const GpuStats *gpu_stats(int previousFrame);

#ifdef __cplusplus
}
#endif

#endif
