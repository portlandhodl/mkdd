#include "Sato/ItemBomb.h"

#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DTexMtx.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/KartChecker.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/ResMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Yamamoto/kartCtrl.h"
#include "mathHelper.h"

#include "JSystem/JMath/JMath.h"

#include <math.h>

namespace
{
    f32 sQtLerpRatio = 0.2f;
    f32 sGravity = -1.75f;
    f32 sMiniBombGravity = -4.0f;
    f32 sMiniNotBombGravity = -2.5f;
    f32 sSpGravity = -3.0f;
    f32 sFallGravity = -1.5f;
    f32 sVelMag = 20.0f;
    f32 sSpVelMag = 120.0f;
    f32 sSpVelMagMiniGame = 120.0f;
    f32 sSpVelDownRatio = 0.998f;
    f32 sSpVelYRatio = 0.45f;
    f32 sSpVelYRatioMiniGame = 0.4f;
    f32 sReflectVelDownRatio = 0.5f;
    f32 sScale = 3.0f;
    f32 sScaleEquip = 1.0f;
    f32 sScaleUpRatio = 1.1f;
    f32 sAirFric = 0.999f;
    f32 sReleaseHeight = 60.0f;
    u8 sMaxNoColCnt = 5;
    u8 sSpMaxNoColCnt = 20;
    f32 sBoundsRadius = 30.0f;
    f32 sBoundRatio = 0.5f;
    f32 sMaxVelBoundY = 50.0f;
    f32 sHeightOffset = 40.0f;
    s16 sBlastWaitFrame = 300;
    s16 sNormReleaseBlastWaitFrame = 300;
    s16 sSpBlastWaitFrame = 90;
    u8 sMaxBombReactFrame = 20;
    f32 sStickMagXZ = 60.0f;
    f32 sMiniGameStickMagRatio = 2.75f;
    f32 sStickMagY = 15.0f;
    f32 sMiniStickYOffsetRatio = -0.05f;
    u8 sMaxGndBoundNum = 1;
    f32 sBoundStdVelMagY = 10.0f;
    f32 sFindNearKartLength = 1400.0f;
    f32 sBombEfctScale = 1.3f;

    JGeometry::TVec3s sBuildRotAngle(0, 0, 0);

    const char *sColorAnmName[7] = {
        "/Item/item_bomb_flash.bpk",
        "/Item/item_bomb_wait.bpk",
        "/Item/item_bomb_flash_fast.bpk",
        "/Item/item_bomb_p1.bpk",
        "/Item/item_bomb_p2.bpk",
        "/Item/item_bomb_p3.bpk",
        "/Item/item_bomb_p4.bpk",
    };
} // namespace

J3DAnmTransform *ItemBomb::mAnmTrans[3] = {nullptr, nullptr, nullptr};
J3DMtxCalc *ItemBomb::mAnmCalc[3] = {nullptr, nullptr, nullptr};
J3DAnmColor *ItemBomb::sColorAnm[7] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
JGeometry::TVec3f ItemBomb::sHandOffsetPos(21.0f, 31.0f, 0.0f);
J3DAnmObjMatColor *ItemBomb::sNowColorAnm;
int ItemBomb::sLocatorJointNo;

inline ItemBombBase::ItemBombBase()
{
    mBombEfct = new BombEffectObj();
    mBombEfctScale = sBombEfctScale;
    if (GetGeoObjMgr()->isBombBattle())
        mBombEfctScale = 1.5f;
    mIsColKart = 0;
}

void ItemBombBase::execute_doDisappear(int kartNo)
{
    if (RaceMgr::getManager()->getRaceMode() == 6 && !mIsColKart && kartNo >= 0)
        RCMGetKartChecker(kartNo)->setBombEvent(KartChecker::EVENT_2, nullptr);
}

ItemBomb::ItemBomb(u32 kind) : ItemObj(kind), ItemBombBase()
{
    mNewBounds = nullptr;
    reset();

    if (GetGeoObjMgr()->isBombBattle()) {
        mJuggle = new ItemJuggle(this);
        mBuild = new ItemBuild(this);
    }
}

