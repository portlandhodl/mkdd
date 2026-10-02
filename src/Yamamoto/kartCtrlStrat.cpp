#include "Yamamoto/KartGame.h"
#include "Yamamoto/kartBody.h"
#include "Kaneshige/RaceMgr.h"
#include "Yamamoto/KartDamage.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/stEffectMgr.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

// comments inside functions are inline functions being called in that function

void KartGame::Init(int idx) {
    mBody = GetKartCtrl()->getKartBody(idx);
    _08 = 0;
    _09 = 0;
    _38.zero();
    _04 = 0;
    _18 = 0.0f;
    _1c = 0.0f;
    _0e = 0;
    _10 = 0;
    mCountDownDuration = 0;
    _0a = 0;
    _0b = 0;
    _0c = 0;
    RaceMgr::getCurrentManager()->getStartPoint(&_20, &_2C, idx);
    _20.y += 300.0f;
}

void KartGame::GetGorundTireNum() {
    KartBody *body = mBody;
    int num = body->mMynum;
    body->mTouchNum = 0;
    KartSus *sus0 = GetKartCtrl()->getKartSus(num * 4);
    KartSus *sus1 = GetKartCtrl()->getKartSus(num * 4 + 1);
    KartSus *sus2 = GetKartCtrl()->getKartSus(num * 4 + 2);
    KartSus *sus3 = GetKartCtrl()->getKartSus(num * 4 + 3);
    if (sus0->_124 & 1)
        body->mTouchNum++;
    if (sus1->_124 & 1)
        body->mTouchNum++;
    if (sus2->_124 & 1)
        body->mTouchNum++;
    if (sus3->_124 & 1)
        body->mTouchNum++;
    GetKartCtrl()->getKartSound(num)->DoSlipSound(num);
    JGeometry::TVec3f pos;
    pos.set(body->mPlayerPosMtx[0][3], body->mPlayerPosMtx[1][3], body->mPlayerPosMtx[2][3]);
    body->mShadowArea.searchShadow(pos);
    if (body->mTouchNum != 0 && body->_58c == 7) {
        ExGeographyObj *obj = body->mBodyGround.getObject();
        JGeometry::TVec3f dir;
        dir.set(0.0f, -3.5f, 0.0f);
        obj->AddVel(pos, dir);
    }
}

void KartGame::WatchEffectAcceleration() {
    KartBody *body = mBody;
    int num = body->mMynum;
    KartGamePad *cont = GetKartCtrl()->GetDriveCont(num);
    if (cont->testButton(GetKartCtrl()->getKartPad(num)->mAccelBtn)) {
        body->mKartRPM = GetKartCtrl()->fcnvge(body->mKartRPM, 1.0f, 0.05f, 0.05f);
    } else {
        body->mKartRPM = GetKartCtrl()->fcnvge(body->mKartRPM, 0.0f, 0.05f, 0.05f);
    }
}

void KartGame::WatchAcceleration() {
    KartBody *body = mBody;
    int num = body->mMynum;
    KartGamePad *cont = GetKartCtrl()->GetDriveCont(num);
    if ((body->mCarStatus & 0x400000) != 0) {
        if (body->getRescue()->_76 >= 3) {
            if (cont->testButton(GetKartCtrl()->getKartPad(num)->mAccelBtn)) {
                body->_3c8 = GetKartCtrl()->fcnvge(body->_3c8, body->_3d0, 1.0f, 1.0f);
                _08 |= 2;
            } else {
                GetKartCtrl()->ChaseFnumber(&body->_3c8, 0.0f, 1.0f);
                _08 &= ~2;
            }
        }
    }
}

void KartGame::DoItmCancel() {
    KartBody *body = mBody;
    body->mCarStatus |= 0x80000000;
    GetItemObjMgr()->abortItemShuffle(body->mMynum);
}

void KartGame::DoStopItm() {
    KartBody *body = mBody;
    u8 num = body->mMynum;
    body->mCarStatus |= 0x80000000;
    GetItemObjMgr()->abortItemShuffle(body->mMynum);

    ItemObjMgr *mgr = GetItemObjMgr();
    ItemObj *item = mgr->getKartEquipItem(num, 0);
    mgr->deleteHeartItem(num);
    if (item) {
        if (item->IsSuccessionItem())
            item->setChildStateForceDisappear();
        else
            item->setStateForceDisappear();
    }

    item = mgr->getKartEquipItem(num, 1);
    if (item) {
        if (item->IsSuccessionItem())
            item->setChildStateForceDisappear();
        else
            item->setStateForceDisappear();
    }
}

