#include "Yamamoto/KartAnime.h"
#include "Yamamoto/kartCtrl.h"
#include "Kawano/driver.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

// comments inside functions are inline functions being called in that function

bool KartAnime::IsAppeal(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsAppeal() != 0);
}

bool KartAnime::IsThrow(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsThrow() != 0);
}

bool KartAnime::IsMove(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsMove() != 0);
}

bool KartAnime::IsChangeStart() {
    return mFlags & 1;
}

bool KartAnime::IsSuspend(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (IsChangeStart() == true)
        return true;
    if (IsChange(idx) == 1)
        return true;
    return (u8)!(1 - IsMove(idx));
}

bool KartAnime::IsDamage(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDamage() != 0);
}

bool KartAnime::IsDamageL(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDamageL() != 0);
}

bool KartAnime::IsDamageS(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDamageS() != 0);
}

bool KartAnime::IsRide(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsStart() != 0);
}

bool KartAnime::IsNormalStart(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsNormalStart() != 0);
}

bool KartAnime::IsBackStart(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsTurnBack() != 0);
}

bool KartAnime::IsRun(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsRun() != 0);
}

bool KartAnime::IsDrift(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDrift() != 0);
}

bool KartAnime::IsDriftStart(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDriftStart() != 0);
}

bool KartAnime::IsDriftLeft(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDriftLeft() != 0);
}

bool KartAnime::IsDriftRight(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDriftRight() != 0);
}

bool KartAnime::IsChange(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsChange() != 0);
}

bool KartAnime::IsRocket(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsRocketStart() != 0);
}

bool KartAnime::IsJump(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsJump() != 0);
}

bool KartAnime::IsSit(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsSit() != 0);
}

bool KartAnime::IsBackAnime(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsBack() != 0);
}

bool KartAnime::IsFloatAnime(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDash() != 0);
}

bool KartAnime::IsDangerAnime(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDanger() != 0);
}

bool KartAnime::IsDangerLoopAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (body->mBodyGround.getAttribute() != 14 && body->mBodyGround.getStaggerCode() == 0) {
        if (body->mDriverModels[0]->IsDanger() >= 2)
            return true;
    }
    return false;
}

bool KartAnime::IsDangerPlusAnime(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDangerPlus() != 0);
}

bool KartAnime::IsGetItemAnime(int idx) {
    return GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsItemThrowOK() == 0;
}

bool KartAnime::IsBurnAnime(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsFire() != 0);
}

bool KartAnime::IsDrive(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[1]->IsDriverDamage() != 0);
}

bool KartAnime::IsBack(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (body->mDriverModels[1]->IsBack() != 0)
        return true;
    return (u8)(body->mDriverModels[0]->IsBack() != 0);
}

bool KartAnime::IsGaol(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsGoal() != 0);
}

bool KartAnime::IsAttack(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsAttack() != 0);
}

bool KartAnime::IsProhibition(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (body->mDriverModels[0]->IsAttack() != 0)
        return true;
    if (body->mDriverModels[0]->IsDrift() != 0)
        return true;
    return (u8)(body->mDriverModels[0]->IsAppeal() != 0);
}

void KartAnime::MakeThrowAnime(int a, int b, int c) {
    _20 = a;
    _24 = b;
    _1c = c;
    mFlags |= 2;
}

void KartAnime::MakeBackAnime(int) {}

void KartAnime::MakeFrontAnime(int) {}

void KartAnime::Init() {}

void KartAnime::DoGetAnime(int) {
    //void ItemObj::clrTransferPartner() {}
    //void ItemObj::tstTransferPartner() const {}
}

void KartAnime::DoBackAnime(int) {
    //void DriverModel::TurnBack() {}
}

void KartAnime::DoFrontAnime(int) {}

void KartAnime::DoChangeAnime(int) {}

void KartAnime::DoDriveAnime(int) {}

void KartAnime::DoStartRunStartAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartStart();
}

void KartAnime::DoStartRunEndAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->BacktoDWait();
}

void KartAnime::DoDriverThrowAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[1]->StartDriverThrow(_3c, _40);
    mDamageFlags &= ~2;
}

