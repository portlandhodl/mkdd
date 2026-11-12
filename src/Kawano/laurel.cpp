#include "Kawano/laurel.h"

#include "Kaneshige/KartDrawer.h"
#include "Kaneshige/RaceDrawer.h"
#include "Kaneshige/RaceLight.h"
#include "Kaneshige/RaceMgr.h"
#include "Kawano/driver.h"
#include "Kawano/driverData.h"
#include "dolphin/mtx.h"
#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

static const f32 sZeroQuatset[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

LaurelMgr *LaurelMgr::mspLaurelMgr;

Laurel::Laurel(DriverModel *driver) : ExModel(1) {
    mDriver = driver;
    setModelData(0, LaurelMgr::ptr()->mModelData);
}

void Laurel::updateOffset() {
    DriverData *data = DriverData::mspDriverData;
    f32 tx, ty, tz;
    f32 sx, sy, sz;
    f32 rx, ry, rz;
    f32 ox, oy, oz;
    Mtx mtx, scale, rotX, rotY, rotZ;

    PSMTXIdentity(mtx);
    if (data->m_54 != 0) {
        sx = data->m_58;
        sy = data->m_5c;
        sz = data->m_60;
        rx = data->m_64;
        ry = data->m_68;
        rz = data->m_6c;
        ox = data->m_70;
        oy = data->m_74;
        oz = data->m_78;
    } else {
        sx = DriverData::msDriverData[mDriver->mDataID].data[86];
        sy = DriverData::msDriverData[mDriver->mDataID].data[87];
        sz = DriverData::msDriverData[mDriver->mDataID].data[88];
        rx = DriverData::msDriverData[mDriver->mDataID].data[89];
        ry = DriverData::msDriverData[mDriver->mDataID].data[90];
        rz = DriverData::msDriverData[mDriver->mDataID].data[91];
        ox = DriverData::msDriverData[mDriver->mDataID].data[92];
        oy = DriverData::msDriverData[mDriver->mDataID].data[93];
        oz = DriverData::msDriverData[mDriver->mDataID].data[94];
    }

    PSMTXCopy((MtxPtr)mDriver->getMtxP(1), mtx);
    tx = mtx[0][3];
    ty = mtx[1][3];
    tz = mtx[2][3];
    mtx[0][3] = 0.0f;
    mtx[1][3] = 0.0f;
    mtx[2][3] = 0.0f;

    PSMTXScale(scale, sx, sy, sz);
    PSMTXRotRad(rotX, 'x', 0.017453292f * rx);
    PSMTXRotRad(rotY, 'y', 0.017453292f * ry);
    PSMTXRotRad(rotZ, 'z', 0.017453292f * rz);
    PSMTXConcat(mtx, scale, mtx);
    PSMTXConcat(mtx, rotX, mtx);
    PSMTXConcat(mtx, rotY, mtx);
    PSMTXConcat(mtx, rotZ, mtx);

    mtx[0][3] = tx;
    mtx[1][3] = ty;
    mtx[2][3] = tz;

    Vec pos;
    Vec offset;
    offset.x = ox;
    offset.y = oy;
    offset.z = oz;
    PSMTXMultVec(mtx, &offset, &pos);
    mtx[0][3] = pos.x;
    mtx[1][3] = pos.y;
    mtx[2][3] = pos.z;

    PSMTXCopy(mtx, getModel()->mPosMtx);
}

void Laurel::update(u16) {
    updateOffset();
    if (DriverData::mspDriverData->m_54 != 0 || mDriver->m_9c != 0)
        show();
    else
        hide();
    ExModel::update(0);
}

bool Laurel::createModel(JKRSolidHeap *heap, u32 p2, u32) {
    ExModel::createDifferedModel(heap, p2, 0x1080200, true);
    return true;
}

void Laurel::setCurrentViewNo(u32 viewNo) {
    ExModel::setCurrentViewNo(viewNo);
    int kartNo = mDriver->mKartNum;
    RaceKartLight *light = RaceMgr::getManager()->getKartDrawer(kartNo)->getLight(viewNo);
    MtxPtr mtx = light->getEffectMtx();
    setEffectMtx(mtx, 1);
}

const char *LaurelMgr::mscaLaurelBmdFileName = "/Driver/Common/ring.bmd";

LaurelMgr::LaurelMgr() {
    for (int i = 0; i < 16; i++)
        mLaurels[i] = nullptr;
    mCount = 0;
    mModelData = createModelData((char *)mscaLaurelBmdFileName);
}

LaurelMgr::~LaurelMgr() {
    mspLaurelMgr = nullptr;
}

LaurelMgr *LaurelMgr::ptr() {
    if (mspLaurelMgr == nullptr)
        mspLaurelMgr = new LaurelMgr();
    return mspLaurelMgr;
}

void LaurelMgr::init(DriverModel *driver) {
    for (int i = 0; i < mCount; i++) {
        if (mLaurels[i]->mDriver == driver)
            return;
    }

    mLaurels[mCount] = new Laurel(driver);
    mCount++;
}

void LaurelMgr::reset() {
    for (int i = 0; i < mCount; i++)
        mLaurels[i]->updateOffset();
}

void LaurelMgr::update(DriverModel *driver) {
    for (int i = 0; i < mCount; i++) {
        if (mLaurels[i]->mDriver == driver) {
            mLaurels[i]->update(0);
            break;
        }
    }
}

void LaurelMgr::createModel(DriverModel *driver, JKRSolidHeap *heap, u32 a, u32 b) {
    for (int i = 0; i < mCount; i++) {
        if (mLaurels[i]->mDriver == driver) {
            mLaurels[i]->createModel(heap, a, b);
            if (i == 0) {
                mLaurels[0]->makeSharedDL();
                mLaurels[0]->lock();
            }
            break;
        }
    }
}

void LaurelMgr::setCurrentViewNo(DriverModel *driver, u32 viewNo) {
    for (int i = 0; i < mCount; i++) {
        if (mLaurels[i]->mDriver == driver) {
            mLaurels[i]->setCurrentViewNo(viewNo);
            break;
        }
    }
}
