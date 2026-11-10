#include "Kawano/osage.h"

#include "JSystem/J3D/J3DSys.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/RaceDrawer.h"
#include "Kaneshige/KartDrawer.h"
#include "Kaneshige/RaceLight.h"

#include "dolphin/mtx.h"

DriverModel *Osage::sCurDriver;
KinopicoHairCtrl *Osage::sCurHair_L;
KinopicoHairCtrl *Osage::sCurHair_R;
Osage *Osage::sCurOsage;

u32 OsageMgr::mNoIKFlag;
OsageMgr *OsageMgr::mspOsageMgr;

const char *OsageMgr::mscaaJntName[7] = {
    "k_hair_all_root", "k_hair_L1", "k_hair_L2", "k_hair_L3",
    "k_hair_R1", "k_hair_R2", "k_hair_R3",
};

HairCallBack OsageMgr::mscaCallBackTbl[7] = {
    (HairCallBack)&Osage::kHairRootCallBack,
    (HairCallBack)&Osage::kHairL1CallBack,
    (HairCallBack)&Osage::kHairL2CallBack,
    (HairCallBack)&Osage::kHairL3CallBack,
    (HairCallBack)&Osage::kHairR1CallBack,
    (HairCallBack)&Osage::kHairR2CallBack,
    (HairCallBack)&Osage::kHairR3CallBack,
};

const char *OsageMgr::mscaOsageBmdFileName = "/Driver/Kinopico/kinoko_hair.bmd";

Osage::Osage(DriverModel *driver) : ExModel(1) {
    mDriver = driver;
    OsageMgr *mgr = OsageMgr::ptr();
    int count = mgr->mCount;
    OsageMgr *mgr2 = OsageMgr::ptr();
    setModelData(0, mgr2->mModelData[count]);

    int kartNum = driver->mKartNum;
    u8 driverNo = driver->mDriverNo;
    mHair_L = new KinopicoHairCtrl(kartNum, driverNo, (KinopicoHairCtrl::KHairLR)0);
    mHair_R = new KinopicoHairCtrl(kartNum, driverNo, (KinopicoHairCtrl::KHairLR)1);
}

void Osage::updateOffset() {
    sCurDriver = mDriver;
    sCurHair_L = mHair_L;
    sCurHair_R = mHair_R;
    sCurOsage = this;
    DriverData *data = DriverData::mspDriverData;
    mHair_L->offsetMtxPos(data->m_18);
    mHair_R->offsetMtxPos(data->m_18);
    sCurDriver = nullptr;
    sCurHair_L = nullptr;
    sCurHair_R = nullptr;
    sCurOsage = nullptr;
}

void Osage::update(u16) {
    DriverData *data = DriverData::mspDriverData;
    sCurDriver = mDriver;
    sCurHair_L = mHair_L;
    sCurHair_R = mHair_R;
    sCurOsage = this;

    Mtx mtx;
    PSMTXCopy((MtxPtr)mDriver->getMtxP(18), mtx);

    f32 x = data->m_48.x;
    f32 y = data->m_48.y;
    f32 z = data->m_48.z;
    Vec offset;
    offset.x = x;
    offset.y = y;
    offset.z = z;

    Vec pos;
    PSMTXMultVec(mtx, &offset, &pos);
    mtx[0][3] = pos.x;
    mtx[1][3] = pos.y;
    mtx[2][3] = pos.z;

    PSMTXCopy(mtx, ExModel::mModel[(u16)OsageMgr::ptr()->mJntNo[0]]->mPosMtx);
    ExModel::update(0);

    sCurDriver = nullptr;
    sCurHair_L = nullptr;
    sCurHair_R = nullptr;
    sCurOsage = nullptr;
}

bool Osage::createModel(JKRSolidHeap *heap, u32 p2, u32 p3) {
    ExModel::createDifferedModel(heap, p2, 0x1080200, true);
    return true;
}

void Osage::setCurrentViewNo(u32 viewNo) {
    ExModel::setCurrentViewNo(viewNo);
    int kartNo = mDriver->mKartNum;
    RaceKartLight *light = RaceMgr::getManager()->getKartDrawer(kartNo)->getLight(viewNo);
    MtxPtr mtx = light->getEffectMtx();
    setEffectMtx(mtx, 1);
}

