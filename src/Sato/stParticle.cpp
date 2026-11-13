#include "Sato/stParticle.h"
#include "Sato/stEmitter.h"
#include "Sato/stMath.h"
#include "dolphin/mtx.h"
#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXTev.h"
#include "dolphin/gx/GXPixel.h"
#include "dolphin/gx/GXTransform.h"
#include "dolphin/gx/GXGeometry.h"
#include "dolphin/gx/GXLighting.h"
#include "dolphin/gx/GXBump.h"
#include "dolphin/gx/GXVert.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Yamamoto/kartCtrl.h"
#include "Yamamoto/kartCamera.h"
#include "macros.h"
#include <math.h>

namespace
{
f32 sParticleBoard[24] = {
    50.0f, 50.0f, 0.0f, 50.0f, -50.0f, 0.0f, -50.0f, -50.0f,
    0.0f, -50.0f, 50.0f, 0.0f, 0.0f, 50.0f, -50.0f, 0.0f,
    -50.0f, -50.0f, 0.0f, -50.0f, 50.0f, 0.0f, 50.0f, 50.0f,
};
f32 sParticleBoardTop[24] = {
    50.0f, 0.0f, 0.0f, 50.0f, -100.0f, 0.0f, -50.0f, -100.0f,
    0.0f, -50.0f, 0.0f, 0.0f, 0.0f, 0.0f, -50.0f, 0.0f,
    -100.0f, -50.0f, 0.0f, -100.0f, 50.0f, 0.0f, 0.0f, 50.0f,
};
f32 sParticleBoardBottom[24] = {
    50.0f, 100.0f, 0.0f, 50.0f, 0.0f, 0.0f, -50.0f, 0.0f,
    0.0f, -50.0f, 100.0f, 0.0f, 0.0f, 100.0f, -50.0f, 0.0f,
    0.0f, -50.0f, 0.0f, 0.0f, 50.0f, 0.0f, 100.0f, 50.0f,
};
f32 sParticleBoardXZ[12] = {
    50.0f, 0.0f, -50.0f, 50.0f, 0.0f, 50.0f, -50.0f, 0.0f,
    50.0f, -50.0f, 0.0f, -50.0f,
};
f32 sParticleTex0[16] = {
    1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
};
u8 sParticleDisplayList[32] ATTRIBUTE_ALIGN(32) = {
    0x80, 0x00, 0x04, 0x00, 0x00, 0x01, 0x01, 0x02,
    0x02, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
u8 sParticleDisplayList2[32] ATTRIBUTE_ALIGN(32) = {
    0x80, 0x00, 0x08, 0x00, 0x00, 0x01, 0x01, 0x02,
    0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06,
    0x06, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

typedef void (stParticleDraw::*DrawIterFunc)(JSUListIterator<stParticle> &);
typedef void (stParticleDraw::*DrawFunc)();
u8 sPrim[4];
u8 sEnv[4];
u8 sUseColor;
u8 sDirRotY;
f32 sParentSizeX;
f32 sParentSizeY;
u8 sParentAlpha;
u8 sDrawNum;
DrawIterFunc sFunc[2];
DrawIterFunc sParentFunc;
DrawFunc sOrderFunc;
}

JGeometry::TVec3f stParticleDynamics::mRndZdir(0.0f, 0.0f, 1.0f);
Quaternion stParticleDynamics::mQuat;

Mtx stParticleDraw::cTexRotMtxFirst;
Mtx stParticleDraw::cTexRotMtxLast;
Mtx *stParticleDraw::mCamMtx;

stParticleDynamics::stParticleDynamics()
    : _00(0), _04(-1), _08(1.0f), mFreeList(nullptr)
{
    mList.initiate();
    mFunc = &stParticleDynamics::doInterpolation60;
}

void stParticleDynamics::Remove(JSUList<stParticle> &list, JSUListIterator<stParticle> &it)
{
    JSULink<stParticle> *link = (JSULink<stParticle> *)it.mLink;
    JSULink<stParticle> *next = link;
    if (link)
        next = (JSULink<stParticle> *)link->getNext();
    mFreeList->mList.append(&((stParticle *)link->mData)->mLink);
    it.mLink = next;
}

void stParticleDynamics::update()
{
    doLife();
    doAirFric();
    doGravity();
    doMove();
    doRotate();
    createParticles();
    (this->*mFunc)();
}

static inline void stSetColor(u8 *dst, const stColorRGB &src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

void stParticleDynamics::createParticles()
{
    stEmitter *e = mEmitter;
    if (e->mFlags & 0x400)
        return;
    if (e->mFlags & 0x2000)
        return;
    if (e->mFlags & 0x200)
        return;
    if (_00 >= e->mRateStep) {
        f32 rate = e->_1c;
        f32 rnd = stGetRnd(3)->get_ufloat_1();
        JSULink<stParticle> *link;
        u8 createRate = mEmitter->mCreateRate;
        u8 num = createRate;
        if (rate <= 0.5f) {
            if (rnd > 2.0 * ((1.0f - rate) - 0.5))
                num = stGetRnd(3)->getRandomMax(createRate);
        } else {
            if (rnd < 2.0f * (1.0f - rate))
                num = stGetRnd(3)->getRandomMax(createRate);
            else
                num = 0;
        }
        stParticle *particle;
        for (u8 i = 0; i < num; i++) {
            particle = nullptr;
            link = (JSULink<stParticle> *)mFreeList->mList.getFirstLink();
            if (link) {
                mList.prepend(&link->getObject()->mLink);
                particle = link->getObject();
            }
            if (particle == nullptr)
                continue;
            particle->mSpeed = (_04 >= 0) ? KartCtrl::getKartCtrl()->getKartBody(_04)->getThunder()->getScale() : 1.0f;
            particle->mSizeScale = particle->mSpeed * _08;
            JGeometry::TVec3f dir;
            getRndDir(&dir);
            particle->mVel = dir;
            particle->mVel.scale(mEmitter->mFirstVel * particle->mSizeScale *
                                 (mEmitter->mVelRndRate * (2.0f * stGetRnd(3)->get_ufloat_1() - 1.0f) + 1.0f));
            if (mEmitter->mFlags & 0x800) {
                stGetRnd(3)->getArbitUnitVecSimple(dir, 1.0f);
                f32 len = mEmitter->_bc * (2.0f * stGetRnd(3)->get_ufloat_1() - 1.0f);
                dir.scale(len);
                particle->mPos.add(dir, mEmitter->_0c);
            } else if (mEmitter->mFlags & 0x1000) {
                stGetRnd(3)->getArbitUnitVecXZ(dir, 1.0f);
                f32 len = mEmitter->_bc * (2.0f * stGetRnd(3)->get_ufloat_1() - 1.0f);
                dir.scale(len);
                particle->mPos.add(dir, mEmitter->_0c);
            } else {
                particle->mPos = mEmitter->_0c;
            }
            particle->mLife = 0;
            particle->mLifeEnd = mEmitter->mLife * (1.0f - mEmitter->mLifeRndRate * stGetRnd(3)->get_ufloat_1());
            particle->mRotX =
                3.1415927f * (mEmitter->mRotXRnd * (2.0f * stGetRnd(3)->get_ufloat_1() - 1.0f) + mEmitter->mRotXBase);
            f32 sign = (2.0f * stGetRnd(3)->get_ufloat_1() - 1.0f >= mEmitter->mRotSpeedThresh) ? -1.0f : 1.0f;
            particle->mRotSpeedX =
                3.1415927f * sign * (mEmitter->mRotRnd * stGetRnd(3)->get_ufloat_1() + mEmitter->mRotBase);
            particle->mAirFric = mEmitter->mAirFricRnd * stGetRnd(3)->get_ufloat_1() + mEmitter->mAirFric;
            f32 size = mEmitter->mSizeRnd * (2.0f * stGetRnd(3)->get_ufloat_1() - 1.0f) + 1.0f;
            if (mEmitter->mSizeX > 0.0f)
                particle->mSizeX = mEmitter->mSizeX * size;
            if (mEmitter->mSizeY > 0.0f) {
                if (mEmitter->mFlags & 0x8000)
                    stGetRnd(3)->get();
                particle->mSizeY = mEmitter->mSizeY * size;
            }
            stSetColor(particle->mColor0, mEmitter->mColor0Start);
            stSetColor(particle->mColor1, mEmitter->mColor1Start);
            particle->mStartPos = particle->mPos;
        }
        _00 = 0;
    } else {
        _00++;
    }
}

void stParticleDynamics::getRndDir(JGeometry::TVec3f *out)
{
    if (mEmitter->mFirstVelRndAngle > 0.0f) {
        JGeometry::TVec3f vec;
        *out = mEmitter->mFirstVelDir;
        stGetRnd(3)->getArbitUnitVecSimple(vec, mEmitter->mFirstVelRndAngle);
        if (stVec2Qt(mQuat, mRndZdir, mEmitter->mFirstVelDir)) {
            Mtx mtx;
            stQt2Mtx(mtx, &mQuat);
            PSMTXMultVec(mtx, (const Vec *)&vec, (Vec *)out);
        }
    } else {
        *out = mEmitter->mFirstVelDir;
    }
}

void stParticleDynamics::doLife()
{
    JSUListIterator<stParticle> it((JSULink<stParticle> *)mList.getFirstLink());
    bool avail;
    while ((avail = !(it == nullptr))) {
        it->mLife++;
        if (it->mLifeEnd <= it->mLife) {
            stParticle *particle = it.getObject();
            if (avail)
                ++it;
            mFreeList->mList.append(&particle->mLink);
        } else {
            ++it;
        }
    }
}

void stParticleDynamics::doAirFric()
{
    JSULink<stParticle> *link = (JSULink<stParticle> *)mList.getFirstLink();
    while (link) {
        stParticle *particle = (stParticle *)link->mData;
        particle->mVel.scale(particle->mAirFric);
        link = (JSULink<stParticle> *)link->getNext();
    }
}

void stParticleDynamics::doGravity()
{
    JSULink<stParticle> *link = (JSULink<stParticle> *)mList.getFirstLink();
    while (link) {
        stParticle *particle = (stParticle *)link->mData;
        particle->mVel.y += mEmitter->mGravity;
        link = (JSULink<stParticle> *)link->getNext();
    }
}

void stParticleDynamics::doMove()
{
    for (JSUListIterator<stParticle> it((JSULink<stParticle> *)mList.getFirstLink()); it.isAvailable(); ++it) {
        it->mPos += it->mVel + mEmitter->_00;
    }
}

void stParticleDynamics::doInterpolation60()
{
    JSULink<stParticle> *link = (JSULink<stParticle> *)mList.getFirstLink();
    while (link) {
        stParticle *particle = (stParticle *)link->mData;
        particle->mOldPos = particle->mPos;
        link = (JSULink<stParticle> *)link->getNext();
    }
}

void stParticleDynamics::doRotate()
{
    for (JSUListIterator<stParticle> it((JSULink<stParticle> *)mList.getFirstLink()); it.isAvailable(); ++it) {
        it->mRotX += it->mRotSpeedX;
        if (it->mRotX >= 3.1415927f)
            it->mRotX = -6.2831855f + it->mRotX;
        else if (it->mRotX <= -3.1415927f)
            it->mRotX = 6.2831855f + it->mRotX;
    }
}

void stParticleDraw::GXInitDraw()
{
    stEmitter *emitter = mDynamics->mEmitter;
    mTexture->load(GX_TEXMAP0);
    dsSetTevBlendMode();
    u8 useColor = emitter->mFlags & 1;
    sDirRotY = 0;
    sUseColor = useColor;
    switch (mDynamics->mEmitter->mDrawType) {
    case 0:
        setBillBoard();
        break;
    case 1:
        setDirectional();
        break;
    case 2:
        setLine();
        break;
    case 3:
        setBillBoardXZ();
        break;
    }
    if (emitter->mFlags & 4)
        sOrderFunc = &stParticleDraw::drawRev;
    else
        sOrderFunc = &stParticleDraw::drawOrder;
    setChild();
}

void stParticleDraw::setBillBoard()
{
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, (GXCompCnt)1, (GXCompType)4, 0);
    void *board = nullptr;
    if (mDynamics->mEmitter->mDrawType == 3)
        board = sParticleBoardXZ;
    else
        switch (mDynamics->mEmitter->_bb) {
        case 0:
            board = sParticleBoard;
            break;
        case 1:
            board = sParticleBoardTop;
            break;
        case 2:
            board = sParticleBoardBottom;
            break;
        }
    GXSetArray(GX_VA_POS, board, 12);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, (GXCompCnt)1, (GXCompType)4, 0);
    GXSetArray(GX_VA_TEX0, sParticleTex0, 8);
    sParentFunc = &stParticleDraw::drawParentBillBoard;
}

void stParticleDraw::setBillBoardXZ()
{
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, (GXCompCnt)1, (GXCompType)4, 0);
    void *board = nullptr;
    if (mDynamics->mEmitter->mDrawType == 3)
        board = sParticleBoardXZ;
    else
        switch (mDynamics->mEmitter->_bb) {
        case 0:
            board = sParticleBoard;
            break;
        case 1:
            board = sParticleBoardTop;
            break;
        case 2:
            board = sParticleBoardBottom;
            break;
        }
    GXSetArray(GX_VA_POS, board, 12);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, (GXCompCnt)1, (GXCompType)4, 0);
    GXSetArray(GX_VA_TEX0, sParticleTex0, 8);
    sParentFunc = &stParticleDraw::drawParentBillBoardXZ;
}

