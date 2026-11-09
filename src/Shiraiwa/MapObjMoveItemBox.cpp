#include "Shiraiwa/Objects/MapObjMoveItemBox.h"

#include "Kaneshige/Objects/GeoItemCar.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Shiraiwa/SiUtil.h"

f32 TMapObjMoveItemBox::sHeight = 115.0f;
f32 TMapObjMoveItemBox::sRollSpeed = 0.43f;
s16 TMapObjMoveItemBox::sAppearFrame = 5;
s16 TMapObjMoveItemBox::sDisappearFrame = 5;
f32 TMapObjMoveItemBox::sGravitySize = 1.0f;
f32 TMapObjMoveItemBox::sReflectRate = 0.45f;
f32 TMapObjMoveItemBox::sAirFriction = 0.998f;
f32 TMapObjMoveItemBox::sFriction = 0.989f;
f32 TMapObjMoveItemBox::sJumpPow = 10.0f;
f32 TMapObjMoveItemBoxLimit::sGravitySize = 1.5f;

StateObserver::StateFuncSet<TMapObjMoveItemBox> TMapObjMoveItemBox::sTable[5] = {
    {0, &TMapObjMoveItemBox::initFunc_Appear, &TMapObjMoveItemBox::doFunc_Appear},
    {1, &TMapObjMoveItemBox::initFunc_Roll, &TMapObjMoveItemBox::doFunc_Roll},
    {2, &TMapObjMoveItemBox::initFunc_Disappear, &TMapObjMoveItemBox::doFunc_Disappear},
    {3, &TMapObjMoveItemBox::initFunc_Hide, &TMapObjMoveItemBox::doFunc_Hide},
    {4, &TMapObjMoveItemBox::initFunc_RaceEnd, &TMapObjMoveItemBox::doFunc_RaceEnd},
};

#include "JSystem/JAudio/JASFakeMatch2.h"

TMapObjMoveItemBox::TMapObjMoveItemBox(const CrsData::SObject &obj) : GeoItemBox(obj), StateObserver() {
    mFreeFall = new TFreeFallShakeSky(&mPos, &mVel, &mRotMtx);
}

TMapObjMoveItemBox::~TMapObjMoveItemBox() {
    delete mFreeFall;
}

void TMapObjMoveItemBox::reset() {
    GeoItemBox::reset();
    resetObject();
    ResetState();
    mGeoObjFlag &= ~0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = -1;
    for (int i = 0; i < 2; i++) {
        mEmitter[i] = nullptr;
    }
    mFreeFall->reset();
    mFreeFall->_18 = sHeight * mScale.y;
    mFreeFall->_14 = sRollSpeed / mScale.y;
    mFreeFall->_0C = sGravitySize;
    mFreeFall->_10 = sReflectRate;
    mFreeFall->_04 = sAirFriction;
    mFreeFall->_08 = sFriction;
    mFreeFall->_38 = 1;
    mFreeFall->_2C = getGeoRnd();
    mFreeFall->_34 = sJumpPow;
    mFreeFall->_1C = 1;
    const CrsData::SObject *data = mObjData;
    mVel.x = data->zNorm.x;
    mVel.y = data->zNorm.y;
    mVel.z = data->zNorm.z;
    mVel.normalize();
}

void TMapObjMoveItemBox::InitExec() {
    const StateObserver::StateFuncSet<TMapObjMoveItemBox> *func =
        FindStateFunc<TMapObjMoveItemBox>(getState(), sTable, 5);
    (this->*func->mInitFunc)();
}

void TMapObjMoveItemBox::MoveExec() {
    const StateObserver::StateFuncSet<TMapObjMoveItemBox> *func =
        FindStateFunc<TMapObjMoveItemBox>(getState(), sTable, 5);
    (this->*func->mExecFunc)();
}

void TMapObjMoveItemBox::calc() {
    doMode();
    ExecuteState();
}

