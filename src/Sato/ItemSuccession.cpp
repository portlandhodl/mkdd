#include "Sato/ItemSuccession.h"

#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ItemObj.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/stMath.h"

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

void ItemBuild::ParamInit(const JGeometry::TVec3<s16> &rot)
{
    mCount = 0;
    _2d = 0;
    _18.z = 0.0f;
    _18.y = 0.0f;
    _18.x = 0.0f;
    mOffset.z = 0.0f;
    mOffset.y = 0.0f;
    mOffset.x = 0.0f;
    mHeight = 0.0f;
    mAngle = 0;
    mRadAngle = 0.0f;
    mFirstPos = 0;

    mObj->mpHandAnmMtx = mObj->getJuggleHandAnmMtx();

    _34 = 0;
    _35 = 0;
    _36 = 0;
    PSMTXIdentity(_38);

    if (mObj->mSuccessionItemLink.getList() != nullptr)
    {
        f32 cy = JMASCos(rot.y);
        f32 cz = JMASCos(rot.z);
        f32 sz = JMASSin(rot.z);
        f32 cx = JMASCos(rot.x);
        f32 sx = JMASSin(rot.x);
        f32 sy = JMASSin(rot.y);

        Mtx &mtx = mObj->_210;
        mtx[0][0] = cy * cz;
        mtx[1][0] = cy * sz;
        mtx[2][0] = -sy;
        mtx[0][1] = sx * sy * cz - cx * sz;
        mtx[1][1] = sx * sy * sz + cx * cz;
        mtx[2][1] = sx * cy;
        mtx[0][2] = cx * cz * sy + sx * sz;
        mtx[1][2] = cx * sz * sy - sx * cz;
        mtx[2][2] = cx * cy;

        f32 gravUp = mObj->mSuccessionParent->mJuggleGravityUp;
        f32 rnd = mObj->getItemRnd()->get_ufloat_1();
        s16 angle = (s16)((2.0f * rnd - 1.0f) * 32767.0f * gravUp);
        f32 s = JMASSin(angle);
        f32 c = JMASCos(angle);

        _38[1][1] = 1.0f;
        _38[0][0] = s;
        _38[2][0] = c;
        _38[0][2] = -c;
        _38[2][2] = s;
        _38[2][1] = 0.0f;
        _38[1][2] = 0.0f;
        _38[1][0] = 0.0f;
        _38[0][1] = 0.0f;
        _38[0][3] = 0.0f;
        _38[1][3] = 0.0f;
        _38[2][3] = 0.0f;
    }
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

void ItemBuild::setFirstPos(JGeometry::TVec3f *pos, f32 scale)
{
    setHeight(scale);
    if (!mFirstPos)
    {
        JGeometry::TVec3f vel;
        ObjUtility::getKartVel(mObj->mOwnerNum, &vel);
        vel.y = 0.0f;
        vel.scale(-0.5f);
        mOffset.z = 0.0f;
        mOffset.x = 15.0f * JMASSin(mAngle) * scale;
        mAngle += 1200;
        mObj->rotationRad(mRadAngle, 0.01f);

        Mtx mtx;
        PSMTXRotRad(mtx, 'Y', mRadAngle);
        PSMTXMultVec(mtx, &mOffset, &mOffset);

        mObj->mPos = *pos;
    }
    else
    {
        mObj->mPos = *pos;
    }

    mObj->mPos.y += mOffset.y;
}

void ItemBuild::setVel(f32 scale)
{
    JGeometry::TVec3f dir;
    dir.sub(mPrev->mOffset, mOffset);
    stVecNormalize(&dir);

    int num = mObj->mSuccessionParent->getSuccessionItemNum();
    f32 ratio = (f32)(num - mCount);
    bool sameNum = false;
    if (ratio == 0.0f)
        sameNum = true;
    if (RaceMgr::getManager()->getRaceMode() == BOMB_BATTLE)
    {
        if (mCount >= 2)
            ratio = 2.0f;
        else if (sameNum)
            ratio = 0.5f;
    }
    dir.scale(0.4f * ratio);
    _18.add(dir);

    f32 mag = PSVECMag((Vec *)&_18);
    f32 limit = (f32)mCount;
    if (RaceMgr::getManager()->getRaceMode() == BOMB_BATTLE)
    {
        if (mCount == 1 && sameNum)
            limit = 0.5f;
        else if (mCount > 2 && limit > 2.0f)
            limit = 2.0f;
    }
    f32 max = 3.0f * limit * scale;
    if (mag > max)
    {
        stVecNormalize((JGeometry::TVec3f *)&_18);
        _18 *= max;
    }
}

void ItemBuild::setPos(JGeometry::TVec3f *pos, f32 scale)
{
    f32 s = scale;
    JGeometry::TVec3f inc(_18);
    inc.scale(s);
    mOffset.add(inc);
    mOffset.y = 0.0f;
    setHeight(s);

    mObj->mPos.add(*pos, mOffset);
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

void ItemBuild::setRotate(Mtx mtx)
{
    Mtx rmtx;
    JGeometry::TVec3f right;
    JGeometry::TVec3f up;
    JGeometry::TVec3f zdir;

    ObjUtility::getKartZdir(mObj->mOwnerNum, &zdir);
    up.sub(mOffset, mPrev->mOffset);
    stVecNormalize(&up);
    PSVECCrossProduct(&up, &zdir, &right);
    stVecNormalize(&right);
    PSVECCrossProduct(&right, &up, &zdir);
    stVecNormalize(&zdir);
    stMakeRMtx(rmtx, right, up, zdir);
    if (mtx != nullptr)
        PSMTXConcat(rmtx, mtx, rmtx);
    PSMTXConcat(rmtx, _38, rmtx);
    mObj->setModelMatrixAndScale(rmtx);
}

void ItemBuild::setFirstRotate(Mtx mtx)
{
    if (mFirstPos && RaceMgr::getManager()->getRaceMode() != BOMB_BATTLE)
    {
        Mtx work;
        if (mtx != nullptr)
            PSMTXConcat(mObj->mpHandAnmMtx, mtx, work);
        else
            PSMTXCopy(mObj->mpHandAnmMtx, work);
        PSMTXConcat(work, _38, work);
        mObj->setModelMatrixAndScale(work);
        return;
    }

    Mtx rmtx;
    JGeometry::TVec3f right;
    JGeometry::TVec3f up;
    JGeometry::TVec3f zdir;

    ObjUtility::getKartZdir(mObj->mOwnerNum, &zdir);
    JGeometry::setTVec3f(&mOffset.x, &up.x);
    up.x += 90.0f;
    up.y = zdir.y + 90.0f;
    stVecNormalize(&up);
    PSVECCrossProduct(&up, &zdir, &right);
    stVecNormalize(&right);
    PSVECCrossProduct(&right, &up, &zdir);
    stVecNormalize(&zdir);
    stMakeRMtx(rmtx, right, up, zdir);
    if (mtx != nullptr)
        PSMTXConcat(rmtx, mtx, rmtx);
    PSMTXConcat(rmtx, _38, rmtx);
    mObj->setModelMatrixAndScale(rmtx);
}

void ItemBuild::InitMoveDivested()
{
    JGeometry::TVec3f pos;
    pos.sub(mObj->mPos, mOffset);
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mObj->mOwnerNum, &kartPos);
    mObj->mDivestedPos.sub(pos, kartPos);
    mObj->mpHandAnmMtx = mObj->getJuggleHandAnmMtx();
    stMtx2Qt(&mObj->_44, mObj->mModel.getBaseTRMtx());
    stMtx2Qt(&mObj->_1d8, mObj->mpHandAnmMtx);
    mObj->initMaxDivestedFrame();
}

void ItemBuild::MoveDivested()
{
    f32 ratio = (f32)mObj->mWaitFrame / (f32)mObj->mDivestedMaxFrame;

    if (mObj->mSuccessionItemLink.getList() == nullptr)
    {
        mObj->doHandOneEquip(true, ratio);
    }
    else
    {
        JGeometry::TVec3f pos;
        PSMTXMultVec(mObj->mpHandAnmMtx, &mObj->mHandOffset, &pos);

        JGeometry::TVec3f equipPos;
        mObj->doPosLerp(&equipPos, pos, ratio);

        f32 scale = 1.0f;
        if (mObj->mState == 1)
        {
            if (mObj->getDriverNum() != GetItemObjMgr()->getNowTandemDriverNum(mObj->mOwnerNum))
                scale = ItemObj::sFrontEquipScaleRatio;
        }
        scale *= mObj->getThunderScale();

        MtxPtr mtx = mObj->_210;
        if (mCount)
        {
            setVel(scale);
            setPos(&equipPos, scale);
            setRotate(mtx);
        }
        else
        {
            setFirstPos(&equipPos, scale);
            setFirstRotate(mtx);
        }
    }

    if (mObj->mWaitFrame >= mObj->mDivestedMaxFrame)
        mObj->setStateEquip(false, false);
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
