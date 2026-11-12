#include "Kaneshige/Course/CrsData.h"
#include "Osako/ResMgr.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Sato/Objects/MapObjMare.h"

TAnmInfo TMapObjMareBase::sAnmInfos[2] = {
    { "/Objects/mareL_a_bye1.bck", nullptr, nullptr, 2, 10, 0, 1, 0 },
    { "/Objects/mareL_a_clap1.bck", nullptr, nullptr, 2, 10, 0, 0, 0 },
};

StateObserver::StateFuncSet<TMapObjMareBase> TMapObjMareBase::sTable[2] = {
    { 0, &TMapObjMareBase::initFunc_Banzai, &TMapObjMareBase::doFunc_Banzai },
    { 1, &TMapObjMareBase::initFunc_Clapping, &TMapObjMareBase::doFunc_Clapping },
};

const s8 TMapObjMareBase::sAnmTable[2] = { 0, 1 };

#include "JSystem/JAudio/JASFakeMatch2.h"

J3DAnmTransform *TMapObjMareWBase::sMareWBckAnmTrans;
J3DMtxCalc *TMapObjMareWBase::sMareWBckMtxCalc;

J3DAnmTransform *TMapObjMareM_A::sMareMBckAnmTrans;
J3DMtxCalc *TMapObjMareM_A::sMareMBckMtxCalc;

TMapObjMareBase::TMapObjMareBase(const CrsData::SObject &obj) : TMapObjDemoObj(obj), StateObserver()
{
    NewAnmCtrl();
    mAnmPlayer.resetAnimations(sAnmInfos, 2);
}

TMapObjMareBase::~TMapObjMareBase() {}

void TMapObjMareBase::reset()
{
    TMapObjDemoObj::reset();
    mAnmCtrl->Reset();
    mAnmPlayer.init(mAnmCtrl, sAnmInfos, 2);

    f32 length = (f32)mObjData->mParam1;
    if (length <= 0.0f)
        length = 700.0f;
    mLenSq2 = length * length;

    SiUtil::setRandomStartFrame(mAnmCtrl->mTrans->getObjAnt(0)->getFrameCtrl(), getGeoRnd());
    StateObserver::ResetState();
}

void TMapObjMareBase::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    mAnmPlayer.loadAnimations(sAnmInfos, 2, modelData, ResMgr::mcArcCourse);

    for (u16 i = 0; i < modelData->getShapeNum(); i++)
        modelData->getShapeNodePointer(i)->setTexMtxLoadType(0x2000);
}

void TMapObjMareBase::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x1000200, true);
    mAnmPlayer.registAnimations(mAnmCtrl, &mModel, sAnmInfos, 2);
}

void TMapObjMareBase::InitExec()
{
    const StateFuncSet<TMapObjMareBase> *func = FindStateFunc(getState(), sTable, 2);
    (this->*func->mInitFunc)();
}

void TMapObjMareBase::changeAllState(u16 state)
{
    for (u16 i = 0; i < 2; i++)
    {
        if (state == sTable[i].mId)
        {
            setState(state);
            i = 2;
        }
    }
}

void TMapObjMareBase::MoveExec()
{
    const StateFuncSet<TMapObjMareBase> *func = FindStateFunc(getState(), sTable, 2);
    (this->*func->mExecFunc)();
}

void TMapObjMareBase::calc()
{
    TMapObjDemoObj::calc();
    mAnmPlayer.update();
    StateObserver::ExecuteState();
}

void TMapObjMareBase::initFunc_Clapping() {}

void TMapObjMareBase::doFunc_Clapping()
{
    if (mLength2 > mLenSq2)
    {
        setState(0);
        if (mAnmPlayer.getCurAnmNumber() != sAnmTable[0])
        {
            mAnmPlayer._11 = sAnmTable[0];
            mAnmPlayer._10 = 1;
            mAnmPlayer._e |= 1;
        }
    }
}

void TMapObjMareBase::initFunc_Banzai() {}

void TMapObjMareBase::doFunc_Banzai()
{
    if (mLength2 < mLenSq2)
    {
        setState(1);
        if (mAnmPlayer.getCurAnmNumber() != sAnmTable[1])
        {
            mAnmPlayer._11 = sAnmTable[1];
            mAnmPlayer._10 = 1;
            mAnmPlayer._e |= 1;
        }
    }
}

TMapObjMareWBase::TMapObjMareWBase(const CrsData::SObject &obj) : TMapObjDemoObj(obj)
{
    NewAnmCtrl();
}

TMapObjMareWBase::~TMapObjMareWBase() {}

void TMapObjMareWBase::reset()
{
    TMapObjDemoObj::reset();
    mAnmCtrl->Reset();
    SiUtil::setRandomStartFrame(mAnmCtrl->mTrans->getObjAnt(0)->getFrameCtrl(), getGeoRnd());
}

void TMapObjMareWBase::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/mareW_a_dance.bck");
    J3DAnmObjTrans::setupTransAnmData(&sMareWBckAnmTrans, &sMareWBckMtxCalc, modelData, bck);

    for (u16 i = 0; i < modelData->getShapeNum(); i++)
        modelData->getShapeNodePointer(i)->setTexMtxLoadType(0x2000);
}

void TMapObjMareWBase::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x1000200, true);

    AnmController *ctrl = mAnmCtrl;
    ctrl->mTrans = new AnmControlTrans();
    ctrl->mTrans->initAnm(1, &mModel);
    mAnmCtrl->mTrans->registration(0, sMareWBckAnmTrans, sMareWBckMtxCalc);
    mAnmCtrl->mTrans->getObjAnt(0)->mFrameCtrl.setAttribute(2);
}

void TMapObjMareWBase::calc()
{
    TMapObjDemoObj::calc();
}

TMapObjMareM_A::TMapObjMareM_A(const CrsData::SObject &obj) : TMapObjDemoObj(obj)
{
    NewAnmCtrl();
}

TMapObjMareM_A::~TMapObjMareM_A() {}

void TMapObjMareM_A::reset()
{
    TMapObjDemoObj::reset();
    mAnmCtrl->Reset();
    SiUtil::setRandomStartFrame(mAnmCtrl->mTrans->getObjAnt(0)->getFrameCtrl(), getGeoRnd());
}

void TMapObjMareM_A::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/mareM_a.bck");
    J3DAnmObjTrans::setupTransAnmData(&sMareMBckAnmTrans, &sMareMBckMtxCalc, modelData, bck);

    for (u16 i = 0; i < modelData->getShapeNum(); i++)
        modelData->getShapeNodePointer(i)->setTexMtxLoadType(0x2000);
}

void TMapObjMareM_A::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x1000200, true);

    AnmController *ctrl = mAnmCtrl;
    ctrl->mTrans = new AnmControlTrans();
    ctrl->mTrans->initAnm(1, &mModel);
    mAnmCtrl->mTrans->registration(0, sMareMBckAnmTrans, sMareMBckMtxCalc);
    mAnmCtrl->mTrans->getObjAnt(0)->mFrameCtrl.setAttribute(2);
}

void TMapObjMareM_A::calc()
{
    TMapObjDemoObj::calc();
}

__declspec(weak) const char *TMapObjMareM_A::getShadowBmdFileName()
{
    return "/Objects/mareL_a_sh.bmd";
}

__declspec(weak) const char *TMapObjMareM_A::getBmdFileName()
{
    return "/Objects/mareM_a.bmd";
}
