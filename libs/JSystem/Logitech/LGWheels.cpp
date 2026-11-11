#include "JSystem/Logitech/LGWheels.h"

#include <dolphin/os.h>
#include <string.h>

extern "C" void LGInit();

LGWheels::LGWheels()
{
    LGInit();
    mFlag = 0xFF;
    for (s32 i = 0; i < 4; i++) {
        InitVars(i);
    }
}

LGWheels::~LGWheels()
{
}

void LGWheels::InitVars(s32 wheel)
{
    mPlaying[wheel] = 0;
    mDamperSuppress[wheel] = 0;
    mSpringSuppress[wheel] = 0;

    for (s32 i = 0; i < 8; i++) {
        mCondition.mIds[wheel][i] = -1;
        mCondition.mBytes[wheel][i] = 0;
        mConstant.mIds[wheel][i] = -1;
        mConstant.mBytes[wheel][i] = 0;
        mPeriodic.mIds[wheel][i] = -1;
        mPeriodic.mBytes[wheel][i] = 0;
        mRamp.mIds[wheel][i] = -1;
        mRamp.mBytes[wheel][i] = 0;
    }
}

void LGWheels::ReadAll()
{
    s32 ret = mWheels.ReadAll();
    memcpy(mPosition, &mWheels, 40);

    if ((s16)ret != -1) {
        InitVars((s16)ret);
    }
}

void LGWheels::StopForce(s32 wheel, s32 type)
{
    switch (type) {
    case 0:
        if (IsPlaying(wheel, 0)) {
            mCondition.Stop(wheel, 0);
        }
        break;
    case 1:
        if (IsPlaying(wheel, 1)) {
            mConstant.Stop(wheel, 0);
        }
        break;
    case 2:
        if (IsPlaying(wheel, 2)) {
            mCondition.Stop(wheel, 1);
        }
        break;
    case 3:
        if (IsPlaying(wheel, 3)) {
            mConstant.Stop(wheel, 1);
        }
        break;
    case 4:
        if (IsPlaying(wheel, 4)) {
            mPeriodic.Stop(wheel, 0);
        }
        break;
    case 5:
        if (IsPlaying(wheel, 5)) {
            mPeriodic.Stop(wheel, 1);
        }
        break;
    case 6:
        if (IsPlaying(wheel, 6)) {
            mPeriodic.Stop(wheel, 2);
        }
        break;
    case 7:
        if (IsPlaying(wheel, 7)) {
            mCondition.Stop(wheel, 2);
        }
        if (mDamperSuppress[wheel] != 0) {
            PlayDamperForce(wheel, mDamper[wheel]);
            mCondition.mBytes[wheel][1] = 1;
            mDamperSuppress[wheel] = 0;
        }
        if (mSpringSuppress[wheel] != 0) {
            PlaySpringForce(wheel, mSpring[wheel].type, mSpring[wheel].clip, mSpring[wheel].offset);
            mCondition.mBytes[wheel][0] = 1;
            mSpringSuppress[wheel] = 0;
        }
        break;
    case 8:
        if (IsPlaying(wheel, 8)) {
            mPeriodic.Stop(wheel, 3);
        }
        break;
    case 9:
        if (!IsPlaying(wheel, 9)) {
            break;
        }
        mPlaying[wheel] = 0;
        if (mAutoCalibFlags[wheel][0] == 1) {
            PlaySpringForce(wheel, mSpring[wheel].type, mSpring[wheel].clip, mSpring[wheel].offset);
        }
        if (mAutoCalibFlags[wheel][1] == 1) {
            PlayConstantForce(wheel, mConstantP[wheel].force, mConstantP[wheel].duration);
        }
        if (mAutoCalibFlags[wheel][2] == 1) {
            PlayDamperForce(wheel, mDamper[wheel]);
        }
        if (mAutoCalibFlags[wheel][5] == 1) {
            PlayDirtRoadEffect(wheel, (u8)mDirt[wheel]);
        }
        if (mAutoCalibFlags[wheel][6] == 1) {
            PlayBumpyRoadEffect(wheel, (u8)mBumpy[wheel]);
        }
        if (mAutoCalibFlags[wheel][7] == 1) {
            PlaySlipperyRoadEffect(wheel, mSlippery[wheel]);
        }
        if (mAutoCalibFlags[wheel][8] == 1) {
            PlaySurfaceEffect(wheel, mSurface[wheel].type, mSurface[wheel].param, mSurface[wheel].duration);
        }
        for (s32 i = 0; i < 10; i++) {
            mAutoCalibFlags[wheel][i] = 0;
        }
        break;
    }
}

