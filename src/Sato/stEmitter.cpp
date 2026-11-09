#include "Sato/stEmitter.h"
#include "Sato/stEffectMgr.h"

typedef void (stEmitterParamLineIP::*IpFunc)();

void stEmitterCallBack::strategy(f32 ratio, f32 max, f32 min, f32 dif, bool b, f32 val)
{
    f32 speed = GetStEfctMgr()->getEfctCarSpeed(kartNum);
    calc_lineIp(speed, ratio, max, min, dif, b, val);
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
