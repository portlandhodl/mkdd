#ifndef FEELING_H
#define FEELING_H

#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JKernel/JKRDisposer.h"

#include "JSystem/J3D/J3DAnmBase.h"
#include "JSystem/J3D/J3DAnmLoader.h"
#include "Sato/J3DAnmObject.h"
#include "JSystem/J3D/J3DAnmTransform.h"
#include "JSystem/J3D/J3DFrameCtrl.h"
#include "JSystem/J3D/J3DGXColor.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DModelLoader.h"
#include "JSystem/J3D/J3DMtxCalc.h"
#include "JSystem/J3D/J3DTypes.h"
#include "JSystem/JUtility/JUTAssert.h"

#include "Kawano/driver.h"
#include "Osako/ResMgr.h"

#include "types.h"

class FeelingOne;

class Feeling : public JKRDisposer {
public:
    Feeling(DriverModel *);
    virtual ~Feeling() {}
    void update(int);
    f32 getBtpFrame();
    // Inline/Unused
    void reset();
    void updateOffset(int);
    void updateState();
    void createModel(JKRSolidHeap *, u32, u32);
    void setCurrentViewNo(u32);

    DriverModel *mDriver;    // 0x18
    FeelingOne *mOne[2];     // 0x1c
    s32 mState;              // 0x24
    s32 m_28;                // 0x28
    s32 m_2c;                // 0x2c
    J3DFrameCtrl mFrameCtrl; // 0x30

    static const char *mscpaFeelingAnmFileName[2];
}; // Size: 0x44

class FeelingMgr : public JKRDisposer {
public:
    FeelingMgr();
    virtual ~FeelingMgr();
    static FeelingMgr *ptr();
    J3DAnmTevRegKey *getBrkFile(int);
    J3DAnmTexPattern *getBtpFile();
    void init(DriverModel *);
    J3DModelData *createModelData(int);
    void update();
    void createModel(DriverModel *, JKRSolidHeap *, u32, u32);
    void setCurrentViewNo(u32);
    J3DModelData *getJ3DModelDataPtr(int);

    Feeling *mFeelings[16];              // 0x18
    s32 mCount;                          // 0x58
    J3DModelData *mModelData[2];         // 0x5c
    J3DMtxCalc *mMtxCalc[2];             // 0x64
    J3DAnmTevRegKey *mTevReg[2];         // 0x6c
    J3DAnmTexPattern *mTexPattern;       // 0x74
    s16 m_78;                            // 0x78
    s32 m_7c;                            // 0x7c
    s16 m_80;                            // 0x80
    s16 m_82;                            // 0x82
    s16 m_84;                            // 0x84
    J3DGXColorS10 mTevColor;             // 0x86
    u8 m_8e[2];                          // 0x8e

    static const J3DTevStageInfo mscTevStage[3];
    static const char *mscpaFeelingBmdFileName[2];
    static const char *mscpaFeelingBrkFileName[2];
    static const char *mscpaFeelingBtpFileName[1];
    static FeelingMgr *mspFeelingMgr;
}; // Size: 0x90

class FeelingOne : public JKRDisposer, public ExModel {
public:
    FeelingOne(Feeling *feel, int idx);
    virtual ~FeelingOne() {}
    virtual void updateF();
    virtual void setCurrentViewNo(u32);
    // Inline/Unused
    void *getAnmFile(int);
    void reset();
    void createModelF(int, JKRSolidHeap *, u32, u32);

    Feeling *mFeeling;       // 0xa4
    J3DAnmTransform *mAnm;   // 0xa8
    J3DAnmObjMaterial mMat0; // 0xac
    J3DAnmObjMaterial mMat1; // 0xcc
    s32 mIndex;              // 0xec
}; // Size: 0xf0

#endif // FEELING_H
