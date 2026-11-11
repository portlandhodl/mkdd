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
#include "Osako/SystemFile.h"
#include <dolphin/dvd.h>
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
    if (mCount < 127)
    {
        GhostFileInfo *info = &mInfos[mCount++];
        GhostFileInfo **link = &mHead;
        GhostFileInfo *prev = nullptr;
        info->mIndex = idx;
        info->mValue = val;
        info->mRecord = record;

        int newTime = record.getRaceTime().get();
        GhostFileInfo *cur = mHead;
        s32 crsID = convCrsID(record.getCourseID());

        while (cur != nullptr)
        {
            s32 curCrsID = convCrsID(cur->mRecord.getCourseID());
            if (curCrsID > crsID)
                break;
            if (curCrsID == crsID)
            {
                if (cur->mRecord.getRaceTime().get() > newTime)
                    break;
                if (newTime == cur->mRecord.getRaceTime().get() && cur->mValue > val)
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
    return 0;
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
    int i;
    mCardId = id;

    for (i = 0; i < 9; i++)
        mPanes[i] = screen->search(mscaPaneTag[i] + mCardId);

    u64 tag = 'MemGD1_A';
    for (i = 0; i < 3; i++)
    {
        J2DPicture *pPane = (J2DPicture *)screen->search(tag + id);
#line 243
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mPicturesA[i] = pPane;
        tag += 0x10000;
    }

    tag = 'MemOS1_A';
    for (i = 0; i < 3; i++)
    {
        J2DPicture *pPane = (J2DPicture *)screen->search(tag + id);
#line 253
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mPicturesB[i] = pPane;
        tag += 0x10000;
    }

    {
        J2DPicture *pPane = (J2DPicture *)screen->search('MemCoN_A' + id);
#line 260
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mCourseText = pPane;
    }

    {
        J2DTextBox *pPane = (J2DTextBox *)screen->search('TSlot_A' + id);
#line 265
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_TextBox);
        mInfoText = pPane;
    }

    mInfoText->setString(128, "");
}

void SceneGhostLoadSave::Card::setCardInfo()
{
    Kart2DCommon::ptr()->changeNumberTexture(mTable.mCount, &mPicturesA[0], 3, false, false);

    int bytes = CardMgr::msaCardData[mCardId].mByteNotUsed / 0xa000;
    int files = CardMgr::msaCardData[mCardId].mFilesNotUsed;
    int *val = (files < bytes) ? &files : &bytes;
    _a = *val;

    Kart2DCommon::ptr()->changeNumberTexture(_a, &mPicturesB[0], 3, false, false);

    switch (mspSceneGhostLoadSave->_253c)
    {
    case 1:
        if (mTable.mCount == 0)
        {
            mState = 1;
            mError = 2;
            error();
        }
        else
        {
            mState = 4;
            insertCard();
            unselect();
        }
        break;
    case 2:
    case 3:
        if (mTable.mCount == 0)
        {
            if (bytes == 0)
            {
                mState = 1;
                mError = 6;
                error();
                break;
            }
            if (files == 0)
            {
                mState = 1;
                mError = 5;
                error();
                break;
            }
        }
        mState = 4;
        insertCard();
        unselect();
        break;
    }
}

inline void SceneGhostLoadSave::GhostFileInfoTable::init()
{
    mCount = 0;
    mHead = nullptr;
    for (int i = 0; i < 127; i++)
    {
        mInfos[i].mPrev = nullptr;
        mInfos[i].mNext = nullptr;
    }
}

inline void SceneGhostLoadSave::GhostFileInfoTable::numbering()
{
    GhostFileInfo *info = mHead;
    u8 no = 1;
    while (info != nullptr)
    {
        info->_0 = no++;
        info = info->mNext;
    }
}

inline void SceneGhostLoadSave::Card::probe()
{
    switch (CardMgr::msaCardData[mCardId].mProbeStatus)
    {
    case CARD_RESULT_READY:
        if (mState == 0)
        {
            if (CardMgr::msaCardData[mCardId].mSectorSize == CARD_SYSTEM_BLOCK_SIZE)
            {
                if (CardMgr::mount(mCardId))
                {
                    mState = 2;
                }
                else
                {
                    mState = 1;
                    mError = 3;
                }
            }
            else
            {
                mState = 1;
                mError = 1;
            }
        }
        break;
    case CARD_RESULT_BUSY:
        break;
    case CARD_RESULT_NOCARD:
        mState = 0;
        break;
    case CARD_RESULT_WRONGDEVICE:
        mState = 1;
        mError = 0;
        break;
    }
}

inline void SceneGhostLoadSave::Card::mounting()
{
    s32 chan = mCardId;
    CardMgr::TaskStatus taskStatus = CardMgr::msaCardData[chan].mTaskStatus;
    if (taskStatus != CardMgr::mcTaskDone)
        return;
    if (taskStatus == CardMgr::mcTaskDone)
        CardMgr::msaCardData[chan].mTaskStatus = CardMgr::mcNoTask;
    {
        switch (CardMgr::msaCardData[mCardId].mCardStatus)
        {
        case CARD_RESULT_READY:
        case CARD_RESULT_BROKEN:
            CardMgr::check(mCardId);
            mState = 3;
            break;
        case CARD_RESULT_ENCODING:
            mState = 1;
            mError = 4;
            break;
        case CARD_RESULT_IOERROR:
            mState = 1;
            mError = 3;
            break;
        case CARD_RESULT_NOCARD:
            mState = 0;
            break;
        case CARD_RESULT_WRONGDEVICE:
            mState = 1;
            mError = 0;
            break;
        }
    }
}

inline void SceneGhostLoadSave::Card::searchGhost()
{
    mTable.init();
    DVDDiskID *diskID = DVDGetCurrentDiskID();
    for (s32 i = 0; i < 127; i++)
    {
        CARDStat stat;
        switch (CARDGetStatus(mCardId, i, &stat))
        {
        case CARD_RESULT_READY:
            if (memcmp(diskID, stat.gameName, 4) == 0 && memcmp(diskID->company, stat.company, 2) == 0 &&
                strncmp(SystemFile::mspFileNameString, stat.fileName, 32) != 0)
            {
                GIRecord record;
                if (GhostFile::analysisFileName(&record, stat.fileName))
                {
                    if (mspSceneGhostLoadSave->_253c != 1 || ResMgr::msCourseID == record.getRecord().getCourseID())
                        mTable.append(i, stat.time, record.getRecord());
                }
            }
            break;
        case CARD_RESULT_BUSY:
        case CARD_RESULT_WRONGDEVICE:
            continue;
        }
    }
    mTable.numbering();
    setCardInfo();
}

inline void SceneGhostLoadSave::Card::checking()
{
    s32 chan = mCardId;
    CardMgr::TaskStatus taskStatus = CardMgr::msaCardData[chan].mTaskStatus;
    if (taskStatus != CardMgr::mcTaskDone)
        return;
    if (taskStatus == CardMgr::mcTaskDone)
        CardMgr::msaCardData[chan].mTaskStatus = CardMgr::mcNoTask;
    {
        switch (CardMgr::msaCardData[mCardId].mCardStatus)
        {
        case CARD_RESULT_READY:
            searchGhost();
            break;
        case CARD_RESULT_ENCODING:
        case CARD_RESULT_BROKEN:
            mState = 1;
            mError = 4;
            break;
        case CARD_RESULT_IOERROR:
            mState = 1;
            mError = 3;
            break;
        case CARD_RESULT_NOCARD:
            mState = 0;
            break;
        case CARD_RESULT_WRONGDEVICE:
            mState = 1;
            mError = 0;
            break;
        }
    }
}

void SceneGhostLoadSave::Card::process()
{
    probe();
    switch (mState)
    {
    case 2:
        mounting();
        break;
    case 3:
        checking();
        break;
    }
}

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
    mAnm[2]->setFrame(1.0f);

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
#line 847
        JUT_ASSERT(0);
        break;
    }

    u16 len;
    void *bmg = ResMgr::getPtr(ResMgr::mcCard);
    int bmgID = ReadPrintMessage::getBmgMessageID((PrintMemoryCard::MessageID)msgID);
    char *message = ReadPrintMessage::getMessage(bmg, bmgID);
    len = *(u16 *)&mInfoText->mStringLength;
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
            char *message = ReadPrintMessage::getMessage(bmg, ReadPrintMessage::getBmgMessageID((PrintMemoryCard::MessageID)msgID));
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

