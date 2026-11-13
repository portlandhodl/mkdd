#include <stdio.h>
#include <string.h>

#include "dolphin/mtx.h"
#include "dolphin/os.h"

#include "JSystem/JUtility/JUTAssert.h"

#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/J3DEfctBomb.h"
#include "Sato/J3DEfctHeart.h"
#include "Sato/J3DEfctIceRock.h"
#include "Sato/J3DEfctSplashDown.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/J3DEffectMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/ObjUtility.h"

J3DEffectMgr *J3DEffectMgr::sJ3DMgr;
J3DEfctKartAnmMgr *J3DEfctKartAnmMgr::sJ3DKartAnmMgr;

namespace {

typedef const char *(*GetBmdNameFn)();
typedef J3DEffectObj *(*NewFn)();
typedef void (*LoadAnmFn)();

struct EfctFunc
{
    GetBmdNameFn mGetBmdName;
    NewFn mNew;
    LoadAnmFn mLoadAnm;
    int mCount;
};

u32 sBombEfctNumAtNormal = 5;
u32 sBombEfctNumAtMiniGameBomb = 60;

EfctFunc EfctFuncTbl[5] = {
    { &J3DEffectObj::GetBmdName<J3DEfctBomb>, &J3DEffectObj::New<J3DEfctBomb>, &J3DEffectObj::LoadAnm<J3DEfctBomb>, sBombEfctNumAtNormal },
    { &J3DEffectObj::GetBmdName<J3DEfctSplashDown>, &J3DEffectObj::New<J3DEfctSplashDown>, &J3DEffectObj::LoadAnm<J3DEfctSplashDown>, 8 },
    { &J3DEffectObj::GetBmdName<J3DEfctMudSplashDown>, &J3DEffectObj::New<J3DEfctMudSplashDown>, &J3DEffectObj::LoadAnm<J3DEfctMudSplashDown>, 8 },
    { &J3DEffectObj::GetBmdName<J3DEfctIceRock>, &J3DEffectObj::New<J3DEfctIceRock>, &J3DEffectObj::LoadAnm<J3DEfctIceRock>, 8 },
    { &J3DEffectObj::GetBmdName<J3DEfctHeart>, &J3DEffectObj::New<J3DEfctHeart>, &J3DEffectObj::LoadAnm<J3DEfctHeart>, 6 },
};

const char *BrkEfctCharName[3] = { "drv0", "drv1", "drv2" };
const char *BrkEfctSimpleKartName[3] = { "krt0", "krt1", "krt2" };
const char *BrkEfctEffectName[3] = { "star", "dark", "frez" };

} // namespace

#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

DECOMP_INLINE PermissionCounter::PermissionCounter() : mCount(0), mPermission(0x14), mLink(this) {}

DECOMP_INLINE PermissionCounter::~PermissionCounter() {}

int GetJ3DEfctNumAtMiniGameBomb()
{
    return sBombEfctNumAtMiniGameBomb;
}

void J3DEffectObj::setRTMtx(Mtx mtx)
{
    PSMTXCopy(mtx, mMtx);
}

J3DEffectMgr::J3DEffectMgr()
{
    sJ3DMgr = this;

    for (int i = 0; i < 5; i++) {
        mFreeList[i].initiate();
    }

    mObserver = new PermissionCounterObserver();

    mCounters = new PermissionCounter[RaceMgr::getManager()->getKartNumber()];
    for (u8 i = 0; i < RaceMgr::getManager()->getKartNumber(); i++) {
        mCounters[i].mPermission = 30;
    }

    mActiveList.initiate();

    loadJ3DdataAll();

    EfctFuncTbl[0].mCount = sBombEfctNumAtNormal;
    if (RaceMgr::getManager()->getRaceMode() == BOMB_BATTLE) {
        EfctFuncTbl[0].mCount = sBombEfctNumAtMiniGameBomb;
    }

    createAll();
}

void J3DEffectMgr::createModel(JKRSolidHeap *heap, u32 size)
{
    JSULink<J3DEffectObj> *link;
    for (int i = 0; i < 5; i++) {
        if ((link = mFreeList[i].getFirst()) != nullptr) {
            for (; link != nullptr; link = link->getNext()) {
                link->getObject()->mModel.createModel(heap, size, 0);
                link->getObject()->onlyinit();
            }
        }
    }
}

