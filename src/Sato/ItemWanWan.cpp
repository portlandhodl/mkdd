#include "macros.h"
#include "Sato/ItemWanWan.h"

#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/CenterLine.h"
#include "Kaneshige/Course/CrsArea.h"
#include "Kaneshige/SysDebug.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/ObjUtility.h"
#include "Sato/stMath.h"
#include "dolphin/mtx.h"

inline void setIdentQuat(Quaternion &q)
{
    Quaternion id = {0.0f, 0.0f, 0.0f, 1.0f};
    q = id;
}

namespace
{
    f32 sTargetR = 500.0f;
    s16 sMaxReleaseCnt = 435;
    f32 sQtLerpRatio = 0.18f;
    f32 sJumpGravity = -1.5f;
    f32 sGravity = -5.5f;
    f32 sGravityAdd = 0.1f;
    f32 sVelMag = 80.0f;
    f32 sHeightOffset = 320.0f;
    f32 sBoundVelMaxY = 10.0f;
    f32 sScale = 1.0f;
    f32 sVelDownRatio = 0.95f;
    f32 sVelDownRatioAir = 0.93f;
    u8 sMaxNoColCnt = 100;
    u8 sReleaseOffsetCnt = 20;
    f32 sReleaseVelRatio = 1.4f;
    f32 sReleaseAddYRatio = 1.0f;
    f32 sBoundsRadius = 380.0f;
    f32 sScaleEquip = 0.12f;
    f32 sScaleUpRatio = 1.18f;
    f32 sScaleUpRatioFall = 1.01f;
    f32 sChainLength = 65.0f;
    f32 sChainOffset = 80.0f;
    f32 sChainScale = 0.6f;
    f32 sChainAdjustRaio = 0.8f;
    f32 sChainGravity = -1.5f;
    f32 sChainAirFric = 0.97f;
    f32 sNormalAddVel = 10.0f;
    f32 sStraightAddVel = 3.0f;
    s16 sAddCnt = 800;
    f32 sRoadWidthRatio = 0.3f;
    f32 sMoveVelDownRatio = 0.9f;
    f32 sMaxRoadWidth = 1000.0f;
    s16 sMaxSeparateCnt = 300;
    u8 sMaxSoundNodeNo = 11;
    f32 sShadowOffsetScale = 10.0f;
    f32 sShadowScaleY = 0.5f;
    f32 sEfctScaleOffset = 10.0f;
    u8 sMaxColWallFrame = 2;
    u8 sMaxColWaitFrame = 10;
    u8 sMaxHitSeWaitCnt = 30;
    f32 sBackReleaseAngle = 120.0f;
    f32 sBoundRatio;
}

JGeometry::TVec3f ItemWanWanObj::sHandOffsetPos(155.0f, 150.0f, -330.0f);
J3DAnmTransform *ItemWanWanObj::mAnmTrans[2] = {nullptr, nullptr};
J3DMtxCalc *ItemWanWanObj::mAnmCalc[2] = {nullptr, nullptr};
J3DDeformData *ItemWanWanObj::mAnmDeform;
J3DAnmCluster *ItemWanWanObj::mAnmCluster;



ItemWanWanObj::ItemWanWanObj(u32 kind) : ItemObj(kind)
{
    mStringObj = new StringObj(15, true);
    mCLPointController = new CLPointController();
    reset();
    mStringObj->loadmodel(ItemObjMgr::gItemMgr->mOtherModel[0]);
    mShadowOffsetScale = sShadowOffsetScale;
    mShadowScaleY = sShadowScaleY;
}


void ItemWanWanObj::reset()
{
    resetObject();
    mKartReaction = 9;
    mEquipFlags = 0;
    _28 = sBoundRatio;
    mHeightOffset = sHeightOffset;
    _2c = 1.0f;
    mLerpRatio = sQtLerpRatio;
    _2b4.zero();
    _2c0 = 0;
    _2c4.zero();
    _2d0 = 0.0f;
    _2d4 = 1;
    mMoveState = 4;
    _2dc = 0;
    _2e0 = 0;
    _2e8 = 0.0f;
    _2ec = 2;
    _328 = 0;
    _32c.zero();
    _338.zero();

    mAnmObjCluster.getFrameCtrl()->reset();
    mController.Reset();

    mGravity = sGravity;
    _348 = 0;
    _349 = 0;
    _34a = 0;
    _34b = 0;
    for (u8 i = 0; i < 2; i++)
    {
        _354[i] = nullptr;
        _34c[i] = nullptr;
    }

    mCLPointController->reset();
    if (mBounds)
        mBounds->setRadius(sBoundsRadius);
    _1ec = 3;
    mStringObj->reset();
    mStringObj->mStringNodeMgr->resetNodeAll(&mPos);
    ((ExStringNodeManager *)mStringObj->mStringNodeMgr)->setNodeLengthAll(sChainLength);

    Mtx mtx;
    mStringObj->mScale = 0.0f;
    mStringObj->mStringNodeMgr->_40 = sMaxSoundNodeNo;
    PSMTXIdentity(mtx);
    mStringObj->_14 = mtx;
    mStringObj->mStringNodeMgr->_24 = sChainAdjustRaio;
    mStringObj->mStringNodeMgr->_1c = sChainGravity;
    mStringObj->mStringNodeMgr->_20 = sChainAirFric;
    _12c |= 0x2000;
}

