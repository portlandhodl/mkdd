#ifndef SMABUTTERFLY_H
#define SMABUTTERFLY_H

#include <JSystem/JGeometry.h>

#include "Shiraiwa/Coord3D.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/StateObserver.h"

class TSMAButterflyGen;

class TSmallAnimal : public TMapObjHioNode
{
public:
    TSmallAnimal(u32 id);
    virtual ~TSmallAnimal();
    virtual void reset();
    virtual const char *getBmdFileName();
    virtual void setOrigin(JGeometry::TVec3f *);

    JGeometry::TVec3f *mOrigin; // 0x14c
}; // Size: 0x150

class TSmallAnimalGen : public TMapObjHioNode
{
public:
    TSmallAnimalGen(const CrsData::SObject &, u32 id);
    virtual ~TSmallAnimalGen();
    virtual void reset();
    virtual void calc();

    void makeObjects(u16);

    void **mObjects; // 0x14c
    u16 mCount;      // 0x150
    u32 mId;         // 0x154
}; // Size: 0x158

class TSMAButterfly : public TSmallAnimal, public J3DAnmObjMaterial
{
public:
    TSMAButterfly();
    virtual ~TSMAButterfly() {}

    virtual const char *getBmdFileName();
    virtual void reset();
    virtual void setOrigin(JGeometry::TVec3f *);
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void calc();
    virtual void update();
    bool checkReachTarget();
    s16 getRandomDeg(s16, s16);

    static s16 sMoveHeight;
    static s16 sMoveWidth;
    static J3DAnmTransform *sButterflyBckAnmTrans;
    static J3DMtxCalc *sButterflyBckMtxCalc;
    static J3DAnmTexPattern *sButterflyBtpAnm;

    JGeometry::TVec3f mTarget; // 0x170
}; // Size: 0x17c

class TSMAButterflyGen : public TSmallAnimalGen, public StateObserver
{
public:
    TSMAButterflyGen(const CrsData::SObject &);
    virtual ~TSMAButterflyGen();

    virtual void reset();
    virtual void calc();
    virtual void MoveExec();
    virtual void InitExec();
    void initFunc_Fly();
    void doFunc_Fly();
    void initFunc_Rest();
    void doFunc_Rest();

    static StateObserver::StateFuncSet<TSMAButterflyGen> sTable[2];
    static s16 sButterflyRestFrame;

    TPathMove *mPathMove; // 0x164
}; // Size: 0x168

#endif
