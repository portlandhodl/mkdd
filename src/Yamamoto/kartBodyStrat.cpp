#include "Yamamoto/KartStrat.h"
#include "Yamamoto/kartBody.h"

#include "JSystem/JAudio/JASFakeMatch2.h"
#include "Yamamoto/kartCtrl.h"

// comments inside functions are inline functions being called in that function

void KartStrat::Init(int) {}

void KartStrat::GetBodySpeed() {
    KartBody *body = mBody;
    body->_454 = 2.16f * body->_448;
    if (body->mGameStatus & 0x200) {
        body->_458 = 0.0f;
    } else {
        body->_458 = body->_454;
    }
}

void KartStrat::GetBodyRoll() {
    // void KartShadowModel::setRoll(float) {}
    // void std::atanf(float) {}
}

void KartStrat::GetRoadBodyRoll() {}

void KartStrat::GetBodyNorm() {}

void KartStrat::DoEnemyMaxmZCrl(float) {}

void KartStrat::DoMaxLevelZXVelCrl(float, float) {}

void KartStrat::DoVelCrl(float) {}

void KartStrat::DoWgCrl(float) {}

void KartStrat::DoRotPose(JGeometry::TVec3<float>, float) {
    // void JGeometry::TVec3<float>::operator* (float) const {}
    // void JGeometry::TVec3<float>::operator*= (float) {}
}

void KartStrat::DoPoseCrl() {}

void KartStrat::MovingSpinClear() {}

void KartStrat::MovingHalfSpinClear() {}

void KartStrat::MovingTornadeClear() {}

void KartStrat::FreezeClear() {}

void KartStrat::RollCrashClear() {}

void KartStrat::AllGravyClear() {
    KartBody *body = mBody;
    body->mLiftframe = 0.0f;
    body->_398 = 0.0f;
    GravyClear();
}

void KartStrat::GravyClear() {
    KartBody *body = mBody;
    body->_4c0 = 0.0f;
    body->_4c4 = 0.0f;
    body->_4d4 = 0.0f;
    body->_4d8 = 0.0f;
    body->_35c.zero();
}

void KartStrat::LiftClear() {
    mBody->mCarStatus &= ~0x1800000000003ull;
}

void KartStrat::PitchClear() {
    mBody->mCarStatus &= ~0x300;
}

void KartStrat::DashClear() {}

void KartStrat::OtherClear() {}

void KartStrat::ShakeGround() {}

void KartStrat::DoAdjustment() { 
    // void KartBody::getPipe() {}
}

void KartStrat::DoWheelSpinCrl() {}

void KartStrat::DoWallCrl() {}

void KartStrat::DoYawLimit() {}

void KartStrat::DoRollLimit() {
    KartBody *body = mBody;
    f32 limit = 0.0697777f;
    if (body->mWg.z > limit) {
        body->mWg.z = limit;
    } else if (body->mWg.z < -limit) {
        body->mWg.z = -limit;
    }
}

void KartStrat::DoLiftCrl() {}

void KartStrat::DoLiftYawCrl() {
    // void SysDebug::checkNaNVector(Vec *, char *) {}
}

void KartStrat::DoRollLim(float, float) {}

void KartStrat::DoPitchLim() {}

void KartStrat::DoRollCrash() {}

void KartStrat::DoTestPitchCrl() {}

void KartStrat::DoDashCrl(float) {}

void KartStrat::DoStarCrl(float) {}

void KartStrat::DoAirCrl() {}

void KartStrat::DoYawCrl() {}

void KartStrat::DoSignalCrl() {
    KartBody *body = mBody;
    body->mWg.y = 0.0f;
    body->mVel.x = 0.0f;
    body->mVel.y = 0.0f;
    body->mVel.z = 0.0f;
    body->_2cc.x = 0.0f;
    body->_2cc.y = 0.0f;
    body->_2cc.z = 0.0f;
}

void KartStrat::DoSpeedCrl() {}

void KartStrat::DoComSpeedCrl() {}

void KartStrat::DoCutSlide() {}

void KartStrat::DoCalcSpeed() {}

void KartStrat::DoMotor(MotorManager::MotorType) {
    // void KartBody::getHandle() {}
}

void KartStrat::DoPowerMotor(float, unsigned char, unsigned char) {}

void KartStrat::DashSpeedCtrl(float scale) { DashSpSpeedCtrl(scale); }

