#include "Kameda/Pause2D.h"
#include "Kameda/PauseManager.h"
#include "Kaneshige/RaceMgr.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/JMath/JMath.h"
#include "mathHelper.h"
#include "Osako/system.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "Kameda/ReadPrintMessage.h"
#include "Osako/FontMgr.h"
#include "Osako/ResMgr.h"

int Pause2D::mPauseSelector;
Pause2D::PauseStatus Pause2D::mStatus;

__declspec(weak) Pause2DParam::Pause2DParam() : mFlag(1) {}

__declspec(weak) Pause2DParam::~Pause2DParam() {}

__declspec(weak) Pause2DHioNode::~Pause2DHioNode() {}

__declspec(weak) Pause2D::~Pause2D() {}

Pause2D::Pause2D(JKRHeap *heap) {
    mOrtho = System::mspJ2DOrtho;
    mNode = new (heap, 0) Pause2DHioNode();

    switch (RCMGetManager()->getRaceMode()) {
    case GRAND_PRIX:
    case AWARD_DEMO:
    case STAFF_ROLL: {
        mScreen = new (heap, 0) J2DScreen();
        mScreen->set("pause2.blo", 0x40000, PauseManager::getManager()->getArchive());
        void *bck = JKRFileLoader::getGlbResource("pause2.bck", PauseManager::getManager()->getArchive());
        mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
        mScreen->setAnimation(mAnm);
        mAnm2 = (J2DAnmColor *)J2DAnmLoaderDataBase::load(
            JKRFileLoader::getGlbResource("pause2.bpk", PauseManager::getManager()->getArchive()));
        mScreen->setAnimation(mAnm2);
        for (int i = 0; i < 2; i++) {
            mSel[i].mPane = (J2DPicture *)mScreen->search(0x4c696e6531ULL + i);
            mSel[i].mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
            mSel[i].mPane->setAnimation(mSel[i].mAnm);
        }
        for (int i = 0; i < 4; i++) {
            J2DPane *pane = mScreen->search(0x6b725f61ULL + i);
#line 91
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mKr[i] = (J2DPicture *)pane;
            mPos[i].x = mKr[i]->getTranslateX();
            mPos[i].y = mKr[i]->getTranslateY();
            mKr[i]->hide();
        }
        for (int i = 0; i < 2; i++) {
            J2DPane *pane = mScreen->search(0x504d31ULL + i);
#line 101
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mPics[i] = (J2DPicture *)pane;
        }
        mPics[0]->changeTexture("Mozi_Continue.bti", 0);
        mPics[1]->changeTexture("Mozi_Quit.bti", 0);
        setPadText();
        mDrawFunc = &Pause2D::setDrawGP;
        mCalcFunc = &Pause2D::selectorGP;
        break;
    }
    case VERSUS_RACE: {
        mScreen = new (heap, 0) J2DScreen();
        mScreen->set("pause5.blo", 0x40000, PauseManager::getManager()->getArchive());
        void *bck = JKRFileLoader::getGlbResource("pause5.bck", PauseManager::getManager()->getArchive());
        mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
        mScreen->setAnimation(mAnm);
        mAnm2 = (J2DAnmColor *)J2DAnmLoaderDataBase::load(
            JKRFileLoader::getGlbResource("pause5.bpk", PauseManager::getManager()->getArchive()));
        mScreen->setAnimation(mAnm2);
        for (int i = 0; i < 5; i++) {
            mSel[i].mPane = (J2DPicture *)mScreen->search(0x4c696e6531ULL + i);
            mSel[i].mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
            mSel[i].mPane->setAnimation(mSel[i].mAnm);
        }
        for (int i = 0; i < 10; i++) {
            J2DPane *pane = mScreen->search(0x6b725f61ULL + i);
#line 135
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mKr[i] = (J2DPicture *)pane;
            mPos[i].x = mKr[i]->getTranslateX();
            mPos[i].y = mKr[i]->getTranslateY();
            mKr[i]->hide();
        }
        for (int i = 0; i < 5; i++) {
            J2DPane *pane = mScreen->search(0x504d31ULL + i);
#line 145
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mPics[i] = (J2DPicture *)pane;
        }
        mDrawFunc = &Pause2D::setDrawVS;
        mCalcFunc = &Pause2D::selectorVS;
        break;
    }
    case TIME_ATTACK: {
        mScreen = new (heap, 0) J2DScreen();
        mScreen->set("pause5.blo", 0x40000, PauseManager::getManager()->getArchive());
        void *bck = JKRFileLoader::getGlbResource("pause5.bck", PauseManager::getManager()->getArchive());
        mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
        mScreen->setAnimation(mAnm);
        mAnm2 = (J2DAnmColor *)J2DAnmLoaderDataBase::load(
            JKRFileLoader::getGlbResource("pause5.bpk", PauseManager::getManager()->getArchive()));
        mScreen->setAnimation(mAnm2);
        for (int i = 0; i < 5; i++) {
            mSel[i].mPane = (J2DPicture *)mScreen->search(0x4c696e6531ULL + i);
            mSel[i].mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
            mSel[i].mPane->setAnimation(mSel[i].mAnm);
        }
        for (int i = 0; i < 10; i++) {
            J2DPane *pane = mScreen->search(0x6b725f61ULL + i);
#line 172
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mKr[i] = (J2DPicture *)pane;
            mPos[i].x = mKr[i]->getTranslateX();
            mPos[i].y = mKr[i]->getTranslateY();
            mKr[i]->hide();
        }
        for (int i = 0; i < 5; i++) {
            J2DPane *pane = mScreen->search(0x504d31ULL + i);
#line 182
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mPics[i] = (J2DPicture *)pane;
        }
        mDrawFunc = &Pause2D::setDrawTA;
        mCalcFunc = &Pause2D::selectorTA;
        break;
    }
    case BALLOON_BATTLE:
    case BOMB_BATTLE:
    case ESCAPE_BATTLE: {
        mScreen = new (heap, 0) J2DScreen();
        mScreen->set("pause6.blo", 0x40000, PauseManager::getManager()->getArchive());
        void *bck = JKRFileLoader::getGlbResource("pause6.bck", PauseManager::getManager()->getArchive());
        mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
        mScreen->setAnimation(mAnm);
        mAnm2 = (J2DAnmColor *)J2DAnmLoaderDataBase::load(
            JKRFileLoader::getGlbResource("pause6.bpk", PauseManager::getManager()->getArchive()));
        mScreen->setAnimation(mAnm2);
        for (int i = 0; i < 6; i++) {
            mSel[i].mPane = (J2DPicture *)mScreen->search(0x4c696e6531ULL + i);
            mSel[i].mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
            mSel[i].mPane->setAnimation(mSel[i].mAnm);
        }
        for (int i = 0; i < 12; i++) {
            J2DPane *pane = mScreen->search(0x6b725f61ULL + i);
#line 211
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mKr[i] = (J2DPicture *)pane;
            mPos[i].x = mKr[i]->getTranslateX();
            mPos[i].y = mKr[i]->getTranslateY();
            mKr[i]->hide();
        }
        for (int i = 0; i < 6; i++) {
            J2DPane *pane = mScreen->search(0x504d31ULL + i);
#line 221
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mPics[i] = (J2DPicture *)pane;
        }
        mPics[2]->changeTexture("Mozi_Change_Map.bti", 0);
        mPics[3]->changeTexture("Mozi_Change_Character.bti", 0);
        mPics[4]->changeTexture("Mozi_Change_MiniGame.bti", 0);
        mDrawFunc = &Pause2D::setDrawMG;
        mCalcFunc = &Pause2D::selectorMG;
        break;
    }
    default:
#line 233
        JUT_ASSERT(0);
        break;
    }

    mResultPic = (J2DPicture *)mScreen->search('kr');
    for (int i = 0; i < 2; i++) {
        J2DPane *pane = mScreen->search(0x7374617231ULL + i);
#line 243
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mCenterPics[i] = (J2DPicture *)pane;
    }

    init();
}

