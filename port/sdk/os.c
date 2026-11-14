/* OS: arena, time, interrupts, reporting and misc system queries. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <emscripten.h>
#include "port_sdk.h"

#define PORT_MEM_SIZE 0x01800000u   /* retail console */
#define PORT_ARENA_SIZE 0x03000000u /* main RAM left after the DOL; generous for the host */

volatile int __OSTVMode = VI_NTSC;
u32 __OSBusClock = 162000000;
u32 __OSCoreClock = 486000000;
u32 __OSFpscrEnableBits;

static void *sArenaLo, *sArenaHi;
static BOOL sInterruptsEnabled = TRUE;
static u32 sProgressive, sSoundMode = 1;

void port_dump_threads(void);
void port_load_dol_data(void);

void port_stub(const char *name)
{
    static const char *seen[512];
    static int nseen;
    int i;
    for (i = 0; i < nseen; i++)
        if (seen[i] == name)
            return;
    if (nseen < 512)
        seen[nseen++] = name;
    fprintf(stderr, "[sdk] stub: %s\n", name);
}

void OSInit(void)
{
    static BOOL done;
    if (done)
        return;
    done = TRUE;
    /* OSLoMem: physical memory size at 0x28, console type at 0x2C */
    *(volatile u32 *)0x28 = PORT_MEM_SIZE;
    *(volatile u32 *)0x2C = 0x10000006; /* development hardware, matches a debug build */
    sArenaLo = memalign(32, PORT_ARENA_SIZE);
    sArenaHi = (u8 *)sArenaLo + PORT_ARENA_SIZE;
    port_os_init();
    port_load_dol_data();
    if (port_env("MKDD_HEAPCHECK"))
        EM_ASM({
            if (typeof process === 'undefined')
                return;
            var dump = function(e) {
                console.error(e && e.stack ? e.stack : e);
                try { wasmExports['port_heap_dump'](); } catch (e2) { console.error('heap dump failed', e2); }
                process.exit(7);
            };
            process.on('uncaughtException', dump);
            process.on('unhandledRejection', dump);
        });
}

void *OSGetArenaLo(void) { OSInit(); return sArenaLo; }
void *OSGetArenaHi(void) { OSInit(); return sArenaHi; }
void OSSetArenaLo(void *lo) { sArenaLo = lo; }
void OSSetArenaHi(void *hi) { sArenaHi = hi; }
void *OSInitAlloc(void *lo, void *hi, int maxHeaps) { (void)hi; (void)maxHeaps; return lo; }
u32 OSGetConsoleSimulatedMemSize(void) { return PORT_MEM_SIZE; }

OSTime OSGetTime(void)
{
    port_poll();
    return (OSTime)(emscripten_get_now() * (PORT_TICKS_PER_SEC / 1000));
}

OSTick OSGetTick(void) { return (OSTick)OSGetTime(); }

void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime *td)
{
    time_t secs = time(NULL);
    struct tm *t = localtime(&secs);
    OSTime sub = ticks % PORT_TICKS_PER_SEC;
    td->sec = t->tm_sec;
    td->min = t->tm_min;
    td->hour = t->tm_hour;
    td->mday = t->tm_mday;
    td->mon = t->tm_mon;
    td->year = t->tm_year + 1900;
    td->wday = t->tm_wday;
    td->yday = t->tm_yday;
    td->msec = (int)(sub / (PORT_TICKS_PER_SEC / 1000));
    td->usec = (int)(sub / (PORT_TICKS_PER_SEC / 1000000) % 1000);
}

BOOL OSDisableInterrupts(void)
{
    BOOL old = sInterruptsEnabled;
    sInterruptsEnabled = FALSE;
    return old;
}

int port_interrupts_enabled(void) { return sInterruptsEnabled; }
void port_interrupts_reenabled(void);

BOOL OSEnableInterrupts(void)
{
    BOOL old = sInterruptsEnabled;
    sInterruptsEnabled = TRUE;
    if (!old)
        port_interrupts_reenabled();
    return old;
}

BOOL OSRestoreInterrupts(BOOL level)
{
    BOOL old = sInterruptsEnabled;
    sInterruptsEnabled = level;
    if (level && !old)
        port_interrupts_reenabled();
    return old;
}

/* OSReport output is assembled into whole lines (the game reports one line in
 * several calls, e.g. JUTAssertion's "WARNING [file:line] " + text + "\n"), and
 * repeats are folded: a game warning can fire every frame, and each line that
 * reaches the browser's log costs far more than the frame budget allows. A line
 * identical to the previous one is counted, not printed; past REPORT_BURST lines
 * within a second the rest are counted and summarised once per second. */