void KartGame::DoChange() {
    KartBody *body = mBody;
    int num = body->mMynum;
    KartGamePad *driveCont = GetKartCtrl()->GetDriveCont(num);
    KartGamePad *coDriveCont = GetKartCtrl()->GetCoDriveCont(num);
    bool changed = false;
    bool changePossible = GetKartCtrl()->MakeChangePossible(num);
    if (body->getChecker()->CheckCheange(num)) {
        _0b = 0;
        return;
    }
    if (_0b != 0)
        _0b--;
    if ((body->mGameStatus & 1) == 0) {
        if (driveCont->testTrigger(GetKartCtrl()->getKartPad(num)->mTrigZ))
            _0b = 20;
        if ((body->mCarStatus & 0x80) != 0) {
            if (body->mDriverModels[2]->IsChange())
                goto check;
            if (body->mDriverModels[3]->IsChange())
                goto check;
            body->mCarStatus &= ~0x80ull;
        } else {
            if (driveCont->testTrigger(GetKartCtrl()->getKartPad(num)->mTrigZ) || changePossible || _0b != 0) {
                _0b = 0;
                changed = true;
            }
        }
    } else {
        if ((body->mCarStatus & 0x80) != 0) {
            if (driveCont->testButton(GetKartCtrl()->getKartPad(num)->mTrigZ) && coDriveCont->testButton(GetKartCtrl()->getKartPad(num)->mTrigZ))
                _0b = 20;
            if (body->mDriverModels[2]->IsChange())
                goto check;
            if (body->mDriverModels[3]->IsChange())
                goto check;
            body->mCarStatus &= ~0x80ull;
        } else {
            if ((driveCont->testButton(GetKartCtrl()->getKartPad(num)->mTrigZ) && coDriveCont->testButton(GetKartCtrl()->getKartPad(num)->mTrigZ)) || _0b != 0) {
                _0b = 0;
                changed = true;
            }
        }
    }
check:
    if (changed) {
        body->mCarStatus &= ~0x4000000000ull;
        body->mCarStatus |= 0x80;
        GetKartCtrl()->getKartAnime(num)->mFlags |= 1;
        GetKartCtrl()->getKartSound(num)->DoChangeVoice();
        GetKartCtrl()->getKartSound(num)->DoChangeStarSound();
    }
}

void KartGame::DoSlide() {}

void KartGame::DoDriftTurboSterr() {
    KartBody *body = mBody;
    if (body->mDriftSterr != 0 && body->mDriftSterr < 30) {
        body->mDriftSterr++;
    }
}

void KartGame::SetDriftTurboSterr() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if (body->mDriftSterr >= ((body->mGameStatus & 1) ? 2 : 6)) {
        body->mMTState++;
        if (body->mMTState == 1) {
            GetKartCtrl()->getKartSound(num)->DoKartsetSeSound(0x100c9);
        } else if (body->mMTState == 2) {
            GetKartCtrl()->getKartSound(num)->DoKartsetSeSound(0x100ca);
        }
        if (body->mMTState >= 2)
            body->mMTState = 2;
        body->mDriftSterr = 0;
    }
}

void KartGame::CheckDriftTurbo() {
    KartBody *body = mBody;
    int num = body->mMynum;

    if (GetKartCtrl()->GetCarSpeed(num) <= 50.0f) {
        body->mCarStatus &= ~0x20000000000ull;
        body->mMTBoost = 0;
        body->mDriftSterr = 0;
        body->mMTState = 0;
        return;
    }
    if (body->mGameStatus & 8)
        return;
    bool drifted = false;
    KartGamePad *coCont = GetKartCtrl()->GetCoDriveCont(num);
    if ((body->mCarStatus & 1) != 0) {
        if ((body->mGameStatus & 1) != 0) {
            if (coCont->getMainStickX() < -0.3f) {
                drifted = true;
                DoDriftTurboSterr();
            }
        } else if (body->mFrame >= -0.5f) {
            drifted = true;
            DoDriftTurboSterr();
        }
        if (!drifted) {
            SetDriftTurboSterr();
            body->mDriftSterr = 1;
        }
    } else if ((body->mCarStatus & 2) != 0) {
        if ((body->mGameStatus & 1) != 0) {
            if (coCont->getMainStickX() > 0.3f) {
                drifted = true;
                DoDriftTurboSterr();
            }
        } else if (body->mFrame <= 0.5f) {
            drifted = true;
            DoDriftTurboSterr();
        }
        if (!drifted) {
            SetDriftTurboSterr();
            body->mDriftSterr = 1;
        }
    } else {
        body->mMTState = 0;
        body->mDriftSterr = 0;
    }
    body->mCarStatus &= ~0x20000000000ull;
    body->mMTBoost = 0;
}

f32 KartGame::DoWarmUpRoll() {}

