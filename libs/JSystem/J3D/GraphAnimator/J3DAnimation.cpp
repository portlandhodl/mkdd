#include "types.h"
#include <dolphin/os.h>
#include "dolphin/os/OSFastCast.h"
#include "JSystem/J3D/J3DAnmColor.h"
#include "JSystem/J3D/J3DAnmTexPattern.h"
#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/J3D/J3DAnmTevRegKey.h"
#include "JSystem/J3D/J3DAnmTransform.h"
#include "JSystem/J3D/J3DAnmVtxColor.h"
#include "JSystem/J3D/J3DFrameCtrl.h"
#include "JSystem/J3D/J3DAnmCluster.h"
#include "JSystem/J3D/J3DModel.h"

void J3DFrameCtrl::init(s16 i_end)
{
    mAttr       = J3DAA_UNKNOWN_2;
    mState      = 0;
    mStartFrame = 0;
    mEndFrame   = i_end;
    mLoop       = 0;
    mRate       = 1.0f;
    mFrame      = 0.0f;
}

void J3DFrameCtrl::update()
{
    mState = 0;
    mFrame += mRate;
    switch (mAttr)
    {
    case J3DAA_UNKNOWN_0:
        if (mFrame < mStartFrame)
        {
            mFrame = mStartFrame;
            mRate  = 0.0f;
            mState |= 1;
        }
        if (mFrame >= mEndFrame)
        {
            mFrame = mEndFrame - 0.001f;
            mRate  = 0.0f;
            mState |= 1;
        }
        break;
    case J3DAA_UNKNOWN_1:
        if (mFrame < mStartFrame)
        {
            mFrame = mStartFrame;
            mRate  = 0.0f;
            mState |= 1;
        }
        if (mFrame >= mEndFrame)
        {
            mFrame = mStartFrame;
            mRate  = 0.0f;
            mState |= 1;
        }
        break;
    case J3DAA_UNKNOWN_2:
        while (mFrame < mStartFrame)
        {
            mState |= 2;
            if (mLoop - mStartFrame <= 0.0f)
            {
                break;
            }
            mFrame += mLoop - mStartFrame;
        }
        while (mFrame >= mEndFrame)
        {
            mState |= 2;
            if (mEndFrame - mLoop <= 0.0f)
            {
                break;
            }
            mFrame -= mEndFrame - mLoop;
        }
        break;
    case J3DAA_UNKNOWN_3:
        if (mFrame >= mEndFrame)
        {
            mFrame = mEndFrame - (mFrame - mEndFrame);
            mRate  = -mRate;
        }
        if (mFrame < mStartFrame)
        {
            mFrame = mStartFrame - (mFrame - mStartFrame);
            mRate  = 0.0f;
            mState |= 1;
        }
        break;
    case J3DAA_UNKNOWN_4:
        if (mFrame >= mEndFrame - 1.0f)
        {
            mFrame = (mEndFrame - 1.0f) - (mFrame - (mEndFrame - 1.0f));
            mRate  = -mRate;
        }
        if (mFrame < mStartFrame)
        {
            mFrame = mStartFrame - (mFrame - mStartFrame);
            mRate  = -mRate;
            mState |= 2;
        }
        break;
    }
}

J3DAnmTransform::J3DAnmTransform(s16 frameMax, f32* scale, s16* rot, f32* trans)
    : J3DAnmBase(frameMax)
    , mScaleVals(scale)
    , mRotationVals(rot)
    , mTranslationVals(trans)
    , _18(0)
    , _1A(0)
    , _1C(0)
    , _1E(0)
{
}

void J3DAnmTransformFull::getTransform(u16 jointNo, J3DTransformInfo* info) const
{
    u16 idx                     = jointNo * 3;
    J3DAnmTransformFullTable* xTable = &mTable[idx];
    J3DAnmTransformFullTable* yTable = &mTable[idx + 1];
    J3DAnmTransformFullTable* zTable = &mTable[idx + 2];

    if (getFrame() < 0.0f)
    {
        info->mScale.x = mScaleVals[xTable->mScaleOffset];
        info->mScale.y = mScaleVals[yTable->mScaleOffset];
        info->mScale.z = mScaleVals[zTable->mScaleOffset];

        info->mRotation.x = mRotationVals[xTable->mRotationOffset];
        info->mRotation.y = mRotationVals[yTable->mRotationOffset];
        info->mRotation.z = mRotationVals[zTable->mRotationOffset];

        info->mTranslation.x = mTranslationVals[xTable->mTranslationOffset];
        info->mTranslation.y = mTranslationVals[yTable->mTranslationOffset];
        info->mTranslation.z = mTranslationVals[zTable->mTranslationOffset];
    }
    else
    {
        u32 maxFrame = (int)(getFrame() + 0.5f);

        if (maxFrame >= xTable->mScaleMaxFrame)
            info->mScale.x = mScaleVals[xTable->mScaleMaxFrame - 1 + xTable->mScaleOffset];
        else
            info->mScale.x = mScaleVals[xTable->mScaleOffset + maxFrame];

        if (maxFrame >= xTable->mRotationMaxFrame)
            info->mRotation.x = mRotationVals[xTable->mRotationMaxFrame - 1 + xTable->mRotationOffset];
        else
            info->mRotation.x = mRotationVals[xTable->mRotationOffset + maxFrame];

        if (maxFrame >= xTable->mTranslationMaxFrame)
            info->mTranslation.x = mTranslationVals[xTable->mTranslationMaxFrame - 1 + xTable->mTranslationOffset];
        else
            info->mTranslation.x = mTranslationVals[xTable->mTranslationOffset + maxFrame];

        if (maxFrame >= yTable->mScaleMaxFrame)
            info->mScale.y = mScaleVals[yTable->mScaleMaxFrame - 1 + yTable->mScaleOffset];
        else
            info->mScale.y = mScaleVals[yTable->mScaleOffset + maxFrame];

        if (maxFrame >= yTable->mRotationMaxFrame)
            info->mRotation.y = mRotationVals[yTable->mRotationMaxFrame - 1 + yTable->mRotationOffset];
        else
            info->mRotation.y = mRotationVals[yTable->mRotationOffset + maxFrame];

        if (maxFrame >= yTable->mTranslationMaxFrame)
            info->mTranslation.y = mTranslationVals[yTable->mTranslationMaxFrame - 1 + yTable->mTranslationOffset];
        else
            info->mTranslation.y = mTranslationVals[yTable->mTranslationOffset + maxFrame];

        if (maxFrame >= zTable->mScaleMaxFrame)
            info->mScale.z = mScaleVals[zTable->mScaleMaxFrame - 1 + zTable->mScaleOffset];
        else
            info->mScale.z = mScaleVals[zTable->mScaleOffset + maxFrame];

        if (maxFrame >= zTable->mRotationMaxFrame)
            info->mRotation.z = mRotationVals[zTable->mRotationMaxFrame - 1 + zTable->mRotationOffset];
        else
            info->mRotation.z = mRotationVals[zTable->mRotationOffset + maxFrame];

        if (maxFrame >= zTable->mTranslationMaxFrame)
            info->mTranslation.z = mTranslationVals[zTable->mTranslationMaxFrame - 1 + zTable->mTranslationOffset];
        else
            info->mTranslation.z = mTranslationVals[zTable->mTranslationOffset + maxFrame];
    }
}

