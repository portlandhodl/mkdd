#include "Osako/LANPlayInfo.h"

LANPlayInfo::LANPlayInfo() : mRandom(0) {}

LANPlayInfo gLANPlayInfo;
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/KartInfo.h"
#include "Osako/NetGameMgr.h"
#include "Osako/SystemRecord.h"

static const ECourseID caRaceCrsID[] = {
    LUIGI_CIRCUIT, PEACH_BEACH,  BABY_PARK,   DRY_DRY_DESERT,
    MUSHROOM_BRIDGE, MARIO_CIRCUIT, DAISY_CRUISER, WALUIGI_STADIUM,
    SHERBET_LAND, MUSHROOM_CITY, YOSHI_CIRCUIT, DK_MOUNTAIN,
    WARIO_COLOSSEUM, DINO_DINO_JUNGLE, BOWSER_CASTLE, RAINBOW_ROAD
};

static const ECourseID caBattleCrsID[] = {
    MINI_7, MINI_3, MINI_2, MINI_8, MINI_1, MINI_5
};

static const ECharID caCharID[] = {
    BABY_MARIO, BABY_LUIGI, PARATROOPA, KOOPATROOPA,
    PEACH, DAISY, MARIO, LUIGI,
    WARIO, WALUIGI, YOSHI, BIRDO,
    DONKEY_KONG, DIDDY_KONG, BOWSER, BOWSER_JR,
    TOAD, TOADETTE
};

void LANPlayInfo::init()
{
    NetGameMgr *net = NetGameMgr::ptr();
    mGameMode = 0;
    mPower = 0;
    mCourseSelect = 0;
    mCourse = 0;
    mCourseSel = 0;
    mOrder = 0;
    mTag = 0;
    mDisplay = 0;
    mIdSw = 0;
    mCrsCount = 0;
    mMask = 0;
    mBattleMask = 0;
    if (!(net->get12ec() & 0x80))
    {
        mBattleMask |= 0x10;
    }
    if (!(net->get12ec() & 0x100))
    {
        mBattleMask |= 0x20;
    }
    mRaceMask = 0;
    if (!(net->get12ec() & 8))
    {
        mRaceMask |= 0x1000;
        mRaceMask |= 0x2000;
        mRaceMask |= 0x4000;
        mRaceMask |= 0x8000;
    }
    for (int i = 0; i < 8; i++)
    {
        mActive[i] = 1;
    }
    mRandom.setSeed(NetGameMgr::ptr()->get12e9());
}

ECourseID LANPlayInfo::getCrsID()
{
    switch (mGameMode)
    {
    case 0:
        switch (mCourseSelect)
        {
        case 0:
            return caRaceCrsID[getRandomMap(16, mRaceMask)];
        case 1:
            return caRaceCrsID[mCourse];
        case 2:
            if ((u32)mCrsCount >= 16)
            {
                mCrsCount = 0;
            }
            if (!(NetGameMgr::ptr()->get12ec() & 8) && (u32)mCrsCount >= 12)
            {
                mCrsCount = 0;
            }
            return caRaceCrsID[mCrsCount++];
        case 3:
            if ((u32)mCrsCount >= 4)
            {
                mCrsCount = 0;
            }
            return caRaceCrsID[mCrsCount++];
        case 4:
            if ((u32)mCrsCount >= 4)
            {
                mCrsCount = 0;
            }
            return caRaceCrsID[mCrsCount++ + 4];
        case 5:
            if ((u32)mCrsCount >= 4)
            {
                mCrsCount = 0;
            }
            return caRaceCrsID[mCrsCount++ + 8];
        case 6:
            if ((u32)mCrsCount >= 4)
            {
                mCrsCount = 0;
            }
            return caRaceCrsID[mCrsCount++ + 12];
        }
        break;
    case 1:
    case 2:
        switch (mCourseSelect)
        {
        case 0:
            return caBattleCrsID[getRandomMap(6, mBattleMask)];
        case 1:
            return caBattleCrsID[mCourse];
        case 2:
            if (!(NetGameMgr::ptr()->get12ec() & 0x80) && mCrsCount == 4)
            {
                mCrsCount++;
            }
            if (!(NetGameMgr::ptr()->get12ec() & 0x100) && mCrsCount == 5)
            {
                mCrsCount++;
            }
            if ((u32)mCrsCount >= 6)
            {
                mCrsCount = 0;
            }
            return caBattleCrsID[mCrsCount++];
        }
        break;
    }
    return CRS_ID_INV;
}

ResMgr::CourseOrder LANPlayInfo::getCrsOrder()
{
    return ResMgr::mcOrder_1;
}

void LANPlayInfo::resumeInfo(LANSelectMode *select)
{
    select->mModeItem->mIdx = mGameMode;
    select->mPowerItem->mIdx = mPower;
    select->mLapItem->mIdx = mOrder;
    select->mOrderItem->mIdx = mCourseSelect;
    select->mTagItem->mIdx = mTag;
    select->mDisplayItem->mIdx = mDisplay;
    select->mCourseSelectItem->changeValueMode((LANSelectMode::SelectableItemSw::ValueMode)mIdSw);
    select->mCourseSelectItem->mIdx = mCourse;
    select->mCourseSelectItem->mIdxSw = mCourseSel;
}

