/* GX, headless: state calls are no-ops (stubs_auto.c); this file holds the parts
 * whose results the game depends on (FIFO objects, render modes, size queries)
 * and the write-gather pipe sink. A WebGL2 backend replaces the sink later. */
#include <string.h>
#include "port_sdk.h"
#include "port_gxfifo.h"

/* ---- write-gather pipe: big-endian bytes, as the hardware FIFO receives them ---- */

static u8 sFifo[1 << 20];
static u32 sFifoPos;

static inline void fifo_put(u8 v)
{
    sFifo[sFifoPos++ & (sizeof sFifo - 1)] = v;
}

void port_gx_w8(u8 v) { fifo_put(v); }
void port_gx_w16(u16 v) { fifo_put((u8)(v >> 8)); fifo_put((u8)v); }
void port_gx_w32(u32 v) { port_gx_w16((u16)(v >> 16)); port_gx_w16((u16)v); }
void port_gx_wf32(f32 v)
{
    u32 u;
    memcpy(&u, &v, 4);
    port_gx_w32(u);
}

/* C++ code refers to the pipe object; it carries no state. */
char port_gx_wgpipe[8];

/* ---- FIFO objects ---- */

static GXFifoObj sFifoObj;
static GXDrawDoneCallback sDrawDone;

GXFifoObj *GXInit(void *base, u32 size)
{
    (void)base;
    (void)size;
    return &sFifoObj;
}

void GXInitFifoBase(GXFifoObj *fifo, void *base, u32 size) { (void)fifo; (void)base; (void)size; }
void GXInitFifoPtrs(GXFifoObj *fifo, void *readPtr, void *writePtr) { (void)fifo; (void)readPtr; (void)writePtr; }
void GXSaveCPUFifo(GXFifoObj *fifo) { (void)fifo; }

void GXGetGPStatus(GXBool *overhi, GXBool *underlow, GXBool *readIdle, GXBool *cmdIdle, GXBool *brkpt)
{
    *overhi = GX_FALSE;
    *underlow = GX_TRUE;
    *readIdle = GX_TRUE;
    *cmdIdle = GX_TRUE;
    *brkpt = GX_FALSE;
}

GXDrawDoneCallback GXSetDrawDoneCallback(GXDrawDoneCallback cb)
{
    GXDrawDoneCallback old = sDrawDone;
    sDrawDone = cb;
    return old;
}

static void draw_done(void *a, void *b)
{
    (void)a;
    (void)b;
    if (sDrawDone)
        sDrawDone();
}

/* GXSetDrawDone: the GPU finishes immediately; the callback arrives as an interrupt. */
void GXSetDrawDone(void) { port_defer(draw_done, NULL, NULL); }
void GXDrawDone(void) {}
void GXFlush(void) { sFifoPos = 0; }
void GXAbortFrame(void) { sFifoPos = 0; }

/* ---- render modes (values from the SDK) ---- */

GXRenderModeObj GXNtsc480IntDf = {
    VI_TVMODE_NTSC_INT, 640, 480, 480, 40, 0, 640, 480, VI_XFBMODE_DF, 0, 0,
    {{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}},
    {8, 8, 10, 12, 10, 8, 8},
};

GXRenderModeObj GXNtsc480Int = {
    VI_TVMODE_NTSC_INT, 640, 480, 480, 40, 0, 640, 480, VI_XFBMODE_DF, 0, 0,
    {{6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}},
    {0, 0, 21, 22, 21, 0, 0},
};

/* ---- size queries ---- */

f32 GXGetYScaleFactor(u16 efbHeight, u16 xfbHeight) { return (f32)xfbHeight / (f32)efbHeight; }

u16 GXGetNumXfbLines(u16 efbHeight, f32 yScale)
{
    u32 lines = (u32)((f32)(efbHeight - 1) * yScale) + 1;
    return (u16)lines;
}

static u32 sScissor[4] = {0, 0, 640, 480};

void GXSetScissor(u32 x, u32 y, u32 w, u32 h)
{
    sScissor[0] = x;
    sScissor[1] = y;
    sScissor[2] = w;
    sScissor[3] = h;
}

void GXGetScissor(u32 *x, u32 *y, u32 *w, u32 *h)
{
    *x = sScissor[0];
    *y = sScissor[1];
    *w = sScissor[2];
    *h = sScissor[3];
}

/* Texture size in bytes: formats are stored in 32-byte tiles. */
u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 max_lod)
{
    u32 tileW, tileH, tileBytes = 32, size = 0, lod, levels;
    switch (format & 0x3F) {
    case GX_TF_I4:
    case GX_TF_CMPR:
    case GX_CTF_R4:
    case GX_CTF_Z4:
        tileW = 8; tileH = 8;
        break;
    case GX_TF_I8:
    case GX_TF_IA4:
    case GX_CTF_RA4:
    case GX_CTF_A8:
    case GX_CTF_R8:
    case GX_CTF_G8:
    case GX_CTF_B8:
    case GX_TF_Z8:
    case GX_CTF_Z8M:
    case GX_CTF_Z8L:
        tileW = 8; tileH = 4;
        break;
    case GX_TF_RGBA8:
    case GX_TF_Z24X8:
        tileW = 4; tileH = 4; tileBytes = 64;
        break;
    default:
        tileW = 4; tileH = 4;
        break;
    }
    levels = mipmap ? max_lod : 1;
    for (lod = 0; lod < levels; lod++) {
        size += ((width + tileW - 1) / tileW) * ((height + tileH - 1) / tileH) * tileBytes;
        if (width == 1 && height == 1)
            break;
        width = width > 1 ? width >> 1 : 1;
        height = height > 1 ? height >> 1 : 1;
    }
    return size;
}
