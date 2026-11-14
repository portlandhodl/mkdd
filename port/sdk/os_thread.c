/* OS threads, message queues, mutexes and alarms on a cooperative scheduler.
 *
 * Each OSThread runs on an Emscripten fiber (Asyncify). The GameCube kernel is
 * priority-preemptive on one core; here a switch happens only at OS calls that
 * block, ready a higher-priority thread, or yield. Hardware interrupts (DVD and
 * ARQ completion, alarms, VI retrace) are delivered from port_poll(), which runs
 * whenever no thread is ready and from time/status queries used in polling loops. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include <emscripten/fiber.h>
#include "port_sdk.h"

#define C_STACK_SIZE (512 * 1024)
#define ASYNC_STACK_SIZE (512 * 1024)
#define MAX_THREADS 64

typedef struct PortThread {
    OSThread *thread;
    emscripten_fiber_t fiber;
    void *cStack;
    void *asyncStack;
    u32 seq; /* FIFO order within a priority */
    int dead; /* exited or cancelled: never scheduled again; slot reusable once switched away */
} PortThread;

static OSThread sDefaultThread;
static PortThread sPorts[MAX_THREADS];
static OSThread *sCurrent;
static PortThread *sCurPort;
static s32 sSchedDisable;
static u32 sSeq;
static int sInIrq;
static OSSwitchThreadCallback sSwitchCallback;
static OSAlarm *sAlarms;

typedef struct Deferred {
    void (*fn)(void *, void *);
    void *a, *b;
} Deferred;
static Deferred sDeferred[256];
static int sDeferHead, sDeferTail;

static PortThread *port_of(OSThread *t)
{
    int i;
    for (i = 0; i < MAX_THREADS; i++)
        if (sPorts[i].thread == t && !sPorts[i].dead)
            return &sPorts[i];
    return NULL;
}

static void mark_dead(OSThread *t)
{
    PortThread *p = port_of(t);
    if (p)
        p->dead = 1;
}

static PortThread *port_alloc(OSThread *t)
{
    PortThread *p = port_of(t);
    int i;
    if (p)
        return p;
    for (i = 0; i < MAX_THREADS; i++)
        if (!sPorts[i].thread || (sPorts[i].dead && &sPorts[i] != sCurPort)) {
            sPorts[i].thread = t;
            sPorts[i].dead = 0;
            return &sPorts[i];
        }
    fprintf(stderr, "[sdk] out of thread slots\n");
    abort();
}

void port_os_init(void)
{
    PortThread *p;
    if (sCurrent)
        return;
    memset(&sDefaultThread, 0, sizeof sDefaultThread);
    sDefaultThread.state = OS_THREAD_STATE_RUNNING;
    sDefaultThread.priority = sDefaultThread.base = 16;
    p = port_alloc(&sDefaultThread);
    p->asyncStack = malloc(ASYNC_STACK_SIZE);
    emscripten_fiber_init_from_current_context(&p->fiber, p->asyncStack, ASYNC_STACK_SIZE);
    sCurrent = &sDefaultThread;
    sCurPort = p;
}

int port_interrupts_enabled(void);

static void ensure_init(void)
{
    if (!sCurrent)
        OSInit();
}

/* ---- wait queues (OSThread.link / OSThread.queue) ---- */

static void queue_push(OSThreadQueue *q, OSThread *t)
{
    t->queue = q;
    t->link.next = NULL;
    t->link.prev = q->tail;
    if (q->tail)
        q->tail->link.next = t;
    else
        q->head = t;
    q->tail = t;
}

static void queue_remove(OSThread *t)
{
    OSThreadQueue *q = t->queue;
    if (!q)
        return;
    if (t->link.prev)
        t->link.prev->link.next = t->link.next;
    else
        q->head = t->link.next;
    if (t->link.next)
        t->link.next->link.prev = t->link.prev;
    else
        q->tail = t->link.prev;
    t->link.next = t->link.prev = NULL;
    t->queue = NULL;
}

/* ---- scheduling ---- */

static BOOL runnable(OSThread *t)
{
    return (t->state == OS_THREAD_STATE_READY || t->state == OS_THREAD_STATE_RUNNING) && t->suspend <= 0;
}

static PortThread *pick(void)
{
    PortThread *best = NULL;
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        PortThread *p = &sPorts[i];
        if (!p->thread || p->dead || !runnable(p->thread))
            continue;
        if (!best || p->thread->priority < best->thread->priority ||
            (p->thread->priority == best->thread->priority && p->seq < best->seq))
            best = p;
    }
    return best;
}

