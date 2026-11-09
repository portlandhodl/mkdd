#include "Sato/ItemBanana.h"

#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceInfo.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/ResMgr.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/ObjUtility.h"

#include <math.h>

namespace
{
    const f32 sQtLerpRatio = 0.2f;
    const f32 sGravity = -3.0f;
    const f32 sSpGravity = -1.0f;
    const f32 sVelMag = 15.0f;
    const f32 sAllUseReleaseYVel = 10.0f;
    const f32 sAllUseVelMag = 40.0f;
    const f32 sSpVelMag = 50.0f;
    const f32 sSpVelDownRatio = 0.998f;
    const f32 sSpVelYRatio = 0.5f;
    const f32 sReflectVelDownRatio = 0.2f;
    const f32 sScale = 3.5f;
    const f32 sScaleEquip = 1.0f;
    const f32 sScaleUpRatio = 1.1f;
    const f32 sAirFric = 0.97f;
    const f32 sSpAirFric = 0.995f;
    const f32 sReleaseHeight = 60.0f;
    u8 sMaxNoColCnt = 10;
    const f32 sBoundsRadius = 25.0f;
    const f32 sBoundsHeight = 20.0f;
    const f32 sStickMagXZ = 60.0f;
    const f32 sStickMagY = 26.0f;
    const f32 sItemBoundRatio = 0.6f;
    const f32 sMaxSoundVelY = -35.0f;
    const f32 sScaleBigEquip = 2.0f;
    const f32 sScaleBig = 10.0f;
    const f32 sScaleBigUpRatio = 1.1f;
    const f32 sBoundsRadiusBig = 25.0f;
    const f32 sBoundsHeightBig = 20.0f;
    const f32 sBigBananaCreateVelMag = 30.0f;
    const f32 sBigShadowScaleY = 0.5f;
    const f32 sScaleFakeEquip = 0.35f;
    const f32 sScaleFake = 1.0f;
    const f32 sScaleFakeUpRatio = 1.1f;
    const f32 sFakeGndOffsetY = 100.0f;
    const f32 sFakeAddGndOffsetY = 35.0f;
    const f32 sFakeBoundRadius = 130.0f;
    const f32 sFakeItemBoxForcEfctScale = 4.5f;
    const f32 sFakeBoundRatio = 0.6f;
    const f32 sFakeMaxVelBoundY = 40.0f;
    const f32 sFakeOffsetVelY = 6.0f;
    const f32 sFakeOffsetVelDown = 0.9f;
}

J3DAnmTransform *ItemBanana::mAnmTrans[3] = {nullptr, nullptr, nullptr};
J3DMtxCalc *ItemBanana::mAnmCalc[3] = {nullptr, nullptr, nullptr};
J3DAnmTransform *ItemBananaBig::mAnmTrans[3] = {nullptr, nullptr, nullptr};
J3DMtxCalc *ItemBananaBig::mAnmCalc[3] = {nullptr, nullptr, nullptr};

namespace
{
    f32 sFakeLODBias[3] = {2.4f, 2.2f, 1.4f};
}

JGeometry::TVec3f ItemBanana::sHandOffsetPos(15.0f, 0.0f, -5.0f);

namespace
{
    void (ItemBanana::*sUnused1)() = &ItemBanana::setInitReleaseParam;
    const char *(*sUnused2)(int) = &ItemBananaBig::getBckName;
    const char *(ItemFakeItemBox::*sUnused3)() = &ItemFakeItemBox::getBtkName;
}

ItemBanana::ItemBanana(u32 kind) : ItemObj(kind), mController()
{
    reset();
}

