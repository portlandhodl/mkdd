#include "Osako/Kart2DCommon.h"
#include "Kameda/Result2D.h"
#include "mathHelper.h"
#include <stdio.h>
#include "Osako/password.h"
#include "Osako/ResMgr.h"
#include "JSystem/JUtility/TColor.h"
#include "JSystem/JMath/JMath.h"
#include "Inagaki/GameAudioMain.h"
#include "Kameda/J2DManager.h"
#include "Kaneshige/KartInfo.h"
#include "Kaneshige/RaceMgr.h"
#include "Kameda/Goal2D.h"
#include "Kameda/SequenceInfo.h"
#include "Kameda/PauseManager.h"
#include "Osako/GhostFile.h"
#include "Osako/CardAgent.h"
#include "Osako/SystemRecord.h"
#include "Yamamoto/kartCtrl.h"
#include "mathHelper.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "Osako/system.h"
#include "Kameda/SceneCourseSelect.h"
#include "Kameda/Race2D.h"

u8 Result2D::mDrawFlag;
int Result2D::mResultSelector;
int Result2D::mResultState;

JUTColor Result2D::mWhiteColor[4][2] = {
    0xff505dff, 0x80283cff, 
    0x0064ffff, 0x003280ff,
    0x1e871eff, 0x004600ff, 
    0xb4b4b4ff, 0xb4b4b4ff
};

const u64 Result2D::mOverBackTag[1] = {'OverBack'};

const char *Result2D::mRankName[] = {
    "position1_1.bti",
    "position1_2.bti",
    "position1_3.bti",
    "position1_4.bti",
    "position1_5.bti",
    "position1_6.bti",
    "position1_7.bti",
    "position1_8.bti",
};

const char *Result2D::mPlayerNumberName[] = {
    "PlayerNumberSimple_1P.bti",
    "PlayerNumberSimple_2P.bti",
    "PlayerNumberSimple_3P.bti",
    "PlayerNumberSimple_4P.bti"
};

const char *Result2D::mResultArrowName[] = {
    "ArrowGP1.bti",
    "ArrowGP2.bti",
    "ArrowGP3.bti"
};

const u64 Result2D::mSetWordLineTag[] = {
    'RCursBL1', 'RCursBL2', 'RCursBL3', 'RCursBR1', 
    'RCursBR2', 'RCursBR3', 'RMozi00', 'RCurs01', 
    'RCurs02', 
};

const u64 Result2D::mResultLayoutTag[] = {
    'RCoNaBA', 'RCoNaA', 'CuNameA', 'RCuPctA', 
    'RPow03A', 'RPow02A', 'RPow01A', 'RPowM1A', 
    'RPowM2A', 'RPowM3A', 'Mirror', 
};

const u64 Result2D::mResultLineTag[] = {
    'RCurs01', 'RCurs02', 'RCurs03', 'RChaB02', 
    'RChara02', 'RChaB01', 'RChara01', 'RPos00', 
    'Rtime00', 'Rtime01', 'Rtime02', 'Rtime03', 
    'Rtime04', 'Rtime05', 'Rtime06', 'RtimeT01', 
    'RtimeT02', 'RPlus01', 'RPlus', 'Arrow_00',
};

const u64 Result2D::mGetPointTag[] = {
    'Point_00', 'Point_01', 'Point_02', 'Point_03'
};

const u64 Result2D::mVSBattleCharacterTag[] = {
    'RChaW2', 'RChaPc2', 'RChaW1', 'RChaPc1', 
    'Win00', 'Pn_Ind1', 'Pn_Ind2', 'Pn_Ind3', 
    'Back1a', 'Back2a', 'Back3a', 'Back1b', 
    'Back2b', 'Back3b',
};

const u64 Result2D::mVSBattlePointTag[] = {
    'PPoint11', 'PPoint12', 
};

Result2D::Result2D(JKRHeap *heap) {
    mNode = new (heap, 0) Result2DHioNode("リザルト表示");
    mOrtho = System::getJ2DOrtho();

    switch (RCMGetManager()->getRaceMode()) {
    case GRAND_PRIX:
    case AWARD_DEMO:
    case STAFF_ROLL: {
        mScreen = new (heap, 0) J2DScreen();
        for (int i = 0; i < 3; i++) {
            mWinScreens[i] = new (heap, 0) J2DScreen();
        }
        mEntries = 3;
        mResultRaceScreen = new (heap, 0) J2DScreen();
        mResultLineScreen = new (heap, 0) J2DScreen();
        for (int i = 0; i < 8; i++) {
            mRankScreens[i] = new (heap, 0) J2DScreen();
        }
        for (int i = 0; i < 8; i++) {
            mGetPointScreens[i] = new (heap, 0) J2DScreen();
        }

        mScreen->set("OverScreen.blo", GETJ2DManager()->getArchive());
        for (int i = 0; i < 3; i++) {
            mWinScreens[i]->set("SetWordLine.blo", GETJ2DManager()->getArchive());
        }
        mResultRaceScreen->set("ResultLayout.blo", GETJ2DManager()->getArchive());
        mResultLineScreen->set("ResultRace.blo", GETJ2DManager()->getArchive());
        for (int i = 0; i < 8; i++) {
            mRankScreens[i]->set("ResultLine.blo", GETJ2DManager()->getArchive());
        }
        for (int i = 0; i < 8; i++) {
            mGetPointScreens[i]->set("GetPoint.blo", GETJ2DManager()->getArchive());
        }

        {
            J2DPane *pane = mScreen->search(mOverBackTag[0]);
#line 215
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mOverBack.mPane = (J2DPicture *)pane;
            JGeometry::TBox2f box(*mOverBack.mPane->getBounds());
            mOverBack.mCenter.x = (box.i.x + box.f.x) / 2;
            mOverBack.mCenter.y = (box.i.y + box.f.y) / 2;
        }
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 9; j++) {
                J2DPane *pane = mWinScreens[i]->search(mSetWordLineTag[j]);
#line 224
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mSetWord[i][j].mPane = (J2DPicture *)pane;
                JGeometry::TBox2f box(*mSetWord[i][j].mPane->getBounds());
                mSetWord[i][j].mCenter.x = (box.i.x + box.f.x) / 2;
                mSetWord[i][j].mCenter.y = (box.i.y + box.f.y) / 2;
            }
            if (i != 0) {
                mSetWord[i][7].mPane->hide();
                mSetWord[i][8].mPane->hide();
            }
        }
        for (int i = 0; i < 11; i++) {
            J2DPane *pane = mResultRaceScreen->search(mResultLayoutTag[i]);
#line 238
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mLayout[i].mPane = (J2DPicture *)pane;
            JGeometry::TBox2f box(*mLayout[i].mPane->getBounds());
            mLayout[i].mCenter.x = (box.i.x + box.f.x) / 2;
            mLayout[i].mCenter.y = (box.i.y + box.f.y) / 2;
        }
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 20; j++) {
                J2DPane *pane = mRankScreens[i]->search(mResultLineTag[j]);
#line 247
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mLines[i][j].mPane = (J2DPicture *)pane;
                JGeometry::TBox2f box(*mLines[i][j].mPane->getBounds());
                mLines[i][j].mCenter.x = (box.i.x + box.f.x) / 2;
                mLines[i][j].mCenter.y = (box.i.y + box.f.y) / 2;
            }
        }
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 4; j++) {
                J2DPane *pane = mGetPointScreens[i]->search(mGetPointTag[j]);
#line 257
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mRankRows[i][j].mPane = (J2DPicture *)pane;
                JGeometry::TBox2f box(*mRankRows[i][j].mPane->getBounds());
                mRankRows[i][j].mCenter.x = (box.i.x + box.f.x) / 2;
                mRankRows[i][j].mCenter.y = (box.i.y + box.f.y) / 2;
            }
        }
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 8; j++) {
                J2DPane *pane = mResultLineScreen->search(j + ('Rposr01' + (i << 8)));
                JGeometry::TBox2f box(*pane->getBounds());
                _404[i][j] = box.i;
            }
        }

        void *courseName = ResMgr::getPtr(ResMgr::mcCourseNameBti);
        if (courseName) {
            mLayout[1].mPane->changeTexture((ResTIMG *)courseName, 0);
            mLayout[1].mPane->show();
        } else {
            mLayout[1].mPane->hide();
        }
        mLayout[2].mPane->changeTexture(Kart2DCommon::ptr()->getCupNameTexture(SceneCourseSelect::mRaceGpCup[gSequenceInfo.getCup()]), 0);
        mLayout[3].mPane->changeTexture(Kart2DCommon::ptr()->getCupIconTexture(SceneCourseSelect::mRaceGpCup[gSequenceInfo.getCup()]), 0);

        switch (RCMGetManager()->getRaceLevel()) {
        case LVL_50CC:
            mLayout[4].mPane->hide();
            mLayout[5].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)5), 0);
            mLayout[6].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)0), 0);
            if (KartLocale::getLanguage() == 2) {
                mLayout[9].mPane->show();
            }
            break;
        case LVL_100CC:
            mLayout[4].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)1), 0);
            mLayout[5].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)0), 0);
            mLayout[6].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)0), 0);
            if (KartLocale::getLanguage() == 2) {
                mLayout[9].mPane->show();
            }
            break;
        case LVL_150CC:
            mLayout[4].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)1), 0);
            mLayout[5].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)5), 0);
            mLayout[6].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)0), 0);
            if (KartLocale::getLanguage() == 2) {
                mLayout[9].mPane->show();
            }
            break;
        case LVL_MIRROR:
            mLayout[4].mPane->hide();
            mLayout[5].mPane->hide();
            mLayout[6].mPane->hide();
            mLayout[7].mPane->hide();
            mLayout[8].mPane->hide();
            mLayout[10].mPane->show();
            break;
        default:
