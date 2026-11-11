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

Vec ItemWanWanObj::sHandOffsetPos;
J3DAnmTransform *ItemWanWanObj::mAnmTrans[2] = {nullptr, nullptr};
J3DMtxCalc *ItemWanWanObj::mAnmCalc[2] = {nullptr, nullptr};
J3DDeformData *ItemWanWanObj::mAnmDeform;
J3DAnmCluster *ItemWanWanObj::mAnmCluster;

namespace
{
struct WanWanInit
{
    WanWanInit()
    {
        ItemWanWanObj::sHandOffsetPos.x = 155.0f;
        ItemWanWanObj::sHandOffsetPos.y = 150.0f;
        ItemWanWanObj::sHandOffsetPos.z = -330.0f;
    }
};
WanWanInit sWanWanInit;
}


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
    J3DAnmObjCluster::loadClusterData(&mAnmDeform, ObjUtility::getPtrMainArc("/Item/wanwan.bls"));
    J3DAnmObjCluster::loadClusterAnmData(&mAnmCluster, ObjUtility::getPtrMainArc("/Item/wanwan.blk"));

    for (int i = 0; i < 2; i++)
    {
        const char *name = getBckName(i);
        void *data = ResMgr::getPtr(ResMgr::mcArcMRAM, name);
        J3DAnmObjTrans::setupTransAnmData(&mAnmTrans[i], &mAnmCalc[i], modelData, data);
    }
    sChainJointNo = modelData->getJointName()->getIndex("mk_wanChain");
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



DECL_WEAK J3DAnmObjCluster::~J3DAnmObjCluster() {}
DECL_WEAK void J3DAnmObjCluster::anmFrameProc()
{
    mAnmBase->setFrame(mFrameCtrl.getFrame());
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



#pragma inline_depth(1)
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
            locPos.x += kartVel.x;
            locPos.y += kartVel.y;
            locPos.z += kartVel.z;
        }

        mgr->setNodePos(0, JGeometry::TVec3f(locPos.x, locPos.y, locPos.z));
        mgr->setNodeVel(0, JGeometry::TVec3f(mPos.x, mPos.y, mPos.z));

        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mOwnerNum, &kartPos);
        _2b4.x = mPos.x - kartPos.x;
        _2b4.y = mPos.y - kartPos.y;
        _2b4.z = mPos.z - kartPos.z;

        u32 last = 0;
        if (!_2e0)
        {
            u32 num = mgr->mStrNodeList.getNumLinks();
            last = num - 1;
            JGeometry::TVec3f kartVel;
            ObjUtility::getKartVel(mOwnerNum, &kartVel);
            mgr->setNodePos(last, JGeometry::TVec3f(kartPos.x, kartPos.y, kartPos.z));
            mgr->setNodeVel(last, JGeometry::TVec3f(kartVel.x, kartVel.y, kartVel.z));
        }

        mStringObj->mScale = sChainScale * mEquipScale;
        mStringObj->calc();

        if (!_2e0)
        {
            u32 num = mgr->mStrNodeList.getNumLinks();
            JUT_ASSERT(num > 0);
            StringNode *node = (StringNode *)mgr->mStrNodeList.getNthLink(0)->getObjectPtr();
            f32 nodeLen = node->_34;
            if (locPos.length() > nodeLen * (num - 1))
                mgr->getNodeVel(last, &_2b4);
        }
    }

    mController.FrameProc();

    if (mState != 1 && mEquipScale == 0.0f)
        mSoundMgr->setSe(0x40021);

    if (_34a)
        _34a--;

    doTraceEfct();
    SysDebug::checkInvalidMatrix((MtxPtr)mModel.getBaseTRMtx(), (char *)"WanWanRTMtx");
}
#pragma inline_depth(4)
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

    JGeometry::TVec4f off = *(JGeometry::TVec4f *)&sHandOffsetPos;
    _44.x = off.x;
    _44.y = off.y;
    _44.z = off.z;
    _44.w = off.w;
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
    f32 t = 1.0f;
    if (ydir.y >= 0.0f)
        t = 1.0f - ydir.y;
    else
        t = 0.0f;

    f32 addY = sReleaseAddYRatio * t;

    JGeometry::TVec3f dir;
    dir.x = zdir.x + ydir.x * addY;
    dir.y = zdir.y + ydir.y * addY;
    dir.z = zdir.z + ydir.z * addY;
    stVecNormalize(dir);

    f32 scale = sReleaseVelRatio * mMaxVel;
    mVel.x = dir.x * scale;
    mVel.y = dir.y * scale;
    mVel.z = dir.z * scale;

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
    f32 ratio = len / (f32)sReleaseOffsetCnt;
    _2d0 = ratio - _2c4.x;
    _2c4.x = ratio / _2c4.x;
    _2c4.y = ratio / _2c4.y;
    _2c4.z = ratio / _2c4.z;
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
        if (mMoveState == 2)
        {
            if (mWaitFrame > (u32)(sMaxReleaseCnt + sMaxSeparateCnt))
            {
                mMoveState = 3;
                goto statecheck;
            }
        }
    }

    if (mWaitFrame > sMaxReleaseCnt)
        separate();

