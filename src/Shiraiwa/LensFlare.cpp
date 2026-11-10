#include "Shiraiwa/LensFlare.h"

#include <JSystem/J2D/J2DGrafContext.h>
#include <JSystem/JUtility/JUTAssert.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

#include "JSystem/JAudio/JASFakeMatch2.h"
#include "Kaneshige/RaceDrawer.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/system.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Yamamoto/kartCamera.h"
#include "Yamamoto/kartCtrl.h"

const char *TLensFlareSupervisor::fileName[4] = {
    "/Objects/mk_LensFX01.bti",
    "/Objects/mk_LensFX02.bti",
    "/Objects/mk_LensFX03.bti",
    "/Objects/mk_LensFX04.bti",
};

TFlareInfo TFlareLine::sFlareInfo[21] = {
    { 1, 10.0f, 0x23, 0x46, 0xaf, 0 },
    { 5, 0.0f, 0, 0, 0, 0 },
    { 3, 0.7f, 0x64, 0x64, 0xc8, 0 },
    { 3, 1.0f, 0x4b, 0xaa, 0x4b, 0 },
    { 0, 2.0f, 0x7d, 0x7d, 0x32, 0 },
    { 0, 1.5f, 0x3c, 0x1e, 0x1e, 0 },
    { 5, 0.0f, 0, 0, 0, 0 },
    { 5, 0.0f, 0, 0, 0, 0 },
    { 1, 0.35f, 0xf5, 0xa0, 0xff, 0 },
    { 0, 3.37f, 0x7d, 0x1e, 0, 0 },
    { 3, 0.8f, 0x7d, 0x4b, 0x4b, 0 },
    { 3, 1.0f, 0xaa, 0x7d, 0x4b, 0 },
    { 3, 1.57f, 0x64, 0x96, 0xc3, 0 },
    { 0, 2.27f, 0, 0x32, 0x7d, 0 },
    { 1, 0.3f, 0xb9, 0xc8, 0xff, 0 },
    { 1, 3.8f, 0x96, 0x28, 0x28, 0 },
    { 0, 5.0f, 0x2d, 0x8c, 0x91, 0 },
    { 0, 2.0f, 0x1e, 0x64, 0xaf, 0 },
    { 5, 0.0f, 0, 0, 0, 0 },
    { 5, 0.0f, 0, 0, 0, 0 },
    { 2, 10.0f, 0xff, 0xff, 0xff, 0 },
};

u8 TLensFlareScrn::sAlphaAccel = 8;
u8 TLensFlareScrn::sAlphaDecel = 4;
u8 TLensFlareScrn::sAlphaMax = 100;
u8 TLensFlareScrn::sAlphaMin = 100;

f32 TFlareLine::sScale = 1.2f;
f32 TFlareLine::sFlareInterval = 1.05f;
f32 TFlareLine::sOutRadius = 120.0f;
u8 TFlareLine::sAlpha = 40;
u8 TFlareLine::sAlphaAccel = 2;
u8 TFlareLine::sAlphaDecel = 4;
f32 TFlareLine::sScrnScaleRange = 0.7f;

#define CAMERA_ASSERT(n)                                                                                                     \
    ((0 <= (n) && (n) < 4)                                                                                                  \
         ? (void)0                                                                                                          \
         : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "RaceMgr.h", 262, "range over: %d <= cameraNo=%d < %d", 0, (n), 4), \
            OSPanic("RaceMgr.h", 262, "Halt")))

TLensFlareSupervisor *TLensFlare::sSupervisor;



TLensFlareScrn::TLensFlareScrn()
{
    mColor.set(0xffffffff);
}

TLensFlareScrn::~TLensFlareScrn() {}

void TLensFlareScrn::init(u32 console, const JUTColor &color, u8 max, u8 min)
{
    mConsole = console;
    mColor = color;
    mAlphaMax = max;
    mAlphaMin = min;
}

