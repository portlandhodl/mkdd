#include "Kameda/SceneTitle.h"
#include "Kameda/Scene.h"
#include "Kameda/SceneCourseSelect.h"
#include "Kameda/SequenceInfo.h"
#include "Kameda/PrintMemoryCard.h"
#include "Kaneshige/HioMgr.h"
#include "Kaneshige/RaceInfo.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/J2D/J2DScreen.h"
#include "JSystem/J2D/J2DPane.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "JSystem/JFramework/JFWDisplay.h"
#include "JSystem/JUtility/JUTFader.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Osako/AppMgr.h"
#include "Osako/BBAMgr.h"
#include "Osako/CardAgent.h"
#include "Osako/RaceApp.h"
#include "Osako/ResMgr.h"
#include "Osako/SequenceApp.h"
#include "Osako/kartPad.h"
#include "Osako/system.h"
#include "dolphin/os.h"
#include "mathHelper.h"

u8 SceneTitleParam::mDemoType;
int SceneTitle::mDemoFlag = 1;

const u64 SceneTitle::mTitleBckBrkTag[] = {
    'TtlL1', 'TtlL2', 'TtlL3', 'TtlL4', 
    'TtlR1', 'TtlR2', 'TtlR3', 'TtlR4', 
    'TtlM1', 'TtlM2', 'TtlM3', 'TtlM4', 
    'TM2', 
};

SceneTitleParam::SceneTitleParam() {}

SceneTitle::SceneTitle(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap) {
    SceneTitleParam *param = new (heap, 0) SceneTitleParam();
    mHioNode = new (heap, 0) HioNode("シーンタイトル", param, 0, 0);
    mPrintMemoryCard = new (heap, 0) PrintMemoryCard(heap);
    mScreen = new (heap, 0) J2DScreen();
    mScreen->set("Title.blo", 0x40000, mArchive);

    SequenceApp::ptr()->ready(SCENE_MENU);
    SequenceApp::ptr()->ready(SCENE_OPTION);
    SequenceApp::ptr()->ready(SCENE_RECORD);

    for (int i = 0; i < 13; i++) {
        mBck[i].mPane = mScreen->search(mTitleBckBrkTag[i]);
        mBck[i].mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("Title.bck", mArchive));
        mBck[i].mPane->setAnimation(mBck[i].mAnm);
        mBck[i].mPane->hide();
    }

    mBtk.mPane = mScreen->search('TM2');
    mBtk.mAnm = (J2DAnmTextureSRTKey *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("Title.btk", mArchive));
    mBtk.mAnm->searchUpdateMaterialID(mScreen);
    mScreen->setAnimation(mBtk.mAnm);

    for (int i = 0; i < 13; i++) {
        mBrk[i].mPane = mScreen->search(mTitleBckBrkTag[i]);
        mBrk[i].mAnm = (J2DAnmTevRegKey *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("Title.brk", mArchive));
        mBrk[i].mAnm->searchUpdateMaterialID(mScreen);
        mBrk[i].mPane->setAnimation(mBrk[i].mAnm);
    }

    mState = 0;
    mPrevState = 0;
    mNextScene = SCENE_MAX;
}

void SceneTitle::init() {
    mScreen->search('root')->show();

    gSequenceInfo._28 = 0;
    if (gRaceInfo.mWaitDemoResult != 1) {
        GetGameAudioMain()->startSequenceBgm(0x1000000);
    } else if (GetGameAudioMain()->getPlayingSequenceID() != 0x1000000) {
        GetGameAudioMain()->startSequenceBgm(0x1000000);
    }

    mState = 0;
    mPrevState = 0;
    mIsPrintCard = false;
    mTimer = 0;

    for (int i = 0; i < 13; i++) {
        mBck[i].mFrame = 0.0f;
        mBck[i].mPane->hide();
    }
    mBtk.mFrame = 0.0f;
    mBtk.mPane->show();
    for (int i = 0; i < 13; i++) {
        mBrk[i].mFrame = 0.0f;
    }

    System::getDisplay()->startFadeIn(15);
    mPrintMemoryCard->reset();
    mPrintMemoryCard->init(PrintMemoryCard::mcMsg40);
}

SceneTitle::~SceneTitle() {
    delete mScreen;
}

void SceneTitle::draw() {
    mOrtho->setPort();
    mScreen->draw(0.0f, 0.0f, mOrtho);
    if (mIsPrintCard)
        mPrintMemoryCard->draw();
}