#line 320
            JUT_ASSERT(0);
            break;
        }

        for (int i = 0; i < 8; i++) {
            J2DPane *pane = mResultRaceScreen->search('SetPos01' + i);
            JGeometry::TBox2f box(*pane->getBounds());
            _3cc[i] = box.i.y;
        }
        for (int i = 0; i < 3; i++) {
            J2DPane *pane = mResultRaceScreen->search('RTr01' + i);
            JGeometry::TBox2f box(*pane->getBounds());
            _3ec[i] = box.i.x;
        }
        for (int i = 0; i < 3; i++) {
            J2DPane *pane = mResultRaceScreen->search('CwhiteP1' + i);
#line 336
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            _3f8[i] = ((J2DPicture *)pane)->getWhite();
        }
        for (int i = 0; i < 8; i++) {
            J2DPicture::TCornerColor corner;
            GETJ2DManager()->getRace2D()->getRankPic(0, i)->getCornerColor(corner);
            mLines[i][7].mPane->setCornerColor(corner);
            mLines[i][7].mPane->changeTexture(mRankName[i], 0);
            mLines[i][17].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)SequenceInfo::RANKPOINT[i]), 0);
        }
        for (int i = 0; i < 2; i++) {
            J2DPane *pane = mRankScreens[0]->search('RPSet01' + i);
            JGeometry::TBox2f box(*pane->getBounds());
            _c44[i] = box.i;
        }
        _dd4 = 224.0f;

        J2DScreen *screen = new (heap, 0) J2DScreen();
        screen->set("ResultLayout.blo", 0x40000, GETJ2DManager()->getArchive());
        J2DAnmTransform *anm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRGetNameResource("ResultLayout.bck", GETJ2DManager()->getArchive()));
        screen->setAnimation(anm);
        for (int i = 0; i < 2; i++) {
            J2DPane *pane = screen->search('RAniS01' + i);
            for (int j = 0; j < 10; j++) {
                anm->setFrame(j);
                screen->animation();
                mLineScaleTbl[i][j].v.x = pane->getScaleX();
                mLineScaleTbl[i][j].v.y = pane->getScaleY();
            }
        }
        delete screen;

        for (int i = 0; i < 11; i++) {
            mLayout[i].mPane->setAlpha(0);
        }
        for (int i = 0; i < 3; i++) {
            mRankIconPics[i] = (ResTIMG *)J2DScreen::getNameResource(mResultArrowName[i]);
        }
        mDrawFunc = &Result2D::drawGP;
        mCalcFunc = &Result2D::setDrawGP;
        mAutoFunc = &Result2D::selectorGP;
        break;
    }
    case VERSUS_RACE:
    case BALLOON_BATTLE:
    case BOMB_BATTLE:
    case ESCAPE_BATTLE: {
        mScreen = new (heap, 0) J2DScreen();
        for (int i = 0; i < 5; i++) {
            mWinScreens[i] = new (heap, 0) J2DScreen();
        }
        mEntries = 5;
        mVSLayoutScreen = new (heap, 0) J2DScreen();
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                mScoreScreens[i][j] = new (heap, 0) J2DScreen();
            }
        }
        for (int i = 0; i < 4; i++) {
            mPointScreens[i] = new (heap, 0) J2DScreen();
        }
        for (int i = 0; i < 4; i++) {
            mPointScreens2[i] = new (heap, 0) J2DScreen();
        }
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                _e1c[i][j] = new (heap, 0) J2DScreen();
            }
        }

        mScreen->set("OverScreen.blo", GETJ2DManager()->getArchive());
        for (int i = 0; i < 5; i++) {
            mWinScreens[i]->set("SetWordLine.blo", GETJ2DManager()->getArchive());
        }
        mVSLayoutScreen->set("VS_Battle_Layout.blo", GETJ2DManager()->getArchive());
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                if (i == 0) {
                    mScoreScreens[i][j]->set("VS_Battle_CharacterIndication.blo", GETJ2DManager()->getArchive());
                } else {
                    mScoreScreens[i][j]->set("VS_Battle_CharacterIndication2.blo", GETJ2DManager()->getArchive());
                }
            }
        }
        for (int i = 0; i < 4; i++) {
            mPointScreens[i]->set("VS_Battle_Point.blo", GETJ2DManager()->getArchive());
        }
        for (int i = 0; i < 4; i++) {
            mPointScreens2[i]->set("VS_Free_Point_1.blo", GETJ2DManager()->getArchive());
        }
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                _e1c[i][j]->set("VS_Free_Point_n.blo", GETJ2DManager()->getArchive());
            }
        }

        {
            J2DPane *pane = mScreen->search(mOverBackTag[0]);
#line 448
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mOverBack.mPane = (J2DPicture *)pane;
            JGeometry::TBox2f box(*mOverBack.mPane->getBounds());
            mOverBack.mCenter.x = (box.i.x + box.f.x) / 2;
            mOverBack.mCenter.y = (box.i.y + box.f.y) / 2;
        }
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 9; j++) {
                J2DPane *pane = mWinScreens[i]->search(mSetWordLineTag[j]);
#line 457
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mSetWord[i][j].mPane = (J2DPicture *)pane;
                JGeometry::TBox2f box(*mSetWord[i][j].mPane->getBounds());
                mSetWord[i][j].mCenter.x = (box.i.x + box.f.x) / 2;
                mSetWord[i][j].mCenter.y = (box.i.y + box.f.y) / 2;
            }
            if (i != 0) {
                mSetWord[i][7].mPane->hide();
                mSetWord[i][8].mPane->hide();
            }
        }
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 14; k++) {
                    J2DPane *pane = mScoreScreens[i][j]->search(mVSBattleCharacterTag[k]);
#line 473
                    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                    mVSBattle[i][j][k].mPane = (J2DPicture *)pane;
                    JGeometry::TBox2f box(*mVSBattle[i][j][k].mPane->getBounds());
                    mVSBattle[i][j][k].mCenter.x = (box.i.x + box.f.x) / 2;
                    mVSBattle[i][j][k].mCenter.y = (box.i.y + box.f.y) / 2;
                }
            }
        }
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 2; j++) {
                J2DPane *pane = mPointScreens[i]->search(mVSBattlePointTag[j]);
#line 484
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mPointRows[i][j].mPane = (J2DPicture *)pane;
                JGeometry::TBox2f box(*mPointRows[i][j].mPane->getBounds());
                mPointRows[i][j].mCenter.x = (box.i.x + box.f.x) / 2;
                mPointRows[i][j].mCenter.y = (box.i.y + box.f.y) / 2;
            }
        }
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                J2DPane *pane = mPointScreens2[i]->search('Rank1_01' + j);
#line 495
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                _1410[i][j].mPane = (J2DPicture *)pane;
                JGeometry::TBox2f box(*_1410[i][j].mPane->getBounds());
                _1410[i][j].mCenter.x = (box.i.x + box.f.x) / 2;
                _1410[i][j].mCenter.y = (box.i.y + box.f.y) / 2;
            }
        }
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                for (int k = 0; k < 4; k++) {
                    J2DPane *pane = _e1c[i][j]->search('RankN_01' + k);
#line 507
                    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                    _14d0[i][j][k].mPane = (J2DPicture *)pane;
                    JGeometry::TBox2f box(*_14d0[i][j][k].mPane->getBounds());
                    _14d0[i][j][k].mCenter.x = (box.i.x + box.f.x) / 2;
                    _14d0[i][j][k].mCenter.y = (box.i.y + box.f.y) / 2;
                }
            }
        }
        for (int i = 0; i < 2; i++) {
            J2DPane *pane = mVSLayoutScreen->search('YPoGame0' + i);
            JGeometry::TBox2f box(*pane->getBounds());
            _e4c[i] = box.i.y;
        }
        {
            J2DPane *pane = mVSLayoutScreen->search('Rank0Y1');
            JGeometry::TBox2f box(*pane->getBounds());
            _e54 = box.i.y;
        }
        for (int i = 0; i < 2; i++) {
            J2DPane *pane = mVSLayoutScreen->search('Rank1Y1' + i * 0x30000);
            JGeometry::TBox2f box(*pane->getBounds());
            _e58[i] = box.i.y;
        }
        for (int i = 0; i < 2; i++) {
            J2DPane *pane = mVSLayoutScreen->search('SetPoi00' + i);
            JGeometry::TBox2f box(*pane->getBounds());
            _e60[i] = box.i.y;
        }
        for (int i = 0; i < 2; i++) {
            J2DPane *pane = mVSLayoutScreen->search('ChaSet01' + i);
            JGeometry::TBox2f box(*pane->getBounds());
            _e68[i] = box.i.y;
        }

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < RCMGetManager()->getStatusNumber(); j++) {
                if (RCMGetManager()->isRaceModeMiniGame()) {
                    mVSBattle[i][j][8].mPane->hide();
                    mVSBattle[i][j][9].mPane->hide();
                    mVSBattle[i][j][10].mPane->hide();
                    mVSBattle[i][j][11].mPane->show();
                    mVSBattle[i][j][12].mPane->show();
                    mVSBattle[i][j][13].mPane->show();
                } else {
                    mVSBattle[i][j][8].mPane->show();
                    mVSBattle[i][j][9].mPane->show();
                    mVSBattle[i][j][10].mPane->show();
                    mVSBattle[i][j][11].mPane->hide();
                    mVSBattle[i][j][12].mPane->hide();
                    mVSBattle[i][j][13].mPane->hide();
                }
                if (gRaceInfo.getKartInfo(j)->isTagMode()) {
                    mVSBattle[i][j][5].mPane->hide();
                    mVSBattle[i][j][6].mPane->show();
                    mVSBattle[i][j][7].mPane->show();
                    int player = gSequenceInfo.getPad2Player(gRaceInfo.getKartInfo(j)->getPad(0));
                    if (player == -1) {
                        player = 0;
                    }
                    if (player >= 0 && player < 4) {
                        mVSBattle[i][j][7].mPane->changeTexture(mPlayerNumberName[player], 0);
                        mVSBattle[i][j][7].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][8].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][9].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][10].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][11].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][12].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][13].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                    }
                    player = gSequenceInfo.getPad2Player(gRaceInfo.getKartInfo(j)->getPad(1));
                    if (player == -1) {
                        player = 0;
                    }
                    if (player >= 0 && player < 4) {
                        mVSBattle[i][j][6].mPane->changeTexture(mPlayerNumberName[player], 0);
                        mVSBattle[i][j][6].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                    }
                } else {
                    mVSBattle[i][j][5].mPane->show();
                    mVSBattle[i][j][6].mPane->hide();
                    mVSBattle[i][j][7].mPane->hide();
                    int player = gSequenceInfo.getPad2Player(gRaceInfo.getKartInfo(j)->getPad(0));
                    if (player == -1) {
                        player = 0;
                    }
                    if (player >= 0 && player < 4) {
                        mVSBattle[i][j][5].mPane->changeTexture(mPlayerNumberName[player], 0);
                        mVSBattle[i][j][5].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][8].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][9].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][10].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][11].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][12].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                        mVSBattle[i][j][13].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                    }
                }
                if (i == 0) {
                    mVSBattle[i][j][4].mPane->show();
                } else {
                    mVSBattle[i][j][4].mPane->hide();
                }
                int charID = gRaceInfo.getKartInfo(j)->getDriverCharID(0);
                if (charID > 0 && charID < 21) {
                    mVSBattle[i][j][3].mPane->changeTexture(Race2D::mCharacterName[charID - 1], 0);
                }
                charID = gRaceInfo.getKartInfo(j)->getDriverCharID(1);
                if (charID > 0 && charID < 21) {
                    mVSBattle[i][j][1].mPane->changeTexture(Race2D::mCharacterName[charID - 1], 0);
                }
            }
        }

        for (int i = 0; i < 4; i++) {
            J2DPicture::TCornerColor corner;
            int player = i;
            if (RCMGetManager()->getKartInfo(0)->isTagMode() && i != 0) {
                player = i + 1;
                if (player >= 4) {
                    player = 3;
                }
            }
            J2DPane *pane = mVSLayoutScreen->search('C4_1P' + (player << 8));
#line 641
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            ((J2DPicture *)pane)->getCornerColor(corner);
            mPointRows[i][0].mPane->setCornerColor(corner);
            mPointRows[i][1].mPane->setCornerColor(corner);
        }
        for (int i = 0; i < 4; i++) {
            int player = gSequenceInfo.getPad2Player(gRaceInfo.getKartInfo(i)->getPad(0));
            if (player == -1) {
                player = 0;
            }
            if (RCMGetManager()->getKartInfo(0)->isTagMode() && player != 0) {
                player++;
                if (player >= 4) {
                    player = 3;
                }
            }
            if (player >= 0 && player < 4) {
                _1410[i][0].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                _1410[i][1].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
            }
        }
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) {
                int player = gSequenceInfo.getPad2Player(gRaceInfo.getKartInfo(i)->getPad(0));
                if (player == -1) {
                    player = 0;
                }
                if (RCMGetManager()->getKartInfo(0)->isTagMode() && player != 0) {
                    player++;
                    if (player >= 4) {
                        player = 3;
                    }
                }
                if (player >= 0 && player < 4) {
                    _14d0[i][j][0].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                    _14d0[i][j][1].mPane->setCornerColor(Race2D::mPlayerNumberColor[player]);
                    J2DPicture::TCornerColor corner;
                    GETJ2DManager()->getRace2D()->getRankPic(0, j + 1)->getCornerColor(corner);
                    _14d0[i][j][2].mPane->setCornerColor(corner);
                    _14d0[i][j][3].mPane->setCornerColor(corner);
                    _14d0[i][j][3].mPane->changeTexture(mRankName[j + 1], 0);
                }
            }
        }

        J2DScreen *screen = new (heap, 0) J2DScreen();
        screen->set("VS_Battle_Layout.blo", 0x40000, GETJ2DManager()->getArchive());
        J2DAnmTransform *anm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRGetNameResource("VS_Battle_Layout.bck", GETJ2DManager()->getArchive()));
        screen->setAnimation(anm);
        J2DPane *pane = screen->search('RCIAni01');
        for (int i = 0; i < 16; i++) {
            anm->setFrame(i);
            screen->animation();
            _1bb4[i] = pane->getScaleX();
        }
        delete screen;

        if (RCMGetManager()->isRaceModeMiniGame()) {
            mDrawFunc = &Result2D::drawMG;
            mCalcFunc = &Result2D::setDrawMG;
            mAutoFunc = &Result2D::selectorMG;
        } else {
            mDrawFunc = &Result2D::drawVS;
            mCalcFunc = &Result2D::setDrawVS;
            mAutoFunc = &Result2D::selectorVS;
        }
        break;
    }
    case TIME_ATTACK: {
        mScreenPw = new (heap, 0) J2DScreen();
        mScreen = new (heap, 0) J2DScreen();
        for (int i = 0; i < 6; i++) {
            mWinScreens[i] = new (heap, 0) J2DScreen();
        }
        mEntries = 6;
        mScreenMG = new (heap, 0) J2DScreen();
        mScreenMG2 = new (heap, 0) J2DScreen();
        for (int i = 0; i < 5; i++) {
            mTAScreens[i] = new (heap, 0) J2DScreen();
        }
        for (int i = 0; i < 6; i++) {
            mTAScreens[i + 5] = new (heap, 0) J2DScreen();
        }

        mScreenPw->set("Password.blo", GETJ2DManager()->getArchive());
        mScreen->set("OverScreen.blo", GETJ2DManager()->getArchive());
        for (int i = 0; i < 6; i++) {
            mWinScreens[i]->set("SetWordLine.blo", GETJ2DManager()->getArchive());
        }
        mScreenMG->set("TANRLayout.blo", 0x1040000, GETJ2DManager()->getArchive());
        mScreenMG2->set("TANRLapLine.blo", 0x1040000, GETJ2DManager()->getArchive());
        for (int i = 0; i < 5; i++) {
            mTAScreens[i]->set("TANRTimeLine.blo", 0x1040000, GETJ2DManager()->getArchive());
        }
        for (int i = 0; i < 6; i++) {
            mTAScreens[i + 5]->set("TANRSpark1.blo", 0x1040000, GETJ2DManager()->getArchive());
        }

        for (int i = 0; i < 6; i++) {
            J2DPane *pane = mScreenMG->search((u64)'Rec1' + i);
            if (i < 5) {
                pane->appendChild(mTAScreens[i]);
            } else {
                pane->appendChild(mScreenMG2);
            }
            pane->appendChild(mTAScreens[i + 5]);
        }

        _1744[0].mAnm = J2DAnmLoaderDataBase::load(JKRGetNameResource("TANRLayout.bck", GETJ2DManager()->getArchive()));
        mScreenMG->setAnimation((J2DAnmTransform *)_1744[0].mAnm);
        _17ac[0].mAnm = J2DAnmLoaderDataBase::load(JKRGetNameResource("TANRLayout.brk", GETJ2DManager()->getArchive()));
        _17ac[0].mAnm->searchUpdateMaterialID(mScreenMG);
        mScreenMG->setAnimation((J2DAnmTevRegKey *)_17ac[0].mAnm);
        _1744[1].mAnm = J2DAnmLoaderDataBase::load(JKRGetNameResource("TANRLapLine.bck", GETJ2DManager()->getArchive()));
        mScreenMG2->setAnimation((J2DAnmTransform *)_1744[1].mAnm);
        _17ac[1].mAnm = J2DAnmLoaderDataBase::load(JKRGetNameResource("TANRLapLine.brk", GETJ2DManager()->getArchive()));
        _17ac[1].mAnm->searchUpdateMaterialID(mScreenMG2);
        mScreenMG2->setAnimation((J2DAnmTevRegKey *)_17ac[1].mAnm);
        for (int i = 0; i < 5; i++) {
            _1754[i].mAnm = J2DAnmLoaderDataBase::load(JKRGetNameResource("TANRTimeLine.bck", GETJ2DManager()->getArchive()));
            mTAScreens[i]->setAnimation((J2DAnmTransform *)_1754[i].mAnm);
        }
        for (int i = 0; i < 5; i++) {
            _17bc[i].mAnm = J2DAnmLoaderDataBase::load(JKRGetNameResource("TANRTimeLine.brk", GETJ2DManager()->getArchive()));
            _17bc[i].mAnm->searchUpdateMaterialID(mTAScreens[i]);
            mTAScreens[i]->setAnimation((J2DAnmTevRegKey *)_17bc[i].mAnm);
        }
        for (int i = 0; i < 6; i++) {
            _177c[i].mAnm = J2DAnmLoaderDataBase::load(JKRGetNameResource("TANRSpark1.bck", GETJ2DManager()->getArchive()));
            mTAScreens[i + 5]->setAnimation((J2DAnmTransform *)_177c[i].mAnm);
        }
        for (int i = 0; i < 6; i++) {
            _17e4[i].mAnm = J2DAnmLoaderDataBase::load(JKRGetNameResource("TANRSpark1.bpk", GETJ2DManager()->getArchive()));
            _17e4[i].mAnm->searchUpdateMaterialID(mTAScreens[i + 5]);
            mTAScreens[i + 5]->setAnimation((J2DAnmColor *)_17e4[i].mAnm);
        }

        for (int i = 0; i < 16; i++) {
            J2DPane *pane = mScreenPw->search('Pass_a' + i);
#line 795
            JUT_ASSERT(pane->getTypeID() == J2DPane_TextBox);
            mPassWDText[i] = (J2DTextBox *)pane;
        }

        {
            J2DPane *pane = mScreen->search(mOverBackTag[0]);
#line 801
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mOverBack.mPane = (J2DPicture *)pane;
            JGeometry::TBox2f box(*mOverBack.mPane->getBounds());
            mOverBack.mCenter.x = (box.i.x + box.f.x) / 2;
            mOverBack.mCenter.y = (box.i.y + box.f.y) / 2;
        }
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 9; j++) {
                J2DPane *pane = mWinScreens[i]->search(mSetWordLineTag[j]);
#line 810
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mSetWord[i][j].mPane = (J2DPicture *)pane;
                JGeometry::TBox2f box(*mSetWord[i][j].mPane->getBounds());
                mSetWord[i][j].mCenter.x = (box.i.x + box.f.x) / 2;
                mSetWord[i][j].mCenter.y = (box.i.y + box.f.y) / 2;
            }
            if (i != 0) {
                mSetWord[i][7].mPane->hide();
                mSetWord[i][8].mPane->hide();
            }
        }

        for (int i = 0; i < 3; i++) {
            J2DPane *pane = mScreenMG2->search((u64)'BLN0' + i);
#line 827
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mTABest.namePics[i] = (J2DPicture *)pane;
        }
        for (int i = 0; i < 7; i++) {
            J2DPane *pane = mScreenMG2->search((u64)'BLT0' + i);
#line 833
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            mTABest.msPics[i] = (J2DPicture *)pane;
        }
        for (int i = 0; i < 5; i++) {
            J2DPicture::TCornerColor corner;
            GETJ2DManager()->getRace2D()->getRankPic(0, i)->getCornerColor(corner);
            J2DPane *pane = mTAScreens[i]->search('BTR1');
#line 845
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            ((J2DPicture *)pane)->setCornerColor(corner);
            ((J2DPicture *)pane)->changeTexture(mRankName[i], 0);
            for (int j = 0; j < 3; j++) {
                J2DPane *pane = mTAScreens[i]->search((u64)'BTN0' + j);
#line 853
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mTALines[i].namePics[j] = (J2DPicture *)pane;
            }
            for (int j = 0; j < 3; j++) {
                mTALines[i].idPics[j] = (J2DPicture *)mTAScreens[i]->search((u64)'BTW0' + j);
            }
            for (int j = 0; j < 3; j++) {
                J2DPane *pane = mTAScreens[i]->search((u64)'BTC0' + j);
#line 863
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                (&mTALines[i].kartPic)[j] = (J2DPicture *)pane;
            }
            for (int j = 0; j < 7; j++) {
                J2DPane *pane = mTAScreens[i]->search((u64)'BTT0' + j);
#line 869
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                mTALines[i].msPics[j] = (J2DPicture *)pane;
            }
        }
        mDrawFunc = &Result2D::drawTA;
        mCalcFunc = &Result2D::setDrawTA;
        mAutoFunc = &Result2D::selectorTA;
        break;
    }
    default:
