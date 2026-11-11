#include "macros.h"
#include "Shiraiwa/Objects/MapObjGeyser.h"


#include "JSystem/JGeometry/Util.h"
#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Shiraiwa/Objects/MapObjGeyser.h"

const char *TMapObjGeyser::scEffectName[6] = {
    "mk_kanketsusen_a",
    "mk_kanketsusen_b",
    "mk_kanketsusen_c",
    "mk_kanketsusen_d",
    "mk_kanketsusen_e",
    "mk_kanketsusen_f",
};

TAnmInfo TMapObjGeyser::sAnmInfos[4] = {
    { "/Objects/Geyser11.bca", nullptr, nullptr, 0, 0, 1, -1, 0 },
    { "/Objects/Geyser12.bca", nullptr, nullptr, 0, 0, 1, -1, 0 },
    { "/Objects/Geyser13.bca", nullptr, nullptr, 0, 0, 1, -1, 0 },
    { "/Objects/Geyser14.bca", nullptr, nullptr, 0, 0, 1, -1, 0 },
};

const u8 TMapObjGeyser::sAnmTable[6] = { 5, 5, 0, 1, 2, 3 };

s16 TMapObjGeyser::sActTime = 120;
f32 TMapObjGeyser::sJumpUpStartVel = 100.0f;
u16 TMapObjGeyser::sCheckCycle = 30;
s16 TMapObjGeyser::sEfctABCDEndFrame = 25;
f32 TMapObjGeyser::sBoundHeight = 2900.0f;
const char *TMapObjGeyser::scJointName = "ef_set";

s16 TMapObjGeyser::sEfctEFEndFrame = sActTime - 25;
int TMapObjGeyser::sJointNum;
J3DAnmTextureSRTKey *TMapObjGeyser::sGeyserBtkAnm;

u16 TMapObjGeyserSearch::sCheckCycle = 30;

const StateObserver::StateFuncSet<TMapObjGeyser> TMapObjGeyser::sTable[6] = {
    { 0, &TMapObjGeyser::initFunc_Start, &TMapObjGeyser::doFunc_Start },
    { 1, &TMapObjGeyser::initFunc_Wait, &TMapObjGeyser::doFunc_Wait },
    { 2, &TMapObjGeyser::initFunc_Act, &TMapObjGeyser::doFunc_Act },
    { 3, &TMapObjGeyser::initFunc_JumpUp, &TMapObjGeyser::doFunc_JumpUp },
    { 4, &TMapObjGeyser::initFunc_JumpStay, &TMapObjGeyser::doFunc_JumpStay },
    { 5, &TMapObjGeyser::initFunc_JumpDown, &TMapObjGeyser::doFunc_JumpDown },
};

static const f32 sDummy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
#include <JSystem/JAudio/JASFakeMatch2.h>


TMapObjGeyser::TMapObjGeyser(const CrsData::SObject &obj)
    : TKartThrower(obj), StateObserver(), mBtk(), mAnmPlayer()
{
    NewAnmCtrl();
    createSoundMgr();
    TAnmPlayer::resetAnimations(sAnmInfos, 4);
    mColPos.zero();
}

TMapObjGeyser::~TMapObjGeyser()
{
}

void TMapObjGeyser::InitExec()
{
    Observer_FindAndInit(TMapObjGeyser, 6);
}

void TMapObjGeyser::MoveExec()
{
    Observer_FindAndExec(TMapObjGeyser, 6);
}

const char *TMapObjGeyser::getBmdFileName()
{
    static const char *cBmdName = "/Objects/Geyser1.bmd";
    return cBmdName;
}

void TMapObjGeyser::createColModel(J3DModelData *modelData)
{
    f32 maxXZ = mScale.x > mScale.z ? mScale.x : mScale.z;
    f32 max = maxXZ > mScale.y ? maxXZ : mScale.y;
    createBoundsCylinder(modelData, maxXZ / max * 140.0f, mScale.y / max * sBoundHeight);
}