void ItemBanana::reset()
{
    resetObject();
    _12c |= 0x8000;
    mKartReaction = 0;
    mEquipScale = 1.0f;
    if (mBounds && mBounds->getKind() == ObjColCylinder::CYLINDER) {
        ObjColCylinder *cyl = (ObjColCylinder *)mBounds;
        cyl->setCylinderRadius(sBoundsRadius);
        cyl->setCylinderHeight(sBoundsHeight);
    }
    mController.Reset();
    if (mController.mTrans) {
        mController.mTrans->setFlag();
    }
    mAllUseFlg = 0;
    _2c5 = 0;
    mIsOnGround = 0;
    mPlayedGndSound = 0;
    mDoSound = 1;
    mEnableGndSound = 1;
    _28 = 0.0f;
    _2c = sReflectVelDownRatio;
    mLerpRatio = sQtLerpRatio;
    mMaxVel = sVelMag;
    _12c |= 0x200;
}

const char *ItemBanana::getBmdFileName()
{
    static const char *cBananaBmdName = "/Item/item_banana.bmd";
    return cBananaBmdName;
}

void ItemBanana::loadAnmData(J3DModelData *model)
{
    for (int i = 0; i < 3; i++) {
        void *anm = ResMgr::getPtr(ResMgr::mcArcMRAM, getBckName(i));
        J3DAnmObjTrans::setupTransAnmData(&mAnmTrans[i], &mAnmCalc[i], model, anm);
    }
}

void ItemBanana::createModel(JKRSolidHeap *heap, u32 p2, u32 p3)
{
    mModel.createDifferedModel(heap, p2, p3 | 0x01000000, false);
    mController.mTrans = new AnmControlTrans();
    mController.mTrans->initAnm(3, &mModel);
    for (u8 i = 0; i < 3; i++) {
        mController.mTrans->registration(i, mAnmTrans[i], mAnmCalc[i]);
        mController.mTrans->getObjAnt(i)->mFrameCtrl.mAttr = 0;
    }
}

const char *ItemBanana::getBckName(int anm)
{
    switch (anm) {
    case 0:
        return "/Item/item_banana_a.bck";
    case 1:
        return "/Item/item_banana_b.bck";
    case 2:
        return "/Item/item_banana_die.bck";
    }
    return nullptr;
}

void ItemBanana::update()
{
    mController.SetAnimation();
    mModel.update(0);
}

void ItemBanana::doUniv()
{
    if (mState != 1 && mState != 5) {
        if (mController.mTrans) {
            mController.mTrans->resetFlag();
        }
    }
    if (!tstAnyObjectHitting() && getState() < 8) {
        switch (getState()) {
        case 1:
        case 5:
            doEquipScaleUp(sScaleEquip);
            break;
        case 7:
            doForceDisappearScaleDown();
            break;
        default:
            if (mEquipScale < sScale) {
                mEquipScale = mEquipScale * sScaleUpRatio;
            }
            setScaleCol(mEquipScale);
            break;
        }
    }
    (this->*mStateFunc)();
    checkAnmState();
    if (mDoSound) {
        doSound(0x40004);
    }
}

void ItemBanana::checkAnmState()
{
    if (mState != 8) {
        if (!(mTransferFlags & 0x100)) {
            if (mController.getNowTransNo() != 0) {
                mController.ChangeTransAnm(0);
            }
        }
        else {
            if (mController.getNowTransNo() != 1) {
                mController.ChangeTransAnm(1);
            }
        }
    }
    mController.FrameProc();
}

void ItemBanana::initEquip()
{
    mpHandAnmMtx = getHandAnmMtx();
    mVel.zero();
    mPos.zero();
    Quaternion qt = {0.0f, 0.0f, 0.0f, 0.0f};
    _34 = qt;
    mEquipFlags = 0;
}

void ItemBanana::doEquip()
{
    doHandOneEquip(false, 0.0f);
}

void ItemBanana::initRelease()
{
    setAllUseInitReleaseVelMag();
    mEquipFlags = 0xffffffff;
    mEquipFlags &= ~(1 << mOwnerNum);
    if (mItemKind == 15) {
        _12c = _12c & 0xFFFFFFFEu;
        _12c |= 2;
    }
    else {
        _12c |= 2;
        _12c |= 1;
    }
    setFrameNoColKart(sMaxNoColCnt, false);

    stRandom *rnd = getItemRnd();
    QuatRotY((rnd->get_ufloat_1() * 2.0f - 1.0f) * 3.1415926f, &_44);
    ObjUtility::getKartZdir(mOwnerNum, &mVel);
    mVel.scale(-mMaxVel);
    if (mAllUseFlg) {
        mVel.y += sAllUseReleaseYVel;
    }
    JGeometry::TVec3f comp;
    f32 compLen;
    getCompVecLength(comp, compLen);
    mVel.add(comp);
    mDoSound = 0;
}