void KartGame::DoRollAnim() {
    KartBody *body = mBody;
    int num = body->mMynum;
    KartGamePad *cont = GetKartCtrl()->GetDriveCont(num);
    if (cont->getMainStickX() >= 0.2f) {
        body->_394 = -1.0f;
    } else if (cont->getMainStickX() <= -0.2f) {
        body->_394 = 1.0f;
    } else {
        body->_394 = 0.0f;
    }
    body->mLiftframe = GetKartCtrl()->fcnvge(body->mLiftframe, body->_394, 0.05f, 0.05f);
    if (!GetKartCtrl()->getKartAnime(num)->IsDrift(num) && !GetKartCtrl()->getKartAnime(num)->IsDriftStart(num)) {
        if (cont->getMainStickX() >= 0.2f) {
            body->_39c = -1.0f;
        } else if (cont->getMainStickX() <= -0.2f) {
            body->_39c = 1.0f;
        } else {
            body->_39c = 0.0f;
        }
        body->_398 = GetKartCtrl()->fcnvge(body->_398, body->_39c, 0.05f, 0.05f);
    }
    if ((body->mCarStatus & 1) != 0) {
        body->_394 = -1.0f;
        body->mLiftframe = -1.0f;
        if (GetKartCtrl()->getKartAnime(num)->IsDriftLeft(num)) {
            body->_39c = 1.0f;
            body->_398 = 1.0f;
        } else {
            body->_39c = -1.0f;
            body->_398 = -1.0f;
        }
    } else if ((body->mCarStatus & 2) != 0) {
        body->_394 = 1.0f;
        body->mLiftframe = 1.0f;
        if (GetKartCtrl()->getKartAnime(num)->IsDriftRight(num)) {
            body->_39c = -1.0f;
            body->_398 = -1.0f;
        } else {
            body->_39c = 1.0f;
            body->_398 = 1.0f;
        }
    }
}

void KartGame::DoDriftClear() {
    KartBody *body = mBody;
    body->mMTBoost = 0;
    body->mDriftSterr = 0;
    body->mMTState = 0;
    body->mCarStatus &= ~0x20000000000ull;
    body->mCarStatus &= ~0x1800000000003ull;
}

void KartGame::DoRoll() {
    KartBody *body = mBody;
    MakeJump();
    f32 roll = DoWarmUpRoll();
    if (GetKartCtrl()->GetCarSpeed(body->mMynum) <= 30.0f || (GetKartCtrl()->GetCarSpeed(body->mMynum) <= 40.0f && body->_3c8 == 0.0f))
        DoDriftClear();
    if (GetKartCtrl()->GetCarSpeed(body->mMynum) < 80.0f && body->_510 > 2.44222f)
        DoDriftClear();
    if (GetKartCtrl()->GetCarSpeed(body->mMynum) <= 80.0f && body->_3cc != 0.0f)
        DoDriftClear();
    if ((body->mCarStatus & 8) != 0)
        DoDriftClear();
    if (body->_458 < 2.0f) {
        body->_4c4 = GetKartCtrl()->fcnvge(body->_4c4, 0.0088235294f * roll, 0.1f, 0.1f);
    } else if (body->_458 < 18.0f) {
        body->_4c4 = GetKartCtrl()->fcnvge(body->_4c4, 0.0023529413f * roll, 0.1f, 0.1f);
    } else {
        if (body->getTouchNum() == 0) {
            roll = 0.023529412f * roll;
        } else {
            roll = (body->_4f8 / 170.0f) * roll;
        }
        GetKartCtrl()->ChaseFnumber(&body->_4c4, roll, body->_3b4);
    }
    if ((body->mCarStatus & 3) == 0)
        body->_4c4 = 0.0f;
    DoRollAnim();
}

void KartGame::DoTestPitch() {
    KartBody *body = mBody;
    KartGamePad *cont = GetKartCtrl()->GetDriveCont(body->mMynum);
    if (body->getTouchNum() != 0) {
        body->_4c0 = 0.0f;
        if ((body->mCarStatus & 0x300) != 0)
            body->getStrat()->PitchClear();
        return;
    }
    f32 stick = cont->getMainStickY();
    if (stick > 0.0f) {
        body->mCarStatus |= 0x200;
        body->mCarStatus &= ~0x100ull;
    } else if (stick < 0.0f) {
        body->mCarStatus |= 0x100;
        body->mCarStatus &= ~0x200ull;
    }
    f32 step = 0.0f;
    f32 target;
    if (stick >= 0.8 || stick <= -0.5f) {
        target = 1.5f * stick;
        step = 0.2f;
    } else {
        target = step;
    }
    GetKartCtrl()->ChaseFnumber(&body->_4c0, target, step);
    if (body->_4c0 == 0.0f)
        body->getStrat()->PitchClear();
}

