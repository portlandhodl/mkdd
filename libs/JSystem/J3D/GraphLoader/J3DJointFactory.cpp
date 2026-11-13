#include "JSystem/J3D/J3DJointFactory.h"
#include "JSystem/J3D/J3DJoint.h"
#include "JSystem/JSupport.h"

static inline void copyVecPod(register const f32* src, register f32* dst)
{
#ifndef __MWERKS__
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
#else
    __asm {
        psq_l f0, 0(src), 0, 0
        psq_st f0, 0(dst), 0, 0
        lfs f0, 8(src)
        stfs f0, 8(dst)
    }
#endif
}

J3DJointFactory::J3DJointFactory(const J3DJointBlock& block)
{
    mInitData = JSUConvertOffsetToPtr<J3DJointInitData>(&block, block._0C);
    mIndexMap = JSUConvertOffsetToPtr<u16>(&block, block._10);
}

J3DJoint* J3DJointFactory::create(int jointIndex)
{
    J3DJoint* joint = new J3DJoint;
    joint->mJointIdx = jointIndex;
    joint->mKind = mInitData[mIndexMap[jointIndex]].mKind;
    joint->mIgnoreParentScaling = mInitData[mIndexMap[jointIndex]].mIgnoreParentScaling;
    J3DTransformInfo* ti = &mInitData[mIndexMap[jointIndex]].mTransformInfo;
    copyVecPod(&ti->mScale.x, &joint->mTransformInfo.mScale.x);
    *(u32*)&joint->mTransformInfo.mRotation = *(u32*)&ti->mRotation;
    *((s16*)&joint->mTransformInfo.mRotation + 2) = *((s16*)&ti->mRotation + 2);
    copyVecPod(&ti->mTranslation.x, &joint->mTransformInfo.mTranslation.x);
    {
        register J3DJoint* j = joint;
        register const f32* p = (const f32*)&mInitData[mIndexMap[jointIndex]];
#ifndef __MWERKS__
        j->mRadius = mInitData[mIndexMap[jointIndex]].mRadius;
#else
        __asm {
            lfs f0, 36(p)
            stfs f0, 56(j)
        }
#endif
    }
    {
        register J3DJoint* j = joint;
        register const f32* p = &mInitData[mIndexMap[jointIndex]].mMin.x;
#ifndef __MWERKS__
        j->mMin.x = p[0];
        j->mMin.y = p[1];
        j->mMin.z = p[2];
#else
        __asm {
            lfs f0, 0(p)
            stfs f0, 60(j)
            lfs f0, 4(p)
            stfs f0, 64(j)
            lfs f0, 8(p)
            stfs f0, 68(j)
        }
#endif
    }
    {
        register J3DJoint* j = joint;
        register const f32* p = &mInitData[mIndexMap[jointIndex]].mMax.x;
#ifndef __MWERKS__
        j->mMax.x = p[0];
        j->mMax.y = p[1];
        j->mMax.z = p[2];
#else
        __asm {
            lfs f0, 0(p)
            stfs f0, 72(j)
            lfs f0, 4(p)
            stfs f0, 76(j)
            lfs f0, 8(p)
            stfs f0, 80(j)
        }
#endif
    }
    joint->mMtxCalc = nullptr;
    if (joint->mIgnoreParentScaling == 0xFF) {
        joint->mIgnoreParentScaling = 0;
    }
    return joint;
}

