#ifndef MAPOBJFIREBALL_H
#define MAPOBJFIREBALL_H

#include "Sato/GeographyObj.h"

class TMapObjFireBallBase : public GeographyObj
{
public:
    TMapObjFireBallBase(u32 id);
    virtual ~TMapObjFireBallBase();
    virtual void calc();
};

class TMapObjFireBall : public TMapObjFireBallBase
{
public:
    TMapObjFireBall(u32 id = 0x106a) : TMapObjFireBallBase(id) {}
    virtual ~TMapObjFireBall() {}

private:
    u8 _extra[0x54]; // sizeof 0x1a0
};

class TMapObjKpFireBall : public TMapObjFireBallBase
{
public:
    TMapObjKpFireBall(u32 id = 0x1260) : TMapObjFireBallBase(id) {}
    virtual ~TMapObjKpFireBall() {}

private:
    u8 _extra[0x5c]; // sizeof 0x1a8
};

class TMapObjKpFireBarBall : public TMapObjFireBallBase
{
public:
    TMapObjKpFireBarBall(u32 id = 0x1263) : TMapObjFireBallBase(id) {}
    virtual ~TMapObjKpFireBarBall() {}

private:
    u8 _extra[0xa8]; // sizeof 0x1f4
};

#endif