bool LGWheels::IsConnected(s32 wheel)
{
    return mWheels.IsConnected(wheel);
}

bool LGWheels::IsPlaying(s32 wheel, s32 type)
{
    switch (type) {
    case 0:
        if (mCondition.mBytes[wheel][0]) return true;
        break;
    case 1:
        if (mConstant.mBytes[wheel][0]) return true;
        break;
    case 2:
        if (mCondition.mBytes[wheel][1]) return true;
        break;
    case 3:
        if (mConstant.mBytes[wheel][1]) return true;
        break;
    case 4:
        if (mPeriodic.mBytes[wheel][0]) return true;
        break;
    case 5:
        if (mPeriodic.mBytes[wheel][1]) return true;
        break;
    case 6:
        if (mPeriodic.mBytes[wheel][2]) return true;
        break;
    case 7:
        if (mCondition.mBytes[wheel][2]) return true;
        break;
    case 8:
        if (mPeriodic.mBytes[wheel][3]) return true;
        break;
    case 9:
        if (mPlaying[wheel] == 1) return true;
        break;
    }
    return false;
}

void LGWheels::ButtonTriggered(s32 wheel, u32 button)
{
}

void LGWheels::ButtonReleased(s32 wheel, u32 button)
{
}

void LGWheels::ButtonIsPressed(s32 wheel, u32 button)
{
}

void LGWheels::PedalsConnected(s32 wheel)
{
}

void LGWheels::PowerConnected(s32 wheel)
{
}

void LGWheels::SetOverallForceGain(s32 wheel, s32 gain)
{
}

s32 LGWheels::GetOverallForceGain(s32 wheel)
{
    return 0;
}

void LGWheels::GenerateNonLinValues(s32 wheel, u8 value)
{
}

void LGWheels::PlayAutoCalibAndSpringForce(s32 wheel)
{
    if (mWheels.IsConnected(wheel) == 0) {
        return;
    }
    if (mPlaying[wheel] != 0) {
        return;
    }

    if ((u32)mPeriodic.mIds[wheel][4] == 0xFFFFFFFFu) {
        mPeriodic.DownloadForce(wheel, 4, mWheels.mEffectId[wheel], 3, 2200, 0, 180, 90, 2200, 0, 0, 0, 0, 0, 0);
        mPeriodic.Start(wheel, 4);
    }

    if ((u32)mCondition.mIds[wheel][0] == 0xFFFFFFFFu) {
        mCondition.DownloadForce(wheel, 0, mWheels.mEffectId[wheel], 7, 0xFFFFFFFF, 2200, 0, 0, 180, 180, 180, 180);
        mCondition.Start(wheel, 0);
    }
}