const char *ItemWanWanObj::getBmdFileName()
{
    static const char *cWanWanBmdName = "/Item/wanwan.bmd";
    return cWanWanBmdName;
}


void ItemWanWanObj::loadAnmData(J3DModelData *modelData)
{
    void *bls = ObjUtility::getPtrMainArc("/Item/wanwan.bls");
    J3DAnmObjCluster::loadClusterData(&mAnmDeform, bls);
    void *blk = ObjUtility::getPtrMainArc("/Item/wanwan.blk");
    J3DAnmObjCluster::loadClusterAnmData(&mAnmCluster, blk);

    for (int i = 0; i < 2; i++)
    {
        void *bck = ResMgr::getPtr(ResMgr::mcArcMRAM, getBckName(i));
        J3DAnmObjTrans::setupTransAnmData(&mAnmTrans[i], &mAnmCalc[i], modelData, bck);
    }
    sChainJointNo = modelData->getJointName()->getIndex("pos_chn");
}


const char *ItemWanWanObj::getBckName(int bckID)
{
    const char *ret = nullptr;
    switch (bckID)
    {
    case 0:
        ret = "/Item/wanwan.bck";
        break;
    case 1:
        ret = "/Item/wanwan_die.bck";
        break;
    }
    return ret;
}



void ItemWanWanObj::createModel(JKRSolidHeap *heap, u32 p2, u32 p3)
{
    p3 |= 0x1000000;
    p3 |= 0x200;
    mModel.createDifferedModel(heap, p2, p3, true);
    J3DAnmObjCluster::setDeformData(&mModel, mAnmDeform, false);
    mAnmObjCluster.setExModel(&mModel, mAnmDeform);
    mAnmObjCluster.attach(mAnmCluster);

    mController.mTrans = new AnmControlTrans();
    mController.mTrans->initAnm(2, &mModel);

    for (u8 i = 0; i < 2; i++)
        mController.mTrans->registration(i, mAnmTrans[i], mAnmCalc[i]);

    mStringObj->createModel(heap, p2);
}


void ItemWanWanObj::update()
{
    mController.SetAnimation();
    u8 anmNo = mController.getNowTransNo();
    f32 frame = mController.mTrans->getObjAnt(anmNo)->getFrame();
    mAnmObjCluster.getDeformData()->setAnm((J3DAnmCluster *)mAnmObjCluster.mAnmBase);
    mAnmObjCluster.mFrameCtrl.setFrame(frame);
    mAnmObjCluster.anmFrameProc();
    mStringObj->update();
    mModel.update(0);
}



void ItemWanWanObj::drawSimpleModel(u32 viewNo, Mtx mtx, J3DUClipper *clipper, Mtx mtx2)
{
    mStringObj->drawSimpleModel(viewNo, mtx, clipper, mtx2);
}


void ItemWanWanObj::viewCalc(u32 viewNo) { mModel.viewCalc(viewNo); }


void ItemWanWanObj::setCurrentViewNo(u32 viewNo)
{
    mModel.setCurrentViewNo(viewNo);
    mStringObj->setCurrentViewNo(viewNo);

    Mtx mtx;
    ObjUtility::getCamDependLightMtx(viewNo, mPos, mtx);
    mModel.setEffectMtx(mtx, 0);
}



