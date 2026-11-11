#ifndef MAPOBJFIREOBJ_H
#define MAPOBJFIREOBJ_H

#include <JSystem/JGeometry.h>

#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/StateObserver.h"

class TMapObjFireBallBase;

// Plain quaternion storage: a TQuat4f array member would make the ctor call __construct_array
struct TMapObjFireRotQuat : public Quaternion
{
    void set(f32 _x, f32 _y, f32 _z, f32 _w)
    {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
    }
    TMapObjFireRotQuat &operator=(const JGeometry::TQuat4f &q)
    {
        x = q.x;
        y = q.y;
        z = q.z;
        w = q.w;
        return *this;
    }
    operator const JGeometry::TQuat4f &() const { return *(const JGeometry::TQuat4f *)this; }
};

class TMapObjFireObj : public TMapObjHioNode, public StateObserver
{
public:
    TMapObjFireObj(const CrsData::SObject &, u32 id);
    virtual ~TMapObjFireObj();

    virtual void reset();
    virtual void calc();

    virtual void MoveExec();
    virtual void InitExec();
    virtual void resetFire();
    virtual void doKartColCallBack(int) {}

    void makeFire(u8, u32);
    void setRotate(const JGeometry::TQuat4f &q);
    bool checkDrive();

    void initFunc_Normal();
    void doFunc_Normal();
    void initFunc_Drive();
    void doFunc_Drive();
    void initFunc_Accel();
    void doFunc_Accel();
    void initFunc_Decel();
    void doFunc_Decel();

    static const StateObserver::StateFuncSet<TMapObjFireObj> sTable[4];
    static s16 sDriveTime;

    TMapObjFireBallBase **mFire; // 0x158
    u8 mFireNum;                 // 0x15c
    u8 _pad15d[3];               // 0x15d
    union {
        f32 mFireRot[8];             // 0x160
        TMapObjFireRotQuat mFireRotQ[2];
    };
    f32 mFireRotRate;            // 0x180
    u8 _pad184[4];               // 0x184
}; // Size: 0x188

#endif
