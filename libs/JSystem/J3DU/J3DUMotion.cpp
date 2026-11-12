#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DMtxBuffer.h"
#include "JSystem/J3D/J3DMtxCalc.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DTransform.h"
#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JUtility/JUTNameTab.h"
#include "macros.h"

J3DMtxCalc* J3DUNewMtxCalcAnm(u32 blendType, J3DAnmTransform* anm0, J3DAnmTransform* anm1, J3DAnmTransform* anm2,
                              J3DAnmTransform* anm3, J3DMtxCalcFlag calcFlag)
{
    J3DMtxCalc* calc = nullptr;
    switch (blendType) {
    case 0:
        switch (calcFlag) {
        case MTXCalc_Blend:
            calc = new J3DMtxCalcBlendAnm<J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlend, J3DMtxCalcScaleBlendBasic>,
                                          J3DMtxCalcJ3DSysInitBasic>(anm0);
            break;
        case MTXCalc_BlendSharedMotionT:
            calc = new J3DMtxCalcBlendAnm<J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlendSharedMotionT, J3DMtxCalcScaleBlendBasic>,
                                          J3DMtxCalcJ3DSysInitBasic>(anm0);
            break;
        }
        calc->setAnmTransform(1, anm1);
        calc->setAnmTransform(2, anm2);
        calc->setAnmTransform(3, anm3);
        calc->setWeight(0, 1.0f);
        calc->setWeight(1, 0.0f);
        calc->setWeight(2, 0.0f);
        calc->setWeight(3, 0.0f);
        break;
    case 1:
        switch (calcFlag) {
        case MTXCalc_Blend:
            calc = new J3DMtxCalcBlendAnm<J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlend, J3DMtxCalcScaleBlendSoftimage>,
                                          J3DMtxCalcJ3DSysInitSoftimage>(anm0);
            break;
        case MTXCalc_BlendSharedMotionT:
            calc = new J3DMtxCalcBlendAnm<J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlendSharedMotionT, J3DMtxCalcScaleBlendSoftimage>,
                                          J3DMtxCalcJ3DSysInitSoftimage>(anm0);
            break;
        }
        calc->setAnmTransform(1, anm1);
        calc->setAnmTransform(2, anm2);
        calc->setAnmTransform(3, anm3);
        calc->setWeight(0, 1.0f);
        calc->setWeight(1, 0.0f);
        calc->setWeight(2, 0.0f);
        calc->setWeight(3, 0.0f);
        break;
    case 2:
        switch (calcFlag) {
        case MTXCalc_Blend:
            calc = new J3DMtxCalcBlendAnm<J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlend, J3DMtxCalcScaleBlendMaya>,
                                          J3DMtxCalcJ3DSysInitMaya>(anm0);
            break;
        case MTXCalc_BlendSharedMotionT:
            calc = new J3DMtxCalcBlendAnm<J3DMtxCalcBlendAdaptorDefault<J3DMtxCalcBlendSharedMotionT, J3DMtxCalcScaleBlendMaya>,
                                          J3DMtxCalcJ3DSysInitMaya>(anm0);
            break;
        }
        calc->setAnmTransform(1, anm1);
        calc->setAnmTransform(2, anm2);
        calc->setAnmTransform(3, anm3);
        calc->setWeight(0, 1.0f);
        calc->setWeight(1, 0.0f);
        calc->setWeight(2, 0.0f);
        calc->setWeight(3, 0.0f);
        break;
    }
    return calc;
}

J3DMtxCalcBlendAnmBase::J3DMtxCalcBlendAnmBase()
{
    for (int i = 0; i < 4; i++) {
        mAnims[i]   = nullptr;
        mWeights[i] = 0.0f;
    }
}

static inline f32 J3DUFastReciprocal(f32 x) { return __fres(x); }

