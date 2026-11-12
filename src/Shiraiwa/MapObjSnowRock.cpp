#include "Shiraiwa/Objects/MapObjSnowRock.h"

#include "JSystem/JMath/JMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ItemObj.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/ObjCollision.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/SiUtil.h"
#include "Yamamoto/kartCtrl.h"

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

StateObserver::StateFuncSet<TMapObjSnowRock> TMapObjSnowRock::sTable[3] = {
    { 0, &TMapObjSnowRock::initFunc_Stand, &TMapObjSnowRock::doFunc_Stand },
    { 1, &TMapObjSnowRock::initFunc_Break, &TMapObjSnowRock::doFunc_Break },
    { 2, &TMapObjSnowRock::initFunc_Die, &TMapObjSnowRock::doFunc_Die },
};

f32 TMapObjSnowRock::sGravity = 0.01f;
f32 TMapObjSnowRock::sStartVel = 70.0f;
TSnowRockSupervisor *TMapObjSnowRock::sSupervisor;

void TSnowRockSupervisor::calc()
{
    for (JSUListIterator<TMapObjSnowRock> it(mList.getFirst()); it != mList.getEnd(); it++) {
        for (JSUListIterator<TMapObjSnowRock> jt(it); jt != mList.getEnd(); jt++) {
            if (it != jt)
                it->hitCheck(jt.getObject());
        }
    }
}

void TSnowRockSupervisor::entry(TMapObjSnowRock *obj)
{
    mList.append(&obj->mLink);
}

TMapObjSnowRock::TMapObjSnowRock(const CrsData::SObject &obj)
    : TMapObjHioNode(obj), StateObserver(), JKRDisposer(), mLink(this)
{
    setObjFlagSimpleDraw();
    createSoundMgr();
    mGround = new CrsGround(RaceMgr::getManager()->getCourse());
    if (getSupervisor() == nullptr)
        sSupervisor = new TSnowRockSupervisor();
    sSupervisor->entry(this);
}

TMapObjSnowRock::~TMapObjSnowRock()
{
    delete mGround;
    sSupervisor = nullptr;
}

void TMapObjSnowRock::InitExec()
{
    Observer_FindAndInit(TMapObjSnowRock, 3);
}

void TMapObjSnowRock::MoveExec()
{
    Observer_FindAndExec(TMapObjSnowRock, 3);
}

const char *TMapObjSnowRock::getBmdFileName()
{
    static const char *cBmdName = "/Objects/SnowRock1.bmd";
    return cBmdName;
}

void TMapObjSnowRock::createColModel(J3DModelData *modelData)
{
    f32 xz = mScale.x > mScale.z ? mScale.x : mScale.z;
    f32 max = xz > mScale.y ? xz : mScale.y;
    createBoundsCylinder(modelData, 500.0f * (xz / max), 950.0f * (mScale.y / max));
}

void TMapObjSnowRock::doKartColCallBack(int kartNo)
{
    mSoundMgr->setSe(0x1005b);
    if (GetKartCtrl()->getKartBody(kartNo)->mCarStatus & 0x40000ull) {
        setState(1);
        return;
    }

    static JGeometry::TVec3f axisY(0.0f, 1.0f, 0.0f);

    JGeometry::TVec3f vec;
    ObjUtility::getKartPos(kartNo, &vec);
    setAttacked(vec, getBounds(0)->getBoundDepth());

    JGeometry::TVec3f kartVel;
    ObjUtility::getKartVel(kartNo, &kartVel);
    kartVel.normalize();

    vec.set(mVel);
    if (vec.dot(kartVel) > 0.8f) {
        JGeometry::TVec3f push;
        push.scaleAdd(-kartVel.dot(vec), kartVel, vec);
        if (push.isZero()) {
            push.cross(kartVel, axisY);
            push.normalize();
        }
        push.y = 0.0f;
        push.setLength(30.0f);
        mVel.add(push);
    }
}

