#include "Shiraiwa/Objects/MapObjDossun.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/AnmController.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Yamamoto/kartCtrl.h"

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

StateObserver::StateFuncSet<TMapObjDossun> TMapObjDossun::sTable[6] = {
    {0, &TMapObjDossun::initFunc_Start, &TMapObjDossun::doFunc_Start},
    {1, &TMapObjDossun::initFunc_Top, &TMapObjDossun::doFunc_Top},
    {2, &TMapObjDossun::initFunc_Down, &TMapObjDossun::doFunc_Down},
    {3, &TMapObjDossun::initFunc_Bottom, &TMapObjDossun::doFunc_Bottom},
    {4, &TMapObjDossun::initFunc_Up, &TMapObjDossun::doFunc_Up},
    {5, &TMapObjDossun::initFunc_Die, &TMapObjDossun::doFunc_Die},
};

JGeometry::TVec3f TMapObjDossun::scDownVelocity0(0.0f, 100.0f, 0.0f);
JGeometry::TVec3f TMapObjDossun::sDieVelocity0(0.0f, 200.0f, 0.0f);
f32 TMapObjDossun::sBtpRevFrameRate = -0.6f;
f32 TMapObjDossun::sBtpPlyFrameRate = 0.6f;
f32 TMapObjDossun::sDieGravityY = 5.0f;
J3DAnmTransform *TMapObjDossun::sDossunBckAnmTrans;
J3DMtxCalc *TMapObjDossun::sDossunBckMtxCalc;
J3DAnmTexPattern *TMapObjDossun::sDossunBtpAnm;

#include "JSystem/JAudio/JASFakeMatch2.h"

// Inlines from AnmController.h whose assert line numbers differ from the shared header
#define ANMCTRL_MINMAX_ASSERT(line, min, cur, max, fmt)                                                            \
    (((min) <= (cur)) && ((cur) < (max)))                                                                         \
        ? (void)0                                                                                                 \
        : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "AnmController.h", line, fmt, (min), (cur), (max)), \
           OSPanic("AnmController.h", line, "Halt"));

template <typename T>
static inline T *getTransObj(TAnmControlBase<T> *trans, u8 anm_no)
{
    ANMCTRL_MINMAX_ASSERT(122, 0, anm_no, trans->mMaxAnmNo, "range over: %d <= anm_no=%d < %d");
    return trans->getObjNoCheck(anm_no);
}

template <typename T>
static inline void changeTransAnm(T *ctrl, u8 AnmNo)
{
    if (ctrl->mTrans == nullptr) {
        return;
    }
    ANMCTRL_MINMAX_ASSERT(338, 0, AnmNo, ctrl->mTrans->mMaxAnmNo, "range over: %d <= AnmNo=%d < %d");
    J3DAnmObjTrans *transObj = getTransObj(ctrl->mTrans, AnmNo);
    if (transObj->_28 & 1) {
        transObj->setWeight(0, 1.0f);
        for (u8 i = 1; i < 4; i++) {
            transObj->setWeight(i, 0.0f);
        }
    }
    ctrl->mTrans->changeAnm(AnmNo, true);
}

TMapObjDossun::TMapObjDossun(const CrsData::SObject &obj)
    : TMapObjStamper(obj), StateObserver(), mHeight(1000), mTopFrame(250), mBottomFrame(30)
{
    NewAnmCtrl();
    createSoundMgr();
    mPathMove = new TPathMove(&obj);
}

TMapObjDossun::~TMapObjDossun()
{
    delete mPathMove;
}

void TMapObjDossun::reset()
{
    resetObject();
    setObjFlagCheckItemHitting();
    setAllCheckKartHitFlag();
    _58 = 10;
    if (mObjData->mParam1 != 0) {
        mTopFrame = mObjData->mParam1;
    }
    if (mObjData->mParam2 != 0) {
        mBottomFrame = mObjData->mParam2;
    }
    if (IsPathMove()) {
        mPathMove->getNodePosition(&mPos, 0);
        mPathMove->setSpeed(10.0f, 10.0f);
    }
    mStartFrame = mObjData->mParam3;
    if (mStartFrame < 0) {
        mStartFrame = 0;
    }
    setState(1);

    CrsGround ground(RaceMgr::getManager()->getCourse());
    ground.search(mPos);
    mHeight = mPos.y - ground.getHeight();
#line 108
    JUT_ASSERT(mHeight >= 0);

    mFreeMove.init(&mPos, &mVel, 200.0f);
    mPathMove->init(&mPos, &mVel);
    if (mHeight == 0.0f) {
        mPathMove->setSpeed(1.4f, mBottomFrame);
    }
    ResetState();
    mAnmCtrl->Reset();
    changeTransAnm(getAnmCtrl(), 0);
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->setFlag();
    }
    mBtpAnmObj.mFrameCtrl.reset();
    mBtpAnmObj.mFrameCtrl.setAttribute(0);
    resetFrame();
}

