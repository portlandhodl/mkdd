#include <stdlib.h>

#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/Course/CrsData.h"

#include "Osako/kartPad.h"

#include "Sato/ItemObjMgr.h"
#include "Sato/RivalBodyCtrl.h"
#include "Sato/RivalItemCtrl.h"
#include "Sato/RivalKart.h"
#include "Sato/stMath.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCtrl.h"

extern "C" int __ptmf_test(void *);

RivalItemCtrl::RivalItemCtrl(RivalKart *kart) : RivalCtrlBase(kart), JKRDisposer() {}

RivalItemCtrl::~RivalItemCtrl() {}

void RivalItemCtrl::init()
{
    reset();
}

void RivalItemCtrl::reset()
{
    mStateFunc = 0;
    mUseCnt = 0;
    mFlags = 0;
    mUseWait = 0;
    mItem = nullptr;
    mSuc = false;
}

void RivalItemCtrl::doItemCtrl()
{
    if (getBody()->mGameStatus & 0x20)
        return;

    if (getBodyCtrl()->mFlags & 0x80)
        return;

    ItemObj *item = GetItemObjMgr()->getKartEquipItem(getMyID(), GetItemObjMgr()->getNowTandemDriverNum(getMyID()));

    if (item)
    {
        if (mItem != item && !(mFlags & 2))
        {
            startItem(item);
            switch (item->getKind())
            {
            case 0:
            case 1:
            case 9:
            case 21:
                initItem_GTurtle();
                break;
            case 17:
                initItem_GTurtleSuc();
                break;
            case 2:
                initItem_RTurtle();
                break;
            case 19:
                initItem_GTurtleSuc();
                break;
            case 8:
                initItem_Bomb();
                break;
            case 3:
            case 4:
            case 15:
                initItem_Banana();
                break;
            case 5:
                initItem_Kinoko();
                break;
            case 18:
                initItem_KinokoSuc();
                break;
            case 6:
                initItem_Star();
                break;
            case 10:
            case 13:
            case 14:
                initItem_Thunder();
                break;
            case 7:
                initItem_WanWan();
                break;
            case 11:
                initItem_YoshiEgg();
                break;
            case 12:
                initItem_GoldenKinoko();
                break;
            default:
                initItem_GTurtle();
                break;
            }
        }

        if (mFlags & 2)
        {
            if (__ptmf_test(&mStateFunc) != 0)
            {
                KartCtrl *kartCtrl = KartCtrl::getKartCtrl();
                if (!kartCtrl->CheckChange(getMyID()))
                {
                    mUseCnt++;
                    (this->*mStateFunc)();
                }
            }
        }
    }
    else
    {
        mItem = nullptr;
        if (mFlags & 2)
            execute_endItemParam();
    }
}

void RivalItemCtrl::startItem(ItemObj *item)
{
    bool throwFlg = 1;
    mUseCnt = 0;
    mUseWait = 0;
    mSuc = false;
    mItem = item;
    mFlags |= 2;
    mFlags &= ~4;

    switch (item->getKind())
    {
    case 3:
    case 4:
        throwFlg = 0;
        break;
    }
    setThrowFlg(throwFlg);
}

void RivalItemCtrl::resetSucItem(ItemObj *item)
{
    mSuc = true;
    mItem = item;
    mUseCnt = 0;
    mUseWait = 0;
}

void RivalItemCtrl::endItem()
{
    execute_endItemParam();
    mUseCnt = 0;
    mUseWait = 0;
}

void RivalItemCtrl::execute_endItemParam()
{
    set_endItemParam();
    mUseCnt = 0;
    mUseWait = 0;
}

void RivalItemCtrl::setRndCount(u32 cnt, u32 wait)
{
    mUseCnt = cnt;
    mUseWait = wait;
}

void RivalItemCtrl::set_endItemParam()
{
    mFlags &= ~2;
    mStateFunc = 0;
}