void LGWheels::PlaySpringForce(s32 wheel, s8 type, u8 clip, s16 offset)
{
    if (mCondition.mBytes[wheel][2] != 0) {
        return;
    }
    if (mWheels.IsConnected(wheel)) {
    if (mPlaying[wheel] != 0) {
        return;
    }

    if (mCondition.mBytes[wheel][0] != 0) {
        if (SameSpringForceParams(wheel, type, clip, offset)) {
            return;
        }
        if (mCondition.UpdateForce(wheel, 0, 7, 0xFFFFFFFF, 0, type, 0, clip, clip, offset, offset) >= 0) {
            mSpring[wheel].type = type;
            mSpring[wheel].clip = clip;
            mSpring[wheel].offset = offset;
        }
        return;
    }

    if ((u32)mCondition.mIds[wheel][0] == 0xFFFFFFFFu) {
        if (mCondition.DownloadForce(wheel, 0, mWheels.mEffectId[wheel], 7, 0xFFFFFFFF, 0, type, 0, clip, clip, offset, offset) >= 0) {
            mSpring[wheel].type = type;
            mSpring[wheel].clip = clip;
            mSpring[wheel].offset = offset;
        }
        mCondition.Start(wheel, 0);
        return;
    }

    if (SameSpringForceParams(wheel, type, clip, offset)) {
        mCondition.Start(wheel, 0);
        return;
    }

    if (mCondition.UpdateForce(wheel, 0, 7, 0xFFFFFFFF, 0, type, 0, clip, clip, offset, offset) >= 0) {
        mSpring[wheel].type = type;
        mSpring[wheel].clip = clip;
        mSpring[wheel].offset = offset;
    }
    mCondition.Start(wheel, 0);
    return;
    }
    OSReport("ERROR: trying to play a force on channel %d but no wheel opened.\n", wheel);
}

void LGWheels::StopSpringForce(s32 wheel)
{
    StopForce(wheel, 0);
}

bool LGWheels::SameSpringForceParams(s32 wheel, s8 type, u8 clip, s16 offset)
{
    return mSpring[wheel].type == type
        && mSpring[wheel].clip == clip
        && mSpring[wheel].offset == offset;
}

void LGWheels::PlayConstantForce(s32 wheel, s16 force, u16 duration)
{
    if (mWheels.IsConnected(wheel)) {
    if (mPlaying[wheel] != 0) {
        return;
    }

    if (mConstant.mBytes[wheel][0] != 0) {
        if (SameConstantForceParams(wheel, force, duration)) {
            return;
        }
        if (mConstant.UpdateForce(wheel, 0, 0xFFFFFFFF, 0, force, duration, 0, 0, 0, 0) >= 0) {
            mConstantP[wheel].force = force;
            mConstantP[wheel].duration = duration;
        }
        return;
    }

    if ((u32)mConstant.mIds[wheel][0] == 0xFFFFFFFFu) {
        if (mConstant.DownloadForce(wheel, 0, mWheels.mEffectId[wheel], 0xFFFFFFFF, 0, force, duration, 0, 0, 0, 0) >= 0) {
            mConstantP[wheel].force = force;
            mConstantP[wheel].duration = duration;
        }
        mConstant.Start(wheel, 0);
        return;
    }

    if (SameConstantForceParams(wheel, force, duration)) {
        mConstant.Start(wheel, 0);
        return;
    }

    if (mConstant.UpdateForce(wheel, 0, 0xFFFFFFFF, 0, force, duration, 0, 0, 0, 0) >= 0) {
        mConstantP[wheel].force = force;
        mConstantP[wheel].duration = duration;
    }
    mConstant.Start(wheel, 0);
    return;
    }
    OSReport("ERROR: trying to play a force on channel %d but no wheel opened.\n", wheel);
}

void LGWheels::StopConstantForce(s32 wheel)
{
    StopForce(wheel, 1);
}

bool LGWheels::SameConstantForceParams(s32 wheel, s16 force, u16 duration)
{
    return mConstantP[wheel].force == force && mConstantP[wheel].duration == duration;
}

