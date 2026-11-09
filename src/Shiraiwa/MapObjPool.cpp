#include "Shiraiwa/Objects/MapObjPool.h"

#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/JMath/JMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"

TPoolSupervisor *TMapObjPool::sSupervisor;
J3DAnmTextureSRTKey *TMapObjPool::sPoolBtkAnm;
u8 TMapObjPool::sUseBtk;

TPoolSupervisor::TPoolSupervisor() : mEntryList() {}

TPoolSupervisor::~TPoolSupervisor() {}

void TPoolSupervisor::entry(TMapObjPool *pool) {
    mEntryList.append(&pool->mEntryLink);
}

void TPoolSupervisor::calc() {}

f32 TPoolSupervisor::getHeight(f32 x, f32 z) {
    f32 height = 0.0f;

    for (JSUPtrLink *link = mEntryList.getFirstLink(); link != nullptr; link = link->getNext()) {
        TMapObjPool *pool = (TMapObjPool *)link->getObjectPtr();

        JGeometry::TVec3f up;
        up.set(pool->mRotMtx[0][1], pool->mRotMtx[1][1], pool->mRotMtx[2][1]);
        up.normalize();

        f32 dx = x - pool->mPos.x;
        f32 dz = z - pool->mPos.z;

        height = pool->mPos.y - (up.x * dx + up.z * dz) / up.y;
        if (height > 0.0f)
            break;
    }

    return height;
}

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
    mGeoObjFlag &= ~3;
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

void TMapObjPool::calc() {}

const char *TMapObjPool::getBmdFileName() {
    static const char *cBmdName = "/Objects/Pool.bmd";
    return cBmdName;
}

#include <JSystem/JAudio/JASFakeMatch2.h>
