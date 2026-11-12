#include "Sato/stEfctExhaust.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/ObjUtility.h"
#include "Sato/RivalBodyCtrl.h"
#include "Sato/RivalKart.h"
#include "Sato/stEffectMgr.h"
#include "Yamamoto/kartCtrl.h"
#include "Yamamoto/kartPad.h"

namespace
{
    f32 sYVelRatio = 0.8f;
    f32 sYVelRatio_Cannon = 0.99f;
    f32 sRpmMax = 0.8f;
    f32 sRpmMin = 0.01f;
    f32 sRpmRate = 1.0f / (sRpmMax - sRpmMin);
    u8 sAccelMaxCnt[2] = {10, 1};
    f32 sAccelAdd[2] = {0.15f, 1.0f};
    f32 sAccelDownRatio[2] = {0.96f, 0.9f};
    f32 sAccelLerpSpd[2] = {20.0f, 50.0f};
    u8 sMaxAccelInvalidCnt = 10;
    f32 sAccelLifeMag = 0.65f;
    f32 sAccelSizeMag = 1.0f;
    f32 sAccelAirFricMag = 0.01f;
    f32 sAccelFirstVelMag = 0.15f;
}

#include <JSystem/JAudio/JASFakeMatch2.h>

stExhaustAccelCounter::stExhaustAccelCounter()
{
    init();
}

void stExhaustAccelCounter::init()
{
    mAccel = 0.0f;
    mCount = 0;
    mInvalidCount = 0;
    mIsInvalid = 0;
    mTrigCount = 0;
    mIsTrig = 0;
    mSpdType = 0;
    mIsSpdTypeSet = 0;
}

void stExhaustAccelCounter::calc(int kartNo)
{
    u32 hold = 0;
    u32 trig = 0;
    if (ObjUtility::isPlayerDriver(kartNo)) {
        KartGamePad *pad = GetKartCtrl()->GetDriveCont(kartNo);
        u32 accelBtn = GetKartCtrl()->getKartPad(kartNo)->mAccelBtn;
        trig = accelBtn & pad->getTrigger();
        hold = accelBtn & pad->getButton();
    } else if (!(GetKartCtrl()->getKartEnemy(kartNo)->getBodyCtrl()->mFlags & 0x80)) {
        hold = 1;
    }

    f32 spd = GetStEfctMgr()->getEfctCarSpeed(kartNo);
    if (trig && !mIsInvalid && !mTrigCount) {
        if (!mIsSpdTypeSet) {
            if (spd >= sAccelLerpSpd[0])
                mSpdType = 1;
            else
                mSpdType = 0;
            mIsSpdTypeSet = 1;
        }
        mIsTrig = 1;
        mTrigCount = sAccelMaxCnt[mSpdType];
    }

    if ((hold || mIsTrig) && !mIsInvalid) {
        if (!mIsSpdTypeSet) {
            if (spd >= sAccelLerpSpd[0])
                mSpdType = 1;
            else
                mSpdType = 0;
            mIsSpdTypeSet = 1;
        }
        mCount++;
        if (mCount > sAccelMaxCnt[mSpdType] || !mTrigCount) {
            mIsInvalid = 1;
            mCount = 0;
            mInvalidCount = sMaxAccelInvalidCnt;
            mIsTrig = 0;
        }
        if (!mIsInvalid)
            mAccel += sAccelAdd[mSpdType];
    } else {
        f32 ratio = sAccelDownRatio[mSpdType];
        if (spd > sAccelLerpSpd[0] && spd < sAccelLerpSpd[1]) {
            ratio = sAccelDownRatio[0] - ((sAccelDownRatio[0] - sAccelDownRatio[1]) * (spd - sAccelLerpSpd[0])) /
                                             (sAccelLerpSpd[1] - sAccelLerpSpd[0]);
        }
        mAccel *= ratio;
        if (mInvalidCount)
            mInvalidCount--;
        if (!hold && !mInvalidCount) {
            mIsSpdTypeSet = 0;
            mIsInvalid = 0;
        }
    }

    if (mAccel > 1.0f)
        mAccel = 1.0f;
    if (mTrigCount)
        mTrigCount--;
}

stEfctExhaust::stEfctExhaust(u8 kartNo, u8 side)
{
    init();
    kartNum = kartNo;
    _22 = side;
    ipData.Init(11, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::RateStep);
    ipData.AddFunc(stEmitterParamLineIP::FirstVel);
    ipData.AddFunc(stEmitterParamLineIP::FirstVelRndAngle);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
    ipData.AddFunc(stEmitterParamLineIP::Size);
    ipData.AddFunc(stEmitterParamLineIP::SizeFirst);
    ipData.AddFunc(stEmitterParamLineIP::SizeLast);
    ipData.AddFunc(stEmitterParamLineIP::LifeRndRate);
    ipData.AddFunc(stEmitterParamLineIP::Life);
    ipData.AddFunc(stEmitterParamLineIP::Gravity);
    ipData.AddFunc(stEmitterParamLineIP::AirFric);
}

