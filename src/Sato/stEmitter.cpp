#include "Sato/stEmitter.h"
#include "Sato/stEffectMgr.h"
#include "Sato/ObjUtility.h"
#include "Yamamoto/kartCtrl.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "dolphin/gx/GXPixel.h"

typedef void (stEmitterParamLineIP::*IpFunc)();

void stEmitterCallBack::setIpData(stFileParamPtrList *list)
{
    stEmitter *emt1 = list->_04;
    ipData._00 = list->_00;
    ipData._04 = emt1;
    _04->mParticle.mDraw.mTexture = (JUTTexture *)list->_08;
    *(stEmitter *)_04 = *list->_00;
    ipData._08 = _04;
}

bool stEmitterCallBack::getTirePos(JGeometry::TVec3f *tirePos)
{
    bool ret = false;
    short kart = kartNum;
    if (_22 == 2)
        ret = KartCtrl::getKartCtrl()->GetLeftTirePos(kart, tirePos);
    else if (_22 == 3)
        ret = KartCtrl::getKartCtrl()->GetRightTirePos(kart, tirePos);
    return ret;
}

bool stEmitterCallBack::getDriftPos(JGeometry::TVec3f *driftPos, const JGeometry::TVec3f &offset)
{
    bool ret = getTirePos(driftPos);
    JGeometry::TVec3f ydir;
    JGeometry::TVec3f zdir;
    ObjUtility::getKartZdir(kartNum, &zdir);
    ObjUtility::getKartYdir(kartNum, &ydir);
    JGeometry::TVec3f ofs(offset);
    ofs.scale(GetKartCtrl()->getKartBody(kartNum)->getThunder()->getScale());
    ydir.scale(ofs.y);
    zdir.scale(ofs.z);
    driftPos->add(ydir);
    driftPos->add(zdir);
    return ret;
}

static inline void stSetEmtPos(stEmitter *e, JGeometry::TVec3f p) { e->_0c = p; }
static inline void stSetEmtDir(stEmitter *e, JGeometry::TVec3f p) { e->mFirstVelDir = p; }
static inline void stSetEmtVel(stEmitter *e, JGeometry::TVec3f p) { e->_00 = p; }

void stEmitterCallBack::calc_lineIp(f32 val, f32 ratio, f32 max, f32 min, f32 dif, bool noDir, f32 rate)
{
    stEmitterPtcl *emt = _04;
    JGeometry::TVec3f vel;
    ObjUtility::getKartEfctVel(kartNum, &vel);
    if (val < min && rate < 1.0f) {
        emt->mFlags |= 0x400;
        vel = emt->_00;
        if (vel.y > 0.0f)
            vel.y = 0.0f;
        vel.scale(rate);
    } else {
        JGeometry::TVec3f pos;
        JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
        Mtx &mtx = ObjUtility::getKartBodyMtx(kartNum);
        if ((u8)getGeneratePos(&pos, mtx)) {
            stSetEmtPos(emt, pos);
            vel.scale(ratio);
            if (!noDir) {
                up.x = mtx[0][1];
                up.y = mtx[1][1];
                up.z = mtx[2][1];
                stSetEmtDir(emt, up);
            }
            calcParam(val, max, min, dif);
            emt->mFlags &= ~0x400;
        } else {
            emt->mFlags |= 0x400;
            if (rate >= 1.0f) {
                stSetEmtPos(emt, pos);
                if (rate >= 2.0f)
                    vel.scale(1.0f);
                else
                    vel.scale(ratio);
                if (!noDir) {
                    up.x = mtx[0][1];
                up.y = mtx[1][1];
                up.z = mtx[2][1];
                    stSetEmtDir(emt, up);
                }
                calcParam(val, max, min, dif);
            } else {
                vel = emt->_00;
                if (vel.y > 0.0f)
                    vel.y = 0.0f;
                vel.scale(rate);
            }
        }
    }
    stSetEmtVel(emt, vel);
}

