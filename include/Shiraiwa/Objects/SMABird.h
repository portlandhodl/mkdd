#ifndef SMABIRD_H
#define SMABIRD_H

#include <JSystem/JGeometry.h>

#include "Shiraiwa/Coord3DUtil.h"
#include "Shiraiwa/Objects/SMAButterfly.h"

class TSMABird;

class TSMABirdGroup
{
public:
    TSMABirdGroup(u8, const CrsData::SObject *);
    virtual ~TSMABirdGroup();
    virtual void makeOffsetTable(f32);
    virtual void getOffset(s16, JGeometry::TVec3f &);

    void reset();
    void attachBird(TSMABird *);
    void getTargetPos(s16, JGeometry::TVec3f &);
    void checkAllPass();
    void replaceMatrix();

    s16 mBirdNum;                      // 0x04
    s16 mMaxBirdNum;                   // 0x06
    u16 _8;                            // 0x08
    u16 mAllMask;                      // 0x0a
    u16 mPassMask;                     // 0x0c
    TPathUtil *mPathUtil;              // 0x10
    JGeometry::TPos3f mMtx;            // 0x14
    JGeometry::TVec2<s16> mOffset[16]; // 0x44
}; // Size: 0x84

class TSMABird : public TSmallAnimal
{
public:
    TSMABird(u32 id);
    virtual ~TSMABird();
    virtual void reset();
    virtual void calc();
    virtual const char *getBmdFileName() = 0;

    f32 getOffset();

    static f32 sBirdDefaultSpeed;
    static f32 sBirdFlyAmp;
    static int sBirdFlyCycle;

    u8 mGroupIdx;              // 0x150
    TFreeFly *mFreeFly;        // 0x154
    TSMABirdGroup *mGroup;     // 0x158
    JGeometry::TVec3f mBasePos; // 0x15c
    s16 mFlyPhase;             // 0x168
    s16 mFlyPhaseSpeed;        // 0x16a
    JGeometry::TPos3f mFlyMtx; // 0x16c
    f32 mSpeedRate;            // 0x19c
}; // Size: 0x1a0

class TSMADonkyBird : public TSMABird
{
public:
    TSMADonkyBird();
    virtual ~TSMADonkyBird() {}
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual const char *getBmdFileName();

    static J3DAnmTransform *sDonkyBirdBckAnmTrans;
    static J3DMtxCalc *sDonkyBirdBckMtxCalc;
}; // Size: 0x1a0

class TSMABirdGen : public TSmallAnimalGen
{
public:
    TSMABirdGen(const CrsData::SObject &, u32);
    virtual ~TSMABirdGen();
    virtual void reset();
    virtual void makeBird() {}

    s16 mGroupNum;            // 0x158
    TSMABirdGroup **mGroups;  // 0x15c
}; // Size: 0x160

class TSMADonkyBirdGen : public TSMABirdGen
{
public:
    TSMADonkyBirdGen(const CrsData::SObject &);
    virtual ~TSMADonkyBirdGen() {}
    virtual void makeBird();
}; // Size: 0x160

#endif
