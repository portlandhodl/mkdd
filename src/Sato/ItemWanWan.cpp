#include "Sato/ItemWanWan.h"

#include "Kaneshige/Course/CrsArea.h"

namespace
{
    f32 sJumpGravity = -1.5f;
    f32 sGravity = -5.5f;
    f32 sGravityAdd = 0.1f;
    u8 sMaxHitSeWaitCnt = 30;
    f32 sEfctScaleOffset = 10.0f;
}

const char *ItemWanWanObj::getBmdFileName()
{
    static const char *cWanWanBmdName = "/Item/wanwan.bmd";
    return cWanWanBmdName;
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

inline void J3DAnmObjCluster::anmFrameProc()
{
    mAnmBase->setFrame(mFrameCtrl.getFrame());
}

void ItemWanWanObj::drawSimpleModel(u32 viewNo, Mtx mtx, J3DUClipper *clipper, Mtx mtx2)
{
    mStringObj->drawSimpleModel(viewNo, mtx, clipper, mtx2);
}

void ItemWanWanObj::viewCalc(u32 viewNo) { mModel.viewCalc(viewNo); }

ItemObj::ItemColFlag ItemWanWanObj::calcColReaction(ItemObj *) { return cColFlag_5; }

void ItemWanWanObj::doEquip() { doHandOneEquip(false, 0.0f); }

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

void ItemWanWanObj::setCLPoint()
{
    if (mCLPointController->mNext == nullptr)
        mCLPointController->init_ByKartIndex(mOwnerNum);
}

void ItemWanWanObj::getFirstNodePos(JGeometry::TVec3f *pos)
{
    mStringObj->mStringNodeMgr->getNodePos(0, pos);
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

f32 ItemWanWanObj::getForceEfctOffsetScale() const { return sEfctScaleOffset; }
