#include "Sato/LumbarCtrl.h"

#include "Kaneshige/Course/CrsGround.h"
#include "Sato/ObjUtility.h"
#include "Sato/StringObj.h"
#include "Sato/stMath.h"
#include "Yamamoto/kartCtrl.h"
#include "Sato/ItemObjMgr.h"
#include "Kawano/driver.h"

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

void LumbarCtrl::update(float (*mtx)[4], DriverModel *)
{
    JGeometry::TPos3f m;
    JGeometry::TPos3f inv;
    JGeometry::TVec3f pos(mtx[0][3], mtx[1][3], mtx[2][3]);
    JGeometry::TVec3f back;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f force;
    JGeometry::TVec3f xdir;
    JGeometry::TVec3f ydir;
    JGeometry::TVec3f proj;
    JGeometry::TVec3f rest;

    if (!(m70 & 1))
    {
        PSMTXIdentity((Mtx &)mMtx);
        m70 |= 1;
    }

    if (mDriverNo == ItemObjMgr::getNowTandemDriverNum(mKartNo))
    {
        f32 mag = PSVECMag(&mE4);
        f32 fric = sAirFric * (sAirFricNormRatio + ((1.0f - sAirFricNormRatio) * mag) / mParam->m18);
        f32 vel = PSVECMag(&mCC);
        fric += sVelFricRatio * (1.0f / (1.0f + vel));
        f32 rate;
        if (fric > 1.0f)
            rate = 1.0f;
        else
            rate = fric;
        mCC.scale(rate);
        mCC.y += sGravity;

        back = mE4;
        back.negate();
        f32 len = PSVECMag(&back);
        if (len > sMinLength)
        {
            dir.normalize(back);
            dir.scale(sBackVelMag * (len - sMinLength));
            mCC.add(dir);
        }

        calcKartInertialForce(&force, sInertialMag, mParam->m1c, sMinKartSpeed);
        m.set(mtx);
        m.ref(0, 3) = 0.0f;
        m.ref(1, 3) = 0.0f;
        m.ref(2, 3) = 0.0f;
        PSMTXConcat(*m.toMtx(), mRotMtx, *m.toMtx());
        inv.set(*m.toMtx());
        PSMTXInverse(*inv.toMtx(), *inv.toMtx());

        if (!force.isZero())
        {
            ObjUtility::getKartXdir(mKartNo, &xdir);
            ObjUtility::getKartYdir(mKartNo, &ydir);
            proj.scale(xdir.dot(force), xdir);
            rest.sub(force, proj);
            proj.scale(sInertialXRatio);
            force.add(proj, rest);
            proj.scale(ydir.dot(force), ydir);
            rest.sub(force, proj);
            proj.scale(sInertialYRatio);
            force.add(proj, rest);
        }
        mCC.add(force);
        mE4.add(mCC);
        if (PSVECMag(&mE4) > mParam->m18)
        {
            stVecNormalize(mE4);
            mE4.scale(mParam->m18);
            mCC.scale(sVelStopMag);
        }
        PSMTXIdentity((Mtx &)mMtx);
    }
    else
    {
        mE4.scale(sFrontDriverDownRatio);
        mCC.zero();
    }

    ((JGeometry::TPos3f &)mMtx).set(mtx);
    JGeometry::TVec3f trans;
    JGeometry::TVec3f ofs(mE4);
    ofs.scale(mScale.x);
    trans.add(pos, ofs);
    ((JGeometry::TPos3f &)mMtx).setTrans(trans);
}

void LumbarCtrl::calcKartInertialForce(JGeometry::TVec3f *force, const f32 scale, const f32 max, const f32 minSpeed)
{
    JGeometry::TVec3f vel;
    ObjUtility::getKartVel(mKartNo, &vel);
    force->sub(mPrevVel, vel);
    force->scale(scale);
    if (GetKartCtrl()->GetCarSpeed(mKartNo) <= minSpeed)
        force->zero();
    else
        stClampVecMax(*force, max);
    mPrevVel = vel;
}

