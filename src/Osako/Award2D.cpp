#include "Osako/Award2D.h"

#include "Inagaki/GameAudioMain.h"
#include "Kameda/SequenceInfo.h"
#include "Osako/NameEntryHelper.h"
#include "Osako/SystemFile.h"
#include "Osako/SystemRecord.h"
#include "Osako/system.h"

#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

Award2D::Award2D(JKRHeap *heap) {
    mScreen = NULL;
    _4 = 0;
    _6 = 0;
    _8 = 0;
    mState = 0;
    _24 = 0;
    _28 = 0;
    _2c = NULL;
    _30 = NULL;
    _34._0 = 0;
    _34._4 = 0;
    _34._6 = 0;
    _34._8 = NULL;
    _34._c = NULL;
    _34._10 = NULL;
    _48 = NULL;
    _4c = NULL;
    _50 = NULL;
    _54 = NULL;
}

void Award2D::calc() {
    if (mState == 0) {
        return;
    }

    switch (mState) {
    case 1:
        GameAudio::Main::getAudio()->startSystemSe(0x2005c);
        _6 = 0;
        _4 = 0;
        mState = 2;
    case 2:
        if (gSequenceInfo._35c != 0) {
            gSystemFile._6024 = 1;
        }
        if (_8 == 0)
            goto noedit;
        if (_4 >= 163) {
            mState = 4;
            _30->startEdit();
        }
        goto edited;
    noedit:
        if (_4 >= 120) {
            mState = 3;
        }
    edited:
        *(f32 *)(_48 + 8) = (f32)_4;
        *(f32 *)(_50 + 8) = (f32)_4;
        *(f32 *)(_54 + 8) = (f32)_4;
        _4 = _4 + 1;
        break;
    case 3:
        if (*(u32 *)(_2c + 0x1c) & 0x100) {
            mState = 6;
        }
        break;
    case 4:
        if (*(int *)((u8 *)_30 + 0x14) == 3) {
            mState = 5;
            _34.zoomIn();
        }
        break;
    case 5:
        if (*(u32 *)(_2c + 0x1c) & 0x100) {
            GameAudio::Main::getAudio()->startSystemSe(0x20002);
            mState = 6;
            _14.setName(_9);
            gSystemRecord.setGPRecord((ERaceGpCup)_24, (ERaceLevel)_28, _14);
            gSystemRecord.setDefaultName(_9);
            bool special = gSequenceInfo._35e != 0 || gSequenceInfo._35f != 0;
            if (special) {
                gSequenceInfo._360 = 1;
                gSystemFile._6024 = 0;
            } else {
                gSystemFile._6024 = 1;
            }
            _34.zoomOut();
        } else if (*(u32 *)(_2c + 0x1c) & 0x200) {
            GameAudio::Main::getAudio()->startSystemSe(0x20005);
            mState = 4;
            _30->startEdit();
            _34.zoomOut();
        }
        break;
    }

    *(f32 *)(_4c + 8) = (f32)_6;
    _6++;
    if (_6 >= 300) {
        _6 = 110;
    }

    if (_8 != 0) {
        _30->calc();
    }
    _34.calcAnm();
    mScreen->animation();
}

void Award2D::draw() {
    if (mState != 0) {
        J2DOrthoGraph *ortho = System::mspJ2DOrtho;
        ortho->setPort();
        mScreen->draw(0.0f, 0.0f, ortho);
        if (_8 != 0) {
            _30->draw(ortho);
        }
    }
}

void Award2D::OKButton::calcAnm() {
    if (_0 == 0) {
        goto after;
    }

    *(f32 *)(_8 + 8) = (f32)_4;
    *(f32 *)(_c + 8) = (f32)_6;
    *(f32 *)(_10 + 8) = (f32)_6;
    _6++;
    if (_6 >= 120) {
        _6 = 0;
    }

after:
    switch (_0) {
    case 1:
        if (++_4 >= 195) {
            _0 = 2;
        }
        break;
    case 3:
        if (--_4 < 180) {
            _0 = 0;
        }
        break;
    }
}

void Award2D::OKButton::zoomIn() {
    _0 = 1;
    _6 = 0;
    _4 = 180;
}

void Award2D::OKButton::zoomOut() {
    _0 = 3;
    _4 = 194;
}
