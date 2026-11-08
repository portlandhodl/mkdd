#include "JSystem/J3D/J3DMtxCalc.h"
#include "macros.h"

DECL_WEAK void J3DMtxCalc::setWeight(u8, f32) { }

DECL_WEAK void J3DMtxCalc::setAnmTransform(u8, J3DAnmTransform*) { }

DECL_WEAK J3DAnmTransform* J3DMtxCalcBlendAnmBase::getAnmTransform() { return mAnims[0]; }

DECL_WEAK void J3DMtxCalcBlendAnmBase::setAnmTransform(u8 idx, J3DAnmTransform* anim)
{
    mAnims[idx] = anim;
}

DECL_WEAK void J3DMtxCalcBlendAnmBase::setAnmTransform(J3DAnmTransform* anim) { mAnims[0] = anim; }

DECL_WEAK J3DAnmTransform* J3DMtxCalcBlendAnmBase::getAnmTransform(u8 idx) { return mAnims[idx]; }

DECL_WEAK void J3DMtxCalcBlendAnmBase::setWeight(u8 idx, f32 weight) { mWeights[idx] = weight; }

DECL_WEAK f32 J3DMtxCalcBlendAnmBase::getWeight(u8 idx) const { return mWeights[idx]; }

DECL_WEAK void J3DMtxCalc::setAnmTransform(J3DAnmTransform*) { }

DECL_WEAK J3DAnmTransform* J3DMtxCalc::getAnmTransform(u8) { return nullptr; }

DECL_WEAK J3DAnmTransform* J3DMtxCalc::getAnmTransform() { return nullptr; }

DECL_WEAK f32 J3DMtxCalc::getWeight(u8) const { return 0.0f; }

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlendSharedMotionT, J3DMtxCalcScaleBlendMaya>,
    J3DMtxCalcJ3DSysInitMaya>::setWeight(u8 idx, f32 weight)
{
    mWeights[idx] = weight;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlendSharedMotionT, J3DMtxCalcScaleBlendMaya>,
    J3DMtxCalcJ3DSysInitMaya>::setAnmTransform(u8 idx, J3DAnmTransform* anim)
{
    mAnims[idx] = anim;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlend, J3DMtxCalcScaleBlendMaya>,
    J3DMtxCalcJ3DSysInitMaya>::setWeight(u8 idx, f32 weight)
{
    mWeights[idx] = weight;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlend, J3DMtxCalcScaleBlendMaya>,
    J3DMtxCalcJ3DSysInitMaya>::setAnmTransform(u8 idx, J3DAnmTransform* anim)
{
    mAnims[idx] = anim;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlendSharedMotionT, J3DMtxCalcScaleBlendSoftimage>,
    J3DMtxCalcJ3DSysInitSoftimage>::setWeight(u8 idx, f32 weight)
{
    mWeights[idx] = weight;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlendSharedMotionT, J3DMtxCalcScaleBlendSoftimage>,
    J3DMtxCalcJ3DSysInitSoftimage>::setAnmTransform(u8 idx, J3DAnmTransform* anim)
{
    mAnims[idx] = anim;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlend, J3DMtxCalcScaleBlendSoftimage>,
    J3DMtxCalcJ3DSysInitSoftimage>::setWeight(u8 idx, f32 weight)
{
    mWeights[idx] = weight;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlend, J3DMtxCalcScaleBlendSoftimage>,
    J3DMtxCalcJ3DSysInitSoftimage>::setAnmTransform(u8 idx, J3DAnmTransform* anim)
{
    mAnims[idx] = anim;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlendSharedMotionT, J3DMtxCalcScaleBlendBasic>,
    J3DMtxCalcJ3DSysInitBasic>::setWeight(u8 idx, f32 weight)
{
    mWeights[idx] = weight;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlendSharedMotionT, J3DMtxCalcScaleBlendBasic>,
    J3DMtxCalcJ3DSysInitBasic>::setAnmTransform(u8 idx, J3DAnmTransform* anim)
{
    mAnims[idx] = anim;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlend, J3DMtxCalcScaleBlendBasic>,
    J3DMtxCalcJ3DSysInitBasic>::setWeight(u8 idx, f32 weight)
{
    mWeights[idx] = weight;
}

template <>
DECL_WEAK void J3DMtxCalcBlendAnm<
    J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlend, J3DMtxCalcScaleBlendBasic>,
    J3DMtxCalcJ3DSysInitBasic>::setAnmTransform(u8 idx, J3DAnmTransform* anim)
{
    mAnims[idx] = anim;
}
