#include "Yamamoto/KartItem.h"
#include "Yamamoto/KartAnime.h"
#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCtrl.h"

#include "JSystem/JAudio/JASFakeMatch2.h"
#include "JSystem/JUtility/JUTGamePad.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/ItemWanWan.h"
#include "Yamamoto/KartGame.h"
#include "Yamamoto/KartSound.h"
#include "Yamamoto/KartStrat.h"
#include "Yamamoto/KartDamage.h"

void KartItem::Init(int idx)
{
    mBody = GetKartCtrl()->getKartBody(idx);
    _4 = 0;
    _6 = 0;
    _8 = 0;
    _9 = 0;
}

void KartItem::DoRabbit()
{
    KartBody *body = mBody;
    RaceMgr *raceMgr = RaceMgr::getCurrentManager();
    GetKartCtrl()->GetKartNumber();
    if (raceMgr->getRaceMode() == ESCAPE_BATTLE) {
        if (RCMGetKartChecker(body->mMynum)->isRabbit()) {
            if (_8 == 0) {
                GetKartCtrl()->getKartSound(body->mMynum)->DoDriverVoice(8);
                _8 = 1;
            }
        } else if (_8 != 0) {
            _8 = 0;
        }
    }
}

void KartItem::SetSucMashItem()
{
    if (RaceMgr::getCurrentManager()->getRaceMode() == TIME_ATTACK) {
        KartBody *body = mBody;
        if (_6 < 1000)
            _6++;

        if (_6 >= 120) {
            if (!GetKartCtrl()->getKartAnime(body->mMynum)->IsFloatAnime(body->mMynum)) {
                if ((body->mCarStatus & 0x02010800) || _4 == 1)
                    return;
                _4 = 1;
                ItemObjMgr *mgr = GetItemObjMgr();
                mgr->stockItemToKart(5, body->mMynum, 0, 0, 0);
                mgr->stockItemToKart(5, body->mMynum, 1, 0, 0);
            }
        }
    }
}

void KartItem::CheckHitItem()
{
    KartBody *body = mBody;
    int num = GetKartCtrl()->GetKartNumber();
    for (int i = 0; i < num; i++) {
        if (body->_5b8[i] == 1) {
            FallStealItem(GetKartCtrl()->getKartBody(i)->mMynum, body->mMynum);
        }
    }
}

void KartItem::FallStealItem(int thiefIdx, int victimIdx)
{
    KartBody *thief = GetKartCtrl()->getKartBody(thiefIdx);
    KartBody *victim = GetKartCtrl()->getKartBody(victimIdx);
    RaceMgr *raceMgr = RaceMgr::getCurrentManager();
    RCMGetKartChecker(victim->mMynum);

    if (raceMgr->getRaceMode() == BALLOON_BATTLE) {
        if (raceMgr->getManager()->robRivalOfBalloon(victimIdx, thiefIdx))
            GetKartCtrl()->getKartSound(victimIdx)->DoKartsetSeSound(0x1007e);
        thief->getCrash()->SetMatchlessTimer();
        thief->getDamage()->mFlags &= ~4;
        return;
    }

    if (raceMgr->getRaceMode() == ESCAPE_BATTLE) {
        raceMgr->getManager()->robRivalOfRabbitMark(victimIdx, thiefIdx);
        return;
    }

    ItemObjMgr *mgr = GetItemObjMgr();
    u8 victimNum = ItemObjMgr::getNowTandemDriverNum(victimIdx);
    mgr->getKartEquipItem(victimIdx, victimNum ^ 1);
    ItemObj *victimItem = mgr->getKartEquipItem(victimIdx, victimNum);

    u8 thiefNum = ItemObjMgr::getNowTandemDriverNum(thiefIdx);
    ItemObj *thiefItem1 = mgr->getKartEquipItem(thiefIdx, thiefNum ^ 1);
    ItemObj *thiefItem2 = mgr->getKartEquipItem(thiefIdx, thiefNum);

    if (victimItem == 0 && thiefItem2 != 0) {
        mgr->robRivalOfItem(victimIdx, thiefIdx, thiefNum);
        if (thief->mCarStatus & 0x4000000000ull)
            thief->mCarStatus &= ~0x4000000000ull;
    } else if (victimItem == 0 && thiefItem2 == 0 && thiefItem1 != 0) {
        mgr->robRivalOfItem(victimIdx, thiefIdx, thiefNum ^ 1);
        if (thief->mCarStatus & 0x4000000000ull)
            thief->mCarStatus &= ~0x4000000000ull;
    }

    if (thiefItem1 && victimItem) {
        if (thiefItem1->getKind() == 21) {
            thiefItem1->setStateFall(true);
        } else if (thiefItem1->IsSuccessionItem()) {
            int n = thiefItem1->getSuccessionItemNum();
            for (int i = 0; i < n; i++)
                thiefItem1->setChildStateFall();
        } else {
            thiefItem1->setStateFall(true);
        }
        if (thief->mCarStatus & 0x4000000000ull)
            thief->mCarStatus &= ~0x4000000000ull;
    }

    if (thiefItem2 && victimItem) {
        if (thiefItem2->getKind() == 21) {
            thiefItem2->setStateFall(true);
        } else if (thiefItem2->IsSuccessionItem()) {
            int n = thiefItem2->getSuccessionItemNum();
            for (int i = 0; i < n; i++)
                thiefItem2->setChildStateFall();
        } else {
            thiefItem2->setStateFall(true);
        }
        if (thief->mCarStatus & 0x4000000000ull)
            thief->mCarStatus &= ~0x4000000000ull;
    }
}

