#include "Sato/stEfctRoad.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "Kawano/driver.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/NeckCtrl.h"
#include "Sato/ObjUtility.h"
#include "Sato/StringObj.h"
#include "Sato/stMath.h"
#include "Yamamoto/kartCtrl.h"

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

class LumbarCtrl : public IKCtrl
{
public:
    StringNodeManager *getStringNodeMgr() { return mStringNodeMgr; }

    u8 _40[0xb8 - 0x40];
    StringNodeManager *mStringNodeMgr; // 0xb8
};

namespace
{
    f32 sSpdMin = 15.0f;
    f32 sSpdMax = 60.0f;
    f32 sSpdRatio = 0.95f;
    f32 sNormSpdRatio = 0.5f;
    f32 sSpdSmokeRatio = 0.95f;
    f32 sOffsetScale = 20.0f;
    f32 sSpdDif = 1.0f / (sSpdMax - sSpdMin);
    f32 sChipSpdMax = 40.0f;
    f32 sChipSpdRatio = 0.975f;
    f32 sDriftStartScale = 1.6f;
    u8 sMaxDriftStartFrame = 4;
    JGeometry::TVec3f sDriftSmokeOffsetPos(0.0f, 0.0f, 0.0f);
    JGeometry::TVec3f sDriftVelDir(0.35f, 1.0f, -0.4f);
    f32 sChipSpdDif = 1.0f / (sChipSpdMax - sSpdMin);
    JGeometry::TVec3f sDriftChipOffsetPos(0.0f, 0.0f, 0.0f);
    JGeometry::TVec3f sDriftChipVelDir(0.0f, 1.0f, -0.75f);
    JGeometry::TVec3f sDriftDirType0(0.4f, 1.0f, -0.4f);
    JGeometry::TVec3f sDriftDirType1(0.35f, 1.0f, -0.4f);
    stDriftKartData sDriftKartData[21] = {
        {1.05f, -15.0f, &sDriftDirType0}, {1.15f, -25.0f, &sDriftDirType0}, {1.05f, -15.0f, &sDriftDirType0},
        {0.95f, -10.0f, &sDriftDirType0}, {1.05f, -20.0f, &sDriftDirType1}, {0.95f, -10.0f, &sDriftDirType1},
        {1.2f, -15.0f, &sDriftDirType1},  {1.2f, -30.0f, &sDriftDirType1},  {1.05f, -15.0f, &sDriftDirType0},
        {0.95f, -10.0f, &sDriftDirType1}, {1.05f, -15.0f, &sDriftDirType0}, {0.95f, -10.0f, &sDriftDirType0},
        {1.05f, -20.0f, &sDriftDirType1}, {0.95f, -10.0f, &sDriftDirType1}, {1.05f, -20.0f, &sDriftDirType0},
        {0.95f, -15.0f, &sDriftDirType0}, {0.95f, -15.0f, &sDriftDirType1}, {0.95f, -15.0f, &sDriftDirType1},
        {1.0f, -15.0f, &sDriftDirType0},  {1.0f, -15.0f, &sDriftDirType0},  {1.2f, -15.0f, &sDriftDirType1},
    };
    f32 sTS_GMax = 80.0f;
    f32 sTS_GRatio = 0.95f;
    JGeometry::TVec3f sTireSpinOffsetPos(0.0f, 5.0f, -10.0f);
    f32 sSpinTurnSpdRatio = 0.25f;
    f32 sSpinTurnMaxSpd = 10.0f;
    f32 sSpinTurnMinSpd = 2.0f;
    f32 sSpinTurnDif = 1.0f / (sSpinTurnMaxSpd - sSpinTurnMinSpd);
    f32 sDirtSpdRatio = 0.3f;
    f32 sCSnowSpdRatio = 0.2f;
    f32 sSmokeCenterOffsetY = 50.0f;
    f32 sSmokeCenterOffsetZ = -100.0f;
    f32 sSnowCenterOffsetY = 65.0f;
    f32 sSnowCenterOffsetZ;
    f32 sCharCenterSpdMin = 5.0f;
    f32 sCharCenterSpdMax = 30.0f;
    f32 sCharCenterSpdRatio = 0.9f;
    f32 sCharCenterSpdDif = 1.0f / (sCharCenterSpdMax - sCharCenterSpdMin);
}

f32 stEfctTireSpin::sMinSideG = 50.0f;

#include <JSystem/JAudio/JASFakeMatch2.h>

