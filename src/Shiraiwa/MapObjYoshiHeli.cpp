#include "Shiraiwa/Objects/MapObjYoshiHeli.h"
#include "JSystem/JMath/JMath.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ObjUtility.h"
#include "mathHelper.h"

StateObserver::StateFuncSet<TMapObjYoshiHeli> TMapObjYoshiHeli::sTable[3] = {
    {0, &TMapObjYoshiHeli::initFunc_Fly, &TMapObjYoshiHeli::doFunc_Fly},
    {1, &TMapObjYoshiHeli::initFunc_Search, &TMapObjYoshiHeli::doFunc_Search},
    {2, &TMapObjYoshiHeli::initFunc_ForceBack, &TMapObjYoshiHeli::doFunc_ForceBack},
};

f32 TMapObjYoshiHeli::sAirFriction = 0.98f;
f32 TMapObjYoshiHeli::sAccel = 2.0f;
f32 TMapObjYoshiHeli::sTurnMaxSpeed = 0.5f;
f32 TMapObjYoshiHeli::sTurnMinSpeed = 0.05f;
f32 TMapObjYoshiHeli::sMinMoveDistance = 10.0f;
f32 TMapObjYoshiHeli::sLeanVelY = 500.0f;
f32 TMapObjYoshiHeli::sFrontTurnMaxSpeed = 0.02f;
f32 TMapObjYoshiHeli::sFloatHeight = 500.0f;
s16 TMapObjYoshiHeli::sFloatSpeed = 250;
J3DAnmTextureSRTKey *TMapObjYoshiHeli::sYoshiHeliBtkAnm;

#include <JSystem/JAudio/JASFakeMatch2.h>

TMapObjYoshiHeli::TMapObjYoshiHeli(const CrsData::SObject &obj) : TMapObjHioNode(obj), StateObserver()
{
    createSoundMgr();
    mPathUtil = new TPathUtil(&obj);
}

TMapObjYoshiHeli::~TMapObjYoshiHeli()
{
    delete mPathUtil;
}

void TMapObjYoshiHeli::reset()
{
    resetObject();
    ResetState();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    _58 = 0;
    mAnmObj.mFrameCtrl.reset();

    mSearchRadius = mObjData->mParam1;
    if (mSearchRadius <= 0.0f) {
        mSearchRadius = 10000.0f;
    }
    _1ac = mObjData->mParam2;
    _1ac = 20000.0f;

    mInitPos.set(mPos);
    mBasePos.set(mPos);
    _190.zero();
    mFront.zero();
    _1b0 = 0.0f;
    mFloatAngle = 0;
    mPathUtil->reset();
}

void TMapObjYoshiHeli::calc()
{
    ExecuteState();
    mPos.set(mBasePos);
    mPos.y += sFloatHeight * JMath::sincosTable_.sinShort(mFloatAngle);
    mFloatAngle += sFloatSpeed;
    mAnmObj.mFrameCtrl.update();
    mSoundMgr->setSe(0x40062);
}

void TMapObjYoshiHeli::update()
{
    mAnmObj.anmFrameProc();
    GeographyObj::update();
}

void TMapObjYoshiHeli::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    J3DAnmObjMaterial::setupTexSRTAnmData(&sYoshiHeliBtkAnm, modelData,
                                          ObjUtility::getPtrCourseArc("/Objects/YoshiHeli.btk"));
}

void TMapObjYoshiHeli::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x200, false);
    mAnmObj.setExModel(&mModel);
    mAnmObj.attach(sYoshiHeliBtkAnm);
}

void TMapObjYoshiHeli::InitExec()
{
    Observer_FindAndInit(TMapObjYoshiHeli, 3);
}

void TMapObjYoshiHeli::MoveExec()
{
    Observer_FindAndExec(TMapObjYoshiHeli, 3);
}

void TMapObjYoshiHeli::initFunc_Fly()
{
    _1b8 = 0;
    mVel.zero();
}

void TMapObjYoshiHeli::doFunc_Fly()
{
    turn(mFront);
    s16 kart = findNearPlayerKart(mSearchRadius);
    if (kart != -1) {
        mTargetKart = kart;
        s16 console = -1;
        for (u32 i = 0; console == -1 && i < RaceMgr::getManager()->getConsoleNumber(); i++) {
            if (mTargetKart == RaceMgr::getManager()->getTargetKartNo(i)) {
                console = i;
            }
        }
        if (console != -1) {
            mTargetConsole = console;
            setState(1);
        }
    }
}

