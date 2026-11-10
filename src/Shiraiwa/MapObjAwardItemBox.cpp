#include "Shiraiwa/Objects/MapObjAward.h"

#include "Kaneshige/KartChecker.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Sato/RivalSpeedCtrl.h"
#include "Yamamoto/kartCtrl.h"

#include "JSystem/JMath/JMath.h"

const int TMapObjAwardItemBox::scIDTable[18] = {
    0xd67, 0xd67, 0xd67, 0xd68, 0xd68, 0xd68, 0xd69, 0xd69, 0xd69,
    0xd6a, 0xd6a, 0xd6a, 0xd6b, 0xd6b, 0xd6b, 0xd6c, 0xd6c, 0xd6c,
};

f32 TMapObjAwardItemBox::sRollSpeed = 0.001f;
s16 TMapObjAwardItemBox::sAppearFrame = 5;
s16 TMapObjAwardItemBox::sDisappearFrame = 5;
TAwardItemBoxSupervisor *TMapObjAwardItemBox::sSupervisor;

StateObserver::StateFuncSet<TMapObjAwardItemBox> TMapObjAwardItemBox::sTable[4] = {
    {0, &TMapObjAwardItemBox::initFunc_Appear, &TMapObjAwardItemBox::doFunc_Appear},
    {1, &TMapObjAwardItemBox::initFunc_Roll, &TMapObjAwardItemBox::doFunc_Roll},
    {2, &TMapObjAwardItemBox::initFunc_Disappear, &TMapObjAwardItemBox::doFunc_Disappear},
    {3, &TMapObjAwardItemBox::initFunc_Hide, &TMapObjAwardItemBox::doFunc_Hide},
};

f32 TMapObjAwardCup::sVelocity0[19];
f32 TMapObjAwardCup::sAirFriction = 0.9999f;
f32 TMapObjAwardCup::sGravity = 0.8f;
f32 TMapObjAwardCup::sScaleVel = 0.05f;
s16 TMapObjAwardCup::sJumpFrame = 0x3c;
f32 TMapObjAwardCup::sRollSpeed = 0.002f;
f32 TMapObjAwardCup::sRollSpeedMax = 0.03f;
s16 TMapObjAwardCup::sRotCycle = 0xf;
s16 TMapObjAwardCup::sAxisRotCycle = 0xccc;
s16 TMapObjAwardCup::sFlyOutRotCycle = 0x4000;
f32 TMapObjAwardCup::sRotVel0 = 0.4f;
f32 TMapObjAwardCup::sRotBrake = 0.003f;
s16 TMapObjAwardCup::sAxisCycleDecel = 0x1e;
f32 TMapObjAwardCup::sOffsetHeight;
TAwardCupSupervisor *TMapObjAwardCup::sSupervisor;
J3DAnmBase *TMapObjAwardCup::sAwardCupBtpAnm;

StateObserver::StateFuncSet<TMapObjAwardCup> TMapObjAwardCup::sTable[3] = {
    {0, &TMapObjAwardCup::initFunc_InBox, &TMapObjAwardCup::doFunc_InBox},
    {1, &TMapObjAwardCup::initFunc_FlyOut, &TMapObjAwardCup::doFunc_FlyOut},
    {2, &TMapObjAwardCup::initFunc_Roll, &TMapObjAwardCup::doFunc_Roll},
};

#include "JSystem/JAudio/JASFakeMatch2.h"









TMapObjAwardItemBox::TMapObjAwardItemBox(const CrsData::SObject &obj)
    : TMapObjHioNode(obj), StateObserver(), JKRDisposer(), mLink(this) {
    int type;
    switch (RaceMgr::getManager()->mRaceInfo->mGpCup) {
    case 0:
        type = 0;
        break;
    case 1:
        type = 1;
        break;
    case 2:
        type = 2;
        break;
    case 3:
        type = 3;
        break;
    case 4:
        type = 5;
        break;
    default:
        JUT_ASSERT_F(0, "range over: %d <= type=%d < %d");
        break;
    }

    type = type * 3 - 1;
    type = mObjData->mParam1 + type;
    JUT_MINMAX_ASSERT(0, type, 18);

    mCup = (TMapObjAwardCup *)GeographyObjManager::gMgr->createSubObj(scIDTable[type]);

    mCup->mMatAnm.getFrameCtrl()->mFrame = (f32)(mObjData->mParam1 - 1);
    mCup->mDir = (mObjData->mParam1 == 3) ? -1 : 1;

    if (getSupervisor() == nullptr) {
        sSupervisor = new TAwardItemBoxSupervisor();
    }
    sSupervisor->entry(this);
}

