#include "Yamamoto/KartGame.h"
#include "Yamamoto/kartBody.h"
#include "Kaneshige/RaceMgr.h"
#include "Yamamoto/KartDamage.h"
#include "Yamamoto/kartParams.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/ItemWanWan.h"
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

void KartGame::DoSlide() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if (body->getChecker()->CheckPartsClearKey(num) || !(body->mGameStatus & 1))
        return;
    if (body->_5b6 != 0)
        body->_5b6--;
    if (body->_5b6 == 0)
        body->mCarStatus &= ~4;
    if ((body->mCarStatus & 4) != 0) {
        if (body->_5b6 < 29)
            return;
        body->_2cc.x += body->_2f0.x * body->_528;
        body->_2cc.y += body->_2f0.y * body->_528;
        body->_2cc.z += body->_2f0.z * body->_528;
        return;
    }
    KartGamePad *coCont = GetKartCtrl()->GetCoDriveCont(num);
    if (body->getTouchNum() == 0 || GetKartCtrl()->GetCarSpeed(num) <= 50.0f)
        return;
    if (coCont->testTrigger(64) || coCont->testTrigger(32)) {
        body->_5b6 = 35;
        if (RaceMgr::getCurrentManager()->isMirror()) {
            if (coCont->testTrigger(64)) {
                body->_528 = -50.0f * body->_3a4;
                GetKartCtrl()->getKartAnime(num)->mFlags |= 0x10;
                JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)27, num, body->mPos, 1);
            } else if (coCont->testTrigger(32)) {
                body->_528 = 50.0f * body->_3a4;
                GetKartCtrl()->getKartAnime(num)->mFlags |= 0x20;
                JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)27, num, body->mPos, 0);
            }
        } else {
            if (coCont->testTrigger(32)) {
                body->_528 = -50.0f * body->_3a4;
                GetKartCtrl()->getKartAnime(num)->mFlags |= 0x10;
                JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)27, num, body->mPos, 1);
            } else if (coCont->testTrigger(64)) {
                body->_528 = 50.0f * body->_3a4;
                GetKartCtrl()->getKartAnime(num)->mFlags |= 0x20;
                JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)27, num, body->mPos, 0);
            }
        }
        body->_2cc.x += body->_2f0.x * body->_528;
        body->_2cc.y += body->_2f0.y * body->_528;
        body->_2cc.z += body->_2f0.z * body->_528;
        body->mCarStatus |= 4;
        GetKartCtrl()->getKartSound(num)->DoStrikeSound();
        GetKartCtrl()->getKartSound(num)->DoTandemVoice(2);
    }
}

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

