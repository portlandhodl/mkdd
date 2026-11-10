#include "Yamamoto/kartChapter.h"

#include "JSystem/JAudio/JASFakeMatch2.h"
#include "JSystem/JUtility/JUTGamePad.h"

#include "Kameda/J2DManager.h"
#include "Kameda/WipeManager.h"
#include "Kaneshige/Course/CrsArea.h"
#include "Kaneshige/JugemPoint.h"
#include "Kaneshige/KartChecker.h"
#include "Kaneshige/KartDrawer.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/RivalBodyCtrl.h"
#include "Sato/RivalKart.h"
#include "Sato/stEffectMgr.h"
#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCamera.h"
#include "Yamamoto/KartAnime.h"
#include "Yamamoto/KartCheck.h"
#include "Yamamoto/KartCrash.h"
#include "Yamamoto/KartDamage.h"
#include "Yamamoto/kartCtrl.h"
#include "Yamamoto/KartDemoCam.h"
#include "Yamamoto/KartItem.h"
#include "Yamamoto/KartMath.h"
#include "Yamamoto/KartSound.h"
#include "Yamamoto/KartStrat.h"
#include "Yamamoto/KartTarget.h"

// comments inside functions are inline functions being called in that function

void KartGame::DoVisible() {
    KartBody *body = mBody;
    KartDrawer *drawer = RaceMgr::getCurrentManager()->getKartDrawer(body->mMynum);
    body->mGameStatus &= ~0x2000;
    body->mShadowModel->clrVisibleAll();
    drawer->hide();
    GetStEfctMgr()->setExhaustStopFlg(body->mMynum, true);
}

void KartGame::DoWinCamera() {
    KartBody *body = mBody;
    int idx = body->mMynum;
    _0c = 1;
    body->getGame()->MakeClear();
    body->_584 = 0;
    body->_588 = 0;
    body->mCarStatus &= ~(u64)0x0000100000400000;
    body->getDamage()->ClrAllDamage();
    GetKartCtrl()->getKartAnime(idx)->mFlags |= (u64)0x800000000;
    body->SetUpRigidBodyPose(&body->getGame()->_20, &body->getGame()->_2C);
    for (int i = 0; i < 4; i++)
        GetKartCtrl()->getKartSus(i + idx * 4)->InitTirePose(i);
    body->SetUpRigidBodyStartGridPose();
    if (GetKartCtrl()->CheckCamera(idx)) {
        int camNum = GetKartCtrl()->GetCameraNum(idx);
        KartCam *cam = GetKartCtrl()->getKartCam(camNum);
        cam->mFlags &= ~0x10;
        cam->mFlags |= 0x200;
        cam->GetDemoCam()->InitID();
    }
}

void KartGame::DoChapterOne() {
    KartBody *body = mBody;
    int idx = body->mMynum;
    GetKartCtrl()->GetDriveCont(idx);
    GetKartCtrl()->GetCoDriveCont(idx);
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    GetGeoObjMgr()->getJugem(0);
    if (_10 < 0xffff)
        _10++;
    if (GetKartCtrl()->CheckJugemuSignal() || _10 > 20) {
        if (GetKartCtrl()->GetDriveCont(idx)->testTrigger(0x100) ||
            GetKartCtrl()->GetCoDriveCont(idx)->testTrigger(0x100)) {
            for (int i = 0; i < mgr->getCameraNumber(); i++)
                GetKartCtrl()->getKartCam(i)->GetDemoCam()->DoExecution();
        } else if (mgr->isJugemCountStart()) {
            JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)41, idx, JGeometry::TVec3f(0.0f, 0.0f, 0.0f), 0);
            body->getScene()->mChapter = 2;
            body->_592 = 0;
            body->_594 = 0;
            GetKartCtrl()->_228 = 0;
            GetKartCtrl()->SetPadClear(GetKartCtrl()->GetDriveCont(idx));
            GetKartCtrl()->SetPadClear(GetKartCtrl()->GetCoDriveCont(idx));
            SetRank();
            if (mgr->getRaceMode() == 8 && body->mMyRank == 1)
                GetKartCtrl()->getKartSound(idx)->DoAllVoice(37);
        }
    }
}