void stEfctExhaust::init()
{
    mIsStop = 0;
    mIsFirst = 1;
}

void stEfctExhaust::calc()
{
    if (mIsFirst) {
        stEmitterPtcl *firstEmt = _04;
        mLife = firstEmt->mLife;
        mAirFric = firstEmt->mAirFric;
        mIsFirst = 0;
    }

    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f emtPos;
    JGeometry::TVec3f emtVel;
    JGeometry::TVec3f emtDir;
    stEmitterPtcl *emt = _04;
    u8 num = kartNum;
    GetStEfctMgr()->getExhaustPosDir(&pos, &dir, num, _22);
    ObjUtility::getKartEfctVel(num, &vel);
    if (vel.length() < 4.0f)
        vel.zero();
    emtPos = pos;
    emt->_0c = emtPos;
    f32 yRatio;
    if (GetKartCtrl()->GetKartStatus(num) & 0x100000000ULL)
        yRatio = sYVelRatio_Cannon;
    else
        yRatio = sYVelRatio;
    vel.y *= yRatio;
    emtVel = vel;
    emt->_00 = emtVel;
    emtDir = dir;
    emt->mFirstVelDir = emtDir;
    calcParamByKartRpm();

    f32 waterHeight = GetKartCtrl()->GetWaterHeight(kartNum);
    if (!RaceMgr::getManager()->isAbleStart() || mIsStop || waterHeight < 0.0f) {
        emt->mFlags |= 0x400;
    } else {
        emt->mFlags &= ~0x400;
        doAccel();
    }
}

void stEfctExhaust::doAccel()
{
    s16 num = kartNum;
    f32 accel = GetStEfctMgr()->getExhaustAccelCounter(num)->mAccel;
    if (accel < 0.03f || RaceMgr::getManager()->isRaceEnd() || RaceMgr::getManager()->isKartGoal(num))
        return;

    stEmitterPtcl *emt = _04;
    emt->mRateStep = emt->mRateStep * (1.0f - accel);
    emt->mFirstVel *= sAccelFirstVelMag * accel + 1.0f;
    emt->mLife = mLife * (sAccelLifeMag * accel + 1.0f);
    f32 size = emt->mSizeX * (sAccelSizeMag * accel + 1.0f);
    emt->mSizeX = size;
    emt->mSizeY = size;
    emt->mAirFric = mAirFric * (1.0f - sAccelAirFricMag * accel);
}

void stEfctExhaust::calcParamByKartRpm()
{
    f32 rpm = GetKartCtrl()->GetKartRpm(kartNum);
    f32 rate = 0.0f;
    if (!RaceMgr::getManager()->isRaceEnd() && !RaceMgr::getManager()->isKartGoal(kartNum)) {
        if (rpm >= sRpmMin) {
            if (rpm >= sRpmMax)
                rate = 1.0f;
            else
                rate = sRpmRate * (rpm - sRpmMin);
        }
    } else {
        rate = 1.0f;
    }
    ipData.DoIp(rate);
}

void stEfctKinokoDashBase::init()
{
    mFrame = 0;
    mIsActive = 1;
    _04->mFlags &= ~0x200;
}

void stEfctKinokoDashBase::calc()
{
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f emtPos;
    JGeometry::TVec3f emtVel;
    JGeometry::TVec3f emtDir;
    u8 num = kartNum;
    GetStEfctMgr()->getExhaustPosDir(&pos, &dir, num, _22);
    stEmitterPtcl *emt = _04;
    emtPos = pos;
    emt->_0c = emtPos;
    ObjUtility::getKartEfctVel(num, &vel);
    emtVel = vel;
    emt->_00 = emtVel;
    emtDir = dir;
    emt->mFirstVelDir = emtDir;
    calcParamByFrame();
}

void stEfctKinokoDashBase::calcParamByFrame()
{
    f32 rate = 0.0f;
    mFrame++;
    if (mIsActive) {
        if (mFrame > 70 && mFrame <= 130)
            rate = mFrame / 130.0f;
        if (mFrame >= 130) {
            _25 = 1;
            mIsActive = 0;
        }
    } else {
        rate = 1.0f;
    }
    ipData.DoIp(rate);
}

stEfctKinokoDash1::stEfctKinokoDash1()
{
    ipData.Init(1, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::Size);
}

void stEfctKinokoDash1::init()
{
    stEfctKinokoDashBase::init();
    u8 side = _22;
    int num = kartNum;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;
    GetStEfctMgr()->getExhaustPosDir(&pos, &dir, num, side);
    JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x1d, num, pos, side);
    GetStEfctMgr()->getExhaustPosDir(&pos, &dir, num, side);
    JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x1e, num, pos, side);
}

stEfctKinokoDash2::stEfctKinokoDash2()
{
    ipData.Init(1, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::Life);
}

stEfctKinokoDash3::stEfctKinokoDash3()
{
    ipData.Init(1, nullptr, nullptr);
    ipData.AddFunc(stEmitterParamLineIP::AlphaStdRate);
}