inline void SceneGhostLoadSave::Card::setBck(void *bck, J2DAnmTransform *anm)
{
    mAnm[0] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mPanes[0]->setAnimation(mAnm[0]);
    mAnm[1] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mPanes[4]->setAnimation(mAnm[1]);
    mPanes[8]->setAnimation(anm);
}

inline void SceneGhostLoadSave::Card::setBpk(void *bpk, J2DScreen *screen)
{
    mAnm[4] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bpk);
    mAnm[4]->searchUpdateMaterialID(screen);
    mPanes[5]->setAnimation((J2DAnmColor *)mAnm[4]);
}

inline void SceneGhostLoadSave::Card::setBtk(void *btk, J2DScreen *screen)
{
    mAnm[5] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(btk);
    mAnm[5]->searchUpdateMaterialID(screen);
    mAnm[6] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(btk);
    mAnm[6]->searchUpdateMaterialID(screen);
    mPanes[3]->setAnimation((J2DAnmTextureSRTKey *)mAnm[5]);
    mPanes[7]->setAnimation((J2DAnmTextureSRTKey *)mAnm[6]);
}

inline void SceneGhostLoadSave::Card::setBrk(void *brk, J2DScreen *screen)
{
    mAnm[2] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(brk);
    mAnm[2]->searchUpdateMaterialID(screen);
    mAnm[3] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(brk);
    mAnm[3]->searchUpdateMaterialID(screen);
    mPanes[1]->setAnimation((J2DAnmTevRegKey *)mAnm[2]);
    mPanes[2]->setAnimation((J2DAnmTevRegKey *)mAnm[2]);
    mPanes[3]->setAnimation((J2DAnmTevRegKey *)mAnm[2]);
    mPanes[6]->setAnimation((J2DAnmTevRegKey *)mAnm[2]);
    mPanes[7]->setAnimation((J2DAnmTevRegKey *)mAnm[3]);
}

