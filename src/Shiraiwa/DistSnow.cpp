#include "Shiraiwa/Objects/DistSnow.h"

#include "Sato/ObjUtility.h"
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

TDistSnowSupervisor *TDistSnow::sSupervisor;

#include "JSystem/JAudio/JASFakeMatch2.h"

void TDistSnowSupervisor::draw(u32) {}

void TDistSnowSupervisor::load()
{
    mDistSnow->load();
}

TSnowPanel::TSnowPanel() {}

TSnowPanel::~TSnowPanel() {}

void TSnowPanel::reset()
{
    mPos.y = 0.0f;
    mPos.x = 0.0f;
    mVel = 0.0f;
    mLife = 0;
    mWidth = 0.0f;
    mHeight = 0.0f;
    mAlpha = 0;
}

void TSnowPanel::calc()
{
    if (mLife != 0) {
        mPos.y += mVel;
        mLife--;
        if (mLife < 30) {
            mAlpha -= 10;
            if (mAlpha < 0) {
                mAlpha = 0;
            }
        } else if (mAlpha < 255) {
            mAlpha += 10;
            if (mAlpha > 255) {
                mAlpha = 255;
            }
        }
    }
}

void TSnowPanel::draw()
{
    if (mLife != 0) {
        f32 hw = 0.5f * mWidth;
        f32 hh = 0.5f * mHeight;
        GXColor color;
        color.r = 255;
        color.g = 255;
        color.b = 255;
        color.a = mAlpha;
        GXSetTevColor(GX_TEVREG0, color);
        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        GXPosition2f32(mPos.x - hw, mPos.y - hh);
        GXTexCoord2s8(0, 0);
        GXPosition2f32(mPos.x + hw, mPos.y - hh);
        GXTexCoord2s8(1, 0);
        GXPosition2f32(mPos.x + hw, mPos.y + hh);
        GXTexCoord2s8(1, 1);
        GXPosition2f32(mPos.x - hw, mPos.y + hh);
        GXTexCoord2s8(0, 1);
        GXEnd();
    }
}

TDistSnow::TDistSnow(const CrsData::SObject &obj) : TMapObjHioNode(obj)
{
    mPanels = new TSnowPanel[8];
    mTexture = new JUTTexture();
    if (sSupervisor == nullptr) {
        sSupervisor = new TDistSnowSupervisor();
    }
    sSupervisor->mDistSnow = this;
}

TDistSnow::~TDistSnow()
{
    sSupervisor = nullptr;
}

void TDistSnow::draw(u32)
{
    Mtx44 ortho;
    C_MTXOrtho(ortho, 0.0f, 480.0f, 0.0f, 480.0f, 0.0f, 1.0f);
    GXSetProjection(ortho, GX_ORTHOGRAPHIC);
    setupGX();
    for (int i = 0; i < 8; i++) {
        Mtx mtx;
        PSMTXTrans(mtx, mOffset.x + (f32)i, mOffset.y, 0.0f);
        GXLoadPosMtxImm(mtx, GX_PNMTX0);
        mPanels[i].draw();
    }
}

void TDistSnow::calc()
{
    for (int i = 0; i < 8; i++) {
        mPanels[i].calc();
        if (mPanels[i].mLife == 0) {
            f32 y = (i < 4) ? -50.0f : 0.0f;
            mPanels[i].mPos.set(90.0f * i, y);
            mPanels[i].mLife = mPanels[i].mMaxLife;
        }
    }
}

void TDistSnow::reset()
{
    mOffset.zero();
    for (int i = 0; i < 8; i++) {
        mPanels[i].reset();
        mPanels[i].setSize(128.0f, 128.0f);
        mPanels[i].mVel = 0.5f * (i & 1) + 0.5f;
        mPanels[i].mMaxLife = 60.0f * getGeoRnd()->get_ufloat_1() + 150.0f;
    }
}

void TDistSnow::setupGX()
{
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S8, 0);
    mTexture->load(GX_TEXMAP0);
    GXSetNumChans(0);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GXSetCullMode(GX_CULL_NONE);
}

void TDistSnow::load()
{
    ResTIMG *img = (ResTIMG *)ObjUtility::getPtrCourseArc("/Objects/fallSnow.bti");
    mTexture->storeTIMG(img, (u8)0);
}