f32 KartGame::DoWarmUpRoll() {
    KartBody *body = mBody;
    int num = body->mMynum;
    f32 ret = 0.0f;
    KartGamePad *cont = GetKartCtrl()->GetDriveCont(num);
    if ((body->mCarStatus & 4) != 0) {
        body->mCarStatus &= ~0x1820000000003ull;
        body->_4d8 = 0.0f;
        body->_4d4 = 0.0f;
        body->_4c4 = 0.0f;
        body->_394 = 0.0f;
        body->mLiftframe = 0.0f;
        body->_39c = 0.0f;
        body->_398 = 0.0f;
        body->mMTBoost = 0;
        body->mDriftSterr = 0;
        body->mMTState = 0;
        body->mSterrNorm = 0.0f;
    }
    if ((body->mCarStatus & 3) != 0 && cont->testTrigger(GetKartCtrl()->getKartPad(num)->mTrigL | GetKartCtrl()->getKartPad(num)->mTrigR)) {
        DoDriftClear();
        return 0.0f;
    }
    if (cont->testButton(GetKartCtrl()->getKartPad(num)->mTrigL | GetKartCtrl()->getKartPad(num)->mTrigR)) {
        if ((body->mCarStatus & 1) != 0) {
            body->mCarStatus |= 0x800000000001ull;
            ret = 150.0f;
            body->mCarStatus &= ~0x1000000000002ull;
            CheckDriftTurbo();
        } else if ((body->mCarStatus & 2) != 0) {
            body->mCarStatus |= 0x1000000000002ull;
            ret = -150.0f;
            body->mCarStatus &= ~0x800000000001ull;
            CheckDriftTurbo();
        } else {
            if (body->mFrame < -0.8f) {
                body->_4d4 = ret;
                body->mCarStatus |= 0x800000000001ull;
                ret = 150.0f;
                body->mCarStatus &= ~0x1000000000002ull;
                body->mSterrNorm = 0.043611f;
                if (body->mFrame > -0.90000004f)
                    body->mSterrNorm = 0.1133888f;
            } else if (body->mFrame > 0.8f) {
                body->_4d4 = ret;
                body->mCarStatus |= 0x1000000000002ull;
                ret = -150.0f;
                body->mCarStatus &= ~0x800000000001ull;
                body->mSterrNorm = 0.043611f;
                if (body->mFrame < 0.90000004f)
                    body->mSterrNorm = 0.1133888f;
            } else {
                body->mMTBoost = 0;
                body->mDriftSterr = 0;
                body->mMTState = 0;
                body->mCarStatus &= ~0x20000000000ull;
                body->mCarStatus &= ~0x1800000000003ull;
            }
        }
        if (!GetKartCtrl()->getKartAnime(num)->IsProhibition(num) && !body->mDriverModels[0]->IsSit()
            && !body->getChecker()->CheckOnlyTandemPartsClearKey(num) && (body->mCarStatus & 3) == 0)
            GetKartCtrl()->getKartAnime(num)->mFlags |= 0x200;
    } else {
        if ((body->mCarStatus & 0x8000) == 0 && body->mDriverModels[0]->IsSit() && !body->mDriverModels[0]->IsStand()
            && !body->getChecker()->CheckOnlyTandemPartsClearKey(num))
            GetKartCtrl()->getKartAnime(num)->mFlags |= 0x400;
        if (body->mMTState >= 2) {
            body->mCarStatus |= 0x20000000000ull;
            body->mMTBoost = body->mMTBoostMax;
            JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)26, num, body->mPos, 0);
            GetKartCtrl()->getKartSound(body->mMynum)->DoKartMiniTuroboSound();
            GetKartCtrl()->getKartSound(body->mMynum)->DoTandemVoice(0);
            body->getStrat()->DoMotor((MotorManager::MotorType)6);
        }
        body->mDriftSterr = 0;
        body->mMTState = 0;
        ret = 0.0f;
        body->mCarStatus &= ~0x1800000000003ull;
    }
    return ret;
}

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
    if (GetKartCtrl()->GetCarSpeed(body->mMynum) < 40.0f && body->_510 > 2.44222f)
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
    f32 target;
    f32 step = 0.0f;
    if (stick < 0.5 && stick > -0.5f) {
        target = step;
    } else {
        target = 1.5f * stick;
        step = 0.2f;
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
        vec20.x *= 1.075f;
        vec20.z *= 1.015f;
        if (vec20.z < 0.0f)
            vec20.z *= -1.0f;
        PSMTXMultVecSR(body->_110, &vec20, &body->mVel);
    }
    body->_518 = body->_514;
    f32 rate;
    f32 share0;
    f32 f1 = body->_4dc * body->_3c8;
    f32 total = body->_518 * f1;
    body->_564 = 0.38f;
    rate = body->_564;
    vec2c.set(body->_344.x, body->_344.y, body->_344.z);
    share0 = total * rate;
    f32 share1 = 1.0f - rate;
    vec20.set(body->_3a4 * share0, 0.0f, body->_3a4 * (total * share1));
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