#line 879
        JUT_ASSERT(0);
        break;
    }

    init();
}

#line 895
void Result2D::init() {
    switch (RaceMgr::getManager()->getRaceMode()) {
    case GRAND_PRIX:
    case AWARD_DEMO:
    case STAFF_ROLL:
        mResultSelector = 5;
        mResultState = 2;
        break;
    case VERSUS_RACE:
    case BALLOON_BATTLE:
    case BOMB_BATTLE:
    case ESCAPE_BATTLE: {
        mResultSelector = 1;
        mResultState = 2;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 14; k++) {
                    mVSBattle[i][j][k].mPane->updateScale(0.0f);
                }
            }
        }
        break;
    }
    case TIME_ATTACK:
        mResultSelector = 1;
        mResultState = 2;
        break;
    default:
        JUT_ASSERT(0);
        break;
    }

    mDrawFlag = RaceMgr::getManager()->isReplayMode() ^ 1;
    mLine._197c = 0;
    mLine._1980 = 0;
    mLine._1984 = 0;
    mLine._1988 = 840.0f;
    mLine._198c = 0;
    mLine._1990 = 0;
    mLine._1994 = 0;
    mLine._1998 = 0;
    for (int i = 0; i < 8; i++) {
        mLine.mLinePoint[i] = SequenceInfo::RANKPOINT[i];
        mLine.mLineYPos[i] = 840.0f;
        for (int j = 0; j < 2; j++) {
            mLine.mLineScale[i][j].set(1.0f, 1.0f);
        }
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            mVSPointYPos[i][j] = -300.0f;
        }
        mVSScale[i] = 0.0f;
    }
    _1ab0 = 0;
    _1ab4 = -1;
    _1ab8 = 0;
    for (int i = 0; i < 4; i++) {
        mMGPointYPos[i] = -300.0f;
        mMGScale[i] = 0.0f;
    }
    mPASSWDAccepted = false;
    mPASSWDEnabled = false;
    mPASSWDCount = 0;
    for (int i = 0; i < 6; i++) {
        mArrowTimer[i] = 0;
    }
    _1af4 = 0;
    _1af8 = 1.0f;
    _1afc = 0;
    _1b00 = 0.0f;
    _1b04 = 0.0f;
    _1b08 = 0.0f;
    mScreen->search(mOverBackTag[0])->setAlpha(0);
    _1744[0].mVal = 0.0f;
    _17ac[0].mVal = 0.0f;
    _1744[1].mVal = 0.0f;
    _17ac[1].mVal = 0.0f;
    for (int i = 0; i < 5; i++) {
        _1754[i].mVal = 0.0f;
        _17bc[i].mVal = 0.0f;
    }
    for (int i = 0; i < 6; i++) {
        _177c[i].mVal = 0.0f;
        _17e4[i].mVal = 0.0f;
    }
    if (RaceMgr::getManager()->getRaceMode() == TIME_ATTACK) {
        calcTA(NULL);
    }
}

void Result2D::draw() {
    struct PauseState {
        u8 _0[0xf];
        bool _f;        // 0xf
        bool mGameOver; // 0x10
    };
    PauseState *pause;
    if ((mNode->mParam.mFlag & 1) && ((pause = (PauseState *)PauseManager::getManager()), !pause->_f)) {
        switch (pause->mGameOver) {
        case false:
            if (mDrawFlag && (RaceMgr::getManager()->isReplayMode() || Goal2D::mDrawEndFlag)) {
                mOrtho->setPort();
                (this->*mDrawFunc)();
            }
        }
    }
}