void LANPlayInfo::saveInfo(LANSelectMode *select)
{
    mGameMode = select->mModeItem->mIdx;
    mPower = select->mPowerItem->mIdx;
    mOrder = select->mLapItem->mIdx;
    mCourseSelect = select->mOrderItem->mIdx;
    mTag = select->mTagItem->mIdx;
    mDisplay = select->mDisplayItem->mIdx;
    mIdSw = select->mCourseSelectItem->mValueMode;
    mCourse = select->mCourseSelectItem->mIdx;
    mCourseSel = select->mCourseSelectItem->mIdxSw;
    mCrsCount = 0;
    mMask = 0xffff;
    for (int i = 0; i < 8; i++)
    {
        mActive[i] = 1;
    }
}

u8 LANPlayInfo::getRandomMap(int max, u32 mask)
{
    u32 lim = (1 << max) - 1;
    if (lim == (mMask & lim))
    {
        mMask = mask;
    }
    u8 idx = getMaskedRandom(max, mMask);
    mMask |= 1 << idx;
    return idx;
}

void LANPlayInfo::shuffleKart(int count)
{
    u32 mask = 0;
    u32 mask2 = 0;
    if (!(NetGameMgr::ptr()->get12ec() & 1))
    {
        mask |= 0x30000;
    }
    for (int i = 0; i < count; i++)
    {
        u8 front = getMaskedRandom(18, mask);
        mPlayers[i].mFront = front;
        mask |= 1 << front;
        u8 back = getMaskedRandom(18, mask);
        mPlayers[i].mBack = back;
        mask |= 1 << back;

        u8 frontWeight = KartInfo::getDriverWeight(caCharID[mPlayers[i].mFront]);
        u8 backWeight = KartInfo::getDriverWeight(caCharID[mPlayers[i].mBack]);
        if (frontWeight >= backWeight)
        {
            mPlayers[i].mKart = getRideKart(mPlayers[i].mFront);
        }
        else
        {
            mPlayers[i].mKart = getRideKart(mPlayers[i].mBack);
        }
        u8 unk = getMaskedRandom(count, mask2);
        mPlayers[i].mUnk = unk;
        mask2 |= 1 << unk;
    }
}



































































































u8 LANPlayInfo::getMaskedRandom(int max, u32 mask)
{
#line 324
    JUT_ASSERT(!(((mask) & (( 1 << max ) - 1) ) == (( 1 << max ) - 1)));
    int n = max * mRandom.get_ufloat_1() + 1.0f;
    u8 idx = 0;
    for (int i = 0; i < n; i++)
    {
        do
        {
            idx++;
            if (idx >= max)
            {
                idx = 0;
            }
        } while (mask & (1 << idx));
    }
    return idx;
}

EKartID LANPlayInfo::getRideKart(u8 idx)
{
    switch (idx)
    {
    case 1:
        if (SystemRecord::tstSecretKart(RATTLE_BUGGY, NetGameMgr::ptr()->get12ea()))
        {
            return RATTLE_BUGGY;
        }
    case 0:
        return GOO_GOO_BUGGY;
    case 2:
        if (SystemRecord::tstSecretKart(PARA_WING, NetGameMgr::ptr()->get12ea()))
        {
            return PARA_WING;
        }
    case 3:
        return KOOPA_DASHER;
    case 5:
        if (SystemRecord::tstSecretKart(BLOOM_COACH, NetGameMgr::ptr()->get12ea()))
        {
            return BLOOM_COACH;
        }
    case 4:
        return HEART_COACH;
    case 9:
        if (SystemRecord::tstSecretKart(WALUIGI_RACER, NetGameMgr::ptr()->get12ea()))
        {
            return WALUIGI_RACER;
        }
    case 7:
        if (SystemRecord::tstSecretKart(GREEN_FIRE, NetGameMgr::ptr()->get12ea()))
        {
            return GREEN_FIRE;
        }
    case 6:
        return RED_FIRE;
    case 8:
        return WARIO_CAR;
    case 11:
        if (SystemRecord::tstSecretKart(TURBO_BIRDO, NetGameMgr::ptr()->get12ea()))
        {
            return TURBO_BIRDO;
        }
    case 10:
        return TURBO_YOSHI;
    case 12:
        return DK_JUMBO;
    case 13:
        if (SystemRecord::tstSecretKart(BARREL_TRAIN, NetGameMgr::ptr()->get12ea()))
        {
            return BARREL_TRAIN;
        }
        return GOO_GOO_BUGGY;
    case 14:
        return KOOPA_KING;
    case 15:
        if (SystemRecord::tstSecretKart(BULLET_BLASTER, NetGameMgr::ptr()->get12ea()))
        {
            return BULLET_BLASTER;
        }
        return KOOPA_DASHER;
    case 17:
        if (SystemRecord::tstSecretKart(TOADETTE_KART, NetGameMgr::ptr()->get12ea()))
        {
            return TOADETTE_KART;
        }
    case 16:
        return TOAD_KART;
    }
    return (EKartID)0;
}

ECharID LANPlayInfo::getFront(int i)
{
    return caCharID[mPlayers[i].mFront];
}

ECharID LANPlayInfo::getBack(int i)
{
    return caCharID[mPlayers[i].mBack];
}

int LANPlayInfo::getDisplayNum()
{
    switch (mDisplay)
    {
    case 0:
        return 1;
    case 1:
        return 2;
    case 2:
        return 4;
    }
    return 0;
}