bool Osage::kHairRootCallBack(J3DJoint *, int joint) {
    if (OsageMgr::ptr()->CheckNoIK())
        return true;

    if (joint == 0) {
        if (sCurDriver == nullptr)
            return true;
        if (sCurDriver->m_e8 != 0)
            return true;
        if ((s32)OsageMgr::ptr()->mNoIKFlag != 0)
            return true;
    }
    return true;
}

bool Osage::kHairL1CallBack(J3DJoint *, int joint) {
    if (OsageMgr::ptr()->CheckNoIK())
        return true;

    if (joint == 0) {
        J3DModel *model = j3dSys.getModel();
        DriverModel *driver = sCurDriver;
        if (driver == nullptr)
            return true;
        if (driver->m_e8 != 0)
            return true;
        if ((s32)OsageMgr::ptr()->mNoIKFlag != 0)
            return true;

        sCurHair_L->update_hair1((MtxPtr)model->mMtxBuffer->mWorldMatrices, (MtxPtr)j3dSys.mCurrentMtx, driver);
        PSMTXCopy(sCurHair_L->mMtx, j3dSys.mCurrentMtx);
        PSMTXCopy(j3dSys.mCurrentMtx, j3dSys.mCurrentMtx);
    }
    return true;
}

bool Osage::kHairL2CallBack(J3DJoint *, int joint) {
    if (OsageMgr::ptr()->CheckNoIK())
        return true;

    if (joint == 0) {
        DriverModel *driver = sCurDriver;
        if (driver == nullptr)
            return true;
        if (driver->m_e8 != 0)
            return true;
        if ((s32)OsageMgr::ptr()->mNoIKFlag != 0)
            return true;

        sCurHair_L->update_hair2((MtxPtr)j3dSys.mCurrentMtx, driver);
        PSMTXCopy(sCurHair_L->mMtx, j3dSys.mCurrentMtx);
        PSMTXCopy(j3dSys.mCurrentMtx, j3dSys.mCurrentMtx);
    }
    return true;
}

bool Osage::kHairL3CallBack(J3DJoint *, int joint) {
    if (OsageMgr::ptr()->CheckNoIK())
        return true;

    if (joint == 0) {
        if (sCurDriver == nullptr)
            return true;
        if (sCurDriver->m_e8 != 0)
            return true;
        if ((s32)OsageMgr::ptr()->mNoIKFlag != 0)
            return true;
    }
    return true;
}

bool Osage::kHairR1CallBack(J3DJoint *, int joint) {
    if (OsageMgr::ptr()->CheckNoIK())
        return true;

    if (joint == 0) {
        J3DModel *model = j3dSys.getModel();
        DriverModel *driver = sCurDriver;
        if (driver == nullptr)
            return true;
        if (driver->m_e8 != 0)
            return true;
        if ((s32)OsageMgr::ptr()->mNoIKFlag != 0)
            return true;

        sCurHair_R->update_hair1((MtxPtr)model->mMtxBuffer->mWorldMatrices, (MtxPtr)j3dSys.mCurrentMtx, driver);
        PSMTXCopy(sCurHair_R->mMtx, j3dSys.mCurrentMtx);
        PSMTXCopy(j3dSys.mCurrentMtx, j3dSys.mCurrentMtx);
    }
    return true;
}

bool Osage::kHairR2CallBack(J3DJoint *, int joint) {
    if (OsageMgr::ptr()->CheckNoIK())
        return true;

    if (joint == 0) {
        DriverModel *driver = sCurDriver;
        if (driver == nullptr)
            return true;
        if (driver->m_e8 != 0)
            return true;
        if ((s32)OsageMgr::ptr()->mNoIKFlag != 0)
            return true;

        sCurHair_R->update_hair2((MtxPtr)j3dSys.mCurrentMtx, driver);
        PSMTXCopy(sCurHair_R->mMtx, j3dSys.mCurrentMtx);
        PSMTXCopy(j3dSys.mCurrentMtx, j3dSys.mCurrentMtx);
    }
    return true;
}

