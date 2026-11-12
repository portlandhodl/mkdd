#include "Sato/NeckCtrl.h"
#include "macros.h"

#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/KartInfo.h"
#include "Kaneshige/Objects/GeoRabbitMark.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/StateObserver.h"
#include "Sato/stMath.h"
#include "Sato/StringObj.h"
#include "Kaneshige/SysDebug.h"
#include "Yamamoto/kartCtrl.h"
#include "JSystem/JGeometry/Quat.h"
#include "Sato/LumbarCtrl.h"
#include "Kawano/driver.h"

#include <stdio.h>
#include "JSystem/JMath/JMath.h"

static const f32 sIdentQuat[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
namespace {
    JGeometry::TVec3f sKGravityAngle(-1.0f, -0.4f, 0.0f);
    u8 sNeckBackFwMaxWaitCnt = 15;
    u8 sNeckBackBwMaxWaitCnt = 30;
}

Mtx NeckCtrl::sRotFrontDirMtx;

static inline void setMtxTrans(float (*mtx)[4], JGeometry::TVec3f pos)
{
    mtx[0][3] = pos.x;
    mtx[1][3] = pos.y;
    mtx[2][3] = pos.z;
}

IKCtrl::IKCtrl(const int index, u8 driverNo)
{
    PSMTXIdentity((Mtx&)mMtx);
    mKartNo = index;
    mDriverNo = driverNo;
    mParam = (NeckIKParam*)CharIKParamMgr::mMgr->mParams[RaceMgr::sRaceManager->getKartInfo(index)->getDriverCharID(driverNo) - 1];
}

void IKCtrl::calcJointScaleMtx(JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >* out, float (*mtx)[4])
{
    out->set(*(const Mtx*)mtx);
    f32 sx = JGeometry::TUtilf::sqrt(out->at(0, 0) * out->at(0, 0) + out->at(1, 0) * out->at(1, 0) + out->at(2, 0) * out->at(2, 0));
    f32 sy = JGeometry::TUtilf::sqrt(out->at(0, 1) * out->at(0, 1) + out->at(1, 1) * out->at(1, 1) + out->at(2, 1) * out->at(2, 1));
    f32 sz = JGeometry::TUtilf::sqrt(out->at(0, 2) * out->at(0, 2) + out->at(1, 2) * out->at(1, 2) + out->at(2, 2) * out->at(2, 2));
    PSMTXScale(*out->toMtx(), sx, sy, sz);
}

NeckCtrl::NeckCtrl(int index, u8 driverNo, bool flag) : IKCtrl(index, driverNo)
{
    createStateFunc(this, 5);
    registStateFunc(0, &NeckCtrl::initFunc_Search, &NeckCtrl::doFunc_Search);
    registStateFunc(1, &NeckCtrl::initFunc_Follow, &NeckCtrl::doFunc_Follow);
    registStateFunc(2, &NeckCtrl::initFunc_Look, &NeckCtrl::doFunc_Look);
    registStateFunc(3, &NeckCtrl::initFunc_Return, &NeckCtrl::doFunc_Return);
    registStateFunc(4, &NeckCtrl::initFunc_LookShine, &NeckCtrl::doFunc_LookShine);
    mPtr108 = new u8[RaceMgr::sRaceManager->getKartNumber()];
    m10c = flag;
    f32 cos = JMath::sincosTable_.mTable[1579].second;
    f32 sin = JMath::sincosTable_.mTable[1579].first;
    sRotFrontDirMtx[0][0] = cos;
    sRotFrontDirMtx[1][1] = 1.0f;
    sRotFrontDirMtx[0][2] = sin;
    sRotFrontDirMtx[2][0] = -sin;
    sRotFrontDirMtx[2][2] = cos;
    sRotFrontDirMtx[0][1] = sRotFrontDirMtx[1][0] = sRotFrontDirMtx[1][2] = sRotFrontDirMtx[2][1] = 0.0f;
    sRotFrontDirMtx[0][3] = 0.0f;
    sRotFrontDirMtx[1][3] = 0.0f;
    sRotFrontDirMtx[2][3] = 0.0f;
    reset();
}

template class StateObserverT<NeckCtrl>;

void NeckCtrl::reset()
{
    mEC = 0;
    mED = 0;
    mF4 = 0.08f;
    mFC = -1;
    mF8 = 0.0f;
    mF0 = 0.0f;
    mPtrE8 = nullptr;
    m10d = 0;
    m104 = 0;
    m115 = 0;
    m114 = 0;
    m116 = 0;
    resetState();
    if (RaceMgr::sRaceManager->getRaceMode() == (ERaceMode)7)
    {
        changeState(4);
    }
    for (u8 i = 0; i < RaceMgr::sRaceManager->getKartNumber(); i++)
        mPtr108[i] = 0;
}

void NeckCtrl::init()
{
    Mtx rotC;
    Mtx rotA;
    Mtx rotB;
    Quaternion qa;
    Quaternion qb;

    mEC = 1;
    stMtx2Qt(&mQuatPrev, ObjUtility::getKartBodyMtx(mKartNo));
    mQuat = mQuatPrev;
    stMTXRotDeg(rotA, 'Z', 180.0f);
    stMTXRotDeg(rotB, 'Y', 90.0f);
    stMtx2Qt(&qa, rotA);
    stMtx2Qt(&qb, rotB);
    PSMTXConcat(rotB, rotA, (Mtx&)mMtx88);
    PSQUATMultiply(&qb, &qa, &mQuat78);

    int id = RaceMgr::getManager()->getKartInfo(mKartNo)->getDriverCharID(mDriverNo);
    f32 ang;
    if (id == KOOPATROOPA || id == PARATROOPA)
        ang = 90.0f;
    else
        ang = -90.0f;
    stMTXRotDeg(rotC, 'X', ang);
    stMTXRotDeg(rotB, 'Y', ang);
    PSMTXConcat(rotB, rotC, (Mtx&)mMtxB8);
    mED = 0;
}

void NeckCtrl::startHitClrCount(int)
{
}

void NeckCtrl::chkHitClrCount()
{
}

static inline f32 stAsinClamp(f32 x)
{
    if (x >= 1.0f)
        return 1.5707964f;
    if (x <= -1.0f)
        return -1.5707964f;
    if (x < 0.0f)
        return -JMath::asinAcosTable_.mTable[(u32)(1023.5f * -x)];
    return JMath::asinAcosTable_.mTable[(u32)(1023.5f * x)];
}

static inline void stGetEulerYXZ(const Mtx m, f32& x, f32& y, f32& z)
{
    f32 eps = JGeometry::TUtilf::epsilon();
    if (m[1][2] - 1.0f >= -eps)
    {
        y = -JGeometry::TUtilf::atan2(m[2][0], m[0][0]);
        x = -1.5707964f;
        z = 0.0f;
    }
    else if (m[1][2] + 1.0f <= eps)
    {
        y = -JGeometry::TUtilf::atan2(m[2][0], m[0][0]);
        x = 1.5707964f;
        z = 0.0f;
    }
    else
    {
        x = -stAsinClamp(m[1][2]);
        y = -JGeometry::TUtilf::atan2(-m[0][2], m[2][2]);
        z = -JGeometry::TUtilf::atan2(-m[1][0], m[1][1]);
    }
}

static inline void stMakeEulerMtxYXZ(Mtx m, f32 z, f32 y, f32 x)
{
    f32 sx;
    f32 sy;
    f32 sz;
    f32 cx;
    f32 cy;
    f32 cz;
    cz = cos(z);
    cy = cos(y);
    cx = cos(x);
    sz = sin(z);
    sy = sin(y);
    sx = sin(x);

    f32 t7;
    f32 t6;
    f32 t5;
    f32 t4;
    f32 t3;
    f32 t2;
    f32 t1;
    f32 t0;

    t6 = cy * cz;
    t1 = sy * cz;
    t7 = sx * sy;
    t0 = sx * t6;
    t5 = sz * cx;
    t4 = sx * cy;
    t3 = -t1;
    t2 = -sz;
    t1 = t7 * cz;
    t7 = t7 * sz + t6;
    t6 = sz * t4 + t3;
    t5 = t2 * cy + t1;
    m[1][0] = sz * cx;
    m[0][0] = t7;
    t4 = cx * cz;
    t3 = sy * sz + t0;
    m[2][0] = t6;
    t2 = cx * sy;
    t1 = -sx;
    m[0][1] = t5;
    t0 = cx * cy;
    m[1][1] = t4;
    m[2][1] = t3;
    m[0][2] = t2;
    m[1][2] = t1;
    m[2][2] = t0;
}

static inline bool isTandemDriver(NeckCtrl* n)
{
    u8 ret = false;
    if (n->mDriverNo == ItemObjMgr::getNowTandemDriverNum(n->mKartNo) && GetKartCtrl())
        ret = true;
    return ret;
}

static inline bool isTandemBack(NeckCtrl* n, bool drv)
{
    u8 ret = false;
    if (drv)
    {
        if (GetKartCtrl()->GetTandemDir(n->mKartNo) == 1)
            ret = true;
    }
    return ret;
}

void NeckCtrl::calcFrontDirQt()
{
    if (mED && mDriverNo == ItemObjMgr::getNowTandemDriverNum(mKartNo))
        return;

    bool spin = false;
    if (GetKartCtrl()->GetKartStatus(mKartNo) & 3)
        spin = true;
    bool back = isTandemBack(this, isTandemDriver(this) ? true : false) ? true : false;
    m10d = 0;
    f32 sign = 1.0f;
    Mtx mtx;
    if (back && !spin)
    {
        PSMTXConcat((MtxPtr)mPtrE8, mMtx88, mtx);
        sign = -1.0f;
    }
    else
    {
        MtxPtr body = ObjUtility::getKartBodyMtx(mKartNo);
        if (back)
        {
            PSMTXConcat(body, sRotFrontDirMtx, mtx);
            m10d = 1;
        }
        else if ((GetKartCtrl()->CheckReverse(mKartNo) || m115) && !m114)
        {
            Mtx rot;
            s16 angle = -32767.0f * mParam->mC;
            f32 cs = JMath::sincosTable_.cosShort(angle);
            f32 sn = JMath::sincosTable_.sinShort(angle);
            rot[0][0] = cs;
            rot[1][1] = 1.0f;
            rot[0][2] = sn;
            rot[2][0] = -sn;
            rot[2][2] = cs;
            rot[0][1] = rot[1][0] = rot[1][2] = rot[2][1] = 0.0f;
            PSMTXConcat(body, rot, mtx);
            m10d = 1;
            if (!m116)
            {
                m115 = sNeckBackBwMaxWaitCnt;
                m114 = 0;
            }
            if (m115)
                m115--;
            m116 = 1;
        }
        else
        {
            if (GetKartCtrl() && GetKartCtrl()->getKartBody(mKartNo) && mDriverNo != ItemObjMgr::getNowTandemDriverNum(mKartNo))
            {
                Mtx rotTire;
                Mtx tmp;
                stMTXRotRad(rotTire, 'Y', GetKartCtrl()->GetTireAngle(mKartNo));
                PSMTXConcat(rotTire, body, tmp);
                body = tmp;
            }
            PSMTXCopy(body, mtx);
            if (m116)
            {
                m114 = sNeckBackFwMaxWaitCnt;
                m115 = 0;
            }
            if (m114)
                m114--;
            m116 = 0;
        }
    }

    JGeometry::TVec3f xdir;
    ObjUtility::getKartXdir(mKartNo, &xdir);
    f32 len = stLength2(xdir.x, xdir.z);
    f32 tilt = atan2(xdir.y, len);
    f32 x;
    f32 y;
    f32 z;
    stGetEulerYXZ(mtx, x, y, z);
    stMakeEulerMtxYXZ(mtx, z - tilt * sign, y, x);
    stMtx2Qt(&mQuat, mtx);
    ((JGeometry::TQuat4f&)mQuat).normalize((JGeometry::TQuat4f&)mQuat);
}

void NeckCtrl::calcTargetPosDirQt(const JGeometry::TVec3<f32>&)
{
}

void NeckCtrl::initFunc_Search()
{
    mED = 0;
    if (GeographyObjManager::gMgr->mIsEscapeBattle && GeoRabbitMark::getSupervisor()->getRabbitKartNo() != mKartNo)
    {
        changeState(4);
    }
}

void NeckCtrl::doFunc_Search()
{
    if (mDriverNo != ItemObjMgr::getNowTandemDriverNum(mKartNo) || m10c || m10d)
    {
        calcFrontDirQt();
        return;
    }
    for (int i = 0; i < RaceMgr::sRaceManager->getKartNumber(); i++)
    {
        int bit = 1 << i;
        if (m104 & bit)
        {
            int v = mPtr108[i] + 1;
            mPtr108[i] = v;
            if (mPtr108[i] >= 90)
                m104 &= ~bit;
        }
    }
    calcFrontDirQt();
    if ((u32)mKartNo == RaceMgr::sRaceManager->getRaceFrame() % RaceMgr::sRaceManager->getKartNumber())
    {
        mFC = searchRivalKart();
        int fc = mFC;
        if (fc >= 0 && !(*(volatile u32 *)&m104 & (1 << fc)))
        {
            m104 = m104 | (1 << fc);
            mPtr108[fc] = 0;
            changeState(1);
        }
    }
    if (GeographyObjManager::gMgr->mIsEscapeBattle && GeoRabbitMark::getSupervisor()->getRabbitKartNo() != mKartNo)
    {
        changeState(4);
    }
}

void NeckCtrl::initFunc_Follow()
{
}

void NeckCtrl::doFunc_Follow()
{
    JGeometry::TVec3f rivalPos;
    ObjUtility::getKartPos(mFC, &rivalPos);
    if (!mED)
    {
        JGeometry::TVec3f right;
        JGeometry::TVec3f up;
        JGeometry::TVec3f dir;
        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mKartNo, &kartPos);
        dir.sub(rivalPos, kartPos);
        stVecNormalize(dir);
        up.set(0.0f, 1.0f, 0.0f);
        right.set(1.0f, 0.0f, 0.0f);
        PSVECCrossProduct(&up, &dir, &right);
        stVecNormalize(right);
        PSVECCrossProduct(&dir, &right, &up);
        stVecNormalize(up);
        Mtx mtx;
        stMakeRMtx(mtx, right, up, dir);
        stMtx2Qt(&mQuat, mtx);
    }
    if (mStateCount >= 10)
    {
        changeState(2);
    }
}

