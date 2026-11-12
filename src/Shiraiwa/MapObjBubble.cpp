#include "Shiraiwa/Objects/MapObjBubble.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "JSystem/JParticle/JPAMath.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

StateObserver::StateFuncSet<TMapObjBubble> TMapObjBubble::sTable[4] = {
    {0, &TMapObjBubble::initFunc_Start, &TMapObjBubble::doFunc_Start},
    {1, &TMapObjBubble::initFunc_Dive, &TMapObjBubble::doFunc_Dive},
    {2, &TMapObjBubble::initFunc_Symptom, &TMapObjBubble::doFunc_Symptom},
    {3, &TMapObjBubble::initFunc_Jump, &TMapObjBubble::doFunc_Jump},
};

f32 TMapObjBubble::sGravity = 1.0f;
J3DAnmTextureSRTKey *TMapObjBubble::sBubbleBtkAnm;

#include "JSystem/JAudio/JASFakeMatch2.h"

TMapObjBubble::TMapObjBubble(const CrsData::SObject &obj) : TMapObjHioNode(obj), StateObserver(), mBtk()
{
    createSoundMgr();
}

TMapObjBubble::~TMapObjBubble() {}

void TMapObjBubble::createColModel(J3DModelData *modelData)
{
    createBoundsSphere(modelData);
}

const char *TMapObjBubble::getBmdFileName()
{
    static const char *cBmdName = "/Objects/Bubble1.bmd";
    return cBmdName;
}

void TMapObjBubble::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x200, false);
    mBtk.setExModel(&mModel);
    mBtk.mAnmBase = sBubbleBtkAnm;
    mBtk.initFrameCtrl(mBtk.mAnmBase);
}

void TMapObjBubble::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    J3DAnmObjMaterial::setupTexSRTAnmData(&sBubbleBtkAnm, modelData, ObjUtility::getPtrCourseArc("/Objects/Bubble1.btk"));
}

void TMapObjBubble::InitExec()
{
    Observer_FindAndInit(TMapObjBubble, 4);
}

void TMapObjBubble::MoveExec()
{
    Observer_FindAndExec(TMapObjBubble, 4);
}

void TMapObjBubble::update()
{
    mBtk.anmFrameProc();
    GeographyObj::update();
}

void TMapObjBubble::reset()
{
    resetObject();
    ResetState();
    mDiveTime = mObjData->mParam1;
    if (mDiveTime == 0) {
        mDiveTime = 180;
    }
    mStartTime = mObjData->mParam2;
    mSymptomTime = mObjData->mParam3;
    if (mSymptomTime == 0) {
        mSymptomTime = 60;
    }
    mJumpVel = (u32)(0.01f * mObjData->mParam4);
    if (mJumpVel == 0.0f) {
        mJumpVel = 80.0f;
    }
    mAccel = 0.01f * mObjData->mParam5;
    mBtk.mFrameCtrl.reset();
    SiUtil::setRandomStartFrame(&mBtk.mFrameCtrl, getGeoRnd());
    clrObjFlagHidding();
    setAllCheckKartHitFlag();
    _58 = 13;
    mRotMtx.getXDir(mDir);
    mDir.normalize();
    mEmitter = nullptr;
    mPrevHeight = 0.0f;
}

void TMapObjBubble::calc()
{
    ExecuteState();
    mBtk.mFrameCtrl.update();
}

void TMapObjBubble::initFunc_Start()
{
    stopParticle();
}

void TMapObjBubble::startParticle()
{
    if (mEmitter == nullptr) {
        mEmitter = JPEffectMgr::getEffectMgr()->createEmt("mk_bubble_a", mPos);
#line 199
        JUT_ASSERT(mEmitter != 0);
    }
    if (mEmitter) {
        mEmitter->clearStatus(1);
    }
}

void TMapObjBubble::stopParticle()
{
    if (mEmitter) {
        mEmitter->setStatus(1);
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

void TMapObjBubble::moveEmitter()
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

void TMapObjBubble::doFunc_Start()
{
    if (getStateCount() > mStartTime) {
        setState(2);
    }
}

void TMapObjBubble::initFunc_Dive()
{
    stopParticle();
}

void TMapObjBubble::doFunc_Dive()
{
    if (getStateCount() > mDiveTime) {
        setState(2);
    }
}

void TMapObjBubble::initFunc_Symptom() {}

void TMapObjBubble::doFunc_Symptom()
{
    if (getStateCount() > mSymptomTime) {
        setState(3);
    }
}

void TMapObjBubble::initFunc_Jump()
{
    mVel.set(0.0f, mJumpVel, 0.0f);
    startParticle();
}

void TMapObjBubble::doFunc_Jump()
{
    mVel.y -= sGravity;
    JGeometry::TVec3f dir;
    dir.set(mDir);
    dir.normalize();
    dir.scale(mAccel);
    mVel.add(dir);
    mPos.add(mVel);
    if (mPos.y < mObjData->position.y) {
        mPos.set(mObjData->position);
        setState(1);
    }

    JGeometry::TVec3f xDir, yDir, zDir;
    yDir.set(mVel);
    yDir.normalize();
    zDir.set(mObjData->zNorm);
    zDir.normalize();
    xDir.cross(yDir, zDir);
    xDir.normalize();
    zDir.cross(xDir, yDir);
    zDir.normalize();
    mRotMtx[0][0] = xDir.x;
    mRotMtx[1][0] = xDir.y;
    mRotMtx[2][0] = xDir.z;
    mRotMtx[0][1] = yDir.x;
    mRotMtx[1][1] = yDir.y;
    mRotMtx[2][1] = yDir.z;
    mRotMtx[0][2] = zDir.x;
    mRotMtx[1][2] = zDir.y;
    mRotMtx[2][2] = zDir.z;

    CrsGround ground(RaceMgr::getManager()->getCourse());
    ground.search(mPos);
    f32 height = mPos.y - ground.getSplashHeight();
    if (height * mPrevHeight < 0.0f) {
        JPABaseEmitter *emt = JPEffectMgr::getEffectMgr()->createEmt("mk_bubbleLand_a", mPos);
        emt->setGlobalScale(mScale);
        emt = JPEffectMgr::getEffectMgr()->createEmt("mk_bubbleLand_b", mPos);
        emt->setGlobalScale(mScale);
        if (mVel.y > 0.0f) {
            mSoundMgr->setSe(0x4003b);
        }
        else {
            mSoundMgr->setSe(0x4003c);
        }
    }
    mPrevHeight = height;

    moveEmitter();
}
