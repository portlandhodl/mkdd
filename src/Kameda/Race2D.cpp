#include "Kameda/Race2D.h"
#include "Kameda/BombStart2D.h"
#include "Kameda/CanNotSaveG2D.h"
#include "Kameda/LapGhost2D.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JParticle/JPAEmitter.h"
#include "Sato/JPEffectMgr.h"
#include "Sato/stMath.h"
#include "Inagaki/GameAudioMain.h"
#include "Kaneshige/KartLoader.h"
#include <stdio.h>
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
#include "Osako/ResMgr.h"
#include "Osako/system.h"
#include "Kaneshige/Course/Course.h"
#include "Shiraiwa/SiUtil.h"
#include "std/math.h"
#include "Kaneshige/Objects/GeoRabbitMark.h"
#include "Osako/Kart2DCommon.h"
#include "Osako/SystemRecord.h"
#include "Osako/RaceApp.h"
#include "Osako/PadRecorder.h"
#include "Osako/KartPadRecord.h"
#include "Kaneshige/RaceInfo.h"
#include "Yamamoto/kartCtrl.h"
#include "kartLocale.h"

namespace std
{
inline f32 atan2(f32 y, f32 x) { return (f32)::atan2(y, x); }
}
#include "JSystem/JUtility/TColor.h"


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

const u64 Race2D::mLapTag[4][4] = {
    'm1_lap', 'ml1_1', 'ml1_s1', 'ml1_2',
    'm2_lap', 'ml2_1', 'ml2_s1', 'ml2_2',
    'm3_lap', 'ml3_1', 'ml3_s1', 'ml3_2',
    'm4_lap', 'ml4_1', 'ml4_s1', 'ml4_2',
};
const u64 Race2D::mItemExTag[4][4] = {
    'itm1bu', 'itm1fu', 'itm1bs', 'itm1fs',
    'itm2bu', 'itm2fu', 'itm2bs', 'itm2fs',
    'itm3bu', 'itm3fu', 'itm3bs', 'itm3fs',
    'itm4bu', 'itm4fu', 'itm4bs', 'itm4fs',
};
const u64 Race2D::mItemExDrawTag[4][2][6] = {
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

const u64 Race2D::mRankTag[4][8] = {
    'pos11', 'pos12', 'pos13', 'pos14', 
    'pos15', 'pos16', 'pos17', 'pos18', 
    'pos21', 'pos22', 'pos23', 'pos24', 
    'pos25', 'pos26', 'pos27', 'pos28', 
    'pos31', 'pos32', 'pos33', 'pos34', 
    'pos35', 'pos36', 'pos37', 'pos38', 
    'pos41', 'pos42', 'pos43', 'pos44', 
    'pos45', 'pos46', 'pos47', 'pos48',
};

const u64 Race2D::mRankGoalTag[2][8] = {
    'pos11g', 'pos12g', 'pos13g', 'pos14g', 
    'pos15g', 'pos16g', 'pos17g', 'pos18g', 
    'pos21g', 'pos22g', 'pos23g', 'pos24g', 
    'pos25g', 'pos26g', 'pos27g', 'pos28g', 
};

const u64 Race2D::mTimeDrawTag[2][10] = {
    'time17', 'time16', 'time15', 'time1t2', 
    'time14', 'time13', 'time1t1', 'time12', 
    'time11', 'time10', 'time27', 'time26', 
    'time25', 'time2t2', 'time24', 'time23', 
    'time2t1', 'time22', 'time21', 'time20',
};

const u64 Race2D::mLapTimeDrawTag[2][9][11] = {
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


Race2DParam::Race2DParam()
{
    mFlag = 1;
    mUnk8 = 0.0f;
    mY = 0.0f;
    mX = 0.0f;
    mS = 1.0f;
}

Race2D::Race2D(JKRHeap *heap) : mLapGhost(nullptr), mCanNotSave(nullptr), mBombStart(nullptr), mMaskScreen(nullptr)
{
    mThis = this;
    mHio = new (heap, 0) Race2DHioNode("レース２Ｄ");
    mGrafContext = System::getJ2DOrtho();
    if (RCMGetManager()->getRaceMode() == TIME_ATTACK) {
        mLapGhost = new (heap, 0) LapGhost2D(heap);
        mCanNotSave = new (heap, 0) CanNotSaveG2D(heap);
    }
    if (RCMGetManager()->getRaceMode() == BOMB_BATTLE)
        mBombStart = new (heap, 0) BombStart2D(heap);

    switch (RCMGetManager()->getConsoleNumber()) {
    case 1: {
        mScreen = new (heap, 0) J2DScreen();
        mScreen->set("p1_race.blo", GETJ2DManager()->getArchive());
        mAlarmScreen = new (heap, 0) J2DScreen();
        mAlarmScreen->set("ItemAlarm.blo", GETJ2DManager()->getArchive());
        J2DPane *pane = mAlarmScreen->search('ItAl01');
#line 384
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mAlarmPic0 = (J2DPicture *)pane;
        JGeometry::TBox2f box(*mAlarmPic0->getBounds());
        mAlarmPos0.x = box.i.x;
        mAlarmPos0.y = box.i.y;
        pane = mAlarmScreen->search('ItAlTx01');
#line 392
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mAlarmPic1 = (J2DPicture *)pane;
        JGeometry::TBox2f box2(*mAlarmPic1->getBounds());
        mAlarmPos1.x = box2.i.x;
        mAlarmPos1.y = box2.i.y;
        mAlarmX[0] = 434.0f;
        break;
    }
    case 2: {
        mScreen = new (heap, 0) J2DScreen();
        mScreen->set("p2_race.blo", GETJ2DManager()->getArchive());
        mAlarmScreen = new (heap, 0) J2DScreen();
        mAlarmScreen->set("ItemAlarm.blo", GETJ2DManager()->getArchive());
        J2DPane *pane = mAlarmScreen->search('ItAl02');
#line 408
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mAlarmPic0 = (J2DPicture *)pane;
        JGeometry::TBox2f box(*mAlarmPic0->getBounds());
        mAlarmPos0.x = box.i.x;
        mAlarmPos0.y = box.i.y;
        pane = mAlarmScreen->search('ItAlTx02');
#line 416
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mAlarmPic1 = (J2DPicture *)pane;
        JGeometry::TBox2f box2(*mAlarmPic1->getBounds());
        mAlarmPos1.x = box2.i.x;
        mAlarmPos1.y = box2.i.y;
        mAlarmX[0] = 224.0f;
        mAlarmX[1] = 434.0f;
        break;
    }
    case 3:
    case 4: {
        mScreen = new (heap, 0) J2DScreen();
        mScreen->set("p4_race.blo", GETJ2DManager()->getArchive());
        mMaskScreen = new (heap, 0) J2DScreen();
        mMaskScreen->set("p4_Mask.blo", GETJ2DManager()->getArchive());
        mAlarmScreen = new (heap, 0) J2DScreen();
        mAlarmScreen->set("ItemAlarm.blo", GETJ2DManager()->getArchive());
        J2DPane *pane = mAlarmScreen->search('ItAl03');
#line 435
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mAlarmPic0 = (J2DPicture *)pane;
        JGeometry::TBox2f box(*mAlarmPic0->getBounds());
        mAlarmPos0.x = box.i.x;
        mAlarmPos0.y = box.i.y;
        pane = mAlarmScreen->search('ItAlTx03');
#line 443
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mAlarmPic1 = (J2DPicture *)pane;
        JGeometry::TBox2f box2(*mAlarmPic1->getBounds());
        mAlarmPos1.x = box2.i.x;
        mAlarmPos1.y = box2.i.y;
        mAlarmX[0] = mAlarmX[1] = 224.0f;
        mAlarmX[2] = mAlarmX[3] = 434.0f;
        break;
    }
    default:
#line 453
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < 20; i++)
        mDriverTex[i] = (const ResTIMG *)J2DScreen::getNameResource(mDriverName[i]);
    for (int i = 0; i < 8; i++) {
        if (J2DManager::getNetFlag())
            mPlayerTex[i] = (const ResTIMG *)J2DScreen::getNameResource(mLANPlayerName[i]);
        else
            mPlayerTex[i] = (const ResTIMG *)J2DScreen::getNameResource(mPlayerName[i]);
    }
    for (int i = 0; i < 20; i++)
        mCharacterTex[i] = (const ResTIMG *)J2DScreen::getNameResource(mCharacterName[i]);
    for (int i = 0; i < 9; i++)
        mLapNumTex[i] = Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(i + 1));
    for (int i = 0; i < 31; i++)
        mItemTex[i] = (const ResTIMG *)J2DScreen::getNameResource(mItemName[i]);
    for (int i = 0; i < 10; i++)
        mNumberTex[i] = Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)i);
    for (int i = 0; i < 10; i++)
        mSpeedTex[i] = (const ResTIMG *)J2DScreen::getNameResource(mSpeedName[i]);

    switch (RCMGetManager()->getConsoleNumber()) {
    case 1:
        mAlarm[0].mBase = System::get3DScisX();
        mAlarm[0]._4 = System::get3DScisY();
        mAlarm[0].mStep = System::get3DScisW();
        mAlarm[0]._c = System::get3DScisH();
        break;
    case 2:
        for (u8 i = 0; i < 2; i++) {
            mAlarm[i].mBase = System::get3DScisDiv2X(i);
            mAlarm[i]._4 = System::get3DScisDiv2Y(i);
            mAlarm[i].mStep = System::get3DScisDiv2W(i);
            mAlarm[i]._c = System::get3DScisDiv2H(i);
        }
        break;
    case 3:
    case 4:
        for (u8 i = 0; i < 4; i++) {
            mAlarm[i].mBase = System::get3DScisDiv4X(i);
            mAlarm[i]._4 = System::get3DScisDiv4Y(i);
            mAlarm[i].mStep = System::get3DScisDiv4W(i);
            mAlarm[i]._c = System::get3DScisDiv4H(i);
        }
        break;
    default:
#line 510
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
            mMark[i][j] = new (heap, 0) J2DPicture();
            if (J2DManager::getNetFlag())
                mMark[i][j]->changeTexture(mLANPlayerNumberName[j], 0);
            else
                mMark[i][j]->changeTexture(mPlayerNumberName[j], 0);
            mMark[i][j]->setCornerColor(mPlayerNumberColor[j]);
            if (!J2DManager::getNetFlag() && RCMGetManager()->getKartInfo(0)->isTagMode()) {
                int idx = j;
                if (idx != 0) {
                    idx++;
                    if (idx >= 8)
                        idx = 7;
                }
                mMark[i][j]->changeTexture(mPlayerNumberName[idx], 0);
                mMark[i][j]->setCornerColor(mPlayerNumberColor[idx]);
            }
        }
    }

    if (mScreen) {
        if (RCMGetManager()->getConsoleNumber() == 1) {
            for (int i = 0; i < 3; i++) {
                J2DPane *pane = mScreen->search(mSpeedTag[i]);
#line 538
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mSpeedPics[i] = (J2DPicture *)pane;
            }
            for (int i = 0; i < 7; i++) {
                J2DPane *pane = mScreen->search(mSpeedChipTag[i]);
#line 544
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mSpeedChipPics[i] = (J2DPicture *)pane;
                mSpeedChipColor[i].set(mSpeedChipPics[i]->getWhite());
            }
            for (int i = 0; i < 11; i++) {
                J2DPane *pane = mScreen->search(mSpeedDrawTag[i]);
#line 551
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mSpeedDrawPics[i] = (J2DPicture *)pane;
                JGeometry::TBox2f box(*mSpeedDrawPics[i]->getBounds());
                mSpeedDrawPos[i].x = (box.i.x + box.f.x) / 2;
                mSpeedDrawPos[i].y = (box.i.y + box.f.y) / 2;
            }
        }
        if (RCMGetManager()->getConsoleNumber() == 4 && RCMGetManager()->getStatusNumber() == 3 && !J2DManager::getNetFlag()) {
            J2DPane *pane = mScreen->search('demo');
#line 564
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mDemoPic = (J2DPicture *)pane;
            JGeometry::TBox2f box(*mDemoPic->getBounds());
            mDemoPos = box.i;
            ResTIMG *img = (ResTIMG *)ResMgr::getPtr(ResMgr::mcCourseNameBti);
            if (img) {
                mDemoPic->changeTexture(img, 0);
                mDemoPic->show();
            } else {
                mDemoPic->hide();
            }
        }

        Race2DParam *param = mHio->getParam();
        switch (ResMgr::getCourseID()) {
    case BABY_PARK:
        mMapInfo.mMin.x = -16572.3f;
        mMapInfo.mMin.y = -8286.1f;
        mMapInfo.mMax.x = 16572.3f;
        mMapInfo.mMax.y = 8286.1f;
        mMapInfo.mRot = 3;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(23.0f);
            param->setY(13.0f);
            param->setS(0.8f);
            break;
        case 2:
            param->setX(23.0f);
            param->setY(-4.0f);
            param->setS(0.8f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(-4.0f);
            param->setS(0.8f);
            break;
        default:
#line 606
            JUT_ASSERT(0);
            break;
        }
        break;
    case PEACH_BEACH:
    case REVERSE_1:
        mMapInfo.mMin.x = -22321.36f;
        mMapInfo.mMin.y = -34855.84f;
        mMapInfo.mMax.x = 12534.399f;
        mMapInfo.mMax.y = 34855.84f;
        mMapInfo.mRot = 2;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            if (RCMGetManager()->isMirror())
                param->setX(-7.0f);
            else
                param->setX(34.0f);
            param->setY(16.0f);
            param->setS(1.2f);
            break;
        case 2:
            if (RCMGetManager()->isMirror())
                param->setX(-7.0f);
            else
                param->setX(34.0f);
            param->setY(-4.0f);
            param->setS(1.2f);
            break;
        case 3:
        case 4:
            if (RCMGetManager()->isMirror())
                param->setX(-27.0f);
            else
                param->setX(28.0f);
            param->setY(0.0f);
            param->setS(1.0f);
            break;
        default:
#line 647
            JUT_ASSERT(0);
            break;
        }
        break;
    case DAISY_CRUISER:
        mMapInfo.mMin.x = -42000.0f;
        mMapInfo.mMin.y = -20000.0f;
        mMapInfo.mMax.x = 38000.0f;
        mMapInfo.mMax.y = 20000.0f;
        mMapInfo.mRot = 3;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(33.0f);
            param->setY(13.0f);
            param->setS(1.15f);
            break;
        case 2:
            param->setX(29.0f);
            param->setY(-2.0f);
            param->setS(1.3f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(0.0f);
            param->setS(1.1f);
            break;
        default:
#line 675
            JUT_ASSERT(0);
            break;
        }
        break;
    case LUIGI_CIRCUIT:
        mMapInfo.mMin.x = -18519.02f;
        mMapInfo.mMin.y = -37634.348f;
        mMapInfo.mMax.x = 16332.94f;
        mMapInfo.mMax.y = 32069.701f;
        mMapInfo.mRot = 2;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(6.0f);
            param->setY(11.0f);
            param->setS(1.2f);
            break;
        case 2:
            param->setX(6.0f);
            param->setY(-4.0f);
            param->setS(1.2f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(0.0f);
            param->setS(1.0f);
            break;
        default:
#line 703
            JUT_ASSERT(0);
            break;
        }
        break;
    case MARIO_CIRCUIT:
    case REVERSE_6:
        mMapInfo.mMin.x = -19360.0f;
        mMapInfo.mMin.y = -38720.0f;
        mMapInfo.mMax.x = 19360.0f;
        mMapInfo.mMax.y = 38720.0f;
        mMapInfo.mRot = 2;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(5.0f);
            param->setY(17.0f);
            param->setS(1.15f);
            break;
        case 2:
            param->setX(-5.0f);
            param->setY(0.0f);
            param->setS(1.3f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(0.0f);
            param->setS(1.0f);
            break;
        default:
#line 732
            JUT_ASSERT(0);
            break;
        }
        break;
    case YOSHI_CIRCUIT:
    case REVERSE_3:
        mMapInfo.mMin.x = -22050.0f;
        mMapInfo.mMin.y = -43050.0f;
        mMapInfo.mMax.x = 28350.0f;
        mMapInfo.mMax.y = 57750.0f;
        mMapInfo.mRot = 0;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(-11.0f);
            param->setY(0.0f);
            param->setS(1.25f);
            break;
        case 2:
            param->setX(-12.0f);
            param->setY(-27.0f);
            param->setS(1.3f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(-24.0f);
            param->setS(1.15f);
            break;
        default:
#line 761
            JUT_ASSERT(0);
            break;
        }
        break;
    case MUSHROOM_BRIDGE:
    case REVERSE_2:
        mMapInfo.mMin.x = -32285.6f;
        mMapInfo.mMin.y = -16658.701f;
        mMapInfo.mMax.x = 45813.0f;
        mMapInfo.mMax.y = 22390.602f;
        mMapInfo.mRot = 3;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(10.0f);
            param->setY(14.0f);
            param->setS(1.3f);
            break;
        case 2:
            param->setX(5.0f);
            param->setY(-7.0f);
            param->setS(1.3f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(0.0f);
            param->setS(1.0f);
            break;
        default:
#line 790
            JUT_ASSERT(0);
            break;
        }
        break;
    case MUSHROOM_CITY:
    case REVERSE_7:
        mMapInfo.mMin.x = -35800.0f;
        mMapInfo.mMin.y = -22500.0f;
        mMapInfo.mMax.x = 54200.0f;
        mMapInfo.mMax.y = 22500.0f;
        mMapInfo.mRot = 3;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(17.0f);
            param->setY(6.5f);
            param->setS(1.15f);
            break;
        case 2:
            param->setX(11.0f);
            param->setY(-14.0f);
            param->setS(1.3f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(-6.0f);
            param->setS(1.05f);
            break;
        default:
#line 819
            JUT_ASSERT(0);
            break;
        }
        break;
    case WALUIGI_STADIUM:
        mMapInfo.mMin.x = -25000.0f;
        mMapInfo.mMin.y = -13000.0f;
        mMapInfo.mMax.x = 27000.0f;
        mMapInfo.mMax.y = 13000.0f;
        mMapInfo.mRot = 1;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(9.0f);
            param->setY(20.0f);
            param->setS(1.0f);
            break;
        case 2:
            param->setX(4.0f);
            param->setY(-8.0f);
            param->setS(1.1f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(0.0f);
            param->setS(0.9f);
            break;
        default:
#line 847
            JUT_ASSERT(0);
            break;
        }
        break;
    case WARIO_COLOSSEUM:
        mMapInfo.mMin.x = -27000.0f;
        mMapInfo.mMin.y = -14500.0f;
        mMapInfo.mMax.x = 33000.0f;
        mMapInfo.mMax.y = 15500.0f;
        mMapInfo.mRot = 3;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(-5.0f);
            param->setY(25.0f);
            param->setS(1.1f);
            break;
        case 2:
            param->setX(-5.0f);
            param->setY(12.0f);
            param->setS(1.2f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(-9.0f);
            param->setS(0.9f);
            break;
        default:
#line 875
            JUT_ASSERT(0);
            break;
        }
        break;
    case DINO_DINO_JUNGLE:
    case REVERSE_4:
        mMapInfo.mMin.x = -53460.0f;
        mMapInfo.mMin.y = -26730.0f;
        mMapInfo.mMax.x = 35640.0f;
        mMapInfo.mMax.y = 17820.0f;
        mMapInfo.mRot = 3;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(-11.0f);
            param->setY(14.0f);
            param->setS(1.3f);
            break;
        case 2:
            param->setX(-15.0f);
            param->setY(-9.0f);
            param->setS(1.5f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(-4.0f);
            param->setS(1.1f);
            break;
        default:
#line 904
            JUT_ASSERT(0);
            break;
        }
        break;
    case DK_MOUNTAIN:
        mMapInfo.mMin.x = -28636.72f;
        mMapInfo.mMin.y = -68430.875f;
        mMapInfo.mMax.x = 17479.201f;
        mMapInfo.mMax.y = 23800.959f;
        mMapInfo.mRot = 2;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(13.5f);
            param->setY(18.0f);
            param->setS(1.15f);
            break;
        case 2:
            param->setX(7.0f);
            param->setY(-4.0f);
            param->setS(1.3f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(0.0f);
            param->setS(1.0f);
            break;
        default:
#line 932
            JUT_ASSERT(0);
            break;
        }
        break;
    case BOWSER_CASTLE:
        mMapInfo.mMin.x = -32400.0f;
        mMapInfo.mMin.y = -39600.0f;
        mMapInfo.mMax.x = 7200.0f;
        mMapInfo.mMax.y = 39600.0f;
        mMapInfo.mRot = 0;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(-5.0f);
            param->setY(17.0f);
            param->setS(1.2f);
            break;
        case 2:
            param->setX(-6.0f);
            param->setY(-22.0f);
            param->setS(1.3f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(-5.0f);
            param->setS(0.9f);
            break;
        default:
#line 960
            JUT_ASSERT(0);
            break;
        }
        break;
    case RAINBOW_ROAD:
        mMapInfo.mMin.x = -28797.12f;
        mMapInfo.mMin.y = -54394.29f;
        mMapInfo.mMax.x = 25597.17f;
        mMapInfo.mMax.y = 54394.29f;
        mMapInfo.mRot = 0;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(-4.0f);
            param->setY(23.0f);
            param->setS(1.25f);
            break;
        case 2:
            param->setX(-4.0f);
            param->setY(-2.0f);
            param->setS(1.4f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(0.0f);
            param->setS(1.1f);
            break;
        default:
#line 988
            JUT_ASSERT(0);
            break;
        }
        break;
    case DRY_DRY_DESERT:
    case REVERSE_8:
        mMapInfo.mMin.x = -34715.9f;
        mMapInfo.mMin.y = -63251.5f;
        mMapInfo.mMax.x = 28535.7f;
        mMapInfo.mMax.y = 63251.5f;
        mMapInfo.mRot = 0;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(-12.0f);
            param->setY(31.0f);
            param->setS(1.35f);
            break;
        case 2:
            param->setX(-12.0f);
            param->setY(5.0f);
            param->setS(1.35f);
            break;
        case 3:
        case 4:
            param->setX(3.0f);
            param->setY(15.0f);
            param->setS(1.35f);
            break;
        default:
#line 1017
            JUT_ASSERT(0);
            break;
        }
        break;
    case SHERBET_LAND:
    case REVERSE_5:
        mMapInfo.mMin.x = -15298.32f;
        mMapInfo.mMin.y = -42345.68f;
        mMapInfo.mMax.x = 29461.521f;
        mMapInfo.mMax.y = 47174.0f;
        mMapInfo.mRot = 0;
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            param->setX(-4.0f);
            param->setY(13.0f);
            param->setS(1.3f);
            break;
        case 2:
            param->setX(-8.0f);
            param->setY(-9.0f);
            param->setS(1.4f);
            break;
        case 3:
        case 4:
            param->setX(0.0f);
            param->setY(0.0f);
            param->setS(1.15f);
            break;
        default:
