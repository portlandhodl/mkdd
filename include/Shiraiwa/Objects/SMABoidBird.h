#ifndef SMABOIDBIRD_H
#define SMABOIDBIRD_H

#include <JSystem/JGeometry.h>

#include "Shiraiwa/BoidsManager.h"
#include "Shiraiwa/Coord3DUtil.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TSMABoidBird : public GeographyObj
{
public:
    TSMABoidBird(u32 id);
    virtual ~TSMABoidBird() {}
    virtual void reset();
    virtual void calc();
    virtual const char *getBmdFileName() = 0;

    bool moveTo(const JGeometry::TVec3f &, f32);

    static f32 sLeaderAccel;
    static f32 sLeaderReachLength2;

    bool mIsLeader; // 0x14c
}; // Size: 0x150

class TSMABoidBirdManager : public TMapObjHioNode
{
public:
    TSMABoidBirdManager(const CrsData::SObject &);
    virtual ~TSMABoidBirdManager();
    virtual void reset();
    virtual void calc();

    void moveLeader();

    static f32 sLeaderSpeed;

    TBoidsManager *mBoidsMgr; // 0x14c
    TSMABoidBird *mLeader;    // 0x150
    TPathUtil *mPathUtil;     // 0x154
    s16 mNum;                 // 0x158
}; // Size: 0x15c

class TSMAKamome : public TSMABoidBird
{
public:
    TSMAKamome();
    virtual ~TSMAKamome() {}
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual const char *getBmdFileName() { return "/Objects/kamome1.bmd"; }

    static J3DAnmTransform *sKamomeBckAnmTrans;
    static J3DMtxCalc *sKamomeBckMtxCalc;
}; // Size: 0x150

#endif
