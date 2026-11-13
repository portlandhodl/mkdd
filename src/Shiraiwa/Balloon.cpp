#include "Shiraiwa/Balloon.h"

#include "JSystem/J3D/J3DAnmBase.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Sato/ObjUtility.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Kaneshige/LightMgr.h"
#include "Kaneshige/SimpleDrawer.h"
#include "Yamamoto/kartCtrl.h"
#include "Yamamoto/kartCamera.h"
#include "JSystem/JMath/JMath.h"
#include "mathHelper.h"

f32 TBalloonManager::sPullDirX = 0.4f;
f32 TBalloonManager::sPullDirY = 1.0f;
f32 TBalloonManager::sPullDirZ;
f32 TBalloonManager::sWinningOffset = 30.0f;
f32 TBalloonManager::sFrameDiff = -10.0f;
f32 TBalloonManager::sStartFrame = -150.0f;

TBalloonManager::TBalloonManager() : TMapObjHioNode(0x26af) {}

void TBalloonManager::init(u8 kartIdx, s8 viewNo, u8 num) {
#line 59
    JUT_ASSERT_F(num <= 3, "%s", "num <= cBalloonMax");
    KartInfo *info = RaceMgr::getManager()->getKartInfo(kartIdx);
    for (int i = 0; i < 3; i++) {
        if (i < num) {
            TBalloonString *string = (TBalloonString *)GetGeoObjMgr()->createGenSubObj(0x26ad);
            string->setMainViewNo(viewNo);
            string->setCharID(info->getKartWeight(), info->getDriverCharID(0), info->getDriverCharID(1));
            mSpares[i] = string;
        } else {
            mSpares[i] = nullptr;
        }
    }
    mCount = num;
    mKartIdx = kartIdx;
    mMainViewNo = viewNo;
}

