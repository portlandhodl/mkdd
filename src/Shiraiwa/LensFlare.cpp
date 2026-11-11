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

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

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

TLensFlareSupervisor *TLensFlare::sSupervisor;



TLensFlareScrn::TLensFlareScrn() {}

TLensFlareScrn::~TLensFlareScrn() {}

void TLensFlareScrn::init(u32 console, const JUTColor &color, u8 max, u8 min)
{
    mConsole = console;
    mColor.set(color);
    mColor.a = 0;
    mAlpha = 0;
    mPrevInside = false;
    mState = 0;
    u8 alpha = min;
    if (alpha == 0) {
        alpha = sAlphaMin;
    }
    mAlphaMin = alpha;
    alpha = max;
    if (alpha == 0) {
        alpha = sAlphaMax;
    }
    mAlphaMax = alpha;
}

#pragma auto_inline off
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

#pragma auto_inline reset
bool TLensFlareScrn::draw()
{
    if (mColor.a == 0) {
        return false;
    }
    setupGX();
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2f32(0.0f, 0.0f);
    GXColor4u8(mColor.r, mColor.g, mColor.b, mColor.a);
    GXPosition2f32(mViewport.f.x - mViewport.i.x, 0.0f);
    GXColor4u8(mColor.r, mColor.g, mColor.b, mColor.a);
    GXPosition2f32(mViewport.f.x - mViewport.i.x, mViewport.f.y - mViewport.i.y);
    GXColor4u8(mColor.r, mColor.g, mColor.b, mColor.a);
    GXPosition2f32(0.0f, mViewport.f.y - mViewport.i.y);
    GXColor4u8(mColor.r, mColor.g, mColor.b, mColor.a);
    return true;
}

void TLensFlareScrn::setupGX()
{
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
}

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

void TLensFlareSupervisor::entry(TLensFlare *flare)
{
    mList.append(&flare->mLink);
}

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
#line 321
        JUT_ASSERT_MSG(mTexInfo[i] != 0, "mTexture[i] != 0");
    }
}

TFlareLine::TFlareLine(u32 cameraNo)
{
    mCameraNo = cameraNo;
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

void TFlareLine::setTexture(ResTIMG **timg, int num)
{
    for (int i = 0; i < 21; i++) {
        if (mFlares[i] != nullptr && sFlareInfo[i].type < num)
            mFlares[i]->mTexture->storeTIMG(timg[sFlareInfo[i].type], (u8)0);
    }
}

void TFlareLine::getBufferZ() {}

void TFlareLine::reset(u8 a1, u8 r, u8 g, u8 b, u8 a5)
{
    SiUtil::getViewport(&mScrnBounds, (u8)mCameraNo, SiUtil::getDrawConsoleNum());
    init(mCameraNo, JUTColor(r, g, b, 0), a5, a5);
    u8 clamp = a1;
    if (clamp == 0) {
        clamp = sAlpha;
    }
    mAlphaClamp = clamp;
    mInside = 0;
    mAlpha2 = 0;
    mCenterX = -1.0f;
    mCenterY = -1.0f;
    mScrnScale = 1.0f;
    resetFlares();
}

f32 TFlareLine::getAspectRate()
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
#line 471
        JUT_ASSERT_MSG(0, "illegal console Number.");
        break;
    }
    return aspect;
}

