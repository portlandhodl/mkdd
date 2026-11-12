#include "Sato/EffectScreen.h"

#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/RaceInfo.h"
#include "Yamamoto/kartCtrl.h"
#include "Yamamoto/kartCamera.h"

namespace {

typedef int (EffectScreen::*ESFunc)();

static u8 sThunderWhiteFrame = 3;
static u8 sThunderWaitFrame = 50;
static u8 sThunderDownFrame = 15;
static u8 sThunderWhiteAlpha = 0xd2;
static u8 sThunderAlpha = 0x96;
static u8 sBombExWhiteFrame = 3;
static u8 sBombExWhiteAlpha = 0xd2;
static u8 sFlashWhiteFrame = 7;
static u8 sDemoScreenAlpha = 0xbe;
static u8 sDemoScreenAddFrame = 10;

static f32 sBombExplosionLength = 3000.0f;
static BombParam sRaceBombParam[2] = {
    { 0xff, 0xff, 0x19, 0x00, 0x19, 0x00 },
    { 0x96, 0xd2, 0x4b, 0xff, 0x96, 0x78 },
};
static BombParam sMiniBombParam[4] = {
    { 0x64, 0xff, 0x14, 0x28, 0x14, 0x96 },
    { 0x28, 0x14, 0x64, 0x64, 0x96, 0xff },
    { 0xff, 0x64, 0x14, 0x28, 0x14, 0x96 },
    { 0x96, 0x64, 0xff, 0xff, 0xff, 0xff },
};

static int sThunderKind = 1;
static int sBombKind = 2;
static int sDemoKind = 3;

ESFunc sFuncList[4] = {
    &EffectScreen::doExplosion,
    &EffectScreen::doThunder,
    &EffectScreen::doBombExplosion,
    &EffectScreen::doDemo,
};

#define ES_VIEW_ASSERT(viewNo, max)                                                                        \
    ((viewNo) < (max)                                                                                      \
         ? (void)0                                                                                         \
         : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "RaceMgr.h", 195,                       \
                                       "range over: %d <= viewNo=%d < %d", 0, (viewNo), (max)),            \
            OSHalt("Halt")))

} // namespace

EffectScreen::EffectScreen()
    : mFrame(0), mActive(0), mParam(nullptr)
{
}

void EffectScreen::setFunction(const ScreenKind &kind)
{
    mFunc = sFuncList[kind];
}

bool EffectScreen::checkCamDrawing() const
{
    return mActive != 0;
}

bool EffectScreen::checkFrame()
{
    mFrame++;
    if (mFrame < mEndFrame) {
        return true;
    }
    mActive = 0;
    return false;
}

void EffectScreen::initThunder()
{
    mFrame = 0;
    mActive = 1;
    mFunc = sFuncList[1];
    mEndFrame = sThunderWhiteFrame + sThunderWaitFrame + sThunderDownFrame;
    mBlendMode = 1;
    mBlendSrc = 4;
    mBlendDst = 5;
    mBlendOp = 0;
    mR = 0xff;
    mG = 0xff;
    mB = 0xff;
    mAlpha = sThunderWhiteAlpha;
    mRate = 1.0f;
}

void EffectScreen::initDemo()
{
    mFrame = 0;
    mActive = 1;
    mFunc = sFuncList[3];
    mBlendMode = 1;
    mBlendSrc = 4;
    mBlendDst = 5;
    mBlendOp = 0;
    mR = 0;
    mG = 0;
    mB = 0;
    mAlpha = 0;
    mEndFrame = sDemoScreenAddFrame;
    mRate = 0.0f;
}

int EffectScreen::doDemo()
{
    mFrame++;

    if (mRate < 1.0f) {
        mRate = (f32)(u32)mFrame / (f32)(u32)mEndFrame;
    }

    mAlpha = (u8)((f32)(u32)sDemoScreenAlpha * mRate);

    return true;
}

void EffectScreen::setBombColorIndex(u8 index)
{
    mParam = &sRaceBombParam[index];
}