void Pause2D::init() {
    mStatus = PAUSE_0;
    _14 = -1;
    _18 = 0;
    for (int i = 0; i < 2; i++) {
        mCounters[i] = 0;
    }

    switch (RCMGetManager()->getRaceMode()) {
    case TIME_ATTACK:
    case GRAND_PRIX:
    case VERSUS_RACE:
    case BALLOON_BATTLE:
    case BOMB_BATTLE:
    case ESCAPE_BATTLE:
    case AWARD_DEMO:
    case STAFF_ROLL:
        mPauseSelector = 0;
        break;
    default:
#line 277
        JUT_ASSERT(0);
        break;
    }

    mDrawState = 0;
    mFrom[0] = mPos[0];
    mTo[0] = mPos[0];
    mFrom[1] = mPos[1];
    mTo[1] = mPos[1];
    mAnm->mCurrentFrame = 0.0f;
    mAnm2->mCurrentFrame = 0.0f;
    for (int i = 0; i < 6; i++) {
        mSel[i].mVel = 0.0f;
    }
    mScreen->animation();
    (this->*mDrawFunc)();
}

void Pause2D::draw() {
    if (mNode->mParam.mFlag & 1) {
        if (PauseManager::tstPause()) {
            mOrtho->setPort();
            mScreen->draw(0.0f, 0.0f, mOrtho);
        }
    }
}