void LGWheels::PlayDamperForce(s32 wheel, s16 force)
{
    if (mCondition.mBytes[wheel][2] != 0) {
        return;
    }
    if (mWheels.IsConnected(wheel)) {
    if (mPlaying[wheel] != 0) {
        return;
    }

    if (mCondition.mBytes[wheel][1] != 0) {
        if (SameDamperForceParams(wheel, force)) {
            return;
        }
        if (mCondition.UpdateForce(wheel, 1, 8, 0xFFFFFFFF, 0, 0, 0, 0xFF, 0xFF, force, force) >= 0) {
            mDamper[wheel] = force;
        }
        return;
    }

    if ((u32)mCondition.mIds[wheel][1] == 0xFFFFFFFFu) {
        if (mCondition.DownloadForce(wheel, 1, mWheels.mEffectId[wheel], 8, 0xFFFFFFFF, 0, 0, 0, 0xFF, 0xFF, force, force) >= 0) {
            mDamper[wheel] = force;
        }
        mCondition.Start(wheel, 1);
        return;
    }

    if (SameDamperForceParams(wheel, force)) {
        mCondition.Start(wheel, 1);
        return;
    }

    if (mCondition.UpdateForce(wheel, 1, 8, 0xFFFFFFFF, 0, 0, 0, 0xFF, 0xFF, force, force) >= 0) {
        mDamper[wheel] = force;
    }
    mCondition.Start(wheel, 1);
    return;
    }
    OSReport("ERROR: trying to play a force on channel %d but no wheel opened.\n", wheel);
}

void LGWheels::StopDamperForce(s32 wheel)
{
    StopForce(wheel, 2);
}

bool LGWheels::SameDamperForceParams(s32 wheel, s16 force)
{
    return mDamper[wheel] == force;
}

void LGWheels::PlaySideCollisionForce(s32 wheel, s16 force, u16 duration)
{
}

bool LGWheels::SameSideCollisionForceParams(s32 wheel, s16 force, u16 duration)
{
    return false;
}

void LGWheels::PlayFrontalCollisionForce(s32 wheel, u8 force)
{
}

bool LGWheels::SameFrontalCollisionForceParams(s32 wheel, s16 force)
{
    return false;
}

void LGWheels::PlayDirtRoadEffect(s32 wheel, u8 magnitude)
{
    if (mWheels.IsConnected(wheel)) {
    if (mPlaying[wheel] != 0) {
        return;
    }

    if (mPeriodic.mBytes[wheel][1] != 0) {
        if (SameDirtRoadEffectParams(wheel, magnitude)) {
            return;
        }
        if (mPeriodic.UpdateForce(wheel, 1, 2, 0xFFFFFFFF, 0, magnitude, 90, 65, 0, 0, 0, 0, 0, 0) >= 0) {
            mDirt[wheel] = magnitude;
        }
        return;
    }

    if ((u32)mPeriodic.mIds[wheel][1] == 0xFFFFFFFFu) {
        if (mPeriodic.DownloadForce(wheel, 1, mWheels.mEffectId[wheel], 2, 0xFFFFFFFF, 0, magnitude, 90, 65, 0, 0, 0, 0, 0, 0) >= 0) {
            mDirt[wheel] = magnitude;
        }
        mPeriodic.Start(wheel, 1);
        return;
    }

    if (SameDirtRoadEffectParams(wheel, magnitude)) {
        mPeriodic.Start(wheel, 1);
        return;
    }

    if (mPeriodic.UpdateForce(wheel, 1, 2, 0xFFFFFFFF, 0, magnitude, 90, 65, 0, 0, 0, 0, 0, 0) >= 0) {
        mDirt[wheel] = magnitude;
    }
    mPeriodic.Start(wheel, 1);
    return;
    }
    OSReport("ERROR: trying to play a force on channel %d but no wheel opened.\n", wheel);
}

void LGWheels::StopDirtRoadEffect(s32 wheel)
{
    StopForce(wheel, 5);
}

bool LGWheels::SameDirtRoadEffectParams(s32 wheel, s16 magnitude)
{
    return mDirt[wheel] == magnitude;
}

