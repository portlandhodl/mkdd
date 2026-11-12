#include "JSystem/Logitech/Periodic.h"
#include "JSystem/Logitech/LGForceEffect.h"

#include <dolphin/os.h>
#include <string.h>

Periodic::Periodic()
{
}

Periodic::~Periodic()
{
}

s32 Periodic::DownloadForce(s32 chan, s32 idx, u32 &device, u8 type, u32 duration, u32 startDelay, u8 magnitude,
                            u16 direction, u16 period, u16 phase, s16 offset, u32 attackTime, u32 fadeTime,
                            u8 attackLevel, u8 fadeLevel)
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
        fx.p.periodic.magnitude = magnitude;
        fx.p.periodic.direction = direction;
        fx.p.periodic.period = period;
        fx.p.periodic.phase = phase;
        fx.p.periodic.offset = offset;
        fx.p.periodic.envelope.attackTime = attackTime;
        fx.p.periodic.envelope.fadeTime = fadeTime;
        fx.p.periodic.envelope.attackLevel = attackLevel;
        fx.p.periodic.envelope.fadeLevel = fadeLevel;
        ret = LGDownloadForceEffect(device, &mIds[chan][idx], &fx);
        if (ret < 0)
        {
            OSReport("ERROR: DownloadForce(periodic force) on channel %d returned %d\n", chan, ret);
            mIds[chan][idx] = -1;
        }
    }
    else
    {
        OSReport("ERROR: Trying to download a periodic force to channel %d but wheel has not been opened.\n", chan);
    }
    return ret;
}

s32 Periodic::UpdateForce(s32 chan, s32 idx, u8 type, u32 duration, u32 startDelay, u8 magnitude, u16 direction,
                          u16 period, u16 phase, s16 offset, u32 attackTime, u32 fadeTime, u8 attackLevel,
                          u8 fadeLevel)
{
    LGForceEffect fx;
    s32 ret;
    memset(&fx, 0, sizeof(fx));
    fx.type = type;
    fx.duration = duration;
    fx.startDelay = startDelay;
    fx.p.periodic.magnitude = magnitude;
    fx.p.periodic.direction = direction;
    fx.p.periodic.period = period;
    fx.p.periodic.phase = phase;
    fx.p.periodic.offset = offset;
    fx.p.periodic.envelope.attackTime = attackTime;
    fx.p.periodic.envelope.fadeTime = fadeTime;
    fx.p.periodic.envelope.attackLevel = attackLevel;
    fx.p.periodic.envelope.fadeLevel = fadeLevel;
    ret = LGUpdateForceEffect(mIds[chan][idx], &fx);
    if (ret < 0)
    {
        OSReport("ERROR: UpdateForce(periodic force) on channel %d returned %d\n", chan, ret);
        mIds[chan][idx] = -1;
    }
    return ret;
}