void ItemBanana::doRelease()
{
    setAllUseBoundsScale();
    mVel.scale(sAirFric);
    mVel.y += sGravity;
    doMovePos();
    doMoveColGround(calcBaseRotQt(), false);
}

void ItemBanana::initSpRelease()
{
    mMaxVel = sSpVelMag;
    mEquipFlags = 0xffffffff;
    mEquipFlags &= ~(1 << mOwnerNum);
    if (mItemKind == 15) {
        _12c = _12c & 0xFFFFFFFEu;
        _12c |= 2;
    }
    else {
        _12c |= 2;
        _12c |= 1;
    }
    setFrameNoColKart(sMaxNoColCnt, false);

    ObjUtility::getKartBodyMtx(mOwnerNum);
    setInitThrowDir(&_240, sSpVelYRatio, false);
    ItemObj::calcBananaFirstQuat(_240);
    mVel = _240;
    initThrowVel(sStickMagXZ, sStickMagY, sSpGravity);
}

void ItemBanana::doSpRelease()
{
    setAllUseBoundsScale();
    f32 mag;
    getCompVecLength(*(JGeometry::TVec3f *)&mVel, mag);
    doMovePos();
}

void ItemBanana::initDisappear()
{
    mEquipFlags = 0;
    _12c = _12c & 0xFFFFFFFEu;
    mController.ChangeTransAnm(2);
    if (mHitObjNum == 0) {
        mVel.zero();
    }
}

void ItemBanana::doDisappear()
{
    u8 anm;
    AnmControlTrans *trans = mController.mTrans;
    anm = trans->getNowAnmNo();
    if (trans->getObj(anm)->mFrameCtrl.checkState(1)) {
        mState = 10;
    }
    doMovePos();
    setModelMatrixAndScale(_34);
}

void ItemBanana::doFall()
{
    doSpRelease();
    if (!(_12c & 1) && mItemKind != 15) {
        checkStartColItemFrame(30);
    }
}

void ItemBanana::moveNormal(CrsGround &ground)
{
    f32 height = ground.getHeight() + mDisScaleAnm.y * mHeightOffset;
    if (mPos.y < height) {
        mTransferFlags |= 2;
        mTransferFlags |= 0x100;
        if (!mIsOnGround && mEnableGndSound) {
            if (!mPlayedGndSound || mVel.y < sMaxSoundVelY) {
                mSoundMgr->setSe(0x40065);
            }
        }
        if (!mPlayedGndSound) {
            mPlayedGndSound = 1;
        }
        mPos.y = height;
        JGeometry::TVec3f normal;
        JGeometry::TVec3f up;
        ground.getNormal(&normal);
        getModelUpDir(&up);
        if (normal.x != up.x || normal.y != up.y || normal.z != up.z) {
            stVec2QtUpdate(_34, _44, up, normal);
        }
        JGeometry::TVec3f vel;
        ground.getVelocity(&vel);
        mVel.set(vel.x, 0.0f, vel.z);
    }
    mIsOnGround = (u32)mTransferFlags << 31;
}

f32 ItemBanana::getItemBoundRatio() const
{
    return sItemBoundRatio;
}

void ItemBanana::setAllUseBoundsScale()
{
    if (mAllUseFlg && mWaitFrame < 60) {
        mBounds->setScale(0.5f * mEquipScale);
    }
}

void ItemBanana::setAllUseInitReleaseVelMag()
{
    if (mAllUseFlg) {
        mMaxVel = sAllUseVelMag;
    }
}

const bool ItemBanana::checkKartColDisappear(int kartNo)
{
    ObjUtility::getKartVel(kartNo, &mVel);
    return true;
}

