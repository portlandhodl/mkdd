#include "Sato/Objects/MapObjNossie.h"

#include "Inagaki/GameSoundMgr.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/Coord3DUtil.h"
#include "Shiraiwa/SiUtil.h"
#include "Yamamoto/kartCtrl.h"
#include "mathHelper.h"

const f32 TMapObjNossie::sOffsetY[4] = { 540.0f, 540.0f, 362.5f, 362.5f };
const char *TMapObjNossie::sNossieLegJointName[4] = { "wrist_l", "wrist_r", "ankle_l", "ankle_r" };
int TMapObjNossie::sNossieLegJointNo[4] = { 0, 0, 0, 0 };

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

const u32 TMapObjNossie::scNossieEffectAppearPos[16] = {
    2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3
};

const char *TMapObjNossie::sHeadJointName = "head";
int TMapObjNossie::sHeadJointNo;
JGeometry::TVec3f TMapObjNossie::sHeadOffset(0.0f, -460.0f, 550.0f);
JGeometry::TVec3f TMapObjNossie::sBodyOffset(0.0f, 2400.0f, 500.0f);
f32 TMapObjNossieColFoot::sShakeRadius = 10000.0f;


f32 TMapObjPtera::sMoveMaxSpeed = 30.0f;
f32 TMapObjPtera::sMoveAccel = 0.5f;
f32 TMapObjPtera::sReachLength = 2000.0f;

f32 TMapObjSwimNossie::sMoveMaxSpeed = 5.0f;
f32 TMapObjSwimNossie::sMoveAccel = 0.02f;
f32 TMapObjSwimNossie::sReachLength = 500.0f;

#include <JSystem/JAudio/JASFakeMatch2.h>

J3DAnmTransform *TMapObjSwimNossie::sSwimNossieBckAnmTrans;
J3DMtxCalc *TMapObjSwimNossie::sSwimNossieBckMtxCalc;
J3DAnmTexPattern *TMapObjSwimNossie::sSwimNossieBtpAnm;

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

TMapObjNossie::~TMapObjNossie() {}

const char *TMapObjNossie::getBmdFileName() {
    static s8 init;
    static const char *cBmdName;
    if (!init) {
        cBmdName = "/Objects/Nossie.bmd";
        init = 1;
    }
    return cBmdName;
}

void TMapObjNossie::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createModel(heap, viewNo, 0);
    if (isTimeAttack()) {
        mAnmPlayer.registAnimations(mAnmCtrl, &mModel, sAnmInfosTA, 1);
    } else {
        mAnmPlayer.registAnimations(mAnmCtrl, &mModel, sAnmInfos, 16);
    }
    for (int i = 0; i < 4; i++) {
        sNossieLegJointNo[i] = mModel.getModelData()->getJointName()->getIndex(sNossieLegJointName[i]);
    }
    sHeadJointNo = mModel.getModelData()->getJointName()->getIndex(sHeadJointName);
}

void TMapObjNossie::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    if (isTimeAttack()) {
        mAnmPlayer.loadAnimations(sAnmInfosTA, 1, modelData, ResMgr::mcArcCourse);
    } else {
        mAnmPlayer.loadAnimations(sAnmInfos, 16, modelData, ResMgr::mcArcCourse);
    }
}

void TMapObjNossie::reset() {
    resetObject();
    mGeoObjFlag &= ~0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    if (isTimeAttack()) {
        mAnmPlayer.init(mAnmCtrl, sAnmInfosTA, 1);
    } else {
        mAnmPlayer.init(mAnmCtrl, sAnmInfos, 16);
    }
    mAnmCtrl->Reset();
    _58 = 10;
    for (int i = 0; i < 4; i++) {
        u32 seID = 0x40045;
        JGeometry::TPos3f mtx;
        mtx.set(mRotMtx);
        if (i == 1 || i == 3) {
            f32 s = sin(3.1415927f);
            f32 c = cos(3.1415927f);
            mtx[1][0] = s;
            mtx[0][0] = c;
            mtx[0][1] = -s;
            mtx[1][1] = c;
            mtx[2][2] = 1.0f;
            mtx[2][1] = 0.0f;
            mtx[1][2] = 0.0f;
            mtx[2][0] = 0.0f;
            mtx[0][2] = 0.0f;
            PSMTXConcat(mRotMtx, mtx, mtx);
            seID = 0x40046;
        }
        mFoot[i]->mRotMtx.set(mtx);
        mFoot[i]->mScale.set(mScale);
        mFoot[i]->mStepSeID = seID;
    }
}

