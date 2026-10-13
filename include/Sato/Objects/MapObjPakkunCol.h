#ifndef MAPOBJPAKKUNCOL_H
#define MAPOBJPAKKUNCOL_H

#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjPakkunColHead : public TMapObjHioNode
{
public:
    TMapObjPakkunColHead(u32 id = 0xe81) : TMapObjHioNode(id), _150(0) {}

    virtual ~TMapObjPakkunColHead() {}

    void calc() {}

    u32 _14c;
    u32 _150;
};

#endif