void TFlareLine::resetFlares()
{
    f32 aspect = getAspectRate();
    f32 sx = mScrnBounds.getWidth() / (aspect * System::get2DVpW());
    f32 sy = mScrnBounds.getHeight() / System::get2DVpH();

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

static inline bool isSameBox(const JGeometry::TBox2f &a, const JGeometry::TBox2f &b)
{
    return a.f.x == b.f.x && a.f.y == b.f.y && a.i.x == b.i.x && a.i.y == b.i.y;
}

void TFlareLine::calc(const JGeometry::TVec3f &pos)
{
    JGeometry::TVec2f scrnPos;
    mBufferZ = SiUtil::getScreenPosCheckMirror(mCameraNo, pos, &scrnPos);

    if (mBufferZ == 0xffffffff) {
        KartCam *cam = RaceMgr::getManager()->getCamera(mCameraNo);
        scrnPos.set((f32)cam->GetVpPosh() - 1.0f, (f32)cam->GetVpPosv() - 1.0f);
    }

    JGeometry::TBox2f viewport;
    SiUtil::getViewport(&viewport, (u8)mCameraNo);

    if (!isSameBox(viewport, mScrnBounds)) {
        f32 aspect = getAspectRate();
        f32 sx = mScrnBounds.getWidth() / (aspect * System::get2DVpW());
        f32 sy = mScrnBounds.getHeight() / System::get2DVpH();
        for (int i = 0; i < 21; i++) {
            if (mFlares[i] != nullptr) {
                mFlares[i]->mRadius.x = sFlareInfo[i].scale * sx;
                mFlares[i]->mRadius.y = sFlareInfo[i].scale * sy;
            }
        }
        mScrnBounds.set(viewport.i, viewport.f);
    }

    mCenterX = scrnPos.x;
    mCenterY = scrnPos.y;

    JGeometry::TVec2f center;
    JGeometry::TVec2f dir;
    center.x = (mScrnBounds.i.x - mScrnBounds.f.x) * 0.5f + mScrnBounds.f.x;
    dir.x = center.x - scrnPos.x;
    center.y = (mScrnBounds.i.y - mScrnBounds.f.y) * 0.5f + mScrnBounds.f.y;
    f32 height = mScrnBounds.f.y - mScrnBounds.i.y;
    dir.y = center.y - scrnPos.y;
    f32 radius = sOutRadius * (height / System::get2DVpH());

    if (0.0f != radius) {
        mScrnScale = dir.squared() / (radius * radius);
        f32 scale = mScrnScale;
        if (scale > 1.0f)
            scale = 1.0f;
        else if (scale < 0.0f)
            scale = 0.0f;
        mScrnScale = scale;

        dir.normalize();
        mAngle = atan2(-dir.x, dir.y);
        dir.scale(sFlareInterval);
        setPosition(center, dir);

        mInside = isInside(scrnPos);
        if (mInside) {
            u32 z = RaceMgr::getManager()->mRaceDrawer->getCaptureMgr()->getZValue(8, mCameraNo);
            RaceMgr::getManager()->mRaceDrawer->getCaptureMgr()->setPosition(8, mCameraNo, (s16)mCenterX, (s16)mCenterY);
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

        f32 range = SiUtil::getNormalRange(JGeometry::TUtil<f32>::abs(mCenterX - center.x), 0.0f, 0.5f * mScrnBounds.getWidth());
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
    JGeometry::TVec2f dir(mCenterX, mCenterY);
    dir -= pos;
    if (0.0f == dir.y)
        return;

    dir.y = __fabsf(dir.y);
    f32 interval = (2.0f * (sFlareInterval * dir.y)) / 21.0f;
    dir.normalize();
    System::get2DVpH();

    for (int i = 0; i < 21; i++) {
        if (mFlares[i] != nullptr) {
            mFlares[i]->mCenter.x = 0.0f;
            mFlares[i]->mCenter.y = (interval * (f32)i) / dir.y;
        }
    }
}

u8 TFlareLine::draw()
{
    u8 drawn = 1;
    if (mAlpha2 > 0) {
        KartCam *cam = RaceMgr::getManager()->getCamera(mCameraNo);

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

    return drawn | TLensFlareScrn::draw();
}

void TFlareLine::drawFixCenter(TFlare &flare)
{
    JUTTexture *tex = flare.mTexture;
    f32 sx = sScale * (0.5f * (flare.mRadius.x * tex->getWidth()));
    f32 sy = sScale * (0.5f * (flare.mRadius.y * tex->getHeight()));
    f32 x = flare.mCenter.x - 0.5f * sx;
    f32 y = flare.mCenter.y - 0.5f * sy;

    tex->load(GX_TEXMAP0);
    GXSetTevColor(GX_TEVREG0, JUTColor((u32)flare.mColor));
    GXSetTevColor(GX_TEVREG1, JUTColor((u32)flare.mColor + mAlpha2));

    if (flare.mAdditive)
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_C1, GX_CC_TEXC, GX_CC_ZERO);
    else
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO);

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2f32(x, y);
    GXTexCoord2s8(0, 0);
    GXPosition2f32(x + sx, y);
    GXTexCoord2s8(2, 0);
    GXPosition2f32(x + sx, y + sy);
    GXTexCoord2s8(2, 2);
    GXPosition2f32(x, y + sy);
    GXTexCoord2s8(0, 2);
}

TLensFlare::TLensFlare(const CrsData::SObject &obj) : TMapObjHioNode(obj), JKRDisposer(), mLink(this)
{
    if (sSupervisor == nullptr) {
        sSupervisor = new TLensFlareSupervisor();
    }
    sSupervisor->entry(this);
    u8 num = SiUtil::getConsoleNum();
    mFlares = new TFlareLine[num];
}

void TLensFlare::reset()
{
    resetObject();
    for (int i = 0; i < SiUtil::getConsoleNum(); i++) {
        mFlares[i].mCameraNo = i;
        mFlares[i].setTexture(sSupervisor->mTexInfo, 4);
        u8 a1 = mObjData->mParam1;
        u8 r = mObjData->mParam2;
        if (r == 0)
            r = 192;
        u8 g = mObjData->mParam3;
        if (g == 0)
            g = 192;
        u8 b = mObjData->mParam4;
        if (b == 0)
            b = 192;
        mFlares[i].reset(a1, r, g, b, mObjData->mParam5);
    }
}

void TLensFlare::calc()
{
    for (int i = 0; i < SiUtil::getConsoleNum(); i++) {
        mFlares[i].calc(mPos);
    }
}

u8 TLensFlare::draw(u32 viewNo)
{
    return mFlares[viewNo].draw();
}

void TLensFlare::getBufferZ(u32) {}

DECL_WEAK TLensFlare::~TLensFlare()
{
    sSupervisor = nullptr;
}