void TLensFlareScrn::calc(bool inside, f32 scale)
{
    switch (mState) {
    case 1:
        if (mPrevInside != 0 && !inside) {
            frameOut();
            mPrevInside = inside;
        }
        mAlpha += sAlphaAccel;
        if (mAlpha > mAlphaMax) {
            mAlpha = mAlphaMax;
            mState = 2;
        }
        break;
    case 2:
        if (!inside) {
            frameOut();
            mPrevInside = inside;
        }
        mAlpha -= sAlphaDecel;
        if (mAlpha < mAlphaMin)
            mAlpha = mAlphaMin;
        break;
    case 3:
        mAlpha -= sAlphaDecel;
        if (mAlpha < 0) {
            mAlpha = 0;
            mState = 0;
        }
        break;
    case 0:
        if (mPrevInside == 0 && inside) {
            frameIn();
            mPrevInside = inside;
        }
        break;
    }
    mColor.a = (u8)(mAlpha * scale);
    SiUtil::getViewport(&mViewport, (u8)mConsole);
}

void TLensFlareScrn::draw() {}

void TLensFlareScrn::setupGX() {}

void TLensFlareScrn::frameIn()
{
    if (mState != 0)
        return;
    mState = 1;
    mAlpha = 0;
    mColor.a = 0;
}

void TLensFlareScrn::frameOut()
{
    mState = 3;
}

TLensFlareSupervisor::TLensFlareSupervisor()
{
    mList.initiate();
    mTexInfo = new ResTIMG *[4];
}

TLensFlareSupervisor::~TLensFlareSupervisor()
{
    delete[] mTexInfo;
}

void TLensFlareSupervisor::entry(TLensFlare *) {}

void TLensFlareSupervisor::draw(u32 viewNo)
{
    KartCam *cam = KartCtrl::getKartCtrl()->getKartCam(viewNo);
    J2DOrthoGraph ortho(cam->GetPosv(), cam->GetPosh(), cam->GetPosv() + cam->GetWidth(), cam->GetPosh() + cam->GetHeight(),
                        -1.0f, 1.0f);
    ortho.setPort();
    setupGX();

    for (JSULink<TLensFlare> *link = mList.getFirst(); link != nullptr; link = link->getNext()) {
        if (link->getObject()->draw(viewNo))
            break;
    }
}

void TLensFlareSupervisor::getBufferZ(u32) {}

void TLensFlareSupervisor::setupGX()
{
    GXSetCullMode(GX_CULL_BACK);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, (GXCompCnt)1, (GXCompType)1, 0);
    GXSetNumChans(0);
    GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
    GXColor black = { 0xff, 0xff, 0xff, 0xff };
    GXSetChanAmbColor(GX_COLOR0A0, black);
    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_A1, GX_CA_TEXA, GX_CA_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
}

void TLensFlareSupervisor::load()
{
    for (int i = 0; i < 4; i++) {
        mTexInfo[i] = (ResTIMG *)ObjUtility::getPtrCourseArc((char *)fileName[i]);
        if (mTexInfo[i] == nullptr) {
            JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "LensFlare.cpp", 0x141, "%s", "mTexture[i] != 0");
            OSPanic("LensFlare.cpp", 0x141, "mTexture[i] != 0");
        }
    }
}

#pragma inline_depth(4)
TFlareLine::TFlareLine(u32)
{
    mCameraNo = 0;
    mFlares = new TFlare *[21];
    for (int i = 0; i < 21; i++) {
        if (sFlareInfo[i].type != 5) {
            mFlares[i] = new TFlare();
            mFlares[i]->mTexture = new JUTTexture();
        } else {
            mFlares[i] = nullptr;
        }
    }
}

TFlareLine::~TFlareLine()
{
    delete[] mFlares;
}

void TFlareLine::setTexture(ResTIMG **, int) {}

void TFlareLine::getBufferZ() {}

void TFlareLine::reset(u8 a1, u8 r, u8 g, u8 b, u8 a5)
{
    SiUtil::getViewport(&mScrnBounds, mCameraNo, SiUtil::getDrawConsoleNum());
    mColor.set(JUTColor(r, g, b, 0));
    mConsole = mCameraNo;
    mAlpha = 0;
    mPrevInside = 0;
    mState = 0;
    mAlphaMin = a5 ? a5 : sAlphaMin;
    mAlphaMax = a5 ? a5 : sAlphaMax;
    mAlphaClamp = a1 ? a1 : sAlpha;
    mInside = 0;
    mCenterX = -1.0f;
    mAlpha2 = 0;
    mCenterY = -1.0f;
    mScrnScale = 1.0f;
    resetFlares();
}

f32 TFlareLine::getAspectRate() { return 0.0f; }