void KartItem::FallItem()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    RaceMgr *raceMgr = RaceMgr::getCurrentManager();
    ItemObjMgr *mgr = GetItemObjMgr();
    u8 num = ItemObjMgr::getNowTandemDriverNum(idx);
    ItemObj *item = mgr->getKartEquipItem(idx, num ^ 1);
    ItemObj *item2 = mgr->getKartEquipItem(idx, num);

    if (raceMgr->getRaceMode() == BOMB_BATTLE)
        return;

    body->getCrash()->DoRabbitMark();

    if (item) {
        if ((body->getDamage()->mFlags & 0x8) ||
            (body->mCarStatus & 0x80000000000ull) ||
            (body->getDossin()->_14 & 1) ||
            (body->getThunder()->mFlags & 1)) {
            if (item->getKind() == 21) {
                item->setStateFall(true);
            } else if (item->IsSuccessionItem()) {
                int n = item->getSuccessionItemNum();
                for (int i = 0; i < n; i++)
                    item->setChildStateFall();
            } else {
                item->setStateFall(true);
            }
            if (body->mCarStatus & 0x4000000000ull)
                body->mCarStatus &= ~0x4000000000ull;
        } else if (item->IsSuccessionItem() && item->getKind() != 21) {
            int n = item->getSuccessionItemNum() - 1;
            for (int i = 0; i < n; i++)
                item->setChildStateFall();
        }
    }

    if (item2 == 0)
        return;

    if (body->mCarStatus & 0x1000) {
        if (item2->getKind() != 7)
            return;
        ((ItemWanWanObj *)ItemObjMgr::getItemWanWanObj(item2))->separate();
        GetKartCtrl()->getKartSound(idx)->DoWanWanStopSound();
        body->mCarStatus &= ~0x1000;
        return;
    }

    if ((body->getDamage()->mFlags & 0x8) ||
        (body->mCarStatus & 0x80000000000ull) ||
        (body->getDossin()->_14 & 1) ||
        (body->getThunder()->mFlags & 1)) {
        if (item2->getKind() == 21) {
            item2->setStateFall(true);
        } else if (item2->IsSuccessionItem()) {
            int n = item2->getSuccessionItemNum();
            for (int i = 0; i < n; i++)
                item2->setChildStateFall();
        } else {
            item2->setStateFall(true);
        }
        if (body->mCarStatus & 0x4000000000ull)
            body->mCarStatus &= ~0x4000000000ull;
    } else if (item2->IsSuccessionItem() && item2->getKind() != 21) {
        int n = item2->getSuccessionItemNum() - 1;
        for (int i = 0; i < n; i++)
            item2->setChildStateFall();
    }
}

