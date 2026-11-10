#ifndef MAPOBJPAKKUNCOL_H
#define MAPOBJPAKKUNCOL_H

#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjPakkun;

class TMapObjPakkunColHead : public TMapObjHioNode
{
public:
    TMapObjPakkunColHead(u32 id = 0xe81) : TMapObjHioNode(id), _14c(0), mPakkun(nullptr) {}

    virtual ~TMapObjPakkunColHead() {}

    virtual const char *getShadowBmdFileName() { return "/Objects/PakkunShadow.bmd"; }
    virtual void createColModel(J3DModelData *);
    void setScale(const JGeometry::TVec3f &);
    bool isDamage();
    void setPosition(const JGeometry::TVec3f &);
    virtual void reset();
    virtual void calc();
    virtual void doKartColCallBack(int);
    virtual void getKartThrowDirPow(JGeometry::TVec3f *, f32 *, int);

    void checkItemHitting();

    u8 _14c;
    TMapObjPakkun *mPakkun;
};

#endif
