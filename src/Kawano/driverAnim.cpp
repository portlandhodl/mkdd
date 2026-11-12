#include "Kawano/driver.h"

#include "Kameda/SequenceInfo.h"
#include "JSystem/J3D/J3DAnmTransform.h"
#include "Kawano/driverMgr.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "Kaneshige/KartChecker.h"
#include "Kaneshige/KartLoader.h"
#include "Kaneshige/RaceMgr.h"
#include "JSystem/JUtility/JUTAssert.h"







void DriverModel::goBack()
{
    if (mKartLoader->mDriverModels[0]->IsPiyo() || mKartLoader->mDriverModels[1]->IsPiyo())
        return;
    switch (mKartLoader->mKartInfo->getKartWeight()) {
    case 0:
        StartAnimation(mcS098);
        break;
    case 1:
        StartAnimation(mcS100);
        break;
    case 2:
        StartAnimation(mcS102);
        break;
    }
}

void DriverModel::goFront()
{
    if (mKartLoader->mDriverModels[0]->IsPiyo() || mKartLoader->mDriverModels[1]->IsPiyo()) {
        StartAnimation(mcS121);
        m_c0 = 10;
        return;
    }
    switch (mKartLoader->mKartInfo->getWeight()) {
    case 0:
        StartAnimation(mcS099);
        break;
    case 1:
        StartAnimation(mcS101);
        break;
    case 2:
        StartAnimation(mcS103);
        break;
    }
}

void DriverModel::TurnFront()
{
    StartAnimation(mcS105);
}

void DriverModel::StartThrow(int a)
{
    StartThrow(a, 0, 0);
}

void DriverModel::StartThrow(int a, int b, int c)
{
    if (a != 0)
        a = 1;
    if (b != 0)
        b = 1;
    if (c != 0)
        c = 1;
    StartAnimation((StateEnum)(a * 4 + b * 2 + c + mcS013));
}

void DriverModel::StartDriverThrow(int a, int b)
{
    if (a != 0)
        a = 1;
    if (b != 0)
        b = 1;
    StartAnimation((StateEnum)(a * 2 + b + mcS009));
}

void DriverModel::StartJump()
{
    if (IsBack())
        StartAnimation(mcS090);
    else
        StartAnimation(mcS089);
}

void DriverModel::StartJump(int arg)
{
    if (arg != 0) {
        StartAnimation(mcS029);
    } else if (IsBack()) {
        StartAnimation(mcS090);
    } else {
        StartAnimation(mcS089);
    }
}

void DriverModel::StartSit()
{
    if (IsBack())
        StartAnimation(mcS092);
    else
        StartAnimation(mcS091);
}

void DriverModel::StartSit(int)
{
    if (IsBack())
        StartAnimation(mcS092);
    else
        StartAnimation(mcS091);
}

void DriverModel::StartStand()
{
    if (IsBack())
        StartAnimation(mcS112);
    else
        StartAnimation(mcS111);
}

void DriverModel::StartStart()
{
    StartAnimation(mcS093);
}

void DriverModel::StartHokanDWait()
{
    StartAnimation(mcS032);
}

void DriverModel::StartAttack(int a, int b)
{
    if (a != 0)
        StartAnimation((StateEnum)(b + 144));
    else
        StartAnimation((StateEnum)(b + 140));
}

void DriverModel::StartAttack(int a)
{
    if (IsBack())
        StartAnimation(mcS144);
    else
        StartAnimation((StateEnum)(a + mcS140));
}

void DriverModel::StartAttackL()
{
    if (IsBack())
        StartAnimation(mcS144);
    else
        StartAnimation((StateEnum)(m_b8 + mcS140));
}

void DriverModel::StartAttackR()
{
    if (IsBack())
        StartAnimation(mcS145);
    else
        StartAnimation((StateEnum)(m_b8 + mcS142));
}

