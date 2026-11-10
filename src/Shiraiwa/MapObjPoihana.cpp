#include "Shiraiwa/Objects/MapObjPoihana.h"

#include <dolphin/types.h>
#include <dolphin/os/OSTime.h>

#include "JSystem/J3D/J3DModel.h"
#include "JSystem/JGeometry/Matrix.h"
#include "JSystem/JGeometry/Util.h"
#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/ResMgr.h"
#include "Sato/GeographyObj.h"
#include "Sato/ItemThunder.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Yamamoto/kartCtrl.h"
#include "JSystem/JGeometry/Vec.h"
#include "types.h"

const u8 TMapObjPoihana::sAnmTable[7] = { 0, 1, 1, 1, 2, 0, 0 };

J3DAnmTexPattern *TMapObjPoihana::sPoihanaBtpAnm;
f32 TMapObjPoihana::sSinkOffset;
TPoihanaSupervisor *TMapObjPoihana::sSupervisor;

s16 TMapObjPoihana::sDownRotFrame = 30;
s16 TMapObjPoihana::sDownFrame = 200;
f32 TMapObjPoihana::sFirstDownVel = 60.0f;
f32 TMapObjPoihana::sFirstDownVelBanana = 40.0f;
f32 TMapObjPoihana::sDownGravity = 4.0f;
s16 TMapObjPoihana::sWakeRotFrame = 30;
f32 TMapObjPoihana::sFirstWakeVel = 70.0f;
f32 TMapObjPoihana::sWakeGravity = 4.0f;
f32 TMapObjPoihana::sWalkWaterRatio = 0.6f;
f32 TMapObjPoihana::sDefaultSearchRadius = 6000.0f;
f32 TMapObjPoihana::sRunSpeedRatio = 2.0f;
f32 TMapObjPoihana::sRunRotRatio = 4.0f;
s16 TMapObjPoihana::sKartIgnoreFrame = 240;
s16 TMapObjPoihana::sWaterDecelBand = 200;
s16 TMapObjPoihana::sReSearchFrame = 30;
f32 TMapObjPoihana::sRunVelScale = 30.0f;
f32 TMapObjPoihana::sWalkSpeed = 9.0f;

TAnmInfo TMapObjPoihana::sAnmInfos[3] = {
    { "/Objects/Poihana1_wait.bck", nullptr, nullptr, 0, 0, 0, -1, 0 },
    { "/Objects/Poihana1_damage.bck", nullptr, nullptr, 0, 0, 0, -1, 0 },
    { "/Objects/Poihana1_throw.bck", nullptr, nullptr, 2, 0, 1, -1, 0 },
};

StateObserver::StateFuncSet<TMapObjPoihana> TMapObjPoihana::sTable[7] = {
    { 0, &TMapObjPoihana::initFunc_Walk, &TMapObjPoihana::doFunc_Walk },
    { 1, &TMapObjPoihana::initFunc_Down, &TMapObjPoihana::doFunc_Down },
    { 2, &TMapObjPoihana::initFunc_Buried, &TMapObjPoihana::doFunc_Buried },
    { 3, &TMapObjPoihana::initFunc_Wake, &TMapObjPoihana::doFunc_Wake },
    { 4, &TMapObjPoihana::initFunc_Throw, &TMapObjPoihana::doFunc_Throw },
    { 5, &TMapObjPoihana::initFunc_Run, &TMapObjPoihana::doFunc_Run },
    { 6, &TMapObjPoihana::initFunc_Rest, &TMapObjPoihana::doFunc_Rest },
};

#line 79
TMapObjPoihana::TMapObjPoihana(const CrsData::SObject &obj)
    : TKartThrower(obj), StateObserver(), JKRDisposer(), mLink(this), mBtp(), mAnmPlayer()
{
    mWalkRotSpeed = 0.015f;
    mWalkParam = 0.3f;
    mThrowPow = 50;
    JUT_ASSERT(mObjData->pathID != 0xffff);
    NewAnmCtrl();
    createSoundMgr();
    mpGround = new CrsGround(RaceMgr::sRaceManager->getCourse());
    TAnmPlayer::resetAnimations(sAnmInfos, 3);
    mPathWalk = new TPathWalk(obj);
    mPathParam = mObjData->mParam4;
    if (mPathParam <= 0) {
        mPathParam = 4000;
    }
    if (getSupervisor() == nullptr) {
        sSupervisor = new TPoihanaSupervisor();
    }
    sSupervisor->entry(this);
}

