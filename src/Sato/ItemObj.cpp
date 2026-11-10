#include "Sato/ItemObj.h"

#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/SysDebug.h"
#include "Sato/GeographyObj.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/ItemSuccession.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Yamamoto/KartSound.h"
#include "Yamamoto/kartCtrl.h"
#include "mathHelper.h"

#include "dolphin/mtx.h"

// declared here since it lacks a header decl; defined in JPEffectPerformer.cpp
bool TstEmitterModeNotAvailableFlg(int, JPABaseEmitter *);

namespace
{
    const f32 sFallParam[4] = {0.98f, -1.5f, 2.0f, 25.0f};

    const f32 sScaleAnmXZ[16] = {9.99999975e-05f, 0.100000001f, 0.230207995f, 0.522221982f, 0.828125f, 1.0f, 0.993956029f, 0.899200022f, 0.767467022f, 0.650488973f, 0.600000024f, 0.630932987f, 0.708800018f, 0.811200023f, 0.91573298f, 1.0f};

    const f32 sScaleAnmY[16] = {9.99999975e-05f, 0.188888997f, 0.411110997f, 0.699999988f, 0.988888979f, 1.21111095f, 1.29999995f, 1.24800003f, 1.12399995f, 0.976000011f, 0.851999998f, 0.800000012f, 0.831250012f, 0.899999976f, 0.96875f, 1.0f};

    const f32 sDisScaleAnmXZ[12] = {1.0f, 1.06037199f, 1.19569898f, 1.33729804f, 1.41648698f, 1.36458099f, 1.19184399f, 0.970719993f, 0.725799978f, 0.481676996f, 0.262941003f, 0.0941850021f};

    const f32 sDisScaleAnmY[12] = {1.0f, 0.91090399f, 0.730135977f, 0.587473989f, 0.612697005f, 0.832813978f, 1.10936499f, 1.29295802f, 1.31825399f, 1.19290805f, 0.924040973f, 0.522159994f};
}

f32 ItemObj::sDivestedGravity = -0.5f;
u8 ItemObj::sDivestedFrame = 40;
u8 ItemObj::sTransferFrame = 30;
f32 ItemObj::sFrontEquipScaleRatio = 0.8f;

ItemObj::ItemObj(u32 kind)
    : mItemKind(kind), mModel(1), mShadow(nullptr), mBounds(nullptr), mBounds2(nullptr), mKartReaction(0x11),
      _150(this), _160(this), _170(this), _180(this), mMiniGameLink(this), mSuccessionItemLink(this),
      mJuggle(nullptr), mBuild(nullptr), mShadowOffsetScale(1.0f), mSoundMgr(nullptr), mShadowScaleY(1.0f)
{
    mAnmPlayer = new ItemDarkAnmPlayer(kind);
    resetObject();
    chkSetFlgByKind();
    createSoundMgr();
}

void ItemObj::reset()
{
    resetObject();
}

void ItemObj::update()
{
    mModel.update(0);
}

void ItemObj::viewCalc(u32 viewNo)
{
    mModel.viewCalc(viewNo);
}

void ItemObj::setCurrentViewNo(u32 viewNo)
{
    mModel.setCurrentViewNo(viewNo);
}

const char *ItemObj::getBmdFileName()
{
    return nullptr;
}

bool ItemObj::IsJuggleItem() const
{
    return false;
}

void ItemObj::createModel(JKRSolidHeap *heap, u32 p2, u32 p3)
{
    mModel.createModel(heap, p2, p3 | 0x60000);
}

void ItemObj::loadmodel(J3DModelData *modelData)
{
    mModel.setModelData(modelData);
    createColModel(modelData);
}

void ItemObj::createColModel(J3DModelData *modelData)
{
    createBoundSphere(modelData);
}

f32 ItemObj::getItemBoundRatio() const
{
    return 1.0f;
}

void ItemObj::doUniv()
{
    (this->*mStateFunc)();
}

void ItemObj::doFall()
{
    moveFall();
}

void ItemObj::initForceDisappear()
{
    bool available = true;
    _12c &= ~1;
    _12c &= ~2;
    mEquipFlags = 0;

    JGeometry::TVec3f pos = mPos;
    pos.y += mEquipScale * _6c;

    if (mOwnerNum >= 0 && mOwnerNum < RCMGetManager()->getKartNumber())
    {
        if (KartInfo::KartCharacter::convPlayerKind(RCMGetManager()->getKartInfo(mOwnerNum)->getPad(0)) == 4)
            available = false;
    }

    if (available)
    {
        const char *emtName;
        if (_12c & 0x200)
            emtName = "mk_item_out_b";
        else
            emtName = "mk_item_out_a";

        mDisappearEmt = GetJPAMgr()->createEmt(emtName, pos);
        mDisappearEmt->setScale(mEquipScale * getForceEfctOffsetScale());
    }
}

void ItemObj::doForceDisappear()
{
    if (_12c & 0x400)
        executeEquipHandFunc();

    mModel.setBaseScale(mDisScaleAnm);

    if (mDisappearEmt != nullptr)
    {
        JPABaseEmitter *emt = mDisappearEmt;
        emt->mGlobalTrs.x = mPos.x;
        emt->mGlobalTrs.y = mPos.y;
        emt->mGlobalTrs.z = mPos.z;

        bool deleteIt = false;
        if (mDisappearEmt->checkStatus(0x8) && mDisappearEmt->getParticleNumber() == 0)
            deleteIt = true;

        if (deleteIt)
            mDisappearEmt = nullptr;
    }

    if (mWaitFrame >= 0x50)
    {
        GetItemObjMgr()->releaseItemByKartEquipItemList(this);
        GetItemObjMgr()->deleteItemByMoveList(this);
        mDisappearEmt = nullptr;
        doDeleteList();
        mState = 10;
        initEnd();
    }
}

void ItemObj::initHeartDisappear()
{
    int deadState = 10;
    mState = deadState;
    initEnd();
    if (mShadow != nullptr)
        mShadow->clrVisibleAll();

    _12c &= ~2;
    _12c &= ~1;
    mEquipFlags = 0;
    deleteEffect();
}

const bool ItemObj::checkKartColDisappear(int)
{
    return true;
}

JSUListIterator<ItemObj> ItemObj::selectSucChild()
{
    return _1a4.getFirst();
}

const char *ItemObj::getShadowBmdFileName(u8 idx)
{
    const char *name = nullptr;
    switch (idx)
    {
    case 0:
        name = "/Item/Item_shadow1.bmd";
        break;
    case 1:
        name = "/Item/IBNkage.bmd";
        break;
    }
    return name;
}

void ItemObj::loadshadowmodel(J3DModelData *modelData)
{
    mShadow = new ShadowModel(0xf, ShadowModel::cShadowKind_Geo, 0xff);
    mShadow->setModelData(modelData);
}

void ItemObj::doStateUniv()
{
    mWaitFrame = 0;
    resetNoColParam();
}

void ItemObj::doDeleteList()
{
    if (mItemKind <= 0x10)
    {
        if (mSuccessionItemLink.getList() == nullptr)
            GetItemObjMgr()->deleteItemByKartEquipItemList(this);
        else
            GetItemObjMgr()->deleteItemByKartEquipItemListSuc(this);
    }
}

void ItemObj::setStateEquip(bool bWait, bool bHeart)
{
    mStateFunc = &ItemObj::doEquip;
    if (mShadow != nullptr)
        mShadow->setVisibleAll();

    _12c &= ~0x80;
    mWaitFrame = 0;
    resetNoColParam();

    if (mSuccessionItemLink.getList() != nullptr && mMulti == mBuild
        && (mSuccessionParent->getSuccessionItemNum() > 1 || GetGeoObjMgr()->isBombBattle()))
    {
        ItemHandOffsetData offsetData = GetItemObjMgr()->getHandOffsetData(this);
        setHandOffsetPos(&offsetData);
    }
    else
    {
        setHandOffsetData();
    }

    mState = 1;
    if (bWait)
        initEquip();

    if (bHeart)
    {
        _12c |= 0x4000;
        for (u8 i = 0; i < 2; i++)
        {
            const char *emtName = ObjUtility::concatSimpleFileName("mk_heartGet", i);
            JPABaseEmitter *emt = JPEffectMgr::mThis->createEmt(emtName, mPos);
            if (emt != nullptr)
            {
                if (!TstEmitterModeNotAvailableFlg(mOwnerNum, emt))
                {
                    emt->setFlag(0x40);
                    mEquipEmt[i] = emt;
                }
            }
        }
        KartCtrl::getKartCtrl()->getKartSound(mOwnerNum)->DoHeartVoice();
    }
}

void ItemObj::setStateHeartWaiting(u8 colorID)
{
    mState = 9;
    mWaitFrame = 0;
    _12c |= 0x80;
    mpHandAnmMtx = getHandAnmMtx();
    setHandOffsetData();
    mColorID = colorID;
    mStateFunc = &ItemObj::doHeartWaiting;
    mSoundMgr->setSe(0x1007f);
}

void ItemObj::setStateRelease(bool flag)
{
    static const char sItemModelMtx0[] = "itemModelMtx 0";
    mState = 2;
    mStateFunc = &ItemObj::doRelease;
    mWaitFrame = 0;
    resetNoColParam();
    GetItemObjMgr()->setItemUseTrigger(mOwnerNum, getDriverNum());

    if (flag)
        initRelease();

    if (mItemKind == 7)
    {
        GetItemObjMgr()->deleteItemKeepMax(this);
    }
    else if (mItemKind != 0xc)
    {
        if (mItemKind <= 0x10)
        {
            if (mSuccessionItemLink.getList() == nullptr)
                GetItemObjMgr()->deleteItemByKartEquipItemList(this);
            else
                GetItemObjMgr()->deleteItemByKartEquipItemListSuc(this);
        }
    }
}

