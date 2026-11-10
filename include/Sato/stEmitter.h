#ifndef STEMITTER_H
#define STEMITTER_H

#include "PowerPC_EABI_Support/Runtime/MWCPlusPlusLib.h"
#include <JSystem/JSupport/JSUList.h>
#include <JSystem/JGeometry.h>
#include "Sato/stParticle.h"

struct stEmitter
{
    stEmitter(); // 0x802359b4
    stEmitter &operator=(const stEmitter &);

    JGeometry::TVec3f _00; // 0x00
    u8 _0c[0xc];
    u8 mCreateRate; // 0x18
    u8 _19[3];
    f32 _1c;
    u8 mRateStep; // 0x20
    u8 _21;
    s16 mLife; // 0x22
    f32 mLifeRndRate; // 0x24
    JGeometry::TVec3f mFirstVelDir; // 0x28
    f32 mFirstVelRndAngle; // 0x34
    f32 mFirstVel; // 0x38
    u8 _3c[4];
    f32 mGravity; // 0x40
    f32 mAirFric; // 0x44
    u8 _48[0x18];
    f32 mSizeX; // 0x60
    f32 mSizeY; // 0x64
    u8 _68[4];
    f32 mSizeFirst; // 0x6c
    f32 mSizeLast; // 0x70
    u8 _74[8];
    f32 mAlphaStdRate; // 0x7c
    u8 _80[0x36];
    u16 mFlags;   // 0xb6
    u8 mTevMode;  // 0xb8
    u8 mKind;     // 0xb9
    u8 mDrawType; // 0xba
    u8 _bb;       // 0xbb
    u8 _bc[0x1c];
}; // Size: 0xd8

struct stEmitterParamLineIP
{
    enum cIpParam
    {
        Life,
        CreateRate,
        RateStep,
        FirstVel,
        Size,
        SizeFirst,
        SizeLast,
        LifeRndRate,
        FirstVelRndAngle,
        AlphaStdRate,
        Gravity,
        AirFric
    };

    void Init(u8 funcCnt, stEmitter *, stEmitter *);
    void AddFunc(cIpParam);
    void DoIp(f32);
    void doLife();
    void doCreateRate();
    void doRateStep();
    void doFirstVel();
    void doSize();
    void doSizeFirst();
    void doSizeLast();
    void doLifeRndRate();
    void doFirstVelRndAngle();
    void doAlphaStdRate();
    void doGravity();
    void doAirFric();

    stEmitter *_00;
    stEmitter *_04;
    stEmitter *_08;
    PTMF *funcList;
    f32 scalar;
    u8 funcCnt;
}; // Size: Unknown, probably 0x18

struct stEmitterCallBack;

struct stEmitterPtcl : public stEmitter
{
    stEmitterPtcl();
    ~stEmitterPtcl();
    // Inline/Unused
    void update();
    void removeAllParticles();

    stParticleMgr mParticle; // 0xd8
    stEmitterCallBack *_130;
    JSUPtrLink _134;
    u16 _144;
    s16 _146;
}; // Size: 0x148

struct stEmitterCallBack
{
    struct stFileParamPtrList {
        stEmitter *_00;
        stEmitter *_04;
        stEmitter *_08;
    }; // Size: 0xC

    stEmitterCallBack() {
        _04 = nullptr;
        kartNum = -1;
        _22 = 0;
        _24 = 0;
        _25 = 0;
    };
    ~stEmitterCallBack() {}

    void setIpData(stFileParamPtrList *list);
    int getTirePos(JGeometry::TVec3f *tirePos);
    int getDriftPos(JGeometry::TVec3f *driftPos, const JGeometry::TVec3f &origin);
    void calc_lineIp(f32, f32, f32, f32, f32, bool, f32);
    void calc_lineIpFollow(f32, f32, f32, f32, f32, bool, f32);
    void calcParam(f32, f32, f32, f32);
    f32 getLerpLineRate(const f32 &, const f32 &, const f32 &, const f32 &);

    virtual void init() = 0;
    virtual void calc() = 0;
    virtual void strategy(f32 ratio, f32 max, f32 min, f32 dif, bool, f32);
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);

    // Unused/Inlined
    bool chkAvailableDistance();

    // _00 Vtable
    stEmitterPtcl *_04;
    stEmitterParamLineIP ipData;
    short kartNum;
    short _22;
    u8 _24;
    u8 _25;
}; // Size: 0x28(padded)

struct stEmitterPtclMgr
{
    void calc();
    void draw(u32);
    stEmitterPtcl *Create(stEmitterCallBack *, u16);
    void RemoveEmt(JSUList<stEmitterPtcl> &, JSUListIterator<stEmitterPtcl> &);
    void ChkRemoveBecomeInvalidEmt(JSUList<stEmitterPtcl> &, JSUListIterator<stEmitterPtcl> &);
    void removeAllEmt();
    void Remove(stEmitterPtcl *);
};

#endif // STEMITTER_H