void KartGame::DoTurbo() {
    KartBody *body = mBody;
    int num = body->mMynum;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f force;
    JGeometry::TVec3f worldPos;
    JGeometry::TVec3f worldForce;
    KartGamePad *cont = GetKartCtrl()->GetDriveCont(num);
    if (body->_464 > 0.785f || body->_464 < -0.785f)
        return;
    if (body->_32c.y < 0.705f || body->mBodyGround.getAttribute() == 6 || body->getTouchNum() < 2 || 0.0f == body->_3c8)
        return;
    if ((body->mCarStatus & 0xe000c20ull) != 0)
        return;
    if (0.0f == body->_3ec)
        return;
    if ((body->mGameStatus & 8) && body->mFrame < 0.2f && body->mFrame > -0.2f)
        return;
    GetKartCtrl()->DevMatrixByVector(&force, &body->mVel, body->_110);
    if ((body->mCarStatus & 0x40000) != 0 && !(body->mGameStatus & 8) && (body->mFrame > 0.5f || body->mFrame < -0.5f)) {
        force.x *= 1.01f;
        force.z *= 0.99f;
    } else if ((body->mCarStatus & 0x8000) != 0 && !(body->mGameStatus & 8) && (body->mFrame > 0.5f || body->mFrame < -0.5f)) {
        force.x *= 1.03f;
        force.z *= 0.97f;
    } else if (!(body->mGameStatus & 8) && (body->mFrame > 0.6f || body->mFrame < -0.6f)
               && cont->testButton(GetKartCtrl()->getKartPad(num)->mAccelBtn)) {
        if (body->mFrame > 0.8f || body->mFrame < -0.8f) {
            force.x *= 1.035f;
            force.z *= 0.948f;
        } else {
            force.x *= 1.025f;
            force.z *= 0.96f;
        }
    }
    if (body->mGameStatus & 8)
        force.z *= 0.97f;
    PSMTXMultVecSR(body->_110, &force, &body->mVel);
    f32 rate;
    if (body->_458 > 60.0f)
        rate = body->_458 / body->_3ec;
    else
        rate = body->_458 / (2.5f * body->_3ec);
    if (rate > 0.5f)
        rate = 1.0f;
    f32 scale = 1.0f;
    f32 power = scale * (rate * (1.465f * body->_3c8));
    f32 ratio = 0.08f;
    if ((body->mCarStatus & 0x40000) != 0 || 0.0f == body->mFrame)
        ratio = 0.0f;
    pos.set(body->_344.x, body->_344.y, body->_344.z);
    force.set(-body->_3a4 * (power * ratio), 0.0f, body->_3a4 * (power * (1.0f - ratio)));
    if (body->mFrame < 0.0f)
        force.x *= -1.0f;
    PSMTXMultVec(body->_110, &pos, &worldPos);
    PSMTXMultVecSR(body->_110, &force, &worldForce);
    body->DoForce(&worldPos, &worldForce);
    pos.set(body->_344.x, body->_344.y, 0.5f * -body->_344.z);
    force.set(-body->_3a4 * (power * ratio), 0.0f, body->_3a4 * (body->_4dc * (1.0f - ratio)));
    if (body->mFrame < 0.0f)
        force.x *= -1.0f;
    PSMTXMultVec(body->_110, &pos, &worldPos);
    PSMTXMultVecSR(body->_110, &force, &worldForce);
    body->DoForce(&worldPos, &worldForce);
}

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
    if (body->getTouchNum() == 0 || (body->mCarStatus & 0x41000) != 0 || (body->mGameStatus & 8)) {
        return false;
    }
    return false;
}

