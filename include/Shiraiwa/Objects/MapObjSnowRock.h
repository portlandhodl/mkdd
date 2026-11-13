#ifndef MAPOBJSNOWROCK_H
#define MAPOBJSNOWROCK_H

#include <JSystem/JGeometry.h>
#include <JSystem/JKernel/JKRDisposer.h>
#include <JSystem/JSupport/JSUList.h>

#include "Kaneshige/Course/CrsGround.h"
#include "Sato/GeographyObj.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjSnowRock;

class TSnowRockSupervisor : public GeoObjSupervisor
{
public:
    TSnowRockSupervisor() { mList.initiate(); }
    virtual ~TSnowRockSupervisor() {}

    void entry(TMapObjSnowRock *);
    virtual void reset() {}
    virtual void calc();

    JSUList<TMapObjSnowRock> mList; // 0x24
}; // Size: 0x30

class TMapObjSnowRock : public TMapObjHioNode, public StateObserver, public JKRDisposer
{
public:
    TMapObjSnowRock(const CrsData::SObject &);
    virtual ~TMapObjSnowRock();

    virtual void MoveExec();
    virtual void InitExec();
    virtual const char *getBmdFileName();
    virtual void createColModel(J3DModelData *);
    virtual void doKartColCallBack(int);
    virtual void reset();
    virtual void calc();
    virtual TSnowRockSupervisor *getSupervisor() { return sSupervisor; }
#ifndef __MWERKS__
    virtual GeoObjSupervisor *getSupervisorVirtual() { return sSupervisor; }
#endif

    void setAttacked(const JGeometry::TVec3f &, f32);
    void initFunc_Stand();
    void doFunc_Stand();
    void checkItemHitting();
    void initFunc_Break();
    void makeBreakEmitter();
    void doFunc_Break();
    void initFunc_Die();
    void doFunc_Die();
    void hitCheck(TMapObjSnowRock *);

    static TSnowRockSupervisor *sSupervisor;
    static f32 sGravity;
    static f32 sStartVel;
    static StateObserver::StateFuncSet<TMapObjSnowRock> sTable[3];

    JSULink<TMapObjSnowRock> mLink; // 0x170
    CrsGround *mGround;             // 0x180
    f32 mFric;                      // 0x184
}; // Size: 0x188

#endif