void TMapObjMoveItemBox::move() {
    JGeometry::TVec3f pos;
    pos.x = mPos.x;
    pos.y = mPos.y;
    pos.z = mPos.z;
    if (RaceMgr::getManager()->getRacePhase() != 1) {
        mFreeFall->update();
    }
    moveShadowModel();
    bool land = false;
    if (mPos.y <= sHeight * mScale.y) {
        land = true;
    } else {
        CrsGround ground(RaceMgr::getManager()->getCourse());
        ground.search(pos);
        if (ground.getAttribute() == CrsGround::Attr_10) {
            f32 h = ground.getHeight() + sHeight;
            if (pos.y - h > 0.01f && mPos.y - h < 0.01f) {
                land = true;
            }
        }
    }
    if (land) {
        setState(2);
    }
}

void TMapObjMoveItemBox::initFunc_Appear() {
    PSMTXIdentity(mRotMtx);
    mCheckKartHitFlags = -1;
    mGeoObjFlag &= ~0x20;
    if (mShadowMdl != nullptr) {
        mShadowMdl->setVisibleAll();
    }
    mVel.x = 0.0f;
    mVel.y = 0.0f;
    mVel.z = 0.0f;
    mScale.x = 0.0f;
    mScale.y = 0.0f;
    mScale.z = 0.0f;
    resetPosition();
}

void TMapObjMoveItemBox::resetPosition() {
    const CrsData::SObject *data = mObjData;
    mPos.x = data->position.x;
    mPos.y = data->position.y;
    mPos.z = data->position.z;
}

void TMapObjMoveItemBox::doFunc_Appear() {
    f32 range = SiUtil::getNormalRange((f32)getStateCount(), 0.01f, (f32)sAppearFrame);
    f32 scale = 1.0f + range;
    if (scale >= 0.0f) {
        scale = 0.0f;
        setState(1);
    }
    f32 s = mObjData->scale.y * scale;
    mScale.x = s;
    mScale.y = s;
    mScale.z = s;
    move();
}

void TMapObjMoveItemBox::initFunc_Roll() {}

void TMapObjMoveItemBox::doFunc_Roll() {
    move();
    if ((*(u8 *)((u8 *)RaceMgr::getManager()->mRaceDirector + 0x2a)) != 0) {
        setState(4);
    }
}

void TMapObjMoveItemBox::setOpeningPos() {
    const CrsData::SObject *data = mObjData;
    mPos.x = data->position.x;
    mPos.y = data->position.y;
    mPos.z = data->position.z;
    PSMTXIdentity(mRotMtx);
}

void TMapObjMoveItemBox::initFunc_Disappear() {
    f32 s = mObjData->scale.y;
    mScale.x = s;
    mScale.y = s;
    mScale.z = s;
    mCheckKartHitFlags = 0;
    JPEffectMgr::getEffectMgr()->createEmt("mk_itemget_normal_a", mPos);
}

void TMapObjMoveItemBox::doFunc_Disappear() {
    f32 range = SiUtil::getNormalRange((f32)getStateCount(), 0.01f, (f32)sDisappearFrame);
    f32 zero = 0.0f;
    f32 scale = zero - range;
    if (scale <= 0.01f) {
        scale = zero;
        mGeoObjFlag |= 0x20;
        setState(3);
    }
    f32 s = mObjData->scale.y * scale;
    mScale.x = s;
    mScale.y = s;
    mScale.z = s;
}

void TMapObjMoveItemBox::initFunc_Hide() {
    if (mShadowMdl != nullptr) {
        mShadowMdl->clrVisibleAll();
    }
    mGeoObjFlag |= 0x20;
}

void TMapObjMoveItemBox::doFunc_Hide() {
    if ((*(u8 *)((u8 *)RaceMgr::getManager()->mRaceDirector + 0x2a)) == 0) {
        if (getStateCount() > GeoItemBox::getInactiveTime()) {
            setState(0);
        }
    }
}

void TMapObjMoveItemBox::initFunc_RaceEnd() {
    f32 s = mObjData->scale.y;
    mScale.x = s;
    mScale.y = s;
    mScale.z = s;
    mCheckKartHitFlags = 0;
}