inline void SceneGhostLoadSave::QuitBtn::searchPane(J2DScreen *screen)
{
    for (int i = 0; i < 5; i++)
        mPanes[i] = screen->search(mscaPaneTag[i]);

    J2DPicture *pPane = (J2DPicture *)screen->search('MemQuit');
#line 1023
    JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
    mMenuPane = pPane;
}

inline void SceneGhostLoadSave::QuitBtn::setBck(void *bck, J2DAnmTransform *anm)
{
    mAnm[0] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);
    mPanes[0]->setAnimation(mAnm[0]);
    mPanes[3]->setAnimation(anm);
}

inline void SceneGhostLoadSave::QuitBtn::setBpk(void *bpk, J2DScreen *screen)
{
    mAnm[3] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bpk);
    mAnm[3]->searchUpdateMaterialID(screen);
    mPanes[1]->setAnimation((J2DAnmColor *)mAnm[3]);
}

inline void SceneGhostLoadSave::QuitBtn::setBtk(void *btk, J2DScreen *screen)
{
    mAnm[2] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(btk);
    mAnm[2]->searchUpdateMaterialID(screen);
    mPanes[2]->setAnimation((J2DAnmTextureSRTKey *)mAnm[2]);
}

inline void SceneGhostLoadSave::QuitBtn::setBrk(void *brk, J2DScreen *screen)
{
    mAnm[1] = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(brk);
    mAnm[1]->searchUpdateMaterialID(screen);
    mPanes[2]->setAnimation((J2DAnmTevRegKey *)mAnm[1]);
}

