#include "dolphin/os.h"
#include "dolphin/siSteering.h"

extern void __OSReschedule(void);

static void __SISteeringHandler(s32 chan, u32 error, OSContext *context)
{
    SISteeringControl *sc;
    void (*proc)(s32);
    SISteeringCallback callback;
    OSContext exceptionContext;

    sc = &__SISteering[chan];
    if (!__SIResetSteering)
    {
        if (error & 8)
        {
            sc->ret = -1;
        }
        else if (error & 7)
        {
            sc->ret = -3;
        }
        else
        {
            sc->ret = 0;
        }

        if (sc->proc != NULL)
        {
            proc = sc->proc;
            sc->proc = NULL;
            proc(chan);
        }

        if (sc->callback != NULL)
        {
            OSClearContext(&exceptionContext);
            OSSetCurrentContext(&exceptionContext);
            callback = sc->callback;
            sc->callback = NULL;
            callback(chan, sc->ret);
            OSClearContext(&exceptionContext);
            OSSetCurrentContext(context);
        }
    }
}

void __SISteeringSyncCallback(s32 chan, s32 result)
{
    SISteeringControl *sc;

    sc = &__SISteering[chan];
    OSWakeupThread(&sc->threadQueue);
}

s32 __SISteeringSync(s32 chan)
{
    SISteeringControl *sc;
    s32 ret;
    BOOL enabled;

    sc = &__SISteering[chan];
    enabled = OSDisableInterrupts();

    while (sc->callback != NULL)
    {
        OSSleepThread(&sc->threadQueue);
    }

    ret = sc->ret;
    OSRestoreInterrupts(enabled);
    return ret;
}

static void TypeAndStatusCallback(s32 chan, u32 type)
{
    SISteeringControl *sc;
    void (*proc)(s32);
    SISteeringCallback callback;
    OSContext exceptionContext;
    OSContext *context;

    sc = &__SISteering[chan];
    if (!__SIResetSteering)
    {
        if ((u32)((type & 0xFFFF0000) + 0xF8000000) == 0)
        {
            if (SITransfer(chan, sc, sc->outputBytes, sc->input, sc->inputBytes, __SISteeringHandler, 0))
            {
                return;
            }
            sc->ret = -2;
        }
        else
        {
            sc->ret = -1;
        }

        if (sc->proc != NULL)
        {
            proc = sc->proc;
            sc->proc = NULL;
            proc(chan);
        }

        if (sc->callback != NULL)
        {
            context = OSGetCurrentContext();
            OSClearContext(&exceptionContext);
            OSSetCurrentContext(&exceptionContext);
            callback = sc->callback;
            sc->callback = NULL;
            callback(chan, sc->ret);
            OSClearContext(&exceptionContext);
            OSSetCurrentContext(context);
            __OSReschedule();
        }
    }
}

s32 __SISteeringTransfer(s32 chan, u32 outputBytes, u32 inputBytes, void (*proc)(s32 chan))
{
    BOOL enabled;
    SISteeringControl *sc;

    sc = &__SISteering[chan];
    enabled = OSDisableInterrupts();

    sc->proc = proc;
    sc->outputBytes = outputBytes;
    sc->inputBytes = inputBytes;
    SIGetTypeAsync(chan, TypeAndStatusCallback);

    OSRestoreInterrupts(enabled);
    return 0;
}