void RivalItemCtrl::setRndFlgItemFront(float stick)
{
    f32 rnd = getRandom()->get_ufloat_1();
    triggerButtonStickFront(rnd * 0.5f + 0.5f);
}

void RivalItemCtrl::setThrowFlg(bool flag)
{
    bool isEasy = false;
    if (RCMGetManager()->getRaceLevel() == 0)
        isEasy = true;

    if (flag)
    {
        if (!isEasy)
        {
            if (getBodyCtrl()->getMyRank() == 1 || getBodyCtrl()->isOneUpperLowerPlayer(true))
            {
                mFlags &= ~4;
                return;
            }
        }

        f32 rnd = getRandom()->get_ufloat_1();
        if (rnd >= 0.2f)
            mFlags |= 4;
        else
            mFlags &= ~4;
    }
    else
    {
        if (!isEasy)
        {
            if (getBodyCtrl()->isOneUpperLowerPlayer(false))
            {
                mFlags |= 4;
                return;
            }
        }
        mFlags &= ~4;
    }
}

bool RivalItemCtrl::chkEnableThrowBack()
{
    int ret = false;
    if (getBodyCtrl()->isNeighborhoodCenter(0.5f) || mUseCnt > 360)
        ret = true;
    return ret;
}

void RivalItemCtrl::initItem_GTurtle()
{
    mUseWait = getRandom()->getRandomMax(90) + 30;
    mStateFunc = &RivalItemCtrl::useItem_GTurtle_Normal;
}

void RivalItemCtrl::useItem_GTurtle_Normal()
{
    if (mUseCnt <= mUseWait)
        return;

    if (mFlags & 4)
    {
        triggerButton();
    }
    else
    {
        if (chkEnableThrowBack())
            triggerButtonStickBack(1.0f);
    }
}

void RivalItemCtrl::initItem_GTurtleSuc()
{
    f32 rnd = getRandom()->get_ufloat_1();
    if (rnd > 0.7f)
        mSuc = true;

    mUseWait = getRandom()->getRandomMax(90) + 30;
    mStateFunc = &RivalItemCtrl::useItem_GTurtleSuc_Normal;
}

void RivalItemCtrl::useItem_GTurtleSuc_Normal()
{
    u8 used = 0;
    if (mUseCnt <= mUseWait)
        return;

    if (mFlags & 4)
    {
        triggerButton();
        used = 1;
    }
    else
    {
        if (chkEnableThrowBack() || mSuc)
        {
            triggerButtonStickBack(1.0f);
            used = 1;
        }
    }

    if (!used)
        return;

    ItemObj *item = mItem;
    bool throwFlg = true;
    mUseCnt = 0;
    mUseWait = 0;
    mFlags &= ~4;

    int kind = item->getKind();
    switch (kind)
    {
    case 3:
    case 4:
    case 15:
        throwFlg = false;
        break;
    }

    bool isEasy = false;
    if (RCMGetManager()->getRaceLevel() == 0)
        isEasy = true;
    if (throwFlg)
    {
        if (!isEasy)
        {
            if (getBodyCtrl()->getMyRank() == 1 || getBodyCtrl()->isOneUpperLowerPlayer(true))
            {
                mFlags &= ~4;
                goto setState;
            }
        }

        {
            f32 rnd = getRandom()->get_ufloat_1();
            if (rnd >= 0.2f)
                mFlags |= 4;
            else
                mFlags &= ~4;
        }
    }
    else
    {
        if (!isEasy)
        {
            if (getBodyCtrl()->isOneUpperLowerPlayer(false))
            {
                mFlags |= 4;
                goto setState;
            }
        }
        mFlags &= ~4;
    }

setState:
    mStateFunc = &RivalItemCtrl::useItem_GTurtleSuc_Normal;
    if (!mSuc)
        mUseWait = getRandom()->getRandomMax(60) + 60;
}

void RivalItemCtrl::initItem_RTurtleSuc()
{
    f32 rnd = getRandom()->get_ufloat_1();
    if (rnd > 0.7f)
        mSuc = true;

    mUseWait = getRandom()->getRandomMax(30) + 30;
    mStateFunc = &RivalItemCtrl::useItem_RTurtleSuc_Normal;
}

