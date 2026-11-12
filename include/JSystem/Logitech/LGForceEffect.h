#ifndef LOGITECH_LGFORCEEFFECT_H
#define LOGITECH_LGFORCEEFFECT_H

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct LGEnvelope
{
    u32 attackTime;
    u32 fadeTime;
    u8 attackLevel;
    u8 fadeLevel;
} LGEnvelope;

typedef struct LGCondition
{
    s8 offset;
    u8 deadband;
    u8 saturationNeg;
    u8 saturationPos;
    s16 coefficientNeg;
    s16 coefficientPos;
} LGCondition;

typedef struct LGForceEffect
{
    u8 type;
    u32 duration;
    u32 startDelay;
    union
    {
        struct
        {
            s16 magnitude;
            u16 direction;
            LGEnvelope envelope;
        } constant;
        struct
        {
            s16 startMagnitude;
            s16 endMagnitude;
            u16 direction;
        } ramp;
        struct
        {
            u8 magnitude;
            u16 direction;
            u16 period;
            u16 phase;
            s16 offset;
            LGEnvelope envelope;
        } periodic;
        LGCondition condition[2];
    } p;
} LGForceEffect;

struct LGPosition;

s32 LGOpen(s32 chan, u32 *handle);
s32 LGRead(struct LGPosition *status);
s32 LGDownloadForceEffect(u32 device, u32 *id, LGForceEffect *fx);
s32 LGUpdateForceEffect(u32 id, LGForceEffect *fx);
s32 LGStartForceEffect(u32 id);
s32 LGStopForceEffect(u32 id);
s32 LGDestroyForceEffect(u32 id);

#ifdef __cplusplus
}
#endif

#endif // LOGITECH_LGFORCEEFFECT_H