void ItemWanWanObj::doUniv()
{
    if (!tstAnyObjectHitting())
    {
        switch (mState)
        {
        case StateEquip:
        case StateDivested:
            doEquipScaleUp(sScaleEquip);
            break;
        case 7:
            doForceDisappearScaleDown();
            break;
        default:
            f32 ratio = sScaleUpRatio;
            if (mState == 4)
                ratio = sScaleUpRatioFall;
            if (mEquipScale < sScale)
            {
                mEquipScale *= ratio;
                if (mEquipScale > sScale)
                    mEquipScale = sScale;
            }
            setScaleCol(mEquipScale);
            break;
        }
    }

    calcGravity();
    (this->*mStateFunc)();

    ((ExStringNodeManager *)mStringObj->mStringNodeMgr)->setNodeLengthAll(sChainLength * mEquipScale);
    StringNodeManager *mgr = mStringObj->mStringNodeMgr;

    if (_2d4)
    {
        _2d4 = 0;
        mStringObj->calc();
    }
    else
    {
        JGeometry::TVec3f locPos;
        Mtx locMtx;
        getChainLocatorPos(&locPos, locMtx);
        mStringObj->_14 = locMtx;

        JGeometry::TVec3f up;
        JGeometry::TVec3f fr;
        getModelUpDir(&up);
        getModelFrDir(&fr);

        if (mState == 1)
        {
            JGeometry::TVec3f kartVel;
            ObjUtility::getKartVel(mOwnerNum, &kartVel);
            locPos.add(kartVel);
        }

        mgr->setNodePos(0, locPos);
        mgr->setNodeVel(0, mVel);

        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mOwnerNum, &kartPos);
        JGeometry::TVec3f diff;
        diff.sub(mPos, kartPos);
        _2b4.zero();

        u32 last = 0;
        if (!_2e0)
        {
            last = mgr->mStrNodeList.getNumLinks() - 1;
            JGeometry::TVec3f kartVel;
            ObjUtility::getKartVel(mOwnerNum, &kartVel);
            mgr->setNodePos(last, kartPos);
            mgr->setNodeVel(last, kartVel);
        }

        mStringObj->mScale = sChainScale * mEquipScale;
        mStringObj->calc();

        if (!_2e0)
        {
            u32 num = mStringObj->mStringNodeMgr->mStrNodeList.getNumLinks();
            StringNode *node = mStringObj->mStringNodeMgr->getNode(0);
            f32 nodeLen = node->_34;
            if (diff.length() > nodeLen * (num - 1))
                mgr->getNodeVel(last, &_2b4);
        }
    }

    mController.FrameProc();

    if (mState != 1 && mAnmObjCluster.getFrame() == 0.0f)
        mSoundMgr->setSe(0x40021);

    if (_34a)
        _34a--;

    doTraceEfct();
    SysDebug::checkInvalidMatrix((MtxPtr)mModel.getBaseTRMtx(), (char *)"WanWanRTMtx");
}
void ItemWanWanObj::setHitSe()
{
    if (_34a == 0)
    {
        mSoundMgr->setSe(0x4002d);
        _34a = sMaxHitSeWaitCnt;
    }
}


ItemObj::ItemColFlag ItemWanWanObj::calcColReaction(ItemObj *) { return cColFlag_5; }


void ItemWanWanObj::initEquip()
{
    mEquipFlags = 0;
    mpHandAnmMtx = getHandAnmMtx();
    mVel.zero();
    _32c.zero();
    _338.zero();
    mPos.zero();

    setIdentQuat(_44);
    mEquipScale = sScaleEquip;
}


void ItemWanWanObj::doEquip() { doHandOneEquip(false, 0.0f); }


void ItemWanWanObj::initRelease()
{
    mMaxVel = sVelMag;
    setInitReleaseParam();

    MtxPtr mtx = ObjUtility::getKartBodyMtx(mOwnerNum);
    JGeometry::TVec3f zdir;
    JGeometry::TVec3f ydir;
    ObjUtility::getKartZdir(mOwnerNum, &zdir);
    ObjUtility::getKartYdir(mOwnerNum, &ydir);

    stMtx2Qt(&_44, mtx);
    f32 t = 1.0f - ydir.y;
    if (ydir.y < 0.0f)
        t = 0.0f;

    ydir.scale(sReleaseAddYRatio * t);
    zdir.add(ydir);
    stVecNormalize(zdir);

    mVel = zdir;
    mVel.scale(sReleaseVelRatio * mMaxVel);

    JGeometry::TVec3f kartVel;
    ObjUtility::getKartVel(mOwnerNum, &kartVel);
    mVel.add(kartVel);
    mMoveState = 0;
}


