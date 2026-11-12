#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DDrawBuffer.h"
#include "JSystem/J3D/J3DMaterial.h"
#include "JSystem/J3D/J3DMaterialAnm.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DMtxBuffer.h"
#include "JSystem/J3D/J3DTransform.h"

void J3DMtxCalcJ3DSysInitBasic::init(const Vec& scale, const Mtx& mtx)
{
    Vec& inputScale = const_cast<Vec&>(scale);
    static_cast<Vec&>(J3DSys::mCurrentS) = inputScale;
    static_cast<Vec&>(J3DSys::mParentS) = (Vec){ 1.0f, 1.0f, 1.0f };
    JMAMTXApplyScale(mtx, J3DSys::mCurrentMtx, inputScale.x, inputScale.y, inputScale.z);
}

void J3DMtxCalcJ3DSysInitMaya::init(const Vec& scale, const Mtx& mtx)
{
    Vec& inputScale = const_cast<Vec&>(scale);
    static_cast<Vec&>(J3DSys::mParentS) = (Vec){ 1.0f, 1.0f, 1.0f };
    static_cast<Vec&>(J3DSys::mCurrentS) = inputScale;
    JMAMTXApplyScale(mtx, J3DSys::mCurrentMtx, inputScale.x, inputScale.y, inputScale.z);
}

J3DMtxBuffer* J3DMtxCalc::mMtxBuffer;
J3DJoint* J3DMtxCalc::mJoint;

inline s32 checkScaleOne(const Vec& vec)
{
    if (vec.x == 1.0f && vec.y == 1.0f && vec.z == 1.0f) {
        return true;
    } else {
        return false;
    }
}

namespace JMath {
inline f32 fastReciprocal(register f32 value)
{
    __asm { fres value, value }
    return value;
}
}

void J3DMtxCalcCalcTransformBasic::calcTransform(const J3DTransformInfo& transInfo)
{
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    u16 jntNo = J3DMtxCalc::getJoint()->mJointIdx;
    Mtx& anmMtx = mtxBuf->getAnmMtx(jntNo);

    J3DSys::mCurrentS.x *= transInfo.mScale.x;
    J3DSys::mCurrentS.y *= transInfo.mScale.y;
    J3DSys::mCurrentS.z *= transInfo.mScale.z;
    J3DGetTranslateRotateMtx(transInfo, anmMtx);

    if (!checkScaleOne(J3DSys::mCurrentS)) {
        mtxBuf->setScaleFlag(jntNo, 0);
        JMAMTXApplyScale(anmMtx, anmMtx, transInfo.mScale.x, transInfo.mScale.y, transInfo.mScale.z);
    } else {
        mtxBuf->setScaleFlag(jntNo, 1);
    }

    PSMTXConcat(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentMtx);
    PSMTXCopy(J3DSys::mCurrentMtx, anmMtx);
}

void J3DMtxCalcCalcTransformSoftimage::calcTransform(const J3DTransformInfo& transInfo)
{
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    u16 jntNo = J3DMtxCalc::getJoint()->mJointIdx;
    Mtx& anmMtx = mtxBuf->getAnmMtx(jntNo);

    J3DGetTranslateRotateMtx(transInfo.mRotation.x, transInfo.mRotation.y, transInfo.mRotation.z,
                             transInfo.mTranslation.x * J3DSys::mCurrentS.x,
                             transInfo.mTranslation.y * J3DSys::mCurrentS.y,
                             transInfo.mTranslation.z * J3DSys::mCurrentS.z, anmMtx);
    PSMTXConcat(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentMtx);

    J3DSys::mCurrentS.x *= transInfo.mScale.x;
    J3DSys::mCurrentS.y *= transInfo.mScale.y;
    J3DSys::mCurrentS.z *= transInfo.mScale.z;

    if (!checkScaleOne(J3DSys::mCurrentS)) {
        mtxBuf->setScaleFlag(jntNo, 0);
        JMAMTXApplyScale(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentS.x, J3DSys::mCurrentS.y,
                         J3DSys::mCurrentS.z);
        anmMtx[0][3] = J3DSys::mCurrentMtx[0][3];
        anmMtx[1][3] = J3DSys::mCurrentMtx[1][3];
        anmMtx[2][3] = J3DSys::mCurrentMtx[2][3];
    } else {
        mtxBuf->setScaleFlag(jntNo, 1);
        PSMTXCopy(J3DSys::mCurrentMtx, anmMtx);
    }
}

