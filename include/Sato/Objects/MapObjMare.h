#ifndef MAPOBJMARE_H
#define MAPOBJMARE_H

#include "Shiraiwa/AnmPlayer.h"
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

    void MoveExec();
    void InitExec();
    void changeAllState(u16);

    void initFunc_Clapping();
    void doFunc_Clapping();
    void initFunc_Banzai();
    void doFunc_Banzai();

    static TAnmInfo sAnmInfos[2];
    static StateObserver::StateFuncSet<TMapObjMareBase> sTable[2];
    static const s8 sAnmTable[2];

private:
    TAnmPlayer mAnmPlayer; // 0x178
    f32 mLenSq2;           // 0x190
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

class TMapObjMareM_A : public TMapObjDemoObj
{
public:
    TMapObjMareM_A(const CrsData::SObject &);

    virtual ~TMapObjMareM_A();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual const char *getShadowBmdFileName();
    virtual const char *getBmdFileName();

    static J3DAnmTransform *sMareMBckAnmTrans;
    static J3DMtxCalc *sMareMBckMtxCalc;

    u32 _16c;
};

#endif