void Pause2D::calc(KartGamePad *pad) {
    if (!(mNode->mParam.mFlag & 1))
        return;

    if (pad != nullptr && mStatus != PAUSE_4 && mDrawState == 0) {
        (this->*mCalcFunc)(pad);
    }

    switch (mStatus) {
    case PAUSE_0:
        break;
    case PAUSE_1:
        if (++_14 == 9)
            mStatus = PAUSE_2;
        break;
    case PAUSE_2:
        if (++_18 < 30)
            _14 = 9;
        else
            mStatus = PAUSE_3;
        break;
    case PAUSE_3:
        _14 = 9;
        break;
    case PAUSE_4:
        if (++_14 >= 20)
            mStatus = PAUSE_0;
        break;
    default:
#line 370
        JUT_ASSERT(0);
        break;
    }

    mAnm->setFrame((f32)_14);
    mAnm2->setFrame((f32)_14);
    mScreen->animation();

    for (int i = 0; i < 2; i++) {
        int t = ++mCounters[i];
        mCounters[i] = t % 50;
        mScales[i] = getScale(mCounters[i]);
    }

    (this->*mDrawFunc)();
}

void Pause2D::selectorGP(KartGamePad *pad) {
    if (pad->getRepeat() & JUTGamePad::MAINSTICK_UP) {
        switch (mPauseSelector) {
        case 0:
            mPauseSelector = 8;
            break;
        case 8:
            mPauseSelector = 0;
            break;
        default:
#line 405
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    } else if (pad->getRepeat() & JUTGamePad::MAINSTICK_DOWN) {
        switch (mPauseSelector) {
        case 0:
            mPauseSelector = 8;
            break;
        case 8:
            mPauseSelector = 0;
            break;
        default:
#line 419
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    }
}

void Pause2D::selectorVS(KartGamePad *pad) {
    if (pad->getRepeat() & JUTGamePad::MAINSTICK_UP) {
        switch (mPauseSelector) {
        case 0:
            mPauseSelector = 8;
            break;
        case 1:
            mPauseSelector = 0;
            break;
        case 2:
            mPauseSelector = 1;
            break;
        case 4:
            mPauseSelector = 2;
            break;
        case 8:
            mPauseSelector = 4;
            break;
        default:
#line 453
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    } else if (pad->getRepeat() & JUTGamePad::MAINSTICK_DOWN) {
        switch (mPauseSelector) {
        case 0:
            mPauseSelector = 1;
            break;
        case 1:
            mPauseSelector = 2;
            break;
        case 2:
            mPauseSelector = 4;
            break;
        case 4:
            mPauseSelector = 8;
            break;
        case 8:
            mPauseSelector = 0;
            break;
        default:
#line 476
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    }
}

void Pause2D::selectorTA(KartGamePad *pad) {
    if (pad->getRepeat() & JUTGamePad::MAINSTICK_UP) {
        switch (mPauseSelector) {
        case 0:
            mPauseSelector = 8;
            break;
        case 1:
            mPauseSelector = 0;
            break;
        case 2:
            mPauseSelector = 1;
            break;
        case 4:
            mPauseSelector = 2;
            break;
        case 8:
            mPauseSelector = 4;
            break;
        default:
#line 510
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    } else if (pad->getRepeat() & JUTGamePad::MAINSTICK_DOWN) {
        switch (mPauseSelector) {
        case 0:
            mPauseSelector = 1;
            break;
        case 1:
            mPauseSelector = 2;
            break;
        case 2:
            mPauseSelector = 4;
            break;
        case 4:
            mPauseSelector = 8;
            break;
        case 8:
            mPauseSelector = 0;
            break;
        default:
#line 533
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    }
}

void Pause2D::selectorMG(KartGamePad *pad) {
    if (pad->getRepeat() & JUTGamePad::MAINSTICK_UP) {
        switch (mPauseSelector) {
        case 0:
            mPauseSelector = 8;
            break;
        case 1:
            mPauseSelector = 0;
            break;
        case 3:
            mPauseSelector = 1;
            break;
        case 4:
            mPauseSelector = 3;
            break;
        case 7:
            mPauseSelector = 4;
            break;
        case 8:
            mPauseSelector = 6;
            break;
        default:
#line 570
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    } else if (pad->getRepeat() & JUTGamePad::MAINSTICK_DOWN) {
        switch (mPauseSelector) {
        case 0:
            mPauseSelector = 1;
            break;
        case 1:
            mPauseSelector = 3;
            break;
        case 3:
            mPauseSelector = 4;
            break;
        case 4:
            mPauseSelector = 6;
            break;
        case 6:
            mPauseSelector = 8;
            break;
        case 8:
            mPauseSelector = 0;
            break;
        default:
#line 596
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    }
}

void Pause2D::selectorCommon() {
    mCounters[0] = 0;
    mCounters[1] = 0;
    GameAudio::Main::getAudio()->startSystemSe(0x20000);
    mDrawState = 1;
}

void Pause2D::setDrawGP() {
    switch (mPauseSelector) {
    case 0:
        mPics[0]->updateScale(mScales[0]);
        mTo[0] = mPos[0];
        mTo[1] = mPos[1];
        if ((mSel[0].mVel += 1.0f) > 7.0f)
            mSel[0].mVel = 7.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        break;
    case 8:
        mPics[1]->updateScale(mScales[0]);
        mTo[0] = mPos[2];
        mTo[1] = mPos[3];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel += 1.0f) > 7.0f)
            mSel[1].mVel = 7.0f;
        break;
    default:
#line 643
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < 2; i++)
        mSel[i].mAnm->setFrame(mSel[i].mVel);

    setDrawCommon();
}

void Pause2D::setDrawVS() {
    switch (mPauseSelector) {
    case 0:
        mPics[0]->updateScale(mScales[0]);
        mTo[0] = mPos[0];
        mTo[1] = mPos[1];
        if ((mSel[0].mVel += 1.0f) > 7.0f)
            mSel[0].mVel = 7.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        break;
    case 1:
        mPics[1]->updateScale(mScales[0]);
        mTo[0] = mPos[2];
        mTo[1] = mPos[3];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel += 1.0f) > 7.0f)
            mSel[1].mVel = 7.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        break;
    case 2:
        mPics[2]->updateScale(mScales[0]);
        mTo[0] = mPos[4];
        mTo[1] = mPos[5];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel += 1.0f) > 7.0f)
            mSel[2].mVel = 7.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        break;
    case 4:
        mPics[3]->updateScale(mScales[0]);
        mTo[0] = mPos[6];
        mTo[1] = mPos[7];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel += 1.0f) > 7.0f)
            mSel[3].mVel = 7.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        break;
    case 8:
        mPics[4]->updateScale(mScales[0]);
        mTo[0] = mPos[8];
        mTo[1] = mPos[9];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel += 1.0f) > 7.0f)
            mSel[4].mVel = 7.0f;
        break;
    default:
