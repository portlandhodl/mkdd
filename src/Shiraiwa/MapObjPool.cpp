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

    JGeometry::TVec3f up(mRotMtx[0][1], mRotMtx[1][1], mRotMtx[2][1]);
    JGeometry::TVec3f dir;
    dir.cross(vec, up);
    if (dir.length() > 0.0006f)
        dir.normalize();
    vec.normalize();

    JGeometry::TVec3f side(mRotMtx[0][2], mRotMtx[1][2], mRotMtx[2][2]);
    side.normalize();

    JGeometry::TVec3f tmp;
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
}

TPoolSupervisor::TPoolSupervisor() : mEntryList() {}

TPoolSupervisor::~TPoolSupervisor() {}

void TPoolSupervisor::calc() {}

void TPoolSupervisor::entry(TMapObjPool *pool) {
    mEntryList.append(&pool->mEntryLink);
}

f32 TPoolSupervisor::getHeight(f32 x, f32 z) {
    f32 height = 0.0f;

    for (JSULink<TMapObjPool> *link = mEntryList.getFirst(); link != nullptr; link = link->getNext()) {
        TMapObjPool *pool = link->getObject();

        f32 dx = x - pool->mPos.x;
        f32 dz = z - pool->mPos.z;

        JGeometry::TVec3f up(pool->mRotMtx[0][1], pool->mRotMtx[1][1], pool->mRotMtx[2][1]);
        up.normalize();

        height = pool->mPos.y - (up.x * dx - up.z * dz) / up.y;
        if (height > 0.0f)
            break;
    }

    return height;
}

__declspec(weak) const char *TMapObjPool::getBmdFileName() {
    return "/Objects/Pool.bmd";
}

#include <JSystem/JAudio/JASFakeMatch2.h>
