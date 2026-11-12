#include "JSystem/Logitech/Wheels.h"
#include "JSystem/Logitech/LGForceEffect.h"

#include <dolphin/os.h>
#include <string.h>

Wheels::Wheels()
{
    for (s32 i = 0; i < 4; i++)
    {
        mEffectId[i] = -1;
        mPosition[i].err = -1;
    }
    memset(mPrevPosition, 0, sizeof(mPrevPosition));
}

Wheels::~Wheels()
{
}

s16 Wheels::ReadAll()
{
    for (s32 i = 0; i < 4; i++)
    {
        if (SIProbe(i) == 0x08000000 && mEffectId[i] == -1)
        {
            if (LGOpen(i, &mEffectId[i]) < 0)
            {
                OSReport("ERROR: Could not open wheel on channel %d\n", i);
            }
            else
            {
                mPosition[i].err = 0;
            }
            break;
        }
    }

    memcpy(mPrevPosition, mPosition, sizeof(mPrevPosition));
    LGRead(mPosition);

    for (s32 i = 0; i < 4; i++)
    {
        if (mPosition[i].err == -1 && mEffectId[i] != -1)
        {
            mEffectId[i] = -1;
            return i;
        }
    }
    return -1;
}

bool Wheels::IsConnected(s32 chan)
{
    return mPosition[chan].err == 0;
}