#line 1046
            JUT_ASSERT(0);
            break;
        }
        break;
    case MINI_1:
        mMapInfo.mMin.x = -7425.0f;
        mMapInfo.mMin.y = -16250.0f;
        mMapInfo.mMax.x = 8825.0f;
        mMapInfo.mMax.y = 16250.0f;
        mMapInfo.mRot = 0;
        param->setS(1.3f);
        break;
    case MINI_2:
        mMapInfo.mMin.x = -5750.0f;
        mMapInfo.mMin.y = -11500.0f;
        mMapInfo.mMax.x = 5750.0f;
        mMapInfo.mMax.y = 11500.0f;
        mMapInfo.mRot = 0;
        param->setS(1.3f);
        break;
    case MINI_3:
        mMapInfo.mMin.x = -9000.0f;
        mMapInfo.mMin.y = -18000.0f;
        mMapInfo.mMax.x = 9000.0f;
        mMapInfo.mMax.y = 18000.0f;
        mMapInfo.mRot = 0;
        param->setS(1.3f);
        break;
    case MINI_5:
        mMapInfo.mMin.x = -10500.0f;
        mMapInfo.mMin.y = -21000.0f;
        mMapInfo.mMax.x = 10500.0f;
        mMapInfo.mMax.y = 21000.0f;
        mMapInfo.mRot = 0;
        param->setS(1.3f);
        break;
    case MINI_7:
        mMapInfo.mMin.x = -8350.7f;
        mMapInfo.mMin.y = -16701.3f;
        mMapInfo.mMax.x = 8350.7f;
        mMapInfo.mMax.y = 16701.3f;
        mMapInfo.mRot = 0;
        param->setS(1.3f);
        break;
    case MINI_8:
        mMapInfo.mMin.x = -10500.0f;
        mMapInfo.mMin.y = -21000.0f;
        mMapInfo.mMax.x = 10500.0f;
        mMapInfo.mMax.y = 21000.0f;
        mMapInfo.mRot = 0;
        param->setS(1.3f);
        break;
    case MINI_4:
    case MINI_6:
        mMapInfo.mMin.x = -12500.0f;
        mMapInfo.mMin.y = -25000.0f;
        mMapInfo.mMax.x = 12500.0f;
        mMapInfo.mMax.y = 25000.0f;
        mMapInfo.mRot = 0;
        param->setS(1.3f);
        break;
    default:
        mMapInfo.mMin.x = -18519.0f;
        mMapInfo.mMin.y = -37634.4f;
        mMapInfo.mMax.x = 16332.94f;
        mMapInfo.mMax.y = 32069.7f;
        mMapInfo.mRot = 2;
        break;

        }

        J2DPane *pane = mScreen->search('course');
#line 1119
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mCoursePic = (J2DPicture *)pane;
        JGeometry::TBox2f box(*mCoursePic->getBounds());
        mCoursePos.x = box.i.x;
        mCoursePos.y = box.i.y;
        ResTIMG *map = (ResTIMG *)ResMgr::getPtr(ResMgr::mcCourseMapBti);
        if (map) {
            mCoursePic->changeTexture(map, 0);
            if (RCMGetManager()->isRaceModeMiniGame())
                mCoursePic->setAlpha(170);
            mCoursePic->show();
        } else {
            mCoursePic->hide();
        }

        mStartLinePic = new (heap, 0) J2DPicture();
        mStartLinePic->changeTexture("StartLine1.bti", 0);
        if (map) {
            mStartLinePic->show();
            mStartLinePic->setBlack(JUTColor(0x0000ff00));
            mStartLinePic->setWhite(JUTColor(0xffffffff));
            mStartLinePic->setBasePosition(J2DPOS_Center);
        } else {
            mStartLinePic->hide();
        }
        if (RCMGetManager()->isRaceModeMiniGame())
            mStartLinePic->hide();

        JGeometry::TVec3f startPos;
        JGeometry::TVec3f startDir;
        RCMGetCourse()->getStartLine(&startPos, &startDir);
        startDir.z *= -1.0f;
        if (RCMGetManager()->isMirror())
            mStartLineRot = 180.0f * std::atan2(startDir.z, -startDir.x) / 3.1415927f;
        else
            mStartLineRot = 180.0f * std::atan2(startDir.z, startDir.x) / 3.1415927f;
        getMapPos(mMapInfo.mRot, startPos, mStartLinePos);
        mStartLineRot += 90.0f * mMapInfo.mRot;
        mStartLineRot -= 90.0f;
        if (mStartLineRot < 0.0f)
            mStartLineRot += 360.0f;
        if (mStartLineRot >= 360.0f)
            mStartLineRot -= 360.0f;

        for (int i = 0; i < RCMGetManager()->getKartNumber(); i++) {
            mKartIcon[i] = new (heap, 0) J2DPicture();
            if (RCMGetManager()->isRaceModeMiniGame())
                mKartIcon[i]->changeTexture("BattleIcon.bti", 0);
            else
                mKartIcon[i]->changeTexture(mDriverTex[i], 0);
            mKartIcon[i]->setBasePosition(J2DPOS_Center);
        }
        for (int i = 0; i < RCMGetManager()->getPlayerNumber(); i++) {
            mPlayerIcon[i] = new (heap, 0) J2DPicture();
            mPlayerIcon[i]->changeTexture(mPlayerTex[i], 0);
            mPlayerIcon[i]->setCornerColor(mPlayerNumberColor[i]);
            if (!J2DManager::getNetFlag() && RCMGetManager()->getKartInfo(0)->isTagMode()) {
                int idx = i;
                if (idx != 0) {
                    idx++;
                    if (idx >= 8)
                        idx = 7;
                }
                mPlayerIcon[i]->changeTexture(mPlayerTex[idx], 0);
                mPlayerIcon[i]->setCornerColor(mPlayerNumberColor[idx]);
            }
            if (RCMGetManager()->isRaceModeMiniGame())
                mPlayerIcon[i]->hide();
        }
        mPlayerGIcon = new (heap, 0) J2DPicture();
        mPlayerGIcon->changeTexture("Map_Icon_PlayerG.bti", 0);
        mPlayerGIcon->setCornerColor(mPlayerNumberColor[8]);
        mPlayerSGIcon = new (heap, 0) J2DPicture();
        mPlayerSGIcon->changeTexture("Map_Icon_PlayerSG.bti", 0);
        mPlayerSGIcon->setCornerColor(mPlayerNumberColor[9]);
        if (RCMGetManager()->getRaceMode() == ESCAPE_BATTLE) {
            mMapShinePic = new (heap, 0) J2DPicture();
            mMapShinePic->changeTexture("MapShine00.bti", 0);
            mMapShinePic->setWhite(JUTColor(0xffff00ff));
            mBattleLightPic = new (heap, 0) J2DPicture();
            mBattleLightPic->changeTexture("BattleIconLight.bti", 0);
            mEscapeFrame = 0;
        }

        for (int i = 0; i < RCMGetManager()->getConsoleNumber(); i++) {
            J2DPane *pane = mScreen->search(mLapTag[i][1]);
#line 1219
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mLapPic[i] = (J2DPicture *)pane;
            pane = mScreen->search(mLapTag[i][3]);
#line 1225
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            ((J2DPicture *)pane)->changeTexture(mLapNumTex[RCMGetManager()->getTotalLapNumber() - 1], 0);
            for (int j = 0; j < 4; j++) {
                J2DPane *pane = mScreen->search(mLapTag[i][j]);
#line 1230
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mLapPics[i][j] = (J2DPicture *)pane;
                JGeometry::TBox2f box(*mLapPics[i][j]->getBounds());
                mPlayerHUD[i].mLapPos[j].x = (box.i.x + box.f.x) / 2;
                mPlayerHUD[i].mLapPos[j].y = (box.i.y + box.f.y) / 2;
            }
            for (int j = 0; j < 4; j++) {
                J2DPane *pane = mScreen->search(mItemExTag[i][j]);
#line 1240
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mItemExPics[i][j] = (J2DPicture *)pane;
                mItemExPics[i][j]->show();
            }
            for (int k = 0; k < 2; k++) {
                for (int j = 0; j < 6; j++) {
                    J2DPane *pane = mScreen->search(mItemExDrawTag[i][k][j]);
#line 1247
                    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                    mItemExDraw[i][k][j] = (J2DPicture *)pane;
                    JGeometry::TBox2f box(*mItemExDraw[i][k][j]->getBounds());
                    mItemExPos[i][k][j].x = (box.i.x + box.f.x) / 2;
                    mItemExPos[i][k][j].y = (box.i.y + box.f.y) / 2;
                    mItemExDraw[i][k][j]->show();
                }
            }
            for (int k = 0; k < 2; k++) {
                mSpecialItem[i][k] = new (heap, 0) J2DPicture();
                mSpecialItem[i][k]->changeTexture("item_SpecialMozi2.bti", 0);
                mSpecialItem[i][k]->setWhite(JUTColor(0xffff50ff));
                mSpecialItem[i][k]->setBlack(JUTColor(0x32000000));
            }
            for (int k = 0; k < 2; k++) {
                for (int j = 0; j < 6; j++) {
                    mKiraPics[i][k][j] = new (heap, 0) J2DPicture();
                    mKiraPics[i][k][j]->changeTexture("kirakira2.bti", 0);
                    switch (j) {
                    case 0:
                    case 3:
                        mKiraPics[i][k][j]->setBlack(JUTColor(0xff780000));
                        break;
                    case 1:
                    case 4:
                        mKiraPics[i][k][j]->setBlack(JUTColor(0xff00ff00));
                        break;
                    case 2:
                    case 5:
                        mKiraPics[i][k][j]->setBlack(JUTColor(0x00ffff00));
                        break;
                    default:
#line 1286
                        JUT_ASSERT(0);
                        break;
                    }
                }
            }
            for (int k = 0; k < 2; k++) {
                switch (RCMGetManager()->getConsoleNumber()) {
                case 1:
                    mPlayerHUD[i].mItemPos[k].x = mItemExPos[i][k][0].x;
                    mPlayerHUD[i].mItemPos[k].y = mItemExPos[i][k][0].y;
                    mPlayerHUD[i].mItemPos2[k].x = mItemExPos[i][k][0].x;
                    mPlayerHUD[i].mItemPos2[k].y = mItemExPos[i][k][0].y - 90.0f;
                    break;
                case 2:
                    switch (i) {
                    case 0:
                    case 1:
                        mPlayerHUD[i].mItemPos[k].x = mItemExPos[i][k][0].x;
                        mPlayerHUD[i].mItemPos[k].y = mItemExPos[i][k][0].y;
                        mPlayerHUD[i].mItemPos2[k].x = mItemExPos[i][k][0].x - 90.0f;
                        mPlayerHUD[i].mItemPos2[k].y = mItemExPos[i][k][0].y;
                        break;
                    default:
#line 1310
                        JUT_ASSERT(0);
                        break;
                    }
                    break;
                case 3:
                case 4:
                    switch (i) {
                    case 0:
                    case 2:
                        mPlayerHUD[i].mItemPos[k].x = mItemExPos[i][k][0].x;
                        mPlayerHUD[i].mItemPos[k].y = mItemExPos[i][k][0].y;
                        mPlayerHUD[i].mItemPos2[k].x = mItemExPos[i][k][0].x - 90.0f;
                        mPlayerHUD[i].mItemPos2[k].y = mItemExPos[i][k][0].y;
                        break;
                    case 1:
                    case 3:
                        mPlayerHUD[i].mItemPos[k].x = mItemExPos[i][k][0].x;
                        mPlayerHUD[i].mItemPos[k].y = mItemExPos[i][k][0].y;
                        mPlayerHUD[i].mItemPos2[k].x = mItemExPos[i][k][0].x + 90.0f;
                        mPlayerHUD[i].mItemPos2[k].y = mItemExPos[i][k][0].y;
                        break;
                    default:
#line 1332
                        JUT_ASSERT(0);
                        break;
                    }
                    break;
                default:
#line 1337
                    JUT_ASSERT(0);
                    break;
                }
            }
            JGeometry::TVec2f zero(0.0f, 0.0f);
            for (int j = 0; j < 10; j++)
                mPlayerHUD[i].mTimePos[j] = zero;
            for (int j = 0; j < 9; j++) {
                for (int k = 0; k < 11; k++) {
                    mPlayerHUD[i].mLapTimePos[j][k] = zero;
                    mPlayerHUD[i].mLapTimePos2[j][k] = zero;
                }
            }
            if (RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2) {
                for (int j = 0; j < 10; j++) {
                    J2DPane *pane = mScreen->search(mTimeDrawTag[i][j]);
#line 1358
                    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                    _694x[i][j] = (J2DPicture *)pane;
                    if (_694x[i][j]) {
                        JGeometry::TBox2f box(*_694x[i][j]->getBounds());
                        mPlayerHUD[i].mTimePos[j].x = (box.i.x + box.f.x) / 2;
                        mPlayerHUD[i].mTimePos[j].y = (box.i.y + box.f.y) / 2;
                    }
                    if (i == 0 && j == 0)
                        _694x[i][j]->getCornerColor(mTimeCornerColor);
                }
                for (int j = 0; j < 9; j++) {
                    for (int k = 0; k < 11; k++) {
                        J2DPane *pane = mScreen->search(mLapTimeDrawTag[i][j][k]);
#line 1375
                        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                        mLapDigits[i][j][k] = (J2DPicture *)pane;
                        if (mLapDigits[i][j][k]) {
                            JGeometry::TBox2f box(*mLapDigits[i][j][k]->getBounds());
                            mPlayerHUD[i].mLapTimePos[j][k].x = box.i.x;
                            mPlayerHUD[i].mLapTimePos[j][k].y = box.i.y;
                        }
                        if (i == 0 && j == 0 && k == 0)
                            mLapDigits[i][j][k]->getCornerColor(mLapTimeCornerColor);
                    }
                }
                if (i == 1) {
                    int lapNum = RCMGetManager()->getTotalLapNumber();
                    if (lapNum > 9)
                        lapNum = 9;
                    for (int j = 0; j < 9; j++) {
                        for (int k = 0; k < 10; k++)
                            mPlayerHUD[i].mLapTimePos[j][k] = mPlayerHUD[i].mLapTimePos[j + 9 - lapNum][k];
                    }
                    for (int j = 0; j < 9; j++)
                        mPlayerHUD[i].mLapTimePos[j][10].y = mPlayerHUD[i].mLapTimePos[0][0].y;
                }
            }
            for (int j = 0; j < 8; j++) {
                J2DPane *pane = mScreen->search(mRankTag[i][j]);
#line 1407
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mRankDigits1[i][j] = (J2DPicture *)pane;
                JGeometry::TBox2f box(*mRankDigits1[i][j]->getBounds());
                mPlayerHUD[i].mRankPos[j].x = (box.i.x + box.f.x) / 2;
                mPlayerHUD[i].mRankPos[j].y = (box.i.y + box.f.y) / 2;
            }
            if (RCMGetManager()->getConsoleNumber() == 2) {
                for (int j = 0; j < 8; j++) {
                    J2DPane *pane = mScreen->search(mRankGoalTag[i][j]);
#line 1416
                    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                    mRankDigits2[i][j] = (J2DPicture *)pane;
                    JGeometry::TBox2f box(*mRankDigits2[i][j]->getBounds());
                    mPlayerHUD[i].mRankPos2[j].x = (box.i.x + box.f.x) / 2;
                    mPlayerHUD[i].mRankPos2[j].y = (box.i.y + box.f.y) / 2;
                    if (RCMGetManager()->getRaceMode() == VERSUS_RACE) {
                        if (i == 0)
                            mPlayerHUD[i].mRankPos[j].y -= 23.0f;
                        else
                            mPlayerHUD[i].mRankPos[j].y += 23.0f;
                    }
                }
            }
        }
    }

    if (RCMGetManager()->isRaceModeMiniGame()) {
        for (int i = 0; i < RCMGetManager()->getConsoleNumber(); i++) {
            for (int j = 0; j < 3; j++) {
                for (int k = 0; k < 4; k++) {
                    J2DPane *pane = mScreen->search('StarA11' + (i << 8) + (j << 16) + k);
#line 1441
                    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                    mMiniPics[i][j][k] = (J2DPicture *)pane;
                    JGeometry::TBox2f box(*pane->getBounds());
                    mMiniPos[i][j][k].x = (box.i.x + box.f.x) / 2;
                    mMiniPos[i][j][k].y = (box.i.y + box.f.y) / 2;
                }
            }
        }
        for (int i = 0; i < RCMGetManager()->getConsoleNumber(); i++) {
            for (int j = 0; j < 2; j++) {
                J2DPane *pane = mScreen->search(i + ('Star3DA1' + (j << 8)));
#line 1453
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mStarPics[i][j] = (J2DPicture *)pane;
                JUTColor color = mPlayerNumberColor[i];
                color.a = 0;
                mStarPics[i][j]->setBlack(color);
            }
        }
        for (int i = 0; i < RCMGetManager()->getConsoleNumber(); i++) {
            J2DPane *pane = mScreen->search('Shine1' + i);
            mShineBase[i].x = pane->getTranslateX();
            mShineBase[i].y = pane->getTranslateY();
            mShineScreen[i] = new (heap, 0) J2DScreen();
            mShineScreen[i]->set("ShineIndication.blo", GETJ2DManager()->getArchive());
            for (int j = 0; j < 5; j++) {
                J2DPane *pane = mShineScreen[i]->search(mShineTimeTag[j]);
#line 1472
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mShinePics[i][j] = (J2DPicture *)pane;
                mShinePos[j].x = pane->getTranslateX();
                mShinePos[j].y = pane->getTranslateY();
            }
        }
    }

    J2DScreen *screen = nullptr;
    J2DAnmTransform *anm = nullptr;
    switch (RCMGetManager()->getConsoleNumber()) {
    case 1:
        screen = new (heap, 0) J2DScreen();
        screen->set("p1_race.blo", 0x40000, GETJ2DManager()->getArchive());
        anm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRGetNameResource("p1_race.bck", GETJ2DManager()->getArchive()));
        screen->setAnimation(anm);
        break;
    case 2:
        screen = new (heap, 0) J2DScreen();
        screen->set("p2_race.blo", 0x40000, GETJ2DManager()->getArchive());
        anm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRGetNameResource("p2_race.bck", GETJ2DManager()->getArchive()));
        screen->setAnimation(anm);
        break;
    case 3:
    case 4:
        screen = new (heap, 0) J2DScreen();
        screen->set("p4_race.blo", 0x40000, GETJ2DManager()->getArchive());
        anm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRGetNameResource("p4_race.bck", GETJ2DManager()->getArchive()));
        screen->setAnimation(anm);
        break;
    default:
