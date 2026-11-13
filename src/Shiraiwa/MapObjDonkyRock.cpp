#include "Shiraiwa/Objects/MapObjDonkyRock.h"

#include <dolphin/mtx.h>

#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/Course/CrsData.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ItemThunder.h"
#include "Sato/JPEffectMgr.h"
#include "Yamamoto/kartCtrl.h"


template <typename T>
inline T wrapAngle(T val, T min, T max)
{
    while (val < min)
        val += max - min;
    while (val >= max)
        val -= max - min;
    return val;
}

f32 TMapObjDonkyRock::sReflectRate = 0.55f;
f32 TMapObjDonkyRock::sFricRate = 0.99f;
f32 TMapObjDonkyRock::sGravity = -1.0f;
f32 TMapObjDonkyRock::sMaxSpeed = 80.0f;
f32 TMapObjDonkyRock::sRotSpeed = 0.0031f;
f32 TMapObjDonkyRock::sKartDownScale = 0.3f;

TDonkyRockSupervisor *TMapObjDonkyRock::sSupervisor;

StateObserver::StateFuncSet<TMapObjDonkyRock> TMapObjDonkyRock::sTable[3] = {
    { 0, &TMapObjDonkyRock::initFunc_Wait, &TMapObjDonkyRock::doFunc_Wait },
    { 1, &TMapObjDonkyRock::initFunc_Fall, &TMapObjDonkyRock::doFunc_Fall },
    { 2, &TMapObjDonkyRock::initFunc_Break, &TMapObjDonkyRock::doFunc_Break },
};

TMapObjDonkyRock::TMapObjDonkyRock()
    : TMapObjHioNode(0x1197), StateObserver(), JKRDisposer(), mLink(this) {
    mGenObjData = nullptr;
    mObjFlag |= 1;

    mpGround = new CrsGround(RaceMgr::sRaceManager->getCourse());

    if (getSupervisor() == nullptr) {
        sSupervisor = new TDonkyRockSupervisor();
    }
    sSupervisor->entry(this);
}

TMapObjDonkyRock::~TMapObjDonkyRock() {
    delete mpGround;
    sSupervisor = nullptr;
}

void TMapObjDonkyRock::reset() {
    resetObject();
    const CrsData::SObject *obj = mGenObjData;
    mScale.x = obj->scale.x;
    mScale.y = obj->scale.y;
    mScale.z = obj->scale.z;
    getBounds(0)->setScale(mScale);
    _58 = 11;
    mGravity.zero();
    mRollAngle = 0.0f;
    mBounceCount = 0;
    StateObserver::ResetState();
    mGeoObjFlag &= ~1;
    mGeoObjFlag |= 0x20;
    if (mShadowMdl) {
        mShadowMdl->clrVisibleAll();
    }
}

void TMapObjDonkyRock::createColModel(J3DModelData *modelData) {
    createBoundsSphere(modelData);
}

static const f32 sConsts[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

const char *TMapObjDonkyRock::getBmdFileName() {
    static const char *cBmdName = "/Objects/DonkyRock1.bmd";
    return cBmdName;
}

const char *TMapObjDonkyRock::getShadowBmdFileName() {
    static const char *cShadowBmdName = "/Objects/DonkyRock1Shadow.bmd";
    return cShadowBmdName;
}

void TMapObjDonkyRock::start() {
#line 166
    JUT_ASSERT_MSG(mGenObjData != nullptr, "mGenObjData != 0");

    JUT_ASSERT_MSG(mGenObjData->pathID != 0xffff, "mGenObjData->mPathID != 0xffff");

    mPos.set(mGenObjData->position.x, mGenObjData->position.y + mHeightOffset, mGenObjData->position.z);
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    mGeoObjFlag &= ~0x20;
    if (mShadowMdl) {
        mShadowMdl->setVisibleAll();
    }
    moveShadowModel();
    _58 = 11;
    setState(1);
}

void TMapObjDonkyRock::loadAnimation() {}

void TMapObjDonkyRock::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createModel(heap, viewNo, 0);
}

