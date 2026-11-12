#include <JSystem/JGeometry.h>
#include <JSystem/JMath/JMath.h>
#include <JSystem/JUtility/JUTAssert.h>

#include "Kaneshige/Course/Course.h"
#include "Kaneshige/RaceDirector.h"
#include "Kaneshige/RaceMgr.h"

#include "Sato/ItemObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/RivalItemCtrl.h"
#include "Sato/RivalBodyCtrl.h"
#include "Sato/RivalKart.h"
#include "Sato/RivalSpeedCtrl.h"
#include "Sato/stMath.h"

#include "Osako/kartPad.h"

#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCtrl.h"

static const f32 sDummy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

namespace {

f32 sKartDriftSuccessRateEasy[8] = { 0.4f, 0.3f, 0.2f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f };
f32 sKartDriftSuccesRateNormal[8] = { 0.7f, 0.6f, 0.5f, 0.3f, 0.1f, 0.0f, 0.0f, 0.0f };
f32 sKartDriftSuccesRateHard[8] = { 1.0f, 0.9f, 0.8f, 0.6f, 0.4f, 0.3f, 0.2f, 0.1f };
RivalBodyCtrl::EngineBParam sEngineSmallBParam = { 0.3f, 0.96f, 0.0f, 0.6f, sKartDriftSuccessRateEasy };
RivalBodyCtrl::EngineBParam sEngineNormalBParam = { 0.7f, 0.93f, 0.5f, 0.2f, sKartDriftSuccesRateNormal };
RivalBodyCtrl::EngineBParam sEngineBigBParam = { 1.0f, 0.9f, 1.0f, 0.0f, sKartDriftSuccesRateHard };

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

} // namespace

int RivalBodyCtrl::sTopPlayerIndex = -1;
int RivalBodyCtrl::sLowestPlayerKartIndex = -1;
int RivalBodyCtrl::sLowestComKartIndex = -1;
f32 RivalBodyCtrl::sCrsStdDistRatio = 1.0f;

namespace {

f32 sAwardBodyRotRatio = 0.12f;

} // namespace

int RivalBodyCtrl::sKartRankArray[24];
int RivalBodyCtrl::sComKartRankArray[8];
f32 RivalBodyCtrl::sKartTotalDist[8];
int RivalBodyCtrl::sPlayerRank[8];
RivalBodyCtrl::EngineBParam *RivalBodyCtrl::sEngineBParamPtr;
u32 RivalBodyCtrl::sCommonFlgs;
f32 RivalBodyCtrl::sCheckTopDistRatio;


RivalCtrlBase::RankParam::RankParam(int firstRank)
{
    mFirstRank = firstRank;
    reset();
}

void RivalCtrlBase::RankParam::reset()
{
    mRankId = mFirstRank;
    mKeepRank = mRankId;
    mKeepSpd = 0.0f;
}

RivalCtrlBase::RivalCtrlBase(RivalKart *rival)
{
    mRival = rival;
}

int RivalCtrlBase::getMyID() { return mRival->mNo; }

RivalBodyCtrl *RivalCtrlBase::getBodyCtrl() { return mRival->mBodyCtrl; }

KartBody *RivalCtrlBase::getBody() { return mRival->mBody; }

stRandom *RivalCtrlBase::getRandom() { return stGetRnd(2); }

u8 RivalCtrlBase::getRankParamID() { return mRankParam->mRankId - 1; }

bool RivalCtrlBase::tstEndRace()
{
    bool ret = true;
    if (!RCMGetManager()->mRaceDirector->isRaceEnd())
    {
        if (!RCMGetManager()->isKartGoal(mRival->mNo))
            ret = false;
    }
    return ret;
}

bool RivalCtrlBase::isRival() const { return mRankParam->mRankId <= 2; }

int RivalCtrlBase::getMyRank() { return RCMGetManager()->getKartChecker(mRival->mNo)->getRank(); }

RivalBodyCtrl::RivalBodyCtrl(RivalKart *rival)
    : RivalCtrlBase(rival), mPointCtrl(new CLPointControllerRivalKart(this))
{
}

RivalBodyCtrl::~RivalBodyCtrl() {}
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

