#ifndef ACCESSORY_H
#define ACCESSORY_H

#include "types.h"

#include "dolphin/os/OSTime.h"
#include "dolphin/os/OSException.h"

#include "JSystem/J3D/J3DAnmTransform.h"
#include "JSystem/J3D/J3DFrameCtrl.h"
#include "JSystem/J3D/J3DJoint.h"
#include "JSystem/J3D/J3DMtxCalc.h"
#include "JSystem/JKernel/JKRDisposer.h"
#include "Kaneshige/ExModel.h"
#include "Osako/ResMgr.h"
#include "Sato/J3DAnmObject.h"

class DriverModel;
class JKRSolidHeap;
class J3DModelData;

class Accessory : public JKRDisposer, public ExModel {
public:
    Accessory(DriverModel *);

    void reset();
    void updateOffset();
    virtual void update(u16);
    void simpleDraw(u32);
    virtual bool createModel(JKRSolidHeap *, u32, u32);
    virtual void setCurrentViewNo(u32);

public:
    DriverModel *mDriver;           // 0xa4
    J3DAnmTransform *mAnm;          // 0xa8
    J3DFrameCtrl mFrameCtrl;        // 0xac
    J3DAnmObjMaterial mMaterialAnm; // 0xc0
};

class AccessoryMgr : public JKRDisposer {
public:
    AccessoryMgr();
    virtual ~AccessoryMgr();

    void init(DriverModel *);
    void update();
    void reset();
    void update(DriverModel *);
    void createModel(DriverModel *, JKRSolidHeap *, u32, u32);
    void setCurrentViewNo(DriverModel *, u32);
    void setCurrentViewNo(u32 viewNo);
    void simpleDraw(u32 viewNo);

    static AccessoryMgr *ptr();
    static AccessoryMgr *mspAccessoryMgr;

    static const char *mscaAccessoryBmdFileName;
    static const char *mscaAccessoryBtkFileName;

    J3DModelData *createModelData();

public:
    J3DModelData *mModelData;        // 0x18
    J3DMtxCalc *mMtxCalc;            // 0x1c
    J3DAnmTextureSRTKey *mAnm;       // 0x20
    s32 mCount;                      // 0x24
    Accessory *mAccessories[16];     // 0x28
};


#endif
