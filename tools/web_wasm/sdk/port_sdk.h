/* Internal interface of the port's GameCube SDK replacement (tools/web_wasm/sdk). */
#ifndef PORT_SDK_H
#define PORT_SDK_H

#include "sdk_all.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Log a call to an SDK function that is still a no-op (once per name). */
void port_stub(const char *name);
#define MKDD_STUB(name) port_stub(name)

/* Scheduler hooks (os_thread.c). */
void port_os_init(void);
/* Run due "interrupt" work: queued completions, alarms and a VI retrace if one is due.
 * Never switches threads; wakeups take effect at the next scheduling point. */
void port_poll(void);
/* A thread may block in OSSleepThread here (scheduler up, not in interrupt context). */
int port_thread_can_wait(void);
/* DVD (dvd.c): wake threads waiting for browser disc reads; reads in flight. */
void port_dvd_poll(void);
int port_dvd_reads_pending(void);
void port_dvd_idle(unsigned ms); /* idle up to ms, returning early when a read completes */
/* Queue a callback to run from the next poll, like a hardware interrupt would. */
void port_defer(void (*fn)(void *, void *), void *a, void *b);
/* VI retrace bookkeeping (vi.c), called by the scheduler when a retrace is due. */
void port_vi_retrace(void);
OSTime port_vi_next_retrace(void);

/* Integer value of a host environment variable (node: process.env), 0 if unset. */
int port_env(const char *name);
void port_dump_threads(void);
/* OSReport: print what was folded (repeats, bursts) in the last second (os.c). */
void port_report_tick(void);

/* Disc root on the host filesystem (dvd.c). */
const char *port_disc_root(void);

#define PORT_TICKS_PER_SEC 40500000ll

/* Frame-time accounting (MKDD_PERF=1, see vi_pad.c): host milliseconds spent per
 * category, reported every 600 retraces. port_perf_enabled is 0 when disabled. */
/* PERF_DVD is wall time in DVD calls: in the browser it includes the reading
 * thread's wait for the File read, while other threads run. */
enum { PERF_IDLE, PERF_PRESENT, PERF_DVD, PERF_READS, PERF_IDLES, PERF_NUM };
/* PERF_READS / PERF_IDLES are counts: browser disc reads, scheduler idle waits. */
extern int port_perf_enabled;
double port_perf_now(void);
void port_perf_add(int cat, double ms);

#ifdef __cplusplus
}
#endif

#endif
