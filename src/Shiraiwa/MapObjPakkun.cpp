#include "Shiraiwa/Objects/MapObjPakkun.h"
#include "Sato/Objects/MapObjPakkunCol.h"

#include <dolphin/types.h>
#include <dolphin/mtx.h>

#include "JSystem/J3D/J3DModel.h"
#include "JSystem/JGeometry/Matrix.h"
#include "JSystem/JMath/JMath.h"
#include "JSystem/JGeometry/Util.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/Course/CrsData.h"
#include "Inagaki/GameSoundMgr.h"
#include "JSystem/JAudio/Interface/JAISe.h"
#include "JSystem/JAudio/System/JASTrack.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/ResMgr.h"
#include "Sato/AnmController.h"
#include "Sato/GeographyObj.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ItemThunder.h"
#include "Sato/ObjUtility.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/SiUtil.h"
#include "Yamamoto/kartCtrl.h"

const u8 TMapObjPakkun::sAnmTable[5] = { 0, 0, 1, 0, 2 };

f32 TMapObjPakkun::sThunderDamageRandeMax = 30.0f;
f32 TMapObjPakkun::sTurnPow = 0.02f;
f32 TMapObjPakkun::sTurnPowFar = 0.1f;
f32 TMapObjPakkun::sTurnAttackScale = 2.0f;
s16 TMapObjPakkun::sRestFrame = 60;
s16 TMapObjPakkun::sSearchFrame = 30;
const char *TMapObjPakkun::scRootJointName = "skil_root";
const char *TMapObjPakkun::scHeadJointName = "kuki_4";
JGeometry::TVec3f TMapObjPakkun::scHeadCenterOffset(-6.0f, 0.0f, 0.0f);
TPakkunSupervisor *TMapObjPakkun::sSupervisor;

TAnmInfo TMapObjPakkun::sAnmInfos[3] = {
    { "/Objects/Pakkun_wait.bca", nullptr, nullptr, 2, 10, 0, 1, 0 },
    { "/Objects/Pakkun_attack.bca", nullptr, nullptr, 0, 10, 0, 0, 0 },
    { "/Objects/Pakkun_damage.bca", nullptr, nullptr, 2, 30, 0, 0, 0 },
};

StateObserver::StateFuncSet<TMapObjPakkun> TMapObjPakkun::sTable[5] = {
    { 0, &TMapObjPakkun::initFunc_Stand, &TMapObjPakkun::doFunc_Stand },
    { 1, &TMapObjPakkun::initFunc_Search, &TMapObjPakkun::doFunc_Search },
    { 2, &TMapObjPakkun::initFunc_Attack, &TMapObjPakkun::doFunc_Attack },
    { 3, &TMapObjPakkun::initFunc_Rest, &TMapObjPakkun::doFunc_Rest },
    { 4, &TMapObjPakkun::initFunc_Damage, &TMapObjPakkun::doFunc_Damage },
};

void TMapObjPakkunColHead::reset()
{
    resetObject();
    if (RaceMgr::sRaceManager->mRaceInfo->mRaceMode != 1) {
        mGeoObjFlag |= 2;
        mCheckKartHitFlags = -1;
        _58 = 18;
    }
    else {
        mGeoObjFlag &= ~2;
        mCheckKartHitFlags = 0;
        _58 = 0;
    }
    _14c = 0;
}
void TMapObjPakkunColHead::createColModel(J3DModelData *modelData)
{
    createBoundsSphere(1500.0f, mScale.y);
}
void TMapObjPakkunColHead::setScale(const JGeometry::TVec3f &scale)
{
    mScale.set(scale);
}

void TMapObjPakkunColHead::doKartColCallBack(int kartNo)
{
    u64 status = GetKartCtrl()->GetKartStatus(kartNo);
    if ((status & 0x40000) != 0) {
        _14c = 1;
    }
}
bool TMapObjPakkunColHead::isDamage()
{
    return _14c != 0;
}

void TMapObjPakkunColHead::calc()
{
    checkItemHitting();
    moveShadowModel();
}
void TMapObjPakkunColHead::setPosition(const JGeometry::TVec3f &pos)
{
    mPos.set(pos);
}