#line 716
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < 5; i++)
        mSel[i].mAnm->setFrame(mSel[i].mVel);

    setDrawCommon();
}

void Pause2D::setDrawTA() {
    switch (mPauseSelector) {
    case 0:
        mPics[0]->updateScale(mScales[0]);
        mTo[0] = mPos[0];
        mTo[1] = mPos[1];
        if ((mSel[0].mVel += 1.0f) > 7.0f)
            mSel[0].mVel = 7.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        break;
    case 1:
        mPics[1]->updateScale(mScales[0]);
        mTo[0] = mPos[2];
        mTo[1] = mPos[3];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel += 1.0f) > 7.0f)
            mSel[1].mVel = 7.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        break;
    case 2:
        mPics[2]->updateScale(mScales[0]);
        mTo[0] = mPos[4];
        mTo[1] = mPos[5];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel += 1.0f) > 7.0f)
            mSel[2].mVel = 7.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        break;
    case 4:
        mPics[3]->updateScale(mScales[0]);
        mTo[0] = mPos[6];
        mTo[1] = mPos[7];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel += 1.0f) > 7.0f)
            mSel[3].mVel = 7.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        break;
    case 8:
        mPics[4]->updateScale(mScales[0]);
        mTo[0] = mPos[8];
        mTo[1] = mPos[9];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel += 1.0f) > 7.0f)
            mSel[4].mVel = 7.0f;
        break;
    default:
