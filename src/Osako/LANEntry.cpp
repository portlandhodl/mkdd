#include "Osako/LANEntry.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/JUtility/JUTFader.h"
#include "Osako/LANPlayInfo.h"
#include "Osako/kartPad.h"
#include "Osako/NetGameMgr.h"
#include "Osako/system.h"
#include "Osako/PadMgr.h"
#include "Kaneshige/RaceInfo.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include <stdio.h>

#ifdef DEBUG
#include "mathHelper.h"
#endif

// UNUSED: only its string literal survives in the target
static ResTIMG *getPlayerNumberTexture(JKRArchive *arc, int num) {
    char buf[64];
    snprintf(buf, sizeof(buf), "timg/PlayerNumberSimple_%dP.bti", num);
    return (ResTIMG *)arc->getResource(buf);
}

LANEntry::LANEntry(JKRArchive *arc, JKRArchive *bgArc) {
    mAnimState = 0;
    mTimerState = 0;
    _34 = 0;
    mBackground.setup(bgArc);

    for (int i = 0; i < 10; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "timg/MFNumber%d.bti", i);
        mTimg[i] = (ResTIMG *)arc->getResource(buf);
    }

    mScreen.setPriority("LANEntry.blo", 0x20000, arc);
    void *brk;
    void *bck = JKRFileLoader::getGlbResource("LANEntry.bck", arc);
    void *bpk = JKRFileLoader::getGlbResource("LANEntry.bpk", arc);
    brk = (brk, JKRFileLoader::getGlbResource("LANEntry.brk", arc));
    mAnm0 = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mAnm1 = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mAnm2 = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mAnm3 = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mAnm4 = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mAnm5 = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mColor = (J2DAnmColor *)J2DAnmLoaderDataBase::load(bpk);
    mTevReg1 = (J2DAnmTevRegKey *)J2DAnmLoaderDataBase::load(brk);
    mTevReg2 = (J2DAnmTevRegKey *)J2DAnmLoaderDataBase::load(brk);
    mTevReg2->searchUpdateMaterialID(&mScreen);

    mScreen.setAnimation(mAnm3);
    mScreen.setAnimation(mColor);
    mScreen.setAnimation(mTevReg1);
    mScreen.search(0x4e5072657373ull)->setAnimation(mAnm1);
    mScreen.search(0x4d5072657373ull)->setAnimation(mAnm2);
    mTagPane = mScreen.search(0x4e4d546167ull);
    mTagPane->hide();
    mScreen.search(0x4e436f756e74ull)->setAnimation(mAnm0);
    mNumber3 = (J2DPicture *)mScreen.search(0x4e756d3131ull);
    for (int i = 0; i < 2; i++) {
        mNumber[i] = (J2DPicture *)mScreen.search(0x4e756d3231ull + i);
    }
    mCarPic = (J2DPicture *)mScreen.search(0x4d43617231ull);
    mEntryNumPic = (J2DPicture *)mScreen.search(0x45434e756dull);
    mEntryPane = mScreen.search(0x4e4543ull);
    mScreen.search(0x4e454374ull)->setAnimation(mAnm2);
    mScreen.search(0x4e4543744cull)->setAnimation(mAnm4);
    mScreen.search(0x4e45437452ull)->setAnimation(mAnm4);
    mLeftPic = (J2DPicture *)mScreen.search(0x4543744c30ull);
    mLeftPic->setAnimation(mAnm5);
    mLeftPic->setAnimation(mTevReg2);
    mRightPic = (J2DPicture *)mScreen.search(0x4543745230ull);
    mRightPic->setAnimation(mAnm5);
    mRightPic->setAnimation(mTevReg2);
}

void LANEntry::changeNumberTexture(int timer) {
    if (timer >= 10) {
        mNumber[0]->changeTexture(mTimg[timer % 10], 0);
        mNumber[1]->changeTexture(mTimg[timer / 10], 0);
        mNumber3->mIsVisible = false;
        mNumber[0]->mIsVisible = true;
        mNumber[1]->mIsVisible = true;
    } else {
        mNumber3->changeTexture(mTimg[timer], 0);
        mNumber3->mIsVisible = true;
        mNumber[0]->mIsVisible = false;
        mNumber[1]->mIsVisible = false;
    }
}

void LANEntry::receiptEntry(int i) {
    mMask |= 1 << i;
    if (i == mSelfIndex) {
        entry();
        mBackground.entry();
        GetGameAudioMain()->startSystemSe(0x20002);
    }
    mNum++;
    mCount += _15[i];
    mCarPic->changeTexture(mTimg[mCount], 0);
}