void LGWheels::PlayBumpyRoadEffect(s32 wheel, u8 magnitude)
{
    if (mWheels.IsConnected(wheel)) {
    if (mPlaying[wheel] != 0) {
        return;
    }

    if (mPeriodic.mBytes[wheel][2] != 0) {
        if (SameBumpyRoadEffectParams(wheel, magnitude)) {
            return;
        }
        if (mPeriodic.UpdateForce(wheel, 2, 3, 0xFFFFFFFF, 0, magnitude, 90, 100, 0, 0, 0, 0, 0, 0) >= 0) {
            mBumpy[wheel] = magnitude;
        }
        return;
    }

    if ((u32)mPeriodic.mIds[wheel][2] == 0xFFFFFFFFu) {
        if (mPeriodic.DownloadForce(wheel, 2, mWheels.mEffectId[wheel], 3, 0xFFFFFFFF, 0, magnitude, 90, 100, 0, 0, 0, 0, 0, 0) >= 0) {
            mBumpy[wheel] = magnitude;
        }
        mPeriodic.Start(wheel, 2);
        return;
    }

    if (SameBumpyRoadEffectParams(wheel, magnitude)) {
        mPeriodic.Start(wheel, 2);
        return;
    }

    if (mPeriodic.UpdateForce(wheel, 2, 3, 0xFFFFFFFF, 0, magnitude, 90, 100, 0, 0, 0, 0, 0, 0) >= 0) {
        mBumpy[wheel] = magnitude;
    }
    mPeriodic.Start(wheel, 2);
    return;
    }
    OSReport("ERROR: trying to play a force on channel %d but no wheel opened.\n", wheel);
}

void LGWheels::StopBumpyRoadEffect(s32 wheel)
{
    StopForce(wheel, 6);
}

bool LGWheels::SameBumpyRoadEffectParams(s32 wheel, s16 magnitude)
{
    return mBumpy[wheel] == magnitude;
}

void LGWheels::PlaySlipperyRoadEffect(s32 wheel, s16 force)
{
    if (IsPlaying(wheel, 2)) {
        StopForce(wheel, 2);
        mCondition.mBytes[wheel][1] = 0;
        mDamperSuppress[wheel] = 1;
    }
    if (IsPlaying(wheel, 0)) {
        StopForce(wheel, 0);
        mCondition.mBytes[wheel][0] = 0;
        mSpringSuppress[wheel] = 1;
    }

    if (mWheels.IsConnected(wheel)) {
    if (mPlaying[wheel] != 0) {
        return;
    }

    if (mCondition.mBytes[wheel][2] != 0) {
        if (SameSlipperyRoadEffectParams(wheel, force)) {
            return;
        }
        if (mCondition.UpdateForce(wheel, 2, 8, 0xFFFFFFFF, 0, 0, 0, 0xFF, 0xFF, -force, -force) >= 0) {
            mSlippery[wheel] = force;
        }
        return;
    }

    if ((u32)mCondition.mIds[wheel][2] == 0xFFFFFFFFu) {
        if (mCondition.DownloadForce(wheel, 2, mWheels.mEffectId[wheel], 8, 0xFFFFFFFF, 0, 0, 0, 0xFF, 0xFF, -force, -force) >= 0) {
            mSlippery[wheel] = force;
        }
        mCondition.Start(wheel, 2);
        return;
    }

    if (SameSlipperyRoadEffectParams(wheel, force)) {
        mCondition.Start(wheel, 2);
        return;
    }

    if (mCondition.UpdateForce(wheel, 2, 8, 0xFFFFFFFF, 0, 0, 0, 0xFF, 0xFF, -force, -force) >= 0) {
        mSlippery[wheel] = force;
    }
    mCondition.Start(wheel, 2);
    return;
    }
    OSReport("ERROR: trying to play a force on channel %d but no wheel opened.\n", wheel);
}

void LGWheels::StopSlipperyRoadEffect(s32 wheel)
{
    StopForce(wheel, 7);
}

bool LGWheels::SameSlipperyRoadEffectParams(s32 wheel, s16 force)
{
    return mSlippery[wheel] == force;
}