bool Osage::kHairR3CallBack(J3DJoint *, int joint) {
    if (OsageMgr::ptr()->CheckNoIK())
        return true;

    if (joint == 0) {
        if (sCurDriver == nullptr)
            return true;
        if (sCurDriver->m_e8 != 0)
            return true;
        if ((s32)OsageMgr::ptr()->mNoIKFlag != 0)
            return true;
    }
    return true;
}

OsageMgr::OsageMgr() {
    for (int i = 0; i < 16; i++) {
        mOsage[i] = nullptr;
        mModelData[i] = nullptr;
    }
    mCount = 0;
    mNoIKFlag = 0;
    mModelData[0] = createModelData((char *)mscaOsageBmdFileName);
    for (int i = 0; i < 7; i++) {
        int idx = mModelData[0]->getJointName()->getIndex(mscaaJntName[i]);
        if (idx != -1)
            mJntNo[i] = idx;
        else
            mJntNo[i] = 0xffff;
    }
    setCallBack(0);
}

OsageMgr::~OsageMgr() {
    mspOsageMgr = nullptr;
}

OsageMgr *OsageMgr::ptr() {
    if (mspOsageMgr == nullptr)
        mspOsageMgr = new OsageMgr();
    return mspOsageMgr;
}

#pragma dont_inline on
void OsageMgr::init(DriverModel *driver) {
    DriverModel *d = driver;
    OsageMgr *mgr = this;
    for (int i = 0; i < mgr->mCount; i++) {
        if (mgr->mOsage[i]->mDriver == d)
            return;
    }

    if (mgr->mCount != 0) {
        mgr->mModelData[mgr->mCount] = mgr->createModelData((char *)mscaOsageBmdFileName);
        mgr->setCallBack(mgr->mCount);
    }

    mgr->mOsage[mgr->mCount] = new Osage(d);
    mgr->mCount++;
}
#pragma dont_inline reset

void OsageMgr::setCallBack(int idx) {
    OsageMgr *mgr = this;
    J3DModelData **modelData = &mgr->mModelData[idx];
    for (u16 ctrlNo = 0; ctrlNo < 7; ctrlNo++) {
        J3DJoint *jnt = nullptr;
#undef __FILE__
#define __FILE__ "osage.h"
#line 95
        JUT_MINMAX_ASSERT(0, ctrlNo, 7);
#undef __FILE__

        u16 jntNo = mgr->mJntNo[ctrlNo];
        if (jntNo != 0xffff)
            jnt = modelData[0]->getJointNodePointer(jntNo);

        if (jnt != nullptr)
            jnt->mFunction = mscaCallBackTbl[ctrlNo];
    }
}

void OsageMgr::reset() {
    for (int i = 0; i < mCount; i++)
        mOsage[i]->update(0);
}

void OsageMgr::update() {
    for (int i = 0; i < mCount; i++)
        mOsage[i]->update(0);
}

void OsageMgr::update(DriverModel *driver) {
    for (int i = 0; i < mCount; i++) {
        if (mOsage[i]->mDriver == driver)
            mOsage[i]->update(0);
    }
}

void OsageMgr::createModel(DriverModel *driver, JKRSolidHeap *heap, u32 p2, u32 p3) {
    for (int i = 0; i < mCount; i++) {
        if (mOsage[i]->mDriver == driver) {
            mOsage[i]->createModel(heap, p2, p3);
            if (i == 0) {
                mNoIKFlag = 1;
                mOsage[0]->makeSharedDL();
                mNoIKFlag = 0;
                mOsage[0]->lock();
            }
            return;
        }
    }
}

void OsageMgr::setCurrentViewNo(DriverModel *driver, u32 viewNo) {
    for (int i = 0; i < mCount; i++) {
        if (mOsage[i]->mDriver == driver) {
            mOsage[i]->setCurrentViewNo(viewNo);
            return;
        }
    }
}

bool OsageMgr::CheckNoIK() {
    return DriverData::mspDriverData->m_18 == 0;
}

Osage *OsageMgr::IsOsageExist(DriverModel *driver) {
    for (int i = 0; i < mCount; i++) {
        if (mOsage[i]->mDriver == driver)
            return mOsage[i];
    }
    return nullptr;
}
