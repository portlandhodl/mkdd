#include "Shiraiwa/Balloon.h"

#include "JSystem/J3D/J3DAnmBase.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Sato/ObjUtility.h"

void TBalloon::reset() {
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    mGeoObjFlag |= 0x20;
    mFlag = 0;
    mCount = 0;
}

void TBalloon::update() {
    mMaterial.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}



























































































































































































































































































































































































































































































void TBalloon::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *anm = ObjUtility::getPtrCourseArc("/Objects/MiniGame_Balloon.brk");
    JUT_ASSERT_MSG(anm != nullptr, "TBalloon: /Objects/MiniGame_Balloon.brk not found.");
    J3DAnmObjMaterial::setupTevRegAnmData(&sBalloonBrkAnm, modelData, anm);
}

void TBalloon::calc() {
    f32 scale = mScaleRatio;
    if (RaceMgr::sRaceManager->getRacePhase() == PHASE_CRS_DEMO) {
        scale = 0.0f;
    } else if (mFrame < scScaleFrame2) {
        f32 s = 0.0f;
        f32 frame = mFrame;
        if (frame > 0.0f) {
            if (frame < scScaleFrame1) {
                s = frame * scScale1 / scScaleFrame1;
            } else {
                if (frame - scScaleFrame1 <= 1.0f) {
                    mString->shake();
                }
                s = scScale1 + (scScale1 - 1.0f) * (scScaleFrame1 - mFrame) / (scScaleFrame2 - scScaleFrame1);
            }
        }
        scale = mScaleRatio * s;
        mFrame += 1.0f;
    }

    if (scale > 0.0f) {
        mScale.x = scale;
        mScale.y = scale;
        mScale.z = scale;
        if (mFlag != 0) {
            if (!mString->tstObjFlagHidding()) {
                mFlag = 0;
                if (mShadowMdl != nullptr) {
                    mShadowMdl->setVisibleAll();
                }
            }
        } else {
            if (mString->tstObjFlagHidding()) {
                mFlag = 1;
                if (mShadowMdl != nullptr) {
                    mShadowMdl->clrVisibleAll();
                }
            } else {
                JGeometry::TVec3f up;
                JGeometry::TVec3f dir;

                mString->getPoint(&mPos);
                JGeometry::TVec3f v1(0.0f, 0.0f, 1.0f);
                mString->getPointDir(&dir);
                up.cross(dir, v1);
                up.normalize();
                v1.cross(up, dir);
                v1.normalize();

                mRotMtx[0][0] = up.x;
                mRotMtx[1][0] = up.y;
                mRotMtx[2][0] = up.z;
                mRotMtx[0][1] = dir.x;
                mRotMtx[1][1] = dir.y;
                mRotMtx[2][1] = dir.z;
                mRotMtx[0][2] = v1.x;
                mRotMtx[1][2] = v1.y;
                mRotMtx[2][2] = v1.z;
            }
        }
    } else {
        f32 one = 1.0f;
        mScale.x = one;
        mScale.y = one;
        mScale.z = one;
        mFlag = 1;
        if (mShadowMdl != nullptr) {
            mShadowMdl->clrVisibleAll();
        }
    }
}

void TBalloon::viewCalc(u32 viewNo) {
    mModel.viewCalc(viewNo);
}

void TBalloon::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mMaterial.setExModel(&mModel);
    mMaterial.setAnmBase(sBalloonBrkAnm);
    mMaterial.initFrameCtrl(mMaterial.mAnmBase);
}

void TBalloon::setCurrentViewNo(u32 viewNo) {
    mModel.setCurrentViewNo(viewNo);
}

TBalloonSupervisor::~TBalloonSupervisor() {}

void TBalloonSupervisor::drawString(u32) {}

void TBalloonSupervisor::drawBalloon(u32 viewNo) {
    TBalloonString *balloons[32];
    int count = 0;
    JSUPtrLink *link = mList.getFirstLink();
    while (link != nullptr) {
        if (count >= 32) {
            break;
        }
        balloons[count++] = (TBalloonString *)link->getObjectPtr();
        link = link->getNext();
    }
    for (int i = count; i < 32; i++) {
        balloons[i] = nullptr;
    }
    for (int i = 0; i < 32; i++) {
        if (balloons[i] != nullptr) {
            balloons[i]->drawBalloon(viewNo);
        }
    }
}

