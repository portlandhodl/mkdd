#include "Shiraiwa/Objects/SMAButterfly.h"

#include "Kaneshige/RaceMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/GeographyObjMgr.h"

s16 TSMAButterfly::sMoveHeight = 100;
s16 TSMAButterfly::sMoveWidth = 500;
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
    mPos.x = mOrigin->x;
    mPos.y = mOrigin->y;
    mPos.z = mOrigin->z;
    mTarget.x = mOrigin->x;
    mTarget.y = mOrigin->y;
    mTarget.z = mOrigin->z;
    f32 rate = 0.5f * (getGeoRnd()->get_ufloat_1() - 1.0f) + 0.75f;
    AnmControlTrans *trans = mAnmCtrl->mTrans;
    trans->getObj(0)->mFrameCtrl.setRate(rate);
    J3DAnmObjMaterial::mFrameCtrl.mFrame = (u16)getGeoRnd()->getRandomMax((s16)(J3DAnmObjMaterial::mFrameCtrl.mEndFrame - 1));
}

void TSMAButterfly::setOrigin(JGeometry::TVec3f *origin) {
    mOrigin = origin;
}

void TSMAButterfly::loadAnimation() {
    J3DModelData *modelData = this->TSmallAnimal::mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/butterfly1.bca");
    J3DAnmObjTrans::setupTransAnmData(&sButterflyBckAnmTrans, &sButterflyBckMtxCalc, modelData, bck);
    void *btp = ObjUtility::getPtrCourseArc("/Objects/butterfly1.btp");
    J3DAnmObjMaterial::setupTexPatternAnmData(&sButterflyBtpAnm, modelData, btp);
}

void TSMAButterfly::createModel(JKRSolidHeap *heap, u32 viewNo) {
    this->TSmallAnimal::mModel.createDifferedModel(heap, viewNo, 0x80000, false);
    AnmController *ctrl = mAnmCtrl;
    ctrl->mTrans = new AnmControlTrans();
    ctrl->mTrans->initAnm(1, &this->TSmallAnimal::mModel);
    mAnmCtrl->mTrans->registration(0, sButterflyBckAnmTrans, sButterflyBckMtxCalc);
    mAnmCtrl->mTrans->getObj(0)->mFrameCtrl.mAttr = 2;
    J3DAnmObjMaterial::mModel = &this->TSmallAnimal::mModel;
    J3DAnmObjMaterial::mAnmBase = sButterflyBtpAnm;
    J3DAnmObjMaterial::initFrameCtrl(J3DAnmObjMaterial::mAnmBase);
}

void TSMAButterfly::calc() {}

void TSMAButterfly::update() {
    J3DAnmObjMaterial &mat = *this;
    mat.anmFrameProc();
    setModelMatrixAndScale();
    this->TSmallAnimal::mModel.update(0);
}

bool TSMAButterfly::checkReachTarget() {
    bool ret = false;
    JGeometry::TVec3f delta;
    delta.sub(mTarget, mPos);
    if (delta.squared() < 1.0f) {
        ret = true;
    }
    return ret;
}

s16 TSMAButterfly::getRandomDeg(s16 min, s16 max) {
    s16 d = min - max;
    int deg = max + getGeoRnd()->getRandomMax(d);
    return (s16)(deg * 0.01f);
}

StateObserver::StateFuncSet<TSMAButterflyGen> TSMAButterflyGen::sTable[2] = {
    {0, &TSMAButterflyGen::initFunc_Fly, &TSMAButterflyGen::doFunc_Fly},
    {1, &TSMAButterflyGen::initFunc_Rest, &TSMAButterflyGen::doFunc_Rest},
};
s16 TSMAButterflyGen::sButterflyRestFrame = 100;

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
        path->_14 = 0.0f;
        path->_18 = 30.0f;
        path->_20 = 0.2f;
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
