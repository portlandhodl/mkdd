#include "Sato/Objects/MapObjSanbo.h"

#include "JSystem/J3D/J3DModel.h"
#include "JSystem/JGeometry/Matrix.h"
#include "JSystem/JGeometry/Vec.h"
#include "JSystem/JKernel/JKRDisposer.h"
#include "JSystem/JMath/JMath.h"
#include "JSystem/JSupport/JSUList.h"
#include "Osako/ResMgr.h"
#include "Sato/AnmController.h"
#include "Sato/GeographyObj.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ItemThunder.h"
#include "Sato/ObjCollision.h"
#include "Shiraiwa/AnmPlayer.h"
#include "Shiraiwa/SiUtil.h"
#include "Yamamoto/kartCtrl.h"
#include "mathHelper.h"

class TMapObjSanbo;
class TSanboSupervisor;

class TSanboSupervisor : public GeoObjSupervisor
{
public:
    TSanboSupervisor();
    virtual ~TSanboSupervisor() {}
    virtual void reset() {}
    virtual void calc() {}
    virtual void thunderDownAll();

    void entry(TMapObjSanbo *obj);

    JSUList<TMapObjSanbo> mList;
};

class TMapObjSanbo : public TMapObjHioNode, public StateObserver, public JKRDisposer
{
public:
    TMapObjSanbo(const CrsData::SObject &);
    virtual ~TMapObjSanbo();

    virtual const char *getBmdFileName() { return "/Objects/Sanbo1.bmd"; }
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void createColModel(J3DModelData *);
    virtual void reset();
    virtual void calc();
    virtual void MoveExec();
    virtual void InitExec();
    virtual void doKartColCallBack(int);
    virtual GeoObjSupervisor *getSupervisor() { return sSupervisor; }
#ifndef __MWERKS__
    virtual GeoObjSupervisor *getSupervisorVirtual() { return sSupervisor; }
#endif

    void checkItemHitting();
    void moveColOffset();
    void setBurst();
    void getJointPos(JGeometry::TVec3f *, JGeometry::TVec3f *, long);
    void changeAllState(u16);
    void thunderDown()
    {
        mHideFrame = mObjData->mParam4 + sThunderDownBonus;
        setState(7);
        if ((s8)sAnmTable[7] != mAnmPlayer.getCurAnmNumber()) {
            mAnmPlayer._11 = sAnmTable[7];
            mAnmPlayer._10 = 1;
            mAnmPlayer._e |= 1;
        }
    }

    void initFunc_Start();
    void doFunc_Start();
    void initFunc_Stand();
    void doFunc_Stand();
    void initFunc_Prepare();
    void doFunc_Prepare();
    void initFunc_Attack();
    void doFunc_Attack();
    void initFunc_Down();
    void doFunc_Down();
    void initFunc_Recover();
    void doFunc_Recover();
    void initFunc_Die();
    void doFunc_Die();
    void initFunc_Hide();
    void doFunc_Hide();
    void initFunc_Born();
    void doFunc_Born();

    static TSanboSupervisor *sSupervisor;
    static TAnmInfo sAnmInfos[4];
    static StateObserver::StateFuncSet<TMapObjSanbo> sTable[10];
    static const u8 sAnmTable[10];
    static const s16 scAppearSoundFrame[5];
    static const char *scJointName[5];
    static int sJointNum[5];
    static f32 sTailOffset;
    static f32 sHeadOffset;
    static s16 sThunderDownBonus;

    JSULink<TMapObjSanbo> mLink;   // 170
    TAnmPlayer mAnmPlayer;         // 180
    u8 mIsFirstFrame;              // 198
    u8 _199;                       // 199
    s16 mStandFrame;               // 19a
    s16 mStartFrame;               // 19c
    s16 mDownFrame;                // 19e
    s16 mHideFrame;                // 1a0
    JGeometry::TPos3f mInvMtx;     // 1a4
    TMapObjSanboParts *mParts[5];  // 1d4
    u8 _1e8[0x284 - 0x1e8];        // 1e8
}; // Size: 0x284