void NeckCtrl::initFunc_Look()
{
}

void NeckCtrl::doFunc_Look()
{
    Mtx mtx;
    JGeometry::TVec3f rivalPos;
    JGeometry::TVec3f right;
    JGeometry::TVec3f up;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mFC, &rivalPos);
    if (!mED)
    {
        ObjUtility::getKartPos(mKartNo, &kartPos);
        dir.sub(rivalPos, kartPos);
        stVecNormalize(dir);
        up.set(0.0f, 1.0f, 0.0f);
        right.set(1.0f, 0.0f, 0.0f);
        PSVECCrossProduct(&up, &dir, &right);
        stVecNormalize(right);
        PSVECCrossProduct(&dir, &right, &up);
        stVecNormalize(up);
        stMakeRMtx(mtx, right, up, dir);
        stMtx2Qt(&mQuat, mtx);
    }
    if (mStateCount >= 30)
    {
        changeState(3);
    }
}

void NeckCtrl::initFunc_Return()
{
}

void NeckCtrl::doFunc_Return()
{
    calcFrontDirQt();
    if (mStateCount >= 20)
    {
        changeState(0);
    }
}

void NeckCtrl::initFunc_LookShine()
{
}

void NeckCtrl::doFunc_LookShine()
{
    if (GeoRabbitMark::getSupervisor()->getRabbitKartNo() == mKartNo)
    {
        changeState(0);
        return;
    }
    if (mDriverNo != ItemObjMgr::getNowTandemDriverNum(mKartNo))
    {
        calcFrontDirQt();
        return;
    }
    Mtx mtx;
    JGeometry::TVec3f markPos;
    JGeometry::TVec3f right;
    JGeometry::TVec3f up;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f kartPos;
    GeoRabbitMark::getSupervisor()->getMarkPosition(&markPos);
    if (!mED)
    {
        ObjUtility::getKartPos(mKartNo, &kartPos);
        dir.sub(markPos, kartPos);
        stVecNormalize(dir);
        up.set(0.0f, 1.0f, 0.0f);
        right.set(1.0f, 0.0f, 0.0f);
        PSVECCrossProduct(&up, &dir, &right);
        stVecNormalize(right);
        PSVECCrossProduct(&dir, &right, &up);
        stVecNormalize(up);
        stMakeRMtx(mtx, right, up, dir);
        stMtx2Qt(&mQuat, mtx);
    }
}

