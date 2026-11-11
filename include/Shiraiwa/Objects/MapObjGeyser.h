#ifndef MAPOBJGEYSER_H
#define MAPOBJGEYSER_H

#include <dolphin/os.h>
#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/JGeometry.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/AnmPlayer.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Shiraiwa/TKartThrower.h"

class JPABaseEmitter;
class ItemObj;

class TMapObjGeyser : public TKartThrower, public StateObserver
{
public:
    TMapObjGeyser(const CrsData::SObject &);
    virtual ~TMapObjGeyser();

    virtual void MoveExec();
    virtual void InitExec();
    virtual const char *getBmdFileName();
    virtual void createColModel(J3DModelData *);
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void loadAnimation();

    inline JPABaseEmitter *getEmitterImpl(u32);
    JPABaseEmitter *getEmitter(u32);
    bool isActiveEmitter(u32);
    void playEmitter(u32, const JGeometry::TVec3f &);
    void moveEmitter(u32, const JGeometry::TVec3f &);
    void stopEmitter(u32);
    void hideEmitter(u32);
    void showEmitter(u32);
    void getEmitterPos(JGeometry::TVec3f *);

    virtual void update();
    virtual void reset();
    void changeAllState(u16);
    virtual void calc();
    virtual void viewCalc(u32);

    virtual void getThrowDir(JGeometry::TVec3f *, s16);
    virtual s32 getThrowPow();
    virtual void getKartThrowDirPow(JGeometry::TVec3f *, f32 *, int);
    virtual void getItemThrowDirPow(JGeometry::TVec3f *, f32 *, const ItemObj &);

    void initFunc_Start();
    void doFunc_Start();
    void initFunc_Wait();
    void doFunc_Wait();
    virtual bool checkChangeAct();
    void initFunc_Act();
    void doFunc_Act();
    void initFunc_JumpUp();
    void doFunc_JumpUp();
    void initFunc_JumpStay();
    void doFunc_JumpStay();
    void initFunc_JumpDown();
    void doFunc_JumpDown();

    static const char *scEffectName[6];                    // 0x803A6078
    static TAnmInfo sAnmInfos[4];                          // 0x803A6090
    static const StateObserver::StateFuncSet<TMapObjGeyser> sTable[6]; // 0x803A6170
    static const u8 sAnmTable[6];                          // 0x8041CBD8
    static s16 sActTime;                                   // 0x80415258
    static f32 sJumpUpStartVel;                            // 0x8041525C
    static u16 sCheckCycle;                                // 0x80415260
    static s16 sEfctABCDEndFrame;                          // 0x80415262
    static f32 sBoundHeight;                               // 0x80415264
    static const char *scJointName;                        // 0x80415268
    static s16 sEfctEFEndFrame;                            // 0x80416F00
    static int sJointNum;                                  // 0x80416F04
    static J3DAnmTextureSRTKey *sGeyserBtkAnm;             // 0x80416F08

    J3DAnmObjMaterial mBtk; // 0x158
    TAnmPlayer mAnmPlayer;  // 0x178
    s32 mActTime;           // 0x190
    s16 mAngle;             // 0x194
    u8 _pad196[2];          // 0x196
    f32 mUnk198;            // 0x198
    u16 mCycle;             // 0x19c
    u8 _pad19e[2];          // 0x19e
    JPABaseEmitter *mEmitters[6]; // 0x1a0
    JGeometry::TVec3f mColPos;    // 0x1b8
    f32 mColHeight;               // 0x1c4
}; // Size: 0x1c8

class TMapObjGeyserSearch : public TMapObjGeyser
{
public:
    virtual ~TMapObjGeyserSearch();

    virtual void reset();
    virtual bool checkChangeAct();

    static u16 sCheckCycle; // 0x8041526C
}; // Size: 0x1c8

#endif // MAPOBJGEYSER_H
