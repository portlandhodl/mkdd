#include "Sato/stEffectMgr.h"
#include "Sato/stEmitter.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/ResMgr.h"
#include "JSystem/JUtility/JUTAssert.h"

#include <stdio.h>

namespace {
char sPathName[64];
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

void stEffectMgr::createModel(JKRSolidHeap *, u32)
{
    model_init();
}

void stEffectMgr::initCallBackPtr(stEmitterCallBack *cb, s16 a, s16 b)
{
    cb->kartNum = a;
    cb->_22 = b;
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
