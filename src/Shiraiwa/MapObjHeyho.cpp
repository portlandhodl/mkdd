#include "Shiraiwa/Objects/MapObjHeyho.h"
#include "JSystem/JMath/JMath.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Yamamoto/kartCtrl.h"

template <>
void JGeometry::TRotation3<JGeometry::TMtx34f>::getQuat(JGeometry::TQuat4f &rDest) const;

#define HEYHO_STATE_MAX 7
#define SUB_HEYHO_STATE_MAX 3

TAnmInfo TMapObjHeyho::sAnmInfos[1] = {
    { "/Objects/Heyho1.bca", nullptr, nullptr, 2, 0, 0, -1, 0 },
};

StateObserver::StateFuncSet<TMapObjHeyho> TMapObjHeyho::sTable[7] = {
    { 0, &TMapObjHeyho::initFunc_Skate, &TMapObjHeyho::doFunc_Skate },
    { 1, &TMapObjHeyho::initFunc_Jump, &TMapObjHeyho::doFunc_Jump },
    { 2, &TMapObjHeyho::initFunc_Bomb, &TMapObjHeyho::doFunc_Bomb },
    { 3, &TMapObjHeyho::initFunc_BombEnd, &TMapObjHeyho::doFunc_BombEnd },
    { 4, &TMapObjHeyho::initFunc_Sink, &TMapObjHeyho::doFunc_Sink },
    { 5, &TMapObjHeyho::initFunc_Recover, &TMapObjHeyho::doFunc_Recover },
    { 6, &TMapObjHeyho::initFunc_Die, &TMapObjHeyho::doFunc_Die },
};

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
f32 TMapObjHeyho::sAirFriction = 0.999f;

THeyhoSupervisor *TMapObjHeyho::sSupervisor;
J3DAnmTexPattern *TMapObjHeyho::sHeyhoBtpAnm;


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
    delete mpPathWalk;
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
    PSMTXIdentity(mExtraMtx);

    mBasePos.set(mPos);
    mBaseMtx.set(mRotMtx);
    mImpulse.z = 0.0f;
    mImpulse.y = 0.0f;
    mImpulse.x = 0.0f;
    mBombRotSpeed = 0.0f;

    mpPathWalk->init(&mBasePos, &mVel, &mBaseMtx);
    mpPathWalk->reset();

    f32 speed = (f32)mObjData->mParam1;
    if (speed == 0.0f) {
        speed = 12.0f;
    }
    mWalkSpeed = speed;

    u16 frame = mObjData->mParam3;
    if ((int)frame >= 3) {
        frame = 0;
    }
    mBtp.setFrame(frame);
    if (mpSub != nullptr) {
        mpSub->mBtp.setFrame(frame);
    }

    mpPathWalk->setParameters(speed, 0.2f, 0.06f, 0.001f, 0.01f);
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

void TMapObjHeyho::callWalkSound() {
    J3DFrameCtrl *ctrl = mAnmPlayer.mController->getCurFrameCtrl();
    if (SiUtil::checkPassFrame(65.0f, ctrl->getFrame(), ctrl->getRate())) {
        mSoundMgr->setSe(0x4004b);
    }
    if (SiUtil::checkPassFrame(0.0f, ctrl->getFrame(), ctrl->getRate())) {
        mSoundMgr->setSe(0x4004a);
    }
}

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

    setState((u16)state);

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

static inline void setRotateY(JGeometry::TPos3f &mtx, f32 rad) {
    f32 s = sin(rad);
    f32 c = cos(rad);
    mtx.ref(1, 1) = 1.0f;
    mtx.ref(0, 0) = c;
    mtx.ref(0, 2) = s;
    mtx.ref(2, 0) = -s;
    mtx.ref(2, 2) = c;
    mtx.ref(2, 1) = 0.0f;
    mtx.ref(1, 2) = 0.0f;
    mtx.ref(1, 0) = 0.0f;
    mtx.ref(0, 1) = 0.0f;
}

void TMapObjHeyho::initFunc_Jump() {
    mJumpOffset = 0.0f;
    mJumpVel = sJumpVel;
    mRotMtx.set(mBaseMtx);
    PSMTXIdentity(mExtraMtx);

    setRotateY(mExtraMtx, sRotSpeed);

    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->setFlag();
    }
    if (mpSub != nullptr) {
        mpSub->setState(1);
    }
    mSoundMgr->setSe(0x4004c);
}

void TMapObjHeyho::doFunc_Jump() {
    mJumpVel -= sJumpGrv;
    mJumpOffset += mJumpVel;
    if (mJumpOffset < 0.0f) {
        mJumpOffset = 0.0f;
        mBaseMtx.set(mRotMtx);
        if (mAnmCtrl->mTrans != nullptr) {
            mAnmCtrl->mTrans->resetFlag();
        }
        mpPathWalk->_90 = 0.5f * mWalkSpeed;
        if (mpSub != nullptr) {
            mpSub->setState(2);
        }
        setState(4);
        mAnmPlayer.setAnm(sAnmTable[4]);
    }
    mPos.set(mBasePos.x, mBasePos.y + mJumpOffset, mBasePos.z);
    PSMTXConcat(mRotMtx, mExtraMtx, mRotMtx);
}