void KartGame::DoWanWan() {
    KartBody *body = mBody;
    int num = body->mMynum;
    JGeometry::TVec3f nodePos;
    JGeometry::TVec3f bodyPos;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f pull;
    body->_2b4.zero();
    if ((body->mCarStatus & 0x1000) == 0)
        return;
    if (!body->getItem()->IsMyItem()) {
        body->mCarStatus &= ~0x1000ull;
        GetKartCtrl()->getKartAnime(num)->mFlags |= 0x1000000;
        GetKartCtrl()->getKartSound(num)->DoWanWanStopSound();
        body->getStrat()->DoMotor((MotorManager::MotorType)4);
        GetKartCtrl()->getKartSound(body->mMynum)->DoKartsetSeSound(0x1007c);
        return;
    }
    ItemObj *item = GetItemObjMgr()->getKartEquipItem(num);
    if (item == nullptr)
        return;
    if (item->getKind() != 7)
        return;
    ItemWanWanObj *wanwan = (ItemWanWanObj *)ItemObjMgr::getItemWanWanObj(item);
    KartGamePad *cont = GetKartCtrl()->GetDriveCont(num);
    body->getStrat()->DashClear();
    wanwan->getPullVec(&pull);
    f32 pullLen = pull.length();
    if (pullLen <= 0.0f)
        return;
    wanwan->getFirstNodePos(&nodePos);
    bodyPos.set(body->_1a0[0][3], body->_1a0[1][3], body->_1a0[2][3]);
    dir.sub(nodePos, bodyPos);
    f32 dist = dir.length();
    f32 chainLength = wanwan->getChainLength();
    bool over = false;
    f32 work;
    if (dist > chainLength) {
        work = dist - chainLength;
        if (work > 150.0f) {
            body->getItem()->ReleseWanWan();
            body->_2b4.zero();
            return;
        }
        if (work > 10.0f) {
            f32 power = work / 27.0f;
            power *= 8.0f;
            power += 1.0f;
            if (power > 8.0f)
                power = 8.0f;
            if (_09 == 0) {
                body->getStrat()->DoPowerMotor(1.0f, power, 0);
                _09 = 16;
            }
            if (_09 != 0)
                _09--;
        }
        body->_2b4.set(dir);
        body->_2b4.normalize();
        body->_2b4.scale(work);
    }
    if (dist > chainLength)
        over = true;
    dir.normalize();
    body->getStrat()->DoRotPose(dir, 0.8f);
    body->_3ec = 2.16f * pullLen;
    body->_3ec = 200.0f;
    bodyPos.set(0.0f, 0.0f, 0.0f);
    PSMTXMultVec(body->_110, &bodyPos, &nodePos);
    if (over == true) {
        work = pullLen * body->_3a4;
        pull.x = body->_308.x * work;
        pull.y = body->_308.y * work;
        pull.z = body->_308.z * work;
        body->DoForce(&nodePos, &pull);
        pull.x = body->_2fc.x * -(0.1f * work);
        pull.y = body->_2fc.y * -(0.1f * work);
        pull.z = body->_2fc.z * -(0.1f * work);
        body->DoForce(&nodePos, &pull);
    } else {
        work = pullLen * -body->_3a4;
        pull.x = body->_308.x * work;
        pull.y = body->_308.y * work;
        pull.z = body->_308.z * work;
        body->DoForce(&nodePos, &pull);
    }
    bodyPos.set(0.0f, 0.0f, 3.0f);
    PSMTXMultVec(body->_110, &bodyPos, &nodePos);
    work = -(10.0f * cont->getMainStickX());
    JGeometry::TVec3f difVel;
    difVel.x = wanwan->getDifVel().x;
    difVel.y = wanwan->getDifVel().y;
    difVel.z = wanwan->getDifVel().z;
    GetKartCtrl()->DevMatrixByVector2(&dir, &difVel, body->_110);
    work = 80.0f * -dir.x + work;
    if (work > 30.0f)
        work = 30.0f;
    if (work < -30.0f)
        work = -30.0f;
    work *= body->_3a4;
    pull.x = body->_2f0.x * work;
    pull.y = body->_2f0.y * work;
    pull.z = body->_2f0.z * work;
    body->DoForce(&nodePos, &pull);
    body->_3c8 = body->_3d0;
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
    if (body->mBodyGround.getAttribute() == 6 || GetKartCtrl()->GetCarSpeed(num) >= 60.0f || (body->mCarStatus & 0x100100509000ull) != 0) {
        body->mCarStatus &= ~8ull;
    }
    if ((body->mCarStatus & 8) != 0)
        GetKartCtrl()->getKartSound(num)->DoSpinTurnSound();
}