void KartItem::FallAllItem()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    RaceMgr *raceMgr = RaceMgr::getCurrentManager();
    ItemObj *item;
    ItemObjMgr *mgr = GetItemObjMgr();
    int num = ItemObjMgr::getNowTandemDriverNum(idx);
    item = mgr->getKartEquipItem(idx, num ^ 1);
    mgr->deleteHeartItem(idx);

    if (raceMgr->getRaceMode() == BOMB_BATTLE && !(body->mCarStatus & 0x400000))
        goto end;

    if (item) {
        if (item->getKind() == 21) {
            item->setStateFall(true);
        } else if (item->IsSuccessionItem()) {
            int n = item->getSuccessionItemNum();
            for (int i = 0; i < n; i++)
                item->setChildStateFall();
        } else {
            item->setStateFall(true);
        }
    }

    item = mgr->getKartEquipItem(idx, num);
    if (item) {
        if (item->getKind() == 21) {
            item->setStateFall(true);
        } else if (item->IsSuccessionItem()) {
            int n = item->getSuccessionItemNum();
            for (int i = 0; i < n; i++)
                item->setChildStateFall();
        } else {
            item->setStateFall(true);
        }
    }

    if (body->mCarStatus & 0x4000000000ull)
        body->mCarStatus &= ~0x4000000000ull;
end:
    ;
}

void KartItem::ReleseWanWan()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    ItemObjMgr *mgr = GetItemObjMgr();
    u8 num = ItemObjMgr::getNowTandemDriverNum(idx);
    ItemObj *item = mgr->getKartEquipItem(idx, num);
    if (item == 0)
        return;
    if (body->mCarStatus & 0x1000) {
        ItemWanWanObj *wanwan = (ItemWanWanObj *)ItemObjMgr::getItemWanWanObj(item);
        if (item->getKind() == 7) {
            wanwan->separate();
            body->mCarStatus &= ~0x1000;
            GetKartCtrl()->getKartAnime(idx)->mFlags |= 0x1000000;
            GetKartCtrl()->getKartSound(idx)->DoWanWanStopSound();
        }
    }
}

bool KartItem::CompulsionReleseWanWan()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    ItemObjMgr *mgr = GetItemObjMgr();
    u8 num = ItemObjMgr::getNowTandemDriverNum(idx);
    ItemObj *item = mgr->getKartEquipItem(idx, num);
    if (item == 0)
        return false;
    if (body->mCarStatus & 0x1000) {
        ItemWanWanObj *wanwan = (ItemWanWanObj *)ItemObjMgr::getItemWanWanObj(item);
        if (item->getKind() == 7) {
            if (wanwan->tstReverseThrow())
                return true;
        }
    }
    return false;
}

__declspec(weak) u8 ItemWanWanObj::tstReverseThrow() const
{
    return _34b;
}

void KartItem::KillWanWan()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    if (body->mCarStatus & 0x1000) {
        body->mCarStatus &= ~0x1000;
        GetKartCtrl()->getKartAnime(idx)->mFlags |= 0x1000000;
        GetKartCtrl()->getKartSound(idx)->DoWanWanStopSound();
    }
}

bool KartItem::IsMyItem()
{
    u8 idx = mBody->mMynum;
    ItemObjMgr *mgr = GetItemObjMgr();
    ItemObj *item = mgr->getKartEquipItem(idx);
    if (item == 0)
        return false;
    return ((ItemWanWanObj *)ItemObjMgr::getItemWanWanObj(item))->IsEndPull() != 1;
}

__declspec(weak) u8 ItemWanWanObj::IsEndPull()
{
    return _2e0;
}
bool KartItem::IsChanging()
{
    KartBody *body = mBody;
    if (GetKartCtrl()->getKartAnime(body->mMynum)->IsChangeStart() == 1 ||
        (body->mCarStatus & 0x80)) {
        body->_57c &= ~9;
        return true;
    }
    return false;
}

