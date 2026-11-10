#include "Sato/stEffectMgr.h"
#include "Sato/stEmitter.h"
#include "Sato/ObjUtility.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Yamamoto/kartCtrl.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/stMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/ResMgr.h"
#include "JSystem/JUtility/JUTAssert.h"

#include <stdio.h>

void __dt__10JSUPtrLinkFv(JSUPtrLink *, short);
void __dt__10JSUPtrListFv(JSUPtrList *, short);

namespace {
char sPathName[64];
f32 sClipMaxLength = 5000.0f;
}

#define ST_EFCT_ASSERT(i, line)                                                                                   \
    ((0 <= (i) && (i) < 8) ? (void)0                                                                               \
                           : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "stEffectMgr.cpp", (line),    \
                                                         "range over: %d <= kart_index=%d < %d", 0, (i), 8),       \
                              OSPanic("stEffectMgr.cpp", (line), "Halt")))

#define ST_EFCT_ASSERT_N(i, n, line)                                                                              \
    ((0 <= (i) && (i) < (n)) ? (void)0                                                                             \
                            : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "stEffectMgr.cpp", (line),   \
                                                          "range over: %d <= kart_index=%d < %d", 0, (i), (n)),    \
                               OSPanic("stEffectMgr.cpp", (line), "Halt")))

__declspec(weak) stEffectMgr::stWaterEffectStatus::stWaterEffectStatus()
{
    u32 *p = (u32 *)this;
    u8 i = 0;
    u32 a = (u32)i;
    u32 b = (u32)i;
    for (; i < 2; i++) {
        p[i + 4] = b;
        p[i + 2] = a;
        p[i] = a;
    }
}

__declspec(weak) stEffectMgr::stTireEffectStatus::stTireEffectStatus()
{
    _1c = 0xff;
    _25 = 0;
    _24 = 0;
    _20 = 0;
    stEmitter *nullp = nullptr;
    u32 minus = 0xffff;
    u8 zero = 0;
    for (u8 i = 0; i < 3; i++)
        ((u32 *)this)[i + 4] = 0;
    for (u8 i = 0; i < 2; i++) {
        mEmitter[i] = nullp;
        mIndex[i] = minus;
        mMaterial[i] = zero;
    }
    _08 = 0;
    _0c = 0;
}

__declspec(weak) stEffectMgr::stTireEfctStatusSet::stTireEfctStatusSet() {}

__declspec(weak) stParticle::~stParticle() {}

__declspec(weak) stParticle::stParticle() : mLink(this) {}

__declspec(weak) stEmitterPtcl::~stEmitterPtcl() {}

__declspec(weak) stEmitterPtcl::stEmitterPtcl() : _134(this)
{
    _130 = 0;
    _146 = 0;
}

void stEffectMgr::createModel(JKRSolidHeap *, u32)
{
    model_init();
}

void stEffectMgr::initCallBackPtr(stEmitterCallBack *cb, s16 a, s16 b)
{
    cb->kartNum = a;
    cb->_22 = b;
}

void stEffectMgr::calc()
{
    ObjUtility::chkAvailableEnemyByDistande((bool *)mIsAvailableDistance, sClipMaxLength);

    for (int i = 0; i < RaceMgr::getManager()->getKartNumber(); i++)
        mpExhaustAccelCounter[i].calc(i);

    Mtx *pa = mpKartBodyMtxA;
    Mtx *pb = mpKartBodyMtxB;
    Mtx *pc = mpKartExhaustMtxL;
    Mtx *pd = mpKartExhaustMtxR;
    for (int i = 0; i < RaceMgr::getManager()->getKartNumber(); i++) {
        Mtx &bodyMtx = ObjUtility::getKartBodyMtx(i);
        PSMTXConcat(bodyMtx, *pa, *pc);
        PSMTXConcat(bodyMtx, *pb, *pd);
        pa++;
        pb++;
        pc++;
        pd++;
    }

    calcGndAttrEffect();
    ((stEmitterPtclMgr *)((u8 *)this + 0xa0))->calc();
    ((stEmitterPtclMgr *)this)->calc();
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
        if (status->mDriftEmitter == nullptr) {
            status->mDriftEmitter =
                createKartCommonEmitter(kartIndex, mpDriftCb[kartIndex][side], (u16)_3cc, *(bool *)&status->_24);
        }
    }
}

