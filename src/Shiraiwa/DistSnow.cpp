#include "Shiraiwa/Objects/DistSnow.h"

#include "Sato/ObjUtility.h"

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

void TDistSnow::load()
{
    ResTIMG *img = (ResTIMG *)ObjUtility::getPtrCourseArc("/Objects/fallSnow.bti");
    mTexture->storeTIMG(img, (u8)0);
}
