#ifndef NECKCTRL_H
#define NECKCTRL_H

#include "types.h"
#include "JSystem/JGeometry.h"
#include "JSystem/JGeometry/Matrix.h"
#include "dolphin/mtx.h"
#include "Sato/StateObserver.h"

class StringNodeManager;
class ObjColSphere;
class DriverModel;

struct NeckIKParam {
    f32 m0;          // 0x00
    f32 m4;          // 0x04
    f32 m8;          // 0x08
    f32 mC;          // 0x0c
    f32 m10;         // 0x10
    f32 m14;         // 0x14
    f32 m18;         // 0x18
    f32 m1c;         // 0x1c
    f32 m20;         // 0x20
    f32 m24;         // 0x24
    f32 m28;         // 0x28
    u8 m2c;          // 0x2c
    u8 m2d;          // 0x2d
    u8 _2e[2];
    f32 m30[5];      // 0x30
    f32 m44;         // 0x44
    f32 m48;         // 0x48
    f32 m4c;         // 0x4c
    f32 m50;         // 0x50
    f32 m54;         // 0x54
    u8 m58;          // 0x58
    u8 _59[3];
    f32 m5c;         // 0x5c
    f32 m60;         // 0x60
};
class LumbarCtrl;
class CharIKParamMgr;

class IKCtrl {
public:
    IKCtrl(int, u8);
    virtual ~IKCtrl() {}
    static void calcJointScaleMtx(JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >*, float (*)[4]);

    f32 mMtx[3][4];   // 0x04
    int mKartNo;      // 0x34
    u8 mDriverNo;     // 0x38
    u8 _39[3];
    NeckIKParam* mParam; // 0x3c
};

class CharIKParamMgr {
public:
    CharIKParamMgr();
    static void createMgr() { mMgr = new CharIKParamMgr(); }
    void load();
    const char* getFileName(u32);
    void setParam(u8);
    static CharIKParamMgr* mMgr;

    void** mParams; // 0x0
};

class HairCtrl : public IKCtrl {
public:
    HairCtrl(int, u8);
    virtual ~HairCtrl() {}
    virtual void reset();
    virtual void update_hair1(float (*)[4], float (*)[4], DriverModel*);
    virtual void update_hair2(float (*)[4], DriverModel*);
    virtual void calcHairString(float (*)[4], const JGeometry::TVec3<f32>&) = 0;
    virtual void calcHairMtx(u32, DriverModel*);
    void makeFaceNormalizeMtx(float (*)[4], JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >&, JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >&);
    void lockString(float (*)[4], const JGeometry::TVec3<f32>&, f32);
    void lockStringMin(float (*)[4], const JGeometry::TVec3<f32>&, f32, f32);
    void update_hair1_common(float (*)[4], DriverModel*);
    void initString(const JGeometry::TVec3<f32>&, f32);
    void calcHairGravity(f32, DriverModel*, float (*)[4]);

    StringNodeManager* mNodeMgr; // 0x40
    u8 m44;                      // 0x44
    u8 _45[3];
};

class PeachHairCtrl : public HairCtrl {
public:
    PeachHairCtrl(int, u8);
    virtual ~PeachHairCtrl() {}
    virtual void update_hair1(float (*)[4], float (*)[4], DriverModel*);
    virtual void update_hair2(float (*)[4], DriverModel*);
    virtual void calcHairMtx(u32, DriverModel*);
    virtual void calcHairString(float (*)[4], const JGeometry::TVec3<f32>&);
};

class DaisyHairCtrl : public HairCtrl {
public:
    DaisyHairCtrl(int, u8);
    virtual ~DaisyHairCtrl() {}
    virtual void update_hair1(float (*)[4], float (*)[4], DriverModel*);
    virtual void update_hair2(float (*)[4], DriverModel*);
    virtual void calcHairString(float (*)[4], const JGeometry::TVec3<f32>&);
};

class KinopicoHairCtrl : public HairCtrl {
public:
    enum KHairLR { };
    KinopicoHairCtrl(int, u8, KHairLR);
    virtual ~KinopicoHairCtrl() {}
    virtual void reset();
    virtual void update_hair1(float (*)[4], float (*)[4], DriverModel*);
    virtual void update_hair2(float (*)[4], DriverModel*);
    virtual void calcHairMtx(u32, DriverModel*);
    void calcJointMtx(u32, JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >&, DriverModel*);
    virtual void calcHairString(float (*)[4], const JGeometry::TVec3<f32>&);
    void offsetMtxPos(u32);
    void lockStringKp(float (*)[4], const JGeometry::TVec3<f32>&);
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > mMtx48; // 0x48
    KHairLR mLR; // 0x78
    ObjColSphere* mHeadCol; // 0x7c
    JGeometry::TVec3f mHeadPos; // 0x80
};

class DonkeyTieCtrl : public HairCtrl {
public:
    DonkeyTieCtrl(int, u8);
    virtual ~DonkeyTieCtrl() {}
    virtual void update_hair1(float (*)[4], float (*)[4], DriverModel*);
    virtual void calcHairString(float (*)[4], const JGeometry::TVec3<f32>&);
    void lockStringTieMin(float (*)[4], const JGeometry::TVec3<f32>&, f32, bool);
};

class NeckCtrl;

class NeckCtrl : public IKCtrl, public TStateObserver<NeckCtrl> {
public:
    NeckCtrl(int, u8, bool);
    virtual ~NeckCtrl() {}
    void reset();
    void calcFrontDirQt();
    void initFunc_Search();
    void doFunc_Search();
    void initFunc_Follow();
    void doFunc_Follow();
    void initFunc_Look();
    void doFunc_Look();
    void initFunc_Return();
    void doFunc_Return();
    void initFunc_LookShine();
    void doFunc_LookShine();
    int searchRivalKart();
    void checkMaxAngle(float (*)[4]);
    void update(float (*)[4], float (*)[4], LumbarCtrl*, u32);
    void init();
    void startHitClrCount(int);
    void chkHitClrCount();
    void calcTargetPosDirQt(const JGeometry::TVec3<f32>&);
    void calcTargetQt(float (*)[4]);
    void checkLerpRatio(u32);

    Quaternion mQuatPrev; // 0x58
    Quaternion mQuat;   // 0x68
    Quaternion mQuat78; // 0x78
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > mMtx88; // 0x88
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > mMtxB8; // 0xb8
    void* mPtrE8;       // 0xe8
    u8 mEC;             // 0xec
    u8 mED;             // 0xed
    u8 _ee[2];
    f32 mF0;            // 0xf0
    f32 mF4;            // 0xf4
    f32 mF8;            // 0xf8
    s32 mFC;            // 0xfc
    u8 _100[4];
    u32 m104;           // 0x104
    u8* mPtr108;        // 0x108
    u8 m10c;            // 0x10c
    u8 m10d;            // 0x10d
    u8 _10e[6];
    u8 m114;            // 0x114
    u8 m115;            // 0x115
    u8 m116;            // 0x116
    u8 _117;

    static Mtx sRotFrontDirMtx;
};


inline void CreateCharIKParamMgr() { CharIKParamMgr::createMgr(); }

#endif
