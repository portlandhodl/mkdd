#include "Kawano/accessory.h"
#include "JSystem/J3D/J3DModelLoader.h"
#include "JSystem/J3D/J3DTransform.h"
#include "Kaneshige/KartDrawer.h"
#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/SimpleDrawer.h"
#include "Kawano/driver.h"
#include "Osako/ResMgr.h"
#include "Yamamoto/KartThunder.h"
#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCtrl.h"
#include "dolphin/mtx.h"
#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

AccessoryMgr *AccessoryMgr::mspAccessoryMgr;

const char *AccessoryMgr::mscaAccessoryBmdFileName = "/Driver/KingTeresa/shine.bmd";
const char *AccessoryMgr::mscaAccessoryBtkFileName = "/Driver/KingTeresa/flicker.btk";

Accessory::Accessory(DriverModel *driver) : ExModel(1) {
    mDriver = driver;
    setModelData(AccessoryMgr::ptr()->mModelData);
    AccessoryMgr *mgr = AccessoryMgr::ptr();
    mFrameCtrl.init(mgr->mAnm->getFrameMax());
    mFrameCtrl.setAttribute(2);
    mMaterialAnm.setExModel(this);
    mMaterialAnm.setAnmBase(AccessoryMgr::ptr()->mAnm);
    mMaterialAnm.initFrameCtrl(mMaterialAnm.mAnmBase);
    mMaterialAnm.setFrame(mMaterialAnm.mFrameCtrl.mStartFrame);
    mMaterialAnm.setRate(1.0f);
    mMaterialAnm.mFrameCtrl.mState = 0;
    mMaterialAnm.stop();
    mMaterialAnm.mFrameCtrl.mAttr = 2;
}

void Accessory::reset() {}

void Accessory::updateOffset() {}

void Accessory::update(u16) {
    f32 scale = 1.0f;
    AccessoryMgr::ptr()->mMtxCalc->setAnmTransform(mAnm);
    mFrameCtrl.update();

    if (KartCtrl::getKartCtrl()->CheckThunderBolt(mDriver->mKartNum))
        scale = KartCtrl::getKartCtrl()->getKartBody(mDriver->mKartNum)->getThunder()->getScale();
    mScale.set(scale, scale, scale);

    Mtx mtx;
    PSMTXCopy((MtxPtr)mDriver->getMtxP(0), (MtxPtr)mtx);
    setBaseTRMtx(mtx);

    if (mMaterialAnm.mAnmBase != nullptr) {
        mMaterialAnm.setFrame(mFrameCtrl.getFrame());
        mMaterialAnm.mFrameCtrl.update();
        mMaterialAnm.anmFrameProc();
    }

    int kartNo = mDriver->mKartNum;
    if (RaceMgr::getManager()->getKartDrawer(kartNo)->getHiddenDrivers() != 0)
        hide();
    else
        show();

    ExModel::update(0);
}

void Accessory::simpleDraw(u32) {}

bool Accessory::createModel(JKRSolidHeap *heap, u32 p2, u32) {
    ExModel::createDifferedModel(heap, p2, 0x1080200, true);
    return true;
}

void Accessory::setCurrentViewNo(u32 viewNo) {
    ExModel::setCurrentViewNo(viewNo);
}

AccessoryMgr::AccessoryMgr() {
    for (int i = 0; i < 16; i++)
        mAccessories[i] = nullptr;
    mCount = 0;
    mModelData = createModelData();
    mMtxCalc = new J3DMtxCalcAnimation<J3DMtxCalcAnimationAdaptorDefault<J3DMtxCalcCalcTransformMaya>, J3DMtxCalcJ3DSysInitMaya>(nullptr);
    mModelData->getJointNodePointer(0)->setMtxCalc(mMtxCalc);
    void *btk = ResMgr::getPtr(ResMgr::mcArcMRAM, mscaAccessoryBtkFileName);
    if (btk != nullptr)
        J3DAnmObjMaterial::setupTexSRTAnmData(&mAnm, mModelData, btk);
}

AccessoryMgr::~AccessoryMgr() {
    mspAccessoryMgr = nullptr;
}

AccessoryMgr *AccessoryMgr::ptr() {
    if (mspAccessoryMgr == nullptr)
        mspAccessoryMgr = new AccessoryMgr();
    return mspAccessoryMgr;
}

J3DModelData *AccessoryMgr::createModelData() {
    J3DModelData *modelData = J3DModelLoaderDataBase::load(ResMgr::getPtr(ResMgr::mcArcMRAM, mscaAccessoryBmdFileName), 0x11020010);
    ExModel::patchModelData(modelData);
    modelData->newSharedDisplayList(0x40000);
    modelData->simpleCalcMaterial(0, *(Mtx *)&j3dDefaultMtx);
    modelData->makeSharedDL();
    return modelData;
}

void AccessoryMgr::init(DriverModel *driver) {
    for (int i = 0; i < mCount; i++) {
        if (mAccessories[i]->mDriver == driver)
            return;
    }

    mAccessories[mCount] = new Accessory(driver);
    mAccessories[mCount]->_18 |= 2;
    mCount++;
}

void AccessoryMgr::reset() {}

void AccessoryMgr::update() {
    for (int i = 0; i < mCount; i++)
        mAccessories[i]->update(0);
}

void AccessoryMgr::update(DriverModel *) {}

void AccessoryMgr::createModel(DriverModel *, JKRSolidHeap *, u32, u32) {}

void AccessoryMgr::setCurrentViewNo(DriverModel *, u32) {}

void AccessoryMgr::setCurrentViewNo(u32 viewNo) {
    for (int i = 0; i < mCount; i++)
        mAccessories[i]->setCurrentViewNo(viewNo);
}

void AccessoryMgr::simpleDraw(u32 viewNo) {
    Accessory *acc;
    int i = 0;
    for (; i < mCount; i++) {
        acc = mAccessories[i];
        SimpleDrawer drawer;
        drawer.drawInit(acc);
        while (drawer.loadPreDrawSetting())
            acc->ExModel::simpleDraw(viewNo, nullptr, 1);
    }
}
