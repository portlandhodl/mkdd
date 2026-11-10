#include "Kameda/SceneGhostLoadSave.h"

#include "Kameda/Scene.h"
#include "Osako/CardAgent.h"
#include "Osako/MenuBackground.h"
#include "Osako/MenuTitleLine.h"
#include "Osako/GhostFile.h"
#include "Osako/system.h"
#include "Osako/CardMgr.h"
#include "Osako/Kart2DCommon.h"
#include "Osako/AppMgr.h"
#include "Osako/RaceApp.h"
#include "Osako/kartPad.h"
#include "Inagaki/GameAudioMain.h"
#include "Kameda/ReadPrintMessage.h"
#include "Osako/ResMgr.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kameda/PrintMemoryCard.h"
#include "kartEnums.h"
#include "mathHelper.h"

const char *scpaBaseFileName[] = {
    "GD_Title", "SelectMemoryCard", "GDIndexLayout", "GDIndexLine",

};

static const ECourseID caRaceCrsID[16] = {
    LUIGI_CIRCUIT, PEACH_BEACH, BABY_PARK, DRY_DRY_DESERT,
    MUSHROOM_BRIDGE, MARIO_CIRCUIT, DAISY_CRUISER, WALUIGI_STADIUM,
    SHERBET_LAND, MUSHROOM_CITY, YOSHI_CIRCUIT, DK_MOUNTAIN,
    WARIO_COLOSSEUM, DINO_DINO_JUNGLE, BOWSER_CASTLE, RAINBOW_ROAD
};

const u64 SceneGhostLoadSave::Card::mscaPaneTag[] = {
    'MeSlot_A', 'MemB_A', 'MemTxB_A', 'MemP_A', 
    'MmData_A', 'SlotWB_A', 'Slot_A', 'SlotW_A', 
    'NMmEf_A', 
};

SceneGhostLoadSave::GhostFileInfo::GhostFileInfo()
{
    mKartID = 0xff;
    mTime = 0x5C0000 + 0x8D7F;
}

s32 SceneGhostLoadSave::GhostFileInfoTable::append(s32 idx, u32 val, TARecord &record)
{
    if (mCount >= 127)
        return 0;

    GhostFileInfo *info = &mInfos[mCount++];
    info->mIndex = idx;
    info->mValue = val;
    const u8 *raw = (const u8 *)&record;
    info->mCharIDs = *(const u16 *)raw;
    info->mKartID = raw[2];
    info->mCourseID = raw[3];
    info->mName = *(const u32 *)(raw + 4);
    info->mTime = record.getRaceTime().get();

    s32 crsID = convCrsID(record.getCourseID());
    GhostFileInfo *cur = mHead;
    GhostFileInfo *prev = nullptr;
    GhostFileInfo **link = &mHead;

    while (cur != nullptr)
    {
        s32 curCrsID = convCrsID(cur->mCourseID);
        if (curCrsID > crsID)
            break;
        if (curCrsID == crsID)
        {
            if ((s32)cur->mTime > (s32)info->mTime)
                break;
            if (cur->mTime == info->mTime && cur->mValue > val)
                break;
        }
        prev = cur;
        link = &cur->mNext;
        cur = cur->mNext;
    }

    *link = info;
    info->mNext = cur;
    info->mPrev = prev;
    if (cur != nullptr)
        cur->mPrev = info;

    return 1;
}

s32 SceneGhostLoadSave::GhostFileInfoTable::convCrsID(u8 courseID)
{
    s32 i = 0;
    for (int n = 16; n != 0; n--, i++)
    {
        if (caRaceCrsID[i] == courseID)
            return i;
    }
    return i;
}

void SceneGhostLoadSave::Card::searchPane(u8, J2DScreen *) {}