TMapObjPoihana::~TMapObjPoihana() {
    delete mPathWalk;
    sSupervisor = nullptr;
}

const char *TMapObjPoihana::getBmdFileName() {
    static const char *cBmdName = "/Objects/Poihana1.bmd";
    return cBmdName;
}

const char *TMapObjPoihana::getShadowBmdFileName() {
    static const char *cBmdName = "/Objects/Poihana1Shadow.bmd";
    return cBmdName;
}

void TMapObjPoihana::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    TAnmPlayer::loadAnimations(sAnmInfos, 3, modelData, ResMgr::mcArcCourse);
    void *btp = ObjUtility::getPtrCourseArc("/Objects/Poihana1.btp");
    J3DAnmObjMaterial::setupTexPatternAnmData(&sPoihanaBtpAnm, modelData, btp);
}

void TMapObjPoihana::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createDifferedModel(heap, viewNo, 0x80000, false);
    TAnmPlayer::registAnimations(mAnmCtrl, &mModel, sAnmInfos, 3);
    mBtp.mModel = &mModel;
    mBtp.mAnmBase = sPoihanaBtpAnm;
    mBtp.initFrameCtrl(mBtp.mAnmBase);
}

void TMapObjPoihana::createColModel(J3DModelData *modelData) {
    static f32 cBoundHeight = (270.0f * mScale.y) / mScale.x;
    createBoundsCylinder(modelData, 200.0f, cBoundHeight);
    f32 y = -230.0f;
    JGeometry::TVec3f pos(0.0f, y, 0.0f);
    setColObjPos(pos, 0);
}

void TMapObjPoihana::changeAllState(u16 state) {
    setState(state);
}

void TMapObjPoihana::InitExec() {
    const StateFuncSet<TMapObjPoihana> *func = FindStateFunc(getState(), sTable, 7);
    (this->*func->mInitFunc)();
}

void TMapObjPoihana::MoveExec() {
    const StateFuncSet<TMapObjPoihana> *func = FindStateFunc(getState(), sTable, 7);
    (this->*func->mExecFunc)();
}

void TMapObjPoihana::update() {
    mBtp.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}

void TMapObjPoihana::reset() {
    resetObject();
    _58 = 14;
    mCheckKartHitFlags = -1;
    mGeoObjFlag |= 2;
    mGeoObjFlag &= ~1;
    StateObserver::ResetState();
    mAnmPlayer.init(mAnmCtrl, sAnmInfos, 3);
    mBtp.resetFrame();
    u16 frame = mObjData->mParam1;
    if (frame >= 4) {
        frame = 0;
    }
    mBtp.setFrame((f32)frame);
    mThrowPow = mObjData->mParam2;
    if (mThrowPow < 0) {
        mThrowPow = 0;
    }
    u32 downRotFrame = (u8)mObjData->mParam3;
    if (downRotFrame > 90) {
        downRotFrame = 90;
    }
    f32 rot = (f32)(u8)downRotFrame;
    mDownRotFrame = (s16)(rot * 182.09f);
    mSearchRadius = sDefaultSearchRadius;
    f32 radius = mSearchRadius;
    if (radius < (f32)mPathParam) {
        mSearchRadius = (f32)mPathParam;
    }
    clearAllIgnore();
    mKartNo = -1;
    mSearchTimer = 0;
    mDownVel = 0.0f;
    mInWater = 0;
    mHeightOffset = 230.0f * mScale.y;
    PSMTXIdentity((Mtx &)mDownMtx);
    PSMTXIdentity((Mtx &)mWakeMtx);
    mPos.y += mHeightOffset;
    mWalkSpeed = sWalkSpeed * mScale.z;
    mPathWalk->init(&mPathPos, &mVel, &mRotMtx);
    mPathWalk->setParameters(mWalkSpeed, mWalkRotSpeed, mWalkParam, 0.0f, 0.0f);
    mPathWalk->reset();
    mKartPos.zero();
    mThrowDir.zero();
    mPathPos.zero();
    PSMTXIdentity((Mtx &)mRotMtx);
    mReaction.setFlg(3, 2);
    JGeometry::TVec3f pos(0.0f, -(mHeightOffset / mScale.y), 0.0f);
    setColObjPos(pos, 0);
}

