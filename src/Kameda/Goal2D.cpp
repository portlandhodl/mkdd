#include "Kameda/Goal2D.h"
#include "Kameda/J2DManager.h"
#include "Kameda/PauseManager.h"
#include "Kameda/Result2D.h"
#include "Kameda/SequenceInfo.h"
#include "Kaneshige/KartChecker.h"
#include "Kaneshige/RaceMgr.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel.h"
#include "Osako/system.h"
#include "Sato/JPEffectMgr.h"
#include "kartLocale.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "mathHelper.h"

bool Goal2D::mDrawEndFlag = false;

Goal2DParam::Goal2DParam() : mUnk4(0), mFlag(true) {}

Goal2DHioNode::Goal2DHioNode() : HioNode("ゴール", &mParam, 0, 0) {}

Goal2D::Goal2D(JKRHeap *heap) {
    mNode = new (heap, 0) Goal2DHioNode();

    for (int i = 0; i < 4; i++) {
        mOrtho[i] = nullptr;
    }

    switch (RCMGetManager()->getConsoleNumber()) {
    case 1: {
        mOrtho[0] = new (heap, 0) J2DOrthoGraph(System::get3DVpX(), System::get3DVpY(), System::get3DVpW(), System::get3DVpH(), -1.0f, 1.0f);
        mOrtho[0]->scissor(System::get3DScisX(), System::get3DScisY(), System::get3DScisW(), System::get3DScisH());
        mStatus[0].mGoalPosX = System::get3DVpX();
        mStatus[0].mGoalPosY = System::get3DVpY();
        break;
    }
    case 2:
        for (u8 i = 0; i < 2; i++) {
            mOrtho[i] = new (heap, 0) J2DOrthoGraph(System::get3DVpDiv2X(i), System::get3DVpDiv2Y(i), System::get3DVpDiv2W(i), System::get3DVpDiv2H(i), -1.0f, 1.0f);
            mOrtho[i]->setOrtho(System::get3DVpDiv2X(i), System::get3DVpDiv2Y(i), System::get3DVpDiv2W(i) * 2.0f, System::get3DVpDiv2H(i) * 2.0f, -1.0f, 1.0f);
            mOrtho[i]->scissor(System::get3DScisDiv2X(i), System::get3DScisDiv2Y(i), System::get3DScisDiv2W(i), System::get3DScisDiv2H(i));
            f32 px = System::get3DVpDiv2X(i);
            mStatus[i].mGoalPosX = px + System::get3DVpDiv2W(i) / 2;
            mStatus[i].mGoalPosY = System::get3DVpDiv2Y(i);
        }
        break;
    case 3:
    case 4:
        for (u8 i = 0; i < 4; i++) {
            mOrtho[i] = new (heap, 0) J2DOrthoGraph(System::get3DVpDiv4X(i), System::get3DVpDiv4Y(i), System::get3DVpDiv4W(i), System::get3DVpDiv4H(i), -1.0f, 1.0f);
            mOrtho[i]->setOrtho(System::get3DVpDiv4X(i), System::get3DVpDiv4Y(i), System::get3DVpDiv4W(i) * 2.0f, System::get3DVpDiv4H(i) * 2.0f, -1.0f, 1.0f);
            mOrtho[i]->scissor(System::get3DScisDiv4X(i), System::get3DScisDiv4Y(i), System::get3DScisDiv4W(i), System::get3DScisDiv4H(i));
            mStatus[i].mGoalPosX = System::get3DVpDiv4X(i);
            mStatus[i].mGoalPosY = System::get3DVpDiv4Y(i);
        }
        break;
    default:
#line 121 "Goal2D.cpp"
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < RCMGetManager()->getConsoleNumber(); i++) {
        mGoalScreen[i] = new (heap, 0) J2DScreen();
        mGoalScreen[i]->set("goal.blo", 0x40000, J2DManager::getManager()->getArchive());
        mGoalAnm1[i] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("goal.bck", J2DManager::getManager()->getArchive()));
        mGoalScreen[i]->setAnimation(mGoalAnm1[i]);
        mGoalAnm2[i] = (J2DAnmTevRegKey *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("goal.brk", J2DManager::getManager()->getArchive()));
        mGoalScreen[i]->setAnimation(mGoalAnm2[i]);
    }

    for (int i = 0; i < RCMGetManager()->getConsoleNumber(); i++) {
        mRetireScreen[i] = new (heap, 0) J2DScreen();
        mRetireScreen[i]->set("retire.blo", 0x40000, J2DManager::getManager()->getArchive());
        mRetireAnm1[i] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("retire.bck", J2DManager::getManager()->getArchive()));
        mRetireScreen[i]->setAnimation(mRetireAnm1[i]);
        mRetireAnm2[i] = (J2DAnmTevRegKey *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("retire.brk", J2DManager::getManager()->getArchive()));
        mRetireScreen[i]->setAnimation(mRetireAnm2[i]);
    }

    for (int i = 0; i < RCMGetManager()->getConsoleNumber(); i++) {
        for (int j = 0; j < 2; j++) {
            J2DPicture *pane = (J2DPicture *)mGoalScreen[i]->search(0x504943545f303031ULL + j);
#line 158 "Goal2D.cpp"
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mGoalPane[i][j] = pane;
            if (RCMGetManager()->isRaceModeMiniGame()) {
                mGoalPane[i][j]->changeTexture("MiniGame2DWin00.bti", 0);
            }
        }
    }

    for (int i = 0; i < RCMGetManager()->getConsoleNumber(); i++) {
        J2DPicture *pane = (J2DPicture *)mRetireScreen[i]->search(0x5265746972653030ULL);
#line 170 "Goal2D.cpp"
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mRetirePane[i] = pane;
        if (RCMGetManager()->isRaceModeMiniGame()) {
            mRetirePane[i]->changeTexture("MiniGame2DLose00.bti", 0);
            J2DPicture *pane2 = (J2DPicture *)mRetireScreen[i]->search(0x0052657469726542ULL);
            pane2->mIsVisible = false;
        }
    }

    init();
}


