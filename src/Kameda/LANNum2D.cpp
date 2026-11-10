#include "Kameda/LANNum2D.h"
#include "Kameda/J2DManager.h"

#include "Osako/system.h"

#include "JSystem/JKernel.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

#include "Kaneshige/RaceMgr.h"
#include "Kameda/Race2D.h"
#include "Kameda/Result2D.h"
#include "Yamamoto/kartCtrl.h"

#include "mathHelper.h"

LANNum2D::LANNum2D(JKRHeap *heap) {
    mNode = new (heap, 0) LANNum2DHioNode();

    mOrtho = System::mspJ2DOrtho;

    mScreen = new (heap, 0) J2DScreen();

    switch (RCMGetManager()->getConsoleNumber()) {
    case 1:
        mScreen->set("LANNum1.blo", 0x40000, J2DManager::mThis->mArchive);
        mAnimation = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
            JKRFileLoader::getGlbResource("LANNum1.bck", J2DManager::getManager()->getArchive()));
        mScreen->setAnimation(mAnimation);
        break;
    case 2:
        mScreen->set("LANNum2.blo", 0x40000, J2DManager::mThis->mArchive);
        mAnimation = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
            JKRFileLoader::getGlbResource("LANNum2.bck", J2DManager::getManager()->getArchive()));
        mScreen->setAnimation(mAnimation);
        for (int i = 0; i < 2; i++) {
            J2DPicture *pane = (J2DPicture *)mScreen->search(0x4e456e6431ULL + i);
            mEnEntry[i].mPane = pane;
            mEnEntry[i].mAnimation = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
                JKRFileLoader::getGlbResource("LANNum2.bck", J2DManager::getManager()->getArchive()));
            mEnEntry[i].mPane->setAnimation(mEnEntry[i].mAnimation);
        }
        for (int i = 0; i < 2; i++) {
            J2DPicture *pane = (J2DPicture *)mScreen->search(0x4e4e756d31ULL + i);
            mNuEntry[i].mPane = pane;
            mNuEntry[i].mAnimation = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
                JKRFileLoader::getGlbResource("LANNum2.bck", J2DManager::getManager()->getArchive()));
            mNuEntry[i].mPane->setAnimation(mNuEntry[i].mAnimation);
        }
        break;
    case 3:
    case 4:
        mScreen->set("LANNum4.blo", 0x40000, J2DManager::mThis->mArchive);
        mAnimation = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
            JKRFileLoader::getGlbResource("LANNum4.bck", J2DManager::getManager()->getArchive()));
        mScreen->setAnimation(mAnimation);
        for (int i = 0; i < 4; i++) {
            J2DPicture *pane = (J2DPicture *)mScreen->search(0x4e456e6431ULL + i);
            mEnEntry[i].mPane = pane;
            mEnEntry[i].mAnimation = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
                JKRFileLoader::getGlbResource("LANNum4.bck", J2DManager::getManager()->getArchive()));
            mEnEntry[i].mPane->setAnimation(mEnEntry[i].mAnimation);
        }
        for (int i = 0; i < 4; i++) {
            J2DPicture *pane = (J2DPicture *)mScreen->search(0x4e4e756d31ULL + i);
            mNuEntry[i].mPane = pane;
            mNuEntry[i].mAnimation = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(
                JKRFileLoader::getGlbResource("LANNum4.bck", J2DManager::getManager()->getArchive()));
            mNuEntry[i].mPane->setAnimation(mNuEntry[i].mAnimation);
        }
        break;
    default:
