#include "JSystem/Logitech/Condition.h"
#include "JSystem/Logitech/LGForceEffect.h"

#include <dolphin/os.h>
#include <string.h>

Condition::Condition()
{
}

Condition::~Condition()
{
}

s32 Condition::DownloadForce(s32 chan, s32 idx, u32 &device, u8 type, u32 duration, u32 startDelay, s8 offset,
                             u8 deadband, u8 saturationNeg, u8 saturationPos, s16 coefficientNeg, s16 coefficientPos)
{
    s32 ret = 0;
    if (mIds[chan][idx] != -1)
    {
        Destroy(chan, idx);
    }

    if (device != -1)
    {
        LGForceEffect fx;
        memset(&fx, 0, sizeof(fx));
        fx.type = type;
        fx.duration = duration;
        fx.startDelay = startDelay;
        fx.p.condition[0].offset = offset;
        fx.p.condition[0].deadband = deadband;
        fx.p.condition[0].saturationNeg = saturationNeg;
        fx.p.condition[0].saturationPos = saturationPos;
        fx.p.condition[0].coefficientNeg = coefficientNeg;
        fx.p.condition[0].coefficientPos = coefficientPos;
        fx.p.condition[1] = fx.p.condition[0];
        ret = LGDownloadForceEffect(device, &mIds[chan][idx], &fx);
        if (ret < 0)
        {
            OSReport("ERROR: DownloadForce(condition force) on channel %d returned %d\n", chan, ret);
            mIds[chan][idx] = -1;
        }
    }
    else
    {
        OSReport("ERROR: Trying to download a condition force to channel %d but wheel has not been opened.\n", chan);
    }
    return ret;
}

s32 Condition::UpdateForce(s32 chan, s32 idx, u8 type, u32 duration, u32 startDelay, s8 offset, u8 deadband,
                           u8 saturationNeg, u8 saturationPos, s16 coefficientNeg, s16 coefficientPos)
{
    LGForceEffect fx;
    s32 ret;
    memset(&fx, 0, sizeof(fx));
    fx.type = type;
    fx.duration = duration;
    fx.startDelay = startDelay;
    fx.p.condition[0].offset = offset;
    fx.p.condition[0].deadband = deadband;
    fx.p.condition[0].saturationNeg = saturationNeg;
    fx.p.condition[0].saturationPos = saturationPos;
    fx.p.condition[0].coefficientNeg = coefficientNeg;
    fx.p.condition[0].coefficientPos = coefficientPos;
    fx.p.condition[1] = fx.p.condition[0];
    ret = LGUpdateForceEffect(mIds[chan][idx], &fx);
    if (ret < 0)
    {
        OSReport("ERROR: UpdateForce(condition force) on channel %d returned %d\n", chan, ret);
        mIds[chan][idx] = -1;
    }
    return ret;
}