#line 788
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < 5; i++)
        mSel[i].mAnm->setFrame(mSel[i].mVel);

    setDrawCommon();
}

void Pause2D::setDrawMG() {
    switch (mPauseSelector) {
    case 0:
        mPics[0]->updateScale(mScales[0]);
        mTo[0] = mPos[0];
        mTo[1] = mPos[1];
        if ((mSel[0].mVel += 1.0f) > 7.0f)
            mSel[0].mVel = 7.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        if ((mSel[5].mVel -= 1.0f) < 0.0f)
            mSel[5].mVel = 0.0f;
        break;
    case 1:
        mPics[1]->updateScale(mScales[0]);
        mTo[0] = mPos[2];
        mTo[1] = mPos[3];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel += 1.0f) > 7.0f)
            mSel[1].mVel = 7.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        if ((mSel[5].mVel -= 1.0f) < 0.0f)
            mSel[5].mVel = 0.0f;
        break;
    case 3:
        mPics[2]->updateScale(mScales[0]);
        mTo[0] = mPos[4];
        mTo[1] = mPos[5];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel += 1.0f) > 7.0f)
            mSel[2].mVel = 7.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        if ((mSel[5].mVel -= 1.0f) < 0.0f)
            mSel[5].mVel = 0.0f;
        break;
    case 4:
        mPics[3]->updateScale(mScales[0]);
        mTo[0] = mPos[6];
        mTo[1] = mPos[7];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel += 1.0f) > 7.0f)
            mSel[3].mVel = 7.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        if ((mSel[5].mVel -= 1.0f) < 0.0f)
            mSel[5].mVel = 0.0f;
        break;
    case 6:
        mPics[4]->updateScale(mScales[0]);
        mTo[0] = mPos[8];
        mTo[1] = mPos[9];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel += 1.0f) > 7.0f)
            mSel[4].mVel = 7.0f;
        if ((mSel[5].mVel -= 1.0f) < 0.0f)
            mSel[5].mVel = 0.0f;
        break;
    case 8:
        mPics[5]->updateScale(mScales[0]);
        mTo[0] = mPos[10];
        mTo[1] = mPos[11];
        if ((mSel[0].mVel -= 1.0f) < 0.0f)
            mSel[0].mVel = 0.0f;
        if ((mSel[1].mVel -= 1.0f) < 0.0f)
            mSel[1].mVel = 0.0f;
        if ((mSel[2].mVel -= 1.0f) < 0.0f)
            mSel[2].mVel = 0.0f;
        if ((mSel[3].mVel -= 1.0f) < 0.0f)
            mSel[3].mVel = 0.0f;
        if ((mSel[4].mVel -= 1.0f) < 0.0f)
            mSel[4].mVel = 0.0f;
        if ((mSel[5].mVel += 1.0f) > 7.0f)
            mSel[5].mVel = 7.0f;
        break;
    default:
