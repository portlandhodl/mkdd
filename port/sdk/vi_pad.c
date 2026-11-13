/* VI (video timing) and PAD (controllers). */
#include <string.h>
#include <emscripten.h>
#include "port_sdk.h"

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

void port_vi_retrace(void)
{
    OSTime now = (OSTime)(emscripten_get_now() * (PORT_TICKS_PER_SEC / 1000));
    sRetraceCount++;
    /* catch up without bursting if the host stalled */
    sNextRetrace += RETRACE_TICKS;
    if (sNextRetrace < now)
        sNextRetrace = now + RETRACE_TICKS;
    if (sPreCallback)
        sPreCallback(sRetraceCount);
    sCurrentFB = sNextFB;
    OSWakeupThread(&sRetraceQueue);
    if (sPostCallback)
        sPostCallback(sRetraceCount);
}

void VIInit(void) { OSInitThreadQueue(&sRetraceQueue); }
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

u32 PADRead(PADStatus *status)
{
    int i;
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
