#include "Sato/stEffectMgr.h"
#include "Sato/stEmitter.h"
#include "Sato/stEfctWater.h"
#include "Sato/stEfctRoad.h"
#include "Sato/stEfctWeed.h"
#include "Sato/ObjUtility.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/LumbarCtrl.h"
#include "Sato/StringObj.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/KartLoader.h"
#include "Kaneshige/RaceDirector.h"
#include "Kaneshige/SysDebug.h"
#include "Kawano/driver.h"
#include "Yamamoto/kartCtrl.h"
#include "Yamamoto/kartPad.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/stMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/ResMgr.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "JSystem/J3D/J3DTransform.h"

#include <stdio.h>

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

namespace {
char sPathName[64];
f32 sClipMaxLength = 5000.0f;
f32 sWaterBodyHeightOffset[21] = {
    50.0f, 55.0f, 55.0f, 43.0f, 60.0f, 50.0f, 43.0f, 65.0f, 50.0f, 55.0f, 55.0f,
    43.0f, 60.0f, 50.0f, 50.0f, 47.0f, 27.0f, 27.0f, 70.0f, 70.0f, 40.0f,
};
} // namespace

stEffectMgr *stEffectMgr::mMgr;

#define ST_EFCT_ASSERT_NAME(name, i, n, line)                                                                      \
    ((0 <= (i) && (i) < (n)) ? (void)0                                                                             \
                            : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "stEffectMgr.cpp", (line),   \
                                                          "range over: %d <= " name "=%d < %d", 0, (i), (n)),      \
                               OSPanic("stEffectMgr.cpp", (line), "Halt")))

#define ST_EFCT_ASSERT_MAX(name, i, n, line)                                                                       \
    (((i) < (n)) ? (void)0                                                                                         \
                 : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "stEffectMgr.cpp", (line),              \
                                               "range over: %d <= " name "=%d < %d", 0, (i), (n)),                 \
                    OSPanic("stEffectMgr.cpp", (line), "Halt")))

#define ST_EFCT_ASSERT(i, line) ST_EFCT_ASSERT_NAME("kart_index", i, 8, line)
#define ST_EFCT_ASSERT_N(i, n, line) ST_EFCT_ASSERT_NAME("kart_index", i, n, line)

#define ST_EFCT_ASSERT_CHK(i, n, line, min)                                                                       \
    (((min) <= (i) && (i) < (n))                                                                                  \
         ? (void)0                                                                                                \
         : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "stEffectMgr.cpp", (line),                   \
                                       "range over: %d <= kart_index=%d < %d", (min), (i), (n)),                  \
            OSPanic("stEffectMgr.cpp", (line), "Halt")))

stEffectMgr::stEffectMgr()
{
    mpClipParam = nullptr;
    mMgr = this;
    _138c = 0;
    for (u8 i = 0; i < 8; i++)
        mIsAvailableDistance[i] = 1;

    SysDebug::getManager()->setHeapGroup("PARTICLE DATA", nullptr);
    mpEmitterTemplate = new stEmitter *[1];
    stEmitter **tmpl = mpEmitterTemplate;
    tmpl[0] = (stEmitter *)ResMgr::getPtr(ResMgr::mcArcMRAM, getSnpName(0));
    loadCourseParam(RaceMgr::getManager()->getCourse()->getCourseID());

    SysDebug::getManager()->setHeapGroup("PARTICLE MGR", nullptr);
    mpTireStatusSet = new stTireEfctStatusSet[RaceMgr::getManager()->getKartNumber()];
    mpCenterStatus = new stTireEffectStatus[RaceMgr::getManager()->getKartNumber()];
    mpCharStatus = new stTireEffectStatus[RaceMgr::getManager()->getKartNumber()];
    mpWaterStatus = new stWaterEffectStatus[RaceMgr::getManager()->getKartNumber()];
    createEmitterAndParticles();
    mpExhaustAccelCounter = new stExhaustAccelCounter[RaceMgr::getManager()->getKartNumber()];
}

__declspec(weak) stEffectMgr::stWaterEffectStatus::stWaterEffectStatus()
{
    u8 i = 0;
    stEmitter *a = (stEmitter *)i;
    stEmitter *b = (stEmitter *)i;
    for (; i < 2; i++) {
        mTrack[i] = a;
        mBubble[i] = b;
        mSpray[i] = b;
    }
}

__declspec(weak) stEffectMgr::stTireEffectStatus::stTireEffectStatus()
{
    init();
}

__declspec(weak) stEffectMgr::stTireEfctStatusSet::stTireEfctStatusSet() {}

void stEffectMgr::createEmitterAndParticles()
{
    stEmitterPtcl *emt = new stEmitterPtcl[200];
    for (u16 i = 0; i < 200; i++, emt++) {
        emt->mParticle.mDynamics.mFreeList = &mDynamics0;
        emt->mParticle.mDynamics.mEmitter = emt;
        mFreeEmt0.mList.append(&emt->_134);
    }

    mParticles0 = new stParticle[800];
    stParticle *ptcl = mParticles0;
    for (u16 i = 0; i < 800; i++, ptcl++) {
        mDynamics0.mList.append(&ptcl->mLink);
    }
    mEmtMgr0.mFreeList = &mFreeEmt0.mList;

    stEmitterPtcl *emt2 = new stEmitterPtcl[50];
    for (u16 i = 0; i < 50; i++, emt2++) {
        emt2->mParticle.mDynamics.mFreeList = &mDynamics2;
        emt2->mParticle.mDynamics.mEmitter = emt2;
        mFreeEmt2.mList.append(&emt2->_134);
    }

    mParticles2 = new stParticle[400];
    stParticle *ptcl2 = mParticles2;
    for (u16 i = 0; i < 400; i++, ptcl2++) {
        mDynamics2.mList.append(&ptcl2->mLink);
    }
    mEmtMgr2.mFreeList = &mFreeEmt2.mList;
}

__declspec(weak) stParticle::~stParticle() {}

__declspec(weak) stParticle::stParticle() : mLink(this) {}

__declspec(weak) stEmitterPtcl::~stEmitterPtcl() {}

__declspec(weak) stEmitterPtcl::stEmitterPtcl() : _130(nullptr), _134(this)
{
    _146 = 0;
}

