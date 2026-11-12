#include "JSystem/JMath/JMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/EffectScreen.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/ItemThunder.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCtrl.h"

static const f32 sDummy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

namespace
{
    f32 sScale = 1.0f;
    f32 sHeightOffset = 26.0f;
}

JGeometry::TVec3f ItemThunder::sHandOffsetPos;
namespace { struct HandInit { HandInit() { ItemThunder::sHandOffsetPos.set(16.0f, 5.0f, -7.0f); } }; HandInit sHandInit; }
JSUList<GeoObjSupervisor> ItemThunder::sSuperVisorList(true);

ItemThunder::ItemThunder(u32 kind) : ItemObj(kind), JKRDisposer()
{
    for (int i = 0; i < 8; i++)
    {
        for (u8 j = 0; j < 4; j++)
        {
            mEffectThunder[i][j] = nullptr;
        }
    }
    reset();
}

ItemThunder::~ItemThunder()
{
    sSuperVisorList.initiate();
}

void ItemThunder::entrySuperVisor(GeoObjSupervisor *supervisor)
{
    sSuperVisorList.append(&supervisor->mLink2);
}

void ItemThunder::reset()
{
    resetObject();
    mKartReaction = 12;
    _12c |= 8;
    _28 = 0.5f;
    _6c = sHeightOffset;
    _2c = 0.5f;
    mLerpRatio = 0.3f;
    mRot = 0.0f;
    _12c |= 0x200;
    mDisappearKartNo = -1;

    for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
    {
        for (u8 j = 0; j < 4; j++)
        {
            mEffectThunder[i][j] = nullptr;
        }
    }
}

const char *ItemThunder::getBmdFileName()
{
    static const char *cThunderBmdName = "/Item/item_thunder.bmd";
    return cThunderBmdName;
}

void ItemThunder::createModel(JKRSolidHeap *heap, u32 p2, u32 p3)
{
    mModel.createDifferedModel(heap, p2, p3 | 0x200, false);
}

void ItemThunder::doUniv()
{
    if (!tstAnyObjectHitting())
    {
        switch (mState)
        {
        case 1:
        case 5:
            doEquipScaleUp(sScale);
            break;
        case 4:
        case 6:
            doFallScaling();
            setScaleCol(mEquipScale);
            break;
        case 7:
            doForceDisappearScaleDown();
            break;
        }
    }

    (this->*mStateFunc)();

    if (mState == 4 && mWaitFrame > 600)
    {
        setStateDisappear(true);
    }
}

void ItemThunder::initEquip()
{
    mEquipFlags = 0;
    mpHandAnmMtx = getHandAnmMtx();
    mVel.zero();
    mPos.zero();
}

void ItemThunder::doEquip()
{
    doHandOneEquip(false, 0.0f);
}

void ItemThunder::initRelease()
{
    _1ec = 3;
    mDisappearKartNo = mOwnerNum;
    setStateDisappear(true);
}

void ItemThunder::doRelease() {}

void ItemThunder::initDisappear()
{
    _1ec = 3;
    _12c &= ~1;
    mEquipFlags = 0;
    _12c &= ~2;

    if (mDisappearKartNo >= 0)
    {
        if ((GetKartCtrl()->getKartBody(mDisappearKartNo)->mCarStatus & 0x400000) == 0)
        {
            setEffectThunder(mDisappearKartNo);
            EffectScreenMgr::ThisMgr->flashThunder(mOwnerNum);

            for (JSUListIterator<GeoObjSupervisor> it(sSuperVisorList.getFirst()); it != sSuperVisorList.getEnd(); it++)
            {
                it.getObject()->thunderDownAll();
            }
        }
        _12c |= 0x80;
    }
    else
    {
        mState = 10;
        ItemObjMgr::gItemMgr->setThunderDisableUseCounter();
    }
}

void ItemThunder::doDisappear()
{
    if (mDisappearKartNo >= 0)
    {
        if (doEffectThunder())
        {
            mState = 10;
            ItemObjMgr::gItemMgr->setThunderDisableUseCounter();
        }
    }
}

void ItemThunder::initFall()
{
    mEquipFlags = 0xffffffff;
    initmoveFall();
}

void ItemThunder::doFall()
{
    Quaternion rot;
    Quaternion out;
    rotationRad(mRot, 0.05f);
    QuatRotY(mRot, &rot);
    moveFall();
    PSQUATMultiply(&_34, &rot, &out);
    Quaternion m = out;
    setModelMatrixAndScale(m);
}

void ItemThunder::initForceDisappear()
{
    deleteEffectThunder();
    ItemObj::initForceDisappear();
}

const bool ItemThunder::checkKartColDisappear(int kartNo)
{
    mEquipFlags = 0;
    mDisappearKartNo = kartNo;
    return true;
}

void ItemThunder::setEffectThunder(int exceptKartNo)
{
    for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
    {
        if (i == exceptKartNo)
            continue;

        JGeometry::TVec3f pos;
        ObjUtility::getKartPos(i, &pos);
        mEffectThunder[i][0] = GetJPAMgr()->createEmtThunder("mk_thunder_a", pos);
        mEffectThunder[i][1] = GetJPAMgr()->createEmtThunder("mk_thunder_b", pos);
        mEffectThunder[i][2] = GetJPAMgr()->createEmtThunder("mk_thunder_c", pos);
        mEffectThunder[i][3] = GetJPAMgr()->createEmtThunder("mk_thunder_d", pos);
    }
}

void ItemThunder::deleteEffectThunder()
{
    for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
    {
        for (u8 j = 0; j < 4; j++)
        {
            if (mEffectThunder[i][j] != nullptr)
            {
                GetJPAMgr()->deleteEmtThunder(mEffectThunder[i][j]);
                mEffectThunder[i][j] = nullptr;
            }
        }
    }
}

u8 ItemThunder::doEffectThunder()
{
    int limit = RCMGetManager()->getKartNumber() * 4;
    int nullNum = 0;
    int ret = 0;

    for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
    {
        JGeometry::TVec3f pos;
        ObjUtility::getKartPos(i, &pos);

        for (u8 j = 0; j < 4; j++)
        {
            if (mEffectThunder[i][j] != nullptr)
            {
                if (mWaitFrame > 5 && j == 3 && !GetKartCtrl()->CheckThunderBolt(i))
                {
                    JPABaseEmitter *emitter = mEffectThunder[i][j];
                    emitter->setStatus(JPAEMIT_StopEmitting);
                    emitter->mMaxFrame = 1;
                }

                u8 dead = false;
                JPABaseEmitter *emt = mEffectThunder[i][j];
                if (emt->checkStatus(JPAEMIT_EnableDeleteEmitter))
                {
                    if (emt->mAlivePtclBase.getNum() + emt->mAlivePtclChld.getNum() == 0)
                        dead = true;
                }

                if (dead)
                    mEffectThunder[i][j] = nullptr;
                else
                    emt->mGlobalTrs.set(pos);
            }
            else
            {
                nullNum++;
            }
        }
    }

    if (nullNum == limit)
        ret = true;
    return ret;
}

#include "JSystem/JAudio/JASFakeMatch2.h"
