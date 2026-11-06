#include "Shiraiwa/Objects/MapObjWluArrow.h"
#include "Sato/AnmController.h"
#include "Sato/ObjUtility.h"

J3DAnmTextureSRTKey *TMapObjWlArrow::sWlArrowBtkAnm;
J3DAnmTransform *TMapObjWlArrow::sWlArrowBckAnmTrans;
J3DMtxCalc *TMapObjWlArrow::sWlArrowBckMtxCalc;

#include <JSystem/JAudio/JASFakeMatch2.h>

TMapObjWlArrow::TMapObjWlArrow(const CrsData::SObject &obj) : TMapObjHioNode(obj)
{
    NewAnmCtrl();
}

TMapObjWlArrow::~TMapObjWlArrow() {}

const char *TMapObjWlArrow::getBmdFileName()
{
    static const char *cBmdName = "/Objects/Wlarrow1.bmd";
    return cBmdName;
}

void TMapObjWlArrow::reset()
{
    resetObject();
    clrObjFlagHidding();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    mAnmObj.mFrameCtrl.reset();
    mAnmCtrl->Reset();
}

void TMapObjWlArrow::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x200, false);
    mAnmObj.setExModel(&mModel);
    mAnmObj.attach(sWlArrowBtkAnm);
    {
        AnmController *anmCtrl = mAnmCtrl;
        anmCtrl->InitRegistration(1, &mModel);
    }
    mAnmCtrl->RegisterTrans(0, sWlArrowBckAnmTrans, sWlArrowBckMtxCalc);
    mAnmCtrl->getFrameCtrl(0)->setAttribute(2);
}

void TMapObjWlArrow::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/Wlarrow1.bck");
    J3DAnmObjTrans::setupTransAnmData(&sWlArrowBckAnmTrans, &sWlArrowBckMtxCalc, modelData, bck);
    void *btk = ObjUtility::getPtrCourseArc("/Objects/Wlarrow1.btk");
    J3DAnmObjMaterial::setupTexSRTAnmData(&sWlArrowBtkAnm, modelData, btk);
}

void TMapObjWlArrow::calc()
{
    mAnmObj.mFrameCtrl.update();
}

void TMapObjWlArrow::update()
{
    mAnmObj.anmFrameProc();
    GeographyObj::update();
}

J3DAnmTransform *TMapObjWluSearchLight::sWlScLightBckAnmTrans;
J3DMtxCalc *TMapObjWluSearchLight::sWlScLightBckMtxCalc;

TMapObjWluSearchLight::TMapObjWluSearchLight(const CrsData::SObject &obj) : TMapObjHioNode(obj)
{
    NewAnmCtrl();
}

TMapObjWluSearchLight::~TMapObjWluSearchLight() {}

const char *TMapObjWluSearchLight::getBmdFileName()
{
    static const char *cBmdName = "/Objects/Wa_Search1.bmd";
    return cBmdName;
}

void TMapObjWluSearchLight::reset()
{
    resetObject();
    clrObjFlagHidding();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    mAnmCtrl->Reset();
}

void TMapObjWluSearchLight::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createModel(heap, viewNo, 0);
    {
        AnmController *anmCtrl = mAnmCtrl;
        anmCtrl->InitRegistration(1, &mModel);
    }
    mAnmCtrl->RegisterTrans(0, sWlScLightBckAnmTrans, sWlScLightBckMtxCalc);
    mAnmCtrl->getFrameCtrl(0)->setAttribute(2);
}

void TMapObjWluSearchLight::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/Wa_Search1.bck");
    J3DAnmObjTrans::setupTransAnmData(&sWlScLightBckAnmTrans, &sWlScLightBckMtxCalc, modelData, bck);
}