void RivalBodyCtrl::reset()
{
    switch (RCMGetManager()->mRaceInfo->getRaceLevel())
    {
    case LVL_50CC:
        sEngineBParamPtr = &sEngineSmallBParam;
        break;
    case LVL_100CC:
        sEngineBParamPtr = &sEngineNormalBParam;
        break;
    case LVL_150CC:
    case LVL_MIRROR:
        sEngineBParamPtr = &sEngineBigBParam;
        break;
    }

#line 407
    JUT_ASSERT(sEngineBParamPtr != 0);

    for (int i = 0; i < 8; i++)
    {
        sKartTotalDist[i] = 0.0f;
        sKartRankArray[i] = -1;
        sComKartRankArray[i] = -1;
    }

    sCommonFlgs = 0;
    sTopPlayerIndex = -1;
    sLowestComKartIndex = -1;
    sCheckTopDistRatio = 0.0f;
    sCrsStdDistRatio = RivalSpeedCtrl::sCrsLevelParam[2] * RCMGetCourse()->getTrackSectorDist() / 270000.0f;
}

void RivalBodyCtrl::setRankArray()
{
    s32 lowest = 1;
    for (int index = 0; index < RCMGetManager()->getKartNumber(); index++)
    {
        KartChecker *checker = RCMGetKartChecker(index);
        f32 dist = checker->getTotalUnitDist();
        sKartTotalDist[index] = dist * sCrsStdDistRatio;
        s32 kartRank = checker->getRank();
        if (kartRank > 0)
        {
            u8 id = kartRank - 1;
            if (!ObjUtility::isPlayerDriver(index) && kartRank > lowest)
            {
                lowest = kartRank;
                sLowestComKartIndex = index;
            }
#line 448
            JUT_MINMAX_ASSERT(0, id, 8);
            sKartRankArray[id] = index;
            if (index < RCMGetManager()->getPlayerNumber())
            {
                kartRank = -1;
                if (ObjUtility::isPlayerDriver(index))
                    kartRank = id + 1;
                sPlayerRank[index] = kartRank;
            }
        }
        if (checker->getRank() == 1)
        {
            if (dist >= 0.0f)
            {
                sCheckTopDistRatio = sKartTotalDist[index];
                if (sCheckTopDistRatio < 0.0f)
                    sCheckTopDistRatio = 0.0f;
                else if (sCheckTopDistRatio > 1.0f)
                    sCheckTopDistRatio = 1.0f;
            }
            else
            {
                sCheckTopDistRatio = 0.0f;
            }
        }
    }

    int j = 0;
    for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
    {
        if (sKartRankArray[i] < 0)
            continue;
        if (ObjUtility::isPlayerDriver(sKartRankArray[i]))
            continue;
        sComKartRankArray[j] = sKartRankArray[i];
        j++;
    }
}

void RivalBodyCtrl::chkPlayerKartCondition()
{
    s32 lowest = 1;
    u8 i;
    sCommonFlgs &= ~2;
    sTopPlayerIndex = -1;
    sLowestPlayerKartIndex = -1;
    for (i = 0; i < RCMGetManager()->getPlayerNumber(); i++)
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

bool RivalBodyCtrl::doNextPoint(bool p1)
{
    if (_84 == 4)
    {
        mPointCtrl->mIsReverse = false;
        _84 = 5;
    }

    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mRival->mNo, &kartPos);
    f32 len = mPointCtrl->getLengthFromCLDir(kartPos);
    mPointCtrl->changeNextPoint();
    mWidth = 0.5f * mPointCtrl->getNext()->getPointData()->scale;
    getFwPointPos(&_28);
    getTargetCurveLR(&_90, 0);
    setDriftInfo();
    if (_84 == 2)
    {
        calcTargetPosDrift();
    }
    else
    {
        calcTargetPosNormal(len);
        if (p1)
            _58 = _28;
        else
            _58.add(_28, _40);
        _34 = mPointCtrl->_c;
    }
    getDriftTargetDir(&_4c);
    return true;
}

void RivalBodyCtrl::calcTargetPosDrift() {}

void RivalBodyCtrl::calcTargetPosNormal(f32)
{
    int swerve = mPointCtrl->getNext()->getPointData()->swerve;
    if (swerve != 0)
    {
        JGeometry::TVec3f dir(mPointCtrl->_18);
        if (swerve > 0)
            dir.negate();
        _a0 = swerve / 3.0f;
        dir.scale(mWidth * JMAAbs(_a0));
        _40 = dir;
    }
    else
    {
        getPointRSideDir(&_40);
        _9d++;
        if (_9d > _9c)
        {
            _9d = 0;
            _9c = stGetRnd(2)->getRandomMax(sRndPointMaxNo);
            f32 rnd = sRndNextPointRatio * (2.0f * stGetRnd(2)->get_ufloat_1() - 1.0f);
            f32 ratio = rnd + _a0;
            if (JMAAbs(ratio) > sRndMaxRoadWidthRatio)
                ratio = ratio > 0.0f ? sRndMaxRoadWidthRatio : -sRndMaxRoadWidthRatio;
            _a0 = ratio;
        }
        _40.scale(mWidth * _a0);
    }
}


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

    dir.sub(nextPos, prevPos);
    dir.y = 0.0f;
    stVecNormalize(dir);

    PSVECCrossProduct(&dir, &up, &right);
    *out = right;
}