void ItemBomb::reset()
{
    resetObject();
    mKartReaction = 8;
    mEquipScale = 1.0f;
    mEquipFlags = 0;
    _2cc = 0.0f;
    mGndBoundCnt = 0;
    mIsOnGnd = 0;
    mGravity = sGravity;
    mFoundKart = 0;
    mKartGone = 0;
    mColorFlg = 0;

    if (GetGeoObjMgr()->isBombBattle()) {
        mGravity = sMiniBombGravity;
    } else if (GetGeoObjMgr()->isMiniGame()) {
        mGravity = sMiniNotBombGravity;
    }

    mIsColKart = 0;
    _2d0.zero();
    _28 = 0.0f;
    _2c = sReflectVelDownRatio;
    mLerpRatio = sQtLerpRatio;
    _6c = sHeightOffset;
    _12c |= 0x200;

    ObjColBase *savedBounds = mNewBounds;
    if (savedBounds != nullptr) {
        mBounds = savedBounds;
        mBounds2 = mBounds;
    }
    if (mBounds != nullptr) {
        mBounds->setRadius(sBoundsRadius);
        mBounds->setScale(1.0f);
    }

    mBombEfct->reset();
    mBlastFrame = 0;
    mDisappearing = 0;
    mBlastWaitFrame = sBlastWaitFrame;

    if (sColorAnm[0] != nullptr) {
        mColorAnm.mAnmBase = sColorAnm[0];
        mColorAnm.initFrameCtrl(mColorAnm.mAnmBase);
        sNowColorAnm = &mColorAnm;
    }
    mColorAnm.getFrameCtrl()->reset();
    mColorAnm.getFrameCtrl()->setRate(0.0f);
    mBlastEmt = nullptr;

    setAnimation(BombBck0, 0);
}

const char *ItemBomb::getBmdFileName()
{
    return "/Item/item_bomb_L.bmd";
}

void ItemBomb::createModel(JKRSolidHeap *heap, u32 p2, u32 p3)
{
    mModel.createDifferedModel(heap, p2, p3 | 0x01000201, true);
    mColorAnm.mModel = &mModel;
    mColorAnm.mAnmBase = sColorAnm[0];
    mColorAnm.initFrameCtrl(mColorAnm.mAnmBase);
    sNowColorAnm = &mColorAnm;

#line 352
    JUT_ASSERT(mBounds != 0);

    mNewBounds = mBounds;
}

void ItemBomb::loadAnmData(J3DModelData *model)
{
    for (int i = 0; i < 3; i++) {
        void *anm = ResMgr::getPtr(ResMgr::mcArcMRAM, getBckName(i));
        mAnmTrans[i] = (J3DAnmTransform *)J3DAnmLoaderDataBase().load(anm, J3DLOADER_UNK_FLAG0);
        mAnmCalc[i] = J3DNewMtxCalcAnm(model->getMtxCalcType() & 0xf, mAnmTrans[i]);
    }
    for (u8 i = 0; i < 7; i++) {
        void *anm = ObjUtility::getPtrMainArc((char *)sColorAnmName[i]);
        J3DAnmObjMaterial::setupColorAnmData(&sColorAnm[i], model, anm);
    }
    sLocatorJointNo = model->getJointName()->getIndex("ef_fire");
}

const char *ItemBomb::getBckName(int idx)
{
    const char *ret = nullptr;
    switch (idx) {
    case 0:
        ret = "/Item/item_bomb_L_wait_a.bck";
        break;
    case 1:
        ret = "/Item/item_bomb_L_down.bck";
        break;
    case 2:
        ret = "/Item/item_bomb_L_wait_b.bck";
        break;
    }
    return ret;
}

void ItemBomb::update()
{
    J3DModelData *modelData = mModel.getModelData();
    Mtx lightMtx;
    ObjUtility::getCamDependLightMtx(0, mPos, lightMtx);
    for (u32 i = 0; i < modelData->getMaterialNum(); i++) {
        J3DTexMtx *texMtx = modelData->getMaterialNodePointer(i)->getTexMtx(0);
        if (texMtx)
            texMtx->setEffectMtx(lightMtx);
    }
    modelData->getJointNodePointer(0)->setMtxCalc(mAnmCalc[mAnmType]);
    mAnmTrans[mAnmType]->setFrame(mAnmFrame);
    mColorAnm.anmFrameProc();
    mColorAnm.changeMatAnm(sNowColorAnm);
    mModel.update(0);
    sNowColorAnm = &mColorAnm;
}

