#ifndef MAPOBSNOWMAN_H
#define MAPOBSNOWMAN_H

#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/J3DAnmObject.h"

class TMapObjSnowMan : public TMapObjHioNode {
public:
    TMapObjSnowMan(const CrsData::SObject &);
    virtual ~TMapObjSnowMan();

    virtual void createModel(JKRSolidHeap *, u32);
    virtual void loadAnimation();
    virtual void update();
    virtual void calc();
    virtual void reset();
    virtual const char *getBmdFileName();

    static J3DDeformData *sSnowManBlsAnmData;
    static J3DAnmCluster *sSnowManBlkAnmData;

private:
    J3DAnmObjCluster mAnmObj;
};

#endif