void TMapObjMoveItemBox::doFunc_RaceEnd() {
    f32 range = SiUtil::getNormalRange((f32)getStateCount(), 0.01f, (f32)sDisappearFrame);
    f32 zero = 0.0f;
    f32 scale = zero - range;
    if (scale <= 0.01f) {
        scale = zero;
        mGeoObjFlag |= 0x20;
        setState(3);
    }
    f32 s = mObjData->scale.y * scale;
    mScale.x = s;
    mScale.y = s;
    mScale.z = s;
}

void TMapObjMoveItemBoxLimit::reset() {
    GeoItemBox::reset();
    resetObject();
    ResetState();
    mGeoObjFlag &= ~0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = -1;
    for (int i = 0; i < 2; i++) {
        mEmitter[i] = nullptr;
    }
    mFreeFall->reset();
    mFreeFall->_18 = sHeight * mScale.y;
    mFreeFall->_14 = sRollSpeed / mScale.y;
    mFreeFall->_0C = sGravitySize;
    mFreeFall->_10 = sReflectRate;
    mFreeFall->_04 = sAirFriction;
    mFreeFall->_08 = sFriction;
    mFreeFall->_38 = 1;
    mFreeFall->_2C = getGeoRnd();
    mFreeFall->_34 = sJumpPow;
    mFreeFall->_1C = 1;
    const CrsData::SObject *data = mObjData;
    mVel.x = data->zNorm.x;
    mVel.y = data->zNorm.y;
    mVel.z = data->zNorm.z;
    mVel.normalize();
    if (mObjData->pathID == 0xffff) {
#line 330
        JUT_ASSERT(mObjData->pathID != 0xffff);
        OSPanic("MapObjMoveItemBox.cpp", 330, "MoveItemBoxLimit: Illegal Path ID");
    }
    CrsData::PointData *p0 = RaceMgr::getManager()->getCourse()->getCrsData()->getPointData(0, mObjData->pathID);
    CrsData::PointData *p1 = RaceMgr::getManager()->getCourse()->getCrsData()->getPointData(1, mObjData->pathID);
    mMin.x = p0->pos.x;
    mMin.y = p0->pos.y;
    mMin.z = p0->pos.z;
    if (mMin.x > p1->pos.x) mMin.x = p1->pos.x;
    if (mMin.y > p1->pos.y) mMin.y = p1->pos.y;
    if (mMin.z > p1->pos.z) mMin.z = p1->pos.z;
    mMax.x = p0->pos.x;
    mMax.y = p0->pos.y;
    mMax.z = p0->pos.z;
    if (mMax.x < p1->pos.x) mMax.x = p1->pos.x;
    if (mMax.y < p1->pos.y) mMax.y = p1->pos.y;
    if (mMax.z < p1->pos.z) mMax.z = p1->pos.z;
    f32 g = sGravitySize;
    mFreeFall->_0C = g;
    mFreeFall->_08 = 0.0f;
    mPos.x = p0->pos.x;
    mPos.y = p0->pos.y;
    mPos.z = p0->pos.z;
    mPos.y += sHeight;
}

void TMapObjMoveItemBoxLimit::move() {
    mFreeFall->update();
    fixMoveSphere();
    moveShadowModel();
}

void TMapObjMoveItemBoxLimit::fixMoveSphere() {
    if (mPos.x < mMin.x) {
        mPos.x = mMin.x;
        mVel.x = 0.0f;
    } else if (mPos.x > mMax.x) {
        mPos.x = mMax.x;
        mVel.x = 0.0f;
    }
    if (mPos.z < mMin.z) {
        mPos.z = mMin.z;
        mVel.z = 0.0f;
    } else if (mPos.z > mMax.z) {
        mPos.z = mMax.z;
        mVel.z = 0.0f;
    }
}

void TMapObjMoveItemBoxLimit::doKartColCallBack(int kart) {
    if (mIsHitKartFlags & (1 << kart)) {
        GeoItemCar::hitItemBoxProc(kart, mEmitter);
        setState(2);
    }
}

inline void TMapObjMoveItemBoxLimit::resetPosition() {}

void TMapObjMoveItemBoxLimit::calc() {
    ExecuteState();
}

