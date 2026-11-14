#ifndef MAPOBJPAKKUN_H
#define MAPOBJPAKKUN_H

#include <dolphin/types.h>
#include <JSystem/JGeometry/Vec.h>
#include <JSystem/JKernel/JKRDisposer.h>
#include <JSystem/JSupport/JSUList.h>

#include "Kaneshige/Course/CrsData.h"
#include "Sato/GeographyObj.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/AnmPlayer.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjPakkun;
class TMapObjPakkunColHead;

class TPakkunSupervisor : public GeoObjSupervisor
{
public:
    TPakkunSupervisor();
    virtual ~TPakkunSupervisor() {}

    void entry(TMapObjPakkun *);
    virtual void thunderDownAll();
    virtual void reset() {}
    virtual void calc() {}

    JSUList<TMapObjPakkun> mList;
};

class TMapObjPakkun : public TMapObjHioNode, public StateObserver, public JKRDisposer
{
public:
    TMapObjPakkun(const CrsData::SObject &);

    virtual ~TMapObjPakkun();

    virtual const char *getBmdFileName() { return "/Objects/Pakkun.bmd"; }
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void createColModel(J3DModelData *);
    virtual void reset();
    virtual void calc();
    virtual void MoveExec();
    virtual void InitExec();
    virtual void doKartColCallBack(int);
    virtual u32 getSoundID() const { return 0x10056; }
    virtual TPakkunSupervisor *getSupervisor() { return sSupervisor; }

    void changeDamageProc();
    u8 checkHeadDamage();
    void thunderDown()
    {
        mDamageAdd = (s16)(getGeoRnd()->get_ufloat_1() * sThunderDamageRandeMax);
        changeAllState(4);
    }
    void changeAllState(u16);
    void setRotMtx(int, Mtx);
    void setHeadPos(Mtx);
    void initFunc_Stand();
    void doFunc_Stand();
    void initFunc_Attack();
    void doFunc_Attack();
    void initFunc_Rest();
    void doFunc_Rest();
    void initFunc_Search();
    void doFunc_Search();
    void turnTo(const JGeometry::TVec3f &, f32);
    void setCallBack(const char *, int (*)(J3DJoint *, int));
    static int rotCallBack(J3DJoint *, int);
    static int headCallBack(J3DJoint *, int);
    void checkItemHitting();
    void initFunc_Damage();
    void doFunc_Damage();

    static TAnmInfo sAnmInfos[3];
    static StateObserver::StateFuncSet<TMapObjPakkun> sTable[5];
    static const u8 sAnmTable[5];
    static f32 sThunderDamageRandeMax;
    static f32 sTurnPow;
    static f32 sTurnPowFar;
    static f32 sTurnAttackScale;
    static s16 sRestFrame;
    static s16 sSearchFrame;
    static const char *scRootJointName;
    static const char *scHeadJointName;
    static JGeometry::TVec3f scHeadCenterOffset;
    static TPakkunSupervisor *sSupervisor;

    JSULink<TMapObjPakkun> mLink;
    TAnmPlayer mAnmPlayer;
    u16 mDamageFrame;
    u16 mDamageAdd;
    f32 mSearchRadius;
    f32 mAttackRadius;
    s16 mTargetKart;
    s16 mSearchTimer;
    JGeometry::TVec3f mDir;
    JGeometry::TPos3f mBaseMtx;
    JGeometry::TPos3f mInvBaseMtx;
    JGeometry::TPos3f mInvScaleMtx;
    TMapObjPakkunColHead *mHead;
};

#endif