void TBalloonManager::reset() {
    mWinningNo = mCount;
    mIsWinning = false;
    JGeometry::TVec3f pos;
    ObjUtility::getKartPos(mKartIdx, &pos);
    JGeometry::TPos3f bodyMtx;
    bodyMtx.set(ObjUtility::getKartBodyMtx(mKartIdx));
    for (int i = 0; i < 4; i++) {
        mCamDist[i] = -1.0f;
    }
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    f32 pullY = sPullDirY;
    f32 pullZ = -1.0f * sPullDirZ;
    KartInfo *info = RaceMgr::getManager()->getKartInfo(mKartIdx);

    static JGeometry::TVec3f ydir(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f right;
    JGeometry::TVec3f zdir;
    ObjUtility::getKartZdir(mKartIdx, &zdir);
    zdir.y = 0.0f;
    zdir.normalize();
    right.cross(ydir, zdir);
    right.normalize();
    Mtx mtx;
    mtx[0][0] = right.x;
    mtx[1][0] = right.y;
    mtx[2][0] = right.z;
    mtx[0][1] = ydir.x;
    mtx[1][1] = ydir.y;
    mtx[2][1] = ydir.z;
    mtx[0][2] = zdir.x;
    mtx[1][2] = zdir.y;
    mtx[2][2] = zdir.z;

    for (int i = 0; i < mCount; i++) {
        mStrings[i] = mSpares[i];
        mStrings[i]->setMainViewNo(mMainViewNo);
        int color = RCMGetKartChecker(mKartIdx)->getPlayerKartColor();
        u32 c = color;
#line 137
        JUT_ASSERT_F(color != -1, "%s", "color != -1");
        mStrings[i]->setColor(c);
        mStrings[i]->setCharID(info->getKartWeight(), info->getDriverCharID(0), info->getDriverCharID(1));
        mStrings[i]->mManager = this;
        mStrings[i]->mIsAlive = 1;
        mStrings[i]->setPosition(pos, bodyMtx);
        mStrings[i]->mBalloons[0]->mFrame = i * sFrameDiff + sStartFrame;
        JGeometry::TVec3f vel(0.0f, pullY, pullZ);
        vel.x += i * (-0.5f * sPullDirX) + sPullDirX;
        vel.normalize();
        PSMTXMultVecSR(mtx, &vel, &vel);
        mStrings[i]->mVelOffset.set(vel);
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
        u8 curNum = RCMGetKartChecker(mKartIdx)->getBalloonNumber();
        if (mWinningNo > curNum) {
#line 193
            JUT_ASSERT_F(mWinningNo - curNum == 1, "%s", "mLastNum - curNum == 1");
            killBalloon(mStrings[mWinningNo - 1]);
            mStrings[mWinningNo - 1] = nullptr;
            mWinningNo = curNum;
        }
    }
    Mtx mtx;
    PSMTXIdentity(mtx);
    static JGeometry::TVec3f ydir(0.0f, 1.0f, 0.0f);
    zdir.y = 0.0f;
    zdir.normalize();
    JGeometry::TVec3f right;
    right.cross(ydir, zdir);
    right.normalize();
    mtx[0][0] = right.x;
    mtx[1][0] = right.y;
    mtx[2][0] = right.z;
    mtx[0][1] = ydir.x;
    mtx[1][1] = ydir.y;
    mtx[2][1] = ydir.z;
    mtx[0][2] = zdir.x;
    mtx[1][2] = zdir.y;
    mtx[2][2] = zdir.z;

    int alive = 0;
    for (int i = 0; i < mCount; i++) {
        if (mStrings[i] != nullptr && mStrings[i]->mIsAlive) {
            alive++;
        }
    }
    JGeometry::TPos3f bodyMtx;
    bodyMtx.set(ObjUtility::getKartBodyMtx(mKartIdx));
    f32 base = -0.5f * sPullDirX * (alive - 1);
    for (int i = 0; i < mCount; i++) {
        if (mStrings[i] != nullptr && mStrings[i]->mIsAlive) {
            JGeometry::TVec3f vel(0.0f, sPullDirY, -1.0f * sPullDirZ);
            vel.x = sPullDirX * i + base;
            vel.normalize();
            PSMTXMultVecSR(mtx, &vel, &vel);
            mStrings[i]->mVelOffset.set(vel);
            mStrings[i]->setPosition(pos, bodyMtx);
        }
    }
}

void TBalloonManager::killBalloon(TBalloonString *string) {
    int num = RCMGetKartChecker(mKartIdx)->getBalloonNumber();
    u32 se = 0x40075;
    if (num == 0) {
        se = 0x40076;
    }
    string->unlock(se);
    string->mVelOffset.set(0.0f, 1.0f, 0.0f);
}

void TBalloonManager::checkCameraDistance() {
    int camNum = RaceMgr::getManager()->getCameraNumber();
    for (int i = 0; i < camNum; i++) {
        const JGeometry::TVec3f *camPos = (const JGeometry::TVec3f *)RaceMgr::getManager()->getCamera(i)->GetCameraPos();
        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mKartIdx, &kartPos);
        JGeometry::TVec3f d;
        d.sub(kartPos, *camPos);
        mCamDist[i] = d.length();
    }
}

void TBalloonManager::robRivalOfBalloon(u32 robbedKart) {
#line 322
    JUT_MINMAX_ASSERT(0, robbedKart, 8);
    TBalloonString *string = GetGeoObjMgr()->getBalloonMgr(robbedKart)->rob();
    RCMGetKartChecker(mKartIdx);
    if (string != nullptr) {
        setBalloon(string);
    } else {
#line 330
        JUT_ASSERT_F(false, "TBalloonManager(%d): Rob Failed", mKartIdx);
    }
}

TBalloonString *TBalloonManager::rob() {
    TBalloonString *string = nullptr;
    int num = RCMGetKartChecker(mKartIdx)->getBalloonNumber();
    if (num >= 0) {
        string = removeBalloon();
    } else {
#line 346
        JUT_ASSERT_F(false, "TBalloonManager(%d): balloon count: %d < 0", mKartIdx, num);
    }
    return string;
}