__declspec(weak) TBalloon::~TBalloon() {}

#include "Sato/GeographyObjMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Kaneshige/LightMgr.h"
#include "Kaneshige/SimpleDrawer.h"
#include "Yamamoto/kartCtrl.h"
#include "Yamamoto/kartCamera.h"
#include "JSystem/JMath/JMath.h"

f32 TBalloonManager::sPullDirX = 0.4f;
f32 TBalloonManager::sPullDirY = 1.0f;
f32 TBalloonManager::sPullDirZ;
f32 TBalloonManager::sWinningOffset = 30.0f;
f32 TBalloonManager::sFrameDiff = -10.0f;
f32 TBalloonManager::sStartFrame = -150.0f;
f32 TBalloon::scScaleFrame1 = 10.0f;
f32 TBalloon::scScaleFrame2 = 15.0f;
f32 TBalloon::scScale1 = 1.5f;
const char *TBalloon::scEfctBreakA = "mk_balloon_a";
const char *TBalloon::scEfctBreakB = "mk_balloon_b";
const char *TBalloon::scEfctBreakColor[8] = {
    "mk_balloon_red",
    "mk_balloon_blue",
    "mk_balloon_green",
    "mk_balloon_yellow",
    "mk_balloon_pink",
    "mk_balloon_orange",
    "mk_balloon_lightb",
    "mk_balloon_purple",
};
f32 TBalloonString::sAdjustRatio = 0.0f;
f32 TBalloonString::sAirFric = 0.0f;
f32 TBalloonString::sPower = 1.0f;
f32 TBalloonString::sDemoPower = 0.9f;
f32 TBalloonString::sGravity = 20.0f;
u8 TBalloonString::sNodeNum = 3;
f32 TBalloonString::sOrgScale = 5.0f;
f32 TBalloonString::sOrgLength = -0.3f;
f32 TBalloonString::sBreakOffset = 0.35f;
f32 TBalloonString::sBreakUpVel = 1200.0f;
f32 TBalloonString::sShakePow = 150.0f;
f32 TBalloonString::sOffset[12] = {
    0.0f, 70.0f, -25.0f,
    0.0f, 80.0f, -20.0f,
    0.0f, 100.0f, -30.0f,
    0.0f, 120.0f, -30.0f,
};
f32 TBalloonString::sTotalLength[21] = {
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f,
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f,
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f,
};
TBalloonSupervisor *TBalloonString::sSupervisor;

TBalloon::TBalloon() : GeographyObj(0x26ae) {
    mObjFlag |= 1;
    mObjFlag |= 0x200;
    createSoundMgr();
}

TBalloonString::TBalloonString() : TMapObjHioNode(0x26ad), JKRDisposer(), mLink(this) {
    init();
}

TBalloonString::~TBalloonString() {
    sSupervisor = nullptr;
    delete mNodeMgr;
}

void TBalloonString::init() {
    mNodeLengths = new f32[sNodeNum];
    mNodeMgr = new ExStringNodeManager(sNodeNum, 30.0f, true, false, 0);
    if (sSupervisor == nullptr) {
        sSupervisor = new TBalloonSupervisor();
    }
    sSupervisor->mList.append(&mLink);
    mBalloons[0] = (TBalloon *)GetGeoObjMgr()->createSubObj(0x26ae);
    mBalloons[0]->mString = this;
    mBalloons[0]->mColor = -1;
    for (int i = 1; i < 8; i++) {
        mBalloons[i] = nullptr;
    }
}

void TBalloonString::setColor(u32 color) {
    for (int i = 0; i < 8; i++) {
        if (mBalloons[i] != nullptr) {
            mBalloons[i]->mMaterial.setFrame((f32)color);
        }
    }
}

void TBalloonString::unlock(u32 val) {
    mStartY = mPos.y;
    mIsAlive = 0;
    for (int i = 0; i < 8; i++) {
        if (mBalloons[i] != nullptr) {
            mBalloons[i]->mCount = val;
        }
    }
}

void TBalloonString::setMainViewNo(s8 viewNo) {
    mMainViewNo = viewNo;
}

