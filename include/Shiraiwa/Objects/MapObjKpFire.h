#ifndef MAPOBJKPFIRE_H
#define MAPOBJKPFIRE_H

#include "JSystem/J3D/J3DAnmTevRegKey.h"
#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/StateObserver.h"

class TPathUtil;

class TMapObjKpFireBall : public TMapObjHioNode, public StateObserver
{
public:
    TMapObjKpFireBall();
    virtual ~TMapObjKpFireBall();

    virtual void createModel(JKRSolidHeap *, u32);
    virtual void loadAnimation();
    virtual void reset();
    virtual void calc();
    virtual void update();
    virtual void createColModel(J3DModelData *);
    virtual void doKartColCallBack(int);
    virtual const char *getBmdFileName();

    void MoveExec();

    void InitExec();
    void setVelDir(const JGeometry::TVec3f &);
    void checkItemHitting();
    void fire(const JGeometry::TVec3f &);
    void initFunc_Hide();
    void doFunc_Hide();
    void initFunc_Appear();
    void doFunc_Appear();
    void initFunc_Fly();
    void doFunc_Fly();
    void initFunc_Disappear();
    void makeBreakEmitter(int);
    void doFunc_Disappear();

    static const StateObserver::StateFuncSet<TMapObjKpFireBall> sTable[4];

    static J3DAnmTextureSRTKey *sKpFireBallBtkAnm;
    static J3DAnmTevRegKey *sKpFireBallTevRegKey;
    static s16 sMaxLife;
    static s16 sAppearFrame;
    static f32 sMaxSpeed;
    static f32 sVelocity0;
    static f32 sDecelSpeed;
    static f32 sOrgScale;
    static f32 sRotRadRange;
    static f32 sRotRadMin;

    J3DAnmObjMaterial mBtk;       // 0x158
    J3DAnmObjMaterial mBrk;       // 0x178
    JGeometry::TVec3f mStartPos;  // 0x198
    JPABaseEmitter *mEmitter;     // 0x1a4
};

class TMapObjKpFire : public TMapObjHioNode, public StateObserver
{
public:
    TMapObjKpFire(const CrsData::SObject &);
    virtual ~TMapObjKpFire();

    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual const char *getBmdFileName();

    void initFunc_Start();
    void doFunc_Start();
    void initFunc_Walk();
    void doFunc_Walk();
    void initFunc_Stop();
    void doFunc_Stop();
    bool checkFire(u16);
    void initFunc_FireOpen();
    void doFunc_FireOpen();
    void initFunc_Fire();
    void doFunc_Fire();
    void makeFireEmitter(const JGeometry::TVec3f &);
    void getFirePos(JGeometry::TVec3f *);
    void initFunc_FireClose();
    void doFunc_FireClose();
    void MoveExec();
    void InitExec();

    static const StateObserver::StateFuncSet<TMapObjKpFire> sTable[6];

    static f32 sAccel;
    static f32 sMaxSpeed;
    static f32 sShakeAccel;
    static f32 sShakeMaxSpeed;
    static s16 sShakeFrame;
    static s16 sWaitFrame;
    static s16 sFireInterval;
    static u8 sFireLineNum;
    static f32 sShakeMaxLength;
    static const char *scFireJointName;
    static int sFireJointNo;
    static J3DAnmTransform *sKpFireBckAnmTrans;
    static J3DMtxCalc *sKpFireBckMtxCalc;

    TPathUtil *mPathUtil;            // 0x158
    TMapObjKpFireBall *mFire[18];    // 0x15c
};

#endif