void J3DMtxCalcBlend::calcBlend(Vec* scale, Vec* trans, J3DAnmTransform** anims, f32* weights)
{
    J3DJoint* joint      = J3DMtxCalc::getJoint();
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    int num              = 0;
    int maxAnim          = 0;
    u16 jntNo            = joint->getJntNo();
    Mtx& anmMtx          = mtxBuf->getAnmMtx(jntNo);
    f32 totalWeight      = 0.0f;
    for (int i = 0; i < 4; i++) {
        if (anims[i] != nullptr && weights[i] > 0.005f) {
            totalWeight += weights[i];
            maxAnim = i;
            num++;
        }
    }
#line 191
    JUT_ASSERT_F(num != 0, "All motion blend weights are zero at the\n calculation of AnmMtx for the joint %s.", j3dSys.getModel()->getModelData()->getJointName()->getName(joint->getJntNo()));
    if (num == 1) {
        J3DTransformInfo info;
        anims[maxAnim]->getTransform(jntNo, &info);
        *scale = info.mScale;
        *trans = info.mTranslation;

        f32 sinX = JMASinShort(info.mRotation.x);
        f32 cosX = JMACosShort(info.mRotation.x);
        f32 sinY = JMASinShort(info.mRotation.y);
        f32 cosY = JMACosShort(info.mRotation.y);
        f32 sinZ = JMASinShort(info.mRotation.z);
        f32 cosZ = JMACosShort(info.mRotation.z);

        f32 sinXcosZ = sinX * cosZ;
        f32 sinXsinZ = sinX * sinZ;
        f32 cosXsinZ = cosX * sinZ;
        f32 cosXcosZ = cosX * cosZ;

        anmMtx[2][0] = -sinY;
        anmMtx[0][0] = cosZ * cosY;
        anmMtx[1][0] = sinZ * cosY;
        anmMtx[2][1] = cosY * sinX;
        anmMtx[2][2] = cosY * cosX;
        anmMtx[0][1] = sinXcosZ * sinY - cosX * sinZ;
        anmMtx[1][2] = cosXsinZ * sinY - sinXcosZ;
        anmMtx[0][2] = cosXcosZ * sinY + sinXsinZ;
        anmMtx[1][1] = sinXsinZ * sinY + cosXcosZ;
    } else {
        f32 invTotal = J3DUFastReciprocal(totalWeight);
        scale->x = scale->y = scale->z = 0.0f;
        trans->x = trans->y = trans->z = 0.0f;
        Quaternion quat;
        quat.w = 0.0f;
        quat.x = quat.y = quat.z = 0.0f;
        f32 accum = 0.0f;
        for (int i = 0; i <= maxAnim; i++) {
            f32 weight;
            if (anims[i] != nullptr && (weight = weights[i]) > 0.005f) {
                weight *= invTotal;
                J3DTransformInfo info;
                anims[i]->getTransform(jntNo, &info);
                JMAVECScaleAdd(&info.mScale, scale, scale, weight);
                JMAVECScaleAdd(&info.mTranslation, trans, trans, weight);
                Quaternion rot;
                JMAEulerToQuat(info.mRotation.x, info.mRotation.y, info.mRotation.z, &rot);
                accum += weight;
                JMAQuatLerp(&quat, &rot, weight * J3DUFastReciprocal(accum), &quat);
            }
        }
        PSMTXQuat(anmMtx, &quat);
    }
}

