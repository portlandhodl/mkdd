#ifndef SHIRAIWA_MAPOBJFIREBALL_H
#define SHIRAIWA_MAPOBJFIREBALL_H

#include "JSystem/J3D/J3DAnmTevRegKey.h"
#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "Sato/GeographyObj.h"
#include "Sato/J3DAnmObject.h"

#include <JSystem/JGeometry.h>

class TMapObjFireBallBase : public GeographyObj
{
public:
    TMapObjFireBallBase(u32 id);
    virtual ~TMapObjFireBallBase();
    virtual void loadAnimation() = 0;
    virtual void createModel(JKRSolidHeap *, u32) = 0;
    virtual void reset();
    virtual void calc();
    virtual void update();
    virtual void setCurrentViewNo(u32);
    virtual const char *getBmdFileName() = 0;
    virtual void createColModel(J3DModelData *);
    virtual void doKartColCallBack(int);
    virtual void setRMtx(const JGeometry::TPos3f &mtx) { mRotMtx.set(mtx.mMtx); }

    void applyDriveState();

    J3DAnmObjMaterial mBtk;     // 0x14c
    u32 _16c;                   // 0x16c
    u8 _170;                    // 0x170
    u8 mIndex;                  // 0x171
    f32 _174;                   // 0x174
    JGeometry::TVec3f _178;     // 0x178
    JGeometry::TVec3f _184;     // 0x184
    JGeometry::TQuat4f mRot;    // 0x190
}; // Size: 0x1a0

class TMapObjFireBall : public TMapObjFireBallBase
{
public:
    TMapObjFireBall(u32 id = 0x106a) : TMapObjFireBallBase(id) {}
    virtual ~TMapObjFireBall() {}
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual const char *getBmdFileName() { return "/Objects/FireBar1Fire.bmd"; }

    static J3DAnmTextureSRTKey *sFireBallBtkAnm;
}; // Size: 0x1a0

class TMapObjKpFireBarBall : public TMapObjFireBallBase
{
public:
    TMapObjKpFireBarBall(u32 id = 0x1263) : TMapObjFireBallBase(id) {}
    virtual ~TMapObjKpFireBarBall() {}
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual void update();
    virtual const char *getBmdFileName() { return "/Objects/KpFireBar1Fire.bmd"; }
    virtual void createColModel(J3DModelData *);
    virtual void setRMtx(const JGeometry::TPos3f &mtx) { mMtx.set(mtx.mMtx); }

    static J3DAnmTextureSRTKey *sKpFireBarBallBtkAnm;
    static J3DAnmTevRegKey *sKpFireBarBallBrkAnm;

    J3DAnmObjMaterial mBrk;     // 0x1a0
    JGeometry::TPos3f mMtx;     // 0x1c0
    JPABaseEmitter *mEmitter;   // 0x1f0
}; // Size: 0x1f4

#endif