void TBalloonString::setCharID(const EKartWeight &weight, const ECharID &id1, const ECharID &id2) {
    int w = weight;
    if (id1 == PETEY || id2 == PETEY) {
        w++;
    }
    mCharID = w * 3;
    f32 a = sTotalLength[id1];
    f32 b = sTotalLength[id2];
    mTotalLength = a > b ? a : b;
    resetNodeLength();
}

void TBalloonString::resetNodeLength() {
    mNodeMgr->resetNodeAll(nullptr);
    f32 len = mTotalLength / (f32)sNodeNum;
    mNodeMgr->setNodeLengthAll(len);
    for (u32 i = 0; i < sNodeNum - 1; i++) {
        mNodeLengths[i] = len;
    }
}

void TBalloonString::setPosition(const JGeometry::TVec3f &pos, const JGeometry::TPos3f &mtx) {
    JGeometry::TVec3f offset(sOffset[mCharID], sOffset[mCharID + 1], sOffset[mCharID + 2]);
    PSMTXMultVecSR(mtx, &offset, &offset);
    mPos.add(pos, offset);
}

void TBalloonString::setParameters() {
    mNodeMgr->_24 = sAdjustRatio;
    mNodeMgr->_20 = sAirFric;
    mNodeMgr->_1c = sGravity;
}

void TBalloonString::reset() {
    resetObject();
    sOrgLength = 1200.0f;
    sOrgScale = 0.35f;
    if (mBalloons[0] != nullptr) {
        mBalloons[0]->mScaleRatio = sOrgScale;
    }
    mGeoObjFlag &= ~7;
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~0x70;
    PSMTXIdentity(mRotMtx);
    mVel.x = 0.0f;
    mVel.y = sBreakUpVel;
    mVel.z = 0.0f;
    mNodeMgr->resetNodeAll(nullptr);
    mIsAlive = 1;
    JGeometry::TVec3f dir(0.0f, 1.0f, 0.0f);
    for (int i = 0; i < sNodeNum; i++) {
        JGeometry::TVec3f pos;
        JMAVECScaleAdd(&pos, &dir, &pos, sOrgLength * i);
        pos.add(mPos);
        mNodeMgr->setNodePos(i, pos);
        StringNode *node = mNodeMgr->mStrNodeList.getNth(i)->getObject();
        node->_30 = 0;
    }
    resetNodeLength();
    setParameters();
    mNodeMgr->calc();
}

void TBalloonString::calc() {
    if (mManager == nullptr) {
        return;
    }
    JGeometry::TVec3f up;
    getUpDir(&up);
    mNodeMgr->_28 = up;
    mNodeMgr->calc();
    if (!mIsAlive) {
        if (mPos.y - mStartY > sBreakOffset) {
            die();
        }
    }
    JGeometry::TVec3f pos = mPos;
    pos += mVel;
    mNodeMgr->setNodePos(0, pos);
    JGeometry::TVec3f vel = mVelOffset;
    f32 power = sPower;
    if (!RaceMgr::getManager()->isAbleStart()) {
        power = sDemoPower;
    }
    vel.y += power;
    mNodeMgr->addNodeVel(mNodeMgr->mStrNodeList.getNumLinks() - 1, vel);
}

void TBalloonString::die() {
    if (mGeoObjFlag & 0x20) {
        return;
    }
    for (int i = 0; i < 8; i++) {
        TBalloon *balloon = mBalloons[i];
        if (balloon == nullptr) {
            continue;
        }
        balloon->mFlag = 1;
        if (balloon->mCount != 0) {
            if (!RaceMgr::getManager()->isRaceEnd()) {
                balloon->getSoundMgr()->setSe(balloon->mCount);
            }
        }
        JPABaseEmitter *emt = JPEffectMgr::mThis->createEmt(TBalloon::scEfctBreakA, balloon->mPos);
        emt->mGlobalScl = balloon->mScale;
        emt->mGlobalPScl.x = balloon->mScale.x;
        emt->mGlobalPScl.y = balloon->mScale.y;
        emt = JPEffectMgr::mThis->createEmt(TBalloon::scEfctBreakB, balloon->mPos);
        emt->mGlobalScl = balloon->mScale;
        emt->mGlobalPScl.x = balloon->mScale.x;
        emt->mGlobalPScl.y = balloon->mScale.y;
        emt = JPEffectMgr::mThis->createEmt(TBalloon::scEfctBreakColor[(int)balloon->mMaterial.getFrame()], balloon->mPos);
        emt->mGlobalScl = balloon->mScale;
        emt->mGlobalPScl.x = balloon->mScale.x;
        emt->mGlobalPScl.y = balloon->mScale.y;
    }
    mGeoObjFlag |= 0x20;
}