void SceneGhostLoadSave::SelectSlot::setup(JKRArchive *archive, JKRHeap *heap)
{
    mScreen = new (heap, 0) J2DScreen();

    char buf[64];
    snprintf(buf, sizeof(buf), "%s.blo", scpaBaseFileName[1]);
    mScreen->set(buf, 0x20000, archive);

    snprintf(buf, sizeof(buf), "%s.bck", scpaBaseFileName[1]);
    void *bck = JKRFileLoader::getGlbResource(buf, archive);
    snprintf(buf, sizeof(buf), "%s.bpk", scpaBaseFileName[1]);
    void *bpk = JKRFileLoader::getGlbResource(buf, archive);
    snprintf(buf, sizeof(buf), "%s.btk", scpaBaseFileName[1]);
    void *btk = JKRFileLoader::getGlbResource(buf, archive);
    snprintf(buf, sizeof(buf), "%s.brk", scpaBaseFileName[1]);
    void *brk = JKRFileLoader::getGlbResource(buf, archive);

    mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(bck);

    for (u8 i = 0; i < 2; i++)
    {
        mCards[i].searchPane(i, mScreen);
        mCards[i].setBck(bck, mAnm);
        mCards[i].setBpk(bpk, mScreen);
        mCards[i].setBtk(btk, mScreen);
        mCards[i].setBrk(brk, mScreen);
    }

    mQuitBtn.searchPane(mScreen);
    mQuitBtn.setBck(bck, mAnm);
    mQuitBtn.setBpk(bpk, mScreen);
    mQuitBtn.setBtk(btk, mScreen);
    mQuitBtn.setBrk(brk, mScreen);
}

inline void SceneGhostLoadSave::Card::noCard()
{
    mPanes[4]->mIsVisible = false;
    mPanes[5]->mIsVisible = false;
    mPanes[7]->mIsVisible = false;
    mPanes[8]->mIsVisible = false;
    mCourseText->mIsVisible = false;
    mInfoText->mIsVisible = false;
    mAnm[2]->setFrame(2.0f);
    mSelectState = 0;
}

inline void SceneGhostLoadSave::Card::init()
{
    switch (mspSceneGhostLoadSave->_253c)
    {
    case 1:
        mAnm[1]->setFrame(10.0f);
        mCourseText->changeTexture((ResTIMG *)CourseName2D::ptr()->getCourseNameTexture(ResMgr::msCourseID), 0);
        mCourseText->mIsVisible = true;
        break;
    case 2:
    case 3:
        mAnm[1]->setFrame(11.0f);
        mCourseText->mIsVisible = false;
        break;
    }
    mState = 0;
    mAnimState = 0;
    noCard();
    mFrame0 = 0;
    mFrame1 = -1;
    mFrame3 = -1;
}

inline void SceneGhostLoadSave::QuitBtn::init()
{
    switch (mspSceneGhostLoadSave->_253c)
    {
    case 1:
        mMenuPane->changeTexture("Mozi_No_Load.bti", 0);
        break;
    case 2:
        mMenuPane->changeTexture("Mozi_No_Save.bti", 0);
        break;
    }
    if (mspSceneGhostLoadSave->_253c == 3)
    {
        mMenuPane->mIsVisible = false;
        mPanes[4]->mIsVisible = true;
    }
    else
    {
        mMenuPane->mIsVisible = true;
        mPanes[4]->mIsVisible = false;
    }
    mAnimState = 0;
    mFrame0 = -1;
    mFrame2 = -1;
    mFrame1 = 0;
}

void SceneGhostLoadSave::SelectSlot::init()
{
    mCards[0].init();
    mCards[1].init();
    mState = 0;
    mFrame = -1;
    mSelecting = 1;
    mSelectedIndex = 0;
    mQuitBtn.init();
}

#pragma dont_inline on
void SceneGhostLoadSave::SelectSlot::processCard()
{
    for (u8 i = 0; i < 2; i++)
        mCards[i].process();
}
#pragma dont_inline off

bool SceneGhostLoadSave::SelectSlot::watchCardState()
{
    bool ok = false;
    void *bmg;
    u32 msgID;

    if (mSelecting == 0)
    {
        Card *card = &mCards[mSelectedIndex];
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
            mSelecting = 1;
            ok = true;
        }
    }

    return ok;
}

inline bool SceneGhostLoadSave::Card::selectable()
{
    switch (mspSceneGhostLoadSave->_253c)
    {
    case 1:
        if (mTable.mCount == 0)
            return false;
    case 2:
        if (mState != 4)
            return false;
        break;
    case 3:
        return false;
    }
    return true;
}

inline void SceneGhostLoadSave::Card::selectIn()
{
    mState = 5;
}

inline void SceneGhostLoadSave::Card::selectOut()
{
    mState = 4;
}

