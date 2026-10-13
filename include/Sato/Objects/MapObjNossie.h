#ifndef MAPOBJNOSSIE_H
#define MAPOBJNOSSIE_H

#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjNossieColBase : public TMapObjStamper
{
public:
    TMapObjNossieColBase(u32 id) : TMapObjStamper(id) {}

    virtual ~TMapObjNossieColBase() {}
    virtual void calc();
};

class TMapObjNossieColFoot : public TMapObjNossieColBase
{
public:
    TMapObjNossieColFoot(u32 id) : TMapObjNossieColBase(id)
    {
        createSoundMgr();
    }

    virtual ~TMapObjNossieColFoot() {}
    virtual void reset();
    virtual void calc();
    virtual void createColModel(J3DModelData *);
    virtual u32 getReleaseFrame();
    virtual u32 getReaction() { return 0xa; }
};

class TMapObjNossieColFootFront : public TMapObjNossieColFoot
{
public:
    TMapObjNossieColFootFront(u32 id = 0x11a3) : TMapObjNossieColFoot(id) {}
    virtual ~TMapObjNossieColFootFront() {}
    virtual const char *getShadowBmdFileName() { return "/Objects/NossieFrontShadow.bmd"; }

private:
    u8 _extra[0x18];
};

class TMapObjNossieColFootRear : public TMapObjNossieColFoot
{
public:
    TMapObjNossieColFootRear(u32 id = 0x11a7) : TMapObjNossieColFoot(id) {}
    virtual ~TMapObjNossieColFootRear() {}
    virtual const char *getShadowBmdFileName() { return "/Objects/NossieBackShadow.bmd"; }

private:
    u8 _extra[0x18];
};

class TMapObjNossieColHead : public TMapObjNossieColBase
{
public:
    TMapObjNossieColHead(int id = 0x11a2) : TMapObjNossieColBase(id) { createSoundMgr(); }
};

class TMapObjNossieColBody : public TMapObjNossieColBase
{
public:
    TMapObjNossieColBody(int id = 0x11a8) : TMapObjNossieColBase(id) {}
};

#endif