void DriverModel::StartLoopAnm(StateEnum state, DriversAnimator *anim, f32 min, f32 max, f32 speed, f32 phase)
{
    mState = state;
    anim->mLoopAnim.mMinValue = min;
    anim->mLoopAnim.mMaxValue = max;
    anim->mLoopAnim.mSpeed = speed;
    anim->mLoopAnim.mCurrentPhase = phase;
    anim->mLoopAnim.start();
}

void DriverModel::StartLoopAnm2(DriversAnimator *anim, f32 min, f32 max, f32 speed, f32 phase)
{
    anim->mLoopAnim.mMinValue = min;
    anim->mLoopAnim.mMaxValue = max;
    anim->mLoopAnim.mSpeed = speed;
    anim->mLoopAnim.mCurrentPhase = phase;
    anim->mLoopAnim.start();
}

void DriverModel::StartWait()
{
    StartAnimation(mcS077);
}

void DriverModel::StartWaitBack()
{
    mState = mcS078;
    mDriveAnim.mLoopAnim.mMinValue = 0.0f;
    mDriveAnim.mLoopAnim.mMaxValue = 1.0f;
    mDriveAnim.mLoopAnim.mSpeed = 0.5f;
    mDriveAnim.mLoopAnim.start();
}

void DriverModel::StartDWait()
{
    f32 phase = 0.7f * mKartNum;
    mState = mcS032;
    mDriveAnim.mLoopAnim.mMinValue = 0.0f;
    mDriveAnim.mLoopAnim.mMaxValue = 1.0f;
    mDriveAnim.mLoopAnim.mSpeed = 1.0f;
    mDriveAnim.mLoopAnim.mCurrentPhase = phase;
    mDriveAnim.mLoopAnim.start();
}

void DriverModel::StartRun2()
{
    StartAnimation(mcS033);
}

void DriverModel::StartDemo()
{
    mState = mcS065;
    m_c8 = 1;
}

void DriverModel::StartDemo2()
{
    mState = mcS066;
    m_c8 = 1;
}

void DriverModel::StartDemo2Front()
{
    mState = mcS067;
    m_c8 = 1;
}

void DriverModel::StartDemo2Back()
{
    mState = mcS068;
    m_c8 = 1;
}

void DriverModel::StartDRunAnim(DriversAnimator *anim)
{
    anim->mLoopAnim.mMinValue = 0.0f;
    anim->mLoopAnim.mMaxValue = 1.0f;
    anim->mLoopAnim.mSpeed = 1.0f;
    anim->mLoopAnim.mCurrentPhase = 0.0f;
    anim->mLoopAnim.reset();
    anim->mLoopAnim.start();
}

void DriverModel::Check4StartDrift()
{
    if (IsBack())
        StartAnimation(mcS058);
    else
        StartAnimation(mcS052);
}

void DriverModel::StartDrift()
{
    if (IsBack())
        StartAnimation(mcS052);
    else
        StartAnimation(mcS049);
}

void DriverModel::StartDrift(int)
{
    IsBack();
}

void DriverModel::StartDriftEnd()
{
    if (IsBack())
        StartAnimation(mcS055);
    else
        StartAnimation(mcS053);
}

void DriverModel::StartDriftEnd(int)
{
    IsBack();
}

void DriverModel::StartDamage(int a, int b)
{
    if (b > 0)
        b = 1;
    if (mState == mcS096) {
        mAnmID = msAnmBlendList[mState][0];
        if (mAnmID == 0)
            return;
        if (getAnmTransformPtr(mAnmID) == 0)
            return;
        u8 flag = DriverManager::ptr()->_5d;
        J3DAnmTransform *anm = getAnmTransformPtr(mAnmID);
        if (anm->getFrame() < (f32)flag) {
            if (b != 0)
                m_dc = b + 106;
            else
                m_dc = mcS110;
        } else {
            if (b != 0)
                StartAnimation((StateEnum)(b + 106));
            else
                StartAnimation(mcS110);
        }
    } else {
        StartAnimation((StateEnum)(b + 106));
    }
}

