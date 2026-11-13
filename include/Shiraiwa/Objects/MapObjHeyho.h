#ifndef MAPOBJHEYHO_H
#define MAPOBJHEYHO_H

#include "types.h"
#include "dolphin/os.h"
#include "JSystem/JGeometry/Matrix.h"
#include "JSystem/JGeometry/Vec.h"
#include "JSystem/JKernel/JKRDisposer.h"
#include "JSystem/JSupport/JSUList.h"
#include "Kaneshige/Course/CrsData.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/AnmPlayer.h"
#include "Shiraiwa/Coord3D.h"
#include "Shiraiwa/Coord3DUtil.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjHeyho;
class TMapObjSubHeyho;

class TMapObjSubHeyho : public GeographyObj, public StateObserver {
public:
    TMapObjSubHeyho();
    virtual ~TMapObjSubHeyho() {}

    virtual void reset();
    virtual void createColModel(J3DModelData *);
    virtual const char *getBmdFileName();
    bool checkItemHitting();
    virtual void MoveExec();
    virtual void InitExec();
    virtual void calc();
    void doFunc_Skate();
    void initFunc_Skate();
    void doFunc_Spin();
    void initFunc_Spin();
    void doFunc_Recover();
    void initFunc_Recover();
    virtual const char *getShadowBmdFileName();
    virtual void update();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    void makeFrontMtx(JGeometry::TPos3f *);

    static StateObserver::StateFuncSet<TMapObjSubHeyho> sTable[3];
    static TAnmInfo sAnmInfos[1];
    static J3DAnmTexPattern *sHeyhoBtpAnm;

    J3DAnmObjMaterial mBtp;        // 158
    JGeometry::TVec3f *mpPos;      // 178
    JGeometry::TVec3f *mpVel;      // 17c
    u32 _180;                      // 180
    JGeometry::TPos3f *mpMtx;      // 184
    JGeometry::TVec3f mOffset;     // 188
    JGeometry::TPos3f mSpinMtx;    // 194
    f32 mRecoverFrame;             // 1c4
}; // Size: 0x1c8

class THeyhoSupervisor : public GeoObjSupervisor {
public:
    THeyhoSupervisor() { mList.initiate(); }

    virtual ~THeyhoSupervisor() {}
    virtual void reset() {}
    virtual void calc();

    void entry(TMapObjHeyho *obj);

    JSUList<TMapObjHeyho> mList;
};

class TMapObjHeyho : public TMapObjHioNode, StateObserver, JKRDisposer {
public:
    TMapObjHeyho(const CrsData::SObject &);
    virtual ~TMapObjHeyho();

    virtual void reset();
    virtual void calc();
    void callWalkSound();
    bool checkItemHitting();
    void checkJump(u16);
    virtual const char *getBmdFileName();
    virtual const char *getShadowBmdFileName();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void createColModel(J3DModelData *);
    virtual void MoveExec();
    virtual void InitExec();
    virtual void update();
    void changeAllState(u16);
    virtual void doKartColCallBack(int);
    void initFunc_Skate();
    void doFunc_Skate();
    void initFunc_Jump();
    void doFunc_Jump();
    void initFunc_Bomb();
    void doFunc_Bomb();
    void initFunc_BombEnd();
    void doFunc_BombEnd();
    void initFunc_Sink();
    void doFunc_Sink();
    void initFunc_Recover();
    void doFunc_Recover();
    void initFunc_Die();
    void doFunc_Die();
    void hitCheck(TMapObjHeyho *);
    virtual GeoObjSupervisor *getSupervisor() { return sSupervisor; }
#ifndef __MWERKS__
    virtual GeoObjSupervisor *getSupervisorVirtual() { return sSupervisor; }
#endif

    static StateObserver::StateFuncSet<TMapObjHeyho> sTable[7];
    static TAnmInfo sAnmInfos[1];
    static const u8 sAnmTable[7];
    static f32 sGravity;
    static f32 sAccel;
    static f32 sJumpVel;
    static f32 sJumpGrv;
    static f32 sRotSpeed;
    static f32 sFirstSinkVel;
    static f32 sSinkAccel;
    static f32 sBombVelH;
    static f32 sBombVelV;
    static f32 sSlideVelH;
    static f32 sSlideVelV;
    static f32 sBombRotSpeed;
    static f32 sBombEndRotDecel;
    static f32 sAirFriction;
    static THeyhoSupervisor *sSupervisor;
    static J3DAnmTexPattern *sHeyhoBtpAnm;

    JSULink<TMapObjHeyho> mLink;   // 170
    J3DAnmObjMaterial mBtp;        // 180
    TAnmPlayer mAnmPlayer;         // 1a0
    TPathWalk *mpPathWalk;         // 1b8
    TMapObjSubHeyho *mpSub;        // 1bc
    JGeometry::TVec3f mBasePos;    // 1c0
    JGeometry::TVec3f mImpulse;    // 1cc
    JGeometry::TPos3f mBaseMtx;    // 1d8
    JGeometry::TPos3f mExtraMtx;   // 208
    f32 mJumpOffset;               // 238
    f32 mJumpVel;                  // 23c
    f32 mSinkVel;                  // 240
    f32 mWalkSpeed;                // 244
    f32 mBombRotSpeed;             // 248
    f32 mBombVelH;                 // 24c
    f32 mBombVelV;                 // 250
}; // Size: 0x254

#endif // MAPOBJHEYHO_H
