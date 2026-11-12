#ifndef MAPOBJSANDPILLAR_H
#define MAPOBJSANDPILLAR_H

#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/JGeometry.h"
#include "Inagaki/GameSoundMgr.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/Coord3DUtil.h"
#include "Shiraiwa/TKartThrower.h"

class JPABaseEmitter;

class TMapObjSandPillar : public TKartThrower, public StateObserver
{
public:
    TMapObjSandPillar(const CrsData::SObject &);
    virtual ~TMapObjSandPillar();

    virtual void MoveExec();
    virtual void InitExec();
    virtual void getKartThrowDirPow(JGeometry::TVec3f *, f32 *, int);
    virtual void getThrowDir(JGeometry::TVec3f *, s16);
    virtual s32 getThrowPow();
    virtual const char *getBmdFileName();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void createColModel(J3DModelData *);
    virtual void reset();
    virtual void calc();
    virtual void update();
    virtual void getItemThrowDirPow(JGeometry::TVec3f *, f32 *, const ItemObj &);

    void createEmitter();
    void moveEmitter();
    void initFunc_Walk();
    void doFunc_Walk();

    static StateFuncSet<TMapObjSandPillar> sTable[1];
    static f32 sItemVel;
    static f32 sItemUpVel;
    static J3DAnmTransform *sSandPillarBckAnmTrans;
    static J3DMtxCalc *sSandPillarBckMtxCalc;
    static J3DAnmTextureSRTKey *sSandPillarBtkAnm;

    J3DAnmObjMaterial mBtk;                     // 0x158
    TPathWalk *mPathWalk;                       // 0x178
    s16 mAngle;                                 // 0x17c
    JGeometry::TPos3f mWalkMtx;                 // 0x180
    JPABaseEmitter *mEmitter;                   // 0x1b0
    GameAudio::CircleSoundMgr *mCircleSoundMgr; // 0x1b4
}; // Size: 0x1b8

#endif