void Goal2D::init() {
    f32 zero = 0.0f;

    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        mStatus[i].mGoalStart = false;
        mStatus[i].mGoalDraw = false;
        mStatus[i].mFrame = 0;
        mGoalAnm1[i]->mCurrentFrame = zero;
        mGoalAnm2[i]->mCurrentFrame = zero;
        mGoalScreen[i]->animation();
        mRetireAnm1[i]->mCurrentFrame = zero;
        mRetireAnm2[i]->mCurrentFrame = zero;
        mRetireScreen[i]->animation();
    }

    mUnkE4 = 0;
    mUnkE8 = false;
    mDrawEndFlag = false;
    mUnkE9 = false;
}

Goal2D::~Goal2D() {
    for (u8 i = 0; i < 4; i++) {
        delete mOrtho[i];
        delete mGoalScreen[i];
    }
}

void Goal2D::drawGoal() {
    if (!(mNode->mParam.mFlag & 1))
        return;
    if (RCMGetManager()->getRacePhase() == PHASE_CRS_DEMO)
        return;
    if (RCMGetManager()->isReplayMode())
        return;
    if (RCMGetManager()->mRaceInfo->mDemoType != 0)
        return;
    if (!mUnkE8)
        return;

    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        int status = J2DManager::getStatus2Kart(i);
        KartChecker *checker = RCMGetManager()->getKartChecker(status);
        if (checker->isGoal() || J2DManager::getManager()->mWinnerAnmFlag[status]) {
            if (mStatus[i].mGoalDraw) {
                mOrtho[i]->setPort();
                mGoalScreen[i]->draw(mStatus[i].mGoalPosX, mStatus[i].mGoalPosY, mOrtho[i]);
            }
        }
    }
}