void TMapObjHeyho::initFunc_Bomb() {
    mpPathWalk->_94 = 0;
    JGeometry::TVec3f dir;
    dir.sub(mPos, mImpulse);
    dir.normalize();
    dir.y = 0.0f;
    dir.setLength(mBombVelH);
    dir.y = mBombVelV;
    mVel.set(dir);
    mRotMtx.set(mBaseMtx);
    PSMTXIdentity(mExtraMtx);
    setRotateY(mExtraMtx, sBombRotSpeed);
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->setFlag();
    }
    if (mpSub != nullptr) {
        mpSub->setState(1);
    }
}

void TMapObjHeyho::doFunc_Bomb() {
    PSMTXConcat(mRotMtx, mExtraMtx, mRotMtx);
    mVel.scale(sAirFriction);
    mVel.y -= sGravity;
    JGeometry::TVec3f prevPos(mPos);
    mPos.add(mVel);
    CrsGround ground(RaceMgr::sRaceManager->getCourse());
    ground.search(mPos, prevPos);
    JGeometry::TVec3f normal;
    f32 depth = ground.getWallNormal(&normal, nullptr);
    if (depth > -1.0f) {
        mPos.scaleAdd(depth, normal, mPos);
    }
    f32 height = ground.getHeight();
    if (mPos.y < height) {
        ground.search(prevPos);
        switch (ground.getAttribute()) {
        case CrsGround::Attr_5:
        case CrsGround::Attr_10:
        case CrsGround::Attr_14:
        case CrsGround::Attr_15:
        case CrsGround::Attr_255:
            setState(6);
            mAnmPlayer.setAnm(sAnmTable[6]);
            break;
        default:
            mPos.y = height;
            mBasePos.set(mPos);
            mBaseMtx.set(mRotMtx);
            mVel.z = 0.0f;
            mVel.y = 0.0f;
            mVel.x = 0.0f;
            mpPathWalk->_90 = 0.0f;
            setState(3);
            mAnmPlayer.setAnm(sAnmTable[3]);
            break;
        }
    }
}

void TMapObjHeyho::initFunc_BombEnd() {
    mBombRotSpeed = sBombRotSpeed;
}

void TMapObjHeyho::doFunc_BombEnd() {
    mBombRotSpeed -= sBombEndRotDecel;
    if (mBombRotSpeed <= 0.0f) {
        mBombRotSpeed = 0.0f;
        JGeometry::TVec3f zDir;
        mRotMtx.getZDir(zDir);
        zDir.y = 0.0f;
        zDir.normalize();
        JGeometry::TVec3f yDir;
        mRotMtx.getYDir(yDir);
        yDir.x = 0.0f;
        yDir.normalize();
        mBaseMtx.set(mRotMtx);
        mpPathWalk->_94 = 1;
        mpPathWalk->forceTurn(yDir, zDir);
        if (mpSub != nullptr) {
            mpSub->setState(2);
        }
        setState(5);
        mAnmPlayer.setAnm(sAnmTable[5]);
    } else {
        Mtx mtx;
        PSMTXIdentity(mtx);
        PSMTXRotRad(mtx, 'y', mBombRotSpeed);
        PSMTXConcat(mRotMtx, mtx, mRotMtx);
    }
}

void TMapObjHeyho::initFunc_Sink() {
    mSinkVel = sFirstSinkVel;
    mSoundMgr->setSe(0x4005a);
}

void TMapObjHeyho::doFunc_Sink() {
    mSinkVel += sSinkAccel;
    mScale.y += mSinkVel;
    if (mScale.y > 1.0f) {
        mScale.y = 1.0f;
    }
    f32 speed = mpPathWalk->_90;
    speed += sAccel;
    if (speed > mWalkSpeed) {
        speed = mWalkSpeed;
        if (mScale.y >= 1.0f) {
            setState(0);
            mAnmPlayer.setAnm(sAnmTable[0]);
        }
    }
    mpPathWalk->_90 = speed;
    mPos.set(mBasePos);
    mRotMtx.set(mBaseMtx);
}

void TMapObjHeyho::initFunc_Recover() {
    mpPathWalk->_94 = 1;
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->resetFlag();
    }
}

void TMapObjHeyho::doFunc_Recover() {
    f32 speed = mpPathWalk->_90;
    speed += sAccel;
    if (speed > mWalkSpeed) {
        speed = mWalkSpeed;
        if (mScale.y >= 1.0f) {
            setState(0);
            mAnmPlayer.setAnm(sAnmTable[0]);
        }
    }
    mpPathWalk->_90 = speed;
    mPos.set(mBasePos);
    mRotMtx.set(mBaseMtx);
}

