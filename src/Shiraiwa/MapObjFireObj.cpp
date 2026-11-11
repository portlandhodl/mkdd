#include "Shiraiwa/Objects/MapObjFireObj.h"

#include "JSystem/JUtility/JUTAssert.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ItemObj.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/Objects/MapObjFireBall.h"
#include "Shiraiwa/SiUtil.h"
#include "Yamamoto/kartCtrl.h"

StateObserver::StateFuncSet<TMapObjFireObj> const TMapObjFireObj::sTable[4] = {
    { 0, &TMapObjFireObj::initFunc_Normal, &TMapObjFireObj::doFunc_Normal },
    { 1, &TMapObjFireObj::initFunc_Accel, &TMapObjFireObj::doFunc_Accel },
    { 2, &TMapObjFireObj::initFunc_Drive, &TMapObjFireObj::doFunc_Drive },
    { 3, &TMapObjFireObj::initFunc_Decel, &TMapObjFireObj::doFunc_Decel },
};

s16 TMapObjFireObj::sDriveTime = 60;

J3DAnmTextureSRTKey *TMapObjFireBall::sFireBallBtkAnm;
J3DAnmTextureSRTKey *TMapObjKpFireBarBall::sKpFireBarBallBtkAnm;
J3DAnmTevRegKey *TMapObjKpFireBarBall::sKpFireBarBallBrkAnm;

#include "JSystem/JAudio/JASFakeMatch2.h"

TMapObjFireBallBase::TMapObjFireBallBase(u32 id) : GeographyObj(id)
{
    _16c = 0;
    _170 = 0;
    mIndex = 0;
}

TMapObjFireBallBase::~TMapObjFireBallBase() {}

void TMapObjFireBallBase::reset()
{
    SiUtil::setItemReaction(GetGeoObjMgr()->getGeoHitTable(), this);
    setAllCheckKartHitFlag();
    setObjFlagCheckItemHitting();
    mRot.set(0.0f, 0.0f, 0.0f, 1.0f);
    _174 = 1.0f;
    _58 = 13;
    _170 = 0;
    _16c = 0;
}

void TMapObjFireBallBase::createColModel(J3DModelData *)
{
    createBoundsSphere(100.0f, 1.0f);
}

void TMapObjFireBall::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x200, false);
    mBtk.setExModel(&mModel);
    mBtk.mAnmBase = sFireBallBtkAnm;
    mBtk.initFrameCtrl(mBtk.mAnmBase);
}

void TMapObjFireBall::reset()
{
    TMapObjFireBallBase::reset();
    mBtk.mFrameCtrl.reset();
    SiUtil::setRandomStartFrame(&mBtk.mFrameCtrl, getGeoRnd());
}

void TMapObjFireBall::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *btk = ObjUtility::getPtrCourseArc("/Objects/FireBar1Fire.btk");
    J3DAnmObjMaterial::setupTexSRTAnmData(&sFireBallBtkAnm, modelData, btk);
}

void TMapObjKpFireBarBall::reset()
{
    TMapObjFireBallBase::reset();
    mBtk.mFrameCtrl.reset();
    SiUtil::setRandomStartFrame(&mBtk.mFrameCtrl, getGeoRnd());
    mBrk.mFrameCtrl.reset();
    SiUtil::setRandomStartFrame(&mBrk.mFrameCtrl, getGeoRnd());
    PSMTXIdentity(mRotMtx);
    mEmitter = nullptr;
}

void TMapObjKpFireBarBall::createColModel(J3DModelData *)
{
    createBoundsSphere(160.0f, 1.0f);
}

void TMapObjKpFireBarBall::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x80200, false);
    mBtk.setExModel(&mModel);
    mBtk.mAnmBase = sKpFireBarBallBtkAnm;
    mBtk.initFrameCtrl(mBtk.mAnmBase);
    mBrk.setExModel(&mModel);
    mBrk.mAnmBase = sKpFireBarBallBrkAnm;
    mBrk.initFrameCtrl(mBrk.mAnmBase);
}