void KartItem::DoTurtleReleaseDir(int back, int suc)
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    if (back == 0) {
        if (GetKartCtrl()->getKartPad(idx)->mItemStickY < -0.1f) {
            GetKartCtrl()->getKartAnime(idx)->MakeThrowAnime(0, 1, suc);
            body->_57c |= 4;
        } else {
            GetKartCtrl()->getKartAnime(idx)->MakeThrowAnime(0, 0, suc);
            body->_57c |= 2;
        }
    } else {
        if (GetKartCtrl()->getKartPad(idx)->mItemStickY > 0.1f) {
            GetKartCtrl()->getKartAnime(idx)->MakeThrowAnime(1, 0, suc);
            body->_57c |= 2;
        } else {
            GetKartCtrl()->getKartAnime(idx)->MakeThrowAnime(1, 1, suc);
            body->_57c |= 4;
        }
    }
}

void KartItem::DoBananaReleaseDir()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    if (GetKartCtrl()->getKartPad(idx)->mItemStickY > 0.1f) {
        GetKartCtrl()->getKartAnime(idx)->MakeThrowAnime(1, 0, 0);
        body->_57c |= 2;
    } else {
        GetKartCtrl()->getKartAnime(idx)->MakeThrowAnime(1, 1, 0);
        body->_57c |= 4;
    }
}

void KartItem::DoTurtleReleaseDynamicDir(u32 kind)
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    ItemObj *item = GetItemObjMgr()->getKartEquipItem(idx);
    if (kind == 21)
        GetKartCtrl()->getKartSound(idx)->DoFireBall();

    if (item->IsSuccessionItem()) {
        if (body->_57c & 4) {
            item->setChildStateSpRelease();
            GetKartCtrl()->getKartSound(idx)->DoPutVoice();
        } else {
            item->setChildStateRelease();
            GetKartCtrl()->getKartSound(idx)->DoThrowVoice();
        }
    } else {
        if (body->_57c & 4) {
            item->setStateSpRelease(true);
            GetKartCtrl()->getKartSound(idx)->DoPutVoice();
        } else {
            item->setStateRelease(true);
            GetKartCtrl()->getKartSound(idx)->DoThrowVoice();
        }
    }
}

void KartItem::DoBananaReleaseDynamicDir()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    ItemObj *item = GetItemObjMgr()->getKartEquipItem(idx);
    if (item->getKind() == 14) {
        item->setStateRelease(true);
        GetKartCtrl()->getKartSound(idx)->DoTandemVoice(0);
    } else if (item->IsSuccessionItem()) {
        if (body->_57c & 2) {
            item->setChildStateSpRelease();
            GetKartCtrl()->getKartSound(idx)->DoThrowVoice();
        } else {
            item->setChildStateRelease();
            GetKartCtrl()->getKartSound(idx)->DoPutVoice();
        }
    } else {
        if (body->_57c & 2) {
            item->setStateSpRelease(true);
            GetKartCtrl()->getKartSound(idx)->DoThrowVoice();
        } else {
            item->setStateRelease(true);
            GetKartCtrl()->getKartSound(idx)->DoPutVoice();
        }
    }
}

void KartItem::TeamRobItm()
{
    KartBody *body = mBody;
    u8 idx = body->mMynum;
    ItemObjMgr *mgr = GetItemObjMgr();
    KartGamePad *pad = GetKartCtrl()->GetDriveCont(idx);
    u32 btn = GetKartCtrl()->getKartPad(idx)->mItemMask;

    if (!(body->mGameStatus & 1))
        return;
    if (!pad->testTrigger(btn))
        return;

    if (ItemObjMgr::getNowTandemDriverNum(idx) == 0) {
        if (mgr->getKartEquipItem(idx, 1) == 0)
            return;
        if (mgr->getKartEquipItem(idx, 0) != 0)
            return;
        GetKartCtrl()->getKartSound(idx)->DoDriverVoice(0x22);
        mgr->transferPartnerOfItem(idx, 1);
    } else {
        if (mgr->getKartEquipItem(idx, 0) == 0)
            return;
        if (mgr->getKartEquipItem(idx, 1) != 0)
            return;
        GetKartCtrl()->getKartSound(idx)->DoDriverVoice(0x22);
        mgr->transferPartnerOfItem(idx, 1);
    }
}