void ItemObj::setStateSpRelease(bool flag)
{
    mStateFunc = &ItemObj::doSpRelease;
    mState = 3;
    mWaitFrame = 0;
    resetNoColParam();
    GetItemObjMgr()->setItemUseTrigger(mOwnerNum, getDriverNum());

    if (flag)
        initSpRelease();

    if (mItemKind <= 0x10)
    {
        if (mSuccessionItemLink.getList() == nullptr)
            GetItemObjMgr()->deleteItemByKartEquipItemList(this);
        else
            GetItemObjMgr()->deleteItemByKartEquipItemListSuc(this);
    }
}

void ItemObj::setStateFall(bool flag)
{
    if (mState == 4 || mState == 10)
        return;

    if (mState == 9)
    {
        mState = 10;
        initEnd();
        return;
    }

    mStateFunc = &ItemObj::doFall;
    if (mItemKind == 0x15)
        GetItemObjMgr()->releaseItemByKartEquipItemList(this);
    if (mShadow != nullptr)
        mShadow->setVisibleAll();

    _12c &= ~0x80;
    mState = 4;
    mWaitFrame = 0;
    resetNoColParam();

    if (flag)
        initFall();

    if (mItemKind <= 0x10)
    {
        if (mSuccessionItemLink.getList() == nullptr)
            GetItemObjMgr()->deleteItemByKartEquipItemList(this);
        else
            GetItemObjMgr()->deleteItemByKartEquipItemListSuc(this);
    }
}

void ItemObj::setStateOccur(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &vel)
{
    mStateFunc = &ItemObj::doOccur;
    mState = 6;
    GetItemObjMgr()->deleteItemKeepMax(this);
    if (mShadow != nullptr)
        mShadow->setVisibleAll();
    initOccur(pos, vel);
}

void ItemObj::setStateDivested(bool flag)
{
    mStateFunc = &ItemObj::doDivested;
    mWaitFrame = 0;
    resetNoColParam();

    if (mSuccessionItemLink.getList() == nullptr)
        setHandOffsetData();

    mState = 5;
    if (!IsSuccessionItem())
    {
        if (flag)
            initDivested();
    }
    else
    {
        for (JSULink<ItemObj> *link = _1a4.getFirst(); link != nullptr; link = link->getNext())
        {
            link->getObject()->mOwnerNum = mOwnerNum;
            link->getObject()->mDriverNum = mDriverNum;
            link->getObject()->mOrigOwnerNum = mOrigOwnerNum;
            link->getObject()->mOrigDriverNum = mOrigDriverNum;
            ItemObj *child = link->getObject();
            child->mStateFunc = &ItemObj::doDivested;
            child->mWaitFrame = 0;
            child->resetNoColParam();
            if (child->mSuccessionItemLink.getSupervisor() == nullptr)
                child->setHandOffsetData();
            child->mState = 5;
            if (!child->IsSuccessionItem())
            {
                child->initDivested();
            }
            else
            {
                for (JSUListIterator<ItemObj> it(child->_1a4.getFirst()); it != child->_1a4.getEnd(); it++)
                {
                    it.getObject()->mOwnerNum = child->mOwnerNum;
                    it.getObject()->mDriverNum = child->mDriverNum;
                    it.getObject()->mOrigOwnerNum = child->mOrigOwnerNum;
                    it.getObject()->mOrigDriverNum = child->mOrigDriverNum;

                    ItemObj *gchild = &(*it);
                    gchild->mStateFunc = &ItemObj::doDivested;
                    gchild->doStateUniv();
                    if (!gchild->IsSuccessionChildItem())
                        gchild->setHandOffsetData();
                    gchild->mState = 5;
                    if (!gchild->IsSuccessionItem())
                    {
                        gchild->initDivested();
                    }
                    else
                    {
                        for (JSUListIterator<ItemObj> it2(gchild->_1a4.getFirst()); it2 != gchild->_1a4.getEnd(); it2++)
                        {
                            (*it2).mOwnerNum = gchild->mOwnerNum;
                            (*it2).mDriverNum = gchild->mDriverNum;
                            (*it2).mOrigOwnerNum = gchild->mOrigOwnerNum;
                            (*it2).mOrigDriverNum = gchild->mOrigDriverNum;
                            (*it2).setStateDivested(true);
                        }
                        gchild->initDivested();
                    }
                }
                child->initDivested();
            }
        }

        if (flag)
            initDivested();
    }
}

inline bool ItemObj::IsSuccessionItem()
{
    return mItemKind > 0x10;
}

inline bool ItemObj::IsSuccessionChildItem()
{
    return mSuccessionItemLink.getList() != nullptr;
}

void ItemObj::setStateDisappear(bool flag)
{
    if (mState == 8 || mState == 7 || mState == 10)
        return;

    mStateFunc = &ItemObj::doDisappear;
    if (mItemKind == 7 || mItemKind == 0x15 || mItemKind == 0xc)
        GetItemObjMgr()->releaseItemByKartEquipItemList(this);
    GetItemObjMgr()->deleteItemByMoveList(this);
    mState = 8;
    mWaitFrame = 0;
    resetNoColParam();
    if (mShadow != nullptr)
        mShadow->clrVisibleAll();
    if (flag)
        initDisappear();
}

void ItemObj::setStateForceDisappear()
{
    if (mState == 8 || mState == 7 || mState == 10)
        return;

    mStateFunc = &ItemObj::doForceDisappear;
    if (mState == 1)
        _12c |= 0x400;
    mState = 7;
    mWaitFrame = 0;
    resetNoColParam();
    if (mShadow != nullptr)
        mShadow->clrVisibleAll();
    initForceDisappear();
}

bool ItemObj::tstAnyObjectHitting()
{
    bool ret = false;
    if ((mHitObjNum != 0) | ((mTransferFlags & 4) != 0) | (mTransferFlags & 1) | ((mTransferFlags >> 3) & 1))
        ret = true;
    return ret;
}

int ItemObj::doSucStateFunc(ItemState state)
{
    int ret = false;
    if (mState == state)
        return false;

    mWaitFrame = 0;
    if (mItemKind == 0x15)
    {
        switch (state)
        {
        case 4:
            if (mState == 4 || mState == 10)
                return 0;
            if (mState == 9)
            {
                mState = 10;
                initEnd();
                return 0;
            }
            mStateFunc = &ItemObj::doFall;
            if (mItemKind == 0x15)
                GetItemObjMgr()->releaseItemByKartEquipItemList(this);
            if (mShadow != nullptr)
                mShadow->setVisibleAll();
            _12c &= ~0x80;
            mState = 4;
            mWaitFrame = 0;
            resetNoColParam();
            initFall();
            doDeleteList();
            return 0;
        case 7:
            if (mState == 8 || mState == 7 || mState == 10)
                return 0;
            mStateFunc = &ItemObj::doForceDisappear;
            if (mState == 1)
                _12c |= 0x400;
            mState = 7;
            mWaitFrame = 0;
            resetNoColParam();
            if (mShadow != nullptr)
                mShadow->clrVisibleAll();
            initForceDisappear();
            return 0;
        case 8:
            if (mState == 8 || mState == 7 || mState == 10)
                return 0;
            mStateFunc = &ItemObj::doDisappear;
            if (mItemKind == 7 || mItemKind == 0x15 || mItemKind == 0xc)
                GetItemObjMgr()->releaseItemByKartEquipItemList(this);
            GetItemObjMgr()->deleteItemByMoveList(this);
            mState = 8;
            mWaitFrame = 0;
            resetNoColParam();
            if (mShadow != nullptr)
                mShadow->clrVisibleAll();
            initDisappear();
            return 0;
        default:
            break;
        }
    }
    else
    {
        if (state == 7)
        {
            if (mState != 8 && mState != 7 && mState != 10)
            {
                mStateFunc = &ItemObj::doForceDisappear;
                if (mState == 1)
                    _12c |= 0x400;
                mState = 7;
                mWaitFrame = 0;
                resetNoColParam();
                if (mShadow != nullptr)
                    mShadow->clrVisibleAll();
                initForceDisappear();
            }
        }
    }

    if (_1a4.getNumLinks() == 0)
    {
        GetItemObjMgr()->releaseItemByKartEquipItemList(this);
        mState = 10;
        initEnd();
        ret = true;
    }
    else if (state == 7)
    {
        for (JSULink<ItemObj> *link = _1a4.getFirst(); link != nullptr; link = link->getNext())
        {
            ItemObj *child = link->getObject();
            if (child->mState != 8 && child->mState != 7 && child->mState != 10)
            {
                child->mStateFunc = &ItemObj::doForceDisappear;
                if (child->mState == 1)
                    child->_12c |= 0x400;
                child->mState = 7;
                child->mWaitFrame = 0;
                child->resetNoColParam();
                if (child->mShadow != nullptr)
                    child->mShadow->clrVisibleAll();
                child->initForceDisappear();
            }
        }
    }
    else
    {
        int divestNum = 1;
        if (_12c & 0x40)
            divestNum = _1a4.getNumLinks();

        for (u32 i = 0; i < (u32)divestNum; i++)
        {
            JSUListIterator<ItemObj> it = selectSucChild();
            ItemObj *childLink = it.mLink->getObject();
            if (childLink == nullptr)
                continue;

            switch (state)
            {
            case 2:
            {
                ItemObj *child = it.mLink->getObject();
                doSucReleaseFunc(child);
                child->setStateRelease(true);
                break;
            }
            case 3:
            {
                ItemObj *child = it.mLink->getObject();
                doSucSpReleaseFunc(child);
                child->setStateSpRelease(true);
                break;
            }
            case 4:
            {
                ItemObj *child = it.mLink->getObject();
                if (child->mState != 4 && child->mState != 10)
                {
                    if (child->mState == 9)
                    {
                        child->mState = 10;
                        child->initEnd();
                    }
                    else
                    {
                        child->mStateFunc = &ItemObj::doFall;
                        if (child->mItemKind == 0x15)
                            GetItemObjMgr()->releaseItemByKartEquipItemList(child);
                        if (child->mShadow != nullptr)
                            child->mShadow->setVisibleAll();
                        child->_12c &= ~0x80;
                        child->mState = 4;
                        child->mWaitFrame = 0;
                        child->resetNoColParam();
                        child->initFall();
                        if (child->mItemKind <= 0x10)
                        {
                            if (child->mSuccessionItemLink.getList() == nullptr)
                                GetItemObjMgr()->deleteItemByKartEquipItemList(child);
                            else
                                GetItemObjMgr()->deleteItemByKartEquipItemListSuc(child);
                        }
                    }
                }
                break;
            }
            case 8:
            {
                ItemObj *child = it.mLink->getObject();
                if (child->mState != 8 && child->mState != 7 && child->mState != 10)
                {
                    child->mStateFunc = &ItemObj::doDisappear;
                    if (child->mItemKind == 7 || child->mItemKind == 0x15 || child->mItemKind == 0xc)
                        GetItemObjMgr()->releaseItemByKartEquipItemList(child);
                    GetItemObjMgr()->deleteItemByMoveList(child);
                    child->mState = 8;
                    child->mWaitFrame = 0;
                    child->resetNoColParam();
                    if (child->mShadow != nullptr)
                        child->mShadow->clrVisibleAll();
                    child->initDisappear();
                }
                break;
            }
            default:
                break;
            }

            _1a4.remove(&childLink->mSuccessionItemLink);
            if (_1a4.mHead == nullptr)
            {
                GetItemObjMgr()->releaseItemByKartEquipItemList(this);
                mState = 10;
                initEnd();
                ret = true;
            }
        }
    }
    return ret;
}


