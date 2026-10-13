#include "Kameda/Race2D.h"
#include "Kameda/BombStart2D.h"
#include "Kameda/J2DManager.h"
#include "Kameda/K2DPicture.h"
#include "Kaneshige/KartChecker.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/ItemTurtle.h"
#include "Kaneshige/RaceTime.h"
#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "dolphin/os.h"
#include "mathHelper.h"


Race2D *Race2D::mThis;
Task *Race2D::mTask;

JUTColor Race2D::mPlayerNumberColor[10] = {
    0xff5a5aff, 0x006affff, 0x00ff00ff, 0xffff00ff,
    0xff80ffff, 0xc87124ff, 0x00caf2ff, 0x9d4cffff,
    0x9696ffff, 0xa0a0a0ff
};

const char *Race2D::mPlayerNumberName[] = {
    "PlayerNumber_1P.bti",
    "PlayerNumber_2P.bti",
    "PlayerNumber_3P.bti",
    "PlayerNumber_4P.bti",
    "PlayerNumber_5P.bti",
    "PlayerNumber_6P.bti",
    "PlayerNumber_7P.bti",
    "PlayerNumber_8P.bti",
};

const char *Race2D::mLANPlayerNumberName[] = {
    "LANPlayerNumber_1.bti",
    "LANPlayerNumber_2.bti",
    "LANPlayerNumber_3.bti",
    "LANPlayerNumber_4.bti",
    "LANPlayerNumber_5.bti",
    "LANPlayerNumber_6.bti",
    "LANPlayerNumber_7.bti",
    "LANPlayerNumber_8.bti",
};

const char *Race2D::mTimeLapName[] = {
    "im_time1_3.bti", "im_lap1_1.bti"
};


const char *Race2D::mDriverName[] = {
    "BabyMario_MapIcon32.bti",
    "BabyLuigi_MapIcon32.bti",
    "Patapata_MapIcon32.bti",
    "Nokonoko_MapIcon32.bti",
    "Peach_MapIcon32.bti",
    "Daisy_MapIcon32.bti",
    "Mario_MapIcon32.bti",
    "Luigi_MapIcon32.bti",
    "Wario_MapIcon32.bti",
    "Waluigi_MapIcon32.bti",
    "yoshi_MapIcon32.bti",
    "Catherine_MapIcon32.bti",
    "DonkeyKong_MapIcon32.bti",
    "DiddyKong_MapIcon32.bti",
    "Koopa_MapIcon32.bti",
    "Koopajr_MapIcon32.bti",
    "Kinopio_MapIcon32.bti",
    "Kinopico_MapIcon32.bti",
    "Teresa_MapIcon32.bti",
    "Pakkun_MapIcon32.bti",
};

const char *Race2D::mPlayerName[] = {
    "Map_Icon_Player1.bti",
    "Map_Icon_Player2.bti",
    "Map_Icon_Player3.bti",
    "Map_Icon_Player4.bti",
    "Map_Icon_Player5.bti",
    "Map_Icon_Player6.bti",
    "Map_Icon_Player7.bti",
    "Map_Icon_Player8.bti",
};

const char *Race2D::mLANPlayerName[] = {
    "LANMap_Icon_Player1.bti",
    "LANMap_Icon_Player2.bti",
    "LANMap_Icon_Player3.bti",
    "LANMap_Icon_Player4.bti",
    "LANMap_Icon_Player5.bti",
    "LANMap_Icon_Player6.bti",
    "LANMap_Icon_Player7.bti",
    "LANMap_Icon_Player8.bti",
};

const char *Race2D::mCharacterName[] = {
    "baby_mario.bti",
    "baby_luigi.bti",
    "pata.bti",
    "noko.bti",
    "peach.bti",
    "daisy.bti",
    "mario.bti",
    "luigi.bti",
    "wario.bti",
    "waluigi.bti",
    "yoshi.bti",
    "catherine.bti",
    "donky.bti",
    "diddy.bti",
    "koopa.bti",
    "koopa_jr.bti",
    "kinopio.bti",
    "kinopico.bti",
    "teresa.bti",
    "pakkun.bti"
};

const u64 Race2D::mLapTag[] = {
    'm1_lap', 'ml1_1', 'ml1_s1', 'ml1_2',
    'm2_lap', 'ml2_1', 'ml2_s1', 'ml2_2',
    'm3_lap', 'ml3_1', 'ml3_s1', 'ml3_2',
    'm4_lap', 'ml4_1', 'ml4_s1', 'ml4_2',
};
const u64 Race2D::mItemExTag[] = {
    'itm1bu', 'itm1fu', 'itm1bs', 'itm1fs',
    'itm2bu', 'itm2fu', 'itm2bs', 'itm2fs',
    'itm3bu', 'itm3fu', 'itm3bs', 'itm3fs',
    'itm4bu', 'itm4fu', 'itm4bs', 'itm4fs',
};
const u64 Race2D::mItemExDrawTag[] = {
    'slot1bb', 'itm1bu', 'itm1bs', 'slot1b', 
    'slot1bh2', 'slot1bh1', 'slot1fb', 'itm1fu', 
    'itm1fs', 'slot1f', 'slot1fh2', 'slot1fh1', 
    'slot2bb', 'itm2bu', 'itm2bs', 'slot2b', 
    'slot2bh2', 'slot2bh1', 'slot2fb', 'itm2fu', 
    'itm2fs', 'slot2f', 'slot2fh2', 'slot2fh1', 
    'slot3bb', 'itm3bu', 'itm3bs', 'slot3b', 
    'slot3bh2', 'slot3bh1', 'slot3fb', 'itm3fu', 
    'itm3fs', 'slot3f', 'slot3fh2', 'slot3fh1', 
    'slot4bb', 'itm4bu', 'itm4bs', 'slot4b', 
    'slot4bh2', 'slot4bh1', 'slot4fb', 'itm4fu', 
    'itm4fs', 'slot4f', 'slot4fh2', 'slot4fh1',
};

