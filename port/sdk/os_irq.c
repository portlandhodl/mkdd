/* Processor-interface interrupts and the graphics register file (port_hw.h).
 * The GX library reaches the CP/PE/PI/MEM registers through these arrays; an
 * interrupt raised by the emulated hardware runs its __OSSetInterruptHandler
 * handler from port_poll(), as a hardware interrupt runs between instructions. */
#include <stdint.h>
#include "port_sdk.h"
#include "port_hw.h"

/* CP status (reg 0) reads as idle with the FIFO drained: underflow, read idle,
 * command idle. */
u16 port_hw_cp[0x80] = {[0] = 0x000E};
u16 port_hw_pe[0x100];
u32 port_hw_pi[0x100];
u16 port_hw_mem[0x80];

#define NUM_INTERRUPTS 32

static __OSInterruptHandler sHandlers[NUM_INTERRUPTS];
static OSInterruptMask sMask = 0xFFFFFFFFu; /* all masked until a driver unmasks */
static OSInterruptMask sPending;

__OSInterruptHandler __OSSetInterruptHandler(__OSInterrupt interrupt, __OSInterruptHandler handler)
{
    __OSInterruptHandler old;
    if ((u32)interrupt >= NUM_INTERRUPTS)
        return NULL;
    old = sHandlers[interrupt];
    sHandlers[interrupt] = handler;
    return old;
}

__OSInterruptHandler __OSGetInterruptHandler(__OSInterrupt interrupt)
{
    return (u32)interrupt < NUM_INTERRUPTS ? sHandlers[interrupt] : NULL;
}

static void dispatch(void *a, void *b)
{
    int interrupt = (int)(uintptr_t)a;
    OSInterruptMask bit = OS_INTERRUPTMASK(interrupt);
    (void)b;
    if (sMask & bit) {
        sPending |= bit; /* masked since it was raised: deliver on unmask */
        return;
    }
    sPending &= ~bit;
    if (sHandlers[interrupt])
        sHandlers[interrupt]((__OSInterrupt)interrupt, OSGetCurrentContext());
}

void port_raise_interrupt(int interrupt)
{
    OSInterruptMask bit;
    if ((u32)interrupt >= NUM_INTERRUPTS)
        return;
    bit = OS_INTERRUPTMASK(interrupt);
    if (sMask & bit) {
        sPending |= bit;
        return;
    }
    port_defer(dispatch, (void *)(uintptr_t)interrupt, NULL);
}

OSInterruptMask __OSMaskInterrupts(OSInterruptMask mask)
{
    OSInterruptMask old = sMask;
    sMask |= mask;
    return old;
}

OSInterruptMask __OSUnmaskInterrupts(OSInterruptMask mask)
{
    OSInterruptMask old = sMask;
    OSInterruptMask ready;
    int i;
    sMask &= ~mask;
    ready = sPending & ~sMask;
    for (i = 0; i < NUM_INTERRUPTS; i++) {
        if (ready & OS_INTERRUPTMASK(i)) {
            sPending &= ~OS_INTERRUPTMASK(i);
            port_defer(dispatch, (void *)(uintptr_t)i, NULL);
        }
    }
    return old;
}

/* Handlers run on the interrupted thread's host stack; the exception context
 * they swap in has nothing to hold. */
void OSClearContext(OSContext *context) { (void)context; }
void OSSetCurrentContext(OSContext *context) { (void)context; }

/* ---- INTERIM: always-idle GPU ----
 * Until port/gpu decodes the command stream, the GPU is treated as having
 * finished everything sent so far the moment a sync is requested. GX's PE
 * interrupt enables live in PE reg 5 (bit 0 token, bit 1 finish), as
 * __GXPEInit sets them. */
int port_gx_idle_gpu = 1;

void port_gx_idle_sync(u32 bpReg)
{
    if (!port_gx_idle_gpu)
        return;
    switch (bpReg >> 24) {
    case 0x47: /* PE token */
    case 0x48: /* PE token + interrupt */
        port_hw_pe[7] = (u16)bpReg;
        if ((bpReg >> 24) == 0x48 && (port_hw_pe[5] & 1)) {
            port_hw_pe[5] |= 4;
            port_raise_interrupt(__OS_INTERRUPT_PI_PE_TOKEN);
        }
        break;
    case 0x45: /* PE done (draw done) */
        if ((bpReg & 2) && (port_hw_pe[5] & 2)) {
            port_hw_pe[5] |= 8;
            port_raise_interrupt(__OS_INTERRUPT_PI_PE_FINISH);
        }
        break;
    }
}