void TMapObjPakkunColHead::getKartThrowDirPow(JGeometry::TVec3f *dir, f32 *pow, int kartNo)
{
#line 151
    JUT_ASSERT(mPakkun != nullptr);
    if (dir == nullptr) {
        return;
    }
    dir->sub(mPos, mPakkun->mPos);
    dir->y = 0.0f;
    if (dir->squared() <= JGeometry::TUtilf::epsilon()) {
        *dir = mVel;
        dir->y = 0.0f;
        if (dir->squared() <= JGeometry::TUtilf::epsilon()) {
            dir->set(1.0f, 0.0f, 0.0f);
        }
    }
    dir->normalize();
}
void TMapObjPakkunColHead::checkItemHitting()
{
    if (!(mGeoObjFlag & 4)) {
        return;
    }
    int kind = mColItemObj->mItemKind;
    if (kind == 10) {
        return;
    }
    if (kind < 10) {
        if (kind >= 3) {
            if (kind >= 7) {
                return;
            }
        }
        else if (kind < 0) {
            return;
        }
    }
    else if (kind >= 12) {
        return;
    }
    _14c = 1;
}
TMapObjPakkun::TMapObjPakkun(const CrsData::SObject &obj)
    : TMapObjHioNode(obj), StateObserver(), JKRDisposer(), mLink(this), mAnmPlayer()
{
    NewAnmCtrl();
    createSoundMgr();
    TAnmPlayer::resetAnimations(sAnmInfos, 3);
    if (getSupervisor() == nullptr) {
        sSupervisor = new TPakkunSupervisor();
    }
    sSupervisor->entry(this);
    mHead = (TMapObjPakkunColHead *)GeographyObjManager::gMgr->createGenSubObj(0xe81);
    mHead->mPakkun = this;
    mBaseMtx.set(mRotMtx);
}
TMapObjPakkun::~TMapObjPakkun()
{
    sSupervisor = nullptr;
}
void TMapObjPakkun::reset()
{
    resetObject();
    StateObserver::ResetState();
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    _58 = 1;
    mAnmCtrl->Reset();
    mAnmPlayer.init(mAnmCtrl, sAnmInfos, 3);
    SiUtil::setRandomStartFrame(mAnmCtrl->mTrans->getFrameCtrl(0), getGeoRnd());

    mDamageFrame = mObjData->mParam1;
    if (mDamageFrame == 0) {
        mDamageFrame = 300;
    }
    f32 search = (f32)mObjData->mParam2;
    if (search <= 0.0f) {
        search = 2500.0f;
    }
    mSearchRadius = search;
    f32 attack = (f32)mObjData->mParam3;
    if (attack <= 0.0f) {
        attack = 1500.0f;
    }
    mAttackRadius = attack;

    mDir.set(mRotMtx[0][2], mRotMtx[1][2], mRotMtx[2][2]);
    mBaseMtx.set(mRotMtx);
    PSMTXIdentity((Mtx &)mInvBaseMtx);
    PSMTXInverse((Mtx &)mRotMtx, (Mtx &)mInvBaseMtx);
    PSMTXConcat((Mtx &)mInvBaseMtx, (Mtx &)mBaseMtx, (Mtx &)mBaseMtx);

    mTargetKart = -1;
    mDamageAdd = 0;
    mSearchTimer = 0;

    Mtx scale;
    PSMTXIdentity(scale);
    PSMTXInverse((Mtx &)mRotMtx, (Mtx &)mInvScaleMtx);
    PSMTXScale(scale, 1.0f / mScale.x, 1.0f / mScale.y, 1.0f / mScale.z);
    PSMTXConcat((Mtx &)mInvScaleMtx, scale, (Mtx &)mInvScaleMtx);

    TMapObjPakkunColHead *head = mHead;
    head->mScale.x = mScale.x;
    head->mScale.y = mScale.y;
    head->mScale.z = mScale.z;
    head->getBounds(0)->setScale(mScale);
}
void TMapObjPakkun::calc()
{
    StateObserver::ExecuteState();
    mAnmPlayer.update();
}
void TMapObjPakkun::loadAnimation()
{
    TAnmPlayer::loadAnimations(sAnmInfos, 3, mModel.getModelData(), ResMgr::mcArcCourse);
}
void TMapObjPakkun::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createModel(heap, viewNo, 0);
    TAnmPlayer::registAnimations(mAnmCtrl, &mModel, sAnmInfos, 3);
    setCallBack(scRootJointName, rotCallBack);
    if (RaceMgr::sRaceManager->mRaceInfo->mRaceMode != 1) {
        setCallBack(scHeadJointName, headCallBack);
    }
}
void TMapObjPakkun::createColModel(J3DModelData *modelData)
{
    createBoundsCylinder(2500.0f, 1500.0f, mScale.x);
}
void TMapObjPakkun::InitExec()
{
    const StateFuncSet<TMapObjPakkun> *func = FindStateFunc(getState(), sTable, 5);
    (this->*func->mInitFunc)();
}
void TMapObjPakkun::MoveExec()
{
    const StateFuncSet<TMapObjPakkun> *func = FindStateFunc(getState(), sTable, 5);
    (this->*func->mExecFunc)();
}
void TMapObjPakkun::changeAllState(u16 state)
{
    setState(state);
}

