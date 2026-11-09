#ifndef MAPOBJFIREBALL_H
#define MAPOBJFIREBALL_H

#include "Sato/GeographyObj.h"
#include "Sato/J3DAnmObject.h"

#include <JSystem/JGeometry.h>

class TMapObjFireBallBase : public GeographyObj
{
public:
    TMapObjFireBallBase(u32 id);
    virtual ~TMapObjFireBallBase();
    virtual void calc();
    virtual void setRMtx(const JGeometry::TPos3f &mtx) { mRotMtx.set(mtx.mMtx); }

    u8 _pad168[0x1c]; // 0x14c
    u32 _168;         // 0x168
    u32 _16c;         // 0x16c
    u8 _170;          // 0x170
    u8 mIndex;        // 0x171
    u8 _pad172[6];    // 0x172
    f32 _178;         // 0x178
    f32 _17c;         // 0x17c
    f32 _180;         // 0x180
    JGeometry::TVec3f _184; // 0x184
}; // Size: 0x190

class TMapObjFireBall : public TMapObjFireBallBase
{
public:
    TMapObjFireBall(u32 id = 0x106a) : TMapObjFireBallBase(id) {}
    virtual ~TMapObjFireBall() {}

private:
    u8 _extra[0x10]; // sizeof 0x1a0
};

class TMapObjKpFireBall : public TMapObjFireBallBase
{
public:
    TMapObjKpFireBall(u32 id = 0x1260) : TMapObjFireBallBase(id) {}
    virtual ~TMapObjKpFireBall() {}

private:
    u8 _extra[0x18]; // sizeof 0x1a8
};

class TMapObjKpFireBarBall : public TMapObjFireBallBase
{
public:
    TMapObjKpFireBarBall(u32 id = 0x1263) : TMapObjFireBallBase(id) {}
    virtual ~TMapObjKpFireBarBall() {}

private:
    u8 _extra[0x310]; // sizeof 0x4a0
};

#endif
