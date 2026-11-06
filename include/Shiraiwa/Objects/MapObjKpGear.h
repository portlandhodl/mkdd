#ifndef MAPOBJKPGEAR_H
#define MAPOBJKPGEAR_H

#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjKpGear : public TMapObjHioNode {
public:
    TMapObjKpGear(const CrsData::SObject &);
    virtual ~TMapObjKpGear();

    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual const char *getBmdFileName();
    virtual void calc();

    static J3DAnmTransform *sKpGearBckAnmTrans;
    static J3DMtxCalc *sKpGearBckMtxCalc;
};

#endif
