#include "Sato/stEfctWater.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "Kawano/driver.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/stMath.h"
#include "Yamamoto/kartCtrl.h"
#include "JSystem/JMath/JMath.h"

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

namespace
{
    f32 sCharHeadOffsetY[20] = {
        60.0f, 60.0f, 49.0f, 49.0f, 38.0f, 36.0f, 59.0f, 67.0f, 69.0f, 63.0f,
        58.0f, 46.0f, 45.0f, 38.0f, 53.0f, 34.0f, 60.0f, 60.0f, 55.0f, 145.0f,
    };
    f32 sWaterSpraySpdRatio = 0.98f;
    f32 sWaterBubbleSpdRatio = 0.98f;
    f32 sWaterSprayPosRatio = 1.0f;
    f32 sWaterBubblePosRatio = 1.0f;
    f32 sWaterBubbleOffsetY = 50.0f;
    f32 sTstKartDownY = -30.0f;
    f32 sCrsWaterOffsetY = -20.0f;
    f32 sSpdMax = 35.0f;
    f32 sSpdMin = 15.0f;
    f32 sSpCenterMinSpd = 15.0f;
    f32 sSpCenterMaxSpd = 30.0f;
    f32 sSpdDif = 1.0f / (sSpdMax - sSpdMin);
    f32 sSpCenterDif = 1.0f / (sSpCenterMaxSpd - sSpCenterMinSpd);
}

#include <JSystem/JAudio/JASFakeMatch2.h>

static inline f32 getCharHeadY(int kartNum, u8 driver, f32 margin)
{
    ECharID charID = RaceMgr::getManager()->getKartInfo(kartNum)->getDriverCharID(driver);
    Mtx *mtx = (charID == 0x13) ? RaceMgr::getManager()->getKartLoader(kartNum)->getExModelDriver(driver)->getMtxP(1)
                                : RaceMgr::getManager()->getKartLoader(kartNum)->getExModelDriver(driver)->getMtxP(0x12);
#line 138
    JUT_MINMAX_ASSERT(0, charID, 21);
    return (*mtx)[1][3] + sCharHeadOffsetY[charID] - margin;
}

inline bool stEfctWaterBase::getCenterGeneratePos(JGeometry::TVec3f *pos, Mtx m, f32 margin)
{
    s16 num = kartNum;
    bool ret = false;
    pos->set(m[0][3], m[1][3], m[2][3]);
    CrsGround *ground = GetKartCtrl()->GetBodyGround(num);
    f32 waterHeight = GetKartCtrl()->GetBodyGround(num)->getWaterHeight();
    f32 headY = getCharHeadY(num, ItemObjMgr::getNowTandemDriverNum(num), margin);
    if (waterHeight > pos->y + sTstKartDownY && waterHeight < headY) {
        ret = true;
        pos->y = waterHeight;
    }
    return ret;
}

inline void stEfctWaterBase::setWaterVelDir()
{
    JGeometry::TVec3f dir;
    ObjUtility::getKartZdir(kartNum, &dir);
    dir.y = 0.0f;
    dir.negate();
    stVecNormalize(dir);
    dir.y = 1.0f;
    stVecNormalize(dir);
    JGeometry::TVec3f velDir(dir);
    _04->mFirstVelDir = velDir;
}

inline bool stEfctWaterBase::getTireGeneratePos(JGeometry::TVec3f *pos, f32 ratio, f32 offsetY)
{
    bool ret = false;
    int num = kartNum;
    JGeometry::TVec3f tirePos;
    getTirePos(&tirePos);
    f32 depth = ratio * (getWaterHeight(num) - tirePos.y);
    if (depth > 0.0f && depth < ratio * (2.0f * GetKartCtrl()->GetTireRadius(num) + offsetY)) {
        ret = true;
        tirePos.y += depth;
    } else {
        tirePos.y += ratio * (2.0f * GetKartCtrl()->GetTireRadius(num));
    }
    *pos = tirePos;
    return ret;
}

f32 stEfctWaterBase::getWaterHeight(int kartNum)
{
    return GetKartCtrl()->GetBodyGround(kartNum)->getWaterHeight();
}

stEfctWaterTrack::stEfctWaterTrack()
{
    ipData.Init(5, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    ipData.AddFunc(stEmitterParamLineIP::Life);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
    ipData.AddFunc(stEmitterParamLineIP::SizeLast);
}

void stEfctWaterTrack::calc()
{
    strategy(0.0f, sSpdMax, sSpdMin, sSpdDif, false, 0.95f);
}

int stEfctWaterTrack::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    bool ret = getCenterGeneratePos(pos, m, 0.0f);
    _28 = ret;
    _2c = *pos;
    return _28;
}

stEfctWaterSpray::stEfctWaterSpray()
{
    ipData.Init(6, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::CreateRate);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    ipData.AddFunc(stEmitterParamLineIP::FirstVel);
    ipData.AddFunc(stEmitterParamLineIP::Gravity);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
}

void stEfctWaterSpray::calc()
{
    calc_lineIpFollow(__fabsf(GetKartCtrl()->GetCarSpeed(kartNum)), sWaterSpraySpdRatio, sSpdMax, sSpdMin, sSpdDif, false,
                      0.9f);
    setWaterVelDir();
}

int stEfctWaterSpray::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    return getTireGeneratePos(pos, sWaterSprayPosRatio, 0.0f);
}

stEfctWaterSprayCenter::stEfctWaterSprayCenter()
{
    ipData.Init(5, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::CreateRate);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    ipData.AddFunc(stEmitterParamLineIP::FirstVel);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
}

void stEfctWaterSprayCenter::calc()
{
    calc_lineIpFollow(__fabsf(GetKartCtrl()->GetCarSpeed(kartNum)), sWaterSpraySpdRatio, sSpCenterMaxSpd,
                      sSpCenterMinSpd, sSpCenterDif, false, 0.9f);
    setWaterVelDir();
}

int stEfctWaterSprayCenter::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    bool ret = getCenterGeneratePos(pos, m, 0.0f);
    return ret;
}

stEfctWaterBubble::stEfctWaterBubble(bool useLife)
{
    u8 funcCnt = 4;
    if (useLife)
        funcCnt = 5;
    ipData.Init(funcCnt, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::FirstVel);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::SizeLast);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    if (useLife)
        ipData.AddFunc(stEmitterParamLineIP::Life);
}

void stEfctWaterBubble::calc()
{
    calc_lineIpFollow(__fabsf(GetKartCtrl()->GetCarSpeed(kartNum)), sWaterBubbleSpdRatio, sSpdMax, sSpdMin, sSpdDif, false,
                      0.9f);
    setWaterVelDir();
}

int stEfctWaterBubble::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    return getTireGeneratePos(pos, sWaterBubblePosRatio, sWaterBubbleOffsetY);
}

stEfctCrsWaterSpray::stEfctCrsWaterSpray() {}

int stEfctCrsWaterSpray::getGeneratePos(JGeometry::TVec3f *pos, Mtx m)
{
    return getTireGeneratePos(pos, sWaterSprayPosRatio, sWaterBubbleOffsetY);
}

f32 stEfctCrsWaterSpray::getWaterHeight(int kartNum)
{
    return GetKartCtrl()->GetBodyGround(kartNum)->getSplashHeight() + sCrsWaterOffsetY;
}

stEfctCrsWaterBubble::stEfctCrsWaterBubble() : stEfctWaterBubble(true) {}

f32 stEfctCrsWaterBubble::getWaterHeight(int kartNum)
{
    return GetKartCtrl()->GetBodyGround(kartNum)->getSplashHeight() + sCrsWaterOffsetY;
}