void J3DMtxCalcCalcTransformMaya::calcTransform(const J3DTransformInfo& transInfo)
{
    J3DJoint* joint = J3DMtxCalc::getJoint();
    J3DMtxBuffer* mtxBuf = J3DMtxCalc::getMtxBuffer();
    u16 jntNo = joint->mJointIdx;
    Mtx& anmMtx = mtxBuf->getAnmMtx(jntNo);

    J3DGetTranslateRotateMtx(transInfo, anmMtx);

    if (transInfo.mScale.x == 1.0f && transInfo.mScale.y == 1.0f && transInfo.mScale.z == 1.0f) {
        mtxBuf->setScaleFlag(jntNo, 1);
    } else {
        mtxBuf->setScaleFlag(jntNo, 0);
        JMAMTXApplyScale(anmMtx, anmMtx, transInfo.mScale.x, transInfo.mScale.y, transInfo.mScale.z);
    }

    if (joint->getScaleCompensate() == 1) {
        Vec inverseScale;
        f32 parentX = J3DSys::mParentS.x;
        f32 parentY = J3DSys::mParentS.y;
        f32 parentZ = J3DSys::mParentS.z;
        inverseScale.z = JMath::fastReciprocal(parentZ);
        inverseScale.y = JMath::fastReciprocal(parentY);
        inverseScale.x = JMath::fastReciprocal(parentX);

        anmMtx[0][0] *= inverseScale.x;
        anmMtx[0][1] *= inverseScale.x;
        anmMtx[0][2] *= inverseScale.x;
        anmMtx[1][0] *= inverseScale.y;
        anmMtx[1][1] *= inverseScale.y;
        anmMtx[1][2] *= inverseScale.y;
        anmMtx[2][0] *= inverseScale.z;
        anmMtx[2][1] *= inverseScale.z;
        anmMtx[2][2] *= inverseScale.z;
    }

    PSMTXConcat(J3DSys::mCurrentMtx, anmMtx, J3DSys::mCurrentMtx);
    PSMTXCopy(J3DSys::mCurrentMtx, anmMtx);

    J3DSys::mParentS.x = transInfo.mScale.x;
    J3DSys::mParentS.y = transInfo.mScale.y;
    J3DSys::mParentS.z = transInfo.mScale.z;
}

template <>
inline void J3DMtxCalcAnimation<J3DMtxCalcAnimationAdaptorDefault<J3DMtxCalcCalcTransformSoftimage>, J3DMtxCalcJ3DSysInitSoftimage>::init(const Vec& scale, const f32 (&mtx)[3][4])
{
    static_cast<Vec&>(J3DSys::mCurrentS) = scale;
    PSMTXCopy(mtx, J3DSys::mCurrentMtx);
}

inline J3DMtxCalcAnmBase::~J3DMtxCalcAnmBase() { }

J3DMtxCalcAnmBase* J3DNewMtxCalcAnm(u32 type, J3DAnmTransform* animation)
{
    switch (type) {
    case 0:
        return new J3DMtxCalcAnimation<J3DMtxCalcAnimationAdaptorDefault<J3DMtxCalcCalcTransformBasic>, J3DMtxCalcJ3DSysInitBasic>(animation);
    case 1:
        return new J3DMtxCalcAnimation<J3DMtxCalcAnimationAdaptorDefault<J3DMtxCalcCalcTransformSoftimage>, J3DMtxCalcJ3DSysInitSoftimage>(animation);
    case 2:
        return new J3DMtxCalcAnimation<J3DMtxCalcAnimationAdaptorDefault<J3DMtxCalcCalcTransformMaya>, J3DMtxCalcJ3DSysInitMaya>(animation);
    }
    return nullptr;
}

void J3DJoint::appendChild(J3DJoint* newChild)
{
    if (mChild == nullptr) {
        mChild = newChild;
    } else {
        J3DJoint* curChild = mChild;
        while (curChild->getYounger() != nullptr) {
            curChild = curChild->getYounger();
        }
        curChild->setYounger(newChild);
    }
}