#line 880
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < 6; i++)
        mSel[i].mAnm->setFrame(mSel[i].mVel);

    setDrawCommon();
}

void Pause2D::setDrawCommon() {
    for (int i = 0; i < 2; i++) {
        if (mDrawState < 8) {
            mCur[i] = mFrom[i] + (mTo[i] - mFrom[i]) * (f32)mDrawState * 0.125f;
        } else {
            mFrom[i] = mTo[i];
            mCur[i] = mFrom[i];
        }
        mCenterPics[i]->setOffset(mCur[i].x + mResultPic->getTranslateX(), mCur[i].y + mResultPic->getTranslateY());
        mCenterPics[i]->updateScale(mScales[1]);
    }

    if (mDrawState != 0) {
        if (mDrawState < 8)
            mDrawState++;
        else
            mDrawState = 0;
    }
}

f32 Pause2D::getScale(int i) {
    int idx = (((i * 0xFFFF) / 50 + 0x7FFF) >> 5) & 0x7FF;
    f32 c = JMath::sincosTable_.mTable[idx].second;
    return 1.0f + 0.3f * (-0.5f * c + 0.5f);
}

int Pause2D::getPauseSelector() {
    return mPauseSelector;
}

Pause2D::PauseStatus Pause2D::getPauseStatus() {
    return mStatus;
}

void Pause2D::setPauseStatus(Pause2D::PauseStatus status) {
    mStatus = status;
}

void Pause2D::setPadText() {
    struct TextData {
        J2DTextBox *mPane;
        u64 mTag;
        int mMsgId;
    };

    TextData data[5] = {
        {nullptr, 0x544d473031ULL, 0},
        {nullptr, 0x544d473032ULL, 0},
        {nullptr, 0x544d4757ULL, 4},
        {nullptr, 0x544d475731ULL, 1},
        {nullptr, 0x544d475732ULL, 2},
    };
    J2DTextBox::TFontSize fontSize[5];
    char buf[64];

    for (int i = 0; i < 64; i++) {
        buf[i] = ' ';
    }
    buf[63] = '\0';

    for (int i = 0; i < 5; i++) {
        J2DPane *pane = mScreen->search(data[i].mTag);
#line 1001
        JUT_ASSERT(pane->getTypeID() == J2DPane_TextBox);
        data[i].mPane = (J2DTextBox *)pane;
        data[i].mPane->setFont(FontMgr::mspResFont);
        data[i].mPane->setString(buf);
        data[i].mPane->getFontSize(fontSize[i]);
    }

    for (int i = 0; i < 5; i++) {
        void *colorRes = ResMgr::getPtr(ResMgr::mcColor);
        void *raceRes = ResMgr::getPtr(ResMgr::mcRace);
        int msgId = data[i].mMsgId;
        char *message = ReadPrintMessage::getMessage(raceRes, msgId);
        JUTColor color = ReadPrintMessage::getColor(colorRes, ReadPrintMessage::getDefaultColor(raceRes));
        data[i].mPane->setFontColor(color, color);
        data[i].mPane->setFontSize(fontSize[i].x * ReadPrintMessage::getRate(raceRes, msgId) / 100.0f,
                                   fontSize[i].y * ReadPrintMessage::getRate(raceRes, msgId) / 100.0f);
        data[i].mPane->setCharSpace(ReadPrintMessage::getCharSpace(raceRes, msgId));
        J2DTextBox::TFontSize fs;
        data[i].mPane->getFontSize(fs);
        char text[64];
        ReadPrintMessage::tagCnv(message, colorRes, fs, sizeof(text), text);
        strncpy(data[i].mPane->getStringPtr(), text, sizeof(text));
    }

    J2DPane *single = mScreen->search(0x4e53696e676c65ULL);
    J2DPane *tag = mScreen->search('NTag');
    if (RCMGetManager()->getKartInfo(0)->isTagMode()) {
        single->hide();
        tag->show();
    } else {
        single->show();
        tag->hide();
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
