#include "Sato/ItemSuccession.h"

#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ItemObj.h"
#include "Sato/ItemObjMgr.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

namespace
{
    f32 sDivestedFrameSyncCheckFrame = 0.05f;

    struct SucFuncSet
    {
        ItemObjSuc::EquipFunc init;
        ItemObjSuc::EquipFunc doFunc;
        ItemObjSuc::SelectFunc select;
    };

    const SucFuncSet sEquipFuncTable[2] = {
        {&ItemObjSuc::initJuggleEquip, &ItemObjSuc::doJuggleEquip, &ItemObjSuc::selectJuggleSucChild},
        {&ItemObjSuc::initBuildEquip, &ItemObjSuc::doBuildEquip, &ItemObjSuc::selectBuildSucChild},
    };
}

f32 ItemJuggle::sJuggleRotStartOffset = 0.02f;
f32 ItemJuggle::sJuggleRotRatio = 0.88f;
f32 ItemJuggle::sJuggleRotRatioSub = 0.05f;

f32 ItemObjSuc::sJuggleXLength = 35.0f;
f32 ItemObjSuc::sJuggleRndAngle = 1.0f;
f32 ItemObjSuc::sJuggleRndAngle2 = 0.1f;
f32 ItemObjSuc::sJuggleRotAxisRatio = 0.8f;
f32 ItemObjSuc::sJuggleGravity = -0.45f;
f32 ItemObjSuc::sJuggleGravityUpRatio = 2.1f;
f32 ItemObjSuc::sJuggleAnmOverFrame = 16.5f;

void ItemJuggle::DoEquipScaleUp(f32 scale)
{
    ItemObj *obj = mObj;
    int frame = _8;
    int end = _1c;
    if (frame < end)
    {
        obj->mEquipScale = 0.2f;
    }
    else if (obj->mEquipScale < scale)
    {
        obj->mEquipScale = (scale * (f32)(obj->_2a8 - frame)) / 20.0f;
    }
    else
    {
        obj->mEquipScale = scale;
    }
}

void ItemJuggle::ParamInit(const JGeometry::TVec3<s16> &)
{
    _8 = 0;
    _c = 0;
    _18 = 0.0f;
    _d = 0;
    _10 = 0;
    _14 = 0;
    _1c = 0;
    _20.z = 0.0f;
    _20.y = 0.0f;
    _20.x = 0.0f;
    _2c.x = 0.0f;
    _2c.y = 0.0f;
    _2c.z = 0.0f;
    _38 = 1;
    _3c.z = 0.0f;
    _3c.y = 0.0f;
    _3c.x = 0.0f;
    _48.z = 0.0f;
    _48.y = 0.0f;
    _48.x = 0.0f;
    _54.z = 0.0f;
    _54.y = 0.0f;
    _54.x = 0.0f;
    mObj->mpHandAnmMtx = mObj->getHandAnmMtx();
}

__declspec(weak) f32 ItemObjSuc::getRotAxisRatio()
{
    return sJuggleRotAxisRatio;
}

void ItemBuild::DoEquipScaleUp(f32 scale)
{
    mObj->mEquipScale = scale * mObj->getThunderScale();
}

void ItemBuild::MoveEquip()
{
    JGeometry::TVec3f pos;
    u32 driver;
    MtxPtr mtx = mObj->_210;
    PSMTXMultVec(mObj->mpHandAnmMtx, &mObj->mHandOffset, &pos);

    if (mObj->mSuccessionItemLink.getList() != nullptr && !_35)
    {
        f32 scale = 1.0f;
        if (mObj->mState == 1)
        {
            driver = mObj->getDriverNum();
            if (driver != (u32)GetItemObjMgr()->getNowTandemDriverNum(mObj->mOwnerNum))
                scale = ItemObj::sFrontEquipScaleRatio;
        }
        scale *= mObj->getThunderScale();
        if (mCount)
        {
            setVel(scale);
            setPos(&pos, scale);
            setRotate(mtx);
        }
        else
        {
            setFirstPos(&pos, scale);
            setFirstRotate(mtx);
        }
    }
    else
    {
        mObj->doHandOneEquip(false, 0.0f);
    }

    if (_36 && !mCount)
        mObj->mpHandAnmMtx = mObj->getHandAnmMtx();
    else
        mObj->mpHandAnmMtx = mObj->getJuggleHandAnmMtx();
}

void ItemBuild::setHeight(f32 height)
{
    if (!_34)
    {
        mHeight = 60.0f * (f32)mCount * height;
        _34 = 1;
    }
    if (mObj->mTransferFlags & 0x20)
    {
        f32 off = -5.0f;
        if (mObj->getKind() == 5)
            off = -3.5f;
        mHeight += off;
    }
    f32 limit = 46.0f * (f32)mCount * height;
    if (mHeight <= limit)
    {
        mHeight = limit;
        _36 = 1;
    }
    else
    {
        _36 = 0;
    }
    mOffset.y = mHeight;
}

void ItemObjSuc::reset()
{
    ItemObj::reset();
    mJuggleXLength = 0.0f;
    mJuggleGravity = 0.0f;
    mJuggleIndex = 0;
    mJuggleFrameStep = 0;
    mJuggleRot = 0.0f;
    mJuggleFirst = 1;
    mJuggleFinished = 0;
    mJuggleGravityUp = 1.0f;
    mUseHandAnm = 0;
}