void J3DAnmTransformFullWithLerp::getTransform(u16 jointNo, J3DTransformInfo* info) const
{
    u16 idx                     = jointNo * 3;
    J3DAnmTransformFullTable* xTable = &mTable[idx];
    J3DAnmTransformFullTable* yTable = &mTable[idx + 1];
    J3DAnmTransformFullTable* zTable = &mTable[idx + 2];

    if (getFrame() < 0.0f)
    {
        info->mScale.x = mScaleVals[xTable->mScaleOffset];
        info->mScale.y = mScaleVals[yTable->mScaleOffset];
        info->mScale.z = mScaleVals[zTable->mScaleOffset];

        info->mRotation.x = mRotationVals[xTable->mRotationOffset];
        info->mRotation.y = mRotationVals[yTable->mRotationOffset];
        info->mRotation.z = mRotationVals[zTable->mRotationOffset];

        info->mTranslation.x = mTranslationVals[xTable->mTranslationOffset];
        info->mTranslation.y = mTranslationVals[yTable->mTranslationOffset];
        info->mTranslation.z = mTranslationVals[zTable->mTranslationOffset];
    }
    else
    {
        int frame = (int)getFrame();

        if (frame == getFrame())
        {
            u32 maxFrame;

            maxFrame = xTable->mScaleMaxFrame;
            if (frame >= maxFrame)
                info->mScale.x = mScaleVals[xTable->mScaleMaxFrame - 1 + xTable->mScaleOffset];
            else
                info->mScale.x = mScaleVals[xTable->mScaleOffset + frame];

            maxFrame = xTable->mRotationMaxFrame;
            if (frame >= maxFrame)
                info->mRotation.x = mRotationVals[xTable->mRotationMaxFrame - 1 + xTable->mRotationOffset];
            else
                info->mRotation.x = mRotationVals[xTable->mRotationOffset + frame];

            maxFrame = xTable->mTranslationMaxFrame;
            if (frame >= maxFrame)
                info->mTranslation.x = mTranslationVals[xTable->mTranslationMaxFrame - 1 + xTable->mTranslationOffset];
            else
                info->mTranslation.x = mTranslationVals[xTable->mTranslationOffset + frame];

            maxFrame = yTable->mScaleMaxFrame;
            if (frame >= maxFrame)
                info->mScale.y = mScaleVals[yTable->mScaleMaxFrame - 1 + yTable->mScaleOffset];
            else
                info->mScale.y = mScaleVals[yTable->mScaleOffset + frame];

            maxFrame = yTable->mRotationMaxFrame;
            if (frame >= maxFrame)
                info->mRotation.y = mRotationVals[yTable->mRotationMaxFrame - 1 + yTable->mRotationOffset];
            else
                info->mRotation.y = mRotationVals[yTable->mRotationOffset + frame];

            maxFrame = yTable->mTranslationMaxFrame;
            if (frame >= maxFrame)
                info->mTranslation.y = mTranslationVals[yTable->mTranslationMaxFrame - 1 + yTable->mTranslationOffset];
            else
                info->mTranslation.y = mTranslationVals[yTable->mTranslationOffset + frame];

            maxFrame = zTable->mScaleMaxFrame;
            if (frame >= maxFrame)
                info->mScale.z = mScaleVals[zTable->mScaleMaxFrame - 1 + zTable->mScaleOffset];
            else
                info->mScale.z = mScaleVals[zTable->mScaleOffset + frame];

            maxFrame = zTable->mRotationMaxFrame;
            if (frame >= maxFrame)
                info->mRotation.z = mRotationVals[zTable->mRotationMaxFrame - 1 + zTable->mRotationOffset];
            else
                info->mRotation.z = mRotationVals[zTable->mRotationOffset + frame];

            maxFrame = zTable->mTranslationMaxFrame;
            if (frame >= maxFrame)
                info->mTranslation.z = mTranslationVals[zTable->mTranslationMaxFrame - 1 + zTable->mTranslationOffset];
            else
                info->mTranslation.z = mTranslationVals[zTable->mTranslationOffset + frame];
        }
        else
        {
            f32 rate = getFrame() - frame;
            u32 nextFrame = frame + 1;
            u32 maxFrame;

            maxFrame = xTable->mScaleMaxFrame;
            if (nextFrame >= maxFrame)
                info->mScale.x = mScaleVals[xTable->mScaleMaxFrame - 1 + xTable->mScaleOffset];
            else
                info->mScale.x = mScaleVals[xTable->mScaleOffset + frame]
                               + rate * (mScaleVals[xTable->mScaleOffset + nextFrame] - mScaleVals[xTable->mScaleOffset + frame]);

            maxFrame = xTable->mRotationMaxFrame;
            if (nextFrame >= maxFrame)
                info->mRotation.x = mRotationVals[xTable->mRotationMaxFrame - 1 + xTable->mRotationOffset];
            else
            {
                u32 rot1  = (u16)mRotationVals[xTable->mRotationOffset + frame];
                u32 rot2  = (u16)mRotationVals[xTable->mRotationOffset + nextFrame];
                int delta = rot2 - rot1;
                if (delta > 0x8000)
                {
                    rot1 += 0x10000;
                    delta -= 0x10000;
                }
                else if (-delta > 0x8000)
                {
                    delta += 0x10000;
                }
                info->mRotation.x = (u32)((f32)rot1 + rate * (f32)delta);
            }

            maxFrame = xTable->mTranslationMaxFrame;
            if (nextFrame >= maxFrame)
                info->mTranslation.x = mTranslationVals[xTable->mTranslationMaxFrame - 1 + xTable->mTranslationOffset];
            else
                info->mTranslation.x = mTranslationVals[xTable->mTranslationOffset + frame]
                                     + rate * (mTranslationVals[xTable->mTranslationOffset + nextFrame] - mTranslationVals[xTable->mTranslationOffset + frame]);

            maxFrame = yTable->mScaleMaxFrame;
            if (nextFrame >= maxFrame)
                info->mScale.y = mScaleVals[yTable->mScaleMaxFrame - 1 + yTable->mScaleOffset];
            else
                info->mScale.y = mScaleVals[yTable->mScaleOffset + frame]
                               + rate * (mScaleVals[yTable->mScaleOffset + nextFrame] - mScaleVals[yTable->mScaleOffset + frame]);

            maxFrame = yTable->mRotationMaxFrame;
            if (nextFrame >= maxFrame)
                info->mRotation.y = mRotationVals[yTable->mRotationMaxFrame - 1 + yTable->mRotationOffset];
            else
            {
                u32 rot1  = (u16)mRotationVals[yTable->mRotationOffset + frame];
                u32 rot2  = (u16)mRotationVals[yTable->mRotationOffset + nextFrame];
                int delta = rot2 - rot1;
                if (delta > 0x8000)
                {
                    rot1 += 0x10000;
                    delta -= 0x10000;
                }
                else if (-delta > 0x8000)
                {
                    delta += 0x10000;
                }
                info->mRotation.y = (u32)((f32)rot1 + rate * (f32)delta);
            }

            maxFrame = yTable->mTranslationMaxFrame;
            if (nextFrame >= maxFrame)
                info->mTranslation.y = mTranslationVals[yTable->mTranslationMaxFrame - 1 + yTable->mTranslationOffset];
            else
                info->mTranslation.y = mTranslationVals[yTable->mTranslationOffset + frame]
                                     + rate * (mTranslationVals[yTable->mTranslationOffset + nextFrame] - mTranslationVals[yTable->mTranslationOffset + frame]);

            maxFrame = zTable->mScaleMaxFrame;
            if (nextFrame >= maxFrame)
                info->mScale.z = mScaleVals[zTable->mScaleMaxFrame - 1 + zTable->mScaleOffset];
            else
                info->mScale.z = mScaleVals[zTable->mScaleOffset + frame]
                               + rate * (mScaleVals[zTable->mScaleOffset + nextFrame] - mScaleVals[zTable->mScaleOffset + frame]);

            maxFrame = zTable->mRotationMaxFrame;
            if (nextFrame >= maxFrame)
                info->mRotation.z = mRotationVals[zTable->mRotationMaxFrame - 1 + zTable->mRotationOffset];
            else
            {
                u32 rot1  = (u16)mRotationVals[zTable->mRotationOffset + frame];
                u32 rot2  = (u16)mRotationVals[zTable->mRotationOffset + nextFrame];
                int delta = rot2 - rot1;
                if (delta > 0x8000)
                {
                    rot1 += 0x10000;
                    delta -= 0x10000;
                }
                else if (-delta > 0x8000)
                {
                    delta += 0x10000;
                }
                info->mRotation.z = (u32)((f32)rot1 + rate * (f32)delta);
            }

            maxFrame = zTable->mTranslationMaxFrame;
            if (nextFrame >= maxFrame)
                info->mTranslation.z = mTranslationVals[zTable->mTranslationMaxFrame - 1 + zTable->mTranslationOffset];
            else
                info->mTranslation.z = mTranslationVals[zTable->mTranslationOffset + frame]
                                     + rate * (mTranslationVals[zTable->mTranslationOffset + nextFrame] - mTranslationVals[zTable->mTranslationOffset + frame]);
        }
    }
}

