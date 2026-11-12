#include "Shiraiwa/Objects/MapObjAward.h"

#include "Kaneshige/KartChecker.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Sato/RivalSpeedCtrl.h"
#include "Yamamoto/kartCtrl.h"
#include "Kaneshige/LightMgr.h"
#include "Kaneshige/SimpleDrawer.h"

#include "JSystem/JMath/JMath.h"

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

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

JGeometry::TVec3f TMapObjAwardCup::sVelocity0(0.0f, 84.0f, 0.0f);
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
J3DAnmTexPattern *TMapObjAwardCup::sAwardCupBtpAnm;

StateObserver::StateFuncSet<TMapObjAwardCup> TMapObjAwardCup::sTable[3] = {
    {0, &TMapObjAwardCup::initFunc_InBox, &TMapObjAwardCup::doFunc_InBox},
    {1, &TMapObjAwardCup::initFunc_FlyOut, &TMapObjAwardCup::doFunc_FlyOut},
    {2, &TMapObjAwardCup::initFunc_Roll, &TMapObjAwardCup::doFunc_Roll},
};










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
#line 93
        JUT_ASSERT(false);
        break;
    }

    type = mObjData->mParam1 + type * 3 - 1;
#line 99
    JUT_MINMAX_ASSERT(0, type, 18);

    mCup = (TMapObjAwardCup *)GeographyObjManager::gMgr->createSubObj(scIDTable[type]);

    mCup->mMatAnm.getFrameCtrl()->mFrame = (s16)(mObjData->mParam1 - 1);
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
    mRotMtx.getXDir(dir);
    dir.normalize();
    mCup->mDirVec.scale((f32)mObjData->mParam2 / (f32)TMapObjAwardCup::sJumpFrame, dir);
    mCup->setOrgMtx(mRotMtx);
}

#pragma auto_inline off
void TMapObjAwardItemBox::skip() {
    int rank = -1;
    int no = RaceMgr::getManager()->getAwardKartNo();
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
    JGeometry::TVec3f vy, vx;
    mRotAngle0 += sRollSpeed * JMASSin(mRandDir);
    vy.scale(mRotAngle0, mModelYDir);
    mRotAngle1 += -sRollSpeed * JMASCos(mRandDir);
    vx.scale(mRotAngle1, mModelXDir);
    mModelZDir.add(vx);
    mModelZDir.add(vy);
    mModelZDir.normalize();
    mModelYDir.cross(mModelZDir, mModelXDir);
    mModelYDir.normalize();
    mModelXDir.cross(mModelYDir, mModelZDir);
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

TMapObjAwardCup::TMapObjAwardCup(u32 id) : TMapObjDemoObj(id), StateObserver(), JKRDisposer(), mLink(this) {
    mFlag &= ~1;
    mObjFlag |= 0x400;
    createSoundMgr();
    setObjFlagSimpleDraw();
    if (getSupervisor() == nullptr) {
        sSupervisor = new TAwardCupSupervisor();
    }
    sSupervisor->entry(this);
}

void TMapObjAwardCup::InitExec() {
    Observer_FindAndInit(TMapObjAwardCup, 3);
}

void TMapObjAwardCup::MoveExec() {
    Observer_FindAndExec(TMapObjAwardCup, 3);
}

void TMapObjAwardCup::createModel(JKRSolidHeap *, u32) {
    mMatAnm.setExModel(&mModel);
    mMatAnm.attach(sAwardCupBtpAnm);
}

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

void TMapObjAwardCup::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    JKRArchive *arc = *(JKRArchive **)((char *)RaceMgr::getManager() + 0x90); // mAwardArc
    J3DAnmObjMaterial::setupTexPatternAnmData(&sAwardCupBtpAnm, modelData, arc->getResource(getBtpFileName()));
}

