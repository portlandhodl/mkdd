#include "Kameda/SceneLanEntry.h"
#include "Kameda/Scene.h"
#include "Kameda/ReadPrintMessage.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/J2D/J2DPrint.h"
#include "JSystem/JFramework/JFWDisplay.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JUtility/JUTFader.h"
#include "Osako/AppMgr.h"
#include "Osako/BBAMgr.h"
#include "Osako/LANPlayInfo.h"
#include "Osako/NetGameMgr.h"
#include "Osako/NetGateApp.h"
#include "Osako/ResMgr.h"
#include "Osako/SequenceApp.h"
#include "Osako/kartPad.h"
#include "Osako/system.h"

#include "mathHelper.h"

extern "C" char *SOInetNtoA(SOInAddr);

void SceneLanEntry::Window::setup(JKRArchive *archive) {
    mMessage = MSG_NONE;
    mState = 0;
    mWindowFrame = 0;
    mScreen.setPriority("Link.blo", 0x2020000, archive);
    void *bck = JKRFileLoader::getGlbResource("Link.bck", archive);
    mScreenAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mWindowAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mScreen.setAnimation(mScreenAnm);
    mScreen.search('W1')->setAnimation(mWindowAnm);
    J2DTextBox *pPane = (J2DTextBox *)mScreen.search('T1');
#line 73
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_TextBox);
    mTextBox = pPane;
    mTextBox->setString(256, "");
    mTextBox->getFontSize(mFontSize);
}

void SceneLanEntry::Window::open(Message msg) {
    mMessage = msg;
    switch (mState) {
    case 0:
    case 1:
        mState = 2;
        break;
    case 2:
    case 3:
        mState = 1;
        break;
    }
}

u16 SceneLanEntry::Window::getBmgMessageID(Message msg) {
    int id = 0;
    switch (msg) {
    case MSG_CONNECTING:
        id = 0x50001;
        break;
    case MSG_FIND:
        id = 0x50009;
        break;
    case MSG_START:
        id = 0x5000d;
        break;
    case MSG_DISCONNECTING:
        id = 0x50003;
        break;
    case MSG_NOT_CONNECT_00:
        id = 0x5000a;
        break;
    case MSG_NOT_CONNECT_03:
        id = 0x5000c;
        break;
    case MSG_NOT_CONNECT_01:
        id = 0x5000b;
        break;
    case MSG_ERROR_02:
        id = 0x50007;
        break;
    case MSG_DISCONNECTING_ERROR:
        id = 0x50004;
        break;
    }
    return id;
}

void SceneLanEntry::Window::calc() {
    mScreenAnm->mCurrentFrame = mScreenFrame;
    mWindowAnm->mCurrentFrame = mWindowFrame;

    switch (mState) {
    case 2:
        if (mMessage != MSG_NONE) {
            void *bmg = ResMgr::getPtr(ResMgr::mcLan);
            u16 id = getBmgMessageID((Message)mMessage);
            char *msg = ReadPrintMessage::getMessage(bmg, id);
            f32 x = mFontSize.x * ReadPrintMessage::getRate(bmg, id) / 100.0f;
            mTextBox->setFontSize(x, mFontSize.y * ReadPrintMessage::getRate(bmg, id) / 100.0f);
            J2DTextBox::TFontSize fontSize;
            mTextBox->getFontSize(fontSize);
            id = *(u16 *)&mTextBox->mStringLength;
            ReadPrintMessage::tagCnv(msg, ResMgr::getPtr(ResMgr::mcColor), fontSize, id, mTextBox->getStringPtr());
            mTextBox->show();
            mMessage = MSG_NONE;
        }
    case 3:
        if (++mWindowFrame >= 10) {
            mState = 2;
            mWindowFrame = 9;
        }
        break;
    case 1:
        mWindowFrame--;
        if (mMessage != MSG_NONE && mWindowFrame < 5) {
            mState = 2;
        } else if (mWindowFrame <= 0) {
            mState = 0;
        }
        break;
    }

    if (mScreenFrame < 59) {
        mScreenFrame++;
    }
    mScreen.animation();
}

SceneLanEntry::SceneLanEntry(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap), mTimer(0), mTimerState(0) {
    mState = 1;
    mBackground.setup(archive);
    mWindow.setup(archive);
}

SceneLanEntry::Window::~Window() {}

static const char *scpaMsgLabel[] = {
    "LA_CONNECTING",
    "LA_FIND",
    "LA_START",
    "LA_DISCONNECTING",
    "LA_DISCONNECTING_ERROR",
    "LA_NOT_CONNECT_00",
    "LA_NOT_CONNECT_03",
    "LA_NOT_CONNECT_01",
    "LA_ERROR_02"
};

