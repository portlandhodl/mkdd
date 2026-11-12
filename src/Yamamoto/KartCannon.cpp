#include "Yamamoto/KartCannon.h"
#include "JSystem/J3D/J3DPacket.h"

#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Sato/RivalBodyCtrl.h"
#include "Sato/JPEffectPerformer.h"
#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCtrl.h"
#include "Kaneshige/JugemPoint.h"
#include "Kaneshige/Objects/GeoCannon.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

// comments inside functions are inline functions being called in that function

void KartCannon::Init(int idx) {
    mBody = GetKartCtrl()->getKartBody(idx);
    mFlags = 0;
    _9 = 0;
    mState = 0;
}

void KartCannon::MakeCannon() {
    KartBody *body = mBody;

    body->getGame()->MakeClear();
    body->mCarStatus &= 0x41000;
    body->_584 = 12;
    mState = 0;
    mCounter = 0;
    mTimer = 35;
    body->getItem()->ReleseWanWan();
    body->mCarStatus |= 0x100000000ULL;
    body->mCarStatus &= ~0x80B02000ULL;

    KartAnime *anime = GetKartCtrl()->getKartAnime(body->mMynum);
    anime->mFlags &= 0xFFFFFFFFFFBFDFFFULL;
    body->mCarStatus&=0xFFFE7FFFFFFFFFFCULL;
    body->getDamage()->ClrAllDamage();

    if (GetKartCtrl()->getKartAnime(body->mMynum)->IsDamage(body->mMynum) == true) {
        GetKartCtrl()->getKartAnime(body->mMynum)->mFlags |= 0x00040000;
    }

    body->getDamage()->SetDriverEndDamageAnime();
    DoKeep();
    _14 = 200.0f;

    if (GetKartCtrl()->CheckCamera(body->mMynum)) {
        int cam = GetKartCtrl()->GetCameraNum(body->mMynum);
        if (GetKartCtrl()->getKartCam(cam)->GetCameraMode() == 0) {
            GetKartCtrl()->getKartCam(cam)->MakeShaker(0.4f);
            JPEffectPerformer::setEffectEachCam(JPEffectPerformer::Effect_Cannon, body->mMynum, cam, 1);
        }
    }

    _9 = 0;

    switch (body->mBodyGround.getGeoCannon()->getSpec()) {
    case 0:
        GetKartCtrl()->getKartSound(body->mMynum)->DoKartsetSeSound(0x1006c);
        GetKartCtrl()->getKartSound(body->mMynum)->DoCannonVoice();
        body->getStrat()->DoMotor((MotorManager::MotorType)18);
        _9 |= 1;
        break;
    case 1:
        GetKartCtrl()->getKartSound(body->mMynum)->DoKartsetSeSound(0x1006a);
        GetKartCtrl()->getKartSound(body->mMynum)->DoTandemVoice(8);
        body->getStrat()->DoMotor((MotorManager::MotorType)21);
        _9 |= 2;
        break;
    case 2:
        GetKartCtrl()->getKartSound(body->mMynum)->DoKartsetSeSound(0x1006b);
        GetKartCtrl()->getKartSound(body->mMynum)->DoTandemVoice(8);
        body->getStrat()->DoMotor((MotorManager::MotorType)20);
        _9 |= 4;
        break;
    default:
        GetKartCtrl()->getKartSound(body->mMynum)->DoKartsetSeSound(0x1006c);
        GetKartCtrl()->getKartSound(body->mMynum)->DoCannonVoice();
        body->getStrat()->DoMotor((MotorManager::MotorType)18);
        _9 |= 1;
        break;
    }
}

void KartCannon::DoKeep() {
    KartBody *body = mBody;

    body->mVel.zero();
    body->_2cc.zero();
    body->mWg.zero();
    body->_2c0.zero();
    body->_2d8.zero();

    f32 zero = 0;
    body->_4d8 = zero;
    body->_4d4 = zero;
    body->_4c4 = zero;
    body->_4c0 = zero;
    body->_3cc = zero;
    body->mTireAngle = zero;
    body->_394 = zero;
    body->mLiftframe = zero;
}

