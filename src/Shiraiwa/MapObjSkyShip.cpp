#include "Sato/Objects/MapObjMashBalloon.h"
#include "Shiraiwa/Objects/MapObjUFO.h"
#include "JSystem/JMath/JMath.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ObjUtility.h"
#include "mathHelper.h"
#include <math.h>

f32 TMapObjSkyShip::sAmplitude = 300.0f;
s16 TMapObjSkyShip::sAmpTimer = 100;

TMapObjSkyShip::TMapObjSkyShip(const CrsData::SObject &obj) : TMapObjHioNode(obj)
{
#line 28
    JUT_ASSERT_MSG(mObjData->pathID != 0xffff, "TMapObjSkyShip: Illegal Path ID");
    setObjFlagSimpleDraw();
    mPathMove = new TPathMove(&obj);
    mFreeRotate = new TFreeRotate();
}

TMapObjSkyShip::~TMapObjSkyShip()
{
    delete mPathMove;
    delete mFreeRotate;
}

void TMapObjSkyShip::reset()
{
    resetObject();
    clrObjFlagCheckGeoHitting();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    _58 = 0;

    f32 speed = 0.01f * mObjData->mParam1;
    if (speed == 0.0f) {
        speed = 10.0f;
    }

    if (IsPathMove()) {
        mPathMove->getNodePosition(&mPos, 0);
        mPathMove->setSpeed(speed, speed);
        mPathMove->init(&mPos, &mVel);
        mFreeRotate->init(&mRotMtx);

        JGeometry::TVec3f dir;
        getNodeDir(mPathMove->getNextNode(), &dir);
        dir.normalize();
        mRotMtx.setZDir(dir);
        mFreeRotate->setTargetVec(dir, 1.0f, 1.0f, 0.0f, 0x7a);
        mPathMove->setTargetNode();
        mVel.zero();
    }

    mBaseY = mPathY = mPos.y;
    mAmpAngle = 0;
}

const char *TMapObjSkyShip::getBmdFileName()
{
    static const char *cBmdName = "/Objects/SkyShip1.bmd";
    return cBmdName;
}

const char *TMapObjSkyShip::getShadowBmdFileName()
{
    static const char *cShadowBmdName = "/Objects/SkyShip1Shadow.bmd";
    return cShadowBmdName;
}

void TMapObjSkyShip::calc()
{
    if (IsPathMove()) {
        mPos.y = mPathY;
        if (!mPathMove->update()) {
            JGeometry::TVec3f dir;
            getNodeDir(mPathMove->getNextNode(), &dir);
            f32 speed = 10.0f / dir.length();
            dir.normalize();
            mFreeRotate->setTargetVec(dir, speed, speed, 0.0f, 0x7a);
            mPathMove->setTargetNode();
        }
        mFreeRotate->update();
        mPathY = mPos.y;
    }
    localMove();
    moveShadowRot();
}

void TMapObjSkyShip::moveShadowRot()
{
    if (mShadowMdl && !mShadowMdl->isInvisibleAll()) {
        CrsGround ground(RaceMgr::sRaceManager->getCourse());
        ground.search(mPos);

        Mtx mtx;
        PSMTXCopy(mRotMtx, mtx);
        mtx[0][3] = mPos.x;
        mtx[1][3] = ground.getHeight();
        mtx[2][3] = mPos.z;

        mShadowMdl->setBaseTRMtx(mtx);
        mShadowMdl->getModel()->setBaseScale(mScale);
    }
}

void TMapObjSkyShip::getNodeDir(u16 node, JGeometry::TVec3f *dir)
{
    JGeometry::TVec3f pos;
    mPathMove->getNodePosition(&pos, node);
    dir->sub(pos, mPos);
}

void TMapObjSkyShip::aimFront()
{
    JGeometry::TVec3f x, y, z;
    mRotMtx.getYDir(y);
    z = mVel;
    z.normalize();
    x.cross(y, z);
    x.normalize();
    mRotMtx.setXYZDir(x, y, z);
}

