#ifndef MAPOBJMARE_H
#define MAPOBJMARE_H

#include "Shiraiwa/Objects/MapObjDemoObj.h"
#include "Sato/Objects/MapObjMareW.h"
#include "Sato/StateObserver.h"

class TMapObjMareBase : public TMapObjDemoObj, public StateObserver
{
public:
    TMapObjMareBase(const CrsData::SObject &);

    virtual ~TMapObjMareBase();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual const char *getShadowBmdFileName() { return "/Objects/mareL_a_sh.bmd"; }

    void MoveExec() {}
    void InitExec() {}
};

class TMapObjMareA : public TMapObjMareBase
{
public:
    TMapObjMareA(const CrsData::SObject &obj) : TMapObjMareBase(obj) {}
    virtual ~TMapObjMareA() {}
    virtual const char *getBmdFileName() { return "/Objects/mareL_a.bmd"; }

private:
    u8 _pad[28];


};

class TMapObjMareB : public TMapObjMareBase
{
public:
    TMapObjMareB(const CrsData::SObject &obj) : TMapObjMareBase(obj) {}
    virtual ~TMapObjMareB() {}
    virtual const char *getBmdFileName() { return "/Objects/mareL_b.bmd"; }

private:
    u8 _pad[28];


};

class TMapObjMareC : public TMapObjMareBase
{
public:
    TMapObjMareC(const CrsData::SObject &obj) : TMapObjMareBase(obj) {}
    virtual ~TMapObjMareC() {}
    virtual const char *getBmdFileName() { return "/Objects/mareL_c.bmd"; }

private:
    u8 _pad[28];


};

class TMapObjMareM_A : public TMapObjMareWBase
{
public:
    TMapObjMareM_A(const CrsData::SObject &); // ctor is not inline; defined in its own unit
    u32 _16c;
};

#endif
