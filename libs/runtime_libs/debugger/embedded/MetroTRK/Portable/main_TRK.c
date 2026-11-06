#include "PowerPC_EABI_Support/MetroTRK/trk.h"

static TRKResult TRK_mainError;

TRKResult TRK_main(void)
{
    MWTRACE(1, "TRK_Main \n");
    TRK_mainError = TRKInitializeNub();

    if (TRK_mainError == TRKSuccess) {
        TRKNubWelcome();
        TRKNubMainLoop();
    }

    TRK_mainError = TRKTerminateNub();

    return TRK_mainError;
}