void TMapObjGeyser::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x200, 0);
    TAnmPlayer::registAnimations(getAnmCtrl(), &mModel, sAnmInfos, 4);
    sJointNum = mModel.getModelData()->getJointName()->getIndex(scJointName);
    mBtk.setExModel(&mModel);
    mBtk.mAnmBase = sGeyserBtkAnm;
    mBtk.initFrameCtrl(mBtk.mAnmBase);
}

void TMapObjGeyser::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    TAnmPlayer::loadAnimations(sAnmInfos, 4, modelData, (ResMgr::ArchiveId)3);
    void *btk = ObjUtility::getPtrCourseArc("/Objects/Geyser1.btk");
    J3DAnmObjMaterial::setupTexSRTAnmData(&sGeyserBtkAnm, modelData, btk);
}







































static inline bool isEmitterEven(u32 id)
{
    bool ret = true;
    switch ((s32)id) {
    case 1:
    case 3:
    case 5:
        ret = false;
        break;
    }
    return ret;
}

inline JPABaseEmitter *TMapObjGeyser::getEmitterImpl(u32 id)
{
    JUT_MINMAX_ASSERT(0, id, 6);

    if (mEmitters[id] == nullptr && isEmitterEven(id)) {
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(scEffectName[id], mPos);
        JUT_ASSERT_MSG(emt != nullptr, "emt != 0");
        emt->setScale(mScale.x > mScale.z ? mScale.x : mScale.z);
        emt->setStatus(1);
        mEmitters[id] = emt;
    }
    return mEmitters[id];
}

JPABaseEmitter *TMapObjGeyser::getEmitter(u32 id)
{
    return getEmitterImpl(id);
}

bool TMapObjGeyser::isActiveEmitter(u32 id)
{
    JUT_MINMAX_ASSERT(0, id, 6);
    return mEmitters[id] != nullptr;
}

void TMapObjGeyser::playEmitter(u32 id, const JGeometry::TVec3f &pos)
{
    if (isEmitterEven(id)) {
        getEmitterImpl(id);

        mEmitters[id]->clearStatus(1);
        moveEmitter(id, pos);
    }
}

void TMapObjGeyser::moveEmitter(u32 id, const JGeometry::TVec3f &pos)
{
    if (isEmitterEven(id)) {
        getEmitterImpl(id);

        mEmitters[id]->mGlobalTrs.set(pos);
    }
}

void TMapObjGeyser::stopEmitter(u32 id)
{
    if (isEmitterEven(id)) {
        getEmitterImpl(id);
        mEmitters[id]->clearStatus(1);
    }
}

void TMapObjGeyser::hideEmitter(u32 id)
{
    if (isEmitterEven(id)) {
        getEmitterImpl(id);
        mEmitters[id]->clearStatus(2);
    }
}

void TMapObjGeyser::showEmitter(u32 id)
{
    if (isEmitterEven(id)) {
        getEmitterImpl(id);
        mEmitters[id]->setStatus(2);
    }
}

void TMapObjGeyser::getEmitterPos(JGeometry::TVec3f *pos)
{
    JPABaseEmitter *emt = mEmitters[4];
    pos->set(emt->mGlobalTrs.x, emt->mGlobalTrs.y, emt->mGlobalTrs.z);
}

void TMapObjGeyser::update()
{
    mBtk.anmFrameProc();
    GeographyObj::update();
}

void TMapObjGeyser::reset()
{
    resetObject();
    _58 = 0;
    mAnmPlayer.init(getAnmCtrl(), sAnmInfos, 4);

    mBtk.mFrameCtrl.setFrame((f32)mBtk.mFrameCtrl.mStartFrame);
    mBtk.mFrameCtrl.setRate(0.0f);
    mBtk.mFrameCtrl.mState = 0;

    mActTime = mObjData->mParam1;
    if (mActTime == 0)
        mActTime = 300;
    mAngle = (s16)(mObjData->mParam4 * 182.04445f);
    mUnk198 = mScale.y * 1000.0f;
    mCycle = 0;

    ResetState();

    if (RaceMgr::sRaceManager->mRaceInfo->mRaceMode != 1 && mObjData->mParam8 != 0)
        setState(1);
    else
        setState(0);

    for (int i = 0; i < 6; i++)
        mEmitters[i] = nullptr;

    f32 bound = -sBoundHeight;
    mColHeight = bound;
    mColPos.zero();
}

