#include "Shiraiwa/Objects/MapObjKpFire.h"

#include "Shiraiwa/Coord3DUtil.h"

#include "JSystem/JParticle/JPAMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Yamamoto/kartCtrl.h"

const StateObserver::StateFuncSet<TMapObjKpFire> TMapObjKpFire::sTable[6] = {
    { 0, &TMapObjKpFire::initFunc_Start, &TMapObjKpFire::doFunc_Start },
    { 1, &TMapObjKpFire::initFunc_Walk, &TMapObjKpFire::doFunc_Walk },
    { 2, &TMapObjKpFire::initFunc_Stop, &TMapObjKpFire::doFunc_Stop },
    { 3, &TMapObjKpFire::initFunc_FireOpen, &TMapObjKpFire::doFunc_FireOpen },
    { 4, &TMapObjKpFire::initFunc_Fire, &TMapObjKpFire::doFunc_Fire },
    { 5, &TMapObjKpFire::initFunc_FireClose, &TMapObjKpFire::doFunc_FireClose },
};

const StateObserver::StateFuncSet<TMapObjKpFireBall> TMapObjKpFireBall::sTable[4] = {
    { 0, &TMapObjKpFireBall::initFunc_Hide, &TMapObjKpFireBall::doFunc_Hide },
    { 1, &TMapObjKpFireBall::initFunc_Appear, &TMapObjKpFireBall::doFunc_Appear },
    { 2, &TMapObjKpFireBall::initFunc_Fly, &TMapObjKpFireBall::doFunc_Fly },
    { 3, &TMapObjKpFireBall::initFunc_Disappear, &TMapObjKpFireBall::doFunc_Disappear },
};

f32 TMapObjKpFire::sAccel = 40.0f;
f32 TMapObjKpFire::sMaxSpeed = 40.0f;
f32 TMapObjKpFire::sShakeAccel = 40.0f;
f32 TMapObjKpFire::sShakeMaxSpeed = 50.0f;
s16 TMapObjKpFire::sShakeFrame = 25;
s16 TMapObjKpFire::sWaitFrame = 20;
s16 TMapObjKpFire::sFireInterval = 60;
u8 TMapObjKpFire::sFireLineNum = 1;
f32 TMapObjKpFire::sShakeMaxLength = 15000.0f;
const char *TMapObjKpFire::scFireJointName = "loc_fire";
int TMapObjKpFire::sFireJointNo;
J3DAnmTransform *TMapObjKpFire::sKpFireBckAnmTrans;
J3DMtxCalc *TMapObjKpFire::sKpFireBckMtxCalc;

J3DAnmTextureSRTKey *TMapObjKpFireBall::sKpFireBallBtkAnm;
J3DAnmTevRegKey *TMapObjKpFireBall::sKpFireBallTevRegKey;
s16 TMapObjKpFireBall::sMaxLife = 500;
s16 TMapObjKpFireBall::sAppearFrame = 1;
f32 TMapObjKpFireBall::sMaxSpeed = 30.0f;
f32 TMapObjKpFireBall::sVelocity0 = 140.0f;
f32 TMapObjKpFireBall::sDecelSpeed = -1.0f;
f32 TMapObjKpFireBall::sOrgScale = 1.0f;
f32 TMapObjKpFireBall::sRotRadRange = 0.05f;
f32 TMapObjKpFireBall::sRotRadMin = 0.1f;

TMapObjKpFire::TMapObjKpFire(const CrsData::SObject &obj) : TMapObjHioNode(obj), StateObserver()
{
    NewAnmCtrl();
    createSoundMgr();
    mPathUtil = new TPathUtil(&obj);
    for (int i = 0; i < 18; i++)
        mFire[i] = (TMapObjKpFireBall *)GetGeoObjMgr()->createSubObj(0x1260);
}

TMapObjKpFire::~TMapObjKpFire()
{
    delete mPathUtil;
}

void TMapObjKpFire::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/KpFire1.bck");
    J3DAnmObjTrans::setupTransAnmData(&sKpFireBckAnmTrans, &sKpFireBckMtxCalc, modelData, bck);
}