void SceneGhostLoadSave::Card::setCardInfo()
{
    Kart2DCommon::ptr()->changeNumberTexture(mTable.mCount, &mPicturesA[0], 3, false, false);

    CardMgr::CardData &data = CardMgr::msaCardData[mCardId];
    int bytes = data.mByteNotUsed / 0x10000;
    int files = data.mFilesNotUsed;
    int *val = &bytes;
    if (files < bytes)
        val = &files;
    _a = *val;

    Kart2DCommon::ptr()->changeNumberTexture(_a, &mPicturesB[0], 3, false, false);

    switch (mspSceneGhostLoadSave->_253c)
    {
    case 1:
        if (mCardId != 0)
        {
            mState = 4;
            insertCard();
            unselect();
        }
        else
        {
            mState = 1;
            mError = 2;
            error();
        }
        break;
    case 2:
    case 3:
        if (mCardId != 0)
        {
            mState = 4;
            insertCard();
            unselect();
        }
        else if (bytes == 0)
        {
            mState = 1;
            mError = 6;
            error();
        }
        else if (files == 0)
        {
            mState = 1;
            mError = 5;
            error();
        }
        else
        {
            mState = 4;
            insertCard();
            unselect();
        }
        break;
    }
}

#pragma dont_inline on
void SceneGhostLoadSave::Card::process() { mSelectState = 0; }
#pragma dont_inline off

void SceneGhostLoadSave::Card::unselect()
{
    mPanes[8]->mIsVisible = false;
    mAnm[3]->setFrame(1.0f);
    mAnm[2]->setFrame(0.0f);
    mSelectState = 3;
}

void SceneGhostLoadSave::Card::insertCard()
{
    mPanes[4]->mIsVisible = true;
    mPanes[5]->mIsVisible = true;
    mPanes[7]->mIsVisible = true;
    mPanes[8]->mIsVisible = true;
    mAnm[2]->setFrame(0.0f);

    if (mspSceneGhostLoadSave->_253c == 1)
        mCourseText->mIsVisible = true;
    mInfoText->mIsVisible = false;
}

void SceneGhostLoadSave::Card::error()
{
    mPanes[4]->mIsVisible = false;
    mPanes[5]->mIsVisible = false;
    mPanes[7]->mIsVisible = false;
    mPanes[8]->mIsVisible = false;
    mCourseText->mIsVisible = false;
    mAnm[2]->setFrame(0.0f);

    u32 msgID;
    switch (mError)
    {
    case 0: msgID = 21; break;
    case 1: msgID = 22; break;
    case 2: msgID = 32; break;
    case 3: msgID = 23; break;
    case 4: msgID = 24; break;
    case 5: msgID = 28; break;
    case 6: msgID = 27; break;
    default:
        JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "SceneGhostLoadSave.cpp", 847, (const char *)0, "Card");
        OSPanic("SceneGhostLoadSave.cpp", 847, (const char *)0, "Card");
        break;
    }

    void *bmg = ResMgr::getPtr(ResMgr::mcCard);
    int bmgID = ReadPrintMessage::getBmgMessageID((PrintMemoryCard::MessageID)msgID);
    char *message = ReadPrintMessage::getMessage(bmg, bmgID);
    J2DTextBox *textBox = mInfoText;
    u16 len = *(u16 *)&textBox->mStringLength;
    char *str = textBox->getStringPtr();
    snprintf(str, len, message);
    textBox->mIsVisible = true;
    mSelectState = 1;
}

void SceneGhostLoadSave::Card::calcAnm() {}

const u64 SceneGhostLoadSave::QuitBtn::mscaPaneTag[] = {
    'NMemQuit', 'MemQuitB', 'MemQuitW', 'NMmEf_M', 
    'MemOK', 
};

void SceneGhostLoadSave::QuitBtn::calcAnm() {}

void SceneGhostLoadSave::SelectSlot::setup(JKRArchive *, JKRHeap *) {}

void SceneGhostLoadSave::SelectSlot::init() {}

void SceneGhostLoadSave::SelectSlot::processCard()
{
    for (u8 i = 0; i < 2; i++)
        mCards[i].process();
}

