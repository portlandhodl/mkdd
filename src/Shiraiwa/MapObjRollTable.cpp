#include "Shiraiwa/Objects/MapObjRollTable.h"
#include "Kaneshige/RaceMgr.h"
#include "Shiraiwa/SiUtil.h"
#include "Shiraiwa/Coord3DUtil.h"


f32 TMapObjRollTable::sFriction = 0.99f;
f32 TMapObjRollTable::sAccParam = 5.0f;
f32 TMapObjRollTable::sAccThreshold = 0.05f;
f32 TMapObjRollTable::sVelStop = 0.1f;
f32 TMapObjRollTable::sRotSpeed = 0.005f;
f32 TMapObjRollTable::sRotMaxSpeed = 10.0f;

TMapObjRollTable::TMapObjRollTable(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
    mPathUtil = new TPathUtil(&obj);
    createSoundMgr();
}

TMapObjRollTable::~TMapObjRollTable() {
    delete mPathUtil;
}

void TMapObjRollTable::createColModel(J3DModelData *mdlData) {
    f32 maxXZ = mScale.x > mScale.z ? mScale.x : mScale.z;
    f32 maxAll = maxXZ > mScale.y ? maxXZ : mScale.y;
    createBoundsCylinder(mdlData, maxXZ / maxAll * 250.0f, mScale.y / maxAll * 220.0f);
}

void TMapObjRollTable::reset() {
    resetObject();
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    _58 = 1;
    mPathUtil->getDirection(mDir);
    mDir.normalize();

    CrsData::PointData *p0 = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mPathUtil->mObj->pathID, 0);
    JGeometry::TVec3f pos0;
    pos0.set(p0->pos);
    CrsData::PointData *p1 = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mPathUtil->mObj->pathID, 1);
    JGeometry::TVec3f pos1;
    pos1.set(p1->pos);

    mMin.set(pos0);
    if (mMin.x >= pos1.x) mMin.x = pos1.x;
    if (mMin.y >= pos1.y) mMin.y = pos1.y;
    if (mMin.z >= pos1.z) mMin.z = pos1.z;

    mMax.set(pos0);
    if (mMax.x <= pos1.x) mMax.x = pos1.x;
    if (mMax.y <= pos1.y) mMax.y = pos1.y;
    if (mMax.z <= pos1.z) mMax.z = pos1.z;

    mFlag = true;
}

void TMapObjRollTable::calc() {
    bool stopped = false;
    if (mVel.isZero())
        stopped = true;

    mVel.scale(sFriction);
    JGeometry::TVec3f pow;
    getPow(&pow);

    JGeometry::TVec3f acc;
    acc.scale(mDir.dot(pow), mDir);
    if (mVel.isZero()) {
        if (acc.length() < sAccThreshold)
            acc.zero();
    }
    acc.scale(sAccParam);
    mVel.add(acc);

    JGeometry::TVec3f newPos(mPos);
    newPos.add(mVel);
    SiUtil::clampVec(newPos, newPos, mMin, mMax);

    if (newPos.equals(mPos) || mVel.length() < sVelStop) {
        mVel.zero();
    } else {
        if (stopped) {
            if (getGeoRnd()->get_ufloat_1() < 0.5f)
                mFlag = -1;
            else
                mFlag = 1;
        } else {
            f32 vol = SiUtil::getNormalRange(mVel.length(), 0.0f, 30.0f);
            getSoundMgr()->setSeWithVelocityVP(0x40049, vol, 0.3f);
        }

        static JGeometry::TVec3f axisY(0.0f, 1.0f, 0.0f);
        JGeometry::TPos3f rot;
        rot.setRotate(axisY, (f32)mFlag * sRotSpeed * SiUtil::getNormalRange(mVel.length(), 0.0f, sRotMaxSpeed));
        PSMTXConcat(mRotMtx, rot, mRotMtx);
    }

    mPos.set(newPos);
}

void TMapObjRollTable::getPow(JGeometry::TVec3f *pow) {
    JGeometry::TVec3f *dir = (JGeometry::TVec3f *)((u8 *)RaceMgr::sRaceManager->getCourse() + 0x234);
    pow->x = dir->x;
    pow->y = dir->y;
    pow->z = dir->z;
    pow->negate();
}

const char *TMapObjRollTable::getBmdFileName() {
    static const char *cBmdName = "/Objects/TestWall1.bmd";
    return cBmdName;
}

#include <JSystem/JAudio/JASFakeMatch2.h>