void ItemBanana::initOccur(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &vel)
{
    ItemObj::initOccur(pos, vel);
    mDoSound = 0;
    mEnableGndSound = 0;
}

void ItemBanana::doSound(u32 id)
{
    if (mState != 1 && mState != 8 && mState != 5 && mState != 2 && !(mTransferFlags & 0x100)) {
        mSoundMgr->setSe(id);
    }
}

ItemBananaBig::ItemBananaBig(u32 kind) : ItemBanana(kind)
{
    mShadowScaleY = sBigShadowScaleY;
}

void ItemBananaBig::reset()
{
    ItemBanana::reset();
    if (mBounds && mBounds->getKind() == ObjColCylinder::CYLINDER) {
        ObjColCylinder *cyl = (ObjColCylinder *)mBounds;
        cyl->setCylinderRadius(sBoundsRadiusBig);
        cyl->setCylinderHeight(sBoundsHeightBig);
    }
    mKartReaction = 2;
}

const char *ItemBananaBig::getBmdFileName()
{
    static const char *cBananaBigBmdName = "/Item/item_banana_sp.bmd";
    return cBananaBigBmdName;
}

void ItemBananaBig::loadAnmData(J3DModelData *model)
{
    for (int i = 0; i < 3; i++) {
        const char *name;
        switch (i) {
        case 0:
            name = "/Item/item_banana_sp_a.bck";
            break;
        case 1:
            name = "/Item/item_banana_sp_b.bck";
            break;
        case 2:
            name = "/Item/item_banana_sp_die.bck";
            break;
        default:
            name = nullptr;
            break;
        }
        void *anm = ResMgr::getPtr(ResMgr::mcArcMRAM, name);
        J3DAnmObjTrans::setupTransAnmData(&mAnmTrans[i], &mAnmCalc[i], model, anm);
    }
}

void ItemBananaBig::createModel(JKRSolidHeap *heap, u32 p2, u32 p3)
{
    mModel.createDifferedModel(heap, p2, p3 | 0x01000000, false);
    mController.mTrans = new AnmControlTrans();
    mController.mTrans->initAnm(3, &mModel);
    for (u8 i = 0; i < 3; i++) {
        mController.mTrans->registration(i, mAnmTrans[i], mAnmCalc[i]);
        mController.mTrans->getObjAnt(i)->mFrameCtrl.mAttr = 0;
    }
}

void ItemBananaBig::doUniv()
{
    if (mState != 1 && mState != 5 && mState != 7) {
        if (mController.mTrans) {
            mController.mTrans->resetFlag();
        }
    }
    if (!tstAnyObjectHitting() && getState() < 8) {
        switch (getState()) {
        case 1:
        case 5:
            doEquipScaleUp(sScaleBigEquip);
            break;
        case 7:
            doForceDisappearScaleDown();
            break;
        default:
            if (mEquipScale < sScaleBig) {
                mEquipScale = mEquipScale * sScaleBigUpRatio;
            }
            setScaleCol(mEquipScale);
            break;
        }
    }
    (this->*mStateFunc)();
    if (mState != 8) {
        if (!(mTransferFlags & 0x100)) {
            if (mController.getNowTransNo() != 0) {
                mController.ChangeTransAnm(0);
            }
        }
        else {
            if (mController.getNowTransNo() != 1) {
                mController.ChangeTransAnm(1);
            }
        }
    }
    mController.FrameProc();
    if (mDoSound && mState != 1 && mState != 8 && mState != 5 && mState != 2 && !(mTransferFlags & 0x100)) {
        mSoundMgr->setSe(0x40004);
    }
}

void ItemBananaBig::initEquip()
{
    mpHandAnmMtx = getHandAnmMtx();
    mVel.zero();
    mPos.zero();
    Quaternion qt = {0.0f, 0.0f, 0.0f, 0.0f};
    _34 = qt;
    mEquipFlags = 0;
    mEquipScale = sScaleBigEquip;
}

void ItemBananaBig::callHitItemSound(ItemObj *other)
{
    u32 kind = other->getKind();
    if (kind == 0 || kind == 2) {
        mSoundMgr->setSe(0x40073);
    }
}