void KartGame::DoChapterTwo() {
    KartBody *body = mBody;
    int idx = body->mMynum;
    KartGamePad *drive = GetKartCtrl()->GetDriveCont(idx);
    GetKartCtrl()->GetCoDriveCont(idx);
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    body->getStrat()->DoStepAccel();
    body->getStrat()->DoStepSterr();
    if (!(body->mGameStatus & 8)) {
        if (drive->testButton(GetKartCtrl()->getKartPad(idx)->mAccelBtn))
            body->_592++;
        else
            body->_592 = 0;
        if (body->_592 >= 30)
            body->_590 |= 1;
        else
            body->_590 &= ~1;
    }
    if (mgr->isAbleStart() == 1) {
        if (body->mIdx == 20) {
            JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)40, body->mMynum, JGeometry::TVec3f(0.0f, 0.0f, 0.0f), 0);
        }
        mCountDownDuration = 180;
        body->getScene()->mChapter = 3;
        body->_3c8 = 0.0f;
        body->_592 = 0;
        body->_594 = 0;
        body->mCarStatus |= (u64)0x2000000;
        if ((body->mGameStatus & 0x10) && (body->mGameStatus & 0x2000))
            body->mShadowModel->setVisibleAll();
        if (body->mGameStatus & 8) {
            if (body->_5c2 == 1)
                body->_590 |= 2;
            else if (body->_5c2 == 2)
                body->_590 |= 1;
        }
    }
}

void KartGame::DoChapterThree() {
    KartBody *body = mBody;
    int idx = body->mMynum;
    KartGamePad *drive = GetKartCtrl()->GetDriveCont(idx);
    KartGamePad *coDrive = GetKartCtrl()->GetCoDriveCont(idx);
    if (body->mGameStatus & 8) {
        if (body->_590 & 2) {
            body->mCarStatus &= 0xfffffffffdffffffull;
            body->_590 |= 2;
            body->getScene()->mChapter = 4;
        } else if (body->_590 & 1) {
            body->mCarStatus &= 0xfffffffffdffffffull;
            body->getScene()->mChapter = 4;
            body->mGameStatus |= 4;
        } else {
            body->mCarStatus &= 0xfffffffffdffffffull;
            body->getScene()->mChapter = 4;
            body->mGameStatus |= 4;
        }
    } else {
        body->getStrat()->DoStepAccel();
        if (body->_592 < 60)
            body->_592++;
        if ((body->mGameStatus & 1) && body->_592 < 9) {
            if (coDrive->testTrigger(GetKartCtrl()->getKartPad(idx)->mAccelBtn)) {
                if (!(body->_590 & 1)) {
                    body->_590 |= 0x20;
                    body->mCarStatus &= 0xfffffffffdffffffull;
                }
            }
        }
        if (body->_592 < 10 && drive->testTrigger(GetKartCtrl()->getKartPad(idx)->mAccelBtn) &&
            !(body->_590 & 1)) {
            body->mCarStatus &= 0xfffffffffdffffffull;
            if (body->_590 & 0x20)
                body->_590 |= 0x40;
            else
                body->_590 |= 2;
            body->_590 &= ~0x20;
            body->getScene()->mChapter = 4;
        } else if (body->_592 > 20 || (body->_590 & 1)) {
            body->mCarStatus &= 0xfffffffffdffffffull;
            body->getScene()->mChapter = 4;
            body->mGameStatus |= 4;
        } else if (drive->testButton(GetKartCtrl()->getKartPad(idx)->mAccelBtn)) {
            body->mCarStatus |= (u64)0x2000000;
            GetKartCtrl()->getKartAnime(idx)->mFlags |= 0x800;
            body->getScene()->mChapter = 7;
            body->mGameStatus |= 4;
        }
    }
}

void KartGame::DoChapterFour() {
    KartBody *body = mBody;
    u16 st = body->_590;
    int idx = body->mMynum;
    if (st & 0x42) {
        MakeStartDash();
        body->_590 &= ~0x42;
        GetKartCtrl()->getKartAnime(idx)->mFlags |= 0x8000;
        body->mGameStatus |= 4;
    } else if (st & 1) {
        body->mCarStatus &= 0xfffffffffdffffffull;
        MakeWheelSpin();
        body->_590 &= ~1;
        GetKartCtrl()->getKartAnime(idx)->mFlags |= 0x10000;
    } else if (body->getDamage()->TstSetDamage() && !(body->getDamage()->mFlags & 8)) {
        GetKartCtrl()->getKartAnime(idx)->mFlags |= 0x4000;
        body->mCarStatus &= 0xfffffffffdffffffull;
    }
    body->getScene()->mChapter = 5;
}

