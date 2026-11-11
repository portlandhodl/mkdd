#include "Shiraiwa/Objects/SMAButterfly.h"

#include "JSystem/JMath/JMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/GeographyObjMgr.h"

s16 TSMAButterfly::sMoveHeight = 200;
s16 TSMAButterfly::sMoveWidth = 200;
J3DAnmTransform *TSMAButterfly::sButterflyBckAnmTrans;
J3DMtxCalc *TSMAButterfly::sButterflyBckMtxCalc;
J3DAnmTexPattern *TSMAButterfly::sButterflyBtpAnm;

TSMAButterfly::TSMAButterfly() : TSmallAnimal(0x119b) {
    if (mAnmCtrl == nullptr) {
        mAnmCtrl = new AnmController();
    }
}

const char *TSMAButterfly::getBmdFileName() {
    static s8 init;
    static const char *cBmdName;
    if (!init) {
        cBmdName = "/Objects/butterfly1.bmd";
        init = 1;
    }
    return cBmdName;
}

void TSMAButterfly::reset() {
    resetObject();
    mGeoObjFlag &= ~1;
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~2;
    mPos.set(*mOrigin);
    mTarget.set(*mOrigin);
    f32 rate = 0.5f * getGeoRnd()->get_ufloat_1() + 0.75f;
    mAnmCtrl->mTrans->getObj(0)->getFrameCtrl()->setRate(rate);
    mBtp.mFrameCtrl.mFrame = (u16)getGeoRnd()->getRandomMax((s16)(mBtp.mFrameCtrl.mEndFrame - 1));
}

void TSMAButterfly::setOrigin(JGeometry::TVec3f *origin) {
    mOrigin = origin;
}

void TSMAButterfly::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/butterfly1.bca");
    J3DAnmObjTrans::setupTransAnmData(&sButterflyBckAnmTrans, &sButterflyBckMtxCalc, modelData, bck);
    void *btp = ObjUtility::getPtrCourseArc("/Objects/butterfly1.btp");
    J3DAnmObjMaterial::setupTexPatternAnmData(&sButterflyBtpAnm, modelData, btp);
}

void TSMAButterfly::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createDifferedModel(heap, viewNo, 0x80000, false);
    AnmController *ctrl = mAnmCtrl;
    ctrl->mTrans = new AnmControlTrans();
    ctrl->mTrans->initAnm(1, &mModel);
    mAnmCtrl->mTrans->registration(0, sButterflyBckAnmTrans, sButterflyBckMtxCalc);
    mAnmCtrl->mTrans->getObj(0)->mFrameCtrl.mAttr = 2;
    mBtp.mModel = &mModel;
    mBtp.mAnmBase = sButterflyBtpAnm;
    mBtp.initFrameCtrl(mBtp.mAnmBase);
}

void TSMAButterfly::calc() {
    if (checkReachTarget()) {
        s16 pitch = getRandomDeg(60, -60);
        s16 yaw = getRandomDeg(180, -180);
        mTarget.x = sMoveWidth * JMASCos(yaw);
        mTarget.z = sMoveWidth * JMASSin(yaw);
        mTarget.y = sMoveHeight * JMASSin(pitch);
        mTarget.scale(getGeoRnd()->get_ufloat_1() + 0.5f);
        mTarget.add(*mOrigin);
        JGeometry::TVec3f zDir;
        zDir.sub(mTarget, mPos);
        zDir.normalize();
        mVel.set(zDir);
        mVel.scale(2.5f);
        JGeometry::TVec3f yDir(0.0f, 1.0f, 0.0f);
        JGeometry::TVec3f xDir;
        if (zDir.y >= -0.4f) {
            xDir.cross(yDir, zDir);
            xDir.normalize();
            yDir.cross(zDir, xDir);
            yDir.normalize();
            mRotMtx.setXYZDir(xDir, yDir, zDir);
        } else {
            zDir.negate();
            yDir.set(zDir);
            mRotMtx.getZDir(zDir);
            xDir.cross(yDir, zDir);
            xDir.normalize();
            zDir.cross(xDir, yDir);
            zDir.normalize();
            mRotMtx.setXYZDir(xDir, yDir, zDir);
        }
    }
    mPos.add(mVel);
}

void TSMAButterfly::update() {
    mBtp.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}

bool TSMAButterfly::checkReachTarget() {
    bool ret = false;
    JGeometry::TVec3f delta;
    delta.sub(mTarget, mPos);
    if (delta.squared() < 2500.0f) {
        ret = true;
    }
    return ret;
}

s16 TSMAButterfly::getRandomDeg(s16 min, s16 max) {
    s16 d = min - max;
    int deg = max + getGeoRnd()->getRandomMax(d);
    return (s16)(182.04445f * deg);
}

StateObserver::StateFuncSet<TSMAButterflyGen> TSMAButterflyGen::sTable[2] = {
    {0, &TSMAButterflyGen::initFunc_Fly, &TSMAButterflyGen::doFunc_Fly},
    {1, &TSMAButterflyGen::initFunc_Rest, &TSMAButterflyGen::doFunc_Rest},
};
s16 TSMAButterflyGen::sButterflyRestFrame = 600;

#include "JSystem/JAudio/JASFakeMatch2.h"

TSMAButterflyGen::TSMAButterflyGen(const CrsData::SObject &obj) : TSmallAnimalGen(obj, 0x119b), StateObserver() {
    mGeoObjFlag &= ~1;
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~2;
    mPathMove = new TPathMove(&obj);
    mPathMove->init(&mPos, &mVel);
}

TSMAButterflyGen::~TSMAButterflyGen() {
    delete mPathMove;
}

void TSMAButterflyGen::reset() {
    resetObject();
    TSmallAnimalGen::reset();
    ResetState();
    mPathMove->reset();
    if (mObjData->pathID != 0xffff) {
        TPathMove *path = mPathMove;
        path->_14 = 0.5f;
        path->_18 = 2.5f;
        path->_20 = 1.5625f;
    }
}

void TSMAButterflyGen::calc() {
    ExecuteState();
    if (mObjData->pathID != 0xffff) {
        mPathMove->update();
    }
}

void TSMAButterflyGen::InitExec() {
    const StateObserver::StateFuncSet<TSMAButterflyGen> *func =
        FindStateFunc<TSMAButterflyGen>(getState(), sTable, 2);
    (this->*func->mInitFunc)();
}

void TSMAButterflyGen::MoveExec() {
    const StateObserver::StateFuncSet<TSMAButterflyGen> *func =
        FindStateFunc<TSMAButterflyGen>(getState(), sTable, 2);
    (this->*func->mExecFunc)();
}

void TSMAButterflyGen::initFunc_Fly() {
    mPathMove->setTargetNode();
}

void TSMAButterflyGen::doFunc_Fly() {
    if (!mPathMove->_1c) {
        setState(1);
    }
}

void TSMAButterflyGen::initFunc_Rest() {}

void TSMAButterflyGen::doFunc_Rest() {
    if (getStateCount() > sButterflyRestFrame) {
        setState(0);
    }
}
