#include "Sato/JPEffectMgr.h"

#include <dolphin/mtx.h>
#include <dolphin/os.h>

#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/JGadget/hashcode.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/JUtility/JUTAssert.h"

#include "Kaneshige/KartInfo.h"
#include "Kaneshige/Course/Course.h"
#include "Kaneshige/Course/CrsArea.h"
#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/SysDebug.h"

#include "Osako/ResMgr.h"
#include "Osako/system.h"

#include "Sato/ItemObjMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/ObjUtility.h"
#include "Sato/stMath.h"

#include "Yamamoto/kartCamera.h"
#include "Yamamoto/kartCtrl.h"

namespace
{
    u16 PartilceMax = 1024;
    u16 AwardDemoPartilceNumOffset = 600;
    u16 ThunderPartilceMax = 512;
    u16 CamParticleMax = 200;
    u16 RacePrev2DParticleMax = 250;
    u16 Race2DParticleMax = 300;
    u16 SnowParticleMax = 800;
    u16 SnowEmitterMax = 3;
    u16 EmitterMax = 128;
    u16 ThunderEmitterMax = 32;
    u16 CamEmitterMax = 8;
    u16 Race2DEmitterMax = 32;
    f32 sSnowSpdRatio = 1.0f;
    u8 sSnowAlphaAdd = 5;

    const f32 sPad[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    JPSnowEmitterCallBack JPSnowECB;
} // namespace

#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

void JPSnowEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = emt->mpUserWork;
    JGeometry::TVec3f camPos(*KartCtrl::getKartCtrl()->getKartCam(kartNo)->GetCameraPos());
    JGeometry::TVec3f lookPos(*KartCtrl::getKartCtrl()->getKartCam(kartNo)->GetCameraLookPos());
    JGeometry::TVec3f dir(lookPos);
    dir.sub(camPos);
    stVecNormalize(&dir);
    JGeometry::TPos3f mtx;
    stMakeDirectionMtx(&mtx, dir, 90);
    JPASetRMtxfromMtx(mtx, emt->mGlobalRot);
    emt->mGlobalTrs = camPos;
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(KartCtrl::getKartCtrl()->getKartCam(kartNo)->mTargetIdx, &kartPos);
    CrsArea area;
    area.search(2, kartPos);
    int alpha = sSnowAlphaAdd;
    if (RaceMgr::sRaceManager->getRacePhase() == PHASE_CRS_DEMO) {
        alpha = 255;
    }
    if (area.getArea() != nullptr) {
        mAlpha -= alpha;
    } else {
        mAlpha += alpha;
    }
    if (mAlpha < 0) {
        mAlpha = 0;
    } else if (mAlpha > 255) {
        mAlpha = 255;
    }
    emt->setGlobalAlpha(mAlpha);
}

void JPSnowEmitterCallBack::draw(JPABaseEmitter *)
{
}

JPEffectMgr *JPEffectMgr::mThis;

JPEffectMgr::JPEffectMgr(JKRHeap *heap, bool p2, u8 mode)
{
    mThis = this;
    mResourceMgr = nullptr;
    mCrsResourceMgr = nullptr;
    mEmitterMgr = nullptr;
    mEm2DMgr = nullptr;
    mPrev2DMgr = nullptr;
    mPrev2DMultiEmitterMgr = nullptr;
    mThunderMgr = nullptr;
    mSnow = 0;
    mSnowPos = nullptr;
    mSnowVel = nullptr;
    if (heap == nullptr) {
        mHeap = JKRHeap::sCurrentHeap;
    } else {
        mHeap = heap;
    }
    if (!p2) {
        mMode = RaceMgr::sRaceManager->mRaceInfo->getConsoleNumber();
        reset();
        JPEffectPerformer::init();
        JPEffectPerformer::loadCourseParam(RaceMgr::sRaceManager->getCourse()->getCourseID());
    } else {
        mMode = mode;
        create_forMenu();
    }
}