bool SceneGhostLoadSave::SelectSlot::watchCardState()
{
    bool ok = false;

    if (mQuitBtn.mSelecting == 0)
    {
        Card *card = &mCards[mQuitBtn.mSelectedIndex];
        if (card->mState != 5)
        {
            switch (card->mState)
            {
            case 0:
                card->mPanes[4]->mIsVisible = false;
                card->mPanes[5]->mIsVisible = false;
                card->mPanes[7]->mIsVisible = false;
                card->mPanes[8]->mIsVisible = false;
                card->mPanes[6]->mIsVisible = false;
                card->mInfoText->mIsVisible = false;
                card->mAnm[0]->setFrame(0.0f);
                card->mSelectState = 0;
                break;
            case 1:
            {
                card->mPanes[4]->mIsVisible = false;
                card->mPanes[5]->mIsVisible = false;
                card->mPanes[7]->mIsVisible = false;
                card->mPanes[8]->mIsVisible = false;
                card->mCourseText->mIsVisible = false;
                card->mAnm[0]->setFrame(0.0f);

                u32 msgID;
                void *bmg;
                int bmgID;
                char *message;
                switch (card->mError)
                {
                case 0: msgID = 21; break;
                case 1: msgID = 22; break;
                case 2: msgID = 32; break;
                case 3: msgID = 23; break;
                case 4: msgID = 24; break;
                case 5: msgID = 28; break;
                case 6: msgID = 27; break;
                default:
                    JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "SceneGhostLoadSave.cpp", 847, (const char *)0, "Card");
                    OSPanic("SceneGhostLoadSave.cpp", 847, (const char *)0, "Card");
                    break;
                }

                bmg = ResMgr::getPtr(ResMgr::mcCard);
                bmgID = ReadPrintMessage::getBmgMessageID((PrintMemoryCard::MessageID)msgID);
                message = ReadPrintMessage::getMessage(bmg, bmgID);
                snprintf(card->mInfoText->getStringPtr(), card->mInfoText->mStringLength, message);
                card->mInfoText->mIsVisible = true;
                card->mSelectState = 1;
                break;
            }
            case 4:
                card->mPanes[8]->mIsVisible = false;
                card->mAnm[3]->setFrame(1.0f);
                card->mAnm[2]->setFrame(0.0f);
                card->mSelectState = 3;
                break;
            default:
                break;
            }

            mQuitBtn.mAnimState = 0;
            mQuitBtn.mSelecting = 1;
            ok = true;
        }
    }

    return ok;
}

void SceneGhostLoadSave::SelectSlot::selectSlot() {}


const u64 SceneGhostLoadSave::GDInexLineVC::mscaPaneTag[] = {
    'NGDline', 'GDNew', 
};

void SceneGhostLoadSave::GDInexLineVC::setup(JKRArchive *, JKRHeap *, bool) {}

void SceneGhostLoadSave::GDInexLineVC::updateInfo() {}

void SceneGhostLoadSave::GDIndexLayoutVC::setup(JKRArchive *, JKRHeap *) {}

void SceneGhostLoadSave::GDIndexLayoutVC::setCard(SceneGhostLoadSave::Card *card)
{
    mCard = card;
    mCardCount = card->mTable.mCount;

    bool flag = false;
    if (mspSceneGhostLoadSave->_253c == 2 && card->_a != 0)
        flag = true;

    _3a8 = 0;
    _3a6 = 0;
    _3ac = mCardCount;
    if (flag)
        _3ac++;
    _3ab = (mLineCount < _3ac) ? mLineCount : _3ac;
    _3aa = _3ab * 4 + 7;

    GhostFileInfo *info = card->mTable.mHead;
    for (u8 i = 0; i < mLineCount; i++)
    {
        if (i == 0 && flag)
        {
            mLines[i].mInfo = nullptr;
            mLines[i].mAnimState = 3;
            mLines[i].mFrame = 0;
            mLines[i].updateInfo();
        }
        else
        {
            mLines[i].mInfo = info;
            mLines[i].mAnimState = 3;
            mLines[i].mFrame = 0;
            mLines[i].updateInfo();
            if (info != nullptr)
                info = info->mNext;
        }
    }

    if (_3b0 != 0)
    {
        mLines[6].mInfo = (GhostFileInfo *)&mspSceneGhostLoadSave->_251c;
        mLines[6].mAnimState = 3;
        mLines[6].mFrame = 0;
        mLines[6].updateInfo();
    }

    for (u8 i = 0; i < _3ab; i++)
    {
        mPanes[i]->mIsVisible = true;
        mPanes[i]->setAlpha(255);
    }

    for (u8 i = 0; i <= 5; i++)
    {
        mPanes[i]->mIsVisible = false;
        mPanes[i]->setAlpha(255);
    }

    u8 sel = mSelectedLine;
    if (flag)
        sel = mSelectedLine - 1;
    mPanes[6]->mIsVisible = false;
    if (mCardCount > sel)
        mPanes[7]->mIsVisible = true;
    else
        mPanes[7]->mIsVisible = false;

    mLines[_3a8].mAnimState = 0;
    mLines[_3a8].mFrame = -1;
}