void stEffectMgr::createKartExhaustMtx()
{
    int kartNum = RaceMgr::getManager()->getKartNumber();
    mpKartBodyMtxA = new Mtx[kartNum];
    mpKartBodyMtxB = new Mtx[kartNum];
    mpKartExhaustMtxL = new Mtx[kartNum];
    mpKartExhaustMtxR = new Mtx[kartNum];
    mpMufllerL = new u8[kartNum];
    mpMufllerR = new u8[kartNum];
    mpEfctCarSpeed = new f32[kartNum];

    Mtx *mtxA = mpKartBodyMtxA;
    Mtx *mtxB = mpKartBodyMtxB;
    for (int i = 0; i < kartNum; i++, mtxA++, mtxB++) {
        J3DModelData *modelData = RaceMgr::getManager()->getKartLoader(i)->getExModelBody()->getModelData();

        int idx = modelData->getJointName()->getIndex("ef_exh_l");
        if (idx != -1) {
            J3DGetTranslateRotateMtx(modelData->getJointNodePointer(idx)->getTransformInfo(), *mtxA);
            mpMufllerL[i] = 1;
        } else {
            PSMTXIdentity(*mtxA);
            mpMufllerL[i] = 0;
        }
        mpEfctCarSpeed[i] = 0.0f;

        idx = modelData->getJointName()->getIndex("ef_exh_r");
        if (idx != -1) {
            J3DGetTranslateRotateMtx(modelData->getJointNodePointer(idx)->getTransformInfo(), *mtxB);
            mpMufllerR[i] = 1;
        } else {
            PSMTXIdentity(*mtxB);
            mpMufllerR[i] = 0;
        }
    }
}

void stEffectMgr::model_init()
{
    createKartExhaustMtx();
    stEfctExhaust **exhaustL = mpExhaustL;
    stEfctExhaust **exhaustR = mpExhaustR;
    int kartNum = RaceMgr::getManager()->getKartNumber();
    for (u8 i = 0; i < kartNum; i++, exhaustL++, exhaustR++) {
        if (mpMufllerL[i]) {
            *exhaustL = new stEfctExhaust(i, 0);
            if (RaceMgr::getManager()->getKartInfo(i)->isAvailableKart() &&
                !RaceMgr::getManager()->getKartInfo(i)->isGhostKart())
                createKartCommonEmitter(i, *exhaustL, _3b8, false);
        }
        if (mpMufllerR[i]) {
            *exhaustR = new stEfctExhaust(i, 1);
            if (RaceMgr::getManager()->getKartInfo(i)->isAvailableKart() &&
                !RaceMgr::getManager()->getKartInfo(i)->isGhostKart())
                createKartCommonEmitter(i, *exhaustR, _3b8, false);
        }
    }
}

void stEffectMgr::createModel(JKRSolidHeap *, u32)
{
    model_init();
}

void stEffectMgr::init()
{
    int kartNum = RaceMgr::getManager()->getKartNumber();
    mpExhaustL = new stEfctExhaust *[kartNum];
    mpExhaustR = new stEfctExhaust *[kartNum];
    mpKinokoDash[0][0] = new stEfctKinokoDash1[kartNum];
    mpKinokoDash[1][0] = new stEfctKinokoDash1[kartNum];
    mpKinokoDash[0][1] = new stEfctKinokoDash2[kartNum];
    mpKinokoDash[1][1] = new stEfctKinokoDash2[kartNum];
    mpKinokoDash[0][2] = new stEfctKinokoDash3[kartNum];
    mpKinokoDash[1][2] = new stEfctKinokoDash3[kartNum];
    mpWaterTrack = new stEfctWaterTrack[kartNum];
    mpWaterSprayCenter = new stEfctWaterSprayCenter[kartNum];
    for (u8 i = 0; i < 2; i++) {
        mpWaterSpray[i] = new stEfctWaterSpray[kartNum];
        mpWaterBubble[i] = new stEfctWaterBubble[kartNum];
    }
    for (u8 i = 0; i < 2; i++) {
        mpCrsWaterSpray[i] = new stEfctCrsWaterSpray[kartNum];
        mpCrsWaterBubble[i] = new stEfctCrsWaterBubble[kartNum];
    }

    for (u8 i = 0; i < kartNum; i++) {
        mpRoadCenterCb[0][i] = new stEfctDirtCenter();
        mpRoadCenterCb[0][i]->kartNum = i;
        mpRoadCenterCb[1][i] = new stEfctDirtCenter();
        mpRoadCenterCb[1][i]->kartNum = i;
        mpRoadCharCenterCb[i] = new stEfctCharCenter();
        mpRoadCharCenterCb[i]->kartNum = i;
        mpWeedCharCenterCb[i] = new stEfctCharWeedCenter();
        mpWeedCharCenterCb[i]->kartNum = i;

        for (s16 j = 0; j < 2; j++) {
            s16 tire = j + 2;
            mpWeedCb[i][j] = new stEfctWeed();
            initCallBackPtr(mpWeedCb[i][j], i, j);
            mpRoadCb[i][j] = new stEfctRoad();
            initCallBackPtr(mpRoadCb[i][j], i, tire);
            mpTireSpinCb[i][j] = new stEfctTireSpin();
            initCallBackPtr(mpTireSpinCb[i][j], i, tire);
            mpSpinTurnCb[i][j] = new stEfctSpinTurn();
            initCallBackPtr(mpSpinTurnCb[i][j], i, tire);
            mpDriftCb[i][j] = new stEfctRoadDriftSmoke();
            initCallBackPtr(mpDriftCb[i][j], i, tire);
            mpDriftCreateCb[i][j] = new stEfctRoadDrift(i, tire);
            mpDriftChipCb[i][j] = new stEfctRoadDriftChip();
            initCallBackPtr(mpDriftChipCb[i][j], i, tire);
        }

        mpWaterTrack[i].kartNum = i;
        mpWaterSprayCenter[i].kartNum = i;
        for (int j = 0; j < 2; j++) {
            s16 tire = j + 2;
            initCallBackPtr(&mpWaterSpray[j][i], i, tire);
            initCallBackPtr(&mpWaterBubble[j][i], i, tire);
            initCallBackPtr(&mpCrsWaterSpray[j][i], i, tire);
            initCallBackPtr(&mpCrsWaterBubble[j][i], i, tire);
        }
    }
}

void stEffectMgr::initCallBackPtr(stEmitterCallBack *cb, s16 a, s16 b)
{
    cb->kartNum = a;
    cb->_22 = b;
}

