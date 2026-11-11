#ifndef LOGITECH_CONDITION_H
#define LOGITECH_CONDITION_H

#include "types.h"
#include "JSystem/Logitech/Force.h"

class Condition : public Force
{
public:
    Condition();
    ~Condition();
    s32 DownloadForce(s32, s32, u32&, u8, u32, u32, s8, u8, u8, u8, s16, s16);
    s32 UpdateForce(s32, s32, u8, u32, u32, s8, u8, u8, u8, s16, s16);
}; // Size: 0xA0

#endif // LOGITECH_CONDITION_H