void LGWheels::PlaySurfaceEffect(s32 wheel, u8 type, u8 param, u16 duration)
{
    if (mWheels.IsConnected(wheel)) {
    if (mPlaying[wheel] != 0) {
        return;
    }

    if (mPeriodic.mBytes[wheel][3] != 0) {
        if (SameSurfaceEffectParams(wheel, type, param, duration)) {
            return;
        }
        s32 result;
        if (type != mSurface[wheel].type) {
            mPeriodic.Destroy(wheel, 3);
            result = mPeriodic.DownloadForce(wheel, 3, mWheels.mEffectId[wheel], type, 0xFFFFFFFF, 0, param, 90, duration, 0, 0, 0, 0, 0, 0);
            mPeriodic.Start(wheel, 3);
        } else {
            result = mPeriodic.UpdateForce(wheel, 3, type, 0xFFFFFFFF, 0, param, 90, duration, 0, 0, 0, 0, 0, 0);
        }
        if (result >= 0) {
            mSurface[wheel].type = type;
            mSurface[wheel].param = param;
            mSurface[wheel].duration = duration;
        }
        return;
    }

    if ((u32)mPeriodic.mIds[wheel][3] == 0xFFFFFFFFu) {
        if (mPeriodic.DownloadForce(wheel, 3, mWheels.mEffectId[wheel], type, 0xFFFFFFFF, 0, param, 90, duration, 0, 0, 0, 0, 0, 0) >= 0) {
            mSurface[wheel].type = type;
            mSurface[wheel].param = param;
            mSurface[wheel].duration = duration;
        }
        mPeriodic.Start(wheel, 3);
        return;
    }

    if (SameSurfaceEffectParams(wheel, type, param, duration)) {
        mPeriodic.Start(wheel, 3);
        return;
    }

    s32 result;
    if (type != mSurface[wheel].type) {
        mPeriodic.Destroy(wheel, 3);
        result = mPeriodic.DownloadForce(wheel, 3, mWheels.mEffectId[wheel], type, 0xFFFFFFFF, 0, param, 90, duration, 0, 0, 0, 0, 0, 0);
    } else {
        result = mPeriodic.UpdateForce(wheel, 3, type, 0xFFFFFFFF, 0, param, 90, duration, 0, 0, 0, 0, 0, 0);
    }
    if (result >= 0) {
        mSurface[wheel].type = type;
        mSurface[wheel].param = param;
        mSurface[wheel].duration = duration;
    }
    mPeriodic.Start(wheel, 3);
    return;
    }
    OSReport("ERROR: trying to play a force on channel %d but no wheel opened.\n", wheel);
}

void LGWheels::StopSurfaceEffect(s32 wheel)
{
    StopForce(wheel, 8);
}

bool LGWheels::SameSurfaceEffectParams(s32 wheel, u8 type, u8 param, u16 duration)
{
    return mSurface[wheel].type == type
        && mSurface[wheel].param == param
        && mSurface[wheel].duration == duration;
}

void LGWheels::PlayCarAirborne(s32 wheel)
{
    if (mWheels.IsConnected(wheel)) {

    mPlaying[wheel] = 1;

    if (IsPlaying(wheel, 0)) {
        StopForce(wheel, 0);
        mAutoCalibFlags[wheel][0] = 1;
    }
    if (IsPlaying(wheel, 1)) {
        StopForce(wheel, 1);
        mAutoCalibFlags[wheel][1] = 1;
    }
    if (IsPlaying(wheel, 2)) {
        StopForce(wheel, 2);
        mAutoCalibFlags[wheel][2] = 1;
    }
    if (IsPlaying(wheel, 5)) {
        StopForce(wheel, 5);
        mAutoCalibFlags[wheel][5] = 1;
    }
    if (IsPlaying(wheel, 6)) {
        StopForce(wheel, 6);
        mAutoCalibFlags[wheel][6] = 1;
    }
    if (IsPlaying(wheel, 7)) {
        StopForce(wheel, 7);
        mAutoCalibFlags[wheel][7] = 1;
    }
    if (IsPlaying(wheel, 8)) {
        StopForce(wheel, 8);
        mAutoCalibFlags[wheel][8] = 1;
    }
    return;
    }
    OSReport("ERROR: trying to play a force on channel %d but no wheel opened.\n", wheel);
}

void LGWheels::StopCarAirborne(s32 wheel)
{
    StopForce(wheel, 9);
}