bool ItemObj::disappearAllChild(bool flag)
{
    if (!IsSuccessionItem())
        return false;

    u32 numChildren = _1a4.getNumLinks();
    bool ret = false;
    for (u32 i = 0; i < numChildren; i++)
    {
        JSUListIterator<ItemObj> it = selectSucChild();
        JSULink<ItemObj> *link = it.mLink;
        if (link != nullptr && flag)
        {
            ItemObj *child = it.mLink->getObject();
            if (child->mState != 8 && child->mState != 7 && child->mState != 10)
            {
                child->mStateFunc = &ItemObj::doDisappear;
                if (child->mItemKind == 7 || child->mItemKind == 0x15 || child->mItemKind == 0xc)
                    GetItemObjMgr()->releaseItemByKartEquipItemList(child);
                GetItemObjMgr()->deleteItemByMoveList(child);
                child->mState = 8;
                child->mWaitFrame = 0;
                child->resetNoColParam();
                if (child->mShadow != nullptr)
                    child->mShadow->clrVisibleAll();
                child->initDisappear();
            }
        }
        _1a4.remove(&it.mLink->getObject()->mSuccessionItemLink);
        if (_1a4.getFirst() == nullptr)
            ret = true;
    }
    return ret;
}

void ItemObj::doSucReleaseFunc(ItemObj *) {}

void ItemObj::doSucSpReleaseFunc(ItemObj *) {}

int ItemObj::getSuccessionItemNum()
{
    int num = -1;
    if (IsSuccessionItem())
        num = _1a4.getNumLinks();
    return num;
}

void ItemObj::setChildStateRelease()
{
    doSucStateFunc((ItemState)2);
}

void ItemObj::setChildStateSpRelease()
{
    doSucStateFunc((ItemState)3);
}

void ItemObj::setChildStateFall()
{
    doSucStateFunc((ItemState)4);
}

void ItemObj::setChildStateForceDisappear()
{
    doSucStateFunc((ItemState)7);
}

void ItemObj::calc()
{
    if (mState == 9)
    {
        doHeartWaiting();
        mWaitFrame++;
    }
    else
    {
        mTransferFlags &= ~0x80;
        doUniv();
        calcDarkRatio();

        if (!IsSuccessionItem())
        {
            calcColPos();
            if (mItemKind != 8 && mItemKind != 7 && mItemKind != 0xc)
            {
                if (_12c & 0x10)
                    checkPrioritySpecial();
                else
                    checkPriorityNormal();
            }
        }

        doFrameNoColKart();

        if (mShadow != nullptr && (mShadow->getFlags() & 0xf) == 0xf)
        {
            Mtx m;
            PSMTXIdentity(m);

            f32 shadowY = mGroundHeight;
            f32 scaleY = mShadowScaleY;
            if (mState == 1)
            {
                shadowY = getHandAnmMtx()[1][3];
                scaleY = 1.0f;
            }
            else if (!(mTransferFlags & 0x80))
            {
                CrsGround ground(RCMGetManager()->getCourse());
                ground.search(mPos, _10);
                shadowY = ground.getHeight();
            }

            m[0][3] = mPos.x;
            m[1][3] = shadowY;
            m[2][3] = mPos.z;
            mShadow->setBaseTRMtx(m);

            JGeometry::TVec3f scaleVec = mDisScaleAnm * mShadowOffsetScale;
            scaleVec.y *= scaleY;
            J3DModel *shadowModel = mShadow->getModel();
            shadowModel->mModelScale.x = scaleVec.x;
            shadowModel->mModelScale.y = scaleVec.y;
            shadowModel->mModelScale.z = scaleVec.z;
        }

        if (mSoundMgr != nullptr)
            mSoundMgr->frameWork();

        CrsArea area;
        area.search(0, mPos);
        static_cast<ItemDarkAnmPlayer *>(mAnmPlayer)->calc(area, mDarkRatio);

        mWaitFrame++;
        _10 = mPos;
    }
}

void ItemObj::calcDarkRatio()
{
    int owner = mOwnerNum;
    if (mState == 1 && owner >= 0 && owner < RCMGetManager()->getKartNumber())
    {
        u8 driverNum = getDriverNum();
        if (!((u8)driverNum == (u8)ItemObjMgr::getNowTandemDriverNum(owner)))
        {
            if (mDarkCount < 20.0f)
            {
                mDarkCount += 1.0f;
                mDarkRatio = mDarkCount * 0.05f;
                mDarkScale = 1.0f - 0.2f * mDarkRatio;
            }
        }
        else
        {
            if (mDarkCount > 0.0f)
            {
                mDarkCount -= 1.0f;
                mDarkRatio = mDarkCount * 0.05f;
                mDarkScale = 1.0f - 0.2f * mDarkRatio;
            }
        }
    }
    else
    {
        if (mDarkCount > 0.0f)
        {
            mDarkCount -= 1.0f;
            mDarkRatio = mDarkCount * 0.05f;
            mDarkScale = 1.0f - 0.2f * mDarkRatio;
        }
    }
}

void ItemObj::checkPriorityNormal()
{
    if (mItemKind == 0xa || mState != 8)
    {
        if ((mTransferFlags & 2) && mVel.length() < 3.0f)
        {
            if (mGndAttr == 1)
                _1ec = 1;
            else
                _1ec = 0;
        }
        else
        {
            _1ec = 1;
        }
    }
}

void ItemObj::checkPrioritySpecial()
{
    if (mTransferFlags & 2)
    {
        if (mVel.length() < 3.0f)
        {
            if (mGndAttr == 1)
                _1ec = 2;
            else
                _1ec = 0;
        }
        else
        {
            _1ec = 2;
        }
    }
}

void ItemObj::initFall()
{
    mEquipFlags = 0xffffffff;
    initmoveFall();
}

void ItemObj::initOccur(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &vel)
{
    mEquipFlags = -1;
    mOwnerNum = -1;
    mDriverNum = 0;
    initCommonMoveFall(vel);
    mPos = pos;
    _10 = mPos;
    _12c &= ~1;
}

void ItemObj::createSoundMgr()
{
    if (mSoundMgr == nullptr)
        mSoundMgr = new GameAudio::ObjectSoundMgr(&mPos, nullptr);
}

void ItemObj::createBoundSphere(J3DModelData *modelData)
{
    mBounds = new ObjColSphere(modelData, mDisScaleAnm);
    mBounds2 = mBounds;
}

void ItemObj::createBoundSphere(f32 radius, f32 scale)
{
    mBounds = new ObjColSphere(radius, scale);
}

void ItemObj::createBoundCylinder(J3DModelData *modelData)
{
    mBounds = new ObjColCylinder(modelData, mDisScaleAnm);
    mBounds2 = mBounds;
}

int ItemObj::getVelStickXThrow(JGeometry::TVec3f *vel, f32 factor, f32 threshold)
{
    vel->z = 0.0f;
    vel->y = 0.0f;
    vel->x = 0.0f;

    int hasThrow = 0;
    f32 stickX = KartCtrl::getKartCtrl()->GetItemStickX(mOwnerNum);
    if (fabs(stickX) > threshold)
    {
        JGeometry::TVec3f xdir;
        ObjUtility::getKartXdir(mOwnerNum, &xdir);
        hasThrow = 1;
        f32 scale = -stickX * factor;
        xdir.scale(scale);
        *vel = xdir;
    }
    return hasThrow;
}

void ItemObj::createShadowModel(JKRSolidHeap *heap, u32 p2, u32 p3)
{
    mShadow->createModel(heap, p2, p3);
    CrsGround ground(RCMGetManager()->getCourse());
    ground.search(mPos, _10);

    MtxPtr mtx = (MtxPtr)&mModel.getBaseTRMtx();
    PSMTXCopy(mtx, mtx);
    mtx[1][3] = ground.getHeight();
    mShadow->setBaseTRMtx(mtx);
    J3DModel *shadowModel = mShadow->getModel();
    shadowModel->mModelScale.x = mDisScaleAnm.x;
    shadowModel->mModelScale.y = mDisScaleAnm.y;
    shadowModel->mModelScale.z = mDisScaleAnm.z;
    mShadow->clrVisibleAll();
}