void stParticleDraw::setDirectional()
{
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, (GXCompCnt)1, (GXCompType)4, 0);
    void *board = nullptr;
    if (mDynamics->mEmitter->mDrawType == 3)
        board = sParticleBoardXZ;
    else
        switch (mDynamics->mEmitter->_bb) {
        case 0:
            board = sParticleBoard;
            break;
        case 1:
            board = sParticleBoardTop;
            break;
        case 2:
            board = sParticleBoardBottom;
            break;
        }
    GXSetArray(GX_VA_POS, board, 12);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, (GXCompCnt)1, (GXCompType)4, 0);
    GXSetArray(GX_VA_TEX0, sParticleTex0, 8);
    if (mDynamics->mEmitter->mFlags & 0x20)
        sDirRotY = 1;
    sParentFunc = &stParticleDraw::drawParentDirectional;
}

void stParticleDraw::setLine()
{
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, (GXCompCnt)1, (GXCompType)4, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, (GXCompCnt)1, (GXCompType)4, 0);
    sParentFunc = &stParticleDraw::drawParentLine;
}

void stParticleDraw::setChild()
{
    stEmitter *emitter = mDynamics->mEmitter;
    if (emitter->mFlags & 2) {
        sDrawNum = 2;
        if (mDynamics->mEmitter->mFlags & 8) {
            sFunc[0] = sParentFunc;
            sFunc[1] = &stParticleDraw::drawChild;
        } else {
            sFunc[0] = &stParticleDraw::drawChild;
            sFunc[1] = sParentFunc;
        }
    } else {
        sFunc[0] = sParentFunc;
        sDrawNum = 1;
    }
}

