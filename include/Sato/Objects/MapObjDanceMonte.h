#ifndef MAPOBJDANCEMONTE_H
#define MAPOBJDANCEMONTE_H

#include "Shiraiwa/Objects/MapObjDemoObj.h"

class TMapObjDanceMonte : public TMapObjDemoObj
{
public:
    TMapObjDanceMonte(const CrsData::SObject &);

    virtual ~TMapObjDanceMonte();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual const char *getShadowBmdFileName() { return "/Objects/monL_a_sh.bmd"; }
};

class TMapObjDanceMonteA : public TMapObjDanceMonte
{
public:
    TMapObjDanceMonteA(const CrsData::SObject &obj) : TMapObjDanceMonte(obj) {}
    virtual ~TMapObjDanceMonteA() {}
    virtual const char *getBmdFileName() { return "/Objects/monF_a.bmd"; }

private:
    u8 _pad[24];

};

class TMapObjDanceMonteB : public TMapObjDanceMonte
{
public:
    TMapObjDanceMonteB(const CrsData::SObject &obj) : TMapObjDanceMonte(obj) {}
    virtual ~TMapObjDanceMonteB() {}
    virtual const char *getBmdFileName() { return "/Objects/monF_b.bmd"; }

private:
    u8 _pad[24];

};

#endif