void KartAnime::DoDriverDamageAnime(int) {}

void KartAnime::DoDriverDriveAnime(int) {}

void KartAnime::DoThrowAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartThrow(_20, _24, _1c);
    mFlags &= ~2;
}

void KartAnime::DoDriftAnime(int) {}

void KartAnime::DoDriftEndAnime(int) {}

void KartAnime::DoSlideAnime(int) {}

void KartAnime::DoJumpAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (!IsSuspend(idx)) {
        if (mFlags & 8)
            body->mDriverModels[0]->StartJump(1);
        else
            body->mDriverModels[0]->StartJump(0);
    }
    mFlags &= ~0x40;
}

void KartAnime::DoCrouchAnime(int) {}

void KartAnime::DoStandAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (body->mDriverModels[0]->IsSit() && !body->mDriverModels[0]->IsStand() && !IsSuspend(idx))
        body->mDriverModels[0]->StartStand();
    mFlags &= ~0x400;
}

void KartAnime::DoPushStartAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartStart();
    mFlags &= ~0x800;
}

void KartAnime::DoNormalStartAnime(int) {
    //void DriverModel::StartNormalStart() {}
}

void KartAnime::DoRocketStartAnime(int) {
    //void DriverModel::StartRocketStart() {}
}

void KartAnime::DoFalseStartAnime(int) {
    //void DriverModel::StartRocketStartFailure() {}
}

void KartAnime::DoDamageStartAnime(int) {}

void KartAnime::DoDamageEndAnime(int) {}

void KartAnime::DoBigDamageStartAnime(int) {}

void KartAnime::DoBigDamageEndAnime(int) {}

void KartAnime::DoBurnStartAnime(int) {}

void KartAnime::DoBurnEndAnime(int) {}

void KartAnime::DoFloatStartAnime(int) {}

void KartAnime::DoHangStartAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartHang();
    mFlags &= ~(u64)0x100000000;
}

void KartAnime::DoGoalStartAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (body->mDriverModels[0]->IsGoal() == 0)
        body->mDriverModels[0]->StartGoalMotion();
    if (body->mDriverModels[1]->IsGoal() == 0)
        body->mDriverModels[1]->StartGoalMotion();
    mFlags &= ~(u64)0x200000000;
}

void KartAnime::DoStaffAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (body->mDriverModels[0]->IsGoal() == 0)
        body->mDriverModels[0]->StartGood();
    if (body->mDriverModels[1]->IsGoal() == 0)
        body->mDriverModels[1]->StartGood();
    mFlags &= ~(u64)0x10000000000;
}

void KartAnime::DoHappyAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    body->mDriverModels[0]->StartHappy();
    body->mDriverModels[1]->StartHappy();
    mFlags &= ~(u64)0x400000000;
}

void KartAnime::DoWinAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    body->mDriverModels[1]->StartGood1st();
    body->mDriverModels[0]->StartGood1st();
    mFlags &= ~(u64)0x800000000;
    mFlags = 0;
}

void KartAnime::DoDefeatAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartBad();
    mFlags &= ~(u64)0x1000000000;
}

void KartAnime::DoFloatEndAnime(int) {}

void KartAnime::DoSurpriseAnime(int) {}

void KartAnime::DoSurpriseEndAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    int n = 0;
    if (IsBack(idx) == 1)
        n = 1;
    body->mDriverModels[0]->StartDangerEnd(n);
    mFlags &= ~0x6000000;
}

void KartAnime::DoHangJumpStartAnime(int) {}

void KartAnime::DoHangJumpEndAnime(int) {}

void KartAnime::DoAppealAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartAppeal();
    mFlags &= ~(u64)0x8000000000;
}

void KartAnime::DoStopAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    body->mDriverModels[0]->SetFreeze();
    body->mDriverModels[1]->SetFreeze();
}

void KartAnime::DoReStopAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    body->mDriverModels[0]->ResetFreeze();
    body->mDriverModels[1]->ResetFreeze();
}

void KartAnime::DoCheck(int) {}

void KartAnime::DoAnime(int) {}