void stEmitterCallBack::calc_lineIpFollow(f32 val, f32 ratio, f32 max, f32 min, f32 dif, bool noDir, f32 rate)
{
    JGeometry::TVec3f pos;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);
    stEmitterPtcl *emt = _04;
    Mtx &mtx = ObjUtility::getKartBodyMtx(kartNum);
    ObjUtility::getKartEfctVel(kartNum, &vel);
    int gen = getGeneratePos(&pos, mtx);
    vel.scale(ratio);
    if (!noDir) {
        up.x = mtx[0][1];
        up.y = mtx[1][1];
        up.z = mtx[2][1];
        stSetEmtDir(emt, up);
    }
    calcParam(val, max, min, dif);
    stSetEmtPos(emt, pos);
    if ((u8)gen && val > min) {
        emt->mFlags &= ~0x400;
    } else {
        emt->mFlags |= 0x400;
        vel = emt->_00;
        if (vel.y > 0.0f)
            vel.y = 0.0f;
        vel.scale(rate);
    }
    stSetEmtVel(emt, vel);
}

void stEmitterCallBack::strategy(f32 ratio, f32 max, f32 min, f32 dif, bool b, f32 val)
{
    f32 speed = GetStEfctMgr()->getEfctCarSpeed(kartNum);
    calc_lineIp(speed, ratio, max, min, dif, b, val);
}

void stEmitterCallBack::calcParam(f32 a, f32 b, f32 c, f32 d)
{
    ipData.DoIp(getLerpLineRate(a, b, c, d));
}

f32 stEmitterCallBack::getLerpLineRate(const f32 &val, const f32 &max, const f32 &min, const f32 &dif)
{
    f32 rate = 0.0f;
    if (val >= min) {
        if (val >= max)
            rate = 1.0f;
        else
            rate = dif * (val - min);
    }
    return rate;
}

int stEmitterCallBack::getGeneratePos(JGeometry::TVec3f *, Mtx)
{
    return 0;
}

void stEmitterParamLineIP::Init(u8 funcCnt, stEmitter *_00, stEmitter *_04)
{
    this->funcCnt = funcCnt;
    this->_00 = _00;
    this->_04 = _04;
    funcList = new PTMF[funcCnt];
    for (u8 i = 0; i < funcCnt; i++)
        ((IpFunc *)funcList)[i] = nullptr;
}

void stEmitterParamLineIP::AddFunc(cIpParam param)
{
    IpFunc func;
    switch (param) {
    case Life:
        func = &stEmitterParamLineIP::doLife;
        break;
    case CreateRate:
        func = &stEmitterParamLineIP::doCreateRate;
        break;
    case RateStep:
        func = &stEmitterParamLineIP::doRateStep;
        break;
    case FirstVel:
        func = &stEmitterParamLineIP::doFirstVel;
        break;
    case Size:
        func = &stEmitterParamLineIP::doSize;
        break;
    case SizeFirst:
        func = &stEmitterParamLineIP::doSizeFirst;
        break;
    case SizeLast:
        func = &stEmitterParamLineIP::doSizeLast;
        break;
    case LifeRndRate:
        func = &stEmitterParamLineIP::doLifeRndRate;
        break;
    case FirstVelRndAngle:
        func = &stEmitterParamLineIP::doFirstVelRndAngle;
        break;
    case AlphaStdRate:
        func = &stEmitterParamLineIP::doAlphaStdRate;
        break;
    case Gravity:
        func = &stEmitterParamLineIP::doGravity;
        break;
    case AirFric:
        func = &stEmitterParamLineIP::doAirFric;
        break;
    }
    for (u8 i = 0; i < funcCnt; i++) {
        if (!((IpFunc *)funcList)[i]) {
            ((IpFunc *)funcList)[i] = func;
            break;
        }
    }
}

void stEmitterParamLineIP::DoIp(f32 value)
{
    scalar = value;
    for (u8 i = 0; i < funcCnt; i++)
        (this->*((IpFunc *)funcList)[i])();
}

