#include "Shiraiwa/Objects/MapObjWall.h"
#include "Sato/AnmController.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/ObjCollision.h"
#include "Sato/ObjUtility.h"

u8 TMapObjWall::sUseBca = 1;
static u8 sPad[4] = {0};
J3DAnmTransform *TMapObjWall::sWlWallBckAnmTrans;
J3DMtxCalc *TMapObjWall::sWlWallBckMtxCalc;

StateObserver::StateFuncSet<TMapObjWall> TMapObjWall::sTable[2] = {
    0, &TMapObjWall::initFunc_Move, &TMapObjWall::doFunc_Move,
    1, &TMapObjWall::initFunc_Rest, &TMapObjWall::doFunc_Rest,
};

#include <JSystem/JAudio/JASFakeMatch2.h>

TMapObjWall::TMapObjWall(const CrsData::SObject &obj) : TMapObjHioNode(obj), StateObserver()
{
    NewAnmCtrl();
    createSoundMgr();
    mPathMove = new TPathMove(&obj);
    reset();
}

TMapObjWall::~TMapObjWall() {}

void TMapObjWall::reset()
{
    resetObject();
    mCheckKartHitFlags = -1;
    mGeoObjFlag |= 2;

    mRestTime = mObjData->mParam1;
    f32 rest = mObjData->mParam2;
    if (rest <= 0.0f) {
        rest = 10.0f;
    }

    if (IsPathMove()) {
        mPathMove->init(&mPos, &mVel);
        TPathMove *path = mPathMove;
        path->_14 = 1.0f;
        f32 speed = 0.5f * rest;
        path->_18 = rest;
        path->_20 = speed * speed;
        ResetState();
    } else {
        setState(1);
    }

    if (sUseBca) {
        mAnmCtrl->Reset();
    }
    mReaction.setFlg(7, 2);
    _58 = 1;
}

const char *TMapObjWall::getBmdFileName()
{
    static const char *cBmdName = "/Objects/wl_wall1.bmd";
    return cBmdName;
}

void TMapObjWall::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *bca = ObjUtility::getPtrCourseArc("/Objects/wl_wall1.bca");
    if (bca != nullptr) {
        sUseBca = true;
        J3DAnmObjTrans::setupTransAnmData(&sWlWallBckAnmTrans, &sWlWallBckMtxCalc, modelData, bca);
    } else {
        sUseBca = false;
    }
}

void TMapObjWall::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createModel(heap, viewNo, 0);
    if (sUseBca) {
        {
            AnmController *anmCtrl = mAnmCtrl;
            anmCtrl->InitRegistration(1, &mModel);
        }
        mAnmCtrl->RegisterTrans(0, sWlWallBckAnmTrans, sWlWallBckMtxCalc);
        mAnmCtrl->getFrameCtrl(0)->setAttribute(2);
    }
}

void TMapObjWall::calc()
{
    ExecuteState();
    if (IsPathMove()) {
        mPathMove->update();
    }

    Mtx mtx;
    PSMTXCopy(mRotMtx, mtx);
    mtx[0][3] = mPos.x;
    mtx[1][3] = mPos.y;
    mtx[2][3] = mPos.z;
    ((ObjColCube *)mBounds[0])->updateParameter(mtx);
}

void TMapObjWall::InitExec()
{
    Observer_FindAndInit(TMapObjWall, 2);
}

void TMapObjWall::MoveExec()
{
    Observer_FindAndExec(TMapObjWall, 2);
}

void TMapObjWall::initFunc_Move()
{
    switch (mPathMove->_8) {
    case 0:
        mSoundMgr->setSe(0x4004e);
        break;
    case 1:
        mSoundMgr->setSe(0x4004f);
        break;
    }
    mPathMove->setTargetNode();
}

void TMapObjWall::doFunc_Move()
{
    if (!mPathMove->_1c) {
        mVel.zero();
        setState(1);
    }
}

void TMapObjWall::initFunc_Rest() {}

void TMapObjWall::doFunc_Rest()
{
    if (IsPathMove()) {
        if (mPathMove->_8 == 0 || getStateCount() > mRestTime) {
            setState(0);
        }
    }
}

void TMapObjWall::createColModel(J3DModelData *modelData)
{
    createBoundsCube(modelData);
}
