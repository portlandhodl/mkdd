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
    virtual const char *getBmdFileName() = 0;
    virtual bool acceptHidding() = 0;
    virtual void MoveExec();
    virtual void InitExec();

    void reset();
    void initFunc_Hide();
    void doFunc_Hide();
    void initFunc_Burst();
    void doFunc_Burst();
    void burst(const JGeometry::TVec3f &, const JGeometry::TPos3f &);

    static StateObserver::StateFuncSet<TMapObjSanboParts> sTable[2];
    static f32 sGravity;
    static f32 sAirFric;
    static f32 sBurstVelH;
    static f32 sBurstVelV;
    static f32 sRotRad;

    JGeometry::TPos3f mRotVel;    // 158
    JGeometry::TVec3f mBaseScale; // 188
};

class TMapObjSanboHead : public TMapObjSanboParts
{
public:
    TMapObjSanboHead(u32 id = 0x138d) : TMapObjSanboParts(id) {}
    virtual ~TMapObjSanboHead() {}

    virtual const char *getBmdFileName() { return "/Objects/Sanbo1_break_head.bmd"; }
    virtual bool acceptHidding();
};

class TMapObjSanboBody : public TMapObjSanboParts
{
public:
    TMapObjSanboBody(u32 id = 0x138e) : TMapObjSanboParts(id) {}
    virtual ~TMapObjSanboBody() {}

    virtual const char *getBmdFileName() { return "/Objects/Sanbo1_break_body.bmd"; }
    virtual bool acceptHidding();
};

#endif