void J3DAnmTransformKey::calcTransform(f32 frame, u16 jointNo, J3DTransformInfo* info) const
{
    u16 idx                    = jointNo * 3;
    J3DAnmTransformKeyTable* xTable = &mTable[idx];
    J3DAnmTransformKeyTable* yTable = &mTable[idx + 1];
    J3DAnmTransformKeyTable* zTable = &mTable[idx + 2];

    switch (xTable->mScaleInfo.mMaxFrame)
    {
    case 0:
        info->mScale.x = 1.0f;
        break;
    case 1:
        info->mScale.x = mScaleVals[xTable->mScaleInfo.mOffset];
        break;
    default:
        info->mScale.x = J3DGetKeyFrameInterpolation(frame, &xTable->mScaleInfo, &mScaleVals[xTable->mScaleInfo.mOffset]);
        break;
    }

    switch (yTable->mScaleInfo.mMaxFrame)
    {
    case 0:
        info->mScale.y = 1.0f;
        break;
    case 1:
        info->mScale.y = mScaleVals[yTable->mScaleInfo.mOffset];
        break;
    default:
        info->mScale.y = J3DGetKeyFrameInterpolation(frame, &yTable->mScaleInfo, &mScaleVals[yTable->mScaleInfo.mOffset]);
        break;
    }

    switch (zTable->mScaleInfo.mMaxFrame)
    {
    case 0:
        info->mScale.z = 1.0f;
        break;
    case 1:
        info->mScale.z = mScaleVals[zTable->mScaleInfo.mOffset];
        break;
    default:
        info->mScale.z = J3DGetKeyFrameInterpolation(frame, &zTable->mScaleInfo, &mScaleVals[zTable->mScaleInfo.mOffset]);
        break;
    }

    switch (xTable->mRotationInfo.mMaxFrame)
    {
    case 0:
        info->mRotation.x = 0;
        break;
    case 1:
        info->mRotation.x = mRotationVals[xTable->mRotationInfo.mOffset] << _20;
        break;
    default:
        info->mRotation.x = (int)J3DGetKeyFrameInterpolation(frame, &xTable->mRotationInfo, &mRotationVals[xTable->mRotationInfo.mOffset]) << _20;
        break;
    }

    switch (yTable->mRotationInfo.mMaxFrame)
    {
    case 0:
        info->mRotation.y = 0;
        break;
    case 1:
        info->mRotation.y = mRotationVals[yTable->mRotationInfo.mOffset] << _20;
        break;
    default:
        info->mRotation.y = (int)J3DGetKeyFrameInterpolation(frame, &yTable->mRotationInfo, &mRotationVals[yTable->mRotationInfo.mOffset]) << _20;
        break;
    }

    switch (zTable->mRotationInfo.mMaxFrame)
    {
    case 0:
        info->mRotation.z = 0;
        break;
    case 1:
        info->mRotation.z = mRotationVals[zTable->mRotationInfo.mOffset] << _20;
        break;
    default:
        info->mRotation.z = (int)J3DGetKeyFrameInterpolation(frame, &zTable->mRotationInfo, &mRotationVals[zTable->mRotationInfo.mOffset]) << _20;
        break;
    }

    switch (xTable->mTranslationInfo.mMaxFrame)
    {
    case 0:
        info->mTranslation.x = 0.0f;
        break;
    case 1:
        info->mTranslation.x = mTranslationVals[xTable->mTranslationInfo.mOffset];
        break;
    default:
        info->mTranslation.x = J3DGetKeyFrameInterpolation(frame, &xTable->mTranslationInfo, &mTranslationVals[xTable->mTranslationInfo.mOffset]);
        break;
    }

    switch (yTable->mTranslationInfo.mMaxFrame)
    {
    case 0:
        info->mTranslation.y = 0.0f;
        break;
    case 1:
        info->mTranslation.y = mTranslationVals[yTable->mTranslationInfo.mOffset];
        break;
    default:
        info->mTranslation.y = J3DGetKeyFrameInterpolation(frame, &yTable->mTranslationInfo, &mTranslationVals[yTable->mTranslationInfo.mOffset]);
        break;
    }

    switch (zTable->mTranslationInfo.mMaxFrame)
    {
    case 0:
        info->mTranslation.z = 0.0f;
        break;
    case 1:
        info->mTranslation.z = mTranslationVals[zTable->mTranslationInfo.mOffset];
        break;
    default:
        info->mTranslation.z = J3DGetKeyFrameInterpolation(frame, &zTable->mTranslationInfo, &mTranslationVals[zTable->mTranslationInfo.mOffset]);
        break;
    }
}