void LANEntry::cancelEntry(int i) {
    mMask &= ~(1 << i);
    if (i == mSelfIndex) {
        unentry();
        mBackground.unentry();
        GetGameAudioMain()->startSystemSe(0x20004);
    }
    mNum--;
    mCount -= _15[i];
    mCarPic->changeTexture(mTimg[mCount], 0);
}

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
                cancelEntry(i);
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
                receiptEntry(i);
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
            changeNumberTexture(mTimer);
            startTimer();
        } else {
            if (mNum < 2 && removed) {
                stopTimer();
            }
            if (mNum == 0 && canceled) {
                mReceipt = 1;
                GetGameAudioMain()->startSystemSe(0x20004);
                endReceipt();
            }
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
    int localNum = _15[mSelfIndex];
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

    gRaceInfo.setRace(mode, mCount, mCount, mDisplayNum, localNum);
    gRaceInfo.setRaceLevel(level);
    gRaceInfo.setRandomSeed(NetGameMgr::ptr()->get12e9());
    gRaceInfo.mLapNumLAN = gLANPlayInfo.mOrder;
    gRaceInfo.mIsLanMode = true;
    gLANPlayInfo.shuffleKart(mCount);

    int kartNo = 0;
    int padNo = 0;
    int target = 0;
    int j;
    for (int i = 0; i < mPlayerNum; i++) {
        if (mMask & (1 << i)) {
            gLANPlayInfo.setActiveNum(i, _15[i]);
            if (_1d) {
                for (j = 0; j < _15[i]; j++) {
                    EKartID kart = (EKartID)gLANPlayInfo.mPlayers[kartNo].mKart;
                    gRaceInfo.setKart(kartNo, kart, gLANPlayInfo.getFront(kartNo),
                                      gpaKartPad[padNo], gLANPlayInfo.getBack(kartNo), gpaKartPad[padNo + 1]);
                    if (i == mSelfIndex) {
                        gRaceInfo.setConsoleTarget(j, kartNo, false);
                    }
                    gRaceInfo.setStartPosIdx(kartNo, gLANPlayInfo.mPlayers[kartNo].mUnk);
                    netMgr->setPadConv(padNo++, i * 4 + j * 2);
                    netMgr->setPadConv(padNo++, i * 4 + j * 2 + 1);
                    kartNo++;
                }
                if (i == mSelfIndex) {
                    for (; j < mDisplayNum; j++) {
                        gRaceInfo.setConsoleTarget(j, 0, true);
                        gRaceInfo.hideConsole(j);
                    }
                }
            } else {
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
                for (j = 0; j < mDisplayNum; j++) {
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

void LANEntry::calcAnm() {
    mBackground.calc();
    mAnm0->setFrame(_20);
    mAnm1->setFrame(_1a0);
    mAnm2->setFrame(_1a2);
    mAnm3->setFrame(_19e);
    mAnm4->setFrame(_1a4);
    mAnm5->setFrame(_1a6);
    mColor->setFrame(_20);
    mTevReg1->setFrame(mFrame);
    mTevReg2->setFrame(_1a8);

    switch (mAnimState) {
    case 3:
        if (--_19e < 89) {
            mAnimState = 0;
            _19e = 89;
        }
        if (--_1a2 < 89) {
            _1a2 = 89;
        }
        break;
    case 1:
        if (++_19e >= 104) {
            mAnimState = 2;
            _19e = 103;
        }
        if (_1a2 < 94) {
            _1a2++;
        }
        break;
    case 0:
        if (++_1a2 >= 40) {
            _1a2 = 0;
        }
        break;
    case 2:
        if (_1a2 < 94) {
            _1a2++;
        }
        break;
    }

    switch (mTimerState) {
    case 1:
        if (++_1a0 >= 106) {
            _1a0 = 106;
        }
        if (++_20 >= 120) {
            _20 = 120;
        }
        if (_1a0 == 106 && _20 >= 120) {
            mTimerState = 2;
        }
        break;
    case 2:
        if (++_20 >= 180) {
            if (--mTimer <= 0) {
                endReceipt();
                mReceipt = 0;
                mTimer = 0;
            }
            changeNumberTexture(mTimer);
            _20 = 120;
        }
        if (mDelay > 0) {
            mDelay--;
        }
        break;
    case 3:
        if (--_1a0 <= 89) {
            _1a0 = 89;
        }
        if (--_20 <= 89) {
            _20 = 89;
        } else if (_20 >= 120) {
            _20 = 119;
        }
        if (_1a0 == 89 && _20 == 89) {
            mTimerState = 0;
        }
        break;
    }

    if (_34 == 1) {
        if (++_1a6 >= 61) {
            _34 = 0;
            _1a6 = 50;
        }
        if (_1a6 == 55) {
            mEntryNumPic->changeTexture(mTimg[_15[mSelfIndex]], 0);
        }
    }

    if (_1a8 > 0) {
        if (++_1a8 >= 20) {
            _1a8 = 0;
        }
    }

    if (++mFrame >= 120) {
        mFrame = 0;
    }

    mScreen.animation();
}

void LANEntry::draw(const J2DGrafContext *graf) {
    mBackground.mScreen.draw(0.0f, 0.0f, graf);
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