ItemObj::ItemColFlag ItemBananaBig::calcColReaction(ItemObj *other)
{
    ItemColFlag ret = cColFlag_5;
    switch (other->getKind()) {
    case 1:
    case 7:
    case 8:
    case 9:
        ret = cColFlag_4;
        break;
    case 4:
        ret = cColFlag_0;
        break;
    }
    return ret;
}

void ItemBananaBig::initDisappear()
{
    mEquipFlags = 0;
    _12c &= ~1;
    mController.ChangeTransAnm(2);
    if (mHitObjNum == 0) {
        mVel.zero();
    }

    stRandom *rnd = getItemRnd();
    f32 angle = rnd->get_ufloat_1() * 2.0f * 3.14159265f;
    for (u8 i = 0; i < 3; i++) {
        JGeometry::TVec3f dir;
        dir.z = cos(angle);
        dir.x = sin(angle);
        dir.y = 2.0f;
        angle += 2.0923009f;
        dir.normalize();

        JGeometry::TVec3f pos;
        pos.add(mPos, dir);
        JGeometry::TVec3f vel;
        vel.scale(sBigBananaCreateVelMag, dir);
        vel.add(mVel);

        ItemObj *child = ItemObjMgr::getItemObjMgr()->occurItem(3, pos, vel);
        if (child) {
            child->setOwnerNum(mOwnerNum);
            child->setDriverNum(mDriverNum);
        }
    }
}

ItemFakeItemBox::ItemFakeItemBox(u32 kind) : ItemBanana(kind), mRotAnimation()
{
}

void ItemFakeItemBox::reset()
{
    ItemBanana::reset();
    mRotAnimation.resetRotAnimation(getItemRnd());
    _12c &= ~0x100;
    mKartReaction = 4;
    mEquipScale = 1.0f;
    for (int i = 0; i < 4; i++) {
        mEqEmt[i] = nullptr;
    }
    mKartNo = -1;
    mGndCount = 0;
    mAddGndOffsetY = 0.0f;
    mOffsetVelY = 0.0f;
    mGndOffsetFixed = 0;
    mTexPatternFrame = 0;

    JGeometry::TVec3f lf;
    JGeometry::TVec3f up;
    JGeometry::TVec3f fr;
    mBaseRotQt.x = 0.0f;
    mBaseRotQt.y = 0.0f;
    mBaseRotQt.z = 0.0f;
    mBaseRotQt.w = 0.0f;
    getModelLfDir(&lf);
    getModelUpDir(&up);
    getModelFrDir(&fr);
    mRotAnimation.setModelXYZDir(lf, up, fr);
    mHeightOffset = sFakeGndOffsetY;
    if (mBounds && mBounds->getKind() == ObjColBase::SPHERE) {
        mBounds->setRadius(sFakeBoundRadius);
    }
    mController.Reset();
}

const char *ItemFakeItemBox::getBmdFileName()
{
    return "/Item/niseItembox.bmd";
}

void ItemFakeItemBox::loadAnmData(J3DModelData *model)
{
    void *btk = ResMgr::getPtr(ResMgr::mcArcMRAM, "/Item/niseItembox.btk");
    J3DAnmObjMaterial::setupTexSRTAnmData(&mAnmTex, model, btk);
    void *btp = ObjUtility::getPtrMainArc("/Item/niseItembox.btp");
    J3DAnmObjMaterial::setupTexPatternAnmData(&sTexPattern, model, btp);

    int idx = 0;
    u8 mode = RaceMgr::sRaceManager->mRaceInfo->mConsoleNum;
    if (mode == 2) {
        idx = 1;
    }
    else if (mode < 5 && mode > 2) {
        idx = 2;
    }
    model->getTexture()->getResTIMG(2)->mLODBias = 100.0f * sFakeLODBias[idx];
}