bool KartItem::SuspendItem()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    GetItemObjMgr()->getKartEquipItem(idx);
    if (body->getChecker()->CheckCrash()) {
        GetKartCtrl()->getKartAnime(idx)->mFlags &= ~8ull;
        body->_57c &= ~9;
        return true;
    }
    return false;
}

void KartItem::CheckOtherAnime()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    ItemObj *item = GetItemObjMgr()->getKartEquipItem(idx);
    GetKartCtrl()->getKartAnime(idx)->_0 = item;
    if (body->getChecker()->CheckCrash()) {
        if (IsChanging() == 1)
            return;
        GetKartCtrl()->GetCoDriveCont(idx);
        GetKartCtrl()->GetItemButton(idx);
        SuspendItem();
    }
}

void KartItem::DoOtherAnime()
{
    u8 geoIdx = mBody->mMynum;
    u8 idx = geoIdx;
    KartBody *body = mBody;
    GetKartCtrl()->GetCoDriveCont(idx);
    GetKartCtrl()->GetItemButton(idx);
    ItemObjMgr *mgr = GetItemObjMgr();
    ItemObj *item = mgr->getKartEquipItem(idx);

    if (mgr->tstHaveItemHeart(idx))
        GetKartCtrl()->getKartSound(idx)->DoKartsetSeSound(0x1009b);

    if (GetGeoObjMgr()->tstKartHitItemBoxTrigger(geoIdx))
        body->getStrat()->DoMotor(MotorManager::MotorType_0);

    if (IsChanging() == true)
        return;
    if (SuspendItem() == true)
        return;

    if (item == 0) {
        GetKartCtrl()->getKartPad(idx)->_51 = 0;
        if (GetKartCtrl()->getKartAnime(idx)->IsFloatAnime(idx))
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsDamage(idx))
            return;
        if (body->getDamage()->mFlags & 0x48)
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsThrow(idx))
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsBackAnime(idx)) {
            GetKartCtrl()->getKartAnime(idx)->mFlags &= ~4ull;
            GetKartCtrl()->getKartAnime(idx)->MakeFrontAnime(idx);
        }
        GetKartCtrl()->getKartAnime(idx)->mFlags &= ~8ull;
        goto end;
    }

    if (item->getState() == 7) {
        GetKartCtrl()->getKartPad(idx)->_51 = 0;
        if (GetKartCtrl()->getKartAnime(idx)->IsFloatAnime(idx))
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsDamage(idx))
            return;
        if (body->getDamage()->mFlags & 0x48)
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsThrow(idx))
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsBackAnime(idx)) {
            GetKartCtrl()->getKartAnime(idx)->mFlags &= ~4ull;
            GetKartCtrl()->getKartAnime(idx)->MakeFrontAnime(idx);
        }
        GetKartCtrl()->getKartAnime(idx)->mFlags &= ~8ull;
        goto end;
    }

    switch (item->getKind()) {
    case 3:
    case 4:
    case 5:
    case 6:
    case 10:
    case 12:
    case 14:
    case 15:
    case 18:
        if (item->getState() == 9 || item->getState() == 5) {
            GetKartCtrl()->getKartPad(idx)->_51 = 0;
            body->_57c &= ~0xfu;
            goto end;
        }
        if (body->getDamage()->mFlags & 0x40)
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsSit(idx))
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsFloatAnime(idx))
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsMove(idx))
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsDamage(idx))
            return;
        if (!GetKartCtrl()->getKartAnime(idx)->IsBackAnime(idx)) {
            GetKartCtrl()->getKartAnime(idx)->mFlags &= ~8ull;
            GetKartCtrl()->getKartAnime(idx)->MakeBackAnime(idx);
        }
        GetKartCtrl()->getKartAnime(idx)->mFlags &= ~4ull;
        break;
    case 7:
    case 8:
    case 9:
    case 11:
    case 13:
    case 16:
    case 17:
        if (item->getState() == 9 || item->getState() == 5) {
            GetKartCtrl()->getKartPad(idx)->_51 = 0;
            if (!GetKartCtrl()->getKartAnime(idx)->IsDamage(idx) &&
                !(body->getDamage()->mFlags & 0x48) &&
                !GetKartCtrl()->getKartAnime(idx)->IsThrow(idx)) {
                if (GetKartCtrl()->getKartAnime(idx)->IsBackAnime(idx)) {
                    GetKartCtrl()->getKartAnime(idx)->mFlags &= ~4ull;
                    GetKartCtrl()->getKartAnime(idx)->MakeFrontAnime(idx);
                }
                GetKartCtrl()->getKartAnime(idx)->mFlags &= ~8ull;
            }
            body->_57c &= ~0xfu;
        }
        break;
    default:
        break;
    }

