#ifndef MAPOBJMONTE_H
#define MAPOBJMONTE_H

#include "Shiraiwa/AnmPlayer.h"
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

    void MoveExec();
    void InitExec();
    void changeAllState(u16);

    void initFunc_Clapping();
    void doFunc_Clapping();
    void initFunc_Banzai();
    void doFunc_Banzai();

    static const f32 sIdentityQuat[4];
    static TAnmInfo sAnmInfos[2];
    static StateObserver::StateFuncSet<TMapObjMonteBase> sTable[2];
    static const s8 sAnmTable[2];

private:
    TAnmPlayer mAnmPlayer; // 0x178
    f32 mLenSq2;           // 0x190
};

class TMapObjMonteA : public TMapObjMonteBase
{
public:
    TMapObjMonteA(const CrsData::SObject &obj) : TMapObjMonteBase(obj) {}
    virtual ~TMapObjMonteA() {}
    virtual const char *getBmdFileName() { return "/Objects/monL_a.bmd"; }
};

class TMapObjMonteB : public TMapObjMonteBase
{
public:
    TMapObjMonteB(const CrsData::SObject &obj) : TMapObjMonteBase(obj) {}
    virtual ~TMapObjMonteB() {}
    virtual const char *getBmdFileName() { return "/Objects/monL_b.bmd"; }
};

class TMapObjMonteC : public TMapObjMonteBase
{
public:
    TMapObjMonteC(const CrsData::SObject &obj) : TMapObjMonteBase(obj) {}
    virtual ~TMapObjMonteC() {}
    virtual const char *getBmdFileName() { return "/Objects/monL_c.bmd"; }
};

class TMapObjMonteD : public TMapObjMonteBase
{
public:
    TMapObjMonteD(const CrsData::SObject &obj) : TMapObjMonteBase(obj) {}
    virtual ~TMapObjMonteD() {}
    virtual const char *getBmdFileName() { return "/Objects/monL_d.bmd"; }
};

class TMapObjMonteE : public TMapObjMonteBase
{
public:
    TMapObjMonteE(const CrsData::SObject &obj) : TMapObjMonteBase(obj) {}
    virtual ~TMapObjMonteE() {}
    virtual const char *getBmdFileName() { return "/Objects/monL_e.bmd"; }
};

class TMapObjUkleleMonte : public TMapObjDemoObj
{
public:
    TMapObjUkleleMonte(const CrsData::SObject &);

    virtual ~TMapObjUkleleMonte();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual const char *getShadowBmdFileName();
    virtual const char *getBmdFileName();

    static J3DAnmTransform *sUkMonteBckAnmTrans;
    static J3DMtxCalc *sUkMonteBckMtxCalc;

    u32 _16c;
};

#endif