void RivalItemCtrl::useItem_RTurtleSuc_Normal()
{
    u8 used = 0;
    if (mUseCnt <= mUseWait)
        return;

    if (mFlags & 4)
    {
        triggerButton();
        used = 1;
    }
    else if (chkEnableThrowBack())
    {
        triggerButtonStickBack(1.0f);
        used = 1;
    }

    if (!used)
        return;

    mUseCnt = 0;
    mUseWait = 0;
    mFlags &= ~4;
}

void RivalItemCtrl::doSpReleaseGTurtle()
{
    triggerButtonStickBack(1.0f);
}

void RivalItemCtrl::initItem_RTurtle()
{
    mUseWait = getRandom()->getRandomMax(30) + 30;
    mStateFunc = &RivalItemCtrl::useItem_RTurtle_Normal;
}

void RivalItemCtrl::useItem_RTurtle_Normal()
{
    if (mUseCnt <= mUseWait)
        return;

    if (mFlags & 4)
    {
        triggerButton();
    }
    else
    {
        if (chkEnableThrowBack())
            triggerButtonStickBack(1.0f);
    }
}

void RivalItemCtrl::initItem_Bomb()
{
    mStateFunc = &RivalItemCtrl::useItem_Bomb_Normal;
    mUseWait = getRandom()->getRandomMax(90) + 30;
}

void RivalItemCtrl::releaseBombFront(const float &f1, const float &f2)
{
    f32 rnd = getRandom()->get_ufloat_1();
    triggerButtonStickFront(rnd * f1 + f2);
}

void RivalItemCtrl::initItem_YoshiEgg()
{
    mStateFunc = &RivalItemCtrl::useItem_YoshiEgg_Normal;
    mUseWait = getRandom()->getRandomMax(30) + 30;
}

void RivalItemCtrl::useItem_YoshiEgg_Normal()
{
    if (mUseCnt <= mUseWait)
        return;

    if (mFlags & 4)
    {
        f32 rnd = getRandom()->get_ufloat_1();
        triggerButtonStickFront(rnd * 0.8f + 0.2f);
    }
    else
    {
        if (chkEnableThrowBack())
            triggerButtonStickBack(1.0f);
    }
}

void RivalItemCtrl::useItem_Bomb_Normal()
{
    if (mUseCnt <= mUseWait)
        return;

    if (mFlags & 4)
    {
        f32 rnd = getRandom()->get_ufloat_1();
        triggerButtonStickFront(rnd * 0.8f + 0.2f);
    }
    else
    {
        if (chkEnableThrowBack())
            triggerButtonStickBack(1.0f);
    }
}

void RivalItemCtrl::initItem_Banana()
{
    mUseWait = getRandom()->getRandomMax(240);
    mStateFunc = &RivalItemCtrl::useItem_Banana_Normal;
}

void RivalItemCtrl::useItem_Banana_Normal()
{
    if (mUseCnt <= mUseWait)
        return;

    if (mFlags & 4)
    {
        f32 rnd = getRandom()->get_ufloat_1();
        triggerButtonStickFront(rnd * 0.5f + 0.5f);
    }
    else
    {
        if (chkEnableThrowBack())
            triggerButtonStickBack(1.0f);
    }
}

void RivalItemCtrl::initItem_Kinoko()
{
    mStateFunc = &RivalItemCtrl::useItem_Kinoko_Normal;
    mUseWait = getRandom()->getRandomMax(180) + 60;
}