void ItemObj::setModelMatrixAndScale(Quaternion quat)
{
    Mtx m;
    stQt2Mtx(m, &quat);
    m[0][3] = mPos.x;
    m[1][3] = mPos.y;
    m[2][3] = mPos.z;

    JGeometry::TVec3f sc = mDisScaleAnm;
    sc.scale(mDarkScale);
    mModel.setBaseScale(sc);
    mModel.setBaseTRMtx(m);
    SysDebug::checkInvalidMatrix(m, "itemModelMtx 1");
}

void ItemObj::setModelMatrixAndScale(Mtx srcMtx)
{
    Mtx m;
    PSMTXCopy(srcMtx, m);
    m[0][3] = mPos.x;
    m[1][3] = mPos.y;
    m[2][3] = mPos.z;

    JGeometry::TVec3f sc = mDisScaleAnm;
    sc.scale(mDarkScale);
    mModel.setBaseScale(sc);
    mModel.setBaseTRMtx(m);
    SysDebug::checkInvalidMatrix(m, "itemModelMtx 2");
}

void ItemObj::setScaleCol(f32 scale)
{
    mDisScaleAnm.set(scale, scale, scale);
    if (mBounds != nullptr)
        mBounds->setScale(scale);
}

void ItemObj::resetObject()
{
    mPos.zero();
    _10.zero();
    mColPos.zero();
    _28 = 0.0f;
    _2c = 0.0f;
    mMaxVel = 0.0f;
    QuaternionReset(&_34);
    QuaternionReset(&_44);
    mLerpRatio = 0.0f;
    mDisScaleAnm.zero();
    mEquipScale = 0.0001f;
    mHeightOffset = 0.0f;
    _6c = 0.0f;
    mVel.zero();

    mState = 0;
    mWaitFrame = 0;
    mOwnerNum = -1;
    mDriverNum = 0xff;
    mpHandAnmMtx = nullptr;
    mGndAttr = 0xff;
    _1fc = 0;
    mSucKind = 0x10;
    mMulti = nullptr;
    mSuccessionParent = nullptr;
    mOrigOwnerNum = -1;
    mOrigDriverNum = 0;
    mDivestedPos.zero();
    QuaternionReset(&_1d8);
    mDivestedGravity = 0.0f;
    mDivestedMaxFrame = 0;
    _1ec = 1;
    mMulti = mJuggle;
    resetNoColParam();
    mReflectWallVec.zero();
    _240.zero();
    mNormal.set(0.0f, 1.0f, 0.0f);
    mHandOffset.zero();
    mHandOffsetPos.zero();
    PSMTXIdentity(_210);
    mDirectHitKartNo = -1;
    mGroundHeight = 0.0f;
    mColorID = 0;
    mDarkRatio = 0.0f;
    mDarkCount = 0.0f;
    mDarkScale = 1.0f;
    _2a8 = 0;

    Mtx mtx;
    stQt2Mtx(mtx, &_34);
    if (mModel.getModel() != nullptr)
        mModel.setBaseTRMtx(mtx);

    mDisappearEmt = nullptr;
    mEquipEmt[1] = nullptr;
    mEquipEmt[0] = nullptr;
    _24c = 0;
    clearAllFlg();
    if (mBounds != nullptr)
        mBounds->setScale(1.0f);
    mAnmPlayer->reset();
}

void ItemObj::resetNoColParam()
{
    mNoColKartMax = 0;
    mNoColKartCnt = 0;
    mNoColKartClr = 0;
    mNoColKart = 0;
}

void ItemObj::clearAllFlg()
{
    _12c = 0;
    mEquipFlags = 0;
    mHitObjNum = 0;
    mTransferFlags = 0;
    chkSetFlgByKind();
}

void ItemObj::chkSetFlgByKind()
{
    if ((u8)IsSpecialItem(mItemKind))
        _12c |= 0x10;

    if (mItemKind == 2 || (_12c & 0x10) || mItemKind == 0xd)
        _12c |= 0x1000;
}

void ItemObj::getModelLfDir(JGeometry::TVec3f *out)
{
    out->set(mModel.mBaseTRMtx[0][0], mModel.mBaseTRMtx[1][0], mModel.mBaseTRMtx[2][0]);
}

void ItemObj::getModelUpDir(JGeometry::TVec3f *out)
{
    out->set(mModel.mBaseTRMtx[0][1], mModel.mBaseTRMtx[1][1], mModel.mBaseTRMtx[2][1]);
}

void ItemObj::getModelFrDir(JGeometry::TVec3f *out)
{
    out->set(mModel.mBaseTRMtx[0][2], mModel.mBaseTRMtx[1][2], mModel.mBaseTRMtx[2][2]);
}

void ItemObj::rotationRad(f32 &out, f32 add)
{
    out += add;
    if (out >= 3.1415927f)
        out = -6.2831855f + out;
    else if (out <= -3.1415927f)
        out = 6.2831855f + out;
}

void ItemObj::QuatRotX(f32 rad, Quaternion *out)
{
    Vec axis = {1.0f, 0.0f, 0.0f};
    C_QUATRotAxisRad(rad, (f32 *)out, &axis);
}

void ItemObj::QuatRotY(f32 rad, Quaternion *out)
{
    Vec axis = {0.0f, 1.0f, 0.0f};
    static const Vec axisZ = {0.0f, 0.0f, 1.0f};
    (void)axisZ;
    C_QUATRotAxisRad(rad, (f32 *)out, &axis);
}

int ItemObj::IsSpecialItem(int kind)
{
    u8 ret = false;
    switch (kind)
    {
    case 1:
    case 4:
    case 7:
    case 8:
    case 11:
    case 12:
    case 15:
    case 18:
    case 20:
    case 21:
        ret = true;
    }
    return ret;
}

void ItemObj::getCompVecLength(JGeometry::TVec3f &outComp, f32 &outLen)
{
    JGeometry::TVec3f kartXdir;
    ObjUtility::getKartXdir(mOwnerNum, &kartXdir);

    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mOwnerNum, &kartPos);

    JGeometry::TVec3f diff;
    diff.sub(mPos, kartPos);

    JGeometry::TVec3f comp;
    comp.scale(-kartXdir.dot(diff), kartXdir);
    outLen = comp.length();
    stVecNormalize(&comp);
    outComp = comp;
}

void ItemObj::setFrameNoColKart(u8 max, bool clr)
{
    mNoColKart = 1;
    mNoColKartCnt = 0;
    mNoColKartMax = max;
    mNoColKartClr = clr;
    if (mNoColKartClr)
    {
        mEquipFlags = 0;
    }
    else
    {
#line 0x760 "ItemObj.cpp"
        JUT_MINMAX_ASSERT(0, mOwnerNum, 8);
        mEquipFlags &= ~(1 << mOwnerNum);
    }
}

void ItemObj::doFrameNoColKart()
{
    if (mNoColKart != 0)
    {
        mNoColKartCnt++;
        if (mNoColKartCnt > mNoColKartMax)
        {
            mNoColKart = 0;
            mNoColKartCnt = 0;
            if (mNoColKartClr)
            {
                mEquipFlags = -1;
                return;
            }
#line 0x778 "ItemObj.cpp"
            JUT_MINMAX_ASSERT(0, mOwnerNum, 8);
            mEquipFlags |= (1 << mOwnerNum);
        }
        else
        {
            if (mNoColKartClr)
            {
                mEquipFlags = 0;
                return;
            }
#line 0x781 "ItemObj.cpp"
            JUT_MINMAX_ASSERT(0, mOwnerNum, 8);
            mEquipFlags &= ~(1 << mOwnerNum);
        }
    }
}

void ItemObj::calcColPos()
{
    JGeometry::TVec3f up;
    getModelUpDir(&up);
    up.scale(_6c * mDisScaleAnm.y);
    mColPos.add(mPos, up);
}

bool ItemObj::IsHitKart(int kartNo, const JGeometry::TVec3f &kartPos, f32 radius)
{
    bool ret = false;
    const JGeometry::TVec3f &kp = kartPos;
    if ((KartCtrl::getKartCtrl()->getKartBody(kartNo)->mCarStatus & 0x00B80000) == 0)
    {
        if (!GetItemObjMgr()->tstColReactionKart(kartNo, this))
            goto hit;
    }

    if (_12c & 0x100)
        return ret;

hit:
    ret = mBounds->IsHitSphere(mColPos, kp, radius);
    return ret;
}

bool ItemObj::IsHitObject(ObjColBase *bounds, const JGeometry::TVec3f &colPos, ObjColBase *kartBounds)
{
    bool hit = false;
    switch (bounds->getKind())
    {
    case ObjColBase::SPHERE:
        hit = kartBounds->IsHitSphere(colPos, mColPos, bounds->getRadius() * bounds->getScale());
        break;
    case ObjColBase::CYLINDER:
        hit = kartBounds->IsHitCylinder(colPos, mColPos, *(ObjColCylinder *)bounds);
        break;
    }
    return hit;
}

void ItemObj::checkStartColItemFrame(u32 frame)
{
    if (mWaitFrame >= frame)
        _12c |= 1;
}


