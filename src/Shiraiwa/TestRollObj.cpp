#include "Shiraiwa/TestRollObj.h"
#include "Sato/ObjUtility.h"


f32 TTestRollObj::sRotSpeed = 0.95f;

TTestRollObj::TTestRollObj(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
    mFreeFall = new TFreeFallShakeSky(&mPos, &mVel, &mRotMtx);
}

TTestRollObj::~TTestRollObj() { delete mFreeFall; }

const char *TTestRollObj::getBmdFileName() {
    static const char *cBmdName = "/Objects/TestPaipo1.bmd";
    return cBmdName;
}

void TTestRollObj::reset() {
    GeographyObj::resetObject();
    _58 = 4;
    mFreeFall->reset();
    mFreeFall->_18 = 50.0f * mScale.y;
    mFreeFall->_14 = sRotSpeed / mScale.y;
    mFreeFall->_2C = getGeoRnd();
    mFreeFall->_1C = 1;
}

void TTestRollObj::calc() {
    mFreeFall->update();
}

void TTestRollObj::createColModel(J3DModelData *mdlData) {
    createBoundsSphere(mdlData);
}

void TTestRollObj::doKartColCallBack(int kartNo) {
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(kartNo, &kartPos);

    JGeometry::TVec3f dir;
    dir.sub(mPos, kartPos);
    dir.normalize();
    dir.y = 0.5f;
    dir.scale(1.5f);
    mVel.add(dir);
}

#include <JSystem/JAudio/JASFakeMatch2.h>