void KartGame::DoLiftTurbo() {
    KartBody *body = mBody;
    JGeometry::TVec3f vec2c;
    JGeometry::TVec3f vec20;
    JGeometry::TVec3f vec14;
    JGeometry::TVec3f vec8;

    if ((body->mCarStatus & 3) == 0)
        body->_518 = 0.0f;
    if ((body->mCarStatus & 3) == 0 || body->getTouchNum() <= 1 || (body->mCarStatus & 0xc000420ull) != 0 || body->mBodyGround.getAttribute() == 6) {
        body->_564 = 0.0f;
        body->_518 = 0.0f;
        return;
    }
    if (GetKartCtrl()->GetCarSpeed(body->mMynum) <= 30.0f)
        return;
    if (GetKartCtrl()->GetCarSpeed(body->mMynum) >= 40.0f && body->_3c8 != 0.0f) {
        GetKartCtrl()->DevMatrixByVector(&vec20, &body->mVel, body->_110);
        vec20.z *= 1.015f;
        vec20.x *= 1.05f;
        if (vec20.z < 0.0f)
            vec20.z *= -1.0f;
        PSMTXMultVecSR(body->_110, &vec20, &body->mVel);
    }
    body->_518 = body->_514;
    f32 f1 = body->_4dc * body->_3c8;
    body->_564 = 0.38f;
    f32 total = body->_518 * f1;
    vec2c.set(body->_344.x, body->_344.y, body->_344.z);
    f32 share0 = total * body->_564;
    f32 share1 = total * (1.0f - body->_564);
    vec20.set(body->_3a4 * share0, 0.0f, body->_3a4 * share1);
    if ((body->mCarStatus & 2) != 0)
        vec20.x *= -1.0f;
    PSMTXMultVec(body->_110, &vec2c, &vec14);
    PSMTXMultVecSR(body->_110, &vec20, &vec8);
    body->DoForce(&vec14, &vec8);
    vec2c.set(body->_344.x, body->_344.y, 0.5f * -body->_344.z);
    vec20.set(body->_3a4 * share0, 0.0f, body->_3a4 * (body->_4dc * share1));
    if ((body->mCarStatus & 2) != 0)
        vec20.x *= -1.0f;
    PSMTXMultVec(body->_110, &vec2c, &vec14);
    PSMTXMultVecSR(body->_110, &vec20, &vec8);
    body->DoForce(&vec14, &vec8);
}

void KartGame::DoTurbo() {}

void KartGame::DoRollThrow() {
    KartBody *body = mBody;
    JGeometry::TVec3f vec1;
    JGeometry::TVec3f vec2;
    JGeometry::TVec3f vec3;
    JGeometry::TVec3f vec4;
    JGeometry::TVec3f vec5;
    f32 push = 3.0f * body->_3a4;
    vec1.set(0.0f, 70.0f, 0.0f);
    PSMTXMultVec(body->_110, &vec1, &vec2);
    vec1.set(0.0f, 70.0f, 100.0f);
    PSMTXMultVec(body->_110, &vec1, &vec3);
    vec4.x = vec2.x - vec3.x;
    vec4.y = vec2.y - vec3.y;
    vec4.z = vec2.z - vec3.z;
    f32 len = GetKartCtrl()->VectorLengthSqrtf(&vec4);
    f32 over = len - 1.0f;
    if (over > 0.0f) {
        vec5.x = vec4.x * (-push * over / len);
        vec5.y = vec4.y * (-push * over / len);
        vec5.z = vec4.z * (-push * over / len);
        body->DoForce(&vec2, &vec5);
    }
}

bool KartGame::DoRollOver() {
    KartBody *body = mBody;
    if (body->getTouchNum() != 0 && (body->mCarStatus & 0x41000) == 0) {
        if (body->mGameStatus & 8) {
            return false;
        }
    }
    return false;
}

void KartGame::DoWanWan() {
    // void ItemWanWanObj::getDifVel() const {}
    // void ItemWanWanObj::getPullVec(JGeometry::TVec3<float> *) {}
}

bool KartGame::DoPushStart() {
    KartBody *body = mBody;
    body->_594++;
    body->_3c8 = body->_3d0;
    if (body->_594 == 30) {
        body->mCarStatus &= ~0x2000000ull;
        return true;
    }
    return false;
}

void KartGame::DoBalance(f32 *balance, f32 scale) {
    KartBody *body = mBody;
    if ((body->mCarStatus & 0xc000420) != 0)
        return;
    if (body->_468 > -0.40122193f && body->_468 < 0.40122193f)
        return;
    *balance *= scale;
}