void ItemObj::doHandOneEquip(bool flag, f32 lerp)
{
    Mtx quatMtx;
    stQt2Mtx(quatMtx, &_34);

    JGeometry::TVec3f offPos = getHandOffsetPos();
    SysDebug::checkInvalidMatrix(mpHandAnmMtx, "ItemHandMtx");
    PSMTXMultVec(mpHandAnmMtx, (Vec *)&offPos, (Vec *)&offPos);
    mPos = offPos;

    Mtx handMtx;
    PSMTXCopy(mpHandAnmMtx, handMtx);
    handMtx[2][3] = 0.0f;
    handMtx[1][3] = 0.0f;
    handMtx[0][3] = 0.0f;

    MtxPtr pMtx = _210;
    if (pMtx != nullptr)
        PSMTXConcat(pMtx, quatMtx, quatMtx);

    PSMTXConcat(handMtx, quatMtx, handMtx);

    if (flag)
    {
        doPosLerp(&mPos, *(JGeometry::TVec3f *)&offPos, lerp);
        doRotationLerp((JGeometry::TPos3f *)handMtx, pMtx, lerp);
    }

    Mtx finalMtx;
    PSMTXCopy(handMtx, finalMtx);
    finalMtx[0][3] = mPos.x;
    finalMtx[1][3] = mPos.y;
    finalMtx[2][3] = mPos.z;

    JGeometry::TVec3f sc = mDisScaleAnm;
    sc.scale(mDarkScale);
    mModel.setBaseScale(sc);
    mModel.setBaseTRMtx(finalMtx);
    SysDebug::checkInvalidMatrix(finalMtx, "itemModelMtx 2");

    if (_12c & 0x4000)
    {
        for (u8 i = 0; i < 2; i++)
        {
            f32 scale = getForceEfctOffsetScale();
            f32 allScale = mEquipScale * scale;
            JPABaseEmitter *emt = mEquipEmt[i];
            if (emt != nullptr)
            {
                emt->mGlobalTrs.x = mPos.x;
                emt->mGlobalTrs.y = mPos.y;
                emt->mGlobalTrs.z = mPos.z;
                mEquipEmt[i]->setScale(allScale);

                bool removeIt = false;
                if (mEquipEmt[i]->checkStatus(0x8) && mEquipEmt[i]->getParticleNumber() == 0)
                    removeIt = true;

                if (removeIt)
                {
                    GetJPAMgr()->deleteEmt(emt);
                    mEquipEmt[i] = nullptr;
                    _12c &= ~0x4000;
                }
            }
        }
    }
}
// 3721+ doHandOneEquip; note getForceEfctOffsetScale virtual call in loop


void ItemObj::doPosLerp(JGeometry::TVec3f *out, const JGeometry::TVec3f &target, f32 ratio)
{
    f32 wf = (f32)mWaitFrame;
    JGeometry::TVec3f pos(mDivestedPos);
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mOwnerNum, &kartPos);

    pos.add(kartPos);
    JMAVECLerp(&pos, (Vec *)&target, (Vec *)out, ratio);

    out->y += mDivestedGravity * wf + wf * (sDivestedGravity * wf);
}

void ItemObj::doRotationLerp(JGeometry::TPos3f *out, f32 (*m2)[4], f32 ratio)
{
    MtxPtr handMtx = getJuggleHandAnmMtx();
    stMtx2Qt(&_44, handMtx);

    Quaternion lerped;
    stQtLerp(&lerped, &_1d8, &_44, ratio);
    stQt2Mtx((MtxPtr)out, &lerped);
    if (m2 != nullptr)
        PSMTXConcat((MtxPtr)out, m2, (MtxPtr)out);
}

void ItemObj::doEquipScaleUp(f32 scale)
{
    if (mState == 1 || mState == 5)
    {
        mEquipScale = scale;
        f32 mul = 1.0f;
        u32 startFrame = 0;
        if (mSuccessionItemLink.getList() != nullptr)
        {
            int num;
            ItemObjSuc *suc = mSuccessionParent;
            switch (*(int *)((char *)suc + 0x2d4))
            {
            case 0:
                startFrame = *(u32 *)((char *)mJuggle + 0x1c);
                break;
            case 1:
            {
                num = -1;
                startFrame = *(u8 *)((char *)mBuild + 0x2d);
                if (*(u32 *)((char *)suc + 0x7c) > 0x10)
                    num = *(int *)((char *)suc + 0x1ac);
                if (num > 1)
                    mul = getThunderScale();
                break;
            }
            }
        }
        else
        {
            startFrame = 3;
        }
        executeEquipScaleUp(startFrame, mul);
    }
}

void ItemObj::executeEquipScaleUp(u32 startFrame, f32 scale)
{
    calcEquipAnmScaleUp(mDisScaleAnm, startFrame);
    f32 totalScale = mEquipScale * scale;
    mDisScaleAnm.scale(totalScale);
    _2a8++;
}

void ItemObj::doForceDisappearScaleDown()
{
    calcForceDisappearScaleDown(mDisScaleAnm, 0);
    mDisScaleAnm.scale(mEquipScale);
}

void ItemObj::calcEquipAnmScaleUp(JGeometry::TVec3f &out, u32 startFrame)
{
    if (_2a8 < startFrame)
    {
        out.set(0.0001f, 0.0001f, 0.0001f);
        return;
    }

    u32 frame = _2a8 - startFrame;
    if (frame > 0xf)
    {
        out.set(1.0f, 1.0f, 1.0f);
        mTransferFlags |= 0x20;
    }
    else
    {
        out.z = sScaleAnmXZ[frame];
        out.x = sScaleAnmXZ[frame];
        out.y = sScaleAnmY[frame];
    }
}

void ItemObj::calcForceDisappearScaleDown(JGeometry::TVec3f &out, u32 startFrame)
{
    u32 frame = mWaitFrame - startFrame;
    if (mWaitFrame < startFrame)
    {
        out.set(1.0f, 1.0f, 1.0f);
        return;
    }
    if (frame > 0xb)
    {
        out.set(0.0f, 0.0f, 0.0f);
    }
    else
    {
        out.z = sDisScaleAnmXZ[frame];
        out.x = sDisScaleAnmXZ[frame];
        out.y = sDisScaleAnmY[frame];
    }
}

void ItemObj::doReflectGeoObj(GeographyObj *geoObj, f32 pow)
{
    JGeometry::TVec3f normVec;
    normVec.set(0.0f, 0.0f, 1.0f);

    f32 mag = PSVECMag(&mVel);
    ObjColBase *col = geoObj->mBounds[0];

    JGeometry::TVec3f outVec;
    outVec.zero();

    if (col->getKind() == ObjColBase::CUBE)
    {
        normVec = col->getPos();
        if (mVel.length() > 0.0f)
        {
            JGeometry::TVec3f nrm(normVec);
            nrm.normalize();

            JGeometry::TVec3f vel(mVel);
            vel.normalize();
            vel.negate();

            f32 d = nrm.dot(vel);
            outVec.x = nrm.x * d + vel.x;
            outVec.y = nrm.y * d + vel.y;
            outVec.z = nrm.z * d + vel.z;
            outVec.y = 0.0f;
            stVecNormalize(&outVec);
        }
    }
    else
    {
        JGeometry::TVec3f diff;
        diff.sub(mPos, geoObj->mPos);
        normVec = diff;
        stVecNormalize(&normVec);

        if (col->getKind() == ObjColBase::CYLINDER)
        {
            outVec = col->getPos();
        }
        else
        {
            outVec.sub(mPos, geoObj->mPos);
            stVecNormalize(&outVec);
        }
    }

    f32 scale = mag * pow;
    normVec.scale(col->getBoundDepth());
    mReflectWallVec += normVec;
    outVec *= scale;
    mVel = outVec;
}

void ItemObj::doMovePos()
{
    if (mVel.y < -95.0f)
        mVel.y = -95.0f;
    mPos += mVel;
}

void ItemObj::doGeoObjectCol(GeographyObj *geoObj)
{
    if (mItemKind == 8 && mState == 8)
        return;

    switch (geoObj->mReaction.tstFlg(mItemKind))
    {
    case 1:
        doReflectGeoObj(geoObj, _2c);
        break;
    case 2:
        if (mState != 8 && mState != 7 && mState != 10)
        {
            mStateFunc = &ItemObj::doDisappear;
            if (mItemKind == 7 || mItemKind == 0x15 || mItemKind == 0xc)
                GetItemObjMgr()->releaseItemByKartEquipItemList(this);
            GetItemObjMgr()->deleteItemByMoveList(this);
            mState = 8;
            mWaitFrame = 0;
            mNoColKartMax = 0;
            mNoColKartCnt = 0;
            mNoColKartClr = 0;
            mNoColKart = 0;
            if (mShadow != nullptr)
                mShadow->clrVisibleAll();
            initDisappear();
        }
        break;
    case 3:
    {
        JGeometry::TVec3f vec;
        f32 mag;
        geoObj->getItemThrowDirPow(&vec, &mag, *this);
        setCollideAddVel(vec, mag);
        break;
    }
    default:
        break;
    }
    executeGeoObjHitCallBack(geoObj);
}

void ItemObj::executeGeoObjHitCallBack(GeographyObj *) {}

void ItemObj::setCollideAddVel(const JGeometry::TVec3f &vel, const f32 &scale)
{
    mVel.add(vel * scale);
}


void ItemObj::initmoveDivested()
{
    if (mSuccessionItemLink.getList() != nullptr)
    {
        mMulti->InitMoveDivested();
        return;
    }

    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mOwnerNum, &kartPos);
    mDivestedPos.sub(mPos, kartPos);
    mpHandAnmMtx = getHandAnmMtx();
    stMtx2Qt(&_44, mModel.getBaseTRMtx());
    stMtx2Qt(&_1d8, mpHandAnmMtx);
    initMaxDivestedFrame();
}

void ItemObj::moveDivested()
{
    if (mSuccessionItemLink.getList() != nullptr)
    {
        mMulti->MoveDivested();
        return;
    }

    f32 ratio = (f32)mWaitFrame / (f32)mDivestedMaxFrame;
    doHandOneEquip(true, ratio);

    if (mWaitFrame < mDivestedMaxFrame)
        return;

    QuaternionReset(&_34);
    mStateFunc = &ItemObj::doEquip;
    if (mShadow != nullptr)
        mShadow->setVisibleAll();
    _12c &= ~0x80;
    mWaitFrame = 0;
    mNoColKartMax = 0;
    mNoColKartCnt = 0;
    mNoColKartClr = 0;
    mNoColKart = 0;

    if (IsSuccessionChildItem())
    {
        if (mMulti == mBuild)
        {
            int num = -1;
            if (mSuccessionParent->mItemKind > 0x10)
                num = mSuccessionParent->_1a4.getNumLinks();
            if (num > 1 || GetGeoObjMgr()->isBombBattle())
            {
                ItemHandOffsetData offsetData = GetItemObjMgr()->getHandOffsetData(this);
                setHandOffsetPos(&offsetData);
            }
            else
            {
                setHandOffsetData();
            }
        }
     }
    mState = 1;
    initEquip();
}