const char *Race2D::mItemName[] = {
    "item_kame_green.bti",
    "item_koopa.bti",
    "item_kame_red.bti",
    "item_banana.bti",
    "item_big_banana.bti",
    "item_kinoko.bti",
    "item_star.bti",
    "item_wanwan.bti",
    "item_bomb_hei.bti",
    "item_fire_ball.bti",
    "item_thunder.bti",
    "item_egg.bti",
    "item_GoldenKinoko.bti",
    "item_kame_wing.bti",
    "item_heart.bti",
    "item_dummybox.bti",
    "item_empty.bti",
    "item_kame_green_3.bti",
    "item_kinoko_3.bti",
    "item_kame_red_3.bti",
    "item_kame_green_3.bti",
    "item_fire_ball.bti",
    "item_empty.bti",
    "item_kame_green_2.bti",
    "item_kinoko_2.bti",
    "item_kame_red_2.bti",
    "item_fire_ball_luigi.bti",
    "item_egg_catherine.bti",
    "item_kame_green.bti",
    "item_kinoko.bti",
    "item_kame_red.bti",
};

const u64 Race2D::mRankTag[] = {
    'pos11', 'pos12', 'pos13', 'pos14', 
    'pos15', 'pos16', 'pos17', 'pos18', 
    'pos21', 'pos22', 'pos23', 'pos24', 
    'pos25', 'pos26', 'pos27', 'pos28', 
    'pos31', 'pos32', 'pos33', 'pos34', 
    'pos35', 'pos36', 'pos37', 'pos38', 
    'pos41', 'pos42', 'pos43', 'pos44', 
    'pos45', 'pos46', 'pos47', 'pos48',
};

const u64 Race2D::mRankGoalTag[] = {
    'pos11g', 'pos12g', 'pos13g', 'pos14g', 
    'pos15g', 'pos16g', 'pos17g', 'pos18g', 
    'pos21g', 'pos22g', 'pos23g', 'pos24g', 
    'pos25g', 'pos26g', 'pos27g', 'pos28g', 
};

const u64 Race2D::mTimeDrawTag[] = {
    'time17', 'time16', 'time15', 'time1t2', 
    'time14', 'time13', 'time1t1', 'time12', 
    'time11', 'time10', 'time27', 'time26', 
    'time25', 'time2t2', 'time24', 'time23', 
    'time2t1', 'time22', 'time21', 'time20',
};

const u64 Race2D::mLapTimeDrawTag[] = {
    'time16_1', 'time15_1', 'time14_1', 'time13_1', 
    'time12_1', 'time11_1', 'time10_1', 'time1t11', 
    'time1t21', 'Tl11', 'Tlapm1', 'time16_2', 
    'time15_2', 'time14_2', 'time13_2', 'time12_2', 
    'time11_2', 'time10_2', 'time1t12', 'time1t22', 
    'Tl12', 'Tlapm1', 'time16_3', 'time15_3', 
    'time14_3', 'time13_3', 'time12_3', 'time11_3', 
    'time10_3', 'time1t13', 'time1t23', 'Tl13', 
    'Tlapm1', 'time16_4', 'time15_4', 'time14_4', 
    'time13_4', 'time12_4', 'time11_4', 'time10_4', 
    'time1t14', 'time1t24', 'Tl14', 'Tlapm1', 
    'time16_5', 'time15_5', 'time14_5', 'time13_5', 
    'time12_5', 'time11_5', 'time10_5', 'time1t15', 
    'time1t25', 'Tl15', 'Tlapm1', 'time16_6', 
    'time15_6', 'time14_6', 'time13_6', 'time12_6', 
    'time11_6', 'time10_6', 'time1t16', 'time1t26', 
    'Tl16', 'Tlapm1', 'time16_7', 'time15_7', 
    'time14_7', 'time13_7', 'time12_7', 'time11_7', 
    'time10_7', 'time1t17', 'time1t27', 'Tl17', 
    'Tlapm1', 'time16_8', 'time15_8', 'time14_8', 
    'time13_8', 'time12_8', 'time11_8', 'time10_8', 
    'time1t18', 'time1t28', 'Tl18', 'Tlapm1', 
    'time16_9', 'time15_9', 'time14_9', 'time13_9', 
    'time12_9', 'time11_9', 'time10_9', 'time1t19', 
    'time1t29', 'Tl19', 'Tlapm1', 'time26_1', 
    'time25_1', 'time24_1', 'time23_1', 'time22_1', 
    'time21_1', 'time20_1', 'time2t11', 'time2t21', 
    'Tl21', 'Tlapm2', 'time26_2', 'time25_2', 
    'time24_2', 'time23_2', 'time22_2', 'time21_2', 
    'time20_2', 'time2t12', 'time2t22', 'Tl22', 
    'Tlapm2', 'time26_3', 'time25_3', 'time24_3', 
    'time23_3', 'time22_3', 'time21_3', 'time20_3', 
    'time2t13', 'time2t23', 'Tl23', 'Tlapm2', 
    'time26_4', 'time25_4', 'time24_4', 'time23_4', 
    'time22_4', 'time21_4', 'time20_4', 'time2t14', 
    'time2t24', 'Tl24', 'Tlapm2', 'time26_5', 
    'time25_5', 'time24_5', 'time23_5', 'time22_5', 
    'time21_5', 'time20_5', 'time2t15', 'time2t25', 
    'Tl25', 'Tlapm2', 'time26_6', 'time25_6', 
    'time24_6', 'time23_6', 'time22_6', 'time21_6', 
    'time20_6', 'time2t16', 'time2t26', 'Tl26', 
    'Tlapm2', 'time26_7', 'time25_7', 'time24_7', 
    'time23_7', 'time22_7', 'time21_7', 'time20_7', 
    'time2t17', 'time2t27', 'Tl27', 'Tlapm2', 
    'time26_8', 'time25_8', 'time24_8', 'time23_8', 
    'time22_8', 'time21_8', 'time20_8', 'time2t18', 
    'time2t28', 'Tl28', 'Tlapm2', 'time26_9', 
    'time25_9', 'time24_9', 'time23_9', 'time22_9', 
    'time21_9', 'time20_9', 'time2t19', 'time2t29', 
    'Tl29', 'Tlapm2',
};

const u64 Race2D::mSpeedTag[] = {
    'sp3', 'sp2', 'sp1',
};

const u64 Race2D::mSpeedChipTag[] = {
    'sp_mtr1', 'sp_mtr7', 'sp_mtr6', 'sp_mtr5', 
    'sp_mtr4', 'sp_mtr3', 'sp_mtr2', 
};

