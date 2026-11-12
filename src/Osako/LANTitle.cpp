#include "Osako/LANTitle.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/J2D/J2DPane.h"
#include "JSystem/J2D/J2DScreen.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "JSystem/JUtility/JUTFader.h"
#include "JSystem/JUtility/JUTGamePad.h"
#include "Osako/kartPad.h"
#include "Osako/system.h"
#include "types.h"

LANTitle::Item::Item(int idx, J2DScreen *screen, void *bck, void *brk) {
    mTrans = (J2DAnmTransform*)J2DAnmLoaderDataBase::load(bck);
    mTrans2 = (J2DAnmTransform*)J2DAnmLoaderDataBase::load(bck);
    mKey = (J2DAnmTevRegKey*)J2DAnmLoaderDataBase::load(brk);
    mKey->searchUpdateMaterialID(screen);

    mTransFrame = 0;
    mKeyFrame = 0;
    mFrame = 0;

    mPane = screen->search(0x534831ull + idx);
    mPane->setAnimation(mTrans2);
    mPane->hide();

    J2DPane *pane = screen->search(0x54746C4D31ull + idx);
    pane->setAnimation(mTrans);
    pane->setAnimation(mKey);
}

void LANTitle::Item::selectIn() {
    mState = 3;
    mTransFrame = 1;
    mKeyFrame = 1;
    mPane->mIsVisible = true;
}

void LANTitle::Item::selectOut() {
    mState = 1;
    mKeyFrame = 0;
    mPane->mIsVisible = false;
}

void LANTitle::Item::unselect() {
    mState = 0;
    mTransFrame = 0;
    mKeyFrame = 0;
    mPane->mIsVisible = false;
}

void LANTitle::Item::calc() {
    mTrans->setFrame(mTransFrame);
    mKey->setFrame(mKeyFrame);
    mTrans2->setFrame(mFrame);

    switch (mState) {
    case 1: {
        if (++mTransFrame >= 4) {
            mTransFrame = 4;
            mState = 2;
        }
        break;
    }
    case 3: {
        if (--mTransFrame < 1) {
            mTransFrame = 0;
            mState = 0;
        }
        break;
    }
    }

    if (++mFrame >= 90) {
        mFrame = 0;
    }
}

LANTitle::LANTitle(JKRArchive *archive) {
    mScreen.setPriority("LANTitle.blo", 0x20000, archive);

    void *bck = JKRGetNameResource("LANTitle.bck", archive);
    void *brk = JKRGetNameResource("LANTitle.brk", archive);

    mAnm = (J2DAnmTevRegKey*)J2DAnmLoaderDataBase::load(brk);
    mScreen.setAnimation(mAnm);

    mAnmFrame = 0;
    mItemIdx = 0;

    for (int i = 0; i < 3; i++) {
        mItems[i] = new Item(i, &mScreen, bck, brk);
    }

    Item *item = mItems[0];
    item->mState = 1;
    item->mFrame = 0;
    item->mPane->mIsVisible = true;
    item->mKeyFrame = 1;
}

int LANTitle::calc() {
    KartGamePad *pad = gpaKartPad[0];
    int ret = 0;

    switch (mState) {
    case 1: {
        if (pad->testRepeat(JUTGamePad::MAINSTICK_UP)) {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            Item *item = mItems[mItemIdx];
            item->mState = 3;
            item->mKeyFrame = 0;
            item->mPane->mIsVisible = false;

            if (--mItemIdx < 0) {
                mItemIdx = 2;
            }

            Item *next = mItems[mItemIdx];
            next->mState = 1;
            next->mFrame = 0;
            next->mPane->mIsVisible = true;
            next->mKeyFrame = 1;
            mAnmFrame = 0;
        } else if (pad->testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            Item *item = mItems[mItemIdx];
            item->mState = 3;
            item->mKeyFrame = 0;
            item->mPane->mIsVisible = false;

            if (++mItemIdx >= 3) {
                mItemIdx = 0;
            }

            Item *next = mItems[mItemIdx];
            next->mState = 1;
            next->mFrame = 0;
            next->mPane->mIsVisible = true;
            next->mKeyFrame = 1;
            mAnmFrame = 0;
        }

        if (pad->testTrigger(JUTGamePad::A)) {
            if (mItemIdx == 2) {
                ret = 3;
            } else {
                GameAudio::Main::getAudio()->startSystemSe(0x20002);
                mState = 2;
                System::getDisplay()->startFadeOut(15);
                GameAudio::Main::getAudio()->fadeOutAll(15);
            }
        }
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
            switch (mItemIdx) {
            case 0: {
                ret = 1;
                break;
            }
            case 1: {
                ret = 2;
                break;
            }
            }
        }
        break;
    }
    }

    calcAnm();
    return ret;
}

void LANTitle::calcAnm() {
    mAnm->setFrame(mAnmFrame);

    if (++mAnmFrame >= 90) {
        mAnmFrame = 0;
    }

    for (int i = 0; i < 3; i++) {
        mItems[i]->calc();
    }

    mScreen.animation();
}

void LANTitle::draw(const J2DGrafContext *graf) {
    mScreen.draw(0.0f, 0.0f, graf);
}

void LANTitle::start() {
    GameAudio::Main::getAudio()->startSequenceBgm(0x1000000);

    if (mItemIdx != 0) {
        Item *item = mItems[mItemIdx];
        item->mState = 0;
        item->mTransFrame = 0;
        item->mKeyFrame = 0;
        item->mPane->mIsVisible = false;
        mItemIdx = 0;
    }

    Item *item = mItems[mItemIdx];
    item->mState = 1;
    item->mFrame = 0;
    item->mPane->mIsVisible = true;
    item->mKeyFrame = 1;

    mState = 0;
    System::getDisplay()->startFadeIn(15);
}

#include "JSystem/JAudio/JASFakeMatch.h"