TAnmInfo TMapObjSanbo::sAnmInfos[4] = {
    { "/Objects/Sanbo1_wait.bca", nullptr, nullptr, 2, 0, 0, -1, 0 },
    { "/Objects/Sanbo1_fall_left.bca", nullptr, nullptr, 0, 0, 0, -1, 0 },
    { "/Objects/Sanbo1_fall_right.bca", nullptr, nullptr, 0, 0, 0, -1, 0 },
    { "/Objects/Sanbo1_revive.bca", nullptr, nullptr, 0, 15, 0, 0, 0 },
};

StateObserver::StateFuncSet<TMapObjSanbo> TMapObjSanbo::sTable[10] = {
    { 0, &TMapObjSanbo::initFunc_Start, &TMapObjSanbo::doFunc_Start },
    { 1, &TMapObjSanbo::initFunc_Stand, &TMapObjSanbo::doFunc_Stand },
    { 2, &TMapObjSanbo::initFunc_Prepare, &TMapObjSanbo::doFunc_Prepare },
    { 3, &TMapObjSanbo::initFunc_Attack, &TMapObjSanbo::doFunc_Attack },
    { 4, &TMapObjSanbo::initFunc_Attack, &TMapObjSanbo::doFunc_Attack },
    { 5, &TMapObjSanbo::initFunc_Down, &TMapObjSanbo::doFunc_Down },
    { 6, &TMapObjSanbo::initFunc_Recover, &TMapObjSanbo::doFunc_Recover },
    { 7, &TMapObjSanbo::initFunc_Die, &TMapObjSanbo::doFunc_Die },
    { 8, &TMapObjSanbo::initFunc_Hide, &TMapObjSanbo::doFunc_Hide },
    { 9, &TMapObjSanbo::initFunc_Born, &TMapObjSanbo::doFunc_Born },
};

const u8 TMapObjSanbo::sAnmTable[10] = { 0, 0, 0, 1, 2, 1, 1, 1, 1, 3 };
const s16 TMapObjSanbo::scAppearSoundFrame[5] = { 5, 19, 33, 47, 61 };
const char *TMapObjSanbo::scJointName[5] = { "joint5", "joint4", "joint3", "joint2", "joint1" };
int TMapObjSanbo::sJointNum[5] = { -1, -1, -1, -1, -1 };
f32 TMapObjSanbo::sTailOffset = -60.0f;
f32 TMapObjSanbo::sHeadOffset = 60.0f;
s16 TMapObjSanbo::sThunderDownBonus = 120;

TSanboSupervisor *TMapObjSanbo::sSupervisor;

TMapObjSanbo::TMapObjSanbo(const CrsData::SObject &obj)
    : TMapObjHioNode(obj), StateObserver(), JKRDisposer(), mLink(this), mAnmPlayer()
{
    NewAnmCtrl();
    createSoundMgr();
    TAnmPlayer::resetAnimations(sAnmInfos, 4);

    mParts[0] = (TMapObjSanboParts *)GeographyObjManager::gMgr->createSubObj(0x138d);
    for (int i = 1; i < 5; i++) {
        mParts[i] = (TMapObjSanboParts *)GeographyObjManager::gMgr->createSubObj(0x138e);
    }

    if (getSupervisor() == nullptr) {
        sSupervisor = new TSanboSupervisor();
    }
    sSupervisor->entry(this);
}

TMapObjSanbo::~TMapObjSanbo() { sSupervisor = nullptr; }

void TMapObjSanbo::reset()
{
    resetObject();
    ResetState();
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    _58 = 4;
    mIsFirstFrame = 1;
    _199 = 1;
    mAnmPlayer.init(mAnmCtrl, sAnmInfos, 4);
    mStandFrame = mObjData->mParam1;
    mStartFrame = mObjData->mParam2;
    mDownFrame = mObjData->mParam3;
    mHideFrame = 0;
    PSMTXInverse(mRotMtx, mInvMtx);
    Mtx scaleMtx;
    PSMTXScale(scaleMtx, 1.0f / mScale.x, 1.0f / mScale.y, 1.0f / mScale.z);
    PSMTXConcat(mInvMtx, scaleMtx, mInvMtx);
    for (int i = 0; i < 5; i++) {
        mParts[i]->mBaseScale.set(mScale);
    }
}