int SceneGhostLoadSave::SelectSlot::selectSlot()
{
    int ret = 0;

    if (watchCardState())
        return 0;

    if (gGamePad1P.testTrigger(JUTGamePad::MAINSTICK_UP))
    {
        if (mSelecting == 1)
        {
            if (mCards[mSelectedIndex].selectable())
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20000);
                mSelecting = 0;
                mCards[mSelectedIndex].selectIn();
                mQuitBtn.mAnimState = 1;
            }
            else if (mCards[1 - mSelectedIndex].selectable())
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20000);
                mSelecting = 0;
                mCards[1 - mSelectedIndex].selectIn();
                mQuitBtn.mAnimState = 1;
                mSelectedIndex = 1 - mSelectedIndex;
            }
        }
    }
    else if (gGamePad1P.testTrigger(JUTGamePad::MAINSTICK_DOWN))
    {
        if (mSelecting == 0)
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            mSelecting = 1;
            mCards[mSelectedIndex].selectOut();
            mQuitBtn.mAnimState = 0;
        }
    }
    else if (gGamePad1P.testTrigger(JUTGamePad::MAINSTICK_LEFT))
    {
        if (mSelecting == 1)
        {
            if (mCards[0].selectable())
            {
                GameAudio::Main::getAudio()->startSystemSe(0x2000c);
                mSelecting = 0;
                mSelectedIndex = 0;
                mCards[0].mState = 5;
                mQuitBtn.mAnimState = 1;
            }
        }
        else if (mSelectedIndex == 1)
        {
            if (mCards[0].selectable())
            {
                GameAudio::Main::getAudio()->startSystemSe(0x2000c);
                mSelectedIndex = 0;
                mCards[0].mState = 5;
                mCards[1].mState = 4;
            }
        }
    }
    else if (gGamePad1P.testTrigger(JUTGamePad::MAINSTICK_RIGHT))
    {
        if (mSelecting == 1)
        {
            if (mCards[1].selectable())
            {
                GameAudio::Main::getAudio()->startSystemSe(0x2000c);
                mSelecting = 0;
                mSelectedIndex = 1;
                mCards[1].mState = 5;
                mQuitBtn.mAnimState = 1;
            }
        }
        else if (mSelectedIndex == 0)
        {
            if (mCards[1].selectable())
            {
                GameAudio::Main::getAudio()->startSystemSe(0x2000c);
                mSelectedIndex = 1;
                mCards[1].mState = 5;
                mCards[0].mState = 4;
            }
        }
    }

    if (gGamePad1P.testTrigger(JUTGamePad::A))
    {
        if (mSelecting == 0)
            ret = 1;
        else
            ret = 3;
    }
    else if (gGamePad1P.testTrigger(JUTGamePad::B))
    {
        ret = 2;
    }

    return ret;
}

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
    mState = 1;
}
#pragma dont_inline off


const u64 SceneGhostLoadSave::GDInexLineVC::mscaPaneTag[] = {
    'NGDline', 'GDNew', 
};

