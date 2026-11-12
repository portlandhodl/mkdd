#include "Yamamoto/KartAnime.h"
#include "Yamamoto/kartCtrl.h"
#include "Kawano/driver.h"
#include "Sato/ItemObjMgr.h"
#include "Yamamoto/KartSound.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

inline void DriverModel::SetFreeze() { m_e8 = 1; }

inline void DriverModel::ResetFreeze() { m_e8 = 0; }

// comments inside functions are inline functions being called in that function

bool KartAnime::IsAppeal(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsAppeal() != 0);
}

bool KartAnime::IsThrow(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsThrow() != 0);
}

bool KartAnime::IsDriverThrow(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[1]->IsThrow() != 0);
}

bool KartAnime::IsMove(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsMove() != 0);
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

bool KartAnime::IsDamageS2(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDamageS2() != 0);
}

bool KartAnime::IsDrift(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDrift() != 0);
}

bool KartAnime::IsDriftStart(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDriftStart() != 0);
}

bool KartAnime::IsDriftEnd(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDriftEnd() != 0);
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

bool KartAnime::IsTandemStartOK(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsTandemAnmStartOK() != 0);
}

bool KartAnime::IsTurn(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsTurn() != 0);
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

bool KartAnime::IsDriftLimAnime(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDriftFinish() != 0);
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

bool KartAnime::IsDangerEndAnime(int idx) {
    return (u8)(GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->IsDangerEnd() != 0);
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

void KartAnime::MakeDriverThrowAnime(int a, int b, int c) {
    _3c = a;
    _40 = b;
    _38 = c;
    mDamageFlags |= 2;
}

void KartAnime::MakeThrowAnime(int a, int b, int c) {
    _20 = a;
    _24 = b;
    _1c = c;
    mFlags |= 2;
}

void KartAnime::MakeBackAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if ((mFlags & 8) == 0) {
        mFlags |= 8;
        mFlags &= ~0x104ull;
    }
}

void KartAnime::MakeFrontAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if ((mFlags & 4) == 0) {
        mFlags |= 4;
        mFlags &= ~0x108ull;
    }
}

void KartAnime::Init() {
    mFlags = 0;
    mFlags |= 0x104;
    _20 = 0;
    _24 = 0;
    _28 = 100;
    _1c = 0;
    _08 = 1.0f;
    _04 = 0.0f;
    mType = 0;
    _2c = 0;
    mDamageFlags = 0;
    _38 = 0;
    _3c = 0;
    _40 = 0;
}

void KartAnime::DoGetAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    ItemObjMgr *itemMgr = GetItemObjMgr();
    int kartNo = idx;
    ItemObj *item;
    if (!itemMgr->getNowTandemDriverNum(idx)) {
        item = itemMgr->getKartEquipItem(idx, 1);
        if (item) {
            if (item->getState() == 7)
                body->mDriverModels[1]->resetItem();
            else
                body->mDriverModels[1]->setItem(item);
        } else {
            body->mDriverModels[1]->resetItem();
        }
        item = itemMgr->getKartEquipItem(kartNo, 0);
        if (item) {
            if (item->getState() == 7) {
                body->mDriverModels[0]->resetItem();
            } else {
                body->mDriverModels[0]->setItem(item);
                if (item->getState() != 5 && item->tstTransferPartner()) {
                    GetKartCtrl()->getKartSound(idx)->DoTandemVoice(35);
                    item->clrTransferPartner();
                }
            }
        } else {
            body->mDriverModels[0]->resetItem();
        }
    } else {
        item = itemMgr->getKartEquipItem(idx, 0);
        if (item) {
            if (item->getState() == 7)
                body->mDriverModels[1]->resetItem();
            body->mDriverModels[1]->setItem(item);
        } else {
            body->mDriverModels[1]->resetItem();
        }
        item = itemMgr->getKartEquipItem(kartNo, 1);
        if (item) {
            if (item->getState() == 7) {
                body->mDriverModels[0]->resetItem();
            } else {
                body->mDriverModels[0]->setItem(item);
                if (item->getState() != 5 && item->tstTransferPartner()) {
                    GetKartCtrl()->getKartSound(idx)->DoTandemVoice(35);
                    item->clrTransferPartner();
                }
            }
        } else {
            body->mDriverModels[0]->resetItem();
        }
    }
    if (IsDangerLoopAnime(idx))
        DoSurpriseEndAnime(idx);
}

void KartAnime::DoBackAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (!IsDangerPlusAnime(idx)) {
        body->mDriverModels[0]->TurnBack();
    }
    mFlags |= 0x100;
}