void RivalItemCtrl::doUseKinoko()
{
    RivalBodyCtrl *bodyCtrl = getBodyCtrl();
    const CrsData::SCLPoint *pointData = bodyCtrl->mPointCtrl->getNext()->getPointData();
    int swerve = (s8)pointData->swerve;
    if (bodyCtrl->_84 == 2)
        return;
    if ((s8)pointData->isNoMushroomZone)
        return;

    bool use = false;
    if (swerve == 0)
    {
        use = true;
    }
    else if (__abs(swerve) < 2)
    {
        if (getBodyCtrl()->_84 != 2)
        {
            if ((bodyCtrl->getBody()->mCarStatus & 0x2003c000) == 0)
            {
                f32 f;
                bodyCtrl->getTargetCurveLR(&f, 2);
                f = 180.0f - f;
                if (f < 30.0f)
                    use = true;
            }
        }
    }

    if (use)
        triggerButton();
}

void RivalItemCtrl::useItem_Kinoko_Normal()
{
    bool use = false;
    if (mUseCnt <= mUseWait)
        return;

    RivalBodyCtrl *bodyCtrl = getBodyCtrl();
    const CrsData::SCLPoint *pointData = bodyCtrl->mPointCtrl->getNext()->getPointData();
    int swerve = (s8)pointData->swerve;
    if (bodyCtrl->_84 == 2)
        return;
    if ((s8)pointData->isNoMushroomZone)
        return;

    if (swerve == 0)
    {
        use = true;
    }
    else
    {
        if (__abs(swerve) < 2)
        {
            if (getBodyCtrl()->_84 != 2)
        {
            if ((bodyCtrl->getBody()->mCarStatus & 0x2002c000) == 0)
            {
                f32 f;
                bodyCtrl->getTargetCurveLR(&f, 2);
                f = 180.0f - f;
                if (f < 30.0f)
                    use = true;
            }
        }
        }
    }

    if (use)
        triggerButton();
}

void RivalItemCtrl::initItem_KinokoSuc()
{
    mUseWait = getRandom()->getRandomMax(180) + 60;

    f32 rnd = getRandom()->get_ufloat_1();
    if (rnd > 0.7f)
        mSuc = true;

    mStateFunc = &RivalItemCtrl::useItem_KinokoSuc_Normal;
}

void RivalItemCtrl::useItem_KinokoSuc_Normal()
{
    if (mUseCnt <= mUseWait)
        return;

    if (getBodyCtrl()->_84 == 2)
        return;
    if ((getBodyCtrl()->getBody()->mCarStatus & 0x2002c000) != 0)
        return;

    f32 f;
    getBodyCtrl()->getTargetCurveLR(&f, 2);
    f = 180.0f - f;
    if (f < 30.0f)
        triggerButton();
}

void RivalItemCtrl::initItem_GoldenKinoko()
{
    mStateFunc = &RivalItemCtrl::useItem_GoldenKinoko;
    mUseWait = getRandom()->getRandomMax(60) + 60;
}

void RivalItemCtrl::useItem_GoldenKinoko()
{
    u8 use = 0;
    if (mUseCnt <= mUseWait)
        return;

    RivalBodyCtrl *bodyCtrl = getBodyCtrl();
    const CrsData::SCLPoint *pointData = bodyCtrl->mPointCtrl->getNext()->getPointData();
    int swerve = (s8)pointData->swerve;
    u8 b = 0;

    if (bodyCtrl->_84 == 2)
    {
        b = 0;
    }
    else if ((s8)pointData->isNoMushroomZone)
    {
        b = 0;
    }
    else if (swerve == 0)
    {
        b = 1;
    }
    else
    {
        if (__abs(swerve) < 2)
        {
            if (getBodyCtrl()->_84 != 2)
            {
                if ((bodyCtrl->getBody()->mCarStatus & 0x2002c000) == 0)
                {
                    f32 f;
                    bodyCtrl->getTargetCurveLR(&f, 2);
                    f = 180.0f - f;
                    if (f < 30.0f)
                        b = 1;
                }
            }
        }
    }

    if (b)
        triggerButton();

    use = b;
    if (use)
        mUseWait = getRandom()->getRandomMax(40) + 30;
}

void RivalItemCtrl::initItem_Star()
{
    mStateFunc = &RivalItemCtrl::useItem_Star_Normal;
    mUseWait = getRandom()->getRandomMax(180) + 60;
}

