#include "Shiraiwa/Objects/MapObjSnowMan.h"
#include "Sato/ObjUtility.h"

#include <JSystem/JAudio/JASFakeMatch2.h>

J3DDeformData *TMapObjSnowMan::sSnowManBlsAnmData;
J3DAnmCluster *TMapObjSnowMan::sSnowManBlkAnmData;

TMapObjSnowMan::TMapObjSnowMan(const CrsData::SObject &obj) : TMapObjHioNode(obj) {}

TMapObjSnowMan::~TMapObjSnowMan() {}

const char *TMapObjSnowMan::getBmdFileName() {
    static const char *cBmdName = "/Objects/Snowman1.bmd";
    return cBmdName;
}

void TMapObjSnowMan::reset() {
    GeographyObj::resetObject();
    clrObjFlagHidding();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();

    mAnmObj.mFrameCtrl.mFrame = mAnmObj.mFrameCtrl.mStartFrame;
    mAnmObj.mFrameCtrl.mRate = 1.0f;
    mAnmObj.mFrameCtrl.mState = 0;
}

void TMapObjSnowMan::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createModel(heap, viewNo, 0);
    J3DAnmObjCluster::setDeformData(&mModel, sSnowManBlsAnmData, false);
    mAnmObj.setExModel(&mModel, sSnowManBlsAnmData);
    mAnmObj.attach(sSnowManBlkAnmData);
    mAnmObj.mFrameCtrl.setAttribute(2);
}

void TMapObjSnowMan::loadAnimation() {
    void *bls = ObjUtility::getPtrCourseArc("/Objects/Snowman1.bls");
    J3DAnmObjCluster::loadClusterData(&sSnowManBlsAnmData, bls);
    void *blk = ObjUtility::getPtrCourseArc("/Objects/Snowman1.blk");
    J3DAnmObjCluster::loadClusterAnmData(&sSnowManBlkAnmData, blk);
}

void TMapObjSnowMan::calc() {}

void TMapObjSnowMan::update() {
    mAnmObj.update();
    GeographyObj::update();
}