void KartAnime::DoFrontAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (!IsDangerPlusAnime(idx)) {
        body->mDriverModels[0]->TurnFront();
    }
    mFlags |= 0x100;
}

void KartAnime::DoChangeAnime(int idx) {
    KartCtrl *kartCtrl = GetKartCtrl();
    KartBody *body = kartCtrl->getKartBody(idx);
    if (!kartCtrl->WhichDriver(idx)) {
        ((DriverModel *)body->mExModels[0])->goBack();
        ((DriverModel *)body->mExModels[1])->goFront();
    } else {
        ((DriverModel *)body->mExModels[1])->goBack();
        ((DriverModel *)body->mExModels[0])->goFront();
    }
    mFlags &= ~1ull;
    body->mDriver ^= 1;
    if (!kartCtrl->WhichDriver(idx)) {
        body->mDriverModels[1] = (DriverModel *)body->mExModels[0];
        body->mDriverModels[0] = (DriverModel *)body->mExModels[1];
    } else {
        body->mDriverModels[1] = (DriverModel *)body->mExModels[1];
        body->mDriverModels[0] = (DriverModel *)body->mExModels[0];
    }
}

void KartAnime::DoDriveAnime(int idx) {
    KartCtrl *kartCtrl = GetKartCtrl();
    KartBody *body = kartCtrl->getKartBody(idx);
    if (kartCtrl->WhichDriver(idx) == 1) {
        ((DriverModel *)body->mExModels[1])->setDriveAnm(body->_388);
    } else {
        ((DriverModel *)body->mExModels[0])->setDriveAnm(body->_388);
    }
    ((DriverModel *)body->mExModels[0])->setTandemAnm(body->_398, body->_39c);
    ((DriverModel *)body->mExModels[1])->setTandemAnm(body->_398, body->_39c);
}

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

void KartAnime::DoDriverDamageAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[1]->StartDriverDamage();
    mDamageFlags &= ~(u64)0x20000;
}

void KartAnime::DoDriverDriveAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[1]->StartDriverDrive();
    mDamageFlags &= ~(u64)0x40000;
}

void KartAnime::DoThrowAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartThrow(_20, _24, _1c);
    mFlags &= ~2;
}

void KartAnime::DoDriftAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (!IsDangerPlusAnime(idx) && (body->mCarStatus & 0x80) == 0 && IsChange(idx) != 1 && IsDamage(idx) != 1 &&
        IsRide(idx) != 1) {
        body->mDriverModels[0]->StartDrift(_28);
        if (_28 == 1) {
            body->_394 = -1.0f;
            body->mLiftframe = -1.0f;
            body->_39c = -1.0f;
            body->_398 = -1.0f;
        } else {
            body->_394 = 1.0f;
            body->mLiftframe = 1.0f;
            body->_39c = 1.0f;
            body->_398 = 1.0f;
        }
    }
    mFlags &= ~(u64)0x1000;
}

void KartAnime::DoDriftEndAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (!IsDangerPlusAnime(idx) && (body->mCarStatus & 0x80) == 0 && IsChange(idx) != 1 && IsDamage(idx) != 1 &&
        IsRide(idx) != 1) {
        body->mDriverModels[0]->StartDriftEnd(_28);
    }
    mFlags &= ~(u64)0x602000;
}

void KartAnime::DoSlideAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (mFlags & 0x10) {
        if (!IsSuspend(idx))
            body->mDriverModels[0]->StartAttackR();
        mFlags &= ~(u64)0x10;
    } else if (mFlags & 0x20) {
        if (!IsSuspend(idx))
            body->mDriverModels[0]->StartAttackL();
        mFlags &= ~(u64)0x20;
    }
}

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

void KartAnime::DoCrouchAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (body->mDriverModels[0]->IsSit() == 0 && !IsSuspend(idx))
        body->mDriverModels[0]->StartSit();
    mFlags &= ~(u64)0x200;
}

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

void KartAnime::DoNormalStartAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartNormalStart();
    mFlags &= ~(u64)0x4000;
}

void KartAnime::DoRocketStartAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartRocketStart();
    mFlags &= ~(u64)0x8000;
}

void KartAnime::DoFalseStartAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartRocketStartFailure();
    mFlags &= ~(u64)0x10000;
}

void KartAnime::DoDamageStartAnime(int idx) {
    KartCtrl *kartCtrl = GetKartCtrl();
    KartBody *body = kartCtrl->getKartBody(idx);
    int back = 0;
    if (IsBack(idx) == 1)
        back = 1;
    if (body->mCarStatus & 0x80) {
        if (kartCtrl->WhichDriver(idx) == 1)
            ((DriverModel *)body->mExModels[0])->StartDamage(back, 0);
        else
            ((DriverModel *)body->mExModels[1])->StartDamage(back, 0);
    } else {
        body->mDriverModels[0]->StartDamage(back, 0);
    }
    mFlags &= ~(u64)0x20000;
}