#line 1512
        JUT_ASSERT(0);
        break;
    }
    f32 ofs = 0.0f;
    if (RCMGetManager()->isRaceModeMiniGame()) {
        switch (RCMGetManager()->getConsoleNumber()) {
        case 1:
            ofs = 75.0f;
            break;
        case 2:
            ofs = 40.0f;
            break;
        }
    }
    for (int i = 0; i < RCMGetManager()->getConsoleNumber(); i++) {
        for (int k = 0; k < 2; k++) {
            J2DPane *pane = screen->search(mItemExDrawTag[i][k][3]);
            for (int f = 0; f < 39; f++) {
                anm->setFrame(f);
                screen->animation();
                JGeometry::TBox2f box(*pane->getBounds());
                mItemPos[i][k][f].x = (box.i.x + box.f.x) / 2 - ofs;
                mItemPos[i][k][f].y = (box.i.y + box.f.y) / 2;
                mItemPos[i][k][f].z = pane->getScaleX();
            }
        }
    }
    delete screen;

    J2DScreen *charScreen = new (heap, 0) J2DScreen();
    charScreen->set("CharacterIndication.blo", 0x40000, GETJ2DManager()->getArchive());
    anm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRGetNameResource("CharacterIndication.bck", GETJ2DManager()->getArchive()));
    charScreen->setAnimation(anm);
    for (int k = 0; k < 2; k++) {
        J2DPane *pane = charScreen->search('cw11' + k);
        for (int f = 0; f < 39; f++) {
            anm->setFrame(f);
            charScreen->animation();
            JGeometry::TBox2f box(*pane->getBounds());
            mCharPos[k][f].x = (box.i.x + box.f.x) / 2;
            mCharPos[k][f].y = (box.i.y + box.f.y) / 2;
            mCharPos[k][f].z = pane->getScaleX();
        }
    }
    delete charScreen;

    if (RCMGetManager()->isRaceModeMiniGame()) {
        J2DScreen *shineScreen = new (heap, 0) J2DScreen();
        shineScreen->set("ShineIndication.blo", 0x40000, GETJ2DManager()->getArchive());
        anm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRGetNameResource("ShineIndication.bck", GETJ2DManager()->getArchive()));
        shineScreen->setAnimation(anm);
        J2DPane *pane = shineScreen->search('ShineB');
        for (int f = 0; f < 50; f++) {
            anm->setFrame(f);
            shineScreen->animation();
            mSpecialItemScale[f] = pane->getScaleX();
        }
        delete shineScreen;
    }

    if (RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2) {
        for (int i = 0; i < 8; i++) {
            mCharWindow[i].mScreen = new (heap, 0) J2DScreen();
            mCharWindow[i].mScreen->set("CharacterIndication.blo", GETJ2DManager()->getArchive());
            for (int j = 0; j < 2; j++) {
                J2DPane *pane = mCharWindow[i].mScreen->search('cw11' + j);
#line 1602
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mCharWindow[i].mPics[j][0] = (J2DPicture *)pane;
                if (i < RCMGetManager()->getPlayerNumber())
                    mCharWindow[i].mPics[j][0]->changeTexture("chara_Window2.bti", 0);
                pane = mCharWindow[i].mScreen->search('c11' + j);
#line 1610
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mCharWindow[i].mPics[j][1] = (J2DPicture *)pane;
            }
            mCharWindow[i].mPane = mCharWindow[i].mScreen->search('posc1');
        }
        for (int i = 0; i < 8; i++) {
            J2DPane *pane = mScreen->search('CI_01' + i);
            mCharIconPos[i].x = pane->getTranslateX();
            mCharIconPos[i].y = pane->getTranslateY();
        }
        JGeometry::TBox2f box1(*mCharWindow[0].mPics[1][1]->getBounds());
        JGeometry::TBox2f box2(*mCharWindow[0].mPics[1][0]->getBounds());
        mCharWindowOfs.x = (box1.i.x + box1.f.x) / 2 - (box2.i.x + box2.f.x) / 2;
        mCharWindowOfs.y = (box1.i.y + box1.f.y) / 2 - (box2.i.y + box2.f.y) / 2;
        JGeometry::TBox2f box3(*mCharWindow[0].mPane->getBounds());
        mCharWindowCenter.x = (box3.i.x + box3.f.x) / 2;
        mCharWindowCenter.y = (box3.i.y + box3.f.y) / 2;
    }

    mTask = new (heap, 0) Task(0x80);
    init();
}

void Race2D::init()
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++) {
            mMark[i][j]->setAlpha(0);
            mMarkOrder[i][j] = 0;
        }
    }

    if (RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2) {
        for (int i = 0; i < 2; i++) {
            mAnmTA[i].mCount[0] = 0;
            mAnmTA[i].mCount[1] = 0;
            mAnmTA[i].mCount[2] = 0;
            mAnmTA[i].mCount[3] = 0;
            mAnmTA[i].mNum = 0;
        }
        for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
            _694x[i][0]->changeTexture(mTimeLapName[0], 0);
            for (int j = 0; j < 10; j++) {
                _694x[i][j]->setAlpha(255);
                _694x[i][j]->setBlack(JUTColor(0));
                _694x[i][j]->setCornerColor(mTimeCornerColor);
            }
            for (int j = 0; j < 9; j++) {
                for (int k = 0; k < 11; k++)
                    mLapDigits[i][j][k]->setBlack(JUTColor(0));
            }
        }
    }

    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
        mPlayerHUD[i].mLap = 0;
        mPlayerHUD[i]._24 = 1;
        mPlayerHUD[i].mRaceTimeAvailable = 0;
        mPlayerHUD[i].mDigits[0] = 0;
        mPlayerHUD[i].mDigits[1] = 0;
        mPlayerHUD[i].mDigits[2] = 0;
        mPlayerHUD[i].mDigits[3] = 0;
        mPlayerHUD[i].mDigits[4] = 0;
        mPlayerHUD[i].mDigits[5] = 0;
        mPlayerHUD[i].mDigits[6] = 0;
        for (int j = 0; j < 9; j++) {
            mPlayerHUD[i].mLapSlots[j].mAvailable = 0;
            mPlayerHUD[i].mLapSlots[j].mDigits[0] = 0;
            mPlayerHUD[i].mLapSlots[j].mDigits[1] = 0;
            mPlayerHUD[i].mLapSlots[j].mDigits[2] = 0;
            mPlayerHUD[i].mLapSlots[j].mDigits[3] = 0;
            mPlayerHUD[i].mLapSlots[j].mDigits[4] = 0;
            mPlayerHUD[i].mLapSlots[j].mDigits[5] = 0;
            mPlayerHUD[i].mLapSlots[j].mDigits[6] = 0;
        }
        for (int k = 0; k < 2; k++) {
            mPlayerHUD[i].mItem[k][0] = 16;
            mPlayerHUD[i].mItem[k][1] = 16;
            mPlayerHUD[i]._7f8[k] = false;
            mPlayerHUD[i]._7fa[k] = true;
            mPlayerHUD[i].mItemPos3[k].x = 0.0f;
            mPlayerHUD[i].mItemPos3[k].y = 0.0f;
            mSpecialItem[i][k]->mIsVisible = false;
            for (int j = 0; j < 6; j++)
                mKiraPics[i][k][j]->mIsVisible = false;
        }
        mPlayerHUD[i]._848 = 0;
        mPlayerHUD[i]._84c[0] = 0;
        mPlayerHUD[i]._84c[1] = 1;
        mPlayerHUD[i]._8d4 = 0;
        mPlayerHUD[i].mIsAlarm = false;
        for (int j = 0; j < 4; j++)
            mPlayerHUD[i].mAlarmAlpha[j] = 0;
        mLapPic[i]->mIsVisible = true;
        mLapPic[i]->setAlpha(255);
        if (RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2) {
            for (int j = 0; j < 9; j++) {
                for (int k = 0; k < 11; k++) {
                    mLapDigits[i][j][k]->setAlpha(0);
                    mLapDigits[i][j][k]->mIsVisible = false;
                    mPlayerHUD[i].mLapTimePos2[j][k].x = mPlayerHUD[i].mLapTimePos[j][k].x;
                    mPlayerHUD[i].mLapTimePos2[j][k].y = mPlayerHUD[i].mLapTimePos[j][k].y;
                }
            }
        }
        for (int j = 0; j < 8; j++) {
            mRankDigits1[i][j]->mIsVisible = true;
            mRankDigits1[i][j]->setAlpha(255);
        }
        if (RCMGetManager()->getConsoleNumber() == 2) {
            for (int j = 0; j < 8; j++) {
                mRankDigits2[i][j]->mIsVisible = false;
                mRankDigits2[i][j]->setAlpha(0);
            }
        }
    }

    if (RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2) {
        for (int i = 0; i < RCMGetManager()->getKartNumber(); i++) {
            mKartSlot[i]._0 = 0;
            mKartSlot[i]._4 = 0;
            mKartSlot[i]._8 = 1;
            for (int j = 0; j < 2; j++) {
                int charID = gRaceInfo.getKartInfo(i)->getDriverCharID(j);
                if (charID > 0 && charID < 21)
                    mCharWindow[i].mPics[j][1]->changeTexture(mCharacterTex[charID - 1], 0);
            }
            int rank = RCMGetManager()->getKartChecker(i)->getRank() - 1;
            if (rank >= 0 && rank < RCMGetManager()->getKartNumber()) {
                mCharWindow[i].mPos.x = mCharIconPos[rank].x;
                mCharWindow[i].mPos.y = mCharIconPos[rank].y;
                mCharWindow[i]._2c.x = mCharWindow[i].mPos.x;
                mCharWindow[i]._2c.y = mCharWindow[i].mPos.y;
                mCharWindow[i]._1c.x = mCharWindow[i]._2c.x;
                mCharWindow[i]._1c.y = mCharWindow[i]._2c.y;
                mCharWindow[i]._34[1] = 255;
                mCharWindow[i]._34[2] = 255;
                mCharWindow[i]._34[0] = 255;
            } else {
                mCharWindow[i].mPos.x = 0.0f;
                mCharWindow[i].mPos.y = 0.0f;
                mCharWindow[i]._2c.x = mCharWindow[i].mPos.x;
                mCharWindow[i]._2c.y = mCharWindow[i].mPos.y;
                mCharWindow[i]._1c.x = mCharWindow[i]._2c.x;
                mCharWindow[i]._1c.y = mCharWindow[i]._2c.y;
                mCharWindow[i]._34[1] = 255;
                mCharWindow[i]._34[2] = 255;
                mCharWindow[i]._34[0] = 255;
            }
        }
        _4e9c[0] = 0;
        _4e9c[1] = 0;
    }

    sChangeFrame = 0;
    sColorIndex = 0;
    for (int i = 0; i < 8; i++)
        sSpinRotate[i] = 0.0f;
    for (int i = 0; i < 4; i++)
        sPreWhichDriver[i] = 0;
    for (int i = 0; i < 8; i++)
        sPreWhichDriver2[i] = 0;
    for (int i = 0; i < 4; i++)
        sPreLap[i] = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 2; j++) {
            sRollingFlag[i][j] = false;
            sItemState[i][j] = 0;
            sUseFlag[i][j] = false;
        }
    }
    sPreShineFlag = false;
    sPreShinePlayer = -1;
    for (int i = 0; i < 4; i++) {
        mBattleTimeCount[i] = 0;
        mShineKind[i] = 0;
        mBombWinFrame[i] = 0;
        sPreBombPoint[i] = 0;
        for (int j = 0; j < 4; j++) {
            mBombMarkFrame[i][j] = 0;
            for (int k = 0; k < 3; k++)
                _ec0[i][j][k] = nullptr;
        }
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 2; k++)
                _f80[i][j][k] = nullptr;
        }
    }
    mTimeColorFrame = 0;
    mBombColorFrame = 0;

    if (RCMGetManager()->getRaceMode() == TIME_ATTACK) {
        TARecord rec = *gSystemRecord.getBestLap(ResMgr::getCourseID());
        if (rec.isValid())
            sBestLapTime = rec.getRaceTime();
        else
            sBestLapTime = RaceTime();
        mLapGhost->init();
        mCanNotSave->init();
    }
    if (RCMGetManager()->getRaceMode() == BOMB_BATTLE)
        mBombStart->init();

    for (int i = 0; i < 4; i++) {
        sPreRank[i] = -1;
        sRankAnm[i] = 0;
    }
    for (int i = 0; i < 8; i++) {
        sPreThunder[i] = 0;
        sThunderAnm[i] = 0;
    }

    if (mMaskScreen)
        mMaskScreen->search('P3Mask')->setAlpha(255);

    mTask->init();
    if (mScreen) {
        mTask->make(calcLapInit, 0x8000);
        mTask->make(calcItem, 0x8000);
    }
    mDrawFlag = false;
    mIsEndBombDemo = false;
    mGoalCount = 0;
    mEscapeTimer = 0;
    mEscapeTimer4 = 0;
    mHideFrame = 0;
}

Race2D::~Race2D() { delete mHio; }

void Race2D::drawPlayerMark()
{
    if (!(mHio->getParam()->mFlag & 1))
        return;
    if (!mDrawFlag)
        return;
    if (RCMGetManager()->getConsoleNumber() < 2 && !J2DManager::getNetFlag())
        return;

    mGrafContext->setPort();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 7);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_TEX0, GX_TEX_ST, GX_U8, 7);

    for (int i = 0; i < 2; i++)
    {
        GXSetTevColorOp((GXTevStageID)i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaOp((GXTevStageID)i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTexCoordGen2((GXTexCoordID)i, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    }
    GXSetCurrentMtx(GX_PNMTX0);

    f32 scale = 1.0f;
    switch (RCMGetManager()->getConsoleNumber())
    {
    case 1:
        scale = 1.0f;
        break;
    case 2:
        scale = 0.85f;
        break;
    case 3:
    case 4:
        scale = 0.75f;
        break;
    default:
#line 1927
        JUT_ASSERT_F(0, "%s", "0");
        break;
    }

    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++)
    {
        GXSetScissor(mAlarm[status].mBase, mAlarm[status]._4, mAlarm[status].mStep, mAlarm[status]._c);
        for (int item = 0; item < RCMGetManager()->getPlayerNumber(); item++)
        {
            int kartNo = mMarkOrder[status][item];
            if (J2DManager::getKart2Status(kartNo) == status)
                continue;
            KartDrawer *kd = RCMGetManager()->getKartDrawer(kartNo);
            if (kd->getHiddenDrivers() != 0)
                continue;

            K2DPicture *pic = (K2DPicture *)mMark[status][kartNo];
            f32 w = pic->mPicture.getWidth() * scale;
            f32 h = pic->mPicture.getHeight() * scale;
            pic->drawK2D(mMarkPos[status][kartNo].x, mMarkPos[status][kartNo].y - h, w, h, true);

            if (RCMGetManager()->getRaceMode() != 6)
                continue;
            u8 alpha = mMark[status][kartNo]->getAlpha();
            if (!isBombReach(kartNo))
                continue;

            for (int type = 0; type < 2; type++)
            {
                f32 s = getReachBombScale(kartNo, type);
                f32 bw = mStarPics[kartNo][type]->getWidth() * s;
                f32 bh = mStarPics[kartNo][type]->getHeight() * s;
                mStarPics[kartNo][type]->setAlpha(alpha);
                ((K2DPicture *)mStarPics[kartNo][type])->drawK2D(10.0f * scale + (mMarkPos[status][kartNo].x - bw / 2.0f),
                                                                 mMarkPos[status][kartNo].y - bh / 2.0f - 47.0f * scale, bw, bh, true);
            }
        }
    }

    GXSetScissor(System::get2DScisX(), System::get2DScisY(), System::get2DScisW(), System::get2DScisH());
}

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

void Race2D::draw()
{
    Race2DParam *param = mHio->getParam();
    if (!(param->mFlag & 1))
        return;
    if (!mDrawFlag)
        return;

    int goalFrame = 0;
    if (RCMGetManager()->isRaceEnd())
        goalFrame = GETJ2DManager()->mAnmFrame;

    int goalAnmFrame[4];
    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
        int frame = GETJ2DManager()->getGoalAnmFrame(status);
        if (frame < goalFrame)
            goalAnmFrame[status] = goalFrame;
        else
            goalAnmFrame[status] = frame;
    }

    f32 scaleA;
    getStartScaleA(mEscapeTimer, scaleA);
    f32 scaleB;
    getStartScaleB(mEscapeTimer, scaleB);

    mGrafContext->setPort();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 7);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_TEX0, GX_TEX_ST, GX_U8, 7);

    for (int i = 0; i < 2; i++) {
        GXSetTevColorOp((GXTevStageID)i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaOp((GXTevStageID)i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTexCoordGen2((GXTexCoordID)i, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    }
    GXSetCurrentMtx(GX_PNMTX0);

    if (RCMGetManager()->getConsoleNumber() == 1 && !RCMGetManager()->isRaceModeMiniGame()) {
        f32 goalX = 0.0f;
        if (goalAnmFrame[0] > 0)
            getGoalSpeedMeterPos(goalAnmFrame[0], goalX);
        for (int i = 0; i < 7; i++) {
            f32 scale = 1.0f;
            getStartSpeedMeterScale(mEscapeTimer, i, scale);
            K2DPicture *pic = (K2DPicture *)mSpeedDrawPics[i];
            f32 w = scaleB * (scale * pic->mPicture.getWidth());
            f32 h = scaleB * (scale * pic->mPicture.getHeight());
            pic->drawK2D(mSpeedDrawPos[i].x - w / 2 - goalX, mSpeedDrawPos[i].y - h / 2, w, h, true);
        }
        for (int i = 7; i < 11; i++) {
            K2DPicture *pic = (K2DPicture *)mSpeedDrawPics[i];
            f32 w = scaleA * pic->mPicture.getWidth();
            f32 h = scaleA * pic->mPicture.getHeight();
            pic->drawK2D(mSpeedDrawPos[i].x - w / 2 - goalX, mSpeedDrawPos[i].y - h / 2, w, h, true);
        }
    }

    if (RCMGetManager()->getConsoleNumber() == 4 && RCMGetManager()->getStatusNumber() == 3 && !J2DManager::getNetFlag())
        ((K2DPicture *)mDemoPic)->drawK2D(mDemoPos.x, mDemoPos.y, mDemoPic->getWidth(), mDemoPic->getHeight(), true);

    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
        if (RCMGetManager()->getRaceMode() != TIME_ATTACK && (!RCMGetManager()->isRaceModeMiniGame() || (param->mFlag & 2))) {
            JGeometry::TVec2f startPos(0.0f, 0.0f);
            getStartRankPos(status, mEscapeTimer, startPos);
            JGeometry::TVec2f goalPos(0.0f, 0.0f);
            f32 goalScale = 1.0f;
            f32 goalRot = 0.0f;
            if (goalFrame > 0) {
                getGoalRankPos(status, goalFrame, goalPos);
                getGoalRankScale(status, goalFrame, goalScale);
                getGoalRankRot(goalFrame, goalRot);
            }
            f32 rankScale = getRaceRankScale(sRankAnm[status]);
            for (int i = 0; i < 8; i++) {
                f32 w = rankScale * (scaleA * (goalScale * mRankDigits1[status][i]->getWidth()));
                f32 h = rankScale * (scaleA * (goalScale * mRankDigits1[status][i]->getHeight()));
                mRankDigits1[status][i]->rotate(w / 2, h / 2, J2DROTATE_Z, goalRot);
                ((K2DPicture *)mRankDigits1[status][i])->drawK2D(goalPos.x + (startPos.x + (mPlayerHUD[status].mRankPos[i].x - w / 2)),
                                                               goalPos.y + (startPos.y + (mPlayerHUD[status].mRankPos[i].y - h / 2)), w, h, true);
            }
            if (RCMGetManager()->getConsoleNumber() == 2) {
                for (int i = 0; i < 8; i++) {
                    f32 w = rankScale * (goalScale * mRankDigits2[status][i]->getWidth());
                    f32 h = rankScale * (goalScale * mRankDigits2[status][i]->getHeight());
                    mRankDigits2[status][i]->rotate(w / 2, h / 2, J2DROTATE_Z, goalRot);
                    ((K2DPicture *)mRankDigits2[status][i])->drawK2D(goalPos.x + (startPos.x + (mPlayerHUD[status].mRankPos2[i].x - w / 2)),
                                                                   goalPos.y + (startPos.y + (mPlayerHUD[status].mRankPos2[i].y - h / 2)), w, h, true);
                }
            }
        }

        if (!RCMGetManager()->isRaceModeMiniGame()) {
            f32 startX = 0.0f;
            getStartLapPos(status, mEscapeTimer, startX);
            f32 goalX = 0.0f;
            if (goalAnmFrame[status] > 0) {
                getGoalLapPos(goalAnmFrame[status], goalX);
                if (RCMGetManager()->getConsoleNumber() != 2 && status % 2 != 0)
                    goalX *= -1.0f;
            }
            for (int i = 0; i < 4; i++) {
                K2DPicture *pic = (K2DPicture *)mLapPics[status][i];
                f32 w = scaleB * pic->mPicture.getWidth();
                f32 h = scaleB * pic->mPicture.getHeight();
                pic->drawK2D(goalX + (startX + (mPlayerHUD[status].mLapPos[i].x - w / 2)), mPlayerHUD[status].mLapPos[i].y - h / 2, w, h, true);
            }
        }

        f32 x, y, z;
        getItemInfo(status, mPlayerHUD[status]._84c[0], mPlayerHUD[status]._848 % 39, x, y, z);
        int drv[2];
        if (z > 0.8f) {
            drv[0] = 1;
            drv[1] = 0;
        } else {
            drv[0] = 0;
            drv[1] = 1;
        }

        for (int d = 0; d < 2; d++) {
            int dd = drv[d];
            if (mPlayerHUD[status].mItem[dd][0] == 16)
                continue;
            if (RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(status))->isGoal())
                continue;
            f32 dx = mPlayerHUD[status].mItemPos3[dd].x - mPlayerHUD[status].mItemPos[dd].x;
            f32 dy = mPlayerHUD[status].mItemPos3[dd].y - mPlayerHUD[status].mItemPos[dd].y;
            f32 ix, iy, iscale;
            getItemInfo(status, mPlayerHUD[status]._84c[dd], mPlayerHUD[status]._848 % 39, ix, iy, iscale);
            f32 rate = (f32)(mPlayerHUD[status].mItemCount[dd] % mPlayerHUD[status].mItemInterval[dd]) / (f32)mPlayerHUD[status].mItemInterval[dd];

            K2DPicture *pic = (K2DPicture *)mItemExDraw[status][dd][0];
            f32 w = iscale * pic->mPicture.getWidth();
            f32 h = iscale * pic->mPicture.getHeight();
            pic->drawK2D(ix + dx - w / 2, iy + dy - h / 2, w, h, true);
            f32 sx = ix + dx - w / 2;
            f32 sy = iy + dy - h / 2;
            if (0.0f <= sx && 0.0f <= sy) {
                GXSetScissor(sx, sy, w, h);
            } else {
                f32 sh = h;
                if (sx < 0.0f) {
                    w += sx;
                    sx = 0.0f;
                }
                if (sy < 0.0f) {
                    sh += sy;
                    sy = 0.0f;
                }
                if (0.0f <= w && 0.0f <= sh)
                    GXSetScissor(sx, sy, w, sh);
            }

            pic = (K2DPicture *)mItemExDraw[status][dd][1];
            w = iscale * pic->mPicture.getWidth();
            h = iscale * pic->mPicture.getHeight();
            pic->drawK2D(ix + dx - w / 2, iy + dy - h / 2 + h * rate - h, w, h, true);

            pic = (K2DPicture *)mItemExDraw[status][dd][2];
            w = iscale * pic->mPicture.getWidth();
            h = iscale * pic->mPicture.getHeight();
            pic->drawK2D(ix + dx - w / 2, iy + dy - h / 2 + h * rate, w, h, true);

            GXSetScissor(System::get2DScisX(), System::get2DScisY(), System::get2DScisW(), System::get2DScisH());

            pic = (K2DPicture *)mItemExDraw[status][dd][3];
            w = iscale * pic->mPicture.getWidth();
            h = iscale * pic->mPicture.getHeight();
            pic->drawK2D(ix + dx - w / 2, iy + dy - h / 2, w, h, true);

            for (int k = 4; k < 6; k++) {
                pic = (K2DPicture *)mItemExDraw[status][dd][k];
                f32 ox = iscale * (mItemExPos[status][dd][k].x - mPlayerHUD[status].mItemPos[dd].x);
                f32 oy = iscale * (mItemExPos[status][dd][k].y - mPlayerHUD[status].mItemPos[dd].y);
                w = iscale * pic->mPicture.getWidth();
                h = iscale * pic->mPicture.getHeight();
                pic->drawK2D(ox + (ix + dx) - w / 2, oy + (iy + dy) - h / 2, w, h, true);
            }
        }

        for (int d = 0; d < 2; d++) {
            int dd = drv[d];
            if (mPlayerHUD[status].mItem[dd][0] == 16)
                continue;
            if (RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(status))->isGoal())
                continue;
            f32 dx = mPlayerHUD[status].mItemPos3[dd].x - mPlayerHUD[status].mItemPos[dd].x;
            f32 dy = mPlayerHUD[status].mItemPos3[dd].y - mPlayerHUD[status].mItemPos[dd].y;
            f32 ix, iy, iscale;
            getItemInfo(status, mPlayerHUD[status]._84c[dd], mPlayerHUD[status]._848 % 39, ix, iy, iscale);
            K2DPicture *pic = (K2DPicture *)mSpecialItem[status][dd];
            f32 w = iscale * (pic->mPicture.getWidth() * pic->mPicture.getScaleX());
            f32 h = iscale * (pic->mPicture.getHeight() * pic->mPicture.getScaleY());
            pic->drawK2D(ix + dx - w / 2, iy + dy - h / 2, w, h, true);
        }

        int timeOrder[10] = {0, 1, 2, 4, 5, 7, 8, 9, 3, 6};
        if ((RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2) &&
            !RCMGetManager()->isRaceModeMiniGame() && RCMGetManager()->getRaceMode() != VERSUS_RACE) {
            f32 goalX = 0.0f;
            if (goalFrame > 0)
                getGoalLapTimePos(goalFrame, 0, goalX);
            for (int i = 0; i < 10; i++) {
                f32 startX = 0.0f;
                int idx = timeOrder[i];
                getStartLapTimePos(mEscapeTimer, idx, startX);
                K2DPicture *pic = (K2DPicture *)_694x[status][idx];
                f32 w = scaleB * pic->mPicture.getWidth();
                f32 h = scaleB * pic->mPicture.getHeight();
                pic->drawK2D(goalX + (startX + (mPlayerHUD[status].mTimePos[idx].x - w / 2)), mPlayerHUD[status].mTimePos[idx].y - h / 2, w, h, true);
            }

            K2DPicture *pic = (K2DPicture *)mLapDigits[status][0][10];
            pic->drawK2D(mPlayerHUD[status].mLapTimePos2[0][10].x + goalX, mPlayerHUD[status].mLapTimePos2[0][10].y,
                         pic->mPicture.getWidth(), pic->mPicture.getHeight(), true);

            for (int lap = 0; lap < 9; lap++) {
                if (goalFrame > 0)
                    getGoalLapTimePos(goalFrame, lap, goalX);
                for (int i = 0; i < 10; i++) {
                    K2DPicture *pic = (K2DPicture *)mLapDigits[status][lap][i];
                    pic->drawK2D(mPlayerHUD[status].mLapTimePos2[lap][i].x + goalX, mPlayerHUD[status].mLapTimePos2[lap][i].y,
                                 pic->mPicture.getWidth(), pic->mPicture.getHeight(), true);
                }
            }
        }
    }

    drawAlarm();
    drawMiniGame();
}