void TMapObjSkyShip::localMove()
{
    f32 ofs = getAmplitude() * JMath::sincosTable_.sinShort(mAmpAngle);
    mPos.y = mBaseY + ofs;
    mAmpAngle += getAmpTimer();
}

f32 TMapObjPathYoshiHeli::sAmplitude = 5.0f;
s16 TMapObjPathYoshiHeli::sAmpTimer = 7000;
f32 TMapObjPathYoshiHeli::sReachDistance = 1000.0f;
J3DAnmTextureSRTKey *TMapObjPathYoshiHeli::sPathYoshiHeliBtkAnm;

void TMapObjPathYoshiHeli::reset()
{
    resetObject();
    clrObjFlagCheckGeoHitting();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    _58 = 0;

    f32 speed = 0.01f * mObjData->mParam1;
    if (speed == 0.0f) {
        speed = 10.0f;
    }

    if (IsPathMove()) {
        mPathMove->getNodePosition(&mPos, 0);
        mPathMove->setSpeed(0.01f * speed, speed);
        mPathMove->_20 = sReachDistance * sReachDistance;
        mPathMove->init(&mPos, &mVel);

        JGeometry::TVec3f dir;
        getNodeDir(mPathMove->getNextNode(), &dir);
        dir.normalize();
        mRotMtx.setZDir(dir);
        mPathMove->setTargetNode();
        mVel.zero();
    }

    mBaseY = mPathY = mPos.y;
    mAmpAngle = 0;
    mAnmObj.mFrameCtrl.reset();
}

void TMapObjPathYoshiHeli::calc()
{
    if (IsPathMove()) {
        mPos.y = mPathY;
        if (!mPathMove->update()) {
            JGeometry::TVec3f dir;
            getNodeDir(mPathMove->getNextNode(), &dir);
            mPathMove->setTargetNode();
        }
        aimVelDir();
        mPathY = mPos.y;
    }
    localMove();
    moveShadowRot();
    mAnmObj.mFrameCtrl.update();
}

void TMapObjPathYoshiHeli::aimVelDir()
{
    JGeometry::TVec3f x, y, z;
    mRotMtx.getXDir(x);
    mRotMtx.getYDir(y);
    mRotMtx.getZDir(z);
    if (!mVel.isZero()) {
        z.set(mVel);
    }
    z.y = 0.0f;
    z.normalize();
    if (z.isZero()) {
        z.cross(x, y);
        z.normalize();
    }
    y.set(0.0f, 1.0f, 0.0f);
    x.cross(y, z);
    x.normalize();
    mRotMtx.setXYZDir(x, y, z);
}

void TMapObjPathYoshiHeli::update()
{
    mAnmObj.anmFrameProc();
    GeographyObj::update();
}

void TMapObjPathYoshiHeli::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    J3DAnmObjMaterial::setupTexSRTAnmData(&sPathYoshiHeliBtkAnm, modelData,
                                          ObjUtility::getPtrCourseArc("/Objects/YoshiHeliB.btk"));
}

void TMapObjPathYoshiHeli::createModel(JKRSolidHeap *, u32)
{
    mAnmObj.setExModel(&mModel);
    mAnmObj.attach(sPathYoshiHeliBtkAnm);
}

f32 TMapObjMashBalloon::sAmplitude = 500.0f;
s16 TMapObjMashBalloon::sAmpTimer = 100;