void TMapObjSanbo::calc()
{
    ExecuteState();
    checkItemHitting();
    moveColOffset();
    mAnmPlayer.update();
    mIsFirstFrame = 0;
}

void TMapObjSanbo::loadAnimation()
{
    TAnmPlayer::loadAnimations(sAnmInfos, 4, mModel.getModelData(), ResMgr::mcArcCourse);
}

void TMapObjSanbo::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createModel(heap, viewNo, 0);
    TAnmPlayer::registAnimations(mAnmCtrl, &mModel, sAnmInfos, 4);
    for (int i = 0; i < 5; i++) {
        sJointNum[i] = mModel.getModelData()->getJointName()->getIndex(scJointName[i]);
    }
}

void TMapObjSanbo::createColModel(J3DModelData *)
{
    f32 r = mScale.x > mScale.z ? mScale.x : mScale.z;
    r = r > mScale.y ? r : mScale.y;
    createMultiBoundsPtrAll(2);
    u8 idx;
    for (int i = 0; (u8)i < mBoundsNum; i++) {
        idx = i;
        createMultiBoundsSphere(idx, 64.0f, r);
        JGeometry::TVec3f zero(0.0f, 0.0f, 0.0f);
        setColObjPos(zero, idx);
    }
}

void TMapObjSanbo::InitExec()
{
    Observer_FindAndInit(TMapObjSanbo, 10);
}

void TMapObjSanbo::MoveExec()
{
    Observer_FindAndExec(TMapObjSanbo, 10);
}

void TMapObjSanbo::changeAllState(u16 state)
{
    setState((u16)state);
    mAnmPlayer.setAnm(sAnmTable[state]);
}

void TMapObjSanbo::checkItemHitting()
{
    if (mGeoObjFlag & 4) {
        switch (mColItemObj->mItemKind) {
        case 1:
        case 7:
        case 8:
        case 9:
            mHideFrame = mObjData->mParam4;
            setState(7);
            break;
        }
    }
}

void TMapObjSanbo::doKartColCallBack(int kart)
{
    if (KartCtrl::getKartCtrl()->GetKartStatus(kart) & 0x40000) {
        mHideFrame = mObjData->mParam4;
        setState(7);
    }
}

void TMapObjSanbo::initFunc_Start()
{
    mGeoObjFlag &= ~0x20;
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
}

void TMapObjSanbo::doFunc_Start()
{
    if (getStateCount() > mStartFrame) {
        changeAllState(1);
    }
}

void TMapObjSanbo::initFunc_Stand()
{
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
}

void TMapObjSanbo::doFunc_Stand()
{
    if (getStateCount() > mStandFrame) {
        changeAllState(2);
    }
}

void TMapObjSanbo::moveColOffset()
{
    JGeometry::TVec3f headPos;
    JGeometry::TVec3f tailPos;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f up;
    getJointPos(&headPos, nullptr, sJointNum[3]);
    getJointPos(&tailPos, &up, sJointNum[0]);
    dir.sub(headPos, mPos);
    dir.normalize();
    dir.scale(sTailOffset * mScale.y);
    headPos.add(dir);
    up.scale(sHeadOffset * mScale.y);
    tailPos.add(up);
    headPos.sub(mPos);
    tailPos.sub(mPos);
    PSMTXMultVecSR(mInvMtx, &headPos, &headPos);
    PSMTXMultVecSR(mInvMtx, &tailPos, &tailPos);
    setColObjPos(headPos, 0);
    setColObjPos(tailPos, 1);
}

void TMapObjSanbo::initFunc_Prepare() {}

void TMapObjSanbo::doFunc_Prepare()
{
    if (mAnmCtrl->getFrameCtrl(0)->getFrame() <= 1.0f) {
        if (getGeoRnd()->get_ufloat_1() < 0.5f) {
            setState(3);
            mAnmPlayer.setAnm(sAnmTable[3]);
        } else {
            setState(4);
            mAnmPlayer.setAnm(sAnmTable[4]);
        }
    }
}