void KartGame::DoChapterFive() {
    KartBody *body = mBody;
    int idx = body->mMynum;
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    KartChecker *checker = RCMGetKartChecker(idx);
    u8 chapter;
    mgr->getKartDrawer(idx);
    body->getItem()->SetSucMashItem();
    if (mCountDownDuration != 0)
        mCountDownDuration--;
    switch (mgr->getRaceMode()) {
    case 4:
        body->getCrash()->DoMatchlessTimer();
        if (idx == GetKartCtrl()->CheckWinner()) {
            if (mgr->isRaceEnd()) {
                DoStopItm();
                body->_592 = 0;
                DoChapterBomEsc();
                chapter = 10;
                body->getScene()->mChapter = chapter;
            }
        }
        if (CheckBalloon() && body->getTouchNum() != 0) {
            DoStopItm();
            body->_592 = 0;
            GetKartCtrl()->getKartAnime(idx)->mFlags |= (u64)0x1000000000;
            chapter = 9;
                body->getScene()->mChapter = chapter;
        }
        break;
    case 7:
        body->getCrash()->DoMatchlessTimer();
        body->getItem()->DoRabbit();
        // fallthrough
    case 5:
        if (mgr->isRaceEnd()) {
            DoStopItm();
            body->mGameStatus |= 0x400;
            body->_592 = 0;
            if (idx == GetKartCtrl()->CheckWinner()) {
                DoChapterBomEsc();
                chapter = 10;
                body->getScene()->mChapter = chapter;
            } else {
                GetKartCtrl()->getKartAnime(idx)->mFlags |= (u64)0x1000000000;
                chapter = 9;
                body->getScene()->mChapter = chapter;
            }
            break;
        }
        break;
    case 6:
        body->getCrash()->DoMatchlessTimer();
        if (mgr->isRaceEnd()) {
            DoStopItm();
            body->mGameStatus |= 0x400;
            body->_592 = 0;
            if (idx == GetKartCtrl()->CheckWinner()) {
                DoChapterBomEsc();
                chapter = 10;
                body->getScene()->mChapter = chapter;
            } else {
                GetKartCtrl()->getKartAnime(idx)->mFlags |= (u64)0x1000000000;
                chapter = 9;
                body->getScene()->mChapter = chapter;
            }
            break;
        }
        break;
    case 1:
        if (!checker->isGoal() && checker->isLapRenewal() && checker->isCurrentLapTimeRenewal())
            GetKartCtrl()->getKartAnime(idx)->mFlags |= (u64)0x400000000;
        break;
    }
    KartCam *cam;
    if (mgr->isAwardDemoMode())
        return;
    if (mgr->isStaffRoll()) {
        DoStaffRoll();
        return;
    }
    if (!mgr->isRaceEnd() && !checker->isGoal())
        goto after;
    if (body->mGameStatus & 0x20)
        goto after;
    if (GetKartCtrl()->CheckCamera(idx)) {
        cam = GetKartCtrl()->getKartCam(GetKartCtrl()->GetCameraNum(idx));
        cam->mArea.searchCamera(body->mPos);
        cam->mCurCamIdx = cam->mArea.getCameraID();
    }
    if (GetKartCtrl()->CheckCamera(idx)) {
        if (!(body->mGameStatus & 0x88))
            cam->mFlags |= 0x800;
    }
    if ((body->mGameStatus & 0x10) && (body->mGameStatus & 0x2000)) {
        GetKartCtrl()->SetGhostPadClear(GetKartCtrl()->GetDriveCont(idx));
        GetKartCtrl()->SetGhostPadClear(GetKartCtrl()->GetCoDriveCont(idx));
        body->mGameStatus |= 0x8028;
        body->mGameStatus &= ~0x893;
        GetKartCtrl()->getKartEnemy(body->mMynum)->getBodyCtrl()->comebackRescure(
            GetKartCtrl()->getKartTarget(idx)->mCenter, true);
    } else {
        body->mGameStatus |= 0x28;
        body->mGameStatus &= ~0x893;
        GetKartCtrl()->getKartEnemy(body->mMynum)->getBodyCtrl()->comebackRescure(
            GetKartCtrl()->getKartTarget(idx)->mCenter, true);
    }
    GetKartCtrl()->SetPadClear(GetKartCtrl()->GetDriveCont(idx));
    GetKartCtrl()->SetPadClear(GetKartCtrl()->GetCoDriveCont(idx));
    body->getStrat()->LiftClear();
    body->getStrat()->PitchClear();
    body->getStrat()->AllGravyClear();
    if (GetKartCtrl()->getKartEnemy(idx)->getMaxSpeed() < body->_454)
        GetKartCtrl()->getKartEnemy(idx)->setMaxSpeed(body->_454);
    if (checker->isGoal()) {
        if (checker->isBestTotalTimeRenewal(0) && mgr->getRaceMode() == 1)
            GetKartCtrl()->getKartSound(idx)->DoKartsetSeSound(0x100B4);
        else
            GetKartCtrl()->getKartSound(idx)->DoGoalSound();
    }
    body->_59a = 0;
    body->getItem()->ReleseWanWan();
    DoStopItm();
    chapter = 6;
                body->getScene()->mChapter = chapter;
after:
    RankWatchMan();
    if (checker->isLapRenewal() && !checker->isFinalLapRenewal() && !(body->mGameStatus & 0x20) &&
        !mgr->isRaceEnd())
        GetKartCtrl()->getKartSound(idx)->DoLapSound();
    if (checker->isLapRenewal() && !(body->mGameStatus & 0x20) && !mgr->isRaceEnd() &&
        mgr->getRaceMode() == 1 && checker->isCurrentLapTimeRenewal())
        GetKartCtrl()->getKartSound(idx)->DoTandemVoice(0x27);
}

