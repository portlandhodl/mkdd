#include "Yamamoto/KartGame.h"
#include "Yamamoto/kartBody.h"
#include "Kaneshige/RaceMgr.h"
#include "Yamamoto/KartDamage.h"
#include "Sato/ItemObjMgr.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

// comments inside functions are inline functions being called in that function

void KartGame::Init(int) {}

void KartGame::GetGorundTireNum() {}

void KartGame::WatchEffectAcceleration() {}

void KartGame::WatchAcceleration() {}

void KartGame::DoItmCancel() {}

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

void KartGame::DoDriftClear() {}

void KartGame::DoRoll() {}

void KartGame::DoTestPitch() {
    // void JUTGamePad::getMainStickY() const {}
}

void KartGame::DoLiftTurbo() {}

void KartGame::DoTurbo() {}

void KartGame::DoRollThrow() {}

void KartGame::DoRollOver() {}

void KartGame::DoWanWan() {
    // void ItemWanWanObj::getDifVel() const {}
    // void ItemWanWanObj::getPullVec(JGeometry::TVec3<float> *) {}
}

void KartGame::DoPushStart() {}

void KartGame::DoBalance(float *, float) {}

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

void KartGame::DoActionCtrl() {}

void KartGame::DoStatus() {
    // void KartCtrl::DoAnime(int) {}
}