void ItemWanWanObj::setInitReleaseParam()
{
    mEquipFlags = -1;
    mEquipFlags &= ~(1 << mOwnerNum);
    _12c |= 2;
    _12c |= 1;
    _2c0 = 0;
    initMoveColGround();
    mpHandAnmMtx = getHandAnmMtx();
    setFrameNoColKart(sMaxNoColCnt, false);

    f32 len = 0.0f;
    getCompVecLength(_2c4, len);
    _2c4.scale(len / (f32)sReleaseOffsetCnt);
}


void ItemWanWanObj::doUnivRelease(bool reverse)
{
    if (mWaitFrame < sReleaseOffsetCnt)
        mPos.add(_2c4);

    mVel.y += mGravity;

    if (!reverse)
    {
        if (mMoveState == 1 || mMoveState == 2)
            moveCenterLine();
        else
            moveStraight();
    }

    if (mVel.length() > mMaxVel)
    {
        f32 ratio = sVelDownRatio;
        if (!(mTransferFlags & 2))
            ratio = sVelDownRatioAir;
        mVel.scale(ratio);
        stClampVecMin(mVel, mMaxVel);
    }
    stClampVecMin(mVel, mMaxVel);

    doMovePos();
    doMoveColGround(nullptr, false);

    if (mTransferFlags & 4)
    {
        _348++;
        _349 = sMaxColWaitFrame;
    }
    else if (_349 != 0)
    {
        _349--;
        if (_349 == 0)
            _348 = 0;
    }

    if (_348 > sMaxColWallFrame)
        setStateDisappear(true);

    _32c = mVel;

    if (((mTransferFlags & 8) || (mTransferFlags & 1)) && _34a == 0)
    {
        mSoundMgr->setSe(0x4002d);
        _34a = sMaxHitSeWaitCnt;
    }
}


void ItemWanWanObj::doRelease()
{
    doUnivRelease(false);

    if (mMoveState != 3)
    {
        if (mMoveState == 2 && mWaitFrame > (u32)(sMaxReleaseCnt + sMaxSeparateCnt))
            mMoveState = 3;
        else if (mWaitFrame > sMaxReleaseCnt)
            separate();
    }

    if (mMoveState == 3 || mMoveState == 2)
    {
        if ((mTransferFlags & 4) || (mTransferFlags & 8))
            setStateDisappear(true);
    }
}



void ItemWanWanObj::calcGravity()
{
    CrsArea area;
    area.search(5, mPos);

    if (area.isInside())
    {
        mGravity = sJumpGravity;
    }
    else
    {
        mGravity -= sGravityAdd;
        if (mGravity < sGravity)
            mGravity = sGravity;
    }
}



void ItemWanWanObj::initDisappear()
{
    mEquipFlags = 0;
    _12c &= ~1;
    mSoundMgr->setSe(0x4007b);
    separate();

    mController.ChangeTransAnm(1);
}


void ItemWanWanObj::doDisappear()
{
    if (mController.mTrans->getObjChk(mController.getNowTransNo())->getFrameCtrl()->checkState(1))
        mState = 10;

    J3DFrameCtrl *ctrl = mController.mTrans->getObjAnt(mController.getNowTransNo())->getFrameCtrl();
    mEquipScale = 1.0f - ctrl->getFrame() / (f32)ctrl->getEnd();
}


void ItemWanWanObj::initFall()
{
    ItemObj::initFall();
    _2e0 = 1;
}


void ItemWanWanObj::doFall()
{
    moveFall();
    if (mTransferFlags & 2)
        setStateDisappear(true);
}


void ItemWanWanObj::moveNormal(CrsGround &ground)
{
    if (ground.getAttribute() == CrsGround::Attr_18)
    {
        moveColWall(ground);
        return;
    }

    f32 groundH = ground.getHeight() + mDisScaleAnm.y * mHeightOffset;
    _2d0 = mPos.y - groundH;
    if (mPos.y < groundH)
    {
        if (mMoveState == 0)
        {
            initmoveCenterLine();
            mMoveState = 1;
        }

        mTransferFlags |= 2;
        mTransferFlags |= 0x100;
        mPos.y = groundH;

        f32 velMag = mVel.length();
        JGeometry::TVec3f normal;
        ground.getNormal(&normal);

        mVel.y = -mVel.y * _28;
        if (mVel.y > sBoundVelMaxY)
            mVel.y = sBoundVelMaxY;

        if (_2c0 && velMag > 0.0f)
        {
            JGeometry::TVec3f side;
            JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
            JGeometry::TVec3f nvel;
            side.cross(up, mVel);
            if (side.length() > 0.0f)
            {
                side.normalize();
                nvel.cross(side, normal);
                if (nvel.length() > 0.0f)
                {
                    nvel.normalize();
                    nvel.scale(velMag);
                    mVel = nvel;
                }
            }
        }

        _2c0 = 1;

        JGeometry::TVec3f side;
        JGeometry::TVec3f velDir;
        JGeometry::setTVec3f(&mVel.x, &velDir.x);
        stVecNormalize(velDir);
        velDir.y = 0.0f;

        side.cross(normal, velDir);
        stVecNormalize(side);
        velDir.cross(side, normal);
        stVecNormalize(velDir);

        Mtx mtx;
        stMakeRMtx(mtx, side, normal, velDir);
        stMtx2Qt(&_44, mtx);
    }
    else
    {
        calcBalanceFlying();
        _2c0 = 0;
    }
}