void ItemBomb::setAnimation(BombBck type, int frame)
{
    mAnmType = type;
    mAnmFrame = frame;
    mAnmRepeat = 0;
}

void ItemBomb::deleteEffect()
{
    if (mBlastEmt == nullptr)
        return;

    GetJPAMgr()->deleteEmt(mBlastEmt);
    mBlastEmt = nullptr;
}

bool ItemBomb::IsHitKart(int kart, const JGeometry::TVec3f &pos, f32 radius)
{
    bool ret = false;
    if (mState != 1)
        ret = mBounds->IsHitSphere(mColPos, pos, radius);
    return ret;
}

void ItemBomb::doUniv()
{
    if (mAnmRepeat == 0) {
        mAnmFrame++;
        if (mAnmFrame >= mAnmTrans[mAnmType]->getFrameMax()) {
            mAnmFrame = 0;
            if (mAnmType == 1)
                mAnmRepeat = 1;
        }
    }

    if (!mFoundKart && mBlastWaitFrame - mBlastFrame < sSpBlastWaitFrame) {
        if (mState == 2 && !mColorFlg) {
            mColorAnm.mAnmBase = sColorAnm[0];
            mColorAnm.initFrameCtrl(mColorAnm.mAnmBase);
            mColorAnm.getFrameCtrl()->reset();
            sNowColorAnm = &mColorAnm;
        }
        mColorAnm.getFrameCtrl()->update();
        mColorFlg = 1;
    } else if (!GetGeoObjMgr()->isBombBattle() && mState == 2) {
        if (mIsOnGnd)
            mColorAnm.getFrameCtrl()->update();
        if (mFoundKart && mColorAnm.getFrameCtrl()->checkState(1))
            mKartGone = 1;
    }

    doScale();
    (this->*mStateFunc)();

    if (!mDisappearing) {
        if ((!mFoundKart && !GetGeoObjMgr()->isBombBattle() && mState == 2) || mState == 3 || mState == 4 || mState == 6)
            mBlastFrame++;

        bool blast = false;
        if (mState == 2) {
            if (GetGeoObjMgr()->isBombBattle()) {
                if (mIsOnGnd)
                    blast = true;
            } else if (mKartGone) {
                blast = true;
            }
        }

        if (mBlastFrame >= mBlastWaitFrame || blast || (mTransferFlags & 1) || (mTransferFlags & 8) || mHitObjNum) {
            setStateDisappear(true);
            mBounds = mBombEfct->getBlastBound();
            mBounds2 = mBombEfct->mSphere2;
            mDisappearing = 1;
        }
    }

    setSparkTranslation();
}

void ItemBomb::calcScale(f32 equipScale, f32 scale)
{
    if (tstAnyObjectHitting())
        return;
    if (*(volatile int *)&mState >= 8)
        return;

    switch (mState) {
    case 1:
    case 5:
        doEquipScaleUp(equipScale);
        break;
    case 7:
        doForceDisappearScaleDown();
        break;
    default:
        f32 ratio = sScaleUpRatio;
        if (mEquipScale < scale)
            mEquipScale *= ratio;
        setScaleCol(mEquipScale);
        break;
    }
}

void ItemBomb::doScale()
{
    calcScale(sScaleEquip, sScale);
}

void ItemBomb::setSparkTranslation()
{
    if (mBlastEmt == nullptr)
        return;

    MtxPtr mtx = mModel.getModel()->getAnmMtx(sLocatorJointNo);
    JGeometry::TVec3f pos(mtx[0][3], mtx[1][3], mtx[2][3]);

    if (mState == 1) {
        JGeometry::TVec3f kartVel;
        ObjUtility::getKartVel(mOwnerNum, &kartVel);
        pos += kartVel;
    }

    mBlastEmt->setTranslation(pos.x, pos.y, pos.z);
}

ItemObj::ItemColFlag ItemBomb::calcColReaction(ItemObj *other)
{
    return cColFlag_4;
}

