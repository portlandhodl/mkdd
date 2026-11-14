/* VI (video timing) and PAD (controllers). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include <emscripten/heap.h>
#include <malloc.h>
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

/* Widescreen option (MKDD_GFX_WIDESCREEN, include/macros.h DECOMP_WIDE): the game's 3D
 * projections (and with them the view-frustum culling) use 4/3 the aspect ratio, and
 * the renderer shows the 4:3 frame stretched to 16:9. The 2D layout stays 4:3, so the
 * HUD is drawn stretched. Re-read whenever the page changes the graphics settings. */
float port_wide_aspect = 1.0f;
EM_JS(int, port_js_gfx_version, (void), { return (typeof Module !== 'undefined' && Module.gfxVersion) | 0; });
EM_JS(int, port_js_gfx_wide, (void), { return (typeof Module !== 'undefined' && Module.env && parseInt(Module.env.MKDD_GFX_WIDESCREEN, 10) > 0) ? 1 : 0; });
static void update_wide_aspect(void)
{
    static int version = -1;
    int v = port_js_gfx_version();
    if (v == version)
        return;
    version = v;
    port_wide_aspect = port_js_gfx_wide() ? 4.0f / 3.0f : 1.0f;
}

/* ---- frame-time accounting (MKDD_PERF=1) ---- */

int port_perf_enabled;
int gpu_webgl_live_textures(void);
static double sPerf[PERF_NUM], sPerfLast, sPerfWinStart, sPerfMaxBusy, sPerfBusySum;
static u32 sPerfLate, sPerfMissed, sPerfFrames;
static double sPerfGl[4]; /* WebGL backend: draw calls, texture uploads, MB hashed, programs compiled */
static const char *const sPerfNames[PERF_NUM] = {"idle", "present", "dvd-wall", "reads", "idles"};

double port_perf_now(void) { return emscripten_get_now(); }
void port_perf_add(int cat, double ms) { sPerf[cat] += ms; }

static void perf_retrace(double now)
{
    static double idleMark;
    double busy;
    if (!sPerfLast) {
        sPerfLast = sPerfWinStart = now;
        return;
    }
    busy = (now - sPerfLast) - (sPerf[PERF_IDLE] - idleMark);
    idleMark = sPerf[PERF_IDLE];
    sPerfLast = now;
    sPerfFrames++;
    sPerfBusySum += busy;
    {
        const GpuWebglStats *gs = gpu_webgl_stats();
        sPerfGl[0] += gs->drawCalls;
        sPerfGl[1] += gs->texUploads;
        sPerfGl[2] += gs->texHashedBytes / 1048576.0;
        sPerfGl[3] += gs->programsCompiled;
    }
    if (busy > sPerfMaxBusy)
        sPerfMaxBusy = busy;
    if (busy > 1000.0 * 1001 / 60000)
        sPerfLate++;
    if (sPerfFrames == 600) {
        char line[768];
        int i, n;
        n = snprintf(line, sizeof line, "[perf] retrace %u: %.2fs wall/600, busy avg %.2f max %.1f ms, %u over budget, %u dropped |",
                     sRetraceCount, (now - sPerfWinStart) / 1000.0, sPerfBusySum / 600, sPerfMaxBusy, sPerfLate, sPerfMissed);
        for (i = 1; i < PERF_NUM && n < (int)sizeof line; i++)
            n += snprintf(line + n, sizeof line - n, " %s %.2f", sPerfNames[i], sPerf[i] / 600);
        if (sPerfGl[0])
            n += snprintf(line + n, sizeof line - n, " ms/frame | gl %.0f draws, %.1f uploads, %.1f MB hashed /frame, %.0f programs, %d textures",
                          sPerfGl[0] / 600, sPerfGl[1] / 600, sPerfGl[2] / 600, sPerfGl[3], gpu_webgl_live_textures());
        else
            n += snprintf(line + n, sizeof line - n, " ms/frame");
        if (n < (int)sizeof line)
            snprintf(line + n, sizeof line - n, " | heap %u MB, malloc %u MB", (unsigned)(emscripten_get_heap_size() >> 20),
                     (unsigned)(mallinfo().uordblks >> 20));
        fprintf(stderr, "%s\n", line);
        memset(sPerf, 0, sizeof sPerf);
        memset(sPerfGl, 0, sizeof sPerfGl);
        idleMark = 0;
        sPerfFrames = sPerfLate = sPerfMissed = 0;
        sPerfMaxBusy = sPerfBusySum = 0;
        sPerfWinStart = now;
    }
}

