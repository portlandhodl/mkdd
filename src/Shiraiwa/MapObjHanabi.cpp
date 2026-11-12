#include "macros.h"
#include "Shiraiwa/Objects/MapObjHanabi.h"
#include "Kameda/MotorManager.h"
#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/RaceInfo.h"
#include "Sato/JPEffectMgr.h"

#include <JSystem/JAudio/JASFakeMatch2.h>

THanabiSupervisor *TMapObjHanabi::sSupervisor;

static const f32 sDummy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

TMapObjHanabi::TMapObjHanabi(const CrsData::SObject &object)
    : TMapObjHioNode(object), JKRDisposer(), mEntryLink(this) {
    if (sSupervisor == nullptr) {
        sSupervisor = new THanabiSupervisor();
    }
    sSupervisor->entry(this);
    createSoundMgr();
}

void TMapObjHanabi::reset() {
    resetObject();
    clrObjFlagCheckGeoHitting();
    clrObjFlagCheckItemHitting();
    mCheckKartHitFlags = 0;
    mDoFire = false;
}

void TMapObjHanabi::calc() {
    if (mDoFire) {
        GetJPAMgr()->createEmt("mk_hanabi_a", mPos);
        GetJPAMgr()->createEmt("mk_hanabi_b", mPos);
        GetJPAMgr()->createEmt("mk_hanabi_c", mPos);
        if (RaceMgr::sRaceManager->mRaceInfo->mRaceMode == AWARD_DEMO) {
            mSoundMgr->setSe(0x40077);
            MotorManager::setAwardMotor(MotorManager::MotorType_24);
        }
        mDoFire = false;
    }
}

#pragma dont_inline on
THanabiSupervisor::THanabiSupervisor() : GeoObjSupervisor(), mEntryLink() {}
#pragma dont_inline reset

THanabiSupervisor::~THanabiSupervisor() {}

#pragma dont_inline on
void THanabiSupervisor::entry(TMapObjHanabi *obj) {
    mEntryLink.append(&obj->mEntryLink);
}
#pragma dont_inline reset

void THanabiSupervisor::fire(s16 frame) {
    for (JSULink<TMapObjHanabi> *link = mEntryLink.getFirst(); link != nullptr; link = link->getNext()) {
        TMapObjHanabi *obj = link->getObject();
        obj->mDoFire = (obj->mObjData->mParam1 == frame);
    }
}

s16 THanabiSupervisor::getFireFrame(s16 frame) {
    JSULink<TMapObjHanabi> *link = mEntryLink.getFirst();
    s16 ret = -1;
    while (link != nullptr) {
        TMapObjHanabi *obj = link->getObject();
        if (frame == obj->mObjData->mParam1) {
            ret = obj->mObjData->mParam2;
            break;
        }
        link = link->getNext();
    }
    return ret;
}