void TMapObjMashBalloon::reset()
{
    resetObject();
    clrObjFlagCheckGeoHitting();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    _58 = 0;
    mRotBase.set(mRotMtx);

    static f32 sMaxSpeed = 10.0f;
    static f32 sAccel = 0.03f;
    static f32 sReachDistance = 2000.0f;

    if (IsPathMove()) {
        mPathMove->getNodePosition(&mPos, 0);
        TPathMove *path = mPathMove;
        f32 maxSpeed = sMaxSpeed;
        f32 half = 0.5f * maxSpeed;
        path->_14 = sAccel;
        path->_18 = maxSpeed;
        path->_20 = half * half;
        mPathMove->_20 = sReachDistance * sReachDistance;
        mPathMove->init(&mPos, &mVel);
        mFreeRotate->init(&mRotBase);
        mPathMove->setTargetNode();
        mVel.zero();
    }

    mBaseY = mPathY = mPos.y;
    mAmpAngle = 0;
}

f32 TMapObjUFO::sRotRad = 0.06f;
f32 TMapObjUFO::sFlySpeed = 500.0f;
s16 TMapObjUFO::sWaitFrame = 120;

StateObserver::StateFuncSet<TMapObjUFO> TMapObjUFO::sTable[2] = {
    {0, &TMapObjUFO::initFunc_Fly, &TMapObjUFO::doFunc_Fly},
    {1, &TMapObjUFO::initFunc_Wait, &TMapObjUFO::doFunc_Wait},
};

#include <JSystem/JAudio/JASFakeMatch2.h>

TMapObjUFO::TMapObjUFO(const CrsData::SObject &obj) : TMapObjHioNode(obj), StateObserver()
{
#line 475
    JUT_ASSERT_MSG(mObjData->pathID != 0xffff, "TMapObjUFO: Illegal Path ID");
    setObjFlagSimpleDraw();
    mPathUtil = new TPathUtil(&obj);
    createSoundMgr();
}

TMapObjUFO::~TMapObjUFO()
{
    delete mPathUtil;
}

static inline void setRotY(Mtx m, f32 rad)
{
    f32 s = sin(rad);
    f32 c = cos(rad);
    m[1][1] = 1.0f;
    m[0][0] = c;
    m[0][2] = s;
    m[2][0] = -s;
    m[2][2] = c;
    m[0][1] = m[1][0] = m[1][2] = m[2][1] = 0.0f;
}

void TMapObjUFO::reset()
{
    resetObject();
    clrObjFlagCheckGeoHitting();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    clrObjFlagHidding();
    _58 = 0;

    PSMTXIdentity(mRotY);
    setRotY(mRotY, sRotRad);
    mPathUtil->reset();
}

void TMapObjUFO::InitExec()
{
    Observer_FindAndInit(TMapObjUFO, 2);
}

void TMapObjUFO::MoveExec()
{
    Observer_FindAndExec(TMapObjUFO, 2);
}

void TMapObjUFO::calc()
{
    ExecuteState();
    PSMTXConcat(mRotMtx, mRotY, mRotMtx);
    mSoundMgr->setSe(0x40081);
}

void TMapObjUFO::initFunc_Fly()
{
    TPathUtil *util = mPathUtil;
    util->mDistanceDirty = true;
    util->mNode = util->getNextNode();

    JGeometry::TVec3f target;
    target.set(RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mPathUtil->mObj->pathID, (u16)mPathUtil->mNode)->pos);
    JGeometry::TVec3f dir;
    dir.sub(target, mPos);
    dir.normalize();
    mVel.scale(sFlySpeed, dir);
}

void TMapObjUFO::doFunc_Fly()
{
    mPos.add(mVel);
    JGeometry::TVec3f target;
    target.set(RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mPathUtil->mObj->pathID, (u16)mPathUtil->mNode)->pos);
    JGeometry::TVec3f dir;
    dir.sub(target, mPos);
    if (dir.dot(mVel) <= 0.0f) {
        mPos.set(target);
        setState(1);
    }
    mSoundMgr->setSe(0x40080);
}

void TMapObjUFO::initFunc_Wait()
{
    if (RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mObjData->pathID, mPathUtil->mNode)->linkPoint == 0) {
        setState(0);
    }
}

void TMapObjUFO::doFunc_Wait()
{
    if (getStateCount() > sWaitFrame) {
        setState(0);
    }
}
