#ifndef MAPOBFOUNTAIN_H
#define MAPOBFOUNTAIN_H

#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/J3DAnmObject.h"

class TMapObjFountain : public TMapObjHioNode {
public:
    TMapObjFountain(const CrsData::SObject &); // 0x802b0f6c
    virtual ~TMapObjFountain(); // 0x802b100c

    virtual const char *getBmdFileName(); // 0x802b10dc
    virtual void reset(); // 0x802b1104
    virtual void createModel(JKRSolidHeap *, u32); // 0x802b1180
    virtual void loadAnimation(); // 0x802b11bc
    virtual void calc(); // 0x802b1204
    virtual void update(); // 0x802b1250

    static J3DAnmTextureSRTKey *sFountainBtkAnm; // 0x80416f58

    J3DAnmObjMaterial mBtk; // 0x14c
};

class TMapObjDemoFountain : public TMapObjFountain {
public:
    TMapObjDemoFountain(const CrsData::SObject &); // 0x802b1294
    virtual ~TMapObjDemoFountain(); // 0x802b1344

    virtual void createModel(JKRSolidHeap *, u32); // 0x802b1424
    virtual void loadAnimation(); // 0x802b1460
    virtual void reset(); // 0x802b14a8
    virtual void calc(); // 0x802b152c
    virtual const char *getBmdFileName() { return "/Objects/DPeachFountain.bmd"; } // 0x802b15a0

    static J3DAnmTextureSRTKey *sDemoFountainBtkAnm; // 0x80416f64

    bool _16c; // 0x16c
};

#endif // MAPOBFOUNTAIN_H
