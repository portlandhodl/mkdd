#include <string.h>
#include "Kameda/PrintMemoryCard.h"
#include "Kameda/PrintWindow.h"
#include "Kameda/ReadPrintMessage.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Osako/NetGameMgr.h"
#include "Osako/ResMgr.h"
#include "Osako/kartPad.h"

PrintMemoryCard::PrintMemoryCard(JKRHeap *heap) {
    mpWindow = new (heap, 0) PrintWindow(heap);
    mpWindow->mInfo.type = 1;
    mpWindow->mInfo._2f0 = 0;
    mColorPtr = ResMgr::getPtr(ResMgr::mcColor);
    mBmgPtr = ResMgr::getPtr(ResMgr::mcCard);
    mColor = ReadPrintMessage::getColor(mColorPtr, ReadPrintMessage::getDefaultColor(mBmgPtr));
    reset();
}

void PrintMemoryCard::reset() {
    _c = 0;
    _d = 0;
    _e = 0;
    _f = 0;
    _20 = 0;
    _24 = 0;
    _28 = 0;
    mpWindow->mInfo.frame = _20;
    mpWindow->calc();
}

void PrintMemoryCard::init(PrintMemoryCard::MessageID msg) {
    _f = 1;
    mMessageID = msg;
    _14 = 2;
    _1c = 2;
}

void PrintMemoryCard::changeMessage() {
    _14 = 2;
    _1c = 0;
    switch (_24) {
    case 0:
        _20 = -1;
        break;
    case 1:
        _20 = 24;
        break;
    }

    if (mMessageID != mcMsg40) {
        mpWindow->mInfo.type = getWindowSize();
        mpWindow->mInfo._2f0 = getWindowColor();
        switch (getChoiceType()) {
        case 0:
            _18 = 2;
            mpWindow->mInfo._2ec = 1;
            mpWindow->mInfo._2f8 = 2;
            break;
        case 1:
            _18 = 0;
            mpWindow->mInfo._2ec = 0;
            mpWindow->mInfo._2f8 = 0;
            break;
        case 2:
            _18 = 1;
            mpWindow->mInfo._2ec = 0;
            mpWindow->mInfo._2f8 = 1;
            break;
        default:
#line 151
            JUT_ASSERT(0);
        }

        if (isMessage())
            mpWindow->getTextBox()->show();
        else
            mpWindow->getTextBox()->hide();

        if (getChoiceType() == 0)
            mpWindow->mInfo._2ec = 1;
        else
            mpWindow->mInfo._2ec = 0;

        if (isMessage()) {
            setBmgPtr();
            int bmgId = ReadPrintMessage::getBmgMessageID(mMessageID);
            char *msg = ReadPrintMessage::getMessage(mBmgPtr, bmgId);
            mColor = ReadPrintMessage::getColor(mColorPtr, ReadPrintMessage::getDefaultColor(mBmgPtr));
            mpWindow->getTextBox()->setFontColor(mColor, mColor);

            J2DTextBox::TFontSize baseFontSize = mpWindow->getBaseFontSize();
            char buf[512];
            J2DTextBox::TFontSize fontSize;

            mpWindow->getTextBox()->setFontSize(baseFontSize.x * ReadPrintMessage::getRate(mBmgPtr, bmgId) / 100,
                                                baseFontSize.y * ReadPrintMessage::getRate(mBmgPtr, bmgId) / 100);
            mpWindow->getTextBox()->setCharSpace(ReadPrintMessage::getCharSpace(mBmgPtr, bmgId));
            mpWindow->getTextBox()->getFontSize(fontSize);
            ReadPrintMessage::tagCnv(msg, mColorPtr, fontSize, sizeof(buf), buf);
            strncpy(mpWindow->getTextBox()->getStringPtr(), buf, sizeof(buf));
        }

        if (!_c) {
            GetGameAudioMain()->startSystemSe(0x2000b);
            _c = 1;
        }
        else if (!_e) {
            GetGameAudioMain()->startSystemSe(0x20047);
        }
        _e = 0;
    }
}

void PrintMemoryCard::draw() {
    if (mMessageID != mcMsg40) {
        mpWindow->draw();
    }
}

