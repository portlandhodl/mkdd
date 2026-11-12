#include "Sato/EffectScreen.h"

#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/RaceInfo.h"
#include "Yamamoto/kartCtrl.h"
#include "Yamamoto/kartCamera.h"

static const f32 sDummy[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

namespace {

typedef int (EffectScreen::*ESFunc)();

static u8 sThunderWhiteFrame = 3;
static u8 sThunderWaitFrame = 50;
static u8 sThunderDownFrame = 15;
static u8 sThunderWhiteAlpha = 0xd2;
static u8 sThunderAlpha = 0x96;
static u8 sBombExWhiteFrame = 3;
static u8 sBombExWhiteAlpha = 0xd2;
static f32 sBombExplosionLength = 3000.0f;
static u8 sFlashWhiteFrame = 7;
static u8 sDemoScreenAlpha = 0xbe;
static u8 sDemoScreenAddFrame = 10;

static BombParam sRaceBombParam[2] = {
    { 0x00, 0x19, 0xff, 0xff, 0xd2, 0x96 },
    { 0x00, 0x19, 0x78, 0x96, 0xff, 0x4b },
};
static BombParam sMiniBombParam[4] = {
    { 0x28, 0x14, 0xff, 0x64, 0x64, 0x96 },
    { 0x28, 0x14, 0x64, 0x64, 0xff, 0x96 },
    { 0x28, 0x14, 0x64, 0xff, 0x64, 0x96 },
    { 0x28, 0x14, 0xff, 0xff, 0x64, 0x96 },
};

ESFunc sFuncList[4] = {
    &EffectScreen::doExplosion,
    &EffectScreen::doThunder,
    &EffectScreen::doBombExplosion,
    &EffectScreen::doDemo,
};

} // namespace

EffectScreen::EffectScreen()
    : mFrame(0), mActive(0), mParam(nullptr)
{
}

static inline bool isDrawConsole(RaceMgr *rm, u32 viewNo)
{
    viewNo < rm->getConsoleNumber()
        ? (void)0
        : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "RaceMgr.h", 195, "range over: %d <= viewNo=%d < %d", 0,
                                      viewNo, rm->getConsoleNumber()),
           OSPanic("RaceMgr.h", 195, "Halt"));
    return rm->mConsole[viewNo].isDraw();
}

void EffectScreen::setFunction(const ScreenKind &kind)
{
    mFunc = sFuncList[kind];
}

bool EffectScreen::checkCamDrawing() const
{
    return isDrawConsole(RaceMgr::getManager(), mKartNo);
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
    if (checkCamDrawing()) {
        mFrame = 0;
        mActive = 1;
        setFunction(Kind_1);
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
}

void EffectScreen::initDemo()
{
    if (checkCamDrawing()) {
        mFrame = 0;
        mActive = 1;
        setFunction(Kind_3);
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

void EffectScreen::setBombColorIndex(u8 color)
{
    if (RaceMgr::getManager()->getRaceMode() == BOMB_BATTLE) {
#line 226
        JUT_MINMAX_ASSERT(0, color, 5);
        mParam = &sMiniBombParam[color];
    } else {
#line 231
        JUT_MINMAX_ASSERT(0, color, 2);
        mParam = &sRaceBombParam[color];
    }
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

void EffectScreen::initBombExplosion(f32 length, u8 color)
{
    setBombColorIndex(color);
    if (checkCamDrawing()) {
        mFrame = 0;
        mActive = 1;
        setFunction(Kind_2);
        mEndFrame = sBombExWhiteFrame + mParam->mWhiteFrame + mParam->mDownFrame;
        mBlendMode = 1;
        mBlendSrc = 4;
        mBlendDst = 5;
        mBlendOp = 0;
        mR = 0xff;
        mG = 0xff;
        mB = 0xff;
        mAlpha = sBombExWhiteAlpha;
        mRate = 1.0f;
    }
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

void EffectScreen::initExplosion(f32 length)
{
    if (checkCamDrawing()) {
        mFrame = 0;
        mActive = 1;
        setFunction(Kind_0);
        mEndFrame = sFlashWhiteFrame;
        mBlendMode = 1;
        mBlendSrc = 4;
        mBlendDst = 5;
        mBlendOp = 0;
        static f32 maxLength = 20000.0f;
        mRate = 1.0f - length / maxLength;
        mRate = (mRate < 0.0f) ? 0.0f : mRate;
        if (mRate < 0.005f) {
            mActive = 0;
        }
    }
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

int EffectScreen::calc()
{
    int ret = 0;
    if (mActive) {
        ret = (this->*mFunc)();
    }
    return ret;
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
        total += mScreens[i].calc();
    }

    if ((u8)total == 0) {
        mKind = 5;
        mPara = -1;
    }
}

void EffectScreenMgr::flashThunder(int para)
{
    setKind(EffectScreen::Kind_1);
    mPara = para;
    for (int i = 0; i < mNumScreens; i++) {
        mScreens[i].initThunder();
    }
}

void EffectScreenMgr::flashBombExplosion(u8 color, const JGeometry::TVec3f &pos)
{
    setKind(EffectScreen::Kind_2);

    for (u8 i = 0; i < mNumScreens; i++) {
        EffectScreen *screen = &mScreens[i];
        JGeometry::TVec3f dir;
        dir.sub(*GetKartCtrl()->getKartCam(screen->mKartNo)->GetCameraPos(), pos);
        f32 length = dir.length();
        if (length < sBombExplosionLength) {
            screen->initBombExplosion(length, color);
        } else if (RaceMgr::getManager()->getRaceMode() != BOMB_BATTLE) {
            screen->initExplosion(length);
        }
    }
}

void EffectScreenMgr::startDemoScreen()
{
    setKind(EffectScreen::Kind_3);
    for (int i = 0; i < mNumScreens; i++) {
        mScreens[i].initDemo();
    }
}

bool EffectScreenMgr::IsContinue(EffectScreen::ScreenKind kind)
{
    return kind == mKind;
}

void EffectScreenMgr::initDraw()
{
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
}

void EffectScreenMgr::drawScreens(u32 viewNo)
{
    if (mScreens[viewNo].mActive) {
        initDraw();
        mScreens[viewNo].draw();
    }
}

void EffectScreenMgr::draw_middle(u32 viewNo)
{
    if (mKind != 1) {
        return;
    }
    drawScreens(viewNo);
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
    drawScreens(viewNo);
}

void EffectScreenMgr::draw_demoScreen(u32 viewNo)
{
    if (mKind != 3) {
        return;
    }
    drawScreens(viewNo);
}

#include "JSystem/JAudio/JASFakeMatch2.h"
