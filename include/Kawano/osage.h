#ifndef OSAGE_H
#define OSAGE_H

#include <JSystem/J3D/J3DJoint.h>
#include "JSystem/JKernel/JKRHeap.h"

#include "Kawano/driver.h"
#include "Kawano/driverObj.h"
#include "Sato/NeckCtrl.h"

typedef bool (*HairCallBack)(J3DJoint *joint, int);

class Osage : public ExModel {
public:
    Osage(DriverModel *);
    void update(u16);
    virtual bool createModel(JKRSolidHeap *, u32, u32);
    virtual void setCurrentViewNo(u32);
    void updateOffset();
    static bool kHairRootCallBack(J3DJoint *, int);
    static bool kHairL1CallBack(J3DJoint *, int);
    static bool kHairL2CallBack(J3DJoint *, int);
    static bool kHairL3CallBack(J3DJoint *, int);
    static bool kHairR1CallBack(J3DJoint *, int);
    static bool kHairR2CallBack(J3DJoint *, int);
    static bool kHairR3CallBack(J3DJoint *, int);

    static DriverModel *sCurDriver;
    static KinopicoHairCtrl *sCurHair_L;
    static KinopicoHairCtrl *sCurHair_R;
    static Osage *sCurOsage;

    DriverModel *mDriver;      // 0x8c
    KinopicoHairCtrl *mHair_L; // 0x90
    KinopicoHairCtrl *mHair_R; // 0x94
};

class OsageMgr : public JKRDisposer, public DriverObj {
public:
    OsageMgr();
    virtual ~OsageMgr();

    void init(DriverModel *);
    void setCallBack(int);
    void createModel(DriverModel *, JKRSolidHeap *, u32, u32);
    void setCurrentViewNo(DriverModel *, u32);
    bool CheckNoIK();
    Osage *IsOsageExist(DriverModel *);

    static OsageMgr *ptr();

    void reset();
    void update();
    void update(DriverModel *);

    static const char *mscaaJntName[7];
    static HairCallBack mscaCallBackTbl[7];
    static const char *mscaOsageBmdFileName;

    static u32 mNoIKFlag;
    static OsageMgr *mspOsageMgr;

    J3DModelData *mModelData[16]; // 0x30
    s32 mCount;                   // 0x70
    Osage *mOsage[16];            // 0x74
    u16 mJntNo[7];                // 0xb4
};

#endif // OSAGE_H
