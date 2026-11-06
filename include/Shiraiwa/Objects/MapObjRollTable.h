#ifndef MAPOBJROLLTABLE_H
#define MAPOBJROLLTABLE_H

#include "Shiraiwa/Objects/MapObjHioNode.h"

class TPathUtil;

class TMapObjRollTable : public TMapObjHioNode {
public:
    TMapObjRollTable(const CrsData::SObject &); // 0x802c4ec4
    virtual ~TMapObjRollTable(); // 0x802c4f40

    virtual void createColModel(J3DModelData *); // 0x802c4ff4
    virtual void reset(); // 0x802c505c
    virtual void calc(); // 0x802c5254
    void getPow(JGeometry::TVec3f *); // 0x802c55a0
    virtual const char *getBmdFileName(); // 0x802c55dc
    virtual u32 getSoundID() const { return 0x10061; } // 0x802c5604

    static f32 sFriction; // 0x80415430
    static f32 sAccParam; // 0x80415434
    static f32 sAccThreshold; // 0x80415438
    static f32 sVelStop; // 0x8041543c
    static f32 sRotSpeed; // 0x80415440
    static f32 sRotMaxSpeed; // 0x80415444

private:
    TPathUtil *mPathUtil; // 0x14c
    JGeometry::TVec3f mDir; // 0x150
    JGeometry::TVec3f mMin; // 0x15c
    JGeometry::TVec3f mMax; // 0x168
    s8 mFlag; // 0x174
}; // Size: 0x178

#endif // MAPOBJROLLTABLE_H