void NeckCtrl::calcTargetQt(float (*)[4])
{
}

int NeckCtrl::searchRivalKart()
{
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;
    ObjUtility::getKartPos(mKartNo, &pos);
    ObjUtility::getKartZdir(mKartNo, &dir);
    return ObjUtility::findNearKartAngle(pos, dir, 1800.0f, 180.0f);
}

void NeckCtrl::checkLerpRatio(u32 mode)
{
    if (mode != 0)
    {
        switch (mode)
        {
        case 1:
            mF4 = 0.2f;
            break;
        case 2:
            mF4 = 1.0f;
            break;
        }
    }
    else
    {
        if (mDriverNo == ItemObjMgr::getNowTandemDriverNum(mKartNo))
            mF4 = 0.1f;
        else
            mF4 = 0.2f;
    }
}

static inline void stAsin(f32 x, f32& out)
{
    if (x >= 1.0f)
        out = 1.5707964f;
    else if (x <= -1.0f)
        out = -1.5707964f;
    else if (x < 0.0f)
        out = -JMath::asinAcosTable_.mTable[(u32)(1023.5f * -x)];
    else
        out = JMath::asinAcosTable_.mTable[(u32)(1023.5f * x)];
}

static inline void stGetEuler360(const Mtx m, f32& yaw, f32& pitch, f32& roll)
{
    f32 zy = m[2][1];
    f32 eps = JGeometry::TUtilf::epsilon();
    if (zy - 1.0f >= -eps)
    {
        yaw = JGeometry::TUtilf::atan2(m[0][2], m[0][0]);
        roll = 1.5707964f;
        pitch = 0.0f;
    }
    else if (zy + 1.0f <= eps)
    {
        yaw = JGeometry::TUtilf::atan2(m[0][2], m[0][0]);
        roll = -1.5707964f;
        pitch = 0.0f;
    }
    else
    {
        stAsin(zy, roll);
        yaw = JGeometry::TUtilf::atan2(-m[2][0], m[2][2]);
        pitch = JGeometry::TUtilf::atan2(-m[0][1], m[1][1]);
    }
}

static inline void stMakeEulerMtx(Mtx m, f32 pitch, f32 yaw, f32 roll)
{
    f32 cp = cos(pitch);
    f32 cy = cos(yaw);
    f32 cr = cos(roll);
    f32 sp = sin(pitch);
    f32 sy = sin(yaw);
    f32 sr = sin(roll);
    m[0][0] = cy * cp - sr * sy * sp;
    m[0][1] = -sp * cr;
    m[0][2] = sp * (sr * cy) + sy * cp;
    m[1][0] = sp * cy + sr * sy * cp;
    m[1][1] = cr * cp;
    m[1][2] = sy * sp - sr * (cy * cp);
    m[2][0] = -cr * sy;
    m[2][1] = sr;
    m[2][2] = cr * cy;
}