void Result2D::drawGP() {
    mScreen->draw(0.0f, 0.0f, mOrtho);
    mResultRaceScreen->draw(0.0f, 0.0f, mOrtho);

    switch (mResultState) {
    case 0:
    case 1: {
        for (int i = 0; i < 8; i++) {
            mRankScreens[i]->draw(_3ec[1] + mLine.mLineYPos[i], _3cc[i], mOrtho);
            mGetPointScreens[i]->draw(_c44[mResultState].x + (_3ec[1] + mLine.mLineYPos[i]), _3cc[i] + _c44[mResultState].y, mOrtho);
        }
        break;
    }
    case 2: {
        if (gSequenceInfo.isEndGP()) {
            if (RaceMgr::getManager()->getPlayerNumber() == 1) {
                mSetWord[0][6].mPane->changeTexture("Mozi_Next_Com.bti", 0);
                if (RaceMgr::getManager()->isReplayMode()) {
                    mSetWord[1][6].mPane->changeTexture("Mozi_First.bti", 0);
                } else {
                    mSetWord[1][6].mPane->changeTexture("Mozi_Replay.bti", 0);
                }
                for (int i = 0; i < 2; i++) {
                    mWinScreens[i]->draw(mLine._1988, _dd4 - 16.0f + 32.0f * i, mOrtho);
                }
            }
        } else {
            if (RaceMgr::getManager()->getPlayerNumber() == 1) {
                mSetWord[0][6].mPane->changeTexture("Mozi_Next.bti", 0);
                if (RaceMgr::getManager()->isReplayMode()) {
                    mSetWord[1][6].mPane->changeTexture("Mozi_First.bti", 0);
                } else {
                    mSetWord[1][6].mPane->changeTexture("Mozi_Replay.bti", 0);
                }
                mSetWord[2][6].mPane->changeTexture("Mozi_Quit.bti", 0);
                for (int i = 0; i < 3; i++) {
                    mWinScreens[i]->draw(mLine._1988, _dd4 - 32.0f + 32.0f * i, mOrtho);
                }
            } else {
                mSetWord[0][6].mPane->changeTexture("Mozi_Next.bti", 0);
                mSetWord[1][6].mPane->changeTexture("Mozi_Quit.bti", 0);
                for (int i = 0; i < 2; i++) {
                    mWinScreens[i]->draw(mLine._1988, _dd4 - 16.0f + 32.0f * i, mOrtho);
                }
            }
        }
        break;
    }
    }
}

void Result2D::drawVS() {
    int num = RaceMgr::getManager()->mRaceInfo->getStatusNumber();

    f32 xpos[4];
    f32 ypos[4];
    for (int i = 0; i < num; i++) {
        xpos[i] = 15.0f + 578.0f / num * i + 578.0f / (num * 2);
        ypos[i] = _e58[0] + (i * ((_e58[1] - _e58[0]) / (num - 1)) + (_e58[1] - _e58[0]) / ((num - 1) * 2));
    }

    mScreen->draw(0.0f, 0.0f, mOrtho);

    for (int row = 0; row < 2; row++) {
        for (int i = 0; i < num; i++) {
            mScoreScreens[row][i]->draw(xpos[i], _e68[0], mOrtho);
        }
    }

    for (int row = 0; row < num; row++) {
        mPointScreens2[row]->draw(xpos[row], _e54 + mVSPointYPos[row][0], mOrtho);
        for (int j = 0; j < num - 1; j++) {
            _e1c[row][j]->draw(xpos[row], mVSPointYPos[row][j + 1] + ypos[j], mOrtho);
        }
    }

    mSetWord[0][6].mPane->changeTexture("Mozi_One_More_Race.bti", 0);
    mSetWord[1][6].mPane->changeTexture("Mozi_Change_Course.bti", 0);
    mSetWord[2][6].mPane->changeTexture("Mozi_Change_Character.bti", 0);
    mSetWord[3][6].mPane->changeTexture("Mozi_Quit.bti", 0);

    for (int i = 0; i < 4; i++) {
        mWinScreens[i]->draw(mLine._1988, 304.0f + 32.0f * i, mOrtho);
    }
}

void Result2D::drawTA() {
    mScreen->draw(0.0f, 0.0f, mOrtho);
    mScreenMG->draw(0.0f, 0.0f, mOrtho);

    if (gGhostFile.mGhostType == 3 && !(gSystemRecord.mOptions & 8)) {
        mSetWord[0][6].mPane->changeTexture("Mozi_One_More_Race.bti", 0);
        mSetWord[1][6].mPane->changeTexture("Mozi_Change_Course.bti", 0);
        mSetWord[2][6].mPane->changeTexture("Mozi_Change_Character.bti", 0);
        if (RCMGetManager()->isReplayMode()) {
            mSetWord[3][6].mPane->changeTexture("Mozi_First.bti", 0);
        } else {
            mSetWord[3][6].mPane->changeTexture("Mozi_Replay.bti", 0);
        }
        mSetWord[4][6].mPane->changeTexture("Mozi_SaveGhost.bti", 0);
        mSetWord[5][6].mPane->changeTexture("Mozi_Quit.bti", 0);

        for (int i = 0; i < 6; i++) {
            mWinScreens[i]->draw(mLine._1988, 144.0f + 32.0f * i, mOrtho);
        }
    } else {
        mSetWord[0][6].mPane->changeTexture("Mozi_One_More_Race.bti", 0);
        mSetWord[1][6].mPane->changeTexture("Mozi_Change_Course.bti", 0);
        mSetWord[2][6].mPane->changeTexture("Mozi_Change_Character.bti", 0);
        if (RCMGetManager()->isReplayMode()) {
            mSetWord[3][6].mPane->changeTexture("Mozi_First.bti", 0);
        } else {
            mSetWord[3][6].mPane->changeTexture("Mozi_Replay.bti", 0);
        }
        mSetWord[4][6].mPane->changeTexture("Mozi_Quit.bti", 0);

        for (int i = 0; i < 5; i++) {
            mWinScreens[i]->draw(mLine._1988, 160.0f + 32.0f * i, mOrtho);
        }
    }

    if (mPASSWDAccepted && mPASSWDEnabled) {
        mScreenPw->draw(0.0f, 0.0f, mOrtho);
    }
}

void Result2D::drawMG() {
    int num = RaceMgr::getManager()->mRaceInfo->getStatusNumber();

    float ypos[4];
    for (int i = 0; i < num; i++) {
        ypos[i] = 15.0f + 578.0f / num * i + 578.0f / (num * 2);
    }

    mScreen->draw(0.0f, 0.0f, mOrtho);

    for (int row = 0; row < 2; row++) {
        for (int i = 0; i < num; i++) {
            mScoreScreens[row][i]->draw(ypos[i], _e68[1], mOrtho);
        }
    }
    for (int i = 0; i < num; i++) {
        mPointScreens[i]->draw(ypos[i], _e60[1] + mMGPointYPos[i], mOrtho);
    }

    mSetWord[0][6].mPane->changeTexture("Mozi_One_More_Race.bti", 0);
    mSetWord[1][6].mPane->changeTexture("Mozi_Change_Map.bti", 0);
    mSetWord[2][6].mPane->changeTexture("Mozi_Change_Character.bti", 0);
    mSetWord[3][6].mPane->changeTexture("Mozi_Change_MiniGame.bti", 0);
    mSetWord[4][6].mPane->changeTexture("Mozi_Quit.bti", 0);

    f32 ybase = 288.0f;
    f32 xstep = 32.0f;
    for (int i = 0; i < 5; i++) {
        mWinScreens[i]->draw(mLine._1988, ybase + xstep * i, mOrtho);
    }
}

void Result2D::calc(KartGamePad *pad) {
    if (!mDrawFlag) {
        return;
    }

    if (pad != NULL && mResultState == 2 && mLine._1988 == 0.0f && _1afc == 0) {
        (this->*mAutoFunc)(pad);
    }
    (this->*mCalcFunc)();

    switch (mResultState) {
    case 0:
        if (mLine._197c < 0 || mLine._197c > 0x168) {
            mLine._197c = 0;
        }
        calcRaceRank();
        setRaceRank();
        if (mLine._197c == 0xd7) {
            mLine._197c--;
        }
        if (pad->testTrigger(JUTGamePad::A)) {
            if (mLine._197c >= 0xcd && mLine._197c < 0x127) {
                mLine._197c = 0x127;
            }
        }
        if (mLine._197c == 0x168) {
            mResultState = 1;
            mLine._197c = -1;
        }
        break;
    case 1:
        if (mLine._197c < 0 || mLine._197c > 0xe6) {
            mLine._197c = 0;
        }
        calcGPRank();
        setGPRank();
        if (mLine._197c == 0x55) {
            mLine._197c--;
        }
        if (pad->testTrigger(JUTGamePad::A)) {
            if (mLine._197c >= 0x4b && mLine._197c < 0xa5) {
                mLine._197c = 0xa5;
            }
        }
        if (mLine._197c == 0xe6) {
            mResultState = 3;
            mLine._197c = -1;
        }
        break;
    case 2:
        switch (RCMGetManager()->mRaceInfo->getRaceMode()) {
        case GRAND_PRIX:
        case AWARD_DEMO:
        case STAFF_ROLL:
            calcResult();
            break;
        case VERSUS_RACE:
            calcVS();
            setVS();
            break;
        case TIME_ATTACK:
            if (mLine._197c < 0) {
                mLine._197c = 0;
            }
            if (mLine._197c == 0xb4) {
                mLine._197c--;
            }
            if (pad->testTrigger(JUTGamePad::A)) {
                if (mLine._197c >= 0x5a && mLine._197c < 0xb4) {
                    mLine._197c = 0xb4;
                }
            }
            if (mLine._197c == 0xf0) {
                CardAgent::ask(CardAgent::mcCommand3, 0);
            }
            if (mLine._197c == 0xf1) {
                if (CardAgent::msState == 0) {
                    CardAgent::getPrintMemoryCard()->setStuff(0, 0, 0);
                } else {
                    mLine._197c--;
                }
            }
            calcTA(pad);
            break;
        case BALLOON_BATTLE:
        case BOMB_BATTLE:
        case ESCAPE_BATTLE:
            calcMG();
            setMG();
            break;
        default:
            break;
        }
        break;
    default:
        mResultState = 2;
        mLine._197c = -1;
        if (gSequenceInfo.isEndGP() && RCMGetManager()->mRaceInfo->getPlayerKartNumber() == 2) {
            struct PauseState {
                u8 _0[0xf];
                u8 _f;
            };
            ((PauseState *)PauseManager::getManager())->_f = 1;
        }
        break;
    }
    mLine._197c++;
    if (mLine._1980 != 0) {
        mLine._1980++;
    }
    int c90 = mLine._1990 + 1;
    mLine._1990 = c90;
    if (c90 >= 50) {
        mLine._1990 = 0;
    }
    int c94 = mLine._1994 + 1;
    mLine._1994 = c94;
    if (c94 >= 40) {
        mLine._1994 = 0;
    }
    int c98 = mLine._1998 + 1;
    mLine._1998 = c98;
    if (c98 >= 60) {
        mLine._1998 = 0;
    }
    mLine._1984 += 10;
    if (mLine._1984 >= 128) {
        mLine._1984 = 128;
    }
    if (mLine._1980 != 0) {
        mLine._1984 -= (mLine._1980 - 1) * 10;
        if (mLine._1984 < 0) {
            mLine._1984 = 0;
        }
    }

    J2DPane *overBack = mScreen->search(mOverBackTag[0]);
    overBack->setAlpha((u8)mLine._1984);

    if (mLine._1988 == 0.0f) {
        int anm = mLine._198c;
        mLine._198c = anm + 1;
        if (anm >= 30) {
            mLine._198c = 30;
        }
    }
    if (_1afc < 8) {
        _1b04 = _1b00 + _1afc * (_1b08 - _1b00) / 8.0f;
    } else {
        _1b04 = _1b00 = _1b08;
    }
    if (_1afc != 0) {
        if (_1afc < 8) {
            _1afc++;
        } else {
            _1afc = 0;
        }
    }
    _1af4++;
    _1af8 = getScale(_1af4);
}