void JPEffectMgr::reset()
{
    bool crs = false;
    if (mCrsResourceMgr == nullptr && RaceMgr::sRaceManager->getCourse()->getCourseID() == 69) {
        const void *res = ResMgr::getPtr(ResMgr::mcArcCourse, "JPACrsEndingEffect.jpc");
        mCrsResourceMgr = new JPAResourceManager(res, mHeap);
        crs = true;
    }
    if (mResourceMgr == nullptr) {
        if (RaceMgr::sRaceManager->getCourse()->getCrsData()->isSnowEnabled()) {
            if (RaceMgr::sRaceManager->mRaceInfo->getConsoleNumber() < 2) {
                mSnow = 1;
            }
        }
        SysDebug::getManager()->setHeapGroup("PARTICLE DATA", nullptr);
        void *res = ResMgr::getPtr(ResMgr::mcArcMRAM, "/Effect/JPAEffect.jpc");
        SysDebug::getManager()->setHeapGroup("PARTICLE MGR", nullptr);
        u16 ptclMax = PartilceMax;
        if (RaceMgr::sRaceManager->mRaceInfo->getRaceMode() == AWARD_DEMO) {
            ptclMax += AwardDemoPartilceNumOffset;
        }
        mResourceMgr = new JPAResourceManager(res, mHeap);
        mEmitterMgr = new (mHeap, 0) JPAEmitterManager(ptclMax, EmitterMax, mHeap, 1, 1);
        mEmitterMgr->entryResourceManager(mResourceMgr, 0);
        mThunderMgr = new (mHeap, 0) JPAEmitterManager(ThunderPartilceMax, ThunderEmitterMax, mHeap, 1, 1);
        mThunderMgr->entryResourceManager(mResourceMgr, 0);
        u16 camPtclMax = CamParticleMax;
        u16 camEmtMax = CamEmitterMax;
        if (mSnow) {
            camPtclMax += SnowParticleMax;
            camEmtMax += SnowEmitterMax;
        }
        mCamEmitterMgr = new JPAEmitterManager *[RaceMgr::sRaceManager->getConsoleNumber()];
        for (u8 i = 0; i < RaceMgr::sRaceManager->getConsoleNumber(); i++) {
            mCamEmitterMgr[i] = new (mHeap, 0) JPAEmitterManager(camPtclMax, camEmtMax, mHeap, 1, 1);
            mCamEmitterMgr[i]->entryResourceManager(mResourceMgr, 0);
        }
        u16 ptcl2d = Race2DParticleMax;
        u16 emt2d = Race2DEmitterMax;
        JPAResourceManager *res2d = mResourceMgr;
        if (crs) {
            res2d = mCrsResourceMgr;
            emt2d = EmitterMax;
            ptcl2d = PartilceMax;
        }
        create2DEmtMgr(res2d, emt2d, ptcl2d);
    } else {
        mEmitterMgr->forceDeleteAllEmitter();
        for (u8 i = 0; i < RaceMgr::sRaceManager->getConsoleNumber(); i++) {
            mCamEmitterMgr[i]->forceDeleteAllEmitter();
            mPrev2DMultiEmitterMgr[i]->forceDeleteAllEmitter();
        }
        mEm2DMgr->forceDeleteAllEmitter();
        mPrev2DMgr->forceDeleteAllEmitter();
        mThunderMgr->forceDeleteAllEmitter();
        JPEffectPerformer::reset();
        if (mSnow) {
            createSnowEmt();
            resetSnowCamVel();
            JPSnowECB.mAlpha = 255;
        }
        if (RaceMgr::sRaceManager->getCourse()->getCourseID() == 49) {
            createRainbowEmt();
        }
    }
}

void JPEffectMgr::reset_forMenu()
{
    mEm2DMgr->forceDeleteAllEmitter();
    mPrev2DMgr->forceDeleteAllEmitter();
    for (u8 i = 0; i < mMode; i++) {
        mPrev2DMultiEmitterMgr[i]->forceDeleteAllEmitter();
    }
}

