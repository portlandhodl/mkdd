#include "Sato/Objects/MapObjSanbo.h"

#include "Shiraiwa/SiUtil.h"
#include "Sato/AnmController.h"
#include "Sato/GeographyObj.h"
#include "Sato/ObjCollision.h"

class TMapObjSanbo;
class TSanboSupervisor;

class TMapObjSanbo : public GeographyObj
{
public:
    TMapObjSanbo(const CrsData::SObject &);
    virtual ~TMapObjSanbo();

    virtual const char *getBmdFileName();
    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void createColModel(J3DModelData *);
    virtual void reset();
    virtual void calc();
    virtual void InitExec();
    virtual void MoveExec();
    virtual void doKartColCallBack(int);
    virtual TSanboSupervisor *getSupervisor();
    virtual void initClassCreateNum() {}

    void checkItemHitting();
    void moveColOffset();
    void setBurst();
    void getJointPos(JGeometry::TVec3f *, JGeometry::TVec3f *, long);

    void initFunc_Start();
    void doFunc_Start();
    void initFunc_Stand();
    void doFunc_Stand();
    void initFunc_Prepare();
    void doFunc_Prepare();
    void initFunc_Attack();
    void doFunc_Attack();
    void initFunc_Down();
    void doFunc_Down();
    void initFunc_Recover();
    void doFunc_Recover();
    void initFunc_Die();
    void doFunc_Die();
    void initFunc_Hide();
    void doFunc_Hide();
    void initFunc_Born();
    void doFunc_Born();

    static TSanboSupervisor *sSupervisor;
    static StateObserver::StateFuncSet<TMapObjSanbo> sTable[10];
    static s16 sJointNum[5];
    static const char *scJointName[5];
    static s16 scAppearSoundFrame[5];
    static f32 sHeadOffset;
    static f32 sTailOffset;
    static s16 sThunderDownBonus;

public:
    u8 _14c[0x0c];
    u8 _158[0x18];
    JSULink<TMapObjSanbo> _170;
    TAnmPlayer mAnmPlayer;
    u8 _198;
    u8 _199;
    s16 mStandFrame;
    s16 mStartFrame;
    s16 mDownFrame;
    s16 mHideFrame;
    JGeometry::TPos3f mInvMtx;
    TMapObjSanboParts *mParts[5];
};

TSanboSupervisor *TMapObjSanbo::sSupervisor;

class TSanboSupervisor : public GeoObjSupervisor
{
public:
    TSanboSupervisor();
    virtual ~TSanboSupervisor();
    virtual void reset();
    virtual void calc();
    virtual void thunderDownAll();

    void entry(TMapObjSanbo *obj);

private:
    JSUList<TMapObjSanbo> mList;
};

TSanboSupervisor *TMapObjSanbo::getSupervisor() { return sSupervisor; }

const char *TMapObjSanbo::getBmdFileName() { return "/Objects/Sanbo1.bmd"; }

void TMapObjSanbo::initFunc_Start()
{
    mGeoObjFlag &= ~0x20;
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
}

void TMapObjSanbo::initFunc_Prepare() {}

void TMapObjSanbo::initFunc_Attack()
{
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->mFlags &= ~1;
    }
}

void TMapObjSanbo::initFunc_Down()
{
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->mFlags |= 1;
    }
}

void TMapObjSanbo::initFunc_Recover()
{
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->mFlags &= ~1;
    }
}

void TMapObjSanbo::initFunc_Die()
{
    setBurst();
    mGeoObjFlag |= 0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
}

void TMapObjSanbo::initFunc_Hide() {}

void TMapObjSanbo::initFunc_Born()
{
    mGeoObjFlag &= ~0x20;
    if (mAnmCtrl->mTrans != nullptr) {
        mAnmCtrl->mTrans->mFlags &= ~1;
    }
}

void TMapObjSanboParts::reset()
{
    resetObject();
    mGeoObjFlag |= 0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    PSMTXIdentity((Mtx &)_158);
    _188.zero();
    StateObserver::ResetState();
}

void TMapObjSanboParts::calc()
{
    StateObserver::ExecuteState();
}

void TMapObjSanboParts::initFunc_Hide()
{
    mGeoObjFlag |= 0x20;
}

void TMapObjSanboParts::doFunc_Hide() {}

void TMapObjSanboParts::initFunc_Burst()
{
    mScale.x = _188.x;
    mScale.y = _188.y;
    mScale.z = _188.z;
}

bool TMapObjSanboHead::acceptHidding()
{
    if (mPos.y + 120.0f < SiUtil::getCourseHeight(mPos)) return true;
    return false;
}

bool TMapObjSanboBody::acceptHidding()
{
    if (mPos.y + 80.0f < SiUtil::getCourseHeight(mPos)) return true;
    return false;
}

const char *TMapObjSanboBody::getBmdFileName() { return "/Objects/Sanbo1_break_body.bmd"; }

const char *TMapObjSanboHead::getBmdFileName() { return "/Objects/Sanbo1_break_head.bmd"; }

void TSanboSupervisor::reset() {}

void TSanboSupervisor::calc() {}

void TSanboSupervisor::entry(TMapObjSanbo *obj) { mList.append(&obj->_170); }
