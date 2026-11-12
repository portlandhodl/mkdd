#include <stdlib.h>
#include <JSystem/JGeometry.h>
#include <JSystem/JUtility/JUTAssert.h>
#include <dolphin/mtx.h>

#include "Kaneshige/KartChecker.h"
#include "Kaneshige/Course/Course.h"
#include "Kaneshige/RaceInfo.h"
#include "Kaneshige/RaceMgr.h"

#include "Osako/ResMgr.h"

#include "Sato/ObjUtility.h"
#include "Sato/RivalBodyCtrl.h"
#include "Sato/RivalItemCtrl.h"
#include "Sato/RivalKart.h"
#include "Sato/RivalSpeedCtrl.h"

#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCtrl.h"

static const f32 sDummy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

class RivalStartRankParam
{
public:
    RivalStartRankParam(u8 first) : mFirstRank(first), mLink(this) {}
    u8 mFirstRank; // 0
    JSULink<RivalStartRankParam> mLink; // 4
}; // Size: 0x14

JSUList<RivalStartRankParam> RivalSpeedCtrl::sStartRankList;

namespace {

f32 sCurveSpdRatio[3] = { 0.25f, 0.45f, 1.0f };
f32 sKartRankOffsetSpdRatio[8] = { 1.1f, 1.0f, 0.8f, 0.7f, 0.7f, 0.7f, 0.7f, 0.7f };
f32 sKeepIntervalDistance[8] = { 0.0f, 0.03f, 0.06f, 0.1f, 0.15f, 0.2f, 0.25f, 0.3f };
f32 sKartDashSuccesRateEasy[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
f32 sKartDashSuccesRateNormal[8] = { 1.0f, 0.9f, 0.5f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f };
f32 sKartDashSuccesRateHard[8] = { 1.0f, 1.0f, 1.0f, 0.8f, 0.6f, 0.3f, 0.0f, 0.0f };
f32 sKartDashFailRateEasy[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.3f, 0.6f, 0.7f };
f32 sKartDashFailRateNormal[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.2f, 0.3f };
f32 sKartDashFailRateHard[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
RivalSpeedCtrl::EngineParam sEngineSmallParam = { { 0.97f, 0.98f, 0.985f, 0.0f, 5.0f, 0.001f, 0.01f, 5.0f, 0.005f, 0.25f, 0.6f, 10.0f, 0.05f }, sKartDashSuccesRateEasy, sKartDashFailRateEasy };
RivalSpeedCtrl::EngineParam sEngineNormalParam = { { 0.97f, 0.98f, 0.985f, -0.01f, 20.0f, 0.001f, 0.01f, 10.0f, 0.005f, 0.5f, 0.63f, 30.0f, 0.1f }, sKartDashSuccesRateNormal, sKartDashFailRateNormal };
RivalSpeedCtrl::EngineParam sEngineBigParam = { { 0.97f, 0.98f, 0.985f, -0.2f, 50.0f, 0.01f, 0.2f, 30.0f, 0.01f, 0.7f, 0.66f, 50.0f, 0.15f }, sKartDashSuccesRateHard, sKartDashFailRateHard };
f32 sKartKindAccelMag[21] = { 0.75f, 0.5f, 1.0f, 1.5f, 1.0f, 1.5f, 0.75f, 0.5f, 0.75f, 0.5f, 0.75f, 1.5f, 1.0f, 1.5f, 1.0f, 0.75f, 1.0f, 1.5f, 0.5f, 1.5f, 0.75f };

} // namespace

f32 RivalSpeedCtrl::sCrsLevelParam[3] = { 0.0f, 1.0f, 1.0f };

namespace {

f32 sReverseSpd = 30.0f;
f32 sLowestPlayerDistMaxDist = 0.1f;
f32 sCurveChkAngle = 10.0f;
f32 sCurveChkSpd = 70.0f;
f32 sCurveRatioStdSpd = 120.0f;
f32 sCurveMaxAngle = 45.0f;
f32 sCurveMaxDownRatio = 0.6f;
f32 sCurveConvergeSub = 4.0f;
f32 sCurveStdSpd = 200.0f;
f32 sAwardMaxSpd = 30.0f;
u16 sAwardStopFrame = 300;
f32 sFowartKartMinSpdRatio = 0.8f;
f32 sTopKartAwayStdRatio = 300.0f;
u8 sTopWaitBackKartRank = 3;
s16 sKartComStrengthPointWeakDif = -1025;
s16 sKartComStrengthPointStrongDif = 1024;
f32 sKartComAccelStdRatio = 1.0f;
f32 sAward2ndLengthRatio = 0.01f;
f32 sAward2ndAccelRatio = 2.0f;
f32 sAward2ndOffsetSpd = 25.0f;
f32 sAward2ndBrake = 0.07f;
f32 sAward2ndLength = 500.0f;
f32 sStaffRollDemoStdSpd = 60.0f;

inline s32 abs32(s32 v)
{
    s32 mask = v >> 31;
    return (mask ^ v) - mask;
}

inline f64 stConverge(f32 cur, f32 target, f32 up, f32 down)
{
    if (cur < target)
    {
        f32 v = cur + up;
        return __fsel(target - v, v, target);
    }
    f32 v = cur - down;
    return __fsel(target - v, target, v);
}

} // namespace

RivalSpeedCtrl::RivalSpeedCtrl(RivalKart *kart) : RivalCtrlBase(kart), JKRDisposer()
{
    s32 firstRank = -1;
    bool found = false;
    mIsPlayerKart = 0;
    mComWeakPointSwapFlg = 0;
    if (sComStrengthState == 2)
        mComWeakPointSwapFlg = 1;

    if (ObjUtility::isPlayerDriver(getMyID()))
    {
        firstRank = RCMGetManager()->getKartNumber() - getMyID();
    }
    else
    {
        if (sComStrengthState == 0)
        {
            int rivalNo = (sComWeakPointSwapFlg != 0) ? 1 : 0;
            int otherNo = (rivalNo == 0) ? 1 : 0;
            if (getMyID() == RCMGetManager()->getRivalKartNo(rivalNo))
            {
                mIsPlayerKart = 1;
                firstRank = 1;
                found = true;
            }
            else if (getMyID() == RCMGetManager()->getRivalKartNo(otherNo))
            {
                firstRank = 2;
                found = true;
            }
        }
        else
        {
            for (u8 i = 0; i < 2; i++)
            {
                int kartNo = RCMGetManager()->getRivalKartNo(i);
                if (kartNo == getMyID())
                {
                    if (i == 0)
                    {
                        mIsPlayerKart = 1;
                        firstRank = 1;
                    }
                    else
                    {
                        firstRank = 2;
                        switch (sComStrengthState)
                        {
                        case 2:
                            firstRank = 1;
                            break;
                        }
                    }
                    found = true;
                    break;
                }
            }
        }

        if (!found)
        {
#line 354
            JUT_ASSERT(sStartRankList.getNumLinks() != 0);
            u32 r = getRandom()->getRandomMax(sStartRankList.getNumLinks() - 1);
            RivalStartRankParam *p = (RivalStartRankParam *)sStartRankList.getNthLink(r)->getObjectPtr();
            firstRank = p->mFirstRank;
            sStartRankList.remove(&p->mLink);
        }
    }

#line 362
    JUT_MINMAX_ASSERT(0, firstRank, RCMGetManager()->getKartNumber() + 1);

    mRankParamAlloc = new RivalCtrlBase::RankParam(firstRank);
    mRankParam = mRankParamAlloc;
    getBodyCtrl()->mRankParam = mRankParamAlloc;

    int kartID = RCMGetManager()->getKartInfo(getMyID())->getKartID();
#line 370
    JUT_MINMAX_ASSERT(0, kartID, 21);

    if (!sConfusedFightModeFlg)
        mKartKindAccelMag = sKartKindAccelMag[kartID];
    else
        mKartKindAccelMag = 2.0f;
}

RivalSpeedCtrl::~RivalSpeedCtrl()
{
    sStartRankPtr = nullptr;
}

void RivalSpeedCtrl::StartInit()
{
    s32 *gpPoint = sRivalGPPoint;
    s32 maxPlayerDist = 0;
    s32 maxComDist = 0;
    sRivalGPPoint[0] = 0;
    sConfusedFightModeFlg = 0;
    sRivalGPPoint[1] = 0;
    sComStrengthState = 1;
    sComWeakPointSwapFlg = 0;

    for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
    {
        int dist = RCMGetManager()->mRaceInfo->getPointTable(i);
        if (ObjUtility::isPlayerDriver(i))
        {
            if (dist > maxPlayerDist)
                maxPlayerDist = dist;
        }
        else
        {
            if (dist > maxComDist)
                maxComDist = dist;
            if (i == RCMGetManager()->mRaceInfo->mRivalKarts[0])
                sRivalGPPoint[0] = dist;
            else if (i == RCMGetManager()->mRaceInfo->mRivalKarts[1])
                gpPoint[1] = dist;
        }
    }

    s32 dif = maxPlayerDist - maxComDist;
    if (dif <= sKartComStrengthPointWeakDif)
    {
        sComStrengthState = 0;
        if (sRivalGPPoint[0] > gpPoint[1])
            sComWeakPointSwapFlg = 1;
    }
    else if (RCMGetManager()->getRaceLevel() != 0)
    {
        if (dif >= sKartComStrengthPointStrongDif)
            sComStrengthState = 2;
    }

    InitRndStartRank();
    ResetInit();
}

void RivalSpeedCtrl::ResetInit()
{
    switch (RCMGetManager()->getRaceLevel())
    {
    case 0:
        sEngineParamPtr = &sEngineSmallParam;
        break;
    case 1:
        sEngineParamPtr = &sEngineNormalParam;
        break;
    case 2:
    case 3:
        sEngineParamPtr = &sEngineBigParam;
        break;
    }
#line 488
    JUT_ASSERT(sEngineParamPtr != 0);
}

void RivalSpeedCtrl::LoadCrsLevelParam()
{
    u8 *data = (u8 *)ResMgr::getPtr(ResMgr::mcArcMRAM, "/Enemy/EnemyKart.dat");
    u8 level = RCMGetManager()->getRaceLevel();
    if (level > 2)
        level = 2;
    s16 a = *(s16 *)(data + 2);
    s16 b = *(s16 *)(data + 4);
    s16 *p = (s16 *)(data + 4 + (((a * b + 2) * level + 1) << 1));
    u8 crs = getNowCrsIndex();
    p += a * crs;
    sCrsLevelParam[0] = p[0];
    sCrsLevelParam[1] = p[1] * 0.01f;
    sCrsLevelParam[2] = p[2] * 0.01f;
}

void RivalSpeedCtrl::InitRndStartRank()
{
    if (sStartRankPtr != 0)
        return;

    sStartRankList.initiate();

    u8 rivalNo;
    u8 numRivals = 0;
    for (rivalNo = 0; rivalNo < 2; rivalNo++)
    {
        if (RCMGetManager()->getRivalKartNo(rivalNo) >= 0)
            numRivals++;
    }

    u8 numPlayers = 0;
    for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
    {
        if (ObjUtility::isPlayerDriver(i))
            numPlayers++;
    }

    u8 allNum = RCMGetManager()->getKartNumber() - numPlayers - numRivals;
#line 557
    JUT_MINMAX_ASSERT(0, allNum, RCMGetManager()->getKartNumber() + 1);

    sStartRankPtr = new RivalStartRankParam *[allNum];
    for (u8 i = 0; i < allNum; i++)
    {
        ((RivalStartRankParam **)sStartRankPtr)[i] = new RivalStartRankParam(i + numRivals + 1);
        sStartRankList.append(&((RivalStartRankParam **)sStartRankPtr)[i]->mLink);
    }
}

int RivalSpeedCtrl::getNowCrsIndex()
{
    s32 ret = 0;
    s32 id = RCMGetCourse()->getCourseID() - 33;
    switch (id)
    {
    case 3: ret = 0; break;
    case 1: ret = 1; break;
    case 0: ret = 2; break;
    case 17: ret = 3; break;
    case 7: ret = 4; break;
    case 4: ret = 5; break;
    case 2: ret = 6; break;
    case 9: ret = 7; break;
    case 18: ret = 8; break;
    case 8: ret = 9; break;
    case 5: ret = 10; break;
    case 12: ret = 11; break;
    case 10: ret = 12; break;
    case 11: ret = 13; break;
    case 14: ret = 14; break;
    case 16: ret = 15; break;
    case 27: ret = 16; break;
    case 28: ret = 17; break;
    case 29: ret = 18; break;
    case 30: ret = 19; break;
    }
    return ret;
}

void RivalSpeedCtrl::reset()
{
    mAccelMag = 0.0f;
    mMaxSpeed = 0.0f;
    mStdMaxSpeed = 0.0f;
    mOffsetMaxSpeed = 0.0f;
    mBaseMaxSpeed = KartCtrl::getKartCtrl()->GetMaxSpeed(getMyID());
    mLimitMaxSpeed = mBaseMaxSpeed;
    mRank = -1;
    mNoAccelFlg = 0;
}

#line 623
void RivalSpeedCtrl::init()
{
    reset();
    mRankParam->reset();
    RivalCtrlBase::RankParam *pRank = mRankParam;
    JUT_MINMAX_ASSERT(0, (pRank->getFirstRank() - 1), 8);
    mAccelMag = mKartKindAccelMag;
    if (!sConfusedFightModeFlg)
        initStartDash();
}

void RivalSpeedCtrl::initStartDash()
{
    if (ObjUtility::isPlayerDriver(getMyID()))
        return;

    u8 dashType = 0;
    f32 rnd = getRandom()->get_ufloat_1();
    u8 rankid = getRankParamID();
#line 651
    JUT_MINMAX_ASSERT(0, rankid, 8);

    if (isRival())
    {
        f32 rate = sEngineParamPtr->mDashSuccessRate[rankid];
        if (rnd < rate || rate >= 1.0f)
        {
            mMaxSpeed = mLimitMaxSpeed;
            dashType = 1;
        }
    }
    else
    {
        if (0.0f < sEngineParamPtr->mDashSuccessRate[rankid] || sEngineParamPtr->mDashSuccessRate[rankid] >= 1.0f)
        {
            mMaxSpeed = mLimitMaxSpeed;
            dashType = 1;
        }
        else if (0.0f < sEngineParamPtr->mDashFailRate[rankid] || sEngineParamPtr->mDashFailRate[rankid] >= 1.0f)
        {
            dashType = 2;
        }
    }
    KartCtrl::getKartCtrl()->getKartBody(getMyID())->_5c2 = dashType;
}

void RivalSpeedCtrl::doSpeedCtrl()
{
    mOffsetMaxSpeed = 0.0f;
    mBaseMaxSpeed = KartCtrl::getKartCtrl()->GetMaxSpeed(getMyID());
    mNoAccelFlg = 0;

    if (sComStrengthState == 2)
    {
        if (RCMGetManager()->getKartChecker(getMyID())->isFinalLapRenewal())
            resetRaceStrengthening();
    }

    KartCtrl::getKartCtrl()->GetDriveCont(getMyID())->mButtons.mButton |= 0x100;

    if (tstEndRace() || ObjUtility::isPlayerDriver(getMyID()))
        mRank = getMyRank();
    else
        mRank = getBodyCtrl()->findMyRankOnlyCom();

    if (getBodyCtrl()->_84 == 3)
    {
        mMaxSpeed = 0.0f;
    }
    else if (!(getBodyCtrl()->mFlags & 0x80))
    {
        KartCtrl::getKartCtrl()->GetDriveCont(getMyID());
        if (getBodyCtrl()->_84 != 2)
        {
            if (!sConfusedFightModeFlg)
                doRankSpdCtrl();

            switch (getBodyCtrl()->_84)
            {
            case 4:
            case 5:
                mMaxSpeed = sReverseSpd;
                break;
            default:
                doCurveDegSpdCtrl();
                break;
            }

            s32 rankIdx;
            if (isRival())
                rankIdx = mRankParam->mRankId - 1;
            else
            {
                rankIdx = mRank - 1;
                if (rankIdx < 2)
                    rankIdx = 2;
            }

            if (!sConfusedFightModeFlg && !tstEndRace())
                mOffsetMaxSpeed += sCrsLevelParam[0] * sKartRankOffsetSpdRatio[rankIdx];

            mLimitMaxSpeed = mBaseMaxSpeed + mOffsetMaxSpeed;
            mStdMaxSpeed = mLimitMaxSpeed * (sEngineParamPtr->mParam[10] - (f32)(mRank - 1) * 0.1f * 0.125f);
        }
        doAccelCtrl();
        doPlayerDistSpeedCtrl();
        clampMaxSpeed();
    }
    else
    {
        mMaxSpeed = KartCtrl::getKartCtrl()->GetCarSpeed(getMyID());
    }
}

void RivalSpeedCtrl::doPlayerDistSpeedCtrl()
{
    f32 dist = 0.0f;
    if (tstEndRace())
        return;
    s32 group = (s8)getBodyCtrl()->mPointCtrl->getNext()->getPointData()->swerve;
    group = __abs(group);
    if (group >= 2)
        return;
    if (!getBodyCtrl()->getLowestPlayerDist(&dist))
        return;

    f32 ratio = sEngineParamPtr->mParam[12];
    f32 rate = 1.0f - ratio;
    if (dist > 0.0f)
    {
        f32 d = dist / sLowestPlayerDistMaxDist;
        if (d > 1.0f)
            d = 1.0f;
        ratio = rate * d + ratio;
    }
    f32 spd = (1.0f + ratio) * mBaseMaxSpeed;
    if (mLimitMaxSpeed > spd)
        mLimitMaxSpeed = spd;
}

#line 905
void RivalSpeedCtrl::doAccelCtrl()
{
    s32 nowRank = mRank;
    JUT_MINMAX_ASSERT(0, nowRank - 1, 8);
    mAccelMag = mKartKindAccelMag * sKartComAccelStdRatio;
    if (!mNoAccelFlg)
        mMaxSpeed += mAccelMag;
}

u8 RivalSpeedCtrl::doCurveDegSpdCtrl()
{
    int ret = false;
    if (getBodyCtrl()->_bc > sCurveChkAngle && mMaxSpeed > sCurveChkSpd)
    {
        f32 spd = __fabsf(KartCtrl::getKartCtrl()->GetCarSpeed(getMyID())) / sCurveRatioStdSpd;
        f32 lim = spd > 1.0f ? 1.0f : spd;
        f32 down = (getBodyCtrl()->_bc - sCurveChkAngle) / sCurveMaxAngle;
        down = down > 1.0f ? 1.0f : down;
        f32 ratio = 1.0f - sCurveMaxDownRatio * (lim * down);

        s32 group = __abs((s8)getBodyCtrl()->mPointCtrl->getNext()->getPointData()->swerve);
        if (group != 0)
        {
            f32 r = 1.0f - sCurveMaxDownRatio * sCurveSpdRatio[group - 1];
            if (ratio > r)
                ratio = r;
        }

        f32 target = sCurveStdSpd * ratio;
        if (mMaxSpeed > target)
        {
            mMaxSpeed = stConverge(mMaxSpeed, target, 0.0f, sCurveConvergeSub);
            ret = true;
        }
    }
    return ret;
}

u8 RivalSpeedCtrl::doRankSpdCtrl()
{
    int ret = 0;
    if (RCMGetManager()->getRaceMode() == TIME_ATTACK)
        return false;

    RivalCtrlBase::RankParam *pRank = mRankParam;
    s32 myRank = mRank;
#line 986
    JUT_ASSERT(pRank != 0);

    if ((pRank->mRankId > 2 && myRank > 2) || tstEndRace())
        pRank->mKeepRank = myRank;

    if (getBodyCtrl()->getKartTotalDist(getMyID()) >= 0.5f && mRankParam->mRankId == 1 && getMyRank() >= 4)
    {
        for (u8 i = 1; i < 4; i++)
        {
            s32 idx = RivalBodyCtrl::getKartIndexByRank(i);
            if (idx < 0)
                continue;
            RivalKart *kart = KartCtrl::getKartCtrl()->getKartEnemy(idx);
            if (!kart->getBodyCtrl()->isRival())
                continue;
            if (kart->getSpeedCtrl()->mRankParam->mRankId != 2)
                continue;
            mRankParam->setRankId(2);
            mRankParam->setKeepRank(2);
            kart->getSpeedCtrl()->mRankParam->setRankId(1);
            kart->getSpeedCtrl()->mRankParam->setKeepRank(1);
            break;
        }
    }

    s32 keepRank = pRank->mKeepRank;
#line 1027
    JUT_MINMAX_ASSERT(0, keepRank -1, 8);
#line 1028
    JUT_MINMAX_ASSERT(0, myRank -1, 8);

    pRank->mKeepSpd = sCrsLevelParam[1] * sKeepIntervalDistance[(u8)(myRank - 1)];

    if (mMaxSpeed < mStdMaxSpeed)
        return false;

    if (myRank < keepRank)
    {
        mMaxSpeed *= sEngineParamPtr->mParam[1];
        mNoAccelFlg = 1;
        ret = 1;
    }
    else if (myRank == keepRank)
    {
        if (mRank == 1)
        {
            s32 back = getBodyCtrl()->getMyBackwardComKartIndex(sTopWaitBackKartRank - 1);
            if (back >= 0)
            {
                f32 d = getBodyCtrl()->getKartTotalDist(getMyID()) - getBodyCtrl()->getKartTotalDist(back);
                if (d > KartCtrl::getKartCtrl()->getKartEnemy(back)->getBodyCtrl()->mRankParam->mKeepSpd)
                {
                    mMaxSpeed *= sEngineParamPtr->mParam[2];
                    mNoAccelFlg = 1;
                    ret = 1;
                }
            }
        }
        else
        {
            f32 topDist = getBodyCtrl()->getKartTotalDist(getBodyCtrl()->getTopComKartIndex(getBodyCtrl()));
            f32 myDist = getBodyCtrl()->getKartTotalDist(getMyID());
            f32 d = (topDist - myDist) - pRank->mKeepSpd * RivalBodyCtrl::sCheckTopDistRatio;
            if (d > 0.0f)
            {
                f32 add = d * sTopKartAwayStdRatio;
                if (add > sCrsLevelParam[0])
                    add = sEngineParamPtr->mParam[11];
                mOffsetMaxSpeed += add;
            }
            else
            {
                s32 foward_kart = getBodyCtrl()->getMyFowardComKartIndex();
                if (foward_kart >= 0)
                {
#line 1081
                    JUT_MINMAX_ASSERT(0, foward_kart, RCMGetManager()->getKartNumber());
                    RivalSpeedCtrl *fwdSpd = KartCtrl::getKartCtrl()->getKartEnemy(foward_kart)->getSpeedCtrl();
                    if (fwdSpd->mMaxSpeed > sFowartKartMinSpdRatio * fwdSpd->mStdMaxSpeed)
                    {
                        f32 dd = getBodyCtrl()->getKartTotalDist(foward_kart) - getBodyCtrl()->getKartTotalDist(getMyID());
                        if (dd < pRank->mKeepSpd * RivalBodyCtrl::sCheckTopDistRatio)
                        {
                            mMaxSpeed *= sEngineParamPtr->mParam[0];
                            mNoAccelFlg = 1;
                            ret = 1;
                        }
                    }
                }
            }
        }
    }
    return ret;
}

inline f32 RivalBodyCtrl::getKartTotalDist(int idx)
{
    return sKartTotalDist[idx];
}

void RivalSpeedCtrl::resetRaceStrengthening()
{
    mComWeakPointSwapFlg = 0;
    s32 id = getMyID();
    if (!ObjUtility::isPlayerDriver(id))
    {
        RivalCtrlBase::RankParam *pRank = mRankParam;
        s32 rankId = pRank->mRankId;
        if (mIsPlayerKart == 0 && rankId == 1)
        {
            pRank->setKeepRank(2);
            mRankParam->setRankId(2);
        }
    }
}

void RivalSpeedCtrl::clampMaxSpeed()
{
    if (getBodyCtrl()->_84 == 2 || mMaxSpeed >= mLimitMaxSpeed)
        mMaxSpeed = mLimitMaxSpeed;
}

RivalAwardDemoSpeedCtrl::RivalAwardDemoSpeedCtrl(RivalKart *kart) : RivalSpeedCtrl(kart)
{
    mStopFlg = 0;
    mStopImmediatelyFlg = 0;
}

void RivalAwardDemoSpeedCtrl::reset()
{
    mAccelMag = 0.0f;
    mMaxSpeed = 0.0f;
    mStdMaxSpeed = 0.0f;
    mOffsetMaxSpeed = 0.0f;
    mBaseMaxSpeed = KartCtrl::getKartCtrl()->GetMaxSpeed(getMyID());
    mLimitMaxSpeed = mBaseMaxSpeed;
    mRank = -1;
    mNoAccelFlg = 0;
    mStopFlg = 0;
    mStopImmediatelyFlg = 0;
    mAwardStopFrame = 0;
}

void RivalAwardDemoSpeedCtrl::initStartDash()
{
    KartCtrl::getKartCtrl()->getKartBody(getMyID())->_5c2 = 0;
}

void RivalAwardDemoSpeedCtrl::doAccelCtrl()
{
    mMaxSpeed += mAccelMag;
}

void RivalAwardDemoSpeedCtrl::doAccelMaxSpdCtrl()
{
    mLimitMaxSpeed = sAwardMaxSpd;
    mAccelMag = 1.0f;
    doAccelCtrl();
    clampMaxSpeed();
}

void RivalAwardDemoSpeedCtrl::doSpeedCtrl()
{
    if (mStopFlg && !mStopImmediatelyFlg)
    {
        mAwardStopFrame++;
        if (mAwardStopFrame >= sAwardStopFrame)
            mStopImmediatelyFlg = 1;
    }

    if (mStopImmediatelyFlg)
    {
        mMaxSpeed = 0.0f;
    }
    else
    {
        KartCtrl::getKartCtrl()->GetDriveCont(getMyID())->mButtons.mButton |= 0x100;
        doAccelMaxSpdCtrl();
    }
}

RivalAwardDemo2ndSpeedCtrl::RivalAwardDemo2ndSpeedCtrl(RivalKart *kart) : RivalAwardDemoSpeedCtrl(kart)
{
    mSpeedDownFlg = 0;
    mAward2ndLength = sAward2ndLength;
}

void RivalAwardDemo2ndSpeedCtrl::reset()
{
    mAccelMag = 0.0f;
    mMaxSpeed = 0.0f;
    mStdMaxSpeed = 0.0f;
    mOffsetMaxSpeed = 0.0f;
    mBaseMaxSpeed = KartCtrl::getKartCtrl()->GetMaxSpeed(getMyID());
    mLimitMaxSpeed = mBaseMaxSpeed;
    mRank = -1;
    mNoAccelFlg = 0;
    mStopFlg = 0;
    mStopImmediatelyFlg = 0;
    mAwardStopFrame = 0;
    mSpeedDownFlg = 0;
}

void RivalAwardDemo2ndSpeedCtrl::doAccelMaxSpdCtrl()
{
    if (mSpeedDownFlg)
    {
        mAward2ndLength = 2.0f * sAward2ndLength;
        mSpeedDownFlg = 0;
    }

    JGeometry::TVec3f v = getBodyCtrl()->_58;
    JGeometry::TVec3f d;
    JGeometry::TVec3f pos;
    ObjUtility::getKartPos(getMyID(), &pos);
    d.sub(v, pos);
    f32 dist = PSVECMag((Vec *)&d);

    mLimitMaxSpeed = sAwardMaxSpd;
    KartCtrl::getKartCtrl()->GetDriveCont(getMyID());

    f32 ratio = (dist - mAward2ndLength) * sAward2ndLengthRatio;
    ratio = ratio > 1.0f ? 1.0f : ratio;
    mAccelMag = sAward2ndAccelRatio * __fabsf(ratio);
    mMaxSpeed += mAccelMag;

    if (ratio > 0.0f)
        mLimitMaxSpeed += ratio * sAward2ndOffsetSpd;
    else
        mMaxSpeed *= 1.0f - sAward2ndBrake * __fabsf(ratio);

    clampMaxSpeed();
}

void RivalAwardDemo2ndSpeedCtrl::doAccelCtrl() {}

RivalStaffRollDemoSpeedCtrl::RivalStaffRollDemoSpeedCtrl(RivalKart *kart) : RivalAwardDemoSpeedCtrl(kart)
{
    mStopImmediatelyFlg2 = 0;
    mMaxSpeed = sStaffRollDemoStdSpd;
}

void RivalStaffRollDemoSpeedCtrl::reset()
{
    mAccelMag = 0.0f;
    mMaxSpeed = 0.0f;
    mStdMaxSpeed = 0.0f;
    mOffsetMaxSpeed = 0.0f;
    mBaseMaxSpeed = KartCtrl::getKartCtrl()->GetMaxSpeed(getMyID());
    mLimitMaxSpeed = mBaseMaxSpeed;
    mRank = -1;
    mNoAccelFlg = 0;
    mStopFlg = 0;
    mStopImmediatelyFlg = 0;
    mAwardStopFrame = 0;
    mMaxSpeed = sStaffRollDemoStdSpd;
    mStopImmediatelyFlg2 = 0;
}

void RivalStaffRollDemoSpeedCtrl::doSpeedCtrl()
{
    if (mStopImmediatelyFlg2)
    {
        mMaxSpeed = 0.0f;
    }
    else
    {
        KartCtrl::getKartCtrl()->GetDriveCont(getMyID())->mButtons.mButton |= 0x100;
    }
}
#include "JSystem/JAudio/JASFakeMatch2.h"