void SceneTitle::calc() {
    switch (mState) {
    case 0:
        fadeIn();
        break;
    case 1:
        title();
        break;
    case 2:
        demo();
        break;
    case 3:
        card();
        break;
    case 4:
        titleMenu();
        break;
    case 5:
        nextScene();
        break;
    default:
#line 241
        JUT_ASSERT(0);
        break;
    }

    if ((mBtk.mFrame += 1.0f) >= 240.0f)
        mBtk.mFrame = 0.0f;
    if ((mBrk[12].mFrame += 1.0f) >= 240.0f)
        mBrk[12].mFrame = 0.0f;

    for (int i = 0; i < 13; i++)
        mBck[i].mAnm->setFrame(mBck[i].mFrame);
    mBtk.mAnm->setFrame(mBtk.mFrame);
    for (int i = 0; i < 13; i++)
        mBrk[i].mAnm->setFrame(mBrk[i].mFrame);

    mScreen->animation();
}

void SceneTitle::fadeIn() {
    if (System::getDisplay()->getFader()->getStatus() == 1 && GetGameAudioMain()->isWaveLoaded(GameAudio::Main::STREAM3)) {
        mPrevState = 0;
        mState = 1;
        mTimer = 0;
    }
}

void SceneTitle::title() {
    if (++mTimer >= 900) {
        mState = 2;
        int course = 0;
        mTimer = 0;
        switch (SceneTitleParam::mDemoType) {
        case 0:
            course = (u32)OSGetTime() % 12;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            course = SceneTitleParam::mDemoType - 1;
            break;
        default:
#line 305
            JUT_ASSERT(0);
            break;
        }

        if (mDemoFlag != 0) {
            if (mDemoFlag == 3)
                gRaceInfo.settingForWaitDemo(true);
            else
                gRaceInfo.settingForWaitDemo(false);

            if (gRaceInfo.getConsoleNumber() < 2)
                ResMgr::loadCourseData(SceneCourseSelect::mCourseData[course / 4][course % 4].courseID, ResMgr::mcOrder_2);
            else
                ResMgr::loadCourseData(SceneCourseSelect::mCourseData[course / 4][course % 4].courseID, ResMgr::mcOrder_1);
        } else {
            GetGameAudioMain()->fadeOutAll(120);
        }
    } else if (gGamePad1P.mButtons.mTrigger & 0x1100) {
        mPrevState = 1;
        if (!(CardAgent::msFlags & 2)) {
            mState = 3;
            CardAgent::ask(CardAgent::mcCommand2, 0);
        } else {
            GetGameAudioMain()->startSystemSe(0x20003);
            mState = 4;
            mCursor = 0;
        }
    }
}

void SceneTitle::card() {
    if (CardAgent::getState() == 0) {
        if (CardAgent::getPrintMemoryCard()->_d == 0)
            GetGameAudioMain()->startSystemSe(0x20003);
        CardAgent::getPrintMemoryCard()->setStuff(0, 0, 0);
        mState = 4;
        mCursor = 0;
    }
}

void SceneTitle::titleMenu() {
    if (mIsPrintCard) {
        mPrintMemoryCard->calc();
        switch (mPrintMemoryCard->get_14Thing()) {
        case 0:
            mIsPrintCard = false;
            mTimer = 0;
            mPrevState = 4;
            mState = 5;
            mNextScene = SCENE_LAN_ENTRY;
            GetGameAudioMain()->fadeOutAll(15);
            mPrintMemoryCard->setStuff(0, 0, 0);
            break;
        case 1:
            mIsPrintCard = false;
            mPrintMemoryCard->init(PrintMemoryCard::mcMsg40);
            mPrintMemoryCard->setStuff(0, 0, 0);
            break;
        }
        return;
    }

    switch (BBAMgr::mspBBAMgr->getState()) {
    case 1:
    case 2:
        mIsLanEnable = 1;
        break;
    default:
        mIsLanEnable = 0;
        break;
    }

    if (gGamePad1P.mButtons.mRepeat & 0x08000000) {
        GetGameAudioMain()->startSystemSe(0x20000);
        if (--mCursor < 0)
            mCursor = 3;
        if (mIsLanEnable != 1 && mCursor == 3) {
            if (--mCursor < 0)
                mCursor = 3;
        }
    } else if (gGamePad1P.mButtons.mRepeat & 0x04000000) {
        GetGameAudioMain()->startSystemSe(0x20000);
        if (++mCursor >= 4)
            mCursor = 0;
        if (mIsLanEnable != 1 && mCursor == 3) {
            if (++mCursor >= 4)
                mCursor = 0;
        }
    }

    if (gGamePad1P.mButtons.mTrigger & 0x1100) {
        switch (mCursor) {
        case 0:
            mTimer = 0;
            mPrevState = 4;
            mState = 5;
            GetGameAudioMain()->fadeOutAll(15);
            GetGameAudioMain()->startSystemSe(0x20001);
            mNextScene = SCENE_MENU;
            break;
        case 2:
            mTimer = 0;
            mPrevState = 4;
            mState = 5;
            GetGameAudioMain()->fadeOutAll(15);
            GetGameAudioMain()->startSystemSe(0x20002);
            mNextScene = SCENE_OPTION;
            break;
        case 1:
            mTimer = 0;
            mPrevState = 4;
            mState = 5;
            GetGameAudioMain()->fadeOutAll(15);
            GetGameAudioMain()->startSystemSe(0x20002);
            mNextScene = SCENE_RECORD;
            break;
        case 3:
            mPrintMemoryCard->init(PrintMemoryCard::mcTooManyFilesContinue);
            mPrintMemoryCard->calc();
            mIsPrintCard = true;
            break;
        }
    }
    titleMenuAnm();
}

