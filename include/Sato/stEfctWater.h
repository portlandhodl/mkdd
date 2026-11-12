#ifndef STEFCTWATER_H
#define STEFCTWATER_H

#include "Sato/stEmitter.h"

struct stEfctWaterBase : stEmitterCallBack
{
    stEfctWaterBase() {}
    virtual void init() {}
    virtual f32 getWaterHeight(int);

    bool getTireGeneratePos(JGeometry::TVec3f *, f32, f32);
    void setWaterVelDir();
    bool getCenterGeneratePos(JGeometry::TVec3f *, Mtx, f32);
};

struct stEfctWaterTrack : stEfctWaterBase
{
    stEfctWaterTrack();
    virtual void init() { _28 = 0; }
    virtual void calc();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);

    u8 _28;
    JGeometry::TVec3f _2c;
};

struct stEfctWaterSpray : stEfctWaterBase
{
    stEfctWaterSpray();
    virtual void init() {}
    virtual void calc();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);
};

struct stEfctWaterSprayCenter : stEfctWaterBase
{
    stEfctWaterSprayCenter();
    virtual void init() {}
    virtual void calc();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);
};

struct stEfctWaterBubble : stEfctWaterBase
{
    stEfctWaterBubble(bool);
    virtual void init() {}
    virtual void calc();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);
};

struct stEfctCrsWaterSpray : stEfctWaterSpray
{
    stEfctCrsWaterSpray();
    virtual int getGeneratePos(JGeometry::TVec3f *, Mtx);
    virtual f32 getWaterHeight(int);
};

struct stEfctCrsWaterBubble : stEfctWaterBubble
{
    stEfctCrsWaterBubble();
    virtual f32 getWaterHeight(int);
};

#endif // STEFCTWATER_H
