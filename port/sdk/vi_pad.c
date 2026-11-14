/* VI (video timing) and PAD (controllers). */
#include <stdio.h>
#include <string.h>
#include <emscripten.h>
#include "port_sdk.h"
#include "webgl_backend.h"
#include "gpu.h"

#define RETRACE_TICKS (PORT_TICKS_PER_SEC * 1001 / 60000) /* NTSC 59.94 Hz */

static u32 sRetraceCount;
static OSTime sNextRetrace;
static OSThreadQueue sRetraceQueue;
static VIRetraceCallback sPreCallback, sPostCallback;
static void *sNextFB, *sCurrentFB;
static BOOL sBlack = TRUE;

OSTime port_vi_next_retrace(void)
{
    if (!sNextRetrace)
        sNextRetrace = (OSTime)(emscripten_get_now() * (PORT_TICKS_PER_SEC / 1000)) + RETRACE_TICKS;
    return sNextRetrace;
}

u32 port_vi_retrace_count(void) { return sRetraceCount; }

void port_vi_retrace(void)
{
    OSTime now = (OSTime)(emscripten_get_now() * (PORT_TICKS_PER_SEC / 1000));
    sRetraceCount++;
    if (sRetraceCount % 600 == 0)
    {
        fprintf(stderr, "[vi] %u retraces (%.1fs)\n", sRetraceCount, emscripten_get_now() / 1000.0);
        if (port_env("MKDD_THREADS"))
            port_dump_threads();
    }
    /* catch up without bursting if the host stalled */
    sNextRetrace += RETRACE_TICKS;
    if (sNextRetrace < now)
        sNextRetrace = now + RETRACE_TICKS;
    if (sPreCallback)
        sPreCallback(sRetraceCount);
    sCurrentFB = sNextFB;
    gpu_present(sBlack ? 0 : (u32)(uintptr_t)sCurrentFB);
    OSWakeupThread(&sRetraceQueue);
    if (sPostCallback)
        sPostCallback(sRetraceCount);
}

void VIInit(void)
{
    OSInitThreadQueue(&sRetraceQueue);
    gpu_init();
    {
        const GpuBackend *be = gpu_webgl_create(); /* NULL under node: null/dump backend stays */
        if (be)
            gpu_set_backend(be);
    }
}
void VIConfigure(const GXRenderModeObj *rm) { (void)rm; }
void VIFlush(void) {}
void VISetBlack(BOOL black) { sBlack = black; }
void VISetNextFrameBuffer(void *fb) { sNextFB = fb; }
void *VIGetNextFrameBuffer(void) { return sNextFB; }
void *VIGetCurrentFrameBuffer(void) { return sCurrentFB; }
u32 VIGetDTVStatus(void) { return 0; }

u32 VIGetRetraceCount(void)
{
    port_poll();
    return sRetraceCount;
}

void VIWaitForRetrace(void)
{
    u32 start = sRetraceCount;
    while (sRetraceCount == start)
        OSSleepThread(&sRetraceQueue);
}

VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback cb)
{
    VIRetraceCallback old = sPreCallback;
    sPreCallback = cb;
    return old;
}

VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback cb)
{
    VIRetraceCallback old = sPostCallback;
    sPostCallback = cb;
    return old;
}

/* ---- PAD: port 0 connected with neutral input until a host input layer exists ---- */

static PADStatus sHostPad[4];

/* Host input entry point (called from JS later). */
EMSCRIPTEN_KEEPALIVE void port_pad_set(int chan, u16 button, s8 sx, s8 sy, s8 cx, s8 cy, u8 tl, u8 tr)
{
    PADStatus *s;
    if (chan < 0 || chan >= 4)
        return;
    s = &sHostPad[chan];
    s->button = button;
    s->stickX = sx;
    s->stickY = sy;
    s->substickX = cx;
    s->substickY = cy;
    s->triggerLeft = tl;
    s->triggerRight = tr;
}

BOOL PADInit(void) { return TRUE; }

/* Port 1 holds a standard controller (keyboard/gamepad); 2-4 are empty. */
u32 SIProbe(s32 chan)
{
    return chan == 0 ? SI_GC_CONTROLLER : SI_ERROR_NO_RESPONSE;
}

/* Scripted input for headless runs: MKDD_PAD="900:1000:10,1000:0:10:-80:0" holds
 * PADStatus.button 0x1000 (hex) on port 1 for 10 frames from retrace 900, then
 * the main stick left (optional x:y) for 10 frames from retrace 1000. */
char *gpu_js_env_str(const char *name);
static void pad_script(void)
{
    static char *script;
    static int loaded;
    const char *p;
    u16 button = 0;
    int sx = 0, sy = 0;
    if (!loaded) {
        loaded = 1;
        script = gpu_js_env_str("MKDD_PAD");
    }
    if (!script)
        return;
    for (p = script; *p;) {
        unsigned start = 0, bits = 0, len = 0;
        int x = 0, y = 0;
        if (sscanf(p, "%u:%x:%u:%d:%d", &start, &bits, &len, &x, &y) >= 3 && sRetraceCount >= start &&
            sRetraceCount < start + len) {
            button |= (u16)bits;
            sx += x;
            sy += y;
        }
        while (*p && *p != ',')
            p++;
        if (*p == ',')
            p++;
    }
    sHostPad[0].button = button;
    sHostPad[0].stickX = (s8)sx;
    sHostPad[0].stickY = (s8)sy;
}

u32 PADRead(PADStatus *status)
{
    int i;
    pad_script();
    for (i = 0; i < 4; i++) {
        status[i] = sHostPad[i];
        status[i].err = i == 0 ? PAD_ERR_NONE : PAD_ERR_NO_CONTROLLER;
    }
    return 0;
}

BOOL PADReset(u32 mask) { (void)mask; return TRUE; }
BOOL PADRecalibrate(u32 mask) { (void)mask; return TRUE; }
void PADClamp(PADStatus *status) { (void)status; }
void PADClampCircle(PADStatus *status) { (void)status; }
void PADControlMotor(s32 chan, u32 cmd) { (void)chan; (void)cmd; }
void PADSetSpec(u32 spec) { (void)spec; }
void PADSetAnalogMode(u32 mode) { (void)mode; }