void TMapObjAwardCup::setOrgMtx(const JGeometry::TPos3f &mtx) {
    JGeometry::TVec3f x, z, y;
    mtx.getXDir(x);
    mtx.getYDir(y);
    mtx.getZDir(z);
    x.cross(z, y);
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
    f32 vel = sVelocity0.y;
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

void TMapObjAwardCup::skip(const JGeometry::TVec3f &pos) {
    mPos.scaleAdd(sJumpFrame, mDirVec, pos);
    mPos.y += sOffsetHeight;
    mFlag222 = 0;
    mRollSpeed = sRollSpeedMax;
    mRollSpeed2 = mDir * sRollSpeedMax;
    mFlag220 = mDir > 0 ? 0 : 0x8000;
    mRotMtx.getXDir(mModelXDir);
    mRotMtx.getYDir(mModelYDir);
    mRotMtx.getZDir(mModelZDir);
    mFlag234 = 0;
    setState(2);
}

void TMapObjAwardCup::initFunc_InBox() {
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~2;
    mFlag234 = 1;
}

void TMapObjAwardCup::doFunc_InBox() {}

void TMapObjAwardCup::initFunc_FlyOut() {
    mScale.x = 0.0f;
    mScale.y = 0.0f;
    mScale.z = 0.0f;
    mVel.set(sVelocity0);
    mFlag234 = 0;
    mRotMtx.getXDir(mModelXDir);
    mRotMtx.getYDir(mModelYDir);
    mRotMtx.getZDir(mModelZDir);
    mRollSpeed = sRotVel0;
    mRollSpeed2 = mDir * sRotVel0;
    mFlag220 = mDir > 0 ? 0 : 0x8000;
    mFlag222 = 0;
    mSoundMgr->setSe(0x20044);
}

void TMapObjAwardCup::doFunc_FlyOut() {
    mVel.scale(sAirFriction);
    mVel.y -= sGravity;
    mPos.add(mVel);
    mPos.add(mDirVec);
    if (mScale.y < 1.0f) {
        f32 s = mScale.y + sScaleVel;
        if (s > 1.0f) {
            s = 1.0f;
        }
        mScale.x = s;
        mScale.y = s;
        mScale.z = s;
    }
    rotAnimation(false);
    if (getStateCount() > sJumpFrame) {
        setState(2);
    }
}

void TMapObjAwardCup::initFunc_Roll() {}

void TMapObjAwardCup::doFunc_Roll() {
    rotAnimation(true);
}

void TMapObjAwardCup::draw(u32 viewNo) {
    if (mFlag234 == 0) {
        mMatAnm.anmFrameProc();
        J3DModelData *modelData = mModel.getModelData();
        SimpleDrawer drawer;
        drawer.drawInit(modelData);
        while (drawer.loadPreDrawSetting()) {
            MtxPtr effectMtx = LightMgr::getManager()->searchLight(viewNo + 0x53434e30)->getEffectMtx();
            mModel.setScale(mScale);
            mModel.clipAll(viewNo, true);
            mModel.simpleDraw(viewNo, effectMtx, 1);
        }
    }
    else {
        mModel.hide();
    }
}

void TMapObjAwardCup::rotAnimation(bool roll) {
    Mtx m1;
    Mtx m2;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f side;
    JGeometry::TVec3f axis;
    f32 maxSpeed;
    if (roll) {
        maxSpeed = sRollSpeedMax;
    }
    else {
        maxSpeed = __fabsf(mRollSpeed2);
    }
    f32 accel = -sRollSpeed * JMASCos(mFlag220);
    mRollSpeed2 += accel;
    if (fabs(mRollSpeed2) > maxSpeed) {
        mRollSpeed2 -= sRotBrake * (mRollSpeed2 < 0.0f ? -1.0f : 1.0f);
        if (fabs(mRollSpeed2) < maxSpeed) {
            mRollSpeed2 = maxSpeed * (mRollSpeed2 < 0.0f ? -1.0f : 1.0f);
        }
    }
    vel.scale(mRollSpeed2, mModelXDir);
    mModelZDir.add(vel);
    mModelZDir.normalize();
    mModelYDir.cross(mModelZDir, mModelXDir);
    mModelYDir.normalize();
    mModelXDir.cross(mModelYDir, mModelZDir);
    mModelXDir.normalize();

    PSMTXIdentity(m1);
    m1[0][0] = mModelXDir.x;
    m1[1][0] = mModelXDir.y;
    m1[2][0] = mModelXDir.z;
    m1[0][1] = mModelYDir.x;
    m1[1][1] = mModelYDir.y;
    m1[2][1] = mModelYDir.z;
    m1[0][2] = mModelZDir.x;
    m1[1][2] = mModelZDir.y;
    m1[2][2] = mModelZDir.z;
    PSMTXIdentity(m2);

    static JGeometry::TVec3f axisZ(0.0f, 0.0f, 1.0f);
    s16 cycle = roll ? sAxisRotCycle : sFlyOutRotCycle;
    if (mFlag224 > cycle) {
        mFlag224 -= sAxisCycleDecel;
    }
    if (mFlag224 < cycle) {
        mFlag224 = cycle;
    }
    s16 angle = mFlag224 * JMASCos(mFlag222);
    axis.set(JMASSin(angle), JMASCos(angle), 0.0f);
    side.cross(axis, axisZ);
    side.normalize();
    m2[0][0] = side.x;
    m2[1][0] = side.y;
    m2[2][0] = side.z;
    m2[0][1] = axis.x;
    m2[1][1] = axis.y;
    m2[2][1] = axis.z;
    m2[0][2] = axisZ.x;
    m2[1][2] = axisZ.y;
    m2[2][2] = axisZ.z;
    PSMTXConcat(m1, m2, m1);
    PSMTXConcat(mOrgMtx, m1, mRotMtx);

    mFlag220 += sRotCycle;
    if (roll) {
        mFlag222 += sRotCycle;
    }
    else {
        mFlag222 += sRotCycle * 15;
    }
}

void TMapObjAwardCup::calc() {
    ExecuteState();
}

void TAwardCupSupervisor::entry(TMapObjAwardCup *cup) {
    mList.append(&cup->mLink);
}

void TAwardCupSupervisor::draw(u32 viewNo) {
    for (JSULink<TMapObjAwardCup> *link = mList.getFirst(); link != nullptr; link = link->getNext()) {
        link->getObject()->draw(viewNo);
    }
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

#include "JSystem/JAudio/JASFakeMatch2.h"
