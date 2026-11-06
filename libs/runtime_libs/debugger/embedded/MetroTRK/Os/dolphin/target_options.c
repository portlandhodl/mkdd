#include "PowerPC_EABI_Support/MetroTRK/target_options.h"

static u8 bUseSerialIO;

void SetUseSerialIO(u8 sio)
{
    bUseSerialIO = sio;
}

u8 GetUseSerialIO(void)
{
    return bUseSerialIO;
}
