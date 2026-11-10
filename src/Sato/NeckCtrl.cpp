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

#include <stdio.h>
#include "JSystem/JMath/JMath.h"

static const f32 sIdentQuat[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
static u8 sResetState = 4;
static u8 sSearchInitState = 4;
static u8 sSearchHitState = 4;
static u8 sSearchRabbitState = 4;
static u8 sReturnState = 4;
static u8 sFollowState = 4;
static u8 sLookState = 4;
static u8 sLookShineState = 4;
static u8 sNeckFuncNum = 5;
static JGeometry::TVec3f sKGravityAngle(32.0f, -0.4f, 0.0f);

namespace {
    f32 sKStringOffset[2] = { 12.0f, 9.0f };
    f32 sDStringLength = 20.0f;
    f32 sDStringAdjustRatio = 0.2f;
    f32 sDStringGravity = -1.5f;
}

#define NECK_MAX_ASSERT(i, max) \
    ((i) < (max)) ? (void)0 : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "StringObj.h", 109, "range over: %d <= num=%d < %d", 0, (i), (max)), OSHalt("Halt"))

static NeckFunc sInitFuncs[5] = {
    { &NeckCtrl::initFunc_Search },
    { &NeckCtrl::initFunc_Follow },
    { &NeckCtrl::initFunc_Look },
    { &NeckCtrl::initFunc_Return },
    { &NeckCtrl::initFunc_LookShine }
};

static NeckFunc sDoFuncs[5] = {
    { &NeckCtrl::doFunc_Search },
    { &NeckCtrl::doFunc_Follow },
    { &NeckCtrl::doFunc_Look },
    { &NeckCtrl::doFunc_Return },
    { &NeckCtrl::doFunc_LookShine }
};

Mtx NeckCtrl::sRotFrontDirMtx;

namespace {
        const char sName0[] __attribute__((section(".sdata2"))) = "Baby_Mario";
        const char sName1[] __attribute__((section(".sdata2"))) = "Baby_Luigi";
        const char sName2[] __attribute__((section(".sdata2"))) = "Patapata";
        const char sName3[] __attribute__((section(".sdata2"))) = "Nokonoko";
        const char sName4[] __attribute__((section(".sdata2"))) = "Peach";
        const char sName5[] __attribute__((section(".sdata2"))) = "Daisy";
        const char sName6[] __attribute__((section(".sdata2"))) = "Mario";
        const char sName7[] __attribute__((section(".sdata2"))) = "Luigi";
        const char sName8[] __attribute__((section(".sdata2"))) = "Wario";
        const char sName9[] __attribute__((section(".sdata2"))) = "Waluigi";
        const char sName10[] __attribute__((section(".sdata2"))) = "Yoshi";
        const char sName11[] __attribute__((section(".sdata2"))) = "Catherine";
        const char sName12[] __attribute__((section(".sdata2"))) = "Donkey";
        const char sName13[] __attribute__((section(".sdata2"))) = "Diddy";
        const char sName14[] __attribute__((section(".sdata2"))) = "Koopa";
        const char sName15[] __attribute__((section(".sdata2"))) = "KoopaJr";
        const char sName16[] __attribute__((section(".sdata2"))) = "Kinopio";
        const char sName17[] __attribute__((section(".sdata2"))) = "Kinopico";
        const char sName18[] __attribute__((section(".sdata2"))) = "KingTeresa";
        const char sName19[] __attribute__((section(".sdata2"))) = "BossPakkun";
    const char* sParamFileNames[20] = {
        sName0, sName1, sName2, sName3, sName4, sName5, sName6, sName7, sName8, sName9, sName10, sName11, sName12, sName13, sName14, sName15, sName16, sName17, sName18, sName19
    };
}

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
    m40 = 0;
    m42 = 1;
    mTimer = 0;
    mCount = 0;
    mCount = sNeckFuncNum;
    mSelf = this;
    mVec48 = new NeckFunc[mCount];
    mVec4c = new NeckFunc[mCount];
    m41 = 0;
    m40 = 0;
    m42 = 1;
    mTimer = 0;
    NECK_MAX_ASSERT(0, mCount);
    mVec48[0] = sInitFuncs[0];
    mVec4c[0] = sDoFuncs[0];
    NECK_MAX_ASSERT(1, mCount);
    mVec48[1] = sInitFuncs[1];
    mVec4c[1] = sDoFuncs[1];
    NECK_MAX_ASSERT(2, mCount);
    mVec48[2] = sInitFuncs[2];
    mVec4c[2] = sDoFuncs[2];
    NECK_MAX_ASSERT(3, mCount);
    mVec48[3] = sInitFuncs[3];
    mVec4c[3] = sDoFuncs[3];
    NECK_MAX_ASSERT(4, mCount);
    mVec48[4] = sInitFuncs[4];
    mVec4c[4] = sDoFuncs[4];
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
    m41 = 0;
    m40 = 0;
    m42 = 1;
    mTimer = 0;
    if (RaceMgr::sRaceManager->getRaceMode() == (ERaceMode)7)
    {
        m41 = m40;
        m40 = sResetState;
        m42 = 1;
        mTimer = 0;
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

#pragma inline_depth(4)
void NeckCtrl::calcFrontDirQt()
{
    JGeometry::TVec3f a;
    JGeometry::TVec3f b;
    ObjUtility::getKartPos(mKartNo, &a);
    ObjUtility::getKartXdir(mKartNo, &b);
    ObjUtility::getKartYdir(mKartNo, &b);
    ObjUtility::getKartZdir(mKartNo, &b);
    ObjUtility::getKartVel(mKartNo, &b);
    ObjUtility::getKartEfctVel(mKartNo, &b);
    for (int i = 0; i < mCount; i++)
        mPtr108[i] = 0;
}
#pragma inline_depth(4)

void NeckCtrl::calcTargetPosDirQt(const JGeometry::TVec3<f32>&)
{
}

void NeckCtrl::initFunc_Search()
{
    mED = 0;
    if (GeographyObjManager::gMgr->mIsEscapeBattle && GeoRabbitMark::getSupervisor()->getRabbitKartNo() != mKartNo)
    {
        m41 = m40;
        m40 = sSearchInitState;
        m42 = 1;
        mTimer = 0;
    }
}

#pragma inline_depth(0)
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
            m41 = m40;
            m40 = sSearchHitState;
            m42 = 1;
            mTimer = 0;
        }
    }
    if (GeographyObjManager::gMgr->mIsEscapeBattle && GeoRabbitMark::getSupervisor()->getRabbitKartNo() != mKartNo)
    {
        m41 = m40;
        m40 = sSearchRabbitState;
        m42 = 1;
        mTimer = 0;
    }
}

