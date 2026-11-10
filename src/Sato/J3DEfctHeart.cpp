#include "Sato/J3DEfctHeart.h"

#include <JSystem/JAudio/JASFakeMatch2.h>
#include <JSystem/JGeometry.h>
#include <JSystem/JMath/JMath.h>
#include <JSystem/JUtility/JUTAssert.h>

#include "Sato/ItemHeart.h"
#include "Sato/ItemObj.h"
#include "Sato/J3DEffectMgr.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/ObjUtility.h"
#include "Sato/StateObserver.h"

struct HeartOffset
{
    f32 x;
    f32 y;
    f32 z;
};

struct HeartOffsetParam
{
    HeartOffset mOffset[14];
    J3DEfctHeart::StateFunc mFunc[8];
};

static f32 sEfctScale = 1.2f;

static u8 sStateFuncNum = 4;
static u8 sStateFuncIdx[4] = { 0, 1, 2, 3 };
static u8 sState_Disappear0 = 2;
static u8 sState_Disappear1 = 2;
static u8 sState_Disappear2 = 2;
static u8 sState_ColItemDisappear = 2;
static u8 sState_MoveToHand = 1;

static HeartOffsetParam sHeartOffsetParam = {
    {
        { 150.0f, 0.0f, 165.0f },
        { 20.0f, 150.0f, 15.0f },
        { 150.0f, 0.0f, 150.0f },
        { 0.0f, 150.0f, 0.0f },
        { 210.0f, 35.0f, 170.0f },
        { 15.0f, 150.0f, 0.0f },
        { 150.0f, 25.0f, 150.0f },
        { 15.0f, 150.0f, 0.0f },
        { 150.0f, 0.0f, 150.0f },
        { 0.0f, 160.0f, 0.0f },
        { 150.0f, 0.0f, 150.0f },
        { 0.0f, 150.0f, 0.0f },
        { 160.0f, -10.0f, 160.0f },
        { -10.0f, 190.0f, -10.0f },
    },
    {
        &J3DEfctHeart::initFunc_Rolling,
        &J3DEfctHeart::doFunc_Rolling,
        &J3DEfctHeart::initFunc_MoveToHand,
        &J3DEfctHeart::doFunc_MoveToHand,
        &J3DEfctHeart::initFunc_Disappear,
        &J3DEfctHeart::doFunc_Disappear,
        &J3DEfctHeart::initFunc_ColItemDisappear,
        &J3DEfctHeart::doFunc_ColItemDisappear,
    },
};

static JPTraceHParticleCallBack JPTraceHPCB;

namespace
{
struct HeartObs : public StateObserverT<J3DEfctHeart>
{
    void MoveExec() {}
    void InitExec() {}
};
HeartObs sObs;
}

static const char sAssertFile[] = "StateObserver.h";
static const char sAssertMsg[] = "range over: %d <= id=%d < %d";

#define HEART_MAX_ASSERT(i, max)                                                                          \
    ((i) < (max)) ? (void)0                                                                               \
                  : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), sAssertFile, 109, sAssertMsg, \
                                                0, (i), (max)),                                           \
                     OSPanic(sAssertFile, 109, "Halt"))

void JPTraceHParticleCallBack::execute(JPABaseEmitter *emt, JPABaseParticle *ptcl)
{
    J3DEfctHeart *heart = (J3DEfctHeart *)emt->mpUserWork;
    JGeometry::TVec3f vel;
    ObjUtility::getKartVel(heart->mKartNo, &vel);
    JGeometry::TVec3f off;
    ptcl->getOffsetPosition(off);
    off.add(off, vel);
    off.add(off, heart->_128);
    ptcl->setOffsetPosition(off);
}

void JPTraceHParticleCallBack::draw(JPABaseEmitter *, JPABaseParticle *) {}

J3DEfctHeart::J3DEfctHeart()
{
    _dc = 0;
    _de = 1;
    _f0 = 0;
    _ec = 0;
}

const char *J3DEfctHeart::getBmdFileName()
{
    return "/Item/heart.bmd";
}

void J3DEfctHeart::onlyinit()
{
    _f0 = sStateFuncNum;
    mSelf = this;
    mInitFuncs = new StateFunc[_f0];
    mDoFuncs = new StateFunc[_f0];
    HeartOffsetParam *param = &sHeartOffsetParam;
    _dd = 0;
    _dc = 0;
    _de = 1;
    _ec = 0;

    HEART_MAX_ASSERT(sStateFuncIdx[0], _f0);
    mInitFuncs[sStateFuncIdx[0]] = param->mFunc[0];
    mDoFuncs[sStateFuncIdx[0]] = param->mFunc[1];

    HEART_MAX_ASSERT(sStateFuncIdx[1], _f0);
    mInitFuncs[sStateFuncIdx[1]] = param->mFunc[1];
    mDoFuncs[sStateFuncIdx[1]] = param->mFunc[2];

    HEART_MAX_ASSERT(sStateFuncIdx[2], _f0);
    mInitFuncs[sStateFuncIdx[2]] = param->mFunc[2];
    mDoFuncs[sStateFuncIdx[2]] = param->mFunc[3];

    HEART_MAX_ASSERT(sStateFuncIdx[3], _f0);
    mInitFuncs[sStateFuncIdx[3]] = param->mFunc[3];
    mDoFuncs[sStateFuncIdx[3]] = param->mFunc[4];
}

