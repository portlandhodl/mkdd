#include "Shiraiwa/Objects/SMABird.h"

#include "JSystem/JMath/JMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/AnmController.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"

f32 TSMABird::sBirdDefaultSpeed = 11.0f;
f32 TSMABird::sBirdFlyAmp = 200.0f;
int TSMABird::sBirdFlyCycle = 0x8000;
J3DAnmTransform *TSMADonkyBird::sDonkyBirdBckAnmTrans;
J3DMtxCalc *TSMADonkyBird::sDonkyBirdBckMtxCalc;

#include "JSystem/JAudio/JASFakeMatch2.h"

static inline int loopValue(int value, int max)
{
    if (max <= 0) {
        return 0;
    }
    while (value < 0) {
        value += max;
    }
    while (value >= max) {
        value -= max;
    }
    return value;
}

TSMABird::TSMABird(u32 id) : TSmallAnimal(id)
{
    mFreeFly = nullptr;
    mGroup = nullptr;
    mSpeedRate = 1.0f;
    mFreeFly = new TFreeFly();
    mFreeFly->init(&mBasePos, &mVel, &mFlyMtx);
}

TSMABird::~TSMABird()
{
    delete mFreeFly;
}

void TSMABird::reset()
{
    mFlyPhase = 0;
    mFlyPhaseSpeed = 0;
    PSMTXIdentity(mFlyMtx);
    mFreeFly->reset();
    mFreeFly->setParameters(sBirdDefaultSpeed * mSpeedRate, 0.02f, 0.02f);
}

void TSMABird::calc()
{
    mPos.set(mBasePos.x, mBasePos.y + getOffset(), mBasePos.z);
    if (!mFreeFly->update()) {
        JGeometry::TVec3f target;
        mGroup->getTargetPos(mGroupIdx, target);
        f32 dist = PSVECDistance(&target, &mPos);
        f32 pathDist = mGroup->mPathUtil->getDistance();
        TFreeFly *freeFly = mFreeFly;
        freeFly->mSpeed = sBirdDefaultSpeed * (dist / (1.5f * pathDist));
        freeFly->mTarget.set(target);
        freeFly->_54 = true;
        mGroup->checkAllPass();
    }
    Course *course = RaceMgr::sRaceManager->getCourse();
    JGeometry::TPos3f mtx;
    f32 xx = course->mDirX.x;
    f32 xy = course->mDirX.y;
    f32 xz = course->mDirX.z;
    f32 yx = course->mDirY.x;
    f32 yy = course->mDirY.y;
    f32 yz = course->mDirY.z;
    f32 zx = course->mDirZ.x;
    f32 zy = course->mDirZ.y;
    f32 zz = course->mDirZ.z;
    mtx.setXDir(xx, xy, xz);
    mtx.setYDir(yx, yy, yz);
    mtx.setZDir(zx, zy, zz);
    PSMTXMultVecSR(mtx, &mPos, &mPos);
    PSMTXConcat(mtx, mFlyMtx, mRotMtx);
    mFlyPhase += mFlyPhaseSpeed;
}

f32 TSMABird::getOffset()
{
    mFlyPhase = loopValue(mFlyPhase, sBirdFlyCycle);
    return sBirdFlyAmp * JMASSin(mFlyPhase);
}

TSMADonkyBird::TSMADonkyBird() : TSMABird(0x119d)
{
    NewAnmCtrl();
}

const char *TSMADonkyBird::getBmdFileName()
{
    static const char *cBmdName = "/Objects/bird1.bmd";
    return cBmdName;
}

void TSMADonkyBird::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    J3DAnmObjTrans::setupTransAnmData(&sDonkyBirdBckAnmTrans, &sDonkyBirdBckMtxCalc, modelData, ObjUtility::getPtrCourseArc("/Objects/bird1.bca"));
}

void TSMADonkyBird::reset()
{
    TSMABird::reset();
    J3DFrameCtrl *frameCtrl = mAnmCtrl->mTrans->getObj(0)->getFrameCtrl();
    frameCtrl->setAttribute(2);
    s16 frame = SiUtil::setRandomStartFrame(frameCtrl, getGeoRnd());
    mFlyPhase = sBirdFlyCycle * ((f32)frame / (f32)frameCtrl->getEnd());
    mFlyPhaseSpeed = (f32)sBirdFlyCycle / (f32)frameCtrl->getEnd();
}

void TSMADonkyBird::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createModel(heap, viewNo, 0);
    AnmController *ctrl = mAnmCtrl;
    ctrl->mTrans = new AnmControlTrans();
    ctrl->mTrans->initAnm(1, &mModel);
    mAnmCtrl->mTrans->registration(0, sDonkyBirdBckAnmTrans, sDonkyBirdBckMtxCalc);
}
