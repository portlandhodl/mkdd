#include "Shiraiwa/Objects/MapObjHeyho.h"
#include "JSystem/JMath/JMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Yamamoto/kartCtrl.h"

#define HEYHO_STATE_MAX 7
#define SUB_HEYHO_STATE_MAX 3

TAnmInfo TMapObjHeyho::sAnmInfos[1];
TAnmInfo TMapObjSubHeyho::sAnmInfos[1];

const u8 TMapObjHeyho::sAnmTable[7] = {};

f32 TMapObjHeyho::sGravity = 4.0f;
f32 TMapObjHeyho::sAccel = 0.05f;
f32 TMapObjHeyho::sJumpVel = 30.0f;
f32 TMapObjHeyho::sJumpGrv = 1.0f;
f32 TMapObjHeyho::sRotSpeed = 0.7f;
f32 TMapObjHeyho::sFirstSinkVel = -0.05f;
f32 TMapObjHeyho::sSinkAccel = 0.01f;
f32 TMapObjHeyho::sBombVelH = 40.0f;
f32 TMapObjHeyho::sBombVelV = 70.0f;
f32 TMapObjHeyho::sSlideVelH = 100.0f;
f32 TMapObjHeyho::sSlideVelV = 30.0f;
f32 TMapObjHeyho::sBombRotSpeed = 0.3f;
f32 TMapObjHeyho::sBombEndRotDecel = 0.005f;
f32 TMapObjHeyho::sAirFriction = 0.99901f;

THeyhoSupervisor *TMapObjHeyho::sSupervisor;
J3DAnmTexPattern *TMapObjHeyho::sHeyhoBtpAnm;
J3DAnmTexPattern *TMapObjSubHeyho::sHeyhoBtpAnm;


void THeyhoSupervisor::calc() {
    for (JSULink<TMapObjHeyho> *a = mList.getFirst(); a != nullptr; a = a->getNext()) {
        for (JSULink<TMapObjHeyho> *b = a; b != nullptr; b = b->getNext()) {
            if (a != b) {
                a->getObject()->hitCheck(b->getObject());
            }
        }
    }
}

__declspec(weak) void THeyhoSupervisor::entry(TMapObjHeyho *obj) {
    mList.append(&obj->mLink);
}

TMapObjHeyho::TMapObjHeyho(const CrsData::SObject &obj)
    : TMapObjHioNode(obj), StateObserver(), JKRDisposer(), mLink(this), mBtp(), mAnmPlayer()
{
    NewAnmCtrl();
    createSoundMgr();

    if (obj.mParam2 == 1) {
        mpSub = (TMapObjSubHeyho *)GeographyObjManager::gMgr->createSubObj(0x13ef);
        mpSub->mpPos = &mPos;
        mpSub->mpVel = &mVel;
        mpSub->mpMtx = &mRotMtx;
        mpSub->mScale.set(mScale);
    } else {
        mpSub = nullptr;
    }

    TAnmPlayer::resetAnimations(sAnmInfos, 1);
    mpPathWalk = new TPathWalk(&obj);

    if (getSupervisor() == nullptr) {
        sSupervisor = new THeyhoSupervisor();
    }
    sSupervisor->mList.append(&mLink);
}

TMapObjHeyho::~TMapObjHeyho() {
    sSupervisor = nullptr;
    if (mpPathWalk != nullptr) {
        delete mpPathWalk;
    }
}

