#include "Sato/Objects/MapObjNossie.h"

#include "Inagaki/GameSoundMgr.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/Coord3DUtil.h"
#include "Yamamoto/kartCtrl.h"

TMapObjPtera::TMapObjPtera(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
    NewAnmCtrl();
    createSoundMgr();
    TAnmPlayer::resetAnimations(sAnmInfos, 2);
    mPath = new TPathUtil(&obj);
}

TMapObjSwimNossie::TMapObjSwimNossie(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
#line 526
    JUT_ASSERT_MSG(IsPathMove(), "SwimNossie: illegal Path ID.");
    mPath = new TPathUtil(&obj);
    NewAnmCtrl();
    createSoundMgr();
}

TMapObjNossie::TMapObjNossie(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
    NewAnmCtrl();
    mHead = (TMapObjNossieColHead *)GeographyObjManager::gMgr->createGenSubObj(0x11a2);
    mBody = (TMapObjNossieColBody *)GeographyObjManager::gMgr->createGenSubObj(0x11a8);
    for (int i = 0; i < 4; i++) {
        u32 id;
        switch (i) {
        case 0:
        case 1:
            id = 0x11a3;
            break;
        case 2:
        case 3:
            id = 0x11a7;
            break;
        default:
#line 107
            JUT_ASSERT(false);
        }
        mFoot[i] = (TMapObjNossieColFoot *)GeographyObjManager::gMgr->createGenSubObj(id);
    }
    if (isTimeAttack())
        mAnmPlayer.resetAnimations(sAnmInfosTA, 1);
    else
        mAnmPlayer.resetAnimations(sAnmInfos, 16);
}

TAnmInfo TMapObjNossie::sAnmInfos[16] = {
	{ "/Objects/Nossie_a1.bca", 0, 0, 0, 0, 1, 1, 0 },
	{ "/Objects/Nossie_a2.bca", 0, 0, 0, 0, 1, 2, 0 },
	{ "/Objects/Nossie_a3.bca", 0, 0, 0, 0, 1, 3, 0 },
	{ "/Objects/Nossie_a4.bca", 0, 0, 0, 0, 1, 4, 0 },
	{ "/Objects/Nossie_a5.bca", 0, 0, 0, 0, 1, 5, 0 },
	{ "/Objects/Nossie_a6.bca", 0, 0, 0, 0, 1, 6, 0 },
	{ "/Objects/Nossie_a7.bca", 0, 0, 0, 0, 1, 7, 0 },
	{ "/Objects/Nossie_a8.bca", 0, 0, 0, 0, 1, 0, 0 },
	{ "/Objects/Nossie_b1.bca", 0, 0, 0, 0, 1, 15, 0 },
	{ "/Objects/Nossie_b2.bca", 0, 0, 0, 0, 1, 8, 0 },
	{ "/Objects/Nossie_b3.bca", 0, 0, 0, 0, 1, 9, 0 },
	{ "/Objects/Nossie_b4.bca", 0, 0, 0, 0, 1, 10, 0 },
	{ "/Objects/Nossie_b5.bca", 0, 0, 0, 0, 1, 11, 0 },
	{ "/Objects/Nossie_b6.bca", 0, 0, 0, 0, 1, 12, 0 },
	{ "/Objects/Nossie_b7.bca", 0, 0, 0, 0, 1, 13, 0 },
	{ "/Objects/Nossie_b8.bca", 0, 0, 0, 0, 1, 14, 0 }
};

TAnmInfo TMapObjNossie::sAnmInfosTA[1] = {
	{ "/Objects/Nossie_TA.bca", 0, 0, 2, 0, 0, 0xFF, 0 }
};

const char *TMapObjNossie::sNossieLegJointName[4] = { "wrist_l", "wrist_r", "ankle_l", "ankle_r" };
int TMapObjNossie::sNossieLegJointNo[4] = { 0, 0, 0, 0 };
const char *TMapObjNossie::sHeadJointName = "head";
int TMapObjNossie::sHeadJointNo;
f32 TMapObjNossie::sOffsetY[4] = { 540.0f, 540.0f, 362.5f, 362.5f };
JGeometry::TVec3f TMapObjNossie::sHeadOffset(0.0f, -460.0f, 550.0f);
JGeometry::TVec3f TMapObjNossie::sBodyOffset(0.0f, 2400.0f, 500.0f);
f32 TMapObjNossieColFoot::sShakeRadius = 10000.0f;

TAnmInfo TMapObjPtera::sAnmInfos[2] = {
	{ "/Objects/PteraFlya.bck", 0, 0, 2, 0, 1, 0, 0 },
	{ "/Objects/PteraFlyb.bck", 0, 0, 2, 0, 0, 0, 0 }
};

f32 TMapObjPtera::sMoveMaxSpeed = 30.0f;
f32 TMapObjPtera::sMoveAccel = 0.5f;
f32 TMapObjPtera::sReachLength = 2000.0f;

f32 TMapObjSwimNossie::sMoveMaxSpeed = 5.0f;
f32 TMapObjSwimNossie::sMoveAccel = 0.02f;
f32 TMapObjSwimNossie::sReachLength = 500.0f;

#include <JSystem/JAudio/JASFakeMatch2.h>

const char *TMapObjNossie::getBmdFileName() {
    static s8 init;
    static const char *cBmdName;
    if (!init) {
        cBmdName = "/Objects/Nossie.bmd";
        init = 1;
    }
    return cBmdName;
}

void TMapObjNossieColBase::reset() {
    resetObject();
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    _58 = getReaction();
}