void stEmitterParamLineIP::doLife()
{
    _08->mLife = (s16)((f32)(_04->mLife - _00->mLife) * scalar + (f32)_00->mLife);
}

void stEmitterParamLineIP::doCreateRate()
{
    _08->mCreateRate = _00->mCreateRate + (s32)(scalar * (f32)(_04->mCreateRate - _00->mCreateRate));
}

void stEmitterParamLineIP::doRateStep()
{
    _08->mRateStep = _00->mRateStep + (s32)(scalar * (f32)(_04->mRateStep - _00->mRateStep));
}

void stEmitterParamLineIP::doFirstVel()
{
    _08->mFirstVel = _00->mFirstVel + scalar * (_04->mFirstVel - _00->mFirstVel);
}

void stEmitterParamLineIP::doSize()
{
    _08->mSizeX = _00->mSizeX + scalar * (_04->mSizeX - _00->mSizeX);
    _08->mSizeY = _00->mSizeY + scalar * (_04->mSizeY - _00->mSizeY);
}

void stEmitterParamLineIP::doSizeFirst()
{
    _08->mSizeFirst = _00->mSizeFirst + scalar * (_04->mSizeFirst - _00->mSizeFirst);
}

void stEmitterParamLineIP::doSizeLast()
{
    _08->mSizeLast = _00->mSizeLast + scalar * (_04->mSizeLast - _00->mSizeLast);
}

void stEmitterParamLineIP::doLifeRndRate()
{
    _08->mLifeRndRate = _00->mLifeRndRate + scalar * (_04->mLifeRndRate - _00->mLifeRndRate);
}

void stEmitterParamLineIP::doFirstVelRndAngle()
{
    _08->mFirstVelRndAngle = _00->mFirstVelRndAngle + scalar * (_04->mFirstVelRndAngle - _00->mFirstVelRndAngle);
}

void stEmitterParamLineIP::doAlphaStdRate()
{
    _08->mAlphaStdRate = _00->mAlphaStdRate + scalar * (_04->mAlphaStdRate - _00->mAlphaStdRate);
}

void stEmitterParamLineIP::doGravity()
{
    _08->mGravity = _00->mGravity + scalar * (_04->mGravity - _00->mGravity);
}

void stEmitterParamLineIP::doAirFric()
{
    _08->mAirFric = _00->mAirFric + scalar * (_04->mAirFric - _00->mAirFric);
}

stEmitter::stEmitter()
{
    mCreateRate = 0;
    _1c = 0.0f;
    mRateStep = 0;
    mLife = 0;
    mLifeRndRate = 0.0f;
    mFirstVelRndAngle = 0.2f;
    mFirstVel = 0.1f;
    mVelRndRate = 0.0f;
    mGravity = 0.0f;
    mAirFric = 1.0f;
    mAirFricRnd = 0.1f;
    mRotBase = 0.0f;
    mRotRnd = 0.1f;
    mRotXBase = 0.0f;
    mRotXRnd = 0.0f;
    mRotSpeedThresh = 0.0f;
    mSizeX = 1.0f;
    mSizeY = 1.0f;
    mSizeRnd = 1.0f;
    mSizeFirst = 0.0f;
    mSizeLast = 0.5f;
    mSizeFirstRate = 0.25f;
    mSizeLastRate = 0.75f;
    mAlphaStdRate = 1.0f;
    mAlphaFirst = 1.0f;
    mAlphaLast = 1.0f;
    mAlphaFirstRate = 0.25f;
    mAlphaLastRate = 0.75f;
    _b4 = 0;
    mFlags = 0;
    mTevMode = 0;
    mKind = 0;
    mDrawType = 0;
    mFirstVelDir.zero();
    _0c.zero();
    _00.zero();
    _9c.zero();
}

void stEmitterPtcl::removeAllParticles()
{
    stParticleMgr *mgr = &mParticle;
    JSUListIterator<stParticle> it(mgr->mDynamics.mList.getFirst());
    JSUList<stParticle> &list = mgr->mDynamics.mList;
    while (it != nullptr)
        mgr->mDynamics.Remove(list, it);
}