void NeckCtrl::checkMaxAngle(float (*mtx)[4])
{
    Mtx tmp;
    Mtx inv;
    Mtx work;
    PSMTXConcat(mtx, mMtxB8, tmp);
    PSMTXInverse(tmp, inv);
    stQt2Mtx((Mtx&)mMtx, &mQuatPrev);
    PSMTXConcat(inv, (Mtx&)mMtx, work);

    f32 pitch;
    f32 yaw;
    f32 roll;
    stGetEuler360(work, yaw, pitch, roll);

    bool rollClamped = false;
    bool yawClamped = false;
    bool pitchClamped = false;
    bool wasLocked = false;
    if (mED)
        wasLocked = true;
    if (wasLocked)
    {
        bool unlock = false;
        if (mF0 > 0.0f && yaw > 0.0f)
        {
            f32 max = mParam->m8;
            if (stAbs(yaw - max) < 0.03f)
            {
                yaw = max;
                unlock = true;
            }
        }
        else if (mF0 < 0.0f && yaw < 0.0f)
        {
            f32 min = mParam->mC;
            if (stAbs(yaw - min) < 0.03f)
            {
                yaw = min;
                unlock = true;
            }
        }
        if (unlock)
            mED = 0;
    }

    if (yaw > 0.0f && yaw > mParam->m8)
    {
        yaw = mParam->m8;
        yawClamped = true;
    }
    else if (yaw < 0.0f && yaw < mParam->mC)
    {
        yaw = mParam->mC;
        yawClamped = true;
    }

    f32 ratio = 1.0f - JGeometry::TUtilf::abs(yaw / mParam->m8);
    ratio = ratio > 1.0f ? 1.0f : ratio;
    ratio = ratio < 0.0f ? 0.0f : ratio;
    f32 scale = (1.0f - mParam->m60) * ratio + mParam->m60;

    if (pitch > 0.0f)
    {
        f32 max = mParam->m10 * scale;
        if (pitch > max)
        {
            pitch = max;
            pitchClamped = true;
        }
    }
    else
    {
        f32 min = mParam->m14 * scale;
        if (pitch < min)
        {
            pitch = min;
            pitchClamped = true;
        }
    }

    if (roll > 0.0f)
    {
        f32 max = mParam->m0 * scale;
        if (roll > max)
        {
            roll = max;
            rollClamped = true;
        }
    }
    else
    {
        if (roll < mParam->m4)
        {
            roll = mParam->m4;
            rollClamped = true;
        }
    }

    stMakeEulerMtx(work, pitch, yaw, roll);
    PSMTXConcat(tmp, work, (Mtx&)mMtx);
    stMtx2Qt(&mQuatPrev, (Mtx&)mMtx);

    if (rollClamped || yawClamped || pitchClamped)
    {
        Mtx m2;
        stQt2Mtx(m2, &mQuat);
        PSMTXConcat(inv, m2, m2);
        f32 y2;
        f32 p2;
        f32 r2;
        stGetEuler360(m2, y2, p2, r2);
        if (rollClamped)
            r2 = roll;
        if (yawClamped)
        {
            if (y2 > 0.0f && yaw < 0.0f)
            {
                mED = 1;
                mF8 = yaw;
                mF0 = 1.0f;
            }
            else if (y2 < 0.0f && yaw > 0.0f)
            {
                mED = 1;
                mF8 = yaw;
                mF0 = -1.0f;
            }
            y2 = yaw;
        }
        if (pitchClamped)
            p2 = pitch;
        stMakeEulerMtx(m2, p2, y2, r2);
        PSMTXConcat(tmp, m2, m2);
        stMtx2Qt(&mQuat, m2);
    }

    if (wasLocked)
    {
        Mtx m3;
        stQt2Mtx(m3, &mQuat);
        PSMTXConcat(inv, m3, m3);
        f32 y3;
        f32 p3;
        f32 r3;
        stGetEuler360(m3, y3, p3, r3);
        mF8 = mF8 + 0.3f * mF0;
        if (mF8 < mParam->mC && mF0 < 0.0f)
            mF8 = mParam->mC;
        else if (mF8 > mParam->m8 && mF0 > 0.0f)
            mF8 = mParam->m8;
        stMakeEulerMtx(m3, p3, mF8, r3);
        PSMTXConcat(tmp, m3, m3);
        stMtx2Qt(&mQuat, m3);
    }
}

void NeckCtrl::update(float (*mtx1)[4], float (*mtx2)[4], LumbarCtrl*, u32 mode)
{
    Mtx inv2;
    Mtx inv1;
    Mtx norm2;
    Mtx norm1;
    JGeometry::TMtx34f copy1;
    JGeometry::TMtx34f copy2;
    Quaternion q;
    Quaternion lerp;
    Quaternion q2;

    if (!mEC)
        init();

    copy1.set(mtx1);
    f32 sx1 = JGeometry::TUtilf::sqrt(copy1.at(0, 0) * copy1.at(0, 0) + copy1.at(1, 0) * copy1.at(1, 0) + copy1.at(2, 0) * copy1.at(2, 0));
    f32 sy1 = JGeometry::TUtilf::sqrt(copy1.at(0, 1) * copy1.at(0, 1) + copy1.at(1, 1) * copy1.at(1, 1) + copy1.at(2, 1) * copy1.at(2, 1));
    f32 sz1 = JGeometry::TUtilf::sqrt(copy1.at(0, 2) * copy1.at(0, 2) + copy1.at(1, 2) * copy1.at(1, 2) + copy1.at(2, 2) * copy1.at(2, 2));
    PSMTXScale((Mtx&)copy1, sx1, sy1, sz1);

    copy2.set(mtx2);
    f32 sx2 = JGeometry::TUtilf::sqrt(copy2.at(0, 0) * copy2.at(0, 0) + copy2.at(1, 0) * copy2.at(1, 0) + copy2.at(2, 0) * copy2.at(2, 0));
    f32 sy2 = JGeometry::TUtilf::sqrt(copy2.at(0, 1) * copy2.at(0, 1) + copy2.at(1, 1) * copy2.at(1, 1) + copy2.at(2, 1) * copy2.at(2, 1));
    f32 sz2 = JGeometry::TUtilf::sqrt(copy2.at(0, 2) * copy2.at(0, 2) + copy2.at(1, 2) * copy2.at(1, 2) + copy2.at(2, 2) * copy2.at(2, 2));
    PSMTXScale((Mtx&)copy2, sx2, sy2, sz2);

    PSMTXScale(inv1, 1.0f / sx1, 1.0f / sy1, 1.0f / sz1);
    PSMTXScale(inv2, 1.0f / sx2, 1.0f / sy2, 1.0f / sz2);

    PSMTXConcat(mtx1, inv1, norm1);
    PSMTXConcat(mtx2, inv2, norm2);

    f32 tx = mtx1[0][3];
    f32 ty = mtx1[1][3];
    f32 tz = mtx1[2][3];

    if (mode != 0)
    {
        stMtx2Qt(&q, norm1);
        PSQUATMultiply(&q, &mQuat78, &mQuat);
    }
    else
    {
        mPtrE8 = norm1;
        if (mIsInit)
        {
            mIsInit = 0;
            (mOwner->*(mInitFuncs[mState]))();
        }
        (mOwner->*(mDoFuncs[mState]))();
        mStateCount++;
    }

    checkLerpRatio(mode);

    stQtLerp(&lerp, &mQuatPrev, &mQuat, mF4);
    mQuatPrev = lerp;

    if (mode == 0)
        checkMaxAngle(norm2);

    PSQUATMultiply(&mQuatPrev, &mQuat78, &q2);
    stQt2Mtx((Mtx&)mMtx, &q2);
    PSMTXConcat((Mtx&)mMtx, (Mtx&)copy1, (Mtx&)mMtx);
    mMtx[0][3] = tx;
    mMtx[1][3] = ty;
    mMtx[2][3] = tz;
}