void PrintMemoryCard::calc() {
    if (mMessageID == mcMsg40)
        return;

    bool repeatLeft;
    bool trigA;
    bool repeatRight;
    if (NetGameMgr::ptr()->getState() != 0) {
        repeatRight = gpaKartPad[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT);
        repeatLeft = gpaKartPad[0]->testRepeat(JUTGamePad::MAINSTICK_LEFT);
        trigA = gpaKartPad[0]->testTrigger(JUTGamePad::A);
    }
    else {
        repeatRight = gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT);
        repeatLeft = gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_LEFT);
        trigA = gpaGamePad[0]->testTrigger(JUTGamePad::A);
    }

    if (getChoiceType() != 0) {
        if (trigA) {
            if (_14 == 2 && _1c == 1) {
                switch (_18) {
                case 0:
                    GetGameAudioMain()->startSystemSe(0x20003);
                    _d = 1;
                    _e = 1;
                    _14 = 0;
                    break;
                case 1:
                    GetGameAudioMain()->startSystemSe(0x20005);
                    _d = 1;
                    _e = 1;
                    _14 = 1;
                    break;
                default:
#line 272
                    JUT_ASSERT(0);
                }
                closeWindowNoSe();
            }
        }
        else if (repeatLeft && _18 == 1) {
            if (_14 == 2 && _1c == 1) {
                GetGameAudioMain()->startSystemSe(0x20000);
                _18 = 0;
            }
        }
        else if (repeatRight && _18 == 0) {
            if (_14 == 2 && _1c == 1) {
                GetGameAudioMain()->startSystemSe(0x20000);
                _18 = 1;
            }
        }
    }
    else {
        switch (mMessageID) {
        case mcMissSaved:
        case mcInCardPushButton:
        case mcChangeCardPushButton:
        case mcMissFormat:
        case mcMissMakeFile:
        case mcGstNoLoad:
        case mcGstBrokenNoLoad:
        case mcBrokenNoLoad:
            if (trigA && _14 == 2 && _1c == 1) {
                GetGameAudioMain()->startSystemSe(0x20003);
                _d = 1;
                _e = 1;
                _14 = 0;
                closeWindowNoSe();
            }
            break;
        }
    }

    switch (_1c) {
    case 0:
        switch (_24) {
        case 0:
            if (++_20 >= 10) {
                _20 = 9;
                _1c = 1;
            }
            break;
        case 1:
            if (++_20 >= 30) {
                _20 = 30;
                _1c = 1;
            }
            break;
        default:
#line 340
            JUT_ASSERT(0);
        }
        break;
    case 1:
        break;
    case 2:
    case 3:
        switch (_28) {
        case 0:
            if (--_20 < 0)
                _20 = 0;
            break;
        case 1:
            if (++_20 >= 25)
                _20 = 25;
            break;
        default:
#line 360
            JUT_ASSERT(0);
        }
        if (_20 == 0 || _20 == 25) {
            if (_f) {
                _f = 0;
                changeMessage();
            }
        }
        break;
    default:
#line 373
        JUT_ASSERT(0);
    }

    if (getChoiceType() != 0) {
        switch (_18) {
        case 0:
            mpWindow->mInfo._2f8 = 0;
            break;
        case 1:
            mpWindow->mInfo._2f8 = 1;
            break;
        case 2:
            mpWindow->mInfo._2f8 = 2;
            break;
        default:
#line 390
            JUT_ASSERT(0);
        }
    }

    mpWindow->mInfo.frame = _20;
    mpWindow->calc();
}

void PrintMemoryCard::closeWindow() {
    if (_1c != 3 && _1c != 2) {
        GetGameAudioMain()->startSystemSe(0x20046);
        _d = 1;
        _e = 1;
        _1c = 2;
        switch (_28) {
        case 0:
            _20 = 9;
            break;
        case 1:
            _20 = 20;
            break;
        default:
#line 424
            JUT_ASSERT(0);
        }
    }
}

void PrintMemoryCard::closeWindowNoSe() {
    if (_1c != 3 && _1c != 2) {
        _1c = 2;
        switch (_28) {
        case 0:
            _20 = 9;
            break;
        case 1:
            _20 = 20;
            break;
        default:
#line 451
            JUT_ASSERT(0);
        }
    }
}

void PrintMemoryCard::setBmgPtr() {
    switch (mMessageID) {
    case mcTooManyFilesContinue:
    case mcNoSpaceContinue:
    case mcMsg38:
        mBmgPtr = ResMgr::getPtr(ResMgr::mcLan);
        break;
    case mcMsg39:
        mBmgPtr = ResMgr::getPtr(ResMgr::mcRace);
        break;
    default:
        mBmgPtr = ResMgr::getPtr(ResMgr::mcCard);
        break;
    }
}

bool PrintMemoryCard::isMessage() {
    bool ret;
    if (mMessageID >= mcNoCardContinue && mMessageID < mcMsgMax) {
        if (mMessageID == mcMsg40)
            ret = false;
        else
            ret = true;
    }
    else {
#line 497
        JUT_ASSERT(0);
    }
    return ret;
}

