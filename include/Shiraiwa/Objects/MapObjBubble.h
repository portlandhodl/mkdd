#ifndef MAPOBJBUBBLE_H
#define MAPOBJBUBBLE_H

#include <dolphin/os.h>

#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/JGeometry.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class JPABaseEmitter;

class TMapObjBubble : public TMapObjHioNode, public StateObserver
{
public:
    TMapObjBubble(const CrsData::SObject &); // 0x802b9e5c
    virtual ~TMapObjBubble();                // 0x802b9f00

    virtual void createColModel(J3DModelData *);   // 0x802b9fe4
    virtual const char *getBmdFileName();          // 0x802ba010
    virtual void createModel(JKRSolidHeap *, u32); // 0x802ba038
    virtual void loadAnimation();                  // 0x802ba098
    virtual void MoveExec();                       // 0x802ba14c
    virtual void InitExec();                       // 0x802ba0e0
    virtual void update();                         // 0x802ba1b8
    virtual void reset();                          // 0x802ba1fc
    virtual void calc();                           // 0x802ba438

    void initFunc_Start();   // 0x802ba470
    void startParticle();    // UNUSED
    void stopParticle();     // 0x802ba490
    void moveEmitter();      // UNUSED
    void doFunc_Start();     // 0x802ba4ac
    void initFunc_Dive();    // 0x802ba4e4
    void doFunc_Dive();      // 0x802ba500
    void initFunc_Symptom(); // 0x802ba538
    void doFunc_Symptom();   // 0x802ba53c
    void initFunc_Jump();    // 0x802ba574
    void doFunc_Jump();      // 0x802ba62c

    static StateFuncSet<TMapObjBubble> sTable[4]; // 0x803a7c00
    static f32 sGravity;                          // 0x80415330
    static J3DAnmTextureSRTKey *sBubbleBtkAnm;    // 0x80417008

private:
    J3DAnmObjMaterial mBtk;    // 0x158
    JGeometry::TVec3f mDir;    // 0x178
    u16 mStartTime;            // 0x184
    u16 mDiveTime;             // 0x186
    u16 mSymptomTime;          // 0x188
    f32 mJumpVel;              // 0x18c
    f32 mAccel;                // 0x190
    f32 mPrevHeight;           // 0x194
    JPABaseEmitter *mEmitter;  // 0x198
}; // Size: 0x19c

#endif // MAPOBJBUBBLE_H
