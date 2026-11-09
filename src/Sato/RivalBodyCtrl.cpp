#include <JSystem/JGeometry.h>
#include <JSystem/JMath/JMath.h>
#include <JSystem/JUtility/JUTAssert.h>

#include "Kaneshige/Course/Course.h"
#include "Kaneshige/RaceDirector.h"
#include "Kaneshige/RaceMgr.h"

#include "Sato/ObjUtility.h"
#include "Sato/RivalBodyCtrl.h"
#include "Sato/RivalKart.h"
#include "Sato/RivalSpeedCtrl.h"
#include "Sato/stMath.h"

#include "Yamamoto/kartBody.h"

namespace {

const f32 sKartDriftSuccessRateEasy[8] = { 0.4f, 0.3f, 0.2f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f };
const f32 sKartDriftSuccesRateNormal[8] = { 0.7f, 0.6f, 0.5f, 0.3f, 0.1f, 0.0f, 0.0f, 0.0f };
const f32 sKartDriftSuccesRateHard[8] = { 1.0f, 0.9f, 0.8f, 0.6f, 0.4f, 0.0f, 0.0f, 0.0f };
const f32 sEngineSmallBParam[5] = { 0.3f, 0.96f, 0.0f, 0.6f, 0.0f };
const f32 sEngineNormalBParam[5] = { 0.7f, 0.93f, 0.0f, 0.2f, 0.0f };
const f32 sEngineBigBParam[5] = { 1.0f, 0.9f, 1.0f, 0.0f, 0.0f };

f32 sMaxCurveAngle = 15.0f;
f32 sMinCurveAngle = 7.0f;
f32 sSterrRatio = 0.7f;
f32 sEndDriftAngle = 3.0f;
f32 sBodyRotRatio = 0.15f;
f32 sReverseBodyRotRatio = 0.3f;
f32 sDriftEndSpd = 80.0f;
f32 sReverseChkAngle = 0.2f;
u8 sColWallJudgeFrame = 120;
f32 sColWallMinSpd = 10.0f;
f32 sAward2ndSideLength = 300.0f;
f32 sRndNextPointRatio = 0.15f;
u8 sRndPointMaxNo = 1;
f32 sRndMaxRoadWidthRatio = 0.7f;
u64 sAccidentStateFlg = 0x0000000001580800;
u64 sDriftInterruptionFlg = 0x0000000004000410;
u8 sMaxColWaitFrame = 5;
f32 sEndDriftSideLength = 30.0f;
f32 sDriftSideLengthRatio = 40.0f;
f32 sDriftRadiusRatio = 3.0f;
u8 sDriftMaxWaitLengthCnt = 60;
u8 sDriftWaitStdAngleMag = 140;
u8 sDriftRandomMaxStartWaitCnt = 5;
u8 sDriftStageStdWaitCnt = 15;
f32 sAwardBodyRotRatio = 1.0f;

} // namespace

s32 RivalBodyCtrl::sKartRankArray[24];
s32 RivalBodyCtrl::sComKartRankArray[8];
f32 RivalBodyCtrl::sKartTotalDist[8];
s32 RivalBodyCtrl::sPlayerRank[8];
s32 RivalBodyCtrl::sTopPlayerIndex;
s32 RivalBodyCtrl::sLowestPlayerKartIndex;
s32 RivalBodyCtrl::sLowestComKartIndex;
f32 RivalBodyCtrl::sCrsStdDistRatio;
void *RivalBodyCtrl::sEngineBParamPtr;
u32 RivalBodyCtrl::sCommonFlgs;
f32 RivalBodyCtrl::sCheckTopDistRatio;


void RivalBodyCtrl::init()
{
    Course *course = RCMGetCourse();
    resetBodyData();
    mPointCtrl->reset();
    mPointCtrl->init_ByCLPointPtr(course->getFirstCLPoint(mRival->mNo));
    resetPointData();

    switch (RCMGetManager()->getKartInfo(mRival->mNo)->getKartID())
    {
    case RED_FIRE:
    case HEART_COACH:
    case WARIO_CAR:
        _8c = 2.0f;
        break;
    case KOOPA_DASHER:
    case GOO_GOO_BUGGY:
        _8c = 1.3f;
        break;
    case DK_JUMBO:
    case TURBO_YOSHI:
    case KOOPA_KING:
        _8c = 2.2f;
        break;
    }
}