void KartGame::DoAirCheck() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if (!GetKartCtrl()->IsMiniGameEnd() && body->mBodyGround.getStaggerCode() && (body->mCarStatus & 0x100000000ull) == 0
        && !GetKartCtrl()->getKartAnime(num)->IsDangerPlusAnime(num) && !GetKartCtrl()->getKartAnime(num)->IsDamage(num)) {
        if (body->_460 >= 0.0f)
            GetKartCtrl()->getKartAnime(num)->mFlags |= 0x4000000;
        else
            GetKartCtrl()->getKartAnime(num)->mFlags |= 0x2000000;
        GetKartCtrl()->getKartSound(num)->DoStaggerVoice();
    }
    if (!(_08 & 1) && body->getTouchNum() == 0 && GetKartCtrl()->getKartAnime(num)->_08 < 10.0f)
        GetKartCtrl()->getKartAnime(num)->_08 += 0.5f;
    if (!(_08 & 1) && body->getTouchNum() == 0 && body->mPlayerPosMtx[1][3] - body->mBodyGround.getHeight() > 150.0f
        && body->mVel.y > 0.0f) {
        _08 |= 1;
        if (!GetKartCtrl()->getKartAnime(num)->IsJump(num) && !GetKartCtrl()->getKartAnime(num)->IsDangerAnime(num)
            && !GetKartCtrl()->getKartAnime(num)->IsFloatAnime(num) && !GetKartCtrl()->getKartAnime(num)->IsGaol(num)
            && !GetKartCtrl()->getKartAnime(num)->IsDrift(num) && !body->getChecker()->CheckPartsClearKey(num)) {
            if ((body->mCarStatus & 0x8000) != 0)
                GetKartCtrl()->getKartAnime(num)->_04 = 0.3f;
            else
                GetKartCtrl()->getKartAnime(num)->_04 = 0.2f;
            GetKartCtrl()->getKartAnime(num)->mFlags |= 0x2000000000ull;
        }
    }
    if (body->getTouchNum() == 0 || (body->mCarStatus & 0x40) != 0) {
        if ((body->mCarStatus & 0x40) == 0 && body->mPlayerPosMtx[1][3] - body->mBodyGround.getHeight() > 300.0f)
            body->mCarStatus |= 0x40;
    }
}

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
    KartBody *body = mBody;
    int num = body->mMynum;
    JGeometry::TVec3f vec;
    if (GetKartCtrl()->GetCarSpeed(num) <= 20.0f || body->_32c.y < 0.705f) {
        body->_35c.zero();
        return;
    }
    if (body->mBodyGround.getAttribute() == 6) {
        body->_35c.zero();
        return;
    }
    if ((body->mCarStatus & 0xc000420ull) != 0) {
        body->_35c.zero();
        return;
    }
    f32 rate = 0.2f;
    KartGamePad *cont = GetKartCtrl()->GetDriveCont(num);
    if (cont->testButton(GetKartCtrl()->getKartPad(num)->mBtnB) || !cont->testButton(GetKartCtrl()->getKartPad(num)->mAccelBtn))
        rate = 0.3f;
    if (GetKartCtrl()->GetCarSpeed(num) <= 50.0f || (body->mCarStatus & 7) != 0 || body->getTouchNum() <= 1) {
        GetKartCtrl()->ChaseFnumber(&body->_35c.x, 0.0f, rate);
        GetKartCtrl()->ChaseFnumber(&body->_35c.z, 0.0f, 0.2f);
        if (body->getTouchNum() != 0)
            DoRearSlidePower();
        return;
    }
    if (body->_3c8 <= 0.1f * body->_3d0 || (body->mFrame <= 0.8f && body->mFrame >= -0.8f)) {
        GetKartCtrl()->ChaseFnumber(&body->_35c.x, 0.0f, rate);
        GetKartCtrl()->ChaseFnumber(&body->_35c.z, 0.0f, 0.2f);
        if (body->getTouchNum() != 0)
            DoRearSlidePower();
        return;
    }
    GetKartCtrl()->DevMatrixByVector(&vec, &body->_2cc, body->_110);
    vec.div(body->_3a4);
    f32 frame = body->mFrame;
    if (frame < 0.0f)
        frame *= -1.0f;
    f32 speedRate = body->_458 / 100.0f;
    if (speedRate > 1.0f)
        speedRate = 1.0f;
    if (vec.x > 14.0f)
        vec.x = 14.0f;
    if (vec.x < -14.0f)
        vec.x = -14.0f;
    if (vec.z > 7.0f)
        vec.z = 7.0f;
    if (vec.z < -7.0f)
        vec.z = -7.0f;
    f32 scale = body->_48c;
    if (scale > 1.0f)
        scale = 1.0f;
    vec.x = vec.x * (scale * (frame * speedRate));
    if (cont->testButton(GetKartCtrl()->getKartPad(num)->mBtnB) || !cont->testButton(GetKartCtrl()->getKartPad(num)->mAccelBtn))
        vec.x *= 0.6f;
    GetKartCtrl()->ChaseFnumber(&body->_35c.x, vec.x, 0.0525f);
    GetKartCtrl()->ChaseFnumber(&body->_35c.z, vec.z, 0.055f);
    DoRearSlidePower();
    body->mWg.y *= 0.005f * frame + 1.0f;
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
    f32 f = 20.0f * speed * (diff * (7.0f * body->_3ac));
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
        body->mVel.scale(0.9999f);
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
    if (mCountDownDuration != 0 || RaceMgr::getCurrentManager()->getRaceMode() == 1 || GetKartCtrl()->IsMiniGame())
        return;
    KartChecker *checker = RCMGetKartChecker(num);
    if (body->_59c == 0 && body->mMyRank > checker->getRank() && GetKartCtrl()->GetCarSpeed(num) > 60.0f && !body->getChecker()->CheckOnlyTandemPartsClearKey(num))
        body->_59c = 120;
    if (body->mMyRank > checker->getRank() && (body->mCarStatus & 0x8000) != 0)
        body->_59c = 100;
    if (body->mMyRank < checker->getRank() && body->_59c >= 90)
        body->_59c = 89;
    if (body->_59c == 90 && GetKartCtrl()->GetCarSpeed(num) > 60.0f && !body->getChecker()->CheckOnlyTandemPartsClearKey(num))
        GetKartCtrl()->getKartSound(num)->DoPathVoice();
    if (body->_59c != 0)
        body->_59c--;
    body->mMyRank = checker->getRank();
    if (_0e != 0)
        _0e--;
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
        KartBody *ownerBody;
        int ownerNum = _14->getOwnerNum();
        ownerBody = GetKartCtrl()->getKartBody(ownerNum);
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