void TBalloonManager::setBalloon(TBalloonString *string) {
    int num = RCMGetKartChecker(mKartIdx)->getBalloonNumber();
    string->setMainViewNo(mMainViewNo);
    string->mManager = this;
    KartInfo *info = RaceMgr::getManager()->getKartInfo(mKartIdx);
    string->setCharID(info->getKartWeight(), info->getDriverCharID(0), info->getDriverCharID(1));
    TBalloonString *old = mStrings[num - 1];
    if (old != nullptr) {
        killBalloon(old);
    }
    mStrings[num - 1] = string;
    mWinningNo = num;
}

TBalloonString *TBalloonManager::removeBalloon() {
    int num = RCMGetKartChecker(mKartIdx)->getBalloonNumber();
    TBalloonString *string = mStrings[num];
    mStrings[num] = nullptr;
    mWinningNo = num;
    return string;
}

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
J3DAnmTevRegKey *TBalloon::sBalloonBrkAnm;

TBalloon::TBalloon() : GeographyObj(0x26ae) {
    mObjFlag |= 1;
    mObjFlag |= 0x200;
    createSoundMgr();
}

void TBalloon::reset() {
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    mGeoObjFlag |= 0x20;
    mFlag = 0;
    mCount = 0;
}

void TBalloon::setColor(u32 color) {
    mMaterial.setFrame(color);
}

void TBalloon::update() {
    mMaterial.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}

void TBalloon::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *anm = ObjUtility::getPtrCourseArc("/Objects/MiniGame_Balloon.brk");
#line 498
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