J3DAnmTextureSRTKey::J3DAnmTextureSRTKey()
    : _30()
    , _64()
{
    _0C                = 0;
    _1A                = 0;
    _18                = 0;
    _16                = 0;
    mUpdateMaterialNum = 0;
    _10                = nullptr;
    _24                = nullptr;
    _1C                = nullptr;
    _20                = nullptr;
    _48                = 0;
    _46                = 0;
    _44                = 0;
    _4A                = 0;
    _58                = nullptr;
    _54                = nullptr;
    _4C                = nullptr;
    _50                = nullptr;
    mTexMtxCalcType    = 0;
}

void J3DAnmTextureSRTKey::calcTransform(f32 frame, u16 jointNo, J3DTextureSRTInfo* info) const
{
    u16 idx                     = jointNo * 3;
    J3DAnmTransformKeyTable* xTable   = &_10[idx];
    J3DAnmTransformKeyTable* yTable   = &_10[idx + 1];
    J3DAnmTransformKeyTable* rotTable = &_10[idx + 2];

    switch (xTable->mScaleInfo.mMaxFrame)
    {
    case 0:
        info->mScaleX = 1.0f;
        break;
    case 1:
        info->mScaleX = _1C[xTable->mScaleInfo.mOffset];
        break;
    default:
        info->mScaleX = J3DGetKeyFrameInterpolation(frame, &xTable->mScaleInfo, &_1C[xTable->mScaleInfo.mOffset]);
        break;
    }

    switch (yTable->mScaleInfo.mMaxFrame)
    {
    case 0:
        info->mScaleY = 1.0f;
        break;
    case 1:
        info->mScaleY = _1C[yTable->mScaleInfo.mOffset];
        break;
    default:
        info->mScaleY = J3DGetKeyFrameInterpolation(frame, &yTable->mScaleInfo, &_1C[yTable->mScaleInfo.mOffset]);
        break;
    }

    switch (rotTable->mRotationInfo.mMaxFrame)
    {
    case 0:
        info->mRotation = 0;
        break;
    case 1:
        info->mRotation = _20[rotTable->mRotationInfo.mOffset] << _0C;
        break;
    default:
        info->mRotation = (int)J3DGetKeyFrameInterpolation(frame, &rotTable->mRotationInfo, &_20[rotTable->mRotationInfo.mOffset]) << _0C;
        break;
    }

    switch (xTable->mTranslationInfo.mMaxFrame)
    {
    case 0:
        info->mTranslationX = 0.0f;
        break;
    case 1:
        info->mTranslationX = _24[xTable->mTranslationInfo.mOffset];
        break;
    default:
        info->mTranslationX = J3DGetKeyFrameInterpolation(frame, &xTable->mTranslationInfo, &_24[xTable->mTranslationInfo.mOffset]);
        break;
    }

    switch (yTable->mTranslationInfo.mMaxFrame)
    {
    case 0:
        info->mTranslationY = 0.0f;
        break;
    case 1:
        info->mTranslationY = _24[yTable->mTranslationInfo.mOffset];
        break;
    default:
        info->mTranslationY = J3DGetKeyFrameInterpolation(frame, &yTable->mTranslationInfo, &_24[yTable->mTranslationInfo.mOffset]);
        break;
    }
}