const u64 Race2D::mSpeedDrawTag[] = {
    'sp_mtr1', 'sp_mtr7', 'sp_mtr6', 'sp_mtr5', 
    'sp_mtr4', 'sp_mtr3', 'sp_mtr2', 'sp1', 
    'sp2', 'sp3', 'km_h',
};

const char *Race2D::mSpeedName[] = {
    "deg0.bti",
    "deg1.bti",
    "deg2.bti",
    "deg3.bti",
    "deg4.bti",
    "deg5.bti",
    "deg6.bti",
    "deg7.bti",
    "deg8.bti",
    "deg9.bti",
};

const u64 Race2D::mShineTimeTag[] = {
    'ShineB', 'ShineM', 'MtimeB10', 'MtimeB11', 
    'MtimeA10', 
};

static f32 sSpinRotate[8];
static int sPreLap[4];
static int sItemState[4][2];
static int sPreBombPoint[4];
static int sPreRank[4];
static int sRankAnm[4];
static int sThunderAnm[8];

static int sChangeFrame;
static int sColorIndex;
static u8 sPreWhichDriver[4];
static u8 sPreWhichDriver2[8];
static bool sPreShineFlag;
static bool sRollingFlag[4][2];
static bool sUseFlag[4][2];
static int sPreShinePlayer;
static RaceTime sBestLapTime;
static u8 sPreThunder[8];

Race2D::Race2D(JKRHeap *) {
    /*
J2DPane::getTranslateY() const {}
J2DPane::getTranslateX() const {}
JUTColor::TColor(const JUTColor &) {}
RaceMgr::getTotalLapNumber() const {}
RaceMgr::getPlayerNumber() const {}
RaceMgr::getKartNumber() const {}
std::atan2(f32, f32) {}
RCMGetCourse() {}
JUTColor::TColor(u32) {}
RaceMgr::isMirror() const {}
Race2DParam::setS(f32) {}
Race2DParam::setY(f32) {}
Race2DParam::setX(f32) {}
ResMgr::getCourseID() {}
Race2DHioNode::getParam() {}
JUTColor::set(_GXColor) {}
J2DManager::getNetFlag() {}
RaceMgr::getConsoleNumber() const {}
Race2DHioNode::Race2DHioNode(const char *) {}
Race2DParam::~Race2DParam() {}
    */
}


void Race2D::init() {}

void Race2D::drawPlayerMark() {}

void Race2D::drawMask()
{
    if (RCMGetManager()->getConsoleNumber() == 4 && RCMGetManager()->getStatusNumber() == 3 &&
        RCMGetManager()->getRacePhase() != PHASE_CRS_DEMO && !J2DManager::mNetFlag)
    {
        mGrafContext->setPort();
        if (RCMGetManager()->mRaceDirector->isRaceEnd())
        {
            J2DPane *pane = mMaskScreen->search('P3Mask');
            pane->setAlpha((u8)(0xff - mHideFrame * 0xff / 20));
        }
        mMaskScreen->draw(0.0f, 0.0f, mGrafContext);
    }
}

void Race2D::draw() {}

void Race2D::drawAlarm() {}

void Race2D::anmTA(int) {}

void Race2D::getAlphaAnmTA(int frame, u8 &alpha)
{
    int t = frame % 0x29;
    if (t < 0xb)
        alpha = 0xff;
    else if (t < 0x15)
        alpha = (u8)(255.0f - ((t - 0xb) * 0xff) / 10.0f);
    else if (t < 0x1f)
        alpha = (u8)(((t - 0x15) * 0xff) / 10.0f);
    else
        alpha = 0xff;
}

void Race2D::getBestTimeColor(int frame, JUTColor &color, J2DPicture::TCornerColor &cornerColor)
{
    f32 t = -0.5f * JMASCos((s16)(frame % 16 * 0xffff / 16)) + 0.5f;
    color.r = (u8)(255.0f * t);
    color.g = (u8)(110.0f * t);
    color.b = 0;
    color.a = 0;
    cornerColor.mColor0.set(255, 255, (u8)(127.0f * t + 128.0f), 255);
    cornerColor.mColor1.set((u8)(177.0f * t + 78.0f), (u8)(255.0f * t), (u8)(128.0f * t), 255);
    cornerColor.mColor2.set((u8)(177.0f * t + 78.0f), (u8)(255.0f * t), (u8)(128.0f * t), 255);
    cornerColor.mColor3.set(255, 255, (u8)(127.0f * t + 128.0f), 255);
}

void Race2D::getBestLapColor(int frame, JUTColor &color)
{
    f32 t = -0.5f * JMASCos((s16)(frame % 16 * 0xffff / 16)) + 0.5f;
    color.r = (u8)(255.0f * t);
    color.g = (u8)(128.0f * t);
    color.b = (u8)(255.0f * t);
    color.a = 0;
}

void Race2D::drawMiniGame()
{
    switch (RCMGetManager()->getRaceMode())
    {
    case BALLOON_BATTLE:
        break;
    case BOMB_BATTLE:
        drawBomb();
        mBombStart->draw();
        break;
    case ESCAPE_BATTLE:
        drawEscape();
        break;
    }
}

void Race2D::drawBomb() {
    //KartChecker::getWinBombPoint() {}
}

// KartChecker::getWinBombPoint is not yet declared in the local KartChecker.h
#pragma dont_inline on
int Race2D::getWinBombPoint() { return KartChecker::sBombPointFull; }
#pragma dont_inline reset

f32 Race2D::getBombScale(int no, int type, int idx)
{
    f32 scale = 0.0f;
    if (isBombReach(no))
    {
        switch (type)
        {
        case 0:
        case 1:
            scale = 1.0f;
            break;
        case 2:
            scale = 2.1f * (-0.5f * JMASCos((s16)(mBombWinFrame[no] * 0xffff / 36)) + 0.5f);
            break;
        }
    }
    else
    {
        int frame = mBombMarkFrame[no][idx];
        if (frame < 0x14)
        {
            switch (type)
            {
            case 0:
            case 1:
                scale = 1.0f;
                break;
            case 2:
                scale = 0.0f;
                break;
            }
        }
        else if (frame - 0x15 < 0xb)
        {
            switch (type)
            {
            case 0:
            case 1:
                scale = 1.0f;
                break;
            case 2:
                scale = 0.0f;
                break;
            }
        }
        else if (frame - 0x15 < 0x10)
        {
            scale = 1.0f + 1.5f * (frame - 0x20) / 5.0f;
        }
        else if (frame - 0x15 < 0x14)
        {
            scale = 2.5f + -2.0f * (frame - 0x25) * 0.25f;
        }
        else if (frame - 0x15 < 0x18)
        {
            scale = 0.5f + 0.5f * (frame - 0x29) * 0.25f;
        }
        else
        {
            scale = 1.0f;
        }
    }
    return scale;
}