static void make_ready(OSThread *t)
{
    PortThread *p = port_of(t);
    t->state = OS_THREAD_STATE_READY;
    if (p)
        p->seq = ++sSeq;
}

static void switch_to(PortThread *next)
{
    PortThread *cur = sCurPort;
    OSThread *from = sCurrent;
    if (next == cur) {
        sCurrent->state = OS_THREAD_STATE_RUNNING;
        return;
    }
    if (from->state == OS_THREAD_STATE_RUNNING)
        from->state = OS_THREAD_STATE_READY;
    if (sSwitchCallback)
        sSwitchCallback(from, next->thread);
    sCurrent = next->thread;
    sCurPort = next;
    sCurrent->state = OS_THREAD_STATE_RUNNING;
    emscripten_fiber_swap(&cur->fiber, &next->fiber);
}

/* Run the highest-priority ready thread; idle (deliver interrupts, wait for the
 * next retrace) while none is ready. */
static void reschedule(void)
{
    for (;;) {
        PortThread *next = pick();
        if (next) {
            switch_to(next);
            return;
        }
        /* deliver pending completions before waiting: they may ready a thread */
        if (sDeferHead != sDeferTail) {
            port_poll();
            continue;
        }
        {
            OSTime now = (OSTime)(emscripten_get_now() * (PORT_TICKS_PER_SEC / 1000));
            OSTime due = port_vi_next_retrace();
            OSAlarm *a;
            for (a = sAlarms; a; a = a->next)
                if (a->fire < due)
                    due = a->fire;
            if (due > now)
                emscripten_sleep((unsigned)((due - now) / (PORT_TICKS_PER_SEC / 1000)) + 1);
            else
                emscripten_sleep(0);
        }
        port_poll();
    }
}

/* Called after a thread became ready: switch if it outranks the caller. */
static void maybe_preempt(void)
{
    PortThread *next;
    if (sInIrq || sSchedDisable > 0)
        return;
    next = pick();
    if (next && next->thread != sCurrent &&
        (!runnable(sCurrent) || next->thread->priority < sCurrent->priority))
        switch_to(next);
}

/* Debug aid: print every live thread (called on OSPanic). */
void port_dump_threads(void)
{
    int i;
    fprintf(stderr, "[os] threads (current %p):\n", (void *)sCurrent);
    for (i = 0; i < MAX_THREADS; i++) {
        PortThread *p = &sPorts[i];
        OSThread *t = p->thread;
        if (!t || p->dead)
            continue;
        fprintf(stderr, "[os]   %p prio %d state %d suspend %d queue %p entry %p%s\n", (void *)t, (int)t->priority,
                t->state, (int)t->suspend, (void *)t->queue, t->specific[0], t == sCurrent ? " <- current" : "");
    }
    fprintf(stderr, "[os] deferred %d, sched disable %d\n", (sDeferTail - sDeferHead + 256) % 256, (int)sSchedDisable);
}

void port_defer(void (*fn)(void *, void *), void *a, void *b)
{
    int next = (sDeferTail + 1) % 256;
    if (next == sDeferHead) {
        fprintf(stderr, "[sdk] deferred queue full\n");
        abort();
    }
    sDeferred[sDeferTail].fn = fn;
    sDeferred[sDeferTail].a = a;
    sDeferred[sDeferTail].b = b;
    sDeferTail = next;
}

void port_poll(void)
{
    OSTime now;
    OSAlarm *a;
    static int exitAfter = -1;
    if (exitAfter < 0)
        exitAfter = port_env("MKDD_EXIT_AFTER");
    if (exitAfter && emscripten_get_now() > exitAfter * 1000.0) {
        fprintf(stderr, "[os] MKDD_EXIT_AFTER reached\n");
        port_dump_threads();
        exit(0);
    }
    if (sInIrq)
        return;
    sInIrq = 1;
    while (sDeferHead != sDeferTail) {
        Deferred d = sDeferred[sDeferHead];
        sDeferHead = (sDeferHead + 1) % 256;
        d.fn(d.a, d.b);
    }
    now = (OSTime)(emscripten_get_now() * (PORT_TICKS_PER_SEC / 1000));
    for (a = sAlarms; a;) {
        OSAlarm *next = a->next;
        if (a->fire <= now) {
            OSCancelAlarm(a);
            if (a->handler)
                a->handler(a, &sCurrent->context);
            next = sAlarms;
        }
        a = next;
    }
    if (port_vi_next_retrace() <= now)
        port_vi_retrace();
    sInIrq = 0;
    /* like returning from an interrupt: run a higher-priority thread it readied */
    if (port_interrupts_enabled())
        maybe_preempt();
}

