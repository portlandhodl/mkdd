#ifndef SHINEARROW_H
#define SHINEARROW_H

#include <dolphin/os.h>
#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/JGeometry.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TShineArrow : public TMapObjHioNode, public StateObserver
{
public:
    TShineArrow();          // 0x802d69e0
    virtual ~TShineArrow(); // 0x802d6ac8

    void init(u8, u8);                             // 0x802d6be8
    virtual void MoveExec();                       // 0x802d6c60
    virtual void InitExec();                       // 0x802d6bf4
    virtual void createModel(JKRSolidHeap *, u32); // 0x802d6ccc
    virtual void loadAnimation();                  // 0x802d6d08
    virtual void reset();                          // 0x802d6d50
    virtual void update();                         // 0x802d6f70
    virtual void calc();                           // 0x802d6fd8
    void moveKartHead();                           // 0x802d700c
    virtual void viewCalc(u32);                    // 0x802d70c8
    void initFunc_Appear();                        // 0x802d70fc
    void doFunc_Appear();                          // 0x802d710c
    void initFunc_Point();                         // 0x802d717c
    void doFunc_Point();                           // 0x802d7180
    bool isDisappear();                            // 0x802d71c8
    bool isAppear();                               // UNUSED
    void changeCheckPos();                         // UNUSED
    void changeCheckPos(const JGeometry::TVec3f &, JGeometry::TVec2f *); // UNUSED
    bool isInside();                               // UNUSED
    void aimToShine();                             // 0x802d7204
    bool isRabbitHolder(u32);                      // 0x802d754c
    void initFunc_Disappear();                     // 0x802d7598
    void doFunc_Disappear();                       // 0x802d759c
    void initFunc_Hide();                          // 0x802d760c
    void doFunc_Hide();                            // 0x802d761c

    virtual const char *getBmdFileName() { return "/Objects/MiniGameArrow.bmd"; } // 0x802d76fc

    static StateFuncSet<TShineArrow> sTable[4];        // 0x803aaf04
    static const f32 scHeightOffset[21][4];             // 0x803858c0
    static const f32 scPakkunHeightOffset[21][4];       // 0x80385a10
    static f32 sScaleSpeed;                             // 0x80415560
    static J3DAnmTextureSRTKey *sShineArrowBtkAnm;      // 0x80417178
    static f32 sRotRad;                                 // 0x8041717c
    static s16 sAppearFrame;                            // 0x80417180
    static s16 sEraseFrame;                             // 0x80417182

private:
    J3DAnmObjMaterial mAnmObj;     // 0x158
    J3DAnmObjMaterial mBtk;        // 0x178
    u8 mKartNo;                    // 0x198
    u8 mViewNo;                    // 0x199
    const f32 *mHeightOffset;      // 0x19c
    JGeometry::TPos3f mAimMtx;     // 0x1a0
    JGeometry::TPos3f mSpinMtx;    // 0x1d0
    JGeometry::TPos3f mRotSpeedMtx; // 0x200
}; // Size: 0x230

class TShineArrowManager
{
public:
    TShineArrowManager();                 // 0x802d68d8
    virtual ~TShineArrowManager() {}      // 0x802d7708
    void create();                        // 0x802d6908

private:
    TShineArrow *mArrows[4]; // 0x4
}; // Size: 0x14

#endif // SHINEARROW_H
