#include "Yamamoto/KartGame.h"
#include "Yamamoto/kartBody.h"
#include "Kaneshige/RaceMgr.h"
#include "Yamamoto/KartDamage.h"
#include "Sato/ItemObjMgr.h"

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

void KartGame::DoChange() {}

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
    // void JUTGamePad::getMainStickX() const {}
}

void KartGame::DoWarmUpRoll() {}

void KartGame::DoRollAnim() {}

void KartGame::DoDriftClear() {
    KartBody *body = mBody;
    body->mMTBoost = 0;
    body->mDriftSterr = 0;
    body->mMTState = 0;
    body->mCarStatus &= ~0x20000000000ull;
    body->mCarStatus &= ~0x1800000000003ull;
}

void KartGame::DoRoll() {}

void KartGame::DoTestPitch() {
    // void JUTGamePad::getMainStickY() const {}
}

void KartGame::DoLiftTurbo() {}

void KartGame::DoTurbo() {}

void KartGame::DoRollThrow() {}

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

void KartGame::MakeBoardDash() {}

void KartGame::MakeJumpDash() {}

void KartGame::MakeSpJumpDash() {}

void KartGame::MakeMashDash() {}

void KartGame::MakeGoldenMashDash() {}

void KartGame::MakeStartDash() {}

void KartGame::MakeCrashDash() {}

void KartGame::MakeWheelSpin() {}

void KartGame::MakeJump() {}

void KartGame::DoAirCheck() {}

void KartGame::DoRearSlidePower() {}

void KartGame::DoRearSlideBody() {
    // void JGeometry::TVec3<float>::div(float) {}
}

void KartGame::DoCorner() {}

void KartGame::FrameWork(float, KartSus *, KartSus *) {}

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

void KartGame::DoElementForce() {}

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

void KartGame::RankWatchMan() {}

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
