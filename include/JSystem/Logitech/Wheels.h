#ifndef LOGITECH_WHEELS_H
#define LOGITECH_WHEELS_H

#include "types.h"
#include <dolphin/lg.h>

class Wheels
{
public:
    Wheels();
    ~Wheels();
    s16 ReadAll();
    bool IsConnected(s32);

    LGPosition mPosition[4];     // 0x000
    u8 _28[0x828 - 0x28];        // 0x028
    u32 mEffectId[4];            // 0x828
    u8 _838[0x858 - 0x838];      // 0x838
    LGPosition mPrevPosition[4]; // 0x858
}; // Size: 0x880

#endif // LOGITECH_WHEELS_H