inline f32 getVecRadAngle(const JGeometry::TVec3f &dir, const JGeometry::TVec3f &vec)
{
    JGeometry::TVec3f cross;
    PSVECCrossProduct(&dir, &vec, &cross);
    f32 mag = PSVECMag(&cross);
    f32 at = JMAAtan2Radian(mag, dir.dot(vec));
    return JGeometry::TUtilf::abs(at);
}

inline f32 getVecDegAngle(const JGeometry::TVec3f &dir, const JGeometry::TVec3f &vec)
{
    JGeometry::TVec3f cross;
    PSVECCrossProduct(&dir, &vec, &cross);
    f32 mag = PSVECMag(&cross);
    f32 at = JMAAtan2Radian(mag, dir.dot(vec));
    return 57.29578f * JGeometry::TUtilf::abs(at);
}

inline void setRotYMtx33(Mtx m, s16 angle)
{
    f32 c = JMASCos(angle);
    f32 s = JMASSin(angle);
    m[0][2] = s;
    m[0][0] = c;
    m[2][0] = -s;
    m[2][2] = c;
    m[1][1] = 1.0f;
    m[2][1] = 0.0f;
    m[1][2] = 0.0f;
    m[1][0] = 0.0f;
    m[0][1] = 0.0f;
}

inline void setRotYMtx(Mtx m, s16 angle)
{
    setRotYMtx33(m, angle);
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
}

#pragma auto_inline off
f32 RivalBodyCtrl::getLengthFCenter()
{
    JGeometry::TVec3f right, nextPos, kartPos, diff;
    getPointRSideDir(&right);
    right.y = 0.0f;
    stVecNormalize(&right);

    mPointCtrl->getNext()->getPosition(&nextPos);
    ObjUtility::getKartPos(mRival->mNo, &kartPos);
    kartPos.y = 0.0f;
    diff.sub(kartPos, nextPos);
    return right.dot(diff);
}
#pragma auto_inline reset

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

void RivalBodyCtrl::getDriftTargetDir(JGeometry::TVec3f *out)
{
    CLPoint *point = mPointCtrl->getNext()->getNextPoint(0);
    CLPoint *next = point->getNextPoint(0);
    JGeometry::TVec3f a, b;
    point->getPosition(&a);
    next->getPosition(&b);
    out->sub(b, a);
    out->y = 0.0f;
    stVecNormalize(out);
}

bool RivalBodyCtrl::getTargetCurveLR(f32 *angle, s16 n)
{
    CLPoint *point = getNthCLPoint(mPointCtrl->getNext(), n);
    CLPoint *next = point->getNextPoint(0);
    CLPoint *prev = point->getPrevPoint(0);
    JGeometry::TVec3f prevPos, pos, nextPos;
    point->getPosition(&pos);
    prev->getPosition(&prevPos);
    next->getPosition(&nextPos);

    JGeometry::TVec3f toNext, toPrev;
    JGeometry::TVec3f cross(0.0f, 1.0f, 0.0f);
    toNext.sub(nextPos, pos);
    toPrev.sub(prevPos, pos);
    toPrev.y = 0.0f;
    toNext.y = 0.0f;
    stVecNormalize(toNext);
    stVecNormalize(toPrev);
    PSVECCrossProduct(&toNext, &toPrev, &cross);

    if (angle)
    {
        JGeometry::TVec3f cross2;
        PSVECCrossProduct(&toNext, &toPrev, &cross2);
        f32 mag = PSVECMag(&cross2);
        f32 at = JMAAtan2Radian(mag, toNext.dot(toPrev));
        *angle = 57.29578f * JGeometry::TUtilf::abs(at);
    }

    bool ret;
    if (cross.y > 0.0f)
        ret = false;
    else
        ret = true;
    return ret;
}