s16 TMapObjYoshiHeli::findNearPlayerKart(f32 radius)
{
    f32 minDist = radius * radius;
    s16 ret = -1;
    for (s16 i = 0; i < RaceMgr::getManager()->getKartNumber(); i++) {
        if (RaceMgr::getManager()->getKartInfo(i)->isPlayerKart()) {
            JGeometry::TVec3f pos;
            ObjUtility::getKartPos(i, &pos);
            pos.sub(mObjData->position);
            f32 dist = pos.x * pos.x + pos.z * pos.z;
            if (dist < minDist) {
                ret = i;
                minDist = dist;
            }
        }
    }
    return ret;
}

void TMapObjYoshiHeli::initFunc_Search()
{
    _1b0 = 0.0f;
    _190.zero();
}

void TMapObjYoshiHeli::doFunc_Search()
{
    JGeometry::TVec3f target;
    target.set(RaceMgr::getManager()->getCourse()->getCrsData()->getPointData(mPathUtil->mObj->pathID, (u16)mPathUtil->mNode)->pos);
    JGeometry::TVec3f dir;
    dir.sub(target, mBasePos);
    f32 dist = dir.squared();
    dir.normalize();
    move(target, true);
    turn(dir);
    if (dist < 9000000.0f) {
        TPathUtil *util = mPathUtil;
        util->mDistanceDirty = true;
        util->mNode = util->getNextNode();
        if (mPathUtil->getNextNode() == 0) {
            setState(2);
        }
    }
}

void TMapObjYoshiHeli::initFunc_ForceBack() {}

void TMapObjYoshiHeli::doFunc_ForceBack()
{
    JGeometry::TVec3f dir;
    dir.sub(mObjData->position, mBasePos);
    f32 dist = dir.normalize();
    move(mObjData->position, true);
    turn(dir);
    if (dist < 3000.0f) {
        mVel.scale(0.89f);
        if (mVel.length() <= 15.0f) {
            setState(0);
        }
    }
}

void TMapObjYoshiHeli::getTargetPos(JGeometry::TVec3f *pos, u8 kartNo)
{
    ObjUtility::getKartPos(kartNo, pos);
    JGeometry::TVec3f dir;
    dir.sub(*pos, mBasePos);
    dir.normalize();
    JGeometry::TVec3f side;
    side.cross(dir, mFront);
    side.normalize();
    pos->scaleAdd(500.0f, side, *pos);
    pos->y += 1000.0f;
}

void TMapObjYoshiHeli::getKartPos(u8 kartNo)
{
    JGeometry::TVec3f pos;
    mTargetKart = kartNo;
    ObjUtility::getKartPos(kartNo, &pos);
    _190.sub(pos, mBasePos);
    _190.y = 0.0f;
    _1b0 = _190.length();
}

void TMapObjYoshiHeli::move(const JGeometry::TVec3f &, bool)
{
    JGeometry::TVec3f front;
    front.scaleAdd(100.0f, mFront, mBasePos);
    mVel.scale(sAirFriction);
    if (mVel.length() > sMinMoveDistance) {
        mBasePos.add(mVel);
    }
    mVel.scaleAdd(sAccel, mFront);
}

bool TMapObjYoshiHeli::isWall(const JGeometry::TVec3f &pos)
{
    CrsGround ground(RaceMgr::getManager()->getCourse());
    ground.search(pos);
    if (ground.isAttributeCollision()) {
        return true;
    }
    return false;
}

void TMapObjYoshiHeli::turn(const JGeometry::TVec3f &dir)
{
    JGeometry::TVec3f front;
    front.set(mFront);
    front.normalize();
    JGeometry::TVec3f diff;
    diff.sub(dir, front);
    JGeometry::TVec3f tmp(diff);
    f32 len = tmp.normalize();
    if (len > sTurnMaxSpeed) {
        diff.setLength(sTurnMaxSpeed);
    } else if (len < sTurnMinSpeed) {
        diff.zero();
    }
    if (dir.dot(front) < 0.0f) {
        diff.scale(10.0f);
    }
    diff.scale(0.1f);
    front.add(diff);
    front.normalize();
    mFront.set(front);
    mFront.y *= 0.01f;
    setFront(front);
}

void TMapObjYoshiHeli::setFront(const JGeometry::TVec3f &dir)
{
    JGeometry::TVec3f x, y, z, diff;
    mRotMtx.getZDir(z);
    z.normalize();
    diff.sub(dir, z);
    diff.y = 0.0f;
    f32 len = diff.length();
    if (len > sFrontTurnMaxSpeed) {
        diff.scale(sFrontTurnMaxSpeed / len);
    }
    z.add(diff);
    z.normalize();
    y.set(0.0f, sLeanVelY, 0.0f);
    y.add(mVel);
    y.normalize();
    x.cross(y, z);
    x.normalize();
    z.cross(x, y);
    z.normalize();
    mRotMtx.setXYZDir(x, y, z);
}
