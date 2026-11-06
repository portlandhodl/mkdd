#include "Shiraiwa/Objects/MapObjFountain.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"

#include <JSystem/JAudio/JASFakeMatch2.h>

J3DAnmTextureSRTKey *TMapObjFountain::sFountainBtkAnm;

TMapObjFountain::TMapObjFountain(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
    setObjFlagSimpleDraw();
    GeographyObj::createSoundMgr();
}

TMapObjFountain::~TMapObjFountain() {}

const char *TMapObjFountain::getBmdFileName() {
    static const char *cBmdName = "/Objects/PeachFountain.bmd";
    return cBmdName;
}

void TMapObjFountain::reset() {
    GeographyObj::reset();
    GeographyObj::clrObjFlagHidding();
    GeographyObj::clrObjFlagCheckItemHitting();
    GeographyObj::clrAllCheckKartHitFlag();
    mBtk.resetFrame();
}

void TMapObjFountain::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mBtk.setExModel(&mModel);
    mBtk.attach(sFountainBtkAnm);
}

void TMapObjFountain::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *btk = ObjUtility::getPtrCourseArc("/Objects/PeachFountain.btk");
    J3DAnmObjMaterial::setupTexSRTAnmData(&sFountainBtkAnm, modelData, btk);
}

void TMapObjFountain::calc() {
    mBtk.update();
    GeographyObj::getSoundMgr()->setSe(0x40048);
}

void TMapObjFountain::update() {
    mBtk.anmFrameProc();
    GeographyObj::update();
}

J3DAnmTextureSRTKey *TMapObjDemoFountain::sDemoFountainBtkAnm;

TMapObjDemoFountain::TMapObjDemoFountain(const CrsData::SObject &obj) : TMapObjFountain(obj) {
    setObjFlagSimpleDraw();
}

TMapObjDemoFountain::~TMapObjDemoFountain() {}

void TMapObjDemoFountain::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mBtk.setExModel(&mModel);
    mBtk.attach(sDemoFountainBtkAnm);
}

void TMapObjDemoFountain::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *btk = ObjUtility::getPtrCourseArc("/Objects/DPeachFountain.btk");
    J3DAnmObjMaterial::setupTexSRTAnmData(&sDemoFountainBtkAnm, modelData, btk);
}

void TMapObjDemoFountain::reset() {
    TMapObjFountain::reset();
    _16c = true;
}

void TMapObjDemoFountain::calc() {
    if (_16c) {
        GetJPAMgr()->createEmt("mk_fountainWtr_a", mPos);
        _16c = false;
    }
    mBtk.update();
    GeographyObj::getSoundMgr()->setSe(0x40082);
}