void KartGame::DoChapterSix() {
    KartBody *body = mBody;
    int idx = body->mMynum;
    if (body->_59a <= (s16)6120)
        body->_59a++;
    if (body->_59a >= (s16)119 && body->_59a <= (s16)1920) {
        RaceMgr *mgr = RaceMgr::getCurrentManager();
        KartChecker *checker = RCMGetKartChecker(idx);
        if (mgr->getRaceMode() == 1) {
            if (!body->getChecker()->CheckOnlyTandemPartsClearKey(idx) &&
                checker->isBestTotalTimeRenewal(0))
                GetKartCtrl()->getKartAnime(idx)->mFlags |= (u64)0x200000000;
        } else {
            if (!body->getChecker()->CheckOnlyTandemPartsClearKey(idx))
                GetKartCtrl()->getKartAnime(idx)->mFlags |= (u64)0x200000000;
        }
    }
    if (body->mGameStatus & 0x8000) {
        if (body->_59a == 59)
            RaceMgr::getCurrentManager()->getKartDrawer(idx)->fadeOutGhost();
        if (body->_59a == 120) {
            body->mGameStatus &= ~0xa000;
            body->mShadowModel->clrVisibleAll();
            GetStEfctMgr()->setExhaustStopFlg(body->mMynum, true);
            body->mEffctVel.zero();
        }
    } else {
        if (body->_59a == 60)
            GetKartCtrl()->getKartSound(idx)->DoGoalVoice();
        if (body->_59a == 240)
            GetKartCtrl()->getKartSound(idx)->DoDriverGoalVoice();
    }
    if (GetKartCtrl()->CheckCamera(idx)) {
        int camNum = GetKartCtrl()->GetCameraNum(idx);
        KartCam *cam = GetKartCtrl()->getKartCam(camNum);
        if (body->_59a == 120 && (cam->mFlags & 0x800)) {
            body->mGameStatus |= 0x40;
            cam->GetDemoCam()->InitID();
            cam->mFlags &= ~0x800;
        }
    }
    if (body->_59a == (s16)620 && (body->mGameStatus & 0x40)) {
        int camNum = GetKartCtrl()->GetCameraNum(idx);
        KartCam *cam = GetKartCtrl()->getKartCam(camNum);
        cam->mArea.searchCamera(body->mPos);
        cam->mCurCamIdx = cam->mArea.getCameraID();
    }
    if (body->_59a > (s16)620 && (body->mGameStatus & 0x40)) {
        int camNum = GetKartCtrl()->GetCameraNum(idx);
        KartCam *cam = GetKartCtrl()->getKartCam(camNum);
        cam->mArea.searchCamera(body->mPos);
        if (cam->mCurCamIdx != cam->mArea.getCameraID()) {
            body->mGameStatus &= ~0x40;
            cam->GetDemoCam()->InitID();
        }
    }
}