void ItemBomb::initEquip()
{
    if (GetGeoObjMgr()->isBombBattle())
        mMulti->ParamInit(sBuildRotAngle);
    else
        mpHandAnmMtx = getHandAnmMtx();

    mVel.zero();
    mPos.zero();
    createEffect();
    setMiniGameBombColor();
}

void ItemBomb::setMiniGameBombColor()
{
    if (GetGeoObjMgr()->isBombBattle()) {
        u8 color = 0;
        switch (RCMGetKartChecker(mOwnerNum)->getPlayerKartColor()) {
        case 0:
            color = 3;
            break;
        case 1:
            color = 4;
            break;
        case 2:
            color = 5;
            break;
        case 3:
            color = 6;
            break;
        }

        mColorAnm.mAnmBase = sColorAnm[color];
        mColorAnm.initFrameCtrl(mColorAnm.mAnmBase);
        sNowColorAnm = &mColorAnm;
    }
}

void ItemBomb::doEquip()
{
    if (GetGeoObjMgr()->isBombBattle()) {
        mMulti->MoveEquip();
    } else {
        doHandOneEquip(false, 0.0f);
    }
}

void ItemBomb::initRelease()
{
    f32 velMag = sSpVelMag;
    f32 stickMag = 1.0f;
    f32 velYRatio = sSpVelYRatio;

    if (!GetGeoObjMgr()->isBombBattle())
        mBlastWaitFrame = sNormReleaseBlastWaitFrame;

    if (RaceMgr::getManager()->mRaceInfo->mRaceMode == 6) {
        velMag = sSpVelMagMiniGame;
        stickMag = sMiniGameStickMagRatio;
        velYRatio = sSpVelYRatioMiniGame;
        f32 stickY = KartCtrl::getKartCtrl()->GetItemStickY(mOwnerNum);
        velYRatio = stickY * sMiniStickYOffsetRatio + velYRatio;
    }

    mMaxVel = velMag;
    u8 noColCnt = sMaxNoColCnt;
    mEquipFlags = 0xffffffff;
    _12c |= 2;
    _12c |= 1;

    if (GetGeoObjMgr()->isBombBattle()) {
        mEquipFlags = 0xffffffff;
        mEquipFlags &= ~(1 << mOwnerNum);
    } else {
        setFrameNoColKart(noColCnt, false);
    }

    initMoveColGround();

    ObjUtility::getKartBodyMtx(mOwnerNum);
    JGeometry::TVec3f dir;
    setInitThrowDir(&dir, velYRatio, false);
    calcBananaFirstQuat(dir);
    mVel = dir;
    mVel.scale(mMaxVel);
    initThrowVel(sStickMagXZ * stickMag, sStickMagY * stickMag, mGravity);
}

void ItemBomb::doRelease()
{
    if (PSVECMag(&mVel) > mMaxVel)
        mVel.scale(sSpVelDownRatio);

    JGeometry::TVec3f dir = mVel;
    stVecNormalize(dir);

    mVel.scale(sAirFric);
    mVel.y += mGravity;
    doMovePos();
    doMoveColGround(nullptr, false);

    if (mIsOnGnd) {
        if (mAnmType == 0) {
            mAnmType = 1;
            mAnmFrame = 0;
            mAnmRepeat = 0;
        } else if (mAnmRepeat != 0) {
            mAnmType = 2;
            mAnmFrame = 0;
            mAnmRepeat = 0;
        }
    }

    if (!mIsOnGnd) {
        JGeometry::TVec3f vec = mVel;
        if (PSVECMag(&vec) > 0.0f) {
            vec.normalize();
            stVec2QtUpdate(_44, dir, vec);
        }
    } else {
        JGeometry::TVec3f up;
        getModelUpDir(&up);
        if (!up.equals(mNormal))
            stVec2QtUpdate(_34, _44, up, mNormal);
    }

    if (mGndBoundCnt && !GetGeoObjMgr()->isBombBattle() && !mFoundKart && !mColorFlg) {
        mFoundKart = findAnyKart();
        if (mFoundKart) {
            mColorAnm.mAnmBase = sColorAnm[2];
            mColorAnm.initFrameCtrl(mColorAnm.mAnmBase);
            mColorAnm.getFrameCtrl()->reset();
            sNowColorAnm = &mColorAnm;
        }
    }

    if (mState == 2 && !(mTransferFlags & 0x100))
        mSoundMgr->setSe(0x40004);
}