void stEffectMgr::reset()
{
    mEmtMgr0.removeAllEmt();
    mEmtMgr2.removeAllEmt();
    for (u8 i = 0; i < 8; i++) {
        mIsAvailableDistance[i] = 1;
        mDriftCnt[i] = 0;
    }

    stEfctExhaust **exhaustL = mpExhaustL;
    stEfctExhaust **exhaustR = mpExhaustR;
    int kartNum = RaceMgr::getManager()->getKartNumber();
    for (int i = 0; i < kartNum; i++, exhaustL++, exhaustR++) {
        mpExhaustAccelCounter[i].init();
        mpEfctCarSpeed[i] = 0.0f;
        if (RaceMgr::getManager()->getKartInfo(i)->isAvailableKart() &&
            !RaceMgr::getManager()->getKartInfo(i)->isGhostKart()) {
            if (mpMufllerL[i])
                createKartCommonEmitter(i, *exhaustL, _3b8, false);
            if (mpMufllerR[i])
                createKartCommonEmitter(i, *exhaustR, _3b8, false);
        }
        for (int j = 0; j < 2; j++)
            mpTireStatusSet[i].mStatus[j].init();
        mpCenterStatus[i].init();
        mpWaterStatus[i].init();
        mpCharStatus[i].init();
    }
}

void stEffectMgr::calc()
{
    ObjUtility::chkAvailableEnemyByDistande((bool *)mIsAvailableDistance, sClipMaxLength);
    Mtx *pd, *pc, *pb, *pa;
    int i;
    int kartNum;

    for (i = 0; i < (kartNum = RaceMgr::getManager()->getKartNumber()); i++)
        mpExhaustAccelCounter[i].calc(i);

    pa = mpKartBodyMtxA;
    pb = mpKartBodyMtxB;
    pc = mpKartExhaustMtxL;
    pd = mpKartExhaustMtxR;
    for (i = 0; i < kartNum; i++, pa++, pb++, pc++, pd++) {
        Mtx &bodyMtx = ObjUtility::getKartBodyMtx(i);
        PSMTXConcat(bodyMtx, *pa, *pc);
        PSMTXConcat(bodyMtx, *pb, *pd);
    }

    calcGndAttrEffect();
    mEmtMgr2.calc();
    mEmtMgr0.calc();
}
void stEffectMgr::calcGndAttrEffect()
{
    int i = 0;
    bool inWater;
    stTireEfctStatusSet *set = mpTireStatusSet;
    for (; i < RaceMgr::getManager()->getKartNumber(); i++, set++) {
        JGeometry::TVec3f vel;
        KartCtrl::getKartCtrl()->GetKartEffctVel(i, &vel);
        mpEfctCarSpeed[i] = PSVECMag(&vel);
        inWater = calcWaterAttrEffect(i);
        calcGndTireEffect(i, inWater, set);
        if (!RaceMgr::getManager()->getKartInfo(i)->isGhostKart()) {
            calcGndCenterEffect(i, inWater);
            calcGndCharEffect(i, inWater);
        }
        mDriftCnt[i] = (u8)KartCtrl::getKartCtrl()->GetDriftCnt(i);
    }
}

void stEffectMgr::calcGndCenterEffect(int kartIndex, bool inWater)
{
    int attr;
    u32 attrIdx;
    CrsGround *ground = KartCtrl::getKartCtrl()->GetBodyGround(kartIndex);
    stTireEffectStatus *status = &mpCenterStatus[kartIndex];
    attr = ground->getAttribute();
    attrIdx = ground->getAttrIndex();

    if (status->mEmitter[0] && (status->mAttr != attr || status->mAttrIdx != attrIdx || inWater)) {
        status->mEmitter[0]->mFlags |= 0x200;
        status->mEmitter[0] = nullptr;
        for (u8 i = 1; i < 2; i++) {
            if (status->mEmitter[i]) {
                status->mEmitter[i]->mFlags |= 0x200;
                status->mEmitter[i] = nullptr;
            }
        }
    }

    for (u8 i = 0; i < 2; i++) {
        if (status->mEmitter[i] == nullptr) {
            stEmitterCallBack::stFileParamPtrList *ptr = findParamPtrCenter(kartIndex, ground, status, i);
            if (ptr && ptr->_00) {
                switch (attr) {
                case CrsGround::Attr_0:
                case CrsGround::Attr_1:
                case CrsGround::Attr_11:
                case CrsGround::Attr_12:
                case CrsGround::Attr_Puller:
                case CrsGround::Attr_19:
                    calcAttrRoadCenter(kartIndex, status, i);
                    break;
                case CrsGround::Attr_Water:
                    if (!inWater)
                        calcAttrRoadCenter(kartIndex, status, i);
                    break;
                }
                stEmitterPtcl *emt = (stEmitterPtcl *)status->mEmitter[i];
                if (emt) {
                    emt->_130->_24 = status->_24;
                    emt->_130->setIpData(ptr);
                    emt->mParticle.init(emt->_130->kartNum);
                }
            }
        }
    }
    status->mAttr = attr;
    status->mAttrIdx = attrIdx;
}

void stEffectMgr::calcGndTireEffect(int kartIndex, bool inWater, stTireEfctStatusSet *set)
{
    for (int i = 0; i < 2; i++) {
        CrsGround *ground;
        switch (i) {
        case 0:
            ground = KartCtrl::getKartCtrl()->GetRLTireGround(kartIndex);
            break;
        case 1:
            ground = KartCtrl::getKartCtrl()->GetRRTireGround(kartIndex);
            break;
        }
        stTireEffectStatus *status = &set->mStatus[i];
        int attr = ground->getAttribute();
        u32 attrIdx = ground->getAttrIndex();
        setDriftStatus(kartIndex, i + 2, status);

        u8 stopFlg = false;
        if (RaceMgr::getManager()->isKartGoal(kartIndex) ||
            !(KartCtrl::getKartCtrl()->GetGameStatus(kartIndex) & 0x2000) ||
            (KartCtrl::getKartCtrl()->getKartBody(kartIndex)->mCarStatus & 0x2000000000000ULL))
            stopFlg = true;

        bool stop = stopFlg != 0;
        u8 driftEndFlg = status->_24 == 0 || stop || inWater;
        bool driftEnd = driftEndFlg != 0;
        u8 changedFlg = status->mAttr != attr || status->mAttrIdx != attrIdx;
        bool changed = changedFlg != 0;
        calcDriftEfct(kartIndex, i, status, driftEnd);

        KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(kartIndex);
        KartPad *kartPad = KartCtrl::getKartCtrl()->getKartPad(kartIndex);
        u32 mask = kartPad->mTrigL | kartPad->mTrigR;
        u32 trigger = mask & pad->getTrigger();
        u32 button = mask & pad->getButton();
        if (!inWater && !stop && !RaceMgr::getManager()->mRaceDirector->isRaceEnd() &&
            __fabsf(KartCtrl::getKartCtrl()->GetCarSpeed(kartIndex)) > 15.0f && (trigger || button)) {
            for (u8 j = 0; j < 2; j++)
                calcDriftSparkEfct(kartIndex, j, &set->mStatus[j], trigger);
        }

        if (!RaceMgr::getManager()->getKartInfo(kartIndex)->isGhostKart()) {
            calcSideGEfct(kartIndex, i, status, inWater);
            calcSpinTurnEfct(kartIndex, i, status, inWater);
            for (u8 j = 0; j < 2; j++) {
                if (status->mEmitter[j] && (changed || inWater)) {
                    status->mEmitter[j]->mFlags |= 0x200;
                    status->mEmitter[j] = nullptr;
                }
                if (status->mEmitter[j] == nullptr) {
                    stEmitterCallBack::stFileParamPtrList *ptr = findParamPtrTire(kartIndex, ground, status, j);
                    if (ptr && ptr->_00) {
                        switch (attr) {
                        case CrsGround::Attr_0:
                        case CrsGround::Attr_1:
                        case CrsGround::Attr_3:
                        case CrsGround::Attr_11:
                        case CrsGround::Attr_12:
                        case CrsGround::Attr_Puller:
                        case CrsGround::Attr_19:
                            calcAttrRoad(kartIndex, i, status, j);
                            break;
                        case CrsGround::Attr_Water:
                            if (!inWater)
                                calcAttrRoad(kartIndex, i, status, j);
                            break;
                        }
                        stEmitterPtcl *emt = (stEmitterPtcl *)status->mEmitter[j];
                        if (emt) {
                            emt->_130->_24 = status->_24;
                            emt->_130->setIpData(ptr);
                            emt->mParticle.init(emt->_130->kartNum);
                        }
                    }
                }
            }
        }
        status->mAttr = attr;
        status->mAttrIdx = attrIdx;
    }
}