void TMapObjKpFire::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createModel(heap, viewNo, 0);
    AnmController *anmCtrl = getAnmCtrl();
    anmCtrl->mTrans = new AnmControlTrans();
    anmCtrl->mTrans->initAnm(1, &mModel);
    getAnmCtrl()->mTrans->registration(0, sKpFireBckAnmTrans, sKpFireBckMtxCalc);
    getAnmCtrl()->mTrans->getObjAnt(0)->mFrameCtrl.setAttribute(0);

    sFireJointNo = mModel.getModelData()->getJointName()->getIndex(scFireJointName);

    if (SiUtil::getConsoleNum() == 1) {
        sAccel = 40.0f;
        sMaxSpeed = 40.0f;
    } else {
        sAccel = 2.0f;
        sMaxSpeed = 10.0f;
    }
}

void TMapObjKpFire::reset()
{
    resetObject();
    StateObserver::ResetState();

    mPathUtil->reset();

    TPathUtil *path = mPathUtil;
    u16 pathID = path->mObj->pathID;
    CrsData *crs = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::PointData *p = crs->getPointData(pathID, 0);
    mPos.set(p->pos);

    mGeoObjFlag &= ~0x20;
    mGeoObjFlag &= ~0x2;
    mCheckKartHitFlags = 0;

    JGeometry::TVec3f dir(mRotMtx[0][2], mRotMtx[1][2], mRotMtx[2][2]);
    dir.normalize();

    getAnmCtrl()->Reset();
    getAnmCtrl()->mTrans->getObjAnt(0)->mFrameCtrl.setRate(0.0f);

    for (int i = 0; i < 18; i++)
        mFire[i]->setVelDir(dir);
}

void TMapObjKpFire::calc()
{
    StateObserver::ExecuteState();
}

void TMapObjKpFire::initFunc_Start() {}

void TMapObjKpFire::doFunc_Start()
{
    if (getStateCount() > mObjData->mParam2)
        setState(1);
}

void TMapObjKpFire::initFunc_Walk()
{
    TPathUtil *path = mPathUtil;
    path->mDistanceDirty = true;
    path->mNode = path->getNextNode();
}

void TMapObjKpFire::doFunc_Walk()
{
    TPathUtil *path = mPathUtil;
    u16 node = path->getNextNode();
    CrsData::PointData *p = RaceMgr::getManager()->getCourse()->getCrsData()->getPointData(path->mObj->pathID, node);

    JGeometry::TVec3f dir;
    dir.sub(p->pos, mPos);
    f32 dist = dir.length();

    if (dist < 50.0f) {
        if (checkFire(mPathUtil->getNextNode()))
            setState(2);
        else
            setState(1);
    }

    dir.normalize();
    mVel.scaleAdd(sAccel, dir);

    if (mVel.length() > sMaxSpeed) {
        mVel.normalize();
        mVel.scale(sMaxSpeed);
    }

    mPos.add(mVel);
    getSoundMgr()->setSe(0x40044);
}

void TMapObjKpFire::initFunc_Stop()
{
    getSoundMgr()->setSe(0x40038);
}

void TMapObjKpFire::doFunc_Stop()
{
    if (getStateCount() < sShakeFrame) {
        TPathUtil *path = mPathUtil;
        u16 node = path->getNextNode();
        CrsData::PointData *p = RaceMgr::getManager()->getCourse()->getCrsData()->getPointData(path->mObj->pathID, node);

        JGeometry::TVec3f dir;
        dir.sub(p->pos, mPos);

        if (dir.length() > JGeometry::TUtilf::epsilon()) {
            dir.normalize();
            mVel.scaleAdd(sShakeAccel, dir);
        }

        if (mVel.length() > sShakeMaxSpeed) {
            mVel.normalize();
            mVel.scale(sShakeMaxSpeed);
        }

        mPos.add(mVel);
    }

    if (getStateCount() > sShakeFrame + sWaitFrame) {
        mVel.zero();
        CrsData::PointData *p = RaceMgr::getManager()->getCourse()->getCrsData()->getPointData(mPathUtil->mObj->pathID, mPathUtil->getNextNode());
        mPos.set(p->pos);
        setState(3);
    }
}

bool TMapObjKpFire::checkFire(u16 node)
{
    CrsData::PointData *p = RaceMgr::getManager()->getCourse()->getCrsData()->getPointData(mObjData->pathID, node);
    return p->linkPoint == 0;
}

