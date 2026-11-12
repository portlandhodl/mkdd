#include "Sato/J3DEfctHeart.h"

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
};

static f32 sEfctScale = 1.2f;


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
};

static JPTraceHParticleCallBack JPTraceHPCB;

#include <JSystem/JAudio/JASFakeMatch2.h>

#define HEART_MAX_ASSERT(i, max)                                                                        \
    ((i) < (max)) ? (void)0                                                                             \
                  : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "StateObserver.h", 109,     \
                                                "range over: %d <= id=%d < %d", 0, (i), (max)),         \
                     OSPanic("StateObserver.h", 109, "Halt"))

inline void J3DEfctHeartStateObs::registStateFunc(const u8 &id, const StateFunc &initFunc, const StateFunc &doFunc)
{
    HEART_MAX_ASSERT(id, _f0);
    mInitFuncs[id] = initFunc;
    mDoFuncs[id] = doFunc;
}

void JPTraceHParticleCallBack::execute(JPABaseEmitter *emt, JPABaseParticle *ptcl)
{
    J3DEfctHeart *heart = (J3DEfctHeart *)emt->mpUserWork;
    JGeometry::TVec3f vel;
    ObjUtility::getKartVel(heart->mKartNo, &vel);
    JGeometry::TVec3f offset;
    JGeometry::TVec3f pos;
    ptcl->getOffsetPosition(pos);
    pos.add(vel);
    offset = heart->_128;
    ptcl->setOffsetPosition(pos);
}

void JPTraceHParticleCallBack::draw(JPABaseEmitter *, JPABaseParticle *) {}

J3DEfctHeart::J3DEfctHeart() {}

const char *J3DEfctHeart::getBmdFileName()
{
    return "/Item/heart.bmd";
}

void J3DEfctHeart::onlyinit()
{
    createStateFunc(this, 4);
    registStateFunc(0, &J3DEfctHeart::initFunc_Rolling, &J3DEfctHeart::doFunc_Rolling);
    registStateFunc(1, &J3DEfctHeart::initFunc_MoveToHand, &J3DEfctHeart::doFunc_MoveToHand);
    registStateFunc(2, &J3DEfctHeart::initFunc_Disappear, &J3DEfctHeart::doFunc_Disappear);
    registStateFunc(3, &J3DEfctHeart::initFunc_ColItemDisappear, &J3DEfctHeart::doFunc_ColItemDisappear);
}

void J3DEfctHeart::init()
{
    mState = 0;
    _140 = 0;
    resetState();
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
        JGeometry::TVec3f pos;
        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(mKartNo, &kartPos);
        pos.add(kartPos, _134);
        _160->mGlobalTrs.set(pos);

        if (_160->isEnableDeleteEmitter())
        {
            JPEffectMgr::getEffectMgr()->deleteEmt(_160);
            _160 = nullptr;
        }
    }
}

static inline void makeHeartRotY(Mtx m, s16 angle)
{
    f32 c = JMACosShort(angle);
    f32 s = JMASinShort(angle);
    m[1][1] = 1.0f;
    m[0][2] = s;
    m[0][0] = c;
    m[2][0] = -s;
    m[2][2] = c;
    m[2][1] = 0.0f;
    m[1][2] = 0.0f;
    m[1][0] = 0.0f;
    m[0][1] = 0.0f;
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
}

void J3DEfctHeart::initFunc_Rolling()
{
    _140 = (s16)(_f4 << 15);
    char *base = "mk_heart";

    for (u8 i = 0; i < 2; i++)
    {
        const char *name = ObjUtility::concatSimpleFileName(base, i);
        mEmitters[i] = JPEffectMgr::getEffectMgr()->createEmt(name, mPos);
        JPABaseEmitter *emt = mEmitters[i];
        if (emt != nullptr)
        {
            emt->mpUserWork = (s32)this;
            emt->mParticleCallback = &JPTraceHPCB;
        }
    }
}

void J3DEfctHeart::doFunc_Rolling()
{
    _140 += 1800;

    Mtx &body = ObjUtility::getKartBodyMtx(mKartNo);
    JGeometry::TPos3f bodyMtx;
    bodyMtx.set(body);
    bodyMtx.zeroTrans();

    Mtx mtx;
    Mtx rot;
    makeHeartRotY(rot, _140);
    PSMTXConcat(bodyMtx, rot, mtx);

    mPos.set(0.0f, 0.0f, sHeartOffsetParam.mOffset[0].x);
    PSMTXMultVec(mtx, &mPos, &mPos);
    _128 = mPos;

    makeHeartRotY(rot, _140 + 1800);
    PSMTXConcat(bodyMtx, rot, mtx);

    _104.set(0.0f, 0.0f, 150.0f);
    PSMTXMultVec(mtx, &_110, &_110);

    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mKartNo, &kartPos);
    mPos.add(kartPos);
    JGeometry::TVec3f up;
    up.set(body[0][2], body[1][2], body[2][2]);
    up.scale(sHeartOffsetParam.mOffset[0].y);
    mPos.add(up);

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
        changeState(2);
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
        changeState(2);
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
        changeState(2);
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
    _134.sub(mPos, kartPos);
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
    _134.sub(mPos, kartPos);
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
        changeState(2);
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
        changeState(1);
    }
}

void J3DEfctHeart::doPosLerp(JGeometry::TVec3f *out, const JGeometry::TVec3f &target)
{
    f32 timer = (f32)_ec;
    JGeometry::TVec3f start;
    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(mKartNo, &kartPos);
    start.add(kartPos, _134);
    f32 ratio = timer / (f32)_150;
    if (ratio > 1.0f)
        ratio = 1.0f;
    JMAVECLerp(&start, &target, out, ratio);
    out->y += _14c * timer + -0.5f * timer * timer;
}