void stEffectMgr::calcDriftSparkEfct(int kartIndex, u8 side, stTireEffectStatus *status, u32 flag)
{
    Vec pos;
    bool ret;
    if (side == 0)
        ret = KartCtrl::getKartCtrl()->GetLeftTirePos(kartIndex, &pos);
    else
        ret = KartCtrl::getKartCtrl()->GetRightTirePos(kartIndex, &pos);

    if (ret) {
        if (flag != 0) {
            int type = 16;
            if (side == 1)
                type = 17;
            JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)type, kartIndex, *(JGeometry::TVec3f *)&pos,
                                         (u8)side);
        }
        if (status->mDriftEmitters[2] == nullptr) {
            status->mDriftEmitters[2] =
                createKartCommonEmitter(kartIndex, mpDriftCb[kartIndex][side], (u16)_3cc, *(bool *)&status->_24);
        }
    }
}

void stEffectMgr::calcSideGEfct(int kartIndex, int tire, stTireEffectStatus *status, bool inWater)
{
    if (status->mSideGEmitter) {
        if (inWater || status->_24 ||
            __fabsf(KartCtrl::getKartCtrl()->GeTireG(kartIndex)) < stEfctTireSpin::sMinSideG ||
            (KartCtrl::getKartCtrl()->getKartBody(kartIndex)->mCarStatus & 0x2000000000000ULL)) {
            status->mSideGEmitter->mFlags |= 0x200;
            status->mSideGEmitter = nullptr;
        }
    } else {
        if (__fabsf(KartCtrl::getKartCtrl()->GeTireG(kartIndex)) >= stEfctTireSpin::sMinSideG && !inWater &&
            !status->_24 && !RaceMgr::getManager()->isKartGoal(kartIndex) &&
            !RaceMgr::getManager()->mRaceDirector->isRaceEnd() && status->mSideGEmitter == nullptr) {
            stEmitterCallBack::stFileParamPtrList *ptr = findParamPtrCommon(_3d0);
            if (ptr) {
                status->mSideGEmitter = createEmitter(mpTireSpinCb[kartIndex][tire], _3d0 + 1);
                stEmitterPtcl *emt = (stEmitterPtcl *)status->mSideGEmitter;
                if (emt) {
                    emt->_130->_24 = status->_24;
                    emt->_130->setIpData(ptr);
                    emt->mParticle.init(emt->_130->kartNum);
                }
            }
        }
    }
}

void stEffectMgr::calcSpinTurnEfct(int kartIndex, int tire, stTireEffectStatus *status, bool inWater)
{
    if (!(KartCtrl::getKartCtrl()->getKartBody(kartIndex)->mCarStatus & 8) || inWater) {
        if (status->mSpinTurnEmitter) {
            status->mSpinTurnEmitter->mFlags |= 0x200;
            status->mSpinTurnEmitter = nullptr;
        }
    } else if (status->mSpinTurnEmitter == nullptr) {
        stEmitterCallBack::stFileParamPtrList *ptr = findParamPtrCommon(_3d8);
        if (ptr) {
            status->mSpinTurnEmitter = createEmitter(mpSpinTurnCb[kartIndex][tire], _3d8 + 1);
            stEmitterPtcl *emt = (stEmitterPtcl *)status->mSpinTurnEmitter;
            if (emt) {
                emt->_130->_24 = status->_24;
                emt->_130->setIpData(ptr);
                emt->mParticle.init(emt->_130->kartNum);
            }
        }
    }
}

void stEffectMgr::calcDriftEfct(int kartIndex, int tire, stTireEffectStatus *status, bool driftEnd)
{
    int driftCnt = (u8)KartCtrl::getKartCtrl()->GetDriftCnt(kartIndex);
    bool cntChanged = false;
    if (driftCnt > 0 && driftCnt != mDriftCnt[kartIndex])
        cntChanged = true;

    u8 remove = driftEnd | cntChanged;
    if (status->mDriftEmitters[0] && remove) {
        status->mDriftEmitters[0]->mFlags |= 0x200;
        status->mDriftEmitters[0] = nullptr;
    }

    KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(kartIndex);
    KartPad *kartPad = KartCtrl::getKartCtrl()->getKartPad(kartIndex);
    u32 mask = kartPad->mTrigL | kartPad->mTrigR;
    if (status->mDriftEmitters[2]) {
        if (!(mask & pad->getButton()) || __fabsf(KartCtrl::getKartCtrl()->GetCarSpeed(kartIndex)) <= 15.0f) {
            status->mDriftEmitters[2]->mFlags |= 0x200;
            status->mDriftEmitters[2] = nullptr;
        }
    }

    if (status->_24 && !driftEnd)
        createDriftEfct(kartIndex, tire, status, driftCnt);
}

void stEffectMgr::createDriftEfct(int kartIndex, int side, stTireEffectStatus *status, int type)
{
    if (status->mDriftEmitters[0] == nullptr) {
        int typeIdx = type - 1;
        ST_EFCT_ASSERT_NAME("stageIndex", typeIdx, 3, 988);
        u32 param = mDriftParam[typeIdx];
        status->mDriftEmitters[0] =
            createKartCommonEmitter(kartIndex, mpDriftCreateCb[kartIndex][side], param, *(bool *)&status->_24);
    }
}

