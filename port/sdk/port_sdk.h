/* Internal interface of the port's GameCube SDK replacement (port/sdk). */
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
/* Queue a callback to run from the next poll, like a hardware interrupt would. */
void port_defer(void (*fn)(void *, void *), void *a, void *b);
/* VI retrace bookkeeping (vi.c), called by the scheduler when a retrace is due. */
void port_vi_retrace(void);
OSTime port_vi_next_retrace(void);

/* Integer value of a host environment variable (node: process.env), 0 if unset. */
int port_env(const char *name);
void port_dump_threads(void);

/* Disc root on the host filesystem (dvd.c). */
const char *port_disc_root(void);

#define PORT_TICKS_PER_SEC 40500000ll

#ifdef __cplusplus
}
#endif

#endif
