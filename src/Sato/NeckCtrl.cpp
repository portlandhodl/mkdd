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

static const char* sParamFileNames[20] = {
    "Baby_Mario", "Baby_Luigi", "Patapata", "Nokonoko", "Peach",
    "Daisy", "Mario", "Luigi", "Wario", "Waluigi", "Yoshi",
    "Catherine", "Donkey", "Diddy", "Koopa", "KoopaJr", "Kinopio",
    "Kinopico", "KingTeresa", "BossPakkun"
};

IKCtrl::IKCtrl(int index, u8 driverNo)
{
    PSMTXIdentity((Mtx&)mMtx);
    mKartNo = index;
    mDriverNo = driverNo;
    mParam = (CharIKParamMgr*)CharIKParamMgr::mMgr->mParams[RaceMgr::sRaceManager->getKartInfo(index)->getDriverCharID(driverNo) - 1];
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

void NeckCtrl::checkMaxAngle(float (*)[4])
{
}

void NeckCtrl::update(float (*)[4], float (*)[4], LumbarCtrl*, u32)
{
}

HairCtrl::HairCtrl(int index, u8 driverNo) : IKCtrl(index, driverNo)
{
}

void HairCtrl::reset()
{
    m44 = 1;
}

void HairCtrl::initString(const JGeometry::TVec3<f32>&, f32)
{
}

void HairCtrl::makeFaceNormalizeMtx(float (*)[4], JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >&, JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >&)
{
}

void HairCtrl::lockString(float (*)[4], const JGeometry::TVec3<f32>&, f32)
{
}

void DonkeyTieCtrl::lockStringTieMin(float (*)[4], const JGeometry::TVec3<f32>&, f32, bool)
{
}

void HairCtrl::lockStringMin(float (*)[4], const JGeometry::TVec3<f32>&, f32, f32)
{
}

void HairCtrl::update_hair1_common(float (*)[4], DriverModel*)
{
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

void PeachHairCtrl::update_hair2(float (*)[4], DriverModel*)
{
}

void PeachHairCtrl::calcHairMtx(u32, DriverModel*)
{
}

void PeachHairCtrl::calcHairString(float (*)[4], const JGeometry::TVec3<f32>&)
{
}

DaisyHairCtrl::DaisyHairCtrl(int a, u8 b) : HairCtrl(a, b)
{
}

void DaisyHairCtrl::update_hair1(float (*)[4], float (*)[4], DriverModel*)
{
}

void DaisyHairCtrl::update_hair2(float (*)[4], DriverModel*)
{
}

void DaisyHairCtrl::calcHairString(float (*)[4], const JGeometry::TVec3<f32>&)
{
}

KinopicoHairCtrl::KinopicoHairCtrl(int a, u8 b, KHairLR) : HairCtrl(a, b)
{
}

void KinopicoHairCtrl::reset()
{
}

void KinopicoHairCtrl::update_hair1(float (*)[4], float (*)[4], DriverModel*)
{
}

void KinopicoHairCtrl::calcJointMtx(u32, JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >&, DriverModel*)
{
}

void KinopicoHairCtrl::update_hair2(float (*)[4], DriverModel*)
{
}

void KinopicoHairCtrl::offsetMtxPos(u32)
{
}

void KinopicoHairCtrl::calcHairString(float (*)[4], const JGeometry::TVec3<f32>&)
{
}

void KinopicoHairCtrl::calcHairMtx(u32, DriverModel*)
{
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

void DonkeyTieCtrl::calcHairString(float (*)[4], const JGeometry::TVec3<f32>&)
{
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
