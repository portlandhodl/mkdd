#ifndef MAPOBJMAREW_H
#define MAPOBJMAREW_H

#include "Shiraiwa/Objects/MapObjDemoObj.h"

class TMapObjMareWBase : public TMapObjDemoObj
{
public:
    TMapObjMareWBase(const CrsData::SObject &);

    virtual ~TMapObjMareWBase();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual const char *getShadowBmdFileName() { return "/Objects/mareL_a_sh.bmd"; }
    virtual const char *getBmdFileName() = 0;

    static J3DAnmTransform *sMareWBckAnmTrans;
    static J3DMtxCalc *sMareWBckMtxCalc;
};

class TMapObjMareW_A : public TMapObjMareWBase
{
public:
    TMapObjMareW_A(const CrsData::SObject &obj) : TMapObjMareWBase(obj) {}
    virtual ~TMapObjMareW_A() {}
    virtual const char *getBmdFileName() { return "/Objects/mareW_a.bmd"; }

private:
    u32 _16c;
};

class TMapObjMareW_B : public TMapObjMareWBase
{
public:
    TMapObjMareW_B(const CrsData::SObject &obj) : TMapObjMareWBase(obj) {}
    virtual ~TMapObjMareW_B() {}
    virtual const char *getBmdFileName() { return "/Objects/mareW_b.bmd"; }

private:
    u32 _16c;
};

class TMapObjMareW_C : public TMapObjMareWBase
{
public:
    TMapObjMareW_C(const CrsData::SObject &obj) : TMapObjMareWBase(obj) {}
    virtual ~TMapObjMareW_C() {}
    virtual const char *getBmdFileName() { return "/Objects/mareW_c.bmd"; }

private:
    u32 _16c;
};

#endif