s32 SceneGhostLoadSave::GDIndexLayoutVC::selectData()
{
    s32 ret = 0;

    if (mAnimState != 2)
        return 0;
    if (mCard->mState != 5)
        return 2;

    if (gGamePad1P.testRepeat(0x08000000))
    {
        _3a8--;
        if (_3a8 < 0)
        {
            _3a8 = 0;
            if (_3a6 > 0)
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20005);
                shiftUp();
                mLines[1].mAnimState = 2;
                mLines[1].mFrame = 8;
                mLines[0].mAnimState = 0;
                mLines[0].mFrame = -1;
                scrollUp();
            }
        }
        else
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20005);
            mLines[_3a8 + 1].mAnimState = 2;
            mLines[_3a8 + 1].mFrame = 8;
            mLines[_3a8].mAnimState = 0;
            mLines[_3a8].mFrame = -1;
        }

        _3a6--;
        if (_3a6 < 0)
            _3a6 = 0;
    }
    else if (gGamePad1P.testRepeat(0x04000000))
    {
        _3a8++;
        if (_3a8 >= _3ab)
        {
            _3a8 = _3ab - 1;
            if (_3a6 < _3aa - 1)
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20005);
                scrollDown();
                mLines[mSelectedLine - 1].mAnimState = 2;
                mLines[mSelectedLine - 1].mFrame = 8;
                mLines[mSelectedLine].mAnimState = 0;
                mLines[mSelectedLine].mFrame = -1;
            }
        }
        else
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20005);
            mLines[_3a8 - 1].mAnimState = 2;
            mLines[_3a8 - 1].mFrame = 8;
            mLines[_3a8].mAnimState = 0;
            mLines[_3a8].mFrame = -1;
        }

        _3a6++;
        if (_3a6 >= _3aa)
            _3a6 = _3aa - 1;
    }

    if (gGamePad1P.testTrigger(0x100))
        ret = 1;
    else if (gGamePad1P.testTrigger(0x200))
        ret = 2;

    return ret;
}

void SceneGhostLoadSave::GDIndexLayoutVC::scrollUp()
{
    mAnimState = 3;
    mFrame0 = 47;
    mPanes[mSelectedLine]->mIsVisible = true;
    mAlpha = 0;
}

void SceneGhostLoadSave::GDIndexLayoutVC::scrollDown()
{
    mAnimState = 4;
    mFrame0 = 39;
    mPanes[mSelectedLine]->mIsVisible = true;
    mAlpha = 0;
}

void SceneGhostLoadSave::GDIndexLayoutVC::shiftUp()
{
    for (int i = mLineCount - 1; i > 0; i--)
    {
        mLines[i].mInfo = mLines[i - 1].mInfo;
        mLines[i].mAnimState = 3;
        mLines[i].mFrame = 0;
        mLines[i].updateInfo();
    }

    mLines[0].mInfo = mLines[0].mInfo->mPrev;
    mLines[0].mAnimState = 3;
    mLines[0].mFrame = 0;
    mLines[0].updateInfo();
}

void SceneGhostLoadSave::GDIndexLayoutVC::calcAnm() {}

SceneGhostLoadSave::SceneGhostLoadSave(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap) {
    // SceneGhostLoadSave::Card::Card() {}
    // SceneGhostLoadSave::GhostFileInfo::GhostFileInfo() {}
}

void SceneGhostLoadSave::init() {}

