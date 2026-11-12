#ifndef STEFCTROAD_H
#define STEFCTROAD_H

#include "Sato/stEmitter.h"

struct stEfctRoadBase : stEmitterCallBack
{
    stEfctRoadBase() {}
    virtual void init() {}
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);
};

struct stEfctRoad : stEfctRoadBase
{
    stEfctRoad();
    virtual void calc();
};

struct stDriftKartData
{
    f32 mScale;
    f32 mOffsetZ;
    JGeometry::TVec3f *mDir;
};

struct stEfctRoadDrift : stEfctRoadBase
{
    stEfctRoadDrift(s16, s16);
    virtual void init() { mFrame = 0; }
    virtual void calc();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);

    stDriftKartData *mKartData; // 0x28
    u8 mFrame;                  // 0x2c
};

struct stEfctTireSpin : stEfctRoadBase
{
    stEfctTireSpin();
    virtual void calc();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);

    static f32 sMinSideG;
};

struct stEfctSpinTurn : stEfctRoadBase
{
    stEfctSpinTurn();
    virtual void calc();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);
};

struct stEfctRoadDriftSmoke : stEfctRoadBase
{
    stEfctRoadDriftSmoke();
    virtual void init();
    virtual void calc();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);
};

struct stEfctRoadDriftChip : stEfctRoadBase
{
    stEfctRoadDriftChip();
    virtual void calc();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);
};

struct stEfctDirtCenter : stEmitterCallBack
{
    stEfctDirtCenter();
    virtual void init() {}
    virtual void calc();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);

    int mIsSnow; // 0x28
};

struct stEfctCharBase : stEmitterCallBack
{
    stEfctCharBase() {}
    virtual void init() {}
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);
};

struct stEfctCharCenter : stEfctCharBase
{
    stEfctCharCenter();
    virtual void calc();
};

struct stEfctCharWeedCenter : stEfctCharBase
{
    stEfctCharWeedCenter();
    virtual void calc();
};

#endif // STEFCTROAD_H