void port_vi_retrace(void)
{
    double nowMs = emscripten_get_now();
    OSTime now = (OSTime)(nowMs * (PORT_TICKS_PER_SEC / 1000));
    sRetraceCount++;
    if (port_perf_enabled)
        perf_retrace(nowMs);
    port_report_tick();
    if (sRetraceCount % 600 == 0)
    {
        fprintf(stderr, "[vi] %u retraces (%.1fs)\n", sRetraceCount, emscripten_get_now() / 1000.0);
        if (port_env("MKDD_THREADS"))
            port_dump_threads();
    }
    /* catch up without bursting if the host stalled */
    sNextRetrace += RETRACE_TICKS;
    if (sNextRetrace < now) {
        sPerfMissed += (u32)((now - sNextRetrace) / RETRACE_TICKS) + 1;
        sNextRetrace = now + RETRACE_TICKS;
    }
    update_wide_aspect();
    if (sPreCallback)
        sPreCallback(sRetraceCount);
    sCurrentFB = sNextFB;
    if (port_perf_enabled) {
        double t0 = emscripten_get_now();
        gpu_present(sBlack ? 0 : (u32)(uintptr_t)sCurrentFB);
        sPerf[PERF_PRESENT] += emscripten_get_now() - t0;
    } else {
        gpu_present(sBlack ? 0 : (u32)(uintptr_t)sCurrentFB);
    }
    OSWakeupThread(&sRetraceQueue);
    if (sPostCallback)
        sPostCallback(sRetraceCount);
}

void VIInit(void)
{
    port_perf_enabled = port_env("MKDD_PERF");
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

/* Port 1 holds a standard controller (keyboard/gamepad); 2-4 are empty, except in the online menus
 * (net/netsync.cpp), where port i+1 is the controller of the room's player i. */
int port_netsync_ports(void);
int port_netsync_pad(PADStatus *status);
u32 SIProbe(s32 chan)
{
    int n = port_netsync_ports();
    return chan == 0 || chan < n ? SI_GC_CONTROLLER : SI_ERROR_NO_RESPONSE;
}

/* Scripted input for headless runs: MKDD_PAD="900:1000:10,1000:0:10:-80:0" holds
 * PADStatus.button 0x1000 (hex) on port 1 for 10 frames from retrace 900, then
 * the main stick left (optional x:y) for 10 frames from retrace 1000. */
char *gpu_js_env_str(const char *name);
void port_autopilot(void);
typedef struct PadStep {
    unsigned start, len;
    u16 bits;
    s8 x, y;
} PadStep;

static void pad_script(void)
{
    static PadStep *steps;
    static int nsteps, loaded;
    u16 button = 0;
    int sx = 0, sy = 0, i;
    if (!loaded) {
        char *script = gpu_js_env_str("MKDD_PAD");
        const char *p;
        loaded = 1;
        if (!script)
            return;
        for (p = script; *p;) { /* parsed once: scripts can hold hundreds of steps */
            unsigned start = 0, bits = 0, len = 0;
            int x = 0, y = 0;
            if (sscanf(p, "%u:%x:%u:%d:%d", &start, &bits, &len, &x, &y) >= 3) {
                steps = (PadStep *)realloc(steps, (nsteps + 1) * sizeof *steps);
                steps[nsteps].start = start;
                steps[nsteps].len = len;
                steps[nsteps].bits = (u16)bits;
                steps[nsteps].x = (s8)x;
                steps[nsteps].y = (s8)y;
                nsteps++;
            }
            while (*p && *p != ',')
                p++;
            if (*p == ',')
                p++;
        }
        free(script);
        loaded = 2;
    }
    if (loaded != 2)
        return;
    for (i = 0; i < nsteps; i++) {
        if (sRetraceCount >= steps[i].start && sRetraceCount < steps[i].start + steps[i].len) {
            button |= steps[i].bits;
            sx += steps[i].x;
            sy += steps[i].y;
        }
    }
    sHostPad[0].button = button;
    sHostPad[0].stickX = (s8)sx;
    sHostPad[0].stickY = (s8)sy;
}

u32 PADRead(PADStatus *status)
{
    int i;
    pad_script();
    port_autopilot();
    for (i = 0; i < 4; i++) {
        status[i] = sHostPad[i];
        status[i].err = i == 0 ? PAD_ERR_NONE : PAD_ERR_NO_CONTROLLER;
    }
    port_netsync_pad(status);
    return 0;
}

BOOL PADReset(u32 mask) { (void)mask; return TRUE; }
BOOL PADRecalibrate(u32 mask) { (void)mask; return TRUE; }
void PADClamp(PADStatus *status) { (void)status; }
void PADClampCircle(PADStatus *status) { (void)status; }
void PADControlMotor(s32 chan, u32 cmd) { (void)chan; (void)cmd; }
void PADSetSpec(u32 spec) { (void)spec; }
void PADSetAnalogMode(u32 mode) { (void)mode; }