void TFlareLine::resetFlares()
{
    f32 aspect;
    switch (SiUtil::getDrawConsoleNum()) {
    case 1:
        aspect = System::getPerspAspect();
        aspect = System::getPerspAspect() / aspect;
        break;
    case 2:
        aspect = System::getPerspAspect();
        aspect = System::getPerspAspectDiv2() / aspect;
        break;
    case 3:
    case 4:
        aspect = System::getPerspAspect();
        aspect = System::getPerspAspectDiv4() / aspect;
        break;
    default:
        JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "LensFlare.cpp", 0x1d7, "%s", "illegal console Number.");
        OSPanic("LensFlare.cpp", 0x1d7, "illegal console Number.");
        break;
    }

    f32 w = mScrnBounds.f.x - mScrnBounds.i.x;
    f32 fw = aspect * System::get2DVpW();
    f32 sx = w / fw;
    f32 h = mScrnBounds.f.y - mScrnBounds.i.y;
    f32 fh = System::get2DVpH();
    f32 sy = h / fh;

    for (int i = 0; i < 21; i++) {
        if (mFlares[i] == nullptr)
            continue;
        mFlares[i]->mRadius.x = sFlareInfo[i].scale * sx;
        mFlares[i]->mRadius.y = sFlareInfo[i].scale * sy;
        mFlares[i]->mCenter.x = 0.0f;
        mFlares[i]->mCenter.y = 0.0f;
        mFlares[i]->mColor.set(sFlareInfo[i].r, sFlareInfo[i].g, sFlareInfo[i].b, 0);
        if (sFlareInfo[i].type == 2)
            mFlares[i]->mAdditive = 0;
        else
            mFlares[i]->mAdditive = 1;
    }
}

void TFlareLine::calc(const JGeometry::TVec3f &pos)
{
    JGeometry::TVec2f scrnPos;
    mBufferZ = SiUtil::getScreenPosCheckMirror(mCameraNo, pos, &scrnPos);

    if (mBufferZ == (u32)-1) {
        CAMERA_ASSERT(mCameraNo);
        KartCam *cam = KartCtrl::getKartCtrl()->getKartCam(mCameraNo);
        scrnPos.y = (f32)cam->GetVpPosv() - 1.0f;
        scrnPos.x = (f32)cam->GetVpPosh() - 1.0f;
    }

    JGeometry::TBox2f viewport;
    SiUtil::getViewport(&viewport, (u8)mCameraNo);

    if (viewport.f.x != mScrnBounds.f.x || viewport.f.y != mScrnBounds.f.y || viewport.i.x != mScrnBounds.i.x
        || viewport.i.y != mScrnBounds.i.y) {
        CAMERA_ASSERT(mCameraNo);

        f32 aspect;
        switch (SiUtil::getDrawConsoleNum()) {
        case 1:
            aspect = System::getPerspAspect();
            aspect = System::getPerspAspect() / aspect;
            break;
        case 2:
            aspect = System::getPerspAspect();
            aspect = System::getPerspAspectDiv2() / aspect;
            break;
        case 3:
        case 4:
            aspect = System::getPerspAspect();
            aspect = System::getPerspAspectDiv4() / aspect;
            break;
        default:
            JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "LensFlare.cpp", 0x1d7, "%s", "illegal console Number.");
            OSPanic("LensFlare.cpp", 0x1d7, "illegal console Number.");
            break;
        }

        f32 sx = (viewport.f.x - viewport.i.x) / (aspect * System::get2DVpW());
        f32 sy = (viewport.f.y - viewport.i.y) / System::get2DVpH();
        for (int i = 0; i < 21; i++) {
            TFlare *flare = mFlares[i];
            if (flare == nullptr)
                continue;
            flare->mRadius.x = sFlareInfo[i].scale * sx;
            flare->mRadius.y = sFlareInfo[i].scale * sy;
        }
        mScrnBounds = viewport;
    }

    mCenterX = scrnPos.x;
    mCenterY = scrnPos.y;

    f32 cx = (mScrnBounds.i.x - mScrnBounds.f.x) * 0.5f + mScrnBounds.f.x;
    f32 dx = cx - scrnPos.x;
    f32 cy = (mScrnBounds.i.y - mScrnBounds.f.y) * 0.5f + mScrnBounds.f.y;
    f32 dy = cy - scrnPos.y;
    f32 radius = sOutRadius * ((mScrnBounds.f.y - mScrnBounds.i.y) / System::get2DVpH());

    if (radius != 0.0f) {
        f32 d2 = dx * dx + dy * dy;
        mScrnScale = d2 / (radius * radius);
        if (mScrnScale > 1.0f)
            mScrnScale = 1.0f;
        else if (mScrnScale < 0.0f)
            mScrnScale = 0.0f;

        f32 len = dx * dx + dy * dy;
        f32 inv;
        if (JGeometry::TUtil<f32>::epsilon() < len)
            inv = JGeometry::TUtil<f32>::inv_sqrt(len);
        else
            inv = len;
        dx *= inv;
        dy *= inv;

        mAngle = JGeometry::TUtil<f32>::atan2(-dx, dy);

        JGeometry::TVec2f offset(dx * sFlareInterval, dy * sFlareInterval);
        JGeometry::TVec2f center(cx, cy);
        setPosition(center, offset);

        mInside = isInside(scrnPos);
        if (mInside) {
            ZCaptureMgr *cap = RaceMgr::getManager()->mRaceDrawer->getCaptureMgr();
            u32 z = cap->getZValue(8, mCameraNo);
            cap->setPosition(8, mCameraNo, (s16)mCenterX, (s16)mCenterY);
            if (z < mBufferZ)
                mInside = 0;
        }

        if (!mInside)
            mAlpha2 -= sAlphaDecel;
        else
            mAlpha2 += sAlphaAccel;

        if (mAlpha2 < 0)
            mAlpha2 = 0;
        else if (mAlpha2 > mAlphaClamp)
            mAlpha2 = mAlphaClamp;

        f32 range = SiUtil::getNormalRange(__fabsf(mCenterX - cx), 0.0f, (mScrnBounds.f.x - mScrnBounds.i.x) * 0.5f);
        TLensFlareScrn::calc(mInside, 1.0f - sScrnScaleRange * range);
    }
}