void ItemWanWanObj::calcBalanceFlying()
{
    JGeometry::TVec3f dir(mVel);
    if (dir.length() > 0.0f)
    {
        dir.normalize();
        JGeometry::TPos3f mtx;
        stMakeDirectionMtx(&mtx, dir, 'Z');
        stMtx2Qt(&_44, mtx);
    }
}


void ItemWanWanObj::initmoveCenterLine()
{
    setCLPoint();
    if (mCLPointController->mPrev)
        mMoveState = 1;

    JGeometry::TVec3f clDir(mCLPointController->_c);
    JGeometry::TVec3f vel(mVel);
    vel.y = 0.0f;
    stVecNormalize(vel);

    JGeometry::TVec3f cross;
    PSVECCrossProduct(&clDir, &vel, &cross);
    f32 mag = cross.length();
    f32 angle = JGeometry::TUtilf::atan2(mag, clDir.dot(vel));
    angle = JGeometry::TUtilf::abs(angle) * 57.29578f;
    if (angle > sBackReleaseAngle)
        _34b = 1;
}


void ItemWanWanObj::setCLPoint()
{
    if (mCLPointController->mNext == nullptr)
        mCLPointController->init_ByKartIndex(mOwnerNum);
}


void ItemWanWanObj::moveCenterLine()
{
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f prevPos;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f side;
    JGeometry::TVec3f target;
    JGeometry::TVec3f delta;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f toTarget;
    JGeometry::TVec3f clDir;

    mCLPointController->mNext->getPosition(&pos);
    tmp.sub(pos, mPos);
    delta = tmp;
    delta.y = 0.0f;
    if (delta.length() < sTargetR || mCLPointController->chkIsOverFwPoint(mPos))
        mCLPointController->changeNextPoint();

    mCLPointController->mPrev->getPosition(&prevPos);
    mCLPointController->mNext->getPosition(&pos);

    dir.sub(pos, prevPos);
    stVecNormalize(dir);

    side.cross(dir, JGeometry::TVec3f(0.0f, 1.0f, 0.0f));

    f32 w = 0.5f * mCLPointController->mNext->getPointData()->scale;
    f32 width = w > sMaxRoadWidth ? sMaxRoadWidth : w;

    f32 lateral = sRoadWidthRatio * (width * JMASinShort(_328));
    side.scale(lateral);
    target.add(side, pos);

    f32 vely = mVel.y;
    mVel.y = 0.0f;

    toTarget.sub(target, mPos);
    toTarget.y = 0.0f;
    stVecNormalize(toTarget);

    toTarget.scale(sNormalAddVel);
    mVel.scale(sMoveVelDownRatio);
    mVel.add(toTarget);
    vel = mVel;
    mVel.y = vely;

    clDir = mCLPointController->_c;
    stVecNormalize(vel);

    _328 += (s16)(sAddCnt * (width / sMaxRoadWidth));

    _338.sub(mVel, _32c);
}

void ItemWanWanObj::moveStraight()
{
    JGeometry::TVec3f dir(mVel);
    dir.y = 0.0f;
    stVecNormalize(dir);
    dir.scale(sStraightAddVel);
    mVel.add(dir);
}


f32 ItemWanWanObj::getChainLength()
{
    u32 num = mStringObj->mStringNodeMgr->mStrNodeList.getNumLinks();
    StringNode *node = mStringObj->mStringNodeMgr->getNode(0);
    return node->_34 * (num - 1);
}

