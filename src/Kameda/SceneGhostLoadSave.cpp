#include "Kameda/SceneGhostLoadSave.h"

#include "Kameda/Scene.h"
#include "Kameda/SceneFactory.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel/JKRFileLoader.h"
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
#include "Kameda/CourseName2D.h"
#include "Kaneshige/RaceTime.h"
#include "kartLocale.h"
#include "Osako/ResMgr.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kameda/PrintMemoryCard.h"
#include "kartEnums.h"
#include "mathHelper.h"

SceneGhostLoadSave *SceneGhostLoadSave::mspSceneGhostLoadSave;

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
    u32 newTime = info->mTime;
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
            if ((s32)cur->mTime > (s32)newTime)
                break;
            if (cur->mTime == newTime && cur->mValue > val)
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

void SceneGhostLoadSave::Card::searchPane(u8 id, J2DScreen *screen)
{
    mCardId = id;

    for (int i = 0; i < 9; i++)
        mPanes[i] = screen->search(mscaPaneTag[i] + id);

    u64 tag = 0x4D656D4744315F41ULL;
    for (int i = 0; i < 3; i++)
    {
        J2DPicture *pPane = (J2DPicture *)screen->search(tag + id);
#line 243
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mPicturesA[i] = pPane;
        tag++;
    }

    tag = 0x4D656D4F53315F41ULL;
    for (int i = 0; i < 3; i++)
    {
        J2DPicture *pPane = (J2DPicture *)screen->search(tag + id);
#line 253
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mPicturesB[i] = pPane;
        tag++;
    }

    tag = 0x4D656D436F4E5F41ULL;
    mCourseText = (J2DTextBox *)screen->search(tag + id);
#line 260
    JUT_ASSERT(mCourseText->getTypeID() == J2DPaneType_Picture);

    tag = 0x000000546F745F41ULL;
    mInfoText = (J2DTextBox *)screen->search(tag + id);
#line 265
    JUT_ASSERT(mInfoText->getTypeID() == J2DPaneType_TextBox);

    mInfoText->setString(128, "");
}

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
        JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "SceneGhostLoadSave.cpp", 847, "%s", "Card");
OSPanic("SceneGhostLoadSave.cpp", 847, "Halt");
        break;
    }

    void *bmg = ResMgr::getPtr(ResMgr::mcCard);
    int bmgID = ReadPrintMessage::getBmgMessageID((PrintMemoryCard::MessageID)msgID);
    char *message = ReadPrintMessage::getMessage(bmg, bmgID);
    u16 len = *(u16 *)&mInfoText->mStringLength;
    char *str = mInfoText->getStringPtr();
    snprintf(str, len, message);
    mInfoText->mIsVisible = true;
    mSelectState = 1;
}

