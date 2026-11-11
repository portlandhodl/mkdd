#include "Osako/LANEntry.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/JUtility/JUTFader.h"
#include "Osako/LANPlayInfo.h"
#include "Osako/kartPad.h"
#include "Osako/NetGameMgr.h"
#include "Osako/system.h"
#include "Osako/PadMgr.h"
#include "Kaneshige/RaceInfo.h"

void LANEntry::waitEntry() {
    int bit;
    bool canceled = false;
    bool removed = false;
    bool changed = false;

    for (int i = 0; i < mPlayerNum; i++) {
        bit = 1 << i;

        if (mMask & bit) {
            if (gpaKartPad[i]->testTrigger(JUTGamePad::A)) {
                if (mTimerState == 2 && mDelay == 0) {
                    _20 = 179;
                    mDelay = 20;
                }
            } else if (gpaKartPad[i]->testTrigger(JUTGamePad::B)) {
                mMask &= ~bit;
                if (i == mSelfIndex) {
                    unentry();
                    mBackground.unentry();
                    GetGameAudioMain()->startSystemSe(0x20004);
                }
                mNum--;
                mCount -= _15[i];
                mCarPic->changeTexture(mTimg[mCount], 0);
                removed = true;
            }
        } else {
            int repeat = gpaKartPad[i]->getRepeat();
            if (repeat & 0x01000000) {
                if (_15[i] > 1) {
                    _15[i]--;
                    if (i == mSelfIndex) {
                        GetGameAudioMain()->startSystemSe(0x2000c);
                        _34 = 1;
                        _1a8 = 1;
                        _1a6 = 50;
                        mLeftPic->setAnimation(mAnm5);
                        mLeftPic->setAnimation(mTevReg2);
                        mRightPic->setAnimation((J2DAnmTransform *)nullptr);
                        mRightPic->setAnimation((J2DAnmTevRegKey *)nullptr);
                    }
                }
            } else if (repeat & JUTGamePad::MAINSTICK_RIGHT) {
                if (_15[i] < mDisplayNum) {
                    _15[i]++;
                        if (i == mSelfIndex) {
                            GetGameAudioMain()->startSystemSe(0x2000c);
                            _34 = 1;
                            _1a8 = 1;
                            _1a6 = 50;
                            mLeftPic->setAnimation((J2DAnmTransform *)nullptr);
                            mLeftPic->setAnimation((J2DAnmTevRegKey *)nullptr);
                            mRightPic->setAnimation(mAnm5);
                            mRightPic->setAnimation(mTevReg2);
                        }
                    }
                }

            if (gpaKartPad[i]->testTrigger(JUTGamePad::A)) {
                mMask |= bit;
                if (i == mSelfIndex) {
                    entry();
                    mBackground.entry();
                    GetGameAudioMain()->startSystemSe(0x20002);
                }
                mNum++;
                mCount += _15[i];
                mCarPic->changeTexture(mTimg[mCount], 0);
                changed = true;
            } else if (gpaKartPad[i]->testTrigger(JUTGamePad::B)) {
                canceled = true;
            }
        }
    }

    if (mNum == mPlayerNum) {
        endReceipt();
        mReceipt = 0;
    } else {
        if (mNum >= 2 && changed) {
            mTimer = 15;
            mDelay = 0;
            int timer = mTimer;
            if (timer >= 10) {
                mNumber1->changeTexture(mTimg[timer % 10], 0);
                mNumber2->changeTexture(mTimg[timer / 10], 0);
                mNumber3->mIsVisible = false;
                mNumber1->mIsVisible = true;
                mNumber2->mIsVisible = true;
            } else {
                mNumber3->changeTexture(mTimg[timer], 0);
                mNumber3->mIsVisible = true;
                mNumber1->mIsVisible = false;
                mNumber2->mIsVisible = false;
            }
            startTimer();
        } else if (mNum < 2 && removed) {
            stopTimer();
        }
        if (mNum == 0 && canceled) {
            mReceipt = 1;
            GetGameAudioMain()->startSystemSe(0x20004);
            endReceipt();
        }
    }
}

void LANEntry::endReceipt() {
    mState = 2;
    System::getDisplay()->startFadeOut(15);
    GetGameAudioMain()->fadeOutAll(15);
}