f32 Race2D::getReachBombScale(int no, int type)
{
    f32 scale = 0.0f;
    switch (type)
    {
    case 0:
        scale = getBombScale(no, 0, 0);
        break;
    case 1:
        scale = getBombScale(no, 2, 0);
        break;
    default:
#line 2762
        JUT_ASSERT_F(0, "%s", "0");
        break;
    }
    return scale;
}

void Race2D::drawEscape() {}

f32 Race2D::getStartShineTimeScale(int frame)
{
    f32 scale;
    if (frame < 11)
        scale = 1.6f * frame / 11.0f;
    else if (frame < 14)
        scale = 1.6f + -0.6f * (frame - 11) / 3.0f;
    else
        scale = 1.0f;
    return scale;
}

void Race2D::drawCourse() {}

void Race2D::calc() {}

void Race2D::calcPlayerMark() {}

void Race2D::calcParticle() {}

void Race2D::setLayoutData() {}

void Race2D::calcMiniGame()
{
    switch (RCMGetManager()->getRaceMode())
    {
    case BALLOON_BATTLE:
        break;
    case BOMB_BATTLE:
        calcBomb();
        mBombStart->calc();
        break;
    case ESCAPE_BATTLE:
        calcEscape();
        break;
    }
}

void Race2D::calcBomb() {}

JUTColor Race2D::getBombColor(JUTColor c1, JUTColor c2)
{
    f32 t = -0.5f * JMASCos((s16)(mBombColorFrame * 0xffff / 50)) + 0.5f;
    JUTColor color;
    color.r = (u8)(t * (c2.r - c1.r) + c1.r);
    color.g = (u8)(t * (c2.g - c1.g) + c1.g);
    color.b = (u8)(t * (c2.b - c1.b) + c1.b);
    color.a = (u8)(t * (c2.a - c1.a) + c1.a);
    return color;
}

bool Race2D::isBombReach(int statusNo)
{
    int kartNo = J2DManager::getStatus2Kart(statusNo);
    int winPoint = (RCMGetManager()->getKartChecker(kartNo), getWinBombPoint());
    KartChecker *checker = RCMGetManager()->getKartChecker(kartNo);
    int bombPoint = checker->getBombPoint();

    for (int i = 0; i < winPoint - 1; i++)
    {
        if (mBombMarkFrame[statusNo][i] != 0)
            return false;
    }
    return winPoint - 1 == bombPoint;
}

bool Race2D::isBombWinner(int statusNo)
{
    int kartNo = J2DManager::getStatus2Kart(statusNo);
    int winPoint = (RCMGetManager()->getKartChecker(kartNo), getWinBombPoint());
    KartChecker *checker = RCMGetManager()->getKartChecker(kartNo);
    return checker->getBombPoint() == winPoint;
}

void Race2D::setEffectPos(int) {}

void Race2D::calcEscape() {}

void Race2D::calcTimeFrame(bool, int) {}

J2DPicture::TCornerColor Race2D::getCornerColor(int idx)
{
    J2DPicture::TCornerColor colors;
    switch (idx)
    {
    case 0:
        colors.mColor0.set(255, 160, 160, 255);
        colors.mColor1.set(220, 0, 0, 255);
        colors.mColor2.set(220, 0, 0, 255);
        colors.mColor3.set(255, 160, 160, 255);
        break;
    case 1:
        colors.mColor0.set(64, 255, 255, 255);
        colors.mColor1.set(0, 0, 255, 255);
        colors.mColor2.set(0, 0, 255, 255);
        colors.mColor3.set(64, 255, 255, 255);
        break;
    case 2:
        colors.mColor0.set(150, 255, 32, 255);
        colors.mColor1.set(0, 100, 0, 255);
        colors.mColor2.set(0, 100, 0, 255);
        colors.mColor3.set(150, 255, 32, 255);
        break;
    case 3:
        colors.mColor0.set(255, 255, 50, 255);
        colors.mColor1.set(150, 64, 0, 255);
        colors.mColor2.set(150, 64, 0, 255);
        colors.mColor3.set(255, 255, 50, 255);
        break;
    case 4:
        colors.mColor0.set(255, 120, 255, 255);
        colors.mColor1.set(100, 0, 100, 255);
        colors.mColor2.set(100, 0, 100, 255);
        colors.mColor3.set(255, 120, 255, 255);
        break;
    case 5:
        colors.mColor0.set(255, 160, 40, 255);
        colors.mColor1.set(120, 50, 0, 255);
        colors.mColor2.set(120, 50, 0, 255);
        colors.mColor3.set(255, 160, 40, 255);
        break;
    case 6:
        colors.mColor0.set(220, 255, 255, 255);
        colors.mColor1.set(0, 200, 255, 255);
        colors.mColor2.set(0, 200, 255, 255);
        colors.mColor3.set(220, 255, 255, 255);
        break;
    case 7:
        colors.mColor0.set(180, 90, 255, 255);
        colors.mColor1.set(50, 0, 200, 255);
        colors.mColor2.set(50, 0, 200, 255);
        colors.mColor3.set(180, 90, 255, 255);
        break;
    default:
#line 4540
        JUT_ASSERT_F(0, "%s", "0");
        break;
    }
    return colors;
}

void Race2D::setEscapeColor(int) {}

JUTColor Race2D::getTimeColor(JUTColor c1, JUTColor c2)
{
    f32 t = -0.5f * JMASCos((s16)(mTimeColorFrame * 0xffff / 60)) + 0.5f;
    JUTColor color;
    color.r = (u8)(t * (c2.r - c1.r) + c1.r);
    color.g = (u8)(t * (c2.g - c1.g) + c1.g);
    color.b = (u8)(t * (c2.b - c1.b) + c1.b);
    color.a = (u8)(t * (c2.a - c1.a) + c1.a);
    return color;
}

void Race2D::startBombDemo() {
    mBombStart->start();
}