SceneGhostLoadSave::~SceneGhostLoadSave() {}

void SceneGhostLoadSave::draw()
{
    mOrtho->setPort();
    ((J2DScreen *)MenuBackground::ptr())->draw(0.0f, 0.0f, mOrtho);
    MenuTitleLine::ptr()->draw(mOrtho);

    switch (_10)
    {
    case 1:
        mSelectSlot.mScreen->draw(0.0f, 0.0f, mOrtho);
        break;
    case 2:
        mGDIndex.mScreen->draw(0.0f, 0.0f, mOrtho);
        break;
    }
}

void SceneGhostLoadSave::calc() {}

void SceneGhostLoadSave::startFadeIn()
{
    mSelectSlot.frameIn();

    switch (_253c)
    {
    case 2:
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcGhostSave);
        break;
    case 1:
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcGhostLoad);
        break;
    case 3:
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcGhostCheck);
        break;
    }

    _c = 1;

    JUTFader *fader = System::mspDisplay->getFader();
    if (fader != nullptr)
        fader->startFadeIn(15);
}

void SceneGhostLoadSave::checkCard() {}

void SceneGhostLoadSave::fadeIn() {
    if (mSelectSlot.mQuitBtn.mState3c == 2) {
        _c = 4;
    }
}

void SceneGhostLoadSave::frameInSlot() {
    if (mSelectSlot.mQuitBtn.mState3c == 2) {
        _c = 4;
    }
}

void SceneGhostLoadSave::selectSlot() {}

void SceneGhostLoadSave::confirm()
{
    if (CardAgent::msState != 0)
        return;

    CardAgent::mspPrintMemoryCard->setStuff(0, 0, 0);

    if (CardAgent::msResult == 0)
    {
        mSelectSlot.mCards[0].mAnimState = 2;
        mSelectSlot.mCards[0].mFrame0 = 10;
        mSelectSlot.mCards[1].mAnimState = 2;
        mSelectSlot.mCards[1].mFrame0 = 10;
        mSelectSlot.mQuitBtn.mSelectState = 2;
        mSelectSlot.mQuitBtn.mFrame0 = 5;
        mSelectSlot.mQuitBtn.mState3c = 3;

        MenuTitleLine::ptr()->lift();

        _c = 10;
    }
    else
    {
        _c = 4;
    }
}

void SceneGhostLoadSave::frameOutSlot()
{
    if (mSelectSlot.mQuitBtn.mState3c != 0)
        return;

    _c = 5;
    _10 = 2;
    u32 offset = mSelectSlot.mQuitBtn.mSelectedIndex * sizeof(Card);
    mGDIndex.setCard((Card *)((u8 *)&mSelectSlot.mCards[0] + offset));
    mGDIndex.mAnimState = 1;
    mGDIndex.mFrame0 = -1;
}

void SceneGhostLoadSave::frameInData() {
    if (mGDIndex.mAnimState == 2) {
        _c = 7;
    }
}

void SceneGhostLoadSave::selectData()
{
    switch (mGDIndex.selectData())
    {
    case 2:
        GameAudio::Main::getAudio()->startSystemSe(0x20005);
        mGDIndex.mAnimState = 5;
        mGDIndex.mFrame0 = mGDIndex._3ac;
        _c = 6;
        break;
    case 1:
    {
        GhostFileInfo *info = mGDIndex.mLines[mGDIndex._3a8].mInfo;
        u8 chan = mSelectSlot.mQuitBtn.mSelectedIndex;

        if (_253c == 2)
        {
            if (info != nullptr)
            {
                gGhostFile.mAccessWay = 0;
                gGhostFile.mFileNo = info->mIndex;
                gGhostFile.setup();
                CardAgent::ask(CardAgent::mcCommand6, chan);
            }
            break;
        }

        gGhostFile.mAccessWay = 0;
        gGhostFile.mFileNo = info->mIndex;
        CardAgent::ask(CardAgent::mcCommand4, chan);
        _c = 8;
        break;
    }
    }
}