void TBalloonString::shake() {
    stRandom *rnd = getGeoRnd();
    u32 r = rnd->getRandom();
    f32 f = (r >> 9 | 0x3f800000);
    f = (*(f32 *)&f) - 1.0f;
    s16 ang = (s16)(f * 10415.48f);
    JGeometry::TVec3f vel;
    vel.x = JMath::sincosTable_.sinShort(ang) * sShakePow;
    vel.y = JMath::sincosTable_.cosShort(ang) * sShakePow;
    vel.z = 0.0f;
    mNodeMgr->addNodeVel(mNodeMgr->mStrNodeList.getNumLinks() - 1, vel);
}

void TBalloonString::drawBalloon(u32 viewNo) {
    if (mGeoObjFlag & 0x20) {
        return;
    }
    for (int i = 0; i < 8; i++) {
        TBalloon *balloon = mBalloons[i];
        if (balloon == nullptr) {
            continue;
        }
        if (balloon->mFlag) {
            balloon->mModel.hide();
            continue;
        }
        balloon->mMaterial.anmFrameProc();
        LightObj *light = LightMgr::getManager()->searchLight(0x424c4e30 + viewNo);
        typedef MtxPtr (*DrawFn)(LightObj *);
        MtxPtr effectMtx = ((DrawFn)(*(void ***)light)[3])(light);
        mModel.setScale(balloon->mScale);
        balloon->mModel.clipAll(viewNo, true);
        J3DModelData *modelData = balloon->mModel.getModelData();
        SimpleDrawer drawer;
        drawer.drawInit(modelData);
        while (drawer.loadPreDrawSetting()) {
            balloon->mModel.simpleDraw(viewNo, effectMtx, 0);
        }
        balloon->mModel.hide();
    }
}

void TBalloonString::getUpDir(JGeometry::TVec3f *up) {
    JGeometry::TVec3f *dir = (JGeometry::TVec3f *)((u8 *)RCMGetCourse() + 0x234);
    f32 y = dir->y;
    f32 z = dir->z;
    f32 x = dir->x;
    up->x = x;
    up->y = y;
    up->z = z;
}

void TBalloonString::getPoint(JGeometry::TVec3f *pos) {
    mNodeMgr->getNodePos(mNodeMgr->mStrNodeList.getNumLinks() - 1, pos);
}

void TBalloonString::getPointDir(JGeometry::TVec3f *dir) {
    JGeometry::TVec3f prev;
    JGeometry::TVec3f cur;
    mNodeMgr->getNodePos(mNodeMgr->mStrNodeList.getNumLinks() - 2, &prev);
    mNodeMgr->getNodePos(mNodeMgr->mStrNodeList.getNumLinks() - 1, &cur);
    dir->sub(cur, prev);
    dir->normalize();
}

TBalloonManager::TBalloonManager() : TMapObjHioNode(0x26af) {}

void TBalloonManager::init(u8 kartIdx, char viewNo, u8 count) {
    JUT_ASSERT_MSG(count <= 3, "num <= cBalloonMax");
    JUT_MINMAX_ASSERT(0, kartIdx, 8);
    KartInfo *info = RaceMgr::getManager()->getKartInfo(kartIdx);
    for (int i = 0; i < 3; i++) {
        if (i < count) {
            TBalloonString *balloon = (TBalloonString *)GetGeoObjMgr()->createGenSubObj(0x26ad);
            balloon->setMainViewNo(viewNo);
            EKartWeight weight = (EKartWeight)info->getWeight();
            ECharID id1 = info->getDriverCharID(0);
            ECharID id2 = info->getDriverCharID(1);
            balloon->setCharID(weight, id1, id2);
            mSpares[i] = balloon;
        } else {
            mSpares[i] = nullptr;
        }
    }
    mCount = count;
    mKartIdx = kartIdx;
    mMainViewNo = viewNo;
}