#pragma inline_depth(4)
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
        stVecNormalize(&dir);
        up.set(0.0f, 1.0f, 0.0f);
        right.set(1.0f, 0.0f, 0.0f);
        PSVECCrossProduct(&up, &dir, &right);
        stVecNormalize(&right);
        PSVECCrossProduct(&dir, &right, &up);
        stVecNormalize(&up);
        Mtx mtx;
        stMakeRMtx(mtx, right, up, dir);
        stMtx2Qt(&mQuat, mtx);
    }
    if (mTimer >= 10)
    {
        m41 = m40;
        m40 = sFollowState;
        m42 = 1;
        mTimer = 0;
    }
}

void NeckCtrl::initFunc_Look()
{
}

void NeckCtrl::doFunc_Look()
{
    JGeometry::TVec3f rivalPos;
    ObjUtility::getKartPos(mFC, &rivalPos);
    if (!mED)
    {
        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mKartNo, &kartPos);
        JGeometry::TVec3f dir;
        dir.sub(rivalPos, kartPos);
        stVecNormalize(&dir);
        JGeometry::TVec3f right(1.0f, 0.0f, 0.0f);
        JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
        PSVECCrossProduct(&up, &dir, &right);
        stVecNormalize(&right);
        PSVECCrossProduct(&dir, &right, &up);
        stVecNormalize(&up);
        Mtx mtx;
        stMakeRMtx(mtx, right, up, dir);
        stMtx2Qt(&mQuat, mtx);
    }
    if (mTimer >= 30)
    {
        m41 = m40;
        m40 = sLookState;
        m42 = 1;
        mTimer = 0;
    }
}

void NeckCtrl::initFunc_Return()
{
}

#pragma inline_depth(0)
void NeckCtrl::doFunc_Return()
{
    calcFrontDirQt();
    if (mTimer >= 20)
    {
        m41 = m40;
        m40 = sReturnState;
        m42 = 1;
        mTimer = 0;
    }
}

#pragma inline_depth(4)
void NeckCtrl::initFunc_LookShine()
{
}