void TMapObjKpFire::initFunc_FireOpen()
{
    J3DAnmObjTrans *anm = getAnmCtrl()->mTrans->getObjAnt(0);
    anm->mFrameCtrl.setFrame(0.0f);
    anm->mFrameCtrl.setRate(1.0f);
}

void TMapObjKpFire::doFunc_FireOpen()
{
    if (getAnmCtrl()->mTrans->getObjAnt(0)->mFrameCtrl.getFrame() > 12.0f)
        setState(4);
}

void TMapObjKpFire::initFunc_Fire() {}

void TMapObjKpFire::doFunc_Fire()
{
    if (getStateCount() % sFireInterval != 0)
        return;

    u32 line = getStateCount() / sFireInterval;
    if (line < sFireLineNum) {
        for (int i = 0; i < 18; i++) {
            if (mFire[i]->getState() == 0) {
                JGeometry::TVec3f pos;
                getFirePos(&pos);
                makeFireEmitter(pos);
                getSoundMgr()->setSe(0x40035);
                ObjUtility::executeShakeCamera(pos, sShakeMaxLength, 0.4f, 5000.0f);
                mFire[i]->fire(pos);
                break;
            } else if (i == 17) {
#line 326
                JUT_WARNING_F2("%s", "TMapObjKpFire: Too Many Fire Balls.\n");
            }
        }
    } else {
        setState(5);
    }
}

void TMapObjKpFire::makeFireEmitter(const JGeometry::TVec3f &pos)
{
    static const char *emName[] = { "mk_koopaz_a", "mk_koopaz_b" };
    static const u8 isCameraClip[] = { 1, 0 };
    static f32 emScale[3];
    static s8 init;
    if (!init) {
        emScale[0] = 8.5f;
        emScale[1] = 8.5f;
        emScale[2] = 8.5f;
        init = 1;
    }

    for (int i = 0; i < 2; i++) {
        JPABaseEmitter *emt;
        if (isCameraClip[i])
            emt = GetJPAMgr()->createEmtCameraClip(emName[i], pos);
        else
            emt = GetJPAMgr()->createEmt(emName[i], pos);

        if (emt) {
            JPASetRMtxfromMtx(mRotMtx, *(Mtx *)((u8 *)emt + 0x68));
            emt->mGlobalScl.x = emScale[0];
            emt->mGlobalScl.y = emScale[1];
            emt->mGlobalScl.z = emScale[2];
            f32 y = emScale[1];
            emt->mGlobalPScl.x = emScale[0];
            emt->mGlobalPScl.y = y;
        }
    }
}

void TMapObjKpFire::getFirePos(JGeometry::TVec3f *pos)
{
    u8 *model = (u8 *)mModel.getModel();
    u8 *mtxBuf = *(u8 **)(model + 0x84);
    u8 *jointMtx = *(u8 **)(mtxBuf + 0xc) + sFireJointNo * 0x30;

    pos->set(*(f32 *)(jointMtx + 0xc), *(f32 *)(jointMtx + 0x1c), *(f32 *)(jointMtx + 0x2c));
}

void TMapObjKpFire::initFunc_FireClose() {}

void TMapObjKpFire::doFunc_FireClose()
{
    if (getAnmCtrl()->mTrans->getObjAnt(0)->mFrameCtrl.mState & 1)
        setState(1);
}

void TMapObjKpFire::InitExec()
{
    const StateFuncSet<TMapObjKpFire> *func = FindStateFunc(getState(), sTable, 6);
    (this->*func->mInitFunc)();
}

void TMapObjKpFire::MoveExec()
{
    const StateFuncSet<TMapObjKpFire> *func = FindStateFunc(getState(), sTable, 6);
    (this->*func->mExecFunc)();
}

TMapObjKpFireBall::TMapObjKpFireBall() : TMapObjHioNode(0x1260), StateObserver()
{
    createSoundMgr();
}

TMapObjKpFireBall::~TMapObjKpFireBall() {}