void Result2D::calcRaceRank() {
    int frame = mLine._197c;
    getRaceRankLayoutAlpha(frame, mLayoutAlpha);
    for (int rank = 0; rank < 8; rank++) {
        getRaceRankLinePos(frame, rank, mLine.mLineYPos[rank]);
        getRaceRankLineScale(frame, rank, mLine.mLineScale[rank][0], mLine.mLineScale[rank][1]);
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 20; j++) {
            JGeometry::TVec2f a = mLine.mLineScale[i][0];
            J2DPane *pane = mLines[i][j].mPane;
            pane->updateScale(a.x, a.y);
        }
    }
    for (int i = 0; i < 8; i++) {
        mRankRows[i][0].mPane->updateScale(mLine.mLineScale[i][1].x, mLine.mLineScale[i][1].y);
        for (int j = 0; j < 3; j++) {
            mRankRows[i][j + 1].mPane->updateScale(mLine.mLineScale[i][0].x, mLine.mLineScale[i][0].y);
        }
    }
    int f = frame - 0x4b;
    if (f >= 0 && f < 0x64 && f % 10 == 9) {
        GameAudio::Main::getAudio()->startSystemSe(0x20033);
        for (int i = 0; i < 8; i++) {
            if (mLine.mLinePoint[i] > 0) {
                mLine.mLinePoint[i]--;
            }
        }
    }
}

void Result2D::calcGPRank() {
    int frame = mLine._197c;
    getGPRankLayoutAlpha(frame, mLayoutAlpha);
    for (int rank = 0; rank < 8; rank++) {
        getGPRankLinePos(frame, rank, mLine.mLineYPos[rank]);
        getGPRankLineScale(frame, rank, mLine.mLineScale[rank][0], mLine.mLineScale[rank][1]);
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 20; j++) {
            JGeometry::TVec2f a = mLine.mLineScale[i][0];
            J2DPane *pane = mLines[i][j].mPane;
            pane->updateScale(a.x, a.y);
        }
    }
    for (int i = 0; i < 8; i++) {
        mRankRows[i][0].mPane->updateScale(mLine.mLineScale[i][1].x, mLine.mLineScale[i][1].y);
        for (int j = 0; j < 3; j++) {
            mRankRows[i][j + 1].mPane->updateScale(mLine.mLineScale[i][0].x, mLine.mLineScale[i][0].y);
        }
        mLines[i][19].mPane->updateScale(mLine.mLineScale[i][0].x, mLine.mLineScale[i][0].y);
    }
}

void Result2D::calcResult() {
    getResultLinePos(mLine._197c, mLine._1988);
}

void Result2D::calcVS() {
    int frame = mLine._197c;
    if (frame == 0xf) {
        GameAudio::Main::getAudio()->startSystemSe(0x2005d);
    }
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            getVSPointYPos(frame, j, mVSPointYPos[i][j]);
        }
        getVSScale(frame, mVSScale[i]);
    }
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 14; k++) {
                f32 s = mVSScale[i];
                J2DPane *pane = mVSBattle[i][j][k].mPane;
                pane->mScale.set(s, s);
                pane->calcMtx();
            }
        }
    }
    getResultLinePos(frame, mLine._1988);
}

void Result2D::calcTA(KartGamePad *pad) {
    getResultLinePos(mLine._197c, mLine._1988);
    if (_1ab0 || _1ab8) {
        if (mLine._197c < 60) {
            _1744[0].mVal = (f32)mLine._197c;
        } else if (mLine._197c < 180) {
            _1744[0].mVal = 70.0f;
        } else if (mLine._197c < 240) {
            _1744[0].mVal = (f32)(mLine._197c - 80);
        } else {
            _1744[0].mVal = 170.0f;
        }
        if ((_17ac[0].mVal += 1.0f) >= 40.0f) {
            _17ac[0].mVal = 0.0f;
        }
        if (_1ab8) {
            if ((_1744[1].mVal += 1.0f) >= 41.0f) {
                _1744[1].mVal = 1.0f;
            }
            if ((_17ac[1].mVal += 1.0f) >= 41.0f) {
                _17ac[1].mVal = 1.0f;
            }
        } else {
            _1744[1].mVal = 0.0f;
            _17ac[1].mVal = 0.0f;
        }
        for (int i = 0; i < 5; i++) {
            if (_1ab0 && i == _1ab4) {
                if ((_1754[i].mVal += 1.0f) >= 41.0f) {
                    _1754[i].mVal = 1.0f;
                }
                if ((_17bc[i].mVal += 1.0f) >= 41.0f) {
                    _17bc[i].mVal = 1.0f;
                }
            } else {
                _1754[i].mVal = 0.0f;
                _17bc[i].mVal = 0.0f;
            }
        }
        for (int i = 0; i < 6; i++) {
            if ((_177c[i].mVal += 1.0f) >= 160.0f) {
                _177c[i].mVal = 0.0f;
            }
            if (mLine._197c < 60) {
                _17e4[i].mVal = 0.0f;
            } else if (mLine._197c < 180) {
                if ((_17e4[i].mVal += 1.0f) > 10.0f) {
                    _17e4[i].mVal = 10.0f;
                }
            } else if (mLine._197c < 240) {
                if ((_17e4[i].mVal -= 1.0f) < 0.0f) {
                    _17e4[i].mVal = 0.0f;
                }
            }
        }
        for (int i = 0; i < 5; i++) {
            if (_1ab0 && i == _1ab4) {
                mTAScreens[5 + i]->mIsVisible = true;
            } else {
                mTAScreens[5 + i]->mIsVisible = false;
            }
        }
        if (_1ab8) {
            mTAScreens[10]->mIsVisible = true;
        } else {
            mTAScreens[10]->mIsVisible = false;
        }
    }

    _1744[0].mAnm->setFrame(_1744[0].mVal);
    _17ac[0].mAnm->setFrame(_17ac[0].mVal);
    _1744[1].mAnm->setFrame(_1744[1].mVal);
    _17ac[1].mAnm->setFrame(_17ac[1].mVal);
    for (int i = 0; i < 5; i++) {
        _1754[i].mAnm->setFrame(_1754[i].mVal);
        _17bc[i].mAnm->setFrame(_17bc[i].mVal);
    }
    for (int i = 0; i < 6; i++) {
        _177c[i].mAnm->setFrame(_177c[i].mVal);
        _17e4[i].mAnm->setFrame(_17e4[i].mVal);
    }

    mScreenMG->animation();
    mScreenMG2->animation();
    for (int i = 0; i < 5; i++) {
        mTAScreens[i]->animation();
    }
    for (int i = 0; i < 6; i++) {
        mTAScreens[5 + i]->animation();
    }

    switch ((int)_1744[0].mVal) {
    case 30:
        GameAudio::Main::getAudio()->startSystemSe(0x2003b);
        break;
    case 34:
        GameAudio::Main::getAudio()->startSystemSe(0x2003a);
        break;
    case 38:
        GameAudio::Main::getAudio()->startSystemSe(0x20039);
        break;
    case 42:
        GameAudio::Main::getAudio()->startSystemSe(0x20038);
        break;
    case 46:
        GameAudio::Main::getAudio()->startSystemSe(0x20037);
        break;
    case 50:
        GameAudio::Main::getAudio()->startSystemSe(0x20036);
        break;
    case 100:
        GameAudio::Main::getAudio()->startSystemSe(0x20043);
        break;
    case 104:
        GameAudio::Main::getAudio()->startSystemSe(0x20042);
        break;
    case 108:
        GameAudio::Main::getAudio()->startSystemSe(0x20041);
        break;
    case 112:
        GameAudio::Main::getAudio()->startSystemSe(0x20040);
        break;
    case 116:
        GameAudio::Main::getAudio()->startSystemSe(0x2003f);
        break;
    case 120:
        GameAudio::Main::getAudio()->startSystemSe(0x2003e);
        break;
    }
    calcPASSWD(pad);
}

void Result2D::calcMG() {
    int frame = mLine._197c;
    if (frame == 0xf) {
        GameAudio::Main::getAudio()->startSystemSe(0x2005d);
    }
    for (int i = 0; i < 4; i++) {
        getMGPointYPos(frame, mMGPointYPos[i]);
        getMGScale(frame, mMGScale[i]);
    }
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 14; k++) {
                f32 s = mMGScale[i];
                J2DPane *pane = mVSBattle[i][j][k].mPane;
                pane->mScale.set(s, s);
                pane->calcMtx();
            }
        }
    }
    getResultLinePos(mLine._197c, mLine._1988);
}

void Result2D::calcPASSWD(KartGamePad *pad) {
    if (pad != NULL && getAnmEnd() && mPASSWDEnabled && !mPASSWDAccepted) {
        u16 code[9] = { 0x40, 0x20, 0x40, 0x20, 0x400, 0x800, 0x400, 0x800, 0x10 };
        u32 trigger = pad->getTrigger();
        if (trigger != 0) {
            if (trigger == code[mPASSWDCount]) {
                if (++mPASSWDCount >= 9) {
                    GameAudio::Main::getAudio()->startSystemSe(0x2005d);
                    mPASSWDAccepted = true;
                    mPASSWDCount = 0;
                }
            } else if (trigger == 0x40) {
                mPASSWDCount = 1;
            } else {
                mPASSWDCount = 0;
            }
        }
    }
}

#line 1756
void Result2D::selectorGP(KartGamePad *pad) {
    if (gSequenceInfo.isEndGP()) {
        if (RCMGetManager()->mRaceInfo->getPlayerKartNumber() == 1) {
            if (pad->testRepeat(JUTGamePad::MAINSTICK_UP)) {
                switch (mResultSelector) {
                case 5:
                    mResultSelector = 7;
                    break;
                case 7:
                    mResultSelector = 5;
                    break;
                default:
                    JUT_ASSERT(0);
                    break;
                }
                selectorCommon();
            }
            if (pad->testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
                switch (mResultSelector) {
                case 5:
                    mResultSelector = 7;
                    break;
                case 7:
                    mResultSelector = 5;
                    break;
                default:
                    JUT_ASSERT(0);
                    break;
                }
                selectorCommon();
            }
        }
    } else {

        if (RCMGetManager()->mRaceInfo->getPlayerKartNumber() == 1) {
            if (pad->testRepeat(JUTGamePad::MAINSTICK_UP)) {
                switch (mResultSelector) {
                case 5:
                    mResultSelector = 8;
                    break;
                case 7:
                    mResultSelector = 5;
                    break;
                case 8:
                    mResultSelector = 7;
                    break;
                default:
                    JUT_ASSERT(0);
                    break;
                }
                selectorCommon();
            }
            if (pad->testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
                switch (mResultSelector) {
                case 5:
                    mResultSelector = 7;
                    break;
                case 7:
                    mResultSelector = 8;
                    break;
                case 8:
                    mResultSelector = 5;
                    break;
                default:
                    JUT_ASSERT(0);
                    break;
                }
                selectorCommon();
            }
        } else {
            if (pad->testRepeat(JUTGamePad::MAINSTICK_UP)) {
                switch (mResultSelector) {
                case 5:
                    mResultSelector = 8;
                    break;
                case 8:
                    mResultSelector = 5;
                    break;
                default:
                    JUT_ASSERT(0);
                    break;
                }
                selectorCommon();
            }
            if (pad->testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
                switch (mResultSelector) {
                case 5:
                    mResultSelector = 8;
                    break;
                case 8:
                    mResultSelector = 5;
                    break;
                default:
                    JUT_ASSERT(0);
                    break;
                }
                selectorCommon();
            }
        }
    }
}