f32 J3DAnmClusterFull::getWeight(u16 index) const
{
    u16 maxFrame = mTables[index]._00[0];
    int frame    = (int)(getFrame() + 0.5f);

    if (getFrame() < 0.0f)
        return _0C[mTables[index]._00[1]];
    else if (frame >= maxFrame)
        return _0C[mTables[index]._00[1] + (maxFrame - 1)];
    else
        return _0C[mTables[index]._00[1] + frame];
}

f32 J3DAnmClusterKey::getWeight(u16 index) const
{
    int maxFrame = (u16)mTables[index].mMaxFrame;
    switch (maxFrame)
    {
    case 0:
        return 1.0f;
    case 1:
        return _0C[mTables[index].mOffset];
    default:
        return J3DGetKeyFrameInterpolation(getFrame(), &mTables[index], &_0C[mTables[index].mOffset]);
    }
}

J3DAnmVtxColorFull::J3DAnmVtxColorFull()
{
    mTable[0] = nullptr;
    mTable[1] = nullptr;
}

void J3DAnmVtxColorFull::getColor(u8 tableNo, u16 index, GXColor* pColor) const
{
    J3DAnmColorFullTable* entry = &mTable[tableNo][index];

    if (getFrame() < 0.0f)
    {
        pColor->r = mRedVals[entry->mData[0][1]];
        pColor->g = mGreenVals[entry->mData[1][1]];
        pColor->b = mBlueVals[entry->mData[2][1]];
        pColor->a = mAlphaVals[entry->mData[3][1]];
    }
    else
    {
        int frame = (int)(getFrame() + 0.5f);
        u16 maxFrame;
        maxFrame = entry->mData[0][0];
        if (frame >= maxFrame)
            pColor->r = mRedVals[entry->mData[0][1] + maxFrame - 1];
        else
            pColor->r = mRedVals[entry->mData[0][1] + frame];

        maxFrame = entry->mData[1][0];
        if (frame >= maxFrame)
            pColor->g = mGreenVals[entry->mData[1][1] + maxFrame - 1];
        else
            pColor->g = mGreenVals[entry->mData[1][1] + frame];

        maxFrame = entry->mData[2][0];
        if (frame >= maxFrame)
            pColor->b = mBlueVals[entry->mData[2][1] + maxFrame - 1];
        else
            pColor->b = mBlueVals[entry->mData[2][1] + frame];

        maxFrame = entry->mData[3][0];
        if (frame >= maxFrame)
            pColor->a = mAlphaVals[entry->mData[3][1] + maxFrame - 1];
        else
            pColor->a = mAlphaVals[entry->mData[3][1] + frame];
    }
}

J3DAnmVtxColorKey::J3DAnmVtxColorKey()
{
    mTable[0] = nullptr;
    mTable[1] = nullptr;
}