#line 114
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++) {
            int n = i * 0x100 + j;
            mConsolePanes[i].mPane[j][0] = (J2DPicture *)mScreen->search(0x4e503131ULL + n);

            J2DPicture *pane = (J2DPicture *)mScreen->search(0x50423131ULL + n);
#line 128
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mConsolePanes[i].mPane[j][1] = pane;

            pane = (J2DPicture *)mScreen->search(0x504d3131ULL + n);
#line 136
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mConsolePanes[i].mPane[j][2] = pane;
        }
    }

    for (int i = 0; i < 4; i++) {
        int n = 0x4e756d31 + i;
        mNumPanes[i].mNum = (J2DPicture *)mScreen->search(0x4e4e756d31ULL + i);
        mNumPanes[i].mNumB = (J2DPicture *)mScreen->search(0x424e756d31ULL + i);
        mNumPanes[i].mNumK = (J2DPicture *)mScreen->search(0x4b4e756d31ULL + i);
    }

    init();
}


LANNum2DParam::~LANNum2DParam() {}

LANNum2DHioNode::~LANNum2DHioNode() {}

void LANNum2D::setDraw1() {
    switch (RCMGetManager()->getStatusNumber()) {
    case 0:
        mConsolePanes[0].mPane[0][0]->hide();
        mConsolePanes[0].mPane[1][0]->hide();
        mNumPanes[0].mNum->hide();
    case 1:
        break;
    default:
#line 190
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        int status = J2DManager::getStatus2Kart(i);
        KartCtrl *ctrl = KartCtrl::getKartCtrl();
        KartGamePad *pad = ctrl->getPad(status);
        KartGamePad *coPad = ctrl->getCoPad(status);

        int port = pad->getPadPort();
        if (port >= 0 && port < 4) {
            mConsolePanes[i].mPane[0][1]->setCornerColor(Race2D::getCornerColor(port));
            mConsolePanes[i].mPane[0][2]->setCornerColor(Race2D::getCornerColor(port));
            mConsolePanes[i].mPane[0][2]->changeTexture(Result2D::mPlayerNumberName[port], 0);
        }

        if (RCMGetManager()->getKartInfo(status)->getPad(1) != NULL) {
            int coPort = coPad->getPadPort();
            if (coPort >= 0 && coPort < 4) {
                mConsolePanes[i].mPane[1][1]->setCornerColor(Race2D::getCornerColor(coPort));
                mConsolePanes[i].mPane[1][2]->setCornerColor(Race2D::getCornerColor(coPort));
                mConsolePanes[i].mPane[1][2]->changeTexture(Result2D::mPlayerNumberName[coPort], 0);
            }
        } else {
            mConsolePanes[i].mPane[1][0]->hide();
        }

        if (status >= 0 && status < 8) {
            mNumPanes[i].mNumB->setCornerColor(Race2D::getCornerColor(status));
            mNumPanes[i].mNumK->setCornerColor(Race2D::getCornerColor(status));
            mNumPanes[i].mNumK->changeTexture(J2DManager::mKartNumberName[status], 0);
        }
    }

    if (RCMGetManager()->getStatusNumber() == 1) {
        int status = J2DManager::getStatus2Kart(0);
        if (RCMGetManager()->getKartInfo(status)->getPad(1) == NULL) {
            mConsolePanes[0].mPane[0][0]->hide();
            mConsolePanes[0].mPane[1][0]->hide();
        }
    }
}