void RivalBodyCtrl::chkPlayerKartCondition()
{
    s32 lowest = 1;
    sCommonFlgs &= ~2;
    sTopPlayerIndex = -1;
    sLowestPlayerKartIndex = -1;

    for (u8 i = 0; i < RCMGetManager()->mRaceInfo->getPlayerKartNumber(); i++)
    {
        s32 rank = RCMGetManager()->getKartChecker(i)->getRank();
        if (rank == 1)
        {
            sTopPlayerIndex = i;
            sCommonFlgs |= 2;
        }
        if (rank >= lowest)
        {
            lowest = rank;
            sLowestPlayerKartIndex = i;
        }
    }
}

void RivalBodyCtrl::chkBodyCondition()
{
    if (sAccidentStateFlg & mRival->mBody->mCarStatus)
        mFlags |= 0x80;
    else
        mFlags &= ~0x80;

    u64 flgs = sDriftInterruptionFlg | sAccidentStateFlg;
    if (flgs & mRival->mBody->mCarStatus)
        mFlags |= 0x100;
    else
        mFlags &= ~0x100;
}

#line 539
int RivalBodyCtrl::getKartIndexByRank(int rank)
{
    JUT_MINMAX_ASSERT(1, rank, RCMGetManager()->mRaceInfo->getKartNumber() + 1);
    return sKartRankArray[rank - 1];
}

void RivalBodyCtrl::calcTargetPosDrift() {}

void RivalBodyCtrl::getFwPointPos(JGeometry::TVec3f *pos)
{
    mPointCtrl->getNext()->getPosition(pos);
}

void RivalBodyCtrl::getPointRSideDir(JGeometry::TVec3f *out)
{
    JGeometry::TVec3f right;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f nextPos, prevPos;
    mPointCtrl->getPrev()->getPosition(&prevPos);
    mPointCtrl->getNext()->getPosition(&nextPos);

    dir.sub(prevPos, nextPos);
    stVecNormalize(&dir);

    PSVECCrossProduct(&dir, &up, &right);
    out->set(right);
}









bool RivalBodyCtrl::isNeighborhoodCenter(f32 threshold)
{
    f32 ratio = 1.0f - JMAAbs(getLengthFCenter()) / (0.01f + mWidth);
    bool ret;
    if (ratio >= threshold)
        ret = true;
    else
        ret = false;
    return ret;
}

f32 RivalBodyCtrl::getLengthFCenter()
{
    JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f prevPos, nextPos;
    mPointCtrl->getPrev()->getPosition(&prevPos);
    mPointCtrl->getNext()->getPosition(&nextPos);

    JGeometry::TVec3f dir;
    dir.sub(nextPos, prevPos);
    stVecNormalize(&dir);

    JGeometry::TVec3f right;
    PSVECCrossProduct(&dir, &up, &right);
    right.y = 0.0f;
    stVecNormalize(&right);

    JGeometry::TVec3f nextPos2;
    mPointCtrl->getNext()->getPosition(&nextPos2);

    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mRival->mNo, &kartPos);
    kartPos.y = 0.0f;

    return kartPos.dot(right) - nextPos2.dot(right);
}





bool RivalBodyCtrl::chkArrivePoint(CLPoint *point)
{
    bool ret = false;
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mRival->mNo, &kartPos);

    JGeometry::TVec3f pointPos;
    point->getPosition(&pointPos);

    JGeometry::TVec3f diff;
    diff.sub(pointPos, kartPos);

    if (diff.length() < 0.5f * point->getPointData()->scale)
        ret = true;
    return ret;
}

int RivalBodyCtrl::calcMaxChangeWaitCnt()
{
    s32 wait = stGetRnd(2)->getRandomMax(1000) + 300;
    _b4 = wait;
    return wait;
}