void Race2D::drawAlarm()
{
    u32 alarmItemKind[4] = { 2, 11, 7, 13 };
    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++)
    {
        mPlayerHUD[status].mIsAlarm = false;
        if (RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(status))->isGoal())
            continue;

        if (RCMGetManager()->mRaceDirector->isRaceEnd())
            break;

        GXSetScissor(mAlarm[status].mBase, mAlarm[status]._4, mAlarm[status].mStep, mAlarm[status]._c);

        for (int i = 0; i < 4; i++)
        {
            bool drawn = false;
            for (JSUListIterator<ItemObj> it(ItemObjMgr::gItemMgr->_7fc.getFirst()); it != ItemObjMgr::gItemMgr->_7fc.getEnd(); it++)
            {
                ItemObj *item = it.getObject();
                if (item == NULL)
                    continue;
                int kartNo = J2DManager::getStatus2Kart(status);
                if (kartNo == item->mOwnerNum)
                    continue;
                if (item->mState == 3 || item->mState == 4)
                    continue;
                if (item->getKind() != alarmItemKind[i])
                    continue;

                KartLoader *loader = RCMGetManager()->getKartLoader(kartNo);
                JGeometry::TVec3f pos = item->mPos;
                f32 dx = pos.x - *(f32 *)((u8 *)loader + 0x4c);
                f32 dy = pos.y - *(f32 *)((u8 *)loader + 0x5c);
                f32 dz = pos.z - *(f32 *)((u8 *)loader + 0x6c);
                if (dx * dx + dy * dy + dz * dz < 16000000.0f)
                {
                    if (item->mItemKind == 11)
                    {
                        if (ItemObjMgr::getItemYoshiEgg(item)->mColorID == 1)
                            mAlarmPic1->changeTexture(mItemTex[item->mItemKind], 0);
                        else
                            mAlarmPic1->changeTexture(mItemTex[27], 0);
                    }
                    else
                    {
                        mAlarmPic1->changeTexture(mItemTex[item->mItemKind], 0);
                    }

                    JGeometry::TVec2f screenPos;
                    if (SiUtil::getScreenPos((long)status, pos, &screenPos, RCMGetManager()->isMirror()) == 0xffffffff)
                    {
                        alarmX(status, screenPos.x);

                        mAlarmPic0->setAlpha(mPlayerHUD[status].mAlarmAlpha[i]);
                        ((K2DPicture *)mAlarmPic0)->drawK2D(screenPos.x + mAlarmPos0.x, mAlarmX[status] + mAlarmPos0.y,
                                                           mAlarmPic0->getWidth(), mAlarmPic0->getHeight(), true);

                        mAlarmPic1->setAlpha(mPlayerHUD[status].mAlarmAlpha[i]);
                        ((K2DPicture *)mAlarmPic1)->drawK2D(screenPos.x + mAlarmPos1.x, mAlarmX[status] + mAlarmPos1.y,
                                                           mAlarmPic1->getWidth(), mAlarmPic1->getHeight(), true);

                        drawn = true;
                        mPlayerHUD[status].mIsAlarm = true;
                    }
                }
            }

            if (drawn)
            {
                mPlayerHUD[status].mAlarmAlpha[i] += 26;
                if (mPlayerHUD[status].mAlarmAlpha[i] >= 255)
                    mPlayerHUD[status].mAlarmAlpha[i] = 255;
            }
            else
            {
                mPlayerHUD[status].mAlarmAlpha[i] -= 13;
                if (mPlayerHUD[status].mAlarmAlpha[i] < 0)
                    mPlayerHUD[status].mAlarmAlpha[i] = 0;
            }
        }
    }

    GXSetScissor(System::get2DScisX(), System::get2DScisY(), System::get2DScisW(), System::get2DScisH());
}

void Race2D::anmTA(int no)
{
    if (mAnmTA[no].mCount[0] != 0)
    {
        int frame = mAnmTA[no].mCount[0]++;
        if (frame < 246)
        {
            u8 alpha;
            getAlphaAnmTA(frame, alpha);
            for (int i = 0; i < 10; i++)
                _694x[no][i]->setAlpha(alpha);
        }
        else
        {
            mAnmTA[no].mCount[0] = 0;
            _694x[no][0]->changeTexture(mTimeLapName[0], 0);
            for (int i = 0; i < 10; i++)
            {
                _694x[no][i]->setCornerColor(mTimeCornerColor);
                _694x[no][i]->setAlpha(255);
            }
        }
    }

    if (mAnmTA[no].mCount[1] != 0)
    {
        int frame = mAnmTA[no].mCount[1]++;
        switch (frame)
        {
        case 1:
            GameAudio::Main::msBasic->startSystemSe(0x2005f);
            break;
        case 17:
            GameAudio::Main::msBasic->startSystemSe(0x20060);
            break;
        case 33:
            GameAudio::Main::msBasic->startSystemSe(0x20061);
            break;
        case 49:
            GameAudio::Main::msBasic->startSystemSe(0x20062);
            break;
        case 65:
            GameAudio::Main::msBasic->startSystemSe(0x20063);
            break;
        case 81:
            GameAudio::Main::msBasic->startSystemSe(0x20064);
            break;
        }

        if (frame < 192)
        {
            JUTColor color;
            J2DPicture::TCornerColor cornerColor;
            getBestTimeColor(frame, color, cornerColor);
            for (int i = 0; i < 10; i++)
            {
                _694x[no][i]->setBlack(color);
                _694x[no][i]->setCornerColor(cornerColor);
            }
        }
        else
        {
            mAnmTA[no].mCount[1] = 0;
            _694x[no][0]->changeTexture(mTimeLapName[0], 0);
            for (int i = 0; i < 10; i++)
            {
                _694x[no][i]->setCornerColor(mTimeCornerColor);
                _694x[no][i]->setBlack(JUTColor(0));
                _694x[no][i]->setCornerColor(mTimeCornerColor);
            }
        }
    }

    if (mAnmTA[no].mCount[2] != 0)
    {
        int frame = mAnmTA[no].mCount[2]++;
        JUTColor color;
        J2DPicture::TCornerColor cornerColor;
        getBestTimeColor(frame, color, cornerColor);
        for (int i = 0; i < 10; i++)
        {
            _694x[no][i]->setBlack(color);
            _694x[no][i]->setCornerColor(cornerColor);
        }
    }

    if (mAnmTA[no].mCount[3] != 0)
    {
        int frame = mAnmTA[no].mCount[3]++;
        JUTColor color;
        getBestLapColor(frame, color);
        for (int lap = 0; lap < 9; lap++)
        {
            for (int d = 0; d < 9; d++)
            {
                if (lap == mAnmTA[no].mNum)
                    mLapDigits[no][lap][d]->setBlack(color);
            }
        }
        mLapDigits[no][0][10]->setBlack(color);
    }
}

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
    u8 b0 = 127.0f * t + 128.0f;
    u8 r1 = 177.0f * t + 78.0f;
    u8 g1 = 255.0f * t;
    u8 b1 = 128.0f * t;
    cornerColor.mColor0.r = cornerColor.mColor3.r = 255;
    cornerColor.mColor0.g = cornerColor.mColor3.g = 255;
    cornerColor.mColor0.b = cornerColor.mColor3.b = b0;
    cornerColor.mColor0.a = cornerColor.mColor3.a = 255;
    cornerColor.mColor1.r = cornerColor.mColor2.r = r1;
    cornerColor.mColor1.g = cornerColor.mColor2.g = g1;
    cornerColor.mColor1.b = cornerColor.mColor2.b = b1;
    cornerColor.mColor1.a = cornerColor.mColor2.a = 255;
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

void Race2D::drawBomb()
{
    if (mIsEndBombDemo) {
        for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
            int kartNo = J2DManager::getStatus2Kart(status);
            int win = (RCMGetManager()->getKartChecker(kartNo), KartChecker::getWinBombPoint());
            RCMGetManager()->getKartChecker(kartNo);
            u8 alpha = K2DPicture::mK2DGlobalAlpha;
            if (isBombWinner(status))
                K2DPicture::mK2DGlobalAlpha = 255;
            for (int j = 0; j < win; j++) {
                f32 scale = getBombScale(status, 0, j);
                K2DPicture *pic = (K2DPicture *)mMiniPics[status][0][j];
                f32 w = scale * pic->mPicture.getWidth();
                f32 h = scale * pic->mPicture.getHeight();
                pic->drawK2D(mMiniPos[status][0][j].x - w / 2, mMiniPos[status][0][j].y - h / 2, w, h, true);
            }
            if (isBombWinner(status))
                K2DPicture::mK2DGlobalAlpha = alpha;
        }
        for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
            int kartNo = J2DManager::getStatus2Kart(status);
            int win = (RCMGetManager()->getKartChecker(kartNo), KartChecker::getWinBombPoint());
            int point = RCMGetManager()->getKartChecker(kartNo)->getBombPoint();
            u8 alpha = K2DPicture::mK2DGlobalAlpha;
            if (isBombWinner(status))
                K2DPicture::mK2DGlobalAlpha = 255;
            for (int j = 0; j < point; j++) {
                f32 scale = getBombScale(status, 1, j);
                K2DPicture *pic = (K2DPicture *)mMiniPics[status][1][j];
                f32 w = scale * pic->mPicture.getWidth();
                f32 h = scale * pic->mPicture.getHeight();
                pic->drawK2D(mMiniPos[status][1][j].x - w / 2, mMiniPos[status][1][j].y - h / 2, w, h, true);
            }
            if (isBombWinner(status))
                K2DPicture::mK2DGlobalAlpha = alpha;
        }
        for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
            int kartNo = J2DManager::getStatus2Kart(status);
            int win = (RCMGetManager()->getKartChecker(kartNo), KartChecker::getWinBombPoint());
            int point = RCMGetManager()->getKartChecker(kartNo)->getBombPoint();
            u8 alpha = K2DPicture::mK2DGlobalAlpha;
            if (isBombWinner(status))
                K2DPicture::mK2DGlobalAlpha = 255;
            for (int j = 0; j < point; j++) {
                f32 scale = getBombScale(status, 2, j);
                K2DPicture *pic = (K2DPicture *)mMiniPics[status][2][j];
                f32 w = scale * pic->mPicture.getWidth();
                f32 h = scale * pic->mPicture.getHeight();
                pic->drawK2D(_e40[status][j].x - w / 2, _e40[status][j].y - h / 2, w, h, true);
            }
            if (isBombWinner(status))
                K2DPicture::mK2DGlobalAlpha = alpha;
        }
    }
}

#pragma dont_inline on
int KartChecker::getWinBombPoint() { return sBombPointFull; }
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
            scale = 1.0f + 1.5f * (frame - 0x20) / 5.0f;
        else if (frame - 0x15 < 0x14)
            scale = 2.5f + -2.0f * (frame - 0x25) / 4.0f;
        else if (frame - 0x15 < 0x18)
            scale = 0.5f + 0.5f * (frame - 0x29) / 4.0f;
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

void Race2D::drawEscape()
{
    int frame;
    int status;
    if (RCMGetManager()->getConsoleNumber() == 3 || RCMGetManager()->getConsoleNumber() == 4)
        frame = mEscapeTimer4;
    else
        frame = mEscapeTimer;
    if (frame < 14)
    {
        f32 scale = getStartShineTimeScale(frame);
        u8 alpha = K2DPicture::mK2DGlobalAlpha;
        K2DPicture::mK2DGlobalAlpha = 0xff;
        for (status = 0; status < RCMGetManager()->getStatusNumber(); status++)
        {
            for (int item = 0; item < 5; item++)
            {
                f32 s = mSpecialItemScale[mShineKind[status]] * scale;
                f32 px = mShinePos[item].x * s;
                f32 w = s * mShinePics[status][item]->getWidth();
                f32 py = mShinePos[item].y * s;
                f32 h = s * mShinePics[status][item]->getHeight();
                ((K2DPicture *)mShinePics[status][item])->drawK2D(mShineBase[status].x + (px - w / 2.0f),
                                                                  mShineBase[status].y + (py - h / 2.0f), w, h, true);
            }
        }
        K2DPicture::mK2DGlobalAlpha = alpha;
    }
    else
    {
        for (status = 0; status < RCMGetManager()->getStatusNumber(); status++)
        {
            for (int item = 0; item < 5; item++)
            {
                f32 s = mSpecialItemScale[mShineKind[status]];
                f32 px = mShinePos[item].x * s;
                f32 w = s * mShinePics[status][item]->getWidth();
                f32 py = mShinePos[item].y * s;
                f32 h = s * mShinePics[status][item]->getHeight();
                ((K2DPicture *)mShinePics[status][item])->drawK2D(mShineBase[status].x + (px - w / 2.0f),
                                                                  mShineBase[status].y + (py - h / 2.0f), w, h, true);
            }
        }
    }
}

f32 Race2D::getStartShineTimeScale(int frame)
{
    f32 scale = 0.0f;
    if (frame < 11)
        scale = 1.6f * frame / 11.0f;
    else if (frame < 14)
        scale = 1.6f + -0.6f * (frame - 11) / 3.0f;
    else
        scale = 1.0f;
    return scale;
}

