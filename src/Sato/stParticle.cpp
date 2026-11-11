#include "Sato/stParticle.h"
#include "Sato/stEmitter.h"
#include "Sato/stMath.h"
#include "dolphin/mtx.h"
#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXTev.h"
#include "dolphin/gx/GXPixel.h"
#include "dolphin/gx/GXTransform.h"
#include "dolphin/gx/GXGeometry.h"

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
u8 sParticleDisplayList[32] = {
    0x80, 0x00, 0x04, 0x00, 0x00, 0x01, 0x01, 0x02,
    0x02, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
u8 sParticleDisplayList2[32] = {
    0x80, 0x00, 0x08, 0x00, 0x00, 0x01, 0x01, 0x02,
    0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06,
    0x06, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

typedef void (stParticleDraw::*DrawIterFunc)(JSUListIterator<stParticle> &);
typedef void (stParticleDraw::*DrawFunc)();
void (stParticleDynamics::*sNullFunc)() = &stParticleDynamics::doInterpolation60;
DrawFunc sDrawRev = &stParticleDraw::drawRev;
DrawFunc sDrawOrder = &stParticleDraw::drawOrder;
DrawIterFunc sDrawParentBillBoard = &stParticleDraw::drawParentBillBoard;
DrawIterFunc sDrawParentBillBoardXZ = &stParticleDraw::drawParentBillBoardXZ;
DrawIterFunc sDrawParentDirectional = &stParticleDraw::drawParentDirectional;
DrawIterFunc sDrawParentLine = &stParticleDraw::drawParentLine;
DrawIterFunc sChildFunc = &stParticleDraw::drawChild;

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
}

JGeometry::TVec3f stParticleDynamics::mRndZdir;
Quaternion stParticleDynamics::mQuat;

Mtx stParticleDraw::cTexRotMtxFirst;
Mtx stParticleDraw::cTexRotMtxLast;
Mtx *stParticleDraw::mCamMtx;
void (stParticleDraw::*stParticleDraw::sOrderFunc)();

stParticleDynamics::stParticleDynamics()
    : _00(0), _04(-1), _08(0.0f), mFreeList(nullptr)
{
    mList.initiate();
    mFunc = nullptr;
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
    JSULink<stParticle> *link = (JSULink<stParticle> *)mList.getFirstLink();
    while (link != nullptr) {
        stParticle *particle = link->getObject();
        particle->mLife++;
        particle = link->getObject();
        if (particle->mLifeEnd > particle->mLife) {
            link = (JSULink<stParticle> *)link->getNext();
            continue;
        }
        if ((u32)this & 0xff)
            link = (JSULink<stParticle> *)link->getNext();
        mFreeList->mList.append(&particle->mLink);
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
    JSUListIterator<stParticle> it((JSULink<stParticle> *)mList.getFirstLink());
    while (it.isAvailable()) {
        stParticle *particle = it.getObject();
        JGeometry::TVec3f vel = particle->mVel;
        particle->mVel += mEmitter->_00;
        particle->mPos += vel;
        it.goNext();
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
    for (JSULink<stParticle> *link = (JSULink<stParticle> *)mList.getFirstLink(); link != nullptr;
         link = (JSULink<stParticle> *)link->getNext()) {
        stParticle *particle = link->getObject();
        particle->mRotX += particle->mRotSpeedX;
        particle = link->getObject();
        if (particle->mRotX >= 3.1415927f)
            particle->mRotX = -6.2831855f + particle->mRotX;
        else if (particle->mRotX <= -3.1415927f)
            particle->mRotX = 6.2831855f + particle->mRotX;
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
        sOrderFunc = sDrawRev;
    else
        sOrderFunc = sDrawOrder;
    setChild();
}

void stParticleDraw::setBillBoard()
{
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, (GXCompCnt)1, (GXCompType)4, 0);
    void *board = nullptr;
    if (mDynamics->mEmitter->mDrawType == 3)
        board = &sParticleBoard[72];
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
    sParentFunc = *(DrawIterFunc *)((u8 *)sParticleBoard + 0x204);
}

void stParticleDraw::setBillBoardXZ()
{
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, (GXCompCnt)1, (GXCompType)4, 0);
    void *board = nullptr;
    if (mDynamics->mEmitter->mDrawType == 3)
        board = &sParticleBoard[72];
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
    sParentFunc = *(DrawIterFunc *)((u8 *)sParticleBoard + 0x210);
}

void stParticleDraw::setDirectional()
{
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, (GXCompCnt)1, (GXCompType)4, 0);
    void *board = nullptr;
    if (mDynamics->mEmitter->mDrawType == 3)
        board = &sParticleBoard[72];
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
    if (mDynamics->mEmitter->mFlags & 0x40)
        sDirRotY = 1;
    sParentFunc = *(DrawIterFunc *)((u8 *)sParticleBoard + 0x21c);
}

void stParticleDraw::setLine()
{
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, (GXCompCnt)1, (GXCompType)4, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, (GXCompCnt)1, (GXCompType)4, 0);
    sParentFunc = sDrawParentLine;
}

