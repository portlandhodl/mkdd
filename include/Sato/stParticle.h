#ifndef STPARTICLE_H
#define STPARTICLE_H

#include "PowerPC_EABI_Support/Runtime/MWCPlusPlusLib.h"
#include <JSystem/JSupport/JSUList.h>
#include <JSystem/JGeometry.h>
#include <JSystem/JUtility/JUTTexture.h>
#include "Sato/stMath.h"

struct stEmitter;
struct stParticleMgr;

struct stParticle
{
    stParticle();
    ~stParticle();

    JGeometry::TVec3f mVel;       // 0x00
    JGeometry::TVec3f mPos;       // 0x0c
    JGeometry::TVec3f mOldPos;    // 0x18
    JGeometry::TVec3f mStartPos;  // 0x24
    u16 mLife;                    // 0x30
    u16 mLifeEnd;                 // 0x32
    f32 mSizeX;                   // 0x34
    f32 mSizeY;                   // 0x38
    f32 mSizeScale;               // 0x3c
    f32 mSpeed;                   // 0x40
    f32 mAirFric;                 // 0x44
    f32 mRotX;                    // 0x48
    f32 mRotSpeedX;               // 0x4c
    u8 mColor0[3];                // 0x50
    u8 mColor1[3];                // 0x53
    u8 _56[6];                    // 0x56
    JSULink<stParticle> mLink;    // 0x5c
}; // Size: 0x68

struct stParticleDynamics
{
    stParticleDynamics();

    void Remove(JSUList<stParticle> &, JSUListIterator<stParticle> &);
    void update();
    void createParticles();
    void getRndDir(JGeometry::TVec3f *);
    void doLife();
    void doAirFric();
    void doGravity();
    void doMove();
    void doInterpolation60();
    void doRotate();

    static JGeometry::TVec3f mRndZdir;
    static Quaternion mQuat;

    u16 _00;                    // 0x00
    u8 _02[2];
    int _04;                    // 0x04
    f32 _08;                    // 0x08
    JSUList<stParticle> mList;  // 0x0c
    stEmitter *mEmitter;        // 0x18
    stParticleDynamics *mFreeList; // 0x1c
    void (stParticleDynamics::*mFunc)(); // 0x20
}; // Size: 0x2c

struct stParticleDraw
{
    enum cKindTevMode
    {
        TevMode0,
        TevMode1,
        TevMode2,
        TevMode3,
        TevMode4,
        TevMode5
    };

    void GXInitDraw();
    void setBillBoard();
    void setBillBoardXZ();
    void setDirectional();
    void setLine();
    void setChild();
    void draw();
    void calcLifeColor(JSUListIterator<stParticle> &);
    void drawParticles();
    void drawEachParticle(JSUListIterator<stParticle> &);
    void drawOrder();
    void drawRev();
    void drawParentBillBoardCommon(JSUListIterator<stParticle> &, f32 (*)[4]);
    void drawParentBillBoard(JSUListIterator<stParticle> &);
    void drawParentBillBoardXZ(JSUListIterator<stParticle> &);
    void drawParentDirectional(JSUListIterator<stParticle> &);
    void drawParentLine(JSUListIterator<stParticle> &);
    void drawChild(JSUListIterator<stParticle> &);
    static void GXDrawBegin(u32);
    void setPresetTevMode(cKindTevMode);
    void dsSetTevBlendMode();
    void setMtx();

    static Mtx cTexRotMtxFirst;
    static Mtx cTexRotMtxLast;
    static Mtx *mCamMtx;

    u32 _00;                        // 0x00
    u32 _04;                        // 0x04
    u32 mBlendType;                 // 0x08
    u32 mBlendSrc;                  // 0x0c
    u32 mTevColorIn[4];             // 0x10
    stParticleDynamics *mDynamics;  // 0x20
    JUTTexture *mTexture;           // 0x24
}; // Size: 0x28

struct stParticleMgr
{
    stParticleMgr();
    void init(int);
    ~stParticleMgr() {}

    u8 _00[4];
    stParticleDynamics mDynamics; // 0x04
    stParticleDraw mDraw;         // 0x30
}; // Size: 0x58

#endif // STPARTICLE_H