bool KartGame::KeepWatch() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if (body->getChecker()->CheckSpecialDyn())
        return true;
    int rescue = 0;
    u8 type;
    KartSus *sus0 = GetKartCtrl()->getKartSus(num * 4);
    KartSus *sus1 = GetKartCtrl()->getKartSus(num * 4 + 1);
    KartSus *sus2 = GetKartCtrl()->getKartSus(num * 4 + 2);
    KartSus *sus3 = GetKartCtrl()->getKartSus(num * 4 + 3);
    if (body->mBodyGround.getAttribute() == 255) {
        if ((sus0->mCrsGnd.getAttribute() == 255 && (sus0->_124 & 1)) || (sus1->mCrsGnd.getAttribute() == 255 && (sus1->_124 & 1))
            || (sus2->mCrsGnd.getAttribute() == 255 && (sus2->_124 & 1)) || (sus3->mCrsGnd.getAttribute() == 255 && (sus3->_124 & 1))) {
            body->getRescue()->MakeRescue(nullptr, 6);
            return true;
        }
    } else {
        if (body->mBodyGround.getJugemPoint() != nullptr) {
            if (body->mBodyGround.getAttribute() == 17) {
                if (body->_1a0[1][3] - body->mBodyGround.getWaterHeight() < -RescueSeaDepth[body->mIdx]) {
                    type = 5;
                    rescue = 1;
                }
            } else if (body->mBodyGround.getAttribute() == 10) {
                if (body->mPlayerPosMtx[1][3] - body->mBodyGround.getHeight() < 120.0f) {
                    GetKartCtrl()->getKartSound(num)->DoFallVoice();
                    if (!GetKartCtrl()->getKartAnime(num)->IsDangerPlusAnime(num) && !GetKartCtrl()->getKartAnime(num)->IsDamage(num)) {
                        if (body->_460 >= 0.0f)
                            GetKartCtrl()->getKartAnime(num)->mFlags |= 0x4000000;
                        else
                            GetKartCtrl()->getKartAnime(num)->mFlags |= 0x2000000;
                    }
                    type = 1;
                    rescue = 1;
                }
            } else if ((sus0->mCrsGnd.getAttribute() == 5 && (sus0->_124 & 1)) || (sus1->mCrsGnd.getAttribute() == 5 && (sus1->_124 & 1))
                || (sus2->mCrsGnd.getAttribute() == 5 && (sus2->_124 & 1)) || (sus3->mCrsGnd.getAttribute() == 5 && (sus3->_124 & 1))) {
                type = 0;
                rescue = 1;
            } else if ((sus0->mCrsGnd.getAttribute() == 15 && (sus0->_124 & 1)) || (sus1->mCrsGnd.getAttribute() == 15 && (sus1->_124 & 1))
                || (sus2->mCrsGnd.getAttribute() == 15 && (sus2->_124 & 1)) || (sus3->mCrsGnd.getAttribute() == 15 && (sus3->_124 & 1))) {
                type = 2;
                rescue = 1;
            } else if ((sus0->mCrsGnd.getAttribute() == 14 && (sus0->_124 & 1)) || (sus1->mCrsGnd.getAttribute() == 14 && (sus1->_124 & 1))
                || (sus2->mCrsGnd.getAttribute() == 14 && (sus2->_124 & 1)) || (sus3->mCrsGnd.getAttribute() == 14 && (sus3->_124 & 1))) {
                type = 3;
                rescue = 1;
            } else if ((sus0->mCrsGnd.getAttribute() == 255 && (sus0->_124 & 1)) || (sus1->mCrsGnd.getAttribute() == 255 && (sus1->_124 & 1))
                || (sus2->mCrsGnd.getAttribute() == 255 && (sus2->_124 & 1)) || (sus3->mCrsGnd.getAttribute() == 255 && (sus3->_124 & 1))) {
                type = 6;
                rescue = 1;
            }
            if (rescue >= 1 && (body->mCarStatus & 0x40000000) == 0) {
                body->getRescue()->MakeRescue(nullptr, type);
                return true;
            }
        }
        if (body->mBodyGround.getCannonPoint() != nullptr) {
            body->getCannon()->MakeCannon();
            return true;
        }
        if (body->mBodyGround.getPipePoint() != nullptr) {
            body->getPipe()->MakePipe();
            return true;
        }
    }
    return false;
}