void J3DMtxCalcBlendSharedMotionT::calcBlend(Vec* scale, Vec* trans, J3DAnmTransform** anims, f32* weights)
{
    J3DJoint* joint      = J3DMtxCalc::getJoint();
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    int num              = 0;
    int maxAnim          = 0;
    u16 jntNo            = joint->getJntNo();
    Mtx& anmMtx          = mtxBuf->getAnmMtx(jntNo);
    f32 totalWeight      = 0.0f;
    for (int i = 0; i < 4; i++) {
        if (anims[i] && weights[i] > 0.005f) {
            totalWeight += weights[i];
            maxAnim = i;
            num++;
        }
    }
#line 302
    JUT_ASSERT_F(num != 0, "All motion blend weights are zero at the\n calculation of AnmMtx for the joint %s.", j3dSys.getModel()->getModelData()->getJointName()->getName(joint->getJntNo()));
    switch (num) {
    case 1: {
        J3DTransformInfo info;
        anims[maxAnim]->getTransform(jntNo, &info);
        info.mTranslation.x *= joint->getTransformInfo().mTranslation.x;
        info.mTranslation.y *= joint->getTransformInfo().mTranslation.y;
        info.mTranslation.z *= joint->getTransformInfo().mTranslation.z;
        *scale = info.mScale;
        *trans = info.mTranslation;

        f32 sinX = JMASinShort(info.mRotation.x);
        f32 cosX = JMACosShort(info.mRotation.x);
        f32 sinY = JMASinShort(info.mRotation.y);
        f32 cosY = JMACosShort(info.mRotation.y);
        f32 sinZ = JMASinShort(info.mRotation.z);
        f32 cosZ = JMACosShort(info.mRotation.z);

        f32 sinXcosZ = sinX * cosZ;
        f32 sinXsinZ = sinX * sinZ;
        f32 cosXsinZ = cosX * sinZ;
        f32 cosXcosZ = cosX * cosZ;

        anmMtx[2][0] = -sinY;
        anmMtx[0][0] = cosZ * cosY;
        anmMtx[1][0] = sinZ * cosY;
        anmMtx[2][1] = cosY * sinX;
        anmMtx[2][2] = cosY * cosX;
        anmMtx[0][1] = sinXcosZ * sinY - cosX * sinZ;
        anmMtx[1][2] = cosXsinZ * sinY - sinXcosZ;
        anmMtx[0][2] = cosXcosZ * sinY + sinXsinZ;
        anmMtx[1][1] = sinXsinZ * sinY + cosXcosZ;
        break;
    }
    default: {
        scale->x = scale->y = scale->z = 0.0f;
        trans->x = trans->y = trans->z = 0.0f;
        Quaternion quat;
        quat.w = 0.0f;
        quat.x = quat.y = quat.z = 0.0f;
        f32 accum = 0.0f;
        for (int i = 0; i <= maxAnim; i++) {
            f32 weight;
            if (anims[i] != nullptr && (weight = weights[i]) > 0.005f) {
                weight /= totalWeight;
                J3DTransformInfo info;
                anims[i]->getTransform(jntNo, &info);
                scale->x += info.mScale.x * weight;
                scale->y += info.mScale.y * weight;
                scale->z += info.mScale.z * weight;
                trans->x += weight * (info.mTranslation.x * joint->getTransformInfo().mTranslation.x);
                trans->y += weight * (info.mTranslation.y * joint->getTransformInfo().mTranslation.y);
                trans->z += weight * (info.mTranslation.z * joint->getTransformInfo().mTranslation.z);
                Quaternion rot;
                JMAEulerToQuat(info.mRotation.x, info.mRotation.y, info.mRotation.z, &rot);
                accum += weight;
                JMAQuatLerp(&quat, &rot, weight * J3DUFastReciprocal(accum), &quat);
            }
        }
        PSMTXQuat(anmMtx, &quat);
        break;
    }
    }
}