void ItemBomb::initSpRelease()
{
    mMaxVel = sVelMag;
    u8 maxNoCol = sSpMaxNoColCnt;
    mEquipFlags = 0xffffffff;
    _12c |= 2;
    _12c |= 1;

    if (GetGeoObjMgr()->isBombBattle()) {
        mEquipFlags = 0xffffffff;
        mEquipFlags &= ~(1 << mOwnerNum);
    } else {
        setFrameNoColKart(maxNoCol, false);
    }

    initMoveColGround();
    mGravity = sSpGravity;

    ObjUtility::getKartZdir(mOwnerNum, &mVel);
    mVel.scale(-mMaxVel);

    f32 compLen = 0.0f;
    getCompVecLength(_2d0, compLen);

    JGeometry::TVec3f comp(_2d0);
    f32 sqrtVal = JMAFastSqrt(2.0f * -mGravity * sReleaseHeight);
    comp.scale((compLen * -mGravity) / sqrtVal);
    mVel.add(comp);

    JGeometry::TVec3f dir = mVel;
    stVecNormalize(dir);
    calcBananaFirstQuat(dir);

    mBlastWaitFrame = sSpBlastWaitFrame;

    mColorAnm.getFrameCtrl()->reset();
}

void ItemBomb::doSpRelease()
{
    doRelease();
}

void ItemBomb::initDisappear()
{
    if (GetGeoObjMgr()->isBombBattle())
        _12c &= ~1;
    else
        mEquipFlags = 0xffffffff;

    mEquipScale = 0.0f;

    JGeometry::TVec3f up;
    JGeometry::TVec3f frDir;
    getModelFrDir(&frDir);

    if (mTransferFlags & 2)
        up = mNormal;
    else
        up.set(0.0f, 1.0f, 0.0f);

    JGeometry::TPos3f mtx;
    ObjUtility::setRMtx(mtx, up, frDir);
    mtx[0][3] = mColPos.x;
    mtx[1][3] = mColPos.y;
    mtx[2][3] = mColPos.z;

    int color = 0;
    if (GetGeoObjMgr()->isBombBattle()) {
        switch (RCMGetKartChecker(mOwnerNum)->getPlayerKartColor()) {
        case 0:
            color = 0;
            break;
        case 1:
            color = 1;
            break;
        case 2:
            color = 2;
            break;
        case 3:
            color = 3;
            break;
        }
    }

    mBombEfct->init(mtx, color, mBombEfctScale);
    _12c |= 0x2000;

    if (mBlastEmt != nullptr) {
        mBlastEmt->becomeInvalidEmitter();
        mBlastEmt = nullptr;
    }
}

void ItemBomb::doDisappear()
{
    mDisScaleAnm.setAll(mEquipScale);

    if (mWaitFrame >= sMaxBombReactFrame)
        mKartReaction = 2;

    if (mBombEfct->execute()) {
        mState = 10;
        int owner = mOwnerNum;
        execute_doDisappear(owner);
    }

    setModelMatrixAndScale(_34);
}

void ItemBomb::initFall()
{
    initmoveFall();
    if (GetGeoObjMgr()->isBombBattle())
        resetNoColParam();

    mGravity = sFallGravity;

    mColorAnm.getFrameCtrl()->reset();

    createEffect();

    u8 maxNoCol = sSpMaxNoColCnt;
    mEquipFlags = 0xffffffff;
    _12c |= 2;
    _12c |= 1;

    if (GetGeoObjMgr()->isBombBattle()) {
        mEquipFlags = 0xffffffff;
        mEquipFlags &= ~(1 << mOwnerNum);
    } else {
        setFrameNoColKart(maxNoCol, false);
    }

    initMoveColGround();

    if (GetGeoObjMgr()->isBombBattle()) {
        _12c &= ~1;
        mEquipFlags = 0;
    }
}

void ItemBomb::initDivested()
{
    initmoveDivested();
    setMiniGameBombColor();
}