/* OSEnableInterrupts/OSRestoreInterrupts: deliver what was held off. */
void port_interrupts_reenabled(void)
{
    if (sCurrent && !sInIrq)
        port_poll();
}

static void thread_entry(void *arg)
{
    OSThread *t = (OSThread *)arg;
    OSThreadStartFunction fn = (OSThreadStartFunction)t->specific[0];
    void *param = t->specific[1];
    t->specific[0] = t->specific[1] = NULL;
    OSExitThread(fn(param));
}

BOOL OSCreateThread(OSThread *thread, OSThreadStartFunction func, void *param, void *stack, u32 stackSize,
                    OSPriority prio, u16 attr)
{
    PortThread *p;
    ensure_init();
    memset(thread, 0, sizeof *thread);
    thread->state = OS_THREAD_STATE_READY;
    thread->attr = attr & OS_THREAD_ATTR_DETACH;
    thread->priority = thread->base = prio;
    thread->suspend = 1;
    thread->stackBase = (u8 *)stack;
    thread->stackEnd = (u32 *)((u8 *)stack - stackSize);
    thread->specific[0] = (void *)func;
    thread->specific[1] = param;
    p = port_alloc(thread);
    if (!p->cStack) {
        /* 16-byte aligned: compiled code assumes an aligned stack pointer (musl's
         * __stdio_write finds its second iovec as (sp + 16) | 8) */
        p->cStack = aligned_alloc(16, C_STACK_SIZE);
        p->asyncStack = malloc(ASYNC_STACK_SIZE);
    }
    emscripten_fiber_init(&p->fiber, thread_entry, thread, p->cStack, C_STACK_SIZE, p->asyncStack, ASYNC_STACK_SIZE);
    p->seq = ++sSeq;
    return TRUE;
}

void OSExitThread(void *val)
{
    OSThread *t = sCurrent;
    t->val = val;
    t->state = (t->attr & OS_THREAD_ATTR_DETACH) ? OS_THREAD_STATE_NULL : OS_THREAD_STATE_MORIBUND;
    sCurPort->dead = 1; /* stacks are reused once we switch away; this fiber never resumes */
    OSWakeupThread(&t->queueJoin);
    reschedule();
    fprintf(stderr, "[sdk] exited thread resumed\n");
    abort();
}

void OSCancelThread(OSThread *thread)
{
    if (thread == sCurrent) {
        OSExitThread(NULL);
        return;
    }
    queue_remove(thread);
    thread->state = (thread->attr & OS_THREAD_ATTR_DETACH) ? OS_THREAD_STATE_NULL : OS_THREAD_STATE_MORIBUND;
    mark_dead(thread);
    OSWakeupThread(&thread->queueJoin);
}

void OSDetachThread(OSThread *thread)
{
    thread->attr |= OS_THREAD_ATTR_DETACH;
    if (thread->state == OS_THREAD_STATE_MORIBUND)
        thread->state = OS_THREAD_STATE_NULL;
}

BOOL OSIsThreadTerminated(OSThread *thread)
{
    return thread->state == OS_THREAD_STATE_MORIBUND || thread->state == OS_THREAD_STATE_NULL;
}

s32 OSResumeThread(OSThread *thread)
{
    s32 old = thread->suspend;
    if (thread->suspend > 0 && --thread->suspend == 0 && thread->state == OS_THREAD_STATE_READY) {
        make_ready(thread);
        maybe_preempt();
    }
    return old;
}

s32 OSSuspendThread(OSThread *thread)
{
    s32 old = thread->suspend++;
    if (thread == sCurrent && old == 0)
        reschedule();
    return old;
}

void OSSleepThread(OSThreadQueue *queue)
{
    ensure_init();
    sCurrent->state = OS_THREAD_STATE_WAITING;
    queue_push(queue, sCurrent);
    reschedule();
}

void OSWakeupThread(OSThreadQueue *queue)
{
    BOOL any = FALSE;
    while (queue->head) {
        OSThread *t = queue->head;
        queue_remove(t);
        if (t->state == OS_THREAD_STATE_WAITING) {
            make_ready(t);
            any = TRUE;
        }
    }
    if (any)
        maybe_preempt();
}

void OSYieldThread(void)
{
    ensure_init();
    /* a yield loop waits on other threads: let due interrupts (audio DMA/DSP) ready them */
    port_poll();
    sCurPort->seq = ++sSeq;
    sCurrent->state = OS_THREAD_STATE_READY;
    reschedule();
}

OSThread *OSGetCurrentThread(void)
{
    ensure_init();
    return sCurrent;
}