void TMapObjDossun::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    J3DAnmObjTrans::setupTransAnmData(&sDossunBckAnmTrans, &sDossunBckMtxCalc, modelData,
                                      ObjUtility::getPtrCourseArc("/Objects/Dossun1.bck"));
    J3DAnmObjMaterial::setupTexPatternAnmData(&sDossunBtpAnm, modelData,
                                              ObjUtility::getPtrCourseArc("/Objects/Dossun1.btp"));
}

const char *TMapObjDossun::getShadowBmdFileName()
{
    static const char *cShadowBmdName = "/Objects/Dossun1_kage.bmd";
    return cShadowBmdName;
}

void TMapObjDossun::createColModel(J3DModelData *modelData)
{
    f32 maxXZ = mScale.x > mScale.z ? mScale.x : mScale.z;
    f32 ratio = mScale.y / maxXZ;
    static f32 cBoundHeight = 800.0f * ratio;
    createBoundsCylinder(modelData, 500.0f, cBoundHeight);
}

void TMapObjDossun::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x40000, false);
    AnmController *anmCtrl = mAnmCtrl;
    anmCtrl->InitRegistration(1, &mModel);
    mAnmCtrl->RegisterTrans(0, sDossunBckAnmTrans, sDossunBckMtxCalc);
    mAnmCtrl->getFrameCtrl(0)->setAttribute(2);
    mBtpAnmObj.setExModel(&mModel);
    mBtpAnmObj.attach(sDossunBtpAnm);
}

const char *TMapObjDossun::getBmdFileName()
{
    static const char *cBmdName = "/Objects/Dossun1.bmd";
    return cBmdName;
}

void TMapObjDossun::update()
{
    mBtpAnmObj.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}

void TMapObjDossun::doKartColCallBack(int kartNo)
{
    if (KartCtrl::getKartCtrl()->getKartBody(kartNo)->mCarStatus & 0x40000ull) {
        setState(5);
    }
    else {
        startStamp(kartNo);
    }
}

void TMapObjDossun::initFunc_Start() {}

void TMapObjDossun::doFunc_Start()
{
    if (getStateCount() > mStartFrame) {
        setState(1);
    }
}

void TMapObjDossun::initFunc_Wait()
{
    mFrame = 0;
}

void TMapObjDossun::doFunc_Wait()
{
    if (getStateCount() > mTopFrame * 2) {
        setState(2);
    }
    else {
        mFrame++;
    }
}



void TMapObjDossun::initFunc_Up()
{
    mBtpAnmObj.mFrameCtrl.setFrame(mBtpAnmObj.mFrameCtrl.getEnd());
    mBtpAnmObj.mFrameCtrl.setRate(sBtpRevFrameRate);
    JGeometry::TVec3f offset(0.0f, (f32)mHeight, 0.0f);
    mFreeMove.setTargetOffsetUniform(offset, 120);
}

void TMapObjDossun::doFunc_Up()
{
    if (!mFreeMove.hasTarget()) {
        setState(1);
    }
}

void TMapObjDossun::initFunc_Top()
{
    mFrame = 0;
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->setFlag();
    }
    if (IsPathMove()) {
        mPathMove->setTargetNode();
        mVel.zero();
    }
}

