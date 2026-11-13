#include "JSystem/J3D/J3DInd.h"
#include "JSystem/J3D/J3DPE.h"
#include "JSystem/J3D/J3DTexMtx.h"
#include "JSystem/J3D/J3DTypes.h"

#ifndef __MWERKS__
static inline void J3DCopySRTInfo(J3DTextureSRTInfo* dstInfo, const J3DTextureSRTInfo* srcInfo)
{
    dstInfo->mScaleX = srcInfo->mScaleX;
    dstInfo->mScaleY = srcInfo->mScaleY;
    *(u32*)&dstInfo->mRotation = *(u32*)&srcInfo->mRotation;
    dstInfo->mTranslationX = srcInfo->mTranslationX;
    dstInfo->mTranslationY = srcInfo->mTranslationY;
}

static inline void J3DPSCopyN(void* dst, const void* src, int n)
{
    const f32* s = (const f32*)src;
    f32* d = (f32*)dst;
    for (int i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

static inline void J3DPSCopy3(void* dst, const void* src) { J3DPSCopyN(dst, src, 3); }
static inline void J3DPSCopy6(void* dst, const void* src) { J3DPSCopyN(dst, src, 6); }
static inline void J3DPSCopy16(void* dst, const void* src) { J3DPSCopyN(dst, src, 16); }
#else
static inline void J3DCopySRTInfo(J3DTextureSRTInfo* dstInfo, const J3DTextureSRTInfo* srcInfo)
{
    register const f32* src = &srcInfo->mScaleX;
    register f32* dst       = &dstInfo->mScaleX;
    register f32 xy;
    asm {
        psq_l  xy, 0(src), 0, 0
        psq_st xy, 0(dst), 0, 0
    }
    *(u32*)&dstInfo->mRotation = *(u32*)&srcInfo->mRotation;
    src = &srcInfo->mTranslationX;
    dst = &dstInfo->mTranslationX;
    asm {
        psq_l  xy, 0(src), 0, 0
        psq_st xy, 0(dst), 0, 0
    }
}

static inline void J3DPSCopy3(register void* dst, register const void* src)
{
    register f32 xy;
    register f32 z;
    asm {
        psq_l  xy, 0(src), 0, 0
        lfs    z, 8(src)
        psq_st xy, 0(dst), 0, 0
        stfs   z, 8(dst)
    }
}

static inline void J3DPSCopy6(register void* dst, register const void* src)
{
    register f32 v0;
    register f32 v1;
    register f32 v2;
    asm {
        psq_l  v0, 0(src), 0, 0
        psq_l  v1, 8(src), 0, 0
        psq_l  v2, 16(src), 0, 0
        psq_st v0, 0(dst), 0, 0
        psq_st v1, 8(dst), 0, 0
        psq_st v2, 16(dst), 0, 0
    }
}

static inline void J3DPSCopy16(register void* dst, register const void* src)
{
    register f32 v0;
    register f32 v1;
    register f32 v2;
    register f32 v3;
    register f32 v4;
    register f32 v5;
    register f32 v6;
    register f32 v7;
    asm {
        psq_l  v0, 0(src), 0, 0
        psq_l  v1, 8(src), 0, 0
        psq_l  v2, 16(src), 0, 0
        psq_l  v3, 24(src), 0, 0
        psq_l  v4, 32(src), 0, 0
        psq_l  v5, 40(src), 0, 0
        psq_l  v6, 48(src), 0, 0
        psq_l  v7, 56(src), 0, 0
        psq_st v0, 0(dst), 0, 0
        psq_st v1, 8(dst), 0, 0
        psq_st v2, 16(dst), 0, 0
        psq_st v3, 24(dst), 0, 0
        psq_st v4, 32(dst), 0, 0
        psq_st v5, 40(dst), 0, 0
        psq_st v6, 48(dst), 0, 0
        psq_st v7, 56(dst), 0, 0
    }
}
#endif

bool J3DTransformInfo::operator==(J3DTransformInfo& other) const
{
    if (mScale.x == other.mScale.x && mScale.y == other.mScale.y && mScale.z == other.mScale.z
        && mRotation.x == other.mRotation.x && mRotation.y == other.mRotation.y && mRotation.z == other.mRotation.z
        && mTranslation.x == other.mTranslation.x && mTranslation.y == other.mTranslation.y
        && mTranslation.z == other.mTranslation.z) {
        return true;
    }
    return false;
}

bool J3DTextureSRTInfo::isIdentity() const
{
    if (mScaleX == 1.0f && mScaleY == 1.0f && mRotation == 0 && mTranslationX == 0.0f && mTranslationY == 0.0f) {
        return true;
    }
    return false;
}

bool J3DTextureSRTInfo::operator==(J3DTextureSRTInfo& other) const
{
    if (mScaleX == other.mScaleX && mScaleY == other.mScaleY && mRotation == other.mRotation
        && mTranslationX == other.mTranslationX && mTranslationY == other.mTranslationY) {
        return true;
    }
    return false;
}

bool J3DTexMtxInfo::isIdentity() const
{
    if (mSRT.isIdentity() && mCenter.x == 0.0f && mCenter.y == 0.0f && mCenter.z == 0.0f) {
        return true;
    }
    return false;
}

bool J3DTexMtxInfo::operator==(J3DTexMtxInfo& other) const
{
    if (mProjection != other.mProjection || mInfo != other.mInfo || !(mSRT == other.mSRT)) {
        return false;
    }
    if (mEffectMtx[0][0] != other.mEffectMtx[0][0]
        || mEffectMtx[0][1] != other.mEffectMtx[0][1]
        || mEffectMtx[0][2] != other.mEffectMtx[0][2]
        || mEffectMtx[0][3] != other.mEffectMtx[0][3]
        || mEffectMtx[1][0] != other.mEffectMtx[1][0]
        || mEffectMtx[1][1] != other.mEffectMtx[1][1]
        || mEffectMtx[1][2] != other.mEffectMtx[1][2]
        || mEffectMtx[1][3] != other.mEffectMtx[1][3]
        || mEffectMtx[2][0] != other.mEffectMtx[2][0]
        || mEffectMtx[2][1] != other.mEffectMtx[2][1]
        || mEffectMtx[2][2] != other.mEffectMtx[2][2]
        || mEffectMtx[2][3] != other.mEffectMtx[2][3]
        || mEffectMtx[3][0] != other.mEffectMtx[3][0]
        || mEffectMtx[3][1] != other.mEffectMtx[3][1]
        || mEffectMtx[3][2] != other.mEffectMtx[3][2]
        || mEffectMtx[3][3] != other.mEffectMtx[3][3]) {
        return false;
    }
    if (mCenter.x != other.mCenter.x || mCenter.y != other.mCenter.y || mCenter.z != other.mCenter.z) {
        return false;
    }
    return true;
}

void J3DTexMtxInfo::operator=(const J3DTexMtxInfo& other)
{
    mProjection = other.mProjection;
    mInfo       = other.mInfo;
    J3DPSCopy3(&mCenter, &other.mCenter);
    J3DCopySRTInfo(&mSRT, &other.mSRT);
    J3DPSCopy16(mEffectMtx, other.mEffectMtx);
}

void J3DTexMtxInfo::setEffectMtx(Mtx mtx)
{
    JMath::gekko_ps_copy12(mEffectMtx, mtx);
    register f32 zero = 0.0f;
    register f32* dst = mEffectMtx[3];
#ifndef __MWERKS__
    dst[0] = zero;
    dst[1] = zero;
#else
    asm { psq_st zero, 0(dst), 0, 0 }
#endif
    mEffectMtx[3][2] = 0.0f;
    mEffectMtx[3][3] = 1.0f;
}

bool J3DIndTexMtxInfo::operator==(J3DIndTexMtxInfo& other) const
{
    if (_00 == other._00 && _04 == other._04 && _08 == other._08 && _0C == other._0C && _10 == other._10
        && _14 == other._14 && _18 == other._18) {
        return true;
    }
    return false;
}

void J3DIndTexMtxInfo::operator=(const J3DIndTexMtxInfo& other)
{
    J3DPSCopy6(&_00, &other._00);
    _18 = other._18;
}

bool J3DTevStageInfo::operator==(J3DTevStageInfo& other) const
{
    if (mTevStageMode == other.mTevStageMode && mColorInA == other.mColorInA && mColorInB == other.mColorInB
        && mColorInC == other.mColorInC && mColorInD == other.mColorInD && mColorOp == other.mColorOp
        && mColorBias == other.mColorBias && mColorScale == other.mColorScale && mColorClamp == other.mColorClamp
        && mColorRegID == other.mColorRegID && mAlphaInA == other.mAlphaInA && mAlphaInB == other.mAlphaInB
        && mAlphaInC == other.mAlphaInC && mAlphaInD == other.mAlphaInD && mAlphaOp == other.mAlphaOp
        && mAlphaBias == other.mAlphaBias && mAlphaScale == other.mAlphaScale && mAlphaClamp == other.mAlphaClamp
        && mAlphaRegID == other.mAlphaRegID) {
        return true;
    }
    return false;
}

bool J3DFogInfo::operator==(J3DFogInfo& other) const
{
    if (mType == other.mType && mAdjEnable == other.mAdjEnable && mCenter == other.mCenter && mStartZ == other.mStartZ
        && mEndZ == other.mEndZ && mNearZ == other.mNearZ && mFarZ == other.mFarZ && mColor.r == other.mColor.r
        && mColor.g == other.mColor.g && mColor.b == other.mColor.b && mColor.a == other.mColor.a) {
        for (int i = 0; i < 10; i++) {
            if (mFogAdjTable[i] != other.mFogAdjTable[i]) {
                return false;
            }
        }
        return true;
    }
    return false;
}

J3DFogInfo& J3DFogInfo::operator=(const J3DFogInfo& other)
{
    mType      = other.mType;
    mAdjEnable = other.mAdjEnable;
    mCenter    = other.mCenter;
    mStartZ    = other.mStartZ;
    mEndZ      = other.mEndZ;
    mNearZ     = other.mNearZ;
    mFarZ      = other.mFarZ;
    mColor     = other.mColor;
    for (int i = 0; i < 10; i++) {
        mFogAdjTable[i] = other.mFogAdjTable[i];
    }
    return *this;
}

bool J3DNBTScaleInfo::operator==(const J3DNBTScaleInfo& other) const
{
    if (mHasScale == other.mHasScale && mScale.x == other.mScale.x && mScale.y == other.mScale.y
        && mScale.z == other.mScale.z) {
        return true;
    }
    return false;
}

void J3DNBTScaleInfo::operator=(const J3DNBTScaleInfo& other)
{
    mHasScale = other.mHasScale;
    mScale    = other.mScale;
}