void TMapObjDonkyRock::setObjData(const CrsData::SObject *obj) {
    mGenObjData = obj;
    mHeightOffset = mGenObjData->scale.y * 300.0f;
    mNodeNum = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPathData(mGenObjData->pathID)->pointCount;
    mSound = new GameAudio::RockSoundMgr(&mPos, obj->scale.y, nullptr);
    setStartVel();
}

void TMapObjDonkyRock::InitExec() {
    const StateFuncSet<TMapObjDonkyRock> *func = FindStateFunc(getState(), sTable, 3);
    (this->*func->mInitFunc)();
}

void TMapObjDonkyRock::MoveExec() {
    const StateFuncSet<TMapObjDonkyRock> *func = FindStateFunc(getState(), sTable, 3);
    (this->*func->mExecFunc)();
}

void TMapObjDonkyRock::calc() {
    StateObserver::ExecuteState();
    mSound->frameWork();
}

void TMapObjDonkyRock::initFunc_Wait() {
    mGeoObjFlag |= 0x20;
    if (mShadowMdl) {
        mShadowMdl->clrVisibleAll();
    }
}

void TMapObjDonkyRock::doFunc_Wait() {}

void TMapObjDonkyRock::initFunc_Fall() {
    mVel.x = mStartVel.x;
    mVel.y = mStartVel.y;
    mVel.z = mStartVel.z;
    mGravity.z = 0.0f;
    mGravity.y = 0.0f;
    mGravity.x = 0.0f;
    mRollAngle = 0.0f;
    mBounceCount = 1;
    mGeoObjFlag &= ~0x20;
    mCheckKartHitFlags = -1;
    if (mShadowMdl) {
        mShadowMdl->setVisibleAll();
    }
}

void TMapObjDonkyRock::setStartVel() {
    static JGeometry::TVec3f axisY(0.0f, 1.0f, 0.0f);

    f32 speed = mGenObjData->mParam3;
    if (speed > 0.0f) {
        JGeometry::TVec3f node;
        getNodePosition(&node, 1);
        mStartVel.sub(node, mGenObjData->position);
        mStartVel.y = 0.0f;
        mStartVel.normalize();

        JGeometry::TVec3f side;
        side.cross(mStartVel, axisY);
        side.normalize();

        JGeometry::TQuat4f quat;
        f32 half = 0.5f * (0.017453292f * mGenObjData->mParam4);
        f32 s = sinf(half);
        quat.xyz().scale(s, side);
        quat.w = cosf(half);
        quat.transform(mStartVel);
        mStartVel.scale(speed);
    } else {
        mStartVel.zero();
    }
}

void TMapObjDonkyRock::doFunc_Fall() {
    u16 node = mBounceCount;
    if (node >= mNodeNum) {
        node = mNodeNum - 1;
    }

    JGeometry::TVec3f pos;
    getNodePosition(&pos, node);
    move_FallDown(pos);
    moveShadowModel();

    pos.sub(mPos);
    f32 dist = pos.x * pos.x + pos.z * pos.z;
    if (pos.y > 0.0f || dist < mHeightOffset * mHeightOffset) {
        mBounceCount++;
    }

    if (mBounceCount >= mNodeNum) {
        setState(2);
    }

    if (tstItemHitting()) {
        u32 kind = getColItemObj()->mItemKind;
        if (kind - 7 <= 2 || kind == 1) {
            setState(2);
        }
    }
}

void TMapObjDonkyRock::move_FallDown(JGeometry::TVec3f &target) {
    JGeometry::TVec3f unitNormal;
    JGeometry::TVec3f axis;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f normal;

    mVel.add(mGravity);
    if (mVel.squared() > sMaxSpeed * sMaxSpeed) {
        mVel.normalize();
        mVel.scale(sMaxSpeed);
    }
    mPos.add(mVel);
    mGravity.set(0.0f, sGravity, 0.0f);
    mpGround->search(mPos);
    u32 attr = mpGround->getAttribute();

    static JGeometry::TVec3f axisY(0.0f, 1.0f, 0.0f);

    if (attr != CrsGround::Attr_10 && mPos.y < mHeightOffset + mpGround->getHeight()) {
        mpGround->getNormal(&normal);
        unitNormal.normalize(normal);

        dir.sub(target, mPos);
        dir.normalize();

        axis.cross(dir, unitNormal);
        axis.normalize();

        mPos.y = mHeightOffset + mpGround->getHeight();
        getReflect(unitNormal, dir);
    }

    dir.set(mVel.x, 0.0f, mVel.z);
    if (!dir.isZero()) {
        unitNormal.set(axisY);
        dir.normalize();

        axis.cross(dir, unitNormal);
        axis.normalize();

        mRollAngle += -sRotSpeed * mVel.length() / mScale.y;
        mRollAngle = wrapAngle(mRollAngle, -3.1415927f, 3.1415927f);
        mRotMtx.setRotate(axis, mRollAngle);
    }
}