void TMapObjHeyho::initFunc_Die() {
    mpPathWalk->_94 = 0;
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->setFlag();
    }
    mGeoObjFlag &= ~0x2;
    mCheckKartHitFlags = 0;
    mGeoObjFlag |= 0x20;
}

void TMapObjHeyho::doFunc_Die() {}

void TMapObjHeyho::hitCheck(TMapObjHeyho *other) {
    ObjColBase *bounds = *mBounds;
    JGeometry::TVec3f otherPos;
    other->getPosition(&otherPos);
    if (bounds->IsHitCylinder(mPos, otherPos, *(ObjColCylinder *)*other->mBounds)) {
        JGeometry::TVec3f dir;
        dir.sub(mPos, otherPos);
        dir.normalize();
        dir.scale((*mBounds)->getBoundDepth());
        mPos.add(dir);
        mBasePos.set(mPos);
    }
}

StateObserver::StateFuncSet<TMapObjSubHeyho> TMapObjSubHeyho::sTable[3] = {
    { 0, &TMapObjSubHeyho::initFunc_Skate, &TMapObjSubHeyho::doFunc_Skate },
    { 1, &TMapObjSubHeyho::initFunc_Spin, &TMapObjSubHeyho::doFunc_Spin },
    { 2, &TMapObjSubHeyho::initFunc_Recover, &TMapObjSubHeyho::doFunc_Recover },
};

TAnmInfo TMapObjSubHeyho::sAnmInfos[1] = {
    { "/Objects/Heyho1.bca", nullptr, nullptr, 2, 0, 0, -1, 0 },
};

J3DAnmTexPattern *TMapObjSubHeyho::sHeyhoBtpAnm;

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
    mOffset.x = 250.0f;
    mOffset.y = 0.0f;
    mOffset.z = 0.0f;
    ResetState();
}

bool TMapObjSubHeyho::checkItemHitting() {
    bool ret = false;
    if (mGeoObjFlag & 4) {
        switch (mColItemObj->mItemKind) {
        case 0:
        case 1:
        case 2:
        case 7:
        case 8:
        case 9:
            ret = true;
            break;
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
    PSMTXMultVecSR(mtx, &mOffset, &out);
    mPos.add(*mpPos, out);
}

void TMapObjSubHeyho::makeFrontMtx(JGeometry::TPos3f *mtx) {
    JGeometry::TVec3f xDir, yDir, zDir;
    if (mpVel->isZero()) {
        mRotMtx.getZDir(zDir);
        if (zDir.isZero()) {
            zDir.set(0.0f, 0.0f, 1.0f);
        }
    } else {
        zDir.set(*mpVel);
    }
    zDir.normalize();
    yDir.set(0.0f, 1.0f, 0.0f);
    xDir.cross(yDir, zDir);
    xDir.normalize();
    mtx->setXYZDir(xDir, yDir, zDir);
}

void TMapObjSubHeyho::initFunc_Spin() {
    mSpinMtx.set(*mpMtx);
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~2;
}

void TMapObjSubHeyho::doFunc_Spin() {
    JGeometry::TVec3f out;
    PSMTXMultVecSR(mSpinMtx, &mOffset, &out);
    mPos.add(*mpPos, out);
}

void TMapObjSubHeyho::initFunc_Recover() {
    mRecoverFrame = 0.0f;
    mSpinMtx.ref(0, 3) = 0.0f;
    mSpinMtx.ref(1, 3) = 0.0f;
    mSpinMtx.ref(2, 3) = 0.0f;
}

void TMapObjSubHeyho::doFunc_Recover() {
    JGeometry::TQuat4f q1(0.0f, 0.0f, 0.0f, 1.0f);
    mSpinMtx.getQuat(q1);
    q1.normalize(q1);
    JGeometry::TPos3f mtx;
    makeFrontMtx(&mtx);
    JGeometry::TQuat4f q2(0.0f, 0.0f, 0.0f, 1.0f);
    mtx.getQuat(q2);
    q2.normalize(q2);
    q1.x = q1.x + (q2.x - q1.x) * mRecoverFrame;
    q1.y = q1.y + (q2.y - q1.y) * mRecoverFrame;
    q1.z = q1.z + (q2.z - q1.z) * mRecoverFrame;
    q1.w = q1.w + (q2.w - q1.w) * mRecoverFrame;
    q1.normalize(q1);
    mtx.setQuat(q1);
    JGeometry::TVec3f out;
    PSMTXMultVecSR(mtx, &mOffset, &out);
    mPos.add(*mpPos, out);
    mRecoverFrame += 0.01f;
    if (mRecoverFrame >= 1.0f) {
        setState(0);
    }
}

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

#include "JSystem/JAudio/JASFakeMatch2.h"