void SceneGhostLoadSave::Card::calcAnm()
{
    switch (mState)
    {
    case 0:
        if (mAnimState == 4)
        {
            mAnimState = 5;
            mFrame0 = 25;
        }
        if (mSelectState != 0)
        {
            mPanes[4]->mIsVisible = false;
            mPanes[5]->mIsVisible = false;
            mPanes[7]->mIsVisible = false;
            mPanes[8]->mIsVisible = false;
            mCourseText->mIsVisible = false;
            mInfoText->mIsVisible = false;
            mAnm[2]->setFrame(0.0f);
            mSelectState = 0;
        }
        break;
    case 1:
        if (mAnimState == 4)
        {
            mAnimState = 5;
            mFrame0 = 25;
        }
        if (mSelectState != 1)
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
                JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "SceneGhostLoadSave.cpp", 847, "%s", "Card");
OSPanic("SceneGhostLoadSave.cpp", 847, "Halt");
                break;
            }

            void *bmg = ResMgr::getPtr(ResMgr::mcCard);
            int bmgID = ReadPrintMessage::getBmgMessageID((PrintMemoryCard::MessageID)msgID);
            char *message = ReadPrintMessage::getMessage(bmg, bmgID);
            u16 len = *(u16 *)&mInfoText->mStringLength;
            char *str = mInfoText->getStringPtr();
            snprintf(str, len, message);
            mInfoText->mIsVisible = true;
            mSelectState = 1;
        }
        break;
    case 2:
    case 3:
    case 5:
        if (mAnimState == 6)
        {
            mAnimState = 3;
            mFrame0 = 20;
        }
        if (mSelectState != 2)
        {
            mPanes[8]->mIsVisible = true;
            mAnm[3]->setFrame(0.0f);
            mAnm[2]->setFrame(-0.0f);
            mSelectState = 2;
            mFrame1 = -1;
        }
        break;
    case 4:
        if (mAnimState == 4)
        {
            mAnimState = 5;
            mFrame0 = 25;
        }
        if (mSelectState != 3)
        {
            mPanes[8]->mIsVisible = false;
            mAnm[3]->setFrame(1.0f);
            mAnm[2]->setFrame(0.0f);
            mSelectState = 3;
        }
        break;
    }

    switch (mAnimState)
    {
    case 1:
        if (++mFrame0 >= 10)
        {
            mFrame0 = 20;
            mAnimState = 6;
        }
        break;
    case 2:
        if (--mFrame0 < 0)
        {
            mFrame0 = 0;
            mAnimState = 0;
        }
        break;
    case 3:
        if (++mFrame0 >= 25)
        {
            mFrame0 = 24;
            mAnimState = 4;
        }
        break;
    case 5:
        if (--mFrame0 < 20)
        {
            mFrame0 = 20;
            mAnimState = 6;
        }
        break;
    }

    mAnm[0]->setFrame((f32)mFrame0);

    switch (mSelectState)
    {
    case 0:
        mFrame2 = 0;
        mAnm[5]->setFrame((f32)mFrame2);
        break;
    case 1:
        if (++mFrame2 >= 360)
            mFrame2 = 0;
        mAnm[5]->setFrame((f32)mFrame2);
        break;
    case 2:
        if (++mFrame1 >= 360)
            mFrame1 = 0;
        if (++mFrame2 >= 360)
            mFrame2 = 0;
        if (++mFrame3 >= 120)
            mFrame3 = 0;
        mAnm[4]->setFrame((f32)mFrame1);
        mAnm[6]->setFrame((f32)mFrame3);
        mAnm[5]->setFrame((f32)mFrame2);
        break;
    case 3:
        if (++mFrame2 >= 360)
            mFrame2 = 0;
        mFrame1 = 30;
        mFrame3 = 0;
        mAnm[4]->setFrame((f32)mFrame1);
        mAnm[6]->setFrame((f32)mFrame3);
        mAnm[5]->setFrame((f32)mFrame2);
        break;
    }
}

const u64 SceneGhostLoadSave::QuitBtn::mscaPaneTag[] = {
    'NMemQuit', 'MemQuitB', 'MemQuitW', 'NMmEf_M', 
    'MemOK', 
};

void SceneGhostLoadSave::QuitBtn::calcAnm()
{
    switch (mAnimState)
    {
    case 0:
        if (mSelect != 0)
        {
            mSelect = 0;
            mFrame2 = -1;
            mPanes[3]->mIsVisible = true;
            mAnm[1]->setFrame(0.0f);
        }
        break;
    case 1:
        if (mSelect != 1)
        {
            mSelect = 1;
            mPanes[3]->mIsVisible = false;
            mAnm[1]->setFrame(1.0f);
        }
        break;
    }

    switch (mSelectState)
    {
    case 1:
        if (++mFrame0 >= 5)
        {
            mFrame0 = 4;
            mSelectState = 0;
        }
        break;
    case 2:
        if (--mFrame0 < 0)
        {
            mFrame0 = 0;
            mSelectState = 0;
        }
        break;
    }

    mAnm[0]->setFrame((f32)mFrame0);

    switch (mSelect)
    {
    case 0:
        if (++mFrame2 >= 360)
            mFrame2 = 0;
        if (++mFrame1 >= 120)
            mFrame1 = 0;
        mAnm[3]->setFrame((f32)mFrame2);
        mAnm[2]->setFrame((f32)mFrame1);
        break;
    case 1:
        mFrame2 = 30;
        mFrame1 = 0;
        mAnm[3]->setFrame((f32)mFrame2);
        mAnm[2]->setFrame((f32)mFrame1);
        break;
    }
}

#pragma dont_inline on
void SceneGhostLoadSave::SelectSlot::setup(JKRArchive *, JKRHeap *) {}
#pragma dont_inline off

#pragma dont_inline on
void SceneGhostLoadSave::SelectSlot::init() {}
#pragma dont_inline off