void J3DEffectMgr::reset()
{
    for (int i = 0; i < RaceMgr::getManager()->getKartNumber(); i++) {
        mCounters[i].mCount = 0;
    }

    JSULink<J3DEffectObj> *link = mActiveList.getFirst();
    while (link != nullptr) {
        remove(link->getObject());
        link = mActiveList.getFirst();
    }
    mActiveList.initiate();

    mObserver->reset();
}

void J3DEffectMgr::loadJ3DdataAll()
{
    for (int i = 0; i < 5; i++) {
        register char *name = (char *)EfctFuncTbl[i].mGetBmdName();
        void *data;
        if (RaceMgr::getManager()->getRaceMode() == BOMB_BATTLE && i == 0) {
            data = ObjUtility::getPtrCourseArc(name);
        } else {
            data = ObjUtility::getPtrMainArc(name);
        }
        mModelDataAll[i] = J3DAnmObjBase::loadJ3DModelData(data, 0x19300010);
        ExModel::patchModelData(mModelDataAll[i]);
        EfctFuncTbl[i].mLoadAnm();
    }
}

void J3DEffectMgr::createAll()
{
    NewFn *newFn;
    int i;
    int *count;
    for (i = 0; i < 5; i++) {
        newFn = &EfctFuncTbl[i].mNew;
        count = &EfctFuncTbl[i].mCount;
        for (int j = 0; j < *count; j++) {
            J3DEffectObj *obj = (*newFn)();
            setModelDataAppend(obj, i);
        }
    }
}

void J3DEffectMgr::append(int type)
{
    J3DEffectObj *obj = nullptr;
    JSULink<J3DEffectObj> *link = mFreeList[type].getFirst();
    if (link != nullptr) {
        obj = link->getObject();
        if (obj != nullptr) {
            obj->init();
            mActiveList.append(&obj->mLink);
        }
    }
}

void J3DEffectMgr::remove(J3DEffectObj *obj)
{
    mFreeList[obj->mType].append(&obj->mLink);
}

J3DEffectObj *J3DEffectMgr::setEffectKart(int kartNo, int efctID, Mtx mtx)
{
    if (KartInfo::KartCharacter::convPlayerKind(RaceMgr::getManager()->getKartInfo(kartNo)->getPad()) == 4) {
        return nullptr;
    }

    if (efctID == 1) {
        PermissionCounter *counter = &mCounters[kartNo];
        if (counter->mLink.getList() != nullptr) {
            return nullptr;
        }
        mObserver->append(&counter->mLink);
    }

    return setEffect(efctID, mtx, kartNo);
}

J3DEffectObj *J3DEffectMgr::setEffectSplash(CrsGround *ground, int kartNo, Mtx mtx)
{
    if (KartInfo::KartCharacter::convPlayerKind(RaceMgr::getManager()->getKartInfo(kartNo)->getPad()) == 4) {
        return nullptr;
    }

    PermissionCounter *counter = &mCounters[kartNo];
    if (counter->mLink.getList() != nullptr) {
        return nullptr;
    }
    mObserver->append(&counter->mLink);

    int efctID = -1;
    switch (ground->getSplashID()) {
    case 1:
    case 2:
        efctID = 1;
        break;
    case 3:
        efctID = 2;
        break;
    }

    if (efctID == -1) {
        return nullptr;
    }

    return setEffect(efctID, mtx, kartNo);
}

J3DEffectObj *J3DEffectMgr::setEffect(int efctID, Mtx mtx, int param)
{
    J3DEffectObj *obj = getFreeObj(efctID);
    if (obj != nullptr) {
        PSMTXCopy(mtx, obj->mMtx);
        obj->mKartNo = param;
    }
    return obj;
}

void J3DEffectMgr::setModelDataAppend(J3DEffectObj *obj, int type)
{
    J3DModelData *modelData = mModelDataAll[type];
    obj->mType = type;
    obj->mModel.setModelData(modelData);
    mFreeList[type].append(&obj->mLink);
}

