#include "Shiraiwa/Objects/MapObjVentilator.h"

#include <JSystem/JGeometry.h>

#include "Inagaki/GameSoundMgr.h"
#include "Kaneshige/RaceMgr.h"

extern void setSeWithVelocityP__Q29GameAudio14ObjectSoundMgrFUlf(GameAudio::ObjectSoundMgr *, u32, f32);
extern void ExecuteState__13StateObserverFv(StateObserver *);

StateObserver::StateFuncSet<TMapObjVentilator> TMapObjVentilator::sTable[4] = {
    { 0, &TMapObjVentilator::initFunc_Slow, &TMapObjVentilator::doFunc_Slow },
    { 1, &TMapObjVentilator::initFunc_Slow2Fast, &TMapObjVentilator::doFunc_Slow2Fast },
    { 2, &TMapObjVentilator::initFunc_Fast, &TMapObjVentilator::doFunc_Fast },
    { 3, &TMapObjVentilator::initFunc_Fast2Slow, &TMapObjVentilator::doFunc_Fast2Slow },
};

TMapObjVentilator::TMapObjVentilator(const CrsData::SObject &obj) : TMapObjHioNode(obj), StateObserver() {
    mObjFlag |= 1;
    createSoundMgr();
}

TMapObjVentilator::~TMapObjVentilator() {}

const char *TMapObjVentilator::getBmdFileName() {
    static const char *cBmdName = "/ObjFcts/Fan1.bmd";
    return cBmdName;
}

void TMapObjVentilator::InitExec() {
    Observer_FindAndInit(TMapObjVentilator, 4);
}

void TMapObjVentilator::MoveExec() {
    Observer_FindAndExec(TMapObjVentilator, 4);
}

void TMapObjVentilator::reset() {
    GeographyObj::resetObject();
    clrObjFlagHidding();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();

    mCurQuat.set(0.0f, 0.0f, 0.0f, 1.0f);
    mRotMtx.zeroTrans();
    mRotMtx.setQuat(mCurQuat);

    mFastQuat.set(0.0f, 0.0f, 0.0f, 1.0f);
    register f32 fastSin = (f32)sin(0.05);
    register f32 fastCos = (f32)cos(0.05);
    mFastQuat.set<f32>(fastSin, 0.0f, 0.0f, fastCos);

    mSlowQuat.set(0.0f, 0.0f, 0.0f, 1.0f);
    register f32 slowSin = (f32)sin(0.015);
    register f32 slowCos = (f32)cos(0.015);
    mSlowQuat.set<f32>(slowSin, 0.0f, 0.0f, slowCos);
}

static const f32 cVentCalc03 = 0.3f;
static const f32 cVentCalc07 = 0.7f;
static const f32 cVentCalc10 = 1.0f;

void TMapObjVentilator::calc() {
    __asm__ volatile {
        mr r31,r3
        addi r3,r31,332
        bl ExecuteState__13StateObserverFv
        lhz r0,338(r31)
        lfs f1, cVentCalc03
        cmpwi r0,2
        beq lcase2
        bge lge
        cmpwi r0,0
        beq lend
        bge lcase1
        b lend
    lge:
        cmpwi r0,4
        bge lend
        b lcase3
    lcase2:
        lfs f1, cVentCalc10
        b lend
    lcase3:
        lfs f1, cVentCalc10
        lfs f0,392(r31)
        lfs f2, cVentCalc07
        fsubs f0,f1,f0
        fmuls f1,f2,f0
        b lend
    lcase1:
        lfs f1, cVentCalc07
        lfs f0,392(r31)
        fmuls f1,f1,f0
    lend:
        lis r4,4
        lwz r3,324(r31)
        addi r4,r4,127
        bl setSeWithVelocityP__Q29GameAudio14ObjectSoundMgrFUlf
    }
}

void TMapObjVentilator::doRotate(const JGeometry::TQuat4f &rQ) {
    PSQUATMultiply(&mCurQuat, &rQ, &mCurQuat);

    f32 sq = mCurQuat.squared();
    if (sq <= JGeometry::TUtilf::epsilon()) {
        mCurQuat.set(0.0f, 0.0f, 0.0f, 1.0f);
    } else {
        f32 s = JGeometry::TUtilf::inv_sqrt(sq);
        mCurQuat.scale(s);
    }

    mRotMtx.setQuat(mCurQuat);
}

void TMapObjVentilator::initFunc_Slow() {}

void TMapObjVentilator::doFunc_Slow() {
    doRotate(mSlowQuat);
    if (getStateCount() > 300) {
        setState(1);
    }
}

void TMapObjVentilator::initFunc_Slow2Fast() {
    mSpeed = 0.0f;
}

void TMapObjVentilator::doFunc_Slow2Fast() {
    JGeometry::TQuat4f q;
    q.x = mSlowQuat.x + mSpeed * (mFastQuat.x - mSlowQuat.x);
    q.y = mSlowQuat.y + mSpeed * (mFastQuat.y - mSlowQuat.y);
    q.z = mSlowQuat.z + mSpeed * (mFastQuat.z - mSlowQuat.z);
    q.w = mSlowQuat.w + mSpeed * (mFastQuat.w - mSlowQuat.w);

    f32 sq = q.squared();
    if (sq <= JGeometry::TUtilf::epsilon()) {
        q.set(0.0f, 0.0f, 0.0f, 1.0f);
    } else {
        f32 s = JGeometry::TUtilf::inv_sqrt(sq);
        q.scale(s);
    }

    doRotate(q);
    mSpeed += 0.01f;
    if (mSpeed >= 1.0f) {
        setState(2);
    }
}

void TMapObjVentilator::initFunc_Fast() {}

void TMapObjVentilator::doFunc_Fast() {
    doRotate(mFastQuat);
    if (getStateCount() > 900) {
        setState(3);
    }
}

void TMapObjVentilator::initFunc_Fast2Slow() {
    mSpeed = 0.0f;
}

void TMapObjVentilator::doFunc_Fast2Slow() {
    JGeometry::TQuat4f q;
    q.x = mFastQuat.x + mSpeed * (mSlowQuat.x - mFastQuat.x);
    q.y = mFastQuat.y + mSpeed * (mSlowQuat.y - mFastQuat.y);
    q.z = mFastQuat.z + mSpeed * (mSlowQuat.z - mFastQuat.z);
    q.w = mFastQuat.w + mSpeed * (mSlowQuat.w - mFastQuat.w);

    f32 sq = q.squared();
    if (sq <= JGeometry::TUtilf::epsilon()) {
        q.set(0.0f, 0.0f, 0.0f, 1.0f);
    } else {
        f32 s = JGeometry::TUtilf::inv_sqrt(sq);
        q.scale(s);
    }

    doRotate(q);
    mSpeed += 0.01f;
    if (mSpeed >= 1.0f) {
        setState(0);
    }
}

#include <JSystem/JAudio/JASFakeMatch2.h>