void SceneGhostLoadSave::SelectSlot::processCard()
{
    for (u8 i = 0; i < 2; i++)
        mCards[i].process();
}

bool SceneGhostLoadSave::SelectSlot::watchCardState()
{
    bool ok = false;
    void *bmg;
    u32 msgID;

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
                card->mCourseText->mIsVisible = false;
                card->mInfoText->mIsVisible = false;
                card->mAnm[2]->setFrame(0.0f);
                card->mSelectState = 0;
                break;
            case 1:
            {
                card->mPanes[4]->mIsVisible = false;
                card->mPanes[5]->mIsVisible = false;
                card->mPanes[7]->mIsVisible = false;
                card->mPanes[8]->mIsVisible = false;
                card->mCourseText->mIsVisible = false;
                card->mAnm[2]->setFrame(0.0f);

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
                    JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "SceneGhostLoadSave.cpp", 847, "%s", "Card");
OSPanic("SceneGhostLoadSave.cpp", 847, "Halt");
                    break;
                }

                bmg = ResMgr::getPtr(ResMgr::mcCard);
                bmgID = ReadPrintMessage::getBmgMessageID((PrintMemoryCard::MessageID)msgID);
                message = ReadPrintMessage::getMessage(bmg, bmgID);
                u16 len = *(u16 *)&card->mInfoText->mStringLength;
                char *str = card->mInfoText->getStringPtr();
                snprintf(str, len, message);
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

#pragma dont_inline on
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
#pragma dont_inline off


const u64 SceneGhostLoadSave::GDInexLineVC::mscaPaneTag[] = {
    'NGDline', 'GDNew', 
};

#pragma dont_inline on
void SceneGhostLoadSave::GDInexLineVC::setup(JKRArchive *, JKRHeap *, bool) {}
#pragma dont_inline off

void SceneGhostLoadSave::GDInexLineVC::updateInfo()
{
    if (mInfo != nullptr)
    {
        mPane0->mIsVisible = true;
        mPane1->mIsVisible = false;

        Kart2DCommon *common = Kart2DCommon::ptr();
        u8 num = *(u8 *)&mInfo->_0;
        if (num)
        {
            if (num < 10)
            {
                common->changeNumberTexture(num, &mPictures[0], 1, false, false);
                mPictures[1]->mIsVisible = false;
                mPictures[2]->mIsVisible = false;
                mPictures[3]->mIsVisible = false;
                mPictures[4]->mIsVisible = false;
                mPictures[5]->mIsVisible = false;
            }
            else if (num < 100)
            {
                common->changeNumberTexture(num, &mPictures[1], 2, false, false);
                mPictures[0]->mIsVisible = false;
                mPictures[3]->mIsVisible = false;
                mPictures[4]->mIsVisible = false;
                mPictures[5]->mIsVisible = false;
            }
            else
            {
                common->changeNumberTexture(num, &mPictures[3], 3, false, false);
                mPictures[0]->mIsVisible = false;
                mPictures[1]->mIsVisible = false;
                mPictures[2]->mIsVisible = false;
            }
        }

        mPictures[25]->changeTexture((ResTIMG *)CourseName2D::ptr()->getCourseNameTexture(
                                         (ECourseID)mInfo->mCourseID),
                                     0);

        int minutes;
        int seconds;
        int miliseconds;
        RaceTime raceTime(mInfo->mTime);
        raceTime.get(&minutes, &seconds, &miliseconds);
        common->changeNumberTexture(minutes, &mPictures[17], 2, false, false);
        common->changeNumberTexture(seconds, &mPictures[15], 2, true, false);
        common->changeNumberTexture(miliseconds, &mPictures[12], 3, true, false);
        common->changeStringTexture((const char *)&mInfo->mName, &mPictures[19], 3);

        u8 charID1 = ((u8 *)&mInfo->mCharIDs)[1];
        mPictures[22]->changeTexture(common->getCharIconTextureChecked((ECharID)charID1), 0);
        u8 charID0 = ((u8 *)&mInfo->mCharIDs)[0];
        mPictures[23]->changeTexture(common->getCharIconTextureChecked((ECharID)charID0), 0);
        mPictures[24]->changeTexture(common->getKartIconTexture((ECharID)mInfo->mKartID), 0);

        OSCalendarTime cal;
        OSTicksToCalendarTime((OSTime)mInfo->mValue * (OSTime)OS_TIMER_CLOCK, &cal);

        int yearPic;
        int monthPic;
        int dayPic;
        switch (KartLocale::getDatingMode())
        {
        case YYYY_MM_DD:
            yearPic = 10;
            monthPic = 8;
            dayPic = 6;
            break;
        case MM_DD_YYYY:
            monthPic = 10;
            dayPic = 8;
            yearPic = 6;
            break;
        case DD_MM_YYYY:
            dayPic = 10;
            monthPic = 8;
            yearPic = 6;
            break;
        }
        common->changeNumberTexture(cal.year, &mPictures[yearPic], 2, true, false);
        common->changeNumberTexture(cal.mon + 1, &mPictures[monthPic], 2, true, false);
        common->changeNumberTexture(cal.mday, &mPictures[dayPic], 2, true, false);
    }
    else
    {
        mPane0->mIsVisible = false;
        mPane1->mIsVisible = true;
    }
}