void ItemFakeItemBox::createModel(JKRSolidHeap *heap, u32 p2, u32 p3)
{
    mModel.createDifferedModel(heap, p2, p3 | 0x02000200, false);
    MatAnmInfo info = {0, 1, 1, 0};
    mController.InitRegistrationMat(info, &mModel);

    mController.mMatData->mpCtrlSRT->getObjIce(0)->mAnmBase = mAnmTex;
    mController.mMatData->mpCtrlSRT->getObjIce(0)->initFrameCtrl(mAnmTex);
    mController.mMatData->mpCtrlSRT->getObjIce(0)->mFrameCtrl.mAttr = 0;
    mController.mMatData->mpCtrlSRT->mNowAnmNo = 0;

    mController.mMatData->mpCtrlPattern->getObj2(0)->mAnmBase = sTexPattern;
    mController.mMatData->mpCtrlPattern->getObj2(0)->initFrameCtrl(sTexPattern);
    mController.mMatData->mpCtrlPattern->getObj2(0)->mFrameCtrl.mAttr = 0;
    mController.mMatData->mpCtrlPattern->mNowAnmNo = 0;
}

void ItemFakeItemBox::doUniv()
{
    if (mState != 1 && mState != 5 && mState != 7) {
        if (mController.mTrans) {
            mController.mTrans->resetFlag();
        }
    }
    if (!mTexPatternFrame && mState != 5) {
        if ((mState < 5 && mState >= 2) || (mState >= 5 && mState < 7)) {
            mTexPatternFrame = 1;
        }
    }
    if (!tstAnyObjectHitting() && getState() < 8) {
        switch (getState()) {
        case 1:
        case 5:
            doEquipScaleUp(sScaleFakeEquip);
            break;
        case 7:
            doForceDisappearScaleDown();
            break;
        default:
            f32 ratio = sScaleFakeUpRatio;
            f32 maxScale = sScaleFake;
            f32 scale = mEquipScale;
            if (scale < maxScale) {
                mEquipScale = scale * ratio;
                if (mEquipScale > maxScale) {
                    mEquipScale = maxScale;
                }
            }
            setScaleCol(mEquipScale);
            break;
        }
    }
    MatAnmData *mat = mController.mMatData;
    mat->mpCtrlSRT->frameProc();
    f32 frame = mTexPatternFrame;
    mat->mpCtrlPattern->getObjAnt(0)->setFrame(frame);
    (this->*mStateFunc)();
    if (mDoSound && mState != 1 && mState != 8 && mState != 5 && mState != 2 && !(mTransferFlags & 0x100)) {
        mSoundMgr->setSe(0x40004);
    }
}

void ItemFakeItemBox::initEquip()
{
    mpHandAnmMtx = getHandAnmMtx();
    mVel.zero();
    mPos.zero();
    Quaternion qt = {0.0f, 0.0f, 0.0f, 0.0f};
    _34 = qt;
    mEquipFlags = 0;
    mEquipScale = sScaleFakeEquip;
}

void ItemFakeItemBox::callHitItemSound(ItemObj *other)
{
}

void ItemFakeItemBox::moveNormal(CrsGround &ground)
{
    f32 height = ground.getHeight() + mDisScaleAnm.y * mHeightOffset;
    if (mPos.y < height) {
        mTransferFlags |= 2;
        mTransferFlags |= 0x100;
        if (!mGndCount) {
            JGeometry::TVec3f normal;
            ground.getNormal(&normal);
            f32 dot = mVel.dot(normal);
            mVel.x = mVel.x * sFakeBoundRatio - normal.x * dot;
            mVel.y = mVel.y * sFakeBoundRatio - normal.y * dot;
            mVel.z = mVel.z * sFakeBoundRatio - normal.z * dot;
            if (mVel.y > sFakeMaxVelBoundY) {
                mVel.y = sFakeMaxVelBoundY;
            }
            mPos.y = height;
            mGndCount++;
            mOffsetVelY = sFakeOffsetVelY;
        }
        else {
            mPos.y = height;
            mVel.y = 0.0f;
            JGeometry::TVec3f vel;
            ground.getVelocity(&vel);
            mVel.x = vel.x;
            mVel.z = vel.z;
            if (mGndCount < 2) {
                mGndCount++;
            }
        }
    }
    if (!mGndOffsetFixed && mGndCount > 1) {
        mOffsetVelY = mOffsetVelY * sFakeOffsetVelDown;
        if (mOffsetVelY < 0.0f) {
            mOffsetVelY = 0.0f;
        }
        mAddGndOffsetY = mAddGndOffsetY + mOffsetVelY;
        if (mAddGndOffsetY > sFakeAddGndOffsetY) {
            mAddGndOffsetY = sFakeAddGndOffsetY;
            mOffsetVelY = 0.0f;
            mGndOffsetFixed = 1;
        }
        mHeightOffset = sFakeGndOffsetY + mAddGndOffsetY;
    }
}