void LANNum2D::setDraw2() {
    switch (RCMGetManager()->getStatusNumber()) {
    case 0:
        mConsolePanes[0].mPane[0][0]->hide();
        mConsolePanes[0].mPane[1][0]->hide();
        mNumPanes[0].mNum->hide();
    case 1:
        mConsolePanes[1].mPane[0][0]->hide();
        mConsolePanes[1].mPane[1][0]->hide();
        mNumPanes[1].mNum->hide();
        break;
    case 2:
        break;
    default:
#line 251
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        int status = J2DManager::getStatus2Kart(i);
        KartCtrl *ctrl = KartCtrl::getKartCtrl();
        KartGamePad *pad = ctrl->getPad(status);
        KartGamePad *coPad = ctrl->getCoPad(status);

        int port = pad->getPadPort();
        if (port >= 0 && port < 4) {
            mConsolePanes[i].mPane[0][1]->setCornerColor(Race2D::getCornerColor(port));
            mConsolePanes[i].mPane[0][2]->setCornerColor(Race2D::getCornerColor(port));
            mConsolePanes[i].mPane[0][2]->changeTexture(Result2D::mPlayerNumberName[port], 0);
        }

        if (RCMGetManager()->getKartInfo(status)->getPad(1) != NULL) {
            int coPort = coPad->getPadPort();
            if (coPort >= 0 && coPort < 4) {
                mConsolePanes[i].mPane[1][1]->setCornerColor(Race2D::getCornerColor(coPort));
                mConsolePanes[i].mPane[1][2]->setCornerColor(Race2D::getCornerColor(coPort));
                mConsolePanes[i].mPane[1][2]->changeTexture(Result2D::mPlayerNumberName[coPort], 0);
            }
        } else {
            mConsolePanes[i].mPane[1][0]->hide();
        }

        if (status >= 0 && status < 8) {
            mNumPanes[i].mNumB->setCornerColor(Race2D::getCornerColor(status));
            mNumPanes[i].mNumK->setCornerColor(Race2D::getCornerColor(status));
            mNumPanes[i].mNumK->changeTexture(J2DManager::mKartNumberName[status], 0);
        }
    }

    if (RCMGetManager()->getStatusNumber() == 1) {
        int status = J2DManager::getStatus2Kart(0);
        if (RCMGetManager()->getKartInfo(status)->getPad(1) == NULL) {
            mConsolePanes[0].mPane[0][0]->hide();
            mConsolePanes[0].mPane[1][0]->hide();
        }
    }
}

void LANNum2D::setDraw4() {
    switch (RCMGetManager()->getStatusNumber()) {
    case 0:
        mConsolePanes[0].mPane[0][0]->hide();
        mConsolePanes[0].mPane[1][0]->hide();
        mNumPanes[0].mNum->hide();
    case 1:
        mConsolePanes[1].mPane[0][0]->hide();
        mNumPanes[1].mNum->hide();
    case 2:
        mConsolePanes[2].mPane[0][0]->hide();
        mNumPanes[2].mNum->hide();
    case 3:
        mConsolePanes[3].mPane[0][0]->hide();
        mNumPanes[3].mNum->hide();
        break;
    case 4:
        break;
    default:
#line 318
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        int status = J2DManager::getStatus2Kart(i);
        KartCtrl *ctrl = KartCtrl::getKartCtrl();
        KartGamePad *pad = ctrl->getPad(status);
        KartGamePad *coPad = ctrl->getCoPad(status);

        int port = pad->getPadPort();
        if (port >= 0 && port < 4) {
            mConsolePanes[i].mPane[0][1]->setCornerColor(Race2D::getCornerColor(port));
            mConsolePanes[i].mPane[0][2]->setCornerColor(Race2D::getCornerColor(port));
            mConsolePanes[i].mPane[0][2]->changeTexture(Result2D::mPlayerNumberName[port], 0);
        }

        if (RCMGetManager()->getKartInfo(status)->getPad(1) != NULL) {
            if (i == 0) {
                int coPort = coPad->getPadPort();
                if (coPort >= 0 && coPort < 4) {
                    mConsolePanes[i].mPane[1][1]->setCornerColor(Race2D::getCornerColor(coPort));
                    mConsolePanes[i].mPane[1][2]->setCornerColor(Race2D::getCornerColor(coPort));
                    mConsolePanes[i].mPane[1][2]->changeTexture(Result2D::mPlayerNumberName[coPort], 0);
                }
            }
        } else {
            if (i == 0) {
                mConsolePanes[i].mPane[1][0]->hide();
            }
        }

        if (status >= 0 && status < 8) {
            mNumPanes[i].mNumB->setCornerColor(Race2D::getCornerColor(status));
            mNumPanes[i].mNumK->setCornerColor(Race2D::getCornerColor(status));
            mNumPanes[i].mNumK->changeTexture(J2DManager::mKartNumberName[status], 0);
        }
    }

    if (RCMGetManager()->getStatusNumber() == 1) {
        int status = J2DManager::getStatus2Kart(0);
        if (RCMGetManager()->getKartInfo(status)->getPad(1) == NULL) {
            mConsolePanes[0].mPane[0][0]->hide();
            mConsolePanes[0].mPane[1][0]->hide();
        }
    }
}