int RivalBodyCtrl::getLRKartTargetDir()
{
    JGeometry::TVec3f kartPos, dir, cross, nextPos;
    int ret = 1;
    ObjUtility::getKartPos(mRival->mNo, &kartPos);
    mPointCtrl->getNext()->getPosition(&nextPos);
    dir.sub(kartPos, nextPos);
    dir.y = 0.0f;
    stVecNormalize(dir);
    JGeometry::TVec3f clDir(mPointCtrl->_c);
    PSVECCrossProduct(&clDir, &dir, &cross);
    if (cross.y > 0.0f)
        ret = 0;
    return ret;
}

void RivalBodyCtrl::chkDriftPoint()
{
    if (_80 == 0)
        return;

    if (mFlags & 0x100)
    {
        if (_84 == 2)
            reviseAfterDriftVtTargetPos();
        _84 = 0;
        _80 = 0;
        return;
    }

    bool isState1 = (_84 == 1);
    if (isState1 || _84 == 2)
    {
        if (isState1)
            _a4++;
        if (_a4 <= _a5 && _84 != 2)
            return;

        switch (_80->getPointData()->driftDir)
        {
        case 1:
            _98 = 0;
            initDrift(true, _80);
            break;
        case 2:
            _98 = 1;
            initDrift(true, _80);
            break;
        default:
            if (_84 == 2)
                reviseAfterDriftVtTargetPos();
            _84 = 0;
            break;
        }
        _80 = 0;
        return;
    }

    if (!chkArrivePoint(_80))
        return;

    switch (_80->getPointData()->driftDir)
    {
    case 1:
    case 2:
        _84 = 1;
        _a4 = 0;
        _a5 = stGetRnd(2)->getRandomMax(sDriftRandomMaxStartWaitCnt);
        break;
    case 3:
        if (_84 == 2)
            reviseAfterDriftVtTargetPos();
        _84 = 0;
        _80 = 0;
        break;
    }
}

bool RivalBodyCtrl::chkArrivePoint(CLPoint *point)
{
    bool ret = false;
    JGeometry::TVec3f kartPos, tmp, pointPos, diff;
    ObjUtility::getKartPos(mRival->mNo, &kartPos);
    point->getPosition(&pointPos);
    tmp.sub(pointPos, kartPos);
    diff = tmp;
    diff.y = 0.0f;
    f32 len = diff.length();
    if (len < 0.5f * point->getPointData()->scale)
        ret = true;
    return ret;
}

void RivalBodyCtrl::initDrift(bool, CLPoint *point)
{
    _84 = 2;
    _c4 = 0;
    _c5 = 0;
    mFlags &= ~0x200;

    f32 rate = sEngineBParamPtr->mMiniTurboRate;
    if (rate > 0.0f && stGetRnd(2)->get_ufloat_1() <= rate)
    {
        mFlags |= 0x200;
        _c6 = sDriftStageStdWaitCnt + stGetRnd(2)->getRandomMax(sDriftRandomMaxStartWaitCnt);
    }

    s32 driftDir = point->getPointData()->driftDir;
    int sign = -1;
    _a8 = driftDir;

    JGeometry::TVec3f side;
    getPointRSideDir(&side);
    if (driftDir == 1)
    {
        side.negate();
        sign = 1;
    }

    _7c = sDriftSideLengthRatio * point->getPointData()->driftAcuteness;
    _40 = side;
    _40.scale(_7c);
    JGeometry::TVec3f center;
    center.add(_28, _40);

    JGeometry::TVec3f toKart(side);
    toKart.negate();
    JGeometry::TVec3f toKart2(toKart);

    Mtx mtx;
    s16 angle = (s16)sign * 182 * point->getPointData()->driftDuration;
    setRotYMtx(mtx, angle);
    PSMTXMultVec(mtx, &toKart, &toKart);
    toKart.scale(_7c);
    _70.add(center, toKart);

    f32 ratio = sEngineBParamPtr->mInnerRatio;
    if (ratio > 0.0f)
    {
        angle *= 0.5f;
        toKart2.scale(_7c);
        setRotYMtx33(mtx, angle);
        PSMTXMultVec(mtx, &toKart2, &toKart2);
        toKart2.scale(ratio);
        center.add(toKart2);
    }

    _64.sub(center, _70);
    _7c = PSVECMag(&_64);
    stVecNormalize(_64);
    _a6 = 0;

    s32 wait = (s32)(sDriftMaxWaitLengthCnt * ((f32)point->getPointData()->driftDuration / sDriftWaitStdAngleMag)) - _a5 + (s8)point->getPointData()->driftSupplement;
    if (wait < 0)
        wait = 0;
    _a7 = wait * sEngineBParamPtr->mWaitRatio;
}

