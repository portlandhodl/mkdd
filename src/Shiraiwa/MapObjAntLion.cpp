#include "JSystem/JParticle/JPAMath.h"
#include "Sato/AnmController.h"
#include "Sato/ItemThunder.h"
#include "Sato/J3DAnmObject.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/Objects/MapObjAntLion.h"

#include <JSystem/JParticle/JPAEmitter.h>

const char *TMapObjAntLion::scJointName = "skl_loot";
J3DAnmTransform *TMapObjAntLion::sAntLionBckAnmTrans;
J3DMtxCalc *TMapObjAntLion::sAntLionBckMtxCalc;
int TMapObjAntLion::sJointNo;

static const char *emtName[2] = {
    "mk_arijigoku_paku_a",
    "mk_arijigoku_paku_b",
};

StateObserver::StateFuncSet<TMapObjAntLion> TMapObjAntLion::sTable[3] = {
    {0, &TMapObjAntLion::initFunc_Start, &TMapObjAntLion::doFunc_Start},
    {1, &TMapObjAntLion::initFunc_Wait, &TMapObjAntLion::doFunc_Wait},
    {2, &TMapObjAntLion::initFunc_Eat, &TMapObjAntLion::doFunc_Eat},
};

TMapObjAntLion::TMapObjAntLion(const CrsData::SObject &obj) : TMapObjHioNode(obj), StateObserver()
{
    NewAnmCtrl();
    createSoundMgr();
}

TMapObjAntLion::~TMapObjAntLion()
{
    delete mCircleSoundMgr;
}

const char *TMapObjAntLion::getBmdFileName()
{
    static const char *cBmdName = "/Objects/AntLion.bmd";
    return cBmdName;
}

void TMapObjAntLion::createColModel(J3DModelData *modelData)
{
    createBoundsCylinder(1500.0f, 600.0f, 1.0f);

    f32 y = (f32)mObjData->mParam1 / mScale.y;
    JGeometry::TVec3f pos;
    pos.x = 0.0f;
    pos.z = 0.0f;
    pos.y = y;
    setColObjPos(pos, 0);
}

void TMapObjAntLion::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/AntLion_eat.bck");
    J3DAnmObjTrans::setupTransAnmData(&sAntLionBckAnmTrans, &sAntLionBckMtxCalc, modelData, bck);
}

void TMapObjAntLion::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createModel(heap, viewNo, 0);
    AnmController *anmCtrl = mAnmCtrl;
    anmCtrl->InitRegistration(1, &mModel);
    mAnmCtrl->RegisterTrans(0, sAntLionBckAnmTrans, sAntLionBckMtxCalc);

    register J3DJoint *joint = mModel.getModelData()->getJointNodePointer(0);
    mCircleSoundMgr = new GameAudio::CircleSoundMgr((Vec *)&mPos, mScale.y * joint->getRadius(), 0.0f, 0.0f, nullptr);

    sJointNo = mModel.getModelData()->getJointName()->getIndex(scJointName);
}

void TMapObjAntLion::InitExec()
{
    const StateFuncSet<TMapObjAntLion> *func = FindStateFunc(getState(), sTable, 3);
    (this->*func->mInitFunc)();
}

void TMapObjAntLion::MoveExec()
{
    const StateFuncSet<TMapObjAntLion> *func = FindStateFunc(getState(), sTable, 3);
    (this->*func->mExecFunc)();
}

void TMapObjAntLion::reset()
{
    resetObject();
    mGeoObjFlag &= ~0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = -1;
    _58 = 17;
    mAnmCtrl->Reset();
    StateObserver::ResetState();
}

void TMapObjAntLion::calc()
{
    StateObserver::ExecuteState();
    mCircleSoundMgr->setSe(0x40025);
    mCircleSoundMgr->frameWork();
}

void TMapObjAntLion::initFunc_Start()
{
    JGeometry::TVec3f pos;
    pos.zero();
    GetJPAMgr()->createEmt("mk_arijigoku_a", pos);
}

void TMapObjAntLion::doFunc_Start()
{
    setState(1);
}

void TMapObjAntLion::initFunc_Wait()
{
#line 127
#line 127
    J3DAnmObjTrans *obj = mAnmCtrl->mTrans->getObjAnt(0);
    obj->setFrame(0.0f);
    if (mAnmCtrl->IsAvailableTrans())
        mAnmCtrl->mTrans->setFlag();
}

void TMapObjAntLion::doFunc_Wait() {}

void TMapObjAntLion::initFunc_Eat()
{
#line 127
    int i;
    J3DAnmObjTrans *obj = mAnmCtrl->mTrans->getObjAnt(0);
    obj->setFrame((f32)obj->mFrameCtrl.mStartFrame);
    obj->mFrameCtrl.setRate(0.0f);
    obj->mFrameCtrl.mState = 0;
    if (mAnmCtrl->IsAvailableTrans())
        mAnmCtrl->mTrans->resetFlag();

    mSoundMgr->setSe(0x4002e);

    JGeometry::TPos3f rotMtx;
    rotMtx.set(mRotMtx);
    rotMtx[0][3] = 0.0f;
    rotMtx[1][3] = 0.0f;
    rotMtx[2][3] = 0.0f;

    u8 *model = (u8 *)mModel.getModel();
    u8 *mtxBuf = *(u8 **)(model + 0x84);
    u8 *jointMtx = *(u8 **)(mtxBuf + 0x0c) + sJointNo * 0x30;

    JGeometry::TVec3f pos;
    f32 y, z, x;
    z = *(f32 *)(jointMtx + 0x2c);
    y = *(f32 *)(jointMtx + 0x1c);
    x = *(f32 *)(jointMtx + 0x0c);
    pos.set(x, y, z);

    for (i = 0; i < 2; i++)
    {
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(emtName[i], pos);
        JPASetRMtxfromMtx(*(Mtx *)&rotMtx, *(Mtx *)((u8 *)emt + 0x68));
    }
}

void TMapObjAntLion::doFunc_Eat()
{
    if (mAnmCtrl->mTrans->getObjAnt(0)->mFrameCtrl.mState & 1)
    {
        setState(1);
    }
}

void TMapObjAntLion::getZDir(JGeometry::TVec3f *dir)
{
    f32 z = mRotMtx[2][2];
    f32 y = mRotMtx[1][2];
    f32 x = mRotMtx[0][2];
    dir->set(x, y, z);
    f32 sq = dir->squared();
    if (sq <= JGeometry::TUtilf::epsilon())
        return;
    f32 invsqrt = JGeometry::TUtilf::inv_sqrt(sq);
    dir->scale(invsqrt);
}

void TMapObjAntLion::doKartColCallBack(int) {}

#include "JSystem/JAudio/JASFakeMatch2.h"
