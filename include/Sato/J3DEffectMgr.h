#ifndef J3DEFFECTMGR
#define J3DEFFECTMGR

#include "JSystem/JSupport/JSUList.h"
#include "Kaneshige/ExModel.h"

class CrsGround;
class PermissionCounter;
class PermissionCounterObserver;

class J3DEffectObj
{
public:
    J3DEffectObj() : mLink(this) {}

    template <class T>
    static J3DEffectObj *New();

    template <class T>
    static const char *GetBmdName();

    template <class T>
    static void LoadAnm();

    virtual ~J3DEffectObj() {}                  // 08, weak
    virtual void onlyinit() = 0;                // 0c
    virtual void init() = 0;                    // 10
    virtual void calc() = 0;                    // 14
    virtual void update() { mModel.update(0); } // 18, weak
    void setRTMtx(Mtx);
    virtual void setTrigger() {}                // 1c, weak

    ExModel mModel;              // 4
    int mState;                  // 90
    Mtx mMtx;                    // 94
    int mType;                   // c4
    JSULink<J3DEffectObj> mLink; // c8
};

class J3DEffectMgr
{
public:
    J3DEffectMgr();
    static void createMgr() { sJ3DMgr = new J3DEffectMgr(); }
    void createModel(JKRSolidHeap *, u32);         // 0x8025657c
    void reset();                                  // 0x80256618
    void loadJ3DdataAll();                         // 0x802566a4
    void createAll();                              // 0x80256758
    void append(int);
    void remove(J3DEffectObj *);                   // 0x802567e4
    J3DEffectObj *setEffectKart(int, int, Mtx m);  // 0x8025681c
    J3DEffectObj *setEffectSplash(CrsGround *, int, Mtx m); // 0x80256920
    J3DEffectObj *setEffect(int, Mtx m, int);      // 0x80256a5c
    void setModelDataAppend(J3DEffectObj *, int);  // 0x80256ae8
    void calc();                                   // 0x80256b40
    void update();                                 // 0x80256bc0
    void viewCalc(u32);                            // 0x80256c3c
    void setCurrentViewNo(u32);                    // 0x80256c90

    static J3DEffectMgr *getJ3DEffectMgr() { return sJ3DMgr; }
    static J3DEffectMgr *sJ3DMgr;

    J3DModelData *getModelData() { return mModelDataAll[3]; }
    J3DModelData *getBombModelData() { return mModelDataAll[0]; }
    J3DModelData *getSplashModelData() { return mModelDataAll[1]; }
    J3DModelData *getMudModelData() { return mModelDataAll[2]; }

private:
    J3DModelData *mModelDataAll[5]; // 00
    u8 _14[0x28 - 0x14];            // 14
    JSUList<J3DEffectObj> mFreeList[5]; // 28
    JSUList<J3DEffectObj> mActiveList;  // 64
    PermissionCounterObserver *mObserver; // 70
    PermissionCounter *mCounters;         // 74
};

class J3DEfctKartAnmMgr
{
public:
    J3DEfctKartAnmMgr();

    void *getSimpleCharBrkAnm(u32, u8 index);
    void *getSimpleKartBrkAnm(u32, u8 index);

    static void createMgr() { sJ3DKartAnmMgr = new J3DEfctKartAnmMgr(); };
    static J3DEfctKartAnmMgr *getMgr() { return sJ3DKartAnmMgr; }

    void initSimpleBrkEfctAnimation();
    void loadSimpleBrkEfctAnimation();
    void loadSimpleCharBrkEfctFile(u32, u8);
    void loadSimpleKartPartsBrkEfctFile(u32, u8);
    const char *getBrkEfctCharName(u8);
    const char *getBrkEfctSimpleKartName(u8);
    const char *getBrkEfctEffectName(u32);

private:
    static J3DEfctKartAnmMgr *sJ3DKartAnmMgr;

    void **mCharBrkAnm[3]; // 0
    void **mKartBrkAnm[3]; // c
};

inline void CreateJ3DEfctKarAnmMgr() { J3DEfctKartAnmMgr::createMgr(); };
inline void CreateJ3DEfctMgr() { J3DEffectMgr::createMgr(); };
inline J3DEffectMgr *GetJ3DEfctMgr() { return J3DEffectMgr::getJ3DEffectMgr(); }
inline J3DEfctKartAnmMgr *GetJ3DEfctKartAnmMgr() { return J3DEfctKartAnmMgr::getMgr(); }

#endif
