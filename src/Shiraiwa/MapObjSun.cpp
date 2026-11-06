#include "Shiraiwa/Objects/MapObjSun.h"

#include "JSystem/JAudio/JASFakeMatch2.h"
#include "Sato/ObjUtility.h"

J3DAnmTextureSRTKey *TMapObjSun::sSunBtkAnm;

TMapObjSun::TMapObjSun(const CrsData::SObject &obj) : TMapObjHioNode(obj) {}

TMapObjSun::~TMapObjSun() {}

const char *TMapObjSun::getBmdFileName() {
    static const char *cBmdName = "/Objects/sun.bmd";
    return cBmdName;
}

void TMapObjSun::reset() {
    GeographyObj::resetObject();
    clrObjFlagHidding();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();

    mAnmObj.mFrameCtrl.mFrame = mAnmObj.mFrameCtrl.mStartFrame;
    mAnmObj.mFrameCtrl.mRate = 1.0f;
    mAnmObj.mFrameCtrl.mState = 0;
}

void TMapObjSun::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createDifferedModel(heap, viewNo, 0x200, false);
    mAnmObj.mModel = &mModel;
    mAnmObj.mAnmBase = sSunBtkAnm;
    mAnmObj.initFrameCtrl(mAnmObj.mAnmBase);
}

void TMapObjSun::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *btk = ObjUtility::getPtrCourseArc("/Objects/sun.btk");
    J3DAnmObjMaterial::setupTexSRTAnmData(&sSunBtkAnm, modelData, btk);
}

void TMapObjSun::calc() {
    mAnmObj.mFrameCtrl.update();
}

void TMapObjSun::update() {
    mAnmObj.anmFrameProc();
    GeographyObj::update();
}