void J3DAnmVtxColorKey::getColor(u8 tableNo, u16 index, GXColor* pColor) const
{
    J3DAnmColorKeyTable* entry = &mTable[tableNo][index];

    f32 col;
    switch (entry->mColorInfo[0].mMaxFrame)
    {
    case 0:
        pColor->r = 0;
        break;
    case 1:
        pColor->r = mRedVals[entry->mColorInfo[0].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mColorInfo[0], &mRedVals[entry->mColorInfo[0].mOffset]);
        if (col <= 0.0f)
            pColor->r = 0;
        else if (col <= 255.0f)
            OSf32tou8(&col, &pColor->r);
        else
            pColor->r = 255;
        break;
    }

    switch (entry->mColorInfo[1].mMaxFrame)
    {
    case 0:
        pColor->g = 0;
        break;
    case 1:
        pColor->g = mGreenVals[entry->mColorInfo[1].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mColorInfo[1], &mGreenVals[entry->mColorInfo[1].mOffset]);
        if (col <= 0.0f)
            pColor->g = 0;
        else if (col <= 255.0f)
            OSf32tou8(&col, &pColor->g);
        else
            pColor->g = 255;
        break;
    }

    switch (entry->mColorInfo[2].mMaxFrame)
    {
    case 0:
        pColor->b = 0;
        break;
    case 1:
        pColor->b = mBlueVals[entry->mColorInfo[2].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mColorInfo[2], &mBlueVals[entry->mColorInfo[2].mOffset]);
        if (col <= 0.0f)
            pColor->b = 0;
        else if (col <= 255.0f)
            OSf32tou8(&col, &pColor->b);
        else
            pColor->b = 255;
        break;
    }

    switch (entry->mColorInfo[3].mMaxFrame)
    {
    case 0:
        pColor->a = 0;
        break;
    case 1:
        pColor->a = mAlphaVals[entry->mColorInfo[3].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mColorInfo[3], &mAlphaVals[entry->mColorInfo[3].mOffset]);
        if (col <= 0.0f)
            pColor->a = 0;
        else if (col <= 255.0f)
            OSf32tou8(&col, &pColor->a);
        else
            pColor->a = 255;
        break;
    }
}

void J3DAnmColor::searchUpdateMaterialID(J3DModelData* modelData)
{
    for (u16 i = 0; i < mUpdateMaterialNum; i++)
    {
        int index = modelData->getMaterialTable().getMaterialName()->getIndex(mNameTab.getName(i));
        if (index != -1)
        {
            mUpdateMaterialID[i] = index;
        }
        else
        {
            mUpdateMaterialID[i] = 0xFFFF;
        }
    }
}

J3DAnmColorFull::J3DAnmColorFull()
    : _2C(nullptr)
    , _30(nullptr)
    , _34(nullptr)
    , _38(nullptr)
    , mTable(nullptr)
{
}

void J3DAnmColorFull::getColor(u16 index, GXColor* pColor) const
{
    J3DAnmColorFullTable* entry = &mTable[index];

    if (getFrame() < 0.0f)
    {
        pColor->r = _2C[entry->mData[0][1]];
        pColor->g = _30[entry->mData[1][1]];
        pColor->b = _34[entry->mData[2][1]];
        pColor->a = _38[entry->mData[3][1]];
    }
    else
    {
        int frame = (int)(getFrame() + 0.5f);
        u16 maxFrame;
        maxFrame = entry->mData[0][0];
        if (frame >= maxFrame)
            pColor->r = _2C[entry->mData[0][1] + maxFrame - 1];
        else
            pColor->r = _2C[entry->mData[0][1] + frame];

        maxFrame = entry->mData[1][0];
        if (frame >= maxFrame)
            pColor->g = _30[entry->mData[1][1] + maxFrame - 1];
        else
            pColor->g = _30[entry->mData[1][1] + frame];

        maxFrame = entry->mData[2][0];
        if (frame >= maxFrame)
            pColor->b = _34[entry->mData[2][1] + maxFrame - 1];
        else
            pColor->b = _34[entry->mData[2][1] + frame];

        maxFrame = entry->mData[3][0];
        if (frame >= maxFrame)
            pColor->a = _38[entry->mData[3][1] + maxFrame - 1];
        else
            pColor->a = _38[entry->mData[3][1] + frame];
    }
}

J3DAnmColorKey::J3DAnmColorKey()
    : _2C(nullptr)
    , _30(nullptr)
    , _34(nullptr)
    , _38(nullptr)
    , mTable(nullptr)
{
}

void J3DAnmColorKey::getColor(u16 index, GXColor* pColor) const
{
    J3DAnmColorKeyTable* entry = &mTable[index];

    f32 col;
    switch (entry->mColorInfo[0].mMaxFrame)
    {
    case 0:
        pColor->r = 0;
        break;
    case 1:
        pColor->r = _2C[entry->mColorInfo[0].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mColorInfo[0], &_2C[entry->mColorInfo[0].mOffset]);
        if (col < 0.0f)
            pColor->r = 0;
        else if (col > 255.0f)
            pColor->r = 255;
        else
            OSf32tou8(&col, &pColor->r);
        break;
    }

    switch (entry->mColorInfo[1].mMaxFrame)
    {
    case 0:
        pColor->g = 0;
        break;
    case 1:
        pColor->g = _30[entry->mColorInfo[1].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mColorInfo[1], &_30[entry->mColorInfo[1].mOffset]);
        if (col < 0.0f)
            pColor->g = 0;
        else if (col > 255.0f)
            pColor->g = 255;
        else
            OSf32tou8(&col, &pColor->g);
        break;
    }

    switch (entry->mColorInfo[2].mMaxFrame)
    {
    case 0:
        pColor->b = 0;
        break;
    case 1:
        pColor->b = _34[entry->mColorInfo[2].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mColorInfo[2], &_34[entry->mColorInfo[2].mOffset]);
        if (col < 0.0f)
            pColor->b = 0;
        else if (col > 255.0f)
            pColor->b = 255;
        else
            OSf32tou8(&col, &pColor->b);
        break;
    }

    switch (entry->mColorInfo[3].mMaxFrame)
    {
    case 0:
        pColor->a = 0;
        break;
    case 1:
        pColor->a = _38[entry->mColorInfo[3].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mColorInfo[3], &_38[entry->mColorInfo[3].mOffset]);
        if (col < 0.0f)
            pColor->a = 0;
        else if (col > 255.0f)
            pColor->a = 255;
        else
            OSf32tou8(&col, &pColor->a);
        break;
    }
}