void TMapObjGeyser::changeAllState(u16 state)
{
    setState(state);
    u32 anm = (s8)sAnmTable[state];
    if (anm != 5) {
        if (anm != mAnmPlayer.mController->mTrans->mNowAnmNo) {
            mAnmPlayer._11 = sAnmTable[state];
            mAnmPlayer._10 = 1;
            mAnmPlayer._e |= 1;
        }
    }
}

void TMapObjGeyser::calc()
{
    u8 *modelData = *(u8 **)(*(u8 **)((u8 *)mModel.getModel() + 0x84) + 0xc);
    f32 *mtx = (f32 *)(modelData + sJointNum * 0x30);
    mColPos.set(*(f32 *)((u8 *)mtx + 0xc), *(f32 *)((u8 *)mtx + 0x1c), *(f32 *)((u8 *)mtx + 0x2c));

    ExecuteState();
    mAnmPlayer.update();
    mBtk.mFrameCtrl.update();

    JGeometry::TVec3f pos;
    pos.set(0.0f, (mColPos.y - mPos.y) / mScale.y + mColHeight, 0.0f);
    setColObjPos(pos, 0);
}

void TMapObjGeyser::viewCalc(u32 viewNo)
{
    mModel.viewCalc(viewNo);
}

void TMapObjGeyser::getThrowDir(JGeometry::TVec3f *dir, s16 angle)
{
    JGeometry::TVec3f vec;
    if (angle == -1) {
        vec.set(0.0f, 1.0f, 0.0f);
    } else {
        ObjUtility::getKartVel(angle, &vec);
        vec.y = 0.0f;
        if (vec.squared() <= JGeometry::TUtilf::epsilon()) {
            ObjUtility::getKartZdir(angle, &vec);
            vec.y = 0.0f;
            if (vec.squared() <= JGeometry::TUtilf::epsilon()) {
                vec.set(0.0f, 0.0f, 1.0f);
            }
        }
        vec.normalize();

        vec.scale(JMath::sincosTable_.cosShort(mAngle));
        vec.y = JMath::sincosTable_.sinShort(mAngle);
    }
    dir->set(vec);
}

s32 TMapObjGeyser::getThrowPow()
{
    return mObjData->mParam3;
}

void TMapObjGeyser::getKartThrowDirPow(JGeometry::TVec3f *dir, f32 *pow, int kartNo)
{
    if (dir != nullptr)
        getThrowDir(dir, kartNo);
    if (pow != nullptr)
        *pow = (f32)getThrowPow();
}

void TMapObjGeyser::getItemThrowDirPow(JGeometry::TVec3f *dir, f32 *pow, const ItemObj &item)
{
    JGeometry::TVec3f ipos;
    JGeometry::TVec3f vec;
    ipos.set(item.mPos);
    vec.sub(ipos, mPos);
    vec.y = 0.0f;
    vec.normalize();

    vec.scale(JMath::sincosTable_.cosShort(mAngle));
    vec.y = JMath::sincosTable_.sinShort(mAngle);
    dir->set(vec);

    *pow = mObjData->mParam3 * 0.1f;
}

void TMapObjGeyser::initFunc_Start()
{
    _58 = 0;
    mReaction.setFlg(0, 0);
    mReaction.setFlg(6, 0);
    mReaction.setFlg(5, 0);
    mReaction.setFlg(3, 0);
}

void TMapObjGeyser::doFunc_Start()
{
    if (getStateCount() > mObjData->mParam2) {
        changeAllState(1);
    }
}