__declspec(weak) u32 TMapObjNossieColBase::getReaction() {
    return 3;
}

void TMapObjNossieColBase::calc() {}

void TMapObjNossieColHead::createColModel(J3DModelData *) {
    createBoundsSphere(1500.0f, mScale.y);
}

void TMapObjNossieColHead::calc() {
    getSoundMgr()->setSe(0x40063);
}

void TMapObjNossieColBody::createColModel(J3DModelData *) {
    createBoundsCylinder(1900.0f, 500.0f, mScale.y);
}

u32 TMapObjNossieColFoot::getReleaseFrame() {
    return 30;
}

void TMapObjNossieColFoot::createColModel(J3DModelData *) {
    createBoundsCylinder(1800.0f, 32.0f, 1500.0f);
}

void TMapObjNossieColFoot::getHitFlag(u8 *flags) {
    for (int i = 0; i < 8; i++) {
        bool hit = (mIsHitKartFlags & (1 << i)) != 0;
        *flags |= hit << i;
    }
}

void TMapObjNossie::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    if (isTimeAttack()) {
        mAnmPlayer.loadAnimations(sAnmInfosTA, 1, modelData, ResMgr::mcArcCourse);
    } else {
        mAnmPlayer.loadAnimations(sAnmInfos, 16, modelData, ResMgr::mcArcCourse);
    }
}

bool TMapObjNossie::isTimeAttack() {
    return RaceMgr::getManager()->mRaceInfo->mRaceMode == TIME_ATTACK;
}

void TMapObjNossie::reverse() {
    if (RaceMgr::getManager()->getRaceMode() == TIME_ATTACK)
        return;

    int anm = (s8)mAnmPlayer.mController->mTrans->getNowAnmNo();
    if (anm <= 7)
        anm += 8;
    else
        anm -= 8;
    mAnmPlayer._11 = anm;
    mAnmPlayer._10 = 1;
    mAnmPlayer._e |= 2;
}

void TMapObjNossie::getJointPosition(JGeometry::TVec3f *pos, long jointNo) {
    Mtx &mtx = mModel.getModel()->mMtxBuffer->mWorldMatrices[jointNo];
    pos->set(mtx[0][3], mtx[1][3], mtx[2][3]);
}

bool TMapObjNossieColFoot::isStepped(int kartNo) {
    bool ret = false;
    if (mHit & (1 << kartNo)) {
        u64 status = KartCtrl::getKartCtrl()->getKartBody(kartNo)->mCarStatus;
        if (status & 0x200000000)
            ret = true;
    }
    return ret;
}

void TMapObjNossieColFoot::moveShadowModel() {
    if (mShadowMdl->isInvisibleAll())
        return;

    CrsGround ground(RaceMgr::getManager()->getCourse());
    ground.search(this, mPos, mPos);

    Mtx mtx;
    PSMTXCopy(mRotMtx, mtx);
    mtx[0][3] = mPos.x;
    mtx[1][3] = ground.getHeight();
    mtx[2][3] = mPos.z;

    mShadowMdl->setBaseTRMtx(mtx);
    mShadowMdl->getModel()->setBaseScale(mScale);
}

__declspec(weak) const char *TMapObjPtera::getBmdFileName() {
    return "/Objects/Ptera.bmd";
}

__declspec(weak) const char *TMapObjSwimNossie::getBmdFileName() {
    return "/Objects/SwimNossie.bmd";
}

TMapObjNossie::~TMapObjNossie() {}

TMapObjSwimNossie::~TMapObjSwimNossie() { delete mPath; }

TMapObjPtera::~TMapObjPtera() { delete mPath; }

void TMapObjNossieColFoot::reset() {
    resetFrame();
    resetObject();
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    _58 = getReaction();
    mShadowPos.set(mPos);
    mShake = 0.0f;
    mPrevHit = 0;
    mHit = 0;
}

void TMapObjPtera::createModel(JKRSolidHeap *heap, u32 p2) {
    mModel.createModel(heap, p2, 0);
    mAnmPlayer.registAnimations(mAnmCtrl, &mModel, sAnmInfos, 2);
}

void TMapObjPtera::update() {
    setModelMatrixAndScale();
    mModel.update(0);
}

void TMapObjSwimNossie::update() {
    mMaterial.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}

void TMapObjSwimNossie::calc() {
    if (moveToNextNode() < sReachLength) {
        TPathUtil *path = mPath;
        path->mDistanceDirty = true;
        path->mNode = path->getNextNode();
    }
    getSoundMgr()->setSe(0x40063);
}

void TMapObjPtera::loadAnimation() {
    TAnmPlayer::loadAnimations(sAnmInfos, 2, mModel.getModelData(), ResMgr::mcArcCourse);
}

void TMapObjSwimNossie::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *bca = ObjUtility::getPtrCourseArc("/Objects/SwimNossie.bca");
    J3DAnmObjTrans::setupTransAnmData(&sSwimNossieBckAnmTrans, &sSwimNossieBckMtxCalc, modelData, bca);
    void *btp = ObjUtility::getPtrCourseArc("/Objects/SwimNossie.btp");
    J3DAnmObjMaterial::setupTexPatternAnmData(&sSwimNossieBtpAnm, modelData, btp);
}

J3DAnmTransform *TMapObjSwimNossie::sSwimNossieBckAnmTrans;
J3DMtxCalc *TMapObjSwimNossie::sSwimNossieBckMtxCalc;
J3DAnmTexPattern *TMapObjSwimNossie::sSwimNossieBtpAnm;
