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

void KartGame::GetGorundTireNum() {}

void KartGame::WatchEffectAcceleration() {}

void KartGame::WatchAcceleration() {}

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

void KartGame::SetDriftTurboSterr() {}

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

bool KartGame::DoRollOver() {}

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

void KartGame::MakeClear() {}

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

void KartGame::DoBodyAction() {}

void KartGame::DoElementForce() {}

bool KartGame::CheckBalloon() {}

void KartGame::SetRank() {
    KartBody *body = mBody;
    body->mMyRank = RCMGetKartChecker(body->mMynum)->getRank();
    body->_59c = 0;
}

void KartGame::RankWatchMan() {}

void KartGame::ItemWatchMan(ItemObj *) {}

void KartGame::AfterItemWatchMan() {}

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