void stParticleDraw::draw()
{
    drawParticles();
}

void stParticleDraw::calcLifeColor(JSUListIterator<stParticle> &it)
{
    stEmitter *e = mDynamics->mEmitter;
    f32 rate = 0.0f;
    if (it->mLifeEnd != 0)
        rate = (f32)it->mLife / (f32)it->mLifeEnd;
    int c0 = (f32)(e->mColor0End[0] - e->mColor0Start[0]) * rate;
    int c1 = (f32)(e->mColor0End[1] - e->mColor0Start[1]) * rate;
    int c2 = (f32)(e->mColor0End[2] - e->mColor0Start[2]) * rate;
    it->mColor0[0] = e->mColor0Start[0] + c0;
    it->mColor0[1] = e->mColor0Start[1] + c1;
    it->mColor0[2] = e->mColor0Start[2] + c2;
    c0 = (f32)(e->mColor1End[0] - e->mColor1Start[0]) * rate;
    c1 = (f32)(e->mColor1End[1] - e->mColor1Start[1]) * rate;
    c2 = (f32)(e->mColor1End[2] - e->mColor1Start[2]) * rate;
    it->mColor1[0] = e->mColor1Start[0] + c0;
    it->mColor1[1] = e->mColor1Start[1] + c1;
    it->mColor1[2] = e->mColor1Start[2] + c2;
}