void SceneGhostLoadSave::GDInexLineVC::setup(JKRArchive *archive, JKRHeap *heap, bool hide)
{
    int i;
    J2DPicture *pPane;
    mScreen = new (heap, 0) J2DScreen();

    char buf[64];
    snprintf(buf, sizeof(buf), "%s.blo", scpaBaseFileName[3]);
    mScreen->set(buf, 0x20000, archive);

    snprintf(buf, sizeof(buf), "%s.bck", scpaBaseFileName[3]);
    mAnim = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource(buf, archive));
    mScreen->setAnimation(mAnim);

    for (i = 0; i < 2; i++)
        mPanes[i] = mScreen->search(mscaPaneTag[i]);

    int n = 0;
    u64 tag = 'GDNum11';
    for (u8 digits = 1; digits <= 3; digits++)
    {
        for (u8 i = 0; i < digits; i++)
        {
            pPane = (J2DPicture *)mScreen->search(tag + i);
#line 1674
            JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
            mPictures[n++] = pPane;
            if (hide)
                pPane->mIsVisible = false;
        }
        tag += 0x100;
    }

    tag = 'GDName03';
    for (i = 19; i <= 21; i++)
    {
        pPane = (J2DPicture *)mScreen->search(tag--);
#line 1685
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mPictures[i] = pPane;
    }

    tag = 'GDtime01';
    for (i = 6; i <= 11; i++)
    {
        pPane = (J2DPicture *)mScreen->search(tag++);
#line 1693
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mPictures[i] = pPane;
    }

    tag = 'GDT00';
    for (i = 12; i <= 18; i++)
    {
        pPane = (J2DPicture *)mScreen->search(tag++);
#line 1701
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mPictures[i] = pPane;
    }

    tag = 'GDCha01';
    for (i = 22; i <= 23; i++)
    {
        pPane = (J2DPicture *)mScreen->search(tag++);
#line 1709
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mPictures[i] = pPane;
    }

    {
        pPane = (J2DPicture *)mScreen->search('GDKart');
#line 1714
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mPictures[24] = pPane;
    }
    {
        pPane = (J2DPicture *)mScreen->search('GDCoName');
#line 1718
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        mPictures[25] = pPane;
    }

    Kart2DCommon *common = Kart2DCommon::ptr();
    tag = 'GDChaW01';
    for (i = 0; i < 3; i++)
    {
        pPane = (J2DPicture *)mScreen->search(tag++);
#line 1729
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        pPane->changeTexture(common->getPartName(1), 0);
    }

    tag = 'GDTt01';
    {
        pPane = (J2DPicture *)mScreen->search(tag++);
#line 1735
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        pPane->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, "timg/MarioFontColon2.bti"), 0);
    }
    {
        pPane = (J2DPicture *)mScreen->search(tag);
#line 1738
        JUT_ASSERT(pPane->getTypeID() == J2DPaneType_Picture);
        pPane->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, "timg/MarioFontColon1.bti"), 0);
    }

    bool loop;
    J2DPane *pane = mScreen;
    while (pane != nullptr)
    {
        pane->setInfluencedAlpha(true, false);
        if (pane->getFirstChildPane() != nullptr)
        {
            pane = pane->getFirstChildPane();
        }
        else if (pane->getNextChildPane() != nullptr)
        {
            pane = pane->getNextChildPane();
        }
        else
        {
            loop = true;
            do
            {
                pane = pane->getParentPane();
                if (pane == nullptr)
                {
                    loop = false;
                }
                else if (pane->getNextChildPane() != nullptr)
                {
                    pane = pane->getNextChildPane();
                    loop = false;
                }
            } while (loop);
        }
    }

    mInfo = nullptr;
}