void J3DEffectMgr::calc()
{
    mObserver->calc();

    for (JSULink<J3DEffectObj> *link = mActiveList.getFirst(); link != nullptr; link = link->getNext()) {
        link->getObject()->calc();
        if (link->getObject()->mState == 1) {
            mFreeList[link->getObject()->mType].append(&link->getObject()->mLink);
        }
    }
}

void J3DEffectMgr::update()
{
    for (JSULink<J3DEffectObj> *link = mActiveList.getFirst(); link != nullptr; link = link->getNext()) {
        link->getObject()->update();
    }
}

void J3DEffectMgr::viewCalc(u32 viewNo)
{
    for (JSULink<J3DEffectObj> *link = mActiveList.getFirst(); link != nullptr; link = link->getNext()) {
        link->getObject()->mModel.viewCalc(viewNo);
    }
}

void J3DEffectMgr::setCurrentViewNo(u32 viewNo)
{
    for (JSULink<J3DEffectObj> *link = mActiveList.getFirst(); link != nullptr; link = link->getNext()) {
        link->getObject()->mModel.setCurrentViewNo(viewNo);
    }
}

J3DEfctKartAnmMgr::J3DEfctKartAnmMgr()
{
    sJ3DKartAnmMgr = this;
    initSimpleBrkEfctAnimation();
    loadSimpleBrkEfctAnimation();
}

void J3DEfctKartAnmMgr::initSimpleBrkEfctAnimation()
{
    for (u8 i = 0; i < 3; i++) {
        mCharBrkAnm[i] = new void *[3];
        mKartBrkAnm[i] = new void *[3];
        for (u8 j = 0; j < 3; j++) {
            mCharBrkAnm[i][j] = nullptr;
            mKartBrkAnm[i][j] = nullptr;
        }
    }
}

void J3DEfctKartAnmMgr::loadSimpleBrkEfctAnimation()
{
    for (u32 i = 0; i < 3; i++) {
        for (u8 j = 0; j < 3; j++) {
            loadSimpleCharBrkEfctFile(i, j);
            loadSimpleKartPartsBrkEfctFile(i, j);
        }
    }
}

void J3DEfctKartAnmMgr::loadSimpleCharBrkEfctFile(u32 id, u8 index)
{
    char buf[64];
    snprintf(buf, 64, "/Effect/driver/%s_%s.brk", getBrkEfctCharName((u8)index), getBrkEfctEffectName(id));
    void *data = ObjUtility::getPtrMainArc(buf);
    if (data != nullptr) {
        mCharBrkAnm[index][id] = data;
    }
}

void J3DEfctKartAnmMgr::loadSimpleKartPartsBrkEfctFile(u32 id, u8 index)
{
    char buf[64];
    snprintf(buf, 64, "/Effect/kart/%s_%s.brk", getBrkEfctSimpleKartName((u8)index), getBrkEfctEffectName(id));
    void *data = ObjUtility::getPtrMainArc(buf);
    if (data != nullptr) {
        mKartBrkAnm[index][id] = data;
    }
}

const char *J3DEfctKartAnmMgr::getBrkEfctCharName(u8 index)
{
#line 574 "J3DEffectMgr.cpp"
    JUT_MINMAX_ASSERT(0, index, 3)
    return BrkEfctCharName[index];
}

const char *J3DEfctKartAnmMgr::getBrkEfctSimpleKartName(u8 index)
{
#line 588 "J3DEffectMgr.cpp"
    JUT_MINMAX_ASSERT(0, index, 3)
    return BrkEfctSimpleKartName[index];
}

const char *J3DEfctKartAnmMgr::getBrkEfctEffectName(u32 efctID)
{
#line 602 "J3DEffectMgr.cpp"
    JUT_MINMAX_ASSERT(0, efctID, 3)
    return BrkEfctEffectName[efctID];
}

void *J3DEfctKartAnmMgr::getSimpleCharBrkAnm(u32 id, u8 index)
{
#line 616 "J3DEffectMgr.cpp"
    JUT_MINMAX_ASSERT(0, index, 3)
    return mCharBrkAnm[index][id];
}

void *J3DEfctKartAnmMgr::getSimpleKartBrkAnm(u32 id, u8 index)
{
#line 630 "J3DEffectMgr.cpp"
    JUT_MINMAX_ASSERT(0, index, 3)
    return mKartBrkAnm[index][id];
}