void stEmitterPtclMgr::calc()
{
    JSUListIterator<stEmitterPtcl> it(mList.getFirst());
    while (it != nullptr) {
        it->_130->_04 = it.getObject();
        it->_130->calc();
        if (it->_146 >= 1) {
            stEffectMgr *mgr = GetStEfctMgr();
            stEmitterCallBack *cb = it->_130;
            if (mgr->mpClipParam[(u16)(it->_146 - 1)]._01 != 0 && !mgr->tstIsAvailableDistance(cb->kartNum))
                it->mFlags |= 0x2000;
            else
                it->mFlags &= ~0x2000;
        }
        if (it->chkCreateStep() && it->_130->_25 == 0) {
            it->mParticle.mDynamics.update();
            if (!ChkRemoveBecomeInvalidEmt(mList, it))
                ++it;
        } else {
            RemoveEmt(mList, it);
        }
    }
}

void stEmitterPtclMgr::draw(u32 viewNo)
{
    int type;
    int prevType = -1;
    stParticleDraw::GXDrawBegin(viewNo);
    for (JSUListIterator<stEmitterPtcl> it(mList.getFirst()); it != nullptr; ++it) {
        type = it->_146;
        if (prevType != type || (it->mFlags & 0x100)) {
            it->mParticle.mDraw.GXInitDraw();
            prevType = type;
        }
        it->mParticle.mDraw.draw();
    }
    GXSetZCompLoc(GX_TRUE);
}

stEmitterPtcl *stEmitterPtclMgr::Create(stEmitterCallBack *cb, u16 type)
{
    stEmitterPtcl *ret = nullptr;
    JSUListIterator<stEmitterPtcl> it(mFreeList->getFirst());
    if (it != nullptr) {
        it->_146 = type;
        it->_144 = 0;
        cb->_04 = it.getObject();
        it->_130 = cb;
        it->_130->init();
        it->_130->_25 = 0;
        ret = it.getObject();
        if (mList.getNumLinks() != 0) {
            for (JSULink<stEmitterPtcl> *it2 = mList.getFirst(); it2 != nullptr; it2 = it2->getNext()) {
                if (it2->getObject()->_146 == it->_146) {
                    mList.insert(&it2->getObject()->_134, &ret->_134);
                    break;
                }
                if (it2 == mList.getLast()) {
                    mList.append(&ret->_134);
                    break;
                }
            }
        } else {
            mList.prepend(&ret->_134);
        }
    }
#line 784
    JUT_ASSERT_F(ret, "SnpEmitter is Max emtNum:%d", mList.getNumLinks());
    return ret;
}

void stEmitterPtclMgr::RemoveEmt(JSUList<stEmitterPtcl> &list, JSUListIterator<stEmitterPtcl> &it)
{
    it->_130 = nullptr;
    it->removeAllParticles();
    JSULink<stEmitterPtcl> *next = it.mLink;
    if (it != nullptr)
        next = it.mLink->getNext();
    mFreeList->append((JSULink<stEmitterPtcl> *)&it->_134);
    it = next;
}

bool stEmitterPtclMgr::ChkRemoveBecomeInvalidEmt(JSUList<stEmitterPtcl> &list, JSUListIterator<stEmitterPtcl> &it)
{
    bool ret = false;
    if ((it->mFlags & 0x200) && it->mParticle.mDynamics.mList.getNumLinks() == 0) {
        RemoveEmt(list, it);
        ret = true;
    }
    return ret;
}

void stEmitterPtclMgr::removeAllEmt()
{
    JSUListIterator<stEmitterPtcl> it(mList.getFirst());
    bool avail;
    while ((avail = !(it == nullptr))) {
        it->_130 = nullptr;
        it->removeAllParticles();
        JSULink<stEmitterPtcl> *next = it.mLink;
        if (avail)
            next = it.mLink->getNext();
        mFreeList->append((JSULink<stEmitterPtcl> *)&it->_134);
        it = next;
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