void KartGame::DoChapterSeven() {
    KartBody *body = mBody;
    if (DoPushStart() == 1)
        body->getScene()->mChapter = 4;
    if (body->mCarStatus & (u64)0x100000)
        body->getScene()->mChapter = 5;
}

void KartGame::DoChapterBalloon() {
    KartBody *body = mBody;
    int idx = body->mMynum;
    int camNum;
    DoStopItm();
    if (body->_592 == 0) {
        GetStEfctMgr()->setExhaustStopFlg(body->mMynum, true);
        body->mEffctVel.zero();
        body->getCrash()->ClearMatchlessTimer();
        if (body->mCarStatus & 0x40000)
            GetKartCtrl()->getKartSound(body->mMynum)->DoStarStopSound();
    }
    if (body->_592 == 60)
        body->mGameStatus |= 0x400;
    if (body->_592 == 20)
        GETJ2DManager()->startLoserAnm(idx);
    if (body->_592 == 50) {
        GETWipeManager()->setWipe(idx, 14, 0, 30, 0, JUTColor(0, 0, 0, 126));
    }
    if (body->_592 == (s16)79)
        GETWipeManager()->stopWipe(idx);
    if (RaceMgr::getCurrentManager()->getRaceMode() == 4) {
        if (body->_592 == (s16)80) {
            if (RCMGetKartChecker(idx)->getRank() != 2)
                GetKartCtrl()->getKartSound(idx)->DoKartsetSeSound(0x10064);
        }
    }
    if (body->_592 == (s16)80)
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)12, body->mMynum, body->mPos, 2);
    if (body->_592 >= (s16)80) {
        body->_568 = GetKartCtrl()->fcnvge(body->_568, 0.01f, 0.05f, 0.05f);
        if (body->_568 == 0.01f) {
            DoVisible();
            if (RaceMgr::getCurrentManager()->getRaceMode() != 4) {
                if (RaceMgr::getCurrentManager()->isLANMode()) {
                    body->mGameStatus |= 0x4000;
                    if (GetKartCtrl()->getKartBody(GetKartCtrl()->CheckWinner())->getGame()->_0c != 0) {
                        if (GetKartCtrl()->CheckCamera(idx)) {
                            camNum = GetKartCtrl()->GetCameraNum(idx);
                            GETWipeManager()->startWipe(idx);
                            KartCam *cam = GetKartCtrl()->getKartCam(camNum);
                            cam->mFlags &= ~0x10;
                            cam->mFlags |= 0x200;
                            cam->GetDemoCam()->_38 &= ~0x100;
                            cam->SetTargetNum(GetKartCtrl()->CheckWinner());
                            cam->SetTarget();
                            cam->GetDemoCam()->mBody = cam->mTargetBody;
                            cam->GetDemoCam()->InitID();
                        }
                        body->getScene()->mChapter = 8;
                        body->mGameStatus &= ~0x4000;
                    }
                } else {
                    body->getScene()->mChapter = 8;
                }
            } else {
                body->mGameStatus |= 0x4000;
            }
        }
    }
    if (RaceMgr::getCurrentManager()->getRaceMode() == 4) {
        if (RaceMgr::getCurrentManager()->isLANMode()) {
            if (GetKartCtrl()->getKartBody(GetKartCtrl()->CheckWinner())->getGame()->_0c != 0) {
                if (GetKartCtrl()->CheckCamera(idx)) {
                    camNum = GetKartCtrl()->GetCameraNum(idx);
                    GETWipeManager()->killWipe(idx);
                    KartCam *cam = GetKartCtrl()->getKartCam(camNum);
                    cam->mFlags &= ~0x10;
                    cam->mFlags |= 0x200;
                    cam->GetDemoCam()->_38 &= ~0x100;
                    cam->SetTargetNum(GetKartCtrl()->CheckWinner());
                    cam->SetTarget();
                    cam->GetDemoCam()->mBody = cam->mTargetBody;
                    cam->GetDemoCam()->InitID();
                }
                body->getScene()->mChapter = 8;
                body->mGameStatus &= ~0x4000;
            } else if (body->_592 == 160) {
                if (GetKartCtrl()->CheckCamera(idx))
                    GetKartCtrl()->getKartCam(GetKartCtrl()->GetCameraNum(idx))->GetDemoCam()->SetUpMapID();
            }
        } else {
            if (body->_592 == 160) {
                if (GetKartCtrl()->CheckCamera(idx))
                    GetKartCtrl()->getKartCam(GetKartCtrl()->GetCameraNum(idx))->GetDemoCam()->SetUpMapID();
                body->getScene()->mChapter = 8;
                body->mGameStatus &= ~0x4000;
            }
        }
    }
    if (body->_592 <= 255)
        body->_592++;
}

