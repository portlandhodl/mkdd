#include "Kameda/PauseManager.h"
#include "Kameda/Goal2D.h"
#include "Kameda/J2DManager.h"
#include "Kameda/Pause2D.h"
#include "Kameda/PrintMemoryCard.h"
#include "Kameda/Result2D.h"
#include "Kameda/SequenceInfo.h"
#include "Kameda/WipeManager.h"
#include "Kaneshige/RaceMgr.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Osako/NetGameMgr.h"
#include "Osako/RaceApp.h"
#include "Osako/ResMgr.h"
#include "Osako/system.h"
#include "mathHelper.h"

PauseManager *PauseManager::mThis;    // 0x80416298
bool PauseManager::mIsPause;
int PauseManager::mPauseSelector;
int PauseManager::mPauseChoice;
int PauseManager::mPauseNextChoice;
KartGamePad *PauseManager::mPausePad;

PauseManager::PauseManager(JKRHeap *heap) {
    if (heap == nullptr)
        heap = JKRHeap::getCurrentHeap();

    mArchive = ResMgr::getArchive(ResMgr::mcArcMRAMLoc);
    mOrtho = System::getJ2DOrtho();
    mPrintMemoryCard = new (heap, 0) PrintMemoryCard(heap);
    mThis = this;
    _0 = NetGameMgr::ptr()->getState() != 0;
    _12 = false;
    mExec = false;
    mPause2D = new (heap, 0) Pause2D(heap);
    mResult2D = new (heap, 0) Result2D(heap);
    _11 = true;

    for (int i = 0; i < RaceMgr::getManager()->getKartNumber(); i++) {
        for (int j = 0; j < 2; j++) {
            if (gSequenceInfo.getPad2Player(gRaceInfo.getKartInfo(i)->getPad(j)) != -1)
                _11 = false;
        }
    }
    reset();
}

PauseManager::~PauseManager() {
    delete mPause2D;
    delete mResult2D;
}

void PauseManager::reset() {
    _12 = true;
    mExec = true;
    GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_SE);
    mIsPause = false;
    mPauseSelector = 0;
    mPauseChoice = 13;
    mPauseNextChoice = 13;
    mPausePad = nullptr;
    mMemCard = false;
    _d = false;
    mResultStart = false;
    _f = false;
    mGameOver = false;
    _14 = -1;
    mPauseEnd = false;
    mPause2D->init();
    mResult2D->init();
    _28 = 0;
    mWipeState = 0;
    mWipeType = 0;
    _34 = 0;
    _38 = false;
    if (RaceMgr::getManager()->isReplayMode() && RaceMgr::getManager()->isRaceModeGp())
        Result2D::setGPClr();
    mPrintMemoryCard->init(PrintMemoryCard::mcMsg40);
}

void PauseManager::setPause() {
    mIsPause = true;
    mPause2D->init();
    Pause2D::setPauseStatus(Pause2D::PAUSE_1);
}

void PauseManager::clrPause() {
    mIsPause = false;
}

void PauseManager::draw() {
    if (!_12)
        return;

    if (!_0)
        mResult2D->draw();

    if (_11 || (!mResultStart && !RaceMgr::getManager()->isReplayMode()))
        mPause2D->draw();

    if (_28 != 0) {
        switch (mPauseNextChoice) {
        case 1:
        case 7:
            GETWipeManager()->getMario()->draw((_28 + 1) / 40.0f, JUTColor(0, 0, 0, 255));
            break;
        default:
            _28 = 1;
            break;
        }
    }

    if (mWipeState > 0 && mWipeType > 0) {
        f32 rate = (f32)mWipeState / (f32)mWipeType;
        if (rate > 1.0f)
            rate = 1.0f;
        GETWipeManager()->getWipeCurtain()->calc(J2DManager::getStatus2Kart(0), rate, JUTColor(0, 0, 0, 255));
        GETWipeManager()->getWipeCurtain()->draw(J2DManager::getStatus2Kart(0), rate, JUTColor(0, 0, 0, 255));
    }

    if (mMemCard)
        mPrintMemoryCard->draw();
}