void stParticleDraw::drawParticles()
{
    if (mDynamics->mList.getNumLinks() != 0)
        (this->*sOrderFunc)();
}

static inline f32 stCalcSize(JSUListIterator<stParticle> it, f32 stParticle::*size, stParticleDraw *draw,
                             f32 stEmitter::*first, f32 stEmitter::*last, f32 stEmitter::*firstRate,
                             f32 stEmitter::*lastRate)
{
    f32 val = it.getObject()->*size;
    f32 firstLife = draw->mDynamics->mEmitter->*firstRate * it->mLifeEnd;
    f32 lastLife = draw->mDynamics->mEmitter->*lastRate * it->mLifeEnd;
    if (it->mLife < firstLife)
        val *= (1.0f - draw->mDynamics->mEmitter->*first) * (it->mLife / firstLife) + draw->mDynamics->mEmitter->*first;
    else if (it->mLife > lastLife)
        val *= (1.0f - draw->mDynamics->mEmitter->*last) * ((it->mLifeEnd - it->mLife) / (it->mLifeEnd - lastLife)) + draw->mDynamics->mEmitter->*last;
    val *= it->mSizeScale;
    return val;
}

static inline f32 stCalcSizeRef(JSUListIterator<stParticle> it, f32 stParticle::*size, const f32 &first, const f32 &last,
                             const f32 &firstRate, const f32 &lastRate)
{
    f32 val = it.getObject()->*size;
    f32 firstLife = firstRate * it->mLifeEnd;
    f32 lastLife = lastRate * it->mLifeEnd;
    if (it->mLife < firstLife)
        val *= (1.0f - first) * (it->mLife / firstLife) + first;
    else if (it->mLife > lastLife)
        val *= (1.0f - last) * ((it->mLifeEnd - it->mLife) / (it->mLifeEnd - lastLife)) + last;
    val *= it->mSizeScale;
    return val;
}