statecheck:
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
    if (mController.mTrans->getFrameCtrl(mController.getNowTransNo())->checkState(1))
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

    f32 angle = JGeometry::TUtilf::atan2(cross.x, cross.z);
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
    JGeometry::TVec3f point;
    mCLPointController->mNext->getPosition(&point);
    JGeometry::TVec3f delta(point);
    delta.sub(mPos);
    delta.y = 0.0f;
    if (delta.length() < sTargetR || mCLPointController->chkIsOverFwPoint(mPos))
        mCLPointController->changeNextPoint();

    JGeometry::TVec3f prevPos;
    JGeometry::TVec3f nextPos;
    mCLPointController->mPrev->getPosition(&prevPos);
    mCLPointController->mNext->getPosition(&nextPos);

    JGeometry::TVec3f dir;
    dir.sub(nextPos, prevPos);
    stVecNormalize(dir);

    JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f side;
    side.cross(up, dir);

    f32 width = mCLPointController->mNext->getPointData()->scale * 0.5f;
    if (width > sMaxRoadWidth)
        width = sMaxRoadWidth;

    f32 lateral = width * JMASinShort(_328) * sRoadWidthRatio;
    side.scale(lateral);
    JGeometry::TVec3f target;
    target.add(side, nextPos);

    f32 vely = mVel.y;
    mVel.y = 0.0f;

    JGeometry::TVec3f toTarget;
    toTarget.sub(target, mPos);
    toTarget.y = 0.0f;
    stVecNormalize(toTarget);

    toTarget.scale(sNormalAddVel);
    mVel.scale(sMoveVelDownRatio);
    mVel.add(toTarget);
    JGeometry::TVec3f vel(mVel);
    mVel.y = vely;

    JGeometry::TVec3f clDir(mCLPointController->_c);
    stVecNormalize(clDir);

    _328 = _328 + (s16)(width / sMaxRoadWidth * sAddCnt);

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
    StringNodeManager *mgr = mStringObj->mStringNodeMgr;
    u32 num = mgr->mStrNodeList.getNumLinks();
    JUT_ASSERT_F(0 < num, "range over: %d <= num=%d < %d", 0, num, mgr->mStrNodeList.getNumLinks());
    StringNode *node = (StringNode *)mgr->mStrNodeList.getNthLink(0)->getObjectPtr();
    return node->_34 * (num - 1);
}


