#ifndef MAPOBJDANCEMONTE_H
#define MAPOBJDANCEMONTE_H

#include "Shiraiwa/AnmPlayer.h"
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

    static TAnmInfo sAnmInfos[2];

private:
    TAnmPlayer mAnmPlayer; // 0x16c
};

class TMapObjDanceMonteA : public TMapObjDanceMonte
{
public:
    TMapObjDanceMonteA(const CrsData::SObject &obj) : TMapObjDanceMonte(obj) {}
    virtual ~TMapObjDanceMonteA() {}
    virtual const char *getBmdFileName() { return "/Objects/monF_a.bmd"; }
};

class TMapObjDanceMonteB : public TMapObjDanceMonte
{
public:
    TMapObjDanceMonteB(const CrsData::SObject &obj) : TMapObjDanceMonte(obj) {}
    virtual ~TMapObjDanceMonteB() {}
    virtual const char *getBmdFileName() { return "/Objects/monF_b.bmd"; }
};

#endif