static inline u8 stCalcAlpha(JSUListIterator<stParticle> it, u8 val, const f32 &first, const f32 &last,
                             const f32 &firstRate, const f32 &lastRate)
{
    f32 firstLife = firstRate * it->mLifeEnd;
    f32 lastLife = lastRate * it->mLifeEnd;
    if (it->mLife < firstLife)
        val = val * ((1.0f - first) * (it->mLife / firstLife) + first);
    else if (it->mLife > lastLife)
        val = val * ((1.0f - last) * ((it->mLifeEnd - it->mLife) / (it->mLifeEnd - lastLife)) + last);
    return val;
}

void stParticleDraw::drawEachParticle(JSUListIterator<stParticle> &it)
{
    for (u8 i = 0; i < sDrawNum; i++) {
        sParentSizeX = stCalcSize(it, &stParticle::mSizeX, this, &stEmitter::mSizeFirst, &stEmitter::mSizeLast,
                                  &stEmitter::mSizeFirstRate, &stEmitter::mSizeLastRate);
        if (mDynamics->mEmitter->mFlags & 0x8000)
            sParentSizeY = stCalcSizeRef(it, &stParticle::mSizeY, mDynamics->mEmitter->mSizeYFirst,
                                         mDynamics->mEmitter->mSizeYLast, mDynamics->mEmitter->mSizeYFirstRate,
                                         mDynamics->mEmitter->mSizeYLastRate);
        else
            sParentSizeY = stCalcSizeRef(it, &stParticle::mSizeY, mDynamics->mEmitter->mSizeFirst,
                                         mDynamics->mEmitter->mSizeLast, mDynamics->mEmitter->mSizeFirstRate,
                                         mDynamics->mEmitter->mSizeLastRate);
        sParentAlpha = stCalcAlpha(it, 255.0f * mDynamics->mEmitter->mAlphaStdRate, mDynamics->mEmitter->mAlphaFirst,
                                   mDynamics->mEmitter->mAlphaLast, mDynamics->mEmitter->mAlphaFirstRate,
                                   mDynamics->mEmitter->mAlphaLastRate);
        (this->*sFunc[i])(it);
    }
}

void stParticleDraw::drawOrder()
{
    if (mDynamics->mEmitter->mFlags & 0x4000) {
        GXSetTevColor((GXTevRegID)3, mDynamics->mEmitter->mOrderColor);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C2);
        GXSetBlendMode(GX_BM_BLEND, (GXBlendFactor)4, (GXBlendFactor)5, GX_LO_SET);
        JSUListIterator<stParticle> it((JSULink<stParticle> *)mDynamics->mList.getFirstLink());
        while (it.mLink) {
            drawEachParticle(it);
            it.mLink = (JSULink<stParticle> *)it.mLink->mNext;
        }
        GXSetTevColorIn(GX_TEVSTAGE0, (GXTevColorArg)mTevColorIn[0],
                        (GXTevColorArg)mTevColorIn[1], (GXTevColorArg)mTevColorIn[2],
                        (GXTevColorArg)mTevColorIn[3]);
        GXSetBlendMode(GX_BM_BLEND, (GXBlendFactor)mBlendType, (GXBlendFactor)mBlendSrc,
                       GX_LO_SET);
        JSUListIterator<stParticle> it2((JSULink<stParticle> *)mDynamics->mList.getFirstLink());
        while (it2.mLink) {
            drawEachParticle(it2);
            it2.mLink = (JSULink<stParticle> *)it2.mLink->mNext;
        }
    } else {
        JSUListIterator<stParticle> it3((JSULink<stParticle> *)mDynamics->mList.getFirstLink());
        while (it3.mLink) {
            drawEachParticle(it3);
            it3.mLink = (JSULink<stParticle> *)it3.mLink->mNext;
        }
    }
}

void stParticleDraw::drawRev()
{
    JSUListIterator<stParticle> it((JSULink<stParticle> *)mDynamics->mList.getLastLink());
    while (it.mLink) {
        drawEachParticle(it);
        it.mLink = (JSULink<stParticle> *)it.mLink->mPrev;
    }
}

void stParticleDraw::drawParentBillBoardCommon(JSUListIterator<stParticle> &it, Mtx out)
{
    if (sUseColor) {
        calcLifeColor(it);
        sPrim[0] = it->mColor0[0];
        sPrim[1] = it->mColor0[1];
        sPrim[2] = it->mColor0[2];
        sEnv[0] = it->mColor1[0];
        sEnv[1] = it->mColor1[1];
        sEnv[2] = it->mColor1[2];
        GXSetTevColor(GX_TEVREG1, *(GXColor *)sEnv);
    }
    sPrim[3] = sParentAlpha;
    GXSetTevColor(GX_TEVREG0, *(GXColor *)sPrim);
    Mtx trans;
    PSMTXTrans(trans, it->mOldPos.x, it->mOldPos.y, it->mOldPos.z);
    PSMTXConcat(*mCamMtx, trans, out);
}