void SceneGhostLoadSave::GDInexLineVC::updateInfo()
{
    if (mInfo != nullptr)
    {
        mPanes[0]->mIsVisible = true;
        mPanes[1]->mIsVisible = false;

        Kart2DCommon *common = Kart2DCommon::ptr();
        u8 num = mInfo->_0;
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
                                         mInfo->mRecord.getCourseID()),
                                     0);

        int minutes;
        int seconds;
        int miliseconds;
        RaceTime raceTime(mInfo->mRecord.getRaceTime().get());
        raceTime.get(&minutes, &seconds, &miliseconds);
        common->changeNumberTexture(minutes, &mPictures[17], 2, false, false);
        common->changeNumberTexture(seconds, &mPictures[15], 2, true, false);
        common->changeNumberTexture(miliseconds, &mPictures[12], 3, true, false);
        common->changeStringTexture(mInfo->mRecord.getName(), &mPictures[19], 3);

        u8 charID1 = mInfo->mRecord.getCharID(1);
        mPictures[22]->changeTexture(common->getCharIconTextureChecked((ECharID)charID1), 0);
        u8 charID0 = mInfo->mRecord.getCharID(0);
        mPictures[23]->changeTexture(common->getCharIconTextureChecked((ECharID)charID0), 0);
        mPictures[24]->changeTexture(common->getKartIconTexture((ECharID)mInfo->mRecord.getKartID()), 0);

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
        mPanes[0]->mIsVisible = false;
        mPanes[1]->mIsVisible = true;
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
    GhostFileInfo *info = mCard->mTable.mHead;
    u8 i;
    mCardCount = mCard->mTable.mCount;

    bool flag = false;
    if (mspSceneGhostLoadSave->_253c == 2 && card->_a != 0)
        flag = true;

    _3a8 = 0;
    _3a6 = 0;
    _3aa = mCardCount;
    if (flag)
        _3aa++;
    _3ab = (mSelectedLine < _3aa) ? mSelectedLine : _3aa;
    _3ac = _3ab * 4 + 7;

    for (i = 0; i < mLineCount; i++)
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
        mLines[6].mInfo = &mspSceneGhostLoadSave->mGhostInfo;
        mLines[6].mAnimState = 3;
        mLines[6].mFrame = 0;
        mLines[6].updateInfo();
    }

    for (i = 0; i < _3ab; i++)
    {
        mPanes[i]->mIsVisible = true;
        mPanes[i]->setAlpha(255);
    }

    for (; i <= 5; i++)
    {
        mPanes[i]->mIsVisible = false;
        mPanes[i]->setAlpha(255);
    }

    u8 sel = mSelectedLine;
    if (flag)
        sel--;
    mPanes[6]->mIsVisible = false;
    if (mCardCount > sel)
        mPanes[7]->mIsVisible = true;
    else
        mPanes[7]->mIsVisible = false;

    GDInexLineVC &line = mLines[_3a8];
    line.mAnimState = 0;
    line.mFrame = -1;
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
        if (--_3a8 < 0)
        {
            _3a8 = 0;
            if (_3a6 > 0)
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20000);
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
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            GDInexLineVC &prev = mLines[_3a8 + 1];
            prev.mAnimState = 2;
            prev.mFrame = 8;
            GDInexLineVC &cur = mLines[_3a8];
            cur.mAnimState = 0;
            cur.mFrame = -1;
        }
        if (--_3a6 < 0)
            _3a6 = 0;
    }
    else if (gGamePad1P.testRepeat(0x04000000))
    {
        if (++_3a8 >= _3ab)
        {
            _3a8 = _3ab - 1;
            if (_3a6 < _3aa - 1)
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20000);
                scrollDown();
                GDInexLineVC &prev = mLines[mSelectedLine - 1];
                prev.mAnimState = 2;
                prev.mFrame = 8;
                GDInexLineVC &cur = mLines[mSelectedLine];
                cur.mAnimState = 0;
                cur.mFrame = -1;
            }
        }
        else
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            GDInexLineVC &prev = mLines[_3a8 - 1];
            prev.mAnimState = 2;
            prev.mFrame = 8;
            GDInexLineVC &cur = mLines[_3a8];
            cur.mAnimState = 0;
            cur.mFrame = -1;
        }
        if (++_3a6 >= _3aa)
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
    mspSceneGhostLoadSave = this;
    MenuBackground::create(SceneFactory::ptr()->getArchive(9));
    MenuTitleLine::create(SceneFactory::ptr()->getArchive(8), nullptr);
    mSelectSlot.setup(archive, heap);
    mGDIndex.setup(archive, heap);
}

inline void SceneGhostLoadSave::GDIndexLayoutVC::init(SequenceInfo::GhostSaveLoad type)
{
    _3b0 = (type == 2);
    mFrame1 = -1;
    mFrame2 = -1;
    mSelectedLine = 5;
    mLineCount = 6;
    mAnimState = 0;
    if (_3b0)
    {
        mSelectedLine--;
        mLineCount--;
        _3af = 100;
        mPane->show();
    }
    else
    {
        _3af = 0;
        mPane->hide();
    }
}

void SceneGhostLoadSave::init()
{
    _10 = 1;
    _c = 0;
    _2518 = 0;
    _2519 = 0;
    _253c = gSequenceInfo.mGhostFlags;
    mGhostInfo.mRecord = gGhostFile.mFileData.mPadRecord;
    mGhostInfo.mValue = OSTicksToSeconds(OSGetTime());
    mGhostInfo._0 = 0;
    mSelectSlot.init();
    mGDIndex.init((SequenceInfo::GhostSaveLoad)_253c);
    switch (_253c)
    {
    case 0:
        break;
    case 1:
        break;
    case 2:
        GameAudio::Main::getAudio()->startSequenceBgm(0x1000001);
        SequenceApp::ptr()->ready(Scene::SCENE_COURSE);
        break;
    case 3:
        break;
    }
    if (_253c == 1)
        gGhostFile.mGhostType = 0;
}

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