void SceneLanEntry::init() {
    mWindow.mState = 0;
    mWindow.mWindowFrame = 0;
    mWindow.mScreenFrame = 0;
    mState = 0;
    mErrorMsg = 0;
    mLinkNum = 0;
    mRemainTime = 0;
    mDebugDraw = false;
    mDebugMsg = 1;
    mDebugIP = false;
    mIPCursor = 0;
    mIP[0] = 0;
    mIP[1] = 0;
    mIP[2] = 0;
    mIP[3] = 0;
    mIPMode = 0;
}

SceneLanEntry::~SceneLanEntry() {}

void SceneLanEntry::draw() {
    mBackground.mScreen.draw(0.0f, 0.0f, System::getJ2DOrtho());
    mWindow.mScreen.draw(0.0f, 0.0f, System::getJ2DOrtho());

    if (mState == 11) {
        System::getJ2DOrtho()->setPort();
        System::getJ2DPrint()->initiate();
        System::getJ2DPrint()->print(34.0f, 32.0f, scpaMsgLabel[mDebugMsg - 1]);
    } else if (mDebugDraw) {
        debugDraw();
    }
}

static inline f32 getDHCPTime() { return BBAMgr::mspBBAMgr->mTimer0; }
static inline f32 getAutoIPTime() { return BBAMgr::mspBBAMgr->mTimer1; }

void SceneLanEntry::debugDraw() {
    NetGameMgr *mgr = NetGameMgr::ptr();
    J2DPrint *print = System::getJ2DPrint();
    int state = mgr->getState();

    System::getJ2DOrtho()->setPort();
    print->initiate();
    print->print(34.0f, 32.0f, "LAN Entry");
    print->print(34.0f, 48.0f, "LAN");
    print->print(136.0f, 48.0f, "%s", state >= 2 ? "OK" : "Checking");

    int y = 48;
    y += 16;
    if (state >= 2 && state <= 16) {
        print->print(34.0f, y, "DHCP");
        if (state == 2) {
            print->print(136.0f, y, "%.2f", BBAMgr::mspBBAMgr->mTimer0);
        } else {
            print->print(136.0f, y, "%s:%.2f", (BBAMgr::mspBBAMgr->mUnk10 & 1) ? "NG" : "OK", getDHCPTime());
        }
        y += 16;
    }
    if (state >= 4 && state <= 16) {
        print->print(34.0f, y, "Auto-IP");
        if (state == 4) {
            print->print(136.0f, y, "%.2f", BBAMgr::mspBBAMgr->mTimer1);
        } else {
            u32 flag = BBAMgr::mspBBAMgr->mUnk10;
            print->print(136.0f, y, "%s:%.2f", (flag & 1) ? ((flag & 2) ? "NG" : "OK") : "Pass", getAutoIPTime());
        }
        y += 16;
    }
    if (state >= 6 && state <= 16) {
        print->print(34.0f, y, "IPAddr");
        print->print(136.0f, y, "%s", SOInetNtoA(*(SOInAddr *)&mgr->mPadConnArray[0].mAddr));
        y += 16;
    }
    if (state >= 7 && state <= 16) {
        print->print(34.0f, y, "M-Search");
        if (state == 7) {
            print->print(136.0f, y, "%.2f", mSearchTime);
        } else {
            print->print(136.0f, y, "Done:%.2f", mSearchTime);
        }
        y += 16;
    }

    const char *gcState[] = {"", "--", "->", "<>", "=="};
    if (state >= 7 && state <= 16) {
        for (int i = 0; i < 16; i++) {
            print->print(376.0f, i * 16 + 32, "%2d:%s", i, SOInetNtoA(*(SOInAddr *)&mgr->mGCInfo[i].mAddr));
            print->print(336.0f, i * 16 + 32, gcState[mgr->mGCInfo[i].mState]);
        }
    }
    if (state >= 8 && state <= 16) {
        print->print(34.0f, y, "Link");
        print->print(136.0f, y, "%d", mgr->mLinkNum + 1);
        if (state >= 9) {
            print->print(208.0f, y, "OK");
        }
        y += 16;
    }
    if (state >= 11 && state <= 16) {
        print->print(34.0f, y, "Sync");
        y += 16;
    }
    if (state >= 12 && state <= 16) {
        for (u8 i = 0; i < mgr->mNetworkCubes; i++) {
            print->print(66.0f, y, "%d:%s", i, SOInetNtoA(*(SOInAddr *)&mgr->mPadConn[i]->mAddr));
            y += 16;
        }
    }
    if (state >= 20) {
        print->print(34.0f, y, "Disconnecting...%.2f", getTime());
    }

    if (mDebugIP) {
        const char *mode[] = {"set", "bye"};
        print->print(160.0f, 324.0f, "IP:%3d.%3d.%3d.%3d %s", mIP[0], mIP[1], mIP[2], mIP[3], mode[mIPMode]);
        if (mIPCursor < 12) {
            print->print((mIPCursor / 3) * 12 + (mIPCursor * 12 + 196), 326.0f, "_");
        } else {
            print->print(388.0f, 326.0f, "___");
        }
    }
}