void stParticleDraw::drawParentBillBoard(JSUListIterator<stParticle> &it)
{
    Mtx out;
    drawParentBillBoardCommon(it, out);
    stParticle *particle = (stParticle *)it.mLink->mData;
    Mtx rot;
    stMTXRotRad(rot, 'Z', particle->mRotX);
    out[0][0] = rot[0][0] * sParentSizeX;
    out[1][0] = rot[1][0] * sParentSizeX;
    out[2][0] = rot[2][0] * sParentSizeX;
    out[0][1] = rot[0][1] * sParentSizeY;
    out[1][1] = rot[1][1] * sParentSizeY;
    out[2][1] = rot[2][1] * sParentSizeY;
    out[2][2] = 0.0f;
    out[1][2] = 0.0f;
    out[0][2] = 0.0f;
    GXLoadPosMtxImm(out, 0);
    GXCallDisplayList(sParticleDisplayList, 0x20);
}

void stParticleDraw::drawParentBillBoardXZ(JSUListIterator<stParticle> &it)
{
    Mtx out;
    drawParentBillBoardCommon(it, out);
    stParticle *particle = (stParticle *)it.mLink->mData;
    Mtx rot;
    stMTXRotRad(rot, 'Y', particle->mRotX);
    rot[0][0] *= sParentSizeX;
    rot[1][0] *= sParentSizeX;
    rot[2][0] *= sParentSizeX;
    rot[0][2] *= sParentSizeY;
    rot[1][2] *= sParentSizeY;
    rot[2][2] *= sParentSizeY;
    PSMTXConcat(out, rot, out);
    GXLoadPosMtxImm(out, 0);
    GXCallDisplayList(sParticleDisplayList, 0x20);
}

namespace JGeometry {
template <class T>
void TRotation3<T>::setRotate(const TVec3f &axis, f32 angle)
{
    TVec3f vec;
    vec.normalize(axis);
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    f32 negc = 1.0f - c;
    f32 x = vec.x;
    f32 y = vec.y;
    f32 z = vec.z;
    f32 xx = x * x;
    f32 yy = y * y;
    f32 zz = z * z;
    this->ref(0, 0) = c + negc * xx;
    this->ref(0, 1) = negc * x * y - s * z;
    this->ref(0, 2) = negc * x * z + s * y;
    this->ref(1, 0) = negc * x * y + s * z;
    this->ref(1, 1) = c + negc * yy;
    this->ref(1, 2) = negc * y * z - s * x;
    this->ref(2, 0) = negc * x * z - s * y;
    this->ref(2, 1) = negc * y * z + s * x;
    this->ref(2, 2) = c + negc * zz;
}
}

void stParticleDraw::drawParentDirectional(JSUListIterator<stParticle> &it)
{
    Mtx out;
    Mtx trans;
    Mtx tmp;
    JGeometry::TRot3f rot;
    Mtx m;
    if (sUseColor) {
        calcLifeColor(it);
        sPrim[0] = it->mColor0[0];
        sPrim[1] = it->mColor0[1];
        sPrim[2] = it->mColor0[2];
        sEnv[0] = it->mColor1[0];
        sEnv[1] = it->mColor1[1];
        sEnv[2] = it->mColor1[2];
        GXSetTevColor(GX_TEVREG1, *(GXColor *)sEnv);
    }
    sPrim[3] = sParentAlpha;
    GXSetTevColor(GX_TEVREG0, *(GXColor *)sPrim);
    PSMTXTrans(trans, it->mOldPos.x, it->mOldPos.y, it->mOldPos.z);
    JGeometry::TVec3f side(1.0f, 0.0f, 0.0f);
    JGeometry::TVec3f vel(it->mVel);
    JGeometry::TVec3f up(0.0f, 0.0f, 1.0f);
    if (PSVECMag(&vel) > 0.0f) {
        vel.normalize();
        JGeometry::TVec3f yUp(0.0f, 1.0f, 0.0f);
        PSVECCrossProduct(&vel, &yUp, &side);
        if (PSVECMag(&side) > 0.0f) {
            side.normalize();
            PSVECCrossProduct(&side, &vel, &up);
            if (PSVECMag(&up) > 0.0f) {
                up.normalize();
            } else {
                side.set(1.0f, 0.0f, 0.0f);
                vel.set(0.0f, 1.0f, 0.0f);
                up.set(0.0f, 0.0f, 1.0f);
            }
        } else {
            side.set(1.0f, 0.0f, 0.0f);
            vel.set(0.0f, 1.0f, 0.0f);
        }
    } else {
        vel.set(0.0f, 1.0f, 0.0f);
    }
    if (sDirRotY) {
        rot.setRotate(vel, it->mRotX);
        rot.ref(0, 3) = 0.0f;
        rot.ref(1, 3) = 0.0f;
        rot.ref(2, 3) = 0.0f;
    } else {
        PSMTXIdentity(rot);
    }
    m[0][0] = side.x * sParentSizeX;
    m[1][0] = side.y * sParentSizeX;
    m[2][0] = side.z * sParentSizeX;
    m[0][1] = vel.x * sParentSizeY;
    m[1][1] = vel.y * sParentSizeY;
    m[2][1] = vel.z * sParentSizeY;
    m[0][2] = up.x * sParentSizeX;
    m[1][2] = up.y * sParentSizeX;
    m[2][2] = up.z * sParentSizeX;
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    PSMTXConcat(rot, m, m);
    PSMTXConcat(trans, m, tmp);
    PSMTXConcat(*mCamMtx, tmp, out);
    GXLoadPosMtxImm(out, 0);
    GXCallDisplayList(sParticleDisplayList2, 0x20);
}