namespace {
    f32 sPHairLength = 20.0f;
    f32 sPMaxOffsetX = 10.0f;
    f32 sPMinOffsetX;
    f32 sPStringLength[2] = { 30.0f, 40.0f };
    f32 sPStringAdjustRatio = 0.2f;
    f32 sPStringAirFric = 0.99f;
    f32 sPStringGravity = -0.8f;
    f32 sPStringMaxAccel = 1.4f;
    f32 sPDamageGravityDirRatio = 0.7f;
    f32 sDStringLength = 20.0f;
    f32 sDStringAdjustRatio = 0.2f;
    f32 sDStringGravity = -1.5f;
    f32 sDaisyHair2RotAngle = 20.0f;
    f32 sDMaxOffsetX = -5.0f;
    f32 sKStringLength[3] = { 10.0f, 10.0f, 30.0f };
    f32 sKStringOffset[2] = { 12.0f, 9.0f };
    f32 sKStringAdjustRatio = 0.2f;
    f32 sKStringAirFric = 0.99f;
    f32 sKStringGravity = -0.5f;
    f32 sKStringMaxAccel = 2.0f;
    f32 sKLockStringVelDownRatio = 0.98f;
    f32 sKColFaceOffsetXdir = 35.0f;
    f32 sKColFaceOffsetYdir = -10.0f;
    f32 sKColHeadRadius = 35.0f;
    f32 sDKStringLength[2] = { 40.0f, 20.0f };
    f32 sDKStringAdjustRatio = 0.2f;
    f32 sDKStringAirFric = 0.99f;
    f32 sDKStringGravity = -0.6f;
    f32 sDKStringMaxAccel = 0.8f;
    f32 sDKMinOffsetX;
}

inline HairCtrl::HairCtrl(int index, u8 driverNo) : IKCtrl(index, driverNo)
{
    mNodeMgr = 0;
    m44 = 1;
}

void HairCtrl::reset()
{
    m44 = 1;
}

void HairCtrl::initString(const JGeometry::TVec3<f32>& pos, f32 len)
{
    JGeometry::TVec3f step(0.0f, -1.0f, 0.0f);
    step.scale(len);
    JGeometry::TVec3f offs;
    offs.zero();
    mNodeMgr->setAllNodePos(pos);
    for (u8 i = 0; i < mNodeMgr->mStrNodeList.getNumLinks(); i++)
    {
        mNodeMgr->addNodePos(i, offs);
        offs.add(step);
    }
}

void HairCtrl::makeFaceNormalizeMtx(float (*mtx)[4], JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >& a, JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >& b)
{
    PSMTXCopy(mtx, *a.toMtx());
    Mtx smtx;
    JGeometry::TVec3f scale;
    scale.x = JGeometry::TUtilf::sqrt(a.at(0, 0) * a.at(0, 0) + a.at(1, 0) * a.at(1, 0) + a.at(2, 0) * a.at(2, 0));
    scale.y = JGeometry::TUtilf::sqrt(a.at(0, 1) * a.at(0, 1) + a.at(1, 1) * a.at(1, 1) + a.at(2, 1) * a.at(2, 1));
    scale.z = JGeometry::TUtilf::sqrt(a.at(0, 2) * a.at(0, 2) + a.at(1, 2) * a.at(1, 2) + a.at(2, 2) * a.at(2, 2));
    scale.add(JGeometry::TVec3f(0.0001f, 0.0001f, 0.0001f));
    PSMTXScale(smtx, 1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);
    PSMTXConcat(*a.toMtx(), smtx, *a.toMtx());
    PSMTXCopy(*a.toMtx(), *b.toMtx());
    a.ref(0, 3) = 0.0f;
    a.ref(1, 3) = 0.0f;
    a.ref(2, 3) = 0.0f;
    b.ref(0, 3) = 0.0f;
    b.ref(1, 3) = 0.0f;
    b.ref(2, 3) = 0.0f;
    PSMTXInverse(*b.toMtx(), *b.toMtx());
}

void HairCtrl::lockString(float (*mtx)[4], const JGeometry::TVec3<f32>& vec, f32 lock)
{
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > inv;
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > norm;
    JGeometry::TVec3f nodePos[2];
    JGeometry::TVec3f tmp2[2];
    JGeometry::TVec3f tmp[2];
    makeFaceNormalizeMtx(mtx, norm, inv);
    SysDebug::checkInvalidMatrix(*(Mtx*)inv.toMtx(), "hairInvMtx");
    for (u8 i = 0; i < 2; i++)
    {
        u8 no = i + 1;
        mNodeMgr->getNodePos(no, &nodePos[i]);
        tmp2[i].sub(nodePos[i], vec);
        PSMTXMultVec(*(Mtx*)inv.toMtx(), &tmp2[i], &tmp[i]);
        if (tmp[i].x > lock * i)
            tmp[i].x = lock * i;
        PSMTXMultVec(*(Mtx*)norm.toMtx(), &tmp[i], &tmp2[i]);
        nodePos[i].add(vec, tmp2[i]);
        mNodeMgr->setNodePos(no, nodePos[i]);
        SysDebug::checkInvalidVector(&nodePos[i], "hair_nodePos");
    }
}

void DonkeyTieCtrl::lockStringTieMin(float (*mtx)[4], const JGeometry::TVec3<f32>& vec, f32 min, bool flag)
{
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > inv;
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > norm;
    JGeometry::TVec3f nodePos[2];
    JGeometry::TVec3f tmp2[2];
    JGeometry::TVec3f tmp[2];
    makeFaceNormalizeMtx(mtx, norm, inv);
    for (u8 i = 0; i < 2; i++)
    {
        u8 no = i + 1;
        mNodeMgr->getNodePos(no, &nodePos[i]);
        tmp2[i].sub(nodePos[i], vec);
        PSMTXMultVec(*(Mtx*)inv.toMtx(), &tmp2[i], &tmp[i]);
        if (flag)
            tmp[i].y = 0.0f;
        else if (tmp[i].y < min)
            tmp[i].y = min;
        PSMTXMultVec(*(Mtx*)norm.toMtx(), &tmp[i], &tmp2[i]);
        nodePos[i].add(vec, tmp2[i]);
        mNodeMgr->setNodePos(no, nodePos[i]);
        SysDebug::checkInvalidVector(&nodePos[i], "tie_nodePos");
    }
}

void HairCtrl::lockStringMin(float (*mtx)[4], const JGeometry::TVec3<f32>& vec, f32 max, f32 min)
{
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > inv;
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > norm;
    JGeometry::TVec3f nodePos[2];
    JGeometry::TVec3f tmp2[2];
    JGeometry::TVec3f tmp[2];
    makeFaceNormalizeMtx(mtx, norm, inv);
    for (u8 i = 0; i < 2; i++)
    {
        u8 no = i + 1;
        mNodeMgr->getNodePos(no, &nodePos[i]);
        tmp2[i].sub(nodePos[i], vec);
        PSMTXMultVec(*(Mtx*)inv.toMtx(), &tmp2[i], &tmp[i]);
        if (tmp[i].x > max)
            tmp[i].x = max;
        else if (tmp[i].x < min)
            tmp[i].x = min;
        PSMTXMultVec(*(Mtx*)norm.toMtx(), &tmp[i], &tmp2[i]);
        nodePos[i].add(vec, tmp2[i]);
        mNodeMgr->setNodePos(no, nodePos[i]);
    }
}

