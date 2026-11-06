#include "Shiraiwa/Objects/MapObjDesun.h"
#include "JSystem/JMath/JMath.h"
#include "Sato/AnmController.h"
#include "Sato/ObjUtility.h"

J3DAnmTransform *TMapObjDesun::sDesunBckAnmTrans;
J3DMtxCalc *TMapObjDesun::sDesunBckMtxCalc;

TMapObjDesun::TMapObjDesun(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
    setObjFlagSimpleDraw();
    GeographyObj::NewAnmCtrl();
}

TMapObjDesun::~TMapObjDesun() {}

const char *TMapObjDesun::getBmdFileName() {
    static const char *cBmdName = "/Objects/desun1.bmd";
    return cBmdName;
}

void TMapObjDesun::reset() {
    GeographyObj::reset();
    GeographyObj::clrObjFlagHidding();
    GeographyObj::clrObjFlagCheckItemHitting();
    GeographyObj::clrAllCheckKartHitFlag();
    GeographyObj::getAnmCtrl()->Reset();
}

void TMapObjDesun::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/desun1.bck");
    J3DAnmObjTrans::setupTransAnmData(&sDesunBckAnmTrans, &sDesunBckMtxCalc, modelData, bck);
}

void TMapObjDesun::createModel(JKRSolidHeap *heap, u32 viewNo) {
    GeographyObj::getAnmCtrl()->InitRegistration(1, &mModel);
    GeographyObj::getAnmCtrl()->RegisterTrans(0, sDesunBckAnmTrans, sDesunBckMtxCalc);
    GeographyObj::getAnmCtrl()->getFrameCtrl(0)->setAttribute(2);
}

void TMapObjDesun::calc() {}

J3DAnmTexPattern *TMapObjDeBalloon::sDeBalloonBtpAnm;
f32 TMapObjDeBalloon::sFloatHeight = 2000.0f;
s16 TMapObjDeBalloon::sFloatSpeed = 100;
s16 TMapObjDeBalloon::scFloatStartDeg;

struct TMapObjDeBalloonInit {
    TMapObjDeBalloonInit() { *(u16 *)&TMapObjDeBalloon::scFloatStartDeg = 0xC000; }
};
static TMapObjDeBalloonInit sDeBalloonInit;

#include <JSystem/JAudio/JASFakeMatch2.h>

TMapObjDeBalloon::TMapObjDeBalloon(const CrsData::SObject &obj) : TMapObjHioNode(obj) {}

TMapObjDeBalloon::~TMapObjDeBalloon() {}

void TMapObjDeBalloon::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createDifferedModel(heap, viewNo, 0x20000, false);
    mBtp.setExModel(&mModel);
    mBtp.attach(sDeBalloonBtpAnm);
}

void TMapObjDeBalloon::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *btp = ObjUtility::getPtrCourseArc("/Objects/DeBalloon1.btp");
    J3DAnmObjMaterial::setupTexPatternAnmData(&sDeBalloonBtpAnm, modelData, btp);
}

void TMapObjDeBalloon::update() {
    mBtp.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}

void TMapObjDeBalloon::reset() {
    GeographyObj::reset();
    GeographyObj::clrObjFlagCheckItemHitting();
    GeographyObj::clrAllCheckKartHitFlag();

    mBtp.resetFrame();

    u16 start = mObjData->mParam1;
    if (start > mBtp.getFrameCtrl()->getEnd()) {
        start = 0;
    }
    s16 angle = scFloatStartDeg;
    mBtp.setFrame(start);
    mAngle = angle;
}

void TMapObjDeBalloon::calc() {
    const JGeometry::TVec3f &pos = mObjData->position;
    mPos.x = pos.x;
    mPos.y = pos.y;
    mPos.z = pos.z;

    f32 sin = JMath::sincosTable_.sinShort(mAngle);
    f32 offset = 0.5f * (1.0f + sin);
    mPos.y += sFloatHeight * offset;
    mAngle += sFloatSpeed;
}