void TMapObjHeyho::reset() {
    resetObject();
    ResetState();
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    _58 = 4;
    mAnmPlayer.init(mAnmCtrl, sAnmInfos, 1);

    mBtp.resetFrame();

    mSinkVel = 0.0f;
    mJumpOffset = 0.0f;
    mJumpVel = 0.0f;
    mBombVelH = 0.0f;
    mBombVelV = 0.0f;

    mBasePos.set(mPos);
    mBaseMtx.set(mRotMtx);
    mImpulse.z = 0.0f;
    mImpulse.y = 0.0f;
    mImpulse.x = 0.0f;
    mBombRotSpeed = 0.0f;

    mpPathWalk->init(&mBasePos, &mVel, &mBaseMtx);
    mpPathWalk->reset();

    f32 depth = (f32)mObjData->mParam1;
    if (depth == 0.0f) {
        depth = 12.0f;
    }
    mSinkDepth = depth;

    u16 frame = mObjData->mParam3;
    if (frame >= 3) {
        frame = 0;
    }
    f32 frameF = (f32)frame;
    mBtp.setFrame(frameF);
    if (mpSub != nullptr) {
        mpSub->mBtp.setFrame(frameF);
    }

    mpPathWalk->setParameters(12.0f, 0.2f, 0.06f, 0.001f, 1.0f);
}

void TMapObjHeyho::calc() {
    s16 node = mpPathWalk->update();
    if (node != -1) {
        checkJump((u16)node);
    }

    if (checkItemHitting()) {
        changeAllState(2);
    }

    ExecuteState();
    moveShadowModel();
    callWalkSound();
}

void TMapObjHeyho::callWalkSound() {}

bool TMapObjHeyho::checkItemHitting() {
    bool ret = false;
    if (mGeoObjFlag & 4) {
        ItemObj *item = mColItemObj;
        int kind = item->mItemKind;
        if (kind == 2) {
            goto slide;
        }
        if (kind < 2) {
            if (kind == 0) {
                goto slide;
            }
            if (kind >= 0) {
                goto bomb;
            }
            goto end;
        } else if (kind == 9) {
            goto slide;
        } else if (kind < 9) {
            if (kind >= 7) {
                goto bomb;
            }
        }
        goto end;

    slide:
        mBombVelH = sSlideVelH;
        mBombVelV = sSlideVelV;
        ret = true;
        mImpulse.x = item->mPos.x;
        mImpulse.y = item->mPos.y;
        mImpulse.z = item->mPos.z;
        goto end;

    bomb:
        mBombVelH = sBombVelH;
        mBombVelV = sBombVelV;
        ret = true;
        mImpulse.x = item->mPos.x;
        mImpulse.y = item->mPos.y;
        mImpulse.z = item->mPos.z;
    }

end:
    if (!ret && mpSub != nullptr && mpSub->checkItemHitting()) {
        ret = true;
    }
    return ret;
}

void TMapObjHeyho::checkJump(u16 node) {
    CrsData::PointData *point = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mObjData->pathID, node);
    if (point->linkPoint != 0) {
        setState(1);
    }
}

const char *TMapObjHeyho::getBmdFileName() {
    static const char *cBmdName = "/Objects/Heyho1.bmd";
    return cBmdName;
}

const char *TMapObjHeyho::getShadowBmdFileName() {
    static const char *cShadowBmdName = "/Objects/Heyho1Shadow.bmd";
    return cShadowBmdName;
}

void TMapObjHeyho::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    mAnmPlayer.loadAnimations(sAnmInfos, 1, modelData, ResMgr::mcArcCourse);
    void *btp = ObjUtility::getPtrCourseArc("/Objects/Heyho1.btp");
    J3DAnmObjMaterial::setupTexPatternAnmData(&sHeyhoBtpAnm, modelData, btp);
}

void TMapObjHeyho::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createDifferedModel(heap, viewNo, 0x20000, false);
    mAnmPlayer.registAnimations(mAnmCtrl, &mModel, sAnmInfos, 1);
    mBtp.setExModel(&mModel);
    mBtp.attach(sHeyhoBtpAnm);
}

void TMapObjHeyho::createColModel(J3DModelData *modelData) {
    f32 a = mScale.x > mScale.z ? mScale.x : mScale.z;
    f32 b = mScale.y;
    f32 max = a > b ? a : b;
    createBoundsCylinder(modelData, a / max * 140.0f, b / max * 320.0f);
}

void TMapObjHeyho::InitExec() {
    Observer_FindAndInit(TMapObjHeyho, HEYHO_STATE_MAX);
}