void RivalBodyCtrl::doDriftCtrl()
{
    if (tstEndRace())
    {
        if (_84 == 2)
        {
            _84 = 0;
            reviseAfterDriftVtTargetPos();
        }
        return;
    }

    if (_84 == 0 || _84 == 1 || _84 == 2)
    {
        chkDriftPoint();
        if (_84 != 2)
            return;

        if (mRival->getSpeedCtrl()->getMaxSpeed() < sDriftEndSpd)
        {
            _84 = 0;
            reviseAfterDriftVtTargetPos();
            return;
        }

        if ((mFlags & 0x200) && _c5 < 2)
        {
            u8 prevStage = _c5;
            _c4++;
            if (_c4 > _c6)
                _c5++;
            if (_c5 != 0 && prevStage != _c5)
            {
                KartCtrl::getKartCtrl()->SetUpMiniTurbo(mRival->mNo, _c5);
                _c4 = 0;
                if (_c5 == 1)
                    _c6 = sDriftStageStdWaitCnt + stGetRnd(2)->getRandomMax(sDriftRandomMaxStartWaitCnt);
            }
        }

        if (_a6 > _a7)
            _7c *= sEngineBParamPtr->mDecayRatio;
        else
            _a6++;

        _40 = _64;
        _40.scale(_7c);
        _58.add(_70, _40);

        KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(mRival->mNo);
        JGeometry::TVec3f vel;
        ObjUtility::getKartVel(mRival->mNo, &vel);
        vel.y = 0.0f;
        stVecNormalize(vel);

        JGeometry::TVec3f cross;
        PSVECCrossProduct(&_4c, &vel, &cross);
        f32 mag = PSVECMag(&cross);
        f32 at = JMAAtan2Radian(mag, _4c.dot(vel));
        f32 angle = 57.29578f * JGeometry::TUtilf::abs(at);

        if ((mFlags & 0x100) || angle < sEndDriftAngle || _7c < sEndDriftSideLength)
        {
            _84 = 0;
            reviseAfterDriftVtTargetPos();
            return;
        }

        if (_98 == 1)
        {
            pad->mButtons.mAnalogR = 150;
            pad->mButtons.mButton |= 0x20;
        }
        else
        {
            pad->mButtons.mAnalogL = 150;
            pad->mButtons.mButton |= 0x40;
        }
    }
}

void RivalBodyCtrl::reviseAfterDriftVtTargetPos()
{
    f32 len = getLengthFCenter();
    JGeometry::TVec3f side;
    getPointRSideDir(&side);
    if (len < 0.0f)
        side.negate();

    f32 dist = JMAAbs(len);
    if (dist > 0.7f * mWidth)
        dist = 0.7f * mWidth;

    side.scale(dist);
    _58 = _28;
    _40 = side;
    _58.add(side);
}

void RivalBodyCtrl::doSterrCtrl()
{
    if (_84 == 3)
        return;
    if (mFlags & 0x80)
        return;

    JGeometry::TVec3f dir = mRival->mBody->mVel;
    dir.y = 0.0f;
    stVecNormalize(dir);

    JGeometry::TVec3f vec;
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mRival->mNo, &kartPos);
    vec.sub(_58, kartPos);
    _c0 = vec.length();
    vec.y = 0.0f;

    if (mPointCtrl->chkArriveAtFwPoint(kartPos, _84 == 2 ? mWidth * sDriftRadiusRatio : mWidth) || mPointCtrl->chkIsOverFwPoint(kartPos))
    {
        if (doNextPoint(false))
            return;
    }

    stVecNormalize(vec);
    doRotPosSterr(vec);
}

void RivalBodyCtrl::doRotPosSterr(const JGeometry::TVec3f &dir)
{
    JGeometry::TVec3f zdir;
    ObjUtility::getKartZdir(mRival->mNo, &zdir);
    zdir.y = 0.0f;
    stVecNormalize(zdir);

    JGeometry::TVec3f cross;
    PSVECCrossProduct(&dir, &zdir, &cross);
    f32 lr = cross.y > 0.0f ? 1.0f : -1.0f;
    _94 = (lr == 1.0f) ? 1 : 0;

    _bc = getVecDegAngle(dir, zdir);

    f32 sterr = 0.0f;
    if (_84 == 2 || _bc >= sMaxCurveAngle)
        sterr = lr;
    if (_bc <= sMinCurveAngle)
        _bc = 0.0f;

    if ((_bc >= sMaxCurveAngle && mRival->getSpeedCtrl()->getMaxSpeed() > 60.0f) || _bc < sMinCurveAngle)
    {
        f32 ratio = sBodyRotRatio;
        if (mRival->mBody->mCarStatus & 0x40)
            ratio *= 0.5f;
        doRotPose(dir, ratio);
    }

    if (_84 != 2)
        sterr = sSterrRatio * (lr * (_bc / sMaxCurveAngle));

    if (sterr > 1.0f)
        sterr = 1.0f;
    else if (sterr < -1.0f)
        sterr = -1.0f;

    KartCtrl::getKartCtrl()->GetDriveCont(mRival->mBody->mMynum)->mMainStick.mX = sterr;
}