void SceneGhostLoadSave::GDIndexLayoutVC::setup(JKRArchive *archive, JKRHeap *heap)
{
    mScreen = new (heap, 0) J2DScreen();

    char buf[64];
    snprintf(buf, sizeof(buf), "%s.blo", scpaBaseFileName[2]);
    mScreen->set(buf, 0x20000, archive);

    snprintf(buf, sizeof(buf), "%s.bck", scpaBaseFileName[2]);
    void *bck = JKRFileLoader::getGlbResource(buf, archive);
    snprintf(buf, sizeof(buf), "%s.brk", scpaBaseFileName[2]);
    void *brk = JKRFileLoader::getGlbResource(buf, archive);

    u64 tag = 0x0000004E756D3030ULL;
    for (int i = 0; i <= 5; i++)
    {
        mPanes[i] = mScreen->search(tag);
        tag++;
    }
    tag = 0x4D4172726F773031ULL;
    for (int i = 6; i <= 7; i++)
    {
        mPanes[i] = mScreen->search(tag);
        tag++;
    }

    mPane = mScreen->search(0x004E536176654744ULL);

    mAnm[0] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mAnm[1] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mAnm[2] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(brk);

    mScreen->setAnimation(mAnm[0]);
    mScreen->setAnimation((J2DAnmTevRegKey *)mAnm[2]);
    mPanes[6]->setAnimation(mAnm[1]);
    mPanes[7]->setAnimation(mAnm[1]);

    for (int i = 0; i < 6; i++)
    {
        mLines[i].setup(archive, heap, false);
        mPanes[i]->appendChild(mLines[i].mScreen);
    }

    mLines[6].setup(archive, heap, true);
    J2DPane *savePane = mScreen->search(0x00004E5361766547ULL);
    savePane->appendChild(mLines[6].mScreen);
}

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
    int i = mLineCount - 1;
    for (; i > 0; i--)
    {
        mLines[i].mInfo = mLines[i - 1].mInfo;
        mLines[i].mAnimState = 3;
        mLines[i].mFrame = 0;
        mLines[i].updateInfo();
    }

    mLines[i].mInfo = mLines[i].mInfo->mPrev;
    mLines[i].mAnimState = 3;
    mLines[i].mFrame = 0;
    mLines[i].updateInfo();
}