void ItemObj::initmoveFall()
{
    JGeometry::TVec3f fallDir;
    MtxPtr kartMtxPtr = ObjUtility::getKartBodyMtx(mOwnerNum);
    getItemRnd()->getArbitUnitVecXZ(fallDir, 0.3f);

    Mtx kartMtx;
    PSMTXCopy(kartMtxPtr, kartMtx);

    kartMtx[0][3] = kartMtx[1][3] = kartMtx[2][3] = 0.0f;
    PSMTXMultVec(kartMtx, (Vec *)&fallDir, (Vec *)&fallDir);

    fallDir.negate(fallDir);

    JGeometry::TVec3f fallUp = fallDir;
    stVecNormalize(&fallUp);
    fallUp.y += sFallParam[2];
    stVecNormalize(&fallUp);

    f32 windup = (0.7f + 0.3f * getItemRnd()->get_ufloat_1()) * sFallParam[3];
    fallUp.scale(windup);

    JGeometry::TVec3f kartVel;
    ObjUtility::getKartVel(mOwnerNum, &kartVel);
    fallUp.add(kartVel);

    initCommonMoveFall(fallUp);
}

void ItemObj::initCommonMoveFall(const JGeometry::TVec3f &vel)
{
    mVel = vel;
    _12c |= 1;
    _12c |= 2;
    setFrameNoColKart(30, true);
    initCommonMoveFallMtx();
}

void ItemObj::initCommonMoveFallMtx()
{
    JGeometry::TVec3f vel = mVel;
    stVecNormalize(&vel);
    _240 = vel;
    calcBananaFirstQuat(_240);
    _34 = _44;
    initMoveColGround();
}

void ItemObj::moveFall()
{
    if (!(_12c & 1) && mWaitFrame >= 0x1e)
        _12c |= 1;

    if (mState != 6)
        doFrameNoColKart();

    mVel.scale(sFallParam[0]);
    mVel.y += sFallParam[1];

    if (mVel.y < -95.0f)
        mVel.y = -95.0f;

    mPos += mVel;
    doMoveColGround(nullptr, true);

    if (!(mTransferFlags & 2))
    {
        JGeometry::TVec3f norm;
        norm.set(mVel);
        if (norm.length() > 0.0f)
        {
            stVecNormalize(&norm);
            stVec2QtUpdate(_44, _240, *(Vec *)&norm);
        }
    }

    _240.set(mVel);
    stVecNormalize(&_240);
}

void ItemObj::doHeartWaiting()
{
    JGeometry::TVec3f offPos = getHandOffsetPos();
    PSMTXMultVec(mpHandAnmMtx, (Vec *)&offPos, (Vec *)&offPos);
    mPos = offPos;
}

void ItemObj::doFallScaling()
{
    if (mEquipScale < 3.0f)
        mEquipScale += 0.2f;
    else
        mEquipScale = 3.0f;
}

ItemObj::ItemColFlag ItemObj::calcColReaction(ItemObj *other)
{
    ItemColFlag ret = cColFlag_4;
    int kind = other->mItemKind;
    switch (kind)
    {
    case 3:
    case 5:
    case 6:
    case 10:
        ret = cColFlag_0;
        break;
    }
    return ret;
}

void ItemObj::colItemReflect(ItemObj *other1, ItemObj *other2, ItemColFlag flag1, ItemColFlag flag2)
{
    JGeometry::TVec3f diff;
    diff.sub(other1->mPos, other2->mPos);

    f32 len1 = other1->mVel.length();
    f32 len2 = other2->mVel.length();
    f32 baseLen = 0.5f * (len1 + len2) + 5.0f;
    (void)baseLen;

    ItemObj *obj = other1;
    ItemColFlag flag = flag1;
    stVecNormalize(&diff);
    for (u32 i = 0; i < 2; i++)
    {
        if (i == 1)
        {
            obj = other2;
            flag = flag2;
            diff.negate();
        }

        switch (flag)
        {
        case 1:
            obj->doColMoveJumpDown(diff, baseLen);
            break;
        case 2:
            obj->doColMoveReflectAndStopDisappear(diff, baseLen);
            break;
        case 0:
            obj->doColMoveReflect(diff, baseLen);
            break;
        case 3:
            obj->doColMoveReflectJumpDownAndDisappear(diff, baseLen);
            break;
        case 4:
        case 5:
        default:
            if (obj->mState != 8 && obj->mState != 7 && obj->mState != 10)
            {
                obj->setStateDisappear(true);   // via calling the inline block?
            }
            break;
        }
    }
}

void ItemObj::doColMoveReflect(JGeometry::TVec3f vec, f32 pow)
{
    if (mVel.x == 0.0f && mVel.z == 0.0f)
    {
        mVel += vec;
        mVel.z += 0.1f;
    }
    else
    {
        JGeometry::TVec3f refVec;
        if (ObjUtility::getReflectVec(vec, mVel, &refVec, false))
            mVel.set(refVec);
        else
            mVel.set(vec);
    }

    mVel *= pow * getItemBoundRatio();
}

void ItemObj::doColMoveReflectAndStopDisappear(JGeometry::TVec3f vec, f32 pow)
{
    bool stopped = false;
    if (mState != 8)
    {
        if (mVel.length() < 3.0f)
            stopped = true;
    }

    bool flag2 = false;
    if (stopped && (mTransferFlags & 2))
        flag2 = true;

    if (flag2)
    {
        if (mState != 8 && mState != 7 && mState != 10)
        {
            mStateFunc = &ItemObj::doDisappear;
            if (mItemKind == 7 || mItemKind == 0x15 || mItemKind == 0xc)
                GetItemObjMgr()->releaseItemByKartEquipItemList(this);
            GetItemObjMgr()->deleteItemByMoveList(this);
            mState = 8;
            mWaitFrame = 0;
            mNoColKartMax = 0;
            mNoColKartCnt = 0;
            mNoColKartClr = 0;
            mNoColKart = 0;
            if (mShadow != nullptr)
                mShadow->clrVisibleAll();
            initDisappear();
        }
        return;
    }

    JGeometry::TVec3f refVec;
    if (ObjUtility::getReflectVec(vec, mVel, &refVec, false))
        mVel.set(refVec);
    else
        mVel.set(vec);
    mVel *= pow * getItemBoundRatio();
}

void ItemObj::doColMoveJumpDown(JGeometry::TVec3f vec, f32 pow)
{
    doColMoveCommonJumpAndReflect(vec, pow, true);
}

void ItemObj::doColMoveReflectJumpDownAndDisappear(JGeometry::TVec3f vec, f32 pow)
{
    bool flag2 = false;
    bool stopped = flag2;
    if (mState != 8)
    {
        if (mVel.length() < 3.0f)
            stopped = true;
    }
    if (stopped && (mTransferFlags & 2))
        flag2 = true;
    if (flag2)
    {
        if (mState != 8 && mState != 7 && mState != 10)
        {
            mStateFunc = &ItemObj::doDisappear;
            if (mItemKind == 7 || mItemKind == 0x15 || mItemKind == 0xc)
                GetItemObjMgr()->releaseItemByKartEquipItemList(this);
            GetItemObjMgr()->deleteItemByMoveList(this);
            mState = 8;
            mWaitFrame = 0;
            mNoColKartMax = 0;
            mNoColKartCnt = 0;
            mNoColKartClr = 0;
            mNoColKart = 0;
            if (mShadow != nullptr)
                mShadow->clrVisibleAll();
            initDisappear();
        }
        return;
    }
    doColMoveCommonJumpAndReflect(vec, pow, false);
}

void ItemObj::doColMoveCommonJumpAndReflect(JGeometry::TVec3f vec, f32 pow, bool upForce)
{
    JGeometry::TVec3f jumpVec;
    jumpVec.zero();

    if (mVel.length() > 0.0f)
    {
        JGeometry::TVec3f nrm(vec);
        nrm.normalize();

        JGeometry::TVec3f vel(mVel);
        vel.normalize();

        f32 dot = vel.dot(nrm);
        jumpVec.scale(-2.0f * dot, nrm);
        jumpVec += vel;
        jumpVec.y = 0.0f;
        stVecNormalize(&jumpVec);
    }

    if (upForce)
        jumpVec.negate();

    stVecNormalize(&mVel);
    jumpVec *= 2.0f;
    mVel += jumpVec;
    stVecNormalize(&mVel);

    mVel.scale(pow * 0.4f);
    mVel.y += 32.0f;
}

MtxPtr ItemObj::getHandAnmMtx()
{
    DriverModel *model = RCMGetManager()->getKartLoader(mOwnerNum)->getExModelDriver(mDriverNum);
    return (MtxPtr)(void *)model->getMtxP_HandR();
}

MtxPtr ItemObj::getJuggleHandAnmMtx()
{
    DriverModel *model = RCMGetManager()->getKartLoader(mOwnerNum)->getExModelDriver(mDriverNum);
    return (MtxPtr)(void *)model->getMtxP_HandR2();
}

bool ItemObj::IsAlwaysColItemToKart()
{
    bool ret = false;
    if ((_12c & 0x1000) && mState != 4)
        ret = true;
    return ret;
}

void ItemObj::initMoveColGround()
{
    stMtx2Qt(&_34, mModel.getBaseTRMtx());
}