u16 JPEffectMgr::getHashValue(const char *name)
{
    return JGadget::getHashCode(name);
}

void JPEffectMgr::create2DEmtMgr(JPAResourceManager *resMgr, u16 emtMax, u16 ptclMax)
{
    mEm2DMgr = new (mHeap, 0) JPAEmitterManager(ptclMax, emtMax, mHeap, 1, 1);
    mEm2DMgr->entryResourceManager(resMgr, 0);
    mPrev2DMgr = new (mHeap, 0) JPAEmitterManager(RacePrev2DParticleMax, Race2DEmitterMax, mHeap, 1, 1);
    mPrev2DMgr->entryResourceManager(mResourceMgr, 0);
    mPrev2DMultiEmitterMgr = new JPAEmitterManager *[mMode];
    for (u8 i = 0; i < mMode; i++) {
        mPrev2DMultiEmitterMgr[i] = new (mHeap, 0) JPAEmitterManager(RacePrev2DParticleMax, Race2DEmitterMax, mHeap, 1, 1);
        mPrev2DMultiEmitterMgr[i]->entryResourceManager(mResourceMgr, 0);
    }
}

void JPEffectMgr::create_forMenu()
{
    JPAResourceManager *resMgr;
    u32 i;
    u16 emtMax, ptclMax;
    void *res = ResMgr::getPtr(ResMgr::mcArcMRAM, "/Effect/JPAEffect.jpc");
    mResourceMgr = new JPAResourceManager(res, mHeap);
    ptclMax = Race2DParticleMax;
    emtMax = Race2DEmitterMax;
    resMgr = mResourceMgr;
    mEm2DMgr = new (mHeap, 0) JPAEmitterManager(ptclMax, emtMax, mHeap, 1, 1);
    mEm2DMgr->entryResourceManager(resMgr, 0);
    mPrev2DMgr = new (mHeap, 0) JPAEmitterManager(RacePrev2DParticleMax, Race2DEmitterMax, mHeap, 1, 1);
    mPrev2DMgr->entryResourceManager(mResourceMgr, 0);
    mPrev2DMultiEmitterMgr = new JPAEmitterManager *[mMode];
    for (i = 0; (u8)i < mMode; i++) {
        mPrev2DMultiEmitterMgr[(u8)i] = new (mHeap, 0) JPAEmitterManager(RacePrev2DParticleMax, Race2DEmitterMax, mHeap, 1, 1);
        mPrev2DMultiEmitterMgr[(u8)i]->entryResourceManager(mResourceMgr, 0);
    }
}

JPABaseEmitter *JPEffectMgr::createEmt(u16 hash, const JGeometry::TVec3f &pos)
{
    const JGeometry::TVec3f *p = &pos;
    return mEmitterMgr->createSimpleEmitter(*p, (u16)hash, nullptr, nullptr);
}

JPABaseEmitter *JPEffectMgr::createEmt(const char *name, const JGeometry::TVec3f &pos)
{
    u16 hash = JGadget::getHashCode(name);
    return mEmitterMgr->createSimpleEmitter(pos, hash, nullptr, nullptr);
}

JPABaseEmitter *JPEffectMgr::createEmtCameraClip(const char *name, const JGeometry::TVec3f &pos)
{
    u16 hash = JGadget::getHashCode(name);
    JPABaseEmitter *emt = mEmitterMgr->createSimpleEmitter(pos, hash, nullptr, nullptr);
    if (clipEmitterCameraNumFlg(emt)) {
        emt = nullptr;
    }
    return emt;
}

JPABaseEmitter *JPEffectMgr::createEmt2D(const char *name, const JGeometry::TVec3f &pos)
{
    u16 hash = JGadget::getHashCode(name);
    return mEm2DMgr->createSimpleEmitter(pos, hash, nullptr, nullptr);
}