void TMapObjHeyho::MoveExec() {
    Observer_FindAndExec(TMapObjHeyho, HEYHO_STATE_MAX);
}

void TMapObjHeyho::update() {
    mBtp.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}

#line 385
void TMapObjHeyho::changeAllState(u16 state) {
    JUT_MINMAX_ASSERT(0, state, HEYHO_STATE_MAX);

    setState(state & 0xffff);

    u8 anm = sAnmTable[state];
    s8 anmS = anm;
    if (anmS != mAnmPlayer.mController->getNowTransNo()) {
        mAnmPlayer._11 = anm;
        mAnmPlayer._10 = 1;
        mAnmPlayer._e |= 1;
    }
}

void TMapObjHeyho::doKartColCallBack(int kart) {
    u64 status = KartCtrl::getKartCtrl()->GetKartStatus(kart);
    if (status & 0x40000) {
        ObjUtility::getKartPos(kart, &mImpulse);
        setState(2);
        u8 anm = sAnmTable[2];
        s8 anmS = anm;
        if (anmS != mAnmPlayer.mController->getNowTransNo()) {
            mAnmPlayer._11 = anm;
            mAnmPlayer._10 = 1;
            mAnmPlayer._e |= 1;
        }
    }
}

void TMapObjHeyho::initFunc_Skate() {
    mpPathWalk->_94 = 1;
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->resetFlag();
    }
}

void TMapObjHeyho::doFunc_Skate() {
    mPos.set(mBasePos);
    mRotMtx.set(mBaseMtx);
}

void TMapObjHeyho::initFunc_Jump() {
    mJumpOffset = 0.0f;
    mJumpVel = sJumpVel;
    mExtraMtx.set(mBaseMtx);
    PSMTXIdentity(mExtraMtx);

    f32 s = sin(sRotSpeed);
    f32 c = cos(sRotSpeed);
    mExtraMtx.ref(0, 0) = c;
    mExtraMtx.ref(0, 1) = 0.0f;
    mExtraMtx.ref(0, 2) = s;
    mExtraMtx.ref(0, 3) = 0.0f;
    mExtraMtx.ref(1, 0) = 0.0f;
    mExtraMtx.ref(1, 1) = 1.0f;
    mExtraMtx.ref(1, 2) = 0.0f;
    mExtraMtx.ref(1, 3) = 0.0f;
    mExtraMtx.ref(2, 0) = -s;
    mExtraMtx.ref(2, 1) = 0.0f;
    mExtraMtx.ref(2, 2) = c;
    mExtraMtx.ref(2, 3) = 0.0f;

    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->setFlag();
    }
    if (mpSub != nullptr) {
        mpSub->setState(1);
    }
    mSoundMgr->setSe(0x4004c);
}

void TMapObjHeyho::doFunc_Jump() {}

void TMapObjHeyho::initFunc_Bomb() {}

void TMapObjHeyho::doFunc_Bomb() {}

void TMapObjHeyho::initFunc_BombEnd() {
    mBombRotSpeed = sBombRotSpeed;
}

void TMapObjHeyho::doFunc_BombEnd() {}

void TMapObjHeyho::initFunc_Sink() {
    mSinkVel = sFirstSinkVel;
    mSoundMgr->setSe(0x4005a);
}

void TMapObjHeyho::doFunc_Sink() {}

void TMapObjHeyho::initFunc_Recover() {
    mpPathWalk->_94 = 1;
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->resetFlag();
    }
}

void TMapObjHeyho::doFunc_Recover() {}

void TMapObjHeyho::initFunc_Die() {
    mpPathWalk->_94 = 1;
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->setFlag();
    }
    mGeoObjFlag &= ~0x2;
    mCheckKartHitFlags = 0;
    mGeoObjFlag |= 0x20;
}

void TMapObjHeyho::doFunc_Die() {}

void TMapObjHeyho::hitCheck(TMapObjHeyho *) {}