void TMapObjSanbo::initFunc_Attack()
{
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->mFlags &= ~1;
    }
}

void TMapObjSanbo::doFunc_Attack()
{
    if (mAnmPlayer.mController->getCurFrameCtrl()->getFrame() > 35.0f) {
        setState(5);
    }
}

void TMapObjSanbo::initFunc_Down()
{
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->mFlags |= 1;
    }
}

void TMapObjSanbo::doFunc_Down()
{
    if (getStateCount() > mDownFrame) {
        setState(6);
    }
}

void TMapObjSanbo::initFunc_Recover()
{
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->mFlags &= ~1;
    }
}

void TMapObjSanbo::doFunc_Recover()
{
    if (mAnmPlayer.mController->getCurFrameCtrl()->checkState(1)) {
        changeAllState(1);
    }
}

void TMapObjSanbo::initFunc_Die()
{
    setBurst();
    mGeoObjFlag |= 0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
}

void TMapObjSanbo::doFunc_Die()
{
    for (int i = 0; i < 5; i++) {
        if (!(mParts[i]->mGeoObjFlag & 0x20)) {
            return;
        }
    }
    changeAllState(8);
}

void TMapObjSanbo::initFunc_Hide() {}

void TMapObjSanbo::doFunc_Hide()
{
    if (getStateCount() > mHideFrame) {
        changeAllState(9);
    }
}

void TMapObjSanbo::initFunc_Born()
{
    mGeoObjFlag &= ~0x20;
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->mFlags &= ~1;
    }
}

void TMapObjSanbo::doFunc_Born()
{
    if (mAnmPlayer.isCurAnmReachEnd()) {
        changeAllState(1);
    }
    for (int i = 0; i < 5; i++) {
        J3DFrameCtrl *ctrl = mAnmPlayer.mController->getCurFrameCtrl();
        if (SiUtil::checkPassFrame(scAppearSoundFrame[i], ctrl->getFrame(), ctrl->getRate())) {
            mSoundMgr->setSe(0x4005b);
        }
    }
}

void TMapObjSanbo::setBurst()
{
    for (int i = 0; i < 5; i++) {
        JGeometry::TPos3f mtx;
        mtx.ref(2, 3) = 0.0f;
        mtx.ref(1, 3) = 0.0f;
        mtx.ref(0, 3) = 0.0f;
        mtx.ref(2, 2) = 0.0f;
        mtx.ref(1, 2) = 0.0f;
        mtx.ref(0, 2) = 0.0f;
        mtx.ref(2, 1) = 0.0f;
        mtx.ref(1, 1) = 0.0f;
        mtx.ref(0, 1) = 0.0f;
        mtx.ref(2, 0) = 0.0f;
        mtx.ref(1, 0) = 0.0f;
        mtx.ref(0, 0) = 0.0f;
        JGeometry::TVec3f pos;
        JGeometry::TVec3f xDir;
        JGeometry::TVec3f yDir;
        JGeometry::TVec3f zDir;
        getJointPos(&pos, &yDir, sJointNum[i]);
        yDir.normalize();
        pos.scaleAdd(55.0f, yDir, pos);
        mRotMtx.getXDir(xDir);
        xDir.normalize();
        zDir.cross(xDir, yDir);
        zDir.normalize();
        xDir.cross(yDir, zDir);
        xDir.normalize();
        mtx.setXYZDir(xDir, yDir, zDir);
        mParts[i]->burst(pos, mtx);
    }
}

void TMapObjSanbo::getJointPos(JGeometry::TVec3f *pos, JGeometry::TVec3f *dir, long jointNo)
{
    if (mIsFirstFrame) {
        pos->set(mPos);
        if (dir != nullptr) {
            dir->set(0.0f, 1.0f, 0.0f);
        }
    } else {
        Mtx &mtx = mModel.getModel()->getAnmMtx(jointNo);
        pos->set(mtx[0][3], mtx[1][3], mtx[2][3]);
        if (dir != nullptr) {
            dir->set(mtx[0][0], mtx[1][0], mtx[2][0]);
            dir->normalize();
        }
    }
}