bool TMapObjNossie::isTimeAttack() {
    return RaceMgr::getManager()->mRaceInfo->mRaceMode == TIME_ATTACK;
}

void TMapObjNossie::calc() {
    JGeometry::TVec3f pos;
    for (u8 i = 0; i < 4; i++) {
        getJointPosition(&pos, sNossieLegJointNo[i]);
        if (!isTimeAttack()) {
            makeStepEmitter(i, pos);
        }
        pos.y -= sOffsetY[i];
        mFoot[i]->mPos.set(pos);
    }
    getJointPosition(&pos, sHeadJointNo);
    JGeometry::TVec3f offset(sHeadOffset);
    PSMTXMultVecSR(mRotMtx, &offset, &offset);
    offset.mul(mScale);
    pos.add(offset);
    mHead->mPos.set(pos);
    offset.set(sBodyOffset);
    PSMTXMultVecSR(mRotMtx, &offset, &offset);
    offset.mul(mScale);
    pos.add(mPos, offset);
    mBody->mPos.set(pos);
    if (!isTimeAttack()) {
        for (int i = 0; i < 4; i++) {
            for (int kartNo = 0; kartNo < RaceMgr::getManager()->getKartNumber(); kartNo++) {
                if (mFoot[i]->isStepped(kartNo)) {
                    reverse();
                    break;
                }
            }
        }
    }
    mAnmPlayer.update();
}