void Race2D::drawCourse()
{
    Race2DParam *param = mHio->getParam();
    if (!(param->mFlag & 1))
        return;
    if (!mDrawFlag)
        return;

    mGrafContext->setPort();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 7);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_TEX0, GX_TEX_ST, GX_U8, 7);

    for (int i = 0; i < 2; i++)
    {
        GXSetTevColorOp((GXTevStageID)i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaOp((GXTevStageID)i, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTexCoordGen2((GXTexCoordID)i, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    }
    GXSetCurrentMtx(GX_PNMTX0);

    int goalFrame = 0;
    if (RCMGetManager()->isRaceEnd())
        goalFrame = GETJ2DManager()->mAnmFrame;
    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++)
        GETJ2DManager()->getGoalAnmFrame(i);

    f32 base = 0.0f;
    f32 scaleA;
    getStartScaleA(mEscapeTimer, scaleA);
    f32 scaleB;
    getStartScaleB(mEscapeTimer, scaleB);

    if ((RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2) &&
        RCMGetManager()->getRaceMode() == GRAND_PRIX)
    {
        int order[8];
        int value[8];
        for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
            order[i] = i;
        for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
            value[i] = mCharWindow[order[i]]._18;
        for (int i = 0; i < RCMGetManager()->getKartNumber() - 1; i++)
        {
            for (int j = RCMGetManager()->getKartNumber() - 1; j > i; j--)
            {
                if (value[order[i]] < value[order[j]])
                {
                    int tmp = order[i];
                    order[i] = order[j];
                    order[j] = tmp;
                }
            }
        }

        for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
        {
            f32 startX = 0.0f;
            int kartNo = order[i];
            getStartCharPos(mEscapeTimer, RCMGetManager()->getKartNumber() - 1 - i, startX);
            f32 goalX = 0.0f;
            if (goalFrame > 0)
                getGoalCharPos(goalFrame, RCMGetManager()->getKartNumber() - 1 - i, goalX);

            f32 x, y, z;
            getCharacterInfo(mKartSlot[kartNo]._4, mKartSlot[kartNo]._0 % 39, x, y, z);
            int drvOrder[2];
            if (z > 0.9f)
            {
                drvOrder[0] = 1;
                drvOrder[1] = 0;
            }
            else
            {
                drvOrder[0] = 0;
                drvOrder[1] = 1;
            }

            f32 scale;
            if (J2DManager::getKart2Status(kartNo) >= 0 && J2DManager::getKart2Status(kartNo) < RCMGetManager()->getStatusNumber())
                scale = 1.2f;
            else
                scale = 1.0f;
            if (sThunderAnm[kartNo] != 0)
                scale *= getThunderCharScale(sThunderAnm[kartNo]);

            for (int j = 0; j < 2; j++)
            {
                int drv = drvOrder[j];
                f32 cx, cy, cs;
                JUTColor color1(0xffffffff);
                JUTColor color2(0xffffffff);
                getCharacterInfo((&mKartSlot[kartNo]._4)[drv], mKartSlot[kartNo]._0 % 39, cx, cy, cs);
                u8 alpha;
                getCharacterClr(kartNo, (&mKartSlot[kartNo]._4)[drv], mKartSlot[kartNo]._0 % 39, color1, color2, alpha);
                alpha = alpha * mCharWindow[kartNo]._34[2] / 255;

                f32 w = scale * (scaleB * (cs * mCharWindow[kartNo].mPics[drv][0]->getWidth()));
                f32 h = scale * (scaleB * (cs * mCharWindow[kartNo].mPics[drv][0]->getHeight()));
                mCharWindow[kartNo].mPics[drv][0]->rotate(w / 2, h / 2, J2DROTATE_Z, sSpinRotate[kartNo]);
                mCharWindow[kartNo].mPics[drv][0]->setWhite(color1);
                mCharWindow[kartNo].mPics[drv][0]->setAlpha(mCharWindow[kartNo]._34[2]);
                ((K2DPicture *)mCharWindow[kartNo].mPics[drv][0])->drawK2D(base + (goalX + (startX + ((cx + mCharWindow[kartNo]._2c.x) - w / 2))),
                             base + ((cy + mCharWindow[kartNo]._2c.y) - h / 2), w, h, true);

                f32 w2 = scale * (scaleB * (cs * mCharWindow[kartNo].mPics[drv][1]->getWidth()));
                f32 h2 = scale * (scaleB * (cs * mCharWindow[kartNo].mPics[drv][1]->getHeight()));
                mCharWindow[kartNo].mPics[drv][1]->rotate(w2 / 2, h2 / 2, J2DROTATE_Z, sSpinRotate[kartNo]);
                mCharWindow[kartNo].mPics[drv][1]->setWhite(color2);
                mCharWindow[kartNo].mPics[drv][1]->setAlpha(alpha);

                f32 ox = mCharWindowOfs.x * cs;
                f32 oy = mCharWindowOfs.y * cs;
                f32 rad = -(3.1415927f * sSpinRotate[kartNo] / 180.0f);
                f32 rx = ox * std::cosf(rad) - oy * std::sinf(rad);
                f32 ry = ox * std::sinf(rad) + oy * std::cosf(rad);
                ((K2DPicture *)mCharWindow[kartNo].mPics[drv][1])->drawK2D(base + (goalX + (startX + ((mCharWindow[kartNo]._2c.x + (rx + cx)) - w2 / 2))),
                             base + ((mCharWindow[kartNo]._2c.y + (ry + cy)) - h2 / 2), w2, h2, true);
            }
        }

        for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
        {
            f32 startX = 0.0f;
            int kartNo = order[i];
            getStartCharPos(mEscapeTimer, RCMGetManager()->getKartNumber() - 1 - i, startX);
            f32 goalX = 0.0f;
            if (goalFrame > 0)
                getGoalCharPos(goalFrame, RCMGetManager()->getKartNumber() - 1 - i, goalX);

            f32 x, y, z;
            getCharacterInfo(mKartSlot[kartNo]._4, mKartSlot[kartNo]._0 % 39, x, y, z);

            f32 scale;
            if (J2DManager::getKart2Status(kartNo) >= 0 && J2DManager::getKart2Status(kartNo) < RCMGetManager()->getStatusNumber())
                scale = 1.2f;
            else
                scale = 1.0f;

            f32 w = scale * (scaleB * mCharWindow[kartNo].mPane->getWidth());
            f32 h = scale * (scaleB * mCharWindow[kartNo].mPane->getHeight());
            mCharWindow[kartNo].mPane->setAlpha(mCharWindow[kartNo]._34[2]);
            ((K2DPicture *)mCharWindow[kartNo].mPane)->drawK2D(base + (goalX + (startX + ((mCharWindowCenter.x + mCharWindow[kartNo]._2c.x) - w / 2))),
                         base + ((mCharWindowCenter.y + mCharWindow[kartNo]._2c.y) - h / 2), w, h, true);
        }
    }

    if ((RCMGetManager()->getConsoleNumber() == 1 && mGoalCount != 1) ||
        (RCMGetManager()->getConsoleNumber() == 2 && mGoalCount != 2) ||
        RCMGetManager()->getConsoleNumber() == 3 || RCMGetManager()->getConsoleNumber() == 4)
    {
        f32 ofsX = 0.0f;
        getStartLapTimePos(mEscapeTimer, 0, ofsX);
        f32 w = mCoursePic->getWidth();
        f32 h = mCoursePic->getHeight();
        f32 ratio = *(f32 *)((u8 *)RCMGetCourse() + 0x1b4);
        f32 px = param->mX;
        f32 py = param->mY;
        f32 s = param->mS;
        if (RCMGetManager()->isMirror())
        {
            ((K2DPicture *)mCoursePic)->drawK2D(ofsX + (w * s + (mCoursePos.x + px + (1.0f - s) * w / 2)),
                            mCoursePos.y + py + h * (1.0f - s * ratio) / 2, -w * s, ratio * (h * s), true);
        }
        else
        {
            ((K2DPicture *)mCoursePic)->drawK2D(ofsX + (mCoursePos.x + px + (1.0f - s) * w / 2),
                            mCoursePos.y + py + h * (1.0f - s * ratio) / 2, w * s, ratio * (h * s), true);
        }

        f32 lw = mStartLinePic->getWidth();
        f32 lh = mStartLinePic->getHeight();
        mStartLinePic->rotate(mStartLineRot);
        ((K2DPicture *)mStartLinePic)->drawK2D(ofsX + (mStartLinePos.x - lw / 2), mStartLinePos.y - lh / 2, lw, lh, true);

        if (RCMGetManager()->getRaceMode() == ESCAPE_BATTLE)
        {
            f32 sw = mMapShinePic->getWidth();
            f32 sh = mMapShinePic->getHeight();
            ((K2DPicture *)mMapShinePic)->drawK2D(ofsX + (mEscape.x - sw / 2), mEscape.y - sh / 2, sw, sh, true);
        }

        int order[8];
        int rank[8];
        for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
            order[i] = i;
        for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
        {
            rank[i] = RCMGetKartChecker(order[i])->getRank();
            int status = J2DManager::getKart2Status(i);
            if (status >= 0 && status < RCMGetManager()->getStatusNumber())
                rank[i] -= 100;
        }
        for (int i = 0; i < RCMGetManager()->getKartNumber() - 1; i++)
        {
            for (int j = RCMGetManager()->getKartNumber() - 1; j > i; j--)
            {
                if (rank[order[i]] < rank[order[j]])
                {
                    int tmp = order[i];
                    order[i] = order[j];
                    order[j] = tmp;
                }
            }
        }

        for (int i = 0; i < RCMGetManager()->getKartNumber(); i++)
        {
            int kartNo = order[i];
            if (!gRaceInfo.getKartInfo(kartNo)->isAvailableKart())
                continue;
            if (RCMGetManager()->getKartDrawer(kartNo)->getHiddenDrivers() != 0)
                continue;
            f32 iw = mKartIcon[kartNo]->getWidth();
            f32 ih = mKartIcon[kartNo]->getHeight();
            if (kartNo < RCMGetManager()->getPlayerNumber())
            {
                switch (gRaceInfo.getKartInfo(kartNo)->getGhostKind())
                {
                case 0:
                    mKartIcon[kartNo]->setCornerColor(JUTColor(0xffffffff));
                    ((K2DPicture *)mPlayerIcon[kartNo])->drawK2D(ofsX + (mEscapeTarget[kartNo].x - iw / 2 - 19.0f),
                                                                 mEscapeTarget[kartNo].y - ih / 2 - 19.0f, iw, ih, true);
                    break;
                case 1:
                    ((K2DPicture *)mPlayerGIcon)->drawK2D(ofsX + (mEscapeTarget[kartNo].x - iw / 2 - 19.0f),
                                                          mEscapeTarget[kartNo].y - ih / 2 - 19.0f, iw, ih, true);
                    break;
                case 2:
                    ((K2DPicture *)mPlayerSGIcon)->drawK2D(ofsX + (mEscapeTarget[kartNo].x - iw / 2 - 19.0f),
                                                           mEscapeTarget[kartNo].y - ih / 2 - 19.0f, iw, ih, true);
                    break;
                }
            }
            else
            {
                mKartIcon[kartNo]->setCornerColor(JUTColor(0xc8c8c88c));
            }

            if (RCMGetManager()->getRaceMode() == ESCAPE_BATTLE && RCMGetKartChecker(kartNo)->isRabbit())
            {
                f32 ls = 0.9f + (1.1f * mEscapeFrame) / 39.0f;
                f32 lw = iw * ls;
                f32 lh = ih * ls;
                mBattleLightPic->setAlpha(255 - mEscapeFrame * 255 / 39);
                mBattleLightPic->setWhite(mPlayerNumberColor[kartNo]);
                ((K2DPicture *)mBattleLightPic)->drawK2D(ofsX + (mEscapeTarget[kartNo].x - lw / 2), mEscapeTarget[kartNo].y - lh / 2, lw, lh, true);
            }

            mKartIcon[kartNo]->rotate(sSpinRotate[kartNo]);
            ((K2DPicture *)mKartIcon[kartNo])->drawK2D(ofsX + (mEscapeTarget[kartNo].x - iw / 2), mEscapeTarget[kartNo].y - ih / 2, iw, ih, true);
        }
    }

    if (RCMGetManager()->getRaceMode() == TIME_ATTACK)
    {
        mLapGhost->draw();
        mCanNotSave->draw();
    }
}

static inline bool isGhostSavable()
{
    PadRecorder *recorder = RaceApp::ptr()->getPadRecorder();
    if (recorder && (int)recorder->getFrame() <= MAX_INPUT_FRAME_COUNT)
        return true;
    return false;
}

void Race2D::calc()
{
    if (!(mHio->getParam()->mFlag & 1))
        return;

    mTask->run();
    if (mScreen)
        setLayoutData();
    calcMiniGame();
    calcPlayerMark();

    if (mDrawFlag)
        mEscapeTimer++;
    if (mEscapeTimer < 20) {
        if (RCMGetManager()->isRaceModeMiniGame())
            K2DPicture::mK2DGlobalAlpha = mEscapeTimer * 255 / 20;
        else
            K2DPicture::mK2DGlobalAlpha = 255;
    } else {
        K2DPicture::mK2DGlobalAlpha = 255;
        if (mEscapeTimer == 0x7fffffff)
            mEscapeTimer = 0x7ffffffe;
    }
    if (GETJ2DManager()->isStartGo321()) {
        if (++mEscapeTimer4 == 0x7fffffff)
            mEscapeTimer4 = 0x7ffffffe;
    }
    if (mHideFrame != 0) {
        if (mHideFrame++ < 20) {
            K2DPicture::mK2DGlobalAlpha = 255 - mHideFrame * 255 / 20;
        } else {
            K2DPicture::mK2DGlobalAlpha = 0;
            mHideFrame = 20;
        }
    }

    if ((RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2) &&
        !RCMGetManager()->isRaceModeMiniGame() && RCMGetManager()->getRaceMode() != VERSUS_RACE && !J2DManager::getNetFlag()) {
        for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
            KartChecker *checker = RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(status));
            if (!checker->isGoal() && checker->isLapRenewal()) {
                _694x[status][0]->changeTexture(mTimeLapName[1], 0);
                for (int i = 0; i < 10; i++)
                    _694x[status][i]->setCornerColor(mLapTimeCornerColor);
                if (RCMGetManager()->getRaceMode() == TIME_ATTACK) {
                    if (checker->getLapTime(checker->getLap() - 1).isLittle(sBestLapTime)) {
                        mAnmTA[status].mCount[1] = 1;
                        sBestLapTime = checker->getLapTime(checker->getLap() - 1);
                    } else {
                        mAnmTA[status].mCount[0] = 1;
                    }
                } else {
                    mAnmTA[status].mCount[0] = 1;
                }
            }
            if (checker->isGoal() && checker->isLapRenewal() && RCMGetManager()->getRaceMode() == TIME_ATTACK) {
                if (checker->isBestTotalTimeRenewal(4))
                    mAnmTA[status].mCount[2] = 1;
                if (checker->getLapTime(checker->getLap() - 1).isLittle(sBestLapTime))
                    sBestLapTime = checker->getLapTime(checker->getLap() - 1);
                if (checker->isBestLapTimeRenewal()) {
                    mAnmTA[status].mCount[3] = 1;
                    for (int i = 0; i < RCMGetManager()->getTotalLapNumber(); i++) {
                        RaceTime lapTime = checker->getLapTime(i);
                        if (checker->getBestLapTime()->get() == lapTime.get())
                            mAnmTA[status].mNum = i;
                    }
                }
            }
            anmTA(status);
        }
    }

    if (RCMGetManager()->getRaceMode() == TIME_ATTACK) {
        KartChecker *checker = RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(0));
        if (!checker->isGoal() && checker->isLapRenewal() && !(gSystemRecord.mOptions & 8)) {
            int lap = checker->getLap() - 1;
            mLapGhost->start(checker->getLapSplit(lap), lap);
        }
        mLapGhost->calc();
        if (!isGhostSavable())
            mCanNotSave->setVisible(true);
        mCanNotSave->calc();
    }
}

void Race2D::calcPlayerMark()
{
    if (!(mHio->getParam()->mFlag & 1))
        return;
    if (!mDrawFlag)
        return;
    if (RCMGetManager()->getConsoleNumber() < 2 && !J2DManager::mNetFlag)
        return;

    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++)
    {
        for (int i = 0; i < RCMGetManager()->getPlayerNumber(); i++)
        {
            mMarkOrder[status][i] = i;
            mMarkPos[status][i].set(-100.0f, -100.0f);
        }

        u32 z[8];
        for (int i = 0; i < RCMGetManager()->getPlayerNumber(); i++)
        {
            KartLoader *loader = RCMGetManager()->getKartLoader(i);
            JGeometry::TVec3f pos(*(f32 *)((u8 *)loader + 0x4c),
                                  *(f32 *)((u8 *)loader + 0x5c),
                                  *(f32 *)((u8 *)loader + 0x6c));
            z[i] = SiUtil::getScreenPos((long)status, pos, &mMarkPos[status][mMarkOrder[status][i]], RCMGetManager()->isMirror());
        }

        for (int i = 0; i < RCMGetManager()->getPlayerNumber() - 1; i++)
        {
            for (int j = RCMGetManager()->getPlayerNumber() - 1; j > i; j--)
            {
                if (z[mMarkOrder[status][i]] < z[mMarkOrder[status][j]])
                {
                    int tmp = mMarkOrder[status][i];
                    mMarkOrder[status][i] = mMarkOrder[status][j];
                    mMarkOrder[status][j] = tmp;
                }
            }
        }

        for (int i = 0; i < RCMGetManager()->getPlayerNumber(); i++)
        {
            if (J2DManager::getKart2Status(i) == status)
                continue;

            int x = mMarkPos[status][i].x;
            int y = mMarkPos[status][i].y;
            u8 alpha = mMark[status][i]->getAlpha();
            u32 curAlpha = alpha;

            if (mAlarm[status].mBase <= (s16)x && (s16)x <= mAlarm[status].mBase + mAlarm[status].mStep &&
                mAlarm[status]._4 <= (s16)y && (s16)y <= mAlarm[status]._4 + mAlarm[status]._c)
            {
                u32 newZ = RCMGetManager()->mRaceDrawer->getCaptureMgr()->getZValue(i, status);
                RCMGetManager()->mRaceDrawer->getCaptureMgr()->setPosition(i, status, x, y);
                if (z[i] > newZ)
                {
                    if (curAlpha > 4)
                        alpha -= 4;
                    else
                        alpha = 0;
                }
                else
                {
                    if (curAlpha < 247)
                        alpha += 8;
                    else
                        alpha = 255;
                }
            }
            else
            {
                if (curAlpha > 4)
                    alpha -= 4;
                else
                    alpha = 0;
            }
            mMark[status][i]->setAlpha(alpha);
        }
    }
}

void Race2D::calcParticle()
{
    if (!(mHio->getParam()->mFlag & 1))
        return;
    if (!mDrawFlag)
        return;
    if (RCMGetManager()->getConsoleNumber() < 2 && !J2DManager::getNetFlag())
        return;

    f32 rate = 1.0f;
    f32 reachScale = 1.0f;
    f32 scale = 1.0f;
    switch (RCMGetManager()->getConsoleNumber()) {
    case 1:
        rate = 1.0f;
        reachScale = 1.0f;
        scale = 1.0f;
        break;
    case 2:
        rate = 0.85f;
        reachScale = 1.33f;
        scale = 0.665f;
        break;
    case 3:
    case 4:
        rate = 0.75f;
        scale = 0.65f;
        break;
    default:
#line 3382
        JUT_ASSERT(0);
        break;
    }
    f32 ofsY = 47.0f * rate;
    f32 ofsX = 10.0f * rate;

    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
        JGeometry::TVec2f screenPos[8];
        for (int i = 0; i < RCMGetManager()->getPlayerNumber(); i++) {
            screenPos[i].x = -100.0f;
            screenPos[i].y = -100.0f;
        }
        for (int i = 0; i < RCMGetManager()->getPlayerNumber(); i++) {
            KartLoader *loader = RCMGetManager()->getKartLoader(i);
            JGeometry::TVec3f pos(*(f32 *)((u8 *)loader + 0x4c), *(f32 *)((u8 *)loader + 0x5c), *(f32 *)((u8 *)loader + 0x6c));
            SiUtil::getScreenPos((long)status, pos, &screenPos[i], RCMGetManager()->isMirror());
        }
        for (int i = 0; i < RCMGetManager()->getPlayerNumber(); i++) {
            if (RCMGetManager()->getRaceMode() != BOMB_BATTLE || !isBombReach(i))
                continue;
            if (status == J2DManager::getKart2Status(i)) {
                for (int j = 0; j < 2; j++) {
                    if (_f80[status][i][j]) {
                        _f80[status][i][j]->setGlobalAlpha(255);
                        int kartNo = J2DManager::getStatus2Kart(status);
                        int win = (RCMGetManager()->getKartChecker(kartNo), KartChecker::getWinBombPoint());
                        _f80[status][i][j]->setGlobalTranslation(mMiniPos[status][0][win - 1].x - mAlarm[status].mBase,
                                                                 mMiniPos[status][0][win - 1].y - mAlarm[status]._4, 0.0f);
                        _f80[status][i][j]->setGlobalScale(reachScale);
                    }
                }
            } else {
                u8 alpha = mMark[status][i]->mAlpha;
                if (RCMGetManager()->getKartDrawer(i)->getHiddenDrivers())
                    alpha = 0;
                for (int j = 0; j < 2; j++) {
                    if (_f80[status][i][j]) {
                        _f80[status][i][j]->setGlobalAlpha(alpha);
                        _f80[status][i][j]->setGlobalTranslation(ofsX + (screenPos[i].x - mAlarm[status].mBase),
                                                                 (screenPos[i].y - mAlarm[status]._4) - ofsY, 0.0f);
                        _f80[status][i][j]->setGlobalScale(scale);
                    }
                }
            }
        }
    }

    if (mHideFrame != 0) {
        u8 alpha = K2DPicture::mK2DGlobalAlpha;
        if (alpha != 0) {
            if (RCMGetManager()->getRaceMode() == BOMB_BATTLE) {
                for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
                    for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
                        for (int j = 0; j < 2; j++) {
                            if (_f80[status][i][j])
                                _f80[status][i][j]->setGlobalAlpha(alpha);
                        }
                    }
                }
            }
        } else if (RCMGetManager()->getRaceMode() == BOMB_BATTLE) {
            for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
                for (int i = 0; i < RCMGetManager()->getStatusNumber(); i++) {
                    for (int j = 0; j < 2; j++) {
                        if (_f80[status][i][j]) {
                            JPEffectMgr::mThis->deleteEmtBefore2DMulti(_f80[status][i][j], status);
                            _f80[status][i][j] = nullptr;
                        }
                    }
                }
            }
        }
    }
}