void stParticleDraw::drawParentLine(JSUListIterator<stParticle> &it)
{
    if (sUseColor) {
        calcLifeColor(it);
        sPrim[0] = it->mColor0[0];
        sPrim[1] = it->mColor0[1];
        sPrim[2] = it->mColor0[2];
        sEnv[0] = it->mColor1[0];
        sEnv[1] = it->mColor1[1];
        sEnv[2] = it->mColor1[2];
        GXSetTevColor(GX_TEVREG1, *(GXColor *)sEnv);
    }
    sPrim[3] = sParentAlpha;
    GXSetTevColor(GX_TEVREG0, *(GXColor *)sPrim);
    JGeometry::TVec3f dir(it->mVel);
    if (PSVECMag(&dir) > 0.0f) {
        dir.normalize();
    } else {
        it->mLife = 0;
        return;
    }
    GXLoadPosMtxImm(*mCamMtx, 0);
    GXSetLineWidth(255.0f * sParentSizeX, GX_TO_ONE);
    f32 len = 100.0f * sParentSizeY;
    JGeometry::TVec3f pos(it->mOldPos);
    JGeometry::TVec3f end(pos);
    JGeometry::TVec3f start(pos);
    JGeometry::TVec3f ofs(dir);
    ofs.scale(len);
    start.add(ofs);
    ofs.scale(-1.0f);
    end.add(ofs);
    GXBegin(GX_LINES, GX_VTXFMT0, 2);
    GXPosition3f32(end.x, end.y, end.z);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition3f32(start.x, start.y, start.z);
    GXTexCoord2f32(0.0f, 1.0f);
    GXEnd();
}

void stParticleDraw::drawChild(JSUListIterator<stParticle> &it)
{
    Mtx out;
    Mtx trans;
    Mtx rot;
    stEmitter *e = mDynamics->mEmitter;
    f32 sizeX = sParentSizeX * e->mChildSize;
    f32 sizeY = sParentSizeY * e->mChildSize;
    if (sUseColor) {
        stParticle *p = it.getObject();
        sPrim[0] = e->_98 * p->mColor0[0];
        sPrim[1] = e->_98 * p->mColor0[1];
        sPrim[2] = e->_98 * p->mColor0[2];
        sEnv[0] = e->_98 * p->mColor1[0];
        sEnv[1] = e->_98 * p->mColor1[1];
        sEnv[2] = e->_98 * p->mColor1[2];
        GXSetTevColor(GX_TEVREG1, *(GXColor *)sEnv);
    }
    sPrim[3] = sParentAlpha * e->mChildAlpha;
    GXSetTevColor(GX_TEVREG0, *(GXColor *)sPrim);
    JGeometry::TVec3f dir(it->mVel);
    JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f side;
    if (PSVECMag(&dir) > 0.0f) {
        dir.normalize();
        PSVECCrossProduct(&up, &dir, &side);
        if (PSVECMag(&side) > 0.0f) {
            side.normalize();
        } else {
            return;
        }
        PSVECCrossProduct(&dir, &side, &up);
        up.normalize();
        side.scale(e->_9c.x);
        up.scale(e->_9c.y);
        dir.scale(e->_9c.z);
    } else {
        return;
    }
    PSMTXTrans(trans, it->mOldPos.x + dir.x + side.x + up.x, it->mOldPos.y + dir.y + side.y + up.y,
               it->mOldPos.z + dir.z + side.z + up.z);
    stMTXRotRad(rot, 'Z', it->mRotX);
    PSMTXConcat(*mCamMtx, trans, out);
    out[0][0] = rot[0][0] * sizeX;
    out[1][0] = rot[1][0] * sizeX;
    out[2][0] = rot[2][0] * sizeX;
    out[0][1] = rot[0][1] * sizeY;
    out[1][1] = rot[1][1] * sizeY;
    out[2][1] = rot[2][1] * sizeY;
    out[2][2] = 0.0f;
    out[1][2] = 0.0f;
    out[0][2] = 0.0f;
    GXLoadPosMtxImm(out, 0);
    GXCallDisplayList(sParticleDisplayList, 0x20);
}

