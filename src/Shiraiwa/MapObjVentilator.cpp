#include "Shiraiwa/Objects/MapObjVentilator.h"

#include <JSystem/JGeometry.h>

#include "Inagaki/GameSoundMgr.h"
#include "Kaneshige/RaceMgr.h"


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
    static const char *cBmdName = "/Objects/Fan1.bmd";
    return cBmdName;
}

void TMapObjVentilator::InitExec() {
    Observer_FindAndInit(TMapObjVentilator, 4);
}

void TMapObjVentilator::MoveExec() {
    Observer_FindAndExec(TMapObjVentilator, 4);
}

static inline void setRotQuat(JGeometry::TQuat4f &q, f32 halfAngle)
{
    f32 s = sin(halfAngle);
    f32 c = cos(halfAngle);
    q.set<f32>(s, 0.0f, 0.0f, c);
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
    setRotQuat(mFastQuat, 0.05f);

    mSlowQuat.set(0.0f, 0.0f, 0.0f, 1.0f);
    setRotQuat(mSlowQuat, 0.015f);
}


void TMapObjVentilator::calc() {
    ExecuteState();
    f32 vel = 0.3f;
    switch (getState()) {
    case 0:
        break;
    case 2:
        vel = 1.0f;
        break;
    case 3:
        vel = 0.7f * (1.0f - mSpeed);
        break;
    case 1:
        vel = 0.7f * mSpeed;
        break;
    }
    mSoundMgr->setSeWithVelocityP(0x4007f, vel);
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