void J3DMtxCalcMayaAdditiveAnm::calcTransform(const J3DTransformInfo& info)
{
    Vec scale = { 0.0f, 0.0f, 0.0f };
    Vec trans = { 0.0f, 0.0f, 0.0f };
    J3DJoint* joint      = J3DMtxCalc::getJoint();
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    u16 jntNo            = joint->getJntNo();
    Mtx& anmMtx          = mtxBuf->getAnmMtx(jntNo);
    scale     = info.mScale;
    trans     = info.mTranslation;
    s16 rx    = info.mRotation.x;
    s16 ry    = info.mRotation.y;
    s16 rz    = info.mRotation.z;

    J3DTransformInfo addInfo[4];
    J3DTransformInfo baseInfo[4];
    for (int i = 1; i < 4; i++) {
        if (mAnims[i] != nullptr) {
            f32 frame = mAnims[i]->getFrame();
            mAnims[i]->getTransform(jntNo, &addInfo[i]);
            mAnims[i]->setFrame(_28);
            mAnims[i]->getTransform(jntNo, &baseInfo[i]);
            mAnims[i]->setFrame(frame);
        }
    }

    if (_24 & 1) {
        scale.x += addInfo[1].mScale.x - baseInfo[1].mScale.x;
        scale.y += addInfo[1].mScale.y - baseInfo[1].mScale.y;
        scale.z += addInfo[1].mScale.z - baseInfo[1].mScale.z;
    }
    if (_24 & 4) {
        trans.x += addInfo[1].mTranslation.x - baseInfo[1].mTranslation.x;
        trans.y += addInfo[1].mTranslation.y - baseInfo[1].mTranslation.y;
        trans.z += addInfo[1].mTranslation.z - baseInfo[1].mTranslation.z;
    }
    if (_24 & 2) {
        rx += addInfo[1].mRotation.x - baseInfo[1].mRotation.x;
        ry += addInfo[1].mRotation.y - baseInfo[1].mRotation.y;
        rz += addInfo[1].mRotation.z - baseInfo[1].mRotation.z;
    }

    J3DGetTranslateRotateMtx(rx, ry, rz, trans.x, trans.y, trans.z, anmMtx);

    if (scale.x == 1.0f && scale.y == 1.0f && scale.z == 1.0f) {
        mtxBuf->setScaleFlag(jntNo, 1);
    } else {
        mtxBuf->setScaleFlag(jntNo, 0);
        JMAMTXApplyScale(anmMtx, anmMtx, scale.x, scale.y, scale.z);
    }

    if (joint->getScaleCompensate() == 1) {
        f32 invX = J3DUFastReciprocal(J3DSys::mParentS.x);
        f32 invY = J3DUFastReciprocal(J3DSys::mParentS.y);
        f32 invZ = J3DUFastReciprocal(J3DSys::mParentS.z);
        anmMtx[0][0] *= invX;
        anmMtx[0][1] *= invX;
        anmMtx[0][2] *= invX;
        anmMtx[1][0] *= invY;
        anmMtx[1][1] *= invY;
        anmMtx[1][2] *= invY;
        anmMtx[2][0] *= invZ;
        anmMtx[2][1] *= invZ;
        anmMtx[2][2] *= invZ;
    }

    PSMTXConcat(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentMtx);
    PSMTXCopy(J3DSys::mCurrentMtx, anmMtx);

    J3DSys::mParentS.x = scale.x;
    J3DSys::mParentS.y = scale.y;
    J3DSys::mParentS.z = scale.z;
}

void J3DMtxCalcScaleBlendBasic::calcScaleBlend(const Vec& scale, const Vec& trans)
{
    J3DJoint* joint      = J3DMtxCalc::getJoint();
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    u16 jntNo            = joint->getJntNo();
    Mtx& anmMtx          = mtxBuf->getAnmMtx(jntNo);
    f32 x                = J3DSys::mCurrentS.x * scale.x;
    f32 y                = J3DSys::mCurrentS.y * scale.y;
    f32 z                = J3DSys::mCurrentS.z * scale.z;
    J3DSys::mCurrentS.x  = x;
    J3DSys::mCurrentS.y  = y;
    J3DSys::mCurrentS.z  = z;

    BOOL isDefScale;
    if (x == 1.0f && y == 1.0f && z == 1.0f) {
        isDefScale = TRUE;
    } else {
        isDefScale = FALSE;
    }

    if (isDefScale == FALSE) {
        mtxBuf->setScaleFlag(jntNo, 0);
        JMAMTXApplyScale(anmMtx, anmMtx, scale.x, scale.y, scale.z);
    } else {
        mtxBuf->setScaleFlag(jntNo, 1);
    }

    anmMtx[0][3] = trans.x;
    anmMtx[1][3] = trans.y;
    anmMtx[2][3] = trans.z;
    PSMTXConcat(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentMtx);
    PSMTXCopy(J3DSys::mCurrentMtx, anmMtx);
}