void RivalBodyCtrl::doStateFlgCtrl() {}

void RivalBodyCtrl::resetBodyData()
{
    _9d = 0;
    _9c = 0;
    _a4 = 0;
    _a5 = 0;
    _a6 = 0;
    _a7 = 0;
    mFlags = 0;
    _b8 = 0;
    _c7 = 0;
    _b0 = 0;

    s32 rnd = stGetRnd(2)->getRandomMax(1000) + 300;
    _b4 = rnd;
    _b4 = rnd;

    _bc = 0.0f;
    _c0 = 0.0f;
    _84 = 0;
    _c4 = 0;
    _c5 = 0;
    _c6 = 0;
}

bool RivalBodyCtrl::isOneUpperLowerPlayer(bool upper)
{
    bool ret = false;
    s32 dir = upper ? -1 : 1;
    for (u8 i = 0; i < RCMGetManager()->mRaceInfo->getPlayerKartNumber(); i++)
    {
        if (sPlayerRank[i] == -1)
            continue;
        s32 rank = RCMGetManager()->getKartChecker(mRival->mNo)->getRank();
        if (rank == sPlayerRank[i] + dir)
        {
            ret = true;
            break;
        }
    }
    return ret;
}

#line 1746
bool RivalBodyCtrl::getLowestPlayerDist(f32 *out)
{
    *out = -1.0f;
    if (sLowestPlayerKartIndex < 0)
        return false;
    JUT_MINMAX_ASSERT(0, sLowestPlayerKartIndex, 8);
    *out = sKartTotalDist[sLowestPlayerKartIndex] - sKartTotalDist[mRival->mNo];
    return true;
}

int RivalBodyCtrl::findMyRankOnlyCom()
{
    int rank = -1;
    const s32 *p = sComKartRankArray;
    s16 kartNum = RCMGetManager()->mRaceInfo->getKartNumber();
    for (u8 i = 0; i < kartNum; i++, p++)
    {
        if (*p == mRival->mNo) { rank = i + 1; break; }
    }
    return rank;
}

#line 1773
int RivalBodyCtrl::getTopComKartIndex(RivalBodyCtrl *)
{
    u8 useKartRank = 1;
    s32 rank = sComKartRankArray[0];
    if (!RCMGetManager()->mRaceDirector->isRaceEnd())
    {
        if (!RCMGetManager()->isKartGoal(mRival->mNo))
            useKartRank = 0;
    }
    if (useKartRank)
        rank = sKartRankArray[0];
    JUT_ASSERT(rank >= 0);
    return rank;
}

#line 1799
int RivalBodyCtrl::getMyFowardComKartIndex()
{
    s32 idx = mRival->getSpeedCtrl()->mRank - 2;
    s32 *rankArray = sComKartRankArray;
    s32 ret = -1;
    u8 useKartRank = 1;
    if (!RCMGetManager()->mRaceDirector->isRaceEnd() && !RCMGetManager()->isKartGoal(mRival->mNo))
        useKartRank = 0;
    if (useKartRank)
        rankArray = sKartRankArray;
    if (idx < 0)
        return ret;
    JUT_MINMAX_ASSERT(1, idx, 9);
    return rankArray[idx];
}

#line 1816
int RivalBodyCtrl::getMyBackwardComKartIndex(u8 offset)
{
    s32 *rankArray = sComKartRankArray;
    s32 ret = -1;
    u8 useKartRank = 1;
    s32 idx = offset - 1 + mRival->getSpeedCtrl()->mRank;
    if (!RCMGetManager()->mRaceDirector->isRaceEnd() && !RCMGetManager()->isKartGoal(mRival->mNo))
        useKartRank = 0;
    if (useKartRank)
        rankArray = sKartRankArray;
    if (idx >= RCMGetManager()->mRaceInfo->getKartNumber())
        return ret;
    JUT_MINMAX_ASSERT(1, idx, 9);
    ret = rankArray[idx];
    return ret;
}

bool RivalAward2ndBodyCtrl::doNextPoint(bool) { return false; }