J3DJoint::J3DJoint()
{
    mCallBackUserData = 0;
    mFunction = nullptr;
    _08 = 0;
    mChild = nullptr;
    mParent = nullptr;
    mJointIdx = 0;
    mKind = 1;
    mIgnoreParentScaling = false;
    __memcpy(&mTransformInfo, &j3dDefaultTransformInfo, sizeof(J3DTransformInfo));
    mRadius = 0.0f;
    mMtxCalc = nullptr;
    mMaterial = nullptr;

    Vec init = { 0.0f, 0.0f, 0.0f };
    static_cast<Vec&>(mMin) = init;
    Vec init2 = { 0.0f, 0.0f, 0.0f };
    static_cast<Vec&>(mMax) = init2;
}

void J3DJoint::entryIn()
{
    MtxPtr anmMtx = j3dSys.getModel()->getAnmMtx(mJointIdx);
    j3dSys.getDrawBuffer(0)->mZMtx = anmMtx;
    j3dSys.getDrawBuffer(1)->mZMtx = anmMtx;

    for (J3DMaterial* mesh = mMaterial; mesh != nullptr;) {
        if (mesh->getShape()->checkFlag(1)) {
            mesh = mesh->getNext();
        } else {
            J3DMatPacket* matPacket = j3dSys.getModel()->getMatPacket(mesh->getIndex());
            J3DShapePacket* shapePacket = j3dSys.getModel()->getShapePacket(mesh->getShape()->getIndex());
            if (!matPacket->checkFlag(1)) {
                if (mesh->getMaterialAnm()) {
                    mesh->getMaterialAnm()->calc(mesh);
                }
                mesh->calc(anmMtx);
            }
            mesh->setCurrentMtx();
            matPacket->mMaterialAnm = mesh->getMaterialAnm();
            matPacket->setShapePacket(shapePacket);
            J3DDrawBuffer* drawBuffer = j3dSys.getDrawBuffer(!mesh->isDrawModeOpaTexEdge());
            if ((u8)matPacket->entry(drawBuffer)) {
                j3dSys.mMatPacket = matPacket;
                J3DDrawBuffer::entryNum++;
                mesh->makeDisplayList();
            }
            mesh = mesh->getNext();
        }
    }
}

J3DMtxCalc* J3DJoint::mCurrentMtxCalc;

void J3DJoint::recursiveCalc()
{
    Mtx currentMtx;
    J3DJointCallBack callback;
    J3DMtxCalc* previousCalc = nullptr;
    PSMTXCopy(J3DSys::mCurrentMtx, currentMtx);
    f32 currentX = J3DSys::mCurrentS.x;
    f32 currentY = J3DSys::mCurrentS.y;
    f32 currentZ = J3DSys::mCurrentS.z;
    f32 parentX = J3DSys::mParentS.x;
    f32 parentY = J3DSys::mParentS.y;
    f32 parentZ = J3DSys::mParentS.z;

    J3DMtxCalc* calc = mMtxCalc;
    if (calc != nullptr) {
        previousCalc = mCurrentMtxCalc;
        J3DMtxCalc::setJoint(this);
        mCurrentMtxCalc = calc;
        calc->calc();
    } else if (mCurrentMtxCalc != nullptr) {
        J3DMtxCalc::setJoint(this);
        mCurrentMtxCalc->calc();
    }

    callback = mFunction;
    if (callback != nullptr) {
        callback(this, 0);
    }
    if (mChild != nullptr) {
        mChild->recursiveCalc();
    }

    PSMTXCopy(currentMtx, J3DSys::mCurrentMtx);
    J3DSys::mCurrentS.x = currentX;
    J3DSys::mCurrentS.y = currentY;
    J3DSys::mCurrentS.z = currentZ;
    J3DSys::mParentS.x = parentX;
    J3DSys::mParentS.y = parentY;
    J3DSys::mParentS.z = parentZ;

    if (previousCalc != nullptr) {
        mCurrentMtxCalc = previousCalc;
    }
    if (callback != nullptr) {
        callback(this, 1);
    }
    if (mParent != nullptr) {
        mParent->recursiveCalc();
    }
}