void TMapObjPoihana::calc() {
    mPathWalk->update();
    StateObserver::ExecuteState();
    mAnmPlayer.update();
    moveShadowModel();
}

void TMapObjPoihana::changeDownState(f32) {
}

void TMapObjPoihana::checkItemHitting() {
}

void TMapObjPoihana::doKartColCallBack(int kartNo) {
    switch (getState()) {
    case 0:
    case 4:
    case 5:
    case 6: {
        u64 status = GetKartCtrl()->GetCarStatus(kartNo);
        _58 = 14;
        if ((status & 0x40000) != 0) {
            ObjUtility::getKartPos(kartNo, &mKartPos);
            mDownVel = sFirstDownVel;
            setState(1);
            mAnmPlayer.setAnm(sAnmTable[1]);
        }
        else {
            if (isIgnore(kartNo)) {
                return;
            }
            JGeometry::TVec3f kartPos;
            ObjUtility::getKartPos(kartNo, &kartPos);
            kartPos.sub(mPos);
            kartPos.normalize();
            mPathWalk->forceTurn(kartPos);
            ignoreKart(kartNo);
            mPathWalk->_88->mFlag = 0;
            setState(4);
            mAnmPlayer.setAnm(sAnmTable[4]);
        }
        return;
    }
    case 1:
    case 3:
        _58 = 0;
        return;
    case 2:
    default:
        _58 = 1;
        return;
    }
}

bool TMapObjPoihana::isIgnore(int kartNo) {
    return mIgnoreKart[kartNo] != 0;
}

void TMapObjPoihana::ignoreKart(int kartNo) {
    mIgnoreKart[kartNo] = sKartIgnoreFrame;
}

void TMapObjPoihana::ignoreUpdate() {
}

void TMapObjPoihana::clearAllIgnore() {
    for (int i = 0; i < 8; i++) {
        mIgnoreKart[i] = 0;
    }
}

void TMapObjPoihana::initFunc_Walk() {
    changeWalkSpeed(1.0f, 1.0f);
    mPathWalk->_94 = 1;
    mInWater = 0;
    f32 y = mPos.y - mHeightOffset;
    f32 z = mPos.z;
    f32 x = mPos.x;
    mPathPos.x = x;
    mPathPos.y = y;
    mPathPos.z = z;
}

void TMapObjPoihana::doFunc_Walk() {
    updatePosition();
    if (mGeoObjFlag & 4) {
        switch (mColItemObj->mItemKind) {
        case 7:
        case 8:
        case 9:
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
            mKartPos = mColItemObj->mPos;
            mDownVel = sFirstDownVel;
            setState(1);
            u32 anm = sAnmTable[1];
            mAnmPlayer.setAnm(anm);
            break;
        case 5:
        case 6:
            if (mColItemObj->mTransferFlags & 2) {
                mKartPos = mColItemObj->mPos;
                mDownVel = sFirstDownVelBanana;
                setState(1);
                u32 anm = sAnmTable[1];
                mAnmPlayer.setAnm(anm);
            }
            break;
        }
    }
    for (int i = 0; i < 8; i++) {
        if (mIgnoreKart[i] != 0) {
            mIgnoreKart[i]--;
        }
    }
    changeSpeedForWater(1.0f, 1.0f);
    int kartNo = searchKart();
    if (kartNo != -1) {
        mKartNo = kartNo;
        setState(5);
        mAnmPlayer.setAnm(sAnmTable[5]);
    }
}