void TMapObjPakkun::doKartColCallBack(int kartNo)
{
    u64 status = GetKartCtrl()->GetKartStatus(kartNo);
    if ((status & 0x40000) != 0) {
        mDamageAdd = 0;
        changeDamageProc();
    }
}
void TMapObjPakkun::changeDamageProc()
{
    u8 anmNo = 2;
    register AnmController *ctrl = mAnmCtrl;
    if (ctrl->mTrans != nullptr) {
#line 353
        JUT_MINMAX_ASSERT(0, anmNo, ctrl->mTrans->mMaxAnmNo);
        ctrl->mTrans->changeBlendAnm(2, 1, 1);
    }
    setState(4);
}
void TMapObjPakkun::initFunc_Stand()
{
    clrObjFlagHidding();
    mTargetKart = -1;
}
void TMapObjPakkun::doFunc_Stand()
{
    checkItemHitting();
    TMapObjPakkunColHead *head = mHead;
    u8 hit = head->_14c;
    head->_14c = 0;
    if (hit != 0) {
        mDamageAdd = 0;
        setState(4);
        mAnmPlayer.setAnm(TMapObjPakkun::sAnmTable[4]);
    }
    s16 kartNo = SiUtil::searchNearKartCircle(mPos, mSearchRadius);
    if (kartNo != -1) {
        mTargetKart = kartNo;
        setState(1);
        mAnmPlayer.setAnm(sAnmTable[1]);
    }
}
void TMapObjPakkun::initFunc_Attack()
{
    mSoundMgr->setSe(0x40058);
}
void TMapObjPakkun::doFunc_Attack()
{
    checkItemHitting();
    TMapObjPakkunColHead *head = mHead;
    u8 hit = head->_14c;
    head->_14c = 0;
    if (hit != 0) {
        mDamageAdd = 0;
        setState(4);
        mAnmPlayer.setAnm(TMapObjPakkun::sAnmTable[4]);
    }
    JGeometry::TVec3f pos;
    ObjUtility::getKartPos(mTargetKart, &pos);
    turnTo(pos, sTurnAttackScale);
    if (mAnmPlayer.isCurAnmReachEnd()) {
        setState(3);
        mAnmPlayer.setAnm(sAnmTable[3]);
    }
}
void TMapObjPakkun::initFunc_Rest()
{
}
void TMapObjPakkun::doFunc_Rest()
{
    checkItemHitting();
    JGeometry::TVec3f pos;
    ObjUtility::getKartPos(mTargetKart, &pos);
    turnTo(pos, 1.0f);
    if (getStateCount() > (u32)sRestFrame) {
        setState(1);
        mAnmPlayer.setAnm(sAnmTable[1]);
    }
}
void TMapObjPakkun::initFunc_Search()
{
    mSearchTimer = 0;
}
void TMapObjPakkun::doFunc_Search()
{
    mSearchTimer++;
    if (mSearchTimer > sSearchFrame) {
        mSearchTimer = 0;
        mTargetKart = SiUtil::searchNearKartCircle(mPos, mSearchRadius);
        if (mTargetKart == -1) {
            setState(0);
            mAnmPlayer.setAnm(sAnmTable[0]);
            return;
        }
    }

    checkItemHitting();

    JGeometry::TVec3f pos;
    ObjUtility::getKartPos(mTargetKart, &pos);
    turnTo(pos, 1.0f);

    JGeometry::TVec3f diff;
    diff.sub(pos, mPos);
    f32 dist = diff.x * diff.x + diff.z * diff.z;

    if (RaceMgr::sRaceManager->mRaceInfo->mRaceMode != 1) {
        f32 attack = mAttackRadius * mAttackRadius;
        if (dist < attack) {
            setState(2);
            mAnmPlayer.setAnm(sAnmTable[2]);
            return;
        }
    }

    f32 search = mSearchRadius * mSearchRadius;
    if (dist > search) {
        setState(0);
        mAnmPlayer.setAnm(sAnmTable[0]);
    }
}
void TMapObjPakkun::turnTo(const JGeometry::TVec3f &pos, f32 scale)
{
    static JGeometry::TVec3f updir(-6.0f, 0.0f, 0.0f);
    static s8 init;
}
void TMapObjPakkun::setCallBack(const char *name, int (*func)(J3DJoint *, int))
{
    int idx = mModel.getModelData()->getJointName()->getIndex(name);
    mModel.getModelData()->getJointNodePointer(idx)->mFunction = (J3DJointCallBack)func;
    mModel._14 = (u32 *)this;
}
void TMapObjPakkun::setRotMtx(int, Mtx mtx)
{
    PSMTXCopy(mtx, (Mtx &)mRotMtx);
}