int stEfctRoadBase::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    bool ret = false;
    if (_24 == 0) {
        ret = getTirePos(pos);
        if (!ret)
            return ret;

        JGeometry::TVec3f offset;
        offset.x = m[0][0];
        offset.y = m[1][0];
        offset.z = m[2][0];
        if (_22 == 3)
            offset.negate();
        offset.scale(sOffsetScale);
        *pos += offset;
    }
    return ret;
}

stEfctRoad::stEfctRoad()
{
    ipData.Init(4, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::Life);
    ipData.AddFunc(stEmitterParamLineIP::CreateRate);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    ipData.AddFunc(stEmitterParamLineIP::FirstVel);
}

void stEfctRoad::calc()
{
    strategy(sNormSpdRatio, sSpdMax, sSpdMin, sSpdDif, false, 0.95f);
}

stEfctRoadDrift::stEfctRoadDrift(s16 kartNo, s16 tireNo)
{
    ipData.Init(4, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::Life);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    kartNum = kartNo;
    _22 = tireNo;
    u32 kartID = RaceMgr::getManager()->getKartInfo(kartNo)->getKartID();
#line 258
    JUT_MAX_ASSERT(kartID, 21);
    mKartData = &sDriftKartData[kartID];
}

void stEfctRoadDrift::calc()
{
    JGeometry::TPos3f mtx;
    JGeometry::TVec3f src(*mKartData->mDir);
    JGeometry::TVec3f velDir;
    JGeometry::TVec3f dir;
    dir = src;
    mtx.set(ObjUtility::getKartBodyMtx(kartNum));
    if (_22 == 3)
        dir.x = -dir.x;
    mtx.zeroTrans();
    PSMTXMultVec(mtx, &dir, &dir);
    stVecNormalize(dir);
    velDir = dir;
    _04->mFirstVelDir = velDir;
    strategy(sSpdRatio, sSpdMax, sSpdMin, sSpdDif, true, 3.0f);
    _04->mParticle.mDynamics._08 = mKartData->mScale * (mFrame < sMaxDriftStartFrame ? sDriftStartScale : 1.0f);
    if (mFrame < sMaxDriftStartFrame)
        mFrame++;
}

int stEfctRoadDrift::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    JGeometry::TVec3f offset(0.0f, 0.0f, mKartData->mOffsetZ);
    return (u8)getDriftPos(pos, offset);
}

stEfctTireSpin::stEfctTireSpin()
{
    ipData.Init(4, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::Life);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
    ipData.AddFunc(stEmitterParamLineIP::FirstVel);
}

void stEfctTireSpin::calc()
{
    f32 tireG = __fabsf(GetKartCtrl()->GeTireG(kartNum));
    calc_lineIp(tireG, sTS_GRatio, sTS_GMax, sMinSideG, 1.0f / (sTS_GMax - sMinSideG), true, 1.0f);
}

int stEfctTireSpin::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    return (u8)getDriftPos(pos, sTireSpinOffsetPos);
}

stEfctSpinTurn::stEfctSpinTurn()
{
    ipData.Init(4, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::Life);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
    ipData.AddFunc(stEmitterParamLineIP::FirstVel);
}

void stEfctSpinTurn::calc()
{
    strategy(sSpinTurnSpdRatio, sSpinTurnMaxSpd, sSpinTurnMinSpd, sSpinTurnDif, true, 1.0f);
}

int stEfctSpinTurn::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    return (u8)getDriftPos(pos, sTireSpinOffsetPos);
}

stEfctRoadDriftSmoke::stEfctRoadDriftSmoke()
{
    ipData.Init(7, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    ipData.AddFunc(stEmitterParamLineIP::FirstVel);
    ipData.AddFunc(stEmitterParamLineIP::Life);
    ipData.AddFunc(stEmitterParamLineIP::LifeRndRate);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::SizeLast);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
}

void stEfctRoadDriftSmoke::init() {}

void stEfctRoadDriftSmoke::calc()
{
    strategy(sSpdSmokeRatio, sSpdMax, sSpdMin, sSpdDif, true, 1.0f);
}

int stEfctRoadDriftSmoke::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    return (u8)getDriftPos(pos, sDriftSmokeOffsetPos);
}

stEfctRoadDriftChip::stEfctRoadDriftChip()
{
    ipData.Init(7, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::FirstVel);
    ipData.AddFunc(stEmitterParamLineIP::AirFric);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    ipData.AddFunc(stEmitterParamLineIP::Life);
    ipData.AddFunc(stEmitterParamLineIP::Gravity);
    ipData.AddFunc(stEmitterParamLineIP::FirstVelRndAngle);
}