end:
    if (GetKartCtrl()->getKartPad(idx)->_51)
        GetKartCtrl()->getKartPad(idx)->_51--;
    if (GetKartCtrl()->getKartPad(idx)->_52)
        GetKartCtrl()->getKartPad(idx)->_52--;

    if (GetKartCtrl()->getKartPad(idx)->_4C) {
        if (GetKartCtrl()->getKartPad(idx)->_50 & 1)
            GetKartCtrl()->getKartPad(idx)->_4C--;
    } else if (GetKartCtrl()->getKartPad(idx)->_50 & 1) {
        GetKartCtrl()->getKartPad(idx)->_50 &= ~1u;
        body->_57c &= ~9;
        GetKartCtrl()->getKartPad(idx)->_54->setStateDisappear(true);
    }
}

void KartItem::DoTandemItemAnime()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    KartGamePad *pad = GetKartCtrl()->GetCoDriveCont(idx);
    u32 btn = GetKartCtrl()->GetItemButton(idx);
    ItemObj *item = GetItemObjMgr()->getKartEquipItem(idx);
    if (IsChanging())
        return;
    if (SuspendItem())
        return;
    if (GetKartCtrl()->getKartAnime(idx)->IsGetItemAnime(idx))
        return;

    int back = 0;
    if (GetKartCtrl()->getKartAnime(idx)->IsBack(idx))
        back = 1;

    if (item == 0)
        return;
    if (body->_57c & 1)
        return;
    if (item->getState() == 9 || item->getState() == 5)
        return;

    switch (item->getKind()) {
    case 0:
    case 1:
    case 2:
    case 8:
    case 9:
    case 11:
    case 13:
    case 17:
    case 19:
    case 20:
    case 21: {
        if (pad->testTrigger(btn))
            body->_57c |= 8;
        if (!(body->_57c & 8))
            return;
        if (GetKartCtrl()->getKartAnime(idx)->IsBackStart(idx) == 1)
            return;
        body->_57c |= 1;
        body->_57c &= ~8;
        int suc = 0;
        if (item->IsSuccessionItem())
            suc = 1;
        GetKartCtrl()->getKartPad(idx)->_51 = 5;
        GetKartCtrl()->getKartPad(idx)->mItemStickY = pad->getMainStickY();
        GetKartCtrl()->getKartPad(idx)->_44 = pad->getMainStickX();
        DoTurtleReleaseDir(back, suc);
        break;
    }
    case 3:
    case 4:
    case 14:
    case 15:
        if (GetKartCtrl()->getKartAnime(idx)->IsBackStart(idx) == 1)
            return;
        if (pad->testTrigger(btn))
            body->_57c |= 8;
        if (!(body->_57c & 8))
            return;
        body->_57c |= 1;
        body->_57c &= ~8;
        GetKartCtrl()->getKartPad(idx)->_51 = 5;
        GetKartCtrl()->getKartPad(idx)->mItemStickY = pad->getMainStickY();
        GetKartCtrl()->getKartPad(idx)->_44 = pad->getMainStickX();
        DoBananaReleaseDir();
        break;
    case 5:
    case 18:
        if (!pad->testTrigger(btn))
            return;
        body->_57c |= 1;
        body->_57c &= ~8;
        GetKartCtrl()->getKartPad(idx)->_51 = 5;
        GetKartCtrl()->getKartAnime(idx)->MakeThrowAnime(back, 1, 0);
        break;
    case 12:
        if (!pad->testTrigger(btn))
            return;
        if (!(GetKartCtrl()->getKartPad(idx)->_50 & 1)) {
            GetKartCtrl()->getKartPad(idx)->_50 |= 1;
            GetKartCtrl()->getKartPad(idx)->_4C = 300;
            GetKartCtrl()->getKartPad(idx)->_54 = item;
        }
        body->_57c |= 1;
        body->_57c &= ~8;
        GetKartCtrl()->getKartPad(idx)->_51 = 3;
        GetKartCtrl()->getKartAnime(idx)->mFlags |= 0x200;
        break;
    case 6:
    case 10:
        if (!pad->testTrigger(btn))
            return;
        body->_57c |= 1;
        body->_57c &= ~8;
        GetKartCtrl()->getKartAnime(idx)->MakeThrowAnime(back, 1, 0);
        GetKartCtrl()->getKartPad(idx)->_51 = 5;
        break;
    case 7:
        if (!pad->testTrigger(btn))
            return;
        GetKartCtrl()->getKartPad(idx)->_51 = 5;
        GetKartCtrl()->getKartAnime(idx)->MakeThrowAnime(back, 0, 0);
        body->_57c |= 1;
        body->_57c &= ~8;
        break;
    case 16:
        break;
    default:
        break;
    }
}

