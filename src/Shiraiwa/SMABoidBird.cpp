#include "Shiraiwa/Objects/SMABoidBird.h"

#include "JSystem/JMath/JMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/AnmController.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ObjUtility.h"

f32 TSMABoidBirdManager::sLeaderSpeed = 20.0f;
f32 TSMABoidBird::sLeaderAccel = 1.5f;
f32 TSMABoidBird::sLeaderReachLength2 = 1000000.0f;
J3DAnmTransform *TSMAKamome::sKamomeBckAnmTrans;
J3DMtxCalc *TSMAKamome::sKamomeBckMtxCalc;

#include "JSystem/JAudio/JASFakeMatch2.h"

TSMABoidBirdManager::TSMABoidBirdManager(const CrsData::SObject &obj) : TMapObjHioNode(obj)
{
    mNum = obj.mParam1 + 1;
    if (mNum < 1) {
        mNum = 1;
    }
    mBoidsMgr = new TBoidsManager(mNum, obj.mParam2);
    mPathUtil = new TPathUtil(&obj);
    for (u32 i = 0; i < mNum; i++) {
        TSMABoidBird *bird = (TSMABoidBird *)GeographyObjManager::gMgr->createSubObj(0xd4c);
        TBoid *boid = mBoidsMgr->mBoids[i];
        boid->mPos = &bird->mPos;
        boid->mVel = &bird->mVel;
        boid->mMat = bird->mRotMtx;
        if (i == 0) {
            bird->mIsLeader = true;
            mLeader = bird;
        }
    }
}

TSMABoidBirdManager::~TSMABoidBirdManager()
{
    delete mBoidsMgr;
}

void TSMABoidBirdManager::reset()
{
    resetObject();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    mBoidsMgr->reset(mPos);
    mPathUtil->reset();
    JGeometry::TVec3f dir;
    mRotMtx.getZDir(dir);
    dir.normalize();
    for (u32 i = 0; i < mNum; i++) {
        JGeometry::TVec3f pos;
        pos.set(mPos);
        JMAVECScaleAdd(&dir, &pos, &pos, 200.0f * i);
        mBoidsMgr->mBoids[i]->mPos->set(pos);
    }
}

void TSMABoidBirdManager::calc()
{
    mBoidsMgr->update();
    moveLeader();
}

void TSMABoidBirdManager::moveLeader()
{
    JGeometry::TVec3f target;
    target.set(RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mPathUtil->mObj->pathID, (u16)mPathUtil->mNode)->pos);
    if (mLeader->moveTo(target, sLeaderSpeed)) {
        TPathUtil *util = mPathUtil;
        util->mDistanceDirty = true;
        util->mNode = util->getNextNode();
    }
}

TSMABoidBird::TSMABoidBird(u32 id) : GeographyObj(id)
{
    mIsLeader = false;
    createSoundMgr();
}

void TSMABoidBird::calc()
{
    if (mIsLeader) {
        mSoundMgr->setSe(0x40027);
    }
}

void TSMABoidBird::reset()
{
    PSMTXIdentity(mRotMtx);
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    if (mIsLeader) {
        setObjFlagHidding();
    }
}

bool TSMABoidBird::moveTo(const JGeometry::TVec3f &target, f32 maxSpeed)
{
    JGeometry::TVec3f dir;
    dir.sub(target, mPos);
    dir.normalize();
    dir.setLength(sLeaderAccel);
    mVel.add(dir);
    if (PSVECMag(&mVel) > maxSpeed) {
        mVel.setLength(maxSpeed);
    }
    mPos.add(mVel);
    bool reached = false;
    JGeometry::TVec3f diff;
    diff.sub(mPos, target);
    if (diff.squared() < sLeaderReachLength2) {
        reached = true;
    }

    JGeometry::TVec3f zDir;
    if (mVel.isZero()) {
        mRotMtx.getZDir(zDir);
        if (zDir.isZero()) {
            zDir.set(0.0f, 0.0f, 1.0f);
        }
    } else {
        zDir.set(mVel);
    }
    zDir.normalize();
    JGeometry::TVec3f xDir;
    JGeometry::TVec3f yDir(0.0f, 1.0f, 0.0f);
    yDir.normalize();
    PSVECCrossProduct(&yDir, &zDir, &xDir);
    xDir.normalize();
    return reached;
}

TSMAKamome::TSMAKamome() : TSMABoidBird(0xd4c)
{
    NewAnmCtrl();
}

void TSMAKamome::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    J3DAnmObjTrans::setupTransAnmData(&sKamomeBckAnmTrans, &sKamomeBckMtxCalc, modelData, ObjUtility::getPtrCourseArc("/Objects/kamome1.bca"));
}

void TSMAKamome::reset()
{
    TSMABoidBird::reset();
    J3DAnmObjTrans *obj = mAnmCtrl->mTrans->getObj(0);
    obj->mFrameCtrl.setAttribute(2);
    obj->mFrameCtrl.setFrame((s16)getGeoRnd()->getRandomMax(obj->mFrameCtrl.getEnd()));
}

void TSMAKamome::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createModel(heap, viewNo, 0);
    AnmController *ctrl = mAnmCtrl;
    ctrl->mTrans = new AnmControlTrans();
    ctrl->mTrans->initAnm(1, &mModel);
    mAnmCtrl->mTrans->registration(0, sKamomeBckAnmTrans, sKamomeBckMtxCalc);
}