void TBalloonManager::reset() {
    mWinningNo = mCount;
    mIsWinning = 0;
    JGeometry::TVec3f pos;
    ObjUtility::getKartPos(mKartIdx, &pos);
    MtxPtr bodyMtx = ObjUtility::getKartBodyMtx(mKartIdx);
    for (int i = 0; i < 4; i++) {
        mCamDist[i] = -1.0f;
    }
    mGeoObjFlag &= ~7;
    mCheckKartHitFlags = 0;

    static JGeometry::TVec3f ydir(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f zdir;
    ObjUtility::getKartZdir(mKartIdx, &zdir);
    zdir.normalize();
    JGeometry::TVec3f right;
    right.cross(ydir, zdir);
    right.normalize();
    Mtx mtx;
    mtx[0][0] = right.x; mtx[0][1] = ydir.x; mtx[0][2] = zdir.x;
    mtx[1][0] = right.y; mtx[1][1] = ydir.y; mtx[1][2] = zdir.y;
    mtx[2][0] = right.z; mtx[2][1] = ydir.z; mtx[2][2] = zdir.z;
    Mtx bodyCopy;
    memcpy(bodyCopy, bodyMtx, sizeof(Mtx));

    for (int i = 0; i < mCount; i++) {
        mStrings[i] = mSpares[i];
        mStrings[i]->setMainViewNo(mMainViewNo);
        JUT_MINMAX_ASSERT(0, mKartIdx, 8);
        int rank = RCMGetKartChecker(mKartIdx)->getRank();
        JUT_ASSERT_MSG(rank != -1, "color != -1");
        mStrings[i]->setColor(rank);
        KartInfo *info = RaceMgr::getManager()->getKartInfo(mKartIdx);
        EKartWeight weight = (EKartWeight)info->getWeight();
        ECharID id1 = info->getDriverCharID(0);
        ECharID id2 = info->getDriverCharID(1);
        mStrings[i]->setCharID(weight, id1, id2);
        mStrings[i]->mManager = this;
        mStrings[i]->mIsAlive = 1;
        mStrings[i]->setPosition(pos, *(JGeometry::TPos3f *)bodyCopy);
        mStrings[i]->mBalloons[0]->mFrame = sStartFrame + sFrameDiff * (f32)i;
        JGeometry::TVec3f vel(sPullDirX * (1.0f - 0.5f * i), sPullDirY, -sPullDirZ);
        vel.normalize();
        PSMTXMultVecSR(mtx, &vel, &vel);
        mStrings[i]->mVelOffset = vel;
    }
}

void TBalloonManager::calc() {
    checkCameraDistance();
    JGeometry::TVec3f pos;
    ObjUtility::getKartPos(mKartIdx, &pos);
    JGeometry::TVec3f zdir;
    ObjUtility::getKartZdir(mKartIdx, &zdir);
    if (mIsWinning) {
        JMAVECScaleAdd(&zdir, &pos, &pos, sWinningOffset);
    }
    if (RaceMgr::getManager()->getRaceMode() == BALLOON_BATTLE) {
        int rank = RCMGetKartChecker(mKartIdx)->getRank();
        if (mWinningNo > rank) {
            JUT_ASSERT_MSG(mWinningNo - rank == 1, "mLastNum - curNum == 1");
            killBalloon(mStrings[mWinningNo - 1]);
            mStrings[mWinningNo - 1] = nullptr;
            mWinningNo = rank;
        }
    }
    Mtx mtx;
    PSMTXIdentity(mtx);
    static JGeometry::TVec3f ydir(0.0f, 1.0f, 0.0f);
    zdir.normalize();
    JGeometry::TVec3f right;
    right.cross(ydir, zdir);
    right.normalize();
    mtx[0][0] = right.x; mtx[0][1] = ydir.x; mtx[0][2] = zdir.x;
    mtx[1][0] = right.y; mtx[1][1] = ydir.y; mtx[1][2] = zdir.y;
    mtx[2][0] = right.z; mtx[2][1] = ydir.z; mtx[2][2] = zdir.z;
    Mtx bodyCopy;
    memcpy(bodyCopy, ObjUtility::getKartBodyMtx(mKartIdx), sizeof(Mtx));

    int alive = 0;
    for (int i = 0; i < mCount; i++) {
        if (mStrings[i] != nullptr && mStrings[i]->mIsAlive) {
            alive++;
        }
    }
    f32 base = -0.5f * sPullDirX * (alive - 1);
    for (int i = 0; i < mCount; i++) {
        if (mStrings[i] != nullptr && mStrings[i]->mIsAlive) {
            JGeometry::TVec3f vel(sPullDirX * i + base, sPullDirY, -sPullDirZ);
            vel.normalize();
            PSMTXMultVecSR(mtx, &vel, &vel);
            mStrings[i]->mVelOffset = vel;
            mStrings[i]->setPosition(pos, *(JGeometry::TPos3f *)bodyCopy);
        }
    }
}

void TBalloonManager::killBalloon(TBalloonString *string) {
    JUT_MINMAX_ASSERT(0, mKartIdx, 8);
    int rank = RCMGetKartChecker(mKartIdx)->getRank();
    u32 val = 0x117;
    if (rank != 0) {
        val = 0x118;
    }
    string->unlock(val);
    string->mVelOffset.set(0.0f, 1.0f, 0.0f);
}

void TBalloonManager::checkCameraDistance() {
    int camNum = RaceMgr::getManager()->getConsoleNumber();
    for (int i = 0; i < camNum; i++) {
        JUT_MINMAX_ASSERT(0, i, 4);
        JGeometry::TVec3f camPos = *GetKartCtrl()->getKartCam(i)->GetCameraPos();
        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mKartIdx, &kartPos);
        JGeometry::TVec3f d;
        d.sub(camPos, kartPos);
        mCamDist[i] = d.length();
    }
}

