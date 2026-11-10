#include "Sato/J3DEfctBomb.h"

#include <JSystem/JUtility/JUTAssert.h>

#include "Sato/GeographyObjMgr.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/J3DEffectMgr.h"
#include "Sato/ObjUtility.h"

static const f32 sDummy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

J3DAnmTransform *J3DEfctBomb::sBckAnmTrans[1];
J3DMtxCalc *J3DEfctBomb::sBckMtxCalc[1];
J3DAnmTextureSRTKey *J3DEfctBomb::sTexSRTKey;
u8 J3DEfctBomb::sBrkAnmNo;
J3DAnmTevRegKey *J3DEfctBomb::sTevRegKey[4] = { nullptr, nullptr, nullptr, nullptr };

static inline J3DAnmObjMatTexSRT *getBombTexObj(AnmControlMatTexSRT *ctrl, u8 AnmNo) {
#line 321 "AnmController.h"
    JUT_MINMAX_ASSERT(0, AnmNo, ctrl->mMaxAnmNo);
    return ((J3DAnmObjMatTexSRT *)(*(void **)((u8 *)ctrl + 0x18))) + AnmNo;
}

static inline void setBombTevAnm(J3DAnmObjMatTevReg *tev, J3DAnmTevRegKey *anm)
{
    tev->mAnmBase = anm;
    tev->initFrameCtrl(tev->mAnmBase);
}

static inline J3DAnmObjMatTevReg *getBombTevObj(AnmControlMatTevReg *ctrl, u8 AnmNo) {
#line 331 "AnmController.h"
    JUT_MINMAX_ASSERT(0, AnmNo, ctrl->mMaxAnmNo);
    return ((J3DAnmObjMatTevReg *)(*(void **)((u8 *)ctrl + 0x18))) + AnmNo;
}

static inline J3DAnmObjMatTevReg *getBombPrevTevObj(AnmControlMatTevReg *ctrl, u8 prevUseAnmNo) {
#line 239 "AnmController.h"
    JUT_MINMAX_ASSERT(0, prevUseAnmNo, ctrl->mMaxAnmNo);
    return ((J3DAnmObjMatTevReg *)(*(void **)((u8 *)ctrl + 0x18))) + prevUseAnmNo;
}

J3DEfctBomb::J3DEfctBomb() {}

void J3DEfctBomb::loadAnmData()
{
    J3DModelData *mdlData = J3DEffectMgr::sJ3DMgr->getBombModelData();

    void *bck;
    if (GeographyObjManager::gMgr->mIsBombBattle)
        bck = ObjUtility::getPtrCourseArc((char *)getBckFileName());
    else
        bck = ObjUtility::getPtrMainArc((char *)getBckFileName());
    J3DAnmObjTrans::setupTransAnmData(&sBckAnmTrans[0], &sBckMtxCalc[0], mdlData, bck);

    void *btk;
    if (GeographyObjManager::gMgr->mIsBombBattle)
        btk = ObjUtility::getPtrCourseArc((char *)getBtkFileName());
    else
        btk = ObjUtility::getPtrMainArc((char *)getBtkFileName());
    J3DAnmObjMaterial::setupTexSRTAnmData(&sTexSRTKey, mdlData, btk);

    u8 numBrks = 2;
    if (GeographyObjManager::gMgr->mIsBombBattle)
        numBrks = 4;

    for (u8 i = 0; i < numBrks; i++) {
        void *brk;
        if (GeographyObjManager::gMgr->mIsBombBattle)
            brk = ObjUtility::getPtrCourseArc((char *)getBrkFileName((u8)i));
        else
            brk = ObjUtility::getPtrMainArc((char *)getBrkFileName((u8)i));
        J3DAnmObjMaterial::setupTevRegAnmData(&sTevRegKey[i], mdlData, brk);
    }

    sBrkAnmNo = numBrks - 1;
}

const char *J3DEfctBomb::getBmdFileName()
{
    const char *name = "/Effect/bombhei_bomb.bmd";
    if (GeographyObjManager::gMgr->mIsBombBattle)
        name = "/Objects/MiniGamebombhei_battle.bmd";
    return name;
}

const char *J3DEfctBomb::getBckFileName()
{
    const char *name = "/Effect/bombhei_bomb.bca";
    if (GeographyObjManager::gMgr->mIsBombBattle)
        name = "/Objects/MiniGamebombhei_battle.bca";
    return name;
}

