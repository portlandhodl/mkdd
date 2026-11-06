#ifndef MAPOBJSUN_H
#define MAPOBJSUN_H

#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/J3DAnmObject.h"

class TMapObjSun : public TMapObjHioNode {
public:
    TMapObjSun(const CrsData::SObject &);
    virtual ~TMapObjSun();

    virtual const char *getBmdFileName();
    virtual void reset();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void loadAnimation();
    virtual void calc();
    virtual void update();

    static J3DAnmTextureSRTKey *sSunBtkAnm;

private:
    J3DAnmObjMaterial mAnmObj;
};

#endif