void ItemObjSuc::initEquip()
{
    (this->*mEquipInitFunc)();
}

void ItemObjSuc::doEquip()
{
    (this->*mEquipDoFunc)();
}

void ItemObjSuc::initJuggleEquip()
{
    int owner = mOwnerNum;
    int driver = mDriverNum;
    KartInfo *info = RaceMgr::getManager()->getKartInfo(owner);
    info->getDriverCharID(driver);

    u8 num = _1fc;
    mJuggleFrameStep = (50 - (num - 1) * 8) / num;
    mJuggleXLength = sJuggleXLength;
    mJuggleGravity = sJuggleGravity;
    mJuggleIndex = num - 1;
    int step = (int)(58.0f / (f32)num);
    int frame = 8;
    for (JSULink<ItemObj> *link = _1a4.getFirst(); link != nullptr; link = link->getNext())
    {
        ItemJuggle *juggle = link->getObject()->mJuggle;
        juggle->_1c = frame;
        juggle->_d = 50;
        juggle->_14 = 8;
        juggle->_10 = 0;
        juggle->_2c.x = mJuggleXLength;
        juggle->_2c.y = (f32)mJuggleIndex;
        juggle->_2c.z = mJuggleGravity;
        juggle->_18 = sJuggleAnmOverFrame;
        frame += step;
    }
    mJuggleFinished = 0;
    mPrevSucNum = num;
}

void ItemObjSuc::doJuggleEquip()
{
    int num = getSuccessionItemNum();
    if (mPrevSucNum == num)
        return;

    mJuggleXLength = sJuggleXLength * ((f32)num / (f32)_1fc);
    mJuggleGravity *= sJuggleGravityUpRatio;
    mJuggleIndex = num - 1;
    if (mJuggleIndex == 0)
        mJuggleFinished = 1;

    ItemJuggle *juggle;
    JSULink<ItemObj> *link = _1a4.getFirst();
    while (link != nullptr)
    {
        ItemObj *child = link->getObject();
        juggle = child->mJuggle;
        juggle->_10 = (num - 1) * juggle->_14 + num * mJuggleFrameStep;
        link = link->getNext();
    }
    mPrevSucNum = num;
}

void ItemObjSuc::setInitBuildAppend()
{
    u32 prev = mPrevSucNum;
    initBuildEquip();
    mPrevSucNum = prev;
}

void ItemObjSuc::initBuildEquip()
{
    int ten;
    int idx;
    ItemBuild *prev;
    idx = 0;
    ten = 0;
    prev = nullptr;
    int num = getSuccessionItemNum();
    mJuggleFinished = (num == 1);

    ItemBuild *build;
    JSULink<ItemObj> *link;
    link = _1a4.getFirst();
    while (link != nullptr)
    {
        build = link->getObject()->mBuild;
        build->mCount = idx;
        build->mPrev = prev;
        prev = build;
        build->mFirstPos = mJuggleFinished;
        if (mUseHandAnm)
            build->_2d = 10;
        else
            build->_2d = ten;
        link = link->getNext();
        idx++;
        ten += 10;
    }
    mPrevSucNum = getSuccessionItemNum();
}

void ItemObjSuc::doBuildEquip()
{
    int num = getSuccessionItemNum();
    if (mPrevSucNum == num)
        return;

    JSULink<ItemObj> *link = _1a4.getFirst();
    int idx = 0;
    while (link != nullptr)
    {
        link->getObject()->mBuild->mCount = idx;
        idx++;
        link = link->getNext();
    }

    if (num < mPrevSucNum)
    {
        if (num == 1)
        {
            _1a4.getFirst()->getObject()->mBuild->mFirstPos = 1;
            if (RaceMgr::getManager()->getRaceMode() != BOMB_BATTLE)
                _1a4.getFirst()->getObject()->setHandOffsetData();
        }
    }
    else
    {
        if (mPrevSucNum == 1)
            _1a4.getFirst()->getObject()->mBuild->mFirstPos = 0;
    }
    mPrevSucNum = num;
}

JSUListIterator<ItemObj> ItemObjSuc::selectSucChild()
{
    return (this->*mSelectSucFunc)();
}

void ItemObjSuc::setSucMultiType(cMultiType type)
{
    mSucMultiType = type;
    const SucFuncSet *funcs = sEquipFuncTable;
    switch (mSucMultiType)
    {
    case cMultiType_0:
        mEquipInitFunc = funcs[0].init;
        mEquipDoFunc = funcs[0].doFunc;
        mSelectSucFunc = funcs[0].select;
        return;
    case cMultiType_1:
        mEquipInitFunc = funcs[1].init;
        mEquipDoFunc = funcs[1].doFunc;
        mSelectSucFunc = funcs[1].select;
        return;
    }
}

__declspec(weak) JSUListIterator<ItemObj> ItemObjSuc::selectBuildSucChild()
{
    return _1a4.getFirst();
}

void ItemObjSuc::initForceDisappear() {}

void ItemObjSuc::doForceDisappear()
{
    doEquip();
    if (mWaitFrame > 0x50)
    {
        if (disappearAllChild(false))
        {
            mState = 10;
            initEnd();
        }
    }
}