void TMapObjDonkyRock::getReflect(JGeometry::TVec3f &normal, JGeometry::TVec3f &dir) {
    JGeometry::TVec3f proj;
    proj.scale(normal.dot(mVel), normal);
    if (proj.squared() <= 100.0f + sGravity * sGravity) {
        proj.scale(-0.9f);
        mVel.add(proj);
        mVel.scale(sFricRate);
        mSound->setRollSe(mVel.length());
    } else {
        proj.scale(-(1.0f + sReflectRate));
        mVel.add(proj);
        f32 len = mVel.length();
        mVel.normalize();
        mVel.x = dir.x;
        mVel.z = dir.z;
        mVel.scale(len);
        mSound->setBoundSe(len);
    }
}

void TMapObjDonkyRock::getNodePosition(JGeometry::TVec3f *pos, u16 node) {
    const CrsData::PointData *point =
        RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mGenObjData->pathID, node);
    pos->x = point->pos.x;
    pos->y = point->pos.y;
    pos->z = point->pos.z;
    pos->y += mHeightOffset;
}

void TMapObjDonkyRock::initFunc_Break() {
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    if (mShadowMdl) {
        mShadowMdl->clrVisibleAll();
    }
    mGeoObjFlag |= 0x20;

    JPABaseEmitter *emt = JPEffectMgr::mThis->createEmt("mk_donkyRock_a", mPos);
    emt->mGlobalScl.x = mScale.x;
    emt->mGlobalScl.y = mScale.y;
    emt->mGlobalScl.z = mScale.z;
    f32 y1 = mScale.y;
    emt->mGlobalPScl.x = mScale.x;
    emt->mGlobalPScl.y = y1;
    emt = JPEffectMgr::mThis->createEmt("mk_donkyRock_b", mPos);
    emt->mGlobalScl.x = mScale.x;
    emt->mGlobalScl.y = mScale.y;
    emt->mGlobalScl.z = mScale.z;
    f32 y2 = mScale.y;
    emt->mGlobalPScl.x = mScale.x;
    emt->mGlobalPScl.y = y2;

    mSound->setBrokenSe();
}

void TMapObjDonkyRock::doFunc_Break() {
    setState(0);
}

void TMapObjDonkyRock::doKartColCallBack(int kartNo) {
    bool change = false;

    if (GetKartCtrl()->getKartBody(kartNo)->mCarStatus & 0x0000000100040004ULL) {
        _58 = 11;
        change = 1;
    } else if (GetKartCtrl()->getKartBody(kartNo)->mCarStatus & 0x0000000000004000ULL) {
        _58 = 11;
        if (RaceMgr::sRaceManager->getRaceMode() != TIME_ATTACK) {
            change = 1;
        }
    } else {
        if (mScale.y < sKartDownScale) {
            _58 = 0;
            change = 1;
        } else {
            _58 = 11;
        }
    }

    if (change) {
        setState(2);
    }
}


TDonkyRockSupervisor::TDonkyRockSupervisor() {
    mList.initiate();
    ItemThunder::entrySuperVisor(this);
}

void TDonkyRockSupervisor::entry(TMapObjDonkyRock *obj) {
    mList.append(&obj->mLink);
}

void TDonkyRockSupervisor::thunderDownAll() {
    for (JSUListIterator<TMapObjDonkyRock> it(mList.getFirst()); it != mList.getEnd(); it++) {
        it.getObject()->setState(2);
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