TMapObjAwardItemBox::~TMapObjAwardItemBox() { sSupervisor = nullptr; }

void TMapObjAwardItemBox::createColModel(J3DModelData *) {
    createBoundsSphere(150.0f, 1.0f);
}

void TMapObjAwardItemBox::reset() {
    resetObject();
    ResetState();
    mGeoObjFlag &= ~0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = -1;
    for (int i = 0; i < 2; i++) {
        mEmitter[i] = nullptr;
    }

    JGeometry::TVec3f dir;
    dir.set(mRotMtx[0][0], mRotMtx[1][0], mRotMtx[2][0]);
    dir.normalize();
    f32 ratio = (f32)TMapObjAwardCup::sJumpFrame / (f32)mObjData->mParam2;
    mCup->mDirVec.x = dir.x;
    mCup->mDirVec.y = dir.y;
    mCup->mDirVec.z = ratio;
    mCup->setOrgMtx(mRotMtx);
}

#pragma auto_inline off
void TMapObjAwardItemBox::skip() {
    int no = RaceMgr::getManager()->mRaceInfo->mAwardKartNo;
    int rank = -1;
    if (no != -1) {
        rank = RaceMgr::getManager()->getKartChecker(no)->getRank();
    }
    if (rank == mObjData->mParam1) {
        mCup->skip(mPos);
    }
    setState(3);
}
#pragma auto_inline reset

void TMapObjAwardItemBox::doKartColCallBack(int kart) {
    KartChecker *kc = RaceMgr::getManager()->getKartChecker(kart);
    bool hit = tstIsHitKartFlg(kart);
    int rank = kc->getRank();
    if (hit && rank == mObjData->mParam1) {
        if (kart == RaceMgr::getManager()->mRaceInfo->mAwardKartNo) {
            mCup->flyOut(mPos);
        }
        JGeometry::TVec3f pos;
        ObjUtility::getKartPos(kart, &pos);
        mEmitter[0] = JPEffectMgr::getEffectMgr()->createEmt(JPEffectMgr::getHashValue("mk_itemget_normal_a"), pos);
        mEmitter[1] = JPEffectMgr::getEffectMgr()->createEmt(JPEffectMgr::getHashValue("mk_itemget_normal_b"), pos);
        KartCtrl::getKartCtrl()->getKartEnemy(kart)->mSpeedCtrl->stop();
        setState(2);
    }
}

void TMapObjAwardItemBox::InitExec() {
    Observer_FindAndInit(TMapObjAwardItemBox, 4);
}

void TMapObjAwardItemBox::MoveExec() {
    Observer_FindAndExec(TMapObjAwardItemBox, 4);
}

void TMapObjAwardItemBox::calc() {
    ExecuteState();
}

void TMapObjAwardItemBox::initFunc_Appear() {
    mCheckKartHitFlags = -1;
    mGeoObjFlag &= ~0x20;
    if (mShadowMdl != nullptr) {
        mShadowMdl->setVisibleAll();
    }
    mScale.x = 0.0f;
    mScale.y = 0.0f;
    mScale.z = 0.0f;
}

void TMapObjAwardItemBox::doFunc_Appear() {
    f32 s = SiUtil::getNormalRange(getStateCount(), 0.0f, sAppearFrame) + 0.01f;
    if (s >= 1.0f) {
        s = 1.0f;
        setState(1);
    }
    mScale.x = s;
    mScale.y = s;
    mScale.z = s;
}

void TMapObjAwardItemBox::initFunc_Roll() {
    mRotAngle0 = 0.0f;
    mRotAngle1 = 0.0f;
    f32 z0 = mRotMtx[2][0];
    f32 y0 = mRotMtx[1][0];
    mModelXDir.set(mRotMtx[0][0], y0, z0);
    f32 z1 = mRotMtx[2][1];
    f32 y1 = mRotMtx[1][1];
    mModelYDir.set(mRotMtx[0][1], y1, z1);
    f32 z2 = mRotMtx[2][2];
    f32 y2 = mRotMtx[1][2];
    mModelZDir.set(mRotMtx[0][2], y2, z2);
    mRandDir = getGeoRnd()->get();
}

void TMapObjAwardItemBox::doFunc_Roll() {
    rotAnimation();
}