void TMapObjPakkun::setHeadPos(Mtx mtx)
{
    mHead->mPos.set(mtx[0][3], mtx[1][3], mtx[2][3]);
}

int TMapObjPakkun::rotCallBack(J3DJoint *joint, int param)
{
    if (param == 0) {
        u16 jointIndex = joint->mJointIdx;
        J3DModel *model = j3dSys._38;
        TMapObjPakkun *obj = (TMapObjPakkun *)((ExModel *)model->getUserArea())->_14;
        if (obj != nullptr) {
            Mtx &mtx = model->getAnmMtx(jointIndex);

            JGeometry::TPos3f copy;
            copy.set(mtx);
            copy.zeroTrans();

            PSMTXConcat((Mtx &)obj->mBaseMtx, (Mtx &)copy, (Mtx &)copy);
            copy[0][3] = mtx[0][3];
            copy[1][3] = mtx[1][3];
            copy[2][3] = mtx[2][3];

            PSMTXCopy((Mtx &)copy, obj->mModel.getModel()->getAnmMtx(jointIndex));
            PSMTXCopy((Mtx &)copy, mtx);
            PSMTXCopy(mtx, j3dSys.mCurrentMtx);
        }
    }
    return true;
}
int TMapObjPakkun::headCallBack(J3DJoint *joint, int param)
{
    if (param == 0) {
        u16 jointIndex = joint->mJointIdx;
        J3DModel *model = j3dSys._38;
        TMapObjPakkun *obj = (TMapObjPakkun *)((ExModel *)model->getUserArea())->_14;
        if (obj != nullptr) {
            Mtx &mtx = model->getAnmMtx(jointIndex);

            JGeometry::TVec3f offset;
            JGeometry::TVec3f pos;
            JGeometry::TPos3f copy;
            pos.set(mtx[0][3], mtx[1][3], mtx[2][3]);
            copy.set(mtx);
            copy.zeroTrans();

            PSMTXMultVecSR((Mtx &)copy, &scHeadCenterOffset, &offset);
            obj->mHead->mPos.set(pos);
            obj->mHead->mPos += offset;
        }
    }
    return true;
}
void TMapObjPakkun::checkItemHitting()
{
    if (!(mGeoObjFlag & 4)) {
        return;
    }
    int kind = mColItemObj->mItemKind;
    if (kind == 10) {
        return;
    }
    if (kind < 10) {
        if (kind >= 3) {
            if (kind >= 7) {
                return;
            }
        }
        else if (kind < 0) {
            return;
        }
    }
    else if (kind >= 12) {
        return;
    }
    mDamageAdd = 0;
    u8 anmNo = 2;
    register AnmController *ctrl = mAnmCtrl;
    if (ctrl->mTrans != nullptr) {
#line 353
        JUT_MINMAX_ASSERT(0, anmNo, ctrl->mTrans->mMaxAnmNo);
        ctrl->mTrans->changeBlendAnm(2, 1, 1);
    }
    setState(4);
}
void TMapObjPakkun::initFunc_Damage()
{
    s16 param = mObjData->mParam1;
    mDamageFrame = param + mDamageAdd;
    mSoundMgr->setSe(0x4007e);
}
void TMapObjPakkun::doFunc_Damage()
{
    if (getStateCount() > (u32)mDamageFrame) {
        setState(0);
        mAnmPlayer.setAnm(sAnmTable[0]);
    }
}
TPakkunSupervisor::TPakkunSupervisor()
{
    mList.initiate();
    ItemThunder::entrySuperVisor(this);
}
void TPakkunSupervisor::entry(TMapObjPakkun *obj)
{
    mList.append(&obj->mLink);
}
void TPakkunSupervisor::thunderDownAll()
{
    for (JSUListIterator<TMapObjPakkun> it(mList.getFirst()); it != mList.getEnd(); it++) {
        TMapObjPakkun *pakkun = it.getObject();
        pakkun->mDamageAdd = (s16)(pakkun->getGeoRnd()->get_ufloat_1() * TMapObjPakkun::sThunderDamageRandeMax);
        u8 anm = TMapObjPakkun::sAnmTable[4];
        pakkun->setState(4);
        pakkun->mAnmPlayer.setAnm(anm);
    }
}

#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