ItemObj::ItemColFlag ItemFakeItemBox::calcColReaction(ItemObj *other)
{
    return (ItemColFlag)6;
}

void ItemFakeItemBox::initDisappear()
{
    _12c |= 0x80;
    mEquipFlags = 0;
}

void ItemFakeItemBox::doDisappear()
{
    if (mKartNo < 0) {
        createEfct(mPos);
        mState = 10;
        doEquip();
    }
    else {
        for (u8 i = 0; i < 4; i++) {
            JPABaseEmitter *emt = mEqEmt[i];
            if (!emt) {
                continue;
            }
            JGeometry::TVec3f pos;
            ObjUtility::getKartPos(mKartNo, &pos);
            emt->mGlobalTrs = pos;
            if (emt->checkStatus(8) && emt->mAlivePtclBase.getNum() + emt->mAlivePtclChld.getNum() == 0) {
                JPEffectMgr::getEffectMgr()->deleteEmt(emt);
                mEqEmt[i] = nullptr;
            }
        }
        u8 cnt = 0;
        for (u8 i = 0; i < 4; i++) {
            if (!mEqEmt[i]) {
                cnt++;
            }
        }
        if (cnt >= 4) {
            mState = 10;
            doEquip();
        }
    }
}

Quaternion *ItemFakeItemBox::calcBaseRotQt()
{
    JGeometry::TPos3f rot;
    mRotAnimation.calcRotAnimation(rot);
    stMtx2Qt(&mBaseRotQt, rot);
    return &mBaseRotQt;
}

const bool ItemFakeItemBox::checkKartColDisappear(int kartNo)
{
    mKartNo = kartNo;
    JGeometry::TVec3f pos;
    ObjUtility::getKartPos(kartNo, &pos);
    createEfct(pos);
    return true;
}

void ItemFakeItemBox::createEfct(const JGeometry::TVec3f &pos)
{
    for (u8 i = 0; i < 3; i++) {
        if (!mEqEmt[i]) {
            mEqEmt[i] = JPEffectMgr::getEffectMgr()->createEmt(ObjUtility::concatSimpleFileName("mk_niseItemget", i), pos);
        }
    }

    if (mKartNo >= 0) {
        static const char *sEfctNames[3] = {"mk_niseItemget_d_s", "mk_niseItemget_d_m", "mk_niseItemget_d_l"};
        int mode = RaceMgr::sRaceManager->mRaceInfo->mConsoleNum;
        int idx;
        if (mode == 2) {
            idx = 1;
        }
        else if (mode < 5 && mode > 2) {
            idx = 2;
        }
        else {
            idx = 0;
        }
        mEqEmt[3] = JPEffectMgr::getEffectMgr()->createEmt(sEfctNames[idx], pos);

        if (mKartNo < RaceMgr::sRaceManager->mRaceInfo->mKartNum) {
            for (u8 i = 0; i < 4; i++) {
                if (TstEmitterModeAndClipNotAvailableFlg(mKartNo, mEqEmt[i])) {
                    mEqEmt[i] = nullptr;
                }
            }
        }
    }
}

f32 ItemFakeItemBox::getForceEfctOffsetScale() const
{
    return sFakeItemBoxForcEfctScale;
}

void ItemFakeItemBox::initFall()
{
    ItemObj::initFall();
    _12c = _12c & 0xFFFFFFFEu;
    _12c &= 0xBFFFFFFF;
}
