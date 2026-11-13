#ifndef LUMBARCTRL_H
#define LUMBARCTRL_H

#include "types.h"
#include "JSystem/JGeometry.h"
#include "Sato/NeckCtrl.h"

class CrsGround;
class DriverModel;
class ExStringNodeManager;

class LumbarCtrl : public IKCtrl {
public:
    LumbarCtrl(int, u8);
    virtual ~LumbarCtrl() {}
    void reset();
    void update(float (*)[4], DriverModel *);
    void calcKartInertialForce(JGeometry::TVec3f *, f32, f32, f32);
    void update_damage(float (*)[4], DriverModel *);
    void calcString(const JGeometry::TVec3f &);
    void calcScaleMtx(float (*)[4]);
    ExStringNodeManager *getStringNodeMgr() const { return mNodeMgr; }

    Mtx mRotMtx;                     // 0x40
    u8 m70;                          // 0x70
    s32 m74;                         // 0x74
    Mtx mScaleMtx;                   // 0x78
    JGeometry::TVec3f mScale;        // 0xa8
    CrsGround *mGround;              // 0xb4
    ExStringNodeManager *mNodeMgr;   // 0xb8
    f32 mBC;                         // 0xbc
    JGeometry::TVec3f mC0;           // 0xc0
    JGeometry::TVec3f mCC;           // 0xcc
    JGeometry::TVec3f mPrevVel;      // 0xd8
    JGeometry::TVec3f mE4;           // 0xe4
};

class FootCtrl : public IKCtrl {
public:
    enum EFootCtrlKind {
        cFootL,
        cFootR,
        cSkirt,
    };

    FootCtrl(int, u8);
    virtual ~FootCtrl() {}
    void reset();
    void update_damage(float (*)[4], const LumbarCtrl &, EFootCtrlKind);

    f32 m40; // 0x40
};

#endif