J3DAnmTevRegKey::J3DAnmTevRegKey()
    : mCRegNameTable()
    , mKRegNameTable()
{
    mKRegUpdateMaterialNum = 0;
    mCRegUpdateMaterialNum = 0;
    _16                    = 0;
    _14                    = 0;
    _12                    = 0;
    _10                    = 0;
    _1E                    = 0;
    _1C                    = 0;
    _1A                    = 0;
    _18                    = 0;
    mKRegUpdateMaterialID  = nullptr;
    mCRegUpdateMaterialID  = nullptr;
    mCAlphaVals            = nullptr;
    mCBlueVals             = nullptr;
    mCGreenVals            = nullptr;
    mCRedVals              = nullptr;
    mKAlphaVals            = nullptr;
    mKBlueVals             = nullptr;
    mKGreenVals            = nullptr;
    mKRedVals              = nullptr;
}

J3DAnmTexPattern::J3DAnmTexPattern()
    : _0C(nullptr)
    , mAnmTable(nullptr)
    , _14(0)
    , mUpdateMaterialNum(0)
    , mUpdateMaterialID(nullptr)
    , mNameTab()
{
}

void J3DAnmTexPattern::getTexNo(u16 index, u16* texNo) const
{
    u16 maxFrame = mAnmTable[index].mData[0][0];

    if (getFrame() < 0.0f)
        *texNo = _0C[mAnmTable[index].mData[0][1]];
    else if (getFrame() >= maxFrame)
        *texNo = _0C[mAnmTable[index].mData[0][1] + (maxFrame - 1)];
    else
        *texNo = _0C[(int)mCurrentFrame + mAnmTable[index].mData[0][1]];
}

void J3DAnmTexPattern::searchUpdateMaterialID(J3DModelData* modelData)
{
    for (u16 i = 0; i < mUpdateMaterialNum; i++)
    {
        int index = modelData->getMaterialTable().getMaterialName()->getIndex(mNameTab.getName(i));
        if (index != -1)
        {
            mUpdateMaterialID[i] = index;
        }
        else
        {
            mUpdateMaterialID[i] = 0xFFFF;
        }
    }
}

void J3DAnmTextureSRTKey::searchUpdateMaterialID(J3DModelData* modelData)
{
    for (u16 i = 0; i < mUpdateMaterialNum / 3; i++)
    {
        int index = modelData->getMaterialTable().getMaterialName()->getIndex(_30.getName(i));
        if (index != -1)
        {
            mUpdateMaterialID[i] = index;
        }
        else
        {
            mUpdateMaterialID[i] = 0xFFFF;
        }
    }

    for (u16 i = 0; i < _4A / 3; i++)
    {
        int index = modelData->getMaterialTable().getMaterialName()->getIndex(_64.getName(i));
        if (index != -1)
        {
            _60[i] = index;
        }
        else
        {
            _60[i] = 0xFFFF;
        }
    }
}

void J3DAnmTevRegKey::getTevColorReg(u16 index, GXColorS10* pColor) const
{
    J3DAnmCRegKeyTable* entry = &mCRegKeyTable[index];

    f32 col;
    switch (entry->mTables[0].mMaxFrame)
    {
    case 0:
        pColor->r = 0;
        break;
    case 1:
        pColor->r = mCRedVals[entry->mTables[0].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mTables[0], &mCRedVals[entry->mTables[0].mOffset]);
        if (col < -0x400)
            pColor->r = -0x400;
        else if (col > 0x3FF)
            pColor->r = 0x3FF;
        else
            OSf32tos16(&col, &pColor->r);
        break;
    }

    switch (entry->mTables[1].mMaxFrame)
    {
    case 0:
        pColor->g = 0;
        break;
    case 1:
        pColor->g = mCGreenVals[entry->mTables[1].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mTables[1], &mCGreenVals[entry->mTables[1].mOffset]);
        if (col < -0x400)
            pColor->g = -0x400;
        else if (col > 0x3FF)
            pColor->g = 0x3FF;
        else
            OSf32tos16(&col, &pColor->g);
        break;
    }

    switch (entry->mTables[2].mMaxFrame)
    {
    case 0:
        pColor->b = 0;
        break;
    case 1:
        pColor->b = mCBlueVals[entry->mTables[2].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mTables[2], &mCBlueVals[entry->mTables[2].mOffset]);
        if (col < -0x400)
            pColor->b = -0x400;
        else if (col > 0x3FF)
            pColor->b = 0x3FF;
        else
            OSf32tos16(&col, &pColor->b);
        break;
    }

    switch (entry->mTables[3].mMaxFrame)
    {
    case 0:
        pColor->a = 0;
        break;
    case 1:
        pColor->a = mCAlphaVals[entry->mTables[3].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mTables[3], &mCAlphaVals[entry->mTables[3].mOffset]);
        if (col < -0x400)
            pColor->a = -0x400;
        else if (col > 0x3FF)
            pColor->a = 0x3FF;
        else
            OSf32tos16(&col, &pColor->a);
        break;
    }
}