void J3DEfctHeart::init()
{
    mState = 0;
    _140 = 0;
    _dd = 0;
    _dc = 0;
    _de = 1;
    _ec = 0;
    mEmitters[1] = nullptr;
    mEmitters[0] = nullptr;
    mPos.zero();
    _104.zero();
    _11c.zero();
    _110.zero();
    _160 = nullptr;

    JGeometry::TVec3f scale(sEfctScale, sEfctScale, sEfctScale);
    mModel.setBaseScale(scale);
}

void J3DEfctHeart::calc()
{
    if (_de)
    {
        _de = 0;
        (mSelf->*mInitFuncs[_dc])();
    }
    (mSelf->*mDoFuncs[_dc])();
    _ec++;

    if (_160 != nullptr)
    {
        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mKartNo, &kartPos);
        _160->mGlobalTrs.set(kartPos);

        if ((_160->mFlags & 0x10) && _160->getParticleNumber() == 0)
        {
            JPEffectMgr::getEffectMgr()->deleteEmt(_160);
            _160 = nullptr;
        }
    }
}

void J3DEfctHeart::initFunc_Rolling()
{
    _140 = _f4 << 15;

    for (u8 i = 0; i < 2; i++)
    {
        const char *name = ObjUtility::concatSimpleFileName((char *)"mk_heart", i);
        mEmitters[i] = JPEffectMgr::getEffectMgr()->createEmt(name, mPos);
        if (mEmitters[i] != nullptr)
        {
            mEmitters[i]->mpUserWork = (s32)this;
            mEmitters[i]->mParticleCallback = &JPTraceHPCB;
        }
    }
}

void J3DEfctHeart::doFunc_Rolling()
{
    _140 += 1800;

    Mtx &body = ObjUtility::getKartBodyMtx(mKartNo);
    Mtx bodyCopy;
    PSMTXCopy(body, bodyCopy);
    bodyCopy[0][3] = 0.0f;
    bodyCopy[1][3] = 0.0f;
    bodyCopy[2][3] = 0.0f;

    Mtx rot;
    f32 sin = JMath::sincosTable_.mTable[((u16)_140) >> 5].first;
    f32 cos = JMath::sincosTable_.mTable[((u16)_140) >> 5].second;
    rot[0][0] = cos;
    rot[0][1] = 0.0f;
    rot[0][2] = sin;
    rot[0][3] = 0.0f;
    rot[1][0] = 0.0f;
    rot[1][1] = 1.0f;
    rot[1][2] = 0.0f;
    rot[1][3] = 0.0f;
    rot[2][0] = -sin;
    rot[2][1] = 0.0f;
    rot[2][2] = cos;
    rot[2][3] = 0.0f;

    Mtx mtx;
    PSMTXConcat(bodyCopy, rot, mtx);

    mPos.set(0.0f, 0.0f, sHeartOffsetParam.mOffset[0].x);
    PSMTXMultVec(mtx, &mPos, &mPos);
    _128 = mPos;

    f32 sin2 = JMath::sincosTable_.mTable[((u16)(_140 + 1800)) >> 5].first;
    f32 cos2 = JMath::sincosTable_.mTable[((u16)(_140 + 1800)) >> 5].second;
    rot[0][0] = cos2;
    rot[0][2] = sin2;
    rot[2][0] = -sin2;
    rot[2][2] = cos2;
    PSMTXConcat(bodyCopy, rot, mtx);

    _104.set(0.0f, 0.0f, 150.0f);
    PSMTXMultVec(mtx, &_104, &_110);

    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mKartNo, &kartPos);
    mPos.add(mPos, kartPos);
    mPos.y += sHeartOffsetParam.mOffset[0].y;

    mMtx[0][3] = mPos.x;
    mMtx[1][3] = mPos.y;
    mMtx[2][3] = mPos.z;
    mModel.setBaseTRMtx(mMtx);

    for (u8 i = 0; i < 2; i++)
    {
        if (mEmitters[i] != nullptr)
        {
            mEmitters[i]->mGlobalTrs.set(mPos);
        }
    }
}

void J3DEfctHeart::initFunc_MoveToHand() {}