void KartAnime::DoDamageEndAnime(int idx) {
    KartCtrl *kartCtrl = GetKartCtrl();
    KartBody *body = kartCtrl->getKartBody(idx);
    int back = 0;
    if (IsBack(idx) == 1)
        back = 1;
    if (body->mCarStatus & 0x80) {
        if (kartCtrl->WhichDriver(idx) == 1)
            ((DriverModel *)body->mExModels[0])->StartDamage_GetUp(back, 0);
        else
            ((DriverModel *)body->mExModels[1])->StartDamage_GetUp(back, 0);
    } else {
        body->mDriverModels[0]->StartDamage_GetUp(back, 0);
    }
    mFlags &= ~(u64)0x40000;
}

void KartAnime::DoBigDamageStartAnime(int idx) {
    KartCtrl *kartCtrl = GetKartCtrl();
    KartBody *body = kartCtrl->getKartBody(idx);
    int back = 0;
    if (IsBack(idx) == 1)
        back = 1;
    if (body->mCarStatus & 0x80) {
        if (kartCtrl->WhichDriver(idx) == 1)
            ((DriverModel *)body->mExModels[0])->StartDamage(back, 1);
        else
            ((DriverModel *)body->mExModels[1])->StartDamage(back, 1);
    } else {
        body->mDriverModels[0]->StartDamage(back, 1);
    }
    mFlags &= ~(u64)0x80000;
}

void KartAnime::DoBigDamageEndAnime(int idx) {
    KartCtrl *kartCtrl = GetKartCtrl();
    KartBody *body = kartCtrl->getKartBody(idx);
    int back = 0;
    if (IsBack(idx) == 1)
        back = 1;
    if (body->mCarStatus & 0x80) {
        if (kartCtrl->WhichDriver(idx) == 1)
            ((DriverModel *)body->mExModels[0])->StartDamage_GetUp(back, 1);
        else
            ((DriverModel *)body->mExModels[1])->StartDamage_GetUp(back, 1);
    } else {
        body->mDriverModels[0]->StartDamage_GetUp(back, 1);
    }
    mFlags &= ~(u64)0x100000;
}

void KartAnime::DoBurnStartAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartFire();
    mFlags &= ~(u64)0x40000000;
}

void KartAnime::DoBurnEndAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartFireEnd();
    mFlags &= ~(u64)0x80000000;
}

void KartAnime::DoFloatStartAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartDash();
    mFlags &= ~(u64)0x800000;
}

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

void KartAnime::DoFloatEndAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartDashEnd();
    mFlags &= ~(u64)0x1000000;
}

void KartAnime::DoSurpriseAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (mFlags & 0x2000000) {
        body->mDriverModels[0]->StartDangerL();
        mFlags |= 0x8000000;
    } else if (mFlags & 0x4000000) {
        body->mDriverModels[0]->StartDangerR();
        mFlags |= 0x10000000;
    }
    mFlags &= ~(u64)0x6000000;
}

void KartAnime::DoSurpriseEndAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    int n = 0;
    if (IsBack(idx) == 1)
        n = 1;
    body->mDriverModels[0]->StartDangerEnd(n);
    mFlags &= ~0x6000000;
}

void KartAnime::DoHangJumpStartAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartHang2(_04, _08);
    mFlags &= ~(u64)0x2000000000;
}

void KartAnime::DoHangJumpEndAnime(int idx) {
    GetKartCtrl()->getKartBody(idx)->mDriverModels[0]->StartDashEnd2(_04, _08);
    mFlags &= ~(u64)0x4000000000;
    _08 = 1.0f;
}

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