void stEffectMgr::calcGndCharEffect(int kartIndex, bool inWater)
{
    u8 driverNum = ItemObjMgr::getNowTandemDriverNum(kartIndex);
    LumbarCtrl *lumbar =
        RaceMgr::getManager()->getKartLoader(kartIndex)->getExModelDriver(driverNum)->mLumbarCtrl;
    stTireEffectStatus *status = &mpCharStatus[kartIndex];
    CrsGround *ground = lumbar->mNodeMgr->mCrsGround;
    int attr = ground->getAttribute();
    u32 attrIdx = ground->getAttrIndex();

    if (!(lumbar->m70 & 4)) {
        if (status->mEmitter[0]) {
            status->mEmitter[0]->mFlags |= 0x200;
            status->mEmitter[0] = nullptr;
        }
        return;
    }

    if (status->mEmitter[0] && (status->mAttr != attr || status->mAttrIdx != attrIdx || inWater)) {
        status->mEmitter[0]->mFlags |= 0x200;
        status->mEmitter[0] = nullptr;
    }

    for (u8 i = 0; i < 1; i++) {
        if (status->mEmitter[i] == nullptr) {
            stEmitterCallBack::stFileParamPtrList *ptr = findParamPtrChar(kartIndex, ground, status, i);
            if (ptr && ptr->_00) {
                switch (attr) {
                case CrsGround::Attr_0:
                case CrsGround::Attr_1:
                case CrsGround::Attr_11:
                case CrsGround::Attr_12:
                case CrsGround::Attr_Puller:
                case CrsGround::Attr_19:
                    calcAttrRoadCharCenter(kartIndex, status, i);
                    break;
                case CrsGround::Attr_3:
                    calcAttrWeedCharCenter(kartIndex, status, i);
                    break;
                case CrsGround::Attr_Water:
                    if (!inWater)
                        calcAttrRoadCharCenter(kartIndex, status, i);
                    break;
                }
                stEmitterPtcl *emt = (stEmitterPtcl *)status->mEmitter[i];
                if (emt) {
                    emt->_130->_24 = status->_24;
                    emt->_130->setIpData(ptr);
                    emt->mParticle.init(emt->_130->kartNum);
                }
            }
        }
    }
    status->mAttr = attr;
    status->mAttrIdx = attrIdx;
}

bool stEffectMgr::calcWaterAttrEffect(int kartIndex)
{
    stWaterEffectStatus *status = &mpWaterStatus[kartIndex];
    bool ret = false;
    if (KartCtrl::getKartCtrl()->GetBodyGround(kartIndex)->getAttribute() == CrsGround::Attr_Water) {
        f32 height = KartCtrl::getKartCtrl()->GetWaterHeight(kartIndex);
        int kartID = RaceMgr::getManager()->getKartInfo(kartIndex)->getKartID();
        ST_EFCT_ASSERT_NAME("kartID", kartID, 21, 1087);
        if (height < sWaterBodyHeightOffset[kartID])
            ret = true;
        if (RaceMgr::getManager()->getKartInfo(kartIndex)->isGhostKart())
            return ret;

        if (status->mTrack[0] == nullptr)
            status->mTrack[0] = createKartCommonEmitter(kartIndex, &mpWaterTrack[kartIndex], _3e2, false);
        if (status->mTrack[1] == nullptr)
            status->mTrack[1] = createKartCommonEmitter(kartIndex, &mpWaterSprayCenter[kartIndex], _3e3, false);
        for (u8 i = 0; i < 2; i++) {
            if (status->mSpray[i] == nullptr)
                status->mSpray[i] = createKartCommonEmitter(kartIndex, &mpWaterSpray[i][kartIndex], _3e0, false);
            if (status->mBubble[i] == nullptr)
                status->mBubble[i] = createKartCommonEmitter(kartIndex, &mpWaterBubble[i][kartIndex], _3e1, false);
        }
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x21, kartIndex, JGeometry::TVec3f(0.0f, 0.0f, 0.0f), 0);
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x22, kartIndex, JGeometry::TVec3f(0.0f, 0.0f, 0.0f), 0);
    } else {
        for (u8 i = 0; i < 2; i++) {
            if (status->mTrack[i]) {
                status->mTrack[i]->mFlags |= 0x200;
                status->mTrack[i] = nullptr;
            }
        }
        for (u8 i = 0; i < 2; i++) {
            if (status->mSpray[i]) {
                status->mSpray[i]->mFlags |= 0x200;
                status->mSpray[i] = nullptr;
            }
            if (status->mBubble[i]) {
                status->mBubble[i]->mFlags |= 0x200;
                status->mBubble[i] = nullptr;
            }
        }
    }
    return ret;
}