int EffectScreen::doThunder()
{
    f32 a = 1.0f;

    if (mFrame < sThunderWhiteFrame) {
        mAlpha = sThunderWhiteAlpha;
    } else {
        mB = 0;
        mG = 0;
        mR = 0;

        if (mFrame > (u8)(sThunderWhiteFrame + sThunderWaitFrame)) {
            int d = mFrame - (u8)(sThunderWhiteFrame + sThunderWaitFrame);
            a = 1.0f - (f32)d / (f32)(u32)sThunderDownFrame;
        }
        mAlpha = (u8)((f32)(u32)sThunderAlpha * a);
    }

    int ret = 1;
    mFrame++;
    if (mFrame >= mEndFrame) {
        mActive = ret = false;
    }
    return ret;
}

void EffectScreen::initBombExplosion(f32 length, u8 index)
{
    setBombColorIndex(index);
    mFrame = 0;
    mActive = 1;
    mFunc = sFuncList[2];
    mEndFrame = sBombExWhiteFrame + mParam->mWhiteFrame + mParam->mDownFrame;
    mBlendMode = 1;
    mBlendSrc = 4;
    mBlendDst = 5;
    mBlendOp = 0;
    mR = 0xff;
    mG = 0xff;
    mB = 0xff;
    mAlpha = sBombExWhiteAlpha;
    mRate = 1.0f - length / sBombExplosionLength;
}

int EffectScreen::doBombExplosion()
{
    f32 a = 1.0f;

    if (mFrame < sBombExWhiteFrame) {
        mAlpha = sBombExWhiteAlpha;
    } else {
        mR = mParam->mR;
        mG = mParam->mG;
        mB = mParam->mB;

        if (mFrame > (u8)(sBombExWhiteFrame + mParam->mWhiteFrame)) {
            int d = mFrame - (u8)(sBombExWhiteFrame + mParam->mWhiteFrame);
            a = 1.0f - (f32)d / (f32)(u32)mParam->mDownFrame;
        }
        mAlpha = (u8)((f32)(u32)mParam->mAlpha * a);
    }

    int ret = 1;
    mFrame++;
    if (mFrame >= mEndFrame) {
        mActive = ret = false;
    }
    return ret;
}

void EffectScreen::initExplosion(f32 rate)
{
    mFrame = 0;
    mActive = 1;
    mFunc = sFuncList[0];
    mEndFrame = sFlashWhiteFrame;
    mBlendMode = 1;
    mBlendSrc = 4;
    mBlendDst = 5;
    mBlendOp = 0;
    mR = 0xff;
    mG = 0xff;
    mB = 0xff;
    mAlpha = 0xff;
    mRate = rate;
}

int EffectScreen::doExplosion()
{
    f32 a = 1.0f - (f32)(u32)mFrame / (f32)(u32)mEndFrame;
    mAlpha = (u8)(mRate * (255.0f * a));
    mR = 255;
    mG = 255;

    f32 b = 1.0f - (f32)(u32)mFrame / (f32)(u32)mEndFrame;
    mB = (u8)(255.0f * b);

    int ret = 1;
    mFrame++;
    if (mFrame >= mEndFrame) {
        mActive = ret = false;
    }
    return ret;
}

void EffectScreen::calc()
{
    if (mActive) {
        (this->*mFunc)();
    }
}

void EffectScreen::draw()
{
    if (!mActive) {
        return;
    }

    GXSetChanMatColor(GX_COLOR0A0, *(GXColor *)&mR);
    GXSetBlendMode((GXBlendMode)mBlendMode, (GXBlendFactor)mBlendSrc, (GXBlendFactor)mBlendDst,
                   (GXLogicOp)mBlendOp);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3s16(mX, mY, 0);
    GXPosition3s16(mW, mY, 0);
    GXPosition3s16(mW, mH, 0);
    GXPosition3s16(mX, mH, 0);
}

EffectScreenMgr::EffectScreenMgr()
    : mLeft(0.0f), mTop(0.0f), mRight(640.0f), mBottom(480.0f), mTex(320, 240, GX_TF_I8)
{
    mNumScreens = RaceMgr::getManager()->mRaceInfo->mConsoleNum;
    mScreens = new EffectScreen[mNumScreens];

    KartCam *cam = GetKartCtrl()->getKartCam(0);
    mLeft = cam->mStartX;
    mTop = cam->mStartY;
    mRight = cam->mEndX;
    mBottom = cam->mEndY;

    PSMTXIdentity(mMtx);
    C_MTXOrtho(mProj, mTop, mBottom, mLeft, mRight, 0.0f, 1.0f);

    reset();
}