bool Race2D::isAlarm(int status)
{
    bool ret = false;
    int kart = J2DManager::getKart2Status(status);
    if (kart >= 0 && kart < RCMGetManager()->getStatusNumber())
        ret = mPlayerHUD[kart].mIsAlarm;
    return ret;
}

void Race2D::getMapPos(int, JGeometry::TVec3f &, JGeometry::TVec2f &) {}

void Race2D::getCharacterInfo(int no, int which, f32 &x, f32 &y, f32 &z)
{
#line 4735
    JUT_MINMAX_ASSERT(0, no, 2);
    JUT_MINMAX_ASSERT(0, which, 39);
    x = mCharPos[1 - no][which].x;
    y = mCharPos[1 - no][which].y;
    z = mCharPos[1 - no][which].z;
}

void Race2D::getCharacterClr(int, int, int, JUTColor &, JUTColor &, u8 &) {}

void Race2D::getItemInfo(int no, int type, int which, f32 &x, f32 &y, f32 &z)
{
#line 4863
    JUT_MINMAX_ASSERT(0, no, 4);
    JUT_MINMAX_ASSERT(0, type, 2);
    JUT_MINMAX_ASSERT(0, which, 39);
    x = mItemPos[no][type][which].x;
    y = mItemPos[no][type][which].y;
    z = mItemPos[no][type][which].z;
}

void Race2D::decideItem(int no, u8 driverNo, int &item)
{
#line 4905
    JUT_MINMAX_ASSERT(0, no, 4);
    JUT_MINMAX_ASSERT(0, driverNo, 2);

    int itemKind = ItemObjMgr::gItemMgr->getStockItem(J2DManager::getStatus2Kart(no), driverNo);
    ItemObj *itemObj = ItemObjMgr::gItemMgr->getKartEquipItem(J2DManager::getStatus2Kart(no), driverNo);
    if (itemKind == 0x10 && itemObj != NULL)
        itemKind = itemObj->getKind();

    if (itemObj == NULL)
    {
        if (itemKind == 0x15)
        {
            int kartNo = J2DManager::getStatus2Kart(no);
            if (RCMGetManager()->mRaceInfo->getKartInfo(kartNo)->getDriverCharID(driverNo) == 8)
                itemKind = 0x1a;
        }
        if (itemKind == 0xb)
        {
            int kartNo = J2DManager::getStatus2Kart(no);
            if (RCMGetManager()->mRaceInfo->getKartInfo(kartNo)->getDriverCharID(driverNo) == 0xc)
                itemKind = 0x1b;
        }
    }
    else
    {
        switch (itemKind)
        {
        case 0xb:
        {
            ItemObj *egg = ItemObjMgr::getItemYoshiEgg(itemObj);
            if (egg != NULL && egg->getItemColorID() != 1)
                itemKind = 0x1b;
            break;
        }
        case 0x11:
            switch (itemObj->getSuccessionItemNum())
            {
            case 1:
                itemKind = 0x1c;
                break;
            case 2:
                itemKind = 0x17;
                break;
            }
            break;
        case 0x12:
            switch (itemObj->getSuccessionItemNum())
            {
            case 1:
                itemKind = 0x1d;
                break;
            case 2:
                itemKind = 0x18;
                break;
            }
            break;
        case 0x13:
            switch (itemObj->getSuccessionItemNum())
            {
            case 1:
                itemKind = 0x1e;
                break;
            case 2:
                itemKind = 0x19;
                break;
            }
            break;
        case 0x15:
        {
            ItemObj *fire = ItemObjMgr::getItemFireBallSuc(itemObj);
            if (fire != NULL && !((ItemFireBallSuc *)fire)->IsEfctTypeRed())
                itemKind = 0x1a;
            break;
        }
        }
    }
    item = itemKind;
}

void Race2D::calcLapInit()
{
    Task::TCB *curCB = mTask->mCurCB;
    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++)
    {
        Task::TCB *tcb = mTask->make(NULL, 0x8000);
        if (tcb != NULL)
        {
            tcb->mExecIdx = i;
            mTask->change(calcLap, tcb);
        }
    }
    curCB->mRequestKill = true;
}

void Race2D::calcLap()
{
    int no = mTask->mCurCB->mExecIdx;
    int kartNo = J2DManager::getStatus2Kart(no);
    KartChecker *checker = RCMGetManager()->getKartChecker(kartNo);

    int lap = checker->getLap();
    if (lap < 0)
        lap = 0;
    if (lap >= RCMGetManager()->getTotalLapNumber())
        lap = RCMGetManager()->getTotalLapNumber();

    int min, sec, ms;
    checker->getTotalTime().get(&min, &sec, &ms);

    int d0 = min % 10;
    int d1 = min / 10 % 10;
    int d2 = sec % 10;
    int d3 = sec / 10 % 10;
    int d4 = ms % 10;
    int d5 = ms / 10 % 10;
    int d6 = ms / 100 % 10;

    u8 timeValid = (checker->getTotalTime().get() != MAX_TIME);

    int dispLap = lap;
    if (lap > RCMGetManager()->getTotalLapNumber() - 1)
        dispLap = RCMGetManager()->getTotalLapNumber() - 1;

    Race2DPlayerHUD *hud = &mThis->mPlayerHUD[no];
    hud->mRaceTimeAvailable = timeValid;
    hud->mLap = dispLap;
    hud->mDigits[0] = d0;
    hud->mDigits[1] = d1;
    hud->mDigits[2] = d2;
    hud->mDigits[3] = d3;
    hud->mDigits[4] = d4;
    hud->mDigits[5] = d5;
    hud->mDigits[6] = d6;

    if (lap != sPreLap[no] && lap != 0)
    {
        if (lap >= 1 && lap <= 10)
        {
            const RaceTime &lapTime = checker->getLapTime(lap - 1);
            lapTime.get(&min, &sec, &ms);

            u8 slotValid = (lapTime.get() != MAX_TIME);

#line 520
            JUT_MINMAX_ASSERT(0, no, 4);

            Race2DLapSlot *slot = &hud->mLapSlots[lap];
            slot->mAvailable = slotValid;
            slot->mDigits[0] = min % 10;
            slot->mDigits[1] = min / 10 % 10;
            slot->mDigits[2] = sec % 10;
            slot->mDigits[3] = sec / 10 % 10;
            slot->mDigits[4] = ms % 10;
            slot->mDigits[5] = ms / 10 % 10;
            slot->mDigits[6] = ms / 100 % 10;

#line 521
            JUT_MINMAX_ASSERT(0, lap - 1, 9);
        }
        Task::TCB *tcb = mTask->make(NULL, 0x8000);
        if (tcb != NULL)
        {
            tcb->mExecIdx = no;
            mTask->change(lapInit, tcb);
        }
    }
    sPreLap[no] = lap;

    if (checker->isGoal() || RCMGetManager()->mRaceDirector->isRaceEnd())
    {
        if (checker->isGoal())
            mThis->mGoalCount++;
        if (RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2)
            mTask->change(lapTimeInit, mTask->mCurCB);
    }
}