void KartGame::MakeClear() {
    KartBody *body = mBody;
    body->_3c8 = 0.0f;
    body->getStrat()->LiftClear();
    body->getStrat()->PitchClear();
    body->getStrat()->AllGravyClear();
    body->getStrat()->DashClear();
    body->getStrat()->OtherClear();
}

void KartGame::MakeBoardDash() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if ((body->mCarStatus & 0x1000) != 0)
        return;
    GetKartCtrl()->getKartSound(num)->DoDashSound();
    body->mCarStatus &= ~0x40020004000ull;
    body->getStrat()->DoMotor(MotorManager::MotorType_7);
    if ((body->mCarStatus & 0x20000) != 0) {
        body->mBoostTimer = 60;
        return;
    }
    body->mCarStatus |= 0x28000;
    body->mBoostTimer = 60;
    body->_52c = 0.4f;
    body->_474 = 0.313f;
    JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)23, num, body->mPos, 2);
    if (GetKartCtrl()->CheckCamera(num)) {
        int camNum = GetKartCtrl()->GetCameraNum(num);
        if (GetKartCtrl()->getKartCam(camNum)->GetCameraMode() == 0) {
            JPEffectPerformer::setEffectEachCam((JPEffectPerformer::EffectType)35, num, (u8)camNum, 0);
        }
    }
}

void KartGame::MakeJumpDash() {
    KartBody *body = mBody;
    if ((body->mCarStatus & 0x1000) != 0)
        return;
    int num = body->mMynum;
    GetKartCtrl()->getKartSound(num)->DoDashSound();
    body->mCarStatus &= ~0x40000034000ull;
    body->getStrat()->DoMotor(MotorManager::MotorType_7);
    if ((body->mCarStatus & 0x20000000) != 0) {
        body->mBoostTimer = 20;
        return;
    }
    body->mCarStatus |= 0x20008000;
    body->mBoostTimer = 20;
    body->_52c = 0.4f;
    body->_474 = 0.313f;
    JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)23, num, body->mPos, 2);
    if (GetKartCtrl()->CheckCamera(num)) {
        int camNum = GetKartCtrl()->GetCameraNum(num);
        if (GetKartCtrl()->getKartCam(camNum)->GetCameraMode() == 0) {
            JPEffectPerformer::setEffectEachCam((JPEffectPerformer::EffectType)35, num, (u8)camNum, 0);
        }
    }
    _04 = body->mBodyGround.getJugemPoint();
    if (_04 != nullptr) {
        JGeometry::TVec3f vec;
        _04->getPosition(&_38);
    } else {
        _1c = 0.0f;
        _18 = 0.0f;
        _04 = nullptr;
        _38.set(body->mPos);
    }
}

void KartGame::MakeSpJumpDash() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if ((body->mCarStatus & 0x1000) != 0)
        return;
    GetKartCtrl()->getKartSound(num)->DoDashSound();
    body->mCarStatus &= ~0x20034000ull;
    body->getStrat()->DoMotor(MotorManager::MotorType_7);
    if ((body->mCarStatus & 0x40000000000ull) != 0) {
        body->mBoostTimer = 20;
        return;
    }
    body->mCarStatus |= 0x40000008000ull;
    body->mBoostTimer = 20;
    body->_52c = 0.4f;
    body->_474 = 0.313f;
    JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)23, num, body->mPos, 2);
    if (GetKartCtrl()->CheckCamera(num)) {
        int camNum = GetKartCtrl()->GetCameraNum(num);
        if (GetKartCtrl()->getKartCam(camNum)->GetCameraMode() == 0) {
            JPEffectPerformer::setEffectEachCam((JPEffectPerformer::EffectType)35, num, (u8)camNum, 0);
        }
    }
    _04 = body->mBodyGround.getJugemPoint();
    if (_04 != nullptr) {
        JGeometry::TVec3f vec;
        _04->getPosition(&_38);
    } else {
        _1c = 0.0f;
        _18 = 0.0f;
        _04 = nullptr;
        _38.set(body->mPos);
    }
}

void KartGame::MakeMashDash() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if ((body->mCarStatus & 0x1000) != 0)
        return;
    GetStEfctMgr()->createKinokoDashEmt(num);
    GetKartCtrl()->getKartSound(num)->DoMashDashSound();
    GetKartCtrl()->getKartSound(num)->DoDashVoice();
    if ((body->mCarStatus & 0x40020020000ull) != 0)
        return;
    body->mCarStatus &= ~0x40020030000ull;
    body->getStrat()->DoMotor(MotorManager::MotorType_5);
    if ((body->mCarStatus & 0x4000) != 0) {
        body->mBoostTimer = 80;
        return;
    }
    body->mCarStatus |= 0xc000;
    body->mBoostTimer = 80;
    body->_52c = 0.4f;
    body->_474 = 0.313f;
    if (GetKartCtrl()->CheckCamera(num)) {
        int camNum = GetKartCtrl()->GetCameraNum(num);
        if (GetKartCtrl()->getKartCam(camNum)->GetCameraMode() == 0) {
            JPEffectPerformer::setEffectEachCam((JPEffectPerformer::EffectType)35, num, (u8)camNum, 0);
        }
    }
}

