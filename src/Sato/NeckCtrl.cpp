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

#include <stdio.h>
#include "JSystem/JMath/JMath.h"

static const f32 sIdentQuat[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
static JGeometry::TVec3f sKGravityAngle(32.0f, -0.4f, 0.0f);

namespace {
    u8 sNeckBackFwMaxWaitCnt = 15;
    u8 sNeckBackBwMaxWaitCnt = 30;
}

Mtx NeckCtrl::sRotFrontDirMtx;

static inline void mulVec3(register const JGeometry::TVec3f* a, register const JGeometry::TVec3f* b, register JGeometry::TVec3f* d)
{
    register f32 axy, az, bxy, bz, rxy, rz;
    __asm {
        psq_l axy, 0(a), 0, 0
        psq_l az, 8(a), 1, 0
        psq_l bxy, 0(b), 0, 0
        psq_l bz, 8(b), 1, 0
        ps_mul rxy, axy, bxy
        ps_mul rz, az, bz
        psq_st rxy, 0(d), 0, 0
        psq_st rz, 8(d), 1, 0
    }
}
static inline JGeometry::TVec3f addVec(const JGeometry::TVec3f& a, const JGeometry::TVec3f& b) { JGeometry::TVec3f r; r.add(a, b); return r; }

IKCtrl::IKCtrl(int index, u8 driverNo)
{
    register int idx;
    register u8 drv;
    asm { mr idx, r4; mr drv, r5; }
    PSMTXIdentity((Mtx&)mMtx);
    mKartNo = idx;
    mDriverNo = drv;
    mParam = (NeckIKParam*)CharIKParamMgr::mMgr->mParams[RaceMgr::sRaceManager->getKartInfo(idx)->getDriverCharID(drv) - 1];
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
    f32 sin = JMath::sincosTable_.mTable[1579].first;
    f32 cos = JMath::sincosTable_.mTable[1579].second;
    sRotFrontDirMtx[0][0] = cos;
    sRotFrontDirMtx[0][1] = 0.0f;
    sRotFrontDirMtx[0][2] = sin;
    sRotFrontDirMtx[0][3] = 0.0f;
    sRotFrontDirMtx[1][0] = 0.0f;
    sRotFrontDirMtx[1][1] = 1.0f;
    sRotFrontDirMtx[1][2] = 0.0f;
    sRotFrontDirMtx[1][3] = 0.0f;
    sRotFrontDirMtx[2][0] = -sin;
    sRotFrontDirMtx[2][1] = 0.0f;
    sRotFrontDirMtx[2][2] = cos;
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
    ((StateObserverT<NeckCtrl>*)0)->~StateObserverT();
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
    bool back = isTandemBack(this, isTandemDriver(this));
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
            f32 sn = JMath::sincosTable_.sinShort(angle);
            f32 cs = JMath::sincosTable_.cosShort(angle);
            rot[2][1] = 0.0f;
            rot[1][1] = 1.0f;
            rot[1][2] = 0.0f;
            rot[1][0] = 0.0f;
            rot[0][2] = sn;
            rot[0][0] = cs;
            rot[2][0] = -sn;
            rot[2][2] = cs;
            rot[0][1] = 0.0f;
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
        u32 bit = 1 << i;
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
        if (mFC >= 0 && !(m104 & (1 << mFC)))
        {
            m104 |= 1 << mFC;
            mPtr108[mFC] = 0;
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

void NeckCtrl::checkLerpRatio(u32)
{
}

static inline void stAsin(f32 x, f32& out)
{
    if (x < 0.0f)
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
        pitch = 1.5707964f;
        roll = 0.0f;
    }
    else if (zy + 1.0f <= eps)
    {
        yaw = JGeometry::TUtilf::atan2(m[0][2], m[0][0]);
        pitch = -1.5707964f;
        roll = 0.0f;
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
    f32 sr;
    f32 sy;
    f32 sp;
    f32 cr;
    f32 cy;
    f32 cp;
    cp = cos(pitch);
    cy = cos(yaw);
    cr = cos(roll);
    sp = sin(pitch);
    sy = sin(yaw);
    sr = sin(roll);

    f32 t7;
    f32 t6;
    f32 t5;
    f32 t4;
    f32 t3;
    f32 t2;
    f32 t1;
    f32 t0;

    t6 = cy * cp;
    t5 = -sp;
    t7 = sr * sy;
    t1 = sr * t6;
    t0 = -cr;
    t2 = t7 * cp;
    t4 = sr * cy;
    t3 = sy * cp;
    t7 = t6 - t7 * sp;
    t6 = t5 * cr;
    t5 = sp * t4 + t3;
    t4 = sp * cy + t2;
    m[0][0] = t7;
    t3 = cr * cp;
    t2 = sy * sp - t1;
    m[0][1] = t6;
    t1 = t0 * sy;
    t0 = cr * cy;
    m[0][2] = t5;
    m[2][1] = sr;
    m[1][0] = t4;
    m[1][1] = t3;
    m[1][2] = t2;
    m[2][0] = t1;
    m[2][2] = t0;
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

    f32 yaw;
    f32 pitch;
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
            if (stAbs(yaw - max) < 0.3f)
            {
                yaw = max;
                unlock = true;
            }
        }
        else if (mF0 < 0.0f && yaw < 0.0f)
        {
            f32 min = mParam->mC;
            if (stAbs(yaw - min) < 0.3f)
            {
                yaw = min;
                unlock = true;
            }
        }
        if (unlock)
            mED = 0;
    }

    if (yaw > 0.0f)
    {
        if (yaw > mParam->m8)
        {
            yaw = mParam->m8;
            yawClamped = true;
        }
    }
    else
    {
        if (yaw < 0.0f && yaw < mParam->mC)
        {
            yaw = mParam->mC;
            yawClamped = true;
        }
    }

    f32 ratio = 1.0f - JGeometry::TUtilf::abs(yaw / mParam->m8);
    f32 clampedRatio = 1.0f;
    if (ratio <= 1.0f)
        clampedRatio = ratio;
    if (clampedRatio < 0.0f)
        clampedRatio = 0.0f;
    f32 scale = (1.0f - mParam->m60) * clampedRatio + mParam->m60;

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
        mF8 = mF8 + 0.0001f * mF0;
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
    Mtx rotB;
    Mtx rotA;
    Mtx rotC;
    Quaternion q;
    Quaternion lerp;
    Quaternion q2;
    Quaternion qb;
    Quaternion qa;

    if (!mEC)
    {
        mEC = 1;
        stMtx2Qt(&mQuatPrev, ObjUtility::getKartBodyMtx(mKartNo));
        mQuat = mQuatPrev;
        stMTXRotDeg(rotA, 'Z', 180.0f);
        stMTXRotDeg(rotB, 'Y', 90.0f);
        stMtx2Qt(&qa, rotA);
        stMtx2Qt(&qb, rotB);
        PSMTXConcat(rotB, rotA, (Mtx&)mMtx88);
        PSQUATMultiply(&qb, &qa, &mQuat78);

        int id = RaceMgr::sRaceManager->getKartInfo(mKartNo)->getDriverCharID(mDriverNo);
        f32 ang;
        if (id == KOOPATROOPA)
            ang = 90.0f;
        else if (id == PARATROOPA)
            ang = 90.0f;
        else
            ang = -90.0f;
        stMTXRotDeg(rotC, 'X', ang);
        stMTXRotDeg(rotB, 'Y', ang);
        PSMTXConcat(rotB, rotC, (Mtx&)mMtxB8);
        mED = 0;
    }

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

    if (mode != 0)
    {
        switch (mode)
        {
        case 2:
            mF4 = 1.0f;
            break;
        case 1:
            mF4 = 0.2f;
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
}

HairCtrl::HairCtrl(int index, u8 driverNo) : IKCtrl(index, driverNo)
{
    mNodeMgr = 0;
    m44 = 1;
}

void HairCtrl::reset()
{
    m44 = 1;
}

void HairCtrl::initString(const JGeometry::TVec3<f32>&, f32)
{
}

void HairCtrl::makeFaceNormalizeMtx(float (*mtx)[4], JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >& a, JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >& b)
{
    PSMTXCopy(mtx, *a.toMtx());
    volatile JGeometry::TVec3f scale;
    JGeometry::TVec3f eps(0.0001f, 0.0001f, 0.0001f);
    scale.x = eps.x; scale.y = eps.y; scale.z = eps.z;
    scale.x = JGeometry::TUtilf::sqrt(a.at(0, 0) * a.at(0, 0) + a.at(1, 0) * a.at(1, 0) + a.at(2, 0) * a.at(2, 0));
    scale.y = JGeometry::TUtilf::sqrt(a.at(0, 1) * a.at(0, 1) + a.at(1, 1) * a.at(1, 1) + a.at(2, 1) * a.at(2, 1));
    scale.z = JGeometry::TUtilf::sqrt(a.at(0, 2) * a.at(0, 2) + a.at(1, 2) * a.at(1, 2) + a.at(2, 2) * a.at(2, 2));
    Mtx smtx;
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
    JGeometry::TVec3f tmp[2];
    JGeometry::TVec3f tmp2[2];
    makeFaceNormalizeMtx(mtx, norm, inv);
    SysDebug::checkInvalidMatrix(*(Mtx*)inv.toMtx(), "hairInvMtx");
    for (u8 i = 0; i < 2; i++)
    {
        mNodeMgr->getNodePos(i + 1, &nodePos[i]);
        mulVec3(&nodePos[i], &vec, &tmp2[i]);
        PSMTXMultVec(*(Mtx*)inv.toMtx(), &tmp2[i], &tmp[i]);
        f32 lim = lock * i;
        if (tmp[i].x > lim)
            tmp[i].x = lim;
        PSMTXMultVec(*(Mtx*)norm.toMtx(), &tmp[i], &tmp2[i]);
        mNodeMgr->setNodePos(i + 1, JGeometry::TVec3f(nodePos[i].x + tmp2[i].x, nodePos[i].y + tmp2[i].y, nodePos[i].z + tmp2[i].z));
        SysDebug::checkInvalidVector(&nodePos[i], "hair_nodePos");
    }
}

void DonkeyTieCtrl::lockStringTieMin(float (*mtx)[4], const JGeometry::TVec3<f32>& vec, f32 min, bool flag)
{
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > inv;
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > norm;
    JGeometry::TVec3f nodePos[2];
    JGeometry::TVec3f tmp[2];
    JGeometry::TVec3f tmp2[2];
    makeFaceNormalizeMtx(mtx, norm, inv);
    for (u8 i = 0; i < 2; i++)
    {
        mNodeMgr->getNodePos(i + 1, &nodePos[i]);
        mulVec3(&nodePos[i], &vec, &tmp2[i]);
        PSMTXMultVec(*(Mtx*)inv.toMtx(), &tmp2[i], &tmp[i]);
        if (flag)
            tmp[i].y = 0.0f;
        else if (tmp[i].y < min)
            tmp[i].y = min;
        PSMTXMultVec(*(Mtx*)norm.toMtx(), &tmp[i], &tmp2[i]);
        mNodeMgr->setNodePos(i + 1, JGeometry::TVec3f(nodePos[i].x + tmp2[i].x, nodePos[i].y + tmp2[i].y, nodePos[i].z + tmp2[i].z));
        SysDebug::checkInvalidVector(&nodePos[i], "hair_nodePos");
    }
}

void HairCtrl::lockStringMin(float (*mtx)[4], const JGeometry::TVec3<f32>& vec, f32 max, f32 min)
{
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > inv;
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > norm;
    JGeometry::TVec3f nodePos[2];
    JGeometry::TVec3f tmp[2];
    JGeometry::TVec3f tmp2[2];
    makeFaceNormalizeMtx(mtx, norm, inv);
    for (u8 i = 0; i < 2; i++)
    {
        mNodeMgr->getNodePos(i + 1, &nodePos[i]);
        tmp2[i].mul(vec, nodePos[i]);
        PSMTXMultVec(*(Mtx*)inv.toMtx(), &tmp2[i], &tmp[i]);
        if (tmp[i].x > max)
            tmp[i].x = max;
        else if (tmp[i].x < min)
            tmp[i].x = min;
        PSMTXMultVec(*(Mtx*)norm.toMtx(), &tmp[i], &tmp2[i]);
        JGeometry::TVec3f out;
        out.add(nodePos[i], tmp2[i]);
        mNodeMgr->setNodePos(i + 1, out);
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

void HairCtrl::calcHairGravity(f32, DriverModel*, float (*)[4])
{
}

void HairCtrl::calcHairMtx(u32, DriverModel*)
{
}

PeachHairCtrl::PeachHairCtrl(int a, u8 b) : HairCtrl(a, b)
{
}

void PeachHairCtrl::update_hair1(float (*)[4], float (*)[4], DriverModel*)
{
}

void PeachHairCtrl::update_hair2(float (*)[4], DriverModel* driver)
{
    calcHairMtx(0, driver);
}

void PeachHairCtrl::calcHairMtx(u32, DriverModel*)
{
}

void PeachHairCtrl::calcHairString(float (*)[4], const JGeometry::TVec3<f32>& pos)
{
    mNodeMgr->setNodePos(0, JGeometry::TVec3f(pos));
    mNodeMgr->calc();
}

DaisyHairCtrl::DaisyHairCtrl(int a, u8 b) : HairCtrl(a, b)
{
    mNodeMgr = new StringNodeManager(3, sDStringLength, true, false, 0);
    u32 i;
    u32 num;
    u32 dummy = 0;
    for (i = 0; i < mNodeMgr->mStrNodeList.getNumLinks(); i++)
    {
        num = mNodeMgr->mStrNodeList.getNumLinks();
        dummy = i;
        if (i >= (int)num)
            (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "StringObj.h", 112, "range over: %d <= num=%d < %d", 0, i, num), OSHalt("Halt"));
        ((StringNode*)mNodeMgr->mStrNodeList.getNthLink(i)->getObjectPtr())->_30 = 0;
    }
    mNodeMgr->_1c = sDStringGravity;
    mNodeMgr->_24 = sDStringAdjustRatio;
    mNodeMgr->_3c = dummy;
}

void DaisyHairCtrl::update_hair1(float (*)[4], float (*)[4], DriverModel*)
{
}

void DaisyHairCtrl::update_hair2(float (*)[4], DriverModel*)
{
}

void DaisyHairCtrl::calcHairString(float (*)[4], const JGeometry::TVec3<f32>& pos)
{
    mNodeMgr->_1c = sDStringGravity;
    mNodeMgr->_24 = sDStringAdjustRatio;
    mNodeMgr->setNodePos(0, JGeometry::TVec3f(pos));
    mNodeMgr->calc();
}

KinopicoHairCtrl::KinopicoHairCtrl(int a, u8 b, KHairLR) : HairCtrl(a, b)
{
}

void KinopicoHairCtrl::reset()
{
    m44 = 1;
    PSMTXIdentity(mMtx48);
    m88 = 0.0f;
    m84 = 0.0f;
    m80 = 0.0f;
}

void KinopicoHairCtrl::update_hair1(float (*)[4], float (*)[4], DriverModel*)
{
}

void KinopicoHairCtrl::calcJointMtx(u32, JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >&, DriverModel*)
{
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

void KinopicoHairCtrl::calcHairString(float (*)[4], const JGeometry::TVec3<f32>&)
{
}

#pragma inline_depth(0)
void KinopicoHairCtrl::calcHairMtx(u32 idx, DriverModel* driver)
{
    calcJointMtx(idx, *(JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >*)&mMtx, driver);
}

void KinopicoHairCtrl::lockStringKp(float (*mtx)[4], const JGeometry::TVec3<f32>& vec)
{
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > inv;
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > norm;
    makeFaceNormalizeMtx(mtx, norm, inv);
    JGeometry::TVec3f out;
    JGeometry::TVec3f vel;
    Vec tmp[3];
    Vec tmp2[3];
    Vec nodePos[3];
    for (u8 i = 0; i < 3; i++)
    {
        mNodeMgr->getNodePos(i + 1, (JGeometry::TVec3f*)&nodePos[i]);
        tmp2[i] = nodePos[i];
        PSMTXMultVec((Mtx&)inv, &tmp2[i], &tmp[i]);
        bool clamped = false;
        if (tmp[i].y < 0.0f)
        {
            tmp[i].y = 0.0f;
            clamped = true;
        }
        PSMTXMultVec((Mtx&)norm, &tmp[i], &tmp2[i]);
        out.add(nodePos[i], tmp2[i]);
        mNodeMgr->setNodePos(i + 1, out);
        if (clamped)
        {
            mNodeMgr->getNodeVel(i + 1, (JGeometry::TVec3f*)&vel);
            if (clamped)
                vel.y *= sKLockStringVelDownRatio;
            mNodeMgr->setNodeVel(i + 1, vel);
        }
    }
}

DonkeyTieCtrl::DonkeyTieCtrl(int a, u8 b) : HairCtrl(a, b)
{
}

void DonkeyTieCtrl::update_hair1(float (*)[4], float (*)[4], DriverModel*)
{
}

#pragma inline_depth(4)
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