void DriverModel::StartDamage_GetUp(int a, int b)
{
    if (b != 0)
        StartAnimation(mcS125);
    else
        StartAnimation(mcS117);
}

void DriverModel::StartHokanDash()
{
    StartAnimation(mcS069);
}

void DriverModel::StartHokanDash2()
{
    StartAnimation(mcS075);
}

void DriverModel::StartDash()
{
    if (IsBack())
        StartAnimation(mcS072);
    else
        StartAnimation(mcS069);
}

void DriverModel::StartHang()
{
    if (IsBack())
        StartAnimation(mcS072);
    else
        StartAnimation(mcS069);
}

void DriverModel::StartDashEnd()
{
    if (IsBack())
        StartAnimation(mcS074);
    else
        StartAnimation(mcS071);
}

void DriverModel::SetHangRate(f32 rate)
{
    DriverManager::ptr()->_28 = rate;
}

void DriverModel::StartHang2(f32 f, u8 b)
{
    if (f >= 0.0f)
        DriverManager::ptr()->_28 = f;
    DriverManager::ptr()->mBlendTimes[3] = b;
    if (IsBack())
        StartAnimation(mcS076);
    else
        StartAnimation(mcS075);
    m_e0 = 0;
}

void DriverModel::StartDashEnd2(f32 f, u8 b)
{
    switch (mState) {
    case mcS075:
    case mcS076:
        if (f >= 0.0f)
            DriverManager::ptr()->_28 = f;
        DriverManager::ptr()->mBlendTimes[4] = b;
        m_e0 = 1;
        u8 t = DriverManager::ptr()->mBlendTimes[3]; DriverManager *mgr = DriverManager::ptr(); m_bb = (f32)m_bb * (f32)mgr->mBlendTimes[4] / (f32)t;
        break;
    }
}

void DriverModel::StartDanger(int a)
{
    if (a != 0)
        StartDangerR();
    else
        StartDangerL();
}

void DriverModel::StartDangerL()
{
    if (mState != mcS108)
        StartAnimation(mcS108);
}

void DriverModel::StartDangerR()
{
    if (mState != mcS109)
        StartAnimation(mcS109);
}

void DriverModel::StartDangerEnd(int a)
{
    if (a == 0)
        StartAnimation(mcS111);
    else
        StartAnimation(mcS112);
}

void DriverModel::StartDriverDamage()
{
    StartAnimation(mcS118);
}

void DriverModel::StartDriverPiyo()
{
    StartAnimation(mcS119);
}

void DriverModel::StartDriverDrive()
{
    StartAnimation(mcS120);
}

void DriverModel::StartSetTire(int a)
{
    if (a != 0)
        StartSetTireR();
    else
        StartSetTireL();
}

void DriverModel::StartSetTireL()
{
    StartAnimation(mcS063);
}

void DriverModel::StartSetTireR()
{
    StartAnimation(mcS064);
}

void DriverModel::StartGoalMotion()
{
    int rank = RaceMgr::getManager()->getKartChecker(mKartLoader->mKartNum)->getRank();
    int front = IsFront();
    int kartNum2 = RaceMgr::getManager()->mRaceInfo->mKartNum;

    int type;
    if (kartNum2 != 8) {
        if (SequenceInfo::RANKPOINT[rank - 1] != 0 && kartNum2 > rank)
            type = 1;
        else
            type = 0;
    } else {
        switch (rank) {
        case 0: case 1: case 2: case 3:
            type = 1;
            break;
        case 4: case 5: case 6: case 7: case 8:
            type = 0;
            break;
        }
    }

    if (rank > 0) {
        if (front) {
            switch (type) {
            case 1:
                StartAnimation(mcS130);
                break;
            case 0:
                StartAnimation(mcS133);
                break;
            }
        } else {
            if (rank == 1)
                StartAnimation(mcS128);
            else {
                switch (type) {
                case 1:
                    StartAnimation(mcS132);
                    break;
                case 0:
                    StartAnimation(mcS134);
                    break;
                }
            }
        }
    }
    m_c2 = 1;
}

