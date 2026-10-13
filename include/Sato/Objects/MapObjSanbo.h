#ifndef MAPOBJSANBO_H
#define MAPOBJSANBO_H

#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/StateObserver.h"

class TMapObjSanboParts : public TMapObjHioNode, public StateObserver
{
public:
    TMapObjSanboParts(u32 id) : TMapObjHioNode(id), StateObserver() {}

    virtual ~TMapObjSanboParts() {}
    virtual void calc();
    virtual void MoveExec() {}
    virtual void InitExec() {}
};

class TMapObjSanboHead : public TMapObjSanboParts
{
public:
    TMapObjSanboHead(u32 id = 0x138d) : TMapObjSanboParts(id) {}
    virtual ~TMapObjSanboHead() {}

private:
    u8 _158[0x3c];
};

class TMapObjSanboBody : public TMapObjSanboParts
{
public:
    TMapObjSanboBody(u32 id = 0x138e) : TMapObjSanboParts(id) {}
    virtual ~TMapObjSanboBody() {}

private:
    u8 _158[0x3c];
};

#endif