bool TFlareLine::isInside(const JGeometry::TVec2f &pos)
{
    bool ret = true;
    if (pos.x < mScrnBounds.i.x || pos.x > mScrnBounds.f.x || pos.y < mScrnBounds.i.y || pos.y > mScrnBounds.f.y)
        ret = false;
    return ret;
}

void TFlareLine::setPosition(const JGeometry::TVec2f &pos, const JGeometry::TVec2f &)
{
    f32 dy = mCenterY - pos.y;
    f32 dx = mCenterX - pos.x;
    if (dy == 0.0f)
        return;

    f32 ady = __fabsf(dy);
    f32 interval = (2.0f * (sFlareInterval * ady)) / 21.0f;
    f32 len = dx * dx + ady * ady;
    f32 inv;
    if (JGeometry::TUtil<f32>::epsilon() < len)
        inv = JGeometry::TUtil<f32>::inv_sqrt(len);
    else
        inv = len;
    f32 norm = ady * inv;

    for (int i = 0; i < 21; i++) {
        TFlare *flare = mFlares[i];
        if (flare == nullptr)
            continue;
        flare->mCenter.x = 0.0f;
        flare->mCenter.y = (interval * (f32)i) / norm;
    }
}

#pragma inline_depth(2)
bool TFlareLine::draw()
{
    u8 drawn = 1;
    KartCam *cam;
    if (mAlpha2 > 0) {
        cam = RaceMgr::getManager()->getCamera(mCameraNo);

        Mtx rot;
        PSMTXRotRad(rot, 0x5a, mAngle);
        Mtx trans;
        PSMTXTrans(trans, mCenterX - (f32)cam->GetVpPosv(), mCenterY - (f32)cam->GetVpPosh(), 0.0f);
        PSMTXConcat(trans, rot, trans);
        GXLoadPosMtxImm(trans, GX_PNMTX0);
        GXSetCurrentMtx(GX_PNMTX0);

        for (int i = 0; i < 21; i++) {
            if (mFlares[i] != nullptr)
                drawFixCenter(*mFlares[i]);
        }
    } else {
        drawn = 0;
    }

    u8 quad = 1;
    if (mColor.a == 0) {
        quad = 0;
    } else {
        GXSetCullMode(GX_CULL_BACK);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT0, GX_DF_NONE, GX_AF_NONE);
        GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXColor black = { 0xff, 0xff, 0xff, 0xff };
        GXSetChanAmbColor(GX_COLOR0A0, black);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);

        Mtx mtx;
        PSMTXIdentity(mtx);
        GXLoadPosMtxImm(mtx, GX_PNMTX0);
        GXSetCurrentMtx(GX_PNMTX0);
        GXLoadTexMtxImm(mtx, GX_IDENTITY, GX_MTX3x4);
        GXSetNumTexGens(0);
        GXSetNumTevStages(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        GXPosition2f32(0.0f, 0.0f);
        GXColor4u8(mColor.r, mColor.g, mColor.b, mColor.a);
        GXPosition2f32(mViewport.f.x - mViewport.i.x, 0.0f);
        GXColor4u8(mColor.r, mColor.g, mColor.b, mColor.a);
        GXPosition2f32(mViewport.f.x - mViewport.i.x, mViewport.f.y - mViewport.i.y);
        GXColor4u8(mColor.r, mColor.g, mColor.b, mColor.a);
        GXPosition2f32(0.0f, mViewport.f.y - mViewport.i.y);
        GXColor4u8(mColor.r, mColor.g, mColor.b, mColor.a);
    }

    return drawn | quad;
}
#pragma inline_depth(4)

