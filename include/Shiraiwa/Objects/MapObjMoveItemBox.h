#ifndef MAPOBJMOVEITEMBOX_H
#define MAPOBJMOVEITEMBOX_H

#include "Sato/Objects/GeoItemBox.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/Coord3D.h"

class TMapObjMoveItemBox : public GeoItemBox, public StateObserver
{
public:
    TMapObjMoveItemBox(const CrsData::SObject &);
    virtual ~TMapObjMoveItemBox();

    virtual void setOpeningPos();
    virtual void MoveExec();
    virtual void InitExec();
    virtual void move();
    virtual void resetPosition();

    virtual void reset();
    virtual void calc();

    void initFunc_Appear();
    void doFunc_Appear();
    void initFunc_Roll();
    void doFunc_Roll();
    void initFunc_Disappear();
    void doFunc_Disappear();
    void initFunc_Hide();
    void doFunc_Hide();
    void initFunc_RaceEnd();
    void doFunc_RaceEnd();

    static StateObserver::StateFuncSet<TMapObjMoveItemBox> sTable[5];

    static f32 sHeight;
    static f32 sRollSpeed;
    static s16 sAppearFrame;
    static s16 sDisappearFrame;
    static f32 sGravitySize;
    static f32 sReflectRate;
    static f32 sAirFriction;
    static f32 sFriction;
    static f32 sJumpPow;

    TFreeFallShakeSky *mFreeFall;   // 0x214
    JPABaseEmitter *mEmitter[2];    // 0x218
}; // Size: 0x220

class TMapObjMoveItemBoxLimit : public TMapObjMoveItemBox
{
public:
    TMapObjMoveItemBoxLimit(const CrsData::SObject &obj) : TMapObjMoveItemBox(obj) {
        createSoundMgr();
    }
    virtual ~TMapObjMoveItemBoxLimit() {}

    virtual void reset();
    virtual void move();
    virtual void resetPosition();
    virtual void calc();
    virtual void doKartColCallBack(int);

    void fixMoveSphere();

    static f32 sGravitySize;

    JGeometry::TVec3f mMin; // 0x220
    JGeometry::TVec3f mMax; // 0x22c
}; // Size: 0x238

#endif