void TMapObjKpFireBall::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x01000200, false);
    mBtk.setExModel(&mModel);
    mBtk.mAnmBase = sKpFireBallBtkAnm;
    mBtk.initFrameCtrl(mBtk.mAnmBase);
    mBrk.setExModel(&mModel);
    mBrk.mAnmBase = sKpFireBallTevRegKey;
    mBrk.initFrameCtrl(mBrk.mAnmBase);
}

void TMapObjKpFireBall::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *btk = ObjUtility::getPtrCourseArc("/Objects/KpFire1Fire.btk");
    J3DAnmObjMaterial::setupTexSRTAnmData(&sKpFireBallBtkAnm, modelData, btk);
    void *brk = ObjUtility::getPtrCourseArc("/Objects/KpFire1Fire.brk");
    J3DAnmObjMaterial::setupTevRegAnmData(&sKpFireBallTevRegKey, modelData, brk);
}

void TMapObjKpFireBall::InitExec()
{
    const StateFuncSet<TMapObjKpFireBall> *func = FindStateFunc(getState(), sTable, 4);
    (this->*func->mInitFunc)();
}

void TMapObjKpFireBall::MoveExec()
{
    const StateFuncSet<TMapObjKpFireBall> *func = FindStateFunc(getState(), sTable, 4);
    (this->*func->mExecFunc)();
}

void TMapObjKpFireBall::setVelDir(const JGeometry::TVec3f &dir)
{
    JGeometry::TVec3f v;

    mVel.set(dir.x, dir.y, dir.z);
    v.set(dir.x, dir.y, dir.z);
    v.normalize();

    JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f right;
    right.cross(up, v);
    right.normalize();

    mRotMtx[0][0] = right.x;
    mRotMtx[0][1] = up.x;
    mRotMtx[0][2] = v.x;
    mRotMtx[1][0] = right.y;
    mRotMtx[1][1] = up.y;
    mRotMtx[1][2] = v.y;
    mRotMtx[2][0] = right.z;
    mRotMtx[2][1] = up.z;
    mRotMtx[2][2] = v.z;
}

void TMapObjKpFireBall::reset()
{
    resetObject();
    StateObserver::ResetState();

    mGeoObjFlag &= ~0x20;
    mGeoObjFlag &= ~0x2;
    mCheckKartHitFlags = 0;
    _58 = 13;

    mBtk.mFrameCtrl.setFrame((f32)mBtk.mFrameCtrl.mStartFrame);
    mBtk.mFrameCtrl.setRate(1.0f);
    mBtk.mFrameCtrl.mState = 0;

    mBrk.mFrameCtrl.setFrame((f32)mBrk.mFrameCtrl.mStartFrame);
    mBrk.mFrameCtrl.setRate(1.0f);
    mBrk.mFrameCtrl.mState = 0;

    mEmitter = nullptr;
}

void TMapObjKpFireBall::calc()
{
    StateObserver::ExecuteState();
    checkItemHitting();
    mBtk.mFrameCtrl.update();
    mBrk.mFrameCtrl.update();

    if (mEmitter) {
        JGeometry::TPos3f mtx;
        mtx.set(mRotMtx);
        mtx[0][3] = mPos.x;
        mtx[1][3] = mPos.y;
        mtx[2][3] = mPos.z;
        JPASetRMtxTVecfromMtx(mtx, *(Mtx *)((u8 *)mEmitter + 0x68), (JGeometry::TVec3f *)((u8 *)mEmitter + 0xa4));
    }
}

void TMapObjKpFireBall::checkItemHitting()
{
    if ((mGeoObjFlag & 0x4) == 0)
        return;

    switch (mColItemObj->getKind()) {
    case 7:
    case 1:
        makeBreakEmitter(3);
        setState(3);
        break;
    }
}

void TMapObjKpFireBall::update()
{
    mBtk.anmFrameProc();
    mBrk.anmFrameProc();
    GeographyObj::update();
}

void TMapObjKpFireBall::fire(const JGeometry::TVec3f &pos)
{
    mStartPos.set(pos);
    setState(1);
    SiUtil::setRandomStartFrame(&mBtk.mFrameCtrl, getGeoRnd());
}

void TMapObjKpFireBall::createColModel(J3DModelData *)
{
    createBoundsSphere(350.0f, 1.0f);
}

