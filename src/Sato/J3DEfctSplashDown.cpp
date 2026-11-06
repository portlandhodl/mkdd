#include "Sato/J3DEfctSplashDown.h"

#include <JSystem/JGeometry.h>
#include <JSystem/JUtility/JUTAssert.h>

#include "Kaneshige/RaceMgr.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/J3DEffectMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/ObjUtility.h"
#include "Sato/stMath.h"

J3DAnmTransform *J3DEfctSplashDown::sBckAnmTrans[1] = { nullptr };
J3DMtxCalc *J3DEfctSplashDown::sBckMtxCalc[1] = { nullptr };
J3DAnmTextureSRTKey *J3DEfctSplashDown::sTexSRTKey = nullptr;
J3DAnmTevRegKey *J3DEfctSplashDown::sTevRegKey = nullptr;

J3DAnmTransform *J3DEfctMudSplashDown::sBckAnmTrans[1] = { nullptr };
J3DMtxCalc *J3DEfctMudSplashDown::sBckMtxCalc[1] = { nullptr };
J3DAnmTextureSRTKey *J3DEfctMudSplashDown::sTexSRTKey = nullptr;
J3DAnmTevRegKey *J3DEfctMudSplashDown::sTevRegKey = nullptr;


extern __declspec(section ".sdata") J3DAnmTransform *sBckAnmTrans__20J3DEfctMudSplashDown[];
J3DEfctSplashDown::J3DEfctSplashDown() {}

void J3DEfctSplashDown::loadAnmData() {
    J3DModelData *mdlData = J3DEffectMgr::sJ3DMgr->getSplashModelData();
    J3DAnmObjTrans::setupTransAnmData(&sBckAnmTrans[0], &sBckMtxCalc[0], mdlData,
                                      ObjUtility::getPtrMainArc((char *)getBckFileName()));
    J3DAnmObjMaterial::setupTexSRTAnmData(&sTexSRTKey, mdlData,
                                          ObjUtility::getPtrMainArc((char *)getBtkFileName()));
    J3DAnmObjMaterial::setupTevRegAnmData(&sTevRegKey, mdlData,
                                          ObjUtility::getPtrMainArc((char *)getBrkFileName()));
}

const char *J3DEfctSplashDown::getBmdFileName() {
    return "/Effect/mizubashira.bmd";
}

const char *J3DEfctSplashDown::getBckFileName() {
    return "/Effect/mizubashira.bca";
}

const char *J3DEfctSplashDown::getBtkFileName() {
    return "/Effect/mizubashira.btk";
}

const char *J3DEfctSplashDown::getBrkFileName() {
    return "/Effect/mizubashira.brk";
}

void J3DEfctSplashDown::onlyinit() {
    mAnmCtrl.mTrans = new AnmControlTrans();
    mAnmCtrl.mTrans->initAnm(1, &mModel);
    mAnmCtrl.mTrans->registration(0, sBckAnmTrans[0], sBckMtxCalc[0]);

    MatAnmInfo info;
    info.mNumBrks = 0;
    info.mNumBtks = 0;
    info.mNumBtps = 0;
    info.mNumBpks = 0;
    info.mNumBtks = 1;
    info.mNumBrks = 1;
    mAnmCtrl.InitRegistrationMat(info, &mModel);

    J3DAnmObjMatTexSRT *tex = mAnmCtrl.mMatData->mpCtrlSRT->getObjIce(0);
    tex->mAnmBase = sTexSRTKey;
    tex->initFrameCtrl(tex->mAnmBase);
    mAnmCtrl.mMatData->mpCtrlSRT->mNowAnmNo = 0;

    J3DAnmObjMatTevReg *tev = mAnmCtrl.mMatData->mpCtrlTevReg->getObjIce(0);
    tev->mAnmBase = sTevRegKey;
    tev->initFrameCtrl(tev->mAnmBase);
    mAnmCtrl.mMatData->mpCtrlTevReg->mNowAnmNo = 0;
}

void J3DEfctSplashDown::init() {
    _f0 = 0;
    getLife() = 0;
    mAnmCtrl.Reset();
}

