#include "Sato/ItemSuccession.h"

#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/KartLoader.h"
#include "Kaneshige/RaceMgr.h"
#include "Kawano/driver.h"
#include "Kawano/driverMgr.h"
#include "Sato/ItemObj.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/stMath.h"
#include "Yamamoto/kartCtrl.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

inline void setIdentQuat(Quaternion &q)
{
    Quaternion id = {0.0f, 0.0f, 0.0f, 1.0f};
    q = id;
}

f32 ItemJuggle::sJuggleRotStartOffset = 0.02f;
f32 ItemJuggle::sJuggleRotRatio = 0.88f;
f32 ItemJuggle::sJuggleRotRatioSub = 0.05f;

namespace
{
    f32 sDivestedFrameSyncCheckFrame = 0.05f;
}

f32 ItemObjSuc::sJuggleXLength = 35.0f;
f32 ItemObjSuc::sJuggleRndAngle = 1.0f;
f32 ItemObjSuc::sJuggleRndAngle2 = 0.1f;
f32 ItemObjSuc::sJuggleRotAxisRatio = 0.8f;
f32 ItemObjSuc::sJuggleGravity = -0.45f;
f32 ItemObjSuc::sJuggleGravityUpRatio = 2.1f;
f32 ItemObjSuc::sJuggleAnmOverFrame = 16.5f;

static inline J3DAnmTransform *getKeepAnmTrans(DriverModel *driver, int anmID)
{
    int keepID = DriverLoader::convAnmIDToKeepID((DriverLoader::AnmEnum)anmID);
    if (keepID >= 0)
        return driver->mKeepAnmTrans[keepID];
    return nullptr;
}

static inline s16 getKeepAnmFrameMax(DriverModel *driver, int anmID)
{
    if (getKeepAnmTrans(driver, anmID) != nullptr)
        return getKeepAnmTrans(driver, anmID)->getFrameMax();
    return 0;
}

static inline f32 getJuggleAnmFrame(int kartNo, int driverNo)
{
    DriverModel *driver = RCMGetManager()->getKartLoader(kartNo)->getExModelDriver(driverNo);
    f32 cur = driver->m_668.mCurrentValue;
    int frame = (int)(cur * (f32)getKeepAnmFrameMax(driver, 80) + 0.5f);
    if (frame >= getKeepAnmFrameMax(driver, 80))
        return 0.0f;
    return (f32)frame;
}

inline f32 getEquipScale(ItemMultiBase *multi)
{
    f32 scale = 1.0f;
    if (multi->mObj->mState == 1)
    {
        if (multi->mObj->getDriverNum() != GetItemObjMgr()->getNowTandemDriverNum(multi->mObj->mOwnerNum))
            scale = ItemObj::sFrontEquipScaleRatio;
    }
    scale *= multi->mObj->getThunderScale();
    return scale;
}

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

void ItemJuggle::InitMoveDivested()
{
    JGeometry::TVec3f kartPos;
    JGeometry::TVec3f pos;
    pos.sub(mObj->mPos, _48);
    ObjUtility::getKartPos(mObj->mOwnerNum, &kartPos);
    pos.sub(kartPos);
    mObj->mDivestedPos = pos;
    mObj->mpHandAnmMtx = mObj->getJuggleHandAnmMtx();
    stMtx2Qt(&mObj->_44, mObj->mModel.getBaseTRMtx());
    stMtx2Qt(&mObj->_1d8, mObj->mpHandAnmMtx);
    mObj->initMaxDivestedFrame();
    ObjUtility::getKartXdir(mObj->mOrigOwnerNum, &_54);
    _54.scale(-1.0f);
    _54.y = 0.0f;
    stVecNormalize(_54);
}