void EffectScreenMgr::reset()
{
    u8 i;
    EffectScreen *screen = mScreens;
    int j = 0;
    for (; (i = j) < mNumScreens; j++, screen++) {
        KartCtrl *k = GetKartCtrl();
        k->GetPortPtr(i);
#line 483
        JUT_ASSERT(k != 0);

        KartCam *cam = GetKartCtrl()->getKartCam(i);
        screen->mX = cam->GetPosv();
        screen->mY = cam->GetPosh();
        screen->mW = screen->mX + (s16)cam->GetWidth();
        screen->mH = screen->mY + (s16)cam->GetHeight();
        screen->mKartNo = i;
    }

    EffectScreen *s;
    for (int k = 0; k < mNumScreens; k++) {
        s = &mScreens[k];
        s->mFrame = 0;
        s->mActive = false;
    }

    mPara = -1;
    mKind = 5;
}

void EffectScreenMgr::calc()
{
    u32 total = 0;
    for (int i = 0; i < mNumScreens; i++) {
        int ret = 0;
        EffectScreen *screen = &mScreens[i];
        if (screen->mActive) {
            ret = (screen->*(screen->mFunc))();
        }
        total += ret;
    }

    if ((u8)total == 0) {
        mKind = 5;
        mPara = -1;
    }
}

void EffectScreenMgr::flashThunder(int para)
{
    mKind = sThunderKind;
    mPara = para;

    int off = 0;
    for (int k = 0; k < mNumScreens; k++) {
        EffectScreen *screen = (EffectScreen *)((u8 *)mScreens + off);
        u32 viewNo = screen->mKartNo;

        if (!(viewNo < (u32)RaceMgr::getManager()->mRaceInfo->mConsoleNum)) {
            JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "RaceMgr.h", 195,
                                       "range over: %d <= viewNo=%d < %d", 0, viewNo,
                                       RaceMgr::getManager()->mRaceInfo->mConsoleNum);
            OSHalt("Halt");
        }

        if (RaceMgr::getManager()->mConsole[viewNo].isDraw()) {
            screen->mFrame = 0;
            screen->mActive = 1;
            screen->mFunc = sFuncList[1];
            screen->mEndFrame = sThunderWhiteFrame + sThunderWaitFrame + sThunderDownFrame;
            screen->mBlendMode = 1;
            screen->mBlendSrc = 4;
            screen->mBlendDst = 5;
            screen->mBlendOp = 0;
            screen->mR = 0xff;
            screen->mG = 0xff;
            screen->mB = 0xff;
            screen->mAlpha = sThunderWhiteAlpha;
            screen->mRate = 1.0f;
        }
        off += 0x3c;
    }
}

void EffectScreenMgr::flashBombExplosion(u8 color, const JGeometry::TVec3f &pos)
{
    mKind = sBombKind;
    (void)color;
    (void)pos;
}

void EffectScreenMgr::startDemoScreen()
{
    mKind = sDemoKind;

    int off = 0;
    for (int k = 0; k < mNumScreens; k++) {
        EffectScreen *screen = (EffectScreen *)((u8 *)mScreens + off);
        u32 viewNo = screen->mKartNo;

        if (!(viewNo < (u32)RaceMgr::getManager()->mRaceInfo->mConsoleNum)) {
            JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "RaceMgr.h", 195,
                                       "range over: %d <= viewNo=%d < %d", 0, viewNo,
                                       RaceMgr::getManager()->mRaceInfo->mConsoleNum);
            OSHalt("Halt");
        }

        if (RaceMgr::getManager()->mConsole[viewNo].isDraw()) {
            screen->mFrame = 0;
            screen->mActive = 1;
            screen->mFunc = sFuncList[3];
            screen->mBlendMode = 1;
            screen->mBlendSrc = 4;
            screen->mBlendDst = 5;
            screen->mBlendOp = 0;
            screen->mR = 0;
            screen->mG = 0;
            screen->mB = 0;
            screen->mAlpha = 0;
            screen->mEndFrame = sDemoScreenAddFrame;
            screen->mRate = 0.0f;
        }
        off += 0x3c;
    }
}

bool EffectScreenMgr::IsContinue(EffectScreen::ScreenKind kind)
{
    return kind == mKind;
}

