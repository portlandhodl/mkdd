#ifndef MAPOBJDOSSUN_H
#define MAPOBJDOSSUN_H

#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/Coord3D.h"

class TMapObjDossun : public TMapObjStamper, public StateObserver
{
public:
    TMapObjDossun(const CrsData::SObject &); // 0x8028a7ac
    virtual void calc();                           // 0x8028b75c
    virtual ~TMapObjDossun();                // 0x8028a8c8

    virtual void reset();                          // 0x8028a9f0
    virtual void loadAnimation();                  // 0x8028ad5c
    virtual const char *getShadowBmdFileName();    // 0x8028adc4
    virtual void createColModel(J3DModelData *);   // 0x8028adec
    virtual void createModel(JKRSolidHeap *, u32); // 0x8028ae54
    virtual const char *getBmdFileName();          // 0x8028afac
    virtual void update();                         // 0x8028afd4
    virtual void doKartColCallBack(int);           // 0x8028b03c
    virtual u32 getReleaseFrame();                 // 0x8028b850
    virtual void MoveExec();                       // 0x8028b91c
    virtual void InitExec();                       // 0x8028b8b0

    void initFunc_Start();    // 0x8028b0b0
    void doFunc_Start();      // 0x8028b0b4
    void initFunc_Wait();     // UNUSED
    void doFunc_Wait();       // UNUSED
    void initFunc_Up();       // 0x8028b0ec
    void doFunc_Up();         // 0x8028b170
    void initFunc_Top();      // 0x8028b1a4
    void doFunc_Top();        // 0x8028b218
    void initFunc_Down();     // 0x8028b3d4
    void doFunc_Down();       // 0x8028b464
    void makeGroundEmitter(); // 0x8028b4cc
    void initFunc_Bottom();   // 0x8028b52c
    void doFunc_Bottom();     // 0x8028b554
    void initFunc_Die();      // 0x8028b630
    void doFunc_Die();        // 0x8028b694
    void shakeCamera();       // 0x8028b72c
    void moveShadow(JGeometry::TVec3f &); // UNUSED
    void checkItemHitting();  // 0x8028b858

    static StateFuncSet<TMapObjDossun> sTable[6];     // 0x803a2dd0
    static JGeometry::TVec3f scDownVelocity0;         // 0x80413a40
    static JGeometry::TVec3f sDieVelocity0;           // 0x80413a98
    static f32 sBtpRevFrameRate;                      // 0x804150f8
    static f32 sBtpPlyFrameRate;                      // 0x804150fc
    static f32 sDieGravityY;                          // 0x80415100
    static J3DAnmTransform *sDossunBckAnmTrans;       // 0x80416d48
    static J3DMtxCalc *sDossunBckMtxCalc;             // 0x80416d4c
    static J3DAnmTexPattern *sDossunBtpAnm;           // 0x80416d50

private:
    J3DAnmObjMaterial mBtpAnmObj; // 0x180
    s16 mHeight;                  // 0x1a0
    u16 mTopFrame;                // 0x1a2
    u8 mBottomFrame;              // 0x1a4
    u16 mFrame;                   // 0x1a6
    TFreeMove mFreeMove;          // 0x1a8
    TPathMove *mPathMove;         // 0x1d0
    s16 mStartFrame;              // 0x1d4
}; // Size: 0x1d8

#endif // MAPOBJDOSSUN_H