void ItemJuggle::MoveDivested()
{
    f32 ratio = (f32)mObj->mWaitFrame / (f32)mObj->mDivestedMaxFrame;

    if (mObj->mSuccessionItemLink.getList() != nullptr)
    {
        JGeometry::TVec3f handOffset(mObj->getHandOffsetPos());
        stMtx2Qt(&mObj->_44, mObj->mpHandAnmMtx);
        Quaternion q;
        stQtLerp(&q, &mObj->_1d8, &mObj->_44, ratio);
        Mtx mtx;
        stQt2Mtx(mtx, &q);
        JGeometry::TVec3f pos;
        PSMTXMultVec(mObj->mpHandAnmMtx, &handOffset, &pos);
        JGeometry::TVec3f equipPos;
        mObj->doPosLerp(&equipPos, pos, ratio);
        handOffset.scale(mObj->mEquipScale);

        if (_8 >= _1c)
        {
            switch (_38)
            {
            case 0:
            {
                JGeometry::TVec3f xdir;
                ObjUtility::getKartXdir(mObj->mOwnerNum, &xdir);
                xdir.scale(-1.0f);
                xdir.y = 0.0f;
                stVecNormalize(xdir);
                JGeometry::TVec3f dir;
                JMAVECLerp(&_54, &xdir, &dir, ratio);
                doMoveJuggleEquip(equipPos, mtx, dir, getEquipScale(this));
                break;
            }
            case 1:
                doHandJuggleEquip(equipPos, mtx);
                break;
            }
        }
        else
        {
            _3c.z = 0.0f;
            _3c.y = 0.0f;
            _3c.x = 0.0f;
            _48.z = 0.0f;
            _48.y = 0.0f;
            _48.x = 0.0f;
            mObj->mPos = equipPos;
            mObj->setModelMatrixAndScale(mObj->mpHandAnmMtx);
            if (mObj->mSuccessionParent->IsAvailableDivested())
                _8++;
        }
    }
    else
    {
        mObj->doHandOneEquip(true, ratio);
    }

    if (mObj->mWaitFrame >= mObj->mDivestedMaxFrame)
    {
#line 217
        JUT_ASSERT_F(mObj->getSuccessionParent()->IsAvailableDivested(), "%s", "mItem->getSuccessionParent()->IsAvailableDivested()");
        mObj->setStateEquip(false, false);
    }
}

void ItemJuggle::MoveEquip()
{
    JGeometry::TVec3f pos;
    PSMTXMultVec(mObj->mpHandAnmMtx, &mObj->mHandOffset, &pos);

    if (mObj->mSuccessionItemLink.getList() != nullptr)
    {
        if (_8 >= _1c)
        {
            switch (_38)
            {
            case 0:
            {
                JGeometry::TVec3f xdir;
                ObjUtility::getKartXdir(mObj->mOwnerNum, &xdir);
                xdir.scale(-1.0f);
                xdir.y = 0.0f;
                stVecNormalize(xdir);
                doMoveJuggleEquip(pos, mObj->mpHandAnmMtx, xdir, getEquipScale(this));
                break;
            }
            case 1:
                doHandJuggleEquip(pos, mObj->mpHandAnmMtx);
                break;
            }
        }
        else
        {
            _3c.z = 0.0f;
            _3c.y = 0.0f;
            _3c.x = 0.0f;
            _48.z = 0.0f;
            _48.y = 0.0f;
            _48.x = 0.0f;
            mObj->mPos = pos;
            mObj->setModelMatrixAndScale(mObj->mpHandAnmMtx);
            _8++;
        }
    }
    else
    {
        mObj->doHandOneEquip(false, 0.0f);
    }
}

static inline void setJuggleParam(ItemJuggle *j, u8 d, int n, ItemObjSuc *p)
{
    j->_d = d;
    j->_2c.x = p->mJuggleXLength;
    j->_2c.y = (f32)p->mJuggleIndex;
    j->_10 = n;
    j->_2c.z = p->mJuggleGravity;
}

void ItemJuggle::doHandJuggleEquip(const JGeometry::TVec3f &pos, Mtx mtx)
{
    Mtx rmtx;
    stQt2Mtx(rmtx, &mObj->_34);
    if (!mObj->mSuccessionParent->mJuggleFinished)
    {
        if (mObj->mSuccessionParent->mJuggleFirst)
            _c++;
        if (_c >= _14 - 2)
        {
            if (getJuggleAnmFrame(mObj->mOwnerNum, mObj->mDriverNum) > _18)
            {
                if (_10 > 0)
                {
                    setJuggleParam(this, _10, 0, mObj->mSuccessionParent);
                }
                mObj->mpHandAnmMtx = mObj->getJuggleHandAnmMtx();
                _38 = 0;
                f32 angle2 = ItemObjSuc::sJuggleRndAngle2;
                f32 angle = ItemObjSuc::sJuggleRndAngle;
                mObj->getItemRnd()->getArbitUnitVec(_20, angle, angle2);
                PSMTXMultVec(rmtx, &_20, &_20);
                _c = 0;
                mObj->mVel.y = 0.5f * (-(f32)(_d + 1) * _2c.z);
            }
        }
    }

    mObj->mPos = pos;
    Mtx m;
    PSMTXCopy(mtx, m);
    m[2][3] = 0.0f;
    m[1][3] = 0.0f;
    m[0][3] = 0.0f;
    PSMTXConcat(m, rmtx, m);
    mObj->setModelMatrixAndScale(m);
}