void RivalBodyCtrl::doRotTurnAndChangeState(RivalCtrlBase::RivalMoveState nextState)
{
    JGeometry::TVec3f dir;
    JGeometry::TVec3f kartPos;
    JGeometry::TVec3f zdir;
    ObjUtility::getKartPos(mRival->mNo, &kartPos);
    dir.sub(_28, kartPos);
    dir.y = 0.0f;
    stVecNormalize(dir);
    doRotPose(dir, sReverseBodyRotRatio);

    ObjUtility::getKartZdir(mRival->mNo, &zdir);
    zdir.y = 0.0f;
    stVecNormalize(zdir);

    if (getVecRadAngle(zdir, dir) < sReverseChkAngle)
        _84 = nextState;
}

void RivalBodyCtrl::doColWallCtrl()
{
    if (_84 == 3 || _84 == 5)
    {
        doRotTurnAndChangeState(_84 == 3 ? MOVE_REVERSE_END : MOVE_NORMAL);
        return;
    }

    u64 status = KartCtrl::getKartCtrl()->GetKartStatus(mRival->mNo);
    if (!(sAccidentStateFlg & status) && (status & 0x400) && KartCtrl::getKartCtrl()->GetCarSpeed(mRival->mNo) < sColWallMinSpd)
    {
        _b8++;
        _c7 = sMaxColWaitFrame;
    }
    else if (_c7 != 0)
    {
        _c7--;
        if (_c7 == 0)
            _b8 = 0;
    }

    if (_b8 > sColWallJudgeFrame)
    {
        _84 = 3;
        mPointCtrl->mIsReverse = true;
        mPointCtrl->changeNextPoint();
        doNextPoint(true);
        _b8 = 0;
    }
}

void RivalBodyCtrl::doChangeDriverCtrl()
{
    int no;
    bool isGoal = false;
    for (u8 i = 0; i < 2; i++)
    {
        if (RCMGetKartLoader(mRival->mNo)->getExModelDriver(i)->IsGoal())
        {
            isGoal = true;
            break;
        }
    }

    if (mFlags & 0x80 || _84 == 2 || mRival->mItemCtrl->mUseCnt > mRival->mItemCtrl->mUseWait || isGoal)
        return;

    no = mRival->mNo;
    ItemObj *item = ItemObjMgr::getItemObjMgr()->getKartEquipItem(no, ItemObjMgr::getNowTandemDriverNum(no));
    if (item && item->getKind() == 12)
        return;

    _b0++;
    if (_b0 > _b4)
    {
        _b0 = 0;
        _b4 = calcMaxChangeWaitCnt();
        KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(mRival->mNo);
        pad->mButtons.mTrigger |= KartCtrl::getKartCtrl()->getKartPad(mRival->mNo)->mTrigZ;
    }
}

int RivalBodyCtrl::calcMaxChangeWaitCnt()
{
    s32 wait = stGetRnd(2)->getRandomMax(1000) + 300;
    _b4 = wait;
    return wait;
}

void RivalBodyCtrl::doStateFlgCtrl() {}

void RivalBodyCtrl::comebackRescure(CLPoint *point, bool p2)
{
    CLPoint *target = point;
    if (!p2)
        target = point->getNextPoint(0);
    mPointCtrl->reset();
    mPointCtrl->init_ByCLPointPtr(target);
    resetPointData();
    resetBodyData();
    mRival->getSpeedCtrl()->reset();
}

