#include "Shiraiwa/Objects/MapObjKpGear.h"

#include "JSystem/JAudio/JASFakeMatch2.h"
#include "Sato/AnmController.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/ObjUtility.h"

J3DAnmTransform *TMapObjKpGear::sKpGearBckAnmTrans;
J3DMtxCalc *TMapObjKpGear::sKpGearBckMtxCalc;

TMapObjKpGear::TMapObjKpGear(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
    GeographyObj::NewAnmCtrl();
}

TMapObjKpGear::~TMapObjKpGear() {}

const char *TMapObjKpGear::getBmdFileName() {
    static const char *cBmdName = "/Objects/KpGear1.bmd";
    return cBmdName;
}

void TMapObjKpGear::reset() {
    GeographyObj::resetObject();
    clrObjFlagHidding();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    getAnmCtrl()->Reset();
}

void TMapObjKpGear::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/KpGear1.bck");
    J3DAnmObjTrans::setupTransAnmData(&sKpGearBckAnmTrans, &sKpGearBckMtxCalc, modelData, bck);
}

void TMapObjKpGear::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createModel(heap, viewNo, 0);
    AnmController *anmCtrl = getAnmCtrl();
    anmCtrl->mTrans = new AnmControlTrans();
    anmCtrl->mTrans->initAnm(1, &mModel);
    getAnmCtrl()->mTrans->registration(0, sKpGearBckAnmTrans, sKpGearBckMtxCalc);
    getAnmCtrl()->getFrameCtrl(0)->setAttribute(2);
}

void TMapObjKpGear::calc() {}