void KartGame::MakeGoldenMashDash() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if ((body->mCarStatus & 0x1000) != 0)
        return;
    GetStEfctMgr()->createKinokoDashEmt(num);
    GetKartCtrl()->getKartSound(num)->DoMashDashSound();
    if ((body->mCarStatus & 0x40020020000ull) != 0)
        return;
    body->mCarStatus &= ~0x40020020000ull;
    body->getStrat()->DoMotor(MotorManager::MotorType_5);
    if ((body->mCarStatus & 0x4000) != 0) {
        body->mBoostTimer = 80;
        GetKartCtrl()->getKartSound(num)->DoGoldenDashVoice();
        return;
    }
    GetKartCtrl()->getKartSound(num)->DoGoldenDashVoice();
    body->mCarStatus |= 0xc000;
    body->mBoostTimer = 80;
    body->_52c = 0.4f;
    body->_474 = 0.313f;
    if (GetKartCtrl()->CheckCamera(num)) {
        int camNum = GetKartCtrl()->GetCameraNum(num);
        if (GetKartCtrl()->getKartCam(camNum)->GetCameraMode() == 0) {
            JPEffectPerformer::setEffectEachCam((JPEffectPerformer::EffectType)35, num, (u8)camNum, 0);
        }
    }
}

void KartGame::MakeStartDash() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if ((body->mCarStatus & 0x10000) != 0) {
        body->mBoostTimer = 45;
        return;
    }
    GetKartCtrl()->getKartSound(num)->DoTandemVoice(33);
    body->mCarStatus |= 0x18000;
    if (body->_590 & 2) {
        GetKartCtrl()->getKartSound(num)->DoMashDashSound();
        body->mBoostTimer = 45;
        body->getStrat()->DoMotor(MotorManager::MotorType_5);
    } else {
        GetKartCtrl()->getKartSound(num)->DoKartsetSeSound(0x100b3);
        body->mBoostTimer = 90;
        body->getStrat()->DoMotor(MotorManager::MotorType_8);
    }
    body->_598 = body->mBoostTimer;
    body->_52c = 0.4f;
    body->_474 = 0.313f;
    if (body->_590 & 2) {
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)24, num, body->mPos, 0);
    } else {
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)25, num, body->mPos, 0);
    }
    if (GetKartCtrl()->CheckCamera(num)) {
        int camNum = GetKartCtrl()->GetCameraNum(num);
        if (GetKartCtrl()->getKartCam(camNum)->GetCameraMode() == 0 ||
            (GetKartCtrl()->getKartCam(camNum)->GetDemoCam()->_38 & 4 && GetKartCtrl()->getKartCam(camNum)->GetCameraMode() == 10)) {
            JPEffectPerformer::setEffectEachCam((JPEffectPerformer::EffectType)35, num, (u8)camNum, 0);
        }
    }
}

void KartGame::MakeCrashDash() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if ((body->mCarStatus & 0x1000) != 0)
        return;
    if ((body->mCarStatus & 0x4000) != 0) {
        body->mBoostTimer = 45;
        return;
    }
    GetKartCtrl()->getKartSound(num)->DoMashDashSound();
    body->mCarStatus |= 0xc000;
    body->mBoostTimer = 20;
    body->_598 = body->mBoostTimer;
    body->_52c = 0.4f;
    body->_474 = 0.313f;
    body->_3c8 = body->_3d0;
    JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)26, num, body->mPos, 0);
    if (GetKartCtrl()->CheckCamera(num)) {
        int camNum = GetKartCtrl()->GetCameraNum(num);
        if (GetKartCtrl()->getKartCam(camNum)->GetCameraMode() == 0 ||
            (GetKartCtrl()->getKartCam(camNum)->GetDemoCam()->_38 & 4 && GetKartCtrl()->getKartCam(camNum)->GetCameraMode() == 10)) {
            JPEffectPerformer::setEffectEachCam((JPEffectPerformer::EffectType)35, num, (u8)camNum, 0);
        }
    }
}

void KartGame::MakeWheelSpin() {
    KartBody *body = mBody;
    int num = body->mMynum;
    body->_584 = 8;
    body->_588 = 0;
    body->_594 = 0;
    MakeClear();
    body->getItem()->FallItem();
    body->mCarStatus |= 0x800;
    JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x20, num, body->mPos, 0);
    JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x20, num, body->mPos, 1);
    GetKartCtrl()->getKartSound(num)->DoWheelSpin();
    GetKartCtrl()->getKartSound(num)->DoTandemVoice(36);
}