void J3DEfctSplashDown::calc() {
    _f0++;
    if (_f0 == 1) {
        f32 z = *(f32 *)((u8 *)this + 0xc0);
        f32 y = *(f32 *)((u8 *)this + 0xb0);
        f32 x = *(f32 *)((u8 *)this + 0xa0);
        JGeometry::TVec3f pos(x, y, z);
        JPEffectMgr *mgr = JPEffectMgr::getEffectMgr();
        JPABaseEmitter *emt = mgr->createEmt(getJPAfilename(), pos);

#line 179
        JUT_MINMAX_ASSERT(0, getKartIndex(), RaceMgr::getManager()->getKartNumber());
        TstEmitterModeNotAvailableFlg(getKartIndex(), emt);

        f32 angle = 3.14f * stGetRnd(3)->get_ufloat_1();
        JGeometry::TVec3f axis(0.0f, 1.0f, 0.0f);
        ((JGeometry::TPos3f *)((u8 *)this + 0x94))->makeRotate(axis, angle);
        *(f32 *)((u8 *)this + 0xa0) = pos.x;
        *(f32 *)((u8 *)this + 0xb0) = pos.y;
        *(f32 *)((u8 *)this + 0xc0) = pos.z;
    }
#line 100

    else if (_f0 >= 100) {
        getLife() = 1;
        return;
    }

    mAnmCtrl.FrameProc();

    J3DModel *model = mModel.getModel();
    model->mModelScale.x = 0.0f;
    model->mModelScale.y = 0.0f;
    model->mModelScale.z = 0.0f;

    mModel.setBaseTRMtx(*(Mtx *)((u8 *)this + 0x94));
}

__declspec(weak) void JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >::makeRotate(
    const JGeometry::TVec3f &rVec, f32 angle) {
    zeroTrans();
    setRotate(rVec, angle);
}

const char *J3DEfctSplashDown::getJPAfilename() {
    return "mk_mizubasira_a";
}

void J3DEfctSplashDown::update() {
    mAnmCtrl.SetAnimation();
    mModel.update(0);
}

J3DEfctMudSplashDown::J3DEfctMudSplashDown() {}

inline J3DEfctSplashDown::~J3DEfctSplashDown() {}

void J3DEfctMudSplashDown::loadAnmData() {
    J3DModelData *mdlData = J3DEffectMgr::sJ3DMgr->getMudModelData();
    J3DAnmObjTrans::setupTransAnmData(&sBckAnmTrans[0], &sBckMtxCalc[0], mdlData,
                                      ObjUtility::getPtrMainArc((char *)getBckFileName()));
    J3DAnmObjMaterial::setupTexSRTAnmData(&sTexSRTKey, mdlData,
                                          ObjUtility::getPtrMainArc((char *)getBtkFileName()));
    J3DAnmObjMaterial::setupTevRegAnmData(&sTevRegKey, mdlData,
                                          ObjUtility::getPtrMainArc((char *)getBrkFileName()));
}
const char *J3DEfctMudSplashDown::getBmdFileName() {
    return "/Effect/mizubashira_mud_a.bmd";
}

const char *J3DEfctMudSplashDown::getBckFileName() {
    return "/Effect/mizubashira_mud_a.bca";
}

const char *J3DEfctMudSplashDown::getBtkFileName() {
    return "/Effect/mizubashira_mud_a.btk";
}

const char *J3DEfctMudSplashDown::getBrkFileName() {
    const char *name = "/Effect/mizubashira_mud_a.brk";
    if (RaceMgr::getManager()->getCourse()->getCourseID() == BOWSER_CASTLE) {
        name = "/Effect/mizubashira_mud_b.brk";
    }
    return name;
}

const char *J3DEfctMudSplashDown::getJPAfilename() {
    const char *name = "mk_mizubasira_b";
    if (RaceMgr::getManager()->getCourse()->getCourseID() == BOWSER_CASTLE) {
        name = "mk_mizubasira_c";
    }
    return name;
}

void J3DEfctMudSplashDown::onlyinit() {
    mAnmCtrl.mTrans = new AnmControlTrans();
    mAnmCtrl.mTrans->initAnm(1, &mModel);
    mAnmCtrl.mTrans->registration(0, sBckAnmTrans[0], sBckMtxCalc[0]);

    MatAnmInfo info;
    info.mNumBrks = 0;
    info.mNumBtks = 0;
    info.mNumBtps = 0;
    info.mNumBpks = 0;
    info.mNumBtks = 1;
    info.mNumBrks = 1;
    mAnmCtrl.InitRegistrationMat(info, &mModel);

    J3DAnmObjMatTexSRT *tex = mAnmCtrl.mMatData->mpCtrlSRT->getObjIce(0);
    tex->mAnmBase = sTexSRTKey;
    tex->initFrameCtrl(tex->mAnmBase);
    mAnmCtrl.mMatData->mpCtrlSRT->mNowAnmNo = 0;

    J3DAnmObjMatTevReg *tev = mAnmCtrl.mMatData->mpCtrlTevReg->getObjIce(0);
    tev->mAnmBase = sTevRegKey;
    tev->initFrameCtrl(tev->mAnmBase);
    mAnmCtrl.mMatData->mpCtrlTevReg->mNowAnmNo = 0;
}

inline J3DEfctMudSplashDown::~J3DEfctMudSplashDown() {}

#include <JSystem/JAudio/JASFakeMatch2.h>