void KartCannon::SetPose() {
    KartBody *body = mBody;
    JGeometry::TVec3f shotPos;
    JGeometry::TVec3f shotDir;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f diff;
    JGeometry::TVec3f up;
    JGeometry::TVec3f right;
    JGeometry::TVec3f forward;

    shotDir.set(body->_308);
    shotPos.set(body->mPos);
    mPoint = body->mBodyGround.getCannonPoint();

    if (mPoint != NULL) {
        GeoCannon *cannon = body->mBodyGround.getGeoCannon();
        if (cannon != NULL) {
            cannon->getShotPosition(&shotPos);
            cannon->getShotDirection(&shotDir);
            cannon->shot();
        }

        mPoint->getPosition(&_28);
        mPoint->getFrDirection(&_34);

        if (_9 & 5) {
            _28.y += 10;
        } else {
            _28.y += 800;
        }

        if (body->mPos.y - shotPos.y >= 0) {
            _28.y += body->mPos.y - shotPos.y;
        }

        shotPos.y = body->mPos.y;
        diff.sub(shotPos, body->mPos);
        up.set(0.0f, 1.0f, 0.0f);
        right.cross(up, shotDir);
        right.normalize();
        forward.cross(right, up);
        forward.normalize();
        up.cross(forward, right);
        up.normalize();

        body->_110[0][0] = right.x;
        body->_110[1][0] = right.y;
        body->_110[2][0] = right.z;
        body->_110[0][1] = up.x;
        body->_110[1][1] = up.y;
        body->_110[2][1] = up.z;
        body->_110[0][2] = forward.x;
        body->_110[1][2] = forward.y;
        body->_110[2][2] = forward.z;

        GetKartCtrl()->DevMatrixByVector(&pos, &diff, body->_110);
        f32 dist = pos.x;
        diff.normalize();
        pos.cross(diff, shotDir);

        if (dist < 0) {
            dist *= -1;
        }

        if (pos.y >= 0) {
            _28.x += right.x * dist;
            _28.y += right.y * dist;
            _28.z += right.z * dist;
        } else {
            _28.x -= right.x * dist;
            _28.y -= right.y * dist;
            _28.z -= right.z * dist;
        }

        shotPos.set(body->mPos);
    }

    body->_110[0][3] = shotPos.x;
    body->_110[1][3] = shotPos.y;
    body->_110[2][3] = shotPos.z;
    body->mPos.set(shotPos);
    PSMTXCopy(body->_110, body->mPlayerPosMtx);
    f32 offset = GetKartCtrl()->GetKartBodyOffset(body->mMynum);
    body->mPlayerPosMtx[0][3] += body->mPlayerPosMtx[0][1] * offset;
    body->mPlayerPosMtx[1][3] += body->mPlayerPosMtx[1][1] * offset;
    body->mPlayerPosMtx[2][3] += body->mPlayerPosMtx[2][1] * offset;

    body->_2f0.set(body->_110[0][0], body->_110[1][0], body->_110[2][0]);
    body->_2fc.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
    body->_308.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
}

void KartCannon::DoCatch() {
    KartBody *body = mBody;

    SetPose();
    body->mVel.zero();
    body->_2cc.zero();
    body->mWg.zero();
    body->_2c0.zero();
    body->_2d8.zero();

    f32 zero = 0;
    body->_4d8 = zero;
    body->_4d4 = zero;
    body->_4c4 = zero;
    body->_4c0 = zero;
    body->_3cc = zero;
    body->mTireAngle = zero;
    body->_394 = zero;
    body->mLiftframe = zero;

    RCMGetKartChecker(body->mMynum)->beginWarp(mPoint);
}

