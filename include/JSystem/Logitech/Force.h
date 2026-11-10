#ifndef LOGITECH_FORCE_H
#define LOGITECH_FORCE_H

#include "types.h"

class Force
{
public:
    Force();
    void InitVars();
    void Start(s32, s32);
    void Stop(s32, s32);
    void Destroy(s32, s32);

    u8 mBytes[4][8];
    s32 mIds[4][8];
}; // Size: 0xA0

#endif // LOGITECH_FORCE_H