#line 1865
void Result2D::selectorVS(KartGamePad *pad) {
    if (pad->testRepeat(JUTGamePad::MAINSTICK_UP)) {
        switch (mResultSelector) {
        case 1:
            mResultSelector = 8;
            break;
        case 2:
            mResultSelector = 1;
            break;
        case 4:
            mResultSelector = 2;
            break;
        case 8:
            mResultSelector = 4;
            break;
        default:
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    }
    if (pad->testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
        switch (mResultSelector) {
        case 1:
            mResultSelector = 2;
            break;
        case 2:
            mResultSelector = 4;
            break;
        case 4:
            mResultSelector = 8;
            break;
        case 8:
            mResultSelector = 1;
            break;
        default:
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    }
}

#line 1917
void Result2D::selectorTA(KartGamePad *pad) {
    if (gGhostFile.mGhostType == 3 && !(gSystemRecord.mOptions & 8)) {
        if (pad->testRepeat(JUTGamePad::MAINSTICK_UP)) {
            switch (mResultSelector) {
            case 1:
                mResultSelector = 8;
                break;
            case 2:
                mResultSelector = 1;
                break;
            case 4:
                mResultSelector = 2;
                break;
            case 7:
                mResultSelector = 4;
                break;
            case 10:
                mResultSelector = 7;
                break;
            case 8:
                mResultSelector = 10;
                break;
            default:
                JUT_ASSERT(0);
                break;
            }
            selectorCommon();
        }
        if (pad->testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
            switch (mResultSelector) {
            case 1:
                mResultSelector = 2;
                break;
            case 2:
                mResultSelector = 4;
                break;
            case 4:
                mResultSelector = 7;
                break;
            case 7:
                mResultSelector = 10;
                break;
            case 10:
                mResultSelector = 8;
                break;
            case 8:
                mResultSelector = 1;
                break;
            default:
                JUT_ASSERT(0);
                break;
            }
            selectorCommon();
        }
    } else {
        if (pad->testRepeat(JUTGamePad::MAINSTICK_UP)) {
            switch (mResultSelector) {
            case 1:
                mResultSelector = 8;
                break;
            case 2:
                mResultSelector = 1;
                break;
            case 4:
                mResultSelector = 2;
                break;
            case 7:
                mResultSelector = 4;
                break;
            case 8:
                mResultSelector = 7;
                break;
            default:
                JUT_ASSERT(0);
                break;
            }
            selectorCommon();
        }
        if (pad->testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
            switch (mResultSelector) {
            case 1:
                mResultSelector = 2;
                break;
            case 2:
                mResultSelector = 4;
                break;
            case 4:
                mResultSelector = 7;
                break;
            case 7:
                mResultSelector = 8;
                break;
            case 8:
                mResultSelector = 1;
                break;
            default:
                JUT_ASSERT(0);
                break;
            }
            selectorCommon();
        }
    }
}

#line 2029
void Result2D::selectorMG(KartGamePad *pad) {
    if (pad->testRepeat(JUTGamePad::MAINSTICK_UP)) {
        switch (mResultSelector) {
        case 1:
            mResultSelector = 8;
            break;
        case 3:
            mResultSelector = 1;
            break;
        case 4:
            mResultSelector = 3;
            break;
        case 6:
            mResultSelector = 4;
            break;
        case 8:
            mResultSelector = 6;
            break;
        default:
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    }
    if (pad->testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
        switch (mResultSelector) {
        case 1:
            mResultSelector = 3;
            break;
        case 3:
            mResultSelector = 4;
            break;
        case 4:
            mResultSelector = 6;
            break;
        case 6:
            mResultSelector = 8;
            break;
        case 8:
            mResultSelector = 1;
            break;
        default:
            JUT_ASSERT(0);
            break;
        }
        selectorCommon();
    }
}

void Result2D::selectorCommon() {
    GameAudio::Main::getAudio()->startSystemSe(0x20000);
    _1af4 = 0;
    _1afc = 1;
}

#line 2101
void Result2D::setDrawGP() {
    if (gSequenceInfo.isEndGP()) {
        if (RCMGetManager()->mRaceInfo->getPlayerKartNumber() == 1) {
            switch (mResultSelector) {
            case 5:
                setScale(0);
                break;
            case 7:
                setScale(1);
                break;
            default:
                JUT_ASSERT(0);
                break;
            }
        }

    } else {
        if (RCMGetManager()->mRaceInfo->getPlayerKartNumber() == 1) {
            switch (mResultSelector) {
            case 5:
                setScale(0);
                break;
            case 7:
                setScale(1);
                break;
            case 8:
                setScale(2);
                break;
            default:
                JUT_ASSERT(0);
                break;
            }
        } else {
            switch (mResultSelector) {
            case 5:
                setScale(0);
                break;
            case 8:
                setScale(1);
                break;
            default:
                JUT_ASSERT(0);
                break;
            }
        }
    }
}

#line 2156
void Result2D::setDrawVS() {
    switch (mResultSelector) {
    case 1:
        setScale(0);
        break;
    case 2:
        setScale(1);
        break;
    case 4:
        setScale(2);
        break;
    case 8:
        setScale(3);
        break;
    default:
        JUT_ASSERT(0);
        break;
    }
}

#line 2185
void Result2D::setDrawTA() {
    if (gGhostFile.mGhostType == 3 && !(gSystemRecord.mOptions & 8)) {
        switch (mResultSelector) {
        case 1:
            setScale(0);
            break;
        case 2:
            setScale(1);
            break;
        case 4:
            setScale(2);
            break;
        case 7:
            setScale(3);
            break;
        case 10:
            setScale(4);
            break;
        case 8:
            setScale(5);
            break;
        default:
            JUT_ASSERT(0);
            break;
        }
    } else {
        switch (mResultSelector) {
        case 1:
            setScale(0);
            break;
        case 2:
            setScale(1);
            break;
        case 4:
            setScale(2);
            break;
        case 7:
            setScale(3);
            break;
        case 8:
            setScale(4);
            break;
        default:
            JUT_ASSERT(0);
            break;
        }
    }
}

#line 2242
void Result2D::setDrawMG() {
    switch (mResultSelector) {
    case 1:
        setScale(0);
        break;
    case 3:
        setScale(1);
        break;
    case 4:
        setScale(2);
        break;
    case 6:
        setScale(3);
        break;
    case 8:
        setScale(4);
        break;
    default:
        JUT_ASSERT(0);
        break;
    }
}

int Result2D::getResultSelector() { return mResultSelector; }

void Result2D::setRaceRank() {
    for (int i = 0; i < 11; i++) {
        mLayout[i].mPane->setAlpha(mLayoutAlpha);
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 8; j < 19; j++) {
            mLines[i][j].mPane->mIsVisible = true;
        }
    }

    for (int i = 0; i < 8; i++) {
        int kartNo = gSequenceInfo.get_2b8(i);
        if (kartNo < RaceMgr::getManager()->getPlayerNumber()) {
            int player = -1;
            if (kartNo == J2DManager::getStatus2Kart(0)) {
                player = 0;
            } else if (kartNo == J2DManager::getStatus2Kart(1)) {
                if (gRaceInfo.getKartInfo(0)->isTagMode()) {
                    player = 2;
                } else {
                    player = 1;
                }
            }
            JUTColor color;
            u8 alpha;
            getFrameClr(player, color, alpha);
            mLines[i][0].mPane->mIsVisible = true;
            mLines[i][1].mPane->mIsVisible = true;
            mLines[i][2].mPane->mIsVisible = true;
            mLines[i][0].mPane->setAlpha(alpha);
            mLines[i][1].mPane->setAlpha(alpha);
            mLines[i][2].mPane->setAlpha(alpha);
            mLines[i][0].mPane->setWhite(mWhiteColor[player][0]);
            mLines[i][1].mPane->setWhite(mWhiteColor[player][0]);
            mLines[i][2].mPane->setWhite(mWhiteColor[player][0]);
            mLines[i][5].mPane->setWhite(color);
            mLines[i][3].mPane->setWhite(color);
        } else {
            mLines[i][0].mPane->mIsVisible = false;
            mLines[i][1].mPane->mIsVisible = false;
            mLines[i][2].mPane->mIsVisible = false;
            mLines[i][5].mPane->setWhite(mWhiteColor[3][0]);
            mLines[i][3].mPane->setWhite(mWhiteColor[3][0]);
        }

        int charID = gRaceInfo.getKartInfo(kartNo)->getDriverCharID(0);
        if (charID > 0 && charID < 21) {
            mLines[i][6].mPane->changeTexture(Race2D::mCharacterName[charID - 1], 0);
        }
        charID = gRaceInfo.getKartInfo(kartNo)->getDriverCharID(1);
        if (charID > 0 && charID < 21) {
            mLines[i][4].mPane->changeTexture(Race2D::mCharacterName[charID - 1], 0);
        }

        for (int j = 0; j < 8; j++) {
            if (gSequenceInfo.getCourse() == 1) {
                mLines[j][19].mPane->mIsVisible = false;
            } else {
                mLines[j][19].mPane->mIsVisible = true;
                mLines[j][19].mPane->updateScale(0.0f);
                int prev = gSequenceInfo.getPrevRank(j);
                int icon;
                if (j < prev) {
                    icon = 0;
                } else if (j > prev) {
                    icon = 2;
                } else {
                    icon = 1;
                }
                mLines[j][19].mPane->changeTexture(mRankIconPics[icon], 0);
            }
        }

        const RaceTime &time = RaceMgr::getManager()->getKartChecker(kartNo)->getTotalTime();
        if (time.isAvailable()) {
            int min, sec, ms;
            time.get(&min, &sec, &ms);
            mLines[i][8].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(ms % 10)), 0);
            mLines[i][9].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((ms / 10) % 10)), 0);
            mLines[i][10].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((ms / 100) % 10)), 0);
            mLines[i][11].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(sec % 10)), 0);
            mLines[i][12].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((sec / 10) % 10)), 0);
            mLines[i][13].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(min % 10)), 0);
            mLines[i][14].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((min / 10) % 10)), 0);
        } else {
            ResTIMG *dash = Kart2DCommon::ptr()->getAsciiTexture('-');
            mLines[i][8].mPane->changeTexture(dash, 0);
            mLines[i][9].mPane->changeTexture(dash, 0);
            mLines[i][10].mPane->changeTexture(dash, 0);
            mLines[i][11].mPane->changeTexture(dash, 0);
            mLines[i][12].mPane->changeTexture(dash, 0);
            mLines[i][13].mPane->changeTexture(dash, 0);
            mLines[i][14].mPane->changeTexture(dash, 0);
        }

        int point = gSequenceInfo.getRankPoint(kartNo) - mLine.mLinePoint[i];
        int hund = point / 100;
        int ones = point % 10;
        int tens = (point - hund * 100) / 10;

        if (hund != 0) {
            if (hund >= 0 && hund < 10) {
                mRankRows[i][3].mPane->mIsVisible = true;
                mRankRows[i][3].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)hund), 0);
            }
        } else {
            mRankRows[i][3].mPane->mIsVisible = false;
        }
        if (tens != 0) {
            if (tens >= 0 && tens < 10) {
                mRankRows[i][2].mPane->mIsVisible = true;
                mRankRows[i][2].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)tens), 0);
            }
        } else if (hund != 0) {
            mRankRows[i][2].mPane->mIsVisible = true;
            mRankRows[i][2].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)tens), 0);
        } else {
            mRankRows[i][2].mPane->mIsVisible = false;
        }
        if (ones >= 0 && ones < 10) {
            mRankRows[i][1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)ones), 0);
        }
    }
}

void Result2D::setGPRank() {
    int j;
    for (int i = 0; i < 11; i++) {
        mLayout[i].mPane->setAlpha(mLayoutAlpha);
    }
    for (int i = 0; i < 8; i++) {
        for (j = 8; j < 19; j++) {
            mLines[i][j].mPane->mIsVisible = false;
        }
    }

    for (int i = 0; i < 8; i++) {
        int kartNo = gSequenceInfo.get_2d8(i);
        if (kartNo < RaceMgr::getManager()->getPlayerNumber()) {
            int player = -1;
            if (kartNo == J2DManager::getStatus2Kart(0)) {
                player = 0;
            } else if (kartNo == J2DManager::getStatus2Kart(1)) {
                if (gRaceInfo.getKartInfo(0)->isTagMode()) {
                    player = 2;
                } else {
                    player = 1;
                }
            }
            JUTColor color;
            u8 alpha;
            getFrameClr(player, color, alpha);
            mLines[i][0].mPane->mIsVisible = true;
            mLines[i][1].mPane->mIsVisible = true;
            mLines[i][2].mPane->mIsVisible = true;
            mLines[i][0].mPane->setAlpha(alpha);
            mLines[i][1].mPane->setAlpha(alpha);
            mLines[i][2].mPane->setAlpha(alpha);
            mLines[i][0].mPane->setWhite(mWhiteColor[player][0]);
            mLines[i][1].mPane->setWhite(mWhiteColor[player][0]);
            mLines[i][2].mPane->setWhite(mWhiteColor[player][0]);
            mLines[i][5].mPane->setWhite(color);
            mLines[i][3].mPane->setWhite(color);
        } else {
            mLines[i][0].mPane->mIsVisible = false;
            mLines[i][1].mPane->mIsVisible = false;
            mLines[i][2].mPane->mIsVisible = false;
            mLines[i][5].mPane->setWhite(mWhiteColor[3][0]);
            mLines[i][3].mPane->setWhite(mWhiteColor[3][0]);
        }

        int charID = gRaceInfo.getKartInfo(kartNo)->getDriverCharID(0);
        if (charID > 0 && charID < 21) {
            mLines[i][6].mPane->changeTexture(Race2D::mCharacterName[charID - 1], 0);
        }
        charID = gRaceInfo.getKartInfo(kartNo)->getDriverCharID(1);
        if (charID > 0 && charID < 21) {
            mLines[i][4].mPane->changeTexture(Race2D::mCharacterName[charID - 1], 0);
        }

        int point = gSequenceInfo.getRankPoint(kartNo);
        int hund = point / 100;
        int ones = point % 10;
        int tens = (point - hund * 100) / 10;

        if (hund != 0) {
            if (hund >= 0 && hund < 10) {
                mRankRows[i][3].mPane->mIsVisible = true;
                mRankRows[i][3].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)hund), 0);
            }
        } else {
            mRankRows[i][3].mPane->mIsVisible = false;
        }
        if (tens != 0) {
            if (tens >= 0 && tens < 10) {
                mRankRows[i][2].mPane->mIsVisible = true;
                mRankRows[i][2].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)tens), 0);
            }
        } else if (hund != 0) {
            mRankRows[i][2].mPane->mIsVisible = true;
            mRankRows[i][2].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)tens), 0);
        } else {
            mRankRows[i][2].mPane->mIsVisible = false;
        }
        if (ones >= 0 && ones < 10) {
            mRankRows[i][1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)ones), 0);
        }
    }
}