void ItemJuggle::doMoveJuggleEquip(const JGeometry::TVec3f &pos, Mtx mtx, JGeometry::TVec3f dir, f32 scale)
{
    if (mObj->mSuccessionParent->mJuggleFirst)
        _c++;

    f32 ratio = sJuggleRotRatio;
    f32 offset = sJuggleRotStartOffset;
    switch (mObj->mSuccessionParent->getSuccessionItemNum())
    {
    case 2:
        ratio -= sJuggleRotRatioSub;
        break;
    case 1:
        ratio = 1.0f;
        offset = 0.0f;
        break;
    }
    ratio = ratio * ((f32)_c / (f32)_d) + offset;

    int kartNo = mObj->mOwnerNum;
    u8 driverNo = mObj->getDriverNum();
    f32 xLength = _2c.x;
    if (driverNo == ItemObjMgr::getNowTandemDriverNum(kartNo))
    {
        if (GetKartCtrl()->GetTandemDir(kartNo) == 1)
            xLength = -_2c.x;
    }

    f32 sinScale = xLength * JMASSin((s16)(65535.0f * ratio));
    JGeometry::TVec3f v(dir);
    v.scale(sinScale);
    _3c.x = v.x;
    _3c.z = v.z;
    if (mObj->mSuccessionParent->mJuggleFirst)
        mObj->mVel.y += _2c.z;
    _3c.y += mObj->mVel.y;

    _48 = _3c;
    _48.scale(scale);
    mObj->mPos = pos;
    mObj->mPos.add(_48);

    Mtx m;
    PSMTXCopy(mtx, m);
    m[2][3] = 0.0f;
    m[1][3] = 0.0f;
    m[0][3] = 0.0f;

    Mtx rot;
    PSMTXRotAxisRad(rot, &_20, 6.2831855f * _2c.y * ratio * ItemObjSuc::getRotAxisRatio());
    Mtx q;
    stQt2Mtx(q, &mObj->_34);
    PSMTXConcat(rot, q, q);
    PSMTXConcat(m, q, rot);
    mObj->setModelMatrixAndScale(rot);

    if (_c >= _d)
    {
        mObj->mpHandAnmMtx = mObj->getHandAnmMtx();
        _c = 0;
        _38 = 1;
        stMtx2Qt(&mObj->_34, q);
        _3c.z = 0.0f;
        _3c.y = 0.0f;
        _3c.x = 0.0f;
    }
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
        s16 ry = rot.y;
        s16 rx = rot.x;
        s16 rz = rot.z;

        f32 cy = JMASCos(ry);
        f32 cz = JMASCos(rz);
        f32 sz = JMASSin(rz);
        f32 cx = JMASCos(rx);
        f32 sx = JMASSin(rx);
        f32 sy = JMASSin(ry);

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
        s16 angle = (s16)(gravUp * (32767.0f * (2.0f * rnd - 1.0f)));
        f32 c = JMASCos(angle);
        f32 s = JMASSin(angle);

        _38[1][1] = 1.0f;
        _38[0][0] = c;
        _38[0][2] = s;
        _38[2][0] = -s;
        _38[2][2] = c;
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
    stVecNormalize(dir);

    f32 ratio = mObj->mSuccessionParent->getSuccessionItemNum() - mCount;
    bool sameNum = false;
    if (ratio == 1.0f)
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

    f32 mag = _18.length();
    f32 limit = mCount;
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
        stVecNormalize(_18);
        _18.scale(max);
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
    stVecNormalize(up);
    PSVECCrossProduct(&up, &zdir, &right);
    stVecNormalize(right);
    PSVECCrossProduct(&right, &up, &zdir);
    stVecNormalize(zdir);
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
    up.y += 90.0f;
    stVecNormalize(up);
    PSVECCrossProduct(&up, &zdir, &right);
    stVecNormalize(right);
    PSVECCrossProduct(&right, &up, &zdir);
    stVecNormalize(zdir);
    stMakeRMtx(rmtx, right, up, zdir);
    if (mtx != nullptr)
        PSMTXConcat(rmtx, mtx, rmtx);
    PSMTXConcat(rmtx, _38, rmtx);
    mObj->setModelMatrixAndScale(rmtx);
}

void ItemBuild::InitMoveDivested()
{
    JGeometry::TVec3f kartPos;
    JGeometry::TVec3f pos;
    pos.sub(mObj->mPos, mOffset);
    ObjUtility::getKartPos(mObj->mOwnerNum, &kartPos);
    pos.sub(kartPos);
    mObj->mDivestedPos = pos;
    mObj->mpHandAnmMtx = mObj->getJuggleHandAnmMtx();
    stMtx2Qt(&mObj->_44, mObj->mModel.getBaseTRMtx());
    stMtx2Qt(&mObj->_1d8, mObj->mpHandAnmMtx);
    mObj->initMaxDivestedFrame();
}