void ItemWanWanObj::getFirstNodePos(JGeometry::TVec3f *pos)
{
    mStringObj->mStringNodeMgr->getNodePos(0, pos);
}
void ItemWanWanObj::getCenterNodePos(JGeometry::TVec3f *pos)
{
    mStringObj->mStringNodeMgr->getNodePos((int)mStringObj->mStringNodeMgr->mStrNodeList.getNumLinks() / 2, pos);
}

void ItemWanWanObj::getLastNodePos(JGeometry::TVec3f *pos)
{
    mStringObj->mStringNodeMgr->getNodePos(mStringObj->mStringNodeMgr->mStrNodeList.getNumLinks() - 1, pos);
}






const bool ItemWanWanObj::checkKartColDisappear(int)
{
    if (_34a == 0)
    {
        mSoundMgr->setSe(0x4002d);
        _34a = sMaxHitSeWaitCnt;
    }
    return false;
}


void ItemWanWanObj::separate()
{
    if (mMoveState == 4)
    {
        setStateForceDisappear();
        return;
    }
    if (_2e0)
        return;

    if (mCLPointController->mNext == nullptr)
        mCLPointController->init_ByKartIndex(mOwnerNum);

    _2e0 = 1;
    mMoveState = 2;

#line 1257
    JUT_ASSERT(getState() != ItemObj::StateEquip);
    JUT_ASSERT(getState() != ItemObj::StateDivested);

    if (mMoveState != 3)
        ItemObjMgr::gItemMgr->releaseItemByKartEquipItemList(this);

    u8 i;
    char *name = "mk_wanChain";
    for (i = 0; i < 2; i++)
    {
        const char *fn = ObjUtility::concatSimpleFileName(name, i);
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(fn, mPos);
        if (!emt)
            continue;
        if (TstEmitterModeNotAvailableFlg(mOwnerNum, emt))
            continue;
        JPASetRMtxTVecfromMtx(ObjUtility::getKartBodyMtx(mOwnerNum), emt->mGlobalRot, &emt->mGlobalTrs);
        _354[i] = emt;
    }
}


void ItemWanWanObj::getChainLocatorPos(JGeometry::TVec3f *pos, Mtx mtx)
{
    PSMTXCopy(mModel.getModel()->mMtxBuffer->getAnmMtx(sChainJointNo), mtx);
    JGeometry::TVec3f zdir(mtx[0][2], mtx[1][2], mtx[2][2]);
    zdir.scale(sChainOffset);
    mtx[0][3] += zdir.x;
    mtx[1][3] += zdir.y;
    mtx[2][3] += zdir.z;
    pos->set(mtx[0][3], mtx[1][3], mtx[2][3]);
}


f32 ItemWanWanObj::getForceEfctOffsetScale() const { return sEfctScaleOffset; }


void ItemWanWanObj::executeGeoObjHitCallBack(GeographyObj *obj)
{
    for (u8 i = 0; i < 2; i++)
    {
        if (_34c[i])
            return;
    }

    bool hit = false;
    switch (obj->mKind)
    {
    case 3402:
    case 3704:
    case 3711:
    case 3712:
    case 3713:
    case 4001:
    case 4002:
    case 4003:
    case 4006:
    case 4007:
    case 4009:
    case 4010:
    case 4701:
    case 5001:
    case 5102:
        hit = true;
        break;
    }

    if (!hit)
        return;

    char *name = "mk_wanHit";
    for (u8 i = 0; i < 2; i++)
    {
        const char *fn = ObjUtility::concatSimpleFileName(name, i);
        _34c[i] = GetJPAMgr()->createEmt(fn, mPos);
    }
}


void ItemWanWanObj::doTraceEfct()
{
    for (u8 i = 0; i < 2; i++)
    {
        if (_34c[i])
        {
            JPASetRMtxTVecfromMtx(mModel.getBaseTRMtx(), _34c[i]->mGlobalRot, &_34c[i]->mGlobalTrs);
            if (_34c[i]->isEnableDeleteEmitter())
                _34c[i] = nullptr;
        }
        if (_354[i])
        {
            JGeometry::TVec3f pos;
            ObjUtility::getKartPos(mOwnerNum, &pos);
            JPABaseEmitter *emt = _354[i];
            emt->mGlobalTrs.x = pos.x;
            emt->mGlobalTrs.y = pos.y;
            emt->mGlobalTrs.z = pos.z;
            if (_354[i]->isEnableDeleteEmitter())
                _354[i] = nullptr;
        }
    }
}

DECL_WEAK ItemWanWanObj::~ItemWanWanObj() {}

#include "JSystem/JAudio/JASFakeMatch2.h"