void ItemBomb::doFall()
{
    if (!(_12c & 1))
        checkStartColItemFrame(30);

    doRelease();

    if (GetGeoObjMgr()->isBombBattle() && mIsOnGnd)
        setStateForceDisappear();
}

void ItemBomb::initOccur(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &vel)
{
    ItemObj::initOccur(pos, vel);
    mGravity = sFallGravity;

    mColorAnm.getFrameCtrl()->reset();

    createEffect();
}

void ItemBomb::initForceDisappear()
{
    deleteEffect();
    ItemObj::initForceDisappear();
}

void ItemBomb::moveNormal(CrsGround &ground)
{
    f32 height = ground.getHeight();
    if (mPos.y < height) {
        mTransferFlags |= 2;
        mTransferFlags |= 0x100;

        if (mState == 2) {
            if (GetGeoObjMgr()->isBombBattle()) {
                mIsOnGnd = 1;
                mVel.y = 0.0f;
            } else if (mGndBoundCnt < sMaxGndBoundNum) {
                mGndBoundCnt++;
                mVel.scale(sBoundRatio);
                JGeometry::TVec3f proj;
                proj.scale(mNormal.dot(mVel), mNormal);
                JGeometry::TVec3f horiz;
                horiz.sub(mVel, proj);
                horiz.y = sBoundStdVelMagY;
                proj.scale(-1.0f);
                mVel.add(horiz, proj);
                mVel.y = mVel.y > sMaxVelBoundY ? sMaxVelBoundY : mVel.y;
            } else {
                mPos.y = height;
                mVel.y = 0.0f;
                JGeometry::TVec3f vel;
                ground.getVelocity(&vel);
                mVel.x = vel.x;
                mVel.z = vel.z;
                if (!mIsOnGnd && !mFoundKart) {
                    mColorAnm.mAnmBase = sColorAnm[1];
                    mColorAnm.initFrameCtrl(mColorAnm.mAnmBase);
                    mColorAnm.getFrameCtrl()->reset();
                    sNowColorAnm = &mColorAnm;
                }
                mIsOnGnd = 1;
                mPos.y = height;
            }
        } else {
            mPos.y = height;
            mVel.y = 0.0f;
            JGeometry::TVec3f vel;
            ground.getVelocity(&vel);
            mVel.x = vel.x;
            mVel.z = vel.z;
            mIsOnGnd = 1;
        }
    }
}

void ItemBomb::moveCannon(CrsGround &ground)
{
    setStateDisappear(true);
    mBounds = mBombEfct->getBlastBound();
    mBounds2 = mBombEfct->mSphere2;
    mDisappearing = 1;
}

const bool ItemBomb::checkKartColDisappear(int kart)
{
    if (mDirectHitKartNo == -1)
        mDirectHitKartNo = kart;
    mIsColKart = 1;
    return false;
}

void ItemBomb::setCurrentViewNo(u32 viewNo)
{
    Mtx lightMtx;
    mModel.setCurrentViewNo(viewNo);
    ObjUtility::getCamDependLightMtx(viewNo, mPos, lightMtx);
    mModel.setEffectMtx(lightMtx, 0);
}

void ItemBomb::createEffect()
{
    if (!GetGeoObjMgr()->isBombBattle() || RaceMgr::getManager()->getConsoleNumber() < 3) {
        if (mBlastEmt == nullptr)
            mBlastEmt = GetJPAMgr()->createEmt("bombchiri01", mPos);
    }
}

u8 ItemBomb::findAnyKart()
{
    bool found = false;
    int kart = ObjUtility::findNearKart(mPos, sFindNearKartLength, mOwnerNum);
    if (kart >= 0 && kart != mOwnerNum)
        found = true;
    return found;
}

u8 ItemBomb::getMaxBombReactFrame()
{
    return sMaxBombReactFrame;
}

ItemBombSuc::ItemBombSuc(u32 kind) : ItemObjSuc(kind) {}

void ItemBombSuc::reset()
{
    ItemObjSuc::reset();
    mKartReaction = 15;
    _1fc = 3;
    mSucKind = 8;
}


#include "JSystem/JAudio/JASFakeMatch2.h"