void SceneLanEntry::calc() {
    if (mTimerState == 1) {
        mTimer++;
    }

    switch (mState) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 11:
        if (gGamePad1P.testTrigger(JUTGamePad::B)) {
            NetGameMgr::ptr()->end(false);
            mState = 6;
            resetTimer();
            GameAudio::Main::getAudio()->startSystemSe(0x20047);
            mWindow.open(Window::MSG_DISCONNECTING);
        }
        break;
    }

    if (gGamePad1P.testTrigger(JUTGamePad::Y)) {
        mDebugDraw = !mDebugDraw;
    }

    switch (mState) {
    case 0:
        start();
        break;
    case 1:
        waitLink();
        break;
    case 2:
        waitIPGet();
        break;
    case 3:
        waitSearch();
        break;
    case 4:
        waitSetup();
        break;
    case 5:
        waitSleep();
        break;
    case 6:
        waitEnd();
        break;
    case 7:
        waitButton();
        break;
    case 8:
        nextScene();
        break;
    case 9:
        backScene();
        break;
    case 11:
        debug();
        break;
    }

    mBackground.calc();
    mWindow.calc();
}

void SceneLanEntry::start() {
    GameAudio::Main::getAudio()->startSequenceBgm(0x1000001);
    NetGameMgr::ptr()->start();
    if (NetGameMgr::ptr()->getState() == 1) {
        mState = 1;
        resetTimer();
    } else {
        mState = 2;
    }

    if (System::getDisplay()->getFader())
        System::getDisplay()->getFader()->startFadeIn(15);

    mWindow.open(Window::MSG_CONNECTING);
}

void SceneLanEntry::waitLink() {
    if (NetGameMgr::ptr()->getState() != 1) {
        mState = 2;
    } else if (getTime() > 20.0f) {
        error(Window::MSG_NOT_CONNECT_00);
    }
}

void SceneLanEntry::waitIPGet() {
    switch (NetGameMgr::ptr()->getState()) {
    case 6:
        NetGameMgr::ptr()->startSearch();
        mState = 3;
        mSearchTime = 0.0f;
        resetTimer();
        mWindow.open(Window::MSG_FIND);
        GameAudio::Main::getAudio()->startSystemSe(0x20047);
        break;
    case 3:
    case 5:
        NetGameMgr::ptr()->end(false);
        if (NetGameMgr::ptr()->getState() == 3) {
            error(Window::MSG_NOT_CONNECT_03);
        } else {
            error(Window::MSG_NOT_CONNECT_01);
        }
        break;
    }
}

void SceneLanEntry::countDownSearchTimer() {
    int remain = 180 - (int)getTime();
    if (mRemainTime != remain) {
        mRemainTime = remain;
        ReadPrintMessage::mNum[0] = mRemainTime;
        mWindow.mMessage = Window::MSG_FIND;
    }
}

void SceneLanEntry::waitSearch() {
    mSearchTime = getTime();
    if (NetGameMgr::ptr()->getState() != 7) {
        mState = 4;
    } else {
        countDownSearchTimer();
        if (getTime() > 180.0f) {
            error(Window::MSG_NOT_CONNECT_00);
        }
    }
}

static inline int ipUp(int v, int cursor) {
    int add[] = {1, 10, 100};
    v += add[2 - cursor % 3];
    if (v > 255) {
        v = 255;
    }
    return v;
}
static inline int ipDown(int v, int cursor) {
    int sub[] = {1, 10, 100};
    v -= sub[2 - cursor % 3];
    if (v < 0) {
        v = 0;
    }
    return v;
}