void TBalloonManager::robRivalOfBalloon(u32 rivalIdx) {
    JUT_ASSERT_MSG(rivalIdx < 8, "rivalIdx < 8");
    TBalloonManager *other = GetGeoObjMgr()->getBalloonMgr(rivalIdx);
    TBalloonString *string = other->rob();
    JUT_MINMAX_ASSERT(0, mKartIdx, 8);
    if (string != nullptr) {
        setBalloon(string);
    } else {
        JUT_ASSERT_MSG(false, "TBalloonManager(0): Rob Failed");
    }
}

TBalloonString *TBalloonManager::rob() {
    JUT_MINMAX_ASSERT(0, mKartIdx, 8);
    int rank = RCMGetKartChecker(mKartIdx)->getRank();
    TBalloonString *string = nullptr;
    if (rank >= 0) {
        string = removeBalloon();
    } else {
        JUT_ASSERT_MSG(false, "TBalloonManager(0): balloon count < 0");
    }
    return string;
}

void TBalloonManager::setBalloon(TBalloonString *string) {
    JUT_MINMAX_ASSERT(0, mKartIdx, 8);
    int rank = RCMGetKartChecker(mKartIdx)->getRank();
    string->setMainViewNo(mMainViewNo);
    string->mManager = this;
    JUT_MINMAX_ASSERT(0, mKartIdx, 8);
    KartInfo *info = RaceMgr::getManager()->getKartInfo(mKartIdx);
    EKartWeight weight = (EKartWeight)info->getWeight();
    ECharID id1 = info->getDriverCharID(0);
    ECharID id2 = info->getDriverCharID(1);
    string->setCharID(weight, id1, id2);
    TBalloonString *old = mStrings[rank - 1];
    if (old != nullptr) {
        JUT_MINMAX_ASSERT(0, mKartIdx, 8);
        int r = RCMGetKartChecker(mKartIdx)->getRank();
        u32 val = 0x117;
        if (r != 0) {
            val = 0x118;
        }
        old->unlock(val);
        old->mVelOffset.set(0.0f, 1.0f, 0.0f);
    }
    mStrings[rank - 1] = string;
    mWinningNo = rank;
}

TBalloonString *TBalloonManager::removeBalloon() {
    JUT_MINMAX_ASSERT(0, mKartIdx, 8);
    int rank = RCMGetKartChecker(mKartIdx)->getRank();
    TBalloonString *string = mStrings[rank];
    mStrings[rank] = nullptr;
    mWinningNo = rank;
    return string;
}
