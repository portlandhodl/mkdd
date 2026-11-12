#ifndef MAPOBJPOIHANA_H
#define MAPOBJPOIHANA_H

#include <dolphin/types.h>
#include <dolphin/os.h>
#include "JSystem/J3D/J3DAnmTransform.h"
#include "JSystem/J3D/J3DFrameCtrl.h"
#include "JSystem/JKernel/JKRDisposer.h"
#include "JSystem/JSupport/JSUList.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Sato/AnmController.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/AnmPlayer.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Shiraiwa/TKartThrower.h"
#include "types.h"

class TMapObjPoihana;

class TPathWalkNode
{
public:
    u8 _00[0x28];
    u8 mFlag; // 0x28
};

class TPathWalkTarget
{
public:
    u8 _00[0x14];
    JGeometry::TVec3f mPos; // 0x14
    u8 mFlag;               // 0x20
    u8 _21[3];
    u8 _24[4];
};

class TPathWalk
{
public:
    TPathWalk(const CrsData::SObject *);
    virtual ~TPathWalk();

    void init(JGeometry::TVec3f *, JGeometry::TVec3f *, JGeometry::TPos3f *);
    void setParameters(f32, f32, f32, f32, f32);
    void reset();
    void update();
    void forceTurn(const JGeometry::TVec3f &);

    TPathWalkNode *_04; // 0x04
    TPathWalkNode *_08; // 0x08
    u8 _0c[0x7c];
    TPathWalkTarget *_88; // 0x88
    u8 _8c[8];
    u8 _94; // 0x94
    u8 _95[0x13];
};

class TPoihanaSupervisor : public GeoObjSupervisor
{
public:
    TPoihanaSupervisor();
    virtual ~TPoihanaSupervisor() {}

    void entry(TMapObjPoihana *);
    virtual void thunderDownAll();
    virtual void reset() {}
    virtual void calc() {}

    JSUList<TMapObjPoihana> mList; // 0x24
};

class TMapObjPoihana : public TKartThrower, public StateObserver, public JKRDisposer
{
public:
    TMapObjPoihana(const CrsData::SObject &); // 0x8029F460

    virtual const char *getBmdFileName();             // 0x8029F864
    virtual const char *getShadowBmdFileName();       // 0x8029F88C
    virtual void loadAnimation();                     // 0x8029F8B4
    virtual void createModel(JKRSolidHeap *, u32);    // 0x8029F914
    virtual void createColModel(J3DModelData *);      // 0x8029F98C
    virtual void reset();                             // 0x8029FB58
    virtual void calc();                              // 0x8029FDF4
    void changeAllState(u16);
    void changeDownState(f32);
    void checkItemHitting();
    virtual void update();                            // 0x8029FAF0
    virtual void doKartColCallBack(int);              // 0x8029FE3C
    virtual TPoihanaSupervisor *getSupervisor() { return sSupervisor; } // 0x8029F67C
    virtual void getKartThrowDirPow(JGeometry::TVec3f *, f32 *, int); // 0x802A1D84
    virtual void getThrowDir(JGeometry::TVec3f *, s16);               // 0x802A1E10
    void thunderDown();
    virtual s32 getThrowPow() { return mThrowPow; }    // 0x802A1E08

    virtual void MoveExec();                          // 0x8029FA84
    virtual void InitExec();                          // 0x8029FA18

    bool isIgnore(int);
    void ignoreKart(int);
    void ignoreUpdate();
    void clearAllIgnore();
    void initFunc_Walk();
    void doFunc_Walk();
    void updatePosition();
    void changeSpeedForWater(f32, f32);
    int searchKart();
    void changeWalkSpeed(f32, f32);
    void initFunc_Down();
    void doFunc_Down();
    void fixWall(const JGeometry::TVec3f &, const JGeometry::TVec3f &);
    void initFunc_Buried();
    void doFunc_Buried();
    void initFunc_Wake();
    void doFunc_Wake();
    void initFunc_Throw();
    void doFunc_Throw();
    void initFunc_Run();
    void doFunc_Run();
    void callWalkSound();
    void initFunc_Rest();
    void doFunc_Rest();

    virtual ~TMapObjPoihana(); // 0x8029F714

    static TAnmInfo sAnmInfos[3];                                  // 0x803A5500
    static StateObserver::StateFuncSet<TMapObjPoihana> sTable[7];  // 0x803A55E4
    static const u8 sAnmTable[7];                                  // 0x8041CA20
    static J3DAnmTexPattern *sPoihanaBtpAnm;                       // 0x80416EA0
    static f32 sSinkOffset;                                        // 0x80416EA4
    static TPoihanaSupervisor *sSupervisor;                        // 0x80416EA8
    static s16 sDownRotFrame;                                      // 0x804151F0
    static s16 sDownFrame;                                         // 0x804151F2
    static f32 sFirstDownVel;                                      // 0x804151F4
    static f32 sFirstDownVelBanana;                                // 0x804151F8
    static f32 sDownGravity;                                       // 0x804151FC
    static s16 sWakeRotFrame;                                      // 0x80415200
    static f32 sFirstWakeVel;                                      // 0x80415204
    static f32 sWakeGravity;                                       // 0x80415208
    static f32 sWalkWaterRatio;                                    // 0x8041520C
    static f32 sDefaultSearchRadius;                               // 0x80415210
    static f32 sRunSpeedRatio;                                     // 0x80415214
    static f32 sRunRotRatio;                                       // 0x80415218
    static s16 sKartIgnoreFrame;                                   // 0x8041521C
    static s16 sWaterDecelBand;                                    // 0x8041521E
    static s16 sReSearchFrame;                                     // 0x80415220
    static f32 sRunVelScale;                                       // 0x80415224
    static f32 sWalkSpeed;                                         // 0x80415228

    JSULink<TMapObjPoihana> mLink;               // 0x170
    J3DAnmObjMaterial mBtp;         // 0x180
    TAnmPlayer mAnmPlayer;          // 0x1a0
    JGeometry::TVec3f mPathPos;     // 0x1b8
    JGeometry::TVec3f mKartPos;     // 0x1c4
    JGeometry::TVec3f mThrowDir;    // 0x1d0
    TPathWalk *mPathWalk;           // 0x1dc
    f32 mWalkSpeed;                 // 0x1e0
    f32 mWalkRotSpeed;              // 0x1e4
    f32 mWalkParam;                 // 0x1e8
    JGeometry::TPos3f mDownMtx;     // 0x1ec
    JGeometry::TPos3f mWakeMtx;     // 0x21c
    f32 mSearchRadius;              // 0x24c
    s32 mThrowPow;                  // 0x250
    f32 mHeightOffset;              // 0x254
    f32 mDownVel;                   // 0x258
    u8 mInWater;                    // 0x25c
    u8 _25d;                        // 0x25d
    s16 mDownRotFrame;              // 0x25e
    s16 mPathParam;                 // 0x260
    s16 mSearchTimer;               // 0x262
    s32 mKartNo;                    // 0x264
    u8 mIgnoreKart[8];              // 0x268
    CrsGround *mpGround;            // 0x270
}; // Size: 0x274

#endif // MAPOBJPOIHANA_H
