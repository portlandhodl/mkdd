#include "JSystem/Logitech/Constant.h"
#include "JSystem/Logitech/LGForceEffect.h"

#include <dolphin/os.h>
#include <string.h>

Constant::Constant()
{
}

Constant::~Constant()
{
}

s32 Constant::DownloadForce(s32 chan, s32 idx, u32 &device, u32 duration, u32 startDelay, s16 magnitude, u16 direction,
                            u32 attackTime, u32 fadeTime, u8 attackLevel, u8 fadeLevel)
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
        fx.type = 0;
        fx.duration = duration;
        fx.startDelay = startDelay;
        fx.p.constant.magnitude = magnitude;
        fx.p.constant.direction = direction;
        fx.p.constant.envelope.attackTime = attackTime;
        fx.p.constant.envelope.attackLevel = attackLevel;
        fx.p.constant.envelope.fadeTime = fadeTime;
        fx.p.constant.envelope.fadeLevel = fadeLevel;
        ret = LGDownloadForceEffect(device, &mIds[chan][idx], &fx);
        if (ret < 0)
        {
            OSReport("ERROR: DownloadForce(constant force) on channel %d returned %d\n", chan, ret);
            mIds[chan][idx] = -1;
        }
    }
    else
    {
        OSReport("ERROR: Trying to download a constant force to channel %d but wheel has not been opened.\n", chan);
    }
    return ret;
}

s32 Constant::UpdateForce(s32 chan, s32 idx, u32 duration, u32 startDelay, s16 magnitude, u16 direction, u32 attackTime,
                          u32 fadeTime, u8 attackLevel, u8 fadeLevel)
{
    LGForceEffect fx;
    s32 ret;
    memset(&fx, 0, sizeof(fx));
    fx.type = 0;
    fx.duration = duration;
    fx.startDelay = startDelay;
    fx.p.constant.magnitude = magnitude;
    fx.p.constant.direction = direction;
    fx.p.constant.envelope.attackTime = attackTime;
    fx.p.constant.envelope.attackLevel = attackLevel;
    fx.p.constant.envelope.fadeTime = fadeTime;
    fx.p.constant.envelope.fadeLevel = fadeLevel;
    ret = LGUpdateForceEffect(mIds[chan][idx], &fx);
    if (ret < 0)
    {
        OSReport("ERROR: UpdateForce(constant force) on channel %d returned %d\n", chan, ret);
        mIds[chan][idx] = -1;
    }
    return ret;
}