void DriverModel::StartGood()
{
    if (IsFront())
        StartAnimation(mcS130);
    else
        StartAnimation(mcS132);
}

void DriverModel::StartGood1st()
{
    if (IsFront())
        StartAnimation(mcS130);
    else
        StartAnimation(mcS128);
}

void DriverModel::StartBad()
{
    if (IsFront())
        StartAnimation(mcS133);
    else
        StartAnimation(mcS134);
}

void DriverModel::StartFire()
{
    if (!IsBack())
        StartAnimation(mcS123);
    else
        StartAnimation(mcS124);
}

void DriverModel::StartFireEnd()
{
    if (!IsBack())
        StartAnimation(mcS111);
    else
        StartAnimation(mcS112);
}

void DriverModel::StartHappy()
{
    if (mItem != 0 && (mItem->mTransferFlags & 0x40))
        return;
    if (IsFront())
        StartAnimation(mcS131);
    else if (IsBack())
        StartAnimation(mcS147);
    else
        StartAnimation(mcS146);
}

void DriverModel::StartAppeal()
{
    StartAnimation(mcS148);
}

void DriverModel::StartJuggle()
{
    if (getAnmTransformPtr(mcS080) == 0)
        return;
    s16 maxFrame = getAnmTransformPtr(mcS080)->getFrameMax();
    m_668.mMinValue = 0.0f;
    m_668.mMaxValue = 1.0f;
    m_668.mSpeed = 60.0f / maxFrame;
    m_668.mCurrentPhase = 0.0f;
    m_668.reset();
    m_668.start();
}

void DriverModel::StartSpeak()
{
    if (getAnmTransformPtr(mcS081) == 0)
        return;
    s16 maxFrame = getAnmTransformPtr(mcS081)->getFrameMax();
    m_688.mMinValue = 0.0f;
    m_688.mMaxValue = 1.0f;
    m_688.mSpeed = 60.0f / maxFrame;
    m_688.mCurrentPhase = 0.0f;
    m_688.reset();
    m_688.start();
}

int DriverModel::IsMove()
{
    if (IsChange())
        return 1;
    return IsTurn() != 0 ? 1 : 0;
}

int DriverModel::IsChange()
{
    if (IsChangeFront())
        return 1;
    return IsChangeBack() ? 1 : 0;
}

