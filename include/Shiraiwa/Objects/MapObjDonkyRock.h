#ifndef MAPOBJDONKYROCK_H
#define MAPOBJDONKYROCK_H

#include <JSystem/JGeometry.h>
#include <JSystem/JKernel/JKRDisposer.h>
#include <JSystem/JSupport/JSUList.h>

#include "Inagaki/GameSoundMgr.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Sato/GeographyObj.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjDonkyRock;

class TDonkyRockSupervisor : public GeoObjSupervisor {
public:
    TDonkyRockSupervisor();
    virtual ~TDonkyRockSupervisor() {}

    void entry(TMapObjDonkyRock *);
    virtual void thunderDownAll();
    virtual void reset() {}
    virtual void calc() {}

    JSUList<TMapObjDonkyRock> mList; // 0x24
};

class TMapObjDonkyRock : public TMapObjHioNode, public StateObserver, public JKRDisposer {
public:
    TMapObjDonkyRock();
    virtual ~TMapObjDonkyRock();

    virtual const char *getBmdFileName();
    virtual const char *getShadowBmdFileName();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void createColModel(J3DModelData *);
    virtual void reset();
    virtual void calc();

    virtual void InitExec();
    virtual void MoveExec();

    void start();
    void setObjData(const CrsData::SObject *);
    void initFunc_Wait();
    void doFunc_Wait();
    void setStartVel();
    void initFunc_Fall();
    void doFunc_Fall();
    void move_FallDown(JGeometry::TVec3f &);
    void getReflect(JGeometry::TVec3f &, JGeometry::TVec3f &);
    void getNodePosition(JGeometry::TVec3f *, u16);
    void initFunc_Break();
    void doFunc_Break();
    virtual void doKartColCallBack(int);
    virtual TDonkyRockSupervisor *getSupervisor() { return sSupervisor; }
#ifndef __MWERKS__
    virtual GeoObjSupervisor *getSupervisorVirtual() { return sSupervisor; }
#endif

    static TDonkyRockSupervisor *sSupervisor;
    static f32 sReflectRate;
    static f32 sFricRate;
    static f32 sGravity;
    static f32 sMaxSpeed;
    static f32 sRotSpeed;
    static f32 sKartDownScale;
    static StateObserver::StateFuncSet<TMapObjDonkyRock> sTable[3];

    JSULink<TMapObjDonkyRock> mLink; // 0x170
    f32 mHeightOffset;              // 0x180
    JGeometry::TVec3f mGravity;     // 0x184
    f32 mRollAngle;                 // 0x190
    u16 mBounceCount;               // 0x194
    u16 mNodeNum;                   // 0x196
    JGeometry::TVec3f mStartVel;    // 0x198
    GameAudio::RockSoundMgr *mSound; // 0x1a4
    const CrsData::SObject *mGenObjData; // 0x1a8
    CrsGround *mpGround;            // 0x1ac
}; // Size: 0x1b0

#endif // MAPOBJDONKYROCK_H