void TMapObjPoihana::updatePosition() {
    JGeometry::TVec3f diff;
    diff.sub(mPathPos, mObjData->position);
    f32 range = (f32)mPathParam - 500.0f;
    if (diff.squared() > range * range) {
        setState(6);
        mAnmPlayer.setAnm(sAnmTable[6]);
    }
    mPos.x = mPathPos.x;
    mPos.y = mPathPos.y + mHeightOffset;
    mPos.z = mPathPos.z;
}

void TMapObjPoihana::changeSpeedForWater(f32 speedScale, f32 rotScale) {
    mpGround->search(mPos);
    f32 waterDiff = mpGround->getWaterHeight() - mPathPos.y;
    if (waterDiff < 0.0f) {
        if (mInWater) {
            changeWalkSpeed(speedScale, rotScale);
            mInWater = 0;
        }
    }
    else {
        f32 ratio = SiUtil::getNormalRange(waterDiff, 0.0f, (f32)sWaterDecelBand);
        ratio = sWalkWaterRatio + (1.0f - sWalkWaterRatio) * (1.0f - ratio);
        changeWalkSpeed(speedScale * ratio, rotScale * ratio);
        mInWater = 1;
    }
}

int TMapObjPoihana::searchKart() {
    for (int i = 0; i < 8; i++) {
        if (mIgnoreKart[i] != 0) {
            mIgnoreKart[i]--;
        }
    }
    if (mSearchTimer < sReSearchFrame) {
        mSearchTimer++;
        return -1;
    }
    mSearchTimer = 0;
    f32 best = mSearchRadius * mSearchRadius;
    int result = -1;
    for (int i = 0; i < RaceMgr::sRaceManager->mRaceInfo->mKartNum; i++) {
        if (mIgnoreKart[i] != 0) {
            continue;
        }
        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(i, &kartPos);
        kartPos.sub(mPos);
        f32 dist = kartPos.squared();
        if (dist < best) {
            best = dist;
            result = i;
        }
    }
    return result;
}

void TMapObjPoihana::changeWalkSpeed(f32 speedScale, f32 rotScale) {
    mPathWalk->setParameters(mWalkSpeed * speedScale, mWalkRotSpeed * rotScale, mWalkParam, 0.0f, 0.0f);
    mAnmPlayer.mController->getCurFrameCtrl()->setRate(speedScale);
}

static JGeometry::TVec3f yAxis;
static s8 yAxisInit;

void TMapObjPoihana::initFunc_Down() {
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= 7;
    if (!yAxisInit) {
        yAxis.set(0.0f, 1.0f, 0.0f);
        yAxisInit = 1;
    }
    JGeometry::TVec3f dir;
    dir.sub(mKartPos, mPos);
    dir.y = 0.0f;
    dir.normalize();
    JGeometry::TVec3f axis;
    axis.cross(dir, yAxis);
    axis.normalize();
    mDownMtx.setRotate(axis, 3.1415927f / (f32)sDownRotFrame);
    mWakeMtx.setRotate(axis, -3.1415927f / (f32)sWakeRotFrame);
    mPathWalk->_94 = 0;
    dir.negate();
    dir.y = 0.0f;
    dir.normalize();
    dir.scale(mDownVel);
    mVel = dir;
}

void TMapObjPoihana::doFunc_Down() {
    bool rot = true;
    if (getStateCount() < (u32)sDownRotFrame) {
        PSMTXConcat((Mtx &)mDownMtx, (Mtx &)mRotMtx, (Mtx &)mRotMtx);
        rot = false;
    }
    JGeometry::TVec3f oldPos = mPos;
    mPos += mVel;
    mVel.y -= sDownGravity;
    fixWall(mPos, oldPos);
    mpGround->search(mPos);
    if (mPos.y < mpGround->getHeight() - sSinkOffset) {
        mPos.y = mpGround->getHeight() - sSinkOffset;
        if (rot) {
            setState(2);
            mAnmPlayer.setAnm(sAnmTable[2]);
        }
    }
}