void RivalBodyCtrl::resetPointData()
{
    mPointCtrl->getNext()->getPosition(&_28);
    _34 = mPointCtrl->_c;
    _4c = _34;
    _64.zero();
    _70.zero();
    _7c = 0.0f;
    _a8 = 0;
    _58 = _28;
    _80 = nullptr;
    mWidth = 0.5f * mPointCtrl->getNext()->getPointData()->scale;
    _8c = 0.0f;
    _90 = 0.0f;
    _a0 = 0.0f;

    JGeometry::TVec3f side(mPointCtrl->_18);
    int no = mRival->mNo;
    if (!ObjUtility::isPlayerDriver(no))
    {
        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mRival->mNo, &kartPos);
        if (getLRKartTargetDir() == 1)
            side.negate();

        f32 len = mPointCtrl->getLengthFromCLDir(kartPos);
        if (len > mWidth)
            len = 0.0f;
        _a0 = len / mWidth;
        side.scale(len);
        _58.add(side);
    }
    _94 = 0;
    _98 = 0;
}

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
        if (RCMGetManager()->getKartChecker(mRival->mNo)->getRank() == dir + sPlayerRank[i])
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
    s16 kartNum = RCMGetManager()->mRaceInfo->getKartNumber();
    for (u8 i = 0; i < kartNum; i++)
    {
        int idx = sComKartRankArray[i];
        if (idx == mRival->mNo)
        {
            rank = i + 1;
            break;
        }
    }
    return rank;
}

#line 1773
int RivalBodyCtrl::getTopComKartIndex(RivalBodyCtrl *)
{
    u8 useKartRank = 1;
    s32 ret = sComKartRankArray[0];
    if (!RCMGetManager()->mRaceDirector->isRaceEnd())
    {
        if (!RCMGetManager()->isKartGoal(mRival->mNo))
            useKartRank = 0;
    }
    if (useKartRank)
        ret = sKartRankArray[0];
    JUT_ASSERT(ret >= 0);
    return ret;
}

#line 1799
int RivalBodyCtrl::getMyFowardComKartIndex()
{
    s32 idx;
    int *rankArray = sComKartRankArray;
    s32 myRank;
    u8 useKartRank;
    s32 ret = -1;
    useKartRank = 1;
    myRank = mRival->getSpeedCtrl()->mRank;
    idx = myRank - 2;
    if (!RCMGetManager()->mRaceDirector->isRaceEnd() && !RCMGetManager()->isKartGoal(mRival->mNo))
        useKartRank = 0;
    if (useKartRank)
        rankArray = sKartRankArray;
    if (idx >= 0)
    {
#line 1804
        JUT_MINMAX_ASSERT(1, myRank, 9);
        ret = rankArray[idx];
    }
    return ret;
}

#line 1816
int RivalBodyCtrl::getMyBackwardComKartIndex(u8 offset)
{
    s32 idx = offset - 1;
    int *rankArray = sComKartRankArray;
    u8 useKartRank = 1;
    s32 ret = -1;
    idx = mRival->getSpeedCtrl()->mRank + idx;
    if (!RCMGetManager()->mRaceDirector->isRaceEnd() && !RCMGetManager()->isKartGoal(mRival->mNo))
        useKartRank = 0;
    if (useKartRank)
        rankArray = sKartRankArray;
    if (idx < RCMGetManager()->mRaceInfo->getKartNumber())
    {
#line 1828
        JUT_MINMAX_ASSERT(1, idx, 9);
        ret = rankArray[idx];
    }
    return ret;
}

void RivalBodyCtrl::setDriftInfo()
{
    u16 driftDir = mPointCtrl->getNext()->getPointData()->driftDir;
    if (driftDir == 0)
        return;
    if (_80 != nullptr)
        return;

    int dIdx = mRankParam->mKeepRank - 1;
#line 1871
    JUT_MINMAX_ASSERT(0, dIdx, RCMGetManager()->mRaceInfo->getKartNumber());
    f32 rate = sEngineBParamPtr->mDriftSuccessRate[dIdx];
    if (stGetRnd(2)->get_ufloat_1() > rate || rate == 0.0f)
        return;

    _80 = mPointCtrl->getNext();
    if (driftDir == 3)
        return;

    getPointRSideDir(&_64);
    if (_80->getPointData()->driftDir == 1)
        _64.negate();
}