stEmitterCallBack::stFileParamPtrList *stEffectMgr::findParamPtrTire(int kartIndex, CrsGround *ground,
                                                                    stTireEffectStatus *status, u8 side)
{
    stEmitterCallBack::stFileParamPtrList *result = nullptr;
    int index = -1;
    u8 material = 0;

    ST_EFCT_ASSERT_NAME("tgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1215);
    if (ground->getAttrIndex() > 20)
        return nullptr;

    switch (ground->getAttribute()) {
    case CrsGround::Attr_1: {
        ST_EFCT_ASSERT_NAME("tgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[0].mMaterial[side][attr];
        index = mAttrData[0].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_0: {
        ST_EFCT_ASSERT_NAME("tgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[3].mMaterial[side][attr];
        index = mAttrData[3].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_12: {
        ST_EFCT_ASSERT_NAME("tgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[4].mMaterial[side][attr];
        index = mAttrData[4].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_3: {
        ST_EFCT_ASSERT_NAME("tgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[1].mMaterial[side][attr];
        index = mAttrData[1].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_11: {
        ST_EFCT_ASSERT_NAME("tgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[2].mMaterial[side][attr];
        index = mAttrData[2].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_Water: {
        ST_EFCT_ASSERT_NAME("tgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[5].mMaterial[side][attr];
        index = mAttrData[5].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_4: {
        ST_EFCT_ASSERT_NAME("tgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[6].mMaterial[side][attr];
        index = mAttrData[6].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_19: {
        ST_EFCT_ASSERT_NAME("tgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[7].mMaterial[side][attr];
        index = mAttrData[7].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_Puller: {
        ST_EFCT_ASSERT_NAME("tgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[8].mMaterial[side][attr];
        index = mAttrData[8].mTire[side][attr];
        break;
    }
    }
    if (index >= 0) {
        result = chkAvailablePtrList(kartIndex, index);
        status->mIndex[side] = index + 1;
        status->mMaterial[side] = material;
    }
    return result;
}

static inline int getCenterIndex(stEffectMgr::stGndAttrDataIndex &data, CrsGround *ground, u8 side)
{
    int index = -1;
    ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1176);
    switch (side) {
    case 0:
        index = data.mCenter[0][ground->getAttrIndex()];
        break;
    case 1:
        index = data.mCenter[1][ground->getAttrIndex()];
        break;
    }
    return index;
}

stEmitterCallBack::stFileParamPtrList *stEffectMgr::findParamPtrCenter(int kartIndex, CrsGround *ground,
                                                                      stTireEffectStatus *status, u8 side)
{
    stEmitterCallBack::stFileParamPtrList *result = nullptr;

    ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1270);
    int index = -1;
    if (ground->getAttrIndex() < 20) {
        switch (ground->getAttribute()) {
    case CrsGround::Attr_1:
        index = getCenterIndex(mAttrData[0], ground, side);
        break;
    case CrsGround::Attr_0:
        index = getCenterIndex(mAttrData[3], ground, side);
        break;
    case CrsGround::Attr_12:
        index = getCenterIndex(mAttrData[4], ground, side);
        break;
    case CrsGround::Attr_11:
        index = getCenterIndex(mAttrData[2], ground, side);
        break;
    case CrsGround::Attr_Water:
        index = getCenterIndex(mAttrData[5], ground, side);
        break;
    case CrsGround::Attr_4:
        index = getCenterIndex(mAttrData[6], ground, side);
        break;
    case CrsGround::Attr_19:
        index = getCenterIndex(mAttrData[7], ground, side);
        break;
    case CrsGround::Attr_Puller:
        index = getCenterIndex(mAttrData[8], ground, side);
        break;
        }
        if (index >= 0) {
            result = chkAvailablePtrList(kartIndex, index);
            status->mIndex[side] = index + 1;
        }
    }
    return result;
}

stEmitterCallBack::stFileParamPtrList *stEffectMgr::findParamPtrChar(int kartIndex, CrsGround *ground,
                                                                    stTireEffectStatus *status, u8 side)
{
    stEmitterCallBack::stFileParamPtrList *result = nullptr;

    ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1319);
    int index = -1;
    if (ground->getAttrIndex() < 20) {
        switch (ground->getAttribute()) {
    case CrsGround::Attr_1: {
        ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[0].mChar[attr];
        break;
    }
    case CrsGround::Attr_3: {
        ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[1].mChar[attr];
        break;
    }
    case CrsGround::Attr_0: {
        ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[3].mChar[attr];
        break;
    }
    case CrsGround::Attr_12: {
        ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[4].mChar[attr];
        break;
    }
    case CrsGround::Attr_11: {
        ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[2].mChar[attr];
        break;
    }
    case CrsGround::Attr_Water: {
        ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[5].mChar[attr];
        break;
    }
    case CrsGround::Attr_4: {
        ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[6].mChar[attr];
        break;
    }
    case CrsGround::Attr_19: {
        ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[7].mChar[attr];
        break;
    }
    case CrsGround::Attr_Puller: {
        ST_EFCT_ASSERT_NAME("bgnd->getAttrIndex()", ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[8].mChar[attr];
        break;
    }
        }
        if (index >= 0) {
            result = chkAvailablePtrList(kartIndex, index);
            status->mIndex[side] = index + 1;
        }
    }
    return result;
}

stEmitterCallBack::stFileParamPtrList *stEffectMgr::findParamPtrCommon(int index)
{
    ST_EFCT_ASSERT_NAME("efct_index", index, _138c, 1372);
    return &mpPtrList[index];
}

stEmitterCallBack::stFileParamPtrList *stEffectMgr::chkAvailablePtrList(int kartIndex, int index)
{
    stEmitterCallBack::stFileParamPtrList *result = nullptr;
    ST_EFCT_ASSERT_CHK(kartIndex, 8, 1387, -1);
    bool available = false;

    if (index >= 0) {
        if (kartIndex != -1) {
            if (ObjUtility::isPlayerDriver(kartIndex) ||
                (mpClipParam[index]._00 == 0 && (mpClipParam[index]._01 == 0 || tstIsAvailableDistance(kartIndex))))
                available = true;
        } else {
            available = true;
        }
    }

    if (available)
        result = findParamPtrCommon(index);
    return result;
}

stEmitterCallBack *stEffectMgr::getCallBackPtr(u8 material, int kartIndex, int tire_num)
{
    stEmitterCallBack *ret = nullptr;
    switch (material) {
    case 0:
        ret = mpRoadCb[kartIndex][tire_num];
        break;
    case 1:
        ret = mpWeedCb[kartIndex][tire_num];
        break;
    case 2:
        ST_EFCT_ASSERT_NAME("tire_num", tire_num, 2, 1425);
        ret = &mpCrsWaterSpray[tire_num][kartIndex];
        break;
    case 3:
        ST_EFCT_ASSERT_NAME("tire_num", tire_num, 2, 1428);
        ret = &mpCrsWaterBubble[tire_num][kartIndex];
        break;
    }
    if (ret == nullptr) {
        JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "stEffectMgr.cpp", 1432, "%s", "ret != 0");
        OSPanic("stEffectMgr.cpp", 1432, "Halt");
    }
    return ret;
}

void stEffectMgr::calcAttrRoad(int kartIndex, int tire, stTireEffectStatus *status, u8 side)
{
    ST_EFCT_ASSERT_NAME("kartnum", kartIndex, 8, 1445);
    ST_EFCT_ASSERT_NAME("tirenum", tire, 2, 1446);
    ST_EFCT_ASSERT_MAX("emNum", side, 2, 1447);
    stEmitterCallBack *cb = getCallBackPtr(status->mMaterial[side], kartIndex, tire);
    status->mEmitter[side] = createEmitter(cb, status->mIndex[side]);
}

void stEffectMgr::calcAttrRoadCenter(int kartIndex, stTireEffectStatus *status, u8 side)
{
    ST_EFCT_ASSERT_NAME("kartnum", kartIndex, 8, 1465);
    ST_EFCT_ASSERT_MAX("emNum", side, 2, 1466);
    switch (side) {
    case 0:
        status->mEmitter[side] = createEmitter(mpRoadCenterCb[0][kartIndex], (u16)status->mIndex[side]);
        break;
    case 1:
        status->mEmitter[side] = createEmitter(mpRoadCenterCb[1][kartIndex], (u16)status->mIndex[side]);
        break;
    }
}

void stEffectMgr::calcAttrRoadCharCenter(int kartIndex, stTireEffectStatus *status, u8 side)
{
    ST_EFCT_ASSERT_NAME("kartnum", kartIndex, 8, 1483);
    ST_EFCT_ASSERT_MAX("emNum", side, 1, 1484);
    switch (side) {
    case 0:
        status->mEmitter[side] = createEmitter(mpRoadCharCenterCb[kartIndex], (u16)status->mIndex[side]);
        break;
    }
}

void stEffectMgr::calcAttrWeedCharCenter(int kartIndex, stTireEffectStatus *status, u8 side)
{
    ST_EFCT_ASSERT_NAME("kartnum", kartIndex, 8, 1500);
    ST_EFCT_ASSERT_MAX("emNum", side, 1, 1501);
    switch (side) {
    case 0:
        status->mEmitter[side] = createEmitter(mpWeedCharCenterCb[kartIndex], (u16)status->mIndex[side]);
        break;
    }
}

int stEffectMgr::setDriftStatus(int kartIndex, int type, stTireEffectStatus *status)
{
    ST_EFCT_ASSERT_NAME("kartnum", kartIndex, 8, 1517);
    u64 carStatus = KartCtrl::getKartCtrl()->getKartBody(kartIndex)->mCarStatus;
    status->_25 = status->_24;
    status->_24 = 0;
    switch (type) {
    case 2:
        if ((carStatus & 3) != 0)
            status->_24 = 1;
        break;
    case 3:
        if ((carStatus & 3) != 0)
            status->_24 = 1;
        break;
    }
    return 0;
}

void stEffectMgr::draw(u32 viewNo)
{
    mEmtMgr0.draw(viewNo);
}

void stEffectMgr::update() {}

void stEffectMgr::viewCalc(u32) {}

void stEffectMgr::setCurrentViewNo(u32) {}

JUTTexture *stEffectMgr::getTexture(cKindTexture id) const
{
    ST_EFCT_ASSERT_NAME("id", id, cKindTexture_Max, 1640);
    return mpTexture[id];
}

stEfctWaterTrack *stEffectMgr::getWaterTrack(int index)
{
    ST_EFCT_ASSERT(index, 1654);
    return &mpWaterTrack[index];
}

Mtx *stEffectMgr::getKartExhaustMtxL(int index)
{
    ST_EFCT_ASSERT(index, 1664);
    return &mpKartExhaustMtxL[index];
}

Mtx *stEffectMgr::getKartExhaustMtxR(int index)
{
    ST_EFCT_ASSERT(index, 1674);
    return &mpKartExhaustMtxR[index];
}

f32 stEffectMgr::getEfctCarSpeed(int index) const
{
    ST_EFCT_ASSERT_N(index, RaceMgr::getManager()->getKartNumber(), 1684);
    return mpEfctCarSpeed[index];
}

stExhaustAccelCounter *stEffectMgr::getExhaustAccelCounter(int index)
{
    ST_EFCT_ASSERT_N(index, RaceMgr::getManager()->getKartNumber(), 1715);
    return &mpExhaustAccelCounter[index];
}

void stEffectMgr::getExhaustPosDir(JGeometry::TVec3f *pos, JGeometry::TVec3f *dir, int index, u8 side)
{
    Mtx *mtx;
    if (side == 0)
        mtx = getKartExhaustMtxL(index);
    else
        mtx = getKartExhaustMtxR(index);
    pos->x = (*mtx)[0][3];
    pos->y = (*mtx)[1][3];
    pos->z = (*mtx)[2][3];
    dir->x = (*mtx)[0][2];
    dir->y = (*mtx)[1][2];
    dir->z = (*mtx)[2][2];
    stVecNormalize(dir);
}

#pragma inline_depth(0)
stEmitter *stEffectMgr::createEmitter(stEmitterCallBack *cb, u16 type)
{
    stEmitterPtcl *emt = mEmtMgr0.Create(cb, type);
    if (emt) {
        if (type < 1) {
            ST_EFCT_ASSERT_NAME("kind", (int)type, 1, 1705);
            ((stEmitter &)*emt) = *mpEmitterTemplate[(int)type];
            emt->mParticle.init(*(s16 *)((u8 *)emt->_130 + 0x20));
        }
    }
    return emt;
}
#pragma inline_depth(4)

stEmitter *stEffectMgr::createKartCommonEmitter(int kartIndex, stEmitterCallBack *cb, u16 index, bool b)
{
    stEmitterPtcl *emt = nullptr;
    stEmitterCallBack::stFileParamPtrList *ptr = chkAvailablePtrList(kartIndex, index);
    if (ptr && ptr->_00) {
        emt = (stEmitterPtcl *)createEmitter(cb, (u16)(index + 1));
        if (emt) {
            emt->_130->setIpData(ptr);
            emt->_130->_24 = b;
            emt->mParticle.init(emt->_130->kartNum);
        }
    }
    return emt;
}

void stEffectMgr::createKinokoDashEmt(int kartIndex)
{
    ST_EFCT_ASSERT(kartIndex, 1828);
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;
    if (tstUseMufllerL(kartIndex)) {
        GetStEfctMgr()->getExhaustPosDir(&pos, &dir, kartIndex, 0);
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x1f, kartIndex, pos, 0);
    }
    if (tstUseMufllerR(kartIndex)) {
        GetStEfctMgr()->getExhaustPosDir(&pos, &dir, kartIndex, 1);
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x1f, kartIndex, pos, 1);
    }
}

u8 stEffectMgr::tstUseMufllerL(int index)
{
    ST_EFCT_ASSERT(index, 1881);
    return mpMufllerL[index];
}

u8 stEffectMgr::tstUseMufllerR(int index)
{
    ST_EFCT_ASSERT(index, 1894);
    return mpMufllerR[index];
}

void stEffectMgr::setExhaustStopFlg(int index, bool flg)
{
    ST_EFCT_ASSERT(index, 1881);
    if (mpMufllerL[index] != 0) {
        mpExhaustL[index]->mIsStop = flg;
    }
    ST_EFCT_ASSERT(index, 1894);
    if (mpMufllerR[index] != 0) {
        mpExhaustR[index]->mIsStop = flg;
    }
}

u8 stEffectMgr::tstIsAvailableDistance(int index) const
{
    ST_EFCT_ASSERT(index, 1923);
    return mIsAvailableDistance[index];
}

void stEffectMgr::resetIndex()
{
    mAttrData[0].init();
    mAttrData[1].init();
    mAttrData[2].init();
    mAttrData[3].init();
    mAttrData[4].init();
    mAttrData[5].init();
    mAttrData[6].init();
    mAttrData[7].init();
    mAttrData[8].init();
}

void stEffectMgr::loadCourseParam(u32 crsID)
{
    resetIndex();
    _138c = 0;
    u8 *data = (u8 *)ResMgr::getPtr(ResMgr::mcArcMRAM, "/Effect/EffectData.kel");
    u8 *courseID = data;
    u8 *commonNum = data + 1;
    u8 *commonData = commonNum + 1;
    if (*courseID != 0x00) {
        JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "stEffectMgr.cpp", 2003, "%s", "*courseID == 0x00");
        OSPanic("stEffectMgr.cpp", 2003, "Halt");
    }

    u8 *crsNum = commonNum;
    u8 *crsData = nullptr;
    while (crsID != *courseID) {
        if (*courseID == 0xff) {
            crsData = nullptr;
            break;
        }
        courseID += *crsNum * 0xae + 2;
        crsNum = courseID + 1;
        crsData = crsNum + 1;
    }

    _138c = (u8)(*commonNum + *crsNum);
    mpTexList = new u32[_138c];
    mpPtrList = new stEmitterCallBack::stFileParamPtrList[_138c];
    stEmitterCallBack::stFileParamPtrList *ptrList;
    stGndAttrParam *param = (stGndAttrParam *)commonData;
    int index = 0;
    ptrList = mpPtrList;
    u8 num = *commonNum;
    mpClipParam = new stClipParam[_138c];
    stClipParam *clip = mpClipParam;
    for (u8 i = 0; i < 2; i++) {
        switch (i) {
        case 1:
            param = (stGndAttrParam *)crsData;
            num = *crsNum;
            break;
        }
        if (param == nullptr)
            return;
        for (u8 j = 0; j < num; j++, index++) {
            switch (RaceMgr::getManager()->getStatusNumber()) {
            case 1:
                ptrList->_00 = (stEmitter *)getEmitterPtr(param->mEmitterName[0]);
                ptrList->_04 = (stEmitter *)getEmitterPtr(param->mEmitterName[1]);
                break;
            case 2:
                ptrList->_00 = (stEmitter *)getEmitterPtr(param->mEmitterName[2]);
                ptrList->_04 = (stEmitter *)getEmitterPtr(param->mEmitterName[3]);
                break;
            case 3:
            case 4:
                ptrList->_00 = (stEmitter *)getEmitterPtr(param->mEmitterName[4]);
                ptrList->_04 = (stEmitter *)getEmitterPtr(param->mEmitterName[5]);
                break;
            default:
                ptrList->_04 = nullptr;
                ptrList->_00 = nullptr;
                break;
            }
            ptrList->_08 = (stEmitter *)new JUTTexture((ResTIMG *)getBtiPtr(param->mBtiName));
            switch (param->mAttr) {
            case CrsGround::Attr_1:
                setIndexToList_StdGnd(index, *param, mAttrData[0]);
                break;
            case CrsGround::Attr_11:
                setIndexToList_StdGnd(index, *param, mAttrData[2]);
                break;
            case CrsGround::Attr_3:
                setIndexToList_StdGnd(index, *param, mAttrData[1]);
                break;
            case CrsGround::Attr_0:
                setIndexToList_StdGnd(index, *param, mAttrData[3]);
                break;
            case CrsGround::Attr_12:
                setIndexToList_StdGnd(index, *param, mAttrData[4]);
                break;
            case CrsGround::Attr_Water:
                setIndexToList_StdGnd(index, *param, mAttrData[5]);
                break;
            case CrsGround::Attr_4:
                setIndexToList_StdGnd(index, *param, mAttrData[6]);
                break;
            case CrsGround::Attr_19:
                setIndexToList_StdGnd(index, *param, mAttrData[7]);
                break;
            case CrsGround::Attr_Puller:
                setIndexToList_StdGnd(index, *param, mAttrData[8]);
                break;
            default:
                setIndexToList_Common(index, *param);
                break;
            }
            clip->_00 = param->mClip0;
            clip->_01 = param->mClip1;
            param++;
            ptrList++;
            clip++;
        }
    }
    delete data;
}

__declspec(weak) stEffectMgr::stClipParam::stClipParam()
{
    _00 = 0;
    _01 = 0;
}

void stEffectMgr::setIndexToList_Common(u8 index, const stGndAttrParam &param)
{
    switch (param.mKind) {
    case 0: _3b8 = index; break;
    case 3: mDriftParam[0] = index; break;
    case 4: mDriftParam[1] = index; break;
    case 5: mDriftParam[2] = index; break;
    case 6: _3c8 = index; break;
    case 9: _3cc = index; break;
    case 11: _3d4 = index; break;
    case 12: _3d5 = index; break;
    case 13: _3d6 = index; break;
    case 14: _3d0 = index; break;
    case 15: _3dc = index; break;
    case 16: _3e0 = index; break;
    case 17: _3e1 = index; break;
    case 18: _3e2 = index; break;
    case 19: _3e3 = index; break;
    case 24: _3d8 = index; break;
    }
}

void stEffectMgr::setIndexToList_StdGnd(u8 index, const stGndAttrParam &param, stGndAttrDataIndex &data)
{
    u8 idx = param.mAttrIdx;
    switch (param.mKind) {
    case 1:
        data.mTire[0][idx] = index;
        data.mMaterial[0][idx] = param.mMaterial;
        break;
    case 2:
        data.mTire[1][idx] = index;
        data.mMaterial[1][idx] = param.mMaterial;
        break;
    case 10:
        data.mCenter[0][idx] = index;
        break;
    case 21:
        data.mCenter[1][idx] = index;
        break;
    case 20:
        data.mChar[idx] = index;
        break;
    }
}

void *stEffectMgr::getEmitterPtr(char *name)
{
    snprintf(sPathName, 64, "/Effect/%s", name);
    return ResMgr::getPtr(ResMgr::mcArcMRAM, sPathName);
}

void *stEffectMgr::getBtiPtr(char *name)
{
    snprintf(sPathName, 64, "/Effect/%s", name);
    return ResMgr::getPtr(ResMgr::mcArcMRAM, sPathName);
}

char *stEffectMgr::getBtiName(cKindTexture kind)
{
    char *name = NULL;
    switch (kind) {
    case cKindTexture_Tool01:
        name = (char *)"/Effect/tool01.bti";
        break;
    case cKindTexture_Tool02:
        name = (char *)"/Effect/tool02.bti";
        break;
    case cKindTexture_Tool03:
        name = (char *)"/Effect/tool03.bti";
        break;
    }
    return name;
}

char *stEffectMgr::getSnpName(u32 id)
{
    char *name = NULL;
    if ((s32)id == 0) {
        name = (char *)"/Effect/tool01.snp";
    }
    return name;
}

#include "JSystem/JAudio/JASFakeMatch2.h"