#define REPORT_BURST 100
static char sReportLine[512], sReportLast[512];
static int sReportLen;
static u32 sReportRepeat, sReportSuppressed, sReportInSecond;
static double sReportSecond;

static void report_summary(void)
{
    if (sReportRepeat)
        printf("  (previous line repeated %u more time%s)\n", sReportRepeat, sReportRepeat > 1 ? "s" : "");
    if (sReportSuppressed)
        printf("  (%u more report lines not shown)\n", sReportSuppressed);
    if (sReportRepeat || sReportSuppressed)
        fflush(stdout);
    sReportRepeat = sReportSuppressed = 0;
}

/* Called once per retrace: report what was folded since the last line. */
void port_report_tick(void)
{
    double now = emscripten_get_now();
    if (now - sReportSecond >= 1000.0) {
        report_summary();
        sReportSecond = now;
        sReportInSecond = 0;
    }
}

static void report_line(void)
{
    sReportLine[sReportLen] = 0;
    if (!strcmp(sReportLine, sReportLast)) {
        sReportRepeat++;
    } else if (sReportInSecond >= REPORT_BURST) {
        sReportSuppressed++;
        sReportLast[0] = 0;
    } else {
        report_summary();
        sReportInSecond++;
        printf("%s\n", sReportLine);
        fflush(stdout);
        memcpy(sReportLast, sReportLine, (size_t)sReportLen + 1);
    }
    sReportLen = 0;
}

void OSReport(const char *msg, ...)
{
    char buf[1024];
    const char *p;
    va_list ap;
    va_start(ap, msg);
    vsnprintf(buf, sizeof buf, msg, ap);
    va_end(ap);
    for (p = buf; *p; p++) {
        if (*p == '\n')
            report_line();
        else if (sReportLen < (int)sizeof sReportLine - 1)
            sReportLine[sReportLen++] = *p;
    }
}

void OSPanic(const char *file, int line, const char *msg, ...)
{
    va_list ap;
    fprintf(stderr, "[OSPanic] %s:%d: ", file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fputc('\n', stderr);
    port_dump_threads();
    abort();
}

OSErrorHandler OSSetErrorHandler(OSError error, OSErrorHandler handler)
{
    (void)error;
    (void)handler;
    return NULL;
}

void OSResetSystem(int reset, u32 resetCode, BOOL forceMenu)
{
    fprintf(stderr, "[sdk] OSResetSystem(%d, 0x%x, %d)\n", reset, resetCode, forceMenu);
    exit(0);
}

BOOL OSGetResetSwitchState(void) { return FALSE; }
u32 OSGetProgressiveMode(void) { return sProgressive; }
void OSSetProgressiveMode(u32 on) { sProgressive = on; }
u32 OSGetSoundMode(void) { return sSoundMode; }
void OSSetSoundMode(u32 mode) { sSoundMode = mode; }
u32 OSGetStackPointer(void) { volatile u32 x; return (u32)&x; }
void OSFillFPUContext(OSContext *ctx) { (void)ctx; }
void OSProtectRange(u32 chan, void *addr, u32 nBytes, u32 control) { (void)chan; (void)addr; (void)nBytes; (void)control; }

void DCFlushRange(void *addr, u32 nBytes) { (void)addr; (void)nBytes; }
void DCInvalidateRange(void *addr, u32 nBytes) { (void)addr; (void)nBytes; }
void DCStoreRange(void *addr, u32 nBytes) { (void)addr; (void)nBytes; }
void DCStoreRangeNoSync(void *addr, u32 nBytes) { (void)addr; (void)nBytes; }
void DCZeroRange(void *addr, u32 nBytes) { memset(addr, 0, nBytes); }

u32 PPCMfmsr(void) { return sInterruptsEnabled ? 0x8000 : 0; }
void PPCMtmsr(u32 msr) { sInterruptsEnabled = (msr & 0x8000) != 0; }
void PPCSync(void) {}
/* HID2/WPAR: the write-gather pipe is port_gx_w* (port_gxfifo.h); nothing to configure. */
u32 PPCMfhid2(void) { return 0; }
void PPCMthid2(u32 val) { (void)val; }
u32 PPCMfwpar(void) { return 0; }
void PPCMtwpar(u32 val) { (void)val; }