void stEfctRoadDriftChip::calc()
{
    JGeometry::TPos3f mtx;
    JGeometry::TVec3f src(sDriftChipVelDir);
    JGeometry::TVec3f velDir;
    JGeometry::TVec3f dir;
    dir = src;
    mtx.set(ObjUtility::getKartBodyMtx(kartNum));
    if (_22 == 3)
        dir.x = -dir.x;
    mtx.zeroTrans();
    PSMTXMultVec(mtx, &dir, &dir);
    stVecNormalize(dir);
    velDir = dir;
    _04->mFirstVelDir = velDir;
    strategy(sChipSpdRatio, sChipSpdMax, sSpdMin, sChipSpdDif, true, 1.0f);
}

int stEfctRoadDriftChip::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    return (u8)getDriftPos(pos, sDriftChipOffsetPos);
}

stEfctDirtCenter::stEfctDirtCenter()
{
    mIsSnow = 0;
    ipData.Init(6, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    ipData.AddFunc(stEmitterParamLineIP::Life);
    ipData.AddFunc(stEmitterParamLineIP::LifeRndRate);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::SizeLast);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
    if (RaceMgr::getManager()->getCourse()->getCourseID() == SHERBET_LAND)
        mIsSnow = 1;
}

int stEfctDirtCenter::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    bool ret = true;
    JGeometry::TVec3f genPos;
    genPos.set(m[0][3], m[1][3], m[2][3]);
    CrsGround *ground = GetKartCtrl()->GetBodyGround(kartNum);
    if (m[1][3] > 150.0f + ground->getHeight()) {
        ret = false;
    } else {
        JGeometry::TVec3f ydir;
        JGeometry::TVec3f zdir;
        ObjUtility::getKartYdir(kartNum, &ydir);
        ObjUtility::getKartZdir(kartNum, &zdir);
        f32 offsetY;
        f32 offsetZ;
        if (mIsSnow == 1) {
            offsetY = sSnowCenterOffsetY;
            offsetZ = sSnowCenterOffsetZ;
        } else {
            offsetY = sSmokeCenterOffsetY;
            offsetZ = sSmokeCenterOffsetZ;
        }
        ydir.scale(offsetY);
        zdir.scale(offsetZ);
        genPos.y = ground->getHeight();
        genPos.add(ydir);
        genPos.add(zdir);
    }
    *pos = genPos;
    return ret;
}

void stEfctDirtCenter::calc()
{
    strategy(mIsSnow == 0 ? sDirtSpdRatio : sCSnowSpdRatio, sSpdMax, sSpdMin, sSpdDif, true, 0.95f);
}

static inline StringNodeManager *getCharStringNodeMgr(int kartNum, u8 driver)
{
    return RaceMgr::getManager()->getKartLoader(kartNum)->getExModelDriver(driver)->mLumbarCtrl->getStringNodeMgr();
}

static inline StringNode *getStringNode(StringNodeManager *mgr, u32 num)
{
    (0 <= num && num < mgr->mStrNodeList.getNumLinks())
        ? (void)0
        : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "StringObj.h", 103, "range over: %d <= num=%d < %d", 0,
                                      num, mgr->mStrNodeList.getNumLinks()),
           OSPanic("StringObj.h", 103, "Halt"));
    return mgr->mStrNodeList.getNth(num)->getObject();
}

int stEfctCharBase::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    s16 num = kartNum;
    StringNodeManager *mgr = getCharStringNodeMgr(num, ItemObjMgr::getNowTandemDriverNum(num));
    u8 ret = getStringNode(mgr, 4)->_31;
    if (ret)
        mgr->getNodePos(4, pos);
    return ret;
}

stEfctCharCenter::stEfctCharCenter()
{
    ipData.Init(3, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::CreateRate);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
}

void stEfctCharCenter::calc()
{
    strategy(sCharCenterSpdRatio, sCharCenterSpdMax, sCharCenterSpdMin, sCharCenterSpdDif, true, 0.95f);
}

stEfctCharWeedCenter::stEfctCharWeedCenter()
{
    ipData.Init(3, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::CreateRate);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    ipData.AddFunc(stEmitterParamLineIP::FirstVel);
}

void stEfctCharWeedCenter::calc()
{
    strategy(sCharCenterSpdRatio, sCharCenterSpdMax, sCharCenterSpdMin, sCharCenterSpdDif, true, 0.95f);
}
