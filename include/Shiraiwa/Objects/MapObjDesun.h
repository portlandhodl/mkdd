#ifndef MAPOBDESUN_H
#define MAPOBDESUN_H

#include "JSystem/J3D/J3DAnmTexPattern.h"
#include "JSystem/J3D/J3DAnmTransform.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/J3DAnmObject.h"

class TMapObjDesun : public TMapObjHioNode {
public:
    TMapObjDesun(const CrsData::SObject &); // 0x802bb674
    virtual ~TMapObjDesun(); // 0x802bb6f8

    virtual void loadAnimation(); // 0x802bb804
    virtual void createModel(JKRSolidHeap *, u32); // 0x802bb850
    virtual void reset(); // 0x802bb7b0
    virtual void calc(); // 0x802bb970
    virtual const char *getBmdFileName(); // 0x802bb788

    static J3DAnmTransform *sDesunBckAnmTrans; // 0x80417030
    static J3DMtxCalc *sDesunBckMtxCalc; // 0x80417034
};

class TMapObjDeBalloon : public TMapObjHioNode {
public:
    TMapObjDeBalloon(const CrsData::SObject &); // 0x802bb974
    virtual ~TMapObjDeBalloon(); // 0x802bba00

    virtual void createModel(JKRSolidHeap *, u32); // 0x802bbad0
    virtual void loadAnimation(); // 0x802bbb30
    virtual void update(); // 0x802bbb78
    virtual void reset(); // 0x802bbbe0
    virtual void calc(); // 0x802bbc94
    virtual const char *getBmdFileName() { return "/Objects/DeBalloon1.bmd"; } // 0x802bbcf8

    static J3DAnmTexPattern *sDeBalloonBtpAnm; // 0x80417040
    static f32 sFloatHeight; // 0x80415340
    static s16 sFloatSpeed; // 0x80415344
    static s16 scFloatStartDeg; // 0x80417044

    J3DAnmObjMaterial mBtp; // 0x14c
    s16 mAngle; // 0x16c
};

#endif // MAPOBDESUN_H
