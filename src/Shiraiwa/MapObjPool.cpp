#include "Shiraiwa/Objects/MapObjPool.h"

#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/JMath/JMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/stMath.h"
#include "Shiraiwa/SiUtil.h"

TPoolSupervisor *TMapObjPool::sSupervisor;
J3DAnmTextureSRTKey *TMapObjPool::sPoolBtkAnm;
u8 TMapObjPool::sUseBtk;

TMapObjPool::TMapObjPool(const CrsData::SObject &obj) : TMapObjHioNode(obj), mEntryLink(this) {
    if (sSupervisor == nullptr)
        sSupervisor = new TPoolSupervisor();

    sSupervisor->entry(this);
    setObjFlagSimpleDraw();
}

TMapObjPool::~TMapObjPool() {
    sSupervisor = nullptr;
}

void TMapObjPool::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *btk = ObjUtility::getPtrCourseArc("/Objects/Pool.btk");

    if (btk != nullptr) {
        J3DAnmObjMaterial::setupTexSRTAnmData(&sPoolBtkAnm, modelData, btk);
        sUseBtk = 1;
    } else {
        sUseBtk = 0;
    }
}

void TMapObjPool::createModel(JKRSolidHeap *heap, u32 viewNo) {
    if (sUseBtk) {
        mBtk.setExModel(&mModel);
        mBtk.attach(sPoolBtkAnm);
    }
}

void TMapObjPool::update() {
    if (sUseBtk)
        mBtk.anmFrameProc();

    GeographyObj::update();
}

void TMapObjPool::reset() {
    GeographyObj::resetObject();
    clrObjFlagCheckGeoHitting();
    clrObjFlagCheckItemHitting();
    mCheckKartHitFlags = 0;

    mBlend = mObjData->mParam1 * 0.1f;
    if (mBlend == 0.0f)
        mBlend = 1.0f;

    if (sUseBtk) {
        mBtk.mFrameCtrl.mFrame = mBtk.mFrameCtrl.mStartFrame;
        mBtk.mFrameCtrl.mRate = 1.0f;
        mBtk.mFrameCtrl.mState = 0;
        SiUtil::setRandomStartFrame(&mBtk.mFrameCtrl, getGeoRnd());
    }
}

void TMapObjPool::getUpDir(JGeometry::TVec3f *up) {
    if (up != nullptr)
        mRotMtx.getYDir(*up);
}

void TMapObjPool::calc() {
    JGeometry::TVec3f vec;
    vec.x = RCMGetCourse()->mDirY.x;
    vec.y = RCMGetCourse()->mDirY.y;
    vec.z = RCMGetCourse()->mDirY.z;

    static f32 ydir[3];
    static s8 init;
    if (!init) {
        ydir[0] = 0.0f;
        ydir[1] = 1.0f;
        ydir[2] = 0.0f;
        init = 1;
    }

    JMAVECLerp((JGeometry::TVec3f *)ydir, &vec, &vec, mBlend);

    JGeometry::TVec3f side;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f up;
    JGeometry::TVec3f dir;
    mRotMtx.getYDir(up);
    dir.sub(vec, up);
    if (dir.length() > 0.0006f)
        dir.setLength(0.0006f);
    vec.add(up, dir);
    vec.normalize();

    mRotMtx.getZDir(side);
    side.normalize();

    tmp.cross(vec, side);
    tmp.normalize();

    side.cross(tmp, vec);
    side.normalize();

    mRotMtx[0][0] = tmp.x;
    mRotMtx[1][0] = tmp.y;
    mRotMtx[2][0] = tmp.z;
    mRotMtx[0][1] = vec.x;
    mRotMtx[1][1] = vec.y;
    mRotMtx[2][1] = vec.z;
    mRotMtx[0][2] = side.x;
    mRotMtx[1][2] = side.y;
    mRotMtx[2][2] = side.z;
    if (sUseBtk)
        mBtk.mFrameCtrl.update();
}

f32 TMapObjPool::getHeight(f32 x, f32 z) {
    JGeometry::TVec3f up;
    mRotMtx.getYDir(up);
    up.normalize();
    f32 dx = x - mPos.x;
    f32 dz = z - mPos.z;
    return mPos.y + -(up.x * dx + up.z * dz) / up.y;
}

TPoolSupervisor::TPoolSupervisor() : mEntryList() {}

TPoolSupervisor::~TPoolSupervisor() {}

void TPoolSupervisor::calc() {}

void TPoolSupervisor::entry(TMapObjPool *pool) {
    mEntryList.append(&pool->mEntryLink);
}

f32 TPoolSupervisor::getHeight(f32 x, f32 z) {
    f32 height = -1.0f;

    for (JSULink<TMapObjPool> *link = mEntryList.getFirst(); link != nullptr; link = link->getNext()) {
        TMapObjPool *pool = link->getObject();

        JGeometry::TVec3f up;
        pool->mRotMtx.getYDir(up);
        f32 dx = x - pool->mPos.x;
        f32 dz = z - pool->mPos.z;
        up.normalize();

        f32 h = pool->mPos.y + -(up.x * dx + up.z * dz) / up.y;
        height = h;
        if (h > -1.0f)
            break;
    }

    return height;
}

#include <JSystem/JAudio/JASFakeMatch2.h>