JPABaseEmitter *JPEffectMgr::createEmtBefore2D(const char *name, const JGeometry::TVec3f &pos)
{
    u16 hash = JGadget::getHashCode(name);
    return mPrev2DMgr->createSimpleEmitter(pos, hash, nullptr, nullptr);
}

JPABaseEmitter *JPEffectMgr::createEmt(u16 hash, const JGeometry::TVec3f &pos, Mtx mtx)
{
    const JGeometry::TVec3f *p = &pos;
    JPABaseEmitter *emt = mEmitterMgr->createSimpleEmitter(*p, (u16)hash, nullptr, nullptr);
    Mtx mtxT;
    PSMTXTranspose(mtx, mtxT);
    JPASetRMtxfromMtx(mtxT, emt->mGlobalRot);
    return emt;
}

JPABaseEmitter *JPEffectMgr::createEmt(const char *name, const JGeometry::TVec3f &pos, Mtx mtx)
{
    u16 hash = JGadget::getHashCode(name);
    JPABaseEmitter *emt = mEmitterMgr->createSimpleEmitter(pos, hash, nullptr, nullptr);
    Mtx mtxT;
    PSMTXTranspose(mtx, mtxT);
    JPASetRMtxfromMtx(mtxT, emt->mGlobalRot);
    return emt;
}

JPABaseEmitter *JPEffectMgr::createEmtThunder(const char *name, const JGeometry::TVec3f &pos)
{
    u16 hash = JGadget::getHashCode(name);
    return mThunderMgr->createSimpleEmitter(pos, hash, nullptr, nullptr);
}

JPABaseEmitter *JPEffectMgr::createEmtCam(const char *name, const JGeometry::TVec3f &pos, u8 camNo)
{
    u16 hash = JGadget::getHashCode(name);
    return mCamEmitterMgr[camNo]->createSimpleEmitter(pos, hash, nullptr, nullptr);
}

JPABaseEmitter *JPEffectMgr::createEmtBefore2DMulti(const char *name, const JGeometry::TVec3f &pos, u8 idx)
{
    u16 hash = JGadget::getHashCode(name);
    return mPrev2DMultiEmitterMgr[idx]->createSimpleEmitter(pos, hash, nullptr, nullptr);
}

void JPEffectMgr::deleteEmt(JPABaseEmitter *emt)
{
    mEmitterMgr->forceDeleteEmitter(emt);
}

void JPEffectMgr::deleteEmtThunder(JPABaseEmitter *emt)
{
    mThunderMgr->forceDeleteEmitter(emt);
}

void JPEffectMgr::deleteEmt2D(JPABaseEmitter *emt)
{
    mEm2DMgr->forceDeleteEmitter(emt);
}

void JPEffectMgr::deleteEmtBefore2D(JPABaseEmitter *emt)
{
    mPrev2DMgr->forceDeleteEmitter(emt);
}

void JPEffectMgr::deleteEmtCam(JPABaseEmitter *emt, u8 camNo)
{
    mCamEmitterMgr[camNo]->forceDeleteEmitter(emt);
}

void JPEffectMgr::deleteEmtBefore2DMulti(JPABaseEmitter *emt, u8 idx)
{
    mPrev2DMultiEmitterMgr[idx]->forceDeleteEmitter(emt);
}

u8 JPEffectMgr::clipEmitterCameraNumFlg(JPABaseEmitter *emt)
{
    u32 ret = 0;
    if ((emt->mResource->getDyn()->getResUserWork() & 2) && RaceMgr::sRaceManager->mRaceInfo->getStatusNumber() > 1) {
        mEmitterMgr->forceDeleteEmitter(emt);
        ret = 1;
    }
    return (u8)ret;
}