void Goal2D::drawRetire() {
    if (mDrawEndFlag)
        return;
    if (!(mNode->mParam.mFlag & 1))
        return;
    if (RCMGetManager()->getRacePhase() == PHASE_CRS_DEMO)
        return;
    if (RCMGetManager()->isReplayMode())
        return;
    if (RCMGetManager()->mRaceInfo->mDemoType != 0)
        return;
    if (!mUnkE8)
        return;

    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        KartChecker *checker = RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(i));
        if (!checker->isGoal()) {
            if (mStatus[i].mGoalDraw) {
                mOrtho[i]->setPort();
                mRetireScreen[i]->draw(mStatus[i].mGoalPosX, mStatus[i].mGoalPosY, mOrtho[i]);
            }
        }
    }
}

void Goal2D::calc() {
    if (mDrawEndFlag)
        return;

    int finished = 0;
    int active = 0;

    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        int kart = J2DManager::getStatus2Kart(i);
        KartChecker *checker = RCMGetManager()->getKartChecker(kart);
        if (!checker->isGoal() && !J2DManager::getManager()->mWinnerAnmFlag[kart] && !J2DManager::getManager()->mLoserAnmFlag[kart])
            continue;

        mUnkE8 = true;

        if (mStatus[i].mFrame < 128) {
            mStatus[i].mGoalStart = true;
            mStatus[i].mGoalDraw = true;
        }

        if (mStatus[i].mFrame == 0 && J2DManager::getManager()->mAnmFrame > 1 && RCMGetManager()->getRaceMode() == VERSUS_RACE) {
            mStatus[i].mFrame = 128;
            goto after_loop;
        }

        mStatus[i].mFrame++;

        if (J2DManager::getManager()->mAnmFrame < 70 && !RCMGetManager()->isRaceModeMiniGame() && mStatus[i].mFrame == 70) {
            mStatus[i].mFrame = 69;
        }

        if (checker->isGoal() || J2DManager::getManager()->mWinnerAnmFlag[kart]) {
            mGoalAnm1[i]->mCurrentFrame = mStatus[i].mFrame;
            mGoalAnm2[i]->mCurrentFrame = mStatus[i].mFrame;
            mGoalScreen[i]->animation();

            if (RCMGetManager()->getConsoleNumber() == 2) {
                for (int j = 0; j < 2; j++) {
                    J2DPicture *pane = mGoalPane[i][j];
                    f32 gx = 1.8f * pane->mScale.x;
                    f32 gy = 1.8f * pane->mScale.y;
                    pane->mScale.x = gx;
                    pane->mScale.y = gy;
                    pane->calcMtx();
                }
            }
        } else {
            mRetireAnm1[i]->mCurrentFrame = mStatus[i].mFrame;
            mRetireAnm2[i]->mCurrentFrame = mStatus[i].mFrame;
            mRetireScreen[i]->animation();

            if (RCMGetManager()->getConsoleNumber() == 2) {
                J2DPicture *pane = mRetirePane[i];
                f32 gx = 1.4f * pane->mScale.x;
                f32 gy = 1.4f * pane->mScale.y;
                pane->mScale.x = gx;
                pane->mScale.y = gy;
                pane->calcMtx();
            }

            u8 alpha = 255;
            if (RCMGetManager()->isRaceModeMiniGame()) {
                int f = mStatus[i].mFrame - 300;
                if (f < 0) {
                    alpha = 255;
                } else if (f < 20) {
                    alpha = 255 - (f * 255) / 20;
                } else {
                    alpha = 0;
                }
            }

            int hide = J2DManager::getManager()->getHideFrameRace2D();
            u8 hideAlpha;
            if (hide < 20) {
                hideAlpha = 255 - (hide * 255) / 20;
            } else {
                hideAlpha = 0;
            }

            if (alpha < hideAlpha) {
                mRetirePane[i]->setAlpha(alpha);
            } else {
                mRetirePane[i]->setAlpha(hideAlpha);
            }

            if (mStatus[i].mFrame == 1) {
                JGeometry::TVec3f scale(1.0f, 1.0f, 1.0f);
                JGeometry::TVec3f vec(0.0f, 0.0f, 0.0f);
                switch (RCMGetManager()->getConsoleNumber()) {
                case 1:
                    vec.set(304.0f, 260.0f, 0.0f);
                    break;
                case 2:
                    scale.x = scale.y = scale.z = 0.7f;
                    if (i == 0) {
                        vec.set(304.0f, 130.0f, 0.0f);
                    } else {
                        vec.set(304.0f, 355.0f, 0.0f);
                    }
                    break;
                case 3:
                case 4:
                    scale.x = scale.y = scale.z = 0.5f;
                    switch (i) {
                    case 0:
                        vec.set(152.0f, 130.0f, 0.0f);
                        break;
                    case 1:
                        vec.set(456.0f, 130.0f, 0.0f);
                        break;
                    case 2:
                        vec.set(152.0f, 355.0f, 0.0f);
                        break;
                    case 3:
                        vec.set(456.0f, 355.0f, 0.0f);
                        break;
                    default:
#line 415 "Goal2D.cpp"
                        JUT_ASSERT(0);
                        break;
                    }
                    break;
                default:
#line 419 "Goal2D.cpp"
                    JUT_ASSERT(0);
                    break;
                }

                JPABaseEmitter *emt = nullptr;
                switch (KartLocale::getLanguage()) {
                case 0:
                case 1:
                    if (!J2DManager::getManager()->isLANDemo())
                        emt = JPEffectMgr::mThis->createEmt2D("mk_2D_retire_b", vec);
                    break;
                case 2:
                case 3:
                case 5:
                    if (!J2DManager::getManager()->isLANDemo())
                        emt = JPEffectMgr::mThis->createEmt2D("mk_2D_retire_c", vec);
                    break;
                case 4:
                    if (!J2DManager::getManager()->isLANDemo())
                        emt = JPEffectMgr::mThis->createEmt2D("mk_2D_retire_a", vec);
                    break;
                default:
#line 443 "Goal2D.cpp"
                    JUT_ASSERT(0);
                    break;
                }

                if (emt != nullptr) {
                    emt->setGlobalScale(scale);
                }
            }
        }

        active++;
        if (mStatus[i].mFrame >= 128)
            finished++;
    }