void KartAnime::DoCheck(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    if (mFlags & 1) {
        mType = 3;
    } else if (mFlags & 0x800000000ull) {
        mType = 30;
    } else if (mFlags & 0x20000) {
        mType = 14;
    } else if (mFlags & 0x80000) {
        mType = 16;
    } else if (mFlags & 0x40000000) {
        mType = 23;
    } else if (mFlags & 0x40000) {
        mType = 15;
    } else if (mFlags & 0x100000) {
        mType = 17;
    } else if (mFlags & 0x80000000ull) {
        mType = 24;
    } else if ((body->mCarStatus & 0x100000000ull) == 0 && IsDamage(idx)) {
        mFlags &= ~0xe227a1de7eull;
        mDamageFlags &= ~2ull;
        _2c = 0;
    } else if (mFlags & 0x20000000) {
        mType = 22;
    } else if (mFlags & 0x6000000) {
        mType = 21;
    } else if ((mFlags & 4) && (mFlags & 0x100) == 0) {
        mType = 1;
    } else if ((mFlags & 8) && (mFlags & 0x100) == 0) {
        mType = 2;
    } else if (mFlags & 2) {
        mType = 4;
    } else if (mFlags & 0x30) {
        mType = 5;
    } else if (mFlags & 0x40) {
        mType = 6;
    } else if (mFlags & 0x200) {
        mType = 7;
    } else if (mFlags & 0x400) {
        mType = 8;
    } else if (mFlags & 0x800) {
        mType = 9;
    } else if (mFlags & 0x4000) {
        mType = 11;
    } else if (mFlags & 0x8000) {
        mType = 12;
    } else if (mFlags & 0x10000) {
        mType = 13;
    } else if (mFlags & 0x1000) {
        mType = 10;
    } else if (mFlags & 0x200000) {
        mType = 18;
    } else if (mFlags & 0x800000) {
        mType = 19;
    } else if (mFlags & 0x1000000) {
        mType = 20;
    } else if (mFlags & 0x100000000ull) {
        mType = 27;
    } else if (mFlags & 0x200000000ull) {
        mType = 28;
    } else if (mFlags & 0x400000000ull) {
        mType = 29;
    } else if (mFlags & 0x1000000000ull) {
        mType = 31;
    } else if (mFlags & 0x2000000000ull) {
        mType = 32;
    } else if (mFlags & 0x4000000000ull) {
        mType = 33;
    } else if (mFlags & 0x8000000000ull) {
        mType = 34;
    } else if (mFlags & 0x10000000000ull) {
        mType = 35;
    }
    if (mDamageFlags & 2) {
        _2c = 4;
    } else if (mDamageFlags & 0x20000) {
        _2c = 14;
    } else if (mDamageFlags & 0x40000) {
        _2c = 15;
    }
}

void KartAnime::DoAnime(int idx) {
    KartBody *body = GetKartCtrl()->getKartBody(idx);
    DoCheck(idx);
    DoDriveAnime(idx);
    DoGetAnime(idx);
    switch (mType) {
    case 3:
        DoChangeAnime(idx);
        break;
    case 2:
        DoBackAnime(idx);
        break;
    case 1:
        DoFrontAnime(idx);
        break;
    case 4:
        DoThrowAnime(idx);
        break;
    case 5:
        DoSlideAnime(idx);
        break;
    case 6:
        DoJumpAnime(idx);
        break;
    case 7:
        DoCrouchAnime(idx);
        break;
    case 8:
        DoStandAnime(idx);
        break;
    case 9:
        DoPushStartAnime(idx);
        break;
    case 10:
        DoDriftAnime(idx);
        break;
    case 18:
        DoDriftEndAnime(idx);
        break;
    case 11:
        DoNormalStartAnime(idx);
        break;
    case 12:
        DoRocketStartAnime(idx);
        break;
    case 13:
        DoFalseStartAnime(idx);
        break;
    case 14:
        DoDamageStartAnime(idx);
        break;
    case 15:
        DoDamageEndAnime(idx);
        break;
    case 16:
        DoBigDamageStartAnime(idx);
        break;
    case 17:
        DoBigDamageEndAnime(idx);
        break;
    case 19:
        DoFloatStartAnime(idx);
        break;
    case 20:
        DoFloatEndAnime(idx);
        break;
    case 21:
        DoSurpriseAnime(idx);
        break;
    case 22:
        DoSurpriseEndAnime(idx);
        break;
    case 23:
        DoBurnStartAnime(idx);
        break;
    case 24:
        DoBurnEndAnime(idx);
        break;
    case 27:
        DoHangStartAnime(idx);
        break;
    case 28:
        DoGoalStartAnime(idx);
        break;
    case 29:
        DoHappyAnime(idx);
        break;
    case 30:
        DoWinAnime(idx);
        break;
    case 31:
        DoDefeatAnime(idx);
        break;
    case 32:
        DoHangJumpStartAnime(idx);
        break;
    case 33:
        DoHangJumpEndAnime(idx);
        break;
    case 34:
        DoAppealAnime(idx);
        break;
    case 35:
        DoStaffAnime(idx);
        break;
    }
    switch (_2c) {
    case 4:
        DoDriverThrowAnime(idx);
        break;
    case 14:
        DoDriverDamageAnime(idx);
        break;
    case 15:
        DoDriverDriveAnime(idx);
        break;
    }
    _2c = 0;
    mType = 0;
    mFlags &= ~0x8627a1de7full;
    mDamageFlags &= ~0x60002ull;
}