void LANNum2D::init() {
    mDrawFlag = false;
    mDrawFlag2 = false;
    mFrame = 0.0f;
    mAnimation->mCurrentFrame = 0.0f;

    if (RCMGetManager()->getConsoleNumber() != 1) {
        for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
            mEnEntry[i].mAnimation->mCurrentFrame = 0.0f;
        }
    }

    mScreen->animation();
}

void LANNum2D::draw() {
    if (mNode->mParam.mFlag & 1 && mDrawFlag) {
        ERacePhase racePhase = RCMGetManager()->getRacePhase();
        if (racePhase != PHASE_CRS_DEMO) {
            RaceMgr *raceMgr = RCMGetManager();
            if (!raceMgr->isReplayMode() && !raceMgr->mRaceInfo->isWaitDemo()) {
                if (mFrame < 200.0f || mDrawFlag2) {
                    mOrtho->setPort();
                    mScreen->draw(0.0f, 0.0f, mOrtho);
                } else {
                    init();
                }
            }
        }
    }
}

void LANNum2D::calc() {
    if (mNode->mParam.mFlag & 1 && mDrawFlag && mFrame < 0.0f) {
        f32 frame = mFrame++;
        if (mDrawFlag2) {
            if (!(frame < 100.0f)) {
                frame = 100.0f;
            }
        } else {
            if (!(frame < 100.0f)) {
                if (frame < 190.0f) {
                    frame = 100.0f;
                } else {
                    frame -= 90.0f;
                }
            }
        }

        mAnimation->mCurrentFrame = frame;

        if (RCMGetManager()->getConsoleNumber() != 1) {
            for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
                mNuEntry[i].mAnimation->mCurrentFrame = frame;
            }
        }

        mScreen->animation();
    }
}

void LANNum2D::start() {
    mDrawFlag = true;

    if (RCMGetManager()->getConsoleNumber() != 1) {
        for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
            mEnEntry[i].mAnimation->mCurrentFrame = 10.0f;
        }
    }

    switch (RCMGetManager()->getConsoleNumber()) {
    case 1:
        setDraw1();
        break;
    case 2:
        setDraw2();
        break;
    case 3:
    case 4:
        setDraw4();
        break;
    default:
#line 489
        JUT_ASSERT(0);
        break;
    }

    if (!J2DManager::getNetFlag()) {
        J2DPane *pane = mScreen->search(0x4e4c616e);
        pane->hide();
    }
}

void LANNum2D::start2() {
    mDrawFlag = true;
    mDrawFlag2 = true;

    if (RCMGetManager()->getConsoleNumber() != 1) {
        for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
            mEnEntry[i].mAnimation->mCurrentFrame = 11.0f;
        }
    }

    switch (RCMGetManager()->getConsoleNumber()) {
    case 1:
        setDraw1();
        mConsolePanes[0].mPane[0][0]->hide();
        mConsolePanes[0].mPane[1][0]->hide();
        break;
    case 2:
        setDraw2();
        for (int i = 0; i < 2; i++) {
            mConsolePanes[i].mPane[0][0]->hide();
            mConsolePanes[i].mPane[1][0]->hide();
        }
        break;
    case 3:
    case 4:
        setDraw4();
        for (int i = 0; i < 4; i++) {
            mConsolePanes[i].mPane[0][0]->hide();
        }
        mConsolePanes[0].mPane[1][0]->hide();
        break;
    default:
#line 538
        JUT_ASSERT(0);
        break;
    }
}