TSanboSupervisor::TSanboSupervisor()
{
    mList.initiate();
    ItemThunder::entrySuperVisor(this);
}

void TSanboSupervisor::entry(TMapObjSanbo *obj) { mList.append(&obj->mLink); }

void TSanboSupervisor::thunderDownAll()
{
    for (JSUListIterator<TMapObjSanbo> it(mList.getFirst()); it != mList.getEnd(); it++) {
        it->thunderDown();
    }
}

StateObserver::StateFuncSet<TMapObjSanboParts> TMapObjSanboParts::sTable[2] = {
    { 0, &TMapObjSanboParts::initFunc_Hide, &TMapObjSanboParts::doFunc_Hide },
    { 1, &TMapObjSanboParts::initFunc_Burst, &TMapObjSanboParts::doFunc_Burst },
};

f32 TMapObjSanboParts::sGravity = 1.5f;
f32 TMapObjSanboParts::sAirFric = 0.999f;
f32 TMapObjSanboParts::sBurstVelH = 25.0f;
f32 TMapObjSanboParts::sBurstVelV = 50.0f;
f32 TMapObjSanboParts::sRotRad = 0.3f;

void TMapObjSanboParts::reset()
{
    resetObject();
    mGeoObjFlag |= 0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    PSMTXIdentity((Mtx &)mRotVel);
    mBaseScale.zero();
    StateObserver::ResetState();
}

void TMapObjSanboParts::calc()
{
    StateObserver::ExecuteState();
}

void TMapObjSanboParts::initFunc_Hide()
{
    mGeoObjFlag |= 0x20;
}

void TMapObjSanboParts::doFunc_Hide() {}

void TMapObjSanboParts::initFunc_Burst()
{
    mScale.x = mBaseScale.x;
    mScale.y = mBaseScale.y;
    mScale.z = mBaseScale.z;
}

void TMapObjSanboParts::doFunc_Burst()
{
    mVel.scale(sAirFric);
    mVel.y -= sGravity;
    mPos.add(mVel);
    PSMTXConcat(mRotMtx, mRotVel, mRotMtx);
    if (acceptHidding()) {
        setState(0);
    }
}

// address-taken temp (unlike get_ufloat_1's union) keeps the conversion from sinking past the next call
static inline f32 getRandFloat(stRandom *rnd)
{
    u32 bits = (rnd->get() >> 9) | 0x3f800000;
    return *(f32 *)&bits - 1.0f;
}

void TMapObjSanboParts::burst(const JGeometry::TVec3f &pos, const JGeometry::TPos3f &mtx)
{
    mPos.set(pos);
    mRotMtx.set(mtx);
    s16 angle = (getRandFloat(getGeoRnd()) * 2.0f - 1.0f) * 3.1415927f * 10430.378f;
    mVel.set(sBurstVelH * JMASCos(angle), sBurstVelV, sBurstVelH * JMASSin(angle));
    s16 angleY = (getRandFloat(getGeoRnd()) * 2.0f - 1.0f) * 3.1415927f * 10430.378f;
    s16 angleX = (getRandFloat(getGeoRnd()) * 2.0f - 1.0f) * 3.1415927f * 10430.378f;
    f32 x = JMASCos(angleY);
    f32 z = JMASSin(angleY);
    JGeometry::TVec3f axis(x, JMASSin(angleX), z);
    axis.normalize();
    mRotVel.zeroTrans();
    mRotVel.setRotate(axis, sRotRad);
    mGeoObjFlag &= ~0x20;
    setState(1);
}

void TMapObjSanboParts::InitExec()
{
    Observer_FindAndInit(TMapObjSanboParts, 2);
}

void TMapObjSanboParts::MoveExec()
{
    Observer_FindAndExec(TMapObjSanboParts, 2);
}

bool TMapObjSanboHead::acceptHidding()
{
    if (mPos.y + 120.0f < SiUtil::getCourseHeight(mPos)) return true;
    return false;
}

bool TMapObjSanboBody::acceptHidding()
{
    if (mPos.y + 80.0f < SiUtil::getCourseHeight(mPos)) return true;
    return false;
}

#include "JSystem/JAudio/JASFakeMatch2.h"