void HairCtrl::update_hair1_common(float (*mtx)[4], DriverModel* driver)
{
    JGeometry::TVec3f dir(mtx[0][2], mtx[1][2], mtx[2][2]);
    stVecNormalize(dir);
    JGeometry::TVec3f p0;
    JGeometry::TVec3f p1;
    mNodeMgr->getNodePos(0, &p0);
    mNodeMgr->getNodePos(1, &p1);
    JGeometry::TVec3f d;
    d.sub(p1, p0);
    stVecNormalize(d);
    JGeometry::TVec3f right;
    PSVECCrossProduct(&d, &dir, &right);
    stVecNormalize(right);
    JGeometry::TVec3f up;
    PSVECCrossProduct(&d, &right, &up);
    stVecNormalize(up);
    mMtx[0][0] = d.x;
    mMtx[1][0] = d.y;
    mMtx[2][0] = d.z;
    mMtx[0][1] = right.x;
    mMtx[1][1] = right.y;
    mMtx[2][1] = right.z;
    mMtx[0][2] = up.x;
    mMtx[1][2] = up.y;
    mMtx[2][2] = up.z;
    PSMTXConcat((Mtx&)mMtx, *(Mtx*)(*(u8**)((u8*)driver + 0x4e0) + 0x78), (Mtx&)mMtx);
    JGeometry::TVec3f p0b;
    mNodeMgr->getNodePos(0, &p0b);
    mMtx[0][3] = p0b.x;
    mMtx[1][3] = p0b.y;
    mMtx[2][3] = p0b.z;
}

void HairCtrl::update_hair1(float (*)[4], float (*)[4], DriverModel*)
{
}

void HairCtrl::update_hair2(float (*)[4], DriverModel*)
{
}

void HairCtrl::calcHairGravity(const f32 ratio, DriverModel* driver, float (*mtx)[4])
{
    if (driver->mLumbarCtrl->m70 & 4)
    {
        JGeometry::TVec3f up(mtx[1][0], mtx[1][1], mtx[1][2]);
        JGeometry::TVec3f g(mtx[0][0], mtx[1][0], mtx[2][0]);
        up.scale(ratio);
        g.add(up);
        stVecNormalize(g);
        (JGeometry::TVec3f&)mNodeMgr->_28 = g;
    }
    else
    {
        (JGeometry::TVec3f&)mNodeMgr->_28 = JGeometry::TVec3f(0.0f, 1.0f, 0.0f);
    }
}

void HairCtrl::calcHairMtx(u32, DriverModel*)
{
}

PeachHairCtrl::PeachHairCtrl(int a, u8 b) : HairCtrl(a, b)
{
    mNodeMgr = new ExStringNodeManager(3, sPStringLength[0], true, false, 0);
    u32 i;
    StringNodeManager* mgr = mNodeMgr;
    for (i = 0; i < mgr->mStrNodeList.getNumLinks() - 1; i++)
    {
        f32 len = sPStringLength[i];
        mgr->getStringNodeAt(i)->_34 = len;
    }
    mNodeMgr->_24 = sPStringAdjustRatio;
    mNodeMgr->_20 = sPStringAirFric;
    mNodeMgr->_1c = sPStringGravity;
    mNodeMgr->_44 = sPStringMaxAccel;
    for (u32 j = 0; j < mNodeMgr->mStrNodeList.getNumLinks(); j++)
        mNodeMgr->getStringNode(j)->_30 = 0;
}

void PeachHairCtrl::update_hair1(float (*mtx1)[4], float (*mtx2)[4], DriverModel* driver)
{
    JGeometry::TVec3f pos(mtx2[0][3], mtx2[1][3], mtx2[2][3]);
    if (m44)
    {
        initString(pos, sPStringLength[0]);
        m44 = 0;
    }
    calcHairGravity(sPDamageGravityDirRatio, driver, mtx1);
    calcHairString(mtx1, pos);
    f32 min = -1000.0f;
    if (mDriverNo != ItemObjMgr::getNowTandemDriverNum(mKartNo))
        min = sPMinOffsetX;
    lockStringMin(mtx1, pos, sPMaxOffsetX, min);
    update_hair1_common(mtx1, driver);
}

void PeachHairCtrl::update_hair2(float (*)[4], DriverModel* driver)
{
    calcHairMtx(0, driver);
}

void PeachHairCtrl::calcHairMtx(u32 idx, DriverModel* driver)
{
    JGeometry::TVec3f z(((JGeometry::TPos3f*)mMtx)->at(0, 2), ((JGeometry::TPos3f*)mMtx)->at(1, 2), ((JGeometry::TPos3f*)mMtx)->at(2, 2));
    JGeometry::TVec3f p0;
    JGeometry::TVec3f p1;
    JGeometry::TVec3f p2;
    JGeometry::TVec3f d;
    JGeometry::TVec3f y;
    JGeometry::TVec3f x;
    mNodeMgr->getNodePos(0, &p0);
    mNodeMgr->getNodePos(idx, &p1);
    mNodeMgr->getNodePos(idx + 2, &p2);
    d.sub(p2, p1);
    stVecNormalize(d);
    d.scale(sPHairLength);
    d.scale(driver->mLumbarCtrl->mScale.x);
    p1.add(p0, d);
    stVecNormalize(d);
    PSVECCrossProduct(&d, &z, &y);
    stVecNormalize(y);
    PSVECCrossProduct(&d, &y, &x);
    stVecNormalize(x);
    mMtx[0][0] = d.x;
    mMtx[1][0] = d.y;
    mMtx[2][0] = d.z;
    mMtx[0][1] = y.x;
    mMtx[1][1] = y.y;
    mMtx[2][1] = y.z;
    mMtx[0][2] = x.x;
    mMtx[1][2] = x.y;
    mMtx[2][2] = x.z;
    PSMTXConcat(mMtx, driver->mLumbarCtrl->mScaleMtx, mMtx);
    setMtxTrans(mMtx, p1);
}

void PeachHairCtrl::calcHairString(float (*)[4], const JGeometry::TVec3<f32>& pos)
{
    mNodeMgr->setNodePos(0, JGeometry::TVec3f(pos));
    mNodeMgr->calc();
}

DaisyHairCtrl::DaisyHairCtrl(int a, u8 b) : HairCtrl(a, b)
{
    mNodeMgr = new StringNodeManager(3, sDStringLength, true, false, 0);
    for (u32 i = 0; i < mNodeMgr->mStrNodeList.getNumLinks(); i++)
        mNodeMgr->getStringNode(i)->_30 = 0;
    mNodeMgr->_1c = sDStringGravity;
    mNodeMgr->_24 = sDStringAdjustRatio;
}

void DaisyHairCtrl::update_hair1(float (*mtx1)[4], float (*mtx2)[4], DriverModel* driver)
{
    JGeometry::TVec3f pos(mtx2[0][3], mtx2[1][3], mtx2[2][3]);
    if (m44)
    {
        initString(pos, sDStringLength);
        m44 = 0;
    }
    calcHairGravity(sPDamageGravityDirRatio, driver, mtx1);
    calcHairString(mtx1, pos);
    lockString(mtx1, pos, sDMaxOffsetX);
    update_hair1_common(mtx1, driver);
    SysDebug::checkInvalidMatrix(mMtx, "daisyHairMtx 1");
}