void TMapObjDossun::doFunc_Top()
{
    if (mPathMove->_1c) {
        moveShadowModel();
    }
    else if (mHeight == 0) {
        if (mFrame < mTopFrame) {
            if (mFrame == 0) {
                shakeCamera();
            }
            mFrame++;
        }
        else {
            mFrame = 0;
            if (mAnmCtrl->mTrans != nullptr) {
                mAnmCtrl->mTrans->setFlag();
            }
            if (IsPathMove()) {
                mPathMove->setTargetNode();
                mVel.zero();
            }
        }
    }
    else {
        u32 anmFrame = getTransObj(mAnmCtrl->mTrans, 0)->mFrameCtrl.getEnd() * 3;
        if (mFrame < mTopFrame + anmFrame) {
            if (mFrame == mTopFrame) {
                if (mAnmCtrl->mTrans != nullptr) {
                    mAnmCtrl->mTrans->resetFlag();
                }
            }
            mFrame++;
        }
        else {
            if (mAnmCtrl->mTrans != nullptr) {
                mAnmCtrl->mTrans->setFlag();
            }
            setState(2);
        }
        if (mFrame == mTopFrame + anmFrame - 6) {
            mBtpAnmObj.mFrameCtrl.setRate(sBtpPlyFrameRate);
        }
    }
}

void TMapObjDossun::initFunc_Down()
{
    mVel.set(scDownVelocity0);
    JGeometry::TVec3f offset(0.0f, (f32)-mHeight, 0.0f);
    mFreeMove.setMaxSpeed(30.0f);
    mFreeMove.setTargetOffset(offset, 10.0f, 200.0f);
}

void TMapObjDossun::doFunc_Down()
{
    if (!mFreeMove.hasTarget()) {
        shakeCamera();
        makeGroundEmitter();
        mSoundMgr->setSe(0x40033);
        setState(3);
    }
}

void TMapObjDossun::makeGroundEmitter()
{
    JPEffectMgr::getEffectMgr()->createEmt("mk_dossun_a", mPos);
    JPEffectMgr::getEffectMgr()->createEmt("mk_dossun_b", mPos);
    JPEffectMgr::getEffectMgr()->createEmt("mk_dossun_c", mPos);
}

void TMapObjDossun::initFunc_Bottom()
{
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->resetFlag();
    }
    mFrame = 0;
}

void TMapObjDossun::doFunc_Bottom()
{
    if (mFrame < mBottomFrame) {
        u32 end = getTransObj(mAnmCtrl->mTrans, 0)->mFrameCtrl.getEnd();
        if (mFrame == end) {
            if (mAnmCtrl->mTrans != nullptr) {
                mAnmCtrl->mTrans->setFlag();
            }
        }
        mFrame++;
    }
    else {
        setState(4);
    }
}

void TMapObjDossun::initFunc_Die()
{
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    mFreeMove.releaseTarget();
    mVel.set(sDieVelocity0);
    mBtpAnmObj.mFrameCtrl.setFrame(mBtpAnmObj.mFrameCtrl.getEnd());
}

void TMapObjDossun::doFunc_Die()
{
    mVel.y -= sDieGravityY;
    mPos.add(mVel);
    if (mPos.y > mObjData->position.y) {
        mPos.y = mObjData->position.y;
        mVel.zero();
        setObjFlagCheckItemHitting();
        setAllCheckKartHitFlag();
        setState(1);
    }
}

void TMapObjDossun::shakeCamera()
{
    ObjUtility::executeShakeCamera(mPos, 10000.0f, 0.4f, 2000.0f);
}

void TMapObjDossun::moveShadow(JGeometry::TVec3f &pos)
{
    mPos.set(pos);
    mVel.y = 0.0f;
    moveShadowModel();
}


void TMapObjDossun::calc()
{
    ExecuteState();
    mFreeMove.update();
    if (IsPathMove() && getState() == 1) {
        mPathMove->update();
    }
    checkItemHitting();
    mBtpAnmObj.mFrameCtrl.update();
    for (int i = 0; i < RaceMgr::getManager()->getKartNumber(); i++) {
        if ((KartCtrl::getKartCtrl()->getKartBody(i)->mCarStatus & 0x200000000ull) == 0) {
            endStamp(i);
        }
    }
    frameProc();
}

u32 TMapObjDossun::getReleaseFrame()
{
    return 100;
}

void TMapObjDossun::checkItemHitting()
{
    if (tstItemHitting()) {
        u32 kind = mColItemObj->getKind();
        if (kind == 7 || kind == 8) {
            mFreeMove.releaseTarget();
            setState(5);
        }
    }
}

void TMapObjDossun::InitExec()
{
    Observer_FindAndInit(TMapObjDossun, 6);
}

void TMapObjDossun::MoveExec()
{
    Observer_FindAndExec(TMapObjDossun, 6);
}