int PrintMemoryCard::getChoiceType() {
    int ret;
    switch (mMessageID) {
    case mcGameSavingNoTouch:
    case mcGetSavingNoTouch:
    case mcMissSaved:
    case mcInCardPushButton:
    case mcChangeCardPushButton:
    case mcMissFormat:
    case mcFormatNoTouch:
    case mcMakeFileNoTouch:
    case mcMadeFile:
    case mcMissMakeFile:
    case mcNoUse:
    case mcWithoutMK:
    case mcBrokenNoUse:
    case mcBrokenNeedFormat:
    case mcLoaded:
    case mcNoSpaceNoSave:
    case mcTooManyFiles:
    case mcSaved:
    case mcNoGst:
    case mcGstNoLoad:
    case mcGstBrokenNoLoad:
    case mcBrokenNoLoad:
    case mcMsg38:
    case mcMsg40:
        ret = 0;
        break;
    case mcMakeFile:
    case mcNoSpaceGoIPL:
    case mcTooManyFilesGoIPL:
        ret = 1;
        break;
    case mcNoCardContinue:
    case mcNoUseContinue:
    case mcWithouMKContinue:
    case mcBrokenNoSaveContinue:
    case mcBrokenGoFormat:
    case mcNoFileNoSave:
    case mcFormatLost:
    case mcNoSaveContinue:
    case mcGstLoad:
    case mcGstSave:
    case mcGstOverwrite:
    case mcTooManyFilesContinue:
    case mcNoSpaceContinue:
    case mcMsg39:
        ret = 2;
        break;
    default:
#line 561
        JUT_ASSERT(0);
    }
    return ret;
}

int PrintMemoryCard::getWindowSize() {
    int ret;
    switch (mMessageID) {
    case mcMadeFile:
    case mcGstLoad:
    case mcLoaded:
    case mcGstSave:
    case mcSaved:
    case mcGstOverwrite:
    case mcMsg39:
        ret = 0;
        break;
    case mcNoCardContinue:
    case mcNoUseContinue:
    case mcWithouMKContinue:
    case mcBrokenNoSaveContinue:
    case mcBrokenGoFormat:
    case mcNoFileNoSave:
    case mcGameSavingNoTouch:
    case mcGetSavingNoTouch:
    case mcMissSaved:
    case mcInCardPushButton:
    case mcChangeCardPushButton:
    case mcMissFormat:
    case mcFormatLost:
    case mcFormatNoTouch:
    case mcNoSaveContinue:
    case mcMakeFile:
    case mcMakeFileNoTouch:
    case mcMissMakeFile:
    case mcNoSpaceGoIPL:
    case mcTooManyFilesGoIPL:
    case mcNoUse:
    case mcWithoutMK:
    case mcBrokenNoUse:
    case mcBrokenNeedFormat:
    case mcNoSpaceNoSave:
    case mcTooManyFiles:
    case mcNoGst:
    case mcGstNoLoad:
    case mcGstBrokenNoLoad:
    case mcBrokenNoLoad:
    case mcTooManyFilesContinue:
    case mcNoSpaceContinue:
    case mcMsg38:
    case mcMsg40:
        ret = 1;
        break;
    default:
#line 624
        JUT_ASSERT(0);
    }
    return ret;
}

int PrintMemoryCard::getWindowColor() {
    int ret;
    switch (mMessageID) {
    case mcNoCardContinue:
    case mcNoUseContinue:
    case mcWithouMKContinue:
    case mcBrokenNoSaveContinue:
    case mcBrokenGoFormat:
    case mcNoFileNoSave:
    case mcGameSavingNoTouch:
    case mcGetSavingNoTouch:
    case mcInCardPushButton:
    case mcChangeCardPushButton:
    case mcFormatLost:
    case mcFormatNoTouch:
    case mcNoSaveContinue:
    case mcMakeFile:
    case mcMakeFileNoTouch:
    case mcMadeFile:
    case mcNoSpaceGoIPL:
    case mcTooManyFilesGoIPL:
    case mcNoUse:
    case mcWithoutMK:
    case mcBrokenNoUse:
    case mcBrokenNeedFormat:
    case mcGstLoad:
    case mcLoaded:
    case mcNoSpaceNoSave:
    case mcTooManyFiles:
    case mcGstSave:
    case mcSaved:
    case mcGstOverwrite:
    case mcNoGst:
    case mcTooManyFilesContinue:
    case mcNoSpaceContinue:
    case mcMsg38:
    case mcMsg39:
    case mcMsg40:
        ret = 0;
        break;
    case mcMissSaved:
    case mcMissFormat:
    case mcMissMakeFile:
    case mcGstNoLoad:
    case mcGstBrokenNoLoad:
    case mcBrokenNoLoad:
        ret = 1;
        break;
    default:
#line 687
        JUT_ASSERT(0);
    }
    return ret;
}

#include "JSystem/JAudio/JASFakeMatch2.h"