void TMapObjKpFireBarBall::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *btk = ObjUtility::getPtrCourseArc("/Objects/KpFireBar1Fire.btk");
    J3DAnmObjMaterial::setupTexSRTAnmData(&sKpFireBarBallBtkAnm, modelData, btk);
    void *brk = ObjUtility::getPtrCourseArc("/Objects/KpFireBar1Fire.brk");
    J3DAnmObjMaterial::setupTevRegAnmData(&sKpFireBarBallBrkAnm, modelData, brk);
}

void TMapObjKpFireBarBall::update()
{
    mBrk.anmFrameProc();
    TMapObjFireBallBase::update();
}

void TMapObjKpFireBarBall::calc()
{
    mBrk.mFrameCtrl.update();
    mRot.transform(_178);
    JGeometry::TVec3f pos;
    pos.x = _178.x;
    pos.y = _178.y;
    pos.z = _178.z;
    if (_174 != 1.0f)
        pos.scale(_174);
    mMtx[0][3] = _184.x;
    mMtx[1][3] = _184.y;
    mMtx[2][3] = _184.z;
    PSMTXMultVec(mMtx, &pos, &mPos);

    if (mEmitter)
        mEmitter->setGlobalTranslation(mPos);
    else
        mEmitter = JPEffectMgr::getEffectMgr()->createEmtCameraClip("mk_kpcFB", mPos);

    if (tstItemHitting()) {
        switch (mColItemObj->getKind()) {
        case 7:
        case 8:
            _170 = 1;
            break;
        }
    }
    mBtk.mFrameCtrl.update();
    if (mIndex && mSoundMgr)
        mSoundMgr->setSe(0x40032);
}

void TMapObjFireBallBase::setCurrentViewNo(u32 viewNo)
{
    mModel.setCurrentViewNo(viewNo);
    Mtx m;
    ObjUtility::getCamDependLightMtx(viewNo, mPos, m);
    mModel.setEffectMtx(m, 1);
}

void TMapObjFireBallBase::calc()
{
    mRot.transform(_178);
    JGeometry::TVec3f pos;
    pos.x = _178.x;
    pos.y = _178.y;
    pos.z = _178.z;
    if (_174 != 1.0f)
        pos.scale(_174);
    mRotMtx[0][3] = _184.x;
    mRotMtx[1][3] = _184.y;
    mRotMtx[2][3] = _184.z;
    PSMTXMultVec(mRotMtx, &pos, &mPos);

    if (tstItemHitting()) {
        switch (mColItemObj->getKind()) {
        case 7:
        case 8:
            _170 = 1;
            break;
        }
    }
    mBtk.mFrameCtrl.update();
    if (mIndex && mSoundMgr)
        mSoundMgr->setSe(0x40032);
    if (mShadowMdl)
        moveShadowModel();
}

void TMapObjFireBallBase::doKartColCallBack(int kartNo)
{
    if (_170)
        return;
    if (GetKartCtrl()->getKartBody(kartNo)->mCarStatus & 0x40000ull)
        _170 = 1;
}

void TMapObjFireBallBase::update()
{
    mBtk.anmFrameProc();
    GeographyObj::update();
}

void TMapObjFireBallBase::applyDriveState()
{
    if (_170) {
        _16c++;
        _170 = 0;
    }
}

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

void TMapObjFireObj::resetFire()
{
    for (int i = 0; i < mFireNum; i++) {
        TMapObjFireBallBase *fire = mFire[i];
        fire->_178.x = 0.0f;
        fire->_178.y = 0.0f;
        fire->_178.z = 0.0f;
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

void TMapObjFireObj::calc()
{
    StateObserver::ExecuteState();
    if (checkDrive())
        setState(1);
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

void TMapObjFireObj::setRotate(const JGeometry::TQuat4f &q)
{
    for (int i = 0; i < mFireNum; i++) {
        TMapObjFireBallBase *fire = mFire[i];
        fire->mRot.x = q.x;
        fire->mRot.y = q.y;
        fire->mRot.z = q.z;
        fire->mRot.w = q.w;
    }
}

void TMapObjFireObj::initFunc_Normal()
{
    setRotate(mFireRotQ[0]);
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

    setRotate(q);

    mFireRotRate += 0.1f;
    if (mFireRotRate > 1.0f) {
        mFireRotRate = 1.0f;
        setState(2);
    }
}

void TMapObjFireObj::initFunc_Drive()
{
    setRotate(mFireRotQ[1]);
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

    setRotate(q);

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