void EffectScreenMgr::initDraw()
{
    GetKartCtrl()->SetGrafPort();
}

void EffectScreenMgr::drawScreens(u32 viewNo)
{
    if (mScreens[viewNo].mActive) {
        mScreens[viewNo].draw();
    }
}

void EffectScreenMgr::draw_middle(u32 viewNo)
{
    if (mKind != 1) {
        return;
    }
    if (!mScreens[viewNo].mActive) {
        return;
    }

    GetKartCtrl()->SetGrafPort();
    GXLoadPosMtxImm(mMtx, 0);
    GXSetCurrentMtx(0);
    GXSetProjection(mProj, GX_ORTHOGRAPHIC);
    GXSetNumChans(1);
    GXSetNumTevStages(1);
    GXSetZMode(GX_TRUE, GX_LESS, GX_FALSE);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);

    EffectScreen *screen = &mScreens[viewNo];
    if (!screen->mActive) {
        return;
    }

    GXSetChanMatColor(GX_COLOR0A0, *(GXColor *)&screen->mR);
    GXSetBlendMode((GXBlendMode)screen->mBlendMode, (GXBlendFactor)screen->mBlendSrc,
                   (GXBlendFactor)screen->mBlendDst, (GXLogicOp)screen->mBlendOp);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3s16(screen->mX, screen->mY, 0);
    GXPosition3s16(screen->mW, screen->mY, 0);
    GXPosition3s16(screen->mW, screen->mH, 0);
    GXPosition3s16(screen->mX, screen->mH, 0);
}

void EffectScreenMgr::draw(u32 viewNo)
{
    if (mKind == 1) {
        return;
    }
    if (mKind == 3) {
        return;
    }
    if (RaceMgr::getManager()->mRaceDirector->isRaceEnd()) {
        return;
    }
    if (!mScreens[viewNo].mActive) {
        return;
    }

    GetKartCtrl()->SetGrafPort();
    GXLoadPosMtxImm(mMtx, 0);
    GXSetCurrentMtx(0);
    GXSetProjection(mProj, GX_ORTHOGRAPHIC);
    GXSetNumChans(1);
    GXSetNumTevStages(1);
    GXSetZMode(GX_TRUE, GX_LESS, GX_FALSE);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);

    EffectScreen *screen = &mScreens[viewNo];
    if (!screen->mActive) {
        return;
    }

    GXSetChanMatColor(GX_COLOR0A0, *(GXColor *)&screen->mR);
    GXSetBlendMode((GXBlendMode)screen->mBlendMode, (GXBlendFactor)screen->mBlendSrc,
                   (GXBlendFactor)screen->mBlendDst, (GXLogicOp)screen->mBlendOp);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3s16(screen->mX, screen->mY, 0);
    GXPosition3s16(screen->mW, screen->mY, 0);
    GXPosition3s16(screen->mW, screen->mH, 0);
    GXPosition3s16(screen->mX, screen->mH, 0);
}

void EffectScreenMgr::draw_demoScreen(u32 viewNo)
{
    if (mKind != 3) {
        return;
    }
    if (!mScreens[viewNo].mActive) {
        return;
    }

    GetKartCtrl()->SetGrafPort();
    GXLoadPosMtxImm(mMtx, 0);
    GXSetCurrentMtx(0);
    GXSetProjection(mProj, GX_ORTHOGRAPHIC);
    GXSetNumChans(1);
    GXSetNumTevStages(1);
    GXSetZMode(GX_TRUE, GX_LESS, GX_FALSE);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_NONE);
    GXSetNumTexGens(0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);

    EffectScreen *screen = &mScreens[viewNo];
    if (!screen->mActive) {
        return;
    }

    GXSetChanMatColor(GX_COLOR0A0, *(GXColor *)&screen->mR);
    GXSetBlendMode((GXBlendMode)screen->mBlendMode, (GXBlendFactor)screen->mBlendSrc,
                   (GXBlendFactor)screen->mBlendDst, (GXLogicOp)screen->mBlendOp);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3s16(screen->mX, screen->mY, 0);
    GXPosition3s16(screen->mW, screen->mY, 0);
    GXPosition3s16(screen->mW, screen->mH, 0);
    GXPosition3s16(screen->mX, screen->mH, 0);
}