after_loop:
    if (!RCMGetManager()->mRaceInfo->mIsLanMode && RCMGetManager()->mRaceDirector->isRaceEnd() && finished == active && PauseManager::getManager()->isResultStart() && J2DManager::getManager()->isRaceEnd()) {
        mDrawEndFlag = true;
        if (!RCMGetManager()->isReplayMode()) {
            switch (RCMGetManager()->getRaceMode()) {
            case TIME_ATTACK:
                sequenceTA();
                break;
            case GRAND_PRIX:
                sequenceGP();
                break;
            case VERSUS_RACE:
                sequenceVS();
                break;
            case BALLOON_BATTLE:
            case BOMB_BATTLE:
            case ESCAPE_BATTLE:
                sequenceMG();
                break;
            default:
#line 485 "Goal2D.cpp"
                JUT_ASSERT(0);
                break;
            }
        }
    }
}

void Goal2D::sequenceGP() {
    Result2D::setGPClr();
    gSequenceInfo.setClrGPCourse();
}

void Goal2D::sequenceVS() {
    setVSMGRank();

    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        int kart = J2DManager::getStatus2Kart(i);
        int rank = RCMGetManager()->getKartChecker(kart)->getRank() - 1;
        gSequenceInfo.inc_40(kart, rank);
    }
}

void Goal2D::sequenceTA() { PauseManager::getManager()->setTA(); }

void Goal2D::sequenceMG() {
    setVSMGRank();

    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        int kart = J2DManager::getStatus2Kart(i);
        KartChecker *checker = RCMGetManager()->getKartChecker(kart);
        if (checker->getRank() - 1 != 0)
            continue;
        gSequenceInfo.inc_80(kart);
    }
}

void Goal2D::setVSMGRank() {
    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        int kart = J2DManager::getStatus2Kart(i);
        int rank = RCMGetManager()->getKartChecker(kart)->getRank() - 1;
        if (rank < 0)
            continue;
        if (rank >= RCMGetManager()->getStatusNumber())
            continue;
        gSequenceInfo.setStartNo(kart, rank);
    }
}


#include "JSystem/JAudio/JASFakeMatch2.h"