void TBalloon::die() {
    mFlag = 1;
    if (mCount != 0) {
        if (!RaceMgr::getManager()->isRaceEnd()) {
            getSoundMgr()->setSe(mCount);
        }
    }
    JPABaseEmitter *emt = JPEffectMgr::mThis->createEmt(scEfctBreakA, mPos);
    emt->setGlobalScale(mScale);
    emt = JPEffectMgr::mThis->createEmt(scEfctBreakB, mPos);
    emt->setGlobalScale(mScale);
    emt = JPEffectMgr::mThis->createEmt(scEfctBreakColor[(u32)mMaterial.getFrame()], mPos);
    emt->setGlobalScale(mScale);
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

void TBalloon::draw(u32 viewNo) {
    if (!mFlag) {
        mMaterial.anmFrameProc();
        MtxPtr effectMtx = LightMgr::getManager()->searchLight('BLN0' + viewNo)->getEffectMtx();
        mModel.setScale(mScale);
        mModel.clipAll(viewNo, true);
        J3DModelData *modelData = mModel.getModelData();
        SimpleDrawer drawer;
        drawer.drawInit(modelData);
        while (drawer.loadPreDrawSetting()) {
            mModel.simpleDraw(viewNo, effectMtx, 0);
        }
    }
    mModel.hide();
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

f32 TBalloonString::sAdjustRatio = 1.0f;
f32 TBalloonString::sAirFric = 0.9f;
f32 TBalloonString::sPower = 20.0f;
f32 TBalloonString::sDemoPower = 5.0f;
f32 TBalloonString::sGravity = -0.3f;
u8 TBalloonString::sNodeNum = 3;
f32 TBalloonString::sOrgScale = 0.35f;
f32 TBalloonString::sOrgLength = 1200.0f;
f32 TBalloonString::sBreakOffset = 150.0f;
f32 TBalloonString::sBreakUpVel = 4.0f;
f32 TBalloonString::sShakePow = 5.0f;
f32 TBalloonString::sOffset[12] = {
    0.0f, 70.0f, -25.0f,
    0.0f, 80.0f, -20.0f,
    0.0f, 100.0f, -30.0f,
    0.0f, 120.0f, -30.0f,
};
f32 TBalloonString::sTotalLength[21] = {
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f,
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f,
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 180.0f,
};
TBalloonSupervisor *TBalloonString::sSupervisor;

TBalloonString::TBalloonString() : TMapObjHioNode(0x26ad), JKRDisposer(), mLink(this) {
    init();
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

TBalloonString::~TBalloonString() {
    sSupervisor = nullptr;
    delete mNodeMgr;
}

void TBalloonString::setColor(u32 color) {
    for (int i = 0; i < 8; i++) {
        if (mBalloons[i] != nullptr) {
            mBalloons[i]->setColor(color);
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

f32 TBalloonString::getCameraDistance(u32 viewNo) {
    return mManager->getCameraDistance(viewNo);
}

void TBalloonString::setMainViewNo(s8 viewNo) {
    mMainViewNo = viewNo;
}

void TBalloonString::reset() {
    resetObject();
    sOrgLength = 1200.0f;
    sOrgScale = 0.35f;
    if (mBalloons[0] != nullptr) {
        mBalloons[0]->mScaleRatio = sOrgScale;
    }
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~0x20;
    PSMTXIdentity(mRotMtx);
    mVel.set(0.0f, sBreakUpVel, 0.0f);
    mNodeMgr->resetNodeAll(nullptr);
    mIsAlive = 1;
    JGeometry::TVec3f dir(0.0f, 1.0f, 0.0f);
    for (u32 i = 0; i < sNodeNum; i++) {
        JGeometry::TVec3f pos;
        JMAVECScaleAdd(&pos, &dir, &pos, i * sOrgLength);
        mNodeMgr->setNodePos(i, pos);
        mNodeMgr->getStringNode(i)->_30 = 0;
    }
    resetNodeLength();
    setParameters();
    mNodeMgr->calc();
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

void TBalloonString::calc() {
    if (mManager == nullptr) {
        return;
    }
    JGeometry::TVec3f up;
    getUpDir(&up);
    *(JGeometry::TVec3f *)&mNodeMgr->_28 = up;
    mNodeMgr->calc();
    if (!mIsAlive) {
        mPos.add(mVel);
        if (mPos.y - mStartY > sBreakOffset) {
            die();
        }
    }
    mNodeMgr->setNodePos(0, mPos);
    JGeometry::TVec3f vel;
    vel.set(mVelOffset);
    f32 power = sPower;
    if (!RaceMgr::getManager()->isAbleStart()) {
        power = sDemoPower;
    }
    vel.scale(power);
    mNodeMgr->addNodeVel(mNodeMgr->mStrNodeList.getNumLinks() - 1, vel);
}

void TBalloonString::die() {
    if (mGeoObjFlag & 0x20) {
        return;
    }
    for (int i = 0; i < 8; i++) {
        if (mBalloons[i] != nullptr) {
            mBalloons[i]->die();
        }
    }
    mGeoObjFlag |= 0x20;
}

void TBalloonString::shake() {
    f32 rnd = getGeoRnd()->get_ufloat_1();
    s16 ang = 10430.378f * (6.2831855f * rnd);
    JGeometry::TVec3f vel;
    vel.x = sShakePow * JMASCos(ang);
    vel.z = sShakePow * JMASSin(ang);
    mNodeMgr->addNodeVel(mNodeMgr->mStrNodeList.getNumLinks() - 1, vel);
}

void TBalloonString::drawBalloon(u32 viewNo) {
    if (mGeoObjFlag & 0x20) {
        return;
    }
    for (int i = 0; i < 8; i++) {
        if (mBalloons[i] != nullptr) {
            mBalloons[i]->draw(viewNo);
        }
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

void TBalloonString::setParameters() {
    mNodeMgr->_24 = sAdjustRatio;
    mNodeMgr->_20 = sAirFric;
    mNodeMgr->_1c = sGravity;
}

#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