int KartCannon::DoLaunch() {
    KartBody *body = mBody;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;

    dir.sub(_28, body->mPos);
    dir.normalize();
    dir.scale(300 * body->_3a4);
    GetKartCtrl()->DevMatrixByVector(&pos, &dir, body->_110);
    PSMTXMultVecSR(body->mPlayerPosMtx, &pos, &dir);
    body->DoForce(&body->mPos, &dir);
    return body->getTouchNum() == 0;
}

bool KartCannon::DoFlight() {
    KartBody *body = mBody;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;

    mFlags |= 0x20;
    dir.sub(_28, body->mPos);
    f32 dist = dir.length();
    dir.normalize();
    dir.scale(300 * body->_3a4);
    GetKartCtrl()->DevMatrixByVector(&pos, &dir, body->_110);
    GetKartCtrl()->ChaseFnumber(&_14, 600, 0.03f);
    PSMTXMultVecSR(body->mPlayerPosMtx, &pos, &dir);
    body->DoForceOnly(&body->mPos, &dir);

    JGeometry::TVec3f torque;
    JGeometry::TVec3f dir2;
    torque.set(0.0f, 0.0f, 20.0f);
    PSMTXMultVec(body->mPlayerPosMtx, &torque, &dir2);
    torque.set(0.0f, 0.1f * body->_3a4, 0.0f);
    PSMTXMultVecSR(body->mPlayerPosMtx, &torque, &dir);
    body->DoTq(&dir2, &dir);

    if (dist < 2 * body->_3a8) {
        RCMGetKartChecker(body->mMynum)->endWarp();
        return 1;
    }
    return 0;
}

bool KartCannon::DoTouchDown() {
    KartBody *body = mBody;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;

    GetKartCtrl()->ChaseFnumber(&_14, 140, 0.11f);
    body->getStrat()->DoVelCrl(_14 / 2.16f / body->mSpeedScale);

    if (_14 > 171) {
        pos.set(0.0f, -52 * body->_3a4, 0.0f);
        PSMTXMultVecSR(body->mPlayerPosMtx, &pos, &dir);
        body->DoForce(&body->mPos, &dir);
        return 0;
    }
    return 1;
}

bool KartCannon::DoLanding() {
    KartBody *body = mBody;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;

    pos.set(0.0f, 0.0f, 50 * body->_3a4);
    PSMTXMultVecSR(body->mPlayerPosMtx, &pos, &dir);
    body->DoForce(&body->mPos, &dir);
    return 1;
}

bool KartCannon::DoCheckEnd() {
    KartBody *body = mBody;

    if (body->mCarStatus & 0x8000000000ULL) {
        if (body->getTouchNum() == 0) {
            JGeometry::TVec3f pos;
            JGeometry::TVec3f dir;

            pos.set(0.0f, -52 * body->_3a4, 80 * body->_3a4);
            PSMTXMultVecSR(body->mPlayerPosMtx, &pos, &dir);
            body->DoForce(&body->mPos, &dir);
            mFlags &= ~0x20;
        }

        if (body->mPos.y - body->mBodyGround.getHeight() > 600.0f) {
            return 1;
        }

        mState = 0;
        GetKartCtrl()->getKartAnime(body->mMynum)->mFlags |= 0x01000000;
        body->mCarStatus &= 0xFFFFFF7FFFFFFFFFULL;
        body->mCarStatus |= 0x400000000000ULL;
        return 1;
    }

    body->mCarStatus &= 0xFFFFFFFEFFFFFFFFULL;
    body->mCarStatus |= 0x8000000000ULL;
    body->_584 = 0;
    return 1;
}

int KartCannon::DoRainbowLaunch() {
    KartBody *body = mBody;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;

    dir.sub(_28, body->mPos);
    dir.normalize();
    dir.scale(300 * body->_3a4);
    GetKartCtrl()->DevMatrixByVector(&pos, &dir, body->_110);
    PSMTXMultVecSR(body->mPlayerPosMtx, &pos, &dir);
    body->DoForce(&body->mPos, &dir);
    return body->getTouchNum() == 0;
}