void Race2D::setLayoutData()
{
    if (RCMGetManager()->getConsoleNumber() == 1) {
        KartChecker *checker = RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(0));
        u64 carStatus = GetKartCtrl()->GetKartStatus(J2DManager::getStatus2Kart(0));
        f32 speed = GetKartCtrl()->GetCarSpeed(checker->getTargetKartNo());
        int kmh = (int)speed / 2;

        for (int i = 0; i < 7; i++) {
            if (kmh > i * 150 / 6 / 2 || mEscapeTimer < 70)
                mSpeedChipPics[i]->setWhite(mSpeedChipColor[i]);
            else {
                JUTColor gray(0x80, 0x80, 0x80, 0xff);
                mSpeedChipPics[i]->setWhite(gray);
            }
        }

        if ((carStatus & 0x1000) || (carStatus & 0x4000) || (carStatus & 0x20000) || (carStatus & 0x100000000ull)) {
            for (int i = 0; i < 7; i++)
                mSpeedChipPics[i]->setWhite(mSpeedChipColor[(i - sColorIndex + 7) % 7]);
            if (sChangeFrame++ % 2 == 0) {
                if (sColorIndex++ >= 7)
                    sColorIndex = 0;
            }
        }

        if (KartLocale::msSpeedUnit == 1)
            kmh = (int)((7.0f * speed / 10.0f) / 2.0f);

        int d100 = kmh / 100;
        int d1 = kmh % 10;
        int d10 = (kmh - d100 * 100) / 10;
        if (d100 != 0) {
            mSpeedPics[0]->show();
            mSpeedPics[0]->changeTexture(mSpeedTex[d100], 0);
            mSpeedPics[1]->show();
            mSpeedPics[1]->changeTexture(mSpeedTex[d10], 0);
            mSpeedPics[2]->show();
            mSpeedPics[2]->changeTexture(mSpeedTex[d1], 0);
        } else if (d10 != 0) {
            mSpeedPics[0]->hide();
            mSpeedPics[1]->show();
            mSpeedPics[1]->changeTexture(mSpeedTex[d10], 0);
            mSpeedPics[2]->show();
            mSpeedPics[2]->changeTexture(mSpeedTex[d1], 0);
        } else {
            mSpeedPics[0]->hide();
            mSpeedPics[1]->hide();
            mSpeedPics[2]->show();
            mSpeedPics[2]->changeTexture(mSpeedTex[d1], 0);
        }
    }

    for (int i = 0; i < RCMGetManager()->getKartNumber(); i++) {
        int charID[2];
        for (int d = 0; d < 2; d++)
            charID[d] = gRaceInfo.getKartInfo(i)->getDriverCharID(d);

        if (RCMGetManager()->isRaceModeMiniGame()) {
            int color = i;
            if (!J2DManager::getNetFlag()) {
                if (RCMGetManager()->getKartInfo(J2DManager::getStatus2Kart(0))->isTagMode() && i != 0) {
                    color++;
                    if (color >= 4)
                        color = 3;
                }
            }
            mKartIcon[i]->setWhite(mPlayerNumberColor[color]);
        } else if (charID[0] > 0 && charID[0] < 21 && charID[1] > 0 && charID[1] < 21) {
            if (GetKartCtrl()->WhichDriver(i))
                mKartIcon[i]->changeTexture(mDriverTex[charID[0] - 1], 0);
            else
                mKartIcon[i]->changeTexture(mDriverTex[charID[1] - 1], 0);
        }

        u64 carStatus = GetKartCtrl()->GetKartStatus(i);
        if ((carStatus & 0x80000) || (carStatus & 0x100000)) {
            sSpinRotate[i] -= 18.0f;
            if (sSpinRotate[i] < 0.0f)
                sSpinRotate[i] += 360.0f;
        } else if (RCMGetManager()->isRaceModeMiniGame()) {
            KartLoader *loader = RCMGetManager()->getKartLoader(i);
            sSpinRotate[i] = 90.0f + (180.0f * std::atan2(*(f32 *)((u8 *)loader + 0x40), *(f32 *)((u8 *)loader + 0x60))) / 3.1415927f;
        } else {
            sSpinRotate[i] = 0.0f;
        }

        KartLoader *loader = RCMGetManager()->getKartLoader(i);
        JGeometry::TVec3f pos(*(f32 *)((u8 *)loader + 0x4c), *(f32 *)((u8 *)loader + 0x5c), *(f32 *)((u8 *)loader + 0x6c));
        getMapPos(mMapInfo.mRot, pos, mEscapeTarget[i]);
    }

    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
        mLapPic[status]->changeTexture(mLapNumTex[mPlayerHUD[status].mLap], 0);
        for (int d = 0; d < 2; d++) {
            if (mPlayerHUD[status].mItem[d][0] != 16) {
                mItemExPics[status][d]->changeTexture(mItemTex[mPlayerHUD[status].mItem[d][0]], 0);
                mItemExPics[status][d + 2]->changeTexture(mItemTex[mPlayerHUD[status].mItem[d][1]], 0);
            }
        }

        if (sPreWhichDriver[status] != GetKartCtrl()->WhichDriver(J2DManager::getStatus2Kart(status))) {
            if (GetKartCtrl()->WhichDriver(J2DManager::getStatus2Kart(status))) {
                mPlayerHUD[status]._84c[0] = 1;
                mPlayerHUD[status]._84c[1] = 0;
            } else {
                mPlayerHUD[status]._84c[0] = 0;
                mPlayerHUD[status]._84c[1] = 1;
            }
            mPlayerHUD[status]._848 = -1;
            sPreWhichDriver[status] = GetKartCtrl()->WhichDriver(J2DManager::getStatus2Kart(status)) != 0;
        }
        if (++mPlayerHUD[status]._848 > 38)
            mPlayerHUD[status]._848 = 38;

        if (RCMGetManager()->getRacePhase() != PHASE_CRS_DEMO && !RCMGetManager()->isReplayMode() &&
            !RCMGetManager()->isWaitDemoMode() && !GETJ2DManager()->isLANDemo()) {
            f32 x, y, z;
            getItemInfo(status, mPlayerHUD[status]._84c[0], mPlayerHUD[status]._848 % 39, x, y, z);
            int drv[2];
            if (z > 0.8f) {
                drv[0] = 1;
                drv[1] = 0;
            } else {
                drv[0] = 0;
                drv[1] = 1;
            }
            for (int d = 0; d < 2; d++) {
                int dd = drv[d];
                if (GetItemObjMgr()->IsRollingSlot(J2DManager::getStatus2Kart(status), d)) {
                    if (dd == 0)
                        GetKartCtrl()->GetKartSoundMgr(J2DManager::getStatus2Kart(status))->setSe(0x10010);
                    else if (dd == 1)
                        GetKartCtrl()->GetKartSoundMgr(J2DManager::getStatus2Kart(status))->setSe(0x1000f);
                }
                if (mPlayerHUD[status]._7f8[d]) {
                    switch (mPlayerHUD[status].mItem[d][0]) {
                    case 1:
                    case 4:
                    case 7:
                    case 8:
                    case 11:
                    case 12:
                    case 14:
                    case 17:
                    case 19:
                    case 21:
                    case 26:
                    case 27:
                        if (!mPlayerHUD[status]._7fa[d])
                            GetKartCtrl()->GetKartSoundMgr(J2DManager::getStatus2Kart(status))->setSe(0x100cd);
                        break;
                    case 23:
                    case 24:
                    case 25:
                    case 28:
                    case 29:
                    case 30:
                        break;
                    default: {
                        bool prev = mPlayerHUD[status]._7fa[d];
                        if (dd == 0) {
                            if (!prev)
                                GetKartCtrl()->GetKartSoundMgr(J2DManager::getStatus2Kart(status))->setSe(0x10034);
                        } else if (dd == 1) {
                            if (!prev)
                                GetKartCtrl()->GetKartSoundMgr(J2DManager::getStatus2Kart(status))->setSe(0x10033);
                        }
                        break;
                    }
                    }
                }
                mPlayerHUD[status]._7fa[d] = mPlayerHUD[status]._7f8[d];
                mPlayerHUD[status]._7f8[d] = false;
            }
        }

        mPlayerHUD[status]._24 = RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(status))->getRank();
        if (sRankAnm[status] != 0) {
            if (++sRankAnm[status] > 9)
                sRankAnm[status] = 0;
        }
        if (mPlayerHUD[status]._24 != sPreRank[status]) {
            sPreRank[status] = mPlayerHUD[status]._24;
            sRankAnm[status] = 1;
        }
        if (sRankAnm[status] == 6 && mPlayerHUD[status]._24 > 0 && mPlayerHUD[status]._24 <= 8) {
            for (int r = 0; r < 8; r++)
                mRankDigits1[status][r]->hide();
            mRankDigits1[status][mPlayerHUD[status]._24 - 1]->show();
            if (RCMGetManager()->getConsoleNumber() == 2) {
                for (int r = 0; r < 8; r++)
                    mRankDigits2[status][r]->hide();
                mRankDigits2[status][mPlayerHUD[status]._24 - 1]->show();
            }
        }

        if (RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2) {
            _694x[status][1]->changeTexture(mNumberTex[mPlayerHUD[status].mDigits[1]], 0);
            _694x[status][2]->changeTexture(mNumberTex[mPlayerHUD[status].mDigits[0]], 0);
            _694x[status][4]->changeTexture(mNumberTex[mPlayerHUD[status].mDigits[3]], 0);
            _694x[status][5]->changeTexture(mNumberTex[mPlayerHUD[status].mDigits[2]], 0);
            _694x[status][7]->changeTexture(mNumberTex[mPlayerHUD[status].mDigits[6]], 0);

            u32 rnd;
            if (status == 0)
                rnd = stGetRnd(4)->getRandom();

            if (RCMGetManager()->isAbleStart() && !RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(status))->isGoal() &&
                RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(status))->getTotalTime().isAvailable()) {
                int v = mPlayerHUD[status].mDigits[4] + mPlayerHUD[status].mDigits[5] * 10;
                switch (v) {
                case 0x00:
                    v = rnd % 16;
                    break;
                case 0x10:
                    v = rnd % 17 + 0x10;
                    break;
                case 0x21:
                    v = rnd % 17 + 0x21;
                    break;
                case 0x32:
                    v = rnd % 16 + 0x32;
                    break;
                case 0x42:
                    v = rnd % 17 + 0x42;
                    break;
                case 0x53:
                    v = rnd % 17 + 0x53;
                    break;
                }
                mPlayerHUD[status].mDigits[5] = v / 10;
                mPlayerHUD[status].mDigits[4] = v % 10;
            }
            _694x[status][8]->changeTexture(mNumberTex[mPlayerHUD[status].mDigits[5]], 0);
            _694x[status][9]->changeTexture(mNumberTex[mPlayerHUD[status].mDigits[4]], 0);

            if (mAnmTA[status].mCount[0] != 0 || mAnmTA[status].mCount[1] != 0) {
                _694x[status][1]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[mPlayerHUD[status].mLap - 1].mDigits[1]], 0);
                _694x[status][2]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[mPlayerHUD[status].mLap - 1].mDigits[0]], 0);
                _694x[status][4]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[mPlayerHUD[status].mLap - 1].mDigits[3]], 0);
                _694x[status][5]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[mPlayerHUD[status].mLap - 1].mDigits[2]], 0);
                _694x[status][7]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[mPlayerHUD[status].mLap - 1].mDigits[6]], 0);
                _694x[status][8]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[mPlayerHUD[status].mLap - 1].mDigits[5]], 0);
                _694x[status][9]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[mPlayerHUD[status].mLap - 1].mDigits[4]], 0);
            }

            if (!mPlayerHUD[status].mRaceTimeAvailable) {
                const ResTIMG *tex = Kart2DCommon::ptr()->getAsciiTexture('-');
                _694x[status][1]->changeTexture(tex, 0);
                _694x[status][2]->changeTexture(tex, 0);
                _694x[status][4]->changeTexture(tex, 0);
                _694x[status][5]->changeTexture(tex, 0);
                _694x[status][7]->changeTexture(tex, 0);
                _694x[status][8]->changeTexture(tex, 0);
                _694x[status][9]->changeTexture(tex, 0);
            }

            for (int lap = 0; lap < 9; lap++) {
                if (mPlayerHUD[status].mLapSlots[lap].mAvailable) {
                    mLapDigits[status][lap][0]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[lap].mDigits[1]], 0);
                    mLapDigits[status][lap][1]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[lap].mDigits[0]], 0);
                    mLapDigits[status][lap][2]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[lap].mDigits[3]], 0);
                    mLapDigits[status][lap][3]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[lap].mDigits[2]], 0);
                    mLapDigits[status][lap][4]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[lap].mDigits[6]], 0);
                    mLapDigits[status][lap][5]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[lap].mDigits[5]], 0);
                    mLapDigits[status][lap][6]->changeTexture(mNumberTex[mPlayerHUD[status].mLapSlots[lap].mDigits[4]], 0);
                } else {
                    const ResTIMG *tex = Kart2DCommon::ptr()->getAsciiTexture('-');
                    mLapDigits[status][lap][0]->changeTexture(tex, 0);
                    mLapDigits[status][lap][1]->changeTexture(tex, 0);
                    mLapDigits[status][lap][2]->changeTexture(tex, 0);
                    mLapDigits[status][lap][3]->changeTexture(tex, 0);
                    mLapDigits[status][lap][4]->changeTexture(tex, 0);
                    mLapDigits[status][lap][5]->changeTexture(tex, 0);
                    mLapDigits[status][lap][6]->changeTexture(tex, 0);
                }
            }
        }
    }

    if ((RCMGetManager()->getConsoleNumber() == 1 || RCMGetManager()->getConsoleNumber() == 2) &&
        RCMGetManager()->getRaceMode() != TIME_ATTACK && !RCMGetManager()->isRaceModeMiniGame()) {
        for (int i = 0; i < RCMGetManager()->getKartNumber(); i++) {
            if (sPreWhichDriver2[i] != GetKartCtrl()->WhichDriver(i)) {
                if (GetKartCtrl()->WhichDriver(i)) {
                    mKartSlot[i]._4 = 1;
                    mKartSlot[i]._8 = 0;
                } else {
                    mKartSlot[i]._4 = 0;
                    mKartSlot[i]._8 = 1;
                }
                mKartSlot[i]._0 = -1;
                sPreWhichDriver2[i] = GetKartCtrl()->WhichDriver(i) != 0;
            }
            if (++mKartSlot[i]._0 > 38)
                mKartSlot[i]._0 = 38;

            if (_4e9c[1] < 11) {
                mCharWindow[i]._2c = mCharWindow[i]._1c + (mCharWindow[i].mPos - mCharWindow[i]._1c) * (f32)_4e9c[1] * 0.09090909f;
                mCharWindow[i]._34[2] = mCharWindow[i]._34[0] + _4e9c[1] * (mCharWindow[i]._34[1] - mCharWindow[i]._34[0]) / 11;
            } else {
                mCharWindow[i]._1c.x = mCharWindow[i].mPos.x;
                mCharWindow[i]._1c.y = mCharWindow[i].mPos.y;
                mCharWindow[i]._2c.x = mCharWindow[i]._1c.x;
                mCharWindow[i]._2c.y = mCharWindow[i]._1c.y;
                mCharWindow[i]._34[2] = mCharWindow[i]._34[0] = mCharWindow[i]._34[1];
            }

            if (sThunderAnm[i] != 0) {
                sThunderAnm[i]++;
                if (sThunderAnm[i] < 540 && GetKartCtrl()->GetKartScale(i) >= 1.0f)
                    sThunderAnm[i] = 540;
                if (sThunderAnm[i] > 550)
                    sThunderAnm[i] = 0;
            }

            bool thunder = GetKartCtrl()->CheckThunderBolt(i);
            if (thunder && thunder != sPreThunder[i]) {
                sPreThunder[i] = thunder;
                sThunderAnm[i] = 1;
            } else {
                sPreThunder[i] = thunder;
            }
        }

        if (++_4e9c[0] >= 40)
            _4e9c[0] = 0;

        if (_4e9c[1] < 11) {
            _4e9c[1]++;
        } else {
            _4e9c[1] = 0;
            for (int i = 0; i < RCMGetManager()->getKartNumber(); i++) {
                int rank = RCMGetManager()->getKartChecker(i)->getRank() - 1;
                if (rank >= 0 && rank < RCMGetManager()->getKartNumber()) {
                    mCharWindow[i].mPos.x = mCharIconPos[rank].x;
                    mCharWindow[i].mPos.y = mCharIconPos[rank].y;
                    ((J2DPicture *)mCharWindow[i].mPane)->changeTexture(mLapNumTex[rank], 0);
                    mCharWindow[i]._18 = rank;
                    if (rank < 4)
                        mCharWindow[i]._34[1] = 0xff;
                    else
                        mCharWindow[i]._34[1] = 0;
                    J2DPicture::TCornerColor cornerColor;
                    GETJ2DManager()->getRace2D()->getRankPic(0, rank)->getCornerColor(cornerColor);
                    ((J2DPicture *)mCharWindow[i].mPane)->setCornerColor(cornerColor);
                }
            }
        }
    }
}

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

