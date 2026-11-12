#include "Sato/LumbarCtrl.h"

#include "Kaneshige/Course/CrsGround.h"
#include "Sato/ObjUtility.h"
#include "Sato/StringObj.h"
#include "Sato/stMath.h"
#include "Yamamoto/kartCtrl.h"

namespace {
    f32 sGravity = -0.2f;
    f32 sAirFric = 0.5f;
    f32 sAirFricNormRatio = 0.5f;
    f32 sVelFricRatio = 0.01f;
    f32 sBackVelMag = 0.1f;
    f32 sInertialMag = 5.0f;
    f32 sMinLength = 1.0f;
    f32 sInertialXRatio = 1.5f;
    f32 sInertialYRatio = 1.3f;
    f32 sVelStopMag = 0.5f;
    f32 sMinKartSpeed = 0.5f;
    f32 sFrontDriverDownRatio = 0.8f;
    f32 sWanWanStringAirFric = 0.99f;
    f32 sWanWanStringGravity = -0.5f;
    f32 sStringAirFric = 0.98f;
    f32 sStringGravity = -1.2f;
    f32 sLumbarStdPullMag = 0.1f;
    u8 sSkirtReferNum = 3;
    u8 sLumbarReferNum;
}

LumbarCtrl::LumbarCtrl(int kartNo, u8 driverNo) : IKCtrl(kartNo, driverNo)
{
    m70 = 0;
    Mtx rotX;
    Mtx rotY;
    stMTXRotDeg(rotX, 'X', -90.0f);
    stMTXRotDeg(rotY, 'Y', -90.0f);
    PSMTXConcat(rotY, rotX, mRotMtx);
    mNodeMgr = new ExStringNodeManager(mParam->m58, 20.0f, false, true, 4);
    for (u8 i = 0; i < 0.5f * mParam->m58; i++)
        mNodeMgr->getStringNode(i)->_30 = false;
    mGround = new CrsGround(nullptr);
    reset();
}

void LumbarCtrl::reset()
{
    mCC.zero();
    mPrevVel.zero();
    mE4.zero();
    m70 = 0;
    m74 = 0;
    mBC = 0.0f;
    mC0.zero();
    mNodeMgr->resetNodeAll(nullptr);
    mScale.set(1.0f, 1.0f, 1.0f);
    PSMTXIdentity(mScaleMtx);
}

void LumbarCtrl::update(float (*)[4], DriverModel *)
{
}

void LumbarCtrl::calcKartInertialForce(JGeometry::TVec3f *force, f32 scale, f32 max, f32 minSpeed)
{
    JGeometry::TVec3f vel;
    ObjUtility::getKartVel(mKartNo, &vel);
    force->sub(mPrevVel, vel);
    force->scale(scale);
    if (GetKartCtrl()->GetCarSpeed(mKartNo) <= minSpeed)
        force->zero();
    else
        stClampVecMax(*force, max);
    mPrevVel.set(vel);
}

void LumbarCtrl::update_damage(float (*)[4], DriverModel *)
{
}

void LumbarCtrl::calcString(const JGeometry::TVec3f &pos)
{
    bool doReset = false;
    if (m70 & 2)
    {
        m70 &= ~2;
        doReset = true;
    }
    if (!(m70 & 4))
        return;

    NeckIKParam *param = mParam;
    JGeometry::TVec3f vel;
    ObjUtility::getKartVel(mKartNo, &vel);
    JGeometry::TVec3f top;
    top.add(pos, vel);
    f32 airFric = sStringAirFric;
    f32 adjust = param->m24;
    f32 gravity = sStringGravity;
    if (m74 == 1)
    {
        JGeometry::TVec3f move;
        move.sub(top, mC0);
        move.scale(param->m54);
        for (u32 i = 1; i < mNodeMgr->mStrNodeList.getNumLinks() - 1; i++)
            mNodeMgr->addNodePos(i, move);
        adjust = param->m50;
        airFric = sWanWanStringAirFric;
        gravity = sWanWanStringGravity;
    }
    mNodeMgr->_24 = adjust;
    mNodeMgr->_20 = airFric;
    mNodeMgr->_1c = gravity;
    mNodeMgr->setNodePos(0, top);
    mNodeMgr->_14 = param->m28;
    mNodeMgr->_18 = param->m44;
    for (u8 i = 0; i < mNodeMgr->mStrNodeList.getNumLinks() - 1; i++)
    {
        f32 length = param->m30[i];
        mNodeMgr->getStringNodeAt(i)->_34 = length;
    }
    if (doReset)
        mNodeMgr->resetNodeAll(&top);
    JGeometry::TVec3f pull;
    ObjUtility::getKartZdir(mKartNo, &pull);
    pull.scale(sLumbarStdPullMag);
    mNodeMgr->addNodeVel(mNodeMgr->mStrNodeList.getNumLinks() - 1, pull);
    mNodeMgr->calc();
    mC0 = top;
}

FootCtrl::FootCtrl(int kartNo, u8 driverNo) : IKCtrl(kartNo, driverNo)
{
    reset();
}

void FootCtrl::reset()
{
    PSMTXIdentity((Mtx &)mMtx);
    m40 = 0.0f;
}

void FootCtrl::update_damage(float (*)[4], const LumbarCtrl &, EFootCtrlKind)
{
}

void LumbarCtrl::calcScaleMtx(float (*mtx)[4])
{
    JGeometry::TPos3f &scaleMtx = (JGeometry::TPos3f &)mScaleMtx;
    scaleMtx.set(*(const Mtx *)mtx);
    mScale.x = JGeometry::TUtilf::sqrt(scaleMtx.at(0, 0) * scaleMtx.at(0, 0) + scaleMtx.at(1, 0) * scaleMtx.at(1, 0) + scaleMtx.at(2, 0) * scaleMtx.at(2, 0));
    mScale.y = JGeometry::TUtilf::sqrt(scaleMtx.at(0, 1) * scaleMtx.at(0, 1) + scaleMtx.at(1, 1) * scaleMtx.at(1, 1) + scaleMtx.at(2, 1) * scaleMtx.at(2, 1));
    mScale.z = JGeometry::TUtilf::sqrt(scaleMtx.at(0, 2) * scaleMtx.at(0, 2) + scaleMtx.at(1, 2) * scaleMtx.at(1, 2) + scaleMtx.at(2, 2) * scaleMtx.at(2, 2));
    PSMTXScale(mScaleMtx, mScale.x, mScale.y, mScale.z);
}

#include "JSystem/JAudio/JASFakeMatch2.h"