void TMapObjKpFireBall::doKartColCallBack(int kart)
{
    if (GetKartCtrl()->getKartBody(kart)->mCarStatus & 0x0004000000000000) {
        makeBreakEmitter(3);
        setState(3);
    }
}

void TMapObjKpFireBall::initFunc_Hide()
{
    mGeoObjFlag |= 0x20;
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~0x2;
}

void TMapObjKpFireBall::doFunc_Hide() {}

void TMapObjKpFireBall::initFunc_Appear()
{
    mGeoObjFlag &= ~0x20;
    mCheckKartHitFlags = -1;
    mGeoObjFlag |= 2;
    mScale.setAll(0.0f);
    mPos.set(mStartPos);
    mVel.normalize(sVelocity0);
    mEmitter = GetJPAMgr()->createEmtCameraClip("mk_kpbballTail", mPos);
}

void TMapObjKpFireBall::doFunc_Appear()
{
    f32 scale = SiUtil::getNormalRange((f32)getStateCount(), 0.0f, (f32)sAppearFrame) + 0.001f;
    if (scale >= 1.0f) {
        scale = 1.0f;
        setState(2);
    }

    mScale.setAll(scale * sOrgScale);
    mPos.add(mVel);
    getSoundMgr()->setSe(0x40034);
}

void TMapObjKpFireBall::initFunc_Fly() {}

void TMapObjKpFireBall::doFunc_Fly()
{
    mPos.add(mVel);

    if (getStateCount() > sMaxLife) {
        makeBreakEmitter(4);
        setState(3);
    }

    f32 speed = mVel.length();
    if (speed > sMaxSpeed) {
        speed += sDecelSpeed;
        if (speed < sMaxSpeed)
            speed = sMaxSpeed;
        mVel.normalize(speed);
    }

    f32 angle = SiUtil::getNormalRange(speed, sMaxSpeed, sVelocity0);
    angle = sRotRadRange * angle + sRotRadMin;

    Mtx rot;
    PSMTXIdentity(rot);
    f32 s = sin(angle);
    f32 c = cos(angle);
    rot[0][0] = c;
    rot[0][1] = -s;
    rot[0][2] = 0.0f;
    rot[1][0] = s;
    rot[1][1] = c;
    rot[1][2] = 0.0f;
    rot[2][0] = 0.0f;
    rot[2][1] = 0.0f;
    rot[2][2] = 1.0f;
    PSMTXConcat(mRotMtx, rot, mRotMtx);

    getSoundMgr()->setSe(0x40034);
}

void TMapObjKpFireBall::initFunc_Disappear()
{
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~0x2;
    if (mEmitter) {
        GetJPAMgr()->deleteEmt(mEmitter);
        mEmitter = nullptr;
    }
}

void TMapObjKpFireBall::makeBreakEmitter(int num)
{
    static const char *emName[] = { "mk_kpbball_a", "mk_kpbball_b", "mk_kpbball_c", "mk_kpbball_d" };
    static const u8 isCameraClip[] = { 1, 1, 1, 1 };
    static f32 emScale[3];
    static s8 init;
    if (!init) {
        emScale[0] = 5.0f;
        emScale[1] = 5.0f;
        emScale[2] = 5.0f;
        init = 1;
    }

    for (int i = 0; i < num; i++) {
        JPABaseEmitter *emt;
        if (isCameraClip[i])
            emt = GetJPAMgr()->createEmtCameraClip(emName[i], mPos);
        else
            emt = GetJPAMgr()->createEmt(emName[i], mPos);

        if (emt)
            emt->mGlobalScl.x = emScale[0];
            emt->mGlobalScl.y = emScale[1];
            emt->mGlobalScl.z = emScale[2];
            f32 y = emScale[1];
            emt->mGlobalPScl.x = emScale[0];
            emt->mGlobalPScl.y = y;
    }
}

void TMapObjKpFireBall::doFunc_Disappear()
{
    getSoundMgr()->setSe(0x40037);
    setState(0);
}

__declspec(weak) const char *TMapObjKpFireBall::getBmdFileName()
{
    return "/Objects/KPFire1Fire.bmd";
}

__declspec(weak) const char *TMapObjKpFire::getBmdFileName()
{
    return "/Objects/KPFire1.bmd";
}
