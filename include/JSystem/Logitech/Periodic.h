#ifndef LOGITECH_PERIODIC_H
#define LOGITECH_PERIODIC_H

#include "types.h"
#include "JSystem/Logitech/Force.h"

class Periodic : public Force
{
public:
    Periodic();
    s32 DownloadForce(s32, s32, u32&, u8, u32, u32, u8, u16, u16, u16, s16, u32, u32, u8, u8);
    s32 UpdateForce(s32, s32, u8, u32, u32, u8, u16, u16, u16, s16, u32, u32, u8, u8);
}; // Size: 0xA0

#endif // LOGITECH_PERIODIC_H