void DaisyHairCtrl::update_hair2(float (*)[4], DriverModel* driver)
{
    JGeometry::TPos3f rot;
    JGeometry::TVec3f z(((JGeometry::TPos3f*)mMtx)->at(0, 2), ((JGeometry::TPos3f*)mMtx)->at(1, 2), ((JGeometry::TPos3f*)mMtx)->at(2, 2));
    JGeometry::TVec3f p0;
    JGeometry::TVec3f p1;
    JGeometry::TVec3f p2;
    JGeometry::TVec3f d;
    JGeometry::TVec3f y;
    JGeometry::TVec3f x;
    mNodeMgr->getNodePos(0, &p0);
    mNodeMgr->getNodePos(1, &p1);
    mNodeMgr->getNodePos(2, &p2);
    d.sub(p2, p1);
    d.sub(p2, p1);
    d.scale(driver->mLumbarCtrl->mScale.x);
    p1.add(p0, d);
    stVecNormalize(d);
    PSVECCrossProduct(&d, &z, &y);
    stVecNormalize(y);
    PSVECCrossProduct(&d, &y, &x);
    stVecNormalize(x);
    mMtx[0][0] = d.x;
    mMtx[1][0] = d.y;
    mMtx[2][0] = d.z;
    mMtx[0][1] = y.x;
    mMtx[1][1] = y.y;
    mMtx[2][1] = y.z;
    mMtx[0][2] = x.x;
    mMtx[1][2] = x.y;
    mMtx[2][2] = x.z;
    PSMTXConcat(mMtx, driver->mLumbarCtrl->mScaleMtx, mMtx);
    JGeometry::TPos3f* m = (JGeometry::TPos3f*)mMtx;
    m->ref(0, 3) = 0.0f;
    m->ref(1, 3) = 0.0f;
    m->ref(2, 3) = 0.0f;
    rot.setRotate(x, 0.017453292f * sDaisyHair2RotAngle);
    rot.ref(0, 3) = 0.0f;
    rot.ref(1, 3) = 0.0f;
    rot.ref(2, 3) = 0.0f;
    PSMTXConcat(*rot.toMtx(), mMtx, mMtx);
    mMtx[0][3] = p1.x;
    m->ref(1, 3) = p1.y;
    m->ref(2, 3) = p1.z;
    SysDebug::checkInvalidMatrix(mMtx, "daisyHairMtx 2");
}

void DaisyHairCtrl::calcHairString(float (*)[4], const JGeometry::TVec3<f32>& pos)
{
    mNodeMgr->_1c = sDStringGravity;
    mNodeMgr->_24 = sDStringAdjustRatio;
    mNodeMgr->setNodePos(0, JGeometry::TVec3f(pos));
    mNodeMgr->calc();
}

KinopicoHairCtrl::KinopicoHairCtrl(int a, u8 b, KHairLR lr) : HairCtrl(a, b)
{
    mLR = lr;
    mNodeMgr = new ExStringNodeManager(4, sKStringLength[0], true, false, 0);
    u32 i;
    StringNodeManager* mgr = mNodeMgr;
    for (i = 0; i < mgr->mStrNodeList.getNumLinks() - 1; i++)
    {
        f32 len = sKStringLength[i];
        mgr->getStringNodeAt(i)->_34 = len;
    }
    mgr->_24 = sKStringAdjustRatio;
    mgr->_20 = sKStringAirFric;
    mgr->_1c = sKStringGravity;
    mgr->_44 = sKStringMaxAccel;
    for (u8 j = 0; j < mgr->mStrNodeList.getNumLinks(); j++)
        mgr->getStringNode(j)->_30 = 0;
    mHeadCol = new ObjColSphere(sKColHeadRadius, 1.0f);
    mHeadPos.zero();
}

void KinopicoHairCtrl::reset()
{
    m44 = 1;
    PSMTXIdentity(mMtx48);
    mHeadPos.zero();
}

void KinopicoHairCtrl::update_hair1(float (*mtx1)[4], float (*mtx2)[4], DriverModel* driver)
{
    f32 xOfs = sKColFaceOffsetXdir;
    f32 yOfs = sKColFaceOffsetYdir;
    JGeometry::TVec3f xdir(mtx1[0][0], mtx1[1][0], mtx1[2][0]);
    JGeometry::TVec3f ydir(-mtx1[0][1], -mtx1[1][1], -mtx1[2][1]);
    mHeadPos.set(mtx1[0][3], mtx1[1][3], mtx1[2][3]);
    xdir.scale(xOfs);
    mHeadPos.add(xdir);
    ydir.scale(yOfs);
    mHeadPos.add(ydir);
    JGeometry::TVec3f pos(mtx2[0][3], mtx2[1][3], mtx2[2][3]);
    if (m44)
    {
        initString(pos, sKStringLength[0]);
        m44 = 0;
    }
    calcHairString(mtx2, pos);
    lockStringKp(mtx1, pos);
    mMtx48.set(mtx1);
    mMtx48.ref(0, 3) = 0.0f;
    mMtx48.ref(1, 3) = 0.0f;
    mMtx48.ref(2, 3) = 0.0f;
    calcHairMtx(1, driver);
    offsetMtxPos(0);
}

void KinopicoHairCtrl::calcJointMtx(const u32 idx, JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >& mtx, DriverModel* driver)
{
    JGeometry::TVec3f d;
    JGeometry::TVec3f z(mMtx48.at(0, 2), mMtx48.at(1, 2), mMtx48.at(2, 2));
    JGeometry::TVec3f y;
    JGeometry::TVec3f p0;
    JGeometry::TVec3f p1;
    stVecNormalize(z);
    mNodeMgr->getNodePos(idx, &p0);
    mNodeMgr->getNodePos(idx + 1, &p1);
    d.sub(p1, p0);
    stVecNormalize(d);
    PSVECCrossProduct(&d, &z, &y);
    stVecNormalize(y);
    PSVECCrossProduct(&y, &d, &z);
    stVecNormalize(z);
    mtx.ref(0, 0) = d.x;
    mtx.ref(1, 0) = d.y;
    mtx.ref(2, 0) = d.z;
    mtx.ref(0, 1) = z.x;
    mtx.ref(1, 1) = z.y;
    mtx.ref(2, 1) = z.z;
    mtx.ref(0, 2) = y.x;
    mtx.ref(1, 2) = y.y;
    mtx.ref(2, 2) = y.z;
    PSMTXConcat(*mtx.toMtx(), driver->mLumbarCtrl->mScaleMtx, *mtx.toMtx());
    JGeometry::TVec3f top = mNodeMgr->getNodeObj(0)->mPos;
    JGeometry::TVec3f pos = mNodeMgr->getNodeObj(idx)->mPos;
    f32 s = driver->mLumbarCtrl->mScale.x;
    JGeometry::TVec3f diff;
    diff.sub(pos, top);
    diff.scale(s);
    pos.add(top, diff);
    mtx.ref(0, 3) = pos.x;
    mtx.ref(1, 3) = pos.y;
    mtx.ref(2, 3) = pos.z;
}

