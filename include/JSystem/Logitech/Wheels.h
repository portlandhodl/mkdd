#ifndef LOGITECH_WHEELS_H
#define LOGITECH_WHEELS_H

#include "types.h"
#include <dolphin/lg.h>

class Wheels
{
public:
    Wheels();
    s32 ReadAll();
    bool IsConnected(s32);

    s32 mHandles[4];
    u8 _10[0x828 - 0x10];
    u32 mEffectId[4];
    u8 _838[0x880 - 0x838];
}; // Size: 0x880

#endif // LOGITECH_WHEELS_H
