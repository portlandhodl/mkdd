#ifndef KARTSUS_H
#define KARTSUS_H

#include <JSystem/JGeometry.h>
#include "Kaneshige/KartLoader.h"
#include "Kaneshige/Course/CrsGround.h"

class KartBody;
class Spline;

class KartSus
{
public:
    KartSus() { }

    void InitTirePose(int);
    void InitSettingParam(int);
    void InitParam(int);
    void Init(int);
    void NormalInit(int);
    void ResetInit(int);
    void CircleFriction(JGeometry::TVec3f *, f32);
    f32 GetCornerForce();
    f32 GetCircleFric();
    void DoSusAction(int); // amogus
    void DoTireAction();
    // Inlines
    KartBody *getKartBody() { return mBody; }

    // Unused
    void InitTestTirePose(int);
    void WallReflection(JGeometry::TVec3<float> *, JGeometry::TVec3<float> *, float, float);

    // TODO
    ExModel *mWheel;
    ExModel *mArm;
    ExModel *mShock;
    KartBody *mBody; // 0xc
    KartLoader *mLoader;
    CrsGround mCrsGnd;
    Spline *mSpline; // 0x74
    JGeometry::TVec3f _78[4];
    f32 _a8;
    f32 _ac;
    f32 _b0;
    f32 _b4;
    f32 _b8;
    f32 _bc;
    JGeometry::TVec3f _c0;
    JGeometry::TVec3f _cc;
    JGeometry::TVec3f _d8;
    JGeometry::TVec3f _e4;
    JGeometry::TVec3f _f0;
    f32 mTireRadius;
    f32 _100;
    f32 _104;
    f32 _108;
    f32 _10c;
    f32 _110;
    f32 _114;
    f32 _118;
    f32 _11c;
    f32 _120;
    u32 _124;
    Mtx _128;
    Mtx _158;
    Mtx _188;
    Mtx _1b8;
    Mtx _1e8;
    Mtx _218;
    Mtx _248;
    f32 _278;
    f32 _27c;
    f32 _280;
    JGeometry::TVec3f _284;
    f32 _290;
    f32 _294;
    f32 _298;
    f32 _29c;
    f32 _2a0;
    u8 _2a4;
    f32 _2a8;
    f32 _2ac;
    f32 _2b0;
    f32 _2b4;
    f32 _2b8;
    f32 _2bc;
    f32 _2c0;
    f32 _2c4;
    int _2c8;
}; // _2cc

#endif