void Result2D::setVS() {
    int num = RaceMgr::getManager()->getStatusNumber();
    setBattleCharacter();

    for (int i = 0; i < num; i++) {
        int point = gSequenceInfo.get_40(i, 0);
        _1410[i][0].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(point % 10)), 0);
        _1410[i][1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(point / 10)), 0);

        if (gSequenceInfo.getStartNo(J2DManager::getStatus2Kart(i)) == 0) {
            JUTColor color;
            getVSColor(i, color);
            _1410[i][0].mPane->setBlack(color);
            _1410[i][1].mPane->setBlack(color);
            getFrameColor(color);
            mVSBattle[0][i][9].mPane->setBlack(color);
        } else {
            _1410[i][0].mPane->setBlack(JUTColor(0));
            _1410[i][1].mPane->setBlack(JUTColor(0));
        }

        for (int j = 0; j < num - 1; j++) {
            int p = gSequenceInfo.get_40(i, j + 1);
            _14d0[i][j][0].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(p % 10)), 0);
            _14d0[i][j][1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(p / 10)), 0);
        }
    }
}

void Result2D::setTA() {
    int kartNo = J2DManager::getStatus2Kart(0);
    ECharID charID0 = gRaceInfo.getKartInfo(kartNo)->getDriverCharID(0);
    ECharID charID1 = gRaceInfo.getKartInfo(kartNo)->getDriverCharID(1);
    EKartID kartID = gRaceInfo.getKartInfo(kartNo)->getKartID();
    ECourseID course = ResMgr::getCourseID();

    TARecord rec((u8)charID0, (u8)charID1, (u8)kartID, (u8)course);
    rec.setName(&gSystemRecord.mDefaultName[0]);
    rec.setRaceTime(RaceMgr::getManager()->getKartChecker(kartNo)->getTotalTime());
    TARecord lap((u8)charID0, (u8)charID1, (u8)kartID, (u8)course);
    lap.setName(&gSystemRecord.mDefaultName[0]);
    lap.setRaceTime(*RaceMgr::getManager()->getKartChecker(kartNo)->getBestLapTime());

    if (RaceMgr::getManager()->getKartChecker(kartNo)->isBestTotalTimeRenewal(4)) {
        int rank = gSystemRecord.rankTARecord(course, rec);
        if (rank >= 1 && rank <= 5) {
            _1ab0 = 1;
            _1ab4 = rank - 1;
        }
    }

    TARecord bestLap = *gSystemRecord.getBestLap(course);
    if (bestLap.isValid()) {
        if (bestLap.less(lap)) {
            gSystemRecord.setBestLap(course, lap);
            _1ab8 = 1;
        }
    } else if (RaceMgr::getManager()->getKartChecker(kartNo)->getBestLapTime()->isAvailable()) {
        gSystemRecord.setBestLap(course, lap);
        _1ab8 = 1;
    }

    if (_1ab0 || _1ab8) {
        int min, sec, ms;
        TARecord r;
        Kart2DCommon *k2d = Kart2DCommon::ptr();
        for (int i = 0; i < 5; i++) {
            r = *gSystemRecord.getTARecord(course, i);
            if (r.isValid()) {
                k2d->changeStringTexture(r.getName(), mTALines[i].namePics, 3);
                for (int j = 0; j < 3; j++) {
                    mTALines[i].idPics[j]->mIsVisible = true;
                }
                mTALines[i].charPic2->changeTexture(k2d->getCharIconTextureChecked(r.getCharID(0)), 0);
                mTALines[i].charPic1->changeTexture(k2d->getCharIconTextureChecked(r.getCharID(1)), 0);
                mTALines[i].kartPic->changeTexture(k2d->getKartIconTexture(r.getKartID()), 0);
                if (r.getRaceTime().isAvailable()) {
                    r.getRaceTime().get(&min, &sec, &ms);
                    k2d->changeNumberTexture(min, mTALines[i].minPics, 2, true, false);
                    k2d->changeNumberTexture(sec, mTALines[i].secPics, 2, true, false);
                    k2d->changeNumberTexture(ms, mTALines[i].msPics, 3, true, false);
                } else {
                    k2d->changeNumberTexture(0, mTALines[i].minPics, 2, true, true);
                    k2d->changeNumberTexture(0, mTALines[i].secPics, 2, true, true);
                    k2d->changeNumberTexture(0, mTALines[i].msPics, 3, true, true);
                }
            } else {
                k2d->changeStringTexture("---", mTALines[i].namePics, 3);
                for (int j = 0; j < 3; j++) {
                    mTALines[i].idPics[j]->mIsVisible = false;
                }
                k2d->changeNumberTexture(0, mTALines[i].minPics, 2, true, true);
                k2d->changeNumberTexture(0, mTALines[i].secPics, 2, true, true);
                k2d->changeNumberTexture(0, mTALines[i].msPics, 3, true, true);
            }
        }

        r = *gSystemRecord.getBestLap(course);
        if (r.isValid()) {
            k2d->changeStringTexture(r.getName(), mTABest.namePics, 3);
            if (r.getRaceTime().isAvailable()) {
                r.getRaceTime().get(&min, &sec, &ms);
                k2d->changeNumberTexture(min, mTABest.minPics, 2, true, false);
                k2d->changeNumberTexture(sec, mTABest.secPics, 2, true, false);
                k2d->changeNumberTexture(ms, mTABest.msPics, 3, true, false);
            } else {
                k2d->changeNumberTexture(0, mTABest.minPics, 2, true, true);
                k2d->changeNumberTexture(0, mTABest.secPics, 2, true, true);
                k2d->changeNumberTexture(0, mTABest.msPics, 3, true, true);
            }
        } else {
            k2d->changeStringTexture("---", mTABest.namePics, 3);
            k2d->changeNumberTexture(0, mTABest.minPics, 2, true, true);
            k2d->changeNumberTexture(0, mTABest.secPics, 2, true, true);
            k2d->changeNumberTexture(0, mTABest.msPics, 3, true, true);
        }
    } else {
        mLine._197c = 242;
    }

    Password pw;
    mPASSWDEnabled = pw.make(kartID, charID0, charID1, course, rec.getRaceTime(), lap.getRaceTime());
    if (mPASSWDEnabled) {
        for (int i = 0; i < 16; i++) {
            char buf[2];
            snprintf(buf, 2, "%c", pw.getPassAtIndex(i));
            strncpy(mPassWDText[i]->getStringPtr(), buf, 2);
        }
    }
}

void Result2D::setMG() {
    int num = RaceMgr::getManager()->getStatusNumber();
    setBattleCharacter();

    for (int i = 0; i < num; i++) {
        int point = gSequenceInfo.get_80(i);
        int digit = point % 10;
        mPointRows[i][0].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)digit), 0);
        mPointRows[i][1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(point / 10)), 0);

        int kartNo = J2DManager::getStatus2Kart(i);
        if (gSequenceInfo.getStartNo(kartNo) == 0) {
            JUTColor color;
            getMGColor(i, color);
            mPointRows[i][0].mPane->setBlack(color);
            mPointRows[i][1].mPane->setBlack(color);
            getFrameColor(color);
            mVSBattle[0][i][12].mPane->setBlack(color);
        } else {
            mPointRows[i][0].mPane->setBlack(JUTColor(0));
            mPointRows[i][1].mPane->setBlack(JUTColor(0));
        }
    }
}

void Result2D::setBattleCharacter() {
    int num = RaceMgr::getManager()->getStatusNumber();
    int kartNo = -1;

    for (int i = 0; i < num; i++) {
        if (gSequenceInfo.getStartNo(J2DManager::getStatus2Kart(i)) == 0) {
            mScoreScreens[0][i]->show();
            mScoreScreens[1][i]->hide();
            kartNo = i;
        } else {
            mScoreScreens[0][i]->hide();
            mScoreScreens[1][i]->show();
        }
    }

    int player = -1;
    if (gRaceInfo.getKartInfo(kartNo)->isTagMode()) {
        int p = gSequenceInfo.getPad2Player(gRaceInfo.getKartInfo(kartNo)->getPad());
        if (p == -1) {
            p = 0;
        }
        switch (p) {
        case 0:
        case 1:
            player = 0;
            break;
        case 2:
        case 3:
            player = 2;
            break;
        default:
#line 2799
            JUT_ASSERT(0);
            break;
        }
    } else {
        int p = gSequenceInfo.getPad2Player(gRaceInfo.getKartInfo(kartNo)->getPad());
        if (p == -1) {
            p = 0;
        }
        player = p;
    }

    J2DPicture::TCornerColor corner;
    J2DPicture *pic = (J2DPicture *)mVSLayoutScreen->search((u64)'C4_1P' + (player << 8));
    pic->getCornerColor(corner);
}

void Result2D::getRaceRankLayoutAlpha(int frame, u8 &alpha) {
    if (frame < 0) {
        alpha = 0;
    } else if (frame < 10) {
        alpha = frame * 255 / 10;
    } else {
        alpha = 255;
    }
}

void Result2D::getGPRankLayoutAlpha(int frame, u8 &alpha) {
    if (frame - 165 < 0) {
        alpha = 255;
    } else if (frame - 165 < 10) {
        alpha = 255 - (frame - 165) * 255 / 10;
    } else {
        alpha = 0;
    }
}

#line 2902
void Result2D::getRaceRankLinePos(int frame, int no, f32 &pos) {
    int f = frame - (7 - no) * 5;
    if (f < 0) {
        pos = 840.0f;
    } else if (f < 10) {
        pos = 840.0f - 840.0f * f / 10.0f;
    } else if (f < 30) {
        pos = 0.0f;
    }
    if (f == 10) {
        switch (no) {
        case 0:
            GameAudio::Main::getAudio()->startSystemSe(0x20034);
            break;
        case 1:
            GameAudio::Main::getAudio()->startSystemSe(0x20035);
            break;
        case 2:
            GameAudio::Main::getAudio()->startSystemSe(0x20036);
            break;
        case 3:
            GameAudio::Main::getAudio()->startSystemSe(0x20037);
            break;
        case 4:
            GameAudio::Main::getAudio()->startSystemSe(0x20038);
            break;
        case 5:
            GameAudio::Main::getAudio()->startSystemSe(0x20039);
            break;
        case 6:
            GameAudio::Main::getAudio()->startSystemSe(0x2003a);
            break;
        case 7:
            GameAudio::Main::getAudio()->startSystemSe(0x2003b);
            break;
        default:
            JUT_ASSERT(0);
            break;
        }
    }
}