void J3DMtxCalcScaleBlendSoftimage::calcScaleBlend(const Vec& scale, const Vec& trans)
{
    J3DJoint* joint      = J3DMtxCalc::getJoint();
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    u16 jntNo            = joint->mJointIdx;
    Mtx& anmMtx          = mtxBuf->getAnmMtx(jntNo);
    anmMtx[0][3]         = trans.x * scale.x;
    anmMtx[1][3]         = trans.y * scale.y;
    anmMtx[2][3]         = trans.z * scale.z;
    J3DSys::mCurrentS.x  = J3DSys::mCurrentS.x * scale.x;
    J3DSys::mCurrentS.y  = J3DSys::mCurrentS.y * scale.y;
    J3DSys::mCurrentS.z  = J3DSys::mCurrentS.z * scale.z;
    PSMTXConcat(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentMtx);

    BOOL isDefScale;
    if (J3DSys::mCurrentS.x == 1.0f && J3DSys::mCurrentS.y == 1.0f && J3DSys::mCurrentS.z == 1.0f) {
        isDefScale = TRUE;
    } else {
        isDefScale = FALSE;
    }

    if (isDefScale == FALSE) {
        mtxBuf->setScaleFlag(jntNo, 0);
        JMAMTXApplyScale(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentS.x, J3DSys::mCurrentS.y, J3DSys::mCurrentS.z);
        anmMtx[0][3] = J3DSys::mCurrentMtx[0][3];
        anmMtx[1][3] = J3DSys::mCurrentMtx[1][3];
        anmMtx[2][3] = J3DSys::mCurrentMtx[2][3];
    } else {
        mtxBuf->setScaleFlag(jntNo, 1);
        PSMTXCopy(J3DSys::mCurrentMtx, anmMtx);
    }
}

void J3DMtxCalcScaleBlendMaya::calcScaleBlend(const Vec& scale, const Vec& trans)
{
    J3DJoint* joint      = J3DMtxCalc::getJoint();
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    u16 jntNo            = joint->mJointIdx;
    Mtx& anmMtx          = mtxBuf->getAnmMtx(jntNo);
    anmMtx[0][3]         = trans.x;
    anmMtx[1][3]         = trans.y;
    anmMtx[2][3]         = trans.z;

    if (scale.x == 1.0f && scale.y == 1.0f && scale.z == 1.0f) {
        mtxBuf->setScaleFlag(jntNo, 1);
    } else {
        mtxBuf->setScaleFlag(jntNo, 0);
        JMAMTXApplyScale(anmMtx, anmMtx, scale.x, scale.y, scale.z);
    }

    if (joint->getScaleCompensate() == 1) {
        f32 invX = J3DUFastReciprocal(J3DSys::mParentS.x);
        f32 invY = J3DUFastReciprocal(J3DSys::mParentS.y);
        f32 invZ = J3DUFastReciprocal(J3DSys::mParentS.z);
        anmMtx[0][0] *= invX;
        anmMtx[0][1] *= invX;
        anmMtx[0][2] *= invX;
        anmMtx[1][0] *= invY;
        anmMtx[1][1] *= invY;
        anmMtx[1][2] *= invY;
        anmMtx[2][0] *= invZ;
        anmMtx[2][1] *= invZ;
        anmMtx[2][2] *= invZ;
    }

    PSMTXConcat(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentMtx);
    PSMTXCopy(J3DSys::mCurrentMtx, anmMtx);

    J3DSys::mParentS.x = scale.x;
    J3DSys::mParentS.y = scale.y;
    J3DSys::mParentS.z = scale.z;
}

J3DMtxCalcMayaAdditiveAnm::J3DMtxCalcMayaAdditiveAnm(J3DAnmTransform* anm)
    : _24(0)
    , _28(0.0f)
{
    mAnims[0]   = anm;
    mWeights[0] = 1.0f;
}

void J3DMtxCalcMayaAdditiveAnm::calc()
{
    j3dSys.mMtxCalc = this;
    J3DTransformInfo* pInfo;
    J3DTransformInfo info;
    if (getAnmTransform() != nullptr) {
        u16 jntNo = J3DMtxCalc::getJoint()->mJointIdx;
        getAnmTransform()->getTransform(jntNo, &info);
        pInfo = &info;
    } else {
        pInfo = &J3DMtxCalc::getJoint()->getTransformInfo();
    }
    calcTransform(*pInfo);
}
