#ifndef MAPOBJUFO_H
#define MAPOBJUFO_H

#include "Sato/StateObserver.h"
#include "Shiraiwa/Coord3DUtil.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjUFO : public TMapObjHioNode, public StateObserver
{
public:
    TMapObjUFO(const CrsData::SObject &); // 0x80299bb8
    virtual ~TMapObjUFO();                // 0x80299c9c

    virtual void reset();                                          // 0x80299dac
    virtual void calc();                                           // 0x80299f68
    virtual void createColModel(J3DModelData *) {}                 // 0x8029a2bc
    virtual const char *getBmdFileName() { return "/Objects/ufo.bmd"; } // 0x8029a2c0
    virtual void MoveExec();                                       // 0x80299efc
    virtual void InitExec();                                       // 0x80299e90

    void initFunc_Fly();  // 0x80299fc4
    void doFunc_Fly();    // 0x8029a128
    void initFunc_Wait(); // 0x8029a224
    void doFunc_Wait();   // 0x8029a284

    static StateFuncSet<TMapObjUFO> sTable[2]; // 0x803a47a8
    static f32 sRotRad;                         // 0x804151b4
    static f32 sFlySpeed;                       // 0x804151b8
    static s16 sWaitFrame;                      // 0x804151bc

private:
    TPathUtil *mPathUtil; // 0x158
    Mtx mRotY;            // 0x15c
}; // Size: 0x18c

#endif // MAPOBJUFO_H
