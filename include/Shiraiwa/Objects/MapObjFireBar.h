#ifndef MAPOBJFIREBAR_H
#define MAPOBJFIREBAR_H

#include "Shiraiwa/Objects/MapObjFireObj.h"

class TMapObjFireBar : public TMapObjFireObj
{
public:
    TMapObjFireBar(const CrsData::SObject &, u32 id);
    virtual ~TMapObjFireBar();

    virtual void reset();
    virtual void calc();
    virtual const char *getBmdFileName();
    virtual const char *getShadowBmdFileName();
    virtual void createColModel(J3DModelData *);
    virtual void resetFire();

    u8 mSep;    // 0x188
    u8 _189[3]; // 0x189
    s16 mAngle; // 0x18c
    u16 _18e;   // 0x18e
    f32 mSpeed; // 0x190
}; // Size: 0x194

class TMapObjKpFireBar : public TMapObjFireBar
{
public:
    TMapObjKpFireBar(const CrsData::SObject &);
    virtual ~TMapObjKpFireBar() {}

    virtual void reset();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void createColModel(J3DModelData *);
    virtual const char *getBmdFileName() { return "/Objects/KpFireBar1.bmd"; }

    static J3DAnmTransform *sKpFireBarBckAnmTrans;
    static J3DMtxCalc *sKpFireBarBckMtxCalc;
    static u8 sUseBck;
};

class TMapObjFireCircle : public TMapObjFireObj
{
public:
    TMapObjFireCircle(const CrsData::SObject &);
    virtual ~TMapObjFireCircle();

    virtual void reset();
    virtual void calc();
    virtual void resetFire();

    f32 mRadius;     // 0x188
    s16 mAngle;      // 0x18c
    s16 mAngleSpeed; // 0x18e
    f32 mSpeed;      // 0x190
    f32 mRotSpeed;   // 0x194
}; // Size: 0x198

#endif