void Race2D::lapInit()
{
    mTask->mCurCB->_10 = 0;
    mTask->change(lapMain, mTask->mCurCB);
}

void Race2D::lapMain()
{
    Task::TCB *curCB = mTask->mCurCB;
    curCB->_10++;
    curCB->mRequestKill = true;
}

void Race2D::lapTimeInit()
{
    mTask->mCurCB->_10 = 0;
    int status = mTask->mCurCB->mExecIdx;
    for (int lap = 0; lap < RCMGetManager()->getTotalLapNumber(); lap++)
    {
        for (int i = 0; i < 11; i++)
        {
#line 484
            JUT_MINMAX_ASSERT(0, status, 4);
            JUT_MINMAX_ASSERT(0, lap, 9);
            JUT_MINMAX_ASSERT(0, i, 11);
            mThis->mLapDigits[status][lap][i]->mIsVisible = true;
        }
    }
    if (RCMGetManager()->getConsoleNumber() == 2)
    {
        for (int i = 0; i < 8; i++)
        {
#line 490
            JUT_MINMAX_ASSERT(0, status, 4);
            JUT_MINMAX_ASSERT(0, i, 8);
            mThis->mRankDigits1[status][i]->setAlpha(0xff);

#line 495
            JUT_MINMAX_ASSERT(0, status, 4);
            JUT_MINMAX_ASSERT(0, i, 8);
            mThis->mRankDigits2[status][i]->setAlpha(0);
        }
    }
    mTask->change(lapTimeMain, mTask->mCurCB);
}

void Race2D::lapTimeMain() {}

void Race2D::calcItem()
{
    if (RCMGetManager()->getRaceMode() == BOMB_BATTLE)
        return;

    int (*itemState)[2] = sItemState;
    bool (*rollingFlag)[2] = (bool (*)[2])sRollingFlag;
    Race2DPlayerHUD *hud = mThis->mPlayerHUD;
    int itemKind2[2];
    for (int no = 0; no < RCMGetManager()->getStatusNumber(); no++, itemState++, rollingFlag++, hud++)
    {
        int w = 0;
        do
        {
            if (ItemObjMgr::gItemMgr->IsRollingSlot(J2DManager::getStatus2Kart(no), w))
            {
                if (!(*rollingFlag)[w])
                {
                    Task::TCB *tcb = mTask->make(NULL, 0x8000);
                    if (tcb != NULL)
                    {
                        tcb->mExecIdx = no;
                        *(u8 *)&tcb->_28 = w;
                        mTask->change(itemStartInit, tcb);
                    }
                }
                (*rollingFlag)[w] = true;
            }
            else
            {
                (*rollingFlag)[w] = false;
            }

            ItemObj *itemObj = ItemObjMgr::gItemMgr->getKartEquipItem(J2DManager::getStatus2Kart(no), w);
            if (itemObj != NULL)
            {
                int state = itemObj->mState;
                if (((*itemState)[w] != 5 && state == 5) || ((*itemState)[w] == 9 && state == 1))
                {
                    Task::TCB *tcb = mTask->make(NULL, 0x8000);
                    if (tcb != NULL)
                    {
                        mThis->decideItem(no, w, itemKind2[0]);

#line 550
                        JUT_MINMAX_ASSERT(0, no, 4);
                        JUT_MINMAX_ASSERT(0, w, 2);
                        JUT_MINMAX_ASSERT(0, itemKind2[0], 0x1f);
                        hud->mItem[w].mShown = itemKind2[0];
                        JUT_MINMAX_ASSERT(0, no, 4);
                        JUT_MINMAX_ASSERT(0, w, 2);
                        JUT_MINMAX_ASSERT(0, itemKind2[1], 0x1f);
                        hud->mItem[w].mNew = itemKind2[1];

                        tcb->mExecIdx = no;
                        *(u8 *)&tcb->_28 = w;
                        mTask->change(itemGetInit, tcb);
                    }
                }
                (*itemState)[w] = state;
            }
            else
            {
                (*itemState)[w] = 0;
            }
            w++;
        } while (w < 2);
    }
}
void Race2D::itemStartInit()
{
    mTask->mCurCB->_10 = 0;
    Task::TCB *tcb = mTask->mCurCB;
    *(u32 *)&tcb->_24 = 0;
    tcb->_2a[1] = 0;
    mTask->change(itemStart, mTask->mCurCB);
}

void Race2D::itemStart() {}

void Race2D::itemBlinkInit()
{
    mTask->mCurCB->_10 = 0;
    mTask->mCurCB->_2a[0] = 1;
    mTask->change(itemBlink, mTask->mCurCB);
}

void Race2D::itemBlink() {}

void Race2D::itemGetInit()
{
    mTask->mCurCB->_10 = 0;
    Task::TCB *tcb = mTask->mCurCB;
    *(u32 *)&tcb->_24 = 0;
    tcb->_2a[1] = 1;
    mTask->change(itemGetMain, mTask->mCurCB);
}

void Race2D::itemGetMain() {}

void Race2D::itemSpecialInit() {
    mTask->mCurCB->_10 = 0;
    mTask->change(itemSpecialMain, mTask->mCurCB);
}

void Race2D::itemSpecialMain() {}

void Race2D::setUsisi(int, ECharID) {}

void Race2D::alarmX(int no, f32 &x)
{
    x = mAlarm[no].mStep * 0.5f + mAlarm[no].mBase;
}

void Race2D::getStartScaleA(int frame, f32 &scale)
{
    if (frame - 0x32 < 0)
        scale = 0.0f;
    else if (frame - 0x32 < 0xb)
        scale = 1.2f * (frame - 0x32) / 11.0f;
    else if (frame - 0x32 < 0x10)
        scale = 1.2f + -0.50000006f * (frame - 0x3d) / 5.0f;
    else if (frame - 0x32 < 0x14)
        scale = 0.3f * (frame - 0x42) * 0.25f + 0.7f;
    else
        scale = 1.0f;
}

