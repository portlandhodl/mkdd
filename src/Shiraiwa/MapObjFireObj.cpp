#include "Shiraiwa/Objects/MapObjFireObj.h"

#include "JSystem/JUtility/JUTAssert.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/Objects/MapObjFireBall.h"

StateObserver::StateFuncSet<TMapObjFireObj> const TMapObjFireObj::sTable[4] = {
    { 0, &TMapObjFireObj::initFunc_Normal, &TMapObjFireObj::doFunc_Normal },
    { 1, &TMapObjFireObj::initFunc_Drive, &TMapObjFireObj::doFunc_Drive },
    { 2, &TMapObjFireObj::initFunc_Accel, &TMapObjFireObj::doFunc_Accel },
    { 3, &TMapObjFireObj::initFunc_Decel, &TMapObjFireObj::doFunc_Decel },
};

s16 TMapObjFireObj::sDriveTime = 60;

TMapObjFireObj::TMapObjFireObj(const CrsData::SObject &obj, u32 id) : TMapObjHioNode(obj), StateObserver()
{
    mFire = nullptr;
    mFireNum = 0;
    if (id == 0x106a)
        mObjFlag |= 1;

    u32 num = (u8)obj.mParam1;
    if (num < 1)
        num = 1;
    makeFire(num, id);
}

TMapObjFireObj::~TMapObjFireObj()
{
    for (int i = 0; i < mFireNum; i++)
        delete mFire[i];
    delete[] mFire;
}

void TMapObjFireObj::reset()
{
    resetObject();
    StateObserver::ResetState();
    resetFire();
}

void TMapObjFireObj::calc()
{
    StateObserver::ExecuteState();
    if (checkDrive())
        setState(1);
}

void TMapObjFireObj::resetFire()
{
    for (int i = 0; i < mFireNum; i++) {
        TMapObjFireBallBase *fire = mFire[i];
        fire->_178 = 0.0f;
        fire->_17c = 0.0f;
        fire->_180 = 0.0f;
        mFire[i]->setRMtx(mRotMtx);
        mFire[i]->_184.set(mPos);
        mFire[i]->mScale.set(mScale);
    }
    mFireRot[0] = 0.0f;
    mFireRot[1] = 0.0f;
    mFireRot[2] = 0.0f;
    mFireRot[3] = 1.0f;
    mFireRot[4] = 0.0f;
    mFireRot[5] = 0.0f;
    mFireRot[6] = 0.0f;
    mFireRot[7] = 1.0f;
}

bool TMapObjFireObj::checkDrive()
{
    bool drove;
    bool ret = false;
    for (int i = 0; i < mFireNum; i++) {
        drove = false;
        if (mFire[i]->_170) {
            drove = true;
            mFire[i]->_170 = 0;
        }
        if (drove) {
            ret = true;
            break;
        }
    }
    return ret;
}

void TMapObjFireObj::initFunc_Normal()
{
    for (int ofs = 0, i = ofs; i < mFireNum; i++, ofs += 4) {
        TMapObjFireBallBase **p = (TMapObjFireBallBase **)((u8 *)mFire + ofs);
        TMapObjFireBallBase *fire = *p;
        fire->mRot.x = mFireRot[0];
        fire->mRot.y = mFireRot[1];
        fire->mRot.z = mFireRot[2];
        fire->mRot.w = mFireRot[3];
    }
}

void TMapObjFireObj::doFunc_Normal() {}

void TMapObjFireObj::initFunc_Accel() {}

void TMapObjFireObj::doFunc_Accel()
{
    JGeometry::TQuat4f q;
    q.x = mFireRot[0] + mFireRotRate * (mFireRot[4] - mFireRot[0]);
    q.y = mFireRot[1] + mFireRotRate * (mFireRot[5] - mFireRot[1]);
    q.z = mFireRot[2] + mFireRotRate * (mFireRot[6] - mFireRot[2]);
    q.w = mFireRot[3] + mFireRotRate * (mFireRot[7] - mFireRot[3]);

    f32 sq = q.squared();
    if (sq <= JGeometry::TUtilf::epsilon())
        q.set(0.0f, 0.0f, 0.0f, 1.0f);
    else
        q.scale(JGeometry::TUtilf::inv_sqrt(sq));

    for (int i = 0; i < mFireNum; i++) {
        TMapObjFireBallBase *fire = mFire[i];
        fire->mRot.x = q.x;
        fire->mRot.y = q.y;
        fire->mRot.z = q.z;
        fire->mRot.w = q.w;
    }

    mFireRotRate += 0.1f;
    if (mFireRotRate > 1.0f) {
        mFireRotRate = 1.0f;
        setState(2);
    }
}

void TMapObjFireObj::initFunc_Drive()
{
    for (int i = 0; i < mFireNum; i++) {
        TMapObjFireBallBase *fire = mFire[i];
        fire->mRot.x = mFireRot[4];
        fire->mRot.y = mFireRot[5];
        fire->mRot.z = mFireRot[6];
        fire->mRot.w = mFireRot[7];
    }
}

void TMapObjFireObj::doFunc_Drive()
{
    if (getStateCount() > sDriveTime)
        setState(3);
}

void TMapObjFireObj::initFunc_Decel() {}

void TMapObjFireObj::doFunc_Decel()
{
    JGeometry::TQuat4f q;
    q.x = mFireRot[0] + mFireRotRate * (mFireRot[4] - mFireRot[0]);
    q.y = mFireRot[1] + mFireRotRate * (mFireRot[5] - mFireRot[1]);
    q.z = mFireRot[2] + mFireRotRate * (mFireRot[6] - mFireRot[2]);
    q.w = mFireRot[3] + mFireRotRate * (mFireRot[7] - mFireRot[3]);

    f32 sq = q.squared();
    if (sq <= JGeometry::TUtilf::epsilon())
        q.set(0.0f, 0.0f, 0.0f, 1.0f);
    else
        q.scale(JGeometry::TUtilf::inv_sqrt(sq));

    for (int i = 0; i < mFireNum; i++) {
        TMapObjFireBallBase *fire = mFire[i];
        fire->mRot.x = q.x;
        fire->mRot.y = q.y;
        fire->mRot.z = q.z;
        fire->mRot.w = q.w;
    }

    mFireRotRate -= 0.01f;
    if (mFireRotRate < 0.0f) {
        mFireRotRate = 0.0f;
        setState(0);
    }
}

void TMapObjFireObj::makeFire(u8 num, u32 id)
{
    mFireNum = num;
    mFire = new TMapObjFireBallBase *[mFireNum];
    for (int i = 0; i < mFireNum; i++) {
        mFire[i] = (TMapObjFireBallBase *)GetGeoObjMgr()->createSubObj(id);
#line 558
        JUT_ASSERT_F(mFire[i], "%s", "Failed to make FireBallBase Object.");
    }
}

void TMapObjFireObj::InitExec()
{
    StateObserver::StateFuncSet<TMapObjFireObj> const *func = FindStateFunc(getState(), sTable, 4);
    (this->*func->mInitFunc)();
}

void TMapObjFireObj::MoveExec()
{
    StateObserver::StateFuncSet<TMapObjFireObj> const *func = FindStateFunc(getState(), sTable, 4);
    (this->*func->mExecFunc)();
}