void SceneGhostLoadSave::GDIndexLayoutVC::calcAnm()
{
    if (++mFrame1 >= 35)
        mFrame1 = 0;
    mAnm[1]->setFrame((f32)(mFrame1 + _3af));

    if (++mFrame2 >= 80)
        mFrame2 = 0;
    mAnm[2]->setFrame((f32)mFrame2);

    switch (mAnimState)
    {
    case 1:
        if (++mFrame0 >= _3ac)
        {
            mFrame0 = _3ac;
            mAnimState = 2;
        }
        mAnm[0]->setFrame((f32)(mFrame0 + _3af));
        break;
    case 5:
        if (--mFrame0 < 0)
        {
            mFrame0 = 0;
            mAnimState = 0;
        }
        mAnm[0]->setFrame((f32)(mFrame0 + _3af));
        break;
    case 3:
        if (mAlpha <= 224)
        {
            mPanes[0]->setAlpha(mAlpha);
            mPanes[mSelectedLine]->setAlpha(255 - mAlpha);
            mAlpha += 32;
            if (mAlpha < 32)
                mAlpha = 255;
        }
        if (--mFrame0 < 40)
        {
            mFrame0 = 39;
            mAnimState = 2;
            mPanes[mSelectedLine]->mIsVisible = false;
            mPanes[0]->setAlpha(255);
            if (_3a6 == 0)
                mPanes[6]->mIsVisible = false;
            mPanes[7]->mIsVisible = true;
        }
        mAnm[0]->setFrame((f32)(mFrame0 + _3af));
        break;
    case 4:
        if (mAlpha <= 224)
        {
            mPanes[0]->setAlpha(255 - mAlpha);
            mPanes[mSelectedLine]->setAlpha(mAlpha);
            mAlpha += 32;
            if (mAlpha < 32)
                mAlpha = 255;
        }
        if (++mFrame0 > 46)
        {
            mFrame0 = 39;
            mAnimState = 2;

            int i = 0;
            while (i < mLineCount - 1)
            {
                GDInexLineVC *line = &mLines[i];
                GDInexLineVC *next = &mLines[i + 1];
                line->mInfo = next->mInfo;
                line->mAnimState = 3;
                line->mFrame = 0;
                line->updateInfo();
                i++;
            }

            GDInexLineVC *tail = &mLines[i];
            tail->mInfo = tail->mInfo->mNext;
            tail->mAnimState = 3;
            tail->mFrame = 0;
            tail->updateInfo();

            GDInexLineVC *prev = &mLines[mSelectedLine - 1];
            prev->mAnimState = 1;
            prev->mFrame = 7;
            GDInexLineVC *cur = &mLines[mSelectedLine];
            cur->mAnimState = 3;
            cur->mFrame = 0;

            mPanes[0]->setAlpha(255);
            mPanes[mSelectedLine]->mIsVisible = false;
            if (_3a6 == _3aa - 1)
                mPanes[7]->mIsVisible = false;
            mPanes[6]->mIsVisible = true;
        }
        mAnm[0]->setFrame((f32)(mFrame0 + _3af));
        break;
    default:
        break;
    }

    for (u8 i = 0; i < 6; i++)
    {
        switch (mLines[i].mAnimState)
        {
        case 0:
            if (++mLines[i].mFrame >= 7)
                mLines[i].mFrame = 7;
            break;
        case 2:
            if (--mLines[i].mFrame <= 0)
                mLines[i].mFrame = 0;
            break;
        default:
            break;
        }
        mLines[i].mAnim->setFrame((f32)mLines[i].mFrame);
    }

    mScreen->animation();
}

SceneGhostLoadSave::SceneGhostLoadSave(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap)
{
    _252a = 0xff;
    _2530 = MAX_TIME;
    mspSceneGhostLoadSave = this;
    MenuBackground::create(SceneFactory::ptr()->getArchive(9));
    MenuTitleLine::create(SceneFactory::ptr()->getArchive(8), nullptr);
    mSelectSlot.setup(archive, heap);
    mGDIndex.setup(archive, heap);
}

void SceneGhostLoadSave::init() {}

SceneGhostLoadSave::~SceneGhostLoadSave()
{
    mspSceneGhostLoadSave = nullptr;
}

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
    GhostFileInfo *info;
    u8 chan;
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
        info = mGDIndex.mLines[mGDIndex._3a8].mInfo;
        chan = mSelectSlot.mQuitBtn.mSelectedIndex;

        switch (_253c)
        {
        case 2:
            if (info != nullptr)
            {
                gGhostFile.mAccessWay = 0;
                gGhostFile.mFileNo = info->mIndex;
                gGhostFile.setup();
                CardAgent::ask(CardAgent::mcCommand6, chan);
            }
            else
            {
                gGhostFile.setup();
                CardAgent::ask(CardAgent::mcCommand5, chan);
            }
            break;
        case 1:
            gGhostFile.mAccessWay = 0;
            gGhostFile.mFileNo = info->mIndex;
            CardAgent::ask(CardAgent::mcCommand4, chan);
            _c = 8;
            break;
        }
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
            break;
        }
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
        if (!SequenceApp::ptr()->ready(Scene::SCENE_MENU))
            return;
        SequenceApp::msNextScene = Scene::SCENE_MENU;
        SequenceApp::ptr()->setState(0);
        gSequenceInfo.mGhostFlags = 0;
        gSequenceInfo._30 = Scene::SCENE_MENU;
        break;
    }
}


#include "JSystem/JAudio/JASFakeMatch2.h"