void SceneTitle::nextScene() {
    if (mTimer == 0) {
        JUtility::TColor color(0, 0, 0, 255);
        System::getDisplay()->getFader()->setColor(color);
        System::getDisplay()->startFadeOut(15);
    }
    if (mTimer++ > 15) {
        if (SequenceApp::ptr()->ready(mNextScene)) {
            gSequenceInfo.init();
            gRaceInfo.reset();
            SequenceApp::ptr()->setNextScene(mNextScene);
        }
    }
    titleMenuAnm();
}

void SceneTitle::demo() {
    if (mDemoFlag != 0) {
        if (mTimer == 0) {
            JUtility::TColor color(0, 0, 0, 255);
            System::getDisplay()->getFader()->setColor(color);
            System::getDisplay()->startFadeOut(15);
        }
        if (mTimer == 15)
            mScreen->search('root')->hide();
        if (mTimer++ > 15) {
            if (ResMgr::isFinishLoadingArc(ResMgr::mcArcCourse)) {
                if (++mDemoFlag > 3)
                    mDemoFlag = 0;
                AppMgr::msRequest |= 1;
                RaceApp::call();
            }
        }
    } else {
        if (mTimer == 0) {
            JUtility::TColor color(0, 0, 0, 255);
            System::getDisplay()->getFader()->setColor(color);
            System::getDisplay()->startFadeOut(120);
        }
        if (mTimer == 120)
            mScreen->search('root')->hide();
        if (mTimer++ > 120) {
            mDemoFlag = 1;
            if (AppMgr::msGameApp == AppMgr::mcSequence) {
                AppMgr::msRequest |= REQUEST_OPENING;
                AppMgr::msRequest |= REQUEST_TITLE;
            }
        }
    }
}

void SceneTitle::titleMenuAnm() {
    for (int i = 0; i < 13; i++)
        mBck[i].mPane->show();
    mBtk.mPane->hide();
    if (mIsLanEnable != 1) {
        mBck[3].mPane->hide();
        mBck[7].mPane->hide();
        mBck[11].mPane->hide();
    }

    for (int i = 0; i < 4; i++) {
        if (i == mCursor) {
            if ((mBck[i].mFrame += 1.0f) > 89.0f)
                mBck[i].mFrame = 89.0f;
            if ((mBck[i + 4].mFrame += 1.0f) > 89.0f)
                mBck[i + 4].mFrame = 89.0f;
            if ((mBck[i + 8].mFrame += 1.0f) > 4.0f)
                mBck[i + 8].mFrame = 4.0f;
            if ((mBrk[i].mFrame += 1.0f) > 89.0f)
                mBrk[i].mFrame = 89.0f;
            if ((mBrk[i + 4].mFrame += 1.0f) > 89.0f)
                mBrk[i + 4].mFrame = 89.0f;
            mBrk[i + 8].mFrame = 1.0f;
        } else {
            mBck[i].mFrame = 0.0f;
            mBck[i + 4].mFrame = 0.0f;
            if ((mBck[i + 8].mFrame -= 1.0f) < 0.0f)
                mBck[i + 8].mFrame = 0.0f;
            mBrk[i].mFrame = 0.0f;
            mBrk[i + 4].mFrame = 0.0f;
            mBrk[i + 8].mFrame = 0.0f;
        }
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
