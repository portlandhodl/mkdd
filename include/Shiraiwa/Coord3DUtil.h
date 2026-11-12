#ifndef COORD3DUTIL_H
#define COORD3DUTIL_H

#include "JSystem/JGeometry/Vec.h"
#include "Kaneshige/Course/CrsData.h"
#include "Shiraiwa/Coord3D.h"
#include "types.h"

class CrsGround;

class TPathUtil {
public:
    TPathUtil();
    TPathUtil(const CrsData::SObject *);
    virtual ~TPathUtil() {}
    virtual void reset();
    void getDirection(JGeometry::TVec3f &);
    f32 getDistance();
    u16 getNextNode();

    const CrsData::SObject *mObj; // 0x04
    u16 mNode;                    // 0x08
    s8 mDir;                      // 0x0a
    f32 mDistance;                // 0x0c
    bool mDistanceDirty;          // 0x10
}; // Size: 0x14

class TPathUtilInterrupt : public TPathUtil {
public:
    TPathUtilInterrupt(const CrsData::SObject *obj)
        : TPathUtil(obj)
    {
        mInterruptPos.set(0.0f, 0.0f, 0.0f);
        _20 = false;
    }
    virtual ~TPathUtilInterrupt() {}
    virtual void reset();
    void getCurrentNodePos(JGeometry::TVec3f *);
    void getNextNodePos(JGeometry::TVec3f *);
    void updateNode();

    JGeometry::TVec3f mInterruptPos; // 0x14
    bool _20;                        // 0x20
}; // Size: 0x24

class TPathWalk {
public:
    TPathWalk(const CrsData::SObject *);
    virtual ~TPathWalk();
    void init(JGeometry::TVec3f *, JGeometry::TVec3f *, JGeometry::TPos3f *);
    void setParameters(f32, f32, f32, f32, f32);
    void setCheckLength(f32);
    void reset();
    void forceTurn(const JGeometry::TVec3f &);
    void forceTurn(const JGeometry::TVec3f &, const JGeometry::TVec3f &);
    void dirUpdate();
    void posUpdate();
    bool checkReachTarget();
    s16 update();

    static s16 sNearlyReachTargetLength;

    TFreeRotate *mpRotZ;           // 0x04
    TFreeRotate *mpRotY;           // 0x08
    CrsGround *mpGround;           // 0x0c
    JGeometry::TVec3f *mpPos;      // 0x10
    JGeometry::TVec3f *mpVel;      // 0x14
    JGeometry::TPos3f *mpMtx;      // 0x18
    JGeometry::TPos3f mMtxZ;       // 0x1c
    JGeometry::TPos3f mMtxY;       // 0x4c
    JGeometry::TVec3f mTarget;     // 0x7c
    TPathUtilInterrupt *mpUtil;    // 0x88
    f32 mCheckLength;              // 0x8c
    f32 _90;                       // 0x90
    bool _94;                      // 0x94
    f32 _98;                       // 0x98
    f32 _9c;                       // 0x9c
    f32 _a0;                       // 0xa0
    f32 _a4;                       // 0xa4
}; // Size: 0xa8

class TPathWalkSphere : public TPathWalk {
public:
    bool checkReachTarget();
};

class TFreeFly {
public:
    TFreeFly();
    virtual ~TFreeFly();
    virtual void reset();
    void init(JGeometry::TVec3f *, JGeometry::TVec3f *, JGeometry::TPos3f *);
    void setParameters(f32, f32, f32);
    bool checkReachTarget();
    void forceTurn(JGeometry::TVec3f &);
    void dirUpdate();
    void posUpdate();
    bool update();

    static s16 sNearlyReachTargetLength;

    TFreeRotate *mpRotate;         // 0x04
    JGeometry::TVec3f *mpPos;      // 0x08
    JGeometry::TVec3f *mpVel;      // 0x0c
    JGeometry::TPos3f *mpMtx;      // 0x10
    JGeometry::TPos3f mMtx;        // 0x14
    JGeometry::TVec3f mTarget;     // 0x44
    f32 mSpeed;                    // 0x50
    bool _54;                      // 0x54
    f32 _58;                       // 0x58
    f32 _5c;                       // 0x5c
}; // Size: 0x60

class TFreeFallDonkyRock : public TFreeFallShakeSky {
public:
    TFreeFallDonkyRock(JGeometry::TVec3f *pos, JGeometry::TVec3f *vel, JGeometry::TPos3f *mtx)
        : TFreeFallShakeSky(pos, vel, mtx) {}
    virtual ~TFreeFallDonkyRock();
    virtual void reset();
    virtual void reflect(const JGeometry::TVec3f &);
    virtual void getReflect(const JGeometry::TVec3f &, JGeometry::TVec3f *);
};

#endif // COORD3DUTIL_H