void ItemWanWanObj::getFirstNodePos(JGeometry::TVec3f *pos)
{
    mStringObj->mStringNodeMgr->getNodePos(0, pos);
}
void ItemWanWanObj::getCenterNodePos(JGeometry::TVec3f *pos)
{
    mStringObj->mStringNodeMgr->getNodePos(mStringObj->mStringNodeMgr->mStrNodeList.getNumLinks() / 2, pos);
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

    JUT_ASSERT(mState != StateEquip);
    JUT_ASSERT(mState != StateDivested);

    if (mMoveState != 3)
        ItemObjMgr::gItemMgr->releaseItemByKartEquipItemList(this);

    char *name = "mk_wanChain";
    for (u8 i = 0; i < 2; i++)
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
    mtx[0][3] += sChainOffset * mtx[0][2];
    mtx[1][3] += sChainOffset * mtx[1][2];
    mtx[2][3] += sChainOffset * mtx[2][2];
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
    case 3714:
    case 3715:
    case 3716:
    case 3717:
    case 3718:
    case 3719:
    case 3720:
    case 3721:
    case 3722:
    case 3723:
    case 3724:
    case 3725:
    case 3726:
    case 3727:
    case 3728:
    case 3729:
    case 3730:
    case 3731:
    case 3732:
    case 3733:
    case 3734:
    case 3735:
    case 3736:
    case 3737:
    case 3738:
    case 3739:
    case 3740:
    case 3741:
    case 3742:
    case 3743:
    case 3744:
    case 3745:
    case 3746:
    case 3747:
    case 3748:
    case 3749:
    case 3750:
    case 3751:
    case 3752:
    case 3753:
    case 3754:
    case 3755:
    case 3756:
    case 3757:
    case 3758:
    case 3759:
    case 3760:
    case 3761:
    case 3762:
    case 3763:
    case 3764:
    case 3765:
    case 3766:
    case 3767:
    case 3768:
    case 3769:
    case 3770:
    case 3771:
    case 3772:
    case 3773:
    case 3774:
    case 3775:
    case 3776:
    case 3777:
    case 3778:
    case 3779:
    case 3780:
    case 3781:
    case 3782:
    case 3783:
    case 3784:
    case 3785:
    case 3786:
    case 3787:
    case 3788:
    case 3789:
    case 3790:
    case 3791:
    case 3792:
    case 3793:
    case 3794:
    case 3795:
    case 3796:
    case 3797:
    case 3798:
    case 3799:
    case 3800:
    case 3801:
    case 3802:
    case 3803:
    case 3804:
    case 3805:
    case 3806:
    case 3807:
    case 3808:
    case 3809:
    case 3810:
    case 3811:
    case 3812:
    case 3813:
    case 3814:
    case 3815:
    case 3816:
    case 3817:
    case 3818:
    case 3819:
    case 3820:
    case 3821:
    case 3822:
    case 3823:
    case 3824:
    case 3825:
    case 3826:
    case 3827:
    case 3828:
    case 3829:
    case 3830:
    case 3831:
    case 3832:
    case 3833:
    case 3834:
    case 3835:
    case 3836:
    case 3837:
    case 3838:
    case 3839:
    case 3840:
    case 3841:
    case 3842:
    case 3843:
    case 3844:
    case 3845:
    case 3846:
    case 3847:
    case 3848:
    case 3849:
    case 3850:
    case 3851:
    case 3852:
    case 3853:
    case 3854:
    case 3855:
    case 3856:
    case 3857:
    case 3858:
    case 3859:
    case 3860:
    case 3861:
    case 3862:
    case 3863:
    case 3864:
    case 3865:
    case 3866:
    case 3867:
    case 3868:
    case 3869:
    case 3870:
    case 3871:
    case 3872:
    case 3873:
    case 3874:
    case 3875:
    case 3876:
    case 3877:
    case 3878:
    case 3879:
    case 3880:
    case 3881:
    case 3882:
    case 3883:
    case 3884:
    case 3885:
    case 3886:
    case 3887:
    case 3888:
    case 3889:
    case 3890:
    case 3891:
    case 3892:
    case 3893:
    case 3894:
    case 3895:
    case 3896:
    case 3897:
    case 3898:
    case 3899:
    case 3900:
    case 3901:
    case 3902:
    case 3903:
    case 3904:
    case 3905:
    case 3906:
    case 3907:
    case 3908:
    case 3909:
    case 3910:
    case 3911:
    case 3912:
    case 3913:
    case 3914:
    case 3915:
    case 3916:
    case 3917:
    case 3918:
    case 3919:
    case 3920:
    case 3921:
    case 3922:
    case 3923:
    case 3924:
    case 3925:
    case 3926:
    case 3927:
    case 3928:
    case 3929:
    case 3930:
    case 3931:
    case 3932:
    case 3933:
    case 3934:
    case 3935:
    case 3936:
    case 3937:
    case 3938:
    case 3939:
    case 3940:
    case 3941:
    case 3942:
    case 3943:
    case 3944:
    case 3945:
    case 3946:
    case 3947:
    case 3948:
    case 3949:
    case 3950:
    case 3951:
    case 3952:
    case 3953:
    case 3954:
    case 3955:
    case 3956:
    case 3957:
    case 3958:
    case 3959:
    case 3960:
    case 3961:
    case 3962:
    case 3963:
    case 3964:
    case 3965:
    case 3966:
    case 3967:
    case 3968:
    case 3969:
    case 3970:
    case 3971:
    case 3972:
    case 3973:
    case 3974:
    case 3975:
    case 3976:
    case 3977:
    case 3978:
    case 3979:
    case 3980:
    case 3981:
    case 3982:
    case 3983:
    case 3984:
    case 3985:
    case 3986:
    case 3987:
    case 3988:
    case 3989:
    case 3990:
    case 3991:
    case 3992:
    case 3993:
    case 3994:
    case 3995:
    case 3996:
    case 3997:
    case 3998:
    case 3999:
    case 4000:
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