void RivalBodyCtrl::doRotPose(JGeometry::TVec3f dir, f32 ratio)
{
    KartBody *body = mRival->mBody;
    JGeometry::TVec3f xAxis, yAxis, zAxis;
    yAxis.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
    zAxis.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
    xAxis.set(dir);
    xAxis.normalize();
    zAxis += xAxis * ratio * body->mSpeedScale;
    zAxis.normalize();
    body->_110[0][2] = zAxis.x;
    body->_110[1][2] = zAxis.y;
    body->_110[2][2] = zAxis.z;

    zAxis.normalize();
    PSVECCrossProduct(&yAxis, &zAxis, &xAxis);
    xAxis.normalize();
    body->_110[0][0] = xAxis.x;
    body->_110[1][0] = xAxis.y;
    body->_110[2][0] = xAxis.z;

    PSVECCrossProduct(&zAxis, &xAxis, &yAxis);
    yAxis.normalize();
    body->_110[0][1] = yAxis.x;
    body->_110[1][1] = yAxis.y;
    body->_110[2][1] = yAxis.z;

    PSMTXCopy(body->_110, body->mPlayerPosMtx);
    f32 offset = KartCtrl::getKartCtrl()->GetKartBodyOffset(body->mMynum);
    body->mPlayerPosMtx[0][3] += body->mPlayerPosMtx[0][1] * offset;
    body->mPlayerPosMtx[1][3] += body->mPlayerPosMtx[1][1] * offset;
    body->mPlayerPosMtx[2][3] += body->mPlayerPosMtx[2][1] * offset;

    body->_2f0.set(body->_110[0][0], body->_110[1][0], body->_110[2][0]);
    body->_2fc.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
    body->_308.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
}

CLPointControllerRivalKart::CLPointControllerRivalKart(RivalBodyCtrl *body)
{
    mBody = body;
}

int CLPointControllerRivalKart::findNextPointNo(CLPoint *point)
{
    int ret = 0;
    if (mBody->_84 != 3)
        ret = findRndPointNoExceptForShortCut(point, stGetRnd(2));
    return ret;
}

RivalAwardBodyCtrl::RivalAwardBodyCtrl(RivalKart *rival) : RivalBodyCtrl(rival) {}



bool RivalAwardBodyCtrl::doNextPoint(bool)
{
    mWidth = 0.5f * mPointCtrl->getNext()->getPointData()->scale;
    mPointCtrl->changeNextPoint();
    mPointCtrl->getNext()->getPosition(&_28);
    _58 = _28;
    _34 = mPointCtrl->_c;

    CLPoint *point = mPointCtrl->getNext()->getNextPoint(0);
    CLPoint *next = point->getNextPoint(0);
    JGeometry::TVec3f b, a;
    point->getPosition(&a);
    next->getPosition(&b);
    _4c.sub(b, a);
    _4c.y = 0.0f;
    stVecNormalize(&_4c);
    return true;
}

void RivalAwardBodyCtrl::doRotPosSterr(const JGeometry::TVec3f &pos)
{
    RivalBodyCtrl::doRotPosSterr(pos);
    doRotPose(pos, sAwardBodyRotRatio);
}

RivalAward2ndBodyCtrl::RivalAward2ndBodyCtrl(RivalKart *rival) : RivalAwardBodyCtrl(rival) {}



void RivalAward2ndBodyCtrl::doSterrCtrl()
{
    calcVtTargetPos();
    if (_84 == 3)
        return;
    if (mFlags & 0x80)
        return;

    JGeometry::TVec3f kartPos;
    JGeometry::TVec3f vec;
    JGeometry::TVec3f dir = mRival->mBody->mVel;
    dir.y = 0.0f;
    stVecNormalize(dir);

    ObjUtility::getKartPos(mRival->mNo, &kartPos);
    vec.sub(_58, kartPos);
    _c0 = vec.length();
    vec.y = 0.0f;

    if (mPointCtrl->chkArriveAtFwPoint(kartPos, _84 == 2 ? mWidth * sDriftRadiusRatio : mWidth) || mPointCtrl->chkIsOverFwPoint(kartPos))
    {
        if (doNextPoint(false))
            return;
    }

    stVecNormalize(vec);
    doRotPosSterr(vec);
}

void RivalAward2ndBodyCtrl::calcVtTargetPos()
{
    JGeometry::TVec3f kartPos, zdir;
    ObjUtility::getKartPos(0, &kartPos);
    ObjUtility::getKartZdir(0, &zdir);
    zdir.y = 0.0f;
    stVecNormalize(zdir);

    JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f right;
    PSVECCrossProduct(&up, &zdir, &right);
    stVecNormalize(right);

    if (mRival->mNo == 1)
        right.negate();

    right.scale(sAward2ndSideLength);
    _28.add(kartPos, right);
    _58 = _28;
}

bool RivalAward2ndBodyCtrl::doNextPoint(bool) { return false; }




#include "JSystem/JAudio/JASFakeMatch2.h"
