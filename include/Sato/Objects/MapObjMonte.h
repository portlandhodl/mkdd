#ifndef MAPOBJMONTE_H
#define MAPOBJMONTE_H

#include "Shiraiwa/Objects/MapObjDemoObj.h"
#include "Sato/StateObserver.h"

class TMapObjMonteBase : public TMapObjDemoObj, public StateObserver
{
public:
    TMapObjMonteBase(const CrsData::SObject &);

    virtual ~TMapObjMonteBase();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual const char *getShadowBmdFileName() { return "/Objects/monL_a_sh.bmd"; }

    void MoveExec() {}
    void InitExec() {}
};

class TMapObjMonteA : public TMapObjMonteBase
{
public:
    TMapObjMonteA(const CrsData::SObject &obj) : TMapObjMonteBase(obj) {}
    virtual ~TMapObjMonteA() {}
    virtual const char *getBmdFileName() { return "/Objects/monL_a.bmd"; }

private:
    u8 _pad[28];

};

class TMapObjMonteB : public TMapObjMonteBase
{
public:
    TMapObjMonteB(const CrsData::SObject &obj) : TMapObjMonteBase(obj) {}
    virtual ~TMapObjMonteB() {}
    virtual const char *getBmdFileName() { return "/Objects/monL_b.bmd"; }

private:
    u8 _pad[28];

};

class TMapObjMonteC : public TMapObjMonteBase
{
public:
    TMapObjMonteC(const CrsData::SObject &obj) : TMapObjMonteBase(obj) {}
    virtual ~TMapObjMonteC() {}
    virtual const char *getBmdFileName() { return "/Objects/monL_c.bmd"; }

private:
    u8 _pad[28];

};

class TMapObjMonteD : public TMapObjMonteBase
{
public:
    TMapObjMonteD(const CrsData::SObject &obj) : TMapObjMonteBase(obj) {}
    virtual ~TMapObjMonteD() {}
    virtual const char *getBmdFileName() { return "/Objects/monL_d.bmd"; }

private:
    u8 _pad[28];

};

class TMapObjMonteE : public TMapObjMonteBase
{
public:
    TMapObjMonteE(const CrsData::SObject &obj) : TMapObjMonteBase(obj) {}
    virtual ~TMapObjMonteE() {}
    virtual const char *getBmdFileName() { return "/Objects/monL_e.bmd"; }

private:
    u8 _pad[28];

};

#endif
