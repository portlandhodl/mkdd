#include "Kaneshige/Course/CrsData.h"
#include "Osako/ResMgr.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Sato/Objects/MapObjDanceMonte.h"
#include "Sato/Objects/MapObjMonte.h"

static const f32 sDummy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

TAnmInfo TMapObjMonteBase::sAnmInfos[2] = {
    { "/Objects/monL_a_bye1.bck", nullptr, nullptr, 2, 10, 0, 1, 0 },
    { "/Objects/monL_a_clap1.bck", nullptr, nullptr, 2, 10, 0, 0, 0 },
};

StateObserver::StateFuncSet<TMapObjMonteBase> TMapObjMonteBase::sTable[2] = {
    { 0, &TMapObjMonteBase::initFunc_Banzai, &TMapObjMonteBase::doFunc_Banzai },
    { 1, &TMapObjMonteBase::initFunc_Clapping, &TMapObjMonteBase::doFunc_Clapping },
};

const s8 TMapObjMonteBase::sAnmTable[2] = { 0, 1 };

#include "JSystem/JAudio/JASFakeMatch2.h"

J3DAnmTransform *TMapObjUkleleMonte::sUkMonteBckAnmTrans;
J3DMtxCalc *TMapObjUkleleMonte::sUkMonteBckMtxCalc;

TMapObjMonteBase::TMapObjMonteBase(const CrsData::SObject &obj) : TMapObjDemoObj(obj), StateObserver()
{
    NewAnmCtrl();
    mAnmPlayer.resetAnimations(sAnmInfos, 2);
}

TMapObjMonteBase::~TMapObjMonteBase() {}

void TMapObjMonteBase::reset()
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

void TMapObjMonteBase::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    mAnmPlayer.loadAnimations(sAnmInfos, 2, modelData, ResMgr::mcArcCourse);

    for (u16 i = 0; i < modelData->getShapeNum(); i++)
        modelData->getShapeNodePointer(i)->setTexMtxLoadType(0x2000);
}

void TMapObjMonteBase::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x1000200, true);
    mAnmPlayer.registAnimations(mAnmCtrl, &mModel, sAnmInfos, 2);
}

void TMapObjMonteBase::InitExec()
{
    const StateFuncSet<TMapObjMonteBase> *func = FindStateFunc(getState(), sTable, 2);
    (this->*func->mInitFunc)();
}

void TMapObjMonteBase::changeAllState(u16 state)
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

void TMapObjMonteBase::MoveExec()
{
    const StateFuncSet<TMapObjMonteBase> *func = FindStateFunc(getState(), sTable, 2);
    (this->*func->mExecFunc)();
}

void TMapObjMonteBase::calc()
{
    TMapObjDemoObj::calc();
    mAnmPlayer.update();
    StateObserver::ExecuteState();
}

void TMapObjMonteBase::initFunc_Clapping() {}

void TMapObjMonteBase::doFunc_Clapping()
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

void TMapObjMonteBase::initFunc_Banzai() {}

void TMapObjMonteBase::doFunc_Banzai()
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

TMapObjUkleleMonte::TMapObjUkleleMonte(const CrsData::SObject &obj) : TMapObjDemoObj(obj)
{
    NewAnmCtrl();
}

TMapObjUkleleMonte::~TMapObjUkleleMonte() {}

void TMapObjUkleleMonte::reset()
{
    TMapObjDemoObj::reset();
    mAnmCtrl->Reset();
    SiUtil::setRandomStartFrame(mAnmCtrl->mTrans->getObjAnt(0)->getFrameCtrl(), getGeoRnd());
}

void TMapObjUkleleMonte::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/uklele_monte.bck");
    J3DAnmObjTrans::setupTransAnmData(&sUkMonteBckAnmTrans, &sUkMonteBckMtxCalc, modelData, bck);

    for (u16 i = 0; i < modelData->getShapeNum(); i++)
        modelData->getShapeNodePointer(i)->setTexMtxLoadType(0x2000);
}

void TMapObjUkleleMonte::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x1000200, true);

    AnmController *ctrl = mAnmCtrl;
    ctrl->mTrans = new AnmControlTrans();
    ctrl->mTrans->initAnm(1, &mModel);
    mAnmCtrl->mTrans->registration(0, sUkMonteBckAnmTrans, sUkMonteBckMtxCalc);
    mAnmCtrl->mTrans->getObjAnt(0)->mFrameCtrl.setAttribute(2);
}

void TMapObjUkleleMonte::calc()
{
    TMapObjDemoObj::calc();
}

TAnmInfo TMapObjDanceMonte::sAnmInfos[2] = {
    { "/Objects/monF_a_dance.bck", nullptr, nullptr, 2, 0, 0, 0, 0 },
    { "/Objects/monF_a_fladance.bck", nullptr, nullptr, 2, 0, 0, 1, 0 },
};

TMapObjDanceMonte::TMapObjDanceMonte(const CrsData::SObject &obj) : TMapObjDemoObj(obj)
{
    NewAnmCtrl();
    mAnmPlayer.resetAnimations(sAnmInfos, 2);
}

TMapObjDanceMonte::~TMapObjDanceMonte() {}

void TMapObjDanceMonte::reset()
{
    TMapObjDemoObj::reset();
    mAnmCtrl->Reset();
    mAnmPlayer.init(mAnmCtrl, sAnmInfos, 2);

#line 305
    JUT_MINMAX_ASSERT(0, mObjData->mParam5, 2);

    s8 anmNo = mObjData->mParam5;
    mAnmPlayer._11 = mObjData->mParam5;
    mAnmPlayer._10 = 1;
    mAnmPlayer._e |= 1;

    if (mObjData->mParam6 != 0)
        SiUtil::setRandomStartFrame(mAnmCtrl->mTrans->getFrameCtrl(anmNo), getGeoRnd());
}

void TMapObjDanceMonte::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    mAnmPlayer.loadAnimations(sAnmInfos, 2, modelData, ResMgr::mcArcCourse);

    for (u16 i = 0; i < modelData->getShapeNum(); i++)
        modelData->getShapeNodePointer(i)->setTexMtxLoadType(0x2000);
}

void TMapObjDanceMonte::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x1000200, true);
    mAnmPlayer.registAnimations(mAnmCtrl, &mModel, sAnmInfos, 2);
}

void TMapObjDanceMonte::calc()
{
    TMapObjDemoObj::calc();
    mAnmPlayer.update();
}

__declspec(weak) const char *TMapObjUkleleMonte::getShadowBmdFileName()
{
    return "/Objects/monL_a_sh.bmd";
}

__declspec(weak) const char *TMapObjUkleleMonte::getBmdFileName()
{
    return "/Objects/uklele_monte.bmd";
}

#include "JSystem/JAudio/JASFakeMatch2.h"