const char *J3DEfctBomb::getBtkFileName()
{
    const char *name = "/Effect/bombhei_bomb.btk";
    if (GeographyObjManager::gMgr->mIsBombBattle)
        name = "/Objects/MiniGamebombhei_battle.btk";
    return name;
}

const char *J3DEfctBomb::getBrkFileName(u8 index)
{
    const char *name = nullptr;
    if (GeographyObjManager::gMgr->mIsBombBattle) {
        switch (index) {
        case 0:
            name = "/Objects/MiniGamebombhei_battle_1P.brk";
            break;
        case 1:
            name = "/Objects/MiniGamebombhei_battle_2P.brk";
            break;
        case 2:
            name = "/Objects/MiniGamebombhei_battle_3P.brk";
            break;
        case 3:
            name = "/Objects/MiniGamebombhei_battle_4P.brk";
            break;
        }
    } else {
        switch (index) {
        case 0:
            name = "/Effect/bombhei_bomb.brk";
            break;
        case 1:
            name = "/Effect/bombhei_bomb_b.brk";
            break;
        }
    }
    return name;
}

static inline void setBombTexAnm(J3DAnmTextureSRTKey *anm, J3DAnmObjMatTexSRT *tex)
{
    tex->mAnmBase = anm;
    tex->initFrameCtrl(tex->mAnmBase);
}


void J3DEfctBomb::onlyinit()
{
    mAnmCtrl.mTrans = new AnmControlTrans();
    mAnmCtrl.mTrans->initAnm(1, &mModel);
    mAnmCtrl.mTrans->registration(0, sBckAnmTrans[0], sBckMtxCalc[0]);

    MatAnmInfo info;
    info.mNumBtks = 0;
    info.mNumBrks = 0;
    info.mNumBtps = 0;
    info.mNumBpks = 0;
    info.mNumBtks = 1;

    u8 numBrks = 2;
    if (GeographyObjManager::gMgr->mIsBombBattle)
        numBrks = 4;
    info.mNumBrks = numBrks;

    mAnmCtrl.InitRegistrationMat(info, &mModel);

    setBombTexAnm(sTexSRTKey,
                  (getBombTexObj(mAnmCtrl.mMatData->mpCtrlSRT, 0),
                   *(J3DAnmObjMatTexSRT **)((u8 *)mAnmCtrl.mMatData->mpCtrlSRT + 0x18)));
    mAnmCtrl.mMatData->mpCtrlSRT->mNowAnmNo = 0;

    for (u8 i = 0; i < numBrks; i++) {
        J3DAnmObjMatTevReg *tev = getBombTevObj(mAnmCtrl.mMatData->mpCtrlTevReg, (u8)i);
        setBombTevAnm(tev, sTevRegKey[i]);
        mAnmCtrl.mMatData->mpCtrlTevReg->mNowAnmNo = i;
    }
}

void J3DEfctBomb::init()
{
    _dc = 0;
    mState = 0;
    mAnmCtrl.Reset();

    if (mAnmCtrl.mMat[3] != nullptr) {
        mAnmCtrl.mMat[3]->changeMatAnm(0, true);
    }
}

void J3DEfctBomb::calc()
{
    _dc++;

    if (_dc >= 99) {
        mState = 1;
        return;
    }

    mAnmCtrl.FrameProc();

    J3DModel *model = mModel.getModel();
    model->mModelScale.x = 1.0f;
    model->mModelScale.y = 1.0f;
    model->mModelScale.z = 1.0f;

    mModel.setBaseTRMtx(mMtx);
}

void J3DEfctBomb::update()
{
    AnmControlMatTevReg *ctrl = mAnmCtrl.mMatData->mpCtrlTevReg;
    J3DAnmObjMatTevReg *prev = getBombPrevTevObj(ctrl, sBrkAnmNo);
    J3DAnmObjMatTevReg *now = &((J3DAnmObjMatTevReg *)(*(void **)((u8 *)ctrl + 0x18)))[ctrl->mNowAnmNo];

    now->changeMatAnm(prev);

    mAnmCtrl.SetAnimation();
    mModel.update(0);

    sBrkAnmNo = mAnmCtrl.mMatData->mpCtrlTevReg->mNowAnmNo;
}

void J3DEfctBomb::changeColor(u8 color)
{
    if (mAnmCtrl.mMat[3] != nullptr) {
        mAnmCtrl.mMat[3]->changeMatAnm(color, true);
    }
}

inline J3DEfctBomb::~J3DEfctBomb() {}

#include <JSystem/JAudio/JASFakeMatch2.h>