static inline KartCam *stGetCamera(int cameraNo)
{
    ((0 <= cameraNo) && (cameraNo < 4))
        ? (void)0
        : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "RaceMgr.h", 262,
                                      "range over: %d <= cameraNo=%d < %d", 0, cameraNo, 4),
           OSPanic("RaceMgr.h", 262, "Halt"));
    return KartCtrl::getKartCtrl()->getKartCam(cameraNo);
}

void stParticleDraw::GXDrawBegin(u32 viewNo)
{
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    GXSetNumIndStages(0);
    GXSetNumChans(0);
    GXSetCurrentMtx(0);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetClipMode(GX_CLIP_ENABLE);
    GXSetZCompLoc(GX_FALSE);
    GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_OR, GX_GREATER, 0);
    GXEnableTexOffsets(GX_TEXCOORD0, GX_TRUE, GX_TRUE);
    GXSetCullMode(GX_CULL_NONE);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetAlphaUpdate(GX_FALSE);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    mCamMtx = (Mtx *)stGetCamera(viewNo)->GetMtx();
}

void stParticleDraw::setPresetTevMode(cKindTevMode mode)
{
    switch (mode) {
    case TevMode0:
        mTevColorIn[0] = 15;
        mTevColorIn[1] = 8;
        mTevColorIn[2] = 12;
        mTevColorIn[3] = 15;
        break;
    case TevMode1:
        mTevColorIn[0] = 15;
        mTevColorIn[1] = 2;
        mTevColorIn[2] = 8;
        mTevColorIn[3] = 15;
        break;
    case TevMode2:
        mTevColorIn[0] = 2;
        mTevColorIn[1] = 12;
        mTevColorIn[2] = 8;
        mTevColorIn[3] = 15;
        break;
    case TevMode3:
        mTevColorIn[0] = 4;
        mTevColorIn[1] = 2;
        mTevColorIn[2] = 8;
        mTevColorIn[3] = 15;
        break;
    case TevMode4:
        mTevColorIn[0] = 15;
        mTevColorIn[1] = 8;
        mTevColorIn[2] = 2;
        mTevColorIn[3] = 4;
        break;
    case TevMode5:
        mTevColorIn[0] = 15;
        mTevColorIn[1] = 15;
        mTevColorIn[2] = 15;
        mTevColorIn[3] = 2;
        break;
    }
}

void stParticleDraw::dsSetTevBlendMode()
{
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevColorIn(GX_TEVSTAGE0, (GXTevColorArg)mTevColorIn[0], (GXTevColorArg)mTevColorIn[1],
                    (GXTevColorArg)mTevColorIn[2], (GXTevColorArg)mTevColorIn[3]);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_A0, GX_CA_ZERO);
    GXSetBlendMode(GX_BM_BLEND, (GXBlendFactor)mBlendType, (GXBlendFactor)mBlendSrc, GX_LO_SET);
}

stParticleMgr::stParticleMgr()
{
    mDraw.setPresetTevMode(stParticleDraw::TevMode0);
    mDraw.mBlendType = 4;
    mDraw.mBlendSrc = 5;
    PSMTXTrans(stParticleDraw::cTexRotMtxFirst, -0.5f, -0.5f, 0.0f);
    PSMTXTrans(stParticleDraw::cTexRotMtxLast, 0.5f, 0.5f, 0.0f);
    mDraw.mDynamics = &mDynamics;
}

void stParticleMgr::init(int num)
{
    mDynamics._00 = 0;
    mDynamics._04 = num < 0 ? -1 : num;
    mDynamics._08 = 1.0f;

    mDraw.setPresetTevMode((stParticleDraw::cKindTevMode)mDraw.mDynamics->mEmitter->mTevMode);
    switch (mDraw.mDynamics->mEmitter->mKind) {
    case 0:
        mDraw.mBlendType = 4;
        mDraw.mBlendSrc = 5;
        break;
    case 1:
        mDraw.mBlendType = 4;
        mDraw.mBlendSrc = 1;
        break;
    case 2:
        mDraw.mBlendType = 2;
        mDraw.mBlendSrc = 0;
        break;
    }
    if (mDraw.mDynamics->mEmitter->_bb >= 3)
        mDraw.mDynamics->mEmitter->_bb = 0;
}

#include "JSystem/JAudio/JASFakeMatch2.h"

#ifndef __MWERKS__
// host build: other units call setRotate through its declaration only
template void JGeometry::TRotation3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >::setRotate(const TVec3f &, f32);
#endif