void TMapObjNossie::makeStepEmitter(u32 footNo, const JGeometry::TVec3f &pos) {
    static const char *emName[4] = { "mk_nossieWrist_a", "mk_nossieWrist_b", "mk_nossieAnkle_a", "mk_nossieAnkle_b" };
    J3DFrameCtrl *ctrl = mAnmPlayer.mController->getCurFrameCtrl();
    if ((f32)ctrl->getEnd() - ctrl->getFrame() <= ctrl->getRate()) {
        u32 appearPos = scNossieEffectAppearPos[mAnmPlayer.mController->mTrans->getNowAnmNo()];
        if (appearPos == footNo) {
            switch (appearPos) {
            case 0:
            case 1:
                GetJPAMgr()->createEmt(emName[0], pos);
                GetJPAMgr()->createEmt(emName[1], pos);
                break;
            case 2:
            case 3:
                GetJPAMgr()->createEmt(emName[2], pos);
                GetJPAMgr()->createEmt(emName[3], pos);
                break;
            }
        }
    }
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

void TMapObjNossieColFoot::createColModel(J3DModelData *) {
    createBoundsCylinder(1800.0f, 32.0f, 1500.0f);
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

void TMapObjNossieColFoot::calc() {
    mVel.sub(mPos, mShadowPos);
    mShadowPos.set(mPos);
    if (mShadowMdl) {
        moveShadowModel();
    }
    bool still = false;
    if (mShadowPos.isZero() || JGeometry::TUtilf::abs(mVel.y) > 100.0f) {
        still = true;
    }
    if (!still && mShake < 0.0f && mShake * (mVel.y - 0.1f) <= 0.0f) {
        ObjUtility::executeShakeCamera(mPos, sShakeRadius, 0.4f, 2000.0f);
        mSoundMgr->setSe(mStepSeID);
    }
    mShake = mVel.y - 0.1f;
    u8 flags = 0;
    getHitFlag(&flags);
    mHit = flags & ~mPrevHit;
    mPrevHit = flags;
    for (int i = 0; i < RaceMgr::getManager()->getKartNumber(); i++) {
        bool stepped = false;
        if ((mHit & (1 << i)) && (KartCtrl::getKartCtrl()->GetKartStatus(i) & 0x200000000)) {
            stepped = true;
        }
        if (stepped) {
            startStamp(i);
        }
        if (!(KartCtrl::getKartCtrl()->GetKartStatus(i) & 0x200000000)) {
            endStamp(i);
        }
    }
    frameProc();
}

void TMapObjNossieColFoot::getHitFlag(u8 *flags) {
    for (int i = 0; i < 8; i++) {
        bool hit = (mIsHitKartFlags & (1 << i)) != 0;
        *flags |= hit << i;
    }
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

TMapObjSwimNossie::TMapObjSwimNossie(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
#line 526
    JUT_ASSERT_MSG(IsPathMove(), "SwimNossie: illegal Path ID.");
    mPath = new TPathUtil(&obj);
    NewAnmCtrl();
    createSoundMgr();
}

TMapObjSwimNossie::~TMapObjSwimNossie() { delete mPath; }

void TMapObjSwimNossie::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createDifferedModel(heap, viewNo, 0x20000, false);
    getAnmCtrl()->InitRegistration(1, &mModel);
    getAnmCtrl()->RegisterTrans(0, sSwimNossieBckAnmTrans, sSwimNossieBckMtxCalc);
    getAnmCtrl()->getFrameCtrl(0)->setAttribute(2);
    mMaterial.mModel = &mModel;
    mMaterial.mAnmBase = sSwimNossieBtpAnm;
    mMaterial.initFrameCtrl(mMaterial.mAnmBase);
}

void TMapObjSwimNossie::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *bca = ObjUtility::getPtrCourseArc("/Objects/SwimNossie.bca");
    J3DAnmObjTrans::setupTransAnmData(&sSwimNossieBckAnmTrans, &sSwimNossieBckMtxCalc, modelData, bca);
    void *btp = ObjUtility::getPtrCourseArc("/Objects/SwimNossie.btp");
    J3DAnmObjMaterial::setupTexPatternAnmData(&sSwimNossieBtpAnm, modelData, btp);
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

TAnmInfo TMapObjPtera::sAnmInfos[2] = {
	{ "/Objects/PteraFlya.bck", 0, 0, 2, 0, 0, 1, 0 },
	{ "/Objects/PteraFlyb.bck", 0, 0, 2, 0, 0, 0, 0 }
};

TMapObjPtera::TMapObjPtera(const CrsData::SObject &obj) : TMapObjHioNode(obj) {
    NewAnmCtrl();
    createSoundMgr();
    TAnmPlayer::resetAnimations(sAnmInfos, 2);
    mPath = new TPathUtil(&obj);
}

TMapObjPtera::~TMapObjPtera() { delete mPath; }

void TMapObjPtera::calc() {
    mAnmPlayer.update();
    if (moveToNextNode() < sReachLength) {
        TPathUtil *path = mPath;
        path->mDistanceDirty = true;
        path->mNode = path->getNextNode();
        path = mPath;
        int next = path->getNextNode();
        f32 nextY = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(path->mObj->pathID, next)->pos.y;
        u8 anm = mAnmPlayer.getCurAnmNumber();
        if (anm == 0 && mPos.y > nextY) {
            mAnmPlayer._10 = 1;
            mAnmPlayer._e |= 2;
        } else if (anm == 1 && mPos.y <= nextY) {
            mAnmPlayer._10 = 1;
            mAnmPlayer._e |= 2;
        }
    }
    getSoundMgr()->setSe(0x4005e);
    if (mAnmPlayer.getCurAnmNumber() == 0) {
        J3DFrameCtrl *ctrl = mAnmPlayer.mController->getCurFrameCtrl();
        if (SiUtil::checkPassFrame(2.0f, ctrl->getFrame(), ctrl->getRate())) {
            getSoundMgr()->setSe(0x4005c);
        }
    }
}

void TMapObjPtera::loadAnimation() {
    TAnmPlayer::loadAnimations(sAnmInfos, 2, mModel.getModelData(), ResMgr::mcArcCourse);
}

void TMapObjPtera::update() {
    setModelMatrixAndScale();
    mModel.update(0);
}

void TMapObjPtera::createModel(JKRSolidHeap *heap, u32 p2) {
    mModel.createModel(heap, p2, 0);
    mAnmPlayer.registAnimations(mAnmCtrl, &mModel, sAnmInfos, 2);
}

__declspec(weak) const char *TMapObjPtera::getBmdFileName() {
    return "/Objects/Ptera.bmd";
}

__declspec(weak) const char *TMapObjSwimNossie::getBmdFileName() {
    return "/Objects/SwimNossie.bmd";
}
