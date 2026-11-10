#include "Shiraiwa/Objects/MapObjDonkyRock.h"

#include <dolphin/mtx.h>

#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/Course/CrsData.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ItemThunder.h"
#include "Sato/JPEffectMgr.h"
#include "Yamamoto/kartCtrl.h"


static inline void mulAdd(const JGeometry::TVec3f &a, const JGeometry::TVec3f &b, JGeometry::TVec3f &dst) {
    register vector signed short va = *(const vector signed short *)&a.x;
    register vector signed short vb = *(const vector signed short *)&b.x;
    register vector signed int vc = *(const vector signed int *)&b.x;
    register vector signed int vd = vec_vmsumshm(va, vb, vc);
    *(vector signed int *)&dst.x = vd;
}
static inline void scaleAddVec(const JGeometry::TVec3f &v, f32 s, JGeometry::TVec3f &dst) {
    register vector float vs = vec_splat(*(vector float *)&s, 0);
    register vector float vv = *(const vector float *)&v.x;
    register vector float vd = *(const vector float *)&dst.x;
    register vector signed int r = vec_vmsumshm((vector signed short)vv, (vector signed short)vs, (vector signed int)vd);
    *(vector float *)&dst.x = (vector float)r;
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

#include "JSystem/JAudio/JASFakeMatch2.h"

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
    static const char *cBmdName = "/Objects/DonkyRock1Shadow.bmd";
    return cBmdName;
}




































































void TMapObjDonkyRock::start() {
    JUT_ASSERT_MSG(mGenObjData != nullptr, "mGenObjData != 0");

    JUT_ASSERT_MSG(mGenObjData->pathID != 0xffff, "mGenObjData->mPathID != 0xffff");

    const CrsData::SObject *obj = mGenObjData;
    f32 height = mHeightOffset;
    f32 px = obj->position.x;
    f32 py = obj->position.y;
    f32 pz = obj->position.z;
    mPos.x = px;
    mPos.y = py + height;
    mPos.z = pz;
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

#pragma dont_inline on
void TMapObjDonkyRock::setStartVel() {
    static JGeometry::TVec3f axisY(0.0f, 1.0f, 0.0f);

    f32 param3 = mGenObjData->mParam3;
    if (param3 > 0.0f) {
        JGeometry::TVec3f node;
        getNodePosition(&node, 1);
        mulAdd(node, mGenObjData->position, mStartVel);
        mStartVel.y = 0.0f;
        mStartVel.normalize();

        JGeometry::TVec3f side;
        side.cross(mStartVel, axisY);
        side.normalize();

        JGeometry::TQuat4f quat;
        quat.setRotate(side, mGenObjData->mParam4 * 0.0174533f);
        quat.transform(mStartVel);
    } else {
        mStartVel.zero();
    }
}
#pragma dont_inline reset

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

void TMapObjDonkyRock::move_FallDown(JGeometry::TVec3f &pos) {
    static JGeometry::TVec3f axisY(0.0f, 1.0f, 0.0f);

    mVel.add(mGravity);
    if (mVel.length() > sMaxSpeed) {
        mVel.normalize(sMaxSpeed);
    }
    mPos.add(mVel);

    mGravity.x = 0.0f;
    mGravity.y = sGravity;
    mGravity.z = 0.0f;

    mpGround->search(mPos);

    if (mpGround->getAttribute() != CrsGround::Attr_10 &&
        mPos.y < mpGround->getHeight() + mHeightOffset) {
        JGeometry::TVec3f normal;
        mpGround->getNormal(&normal);
        normal.normalize();

        JGeometry::TVec3f dir;
        mulAdd(pos, mPos, dir);

        JGeometry::TVec3f axis;
        axis.cross(dir, normal);
        axis.normalize();

        mPos.y = mpGround->getHeight() + mHeightOffset;
        getReflect(normal, dir);
    }

    JGeometry::TVec3f tangent;
    tangent.x = mVel.x;
    tangent.y = 0.0f;
    tangent.z = mVel.z;

    if (tangent.length() > JGeometry::TUtilf::epsilon()) {
        JGeometry::TVec3f axis;
        axis.cross(tangent, axisY);
        axis.normalize();

        f32 roll = mRollAngle - sRotSpeed * mVel.length() / mScale.y;
        while (roll < 0.0f) {
            roll += 6.2831855f;
        }
        while (roll > 3.1415927f) {
            roll -= 6.2831855f;
        }
        mRollAngle = roll;
        mRotMtx.setRotate(axis, roll);
    }
}
void TMapObjDonkyRock::getReflect(JGeometry::TVec3f &vel, JGeometry::TVec3f &dir) {
    if (vel.squared() <= 100.0f + sGravity * sGravity) {
        scaleAddVec(vel, -0.9f, mVel);
        mSound->setRollSe(mVel.length());
    } else {
        f32 scale = 1.0f + sReflectRate;
        scaleAddVec(vel, -scale, mVel);
        f32 mag = mVel.length();
        mVel.normalize();
        mVel.x = dir.x * mag;
        mVel.y = dir.y * mag;
        mVel.z = dir.z * mag;
        mSound->setBoundSe(mag);
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
    u32 change = 0;
    u64 status = GetKartCtrl()->GetKartStatus(kartNo);

    if ((status & 0x0004000400000001ULL) != 0) {
        _58 = 11;
        change = 1;
    } else if ((status & 0x0000400000000000ULL) != 0) {
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

