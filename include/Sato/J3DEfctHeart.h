#ifndef J3DEFCTHEART_H
#define J3DEFCTHEART_H

#include <JSystem/JParticle/JPAEmitter.h>
#include "Sato/J3DEffectMgr.h"

class ItemObj;
class ItemHeart;

class JPTraceHParticleCallBack : public JPAParticleCallBack
{
public:
    void execute(JPABaseEmitter *, JPABaseParticle *);
    void draw(JPABaseEmitter *, JPABaseParticle *);
    virtual ~JPTraceHParticleCallBack() {} // 0x80288ba4
};

class J3DEfctHeart;

// Layout and constructor of TStateObserver<J3DEfctHeart>; field names kept for ItemHeart.cpp
struct J3DEfctHeartStateObs
{
    typedef void (J3DEfctHeart::*StateFunc)();

    J3DEfctHeartStateObs() : _dc(0), _de(1), _ec(0), _f0(0) {}

    void createStateFunc(J3DEfctHeart *owner, const u8 &num)
    {
        _f0 = num;
        mSelf = owner;
        mInitFuncs = new StateFunc[_f0];
        mDoFuncs = new StateFunc[_f0];
        resetState();
    }

    void resetState()
    {
        _dd = 0;
        _dc = 0;
        _de = 1;
        _ec = 0;
    }

    void changeState(const u8 &state)
    {
        _dd = _dc;
        _dc = state;
        _de = 1;
        _ec = 0;
    }

    void registStateFunc(const u8 &id, const StateFunc &initFunc, const StateFunc &doFunc);

    u8 _dc;                  // dc
    u8 _dd;                  // dd
    u8 _de;                  // de
    J3DEfctHeart *mSelf;     // e0
    StateFunc *mInitFuncs;   // e4
    StateFunc *mDoFuncs;     // e8
    u32 _ec;                 // ec
    u8 _f0;                  // f0
};

class J3DEfctHeart : public J3DEffectObj, public J3DEfctHeartStateObs
{
public:
    typedef void (J3DEfctHeart::*StateFunc)();

    J3DEfctHeart();                                                             // 0x80287b74
    static void loadAnmData() {}
    static const char *getBmdFileName();                                        // 0x80287be8
    void onlyinit();                                                            // 0x80287bf4
    void init();                                                                // 0x80287ef8
    void calc();                                                                // 0x80287f94
    void initFunc_Rolling();                                                    // 0x802880a8
    void doFunc_Rolling();                                                      // 0x80288144
    void initFunc_MoveToHand();                                                 // 0x802883e4
    void doFunc_MoveToHand();                                                   // 0x802883e8
    void initFunc_ColItemDisappear();                                           // 0x80288524
    void doFunc_ColItemDisappear();                                             // 0x80288528
    void initFunc_Disappear();                                                  // 0x802886c0
    void doFunc_Disappear();                                                    // 0x8028872c
    void update();                                                              // 0x80288730
    void setHitStartPos(const ItemObj &);                                       // 0x80288760
    void initItemHit(const ItemObj &);                                          // 0x80288864
    void setMoveEfct(ItemObj *, ItemHeart *);                                   // 0x80288998
    void doPosLerp(JGeometry::TVec3f *, const JGeometry::TVec3f &); // 0x80288a3c
    // Inline/Unused
    // void StateObserverT<J3DEfctHeart>::~StateObserverT();
    // Inline
    virtual ~J3DEfctHeart() {} // 0x80288b24

    void set_dc(const u8 &val)
    {
        _dc = val;
    }

    u8 _f4;                  // f4
    u8 _f5[0xf8 - 0xf5];     //
    JGeometry::TVec3f mPos;   // f8
    JGeometry::TVec3f _104;  // 104
    JGeometry::TVec3f _110;  // 110
    JGeometry::TVec3f _11c;  // 11c
    JGeometry::TVec3f _128;  // 128
    JGeometry::TVec3f _134;  // 134
    s16 _140;                // 140
    u8 _142[0x144 - 0x142];  //
    JPABaseEmitter *mEmitters[2]; // 144
    f32 _14c;                // 14c
    u32 _150;                // 150
    ItemObj *mItem;          // 154
    ItemHeart *mHeartLink;   // 158
    u8 _15c;                 // 15c
    u8 _15d[0x160 - 0x15d];  //
    JPABaseEmitter *_160;    // 160
}; // 0x164

#endif // J3DEFCTHEART_H