void ItemBuild::MoveDivested()
{
    f32 ratio = (f32)mObj->mWaitFrame / (f32)mObj->mDivestedMaxFrame;

    if (mObj->mSuccessionItemLink.getList() != nullptr)
    {
        MtxPtr mtx = mObj->_210;
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
    else
    {
        mObj->doHandOneEquip(true, ratio);
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

    int frame = 8;
    u8 num = _1fc;
    f32 anmOver = sJuggleAnmOverFrame;
    int step = (int)(58.0f / (f32)num);
    mJuggleFrameStep = (50 - (num - 1) * 8) / num;
    mJuggleXLength = sJuggleXLength;
    mJuggleGravity = sJuggleGravity;
    mJuggleIndex = _1fc - 1;
    for (JSUListIterator<ItemObj> it(_1a4.getFirst()); it != _1a4.getEnd(); it++)
    {
        ItemJuggle *juggle = it->mJuggle;
        juggle->_1c = frame;
        juggle->_d = 50;
        juggle->_14 = 8;
        juggle->_10 = 0;
        juggle->_2c.x = mJuggleXLength;
        juggle->_2c.y = (f32)mJuggleIndex;
        juggle->_2c.z = mJuggleGravity;
        juggle->_18 = anmOver;
        frame += step;
    }
    mJuggleFinished = 0;
    mPrevSucNum = _1fc;
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

JSUListIterator<ItemObj> ItemObjSuc::selectJuggleSucChild()
{
    JSULink<ItemObj> *result = nullptr;
    for (JSUListIterator<ItemObj> it(_1a4.getFirst()); it != _1a4.getEnd(); ++it)
    {
        if (it->mJuggle->_38 == 1)
            return it;
    }

    if (_1a4.getNumLinks() > 1)
    {
        JSULink<ItemObj> *link = _1a4.getLast();
        while (link != _1a4.getFirst())
        {
            ItemObj *obj = link->getObject();
            ItemObj *first = _1a4.getFirst()->getObject();
            link = link->getPrev();
            if (obj->mJuggle->_c > first->mJuggle->_c)
                _1a4.prepend(&obj->mSuccessionItemLink);
        }
    }

    if (_1a4.getNumLinks() != 0)
    {
        result = _1a4.getFirst();
        f32 min = result->getObject()->mJuggle->_3c.y;
        for (JSULink<ItemObj> *link = result; link != nullptr; link = link->getNext())
        {
            ItemJuggle *juggle = link->getObject()->mJuggle;
            if (juggle->_c >= -5 || juggle->_c <= 5u)
            {
                if (juggle->_3c.y < min)
                {
                    min = juggle->_3c.y;
                    result = link;
                }
            }
        }
    }
    return result;
}

void ItemObjSuc::initDivested()
{
    if (mSucMultiType == cMultiType_0)
    {
        mJuggleFirst = 0;
        mJuggleRot = getJuggleAnmFrame(mOrigOwnerNum, mOrigDriverNum);
        mTransferFlags |= 0x40;
    }
    initmoveDivested();
}

void ItemObjSuc::doDivested()
{
    if (mSucMultiType == cMultiType_0)
    {
        f32 rot = getJuggleAnmFrame(getOwnerNum(), getDriverNum());
        if (!mJuggleFirst)
        {
            if (fabs(mJuggleRot - rot) < sDivestedFrameSyncCheckFrame)
            {
                mJuggleFirst = 1;
                mTransferFlags &= ~0x40;
                doEquip();
            }
        }
    }

    int num = 0;
    for (JSULink<ItemObj> *link = _1a4.getFirst(); link != nullptr; link = link->getNext())
    {
        if (link->getObject()->mState == 1)
            num++;
    }
    if (num > 0)
        setStateEquip(false, false);
}

void ItemObjSuc::setSucMultiType(cMultiType type)
{
    mSucMultiType = type;
    switch (mSucMultiType)
    {
    case cMultiType_0:
        mEquipInitFunc = &ItemObjSuc::initJuggleEquip;
        mEquipDoFunc = &ItemObjSuc::doJuggleEquip;
        mSelectSucFunc = &ItemObjSuc::selectJuggleSucChild;
        return;
    case cMultiType_1:
        mEquipInitFunc = &ItemObjSuc::initBuildEquip;
        mEquipDoFunc = &ItemObjSuc::doBuildEquip;
        mSelectSucFunc = &ItemObjSuc::selectBuildSucChild;
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
