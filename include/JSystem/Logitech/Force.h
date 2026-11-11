#ifndef LOGITECH_FORCE_H
#define LOGITECH_FORCE_H

#include "types.h"

class Force
{
public:
    Force();
    ~Force();
    void InitVars();
    s32 Start(s32, s32);
    s32 Stop(s32, s32);
    s32 Destroy(s32, s32);

    u8 mBytes[4][8];
    u32 mIds[4][8];
}; // Size: 0xA0

#endif // LOGITECH_FORCE_H