void PauseManager::exec() {
    static int button;

    if (!mExec)
        return;

    if (_14 >= 0)
        _14++;

    if (mMemCard) {
        mPrintMemoryCard->calc();
        switch (mPrintMemoryCard->get_14Thing()) {
        case 0:
            mMemCard = false;
            if (_d)
                goto pause_cancel;
            goto result_cancel;
        case 1:
            mMemCard = false;
            mPrintMemoryCard->init(PrintMemoryCard::mcMsg40);
            mPrintMemoryCard->setStuff(0, 0, 0);
            break;
        }
        return;
    }
    else {
        if (mWipeState > 0 && mWipeType > 0) {
            if (mWipeState == mWipeType)
                System::getDisplay()->startFadeOut(1);
            mWipeState++;
        }

        if (_28 != 0) {
            if (_28 < 40) {
                _28++;
            }
            else {
                mPauseChoice = mPauseNextChoice;
                mPausePad = nullptr;
                GETJ2DManager()->startPreRace();
                _28 = 0;
                Result2D::setDrawFlag(_38);
            }
            return;
        }

        if (_34 != 0) {
            if (_34 < 11) {
                _34++;
            }
            else {
                mPauseChoice = mPauseNextChoice;
                mPausePad = nullptr;
                _34 = 0;
                Result2D::setDrawFlag(_38);
                switch (mPauseNextChoice) {
                case 1:
                case 7:
                    mPauseChoice = 13;
                    _28 = 1;
                    GetGameAudioMain()->fadeOutAll(40);
                    break;
                default:
                    switch (mPauseNextChoice) {
                    case 13:
                        if (!_38)
                            mIsPause = false;
                        break;
                    default:
                        _34 = 1;
                        break;
                    }
                    break;
                }
            }
            mResult2D->calc(gSequenceInfo.getDecidePad());
            return;
        }

        if (_14 < 120)
            return;

        if (!tstPause() && !RaceMgr::getManager()->isReplayMode()) {
            if (_11) {
                mPausePad = &gGamePad1P;
                mPauseSelector = 0;
                mPauseChoice = 13;
            }
            else {
                KartGamePad *pad = gpaGamePad[0];
                if (pad && pad->testTrigger(JUTGamePad::START)) {
                    mPausePad = pad;
                    mPauseSelector = 0;
                    mPauseChoice = 13;
                }
            }
        }

        if (!_0) {
            if ((mResultStart && Goal2D::mDrawEndFlag) || RaceMgr::getManager()->isReplayMode()) {
                mResult2D->calc(gSequenceInfo.getDecidePad());
                if (RaceMgr::getManager()->isReplayMode() && (int)RaceApp::ptr()->getPadRecorder()->getFrame() >= 17864) {
                    mIsPause = true;
                    mPause2D->init();
                    Pause2D::setPauseStatus(Pause2D::PAUSE_1);
                    GetGameAudioMain()->fadeOutAll(40);
                    mPauseNextChoice = 7;
                    _34 = 1;
                    _38 = false;
                }
                else if (gSequenceInfo.getDecidePad()->testTrigger(JUTGamePad::A) && Result2D::mResultState == 2 && mResult2D->getAnmEnd()) {
                    if (Result2D::getDrawFlag()) {
                        switch (Result2D::getResultSelector()) {
                        case 1:
                        case 7:
                            GetGameAudioMain()->startSystemSe(0x20002);
                            GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_SE);
                            GetGameAudioMain()->fadeOutAll(40);
                            mPauseNextChoice = Result2D::getResultSelector();
                            _34 = 1;
                            _38 = false;
                            mResult2D->set_1980(1);
                            break;
                        case 8:
                            mPauseNextChoice = Result2D::getResultSelector();
                            if (RaceMgr::getManager()->isRaceModeGp()) {
                                mMemCard = true;
                                _d = false;
                                mPrintMemoryCard->init(PrintMemoryCard::mcMsg39);
                                mPrintMemoryCard->calc();
                            }
                            else {
                                GetGameAudioMain()->startSystemSe(0x20023);
                            result_cancel:
                                _34 = 1;
                                _38 = false;
                                mResult2D->set_1980(1);
                            }
                            break;
                        default:
                            GetGameAudioMain()->startSystemSe(0x20002);
                            GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_SE);
                            mPauseNextChoice = Result2D::getResultSelector();
                            _34 = 1;
                            _38 = false;
                            mResult2D->set_1980(1);
                            break;
                        }
                    }
                }
                else if (gSequenceInfo.getDecidePad()->testTrigger(JUTGamePad::START)) {
                    if (RaceMgr::getManager()->isReplayMode()) {
                        if (!Result2D::getDrawFlag()) {
                            mIsPause = true;
                            mPause2D->init();
                            Pause2D::setPauseStatus(Pause2D::PAUSE_1);
                            GetGameAudioMain()->startSystemSe(0x20008);
                            GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_ALL);
                            mPauseNextChoice = 13;
                            _34 = 1;
                            _38 = true;
                            Result2D::setDrawFlag(true);
                            mResult2D->init();
                        }
                        else if (mResult2D->getAnmEnd()) {
                            GetGameAudioMain()->startSystemSe(0x20009);
                            GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_NONE);
                            mPauseNextChoice = 13;
                            _34 = 1;
                            _38 = false;
                            mResult2D->set_1980(1);
                        }
                    }
                }
                else if (gSequenceInfo.getDecidePad()->testTrigger(JUTGamePad::A)) {
                    if (RaceMgr::getManager()->isReplayMode() && !Result2D::getDrawFlag()) {
                        mIsPause = true;
                        mPause2D->init();
                        Pause2D::setPauseStatus(Pause2D::PAUSE_1);
                        GetGameAudioMain()->startSystemSe(0x20008);
                        GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_ALL);
                        mPauseNextChoice = 13;
                        _34 = 1;
                        _38 = true;
                        Result2D::setDrawFlag(true);
                        mResult2D->init();
                    }
                }
                else if (gSequenceInfo.getDecidePad()->testTrigger(JUTGamePad::B)) {
                    if (RaceMgr::getManager()->isReplayMode() && Result2D::getDrawFlag()) {
                        GetGameAudioMain()->startSystemSe(0x20009);
                        GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_NONE);
                        mPauseNextChoice = 13;
                        _34 = 1;
                        _38 = false;
                        mResult2D->set_1980(1);
                    }
                }
            }
        }
    }

    if (mPausePad != nullptr && !_0) {
        if ((!mPauseEnd || _11) && !RaceMgr::getManager()->isReplayMode()) {
            mPause2D->calc(mPausePad);
            if (tstPause()) {
                if (Pause2D::getPauseStatus() == Pause2D::PAUSE_3) {
                    mPauseChoice = 13;
                    if (mPausePad->testTrigger(JUTGamePad::A)) {
                        switch (Pause2D::getPauseSelector()) {
                        case 1:
                            GetGameAudioMain()->startSystemSe(0x20023);
                            _28 = 1;
                            GetGameAudioMain()->fadeOutAll(40);
                            GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_SE);
                            mPauseNextChoice = Pause2D::getPauseSelector();
                            break;
                        case 8:
                            if (RaceMgr::getManager()->isRaceModeGp()) {
                                mMemCard = true;
                                _d = true;
                                mPrintMemoryCard->init(PrintMemoryCard::mcMsg39);
                                mPrintMemoryCard->calc();
                            }
                            else {
                                GetGameAudioMain()->startSystemSe(0x20023);
                            pause_cancel:
                                button = 0;
                                Pause2D::setPauseStatus(Pause2D::PAUSE_4);
                            }
                            break;
                        case 0:
                            GetGameAudioMain()->startSystemSe(0x20009);
                            button = 0;
                            Pause2D::setPauseStatus(Pause2D::PAUSE_4);
                            break;
                        default:
                            GetGameAudioMain()->startSystemSe(0x20023);
                            button = 0;
                            Pause2D::setPauseStatus(Pause2D::PAUSE_4);
                            break;
                        }
                    }
                    else if (mPausePad->testTrigger(JUTGamePad::START)) {
                        button = 1;
                        Pause2D::setPauseStatus(Pause2D::PAUSE_4);
                        GetGameAudioMain()->startSystemSe(0x20009);
                        GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_NONE);
                    }
                }
                else if (Pause2D::getPauseStatus() == Pause2D::PAUSE_0) {
                    mPauseChoice = 13;
                    switch (button) {
                    case 0:
                        switch (Pause2D::getPauseSelector()) {
                        case 0:
                            mIsPause = false;
                            GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_NONE);
                        default:
                            mPauseChoice = Pause2D::getPauseSelector();
                            mPausePad = nullptr;
                            GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_SE);
                            break;
                        }
                        break;
                    case 1:
                        mIsPause = false;
                        mPauseSelector = 0;
                        mPauseChoice = 0;
                        mPausePad = nullptr;
                        break;
                    default:
        #line 553
                        JUT_ASSERT_F(0, "%s", "0");
                        break;
                    }
                }
            }
            else if (mPausePad->testTrigger(JUTGamePad::START)) {
                mIsPause = true;
                mPause2D->init();
                Pause2D::setPauseStatus(Pause2D::PAUSE_1);
                GetGameAudioMain()->startSystemSe(0x20008);
                GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_ALL);
            }
        }
    }
}