void Race2D::getStartScaleB(int frame, f32 &scale)
{
    if (frame - 0x32 < 0)
        scale = 1.0f;
    else if (frame - 0x32 < 0xb)
        scale = 1.0f + 0.20000005f * (frame - 0x32) / 11.0f;
    else if (frame - 0x32 < 0x10)
        scale = 1.2f + -0.50000006f * (frame - 0x3d) / 5.0f;
    else if (frame - 0x32 < 0x14)
        scale = 0.3f * (frame - 0x42) * 0.25f + 0.7f;
    else
        scale = 1.0f;
}

void Race2D::getStartSpeedMeterScale(int frame, int no, f32 &scale)
{
    int t = frame - no * 4;
    if (t < 0)
        scale = 0.0f;
    else if (t < 9)
        scale = 1.2f * t / 9.0f;
    else if (t < 13)
        scale = -0.20000005f * (t - 9) * 0.25f + 1.2f;
    else
        scale = 1.0f;
}

void Race2D::getGoalSpeedMeterPos(int frame, f32 &x)
{
    if (frame < 15)
        x = -182.0f * frame / 15.0f;
    else
        x = -182.0f;
}

void Race2D::getStartLapPos(int kartNo, int frame, f32 &x)
{
    if (RCMGetManager()->getConsoleNumber() == 3 || RCMGetManager()->getConsoleNumber() == 4)
    {
        if (frame >= 10)
        {
            if (frame < 50)
                frame = 10;
            else
                frame -= 40;
        }
        if (frame < 0)
        {
            if (kartNo % 2 != 0)
                x = 140.0f;
            else
                x = -140.0f;
        }
        else if (frame < 15)
        {
            if (kartNo % 2 != 0)
                x = 140.0f - 140.0f * frame / 15.0f;
            else
                x = -140.0f + 140.0f * frame / 15.0f;
        }
        else
        {
            x = 0.0f;
        }
    }
    else
    {
        if (frame < 0)
            x = -166.0f;
        else if (frame < 15)
            x = -166.0f + 168.0f * frame / 15.0f;
        else if (frame < 18)
            x = 2.0f + -5.0f * (frame - 15) / 3.0f;
        else if (frame < 21)
            x = -3.0f + 3.0f * (frame - 18) / 3.0f;
        else
            x = 0.0f;
    }
}

void Race2D::getGoalLapPos(int frame, f32 &x)
{
    if (frame < 15)
        x = -140.0f * frame / 15.0f;
    else
        x = -140.0f;
}

void Race2D::getStartCharPos(int frame, int no, f32 &x)
{
    int t = frame - no * 4;
    if (t < 0)
        x = -140.0f;
    else if (t < 15)
        x = -140.0f + 178.0f * t / 15.0f;
    else if (t < 21)
        x = 38.0f + -55.0f * (t - 15) / 6.0f;
    else if (t < 24)
        x = -17.0f + 17.0f * (t - 21) / 3.0f;
    else
        x = 0.0f;
}

void Race2D::getGoalCharPos(int frame, int no, f32 &x)
{
    if (no < 4)
        frame -= (3 - no) * 4;
    if (frame < 0)
        x = 0.0f;
    else if (frame < 15)
        x = -140.0f * frame / 15.0f;
    else
        x = -140.0f;
}

f32 Race2D::getThunderCharScale(int frame)
{
    f32 scale;
    if (frame < 6)
        scale = 1.0f + -0.39999998f * frame / 6.0f;
    else if (frame < 0x21c)
        scale = 0.6f;
    else if (frame < 0x222)
        scale = 0.6f + 0.9f * (frame - 0x21c) / 6.0f;
    else
        scale = 1.5f + -0.5f * (frame - 0x222) / 6.0f;
    return scale;
}

void Race2D::getStartRankPos(int kartNo, int frame, JGeometry::TVec2f &pos)
{
    frame -= 0x14;
    if (RCMGetManager()->getConsoleNumber() == 3 || RCMGetManager()->getConsoleNumber() == 4)
        frame -= 0x28;

    int consoleNo = RCMGetManager()->getConsoleNumber();
    if (frame < 0)
    {
        switch (consoleNo)
        {
        case 1:
            pos.x = 0.0f;
            pos.y = 0.0f;
            break;
        case 2:
            pos.x = 0.0f;
            pos.y = 0.0f;
            break;
        case 3:
        case 4:
            if (kartNo % 2 != 0)
            {
                pos.x = 140.0f;
                pos.y = 0.0f;
            }
            else
            {
                pos.x = -140.0f;
                pos.y = 0.0f;
            }
            break;
        default:
#line 5986
            JUT_ASSERT_F(0, "%s", "0");
            break;
        }
    }
    else if (frame < 15)
    {
        switch (consoleNo)
        {
        case 1:
            pos.x = 0.0f;
            pos.y = 0.0f;
            break;
        case 2:
            pos.x = 0.0f;
            pos.y = 0.0f;
            break;
        case 3:
        case 4:
            if (kartNo % 2 != 0)
            {
                pos.x = 140.0f - 140.0f * frame / 15.0f;
                pos.y = 0.0f;
            }
            else
            {
                pos.x = -140.0f + 140.0f * frame / 15.0f;
                pos.y = 0.0f;
            }
            break;
        default:
#line 6007
            JUT_ASSERT_F(0, "%s", "0");
            break;
        }
    }
    else
    {
        pos.x = 0.0f;
        pos.y = 0.0f;
    }
}

f32 Race2D::getRaceRankScale(int frame)
{
    f32 scale;
    if (frame < 3)
        scale = 1.0f + -0.3f * frame / 3.0f;
    else if (frame < 6)
        scale = 0.7f + 0.40000004f * (frame - 2) / 3.0f;
    else
        scale = 1.1f + -0.100000024f * (frame - 4) / 3.0f;
    return scale;
}