void LumbarCtrl::update_damage(float (*mtx)[4], DriverModel *driver)
{
    if (m70 & 4)
    {
        Mtx body;
        Mtx inv;
        Mtx rot;
        JGeometry::TVec3f top;
        JGeometry::TVec3f nodePos;
        JGeometry::TVec3f lumbar;
        JGeometry::TVec3f d;
        JGeometry::TVec3f pos;
        JGeometry::TVec3f local;
        JGeometry::TVec3f refer;
        JGeometry::TVec3f dir;
        JGeometry::TVec3f y;
        JGeometry::TVec3f z;
        JGeometry::TVec3f xdir;
        NeckIKParam *param = mParam;
        PSMTXCopy(mtx, (Mtx &)mMtx);
        top = mNodeMgr->getNodeObj(0)->mPos;
        nodePos = mNodeMgr->getNodeObj(param->m2c)->mPos;
        d.sub(nodePos, top);
        d.scale(mScale.x);
        lumbar.add(top, d);
        pos.set(mtx[0][3], mtx[1][3], mtx[2][3]);
        PSMTXCopy(ObjUtility::getKartBodyMtx(mKartNo), body);
        body[0][3] = 0.0f;
        body[1][3] = 0.0f;
        body[2][3] = 0.0f;
        PSMTXInverse(body, inv);
        local.sub(lumbar, pos);
        PSMTXMultVec(inv, &local, &local);
        if (local.z > param->m48)
            local.z = param->m48;
        PSMTXMultVec(body, &local, &local);
        lumbar.add(pos, local);
        StringNodeManager *mgr = mNodeMgr;
        refer = mgr->getNodeObj(sLumbarReferNum)->mPos;
        dir.sub(refer, lumbar);
        if (PSVECMag(&dir) == 0.0f)
            ObjUtility::getKartZdir(mKartNo, &dir);
        else
            dir.normalize();
        ObjUtility::getKartXdir(mKartNo, &xdir);
        PSVECCrossProduct(&xdir, &dir, &y);
        stVecNormalize(y);
        PSVECCrossProduct(&dir, &y, &z);
        stVecNormalize(z);
        if (driver->mCharID == PARATROOPA || driver->mCharID == KOOPATROOPA)
        {
            y.negate();
            z.negate();
        }
        stMTXRotDeg(rot, 'Z', -param->m5c);
        mMtx[0][0] = dir.x;
        mMtx[1][0] = dir.y;
        mMtx[2][0] = dir.z;
        mMtx[0][1] = y.x;
        mMtx[1][1] = y.y;
        mMtx[2][1] = y.z;
        mMtx[0][2] = z.x;
        mMtx[1][2] = z.y;
        mMtx[2][2] = z.z;
        PSMTXConcat((Mtx &)mMtx, rot, (Mtx &)mMtx);
        PSMTXConcat((Mtx &)mMtx, mScaleMtx, (Mtx &)mMtx);
        mMtx[0][3] = lumbar.x;
        mMtx[1][3] = lumbar.y;
        mMtx[2][3] = lumbar.z;
    }
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

void FootCtrl::update_damage(float (*mtx)[4], const LumbarCtrl &lumbar, EFootCtrlKind kind)
{
    Mtx body;
    Mtx inv;
    NeckIKParam *param = mParam;
    PSMTXCopy(mtx, (Mtx &)mMtx);
    JGeometry::TVec3f pos(mtx[0][3], mtx[1][3], mtx[2][3]);
    JGeometry::TVec3f d;
    JGeometry::TVec3f last;
    JGeometry::TVec3f top;
    JGeometry::TVec3f end;
    JGeometry::TVec3f ydir;
    JGeometry::TVec3f refer;
    JGeometry::TVec3f side;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f local;
    last = lumbar.getStringNodeMgr()->getNodeObj((u8)(mParam->m58 - 1))->mPos;
    top = lumbar.getStringNodeMgr()->getNodeObj(0)->mPos;
    d.sub(last, top);
    d.scale(lumbar.mScale.x);
    end.add(top, d);
    ObjUtility::getKartYdir(mKartNo, &ydir);
    u8 referNum;
    if (kind == cSkirt)
        referNum = sSkirtReferNum;
    else
        referNum = param->m2d;
    refer = lumbar.getStringNodeMgr()->getNodeObj(referNum)->mPos;
    d.sub(refer, top);
    d.scale(lumbar.mScale.x);
    refer.add(top, d);
    PSMTXCopy(ObjUtility::getKartBodyMtx(mKartNo), body);
    body[0][3] = 0.0f;
    body[1][3] = 0.0f;
    body[2][3] = 0.0f;
    PSMTXInverse(body, inv);
    local.sub(end, pos);
    PSMTXMultVec(inv, &local, &local);
    if (local.z > param->m4c)
        local.z = param->m4c;
    PSMTXMultVec(body, &local, &local);
    end.add(pos, local);
    dir.sub(refer, end);
    stVecNormalize(dir);
    if (kind != cSkirt)
    {
        if (kind == cFootL)
            PSVECCrossProduct(&ydir, &dir, &side);
        else
            PSVECCrossProduct(&dir, &ydir, &side);
        stVecNormalize(side);
        side.scale(param->m20 * lumbar.mScale.x);
        end.add(side);
    }
    if (kind == cSkirt)
    {
        JGeometry::TVec3f skirt;
        JGeometry::TVec3f sd;
        JGeometry::TVec3f y;
        JGeometry::TVec3f z;
        JGeometry::TVec3f xdir;
        skirt = lumbar.getStringNodeMgr()->getNodeObj(sSkirtReferNum)->mPos;
        sd.sub(end, skirt);
        stVecNormalize(sd);
        ObjUtility::getKartXdir(mKartNo, &xdir);
        PSVECCrossProduct(&sd, &xdir, &y);
        stVecNormalize(y);
        PSVECCrossProduct(&sd, &y, &z);
        stVecNormalize(z);
        mMtx[0][0] = sd.x;
        mMtx[1][0] = sd.y;
        mMtx[2][0] = sd.z;
        mMtx[0][1] = y.x;
        mMtx[1][1] = y.y;
        mMtx[2][1] = y.z;
        mMtx[0][2] = z.x;
        mMtx[1][2] = z.y;
        mMtx[2][2] = z.z;
        PSMTXConcat((Mtx &)mMtx, (Mtx &)lumbar.mScaleMtx, (Mtx &)mMtx);
    }
    mMtx[0][3] = end.x;
    mMtx[1][3] = end.y;
    mMtx[2][3] = end.z;
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