void TFlareLine::drawFixCenter(TFlare &flare)
{
    JUTTexture *tex = flare.mTexture;
    f32 width = tex->getWidth();
    f32 height = tex->getHeight();
    f32 rx = flare.mRadius.x;
    f32 ry = flare.mRadius.y;
    f32 sx = sScale * (0.5f * (rx * width));
    f32 sy = sScale * (0.5f * (ry * height));
    f32 x = flare.mCenter.x - 0.5f * sx;
    f32 y = flare.mCenter.y - 0.5f * sy;

    tex->load(GX_TEXMAP0);

    GXColor color;
    *(u32 *)&color = *(u32 *)&flare.mColor;
    GXColor colorArg = color;
    GXSetTevColor(GX_TEVREG0, colorArg);
    u32 c2 = *(u32 *)&flare.mColor + mAlpha2;
    GXColor color2;
    *(u32 *)&color2 = c2;
    GXColor color2Arg = color2;
    GXSetTevColor(GX_TEVREG1, color2Arg);

    if (flare.mAdditive)
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_C1, GX_CC_TEXC, GX_CC_ZERO);
    else
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO);

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(x, y, 0.0f);
    GXColor1u32(flare.mColor);
    GXPosition3f32(x + sx, y, 0.0f);
    GXColor1u32(flare.mColor);
    GXPosition3f32(x + sx, y + sy, 0.0f);
    GXColor1u32(flare.mColor);
    GXPosition3f32(x, y + sy, 0.0f);
    GXColor1u32(flare.mColor);
}

#pragma inline_depth(4)
TLensFlare::TLensFlare(const CrsData::SObject &obj) : TMapObjHioNode(obj), JKRDisposer(), mLink(this)
{
    if (sSupervisor == nullptr) {
        sSupervisor = new TLensFlareSupervisor();
    }
    sSupervisor->mList.append(&mLink);
    u8 num = SiUtil::getConsoleNum();
    mFlares = new TFlareLine[num];
}

#pragma inline_depth(0)
void TLensFlare::reset()
{
    resetObject();
    for (int i = 0; i < SiUtil::getConsoleNum(); i++) {
        mFlares[i].mCameraNo = i;
        TFlareLine *flare = &mFlares[i];
        ResTIMG **timg = sSupervisor->mTexInfo;
        for (int j = 0; j < 21; j++) {
            TFlareLine::TFlare *f = flare->mFlares[j];
            if (f != nullptr && TFlareLine::sFlareInfo[j].type < 4)
                f->mTexture->storeTIMG(timg[TFlareLine::sFlareInfo[j].type], (u8)0);
        }
        const CrsData::SObject *d = mObjData;
        u8 a1 = d->mParam1;
        u8 r = d->mParam2;
        if (r == 0)
            r = 192;
        u8 g = d->mParam3;
        if (g == 0)
            g = 192;
        u8 b = d->mParam4;
        if (b == 0)
            b = 192;
        u8 a5 = d->mParam5;
        mFlares[i].reset(a1, r, g, b, a5);
    }
}
#pragma inline_depth(4)

void TLensFlare::calc()
{
    for (int i = 0; i < SiUtil::getConsoleNum(); i++) {
        mFlares[i].calc(mPos);
    }
}

bool TLensFlare::draw(u32 viewNo)
{
    return mFlares[viewNo].draw();
}

void TLensFlare::getBufferZ(u32) {}

DECL_WEAK TLensFlare::~TLensFlare()
{
    sSupervisor = nullptr;
}