OSContext *OSGetCurrentContext(void)
{
    ensure_init();
    return &sCurrent->context;
}

s32 OSDisableScheduler(void) { return sSchedDisable++; }

s32 OSEnableScheduler(void)
{
    s32 old = sSchedDisable--;
    if (sSchedDisable == 0)
        maybe_preempt();
    return old;
}

OSSwitchThreadCallback OSSetSwitchThreadCallback(OSSwitchThreadCallback cb)
{
    OSSwitchThreadCallback old = sSwitchCallback;
    sSwitchCallback = cb;
    return old;
}

void OSInitThreadQueue(OSThreadQueue *queue) { queue->head = queue->tail = NULL; }

/* ---- messages ---- */

void OSInitMessageQueue(OSMessageQueue *mq, OSMessage *msgArray, s32 msgCount)
{
    OSInitThreadQueue(&mq->queueSend);
    OSInitThreadQueue(&mq->queueReceive);
    mq->msgArray = msgArray;
    mq->msgCount = msgCount;
    mq->firstIndex = 0;
    mq->usedCount = 0;
}

BOOL OSSendMessage(OSMessageQueue *mq, OSMessage msg, s32 flags)
{
    while (mq->usedCount >= mq->msgCount) {
        if (!(flags & OS_MESSAGE_BLOCK))
            return FALSE;
        OSSleepThread(&mq->queueSend);
    }
    mq->msgArray[(mq->firstIndex + mq->usedCount) % mq->msgCount] = msg;
    mq->usedCount++;
    OSWakeupThread(&mq->queueReceive);
    return TRUE;
}

BOOL OSJamMessage(OSMessageQueue *mq, OSMessage msg, s32 flags)
{
    while (mq->usedCount >= mq->msgCount) {
        if (!(flags & OS_MESSAGE_BLOCK))
            return FALSE;
        OSSleepThread(&mq->queueSend);
    }
    mq->firstIndex = (mq->firstIndex + mq->msgCount - 1) % mq->msgCount;
    mq->msgArray[mq->firstIndex] = msg;
    mq->usedCount++;
    OSWakeupThread(&mq->queueReceive);
    return TRUE;
}

BOOL OSReceiveMessage(OSMessageQueue *mq, OSMessage *msg, s32 flags)
{
    while (mq->usedCount == 0) {
        if (!(flags & OS_MESSAGE_BLOCK)) {
            port_poll();
            if (mq->usedCount == 0)
                return FALSE;
            break;
        }
        OSSleepThread(&mq->queueReceive);
    }
    if (msg)
        *msg = mq->msgArray[mq->firstIndex];
    mq->firstIndex = (mq->firstIndex + 1) % mq->msgCount;
    mq->usedCount--;
    OSWakeupThread(&mq->queueSend);
    return TRUE;
}

/* ---- mutexes ---- */

void OSInitMutex(OSMutex *mutex)
{
    OSInitThreadQueue(&mutex->queue);
    mutex->thread = NULL;
    mutex->count = 0;
}

void OSLockMutex(OSMutex *mutex)
{
    ensure_init();
    while (mutex->thread && mutex->thread != sCurrent)
        OSSleepThread(&mutex->queue);
    mutex->thread = sCurrent;
    mutex->count++;
}

void OSUnlockMutex(OSMutex *mutex)
{
    if (mutex->thread != sCurrent || --mutex->count > 0)
        return;
    mutex->thread = NULL;
    OSWakeupThread(&mutex->queue);
}

/* ---- alarms ---- */

void OSCreateAlarm(OSAlarm *alarm)
{
    alarm->handler = NULL;
    alarm->tag = 0;
    alarm->prev = alarm->next = NULL;
}

void OSCancelAlarm(OSAlarm *alarm)
{
    OSAlarm *a;
    for (a = sAlarms; a; a = a->next)
        if (a == alarm)
            break;
    if (!a)
        return;
    if (alarm->prev)
        alarm->prev->next = alarm->next;
    else
        sAlarms = alarm->next;
    if (alarm->next)
        alarm->next->prev = alarm->prev;
    alarm->prev = alarm->next = NULL;
}

void OSSetAlarm(OSAlarm *alarm, OSTime tick, OSAlarmHandler handler)
{
    OSCancelAlarm(alarm);
    alarm->handler = handler;
    alarm->period = 0;
    alarm->start = 0;
    alarm->fire = (OSTime)(emscripten_get_now() * (PORT_TICKS_PER_SEC / 1000)) + tick;
    alarm->prev = NULL;
    alarm->next = sAlarms;
    if (sAlarms)
        sAlarms->prev = alarm;
    sAlarms = alarm;
}