void Race2D::getGoalRankPos(int kartNo, int frame, JGeometry::TVec2f &pos)
{
    f32 baseX = 0.0f;
    f32 baseY = 0.0f;
    int consoleNo = RCMGetManager()->getConsoleNumber();
    switch (consoleNo)
    {
    case 1:
        baseX = 101.0f - mPlayerHUD[kartNo].mGoalX;
        baseY = 101.0f - mPlayerHUD[kartNo].mGoalY;
        break;
    case 2:
        switch (kartNo)
        {
        case 0:
            baseX = 90.0f - mPlayerHUD[kartNo].mGoalX2;
            baseY = 82.0f - mPlayerHUD[kartNo].mGoalY2;
            break;
        case 1:
            baseX = 90.0f - mPlayerHUD[kartNo].mGoalX2;
            baseY = 368.0f - mPlayerHUD[kartNo].mGoalY2;
            break;
        default:
#line 6070
            JUT_ASSERT_F(0, "%s", "0");
            break;
        }
        break;
    case 3:
    case 4:
        switch (kartNo)
        {
        case 0:
            baseX = 78.0f - mPlayerHUD[kartNo].mGoalX;
            baseY = 68.0f - mPlayerHUD[kartNo].mGoalY;
            break;
        case 1:
            baseX = 529.0f - mPlayerHUD[kartNo].mGoalX;
            baseY = 68.0f - mPlayerHUD[kartNo].mGoalY;
            break;
        case 2:
            baseX = 78.0f - mPlayerHUD[kartNo].mGoalX;
            baseY = 291.0f - mPlayerHUD[kartNo].mGoalY;
            break;
        case 3:
            baseX = 529.0f - mPlayerHUD[kartNo].mGoalX;
            baseY = 291.0f - mPlayerHUD[kartNo].mGoalY;
            break;
        default:
#line 6098
            JUT_ASSERT_F(0, "%s", "0");
            break;
        }
        break;
    default:
#line 6103
        JUT_ASSERT_F(0, "%s", "0");
        break;
    }

    if (frame - 0x80 < 0)
    {
        pos.x = 0.0f;
        pos.y = 0.0f;
    }
else if (frame - 0x80 < 0x1e)
    {
        JGeometry::TVec2f tmp(baseX * (frame - 0x80), baseY * (frame - 0x80));
        pos.x = tmp.x * 0.033333335f;
        pos.y = tmp.y * 0.033333335f;
    }
    else if (frame - 0x80 < 0xd7)
    {
        pos.x = baseX;
        pos.y = baseY;
    }
    else if (frame - 0x80 < 0xe1)
    {
        f32 dir = -140.0f;
        if ((RCMGetManager()->getConsoleNumber() == 3 || RCMGetManager()->getConsoleNumber() == 4) &&
            kartNo % 2 != 0)
            dir *= -1.0f;
        pos.x = baseX + (2.0f * dir - baseX) * (frame - 0x157) / 10.0f;
        pos.y = baseY;
    }
    else
    {
        f32 dir = -140.0f;
        if ((RCMGetManager()->getConsoleNumber() == 3 || RCMGetManager()->getConsoleNumber() == 4) &&
            kartNo % 2 != 0)
            dir *= -1.0f;
        pos.x = 2.0f * dir;
        pos.y = baseY;
    }
}

void Race2D::getGoalRankScale(int kartNo, int frame, f32 &scale)
{
    f32 startScale, endScale, midScale;
    switch (RCMGetManager()->getConsoleNumber())
    {
    case 1:
        startScale = 1.2f;
        endScale = 1.6f;
        midScale = 2.0f;
        break;
    case 2:
        startScale = 1.5f;
        endScale = 1.8f;
        midScale = 2.1f;
        break;
    case 3:
    case 4:
        startScale = 2.6f;
        endScale = 2.8f;
        midScale = 3.2f;
        break;
    default:
#line 6171
        JUT_ASSERT_F(0, "%s", "0");
        break;
    }
    if (RCMGetManager()->getRaceMode() == VERSUS_RACE)
    {
        kartNo = J2DManager::getStatus2Kart(kartNo);
        if (RCMGetManager()->getKartChecker(kartNo)->getRank() >= 2)
        {
            startScale *= 0.7f;
            endScale *= 0.7f;
            midScale *= 0.7f;
        }
    }
    if (frame - 0x80 < 0)
        scale = 1.0f;
    else if (frame - 0x80 < 0xf)
        scale = 1.0f + (endScale - 1.0f) * (frame - 0x80) / 15.0f;
    else if (frame - 0x80 < 0x1e)
        scale = endScale;
    else if (frame - 0x80 < 0x22)
        scale = endScale + 0.25f * (startScale - endScale) * (frame - 0x9e);
    else if (frame - 0x80 < 0x25)
        scale = startScale + (midScale - startScale) * (frame - 0xa2) / 3.0f;
    else if (frame - 0x80 < 0x28)
        scale = midScale + (endScale - midScale) * (frame - 0xa5) / 3.0f;
    else
        scale = endScale;
}

void Race2D::getGoalRankRot(int frame, f32 &rot)
{
    if (frame - 0x80 < 0)
        rot = 0.0f;
    else if (frame - 0x80 < 0x1e)
        rot = -720.0f * (frame - 0x80) / 30.0f;
    else
        rot = 0.0f;
}

void Race2D::getStartLapTimePos(int frame, int no, f32 &x)
{
    f32 pos;
    if (RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2)
        pos = 240.0f;
    else
        pos = 480.0f;

    int t = frame - no * 3;
    if (t < 0)
        x = pos;
    else if (t < 0xf)
        x = pos + -pos * t / 15.0f;
    else if (t < 0x13)
        x = 3.0f * (t - 0xf) * 0.25f;
    else if (t < 0x16)
        x = 3.0f + -3.0f * (t - 0x13) / 3.0f;
    else
        x = 0.0f;
}

void Race2D::getGoalLapTimePos(int frame, int no, f32 &x)
{
    if (RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2)
    {
        int t = frame - 0x80;
        if (t < 0xd2)
        {
            t -= no * 4;
            if (t < 0)
                x = 0.0f;
            else if (t < 0x14)
                x = -217.0f * t / 20.0f;
            else
                x = -217.0f;
        }
        else if (t - 0xd2 < 0xf)
        {
            x = -217.0f + 454.0f * (t - 0xd2) / 15.0f;
        }
        else
        {
            x = 237.0f;
        }
    }
    else
    {
        x = 0.0f;
    }
}

#include "JSystem/JAUdio/JASFakeMatch2.h"