void JPEffectMgr::calc()
{
    if (mSnow) {
        calcSnowCamVel();
    }
    JPEffectPerformer::calc();
    mEmitterMgr->calc();
    mThunderMgr->calc();
    for (u8 i = 0; i < RaceMgr::sRaceManager->getConsoleNumber(); i++) {
        mCamEmitterMgr[i]->calc();
    }
}

void JPEffectMgr::calc_forMenu()
{
    mEm2DMgr->calc();
    mPrev2DMgr->calc();
    for (u8 i = 0; i < mMode; i++) {
        mPrev2DMultiEmitterMgr[i]->calc();
    }
}

u16 cb(u16 hash)
{
    u32 h = hash;
    for (u32 i = 0; i < 4; i++) {
        h = h * 31 + i;
        h ^= h >> 8;
    }
    h += 7;
    h += 2;
    h += 3;
    h ^= 0x55;
    h ^= h << 3;
    return (u16)h;
}

void JPEffectMgr::draw(int cameraNo)
{
    KartCam *cam = RaceMgr::sRaceManager->getCamera(cameraNo);
    f32 fovy, aspect;
    aspect = KartCtrl::getKartCtrl()->GetCamAspect(cameraNo);
    fovy = KartCtrl::getKartCtrl()->GetCamFovy(cameraNo);
    JPADrawInfo info(cam->mMtx, fovy, aspect);
    mEmitterMgr->draw(&info);
    mCamEmitterMgr[cameraNo]->draw(&info);
}

void JPEffectMgr::draw_thunder(int cameraNo)
{
    KartCam *cam = RaceMgr::sRaceManager->getCamera(cameraNo);
    f32 fovy, aspect;
    aspect = KartCtrl::getKartCtrl()->GetCamAspect(cameraNo);
    fovy = KartCtrl::getKartCtrl()->GetCamFovy(cameraNo);
    JPADrawInfo info(cam->mMtx, fovy, aspect);
    mThunderMgr->draw(&info);
}

void JPEffectMgr::draw_for2D()
{
    Mtx mtx;
    PSMTXIdentity(mtx);
    f32 l, r, t, b;
    b = System::getOrthoB();
    t = System::getOrthoT();
    r = System::getOrthoR();
    l = System::getOrthoL();
    JPADrawInfo info(mtx, l, r, t, b);
    mEm2DMgr->draw(&info);
}

void JPEffectMgr::draw_forBefore2D()
{
    Mtx mtx;
    PSMTXIdentity(mtx);
    f32 l, r, t, b;
    b = System::getOrthoB();
    t = System::getOrthoT();
    r = System::getOrthoR();
    l = System::getOrthoL();
    JPADrawInfo info(mtx, l, r, t, b);
    mPrev2DMgr->draw(&info);
}

void JPEffectMgr::draw_forBefore2D(const u8 cameraNo)
{
    f32 x, y, w, h;
    if (mMode == 1) {
        x = System::get2DVpX();
        y = System::get2DVpY();
        w = System::get2DVpW();
        h = System::get2DVpH();
    } else if (mMode == 2) {
        x = System::get3DVpDiv2X(cameraNo);
        y = System::get3DVpDiv2Y(cameraNo);
        w = System::get3DVpDiv2W(cameraNo);
        h = System::get3DVpDiv2H(cameraNo);
    } else {
        x = System::get3DVpDiv4X(cameraNo);
        y = System::get3DVpDiv4Y(cameraNo);
        w = System::get3DVpDiv4W(cameraNo);
        h = System::get3DVpDiv4H(cameraNo);
    }
    J2DOrthoGraph ortho(x, y, w, h, -1000.0f, 1000.0f);
    ortho.setPort();
    Mtx mtx;
    PSMTXIdentity(mtx);
    JPADrawInfo info(mtx, x, w, y, h);
    mPrev2DMultiEmitterMgr[cameraNo]->draw(&info);
}