inline ItemObjMgr::KartHitList *ItemObjMgr::getKartHitList(int idx) { return &mHitList[idx]; }

void KartGame::DoActionMgr() {
    KartBody *body = mBody;
    int num = body->mMynum;
    ItemObjMgr *mgr = GetItemObjMgr();
    ItemObjMgr::KartHitList *hitList = mgr->getKartHitList(num);
    GeographyObj **geoList = GetGeoObjMgr()->getKartReactHitObjectList(body->mMynum);
    if (KeepWatch())
        return;
    if (!GetKartCtrl()->IsMiniGameEnd() && !(body->mGameStatus & 0x400) && (body->mCarStatus & 0x40000) == 0
        && !body->getCrash()->CheckMatchlessTimer()) {
        GeographyObj *geoObj;
        ItemObj *item;
        if (geoList[16] != nullptr) {
            body->getCrash()->MakeTornado();
        } else if ((geoObj = geoList[17]) != nullptr) {
            body->getRescue()->MakeRescue(geoObj, 4);
        } else if ((item = hitList->mObjects[9]) != nullptr) {
            body->getTumble()->MakeWanWanTumble(item);
        } else if ((item = hitList->mObjects[4]) != nullptr) {
            body->getCrash()->MakeKameCrash(item);
        } else if ((item = hitList->mObjects[3]) != nullptr) {
            body->getCrash()->MakeKameCrash(item);
        } else if ((item = hitList->mObjects[8]) != nullptr) {
            body->getCrash()->MakeBombCrash(item);
        } else if ((item = hitList->mObjects[2]) != nullptr) {
            body->getCrash()->MakeSpin(item);
        } else if ((item = hitList->mObjects[0]) != nullptr) {
            body->getCrash()->MakeHalfSpin(item);
        } else if ((item = hitList->mObjects[11]) != nullptr) {
            body->getTumble()->MakeKameTumble(item);
        } else if ((item = hitList->mObjects[10]) != nullptr) {
            body->getCrash()->MakeBurn(item);
        } else if (hitList->mObjects[12] != nullptr) {
            body->getThunder()->MakeThunder();
        } else if (body->_590 & 4) {
            body->getCrash()->MakeSpin(nullptr);
        } else if (body->_590 & 8) {
            body->getCrash()->MakeHalfSpin(nullptr);
        }
        if ((body->mCarStatus & 0x100000) != 0)
            return;
        if (body->_590 & 0x10) {
            body->getTumble()->MakeStarTumble();
        } else if (body->getDamage()->mFlags & 4) {
            body->getCrash()->MakeHalfSpin(nullptr);
        } else if (body->getDamage()->mFlags & 2) {
            body->getDamage()->MakeHalfDamage();
        } else if (body->getChecker()->CheckCrash() == true) {
            return;
        }
    }
    DoRollOver();
    if (hitList->mObjects[7] != nullptr) {
        body->getStar()->MakeStar();
        return;
    }
    if (hitList->mObjects[6] != nullptr) {
        MakeMashDash();
        return;
    }
    if (body->mBodyGround.isObject() && body->getTouchNum() >= 1) {
        switch (body->mBodyGround.getObject()->getKind()) {
        case 1021:
            MakeBoardDash();
            break;
        }
        return;
    }
    if (body->mBodyGround.getAttribute() == 7 && body->getTouchNum() >= 1)
        MakeJumpDash();
    if (body->mBodyGround.getAttribute() == 55 && body->getTouchNum() >= 1)
        MakeSpJumpDash();
    if (body->mBodyGround.getAttribute() == 71 && body->getTouchNum() >= 1)
        MakeSpJumpDash();
    if (body->mBodyGround.getAttribute() == 8 && body->getTouchNum() >= 1)
        MakeBoardDash();
}

