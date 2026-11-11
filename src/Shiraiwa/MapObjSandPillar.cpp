#include "Shiraiwa/Objects/MapObjSandPillar.h"

#include "JSystem/JParticle/JPAEmitter.h"
#include "JSystem/JParticle/JPAMath.h"
#include "Sato/AnmController.h"
#include "Sato/ItemObj.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

StateObserver::StateFuncSet<TMapObjSandPillar> TMapObjSandPillar::sTable[1] = {
    {0, &TMapObjSandPillar::initFunc_Walk, &TMapObjSandPillar::doFunc_Walk},
};

f32 TMapObjSandPillar::sItemVel = 50.0f;
f32 TMapObjSandPillar::sItemUpVel = 0.08f;
J3DAnmTransform *TMapObjSandPillar::sSandPillarBckAnmTrans;
J3DMtxCalc *TMapObjSandPillar::sSandPillarBckMtxCalc;
J3DAnmTextureSRTKey *TMapObjSandPillar::sSandPillarBtkAnm;

#include "JSystem/JAudio/JASFakeMatch2.h"

TMapObjSandPillar::TMapObjSandPillar(const CrsData::SObject &obj) : TKartThrower(obj), StateObserver(), mBtk()
{
    NewAnmCtrl();
    mPathWalk = new TPathWalk(&obj);
}

TMapObjSandPillar::~TMapObjSandPillar()
{
    delete mPathWalk;
}

void TMapObjSandPillar::InitExec()
{
    Observer_FindAndInit(TMapObjSandPillar, 1);
}

void TMapObjSandPillar::MoveExec()
{
    Observer_FindAndExec(TMapObjSandPillar, 1);
}

void TMapObjSandPillar::getKartThrowDirPow(JGeometry::TVec3f *dir, f32 *pow, int)
{
    if (dir != nullptr) {
        dir->set(mVel);
        if (dir->isZero()) {
            dir->set(mObjData->zNorm);
        }
        dir->y = 0.0f;
        dir->normalize();
    }
    if (pow != nullptr) {
        s32 throwPow = mObjData->mParam2;
        if (throwPow <= 0) {
            throwPow = 50;
        }
        *pow = throwPow;
    }
}

void TMapObjSandPillar::getThrowDir(JGeometry::TVec3f *dir, s16)
{
    dir->set(mVel);
    dir->y = 0.0f;
    dir->normalize();
}

s32 TMapObjSandPillar::getThrowPow()
{
    s32 throwPow = mObjData->mParam2;
    if (throwPow <= 0) {
        throwPow = 50;
    }
    return throwPow;
}

const char *TMapObjSandPillar::getBmdFileName()
{
    static const char *cBmdName = "/Objects/Tornado.bmd";
    return cBmdName;
}

void TMapObjSandPillar::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    J3DAnmObjTrans::setupTransAnmData(&sSandPillarBckAnmTrans, &sSandPillarBckMtxCalc, modelData, ObjUtility::getPtrCourseArc("/Objects/Tornado.bca"));
    J3DAnmObjMaterial::setupTexSRTAnmData(&sSandPillarBtkAnm, modelData, ObjUtility::getPtrCourseArc("/Objects/Tornado.btk"));
}

void TMapObjSandPillar::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x200, false);
    AnmController *ctrl = mAnmCtrl;
    ctrl->mTrans = new AnmControlTrans();
    ctrl->mTrans->initAnm(1, &mModel);
    mAnmCtrl->mTrans->registration(0, sSandPillarBckAnmTrans, sSandPillarBckMtxCalc);
    mAnmCtrl->mTrans->getObj(0)->getFrameCtrl()->setAttribute(2);
    mBtk.mModel = &mModel;
    mBtk.mAnmBase = sSandPillarBtkAnm;
    mBtk.initFrameCtrl(mBtk.mAnmBase);
    f32 maxXZ = mScale.x > mScale.z ? mScale.x : mScale.z;
    mCircleSoundMgr = new GameAudio::CircleSoundMgr(&mPos, 330.0f * maxXZ, 0.0f, 2000.0f * mScale.y, nullptr);
}

void TMapObjSandPillar::createColModel(J3DModelData *modelData)
{
    f32 maxXZ = mScale.x > mScale.z ? mScale.x : mScale.z;
    f32 max = maxXZ > mScale.y ? maxXZ : mScale.y;
    createBoundsCylinder(modelData, 330.0f * (maxXZ / max), 2000.0f * (mScale.y / max));
}

void TMapObjSandPillar::reset()
{
    resetObject();
    _58 = 0x10;
    f32 speed = mObjData->mParam1;
    if (0.0f == speed) {
        speed = 12.0f;
    }
    mWalkMtx.set(mRotMtx);
    mPathWalk->init(&mPos, &mVel, &mWalkMtx);
    mPathWalk->setParameters(speed, 0.2f, 0.2f, 0.001f, 0.0f);
    mPathWalk->setCheckLength(600.0f);
    s16 angle = mObjData->mParam3;
    if (angle == 0) {
        angle = 90;
    }
    mAngle = 182.04445f * angle;
    mAnmCtrl->Reset();
    mBtk.mFrameCtrl.setFrame((f32)mBtk.mFrameCtrl.mStartFrame);
    mBtk.mFrameCtrl.setRate(1.0f);
    mBtk.mFrameCtrl.mState = 0;
    mEmitter = nullptr;
    ResetState();
}

void TMapObjSandPillar::createEmitter()
{
    if (mEmitter == nullptr) {
        mEmitter = GetJPAMgr()->createEmt("mk_tatsumaki_a", mPos);
#line 268
        JUT_ASSERT(mEmitter != 0);
    }
}

static inline void setScaleMtx(Mtx m, f32 x, f32 y, f32 z)
{
    m[0][0] = x;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[1][0] = 0.0f;
    m[1][1] = y;
    m[1][2] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = z;
}

void TMapObjSandPillar::moveEmitter()
{
    Mtx m;
    PSMTXIdentity(m);
    setScaleMtx(m, mScale.x, mScale.y, mScale.z);
    PSMTXConcat(m, mRotMtx, m);
    m[0][3] = mPos.x;
    m[1][3] = mPos.y;
    m[2][3] = mPos.z;
    JPABaseEmitter *emitter = mEmitter;
    if (emitter) {
        JPASetRMtxSTVecfromMtx(m, emitter->mGlobalRot, &emitter->mGlobalScl, &emitter->mGlobalTrs);
        emitter->setGlobalParticleScale(emitter->mGlobalScl);
    }
}

void TMapObjSandPillar::calc()
{
    createEmitter();
    mPathWalk->update();
    mBtk.mFrameCtrl.update();
    moveEmitter();
    ExecuteState();
    mCircleSoundMgr->setSe(0x40026);
    mCircleSoundMgr->frameWork();
}

void TMapObjSandPillar::update()
{
    mBtk.anmFrameProc();
    GeographyObj::update();
}

void TMapObjSandPillar::getItemThrowDirPow(JGeometry::TVec3f *dir, f32 *pow, const ItemObj &item)
{
    JGeometry::TVec3f itemPos;
    JGeometry::TVec3f vec;
    itemPos.set(item.mPos);
    vec.sub(mPos, itemPos);
    vec.y = 0.0f;
    vec.normalize();
    vec.y = sItemUpVel;
    vec.normalize();
    dir->set(vec);
    *pow = sItemVel;
}

void TMapObjSandPillar::initFunc_Walk()
{
    mPathWalk->_94 = true;
}

void TMapObjSandPillar::doFunc_Walk() {}