int DriverModel::IsChangeFront()
{
    switch (mState) {
    case mcChangeBtoA:
    case mcS097:
    case mcS099:
    case mcS101:
    case mcS103:
    case mcS121:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsChangeBack()
{
    switch (mState) {
    case mcChangeAtoB:
    case mcS096:
    case mcS098:
    case mcS100:
    case mcS102:
    case mcS122:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsTurn()
{
    if (IsTurnBack())
        return 1;
    return IsTurnFront() ? 1 : 0;
}

int DriverModel::IsTurnBack()
{
    switch (mState) {
    case mcS007:
    case mcS104:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsTurnFront()
{
    switch (mState) {
    case mcS008:
    case mcS105:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsThrow()
{
    switch (mState) {
    case mcS009: case mcS010: case mcS011: case mcS012: case mcS013: case mcS014:
    case mcS015: case mcS016: case mcS017: case mcS018: case mcS019: case mcS020:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsThrowBack()
{
    switch (mState) {
    case mcS063:
    case mcS064:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsTire()
{
    switch (mState) {
    case mcS063: case mcS064: case mcS126: case mcS127:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsTire2()
{
    switch (mState) {
    case mcS126:
    case mcS127:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsAttack()
{
    switch (mState) {
    case mcS021: case mcS022: case mcS023: case mcS024: case mcS025: case mcS026:
    case mcS085: case mcS086: case mcS087: case mcS088:
    case mcS140: case mcS141: case mcS142: case mcS143: case mcS144: case mcS145:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsJump()
{
    switch (mState) {
    case mcS027: case mcS028: case mcS029: case mcS089: case mcS090:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsSit()
{
    switch (mState) {
    case mcS030: case mcS031: case mcS091: case mcS092:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsStand()
{
    switch (mState) {
    case mcS111:
    case mcS112:
        switch (mBeforeState) {
        case mcS091:
        case mcS092:
            return 1;
        }
    }
    return 0;
}

int DriverModel::IsBack()
{
    switch (mState) {
    case mcDriveC:
    case mcTandemAlone:
    case mcS017: case mcS018: case mcS019: case mcS020:
    case mcS031:
    case mcS055: case mcS056: case mcS057: case mcS058: case mcS059: case mcS060:
    case mcS072: case mcS073: case mcS074: case mcS076: case mcS078:
    case mcS080:
    case mcS083: case mcS084:
    case mcS087: case mcS088:
    case mcS090:
    case mcS092:
    case mcS112:
    case mcS115: case mcS116:
    case mcS124:
    case mcS138: case mcS139:
    case mcS144: case mcS145:
    case mcS147:
        return 1;
    }
    if (IsTurnBack())
        return 1;
    return 0;
}

int DriverModel::IsStart()
{
    switch (mState) {
    case mcS034: case mcS035: case mcS036:
    case mcS094: case mcS095:
    case mcS125:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsBeforeStart()
{
    switch (mState) {
    case mcS032:
    case mcS033:
    case mcS093:
    case mcS094:
    case mcS095:
    case mcS125:
    case mcS149:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsNormalStart()
{
    return mState == mcS034 ? 1 : 0;
}

int DriverModel::IsRocketStart()
{
    return mState == mcS094 ? 1 : 0;
}

int DriverModel::IsDamage()
{
    if (m_dc != 0)
        return 1;
    switch (mState) {
    case mcS037: case mcS038: case mcS039: case mcS040: case mcS041: case mcS042:
    case mcS043: case mcS044: case mcS045: case mcS046: case mcS047: case mcS048:
    case mcS106: case mcS107:
    case mcS110:
    case mcS117:
        return 1;
    case mcS111:
        switch (mBeforeState) {
        case mcS117:
            return 1;
        }
    }
    return 0;
}

int DriverModel::IsDamageS()
{
    if (m_dc == mcS110)
        return 1;
    switch (mState) {
    case mcS037: case mcS038: case mcS039:
    case mcS043: case mcS044: case mcS045:
    case mcS106:
    case mcS110:
    case mcS117:
        return 1;
    case mcS111:
        switch (mBeforeState) {
        case mcS117:
            return 1;
        }
    }
    return 0;
}

int DriverModel::IsDamageL()
{
    if (m_dc == mcS107)
        return 1;
    switch (mState) {
    case mcS040:
    case mcS041:
    case mcS046:
    case mcS047:
    case mcS107:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDamage2()
{
    switch (mState) {
    case mcS110:
    case mcS111:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDamageS2()
{
    switch (mState) {
    case mcS110:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDamageL2()
{
    switch (mState) {
    case mcS107:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDamageS12()
{
    switch (mState) {
    case mcS037: case mcS038:
    case mcS043: case mcS044:
    case mcS106:
    case mcS110:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDamageL12()
{
    switch (mState) {
    case mcS040: case mcS041:
    case mcS046: case mcS047:
    case mcS107:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsRun()
{
    switch (mState) {
    case mcS033: case mcS042: case mcS048:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDrift(StateEnum state)
{
    switch (state) {
    case mcS049: case mcS050: case mcS051: case mcS052: case mcS053: case mcS054:
    case mcS055: case mcS056: case mcS057: case mcS058: case mcS059: case mcS060:
    case mcS136: case mcS137: case mcS138: case mcS139:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDrift()
{
    switch (mState) {
    case mcS049: case mcS050: case mcS051: case mcS052: case mcS053: case mcS054:
    case mcS055: case mcS056: case mcS057: case mcS058: case mcS059: case mcS060:
    case mcS136: case mcS137: case mcS138: case mcS139:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDriftStart()
{
    switch (mState) {
    case mcS049:
    case mcS052:
    case mcS055:
    case mcS058:
    case mcS136:
    case mcS137:
    case mcS138:
    case mcS139:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDriftEnd()
{
    switch (mState) {
    case mcS050: case mcS053: case mcS056: case mcS059:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDriftFinish()
{
    switch (mState) {
    case mcS051: case mcS054: case mcS057: case mcS060:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDriftLeft()
{
    switch (mState) {
    case mcS049:
    case mcS050:
    case mcS051:
    case mcS055:
    case mcS056:
    case mcS057:
    case mcS136:
    case mcS138:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDriftRight()
{
    switch (mState) {
    case mcS052:
    case mcS053:
    case mcS054:
    case mcS058:
    case mcS059:
    case mcS060:
    case mcS137:
    case mcS139:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsTandemAnmStartOK()
{
    int ret = 0;
    switch (mState) {
    case mcChangeAtoB:
    case mcChangeBtoA:
    case mcS096: case mcS097: case mcS098: case mcS099:
    case mcS100: case mcS101: case mcS102: case mcS103:
        mAnmID = msAnmBlendList[mState][0];
        if (mAnmID != 0) {
            J3DAnmTransform *anm = getAnmTransformPtr(mAnmID);
            if (anm != 0) {
                s16 cur = (s16)(anm->getFrame() + 0.5f);
                s16 max = anm->getFrameMax();
                DriverManager::ptr();
                if (max - cur <= DriverManager::ptr()->getBlendTime(17))
                    ret = 1;
            }
        }
        break;
    }
    return ret;
}

int DriverModel::IsDanger()
{
    switch (mState) {
    case mcS061:
    case mcS062:
    case mcS108:
    case mcS109:
        return (m_be >= DriverManager::ptr()->getDangerTime() ? 1 : 0) + 1;
    default:
        return 0;
    }
}

int DriverModel::IsDangerEnd()
{
    switch (mState) {
    case mcS111:
    case mcS112:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDangerPlus()
{
    switch (mState) {
    case mcS061:
    case mcS062:
    case mcS108:
    case mcS109:
        return 1;
    case mcS111:
    case mcS112:
    case mcS113:
    case mcS114:
    case mcS115:
    case mcS116:
        switch (mBeforeState) {
        case mcS061:
        case mcS062:
        case mcS108:
        case mcS109:
            return 1;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return 0;
}

int DriverModel::IsDash()
{
    switch (mState) {
    case mcS069: case mcS070: case mcS071: case mcS072: case mcS073: case mcS074:
    case mcS075: case mcS076:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDash1()
{
    switch (mState) {
    case mcS069: case mcS070: case mcS071: case mcS072: case mcS073: case mcS074:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDash2()
{
    switch (mState) {
    case mcS075: case mcS076:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsJuggle()
{
    if (mItem != 0) {
        if (m_b8 != 0) {
            if (mItem->getSuccessionItemNum() > 1)
                return 1;
        }
    }
    return 0;
}

int DriverModel::IsDriverDamage()
{
    return mState == mcS118 ? 1 : 0;
}

int DriverModel::IsPiyo()
{
    return mState == mcS119 ? 1 : 0;
}

int DriverModel::IsFire()
{
    switch (mState) {
    case mcS123: case mcS124:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDrive(StateEnum state)
{
    switch (state) {
    case mcDrive:
    case mcS118:
    case mcS119:
    case mcS120:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDrive()
{
    return IsDrive((StateEnum)mState);
}

int DriverModel::IsGoal()
{
    switch (mState) {
    case mcS128:
    case mcS130:
    case mcS132:
    case mcS133:
    case mcS134:
    case mcS135:
        return (m_c2 >= DriverManager::ptr()->getGoalTime() * 60 ? 1 : 0) + 1;
    default:
        return 0;
    }
}

int DriverModel::IsGoalFront()
{
    switch (mState) {
    case mcS130:
    case mcS133:
        if (m_c2 >= DriverManager::ptr()->getGoalTime() * 60)
            return 2;
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsGoalBack()
{
    switch (mState) {
    case mcS128:
    case mcS132:
    case mcS134:
    case mcS135:
        return (m_c2 >= DriverManager::ptr()->getGoalTime() * 60 ? 1 : 0) + 1;
    default:
        return 0;
    }
}

int DriverModel::IsHappy()
{
    switch (mState) {
    case mcS129:
        return IsHappyBack();
    case mcS131:
        return IsHappyFront();
    default:
        return 0;
    }
}

int DriverModel::IsHappyFront()
{
    switch (mState) {
    case mcS131:
        if (DriverManager::ptr()->_838 != 0) {
            if (m_c4 >= DriverManager::ptr()->getHappyFrontTime())
                return 2;
        } else {
            if (m_c4 >= DriverData::msDriverData[mDataID].data[84])
                return 2;
        }
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsHappyBack()
{
    switch (mState) {
    case mcS129:
        if (DriverManager::ptr()->_838 != 0) {
            if (m_c4 >= DriverManager::ptr()->getHappyBackTime())
                return 2;
        } else {
            if (m_c4 >= DriverData::msDriverData[mDataID].data[85])
                return 2;
        }
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsAppeal(StateEnum state)
{
    switch (state) {
    case mcS148:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsAppeal()
{
    switch (mState) {
    case mcS148:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsFront()
{
    switch (mState) {
    case mcDrive:
    case mcChangeBtoA:
    case mcS009:
    case mcS010:
    case mcS011:
    case mcS012:
    case mcS097:
    case mcS099:
    case mcS101:
    case mcS103:
    case mcS118:
    case mcS119:
    case mcS120:
    case mcS121:
    case mcS130:
    case mcS131:
    case mcS133:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsItemThrowOK()
{
    if ((u32)m_ba >= (u32)DriverManager::ptr()->getBlendTime(0))
        return 1;
    return 0;
}

int DriverModel::IsTandem(StateEnum state)
{
    switch (state) {
    case mcTandemB: case mcTandemAlone:
    case mcS079: case mcS080: case mcS081: case mcS082: case mcS083: case mcS084:
    case mcS111: case mcS112:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsTandem()
{
    switch (mState) {
    case mcTandemB: case mcTandemAlone:
    case mcS079: case mcS080: case mcS081: case mcS082: case mcS083: case mcS084:
    case mcS111: case mcS112:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsWait(StateEnum state)
{
    switch (state) {
    case mcDriveBack:
    case mcDriveC:
    case mcS077:
    case mcS078:
    case mcS079:
    case mcS080:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsWait()
{
    switch (mState) {
    case mcDriveBack:
    case mcDriveC:
    case mcS077:
    case mcS078:
    case mcS079:
    case mcS080:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDemo(StateEnum state)
{
    switch (state) {
    case mcS065: case mcS066: case mcS067: case mcS068:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDemoFront(StateEnum state)
{
    switch (state) {
    case mcS067:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDemoBack(StateEnum state)
{
    switch (state) {
    case mcS068:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDemo()
{
    switch (mState) {
    case mcS065: case mcS066: case mcS067: case mcS068:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDemoFront()
{
    switch (mState) {
    case mcS067:
        return 1;
    default:
        return 0;
    }
}

int DriverModel::IsDemoBack()
{
    switch (mState) {
    case mcS068:
        return 1;
    default:
        return 0;
    }
}