TMapObjSubHeyho::TMapObjSubHeyho() : GeographyObj(0x13ef), StateObserver(), mBtp() {
    NewAnmCtrl();
    TAnmPlayer::resetAnimations(sAnmInfos, 1);
}

void TMapObjSubHeyho::reset() {
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    _58 = 4;
    mAnmCtrl->Reset();
    mBtp.resetFrame();
    mVel.x = 0.0f;
    mVel.y = 0.0f;
    mVel.z = 0.0f;
    ResetState();
}

bool TMapObjSubHeyho::checkItemHitting() {
    bool ret = false;
    if (mGeoObjFlag & 4) {
        int kind = mColItemObj->mItemKind;
        if (kind < 7) {
            if (kind >= 3) {
            } else if (kind >= 0) {
                ret = true;
            }
        } else {
            if (kind < 10) {
                ret = true;
            }
        }
    }
    return ret;
}

void TMapObjSubHeyho::InitExec() {
    Observer_FindAndInit(TMapObjSubHeyho, SUB_HEYHO_STATE_MAX);
}

void TMapObjSubHeyho::MoveExec() {
    Observer_FindAndExec(TMapObjSubHeyho, SUB_HEYHO_STATE_MAX);
}

void TMapObjSubHeyho::calc() {
    mRotMtx.set(*mpMtx);
    ExecuteState();
    moveShadowModel();
}

void TMapObjSubHeyho::initFunc_Skate() {
    mCheckKartHitFlags = -1;
    mGeoObjFlag |= 2;
}

void TMapObjSubHeyho::doFunc_Skate() {
    JGeometry::TPos3f mtx;
    makeFrontMtx(&mtx);
    JGeometry::TVec3f out;
    PSMTXMultVecSR(mtx, &mVel, &out);
    *mpPos += out;
}

void TMapObjSubHeyho::makeFrontMtx(JGeometry::TPos3f *) {}

void TMapObjSubHeyho::initFunc_Spin() {
    mRotMtx.set(*mpMtx);
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~2;
}

void TMapObjSubHeyho::doFunc_Spin() {
    JGeometry::TVec3f out;
    PSMTXMultVecSR(mRotMtx, &mVel, &out);
    *mpPos += out;
}

void TMapObjSubHeyho::initFunc_Recover() {
    mRecoverFrame = 0.0f;
    mRotMtx.ref(0, 3) = 0.0f;
    mRotMtx.ref(1, 3) = 0.0f;
    mRotMtx.ref(2, 3) = 0.0f;
}

void TMapObjSubHeyho::doFunc_Recover() {}

const char *TMapObjSubHeyho::getBmdFileName() {
    static const char *cBmdName = "/Objects/Heyho1.bmd";
    return cBmdName;
}

const char *TMapObjSubHeyho::getShadowBmdFileName() {
    static const char *cShadowBmdName = "/Objects/Heyho1Shadow.bmd";
    return cShadowBmdName;
}

void TMapObjSubHeyho::update() {
    mBtp.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}

void TMapObjSubHeyho::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    TAnmPlayer::loadAnimations(sAnmInfos, 1, modelData, ResMgr::mcArcCourse);
    void *btp = ObjUtility::getPtrCourseArc("/Objects/Heyho1.btp");
    J3DAnmObjMaterial::setupTexPatternAnmData(&sHeyhoBtpAnm, modelData, btp);
}

void TMapObjSubHeyho::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createDifferedModel(heap, viewNo, 0x20000, false);
    TAnmPlayer::registAnimations(mAnmCtrl, &mModel, sAnmInfos, 1);
    mBtp.setExModel(&mModel);
    mBtp.attach(sHeyhoBtpAnm);
}

void TMapObjSubHeyho::createColModel(J3DModelData *modelData) {
    f32 a = mScale.x > mScale.z ? mScale.x : mScale.z;
    f32 b = mScale.y;
    f32 max = a > b ? a : b;
    createBoundsCylinder(modelData, a / max * 140.0f, b / max * 320.0f);
}

extern "C" static void __sinit_MapObjHeyho_cpp() {}
