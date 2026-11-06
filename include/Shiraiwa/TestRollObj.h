#ifndef TESTROLLOBJ_H
#define TESTROLLOBJ_H

#include "Shiraiwa/Coord3D.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TTestRollObj : public TMapObjHioNode {
public:
    TTestRollObj(const CrsData::SObject &);
    virtual ~TTestRollObj();

    virtual void doKartColCallBack(int);
    virtual void reset();
    virtual void createColModel(J3DModelData *);
    virtual void calc();
    virtual const char *getBmdFileName();

    static f32 sRotSpeed;

private:
    u8 _14C[8];
    TFreeFallShakeSky *mFreeFall;
}; // Size: 0x158

#endif
