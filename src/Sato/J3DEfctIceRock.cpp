#include "Sato/J3DEfctIceRock.h"
#include "Kaneshige/KartInfo.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/J3DEffectMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/ObjUtility.h"

#include <JSystem/JAudio/JASFakeMatch2.h>
#include <JSystem/JGeometry.h>
#include <JSystem/JUtility/JUTAssert.h>

J3DAnmTransform *J3DEfctIceRock::sBckAnmTrans[1] = { 0 };
J3DMtxCalc *J3DEfctIceRock::sBckMtxCalc[1] = { 0 };
J3DAnmTextureSRTKey *J3DEfctIceRock::sTexSRTKey;

struct IceOffsetData {
    f32 mScaleX;
    f32 mScaleY;
    f32 mScaleZ;
    f32 mOffsetY;
    f32 mOffsetZ;
};

const char *sJPAFileName[3] = {
    "mk_freeze_d",
    "mk_freeze_b",
    "mk_freeze_c",
};

IceOffsetData sIceOffsetData[21] = {
    { 0.92f, 0.92f, 0.92f, -80.0f, -10.0f },
    { 1.1f, 1.1f, 1.3f, -90.0f, 10.0f },
    { 0.95f, 0.95f, 1.0f, -70.0f, 5.0f },
    { 0.9f, 0.9f, 0.9f, -100.0f, 0.0f },
    { 0.95f, 0.95f, 0.95f, -80.0f, 5.0f },
    { 0.92f, 0.92f, 0.92f, -60.0f, -10.0f },
    { 1.3f, 1.3f, 1.5f, -90.0f, 25.0f },
    { 1.25f, 1.1f, 1.25f, -100.0f, 0.0f },
    { 0.92f, 0.92f, 0.92f, -80.0f, -10.0f },
    { 0.92f, 0.92f, 1.05f, -70.0f, 25.0f },
    { 0.95f, 0.95f, 1.0f, -70.0f, 5.0f },
    { 0.9f, 0.9f, 0.9f, -100.0f, 0.0f },
    { 0.95f, 0.95f, 0.95f, -80.0f, 5.0f },
    { 0.92f, 0.92f, 0.92f, -60.0f, -10.0f },
    { 1.1f, 1.0f, 1.2f, -80.0f, 0.0f },
    { 0.92f, 0.92f, 0.92f, -75.0f, 0.0f },
    { 0.92f, 0.92f, 0.92f, -90.0f, 0.0f },
    { 0.92f, 0.92f, 0.92f, -90.0f, 0.0f },
    { 1.1f, 1.1f, 1.1f, -95.0f, 0.0f },
    { 1.1f, 1.1f, 1.1f, -95.0f, 0.0f },
    { 1.2f, 1.2f, 1.5f, -90.0f, -20.0f },
};


J3DEfctIceRock::J3DEfctIceRock() {}

void J3DEfctIceRock::loadAnmData() {
    J3DModelData *mdlData = J3DEffectMgr::sJ3DMgr->getModelData();
    for (u8 i = 0; i < 1; i++) {
        void *bck = ObjUtility::getPtrMainArc((char *)getBckFileName((u8)i));
        J3DAnmObjTrans::setupTransAnmData(&sBckAnmTrans[i], &sBckMtxCalc[i], mdlData, bck);
    }
    void *btk = ObjUtility::getPtrMainArc((char *)getBtkFileName());
    J3DAnmObjMaterial::setupTexSRTAnmData(&sTexSRTKey, mdlData, btk);
}

const char *J3DEfctIceRock::getBmdFileName() {
    return "/Effect/IceRock.bmd";
}

const char *J3DEfctIceRock::getBckFileName(u8 index) {
    const char *name = nullptr;
    if (index == 0) name = "/Effect/IceRock_app.bca";
    return name;
}

const char *J3DEfctIceRock::getBtkFileName() {
    return "/Effect/IceRock.btk";
}

static inline void setIceRockTexAnm(J3DAnmObjMatTexSRT *tex, J3DAnmTextureSRTKey *anm) {
    tex->mAnmBase = anm;
    tex->initFrameCtrl(tex->mAnmBase);
}

void J3DEfctIceRock::onlyinit() {
    mAnmCtrl.mTrans = new AnmControlTrans();
    mAnmCtrl.mTrans->initAnm(1, &mModel);

    for (u8 i = 0; i < 1; i++) {
        mAnmCtrl.mTrans->registration(i, sBckAnmTrans[i], sBckMtxCalc[i]);
    }

    MatAnmInfo info;
    info.mNumBtks = 0;
    info.mNumBrks = 0;
    info.mNumBtps = 0;
    info.mNumBpks = 0;
    info.mNumBtks = 1;
    mAnmCtrl.InitRegistrationMat(info, &mModel);

    // Separating getObjIce's check from the mObj reload reproduces MWCC's inlined register allocation.
    setIceRockTexAnm((mAnmCtrl.mMatData->mpCtrlSRT->getObjIce(0),
                      *(J3DAnmObjMatTexSRT **)((u8 *)mAnmCtrl.mMatData->mpCtrlSRT + 0x18)),
                     sTexSRTKey);
    mAnmCtrl.mMatData->mpCtrlSRT->mNowAnmNo = 0;
}

void J3DEfctIceRock::init() {
    mState = 0;
    mAnmCtrl.Reset();
    _f2 = 0;
    _f0 = 0;
}

void J3DEfctIceRock::calc() {
    mAnmCtrl.FrameProc();
    setModelMtx();
}

void J3DEfctIceRock::setTrigger() {
    mState = 1;
    JGeometry::TVec3f pos;
    ObjUtility::getKartPos(mKartNo, &pos);

    for (u8 i = 0; i < 3; i++) {
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(sJPAFileName[i], pos);
        if (emt != nullptr && !TstEmitterModeNotAvailableFlg(mKartNo, emt)) {
#line 170 "RaceMgr.h"
            JUT_MINMAX_ASSERT(0, mKartNo, 8);
        }
    }
}

void J3DEfctIceRock::update() {
    mAnmCtrl.SetAnimation();
    mModel.update(0);
}

void J3DEfctIceRock::setModelMtx() {
    IceOffsetData *data = &sIceOffsetData[RaceMgr::getManager()->getKartInfo(mKartNo)->getKartID()];
    Mtx scaleMtx;
    Mtx transMtx;
    Mtx mtx;

    PSMTXScale(scaleMtx, data->mScaleX, data->mScaleY, data->mScaleZ);
    PSMTXTrans(transMtx, 0.0f, data->mOffsetY, data->mOffsetZ);
    PSMTXConcat(transMtx, scaleMtx, mtx);
    PSMTXConcat(ObjUtility::getKartBodyMtx(mKartNo), mtx, mtx);
    mModel.setBaseTRMtx(mtx);
}


inline J3DEfctIceRock::~J3DEfctIceRock() {}