void SceneLanEntry::waitSetup() {
    switch (NetGameMgr::ptr()->getState()) {
    case 9:
        if (mLinkNum != NetGameMgr::ptr()->mLinkNum + 1) {
            mLinkNum = NetGameMgr::ptr()->mLinkNum + 1;
            ReadPrintMessage::mNum[0] = mLinkNum;
            mWindow.open(Window::MSG_START);
            GameAudio::Main::getAudio()->startSystemSe(0x20047);
        }
        if (!mDebugIP && gGamePad1P.testTrigger(JUTGamePad::START | JUTGamePad::A)) {
            if (NetGameMgr::ptr()->finishLink()) {
                resetTimer();
            } else {
                error(Window::MSG_NOT_CONNECT_00);
            }
        }
        break;
    case 11:
        if (getTime() > 5.0f) {
            error(Window::MSG_ERROR_02);
        }
        break;
    case 13:
        GameAudio::Main::getAudio()->startSystemSe(0x20003);
        NetGameMgr::ptr()->sleep();
        mState = 5;
        resetTimer();
        break;
    }

    if (gGamePad1P.testTrigger(JUTGamePad::X)) {
        mDebugIP = !mDebugIP;
    }

    if (mDebugIP) {
        if (gGamePad1P.testRepeat(JUTGamePad::MAINSTICK_UP)) {
            if (mIPCursor < 12) {
                mIP[mIPCursor / 3] = ipUp(mIP[mIPCursor / 3], mIPCursor);
            } else {
                mIPMode = (mIPMode + 1) & 1;
            }
        } else if (gGamePad1P.testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
            if (mIPCursor < 12) {
                mIP[mIPCursor / 3] = ipDown(mIP[mIPCursor / 3], mIPCursor);
            } else {
                mIPMode = (mIPMode + 1) & 1;
            }
        }

        if (gGamePad1P.testRepeat(JUTGamePad::MAINSTICK_LEFT)) {
            if (--mIPCursor < 0) {
                mIPCursor = 0;
            }
        } else if (gGamePad1P.testRepeat(JUTGamePad::MAINSTICK_RIGHT)) {
            if (++mIPCursor > 12) {
                mIPCursor = 11;
            }
        }

        if (mIPCursor == 12 && gGamePad1P.testTrigger(JUTGamePad::A)) {
            SOInAddr addr;
            addr.mAddr = (mIP[0] << 24) | (mIP[1] << 16) | (mIP[2] << 8) | mIP[3];
            switch (mIPMode) {
            case 0:
                NetGameMgr::ptr()->appendGCInfo(&addr);
                break;
            case 1:
                NetGameMgr::ptr()->removeGCInfo(&addr);
                break;
            }
        }
    }
}

void SceneLanEntry::waitSleep() {
    if (NetGameMgr::ptr()->getState() == 15) {
        mState = 8;
        if (System::getDisplay()->getFader())
            System::getDisplay()->getFader()->startFadeOut(15);
    } else if (getTime() > 10.0f) {
        error(Window::MSG_NOT_CONNECT_00);
    }
}

void SceneLanEntry::waitEnd() {
    if (NetGameMgr::ptr()->getState() != 20 && getTime() > 2.0f) {
        if (mErrorMsg == 0) {
            mState = 9;
            if (System::getDisplay()->getFader())
                System::getDisplay()->getFader()->startFadeOut(15);
        } else {
            mState = 7;
            GameAudio::Main::getAudio()->startSystemSe(0x20047);
            mWindow.open((Window::Message)mErrorMsg);
            resetTimer();
        }
    }
}

void SceneLanEntry::waitButton() {
    if (gGamePad1P.testTrigger(JUTGamePad::A | JUTGamePad::B) && getTime() > 2.0f) {
        mState = 9;
        if (System::getDisplay()->getFader())
            System::getDisplay()->getFader()->startFadeOut(15);
    }
}

void SceneLanEntry::nextScene() {
    if (System::getDisplay()->getFader()->mStatus == JUTFader::Status_Out) {
        gLANPlayInfo.init();
        AppMgr::deleteCurrentApp();
        NetGateApp::call(false);
    }
}

void SceneLanEntry::backScene() {
    if (SequenceApp::ptr()->ready(Scene::SCENE_TITLE)) {
        if (System::getDisplay()->getFader()->mStatus == JUTFader::Status_Out) {
            SequenceApp::ptr()->setNextScene(Scene::SCENE_TITLE);
        }
    }
}

void SceneLanEntry::error(Window::Message msg) {
    NetGameMgr::ptr()->end(false);
    mState = 6;
    mErrorMsg = msg;
    resetTimer();
    GameAudio::Main::getAudio()->startSystemSe(0x20047);
    mWindow.open(Window::MSG_DISCONNECTING_ERROR);
}

void SceneLanEntry::debug() {
    if (gGamePad1P.testRepeat(JUTGamePad::MAINSTICK_UP)) {
        if (--mDebugMsg <= 0) {
            mDebugMsg = 9;
        }
        GameAudio::Main::getAudio()->startSystemSe(0x20047);
        mWindow.open((Window::Message)mDebugMsg);
    } else if (gGamePad1P.testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
        if (++mDebugMsg >= 10) {
            mDebugMsg = 1;
        }
        GameAudio::Main::getAudio()->startSystemSe(0x20047);
        mWindow.open((Window::Message)mDebugMsg);
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
