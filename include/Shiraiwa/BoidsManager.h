#ifndef BOIDSMANAGER_H
#define BOIDSMANAGER_H

#include "JSystem/JGeometry.h"
#include <dolphin/mtx.h>
#include "types.h"

struct TBoidParam {
    f32 _0;
    f32 _4;
    f32 _8;
    f32 _c;
    f32 _10;
    f32 _14;
    f32 _18;
};

class TBoid {
public:
    TBoid(f32 c, f32 maxSpeed) {
        _c = c;
        mMaxSpeed = maxSpeed;
    }
    virtual ~TBoid() {}

    void update(const JGeometry::TVec3<f32> &, f32);

    f32 _4;                          // 0x04
    f32 mMaxSpeed;                   // 0x08
    f32 _c;                          // 0x0c
    JGeometry::TVec3<f32> *mPos;     // 0x10
    JGeometry::TVec3<f32> *mVel;     // 0x14
    MtxPtr mMat;                       // 0x18

    static f32 sUpPow;
};

class TBoidsManager {
public:
    TBoidsManager(u32, u8);
    virtual ~TBoidsManager();

    void reset(const JGeometry::TVec3<f32> &);
    void update();
    void getCenter();
    void getGlbDir();
    void getPow(u32, JGeometry::TVec3<f32> *);

    static TBoidParam sParams[3];

    TBoidParam *mParam;              // 0x04
    TBoid **mBoids;                  // 0x08
    u32 mNum;                        // 0x0c
    JGeometry::TVec3<f32> mCenter;   // 0x10
    JGeometry::TVec3<f32> mGlbDir;   // 0x1c
};

#endif // BOIDSMANAGER_H
