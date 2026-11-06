#ifndef MAPOBJWALL_H
#define MAPOBJWALL_H

#include "Sato/StateObserver.h"
#include "Shiraiwa/Coord3D.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjWall : public TMapObjHioNode, public StateObserver
{
public:
    TMapObjWall(const CrsData::SObject &); // 0x802a5284
    virtual ~TMapObjWall();                // 0x802a534c

    virtual void reset();                                       // 0x802a53f4
    virtual const char *getBmdFileName();                       // 0x802a5508
    virtual void loadAnimation();                               // 0x802a5530
    virtual void createModel(JKRSolidHeap *, u32);              // 0x802a5598
    virtual void calc();                                        // 0x802a56dc
    virtual void InitExec();                                    // 0x802a5758
    virtual void MoveExec();                                    // 0x802a57c4
    virtual void createColModel(J3DModelData *);                // 0x802a5964

    void initFunc_Move(); // 0x802a5830
    void doFunc_Move();   // 0x802a58bc
    void initFunc_Rest(); // 0x802a5908
    void doFunc_Rest();   // 0x802a590c

    static StateFuncSet<TMapObjWall> sTable[2]; // 0x803a5978
    static u8 sUseBca;                           // 0x80415238
    static J3DAnmTransform *sWlWallBckAnmTrans;  // 0x80416ec8
    static J3DMtxCalc *sWlWallBckMtxCalc;        // 0x80416ecc

private:
    TPathMove *mPathMove; // 0x158
    u16 mRestTime;        // 0x15c
}; // Size: 0x160

#endif // MAPOBJWALL_H
