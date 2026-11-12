#ifndef STEFCTEXHAUST_H
#define STEFCTEXHAUST_H

#include "Sato/stEmitter.h"

struct stExhaustAccelCounter
{
    stExhaustAccelCounter();
    void init();
    void calc(int);

    f32 mAccel;        // 0x00
    u32 mCount;        // 0x04
    u8 mInvalidCount;  // 0x08
    u8 mIsInvalid;     // 0x09
    u8 mTrigCount;     // 0x0a
    u8 mIsTrig;        // 0x0b
    u8 mSpdType;       // 0x0c
    u8 mIsSpdTypeSet;  // 0x0d
};

struct stEfctMuffler : stEmitterCallBack
{
    stEfctMuffler() {}
    virtual void init() {}
};

struct stEfctExhaust : stEfctMuffler
{
    stEfctExhaust(u8, u8);
    virtual void init();
    virtual void calc();
    void doAccel();
    void calcParamByKartRpm();

    s16 mLife;      // 0x28
    f32 mAirFric;   // 0x2c
    u8 mIsFirst;    // 0x30
    u8 mIsStop;     // 0x31
};

struct stEfctKinokoDashBase : stEfctMuffler
{
    stEfctKinokoDashBase() : mFrame(0), mIsActive(0) {}
    virtual void init();
    virtual void calc();
    void calcParamByFrame();

    u8 mFrame;    // 0x28
    u8 mIsActive; // 0x29
};

struct stEfctKinokoDash1 : stEfctKinokoDashBase
{
    stEfctKinokoDash1();
    virtual void init();
};

struct stEfctKinokoDash2 : stEfctKinokoDashBase
{
    stEfctKinokoDash2();
};

struct stEfctKinokoDash3 : stEfctKinokoDashBase
{
    stEfctKinokoDash3();
};

#endif // STEFCTEXHAUST_H