void Race2D::calcBomb()
{
    JUTColor white[2][8] = {
        {0xff9696ff, 0x40ffffff, 0x80ff00ff, 0xffff00ff, 0xff9696ff, 0x40ffffff, 0x80ff00ff, 0xffff00ff},
        {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},
    };
    JUTColor black[2][8] = {
        {0xaa000000, 0x0000ff00, 0x00200000, 0x50180000, 0xaa000000, 0x0000ff00, 0x00200000, 0x50180000},
        {0xff303000, 0x00c8ff00, 0x00ff0000, 0xffc80000, 0xff303000, 0x00c8ff00, 0x00ff0000, 0xffc80000},
    };

    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
        int kartNo = J2DManager::getStatus2Kart(status);
        int win = (RCMGetManager()->getKartChecker(kartNo), KartChecker::getWinBombPoint());
        RCMGetManager()->getKartChecker(kartNo);
        for (int i = 0; i < win; i++) {
            int idx = RCMGetManager()->getKartChecker(kartNo)->getBombPointKart(i);
            if (RCMGetManager()->getKartInfo(J2DManager::getStatus2Kart(0))->isTagMode() && idx != 0) {
                idx++;
                if (idx >= 4)
                    idx = 3;
            }
            if (idx >= 0 && idx < 8) {
                mMiniPics[status][1][i]->setWhite(getBombColor(white[0][idx], white[1][idx]));
                mMiniPics[status][1][i]->setBlack(getBombColor(black[0][idx], black[1][idx]));
            }
        }
    }

    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
        for (int target = 0; target < RCMGetManager()->getStatusNumber(); target++) {
            if (isBombReach(target)) {
                int idx = target;
                if (RCMGetManager()->getKartInfo(J2DManager::getStatus2Kart(0))->isTagMode() && target != 0) {
                    idx++;
                    if (idx >= 4)
                        idx = 3;
                }
                for (int j = 0; j < 2; j++) {
                    if (_f80[status][target][j] == nullptr) {
                        char suffix[2] = {'a', 'b'};
                        char name[32];
                        snprintf(name, sizeof(name), "mk_2dStarShine%dP_%c", idx + 1, suffix[j]);
                        JGeometry::TVec3f pos(0.0f, 0.0f, 0.0f);
                        _f80[status][target][j] = JPEffectMgr::mThis->createEmtBefore2DMulti(name, pos, status);
                        if (_f80[status][target][j])
                            _f80[status][target][j]->setGlobalAlpha(0);
                    }
                }
            } else {
                for (int j = 0; j < 2; j++) {
                    if (_f80[status][target][j]) {
                        JPEffectMgr::mThis->deleteEmtBefore2DMulti(_f80[status][target][j], status);
                        _f80[status][target][j] = nullptr;
                    }
                }
            }
        }
    }

    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
        if (!isBombReach(status))
            mBombWinFrame[status] = 0;
        for (int i = 0; i < 4; i++) {
            if (mBombMarkFrame[status][i] == 0)
                continue;
            if (mBombMarkFrame[status][i] == 21) {
                int kartNo = J2DManager::getStatus2Kart(status);
                int pointKart = RCMGetManager()->getKartChecker(kartNo)->getBombPointKart(i);
                if (pointKart >= 0 && pointKart < 8) {
                    int idx = J2DManager::getKart2Status(pointKart);
                    if (RCMGetManager()->getKartInfo(J2DManager::getStatus2Kart(0))->isTagMode() && idx != 0) {
                        idx++;
                        if (idx >= 4)
                            idx = 3;
                    }
                    JGeometry::TVec3f pos(_dc0[status][i].x, _dc0[status][i].y, 0.0f);
                    JGeometry::TVec2f dir = mMiniPos[status][0][i] - _dc0[status][i];
                    s16 angle = 36863.0f * (f32)atan2(-dir.x, dir.y) / 3.1415927f;
                    char suffix[3] = {'a', 'b', 'c'};
                    JGeometry::TVec3s rot((s16)0, (s16)0, angle);
                    for (int j = 0; j < 3; j++) {
                        char name[32];
                        snprintf(name, sizeof(name), "mk_2dBlur%dP_%c", idx + 1, suffix[j]);
                        _ec0[status][i][j] = JPEffectMgr::mThis->createEmt2D(name, pos);
                        if (_ec0[status][i][j])
                            _ec0[status][i][j]->setGlobalRotation(rot);
                    }
                } else {
                    mBombMarkFrame[status][i] = 0;
                }
            }
            if (++mBombMarkFrame[status][i] >= 44) {
                mBombMarkFrame[status][i] = 0;
                for (int j = 0; j < 3; j++)
                    _ec0[status][i][j] = nullptr;
            }
        }
    }

    mBombColorFrame = (mBombColorFrame + 1) % 50;
    JGeometry::TVec2f screenPos[4];
    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
        int kart = J2DManager::getStatus2Kart(status);
        KartLoader *loader = RCMGetManager()->getKartLoader(kart);
        JGeometry::TVec3f pos(*(f32 *)((u8 *)loader + 0x4c), *(f32 *)((u8 *)loader + 0x5c), *(f32 *)((u8 *)loader + 0x6c));
        SiUtil::getScreenPos(status, pos, &screenPos[status], RCMGetManager()->isMirror());
    }

    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++) {
        int kartNo = J2DManager::getStatus2Kart(status);
        int point = RCMGetManager()->getKartChecker(kartNo)->getBombPoint();
        if (point > sPreBombPoint[status]) {
            for (int i = sPreBombPoint[status]; i < point; i++) {
                int pointKart = RCMGetManager()->getKartChecker(kartNo)->getBombPointKart(i);
                if (pointKart == -1)
                    continue;
                int idx = J2DManager::getKart2Status(pointKart);
                if (idx < 0 || idx >= RCMGetManager()->getStatusNumber())
                    continue;
                _dc0[status][i].x = screenPos[idx].x;
                _dc0[status][i].y = screenPos[idx].y;
                mBombMarkFrame[status][i] = 1;
                JGeometry::TVec3f pos(screenPos[idx].x, screenPos[idx].y, 0.0f);
                if (RCMGetManager()->getKartInfo(J2DManager::getStatus2Kart(0))->isTagMode() && idx != 0) {
                    idx++;
                    if (idx >= 4)
                        idx = 3;
                }
                int win = (RCMGetManager()->getKartChecker(kartNo), KartChecker::getWinBombPoint());
                if (i < win - 2)
                    GameAudio::Main::getAudio()->startBombPointSe(pointKart, kartNo);
                else if (i == win - 2)
                    GameAudio::Main::getAudio()->startBombReachSe(pointKart, kartNo);
                if (i == win - 1)
                    GameAudio::Main::getAudio()->startBombPointSe(pointKart, kartNo);
                for (int j = 0; j < 3; j++) {
                    char name[32];
                    snprintf(name, sizeof(name), "mk_2dHit%dP_a", idx + 1);
                    JPEffectMgr::mThis->createEmt2D(name, pos);
                }
                JUTColor color = mPlayerNumberColor[idx];
                color.a = 0;
                mMiniPics[status][2][i]->setBlack(color);
            }
        }
        setEffectPos(status);
        sPreBombPoint[status] = point;
    }
}

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
    int winPoint = (RCMGetManager()->getKartChecker(kartNo), KartChecker::getWinBombPoint());
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
    int winPoint = (RCMGetManager()->getKartChecker(kartNo), KartChecker::getWinBombPoint());
    KartChecker *checker = RCMGetManager()->getKartChecker(kartNo);
    return checker->getBombPoint() == winPoint;
}

void Race2D::setEffectPos(int status)
{
    int kartNo = J2DManager::getStatus2Kart(status);
    int win = (RCMGetManager()->getKartChecker(kartNo), KartChecker::getWinBombPoint());
    RCMGetManager()->getKartChecker(kartNo);
    for (int i = 0; i < win; i++) {
        if (mBombMarkFrame[status][i] < 20) {
            _e40[status][i].x = mMiniPos[status][0][i].x;
            _e40[status][i].y = mMiniPos[status][0][i].y;
        } else if (mBombMarkFrame[status][i] < 31) {
            _e40[status][i].set(_dc0[status][i].x + (mBombMarkFrame[status][i] - 21) * (mMiniPos[status][0][i].x - _dc0[status][i].x) / 10.0f,
                                _dc0[status][i].y + (mBombMarkFrame[status][i] - 21) * (mMiniPos[status][0][i].y - _dc0[status][i].y) / 10.0f);
            for (int k = 0; k < 3; k++) {
                if (_ec0[status][i][k])
                    _ec0[status][i][k]->setGlobalTranslation(_e40[status][i].x, _e40[status][i].y, 0.0f);
            }
        } else {
            _e40[status][i].x = mMiniPos[status][0][i].x;
            _e40[status][i].y = mMiniPos[status][0][i].y;
            for (int k = 0; k < 3; k++) {
                if (_ec0[status][i][k])
                    _ec0[status][i][k]->setGlobalTranslation(_e40[status][i].x, _e40[status][i].y, 0.0f);
            }
        }
    }
}

void Race2D::calcEscape()
{
    JGeometry::TVec3f markPos;
    JGeometry::TVec2f dst;
    GeoRabbitMark::sSupervisor->getMarkPosition(&markPos);
    getMapPos(0, markPos, dst);
    if (!RCMGetManager()->isAbleStart())
    {
        mEscape.x = dst.x;
        mEscape.y = dst.y;
    }

    int i;
    Kart2DCommon *common;
    bool found = false;
    int rabbitNo = -1;
    for (i = 0; i < RCMGetManager()->getKartNumber(); i++)
    {
        if (RCMGetManager()->getKartChecker(i)->isRabbit())
        {
            rabbitNo = i;
            found = true;
            break;
        }
    }
    setEscapeColor(rabbitNo);

    const RaceTime &mgTime = RCMGetManager()->getMiniGameTime();
    int count = (mgTime.get() + 999) / 1000;
    if (found)
    {
        mEscapeFrame = (mEscapeFrame + 1) % 39;
        mTimeColorFrame = (mTimeColorFrame + 1) % 60;
        JGeometry::TVec2f d = mEscape - mEscapeTarget[rabbitNo];
        f32 dist = std::sqrtf(d.y * d.y + d.x * d.x);
        if (dist > 16.0f)
        {
            f32 f = 16.0f / dist;
            dst.x = f * d.x + mEscapeTarget[rabbitNo].x;
            dst.y = f * d.y + mEscapeTarget[rabbitNo].y;
            mEscape.set(dst);
        }
    }
    else
    {
        mEscapeFrame = 0;
        mTimeColorFrame = 0;
        JGeometry::TVec2f d = mEscape - dst;
        f32 dist = std::sqrtf(d.y * d.y + d.x * d.x);
        if (dist > 1.0f)
        {
            dst.x = d.x * ((dist - 1.0f) / dist) + dst.x;
            dst.y = d.y * ((dist - 1.0f) / dist) + dst.y;
            mEscape.set(dst);
        }
        else
        {
            mEscape.set(dst);
        }
    }

    calcTimeFrame(found, rabbitNo);

    common = Kart2DCommon::mspKart2DCommon;
    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++)
    {
        if (count >= 10)
        {
            common->changeNumberTexture(count, &mShinePics[status][2], 2, true, false);
            mShinePics[status][2]->show();
            mShinePics[status][3]->show();
            mShinePics[status][4]->hide();
        }
        else
        {
            common->changeNumberTexture(count, &mShinePics[status][4], 1, true, false);
            mShinePics[status][2]->hide();
            mShinePics[status][3]->hide();
            mShinePics[status][4]->show();
        }
    }

    sPreShineFlag = found;
    sPreShinePlayer = rabbitNo;
}

void Race2D::calcTimeFrame(bool shineFlag, int player)
{
    int mgTime = RCMGetManager()->getMiniGameTime().get();
    int ms = mgTime % 1000;
    for (int no = 0; no < RCMGetManager()->getStatusNumber(); no++)
    {
        int kartNo = J2DManager::getStatus2Kart(no);
        if (shineFlag && shineFlag != sPreShineFlag)
        {
            {
                if (player == kartNo)
                {
                    mBattleTimeCount[no] = 1;
                    mShineKind[no] = 10;
                }
                else
                {
                    mBattleTimeCount[no] = 2;
                    mShineKind[no] = 0;
                }
                continue;
            }
        }
        if (!shineFlag && shineFlag != sPreShineFlag)
        {
            {
                if (sPreShinePlayer == kartNo)
                {
                    mBattleTimeCount[no] = 3;
                    mShineKind[no] = 0x13;
                }
                else
                {
                    mBattleTimeCount[no] = 4;
                    mShineKind[no] = 9;
                }
                continue;
            }
        }
        if (shineFlag && shineFlag == sPreShineFlag && player != sPreShinePlayer)
        {
            if (player == kartNo)
            {
                mBattleTimeCount[no] = 1;
                mShineKind[no] = 10;
            }
            else if (sPreShinePlayer == kartNo)
            {
                mBattleTimeCount[no] = 3;
                mShineKind[no] = 0x13;
            }
            else
            {
                mBattleTimeCount[no] = 2;
                mShineKind[no] = 0;
            }
            continue;
        }
        if (shineFlag != sPreShineFlag)
            continue;
        if (player != sPreShinePlayer)
            continue;

        switch (mBattleTimeCount[no])
        {
        case 0:
            mShineKind[no] = 0;
            break;
        case 1:
        {
            int k = ++mShineKind[no];
            if (k > 0x13)
                mShineKind[no] = 0x13;
            if (ms == 100)
            {
                mBattleTimeCount[no] = 5;
                if (mgTime > 10100)
                    mShineKind[no] = 30;
                else
                    mShineKind[no] = 20;
            }
            break;
        }
        case 2:
        {
            int k = ++mShineKind[no];
            if (k > 9)
                mShineKind[no] = 9;
            if (ms == 100)
            {
                mBattleTimeCount[no] = 6;
                if (mgTime > 10100)
                    mShineKind[no] = 40;
                else
                    mShineKind[no] = 0;
            }
            break;
        }
        case 3:
        {
            int k = --mShineKind[no];
            if (k < 10)
                mShineKind[no] = 10;
            if (ms == 100)
            {
                mBattleTimeCount[no] = 6;
                if (mgTime > 10100)
                    mShineKind[no] = 40;
                else
                    mShineKind[no] = 0;
            }
            break;
        }
        case 4:
        {
            int k = --mShineKind[no];
            if (k < 0)
                mShineKind[no] = 0;
            if (ms == 100)
            {
                mBattleTimeCount[no] = 6;
                if (mgTime > 10100)
                    mShineKind[no] = 40;
                else
                    mShineKind[no] = 0;
            }
            break;
        }
        case 5:
            if (mgTime > 10100)
            {
                int k = ++mShineKind[no];
                if (k > 0x27)
                    mShineKind[no] = 0x27;
                if (ms == 100)
                    mShineKind[no] = 30;
            }
            else
            {
                int k = ++mShineKind[no];
                if (k > 0x1d)
                    mShineKind[no] = 0x1d;
                if (ms == 100)
                    mShineKind[no] = 20;
            }
            break;
        case 6:
            if (mgTime > 10100)
            {
                int k = ++mShineKind[no];
                if (k > 0x31)
                    mShineKind[no] = 0x31;
                if (ms == 100)
                    mShineKind[no] = 40;
            }
            else
            {
                int k = ++mShineKind[no];
                if (k > 9)
                    mShineKind[no] = 9;
                if (ms == 100)
                    mShineKind[no] = 0;
            }
            break;
        default:
#line 4473
            JUT_ASSERT_F(0, "%s", "0");
            break;
        }
    }
}
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

void Race2D::setEscapeColor(int rabbitNo)
{
    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++)
    {
        int no = status;
        if (J2DManager::mNetFlag)
        {
            no = J2DManager::getStatus2Kart(status);
        }
        else if (RCMGetManager()->getKartInfo(J2DManager::getStatus2Kart(0))->isTagMode() && status != 0)
        {
            no++;
            if (no >= 4)
                no = 3;
        }

        if (rabbitNo == -1)
        {
            mShinePics[status][0]->setCornerColor(getCornerColor(no));
            mShinePics[status][0]->setBlack(JUTColor(0));
            mShinePics[status][1]->setWhite(JUTColor(0xff));
            mShinePics[status][2]->setCornerColor(getCornerColor(no));
            mShinePics[status][3]->setCornerColor(getCornerColor(no));
            mShinePics[status][4]->setCornerColor(getCornerColor(no));
        }
        else if (rabbitNo == J2DManager::getStatus2Kart(status))
        {
            J2DPicture::TCornerColor cc;
            cc.mColor0.set(getTimeColor(JUTColor(255, 255, 0, 255), JUTColor(255, 255, 90, 255)));
            cc.mColor1.set(getTimeColor(JUTColor(255, 100, 0, 255), JUTColor(255, 255, 90, 255)));
            cc.mColor2.set(getTimeColor(JUTColor(255, 100, 0, 255), JUTColor(255, 255, 90, 255)));
            cc.mColor3.set(getTimeColor(JUTColor(255, 255, 0, 255), JUTColor(255, 255, 90, 255)));
            mShinePics[status][0]->setCornerColor(cc);
            mShinePics[status][0]->setBlack(getTimeColor(JUTColor(0, 0, 0, 0), JUTColor(100, 64, 0, 0)));
            mShinePics[status][1]->setWhite(getTimeColor(JUTColor(0, 0, 0, 255), JUTColor(0, 100, 220, 255)));
            mShinePics[status][2]->setCornerColor(cc);
            mShinePics[status][3]->setCornerColor(cc);
            mShinePics[status][4]->setCornerColor(cc);
        }
        else
        {
            J2DPicture::TCornerColor cc;
            cc.mColor0.set(getTimeColor(JUTColor(255, 100, 0, 255), JUTColor(255, 160, 0, 255)));
            cc.mColor1.set(getTimeColor(JUTColor(255, 100, 0, 255), JUTColor(255, 160, 0, 255)));
            cc.mColor2.set(getTimeColor(JUTColor(255, 100, 0, 255), JUTColor(255, 160, 0, 255)));
            cc.mColor3.set(getTimeColor(JUTColor(255, 100, 0, 255), JUTColor(255, 160, 0, 255)));
            mShinePics[status][0]->setCornerColor(cc);
            mShinePics[status][0]->setBlack(getTimeColor(JUTColor(0, 0, 0, 0), JUTColor(180, 24, 0, 0)));
            mShinePics[status][1]->setWhite(getTimeColor(JUTColor(0, 0, 0, 255), JUTColor(160, 32, 0, 255)));
            mShinePics[status][2]->setCornerColor(cc);
            mShinePics[status][3]->setCornerColor(cc);
            mShinePics[status][4]->setCornerColor(cc);
        }
    }
}

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

void Race2D::getMapPos(int rot, JGeometry::TVec3f &pos, JGeometry::TVec2f &dst)
{
    f32 w, h, ofsY, scale, baseY, rangeX, rangeY, ofsX, baseX;
    w = mCoursePic->getWidth();
    h = mCoursePic->getHeight();
    rangeX = mMapInfo.mMax.x - mMapInfo.mMin.x;
    rangeY = mMapInfo.mMax.y - mMapInfo.mMin.y;
    baseX = mCoursePos.x;
    baseY = mCoursePos.y;
    ofsX = mHio->getParam()->mX;
    ofsY = mHio->getParam()->mY;
    scale = mHio->getParam()->mS;
    switch (rot) {
    case 0:
        if (RCMGetManager()->isMirror())
            dst.x = baseX + ofsX - 0.5f * w * (scale - 1.0f) + w * scale * (1.0f - ((pos.x - mMapInfo.mMin.x) / rangeX));
        else
            dst.x = baseX + ofsX - 0.5f * w * (scale - 1.0f) + w * scale * ((pos.x - mMapInfo.mMin.x) / rangeX);
        dst.y = baseY + ofsY - 0.5f * h * (scale - 1.0f) + h * scale * ((pos.z - mMapInfo.mMin.y) / rangeY);
        break;
    case 1:
        if (RCMGetManager()->isMirror())
            dst.x = baseX + ofsX - 0.5f * w * (scale - 1.0f) + w * scale * (1.0f - ((mMapInfo.mMax.y - pos.z) / rangeY));
        else
            dst.x = baseX + ofsX - 0.5f * w * (scale - 1.0f) + w * scale * ((mMapInfo.mMax.y - pos.z) / rangeY);
        dst.y = baseY + ofsY - 0.5f * h * (scale - 1.0f) + h * scale * ((pos.x - mMapInfo.mMin.x) / rangeX);
        break;
    case 2:
        if (RCMGetManager()->isMirror())
            dst.x = baseX + ofsX - 0.5f * w * (scale - 1.0f) + w * scale * (1.0f - ((mMapInfo.mMax.x - pos.x) / rangeX));
        else
            dst.x = baseX + ofsX - 0.5f * w * (scale - 1.0f) + w * scale * ((mMapInfo.mMax.x - pos.x) / rangeX);
        dst.y = baseY + ofsY - 0.5f * h * (scale - 1.0f) + h * scale * ((mMapInfo.mMax.y - pos.z) / rangeY);
        break;
    case 3:
        if (RCMGetManager()->isMirror())
            dst.x = baseX + ofsX - 0.5f * w * (scale - 1.0f) + w * scale * (1.0f - ((pos.z - mMapInfo.mMin.y) / rangeY));
        else
            dst.x = baseX + ofsX - 0.5f * w * (scale - 1.0f) + w * scale * ((pos.z - mMapInfo.mMin.y) / rangeY);
        dst.y = baseY + ofsY - 0.5f * h * (scale - 1.0f) + h * scale * ((mMapInfo.mMax.x - pos.x) / rangeX);
        break;
    }
}

void Race2D::getCharacterInfo(int type, int frame, f32 &x, f32 &y, f32 &z)
{
#line 4735
    JUT_MINMAX_ASSERT(0, type, 2);
#line 4736
    JUT_MINMAX_ASSERT(0, frame, 39);
    x = mCharPos[1 - type][frame].x;
    y = mCharPos[1 - type][frame].y;
    z = mCharPos[1 - type][frame].z;
}

void Race2D::getCharacterClr(int kart, int type, int frame, JUTColor &color1, JUTColor &color2, u8 &alpha)
{
#line 4756
    JUT_MINMAX_ASSERT(0, kart, 8);
#line 4757
    JUT_MINMAX_ASSERT(0, type, 2);
#line 4758
    JUT_MINMAX_ASSERT(0, frame, 39);

    u8 w0 = 80;
    u8 w2 = 100;
    u8 v[6];
    v[1] = 125;
    v[4] = 140;
    v[5] = 255;
    v[0] = v[1];
    v[2] = v[4];
    v[3] = v[5];

    if (kart == J2DManager::getStatus2Kart(0))
    {
        v[0] = 255;
        v[1] = 190;
        v[4] = v[1];
        v[5] = 230;
        v[2] = v[0];
        v[3] = v[0];
    }
    else if (kart == J2DManager::getStatus2Kart(1))
    {
        v[0] = 255;
        v[1] = 190;
        v[4] = v[1];
        v[5] = 230;
        v[2] = v[0];
        v[3] = v[0];
    }

    f32 t = frame / 38.0f;
    switch (1 - type)
    {
    case 0:
        color1.r = w0 + t * (w0 - w0);
        color1.g = w0 + t * (w0 - w0);
        color1.b = w2 + t * (w2 - w2);
        color1.a = 255;
        color2.r = v[1] + t * (v[0] - v[1]);
        color2.g = v[4] + t * (v[2] - v[4]);
        color2.b = v[5] + t * (v[3] - v[5]);
        color2.a = 255;
        alpha = 255;
        break;
    case 1:
        color1.r = w0 + t * (w0 - w0);
        color1.g = w0 + t * (w0 - w0);
        color1.b = w2 + t * (w2 - w2);
        color1.a = 255;
        color2.r = v[0] + t * (v[1] - v[0]);
        color2.g = v[2] + t * (v[4] - v[2]);
        color2.b = v[3] + t * (v[5] - v[3]);
        color2.a = 255;
        alpha = 255;
        break;
    }

    u8 u[6];
    if (kart == J2DManager::getStatus2Kart(0))
    {
        u[0] = 255;
        u[2] = 80;
        u[4] = 93;
        u[1] = 128;
        u[3] = 55;
        u[5] = 70;
    }
    else if (kart == J2DManager::getStatus2Kart(1))
    {
        if (RCMGetManager()->getKartInfo(0)->getPad(1) != NULL)
        {
            u[0] = 0;
            u[2] = 220;
            u[4] = u[0];
            u[1] = u[0];
            u[3] = 128;
            u[5] = u[0];
        }
        else
        {
            u[0] = 0;
            u[2] = 150;
            u[4] = 255;
            u[1] = u[0];
            u[3] = 64;
            u[5] = 128;
        }
    }
    else
        return;

    f32 f = 0.5f + -0.5f * JMASCos(_4e9c[0] * 0xffff / 40 + 0x7fff);
    f32 rate = 1.0f;

    switch (type)
    {
    case 0:
        color1.r = rate * (u[0] + f * (u[1] - u[0]));
        color1.g = rate * (u[2] + f * (u[3] - u[2]));
        color1.b = rate * (u[4] + f * (u[5] - u[4]));
        color1.a = 255;
        break;
    case 1:
        color1.r = rate * (u[0] + f * (u[1] - u[0]));
        color1.g = rate * (u[2] + f * (u[3] - u[2]));
        color1.b = rate * (u[4] + f * (u[5] - u[4]));
        color1.a = 255;
        break;
    }
}

