#ifndef LOGITECH_CONSTANT_H
#define LOGITECH_CONSTANT_H

#include "types.h"
#include "JSystem/Logitech/Force.h"

class Constant : public Force
{
public:
    Constant();
    s32 DownloadForce(s32, s32, u32&, u32, u32, s16, u16, u32, u32, u8, u8);
    s32 UpdateForce(s32, s32, u32, u32, s16, u16, u32, u32, u8, u8);
}; // Size: 0xA0

#endif // LOGITECH_CONSTANT_H
