#ifndef STMATH_H
#define STMATH_H

#include "types.h"

#include "JSystem/JGeometry.h"

inline f32 stAbs(f32 x) {
    if(x > 0.0f) {
        return x;
    }
    return -x;
}

struct stPlaneParam
{
    f32 x, y, z; // Components of the unit normal vector N.
                 // Vec and TVec3f, either as a member or as a base, change the ordering of operator=
    f32 d;       // Constant D in the plane equation N.P + D = 0.
                 // d is signed distance from origin along normal axis
};

inline JGeometry::TVec3f stPlaneNormal(const stPlaneParam& plane) {
    return JGeometry::TVec3f(plane.x, plane.y, plane.z);
}

inline f32 stDot(const stPlaneParam& plane, const JGeometry::TVec3f &vec) {
    return stPlaneNormal(plane).dot(vec);
}

int stVecNormalize(JGeometry::TVec3f &);
int stVecNormalize(JGeometry::TVec3f *);
void stClampVecMax(JGeometry::TVec3f &, f32);
void stClampVecMin(JGeometry::TVec3f &, f32);
f32 stspeedy_sqrtf(register f32 x);
f32 stLength2(f32, f32);
void stMakeDirectionMtx(JGeometry::TPos3f *, const JGeometry::TVec3f &, char);
void stMakeRMtx(Mtx, const JGeometry::TVec3f &, const JGeometry::TVec3f &, const JGeometry::TVec3f &);
void stQt2Mtx(Mtx, const Quaternion *);
void stVec2QtUpdate(Quaternion &, Quaternion &, const JGeometry::TVec3f &, const JGeometry::TVec3f &);
void stVec2QtUpdate(Quaternion &, const Vec &, const Vec &);
bool stVec2Qt(Quaternion &, const Vec &, const Vec &);
void stMtx2Qt(Quaternion *, const Mtx);
void stQtLerp(Quaternion *, const Quaternion *, const Quaternion *, f32);
f32 stQtNormalize(Quaternion *, const Quaternion *);
int stMakePlaneParam(stPlaneParam &, JGeometry::TVec3f &, const JGeometry::TVec3f &);
int stMakePlaneParam(stPlaneParam &, const JGeometry::TVec3f &, const JGeometry::TVec3f &, const JGeometry::TVec3f &);
u8 stSearchInSurface(const JGeometry::TVec3f &, const JGeometry::TVec3f &, const JGeometry::TVec3f &);
u8 stSearchInSurface(const JGeometry::TVec3f &, const stPlaneParam &);
u8 stCollideSurfaceAndSphere(const JGeometry::TVec3f &, f32, const stPlaneParam &, f32 &);
f32 stCollideLineToPlaneIn(const JGeometry::TVec3f &, const JGeometry::TVec3f &, const stPlaneParam &);
JGeometry::TVec3f stGetCollidePosFromT(const JGeometry::TVec3f &, const JGeometry::TVec3f &, f32);
f32 stGetCollideDepthFromT(const JGeometry::TVec3f &, const JGeometry::TVec3f &, f32);
void stMTXRotDeg(Mtx, char, f32);
void stMTXRotRad(Mtx, char, f32);

inline void stMakeRotMtx(Mtx m, s16 x, s16 y, s16 z)
{
    f32 cz = JMASCos(z);
    f32 sy = JMASSin(y);
    f32 sz = JMASSin(z);
    f32 cx = JMASCos(x);
    f32 cy = JMASCos(y);
    f32 sx = JMASSin(x);
    f32 cxcz = cx * cz;
    f32 cxsz = cx * sz;
    m[0][0] = cy * cz;
    m[1][0] = cy * sz;
    m[2][0] = -sy;
    f32 sxsy = sx * sy;
    f32 sxsz = sx * sz;
    m[0][1] = sxsy * cz - cxsz;
    m[1][1] = sxsy * sz + cxcz;
    m[2][1] = sx * cy;
    f32 sxcz = sx * cz;
    m[0][2] = cxcz * sy + sxsz;
    m[1][2] = cxsz * sy - sxcz;
    m[2][2] = cx * cy;
}

struct stRandom : public JMath::TRandom_fast_
{
    stRandom() : JMath::TRandom_fast_(0) {
        permission = true;
        sMgr = this;
    };

    ~stRandom() { }

    static void createAllRandom();
    static void deleteAllRandom();

    u32 getRandom() { return get(); }
    u32 getRandomMax(u32 max);
    void getArbitUnitVec(JGeometry::TVec3f &, f32, f32);
    void getArbitUnitVecSimple(JGeometry::TVec3f &, f32);
    void getArbitUnitVecXZ(JGeometry::TVec3f&, f32);
    void setSeed(u32 seed) {
        randomSeed = seed;
        JMath::TRandom_fast_::setSeed(randomSeed);
    }

    static stRandom *sRndMgr[6];
    static stRandom *sMgr; // not sure if this get's accesssed by something else, more research needed

    s16 _0x4; // sin
    s16 _0x6; // cos
    f32 _0x8; // angle?
    u8 _0xC[4]; // unknown
    u32 randomSeed;
    bool permission;
    // padding bytes
};

stRandom * stGetRnd(u32 idx);
void stSetRndPermission(u32 idx, bool perm);

#endif // !STMATH_H