void stEffectMgr::createDriftEfct(int kartIndex, int side, stTireEffectStatus *status, int type)
{
    if (status->mDriftCreateEmitter == nullptr) {
        int typeIdx = type - 1;
        ST_EFCT_ASSERT_N(typeIdx, 3, 988);
        stEmitterCallBack *cb = mpDriftCreateCb[kartIndex][side];
        u32 param = mDriftParam[typeIdx];
        status->mDriftCreateEmitter = createKartCommonEmitter(kartIndex, cb, (u16)param, *(bool *)&status->_24);
    }
}

stEmitterCallBack::stFileParamPtrList *stEffectMgr::findParamPtrTire(int kartIndex, CrsGround *ground,
                                                                    stTireEffectStatus *status, u8 side)
{
    stEmitterCallBack::stFileParamPtrList *result = nullptr;
    int index = -1;
    u8 material = 0;

    ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1215);
    if (ground->getAttrIndex() > 20)
        return nullptr;

    switch (ground->getAttribute()) {
    case CrsGround::Attr_1: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[0].mMaterial[side][attr];
        index = mAttrData[0].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_0: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[3].mMaterial[side][attr];
        index = mAttrData[3].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_12: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[4].mMaterial[side][attr];
        index = mAttrData[4].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_3: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[1].mMaterial[side][attr];
        index = mAttrData[1].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_11: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[2].mMaterial[side][attr];
        index = mAttrData[2].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_Water: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[5].mMaterial[side][attr];
        index = mAttrData[5].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_4: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[6].mMaterial[side][attr];
        index = mAttrData[6].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_19: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1159);
        u32 attr = ground->getAttrIndex();
        material = mAttrData[7].mMaterial[side][attr];
        index = mAttrData[7].mTire[side][attr];
        break;
    }
    case CrsGround::Attr_Puller: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1159);
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

stEmitterCallBack::stFileParamPtrList *stEffectMgr::findParamPtrCenter(int kartIndex, CrsGround *ground,
                                                                      stTireEffectStatus *status, u8 side)
{
    int index = -1;
    stEmitterCallBack::stFileParamPtrList *result = nullptr;

    ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1270);
    if (ground->getAttrIndex() < 20) {
        switch (ground->getAttribute()) {
    case CrsGround::Attr_1: {
        index = -1;
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1176);
        switch (side) {
        case 0: index = mAttrData[0].mCenter[0][ground->getAttrIndex()]; break;
        case 1: index = mAttrData[0].mCenter[1][ground->getAttrIndex()]; break;
        }
        break;
    }
    case CrsGround::Attr_0: {
        index = -1;
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1176);
        switch (side) {
        case 0: index = mAttrData[3].mCenter[0][ground->getAttrIndex()]; break;
        case 1: index = mAttrData[3].mCenter[1][ground->getAttrIndex()]; break;
        }
        break;
    }
    case CrsGround::Attr_12: {
        index = -1;
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1176);
        switch (side) {
        case 0: index = mAttrData[4].mCenter[0][ground->getAttrIndex()]; break;
        case 1: index = mAttrData[4].mCenter[1][ground->getAttrIndex()]; break;
        }
        break;
    }
    case CrsGround::Attr_11: {
        index = -1;
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1176);
        switch (side) {
        case 0: index = mAttrData[2].mCenter[0][ground->getAttrIndex()]; break;
        case 1: index = mAttrData[2].mCenter[1][ground->getAttrIndex()]; break;
        }
        break;
    }
    case CrsGround::Attr_Water: {
        index = -1;
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1176);
        switch (side) {
        case 0: index = mAttrData[5].mCenter[0][ground->getAttrIndex()]; break;
        case 1: index = mAttrData[5].mCenter[1][ground->getAttrIndex()]; break;
        }
        break;
    }
    case CrsGround::Attr_4: {
        index = -1;
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1176);
        switch (side) {
        case 0: index = mAttrData[6].mCenter[0][ground->getAttrIndex()]; break;
        case 1: index = mAttrData[6].mCenter[1][ground->getAttrIndex()]; break;
        }
        break;
    }
    case CrsGround::Attr_19: {
        index = -1;
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1176);
        switch (side) {
        case 0: index = mAttrData[7].mCenter[0][ground->getAttrIndex()]; break;
        case 1: index = mAttrData[7].mCenter[1][ground->getAttrIndex()]; break;
        }
        break;
    }
    case CrsGround::Attr_Puller: {
        index = -1;
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1176);
        switch (side) {
        case 0: index = mAttrData[8].mCenter[0][ground->getAttrIndex()]; break;
        case 1: index = mAttrData[8].mCenter[1][ground->getAttrIndex()]; break;
        }
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