void JPEffectMgr::createSnowEmt()
{
    u8 camNum = RaceMgr::sRaceManager->getConsoleNumber();
    if (mSnowVel == nullptr) {
        mSnowVel = new JGeometry::TVec3f[camNum];
    }
    if (mSnowPos == nullptr) {
        mSnowPos = new JGeometry::TVec3f[camNum];
    }
    for (u32 i = 0; (u8)i < camNum; i++) {
        JGeometry::TVec3f zero(0.0f, 0.0f, 0.0f);
        JPABaseEmitter *emt = mCamEmitterMgr[i & 0xff]->createSimpleEmitter(zero, JGadget::getHashCode("mk_fallSnow01"), 0, 0);
        if (emt) {
            emt->mpUserWork = (u8)i;
            emt->mEmitterCallback = &JPSnowECB;
        }
        mSnowVel[(u8)i].zero();
        mSnowPos[(u8)i].zero();
    }
}

void JPEffectMgr::createAwardDemoEmt(const bool p1)
{
    JGeometry::TVec3f zero(0.0f, 0.0f, 0.0f);
    if (p1) {
        mEmitterMgr->createSimpleEmitter(zero, JGadget::getHashCode("mk_kamifubuki_a"), nullptr, nullptr);
        mEmitterMgr->createSimpleEmitter(zero, JGadget::getHashCode("mk_kamifubuki_b"), nullptr, nullptr);
    } else {
        mEmitterMgr->createSimpleEmitter(zero, JGadget::getHashCode("mk_kamifubuki_c"), nullptr, nullptr);
    }
}

void JPEffectMgr::createRainbowEmt()
{
    JGeometry::TVec3f zero(0.0f, 0.0f, 0.0f);
    mEmitterMgr->createSimpleEmitter(zero, JGadget::getHashCode("mk_rbCannon_a"), nullptr, nullptr);
}

void JPEffectMgr::createCupStartDemoEmt()
{
    JGeometry::TVec3f zero(0.0f, 0.0f, 0.0f);
    const char *name = nullptr;
    switch (RaceMgr::sRaceManager->getCourse()->getCourseID() - 36) {
    case 0:
        name = "mk_cdkfLuigi";
        break;
    case 4:
        name = "mk_cdkfKbridge";
        break;
    case 15:
        name = "mk_cdkfSnow";
        break;
    case 7:
        name = "mk_cdkfWario";
        break;
    }
    if (name != nullptr && RaceMgr::sRaceManager->mRaceInfo->getGpStageNo() == 0) {
        mEmitterMgr->createSimpleEmitter(zero, JGadget::getHashCode(name), nullptr, nullptr);
    }
}

void JPEffectMgr::createStaffRollEfct()
{
    JGeometry::TVec3f zero(0.0f, 0.0f, 0.0f);
    for (u8 i = 0; i < 8; i++) {
        mEmitterMgr->createSimpleEmitter(zero, cb(i), nullptr, nullptr);
        mEmitterMgr->createSimpleEmitter(zero, cb(i), nullptr, nullptr);
    }
    mEmitterMgr->calc();
    getSnowEmtSpdRatio();
}

void JPEffectMgr::resetSnowCamVel()
{
    for (u8 i = 0; i < RaceMgr::sRaceManager->getConsoleNumber(); i++) {
        mSnowVel[i].zero();
        mSnowPos[i].zero();
    }
}

void JPEffectMgr::calcSnowCamVel()
{
    for (u8 i = 0; i < RaceMgr::sRaceManager->mRaceInfo->getConsoleNumber(); i++) {
        KartCam *cam = KartCtrl::getKartCtrl()->getKartCam(i);
        JGeometry::TVec3f camPos(*cam->GetCameraPos());
        mSnowVel[i].sub(camPos, mSnowPos[i]);
        mSnowVel[i].scale(sSnowSpdRatio);
        mSnowPos[i].add(mSnowVel[i]);
    }
}

f32 getSnowEmtSpdRatio()
{
    return sSnowSpdRatio;
}