void KartItem::DoTandemItemRelease()
{
    u8 idx = mBody->mMynum;
    KartBody *body = mBody;
    GetKartCtrl()->GetCoDriveCont(idx);
    ItemObj *item = GetItemObjMgr()->getKartEquipItem(idx);
    if (item == 0)
        return;
    if (item->getState() == 9 || item->getState() == 5)
        return;
    if (!(body->_57c & 1))
        return;
    if (GetKartCtrl()->getKartPad(idx)->_51 != 0)
        return;

    body->_57c &= ~1u;
    GetKartCtrl()->getKartPad(idx)->_51 = 0;

    switch (item->getKind()) {
    case 0:
    case 1:
    case 2:
    case 8:
    case 9:
    case 11:
    case 13:
    case 17:
    case 19:
    case 20:
    case 21:
        DoTurtleReleaseDynamicDir(item->getKind());
        break;
    case 3:
    case 4:
    case 14:
    case 15:
        DoBananaReleaseDynamicDir();
        break;
    case 5:
    case 18:
        body->getGame()->MakeMashDash();
        if (item->IsSuccessionItem())
            item->setChildStateRelease();
        else
            item->setStateRelease(true);
        break;
    case 12:
        body->getGame()->MakeGoldenMashDash();
        item->setStateRelease(true);
        break;
    case 6:
        GetKartCtrl()->getKartSound(idx)->DoTandemVoice(8);
        body->getStar()->MakeStar();
        item->setStateRelease(true);
        break;
    case 10:
        GetKartCtrl()->getKartSound(idx)->DoTandemVoice(14);
        body->getThunder()->MakeThunder();
        item->setStateRelease(true);
        break;
    case 7:
        item->setStateRelease(true);
        GetKartCtrl()->getKartAnime(idx)->mFlags |= 0x800000;
        body->mCarStatus |= 0x1000;
        GetKartCtrl()->getKartSound(idx)->DoWanwanVoice();
        GetKartCtrl()->getKartSound(idx)->DoWanWanStartSound();
        body->getGame()->_09 = 0;
        break;
    case 16:
        break;
    default:
        break;
    }
    body->_57c &= ~7;
}

void KartItem::DoItem()
{
    if (mBody->getChecker()->CheckCrash())
        return;
    DoOtherAnime();
    TeamRobItm();
    DoTandemItemAnime();
    DoTandemItemRelease();
}