void TMapObjPoihana::fixWall(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &vel) {
    CrsGround ground(RaceMgr::sRaceManager->getCourse());
    ground.search(pos, vel);
    JGeometry::TVec3f normal;
    f32 depth = ground.getWallNormal(&normal, nullptr);
    if (depth > -1.0f) {
        normal.normalize();
        mPos.scaleAdd(depth, normal, pos);
    }
}

void TMapObjPoihana::initFunc_Buried() {
    mCheckKartHitFlags = -1;
    mGeoObjFlag |= 2;
    mReaction.setFlg(8, 2);
    mReaction.setFlg(9, 2);
    mReaction.setFlg(7, 0);
    mReaction.setFlg(2, 2);
    mReaction.setFlg(0, 1);
    mReaction.setFlg(1, 1);
    JGeometry::TVec3f pos;
    pos.y = 0.5f * mHeightOffset / mScale.y;
    pos.x = 0.0f;
    pos.z = 0.0f;
    setColObjPos(pos, 0);
}

void TMapObjPoihana::doFunc_Buried() {
    if (getStateCount() > (u32)sDownFrame) {
        setState(3);
        mAnmPlayer.setAnm(sAnmTable[3]);
    }
}

void TMapObjPoihana::initFunc_Wake() {
    mVel.set(0.0f, sFirstWakeVel, 0.0f);
}

void TMapObjPoihana::doFunc_Wake() {
    bool rot = true;
    if (getStateCount() < (u32)sWakeRotFrame) {
        PSMTXConcat((Mtx &)mWakeMtx, (Mtx &)mRotMtx, (Mtx &)mRotMtx);
        rot = false;
    }
    mPos += mVel;
    mVel.y -= sWakeGravity;
    mpGround->search(mPos);
    JGeometry::TVec3f normal;
    f32 depth = mpGround->getWallNormal(&normal, nullptr);
    if (depth > -1.0f) {
        normal.normalize();
        mPos.scaleAdd(depth, normal);
    }
    if (mVel.y < 0.0f) {
        if (mPos.y < mpGround->getHeight() + mHeightOffset) {
            mPos.y = mpGround->getHeight() + mHeightOffset;
            if (rot) {
                setState(6);
                mAnmPlayer.setAnm(sAnmTable[6]);
            }
        }
    }
}

void TMapObjPoihana::initFunc_Throw() {
    mPathWalk->_94 = 0;
}

void TMapObjPoihana::doFunc_Throw() {
    if (!mAnmPlayer._10) {
        setState(0);
        s8 anm = (s8)sAnmTable[0];
        if (anm != mAnmPlayer.mController->mTrans->mNowAnmNo) {
            mAnmPlayer._11 = anm;
            mAnmPlayer._10 = 1;
            mAnmPlayer._e |= 1;
        }
    }
}

void TMapObjPoihana::initFunc_Run() {
}

void TMapObjPoihana::doFunc_Run() {
}

void TMapObjPoihana::callWalkSound() {
}

void TMapObjPoihana::initFunc_Rest() {
}

void TMapObjPoihana::doFunc_Rest() {
}

void TMapObjPoihana::getKartThrowDirPow(JGeometry::TVec3f *dir, f32 *pow, int kartNo) {
    if (dir != nullptr) {
        getThrowDir(dir, kartNo);
    }
    if (pow != nullptr) {
        *pow = (f32)getThrowPow();
    }
}

void TMapObjPoihana::getThrowDir(JGeometry::TVec3f *dir, s16 angle) {
}

void TMapObjPoihana::thunderDown() {
}

TPoihanaSupervisor::TPoihanaSupervisor() {
    mList.initiate();
    ItemThunder::entrySuperVisor(this);
}

void TPoihanaSupervisor::entry(TMapObjPoihana *obj) {
    mList.append(&obj->mLink);
}

void TPoihanaSupervisor::thunderDownAll() {
}