void Race2D::getItemInfo(int status, int type, int frame, f32 &x, f32 &y, f32 &z)
{
#line 4863
    JUT_MINMAX_ASSERT(0, status, 4);
#line 4864
    JUT_MINMAX_ASSERT(0, type, 2);
#line 4865
    JUT_MINMAX_ASSERT(0, frame, 39);
    x = mItemPos[status][type][frame].x;
    y = mItemPos[status][type][frame].y;
    z = mItemPos[status][type][frame].z;
}

void Race2D::getItemPos(int, int, JGeometry::TVec2f &) {}

void Race2D::decideItem(int status, u8 driver, int &item)
{
#line 4905
    JUT_MINMAX_ASSERT(0, status, 4);
    JUT_MINMAX_ASSERT(0, driver, 2);

    int itemKind = ItemObjMgr::getItemObjMgr()->getStockItem(J2DManager::getStatus2Kart(status), (u8)driver);
    ItemObj *itemObj = ItemObjMgr::getItemObjMgr()->getKartEquipItem(J2DManager::getStatus2Kart(status), (u8)driver);
    if (itemKind == 0x10 && itemObj != NULL)
        itemKind = itemObj->getKind();

    if (itemObj != NULL)
    {
        switch (itemKind)
        {
        case 0x11:
            switch (itemObj->getSuccessionItemNum())
            {
            default:
                break;
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
            default:
                break;
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
            default:
                break;
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
            if (fire != NULL && !(u8)((ItemFireBallSuc *)fire)->IsEfctTypeRed())
                itemKind = 0x1a;
            break;
        }
        case 0xb:
        {
            ItemObj *egg = ItemObjMgr::getItemYoshiEgg(itemObj);
            if (egg != NULL && egg->getItemColorID() != 1)
                itemKind = 0x1b;
            break;
        }
        }
    }
    else if (itemKind == 0x15)
    {
        if (gRaceInfo.getKartInfo(J2DManager::getStatus2Kart(status))->getDriverCharID(driver) == 8)
            itemKind = 0x1a;
    }
    else if (itemKind == 0xb)
    {
        if (gRaceInfo.getKartInfo(J2DManager::getStatus2Kart(status))->getDriverCharID(driver) == 0xc)
            itemKind = 0x1b;
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
    int status = mTask->mCurCB->mExecIdx;
    KartChecker *checker = RCMGetManager()->getKartChecker(J2DManager::getStatus2Kart(status));
    int lap = checker->getLap();
    if (lap < 0)
        lap = 0;
    if (lap >= RCMGetManager()->getTotalLapNumber())
        lap = RCMGetManager()->getTotalLapNumber();

    int min, sec, ms;
    checker->getTotalTime().get(&min, &sec, &ms);
    int d6 = ms / 100 % 10;
    int d5 = ms / 10 % 10;
    int d4 = ms % 10;
    int d3 = sec / 10 % 10;
    int d2 = sec % 10;
    int d1 = min / 10 % 10;
    int d0 = min % 10;
    bool available = checker->getTotalTime().isAvailable();
    int dispLap = lap;
    if (lap > RCMGetManager()->getTotalLapNumber() - 1)
        dispLap = RCMGetManager()->getTotalLapNumber() - 1;
    mThis->setRaceTime(status, available, dispLap, d0, d1, d2, d3, d4, d5, d6);

    if (lap != sPreLap[status] && lap != 0) {
        if (lap >= 1 && lap <= 10) {
            checker->getLapTime(lap - 1).get(&min, &sec, &ms);
            int l6 = ms / 100 % 10;
            int l5 = ms / 10 % 10;
            int l4 = ms % 10;
            int l3 = sec / 10 % 10;
            int l2 = sec % 10;
            int l1 = min / 10 % 10;
            int l0 = min % 10;
            bool lapAvailable = checker->getLapTime(lap - 1).isAvailable();
            mThis->setLapTime(status, lap - 1, lapAvailable, l0, l1, l2, l3, l4, l5, l6);
        }
        Task::TCB *tcb = mTask->make(NULL, 0x8000);
        if (tcb != NULL) {
            tcb->mExecIdx = status;
            mTask->change(lapInit, tcb);
        }
    }
    sPreLap[status] = lap;

    if (checker->isGoal() || RCMGetManager()->mRaceDirector->isRaceEnd()) {
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
    for (int lap = 0; lap < RCMGetManager()->getTotalLapNumber(); lap++) {
        for (int i = 0; i < 11; i++)
            mThis->getLapTimePic(status, lap, i)->show();
    }
    if (RCMGetManager()->getConsoleNumber() == 2) {
        for (int i = 0; i < 8; i++) {
            mThis->getRankPic(status, i)->setAlpha(255);
            mThis->getRankGoalPic(status, i)->setAlpha(0);
        }
    }
    mTask->change(lapTimeMain, mTask->mCurCB);
}

void Race2D::lapTimeMain()
{
    Task::TCB *cb = mTask->mCurCB;
    int status = cb->mExecIdx;
    int frame = cb->_10++;
    if (frame >= 15) {
        for (int lap = 0; lap < 9; lap++) {
            for (int tag = 0; tag < 11; tag++) {
                JGeometry::TVec2f pos = mThis->getLapTimePos(status, lap, tag);
                mThis->setLapTimePos2(status, lap, tag, pos);
                mThis->getLapTimePic(status, lap, tag)->setAlpha(255);
            }
        }
        if (RCMGetManager()->getConsoleNumber() == 2) {
            for (int i = 0; i < 8; i++) {
                mThis->getRankPic(status, i)->setAlpha(0);
                mThis->getRankGoalPic(status, i)->setAlpha(255);
            }
        }
        cb->mRequestKill = true;
    } else {
        int alpha = frame * 255 / 15;
        for (int lap = 0; lap < 9; lap++) {
            for (int tag = 0; tag < 11; tag++) {
                JGeometry::TVec2f pos = mThis->getLapTimePos(status, lap, tag);
                f32 y = mThis->getTimePosY(status);
                JGeometry::TVec2f newPos;
                newPos.x = pos.x;
                newPos.y = y + frame * (pos.y - y) / 15.0f;
                mThis->setLapTimePos2(status, lap, tag, newPos);
                mThis->getLapTimePic(status, lap, tag)->setAlpha(alpha);
            }
        }
        if (RCMGetManager()->getConsoleNumber() == 2) {
            u8 a = alpha;
            for (int i = 0; i < 8; i++) {
                mThis->getRankPic(status, i)->setAlpha(255 - a);
                mThis->getRankGoalPic(status, i)->setAlpha(a);
            }
        }
    }
}

void Race2D::calcItem()
{
    if (RCMGetManager()->getRaceMode() == BOMB_BATTLE)
        return;

    int item;
    for (int status = 0; status < RCMGetManager()->getStatusNumber(); status++)
    {
        for (u8 driver = 0; driver < 2; driver++)
        {
            if (ItemObjMgr::getItemObjMgr()->IsRollingSlot(J2DManager::getStatus2Kart(status), (u8)driver))
            {
                if (!sRollingFlag[status][driver])
                {
                    Task::TCB *tcb = mTask->make(NULL, 0x8000);
                    if (tcb != NULL)
                    {
                        tcb->mExecIdx = status;
                        *(u8 *)&tcb->_28 = driver;
                        mTask->change(itemStartInit, tcb);
                    }
                }
                sRollingFlag[status][driver] = true;
            }
            else
            {
                sRollingFlag[status][driver] = false;
            }

            ItemObj *itemObj = ItemObjMgr::getItemObjMgr()->getKartEquipItem(J2DManager::getStatus2Kart(status), (u8)driver);
            if (itemObj != NULL)
            {
                int preState = sItemState[status][driver];
                int state = itemObj->mState;
                if ((preState != 5 && state == 5) || (preState == 9 && state == 1))
                {
                    Task::TCB *tcb = mTask->make(NULL, 0x8000);
                    if (tcb != NULL)
                    {
                        mThis->decideItem(status, driver, item);

                        mThis->setItem(status, driver, 0, item);
                        mThis->setItem(status, driver, 1, item);

                        tcb->mExecIdx = status;
                        *(u8 *)&tcb->_28 = driver;
                        mTask->change(itemGetInit, tcb);
                    }
                }
                sItemState[status][driver] = itemObj->mState;
            }
            else
            {
                sItemState[status][driver] = 0;
            }
        }
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

void Race2D::itemStart()
{
    Task::TCB *cb = mTask->mCurCB;
    int frame;
    int status = cb->mExecIdx;
    u8 driver = *(u8 *)&cb->_28;
    frame = cb->_10++;
    int count = ++(*(int *)&cb->_24);
    static int SLOT_INVALID_BUTTON_FRAME = ItemShuffleMgr::SLOT_INVALID_BUTTON_COUNT;
    static int SLOT_MAX_FRAME = ItemShuffleMgr::SLOT_MAX_COUNT;

    int interval;
    if (frame < 80)
        interval = 5;
    else if (frame < 120)
        interval = 10;
    else
        interval = 15;

    if (ItemObjMgr::getItemObjMgr()->IsRollingSlot(J2DManager::getStatus2Kart(status), driver)) {
        if (frame % interval == 0) {
            *(int *)&cb->_24 = 0;
            count = 0;
            int slotItem;
            do {
                slotItem = ItemShuffleMgr::getRndItemKindPossibilityGetting(stGetRnd(4), J2DManager::getStatus2Kart(status), (u8)driver);
#line 5372
                JUT_MINMAX_ASSERT(0, slotItem, 31);
            } while (slotItem == mThis->getItem(status, driver, 1));
            switch (slotItem) {
            case 21:
                if (gRaceInfo.getKartInfo(J2DManager::getStatus2Kart(status))->getDriverCharID(driver) == 8)
                    slotItem = 26;
                break;
            case 11:
                if (gRaceInfo.getKartInfo(J2DManager::getStatus2Kart(status))->getDriverCharID(driver) == 12)
                    slotItem = 27;
                break;
            }
            int prev = mThis->getItem(status, driver, 0);
            mThis->setItem(status, driver, 1, prev);
            mThis->setItem(status, driver, 0, slotItem);
        }
        if (frame < 15) {
            JGeometry::TVec2f pos = mThis->getItemPos(status, driver);
            JGeometry::TVec2f start = mThis->getItemPos2(status, driver);
            JGeometry::TVec2f res = start + (pos - start) * frame * 0.06666667f;
            mThis->setItemPos3(status, driver, res);
        } else {
            JGeometry::TVec2f pos = mThis->getItemPos(status, driver);
            mThis->setItemPos3(status, driver, pos);
        }
        mThis->setItemFrame(status, driver, interval, count);
    } else {
        mTask->change(itemBlinkInit, mTask->mCurCB);
    }
}

void Race2D::itemBlinkInit()
{
    mTask->mCurCB->_10 = 0;
    mTask->mCurCB->_2a[0] = 1;
    mTask->change(itemBlink, mTask->mCurCB);
}

void Race2D::itemBlink()
{
    int count;
    Task::TCB *cb = mTask->mCurCB;
    int status = cb->mExecIdx;
    u8 driver = *(u8 *)&cb->_28;
    cb->_10++;
    count = (*(int *)&cb->_24)++;
    int item;
    mThis->decideItem(status, driver, item);
    JGeometry::TVec2f pos = mThis->getItemPos(status, driver);
    mThis->setItemPos3(status, driver, pos);
    if (ItemObjMgr::getItemObjMgr()->getStockItem(J2DManager::getStatus2Kart(status), (u8)driver) == 16) {
        mThis->setItem(status, driver, 0, 16);
        cb->mRequestKill = true;
        ItemObjMgr::getItemObjMgr()->tstItemUseTrigger(J2DManager::getStatus2Kart(status), (u8)driver);
    } else {
        if (count % 15 == 0) {
            if (item == mThis->getItem(status, driver, 0)) {
                *(int *)&cb->_24 = count = 0;
                mThis->setItem(status, driver, 1, item);
                if (!cb->_2a[1])
                    mThis->setItemFlag(status, driver, true);
                if (cb->_2a[0]) {
                    switch (item) {
                    case 1:
                    case 4:
                    case 7:
                    case 8:
                    case 11:
                    case 12:
                    case 14:
                    case 17:
                    case 19:
                    case 21:
                    case 26:
                    case 27: {
                        Task::TCB *tcb = mTask->make(NULL, 0x8000);
                        if (tcb != NULL) {
                            tcb->mExecIdx = status;
                            *(u8 *)&tcb->_28 = driver;
                            mTask->change(itemSpecialInit, tcb);
                        }
                        break;
                    }
                    }
                    cb->_2a[0] = 0;
                }
            } else {
                int prev = mThis->getItem(status, driver, 0);
                mThis->setItem(status, driver, 0, item);
                mThis->setItem(status, driver, 1, prev);
                cb->_2a[1] = 0;
            }
        }
        mThis->setItemFrame(status, driver, 15, count);
    }
}

void Race2D::itemGetInit()
{
    mTask->mCurCB->_10 = 0;
    Task::TCB *tcb = mTask->mCurCB;
    *(u32 *)&tcb->_24 = 0;
    tcb->_2a[1] = 1;
    mTask->change(itemGetMain, mTask->mCurCB);
}

void Race2D::itemGetMain()
{
    Task::TCB *cb = mTask->mCurCB;
    int frame;
    int status = cb->mExecIdx;
    u8 driver = *(u8 *)&cb->_28;
    frame = cb->_10++;
    if (frame < 15) {
        JGeometry::TVec2f pos = mThis->getItemPos(status, driver);
        JGeometry::TVec2f start = mThis->getItemPos2(status, driver);
        JGeometry::TVec2f res = start + (pos - start) * frame * 0.06666667f;
        mThis->setItemPos3(status, driver, res);
    } else {
        JGeometry::TVec2f pos = mThis->getItemPos(status, driver);
        mThis->setItemPos3(status, driver, pos);
        mTask->change(itemBlinkInit, mTask->mCurCB);
    }
    mThis->setItemFrame(status, driver, 15, 0);
}

void Race2D::itemSpecialInit() {
    mTask->mCurCB->_10 = 0;
    mTask->change(itemSpecialMain, mTask->mCurCB);
}

void Race2D::itemSpecialMain()
{
    Task::TCB *tcb = mTask->mCurCB;
    int frame;
    int status = tcb->mExecIdx;
    u8 slot = *(u8 *)&tcb->_28;
    frame = tcb->_10++;
    f32 scale = 0.0f;
    switch (RCMGetManager()->getConsoleNumber())
    {
    case 1:
        scale = 1.0f;
        break;
    case 2:
        scale = 0.7f;
        break;
    case 3:
    case 4:
        scale = 0.56f;
        break;
    default:
#line 5581
        JUT_ASSERT_F(0, "%s", "0");
        break;
    }
    if (frame < 8)
        scale *= 1.2f * frame / 7.0f;
    else if (frame < 10)
        scale *= (1.0f - 1.2f) * (frame - 8) / 2 + 1.2f;
    else if (frame >= 81)
        scale *= 1.0f - (frame - 81) / 9.0f;

    u8 alpha;
    if (frame < 15)
        alpha = 255;
    else if (frame < 20)
        alpha = (u8)(255 - (frame - 20) * 255 / 5);
    else if (frame < 25)
        alpha = (u8)((frame - 25) * 255 / 5);
    else if (frame < 40)
        alpha = 255;
    else if (frame < 45)
        alpha = (u8)(255 - (frame - 40) * 255 / 5);
    else if (frame < 50)
        alpha = (u8)((frame - 45) * 255 / 5);
    else
        alpha = 255;

    mThis->getSpecialItem(status, slot)->show();
    mThis->getSpecialItem(status, slot)->updateScale(scale, scale);
    mThis->getSpecialItem(status, slot)->setAlpha(alpha);

    if (frame < 90 && mThis->getItem(status, slot, 0) != 16)
        return;

    mThis->getSpecialItem(status, slot)->hide();
    tcb->mRequestKill = true;
}

void Race2D::itemUseInit() {}

void Race2D::itemUseMain() {}

void Race2D::setUsisi(int, ECharID) {}

void Race2D::alarmX(int no, f32 &x)
{
    f32 step = mAlarm[no].mStep;
    f32 base = mAlarm[no].mBase;
    f32 half = 0.5f;
    x = step * half + base;
}

void Race2D::getStartScaleA(int frame, f32 &scale)
{
    if (frame - 0x32 < 0)
        scale = 0.0f;
    else if (frame - 0x32 < 0xb)
        scale = 1.2f * (frame - 0x32) / 11.0f;
    else if (frame - 0x32 < 0x10)
        scale = 1.2f + (frame - 0x3d) * -0.50000006f / 5.0f;
    else if (frame - 0x32 < 0x14)
        scale = (frame - 0x42) * 0.3f / 4.0f + 0.7f;
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
        scale = 1.2f + (frame - 0x3d) * -0.50000006f / 5.0f;
    else if (frame - 0x32 < 0x14)
        scale = (frame - 0x42) * 0.3f / 4.0f + 0.7f;
    else
        scale = 1.0f;
}

void Race2D::getStartSpeedMeterScale(int frame, int no, f32 &scale)
{
    int t = frame - no * 4;
    if (t < 0)
        scale = 0.0f;
    else if (t < 9)
        scale = (f32)t * 1.2f / 9.0f;
    else if (t < 13)
    {
        scale = 1.2f + (f32)(t - 9) * -0.20000005f / 4.0f;
    }
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
            if (frame >= 10 && frame < 50)
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
    f32 scale = 0.0f;
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
    f32 scale = 0.0f;
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
        baseX = 101.0f - mPlayerHUD[kartNo].mRankPos[0].x;
        baseY = 101.0f - mPlayerHUD[kartNo].mRankPos[0].y;
        break;
    case 2:
        switch (kartNo)
        {
        case 0:
            baseX = 90.0f - mPlayerHUD[kartNo].mRankPos2[0].x;
            baseY = 82.0f - mPlayerHUD[kartNo].mRankPos2[0].y;
            break;
        case 1:
            baseX = 90.0f - mPlayerHUD[kartNo].mRankPos2[0].x;
            baseY = 368.0f - mPlayerHUD[kartNo].mRankPos2[0].y;
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
            baseX = 78.0f - mPlayerHUD[kartNo].mRankPos[0].x;
            baseY = 68.0f - mPlayerHUD[kartNo].mRankPos[0].y;
            break;
        case 1:
            baseX = 529.0f - mPlayerHUD[kartNo].mRankPos[0].x;
            baseY = 68.0f - mPlayerHUD[kartNo].mRankPos[0].y;
            break;
        case 2:
            baseX = 78.0f - mPlayerHUD[kartNo].mRankPos[0].x;
            baseY = 291.0f - mPlayerHUD[kartNo].mRankPos[0].y;
            break;
        case 3:
            baseX = 529.0f - mPlayerHUD[kartNo].mRankPos[0].x;
            baseY = 291.0f - mPlayerHUD[kartNo].mRankPos[0].y;
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
        JGeometry::TVec2f base(baseX, baseY);
        pos = base * (frame - 0x80) * 0.033333335f;
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
        scale = endScale + (startScale - endScale) * (frame - 0x9e) / 4.0f;
    else if (frame - 0x80 < 0x25)
        scale = startScale + (midScale - startScale) * (frame - 0xa2) / 3.0f;
    else if (frame - 0x80 < 0x28)
        scale = midScale + (frame - 0xa5) * (endScale - midScale) / 3.0f;
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
        x = 3.0f * (t - 0xf) / 4.0f;
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

#include "JSystem/JAudio/JASFakeMatch2.h"