void ItemObj::doMoveColGround(Quaternion *quat, bool flagExp)
{
    mTransferFlags &= ~0x2;
    mTransferFlags &= ~0x4;

    JGeometry::TVec3f pos(mPos);
    JGeometry::TVec3f _10vec(_10);
    pos.y -= mHeightOffset * mDisScaleAnm.y;
    _10vec.y -= mHeightOffset * mDisScaleAnm.y;

    CrsGround gnd(RCMGetManager()->getCourse());
    gnd.search(pos, _10vec);
    mGroundHeight = gnd.getHeight();
    mTransferFlags |= 0x80;
    mGndAttr = gnd.getAttribute();

    if (!gnd.isItemInvalGround())
    {
        switch (mGndAttr)
        {
        case 0x2:
            moveColWall(gnd);
            break;
        case 0x5:
        case 0xa:
        case 0xd:
        case 0xe:
        case 0xf:
        case 0xff:
            moveRescue(gnd);
            break;
        case 0x9:
            moveCannon(gnd);
            break;
        default:
            moveCommonAttr(gnd, flagExp);
            break;
        }
    }

    CrsArea roofArea;
    roofArea.searchRoof(pos);
    if (roofArea.isInside())
    {
        JGeometry::TVec3f roofPos;
        roofArea.getRoofPosition(&roofPos);
        if (roofPos.y > gnd.getHeight() && pos.y + 100.0f > roofPos.y)
        {
            mVel.y = 0.0f;
            mVel.x *= 0.5f;
            mVel.z *= 0.5f;
            mPos.y = (roofPos.y - 100.0f) + mDisScaleAnm.y * mHeightOffset;
        }
    }

    if (mGndAttr == 0x12 && (mState == 4 || mState == 6) && mWaitFrame > 0xb4 && (mTransferFlags & 0x2) && mState != 0xa)
    {
        mStateFunc = &ItemObj::doForceDisappear;
        if (mState == 1)
            _12c |= 0x400;
        mState = 7;
        mWaitFrame = 0;
        mNoColKartMax = 0;
        mNoColKartCnt = 0;
        mNoColKartClr = 0;
        mNoColKart = 0;
        if (mShadow != nullptr)
            mShadow->clrVisibleAll();
        initForceDisappear();
    }

    Quaternion lerped;
    stQtLerp(&lerped, &_34, &_44, mLerpRatio);
    _34 = lerped;
    if (quat != nullptr)
        PSQUATMultiply(&lerped, quat, &lerped);
    setModelMatrixAndScale(lerped);
}

void ItemObj::moveCommonAttr(CrsGround &gnd, bool updVel)
{
    gnd.getNormal(&mNormal);
    JGeometry::TVec3f vel;
    if (updVel)
    {
        f32 yLevel = gnd.getHeight() + mHeightOffset * mDisScaleAnm.y;
        if (mPos.y > yLevel)
        {
            mTransferFlags |= 0x2;
            mTransferFlags |= 0x100;
            mPos.y = yLevel;

            mVel.scale(_28);

            gnd.getVelocity(&vel);

            if (!vel.isZero())
            {
                if (RCMGetCourse()->getCourseID() != MINI_5)
                    mVel.y *= -1.0f;
            }

            bool sameUp = mModel.mBaseTRMtx[0][1] == mNormal.x && mModel.mBaseTRMtx[1][1] == mNormal.y &&
                          mModel.mBaseTRMtx[2][1] == mNormal.z;
            if (!sameUp)
            {
                Mtx rotM;
                stQt2Mtx(rotM, &_34);
                JGeometry::TVec3f up(rotM[0][1], rotM[1][1], rotM[2][1]);
                stVec2QtUpdate(_34, _44, up, mNormal);
            }

            mVel.x = vel.x;
            mVel.z = vel.z;
        }
    }
    else
        moveNormal(gnd);
}

void ItemObj::moveColWall(CrsGround &gnd)
{
    mTransferFlags |= 0x4;
    JGeometry::TVec3f wallNorm;
    JGeometry::TVec3f wallPos;
    gnd.getWallNormal(&wallNorm, &wallPos);

    f32 velLen = mVel.length();
    JGeometry::TVec3f refDir;
    refDir.zero();
    if (velLen > 0.0f)
    {
        JGeometry::TVec3f nrm;
        JGeometry::setTVec3f(&wallNorm.x, &nrm.x);
        nrm.normalize();
        JGeometry::TVec3f vel;
        JGeometry::setTVec3f(&mVel.x, &vel.x);
        vel.normalize();
        vel.negate();

        f32 d = nrm.dot(vel);
        refDir.x = nrm.x * d + vel.x;
        refDir.y = nrm.y * d + vel.y;
        refDir.z = nrm.z * d + vel.z;
        refDir.y = 0.0f;
        stVecNormalize(&refDir);
    }

    SysDebug::checkInvalidVector((Vec *)&mVel, (char *)"WanWan_mVel");
    SysDebug::checkInvalidVector((Vec *)&wallNorm, (char *)"WanWan_wallnorm");
    SysDebug::checkInvalidVector((Vec *)&refDir, (char *)"WanWan_refDir");

    mVel = refDir;
    mVel.scale(velLen * _2c);

    mPos = wallPos;
    mPos.y += mDisScaleAnm.y * mHeightOffset;

    wallNorm.scale(5.0f);
    mPos += wallNorm;
}

void ItemObj::moveRescue(CrsGround &gnd)
{
    f32 yLimit = mHeightOffset * mDisScaleAnm.y + gnd.getHeight();
    if (mPos.y <= yLimit)
    {
        if (mState != 8 && mState != 7 && mState != 10)
        {
            mStateFunc = &ItemObj::doDisappear;
            if (mItemKind == 7 || mItemKind == 0x15 || mItemKind == 0xc)
                GetItemObjMgr()->releaseItemByKartEquipItemList(this);
            GetItemObjMgr()->deleteItemByMoveList(this);
            mState = 8;
            mWaitFrame = 0;
            mNoColKartMax = 0;
            mNoColKartCnt = 0;
            mNoColKartClr = 0;
            mNoColKart = 0;
            if (mShadow != nullptr)
                mShadow->clrVisibleAll();
            initDisappear();
        }
    }
    else
        moveNormal(gnd);
}

void ItemObj::moveCannon(CrsGround &gnd)
{
    if (mState != 8 && mState != 7 && mState != 10)
    {
        mStateFunc = &ItemObj::doDisappear;
        if (mItemKind == 7 || mItemKind == 0x15 || mItemKind == 0xc)
            GetItemObjMgr()->releaseItemByKartEquipItemList(this);
        GetItemObjMgr()->deleteItemByMoveList(this);
        mState = 8;
        mWaitFrame = 0;
        mNoColKartMax = 0;
        mNoColKartCnt = 0;
        mNoColKartClr = 0;
        mNoColKart = 0;
        if (mShadow != nullptr)
            mShadow->clrVisibleAll();
        initDisappear();
    }
}

void ItemObj::simpleDraw(u32 viewNo, f32 (*mtx)[4], u32 num)
{
    JGeometry::TVec3f sc;
    JGeometry::setTVec3f(&mDisScaleAnm.x, &sc.x);
    sc.scale(mDarkScale);
    JGeometry::setTVec3f(&sc.x, &mModel.mScale.x);
    mModel.simpleDraw(viewNo, mtx, num);
}

void ItemObj::setMultiType(ItemObj::cMultiType type)
{
    switch (type)
    {
    case cMultiType_0:
        mMulti = mJuggle;
        return;
    case cMultiType_1:
        mMulti = mBuild;
    }
}

ItemObj::cMultiType ItemObj::getCharaMultiType(int charID, u8 driverNum)
{
    u8 drv = driverNum;
    u8 drv2 = drv;
    cMultiType ret = cMultiType_0;
    ECharID charBit = RCMGetManager()->getKartInfo(charID)->getDriverCharID(drv2);
    switch (charBit)
    {
    case BABY_LUIGI:
    case KOOPATROOPA:
    case PEACH:
    case LUIGI:
    case WARIO:
    case BIRDO:
    case DIDDY_KONG:
    case BOWSER:
    case KING_BOO:
    case PETEY:
        ret = cMultiType_1;
        break;
    }
    return ret;
}

void ItemObj::doMoveGndParallel(const JGeometry::TVec3f &normVec, f32 pow)
{
    JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f &n = mNormal;
    f32 angle = up.angle(n);

    JGeometry::TVec3f hv(mVel);
    hv.y = 0.0f;
    stVecNormalize(&hv);

    JGeometry::TVec3f hn = n;
    hn.y = 0.0f;
    f32 normMag = PSVECMag(&hn);
    stVecNormalize(&hn);

    if (!hn.isZero())
    {
        angle *= (1.0f - normMag) * fabs(hv.dot(hn));
    }

    JGeometry::TVec3f axis;
    axis.cross(up, normVec);

    f32 ang2 = hv.angle(n);
    if (fabs(57.29578f * ang2) < 90.0f)
        angle = -angle;

    JGeometry::TPos3f mtx;
    mtx.setRotate(axis, angle);
    mtx.zeroTrans();
    PSMTXMultVec(mtx.mMtx, (Vec *)&normVec, (Vec *)&mVel);
    stVecNormalize(&mVel);

    f32 d = mMaxVel * mMaxVel - pow * pow;
    f32 scl = JMAFastSqrt(d);
    mVel.scale(scl);
    mVel.y += pow;
}