void J3DEfctHeart::doFunc_MoveToHand()
{
    if (mItem->mState != 9)
    {
        _dd = _dc;
        _dc = sState_Disappear0;
        _de = 1;
        _ec = 0;
        mHeartLink->mLostNum--;
        return;
    }

    doPosLerp(&mPos, mItem->mPos);
    mMtx[0][3] = mPos.x;
    mMtx[1][3] = mPos.y;
    mMtx[2][3] = mPos.z;
    mModel.setBaseTRMtx(mMtx);

    for (u8 i = 0; i < 2; i++)
    {
        if (mEmitters[i] != nullptr)
        {
            mEmitters[i]->mGlobalTrs.set(mPos);
        }
    }

    if (_ec >= _150 && _160 == nullptr)
    {
        _dd = _dc;
        _dc = sState_Disappear1;
        _de = 1;
        _ec = 0;
        mItem->setStateEquip(true, true);
        mHeartLink->mLostNum--;
    }
}

void J3DEfctHeart::initFunc_ColItemDisappear() {}

void J3DEfctHeart::doFunc_ColItemDisappear()
{
    _134.y += 5.0f;

    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mKartNo, &kartPos);

    mPos.add(kartPos, _134);
    mMtx[0][3] = mPos.x;
    mMtx[1][3] = mPos.y;
    mMtx[2][3] = mPos.z;

    f32 t = (f32)_ec;
    f32 scale = 1.0f - t / 60.0f;
    if (scale > 1.0f)
        scale = 1.0f;

    Mtx scaleMtx;
    PSMTXScale(scaleMtx, scale, scale, scale);
    Mtx mtx;
    PSMTXConcat(mMtx, scaleMtx, mtx);
    mModel.setBaseTRMtx(mtx);

    for (u8 i = 0; i < 2; i++)
    {
        if (mEmitters[i] != nullptr)
        {
            mEmitters[i]->mGlobalTrs.set(mPos);
            mEmitters[i]->setGlobalScale(scale);
        }
    }

    if (_ec > 60 && _160 == nullptr)
    {
        _dd = _dc;
        _dc = sState_Disappear2;
        _de = 1;
        _ec = 0;
        mHeartLink->mLostNum--;
    }
}

void J3DEfctHeart::initFunc_Disappear()
{
    mState = 1;
    for (u8 i = 0; i < 2; i++)
    {
        if (mEmitters[i] != nullptr)
        {
            JPEffectMgr::getEffectMgr()->deleteEmt(mEmitters[i]);
            mEmitters[i] = nullptr;
        }
    }
}

void J3DEfctHeart::doFunc_Disappear() {}

void J3DEfctHeart::update()
{
    mModel.update(0);
}

void J3DEfctHeart::setHitStartPos(const ItemObj &item)
{
    mPos = item.mPos;
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mKartNo, &kartPos);
    _134.mul(mPos, kartPos);
    _15c = 1;
    _160 = JPEffectMgr::getEffectMgr()->createEmt("mk_heartHit_a", mPos);
    if (_160 != nullptr)
    {
        if (TstEmitterModeNotAvailableFlg(mKartNo, _160))
        {
            _160 = nullptr;
            return;
        }

        f32 equipScale = item.mEquipScale;
        f32 scale = equipScale * item.getForceEfctOffsetScale();
        _160->mFlags |= 0x40;
        _160->setGlobalScale(scale);
    }
}

void J3DEfctHeart::initItemHit(const ItemObj &item)
{
    mPos = item.mPos;
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mKartNo, &kartPos);
    _134.mul(mPos, kartPos);
    _15c = 1;
    _160 = JPEffectMgr::getEffectMgr()->createEmt("mk_heartHit_a", mPos);
    if (_160 != nullptr)
    {
        if (TstEmitterModeNotAvailableFlg(mKartNo, _160))
        {
            _160 = nullptr;
        }
        else
        {
            f32 equipScale = item.mEquipScale;
            f32 scale = equipScale * item.getForceEfctOffsetScale();
            _160->mFlags |= 0x40;
            _160->setGlobalScale(scale);
        }
    }

    _150 = 50;
    _14c = 0.5f * (f32)_150;
}

void J3DEfctHeart::setMoveEfct(ItemObj *item, ItemHeart *heart)
{
    if (item == nullptr)
    {
        _dd = _dc;
        _dc = sState_Disappear0;
        _de = 1;
        _ec = 0;
        heart->mLostNum--;
        if (_160 != nullptr)
        {
            JPEffectMgr::getEffectMgr()->deleteEmt(_160);
            _160 = nullptr;
        }
    }
    else
    {
        mItem = item;
        mHeartLink = heart;
        _dd = _dc;
        _dc = sState_MoveToHand;
        _de = 1;
        _ec = 0;
    }
}

void J3DEfctHeart::doPosLerp(JGeometry::TVec3f *out, const JGeometry::TVec3f &target)
{
    f32 timer = (f32)_ec;
    JGeometry::TVec3f start;
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mKartNo, &kartPos);
    start = _134;
    f32 ratio = timer / (f32)_150;
    if (ratio > 1.0f)
        ratio = 1.0f;
    JMAVECLerp(&start, &target, out, ratio);
    out->y += _14c * timer - 0.5f * timer * timer;
}