void LANEntry::setRaceInfo() {
    NetGameMgr *netMgr = NetGameMgr::ptr();
    ERaceLevel level = LVL_50CC;
    ERaceMode mode = GRAND_PRIX;
    switch (gLANPlayInfo.mGameMode) {
    case 0:
        level = (ERaceLevel)gLANPlayInfo.mPower;
        break;
    case 1:
        mode = BALLOON_BATTLE;
        break;
    case 2:
        mode = ESCAPE_BATTLE;
        break;
    }

    gRaceInfo.setRace(mode, mCount, mCount, mDisplayNum, _15[mSelfIndex]);
    gRaceInfo.setRaceLevel(level);
    gRaceInfo.setRandomSeed(NetGameMgr::ptr()->get12e9());
    gRaceInfo.mLapNumLAN = gLANPlayInfo.mOrder;
    gRaceInfo.mIsLanMode = true;
    gLANPlayInfo.shuffleKart(mCount);

    int kartNo = 0;
    int padNo = 0;
    int target = 0;
    for (int i = 0; i < mPlayerNum; i++) {
        if (mMask & (1 << i)) {
            gLANPlayInfo.setActiveNum(i, _15[i]);
            if (_1d) {
                int j;
                for (j = 0; j < _15[i]; j++) {
                    EKartID kart = (EKartID)gLANPlayInfo.mPlayers[kartNo].mKart;
                    gRaceInfo.setKart(kartNo, kart, gLANPlayInfo.getFront(kartNo),
                                      gpaKartPad[padNo], gLANPlayInfo.getBack(kartNo), gpaKartPad[padNo + 1]);
                    if (i == mSelfIndex) {
                        gRaceInfo.setConsoleTarget(j, kartNo, false);
                    }
                    gRaceInfo.setStartPosIdx(kartNo, gLANPlayInfo.mPlayers[kartNo].mUnk);
                    netMgr->setPadConv(padNo, i * 4 + j * 2);
                    netMgr->setPadConv(padNo + 1, i * 4 + j * 2 + 1);
                    kartNo++;
                    padNo += 2;
                }
                if (i == mSelfIndex) {
                    for (; j < mDisplayNum; j++) {
                        gRaceInfo.setConsoleTarget(j, 0, true);
                        gRaceInfo.hideConsole(j);
                    }
                }
            } else {
                int j;
                for (j = 0; j < _15[i]; j++) {
                    EKartID kart = (EKartID)gLANPlayInfo.mPlayers[kartNo].mKart;
                    gRaceInfo.setKart(kartNo, kart, gLANPlayInfo.getFront(kartNo),
                                      gpaKartPad[padNo], gLANPlayInfo.getBack(kartNo), nullptr);
                    if (i == mSelfIndex) {
                        gRaceInfo.setConsoleTarget(j, kartNo, false);
                    }
                    gRaceInfo.setStartPosIdx(kartNo, gLANPlayInfo.mPlayers[kartNo].mUnk);
                    netMgr->setPadConv(padNo, i * 4 + j);
                    kartNo++;
                    padNo++;
                }
                if (i == mSelfIndex) {
                    for (; j < mDisplayNum; j++) {
                        gRaceInfo.setConsoleTarget(j, 0, true);
                        gRaceInfo.hideConsole(j);
                    }
                }
            }
        } else {
            if (i == mSelfIndex) {
                for (int j = 0; j < mDisplayNum; j++) {
                    gRaceInfo.setConsoleTarget(j, target, true);
                }
            }
            if (++target >= mCount) {
                target = 0;
            }
        }
    }

    PadMgr::resetPort();
}

int LANEntry::calc() {
    int ret = 0;

    switch (mState) {
    case 1: {
        waitEntry();
        break;
    }
    case 0: {
        if (System::getDisplay()->getFader()->getStatus() == JUTFader::Status_In) {
            mState = 1;
        }
        break;
    }
    case 2: {
        if (System::getDisplay()->getFader()->getStatus() == JUTFader::Status_Out) {
            if (mReceipt) {
                ret = 2;
            } else {
                setRaceInfo();
                if (mDisplayNum == 4 && (mCount >= 7 || (mCount == 6 && _15[0] != 3))) {
                    NetGameMgr::ptr()->set12ee(1);
                } else {
                    NetGameMgr::ptr()->set12ee(0);
                }
                ret = 1;
            }
        }
        break;
    }
    }

    calcAnm();
    return ret;
}

void LANEntry::draw(const J2DGrafContext *graf) {
    mBackground.draw(0.0f, 0.0f, graf);
    mScreen.draw(0.0f, 0.0f, graf);
}

void LANEntry::start() {
    mDisplayNum = gLANPlayInfo.getDisplayNum();
    _1d = gLANPlayInfo.mTag != 0;
    mCount = 0;
    _13 = 0;
    mMask = 0;
    mNum = 0;
    mPlayerNum = NetGameMgr::ptr()->getNetworkCubes();
    mSelfIndex = NetGameMgr::ptr()->getNetworkNum();
    _12 = mPlayerNum * mDisplayNum;
    mState = 0;
    mTimerState = 0;

    mFrame = 0;
    _19e = 89;
    _1a0 = 89;
    _1a2 = 0;
    _20 = 89;
    _1a4 = 0;
    _1a6 = 50;
    _1a8 = 0;

    for (int i = 0; i < 8; i++) {
        if (i < mPlayerNum) {
            _15[i] = gLANPlayInfo.mActive[(u8)i];
        } else {
            _15[i] = 0;
        }
    }

    if (_1d) {
        mTagPane->mIsVisible = true;
    } else {
        mTagPane->mIsVisible = false;
    }

    if (mDisplayNum > 1) {
        mEntryPane->mIsVisible = true;
    } else {
        mEntryPane->mIsVisible = false;
    }

    GetGameAudioMain()->startSequenceBgm(0x1000001);
    System::getDisplay()->startFadeIn(15);
    mCarPic->changeTexture(mTimg[0], 0);
    mEntryNumPic->changeTexture(mTimg[_15[mSelfIndex]], 0);
}

void LANEntry::entry() {
    mAnimState = 1;
    _1a2 = 89;
}

void LANEntry::unentry() {
    mAnimState = 3;
}

void LANEntry::startTimer() {
    mTimerState = 1;
}

void LANEntry::stopTimer() {
    mTimerState = 3;
}