inline u8 Result2D::getDrawFlag() { return mDrawFlag; }

inline void Result2D::setDrawFlag(bool flag) { mDrawFlag = flag; }

bool PauseManager::tstPause() {
    return mIsPause;
}

void PauseManager::wipeIn() {}

void PauseManager::wipeOut(int wipeType) {
    mWipeState = 1;
    mWipeType = wipeType;
    GETWipeManager()->getWipeCurtain()->reset();
}

int PauseManager::getPauseChoice() {
    if (mPauseChoice == 8)
        GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_SE);

    if (mThis->_f) {
        mPauseChoice = 5;
        GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_SE);
    }
    if (mThis->mGameOver) {
        mPauseChoice = 9;
        GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_SE);
    }

    switch (mPauseChoice) {
    case 2:
    case 3:
    case 4:
    case 6:
        gSequenceInfo._28 = 1;
        break;
    }

    switch (mPauseChoice) {
    case 4:
        gSequenceInfo._2c = 0;
        break;
    case 6:
        gSequenceInfo._2c = 1;
        break;
    }

    if (mPauseChoice == 5) {
        GetGameAudioMain()->setPause(GameAudio::Main::PAUSE_SE);
        if (gSequenceInfo.isEndGP()) {
            if (gSequenceInfo.isGameClr())
                mPauseChoice = 11;
            else
                mPauseChoice = 12;
        }
    }

    return mPauseChoice;
}

void PauseManager::getCharRankPos(int idx) {
    mResult2D->getCharPos(idx);
}

void PauseManager::setTA() {
    mResult2D->setTA();
}

#include "JSystem/JAudio/JASFakeMatch2.h"
