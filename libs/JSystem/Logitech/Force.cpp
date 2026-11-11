#include "JSystem/Logitech/Force.h"
#include "JSystem/Logitech/LGForceEffect.h"

#include <dolphin/os.h>

Force::Force()
{
    InitVars();
}

Force::~Force()
{
}

void Force::InitVars()
{
    for (s32 i = 0; i < 4; i++)
    {
        for (s32 j = 0; j < 8; j++)
        {
            mBytes[i][j] = 0;
            mIds[i][j] = -1;
        }
    }
}

s32 Force::Start(s32 chan, s32 idx)
{
    s32 ret = 0;
    if (mIds[chan][idx] != -1)
    {
        ret = LGStartForceEffect(mIds[chan][idx]);
        if (ret < 0)
        {
            OSReport("ERROR: Failed to start force effect on channel %d\n", chan);
        }
        else
        {
            mBytes[chan][idx] = 1;
        }
    }
    else
    {
        OSReport("ERROR: Trying to start force effect on channel %d but we have an invalid effectid\n", chan);
    }
    return ret;
}

s32 Force::Stop(s32 chan, s32 idx)
{
    s32 ret = 0;
    if (mIds[chan][idx] != -1)
    {
        ret = LGStopForceEffect(mIds[chan][idx]);
        if (ret < 0)
        {
            OSReport("ERROR: Failed to stop force effect on channel %d\n", chan);
        }
        else
        {
            mBytes[chan][idx] = 0;
        }
    }
    else
    {
        OSReport("ERROR: Trying to stop force effect on channel %d but we have an invalid effectid\n", chan);
    }
    return ret;
}

s32 Force::Destroy(s32 chan, s32 idx)
{
    s32 ret = 0;
    if (mIds[chan][idx] != -1)
    {
        ret = LGDestroyForceEffect(mIds[chan][idx]);
        if (ret < 0)
        {
            OSReport("ERROR: Failed to destroy force effect on channel %d\n", chan);
        }
        else
        {
            mBytes[chan][idx] = 0;
            mIds[chan][idx] = -1;
        }
    }
    else
    {
        OSReport("ERROR: Trying to destroy force effect on channel %d but we have an invalid effectid\n", chan);
    }
    return ret;
}