stEmitterCallBack::stFileParamPtrList *stEffectMgr::findParamPtrChar(int kartIndex, CrsGround *ground,
                                                                    stTireEffectStatus *status, u8 side)
{
    stEmitterCallBack::stFileParamPtrList *result = nullptr;

    ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1319);
    int index = -1;
    if (ground->getAttrIndex() < 20) {
        switch (ground->getAttribute()) {
    case CrsGround::Attr_1: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[0].mChar[attr];
        break;
    }
    case CrsGround::Attr_3: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[1].mChar[attr];
        break;
    }
    case CrsGround::Attr_0: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[3].mChar[attr];
        break;
    }
    case CrsGround::Attr_12: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[4].mChar[attr];
        break;
    }
    case CrsGround::Attr_11: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[2].mChar[attr];
        break;
    }
    case CrsGround::Attr_Water: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[5].mChar[attr];
        break;
    }
    case CrsGround::Attr_4: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[6].mChar[attr];
        break;
    }
    case CrsGround::Attr_19: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1196);
        u32 attr = ground->getAttrIndex();
        index = mAttrData[7].mChar[attr];
        break;
    }
    case CrsGround::Attr_Puller: {
        ST_EFCT_ASSERT_N(ground->getAttrIndex(), 20, 1196);
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
    ST_EFCT_ASSERT_N(index, _138c, 1372);
    return &mpPtrList[index];
}

#define ST_EFCT_ASSERT_KART(i, line)                                                                              \
    ((0 <= (i) && (i) < 8) ? (void)0                                                                              \
                           : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "stEffectMgr.cpp", (line),   \
                                                         "range over: %d <= kartnum=%d < %d", 0, (i), 8),         \
                              OSPanic("stEffectMgr.cpp", (line), "Halt")))

#define ST_EFCT_ASSERT_NUM(i, n, line)                                                                            \
    ((0 <= (i) && (i) < (n)) ? (void)0                                                                            \
                             : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "stEffectMgr.cpp", (line),  \
                                                           "range over: %d <= tirenum=%d < %d", 0, (i), (n)),     \
                               OSPanic("stEffectMgr.cpp", (line), "Halt")))

#define ST_EFCT_ASSERT_CHK(i, n, line, min)                                                                       \
    (((min) <= (i) && (i) < (n))                                                                                  \
         ? (void)0                                                                                                \
         : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "stEffectMgr.cpp", (line),                   \
                                       "range over: %d <= kart_index=%d < %d", (min), (i), (n)),                  \
            OSPanic("stEffectMgr.cpp", (line), "Halt")))

stEmitterCallBack::stFileParamPtrList *stEffectMgr::chkAvailablePtrList(int kartIndex, int index)
{
    stEmitterCallBack::stFileParamPtrList *result = nullptr;
    ST_EFCT_ASSERT_CHK(kartIndex, 8, 1387, -1);
    bool available = false;

    if (index >= 0) {
        if (kartIndex == -1) {
            available = true;
        } else {
            if (ObjUtility::isPlayerDriver(kartIndex)) {
                available = true;
            } else {
                if (mpClipParam[index]._00 == 0) {
                    if (mpClipParam[index]._01 == 0) {
                        available = true;
                    } else {
                        if (tstIsAvailableDistance(kartIndex))
                            available = true;
                    }
                }
            }
        }
    }

    if (!available)
        return result;

    ST_EFCT_ASSERT_CHK(index, _138c, 1372, 0);
    result = &mpPtrList[index];
    return result;
}

void stEffectMgr::calcAttrRoadCenter(int kartIndex, stTireEffectStatus *status, u8 side)
{
    ST_EFCT_ASSERT_KART(kartIndex, 1465);
    ST_EFCT_ASSERT_NUM(side, 2, 1466);
    switch (side) {
    case 0:
        status->mEmitter[side] = createEmitter(mpRoadCenterCb[0][kartIndex], status->mIndex[side]);
        break;
    case 1:
        status->mEmitter[side] = createEmitter(mpRoadCenterCb[1][kartIndex], status->mIndex[side]);
        break;
    }
}

void stEffectMgr::calcAttrRoadCharCenter(int kartIndex, stTireEffectStatus *status, u8 side)
{
    ST_EFCT_ASSERT_KART(kartIndex, 1483);
    ST_EFCT_ASSERT_NUM(side, 1, 1484);
    if (side == 0)
        status->mEmitter[0] = createEmitter(mpRoadCharCenterCb[kartIndex], status->mIndex[0]);
}

void stEffectMgr::calcAttrWeedCharCenter(int kartIndex, stTireEffectStatus *status, u8 side)
{
    ST_EFCT_ASSERT_KART(kartIndex, 1483);
    ST_EFCT_ASSERT_NUM(side, 1, 1484);
    if (side == 0)
        status->mEmitter[0] = createEmitter(mpWeedCharCenterCb[kartIndex], status->mIndex[0]);
}