void TMapObjSnowRock::setAttacked(const JGeometry::TVec3f &pos, f32 power)
{
    if (RaceMgr::getManager()->getRaceMode() == TIME_ATTACK)
        return;

    JGeometry::TVec3f dir;
    dir.sub(mPos, pos);
    dir.normalize();
    dir.scale(sStartVel * SiUtil::getNormalRange(power, 0.0f, 30.0f));
    mVel.add(dir);
}

void TMapObjSnowRock::reset()
{
    resetObject();
    setObjFlagCheckItemHitting();
    setAllCheckKartHitFlag();
    setItemColReaction(8, 0);
    setItemColReaction(7, 0);
    _58 = 19;
    mFric = mObjData->mParam1 * 0.001f;
    if (mFric == 0.0f)
        mFric = 0.99f;
    StateObserver::ResetState();
}

void TMapObjSnowRock::calc()
{
    StateObserver::ExecuteState();
}

void TMapObjSnowRock::initFunc_Stand()
{
    mVel.zero();
}

void TMapObjSnowRock::doFunc_Stand()
{
    JGeometry::TVec3f prevPos;
    prevPos.set(mPos);
    mVel.scale(mFric);
    mVel.y -= sGravity;
    mPos.add(mVel);
    mGround->search(mPos, prevPos);
    f32 height = mGround->getHeight();
    if (mPos.y < height) {
        mPos.y = height;
        mVel.y = 0.0f;
    }
    if (mGround->getAttribute() != CrsGround::Attr_4)
        setState(1);

    checkItemHitting();
    if (!mVel.isZero())
        mSoundMgr->setSeWithVelocityV(0x40031, SiUtil::getNormalRange(mVel.length(), 0.0f, 30.0f), 0.3f);
}

void TMapObjSnowRock::checkItemHitting()
{
    if (!tstItemHitting())
        return;

    ItemObj *item = mColItemObj;
    JGeometry::TVec3f pos;
    switch (item->getKind()) {
    case 7:
        pos = item->getPos();
        setAttacked(pos, getBounds(0)->getBoundDepth());
        break;
    case 1:
        pos = item->getPos();
        setAttacked(pos, item->getVel().length());
        break;
    case 8:
        setState(1);
        break;
    }
}

void TMapObjSnowRock::initFunc_Break()
{
    clrAllCheckKartHitFlag();
    clrObjFlagCheckItemHitting();
    makeBreakEmitter();
    mSoundMgr->setSe(0x40030);
    _58 = 0;
}

void TMapObjSnowRock::makeBreakEmitter()
{
    JPABaseEmitter *emt = JPEffectMgr::getEffectMgr()->createEmt("mk_icerockCrash_a", mPos);
    emt->setGlobalScale(mScale);
    emt = JPEffectMgr::getEffectMgr()->createEmt("mk_icerockCrash_b", mPos);
    emt->setGlobalScale(mScale);
    emt = JPEffectMgr::getEffectMgr()->createEmt("mk_icerockCrash_c", mPos);
    emt->setGlobalScale(mScale);
}

void TMapObjSnowRock::doFunc_Break()
{
    setState(2);
}

void TMapObjSnowRock::initFunc_Die()
{
    setObjFlagHidding();
}

void TMapObjSnowRock::doFunc_Die() {}

void TMapObjSnowRock::hitCheck(TMapObjSnowRock *other)
{
    JGeometry::TVec3f otherPos;
    other->getPosition(&otherPos);
    if (getBounds(0)->IsHitCylinder(mPos, otherPos, *(ObjColCylinder *)other->getBounds(0))) {
        JGeometry::TVec3f dir;
        dir.sub(mPos, otherPos);
        dir.normalize();
        dir.scale(sStartVel * SiUtil::getNormalRange(getBounds(0)->getBoundDepth(), 0.0f, 30.0f));
        mVel.add(dir);
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