void KinopicoHairCtrl::update_hair2(float (*)[4], DriverModel* driver)
{
    calcHairMtx(2, driver);
    offsetMtxPos(1);
}

void KinopicoHairCtrl::offsetMtxPos(u32 idx)
{
    f32 s = sKStringOffset[idx];
    JGeometry::TVec3f vec(-mMtx[0][0], -mMtx[1][0], -mMtx[2][0]);
    vec.scale(s);
    mMtx[0][3] += vec.x;
    mMtx[1][3] += vec.y;
    mMtx[2][3] += vec.z;
}

void KinopicoHairCtrl::calcHairString(float (*mtx)[4], const JGeometry::TVec3<f32>& pos)
{
    JGeometry::TPos3f m;
    JGeometry::TVec3f nodePos;
    JGeometry::TVec3f push;
    JGeometry::TVec3f g;
    StringNodeManager* mgr = mNodeMgr;
    mHeadCol->setRadius(sKColHeadRadius);
    for (u32 i = 1; i < mgr->mStrNodeList.getNumLinks(); i++)
    {
        nodePos = mgr->getNodeObj(i)->mPos;
        ObjColSphere* col = mHeadCol;
        if (col->IsHitSphere(mHeadPos, nodePos, 5.0f))
        {
            push = col->getPos();
            push.scale(col->getBoundDepth());
            nodePos.add(push);
            mgr->setNodePos(i, nodePos);
        }
    }
    m.set(mtx);
    m.ref(0, 3) = 0.0f;
    m.ref(1, 3) = 0.0f;
    m.ref(2, 3) = 0.0f;
    g = sKGravityAngle;
    if (mLR == 1)
        g.y = -g.y;
    stVecNormalize(g);
    PSMTXMultVec(*m.toMtx(), &g, &g);
    mNodeMgr->setNodePos(0, pos);
    (JGeometry::TVec3f&)mNodeMgr->_28 = g;
    mNodeMgr->calc();
}

void KinopicoHairCtrl::calcHairMtx(u32 idx, DriverModel* driver)
{
    calcJointMtx(idx, *(JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >*)&mMtx, driver);
}

void KinopicoHairCtrl::lockStringKp(float (*mtx)[4], const JGeometry::TVec3<f32>& vec)
{
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > inv;
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > norm;
    JGeometry::TVec3f nodePos[3];
    JGeometry::TVec3f tmp2[3];
    JGeometry::TVec3f tmp[3];
    JGeometry::TVec3f vel;
    makeFaceNormalizeMtx(mtx, norm, inv);
    for (u8 i = 0; i < 3; i++)
    {
        u8 no = i + 1;
        mNodeMgr->getNodePos(no, &nodePos[i]);
        tmp2[i].sub(nodePos[i], vec);
        PSMTXMultVec(*(Mtx*)inv.toMtx(), &tmp2[i], &tmp[i]);
        bool clamped = false;
        if (tmp[i].y < 0.0f)
        {
            tmp[i].y = 0.0f;
            clamped = true;
        }
        PSMTXMultVec(*(Mtx*)norm.toMtx(), &tmp[i], &tmp2[i]);
        nodePos[i].add(vec, tmp2[i]);
        mNodeMgr->setNodePos(no, nodePos[i]);
        if (clamped)
        {
            mNodeMgr->getNodeVel(no, &vel);
            if (clamped)
                vel.y *= sKLockStringVelDownRatio;
            mNodeMgr->setNodeVel(no, vel);
        }
    }
}

DonkeyTieCtrl::DonkeyTieCtrl(int a, u8 b) : HairCtrl(a, b)
{
    mNodeMgr = new ExStringNodeManager(3, sDKStringLength[0], true, false, 0);
    u32 i;
    StringNodeManager* mgr = mNodeMgr;
    for (i = 0; i < mgr->mStrNodeList.getNumLinks() - 1; i++)
    {
        f32 len = sDKStringLength[i];
        mgr->getStringNodeAt(i)->_34 = len;
    }
    mNodeMgr->_24 = sDKStringAdjustRatio;
    mNodeMgr->_20 = sDKStringAirFric;
    mNodeMgr->_1c = sDKStringGravity;
    mNodeMgr->_44 = sDKStringMaxAccel;
    for (u32 j = 0; j < mgr->mStrNodeList.getNumLinks(); j++)
        mNodeMgr->getStringNode(j)->_30 = 0;
}

void DonkeyTieCtrl::update_hair1(float (*mtx1)[4], float (*mtx2)[4], DriverModel* driver)
{
    JGeometry::TVec3f pos(mtx2[0][3], mtx2[1][3], mtx2[2][3]);
    if (m44)
    {
        initString(pos, sDKStringLength[0]);
        m44 = 0;
    }
    calcHairString(mtx1, pos);
    bool flag = false;
    if (mDriverNo != ItemObjMgr::getNowTandemDriverNum(mKartNo))
        flag = true;
    lockStringTieMin(mtx1, pos, sDKMinOffsetX, flag);
    update_hair1_common(mtx1, driver);
    JGeometry::TVec3f xdir;
    ObjUtility::getKartXdir(mKartNo, &xdir);
    xdir.scale(100.0f);
}

void DonkeyTieCtrl::calcHairString(float (*)[4], const JGeometry::TVec3<f32>& pos)
{
    mNodeMgr->setNodePos(0, JGeometry::TVec3f(pos));
    mNodeMgr->calc();
}

namespace {
    const char* sParamFileNames[20] = {
        "Baby_Mario", "Baby_Luigi", "Patapata", "Nokonoko", "Peach", "Daisy", "Mario", "Luigi", "Wario", "Waluigi",
        "Yoshi", "Catherine", "Donkey", "Diddy", "Koopa", "KoopaJr", "Kinopio", "Kinopico", "KingTeresa", "BossPakkun"
    };
}

CharIKParamMgr::CharIKParamMgr()
{
    mMgr = this;
    mParams = (void**)new CharIKParamMgr*[20];
    for (u8 i = 0; i < 20; i++)
        mParams[i] = 0;
    load();
}

void CharIKParamMgr::load()
{
    for (int kart = 0; kart < RaceMgr::sRaceManager->getKartNumber(); kart++)
    {
        for (u8 driver = 0; driver < 2; driver++)
        {
            int charID = RaceMgr::sRaceManager->getKartInfo(kart)->getDriverCharID(driver);
            int idx = charID - 1;
            char buf[64];
            snprintf(buf, 64, "/IK/%s.ikp", getFileName(idx));
            mParams[idx] = ObjUtility::getPtrMainArc(buf);
        }
    }
}

const char* CharIKParamMgr::getFileName(u32 idx)
{
    return sParamFileNames[idx];
}

void CharIKParamMgr::setParam(u8)
{
}
#include "JSystem/JAudio/JASFakeMatch2.h"