void KartGame::DoChapterBomEsc() {
    KartBody *body = mBody;
    int idx = body->mMynum;
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    int winner = GetKartCtrl()->CheckWinner();
    if (idx == winner) {
        DoStopItm();
        if (body->_592 == 0) {
            GetStEfctMgr()->setExhaustStopFlg(body->mMynum, true);
            body->mEffctVel.zero();
            body->getCrash()->ClearMatchlessTimer();
        }
        if (body->_592 == 60)
            body->mGameStatus |= 0x400;
        if (body->_592 == 120)
            GetKartCtrl()->getKartSound(idx)->DoGoalVoice();
        if (body->_592 == 300)
            GetKartCtrl()->getKartSound(idx)->DoDriverGoalVoice();
        if (mgr->isPlayingWinnerAnm())
            GetKartCtrl()->getKartAnime(idx)->mFlags |= (u64)0x800000000;
        if (mgr->isMovingToStartPoint())
            DoWinCamera();
        if (mgr->isPlayingWinnerAnm() && mgr->isMovingToStartPoint())
            body->getScene()->mChapter = 11;
    } else {
        GetKartCtrl()->getKartAnime(idx)->mFlags |= (u64)0x1000000000;
        DoVisible();
        body->getScene()->mChapter = 8;
    }
    if (body->_592 <= 1024)
        body->_592++;
}

void KartGame::DoStaffRoll() {
    KartBody *body = mBody;
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    KartChecker *checker = RCMGetKartChecker(body->mMynum);
    JugemPoint *demo = checker->getDemoPoint();
    if (demo != nullptr) {
        JGeometry::TVec3f pos;
        JGeometry::TVec3f dir;
        demo->getPosition(&pos);
        demo->getFrDirection(&dir);
        body->SetUpRigidBodyPose(&pos, &dir);
        for (int i = 0; i < 4; i++)
            GetKartCtrl()->getKartSus(i + body->mMynum * 4)->InitTirePose(i);
        body->SetUpRigidBodyStartGridPose();
        CLPoint *point = demo->getCLPoint();
        GetKartCtrl()->getKartTarget(body->mMynum)->DoSearchRescueTarget(point);
        GetKartCtrl()->getKartEnemy(body->mMynum)->getBodyCtrl()->comebackRescure(point, false);
        if (mgr->isPlayingDemoWinnerAnm())
            GetKartCtrl()->getKartAnime(body->mMynum)->mFlags |= (u64)0x10000000000;
        checker->clrDemoPoint();
    }
}

void KartGame::DoWaitBattleWin() {
    KartBody *body = mBody;
    int idx = body->mMynum;
    int camNum;
    DoStopItm();
    if (body->_592 == 120)
        GetKartCtrl()->getKartSound(idx)->DoGoalVoice();
    if (body->_592 == 300)
        GetKartCtrl()->getKartSound(idx)->DoDriverGoalVoice();
    if (body->_592 <= 1024)
        body->_592++;
}

void KartGame::DoStartGoalCtl() {
    switch (mBody->getScene()->mChapter) {
    case 1:
        DoChapterOne();
        break;
    case 2:
        DoChapterTwo();
        break;
    case 3:
        DoChapterThree();
        break;
    case 4:
        DoChapterFour();
        break;
    case 5:
        DoChapterFive();
        break;
    case 6:
        DoChapterSix();
        break;
    case 7:
        DoChapterSeven();
        break;
    case 8:
        DoStopItm();
        break;
    case 9:
        DoChapterBalloon();
        break;
    case 10:
        DoChapterBomEsc();
        break;
    case 11:
        DoWaitBattleWin();
        break;
    }
}