void TMapObjAwardItemBox::rotAnimation() {
    mRotAngle0 += JMASSin(mRandDir) * sRollSpeed;
    mRotAngle1 += JMASCos(mRandDir) * -sRollSpeed;
    mModelZDir.x += mModelXDir.x * mRotAngle1;
    mModelZDir.y += mModelXDir.y * mRotAngle1;
    mModelZDir.z += mModelXDir.z * mRotAngle1;
    mModelZDir.x += mModelYDir.x * mRotAngle0;
    mModelZDir.y += mModelYDir.y * mRotAngle0;
    mModelZDir.z += mModelYDir.z * mRotAngle0;
    mModelZDir.normalize();
    mModelZDir.cross(mModelXDir, mModelYDir);
    mModelYDir.cross(mModelZDir, mModelXDir);
    mRandDir += 0x100;
    stMakeRMtx(mRotMtx, mModelXDir, mModelYDir, mModelZDir);
}

void TMapObjAwardItemBox::initFunc_Disappear() {
    mScale.x = 1.0f;
    mScale.y = 1.0f;
    mScale.z = 1.0f;
    mCheckKartHitFlags = 0;
}

void TMapObjAwardItemBox::doFunc_Disappear() {
    f32 s = 1.0f - SiUtil::getNormalRange(getStateCount(), 0.0f, sDisappearFrame);
    if (s <= 0.0f) {
        s = 1.0f;
        mGeoObjFlag |= 0x20;
        setState(3);
    }
    mScale.x = s;
    mScale.y = s;
    mScale.z = s;
}

void TMapObjAwardItemBox::initFunc_Hide() {
    if (mShadowMdl != nullptr) {
        mShadowMdl->clrVisibleAll();
    }
    mGeoObjFlag |= 0x20;
}

void TMapObjAwardItemBox::doFunc_Hide() {}

void TMapObjAwardCup::update() {
    mMatAnm.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}

void TMapObjAwardCup::reset() {
    resetObject();
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~2;
    mGeoObjFlag |= 0x20;
    mFlag234 = 1;
    ResetState();
    PSMTXIdentity(mOrgMtx);
    mFlag224 = 0;
    mDirVec.zero();
    if (sOffsetHeight == 0.0f) {
        sOffsetHeight = makeOffset();
    }
}

void TMapObjAwardCup::setOrgMtx(const JGeometry::TPos3f &mtx) {
    JGeometry::TVec3f z;
    JGeometry::TVec3f x(mtx[0][0], mtx[1][0], mtx[2][0]);
    JGeometry::TVec3f y(mtx[0][1], mtx[1][1], mtx[2][1]);
    z.cross(x, y);
    x.normalize();
    mOrgMtx[0][0] = x.x;
    mOrgMtx[1][0] = x.y;
    mOrgMtx[2][0] = x.z;
    mOrgMtx[0][1] = z.x;
    mOrgMtx[1][1] = z.y;
    mOrgMtx[2][1] = z.z;
    mOrgMtx[0][2] = y.x;
    mOrgMtx[1][2] = y.y;
    mOrgMtx[2][2] = y.z;
}

f32 TMapObjAwardCup::makeOffset() {
    f32 height = 0.0f;
    f32 vel = sVelocity0[1];
    int n = sJumpFrame + 1;
    for (int i = 0; i <= n; i++) {
        vel *= sAirFriction;
        vel -= sGravity;
        height += vel;
    }
    return height;
}

void TMapObjAwardCup::flyOut(const JGeometry::TVec3f &pos) {
    setState(1);
    mPos.x = pos.x;
    mPos.y = pos.y;
    mPos.z = pos.z;
}

void TMapObjAwardCup::initFunc_InBox() {
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~2;
    mFlag234 = 1;
}

void TMapObjAwardCup::doFunc_InBox() {}

void TMapObjAwardCup::calc() {
    ExecuteState();
}

void TAwardCupSupervisor::entry(TMapObjAwardCup *cup) {
    mList.append(&cup->mLink);
}

TAwardItemBoxSupervisor::TAwardItemBoxSupervisor() : GeoObjSupervisor() { mList.initiate(); }

void TAwardItemBoxSupervisor::entry(TMapObjAwardItemBox *obj) {
    mList.append(&obj->mLink);
}

void TAwardItemBoxSupervisor::skip() {
    JSULink<TMapObjAwardItemBox> *link = mList.getFirst();
    while (link != nullptr) {
        link->getObject()->skip();
        link = link->getNext();
    }
}
bool TAwardItemBoxSupervisor::isRollState() {
    bool ret = false;
    for (JSULink<TMapObjAwardItemBox> *link = mList.getFirst(); link != nullptr;
         link = link->getNext()) {
        TMapObjAwardItemBox *obj = link->getObject();
        bool roll;
        if (obj->mCup != nullptr) {
            roll = obj->mCup->getState() == 2;
        } else {
            roll = false;
        }
        ret |= roll;
    }
    return ret;
}