#pragma inline_depth(0)
void NeckCtrl::doFunc_LookShine()
{
    if (GeoRabbitMark::getSupervisor()->getRabbitKartNo() == mKartNo)
    {
        m41 = m40;
        m40 = sLookShineState;
        m42 = 1;
        mTimer = 0;
        return;
    }
    if (mDriverNo != ItemObjMgr::getNowTandemDriverNum(mKartNo))
    {
        calcFrontDirQt();
        return;
    }
    JGeometry::TVec3f markPos;
    GeoRabbitMark::getSupervisor()->getMarkPosition(&markPos);
    if (!mED)
    {
        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mKartNo, &kartPos);
        JGeometry::TVec3f dir;
        dir.sub(markPos, kartPos);
        stVecNormalize(&dir);
        JGeometry::TVec3f right(1.0f, 0.0f, 0.0f);
        JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
        PSVECCrossProduct(&up, &dir, &right);
        stVecNormalize(&right);
        PSVECCrossProduct(&dir, &right, &up);
        stVecNormalize(&up);
        Mtx mtx;
        stMakeRMtx(mtx, right, up, dir);
        stMtx2Qt(&mQuat, mtx);
    }
}

#pragma inline_depth(4)
void NeckCtrl::calcTargetQt(float (*)[4])
{
}

int NeckCtrl::searchRivalKart()
{
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;
    ObjUtility::getKartPos(mKartNo, &pos);
    ObjUtility::getKartZdir(mKartNo, &dir);
    return ObjUtility::findNearKartAngle(pos, dir, 180.0f, 90.0f);
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

void NeckCtrl::update(float (*)[4], float (*)[4], LumbarCtrl*, u32)
{
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
    volatile f32 pad[3];
    volatile JGeometry::TVec3f scale;
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
    makeFaceNormalizeMtx(mtx, norm, inv);
    SysDebug::checkInvalidMatrix(*inv.toMtx(), "hairInvMtx");
    JGeometry::TVec3f nodePos[2];
    JGeometry::TVec3f tmp[2];
    JGeometry::TVec3f tmp2[2];
    for (u8 i = 0; i < 2; i++)
    {
        mNodeMgr->getNodePos(i + 1, &nodePos[i]);
        PSMTXMultVec(*inv.toMtx(), &nodePos[i], &tmp[i]);
        f32 lim = lock * i;
        if (tmp[i].x > lim)
            tmp[i].x = lim;
        PSMTXMultVec(*norm.toMtx(), &tmp[i], &tmp2[i]);
        JGeometry::TVec3f out;
        out.add(nodePos[i], tmp2[i]);
        mNodeMgr->setNodePos(i + 1, out);
        SysDebug::checkInvalidVector(&nodePos[i], "hair_nodePos");
    }
}

void DonkeyTieCtrl::lockStringTieMin(float (*)[4], const JGeometry::TVec3<f32>&, f32, bool)
{
}

void HairCtrl::lockStringMin(float (*mtx)[4], const JGeometry::TVec3<f32>& vec, f32 max, f32 min)
{
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > inv;
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > norm;
    makeFaceNormalizeMtx(mtx, norm, inv);
    JGeometry::TVec3f nodePos[2];
    JGeometry::TVec3f tmp[2];
    JGeometry::TVec3f tmp2[2];
    for (u8 i = 0; i < 2; i++)
    {
        mNodeMgr->getNodePos(i + 1, &nodePos[i]);
        PSMTXMultVec(*inv.toMtx(), &nodePos[i], &tmp[i]);
        if (tmp[i].x > max)
            tmp[i].x = max;
        else if (tmp[i].x < min)
            tmp[i].x = min;
        PSMTXMultVec(*norm.toMtx(), &tmp[i], &tmp2[i]);
        JGeometry::TVec3f out;
        out.add(nodePos[i], tmp2[i]);
        mNodeMgr->setNodePos(i + 1, out);
    }
}

void HairCtrl::update_hair1_common(float (*mtx)[4], DriverModel* driver)
{
    JGeometry::TVec3f dir(mtx[0][2], mtx[1][2], mtx[2][2]);
    stVecNormalize(&dir);
    JGeometry::TVec3f p0;
    JGeometry::TVec3f p1;
    mNodeMgr->getNodePos(0, &p0);
    mNodeMgr->getNodePos(1, &p1);
    JGeometry::TVec3f d;
    d.sub(p1, p0);
    stVecNormalize(&d);
    JGeometry::TVec3f right;
    PSVECCrossProduct(&d, &dir, &right);
    stVecNormalize(&right);
    JGeometry::TVec3f up;
    PSVECCrossProduct(&d, &right, &up);
    stVecNormalize(&up);
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
    mNodeMgr->setNodePos(0, JGeometry::TVec3f(pos));
    mNodeMgr->_1c = sDStringGravity;
    mNodeMgr->_24 = sDStringAdjustRatio;
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

void KinopicoHairCtrl::lockStringKp(float (*)[4], const JGeometry::TVec3<f32>&)
{
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