void ItemObj::initThrowVel(f32 dirScale, f32 upScale, f32 power)
{
    JGeometry::TVec3f kartVel;
    ObjUtility::getKartVel(mOwnerNum, &kartVel);
    kartVel.y *= 0.3f;
    mVel += kartVel;

    JGeometry::TVec3f mixDir;
    mixDir.zero();

    f32 stickY = KartCtrl::getKartCtrl()->GetItemStickY(mOwnerNum);
    if (stickY > 0.0f)
    {
        if (stickY < 0.8f)
            stickY *= 0.6f;

        JGeometry::TVec3f zdir;
        ObjUtility::getKartZdir(mOwnerNum, &zdir);
        JGeometry::TVec3f ydir;
        ObjUtility::getKartYdir(mOwnerNum, &ydir);

        mixDir = zdir * (stickY * dirScale) + ydir * (stickY * upScale);
    }

    JGeometry::TVec3f sideVec;
    sideVec.zero();

    u8 hasSide = 0;
    f32 stickX = KartCtrl::getKartCtrl()->GetItemStickX(mOwnerNum);
    if (fabs(stickX) > 0.1f)
    {
        JGeometry::TVec3f xdir;
        ObjUtility::getKartXdir(mOwnerNum, &xdir);
        hasSide = 1;
        sideVec.scale(-stickX * 35.0f, xdir);
    }

    mVel += mixDir;

    if (hasSide)
    {
        mVel += sideVec;
    }
    else
    {
        JGeometry::TVec3f xdir;
        ObjUtility::getKartXdir(mOwnerNum, &xdir);
        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mOwnerNum, &kartPos);

        JGeometry::TVec3f diff;
    diff.sub(mPos, kartPos);
        JGeometry::TVec3f mdir(diff);
        f32 mag = PSVECMag(&mdir);
        stVecNormalize(&mdir);

        JGeometry::TVec3f apart = mdir;
        float sl = (-power * mag) / (mVel.y * 2.0f);

        mVel.x += apart.x * sl;
        mVel.y += apart.y * sl;
        mVel.z += apart.z * sl;
    }

    if (PSVECMag(&mVel) > 300.0f)
    {
        stVecNormalize(&mVel);
        mVel.scale(300.0f);
    }
}

void ItemObj::setInitThrowDir(JGeometry::TVec3f *dir, f32 pow, bool useUpForce)
{
    ObjUtility::getKartZdir(mOwnerNum, dir);
    JGeometry::TVec3f ydir;
    ObjUtility::getKartYdir(mOwnerNum, &ydir);

    if (!useUpForce || dir->y <= 0.0f)
    {
        f32 p = pow;
        ydir.scale(p);
        *dir += ydir;
        stVecNormalize(dir);
    }
}

void ItemObj::setVelUpDown(f32 upAmount, f32 downAmount)
{
    if (mVel.length() > mMaxVel)
    {
        calcVelUpDown(downAmount);
        if (mVel.length() < mMaxVel)
            setVelMax();
    }
    else if (mVel.length() < mMaxVel)
    {
        calcVelUpDown(upAmount);
        if (mVel.length() > mMaxVel)
            setVelMax();
    }
}

void ItemObj::setVelMax()
{
    stVecNormalize(&mVel);
    mVel *= mMaxVel;
}

void ItemObj::calcVelUpDown(f32 amount)
{
    f32 velY = mVel.y;
    f32 len = mVel.length();
    mVel.y = 0.0f;
    stVecNormalize(&mVel);
    f32 lenSq = len * len;
    f32 ySq = velY * velY;
    f32 horiz = JMAFastSqrt(lenSq - ySq);
    mVel *= amount * horiz;
    mVel.y = velY;
}

void ItemObj::calcGndNormal(JGeometry::TVec3f *dir, CrsGround &gnd, f32 threshold, f32 pow)
{
    JGeometry::TVec3f add;
    JGeometry::TVec3f gndNormal;
    gnd.getNormal(&gndNormal);

    JGeometry::TVec3f cross;
    PSVECCrossProduct(dir, &gndNormal, &cross);
    f32 len = PSVECMag(&cross);
    f32 angle = fabs(JMAAtan2Radian(len, dir->dot(gndNormal)));

    if (fabs(57.29578f * angle) > threshold)
    {
        add.set(gndNormal);
        add.scale(pow);
        *dir += add;
        stVecNormalize(dir);
    }
}

void ItemObj::calcBananaFirstQuat(const JGeometry::TVec3f &dir)
{
    f32 angle = (getItemRnd()->get_ufloat_1() * 2.0f - 1.0f) * 3.14f;

    Quaternion spin;
    QuatRotY(angle, &spin);

    JGeometry::TVec3f negDir;
    JGeometry::negateInternal(&dir.x, &negDir.x);

    JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f xdir;
    xdir.cross(negDir, up);
    stVecNormalize(&xdir);

    JGeometry::TVec3f zdir;
    zdir.cross(xdir, negDir);
    stVecNormalize(&zdir);

    Mtx rotMtx;
    stMakeRMtx(rotMtx, xdir, negDir, zdir);
    stMtx2Qt(&_44, rotMtx);
    PSQUATMultiply(&_44, &spin, &_44);
}

void ItemObj::calcScale(f32 scale, f32 maxScale, f32 step)
{
    if (tstAnyObjectHitting() == false)
    {
        switch (mState)
        {
        case 1:
        case 5:
            doEquipScaleUp(scale);
            break;
        case 4:
        case 6:
            doFallScaling();
            setScaleCol(mEquipScale);
            break;
        case 2:
        case 3:
            if (mEquipScale < maxScale)
            {
                mEquipScale *= step;
                if (mEquipScale > maxScale)
                    mEquipScale = maxScale;
            }
            setScaleCol(mEquipScale);
            break;
        case 7:
            doForceDisappearScaleDown();
            break;
        }
    }
}

void ItemObj::calcAllUseInitVel(u8 type, f32 val)
{
    int count = mSuccessionParent->get_1fc();
    JGeometry::TVec3f ydir;
    ObjUtility::getKartYdir(mOwnerNum, &ydir);

    f32 half = 0.5f * (f32)(count - 1);
    f32 a = (-val) * half;
    f32 angle = 0.017453292f * ((f32)type * val + a);

    JGeometry::TPos3f mtx;
    mtx.setRotate(ydir, angle);
    mtx.zeroTrans();
    PSMTXMultVec(mtx.mMtx, (Vec *)&mVel, (Vec *)&mVel);
    _12c &= ~1;
}

void ItemObj::callHitItemSound(ItemObj *item)
{
}


void ItemObj::callHitKartSound(int kartIndex)
{
    KartBody *kartBody = KartCtrl::getKartCtrl()->getKartBody(kartIndex);
    if ((kartBody->mCarStatus & 0x40000) != 0)
    {
        if (_12c & 0x8000)
            mSoundMgr->setSe(0x10079);
    }
}

int ItemObj::getSpecialKind(const int &ref, const u8 &b)
{
    int kind = 16;
    ECharID id = RCMGetManager()->getKartInfo(ref)->getDriverCharID(b);
    switch (id)
    {
    case BABY_MARIO:
    case BABY_LUIGI:
        kind = 7;
        break;
    case PARATROOPA:
    case KOOPATROOPA:
        kind = 17;
        break;
    case PEACH:
    case DAISY:
        kind = 14;
        break;
    case MARIO:
    case LUIGI:
        kind = 21;
        break;
    case WARIO:
    case WALUIGI:
        kind = 8;
        break;
    case YOSHI:
    case BIRDO:
        kind = 11;
        break;
    case DONKEY_KONG:
    case DIDDY_KONG:
        kind = 4;
        break;
    case BOWSER:
    case BOWSER_JR:
        kind = 1;
        break;
    case TOAD:
    case TOADETTE:
        kind = 12;
        break;
    case KING_BOO:
    case PETEY:
        kind = 8;
        break;
    }
    return kind;
}

void ItemObj::drawSimpleModel(u32, f32 (*)[4], J3DUClipper *, f32 (*)[4]) {}

stRandom *ItemObj::getItemRnd()
{
    return stGetRnd(0);
}

void ItemObj::setHandOffsetData()
{
    ItemHandOffsetData data = GetItemObjMgr()->getHandOffsetData(this);
    setHandOffsetPos(&data);

    s16 ry = data._c[1];
    s16 rx = data._c[0];
    s16 rz = data._c[2];

    f32 cy = JMASCos(ry);
    f32 cz = JMASCos(rz);
    f32 sz = JMASSin(rz);
    f32 cx = JMASCos(rx);
    f32 sx = JMASSin(rx);
    f32 sy = JMASSin(ry);

    _210[0][0] = cy * cz;
    _210[1][0] = cy * sz;
    _210[2][0] = -sy;

    f32 cxsz = cx * sz;
    f32 sxcz = sx * cz;
    _210[0][1] = sxcz * sy - cxsz;

    f32 sxsz = sx * sz;
    f32 cxcz = cx * cz;
    _210[1][1] = cxcz + sxsz * sy;
    _210[2][1] = sx * cy;
    _210[0][2] = sxsz + cxcz * sy;
    _210[1][2] = cxsz * sy - sxcz;
    _210[2][2] = cx * cy;
}

void ItemObj::setHandOffsetPos(ItemHandOffsetData *data)
{
    mHandOffset = data->_0;
    mHandOffsetPos = mHandOffset;
}

const JGeometry::TVec3f &ItemObj::getHandOffsetPos()
{
    JGeometry::setTVec3f(&mHandOffset.x, &mHandOffsetPos.x);
    mHandOffsetPos *= mDarkScale;
    return mHandOffsetPos;
}

f32 ItemObj::getThunderScale()
{
#line 3592 "ItemObj.cpp"
    JUT_MINMAX_ASSERT(0, mOwnerNum, 8);
    f32 scale = 1.0f;
    if (GetKartCtrl()->CheckThunderBolt(mOwnerNum))
    {
        scale = GetKartCtrl()->getKartBody(mOwnerNum)->getThunder()->getScale();
    }
    return scale;
}

void ItemObj::initMaxDivestedFrame()
{
    if (mOrigOwnerNum == mOwnerNum)
        mDivestedMaxFrame = sTransferFrame;
    else
        mDivestedMaxFrame = sDivestedFrame;

    mDivestedGravity = -((f32)mDivestedMaxFrame) * sDivestedGravity;
}

inline void ItemObj::doDivested()
{
    moveDivested();
}