bool KartCannon::DoRainbowFlight() {
    KartBody *body = mBody;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;

    mFlags |= 0x20;
    dir.sub(_28, body->mPos);
    f32 dist = dir.length();
    dir.normalize();
    dir.scale(300 * body->_3a4);
    GetKartCtrl()->DevMatrixByVector(&pos, &dir, body->_110);
    GetKartCtrl()->ChaseFnumber(&_14, 600, 0.03f);
    PSMTXMultVecSR(body->mPlayerPosMtx, &pos, &dir);
    body->DoForceOnly(&body->mPos, &dir);

    JGeometry::TVec3f torque;
    JGeometry::TVec3f dir2;
    torque.set(0.0f, 0.0f, 20.0f);
    PSMTXMultVec(body->mPlayerPosMtx, &torque, &dir2);
    torque.set(0.0f, 0.1f * body->_3a4, 0.0f);
    PSMTXMultVecSR(body->mPlayerPosMtx, &torque, &dir);
    body->DoTq(&dir2, &dir);

    if (dist < 2 * body->_3a8) {
        RCMGetKartChecker(body->mMynum)->endWarp();
        GetKartCtrl()->ChaseFnumber(&_14, 140, 0.03f);
        body->getStrat()->DoVelCrl(_14 / 2.16f / body->mSpeedScale);
        return 1;
    }
    return 0;
}

bool KartCannon::DoRainbowTouchDown() {
    KartBody *body = mBody;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;

    GetKartCtrl()->ChaseFnumber(&_14, 140, 0.15f);
    body->getStrat()->DoVelCrl(_14 / 2.16f / body->mSpeedScale);

    if (_14 > 141) {
        pos.set(0.0f, -40 * body->_3a4, 40 * body->_3a4);
        PSMTXMultVecSR(body->mPlayerPosMtx, &pos, &dir);
        body->DoForce(&body->mPos, &dir);
        return 0;
    }
    return 1;
}

bool KartCannon::DoRainbowLanding() {
    KartBody *body = mBody;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;

    pos.set(0.0f, -30 * body->_3a4, 50 * body->_3a4);
    PSMTXMultVecSR(body->mPlayerPosMtx, &pos, &dir);
    body->DoForce(&body->mPos, &dir);
    return 1;
}

bool KartCannon::DoRainbowCheckEnd() {
    KartBody *body = mBody;

    if (body->mCarStatus & 0x8000000000ULL) {
        if (body->mPos.y - body->mBodyGround.getHeight() > 600.0f) {
            return 1;
        }

        mState = 0;
        GetKartCtrl()->getKartAnime(body->mMynum)->mFlags |= 0x01000000;
        body->mCarStatus &= 0xFFFFFF7FFFFFFFFFULL;
        return 1;
    }

    body->mCarStatus &= 0xFFFFFFFEFFFFFFFFULL;
    body->mCarStatus |= 0x8000000000ULL;
    body->_584 = 0;
    return 1;
}

void KartCannon::DoCannonCrl() {
    if (_9 & 5) {
        DoDonkyCannonCrl();
    } else {
        DoRainbowCannonCrl();
    }
}