#line 2960
void Result2D::getGPRankLinePos(int frame, int no, f32 &pos) {
    frame -= 0xa5;
    int f = frame;
    f -= (7 - no) * 5;
    if (f < 0) {
        pos = 0.0f;
    } else if (f < 10) {
        pos = -840.0f * f / 10.0f;
    } else if (f < 30) {
        pos = -840.0f;
    }
    if (f == 1) {
        switch (no) {
        case 0:
            GameAudio::Main::getAudio()->startSystemSe(0x2003c);
            break;
        case 1:
            GameAudio::Main::getAudio()->startSystemSe(0x2003d);
            break;
        case 2:
            GameAudio::Main::getAudio()->startSystemSe(0x2003e);
            break;
        case 3:
            GameAudio::Main::getAudio()->startSystemSe(0x2003f);
            break;
        case 4:
            GameAudio::Main::getAudio()->startSystemSe(0x20040);
            break;
        case 5:
            GameAudio::Main::getAudio()->startSystemSe(0x20041);
            break;
        case 6:
            GameAudio::Main::getAudio()->startSystemSe(0x20042);
            break;
        case 7:
            GameAudio::Main::getAudio()->startSystemSe(0x20043);
            break;
        default:
            JUT_ASSERT(0);
            break;
        }
    }
}

void Result2D::getResultLinePos(int frame, f32 &pos) {
    RaceMgr *mgr = RaceMgr::getManager();
    if (!mgr->isReplayMode()) {
        switch (mgr->getRaceMode()) {
        case VERSUS_RACE:
            frame -= 0x28;
            break;
        case TIME_ATTACK:
            frame -= 0x104;
            break;
        case BALLOON_BATTLE:
        case BOMB_BATTLE:
        case ESCAPE_BATTLE:
            frame -= 0x28;
            break;
        default:
            break;
        }
    }
    if (frame < 0) {
        pos = 840.0f;
    } else if (frame < 10) {
        pos = 840.0f - 840.0f * frame / 10.0f;
    } else {
        pos = 0.0f;
    }
    if (mLine._1980 != 0) {
        frame = mLine._1980 - 1;
        if (frame < 0) {
            pos = 0.0f;
        } else if (frame < 10) {
            pos = -840.0f * frame / 10.0f;
        } else {
            pos = -840.0f;
        }
    }
}

#line 3062
void Result2D::getRaceRankLineScale(int frame, int no, JGeometry::TVec2f &scaleA, JGeometry::TVec2f &scaleB) {
    frame -= 0x127;
    int rem = frame;
    rem -= no * 5;
    if (rem < 0) {
        scaleB.x = scaleB.y = 1.0f;
        scaleA.x = scaleA.y = 1.0f;
    } else if (rem < 10) {
        scaleA.x = mLineScaleTbl[0][rem].v.x;
        scaleA.y = mLineScaleTbl[0][rem].v.y;
        scaleB.x = mLineScaleTbl[1][rem].v.x;
        scaleB.y = mLineScaleTbl[1][rem].v.y;
    } else {
        scaleB.x = scaleB.y = 0.0f;
        scaleA.x = scaleA.y = 0.0f;
    }
    if (rem == 1) {
        switch (no) {
        case 0:
            GameAudio::Main::getAudio()->startSystemSe(0x20043);
            break;
        case 1:
            GameAudio::Main::getAudio()->startSystemSe(0x20042);
            break;
        case 2:
            GameAudio::Main::getAudio()->startSystemSe(0x20041);
            break;
        case 3:
            GameAudio::Main::getAudio()->startSystemSe(0x20040);
            break;
        case 4:
            GameAudio::Main::getAudio()->startSystemSe(0x2003f);
            break;
        case 5:
            GameAudio::Main::getAudio()->startSystemSe(0x2003e);
            break;
        case 6:
            GameAudio::Main::getAudio()->startSystemSe(0x2003d);
            break;
        case 7:
            GameAudio::Main::getAudio()->startSystemSe(0x2003c);
            break;
        default:
            JUT_ASSERT(0);
            break;
        }
    }
}

#line 3120
void Result2D::getGPRankLineScale(int frame, int no, JGeometry::TVec2f &scaleA, JGeometry::TVec2f &scaleB) {
    int rem = frame - (7 - no) * 5;
    if (rem < 0) {
        scaleB.x = scaleB.y = 0.0f;
        scaleA.x = scaleA.y = 0.0f;
    } else if (rem < 10) {
        int idx = 9 - rem;
        scaleA.x = mLineScaleTbl[0][idx].v.x;
        scaleA.y = mLineScaleTbl[0][idx].v.y;
        scaleB.x = mLineScaleTbl[1][idx].v.x;
        scaleB.y = mLineScaleTbl[1][idx].v.y;
    } else {
        scaleB.x = scaleB.y = 1.0f;
        scaleA.x = scaleA.y = 1.0f;
    }
    if (rem == 0) {
        switch (no) {
        case 0:
            GameAudio::Main::getAudio()->startSystemSe(0x20034);
            break;
        case 1:
            GameAudio::Main::getAudio()->startSystemSe(0x20035);
            break;
        case 2:
            GameAudio::Main::getAudio()->startSystemSe(0x20036);
            break;
        case 3:
            GameAudio::Main::getAudio()->startSystemSe(0x20037);
            break;
        case 4:
            GameAudio::Main::getAudio()->startSystemSe(0x20038);
            break;
        case 5:
            GameAudio::Main::getAudio()->startSystemSe(0x20039);
            break;
        case 6:
            GameAudio::Main::getAudio()->startSystemSe(0x2003a);
            break;
        case 7:
            GameAudio::Main::getAudio()->startSystemSe(0x2003b);
            break;
        default:
            JUT_ASSERT(0);
            break;
        }
    }
}

#line 3178
void Result2D::getFrameClr(int player, JUTColor &color, u8 &alpha) {
    JUT_MINMAX_ASSERT(0, player, 3);

    JUTColor c0 = mWhiteColor[player][0];
    JUTColor c1 = mWhiteColor[player][1];

    f32 wave = -0.5f * JMACosShort((s16)(mLine._1990 * (0x10000 - 1) / 50 + 0x7fff)) + 0.5f;

    color.r = c0.r + (c1.r - c0.r) * wave;
    color.g = c0.g + (c1.g - c0.g) * wave;
    color.b = c0.b + (c1.b - c0.b) * wave;
    color.a = 0xff;

    alpha = -95.0f * wave + 170.0f;
}

void Result2D::getVSPointYPos(int frame, int no, f32 &yPos) {
    int f = frame - (3 - no) * 3;
    if (f < 0) {
        yPos = -300.0f;
    } else if (f < 10) {
        yPos = -300.0f - -300.0f * f / 10.0f;
    } else {
        yPos = 0.0f;
    }
}

void Result2D::getMGPointYPos(int frame, f32 &yPos) {
    f32 h = _3cc[0] - _e60[1];
    if (frame < 0) {
        yPos = h;
    } else if (frame < 10) {
        yPos = h - h * frame / 10.0f;
    } else {
        yPos = 0.0f;
    }
}

void Result2D::getVSScale(int frame, f32 &scale) {
    if (frame - 15 < 0) {
        scale = 0.0f;
    } else if (frame - 15 < 10) {
        int f = frame - 15;
        scale = _1bb4[f];
    } else {
        scale = 1.0f;
    }
}

void Result2D::getMGScale(int frame, f32 &scale) {
    if (frame - 15 < 0) {
        scale = 0.0f;
    } else if (frame - 15 < 10) {
        int f = frame - 15;
        scale = _1bb4[f];
    } else {
        scale = 1.0f;
    }
}

void Result2D::getVSColor(int player, JUTColor &color) {
#line 3293
    JUT_MINMAX_ASSERT(0, player, 4);
    JUTColor colors[4];
    colors[0].set(0x95, 0x95, 0x4e, 0);
    colors[1].set(0x4e, 0xa3, 0xc9, 0);
    colors[2].set(0x79, 0x82, 0x7d, 0);
    colors[3].set(0x95, 0x95, 0x4e, 0);

    if (RaceMgr::getManager()->getKartInfo(J2DManager::getStatus2Kart(0))->isTagMode() && player != 0) {
        player++;
        if (player >= 4) {
            player = 3;
        }
    }
    f32 wave = -0.5f * JMASCos((s16)(mLine._1994 * (0x10000 - 1) / 40)) + 0.5f;
    color.r = wave * colors[player].r;
    color.g = wave * colors[player].g;
    color.b = wave * colors[player].b;
    color.a = 0;
}

void Result2D::getMGColor(int player, JUTColor &color) {
#line 3321
    JUT_MINMAX_ASSERT(0, player, 4);
    JUTColor colors[4];
    colors[0].set(0x95, 0x95, 0x4e, 0);
    colors[1].set(0x4e, 0xa3, 0xc9, 0);
    colors[2].set(0x79, 0x82, 0x7d, 0);
    colors[3].set(0x95, 0x95, 0x4e, 0);

    if (RaceMgr::getManager()->getKartInfo(J2DManager::getStatus2Kart(0))->isTagMode() && player != 0) {
        player++;
        if (player >= 4) {
            player = 3;
        }
    }
    f32 wave = -0.5f * JMASCos((s16)(mLine._1994 * (0x10000 - 1) / 40)) + 0.5f;
    color.r = wave * colors[player].r;
    color.g = wave * colors[player].g;
    color.b = wave * colors[player].b;
    color.a = 0;
}

void Result2D::getFrameColor(JUTColor &color) {
    f32 fade = -0.5f * JMASCos((s16)(mLine._1998 * (0x10000 - 1) / 60)) + 0.5f;
    color.r = 255.0f - 255.0f * fade;
    color.g = 255.0f - 255.0f * fade;
    color.b = 255.0f - 255.0f * fade;
    color.a = 0;
}

f32 Result2D::getScale(int frame) {
    f32 c = -0.5f * JMASCos((s16)(frame * (0x10000 - 1) / 50 + 0x7fff)) + 0.5f;
    return 0.3f * c + 1.0f;
}

void Result2D::setScale(int sel) {
    f32 sway = _1af8;
    for (int i = 0; i < mEntries; i++) {
        f32 targetY = 32.0f * sel;
        if (sel == i) {
            _1b08 = targetY;
            int arrowT = ++mArrowTimer[i];
            if (arrowT > 8) {
                mArrowTimer[i] = 8;
            }
            J2DPane *cursor = mSetWord[i][6].mPane;
            cursor->mScale.x = sway;
            cursor->mScale.y = sway;
            cursor->calcMtx();
            J2DPane *icon = mSetWord[0][7].mPane;
            icon->mScale.x = sway;
            icon->mScale.y = sway;
            icon->calcMtx();
            J2DPane *num = mSetWord[0][8].mPane;
            num->mScale.x = sway;
            num->mScale.y = sway;
            num->calcMtx();
        } else {
            int arrowT = --mArrowTimer[i];
            if (arrowT < 0) {
                mArrowTimer[i] = 0;
            }
            J2DPane *cursor = mSetWord[i][6].mPane;
            cursor->mScale.x = 1.0f;
            cursor->mScale.y = 1.0f;
            cursor->calcMtx();
        }
        f32 s = mArrowTimer[i] / 8.0f;
        for (int j = 0; j <= 5; j++) {
            J2DPane *pane = mSetWord[i][j].mPane;
            pane->mScale.x = s;
            pane->mScale.y = 1.0f;
            pane->calcMtx();
        }
    }
    J2DPane *icon = mSetWord[0][7].mPane;
    icon->mOffset.y = _1b04;
    icon->calcMtx();
    J2DPane *num = mSetWord[0][8].mPane;
    num->mOffset.y = _1b04;
    num->calcMtx();
}

void Result2D::setGPClr() {
    mResultSelector = 5;
    mResultState = 0;
    if (RaceMgr::getManager()->isReplayMode()) {
        mResultState = 2;
    }
}

bool Result2D::getAnmEnd() {
    bool ret = false;
    if (mLine._1988 == 0.0f && mLine._198c >= 30) {
        ret = true;
    }
    return ret;
}

#include "JSystem/JAudio/JASFakeMatch2.h"
