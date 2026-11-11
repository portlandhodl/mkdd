#ifndef MAPOBJAWARD_H
#define MAPOBJAWARD_H

#include "Sato/GeographyObj.h"
#include "Sato/StateObserver.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/Objects/GeoItemBox.h"
#include "Shiraiwa/Objects/MapObjAwardItemBox.h"
#include "Shiraiwa/Objects/MapObjDemoObj.h"

// Inline/Unused
//void JSULink<TMapObjAwardItemBox>::~JSULink();
//void JSUList<TMapObjAwardCup>::~JSUList();
//void JSULink<TMapObjAwardCup>::~JSULink();
//void JSUList<TMapObjAwardItemBox>::~JSUList();
//void TMapObjAwardCup::sSidePowDecel;
//void TMapObjAwardCup::sAxisCycle;

class TMapObjAwardCup;

class TAwardCupSupervisor : public GeoObjSupervisor {
public:
    TAwardCupSupervisor() { mList.initiate(); }

    void entry(TMapObjAwardCup *);
    void draw(u32);

    virtual ~TAwardCupSupervisor() {} // 0x802cd624
    void reset() {} // 0x802cd6c4
    void calc() {}  // 0x802cd6c8

private:
    JSUList<TMapObjAwardCup> mList; // 0x24
}; // Size: 0x30

class TMapObjAwardCup : public TMapObjDemoObj, public StateObserver, public JKRDisposer {
public:
    TMapObjAwardCup(u32);
    void createModel(JKRSolidHeap *, u32);
    void update();
    void reset();
    void loadAnimation();
    void setOrgMtx(const JGeometry::TPos3f &);
    f32 makeOffset();
    void flyOut(const JGeometry::TVec3f &);
    void skip(const JGeometry::TVec3f &);
    void initFunc_InBox();
    void doFunc_InBox();
    void initFunc_FlyOut();
    void doFunc_FlyOut();
    void initFunc_Roll();
    void doFunc_Roll();
    void rotAnimation(bool);
    void calc();
    void draw(u32);

    static StateFuncSet<TMapObjAwardCup> sTable[3];
    static JGeometry::TVec3f sVelocity0;
    static f32 sAirFriction;
    static f32 sGravity;
    static f32 sScaleVel;
    static s16 sJumpFrame;
    static f32 sRollSpeed;
    static f32 sRollSpeedMax;
    static s16 sRotCycle;
    static s16 sAxisRotCycle;
    static s16 sFlyOutRotCycle;
    static f32 sRotVel0;
    static f32 sRotBrake;
    static s16 sAxisCycleDecel;
    static f32 sOffsetHeight;
    static TAwardCupSupervisor *sSupervisor;
    static J3DAnmTexPattern *sAwardCupBtpAnm;

    void MoveExec(); // 0x802cc610
    void InitExec(); // 0x802cc5a4
    virtual const char *getBmdFileName() = 0;
    virtual const char *getBtpFileName() = 0;
    virtual GeoObjSupervisor *getSupervisor() { return sSupervisor; }
    virtual u32 getJ3DModelDataTevStageNum() const { return 0x20000; }
    friend class TMapObjAwardItemBox;
    friend class TAwardCupSupervisor;
    JSULink<TMapObjAwardCup> mLink; // 0x190
private:
    J3DAnmObjMaterial mMatAnm; // 0x1a0
    s8 mDir;                   // 0x1c0
    u8 _pad1c1[3];             // 0x1c1
    JGeometry::TVec3f mModelZDir; // 0x1c4
    JGeometry::TVec3f mModelYDir; // 0x1d0
    JGeometry::TVec3f mModelXDir; // 0x1dc
    Mtx mOrgMtx;               // 0x1e8
    f32 mRollSpeed;            // 0x218
    f32 mRollSpeed2;           // 0x21c
    s16 mFlag220;              // 0x220
    s16 mFlag222;              // 0x222
    s16 mFlag224;              // 0x224
    JGeometry::TVec3f mDirVec; // 0x228
    u8 mFlag234;               // 0x234
    u8 _pad235[3];             // 0x235
}; // Size: 0x238


class TMapObjAwardCupKinoko : public TMapObjAwardCup
{
public:
    TMapObjAwardCupKinoko(u32 id = 0xd67) : TMapObjAwardCup(id) {}
    virtual ~TMapObjAwardCupKinoko() {}
    virtual const char *getBmdFileName() { return "/AwardKinokoCup.bmd"; }
    virtual const char *getBtpFileName() { return "/AwardKinokoCup.btp"; }

private:
    u8 _pad[0xa8];
};

class TMapObjAwardCupFlower : public TMapObjAwardCup
{
public:
    TMapObjAwardCupFlower(u32 id = 0xd68) : TMapObjAwardCup(id) {}
    virtual ~TMapObjAwardCupFlower() {}
    virtual const char *getBmdFileName() { return "/AwardFlowerCup.bmd"; }
    virtual const char *getBtpFileName() { return "/AwardFlowerCup.btp"; }

private:
    u8 _pad[0xa8];
};

class TMapObjAwardCupStar : public TMapObjAwardCup
{
public:
    TMapObjAwardCupStar(u32 id = 0xd69) : TMapObjAwardCup(id) {}
    virtual ~TMapObjAwardCupStar() {}
    virtual const char *getBmdFileName() { return "/AwardStarCup.bmd"; }
    virtual const char *getBtpFileName() { return "/AwardStarCup.btp"; }

private:
    u8 _pad[0xa8];
};

class TMapObjAwardCupSpecial : public TMapObjAwardCup
{
public:
    TMapObjAwardCupSpecial(u32 id = 0xd6a) : TMapObjAwardCup(id) {}
    virtual ~TMapObjAwardCupSpecial() {}
    virtual const char *getBmdFileName() { return "/AwardSpecialCup.bmd"; }
    virtual const char *getBtpFileName() { return "/AwardSpecialCup.btp"; }

private:
    u8 _pad[0xa8];
};

class TMapObjAwardCupReverse : public TMapObjAwardCup
{
public:
    TMapObjAwardCupReverse(u32 id = 0xd6b) : TMapObjAwardCup(id) {}
    virtual ~TMapObjAwardCupReverse() {}
    virtual const char *getBmdFileName() { return "/AwardReverseCup.bmd"; }
    virtual const char *getBtpFileName() { return "/AwardReverseCup.btp"; }

private:
    u8 _pad[0xa8];
};

class TMapObjAwardCupAllTour : public TMapObjAwardCup
{
public:
    TMapObjAwardCupAllTour(u32 id = 0xd6c) : TMapObjAwardCup(id) {}
    virtual ~TMapObjAwardCupAllTour() {}
    virtual const char *getBmdFileName() { return "/AwardAllCupTour.bmd"; }
    virtual const char *getBtpFileName() { return "/AwardAllCupTour.btp"; }

private:
    u8 _pad[0xa8];
};

#endif // MAPOBJAWARD_H