void KartStrat::DashSpSpeedCtrl(float) {}

void KartStrat::CompulsionDash(JGeometry::TVec3<float> *) {}

void KartStrat::DoDash() {
    // void KartCam::GetCameraMode() {}
}

int KartStrat::DoStatusCrl() {
    KartBody *kartBody = mBody;
    
    if ((kartBody->mGameStatus & 4) == 0) {
        DoSignalCrl();
        return 2;
    }

    kartBody->mGameStatus = kartBody->mGameStatus & 0xfffffdff;

    switch(kartBody->_584) {
        case 1:
            kartBody->getCrash()->DoSpinCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 2:
        case 3:
            kartBody->getCrash()->DoFreezeCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 4:
            kartBody->getCrash()->DoHalfSpinCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 5:
            kartBody->getCrash()->DotornadeCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 6:
            kartBody->getCrash()->DoRollCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 7:
            kartBody->getCrash()->DoPitchCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 8:
            kartBody->getTumble()->DoAfterTumbleCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 9:
            kartBody->getCrash()->DoBombCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 0xc:
            kartBody->getTumble()->DoShootCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 0xd:
            DoWheelSpinCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            if ((kartBody->mCarStatus & 0x10) == 0) {
                return 3;
            }
            break;
        case 0xe:
            kartBody->getRescue()->DoAfterRescueCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            return 0;
        case 0xf:
            kartBody->getCannon()->DoAfterCannonCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            return 0;
        case 0x10:
            kartBody->getPipe()->DoAfterPipeCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            return 0;
        case 0x11:
            kartBody->getAnt()->DoAfterAntCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            return 0;
        case 0x12:
            kartBody->getDossin()->DoAfterDossinCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            return 0;
        default:
            DoWallCrl();
            kartBody->DegubBody(0x22);
            if (kartBody->getChecker()->CheckCrash() == 0) {
                DoLiftCrl();
                kartBody->DegubBody(0x23);      // MJB - are these constants part of an enum?
                DoRollCrash();
                kartBody->DegubBody(0x24);
                DoTestPitchCrl();
                DoPitchLim();
                kartBody->DegubBody(0x25);
            }
            DoSpeedCrl();
            kartBody->DegubBody(0x26);
            DoAirCrl();
            kartBody->DegubBody(0x27);
            DoCutSlide();
            kartBody->DegubBody(0x28);
            DoYawCrl();
            kartBody->DegubBody(0x29);
            DoCalcSpeed();
            kartBody->DegubBody(0x2a);
            if ((kartBody->mGameStatus & 0x400) != 0) {
                kartBody->mGameStatus |= 0x200;
                kartBody->mEffctVel.zero();
                return 2;
            }
            if ((kartBody->_29c.x != 0.0f) || (kartBody->_29c.y != 0.0f) || (kartBody->_29c.z != 0.0f)) {
                return 0;
            }
            if ((kartBody->_284.x != 0.0f) || (kartBody->_284.y != 0.0f) || (kartBody->_284.z != 0.0f)) {
                return 0;
            }

            if (((kartBody->getTouchNum() < 3) || (kartBody->_3c8 > 0.0f)) || ((kartBody->_4d4 != 0.0f || ((kartBody->mCarStatus & 0x2100010) != 0)))) {
                return 0;
            }

            if (kartBody->mWg.length() > 0.0872222f) {
                return 0;
            }
            if ((kartBody->mBodyGround.isShaking() != 0) || (kartBody->_58c == 7)) {
                return 0;
            }
            kartBody->DegubBody(0x2b);
            if (GetKartCtrl()->GetCarSpeed(kartBody->mMynum) < 0.052333299f) {
                if (kartBody->getChecker()->CheckCrash() == 0) {
                    if (kartBody->mWg.y < 0.052333299f && kartBody->mWg.y > -0.052333299f) {
                        kartBody->mWg.y = 0.0f;
                    }
                    if (kartBody->mVel.y > 1.0f) {
                        kartBody->mGameStatus = kartBody->mGameStatus | 0x200;
                        return 3;
                    }
                    kartBody->mGameStatus = kartBody->mGameStatus | 0x200;
                    kartBody->mEffctVel.zero();
                    return 2;
                }
            }
            break;
    }
    
    kartBody->DegubBody(0x2c);
    return 0;
}
