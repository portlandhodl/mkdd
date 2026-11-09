#include "Shiraiwa/Objects/MapObjRollTable.h"
#include "Kaneshige/RaceMgr.h"
#include "Shiraiwa/SiUtil.h"


class TPathUtil {
public:
    TPathUtil(const CrsData::SObject &); // 0x802c4c28
    virtual ~TPathUtil(); // 0x802c4c74

    void getDirection(JGeometry::TVec3f &); // 0x802c4c80

    CrsData::SObject *mObj; // 0x4
    u8 _8[0xC];
};

static JGeometry::TVec3f axisY;
static u8 axisYInit;
extern f32 sFriction__16TMapObjRollTable;
extern f32 sAccParam__16TMapObjRollTable;
extern f32 sAccThreshold__16TMapObjRollTable;
extern f32 sVelStop__16TMapObjRollTable;
extern f32 sRotSpeed__16TMapObjRollTable;
extern f32 sRotMaxSpeed__16TMapObjRollTable;
extern void *sRaceManager__7RaceMgr;
extern JGeometry::TVec3f axisY;
extern u8 init;
extern CrsData::PointData *getPointData__7CrsDataCFUsUs(const CrsData *, u16, u16);
extern f32 getNormalRange__6SiUtilFfff(f32, f32, f32);
extern int getGeoRnd__12GeographyObjFv();
extern void resetObject__12GeographyObjFv(GeographyObj *);
extern void PSMTXConcat(const void *, const void *, void *);
extern f32 PSVECMag(const void *);
extern void setSeWithVelocityVP__Q29GameAudio14ObjectSoundMgrFUlff(GameAudio::ObjectSoundMgr *, u32, f32, f32);
double asmD_4330000080000000 = 4503601774854144.0;
f32 asmF_00000000 = 0.0f;
f32 asmF_3e99999a = 0.3f;
f32 asmF_3f000000 = 0.5f;
f32 asmF_3f800000 = 1.0f;
f32 asmF_40400000 = 3.0f;
f32 asmF_41f00000 = 30.0f;
f32 asmF_42000000 = 32.0f;

f32 TMapObjRollTable::sFriction = 0.99f;
f32 TMapObjRollTable::sAccParam = 5.0f;
f32 TMapObjRollTable::sAccThreshold = 0.05f;
f32 TMapObjRollTable::sVelStop = 0.1f;
f32 TMapObjRollTable::sRotSpeed = 0.005f;
f32 TMapObjRollTable::sRotMaxSpeed = 10.0f;

TMapObjRollTable::TMapObjRollTable(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
    mPathUtil = new TPathUtil(obj);
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
    JGeometry::TVec3f pos0 = p0->pos;
    CrsData::PointData *p1 = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mPathUtil->mObj->pathID, 1);

    mMin.set(pos0);
    if (mMin.x >= p1->pos.x) mMin.x = p1->pos.x;
    if (mMin.y >= p1->pos.y) mMin.y = p1->pos.y;
    if (mMin.z >= p1->pos.z) mMin.z = p1->pos.z;

    mMax.set(pos0);
    if (mMax.x <= p1->pos.x) mMax.x = p1->pos.x;
    if (mMax.y <= p1->pos.y) mMax.y = p1->pos.y;
    if (mMax.z <= p1->pos.z) mMax.z = p1->pos.z;

    mFlag = true;
}

void TMapObjRollTable::calc() {
    bool stopped = false;
    if (mVel.squared() <= JGeometry::TUtilf::epsilon())
        stopped = true;

    mVel.scale(sFriction);
    JGeometry::TVec3f pow;
    getPow(&pow);

    JGeometry::TVec3f acc;
    acc.x = mDir.x * pow.x;
    acc.y = mDir.y * pow.y;
    acc.z = mDir.z * pow.z;

    if (acc.length() < sAccThreshold)
        acc.zero();

    mVel.scaleAdd(sAccParam, acc);

    JGeometry::TVec3f newPos;
    newPos.add(mPos, mVel);
    SiUtil::clampVec(newPos, newPos, mMin, mMax);

    if ((newPos.x == mPos.x && newPos.y == mPos.y && newPos.z == mPos.z) || mVel.length() < sVelStop) {
        mVel.zero();
        mPos.set(newPos);
        return;
    }

    if (stopped) {
        if (getGeoRnd()->get_ufloat_1() < 0.5f)
            mFlag = -1;
        else
            mFlag = 1;
    } else {
        getSoundMgr()->setSeWithVelocityVP(0x40049, SiUtil::getNormalRange(mVel.length(), 0.0f, 30.0f), 0.3f);
    }

    f32 ang = SiUtil::getNormalRange(mVel.length(), 0.0f, sRotMaxSpeed);
    JGeometry::TPos3f rot;
    rot.setRotate(axisY, ang * sRotSpeed * (f32)mFlag);
    PSMTXConcat(mRotMtx, rot, mRotMtx);

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
