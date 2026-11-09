#include "Sato/Objects/MapObjNossie.h"

#include "Inagaki/GameSoundMgr.h"
#include "Kaneshige/RaceMgr.h"

const char *TMapObjNossie::getBmdFileName() {
    static s8 init;
    static const char *cBmdName;
    if (!init) {
        cBmdName = "/Objects/Nossie.bmd";
        init = 1;
    }
    return cBmdName;
}

bool TMapObjNossie::isTimeAttack() {
    return RaceMgr::getManager()->mRaceInfo->mRaceMode == TIME_ATTACK;
}

void TMapObjNossieColBase::reset() {
    resetObject();
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    _58 = getReaction();
}

__declspec(weak) u32 TMapObjNossieColBase::getReaction() {
    return 3;
}

void TMapObjNossieColBase::calc() {}

void TMapObjNossieColHead::createColModel(J3DModelData *) {
    createBoundsSphere(1500.0f, mScale.y);
}

void TMapObjNossieColHead::calc() {
    getSoundMgr()->setSe(0x40063);
}

void TMapObjNossieColBody::createColModel(J3DModelData *) {
    createBoundsCylinder(1900.0f, 500.0f, mScale.y);
}

u32 TMapObjNossieColFoot::getReleaseFrame() {
    return 30;
}

void TMapObjNossieColFoot::createColModel(J3DModelData *) {
    createBoundsCylinder(1800.0f, 32.0f, 1500.0f);
}

void TMapObjNossieColFoot::getHitFlag(u8 *flags) {
    for (int i = 0; i < 8; i++) {
        bool hit = (mIsHitKartFlags & (1 << i)) != 0;
        *flags |= hit << i;
    }
}

__declspec(weak) const char *TMapObjPtera::getBmdFileName() {
    return "/Objects/Ptera.bmd";
}

__declspec(weak) const char *TMapObjSwimNossie::getBmdFileName() {
    return "/Objects/SwimNossie.bmd";
}