void KartCannon::DoDonkyCannonCrl() {
    KartBody *body = mBody;

    for (int i = 0; i < 4; i++) {
        GetKartCtrl()->getKartSus(body->mMynum * 4 + i)->_10c += 0.1395555f;
    }

    if (mTimer != 0) {
        mTimer--;
    }

    switch (mState) {
    case 0:
        DoKeep();
        DoCatch();
        mCounter = 0;
        mState = 1;
        break;
    case 1:
        mCounter++;
        DoKeep();
        if (mCounter == 1) {
            DoLaunch();
            GetKartCtrl()->getKartAnime(body->mMynum)->mFlags |= 0x800000;
            mCounter = 0;
            mState = 2;
        }
        break;
    case 2:
        if ((u8)DoLaunch() == 1) {
            mCounter = 0;
            mState = 3;
            if (mPoint != NULL) {
                CLPoint *point = mPoint->getCLPoint();
                GetKartCtrl()->getKartTarget(body->mMynum)->DoSearchRescueTarget(point);
                if (body->mGameStatus & 0x8) {
                    GetKartCtrl()->getKartEnemy(body->mMynum)->getBodyCtrl()->comebackRescure(point, false);
                }
            }
            GetKartCtrl()->getKartTarget(body->mMynum)->DoSearchTarget(body->mMynum);
            body->mBodyGround.reset();
        }
        break;
    case 3:
        if (DoFlight() == true) {
            mState = 4;
            mCounter = 0;
        }
        break;
    case 4:
        if (DoTouchDown()) {
            mState = 6;
            mCounter = 0;
            DoCheckEnd();
        }
        break;
    case 5:
        mCounter++;
        DoLanding();
        if (mCounter == 5) {
            mState = 6;
            mCounter = 0;
        }
        break;
    case 6:
        DoCheckEnd();
        break;
    }
}

void KartCannon::DoRainbowCannonCrl() {
    KartBody *body = mBody;

    for (int i = 0; i < 4; i++) {
        GetKartCtrl()->getKartSus(body->mMynum * 4 + i)->_10c += 0.1395555f;
    }

    if (mTimer != 0) {
        mTimer--;
    }

    switch (mState) {
    case 0:
        DoKeep();
        DoCatch();
        mCounter = 0;
        mState = 1;
        break;
    case 1:
        mCounter++;
        DoKeep();
        if (mCounter == 1) {
            DoRainbowLaunch();
            GetKartCtrl()->getKartAnime(body->mMynum)->mFlags |= 0x800000;
            mCounter = 0;
            mState = 2;
        }
        break;
    case 2:
        if ((u8)DoRainbowLaunch() == 1) {
            mCounter = 0;
            mState = 3;
            if (mPoint != NULL) {
                CLPoint *point = mPoint->getCLPoint();
                GetKartCtrl()->getKartTarget(body->mMynum)->DoSearchRescueTarget(point);
                if (body->mGameStatus & 0x8) {
                    GetKartCtrl()->getKartEnemy(body->mMynum)->getBodyCtrl()->comebackRescure(point, false);
                }
            }
            GetKartCtrl()->getKartTarget(body->mMynum)->DoSearchTarget(body->mMynum);
            body->mBodyGround.reset();
        }
        break;
    case 3:
        if (DoRainbowFlight() == true) {
            mState = 4;
            mCounter = 0;
        }
        break;
    case 4:
        if (DoRainbowTouchDown() == true) {
            mState = 5;
            mCounter = 0;
        }
        break;
    case 5:
        mCounter++;
        DoRainbowLanding();
        if (mCounter == 5) {
            mState = 6;
            mCounter = 0;
            mFlags &= ~0x20;
        }
        break;
    case 6:
        DoRainbowCheckEnd();
        break;
    }
}

void KartCannon::DoAfterCannonCrl() {
    KartBody *body = mBody;

    body->mWg.z = 0;
    body->mWg.y = 0;

    switch (mState) {
    case 0:
    case 1:
        break;
    case 3: {
        f32 stick = GetKartCtrl()->GetCoDriveCont(body->mMynum)->getMainStickY() - 0.1f;

        if (body->_464 < -0.174444f && stick > 0) {
            stick = 0;
        }
        if (body->_464 > 0.680333f && stick < 0) {
            stick = 0;
        }
        body->mWg.x = 0.0348888f * stick;
        break;
    }
    case 2:
        return;
    }
}