void stParticleDraw::setChild()
{
    stEmitter *emitter = mDynamics->mEmitter;
    if (emitter->mFlags & 2) {
        sDrawNum = 2;
        if (mDynamics->mEmitter->mFlags & 8) {
            sFunc[0] = sParentFunc;
            sFunc[1] = sChildFunc;
        } else {
            sFunc[0] = sParentFunc;
            sFunc[1] = sChildFunc;
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

void stParticleDraw::drawParticles()
{
    if (mDynamics->mList.getNumLinks() != 0)
        (this->*sOrderFunc)();
}

void stParticleDraw::drawRev()
{
    JSUListIterator<stParticle> it((JSULink<stParticle> *)mDynamics->mList.getLastLink());
    while (it.mLink) {
        drawEachParticle(it);
        it.mLink = (JSULink<stParticle> *)it.mLink->mPrev;
    }
}

static inline void stSetParentColor(JSUListIterator<stParticle> &it, stEmitter *e)
{
    if (!sUseColor)
        return;
    stParticle *p = it.getObject();
    f32 rate = 0.0f;
    if (it.getObject()->mLifeEnd != 0)
        rate = (f32)it.getObject()->mLife / (f32)it.getObject()->mLifeEnd;
    p->mColor0[0] = e->mColor0Start[0] +
        (int)((f32)(e->mColor0End[0] - e->mColor0Start[0]) * rate);
    it.getObject()->mColor0[1] = e->mColor0Start[1] +
        (int)((f32)(e->mColor0End[1] - e->mColor0Start[1]) * rate);
    it.getObject()->mColor0[2] = e->mColor0Start[2] +
        (int)((f32)(e->mColor0End[2] - e->mColor0Start[2]) * rate);
    it.getObject()->mColor1[0] = e->mColor1Start[0] +
        (int)((f32)(e->mColor1End[0] - e->mColor1Start[0]) * rate);
    it.getObject()->mColor1[1] = e->mColor1Start[1] +
        (int)((f32)(e->mColor1End[1] - e->mColor1Start[1]) * rate);
    it.getObject()->mColor1[2] = e->mColor1Start[2] +
        (int)((f32)(e->mColor1End[2] - e->mColor1Start[2]) * rate);
    sPrim[0] = it.getObject()->mColor0[0];
    sPrim[1] = it.getObject()->mColor0[1];
    sPrim[2] = it.getObject()->mColor0[2];
    sEnv[0] = it.getObject()->mColor1[0];
    sEnv[1] = it.getObject()->mColor1[1];
    sEnv[2] = it.getObject()->mColor1[2];
    GXSetTevColor(GX_TEVREG2, *(GXColor *)sEnv);
    sPrim[3] = sParentAlpha;
    GXSetTevColor(GX_TEVREG1, *(GXColor *)sPrim);
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

void stParticleDraw::drawParentBillBoardCommon(JSUListIterator<stParticle> &it, Mtx out)
{
    if (sUseColor) {
        stEmitter *e = mDynamics->mEmitter;
        f32 rate = 0.0f;
        if (it.getObject()->mLifeEnd != 0)
            rate = (f32)it.getObject()->mLife / (f32)it.getObject()->mLifeEnd;
        it.getObject()->mColor0[0] = e->mColor0Start[0] +
            (int)((f32)(e->mColor0End[0] - e->mColor0Start[0]) * rate);
        it.getObject()->mColor0[1] = e->mColor0Start[1] +
            (int)((f32)(e->mColor0End[1] - e->mColor0Start[1]) * rate);
        it.getObject()->mColor0[2] = e->mColor0Start[2] +
            (int)((f32)(e->mColor0End[2] - e->mColor0Start[2]) * rate);
        it.getObject()->mColor1[0] = e->mColor1Start[0] +
            (int)((f32)(e->mColor1End[0] - e->mColor1Start[0]) * rate);
        it.getObject()->mColor1[1] = e->mColor1Start[1] +
            (int)((f32)(e->mColor1End[1] - e->mColor1Start[1]) * rate);
        it.getObject()->mColor1[2] = e->mColor1Start[2] +
            (int)((f32)(e->mColor1End[2] - e->mColor1Start[2]) * rate);
        sPrim[0] = it.getObject()->mColor0[0];
        sPrim[1] = it.getObject()->mColor0[1];
        sPrim[2] = it.getObject()->mColor0[2];
        sEnv[0] = it.getObject()->mColor1[0];
        sEnv[1] = it.getObject()->mColor1[1];
        sEnv[2] = it.getObject()->mColor1[2];
        GXSetTevColor(GX_TEVREG2, *(GXColor *)sEnv);
        sPrim[3] = sParentAlpha;
        GXSetTevColor(GX_TEVREG1, *(GXColor *)sPrim);
    }
    Mtx trans;
    PSMTXTrans(trans, it.getObject()->mPos.x, it.getObject()->mPos.y, it.getObject()->mPos.z);
    PSMTXConcat(*mCamMtx, trans, out);
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
    mDynamics._08 = 0.0f;

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