void KartGame::MakeJump() {
    KartBody *body = mBody;
    int num = body->mMynum;
    KartGamePad *cont = GetKartCtrl()->GetDriveCont(num);
    if (body->getTouchNum() != 0
        && cont->testButton(GetKartCtrl()->getKartPad(num)->mTrigL)
        && cont->testButton(GetKartCtrl()->getKartPad(num)->mTrigR)
        && cont->testButton(GetKartCtrl()->getKartPad(num)->mAccelBtn)
        && (body->mFrame > 0.3f || body->mFrame < -0.3f)) {
        body->mCarStatus |= 8;
    } else {
        if ((body->mCarStatus & 8) != 0 && !cont->testButton(GetKartCtrl()->getKartPad(num)->mAccelBtn)) {
            body->_3c8 = 0.0f;
            body->mVel.x = 0.0f;
            body->mVel.z = 0.0f;
        }
        body->mCarStatus &= ~8ull;
    }
    if (body->mBodyGround.getAttribute() == 6 || !(GetKartCtrl()->GetCarSpeed(num) >= 2.5f) || (body->mCarStatus & 0x100100509000ull) != 0) {
        body->mCarStatus &= ~8ull;
    }
    if ((body->mCarStatus & 8) != 0)
        GetKartCtrl()->getKartSound(num)->DoSpinTurnSound();
}

void KartGame::DoAirCheck() {}

void KartGame::DoRearSlidePower() {
    KartBody *body = mBody;
    JGeometry::TVec3f vec1;
    JGeometry::TVec3f vec2;
    JGeometry::TVec3f vec3;
    JGeometry::TVec3f vec4;
    vec1.set(0.0f, 0.0f, -1.2f);
    vec2.set(-body->_35c.x * body->_3a4, 0.0f, body->_35c.z * body->_3a4);
    PSMTXMultVec(body->_110, &vec1, &vec3);
    PSMTXMultVecSR(body->_110, &vec2, &vec4);
    body->DoForce(&vec3, &vec4);
    vec1.set(0.0f, 0.0f, 1.0f);
    PSMTXMultVec(body->_110, &vec1, &vec3);
    PSMTXMultVecSR(body->_110, &vec2, &vec4);
    body->DoForce(&vec3, &vec4);
}

void KartGame::DoRearSlideBody() {
    // void JGeometry::TVec3<float>::div(float) {}
}

void KartGame::DoCorner() {
    KartBody *body = mBody;
    JGeometry::TVec3f vec;
    if (body->getTouchNum() == 0 || body->_458 <= 30.0f || (body->mCarStatus & 0x1003) != 0)
        body->_4d4 = 0.0f;
    if ((body->mFrame <= 0.7f && body->mFrame >= -0.7f) || body->_458 < 45.0f) {
        body->_4d4 = 0.0f;
        return;
    }
    GetKartCtrl()->DevMatrixByVector(&vec, &body->_2cc, body->_110);
    vec.x *= 0.3f;
    f32 f31 = vec.x / body->_3a4;
    if (f31 < 1.5f && f31 > -1.5f) {
        f31 = 0.0f;
    } else if (f31 > 2.5f) {
        f31 = 2.5f;
    } else if (f31 < -2.5f) {
        f31 = -2.5f;
    }
    if (body->_468 > 0.261666f || body->_468 < -0.261666f)
        f31 *= 0.1f;
    if (f31 > 2.44222f)
        f31 = 2.44222f;
    if (f31 < -2.44222f)
        f31 = -2.44222f;
    if (body->mFrame > 0.0f && f31 < 0.0f)
        f31 *= -1.0f;
    else if (body->mFrame < 0.0f && f31 > 0.0f)
        f31 *= -1.0f;
    GetKartCtrl()->ChaseFnumber(&body->_4d4, f31, 0.1f);
    DoBalance(&body->_4d4, 0.4f);
}

void KartGame::FrameWork(f32 speed, KartSus *sus1, KartSus *sus2) {
    KartBody *body = mBody;
    JGeometry::TVec3f force;
    f32 diff = sus1->_b4 - sus2->_b4;
    f32 f = 0.313f * speed * (diff * (-14.0f * body->_3ac));
    force.set(body->_110[0][1] * f, body->_110[1][1] * f, body->_110[2][1] * f);
    body->DoForce(&sus2->_c0, &force);
    GetKartCtrl()->MulVector(&force, -1.0f, -1.0f, -1.0f);
    body->DoForce(&sus1->_c0, &force);
}