inline void SceneGhostLoadSave::SelectSlot::calcAnm()
{
    if (++mFrame >= 79)
        mFrame = 0;
    mAnm->setFrame(mFrame);
    mCards[0].calcAnm();
    mCards[1].calcAnm();
    mQuitBtn.calcAnm();
    switch (mState)
    {
    case 1:
        if (mCards[0].mAnimState != 1 && mCards[1].mAnimState != 1 && mQuitBtn.mSelectState != 1)
            mState = 2;
        break;
    case 2:
        break;
    case 3:
        if (mCards[0].mAnimState != 2 && mCards[1].mAnimState != 2 && mQuitBtn.mSelectState != 2)
            mState = 0;
        break;
    }
    mScreen->animation();
}

void SceneGhostLoadSave::calc()
{
    mSelectSlot.processCard();
    switch (_c)
    {
    case 0:
        checkCard();
        break;
    case 1:
        fadeIn();
        break;
    case 2:
        frameInSlot();
        break;
    case 4:
        selectSlot();
        break;
    case 3:
        frameOutSlot();
        break;
    case 5:
        frameInData();
        break;
    case 7:
        selectData();
        break;
    case 6:
        frameOutData();
        break;
    case 8:
        ghostSaveLoad();
        break;
    case 10:
        nextScene();
        break;
    case 9:
        prevScene();
        break;
    case 11:
        confirm();
        break;
    }
    MenuBackground::ptr()->calc();
    MenuTitleLine::ptr()->calc();
    switch (_10)
    {
    case 1:
        mSelectSlot.calcAnm();
        break;
    case 2:
        mGDIndex.calcAnm();
        break;
    }
}

#pragma auto_inline off
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
#pragma auto_inline on

void SceneGhostLoadSave::checkCard()
{
    if (mSelectSlot.isCardChecking())
        return;

    switch (_253c)
    {
    case 3:
        if (CardAgent::isGhostSavable())
        {
            _c = 10;
            return;
        }
        break;
    case 1:
        if (!mSelectSlot.existGhostCard())
        {
            System::getDisplay()->startFadeOut(15);
            GameAudio::Main::getAudio()->fadeOutAll(15);
            _2518 = 1;
            _c = 10;
            return;
        }
        mSelectSlot.initSelect(false);
        break;
    case 2:
        mSelectSlot.initSelect(true);
        break;
    }
    startFadeIn();
}

void SceneGhostLoadSave::fadeIn() {
    if (mSelectSlot.mState == 2) {
        _c = 4;
    }
}

void SceneGhostLoadSave::frameInSlot() {
    if (mSelectSlot.mState == 2) {
        _c = 4;
    }
}

void SceneGhostLoadSave::selectSlot()
{
    switch (mSelectSlot.selectSlot())
    {
    case 1:
        GameAudio::Main::getAudio()->startSystemSe(0x20003);
        mSelectSlot.frameOut();
        _c = 3;
        break;
    case 3:
        if (_253c == 3 && !mSelectSlot.existLoadableCard())
        {
            _c = 11;
            CardAgent::ask(CardAgent::mcCommand7, 0);
        }
        else
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20002);
            if (_253c == 1)
            {
                System::getDisplay()->startFadeOut(15);
                GameAudio::Main::getAudio()->fadeOutAll(15);
                _2518 = 1;
            }
            else
            {
                _2518 = 0;
            }
            mSelectSlot.frameOut();
            MenuTitleLine::ptr()->lift();
            _c = 10;
        }
        break;
    case 2:
        if (_253c != 2)
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20005);
            mSelectSlot.frameOut();
            MenuTitleLine::ptr()->lift();
            _c = 9;
            _2518 = 0;
        }
        break;
    }
}

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
        mSelectSlot.mState = 3;

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
    if (mSelectSlot.mState != 0)
        return;

    _c = 5;
    _10 = 2;
    u32 offset = mSelectSlot.mSelectedIndex * sizeof(Card);
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
        chan = mSelectSlot.mSelectedIndex;

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
    if (mSelectSlot.mState != 0)
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
    if (mSelectSlot.mState != 0)
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