void J3DAnmTevRegKey::getTevKonstReg(u16 index, GXColor* pColor) const
{
    J3DAnmKRegKeyTable* entry = &mKRegKeyTable[index];

    f32 col;
    switch (entry->mTables[0].mMaxFrame)
    {
    case 0:
        pColor->r = 0;
        break;
    case 1:
        pColor->r = mKRedVals[entry->mTables[0].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mTables[0], &mKRedVals[entry->mTables[0].mOffset]);
        if (col < 0)
            pColor->r = 0;
        else if (col > 0xFF)
            pColor->r = 0xFF;
        else
            OSf32tou8(&col, &pColor->r);
        break;
    }

    switch (entry->mTables[1].mMaxFrame)
    {
    case 0:
        pColor->g = 0;
        break;
    case 1:
        pColor->g = mKGreenVals[entry->mTables[1].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mTables[1], &mKGreenVals[entry->mTables[1].mOffset]);
        if (col < 0)
            pColor->g = 0;
        else if (col > 0xFF)
            pColor->g = 0xFF;
        else
            OSf32tou8(&col, &pColor->g);
        break;
    }

    switch (entry->mTables[2].mMaxFrame)
    {
    case 0:
        pColor->b = 0;
        break;
    case 1:
        pColor->b = mKBlueVals[entry->mTables[2].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mTables[2], &mKBlueVals[entry->mTables[2].mOffset]);
        if (col < 0)
            pColor->b = 0;
        else if (col > 0xFF)
            pColor->b = 0xFF;
        else
            OSf32tou8(&col, &pColor->b);
        break;
    }

    switch (entry->mTables[3].mMaxFrame)
    {
    case 0:
        pColor->a = 0;
        break;
    case 1:
        pColor->a = mKAlphaVals[entry->mTables[3].mOffset];
        break;
    default:
        col = J3DGetKeyFrameInterpolation(getFrame(), &entry->mTables[3], &mKAlphaVals[entry->mTables[3].mOffset]);
        if (col < 0)
            pColor->a = 0;
        else if (col > 0xFF)
            pColor->a = 0xFF;
        else
            OSf32tou8(&col, &pColor->a);
        break;
    }
}

void J3DAnmTevRegKey::searchUpdateMaterialID(J3DModelData* modelData)
{
    for (u16 i = 0; i < mCRegUpdateMaterialNum; i++)
    {
        s32 index = modelData->getMaterialTable().getMaterialName()->getIndex(mCRegNameTable.getName(i));
        if (index != -1)
        {
            mCRegUpdateMaterialID[i] = index;
        }
        else
        {
            mCRegUpdateMaterialID[i] = 0xFFFF;
        }
    }

    for (u16 i = 0; i < mKRegUpdateMaterialNum; i++)
    {
        s32 index = modelData->getMaterialTable().getMaterialName()->getIndex(mKRegNameTable.getName(i));
        if (index != -1)
        {
            mKRegUpdateMaterialID[i] = index;
        }
        else
        {
            mKRegUpdateMaterialID[i] = 0xFFFF;
        }
    }
}

template <typename T>
static f32 J3DGetKeyFrameInterpolation(f32 frame, J3DAnmKeyTableBase* pKeyTable, T* pData)
{
    if (frame < pData[0])
        return pData[1];

    if (pKeyTable->mType == 0)
    {
        if (pData[(pKeyTable->mMaxFrame - 1) * 3] <= frame)
            return pData[(pKeyTable->mMaxFrame - 1) * 3 + 1];

        u32 uVar7 = pKeyTable->mMaxFrame;
        while (uVar7 > 1)
        {
            u32 uVar2 = uVar7 >> 1;
            u32 tmp   = uVar2 * 3;
            if (frame >= pData[tmp])
            {
                pData += tmp;
                uVar7 = uVar7 - uVar2;
            }
            else
            {
                uVar7 = uVar2;
            }
        }

        return J3DHermiteInterpolation(frame, &pData[0], &pData[1], &pData[2], &pData[3], &pData[4], &pData[5]);
    }
    else
    {
        u32 idx = pKeyTable->mMaxFrame - 1;
        if (pData[idx * 4] <= frame)
            return pData[idx * 4 + 1];

        u32 uVar7 = pKeyTable->mMaxFrame;
        while (uVar7 > 1)
        {
            u32 uVar2 = uVar7 >> 1;
            u32 tmp   = uVar2 * 4;
            if (frame >= pData[tmp])
            {
                pData += tmp;
                uVar7 = uVar7 - uVar2;
            }
            else
            {
                uVar7 = uVar2;
            }
        }

        return J3DHermiteInterpolation(frame, &pData[0], &pData[1], &pData[3], &pData[4], &pData[5], &pData[6]);
    }
}

template f32 J3DGetKeyFrameInterpolation(f32, J3DAnmKeyTableBase*, s16*);
template f32 J3DGetKeyFrameInterpolation(f32, J3DAnmKeyTableBase*, f32*);