void SceneGhostLoadSave::ghostSaveLoad()
{
    if (CardAgent::msState != 0)
        return;

    CardAgent::mspPrintMemoryCard->setStuff(0, 0, 0);

    switch (CardAgent::msResult)
    {
    case 0:
        if (_253c == 1)
        {
            JUTFader *fader = System::mspDisplay->getFader();
            if (fader != nullptr)
                fader->startFadeOut(15);
            GameAudio::Main::getAudio()->fadeOutAll(15);
            _2518 = 1;
        }
        else
        {
            _2518 = 0;
        }
        mGDIndex.mAnimState = 5;
        mGDIndex.mFrame0 = mGDIndex._3ac;
        MenuTitleLine::ptr()->lift();
        _c = 10;
        break;
    case 1:
        if (_253c == 2)
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20005);
            mGDIndex.mAnimState = 5;
            mGDIndex.mFrame0 = mGDIndex._3ac;
            _c = 6;
            _2519 = 1;
            mSelectSlot.init();
        }
        else
        {
            _c = 7;
            _10 = 2;
        }
        break;
    default:
        _c = 7;
        _10 = 2;
        break;
    }
}

void SceneGhostLoadSave::frameOutData()
{
    if (mGDIndex.mAnimState != 0)
        return;

    if (_2519 != 0)
    {
        _c = 0;
        _10 = 1;
        _2519 = 0;
    }
    else
    {
        _c = 2;
        _10 = 1;
        mSelectSlot.frameIn();
    }
}

void SceneGhostLoadSave::nextScene()
{
    if (mSelectSlot.mQuitBtn.mState3c != 0)
        return;
    if (mGDIndex.mAnimState != 0)
        return;
    if (_2518 != 0 && System::mspDisplay->getFader()->getStatus() != 0)
        return;

    switch (_253c)
    {
    case 2:
        if (!SequenceApp::ptr()->ready(Scene::SCENE_COURSE))
            return;
        SequenceApp::msNextScene = Scene::SCENE_COURSE;
        SequenceApp::ptr()->setState(0);
        gSequenceInfo.mGhostFlags = 0;
        break;
    case 1:
        if (!SequenceApp::ptr()->checkFinishAllLoading())
            return;
        AppMgr::deleteCurrentApp();
        RaceApp::call();
        gSequenceInfo.mGhostFlags = 0;
        break;
    case 3:
        if (!SequenceApp::ptr()->ready(Scene::SCENE_MENU))
            return;
        SequenceApp::msNextScene = Scene::SCENE_MENU;
        SequenceApp::ptr()->setState(0);
        gSequenceInfo.mGhostFlags = 0;
        gSequenceInfo._30 = 0;
        break;
    }
}

void SceneGhostLoadSave::prevScene()
{
    if (mSelectSlot.mQuitBtn.mState3c != 0)
        return;
    if (mGDIndex.mAnimState != 0)
        return;
    if (_2518 != 0 && System::mspDisplay->getFader()->getStatus() != 0)
        return;

    switch (_253c)
    {
    case 1:
        if (!SequenceApp::ptr()->ready(Scene::SCENE_COURSE))
            return;
        SequenceApp::msNextScene = Scene::SCENE_COURSE;
        SequenceApp::ptr()->setState(0);
        gSequenceInfo.mGhostFlags = 0;
        break;
    case 3:
    {
        Scene::SceneType next = Scene::SCENE_MENU;
        if (!SequenceApp::ptr()->ready(Scene::SCENE_MENU))
            return;
        SequenceApp::msNextScene = next;
        SequenceApp::ptr()->setState(0);
        gSequenceInfo.mGhostFlags = 0;
        gSequenceInfo._30 = next;
        break;
    }
    }
}

void SceneGhostLoadSave::SelectSlot::frameIn()
{
    watchCardState();

    mCards[0].mAnimState = 1;
    mCards[0].mFrame0 = -1;
    mCards[1].mAnimState = 1;
    mCards[1].mFrame0 = -1;
    mQuitBtn.mSelectState = 1;
    mQuitBtn.mFrame0 = -1;
    mQuitBtn.mState3c = 1;
}

#include "JSystem/JAudio/JASFakeMatch2.h"