void RivalItemCtrl::useItem_Star_Normal()
{
    if (mUseCnt <= mUseWait)
        return;

    if (getBodyCtrl()->_84 == 2)
        return;
    if ((getBodyCtrl()->getBody()->mCarStatus & 0x2002c000) != 0)
        return;

    f32 f;
    getBodyCtrl()->getTargetCurveLR(&f, 2);
    f = 180.0f - f;
    if (f < 30.0f)
        triggerButton();
}

void RivalItemCtrl::initItem_Thunder()
{
    mStateFunc = &RivalItemCtrl::useItem_Thunder_Normal;
    mUseWait = getRandom()->getRandomMax(30) + 30;
}

void RivalItemCtrl::useItem_Thunder_Normal()
{
    if (mUseCnt <= mUseWait)
        return;

    triggerButton();
}

void RivalItemCtrl::initItem_WanWan()
{
    mStateFunc = &RivalItemCtrl::useItem_WanWan_Normal;
    mUseWait = getRandom()->getRandomMax(180) + 60;
}

void RivalItemCtrl::useItem_WanWan_Normal()
{
    if (mUseCnt <= mUseWait)
        return;

    triggerButton();
}

void RivalItemCtrl::pressButton()
{
    KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(getMyID());
    pad->mButtons.mButton |= KartCtrl::getKartCtrl()->getKartPad(getMyID())->mItemMask;
}

void RivalItemCtrl::releaseButton()
{
    KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(getMyID());
    pad->mButtons.mRelease |= KartCtrl::getKartCtrl()->getKartPad(getMyID())->mItemMask;
}

void RivalItemCtrl::releaseButtonStickFront(float stick)
{
    KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(getMyID());
    pad->mButtons.mRelease |= KartCtrl::getKartCtrl()->getKartPad(getMyID())->mItemMask;
    pad->mMainStick.mY = stick;
}

void RivalItemCtrl::releaseButtonStickBack(float stick)
{
    KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(getMyID());
    pad->mButtons.mRelease |= KartCtrl::getKartCtrl()->getKartPad(getMyID())->mItemMask;
    pad->mMainStick.mY = -stick;
}

void RivalItemCtrl::triggerButton()
{
    KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(getMyID());
    pad->mButtons.mTrigger |= KartCtrl::getKartCtrl()->getKartPad(getMyID())->mItemMask;

    if (mItem->getKind() > 16)
    {
        if (mItem->getSuccessionItemNum() == 0)
        {
            mFlags &= ~2;
            mStateFunc = 0;
        }
    }
    else if (mItem->getKind() != 12)
    {
        mFlags &= ~2;
        mStateFunc = 0;
    }

    mUseCnt = 0;
    mUseWait = 0;
}

void RivalItemCtrl::triggerButtonStickFront(float stick)
{
    KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(getMyID());
    pad->mButtons.mTrigger |= KartCtrl::getKartCtrl()->getKartPad(getMyID())->mItemMask;
    pad->mMainStick.mY = stick;

    if (mItem->getKind() > 16)
    {
        if (mItem->getSuccessionItemNum() == 0)
        {
            mFlags &= ~2;
            mStateFunc = 0;
        }
    }
    else if (mItem->getKind() != 12)
    {
        mFlags &= ~2;
        mStateFunc = 0;
    }

    mUseCnt = 0;
    mUseWait = 0;
}

void RivalItemCtrl::triggerButtonStickBack(float stick)
{
    KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(getMyID());
    pad->mButtons.mTrigger |= KartCtrl::getKartCtrl()->getKartPad(getMyID())->mItemMask;
    pad->mMainStick.mY = -stick;

    if (mItem->getKind() > 16)
    {
        if (mItem->getSuccessionItemNum() == 0)
        {
            mFlags &= ~2;
            mStateFunc = 0;
        }
    }
    else if (mItem->getKind() != 12)
    {
        mFlags &= ~2;
        mStateFunc = 0;
    }

    mUseCnt = 0;
    mUseWait = 0;
}