int stEffectMgr::setDriftStatus(int kartIndex, int type, stTireEffectStatus *status)
{
    ST_EFCT_ASSERT(kartIndex, 1517);
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
    ((stEmitterPtclMgr *)this)->draw(viewNo);
}

void stEffectMgr::update() {}

void stEffectMgr::viewCalc(u32) {}

void stEffectMgr::setCurrentViewNo(u32) {}

stEffectMgr::stWaterEffectStatus *stEffectMgr::getWaterTrack(int index)
{
    ST_EFCT_ASSERT(index, 1654);
    return &mpWaterEffectStatus[index];
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

stEffectMgr::stExhaustAccelCounter *stEffectMgr::getExhaustAccelCounter(int index)
{
    ST_EFCT_ASSERT_N(index, RaceMgr::getManager()->getKartNumber(), 1715);
    return &mpExhaustAccelCounter[index];
}

void stEffectMgr::getExhaustPosDir(JGeometry::TVec3f *pos, JGeometry::TVec3f *dir, int index, u8 side)
{
    Mtx *mtx;
    if (side == 0) {
        ST_EFCT_ASSERT(index, 1664);
        mtx = &mpKartExhaustMtxL[index];
    } else {
        ST_EFCT_ASSERT(index, 1674);
        mtx = &mpKartExhaustMtxR[index];
    }
    pos->set((*mtx)[0][3], (*mtx)[1][3], (*mtx)[2][3]);
    dir->set((*mtx)[0][2], (*mtx)[1][2], (*mtx)[2][2]);
    stVecNormalize(dir);
}

#pragma inline_depth(0)
stEmitter *stEffectMgr::createEmitter(stEmitterCallBack *cb, u16 type)
{
    stEmitterPtcl *emt = ((stEmitterPtclMgr *)this)->Create(cb, type);
    if (emt) {
        if (type < 1) {
            ST_EFCT_ASSERT_N((int)type, 1, 1705);
            ((stEmitter &)*emt) = *mpEmitterTemplate[(int)type];
            emt->mParticle.init(*(s16 *)((u8 *)emt->_130 + 0x20));
        }
    }
    return emt;
}

stEmitter *stEffectMgr::createKartCommonEmitter(int kartIndex, stEmitterCallBack *cb, u16 index, bool b)
{
    ST_EFCT_ASSERT_CHK(kartIndex, 8, 1387, -1);
    u16 idx = index;
    bool available = false;

    if (idx >= 0) {
        if (kartIndex == -1)
            available = true;
        else if (ObjUtility::isPlayerDriver(kartIndex))
            available = true;
        else if (mpClipParam[idx]._00 == 0) {
            if (mpClipParam[idx]._01 == 0)
                available = true;
            else if (tstIsAvailableDistance(kartIndex))
                available = true;
        }
    }

    stEmitterCallBack::stFileParamPtrList *ptr = nullptr;
    if (available) {
        ST_EFCT_ASSERT_CHK(idx, _138c, 1372, 0);
        ptr = &mpPtrList[idx];
    }

    stEmitterPtcl *emt = nullptr;
    if (ptr && ptr->_00) {
        emt = (stEmitterPtcl *)createEmitter(cb, idx + 1);
        if (emt) {
            emt->_130->setIpData(ptr);
            emt->_130->_24 = b;
            emt->mParticle.init(*(s16 *)((u8 *)emt->_130 + 0x20));
        }
    }
    return emt;
}

#pragma inline_depth(4)

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
        mpExhaustStopL[index][0x31] = flg;
    }
    ST_EFCT_ASSERT(index, 1894);
    if (mpMufllerR[index] != 0) {
        mpExhaustStopR[index][0x31] = flg;
    }
}

u8 stEffectMgr::tstIsAvailableDistance(int index) const
{
    ST_EFCT_ASSERT(index, 1923);
    return mIsAvailableDistance[index];
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
    u8 idx = param._00[1];
    switch (param.mKind) {
    case 1:
        data._000[idx] = index;
        data._190[idx] = param._ab;
        break;
    case 2:
        data._050[idx] = index;
        data._1a4[idx] = param._ab;
        break;
    case 10:
        data._0a0[idx] = index;
        break;
    case 21:
        data._0f0[idx] = index;
        break;
    case 20:
        data._140[idx] = index;
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

char *stEffectMgr::getSnpName(u32 id)
{
    char *name = NULL;
    if ((s32)id == 0) {
        name = (char *)"/Effect/tool01.snp";
    }
    return name;
}