void TMapObjGeyser::initFunc_Wait()
{
    _58 = 0;
    mReaction.setFlg(0, 0);
    mReaction.setFlg(6, 0);
    mReaction.setFlg(5, 0);
    mReaction.setFlg(3, 0);
}

void TMapObjGeyser::doFunc_Wait()
{
    if (checkChangeAct()) {
        changeAllState(2);
    }
}

bool TMapObjGeyser::checkChangeAct()
{
    bool ret;
    if (RaceMgr::sRaceManager->mRaceInfo->mRaceMode != 1 && mObjData->mParam8 != 0) {
        mCycle++;
        if (mCycle < sCheckCycle) {
            ret = false;
        } else {
            mCycle = 0;
            s16 kartNo = SiUtil::searchNearKartCircle(mPos, (f32)mObjData->mParam5);
            if (kartNo != -1)
                ret = true;
            else
                ret = false;
        }
    } else {
        ret = getStateCount() > mActTime;
    }
    return ret;
}

void TMapObjGeyser::initFunc_Act()
{
    playEmitter(4, mPos);
    playEmitter(5, mPos);
}

void TMapObjGeyser::doFunc_Act()
{
    if (getStateCount() > sActTime) {
        changeAllState(3);
    }

    if (getStateCount() == sEfctEFEndFrame) {
        JPABaseEmitter *emt = getEmitterImpl(4);
        emt->setStatus(1);
    }
}

void TMapObjGeyser::initFunc_JumpUp()
{
    _58 = 14;
    mReaction.setFlg(0, 3);
    mReaction.setFlg(6, 3);
    mReaction.setFlg(5, 3);
    mReaction.setFlg(3, 3);

    mVel.set(0.0f, sJumpUpStartVel * mScale.y, 0.0f);

    playEmitter(0, mColPos);
    playEmitter(1, mColPos);
    playEmitter(2, mPos);
    playEmitter(3, mPos);
}

void TMapObjGeyser::doFunc_JumpUp()
{
    u8 id = mAnmPlayer.getCurAnmNumber();
    if (mAnmPlayer.mController->mTrans->getObjAnt(id)->mFrameCtrl.mState & 1) {
        changeAllState(4);
    }

    getSoundMgr()->setSe(0x40047);

    getEmitterImpl(0)->mGlobalTrs.set(mColPos);
}

void TMapObjGeyser::initFunc_JumpStay()
{
}

void TMapObjGeyser::doFunc_JumpStay()
{
    u8 id = mAnmPlayer.getCurAnmNumber();
    if (mAnmPlayer.mController->mTrans->getObjAnt(id)->mFrameCtrl.mState & 1) {
        changeAllState(5);
    }

    getSoundMgr()->setSe(0x40047);

    getEmitterImpl(0)->mGlobalTrs.set(mColPos);
}

void TMapObjGeyser::initFunc_JumpDown()
{
    mVel.zero();
}

void TMapObjGeyser::doFunc_JumpDown()
{
    u8 id = mAnmPlayer.getCurAnmNumber();
    if (mAnmPlayer.mController->mTrans->getObjAnt(id)->mFrameCtrl.mState & 1) {
        changeAllState(1);
    }

    getSoundMgr()->setSe(0x40047);

    getEmitterImpl(0)->mGlobalTrs.set(mColPos);

    if (getStateCount() == sEfctABCDEndFrame) {
        getEmitterImpl(0)->setStatus(1);
        getEmitterImpl(2)->setStatus(1);
    }
}

void TMapObjGeyserSearch::reset()
{
    TMapObjGeyser::reset();
    setState(1);
    mCycle = 0;
}

bool TMapObjGeyserSearch::checkChangeAct()
{
    mCycle++;
    if (mCycle < sCheckCycle)
        return false;
    mCycle = 0;
    return SiUtil::searchNearKartCircle(mPos, (f32)mObjData->mParam1) != -1;
}

__declspec(weak) TMapObjGeyserSearch::~TMapObjGeyserSearch()
{
}