void KartGame::DoActionCtrl() {
    if (mBody->getChecker()->CheckCrash() != true) {
        DoSlide();
        DoWanWan();
    }
}

void KartGame::DoStatus() {
    KartBody *body = mBody;
    int num = body->mMynum;
    if (!body->getChecker()->CheckIndication()) {
        if (body->mGameStatus & 0x4000) {
            DoStartGoalCtl();
        } else {
            return;
        }
    } else {
        DoStartGoalCtl();
    }
    body->DegubBody(0);
    if (!(body->mGameStatus & 8))
        WatchEffectAcceleration();
    body->DegubBody(1);
    WatchAcceleration();
    body->DegubBody(2);
    if (!(body->mGameStatus & 8))
        GetKartCtrl()->getKartSound(num)->DoInfo(num);
    body->DegubBody(3);
    GetKartCtrl()->DoContCtl(num);
    body->DegubBody(4);
    if (body->mGameStatus & 8)
        GetKartCtrl()->getKartSound(num)->DoInfo(num);
    if (body->mGameStatus & 8)
        WatchEffectAcceleration();
    GetGorundTireNum();
    body->DegubBody(5);
    if (!(body->mGameStatus & 4))
        return;
    body->getItem()->CheckOtherAnime();
    body->DegubBody(6);
    if (!body->getChecker()->CheckCrash()) {
        GetKartCtrl()->getKartSound(num)->DoHorn();
        body->DegubBody(7);
        DoRoll();
        body->DegubBody(8);
        if (!(body->mGameStatus & 8))
            DoTestPitch();
        body->DegubBody(9);
        DoLiftTurbo();
        body->DegubBody(10);
        if ((body->mCarStatus & 3) == 0)
            DoTurbo();
        body->DegubBody(11);
        if (!(body->mGameStatus & 8))
            DoCorner();
        body->DegubBody(12);
        if (!(body->mGameStatus & 8))
            DoRearSlideBody();
        body->DegubBody(13);
        DoBodyAction();
        body->DegubBody(14);
    }
    DoAirCheck();
    body->DegubBody(15);
    DoChange();
    body->DegubBody(16);
    DoActionCtrl();
    body->DegubBody(17);
    if (!body->getChecker()->CheckCrash())
        body->getItem()->DoItem();
    body->DegubBody(18);
    GetKartCtrl()->DoAnime(num);
    body->DegubBody(19);
    DoActionMgr();
    body->DegubBody(20);
    DoFlagCtrl();
    body->DegubBody(21);
    if ((body->mCarStatus & 0x400000) != 0)
        body->getRescue()->DoRescueCrl();
    body->DegubBody(22);
    if ((body->mCarStatus & 0x8100000000ull) != 0)
        body->getCannon()->DoCannonCrl();
    body->DegubBody(23);
    if ((body->mCarStatus & 0x100000000000ull) != 0)
        body->getPipe()->DoPipeCrl();
    body->DegubBody(24);
    if ((body->mCarStatus & 0x200000000000ull) != 0)
        body->getAnt()->DoAntCrl();
    body->DegubBody(25);
    if ((body->mCarStatus & 0x200000000ull) != 0)
        body->getDossin()->DoDossinCrl();
    body->DegubBody(26);
    body->getDossin()->DoDossinTimer();
    body->DegubBody(27);
    if ((body->mCarStatus & 0x40000) != 0)
        body->getStar()->DoStarCrl();
    body->DegubBody(28);
    if (body->getTumble()->_4 & 1)
        body->getTumble()->DoTumbleCrl();
    body->DegubBody(29);
    if (body->getThunder()->mFlags & 1)
        body->getThunder()->DoThunderCrl();
    body->DegubBody(30);
    if (body->getDamage()->mFlags & 0x170)
        body->getDamage()->DoDamageCrl();
    body->DegubBody(31);
    body->getGame()->AfterItemWatchMan();
    body->DegubBody(32);
}