void KartGame::DoBodyAction() {
    KartBody *body = mBody;
    int num = body->mMynum;
    KartSus *sus0 = GetKartCtrl()->getKartSus(num * 4);
    KartSus *sus1 = GetKartCtrl()->getKartSus(num * 4 + 1);
    KartSus *sus2 = GetKartCtrl()->getKartSus(num * 4 + 2);
    KartSus *sus3 = GetKartCtrl()->getKartSus(num * 4 + 3);
    FrameWork(body->_3b0, sus0, sus1);
    FrameWork(body->_3b0, sus2, sus3);
    FrameWork(body->_3b0, sus0, sus3);
    FrameWork(body->_3b0, sus1, sus2);
}

void KartGame::DoElementForce() {
    KartBody *body = mBody;
    if (body->getTouchNum() != 0 || (body->mCarStatus & 0x100000) != 0)
        body->mVel.scale(0.99f);
    else
        body->mVel.scale(-2.44222f);
    body->mWg.scale(0.98f);
}

bool KartGame::CheckBalloon() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if (RaceMgr::getCurrentManager()->getRaceMode() != 4)
        return false;
    if (GetKartCtrl()->HaveBalloon(num) && !(body->mCarStatus & 0x100000) && !(body->mCarStatus & 0x400000) && !(body->getThunder()->mFlags & 1))
        return true;
    return false;
}

void KartGame::SetRank() {
    KartBody *body = mBody;
    body->mMyRank = RCMGetKartChecker(body->mMynum)->getRank();
    body->_59c = 0;
}

void KartGame::RankWatchMan() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if (mCountDownDuration != 0)
        return;
    if (RaceMgr::getCurrentManager()->getRaceMode() == 1)
        return;
    if (GetKartCtrl()->IsMiniGame()) {
    } else {
        KartChecker *checker = RCMGetKartChecker(num);
        if (body->_59c == 0 && body->mMyRank > checker->getRank() && GetKartCtrl()->GetCarSpeed(num) > 2.5f && !body->getChecker()->CheckOnlyTandemPartsClearKey(num))
            body->_59c = 120;
        if (body->mMyRank > checker->getRank() && (body->mCarStatus & 0x8000) != 0)
            body->_59c = 100;
        if (body->mMyRank < checker->getRank() && body->_59c >= 90)
            body->_59c = 89;
        if (body->_59c == 90 && GetKartCtrl()->GetCarSpeed(num) > 2.5f && !body->getChecker()->CheckOnlyTandemPartsClearKey(num))
            GetKartCtrl()->getKartSound(num)->DoPathVoice();
        if (body->_59c != 0)
            body->_59c--;
        body->mMyRank = checker->getRank();
        if (_0e != 0)
            _0e--;
    }
}

void KartGame::ItemWatchMan(ItemObj *item) {
    if (item == nullptr)
        return;
    int ownerNum = item->getOwnerNum();
    KartBody *ownerBody = GetKartCtrl()->getKartBody(ownerNum);
    RCMGetKartChecker(ownerNum);
    if (ownerBody->getGame()->_0e != 0)
        return;
    if (ownerBody->getChecker()->CheckOnlyTandemPartsClearKey(ownerNum))
        return;
    if (GetKartCtrl()->CheckTandemItmGet(ownerNum))
        return;
    ownerBody->getGame()->_0e = 120;
    ownerBody->getGame()->_14 = item;
}

void KartGame::AfterItemWatchMan() {
    GetKartCtrl()->getKartSound(mBody->mMynum)->DoItemAlarm();
    if (_0e == 119) {
        int ownerNum = _14->getOwnerNum();
        KartBody *ownerBody = GetKartCtrl()->getKartBody(ownerNum);
        if (_14->getState() != 5) {
            if (!ownerBody->getChecker()->CheckOnlyTandemPartsClearKey(ownerNum)) {
                if (!GetKartCtrl()->CheckTandemItmGet(ownerNum)) {
                    GetKartCtrl()->getKartSound(ownerNum)->DoItmHitVoice();
                    GetKartCtrl()->getKartAnime(ownerNum)->mFlags |= 0x400000000ull;
                }
            }
        }
        _0e = 0;
    }
    if (_0e == 118)
        _0e = 0;
}

void KartGame::DoFlagCtrl() {
    KartBody *body = mBody;
    body->_590 &= ~0x5c;
    body->getDamage()->mFlags &= ~2;
}

void KartGame::KeepWatch() {}

void KartGame::DoActionMgr() {
    // void KartBody::getStar() {}
    // void ItemObjMgr::getKartHitList(int) {}
}

void KartGame::DoActionCtrl() {
    if (mBody->getChecker()->CheckCrash() != true) {
        DoSlide();
        DoWanWan();
    }
}

void KartGame::DoStatus() {
    // void KartCtrl::DoAnime(int) {}
}
