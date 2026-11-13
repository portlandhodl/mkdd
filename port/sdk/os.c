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

BOOL OSEnableInterrupts(void)
{
    BOOL old = sInterruptsEnabled;
    sInterruptsEnabled = TRUE;
    return old;
}

BOOL OSRestoreInterrupts(BOOL level)
{
    BOOL old = sInterruptsEnabled;
    sInterruptsEnabled = level;
    return old;
}

void OSReport(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
    fflush(stdout);
}

void OSPanic(const char *file, int line, const char *msg, ...)
{
    va_list ap;
    fprintf(stderr, "[OSPanic] %s:%d: ", file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fputc('\n', stderr);
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
