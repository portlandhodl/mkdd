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

u8 Result2D::mDrawFlag;
int Result2D::mResultSelector;
int Result2D::mResultState;

JUTColor Result2D::mWhiteColor[4][2] = {
    0xff505dff, 0x80283cff, 
    0x0064ffff, 0x003280ff,
    0x1e871eff, 0x004600ff, 
    0xb4b4b4ff, 0xb4b4b4ff
};

u64 Result2D::mOverBackTag = 'OverBack';

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

Result2D::Result2D(JKRHeap *) {
    /*
RaceMgr::getKartInfo(int) {}
RaceMgr::getStatusNumber() const {}
KartInfo::getDriverCharID(int) const {}
J2DPicture::setCornerColor(JUTColor) {}
KartInfo::getPad(int) {}
KartInfo::isTagMode() const {}
RaceInfo::getKartInfo(int) {}
J2DPane::getScaleY() const {}
J2DPane::getScaleX() const {}
J2DAnmBase::setFrame(float) {}
JKRGetNameResource(const char *, JKRFileLoader *) {}
J2DPicture::setCornerColor(J2DPicture::TCornerColor) {}
J2DPicture::getCornerColor(J2DPicture::TCornerColor &) {}
Race2D::getRankPic(int, int) {}
J2DManager::getRace2D() {}
JUTColor::operator= (const JUTColor &) {}
KartLocale::getLanguage() {}
Kart2DCommon::getNumberTexture(Kart2DCommon::NumberTextureID) {}
RaceMgr::getRaceLevel() const {}
Kart2DCommon::getCupIconTexture(ERaceGpCup) {}
Kart2DCommon::getCupNameTexture(ERaceGpCup) {}
SequenceInfo::getCup() const {}
Kart2DCommon::ptr() {}
J2DPane::show() {}
JGeometry::TVec2f::operator= (const JGeometry::TVec2f &) {}
J2DPane::hide() {}
JGeometry::TBox2f::TBox2(const JGeometry::TBox2f &) {}
J2DScreen::set(const char *, JKRArchive *) {}
J2DManager::getArchive() const {}
GETJ2DManager() {}
RaceMgr::getRaceMode() const {}
RCMGetManager() {}
System::getJ2DOrtho() {}
Result2DHioNode::Result2DHioNode(const char *) {}
Result2DParam::~Result2DParam() {}
Result2D::data2D::data2D() {}
    */
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
    _197c = 0;
    _1980 = 0;
    _1984 = 0;
    _1988 = 840.0f;
    _198c = 0;
    _1990 = 0;
    _1994 = 0;
    _1998 = 0;
    for (int i = 0; i < 8; i++) {
        mLinePoint[i] = SequenceInfo::RANKPOINT[i];
        mLineYPos[i] = 840.0f;
        for (int j = 0; j < 2; j++) {
            (&mLineScale[i].a)[j].set(1.0f, 1.0f);
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
    u64 ot = mOverBackTag;
    _1afc = 0;
    _1b00 = 0.0f;
    _1b04 = 0.0f;
    _1b08 = 0.0f;
    J2DPane *pane;
    pane = mScreen->search(ot);
    pane->setAlpha(0);
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
        if (pause->mGameOver) {
        } else {
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
            mRankScreens[i]->draw(_3f0 + mLineYPos[i], _3cc[i], mOrtho);
            f32 *off = (f32 *)((u8 *)this + mResultState * 8);
            f32 x = _3cc[i] + off[786];
            f32 y = _3f0 + mLineYPos[i] + off[785];
            mGetPointScreens[i]->draw(y, x, mOrtho);
        }
        break;
    }
    case 2: {
        f32 *baseY = (f32 *)((u8 *)&mRankRows[7].sub[2] + 4);
        if (gSequenceInfo.isEndGP()) {
            if (RaceMgr::getManager()->getPlayerNumber() == 1) {
                mSelPanes[0].cursor->changeTexture("Mozi_Next_Com.bti", 0);
                if (RaceMgr::getManager()->isReplayMode()) {
                    mSelPanes[1].cursor->changeTexture("Mozi_First.bti", 0);
                } else {
                    mSelPanes[1].cursor->changeTexture("Mozi_Replay.bti", 0);
                }
                for (int i = 0; i < 2; i++) {
                    mWinScreens[i]->draw(_1988, *baseY - 16.0f + 32.0f * i, mOrtho);
                }
            }
        } else {
            if (RaceMgr::getManager()->getPlayerNumber() == 1) {
                mSelPanes[0].cursor->changeTexture("Mozi_Next.bti", 0);
                if (RaceMgr::getManager()->isReplayMode()) {
                    mSelPanes[1].cursor->changeTexture("Mozi_First.bti", 0);
                } else {
                    mSelPanes[1].cursor->changeTexture("Mozi_Replay.bti", 0);
                }
                mSelPanes[2].cursor->changeTexture("Mozi_Quit.bti", 0);
                for (int i = 0; i < 3; i++) {
                    mWinScreens[i]->draw(_1988, *baseY - 32.0f + 32.0f * i, mOrtho);
                }
            } else {
                mSelPanes[0].cursor->changeTexture("Mozi_Next.bti", 0);
                mSelPanes[1].cursor->changeTexture("Mozi_Quit.bti", 0);
                for (int i = 0; i < 2; i++) {
                    mWinScreens[i]->draw(_1988, *baseY - 32.0f + 32.0f * i, mOrtho);
                }
            }
        }
        break;
    }
    }
}

void Result2D::drawVS() {
    int num = RaceMgr::getManager()->mRaceInfo->getStatusNumber();

    f32 ypos[4];
    f32 xpos[4];
    for (int i = 0; i < num; i++) {
        xpos[i] = 15.0f + 578.0f / i * (i + 1) + 578.0f / (num * 2);
        ypos[i] = _e58 + (_e5c - _e58) / (num * 2) * (i + 1) + (_e5c - _e58) / ((num - 1) * 2);
    }

    mScreen->draw(0.0f, 0.0f, mOrtho);

    for (int row = 0; row < 2; row++) {
        for (int i = 0; i < num; i++) {
            mScoreScreens[row][i]->draw(xpos[i], _e68, mOrtho);
        }
    }

    for (int row = 0; row < num; row++) {
        mPointScreens2[row]->draw(xpos[row], _e54 + mVSPointYPos[row][0], mOrtho);
        for (int j = 0; j < num - 1; j++) {
            J2DPane **pane = (J2DPane **)((u8 *)&_e1c[row]);
            pane[j]->draw(xpos[row], mVSPointYPos[row][j + 1] + ypos[j], mOrtho, true, true);
        }
    }

    mSelPanes[0].cursor->changeTexture("Mozi_One_More_Race.bti", 0);
    mSelPanes[1].cursor->changeTexture("Mozi_Change_Course.bti", 0);
    mSelPanes[2].cursor->changeTexture("Mozi_Change_Character.bti", 0);
    if (RCMGetManager()->isReplayMode()) {
        mSelPanes[3].cursor->changeTexture("Mozi_First.bti", 0);
    } else {
        mSelPanes[3].cursor->changeTexture("Mozi_Replay.bti", 0);
    }

    for (int i = 0; i < 4; i++) {
        mWinScreens[i]->draw(_1988, 304.0f + 32.0f * i, mOrtho);
    }
}

void Result2D::drawTA() {
    mScreen->draw(0.0f, 0.0f, mOrtho);
    mScreenMG->draw(0.0f, 0.0f, mOrtho);

    if (gGhostFile.mGhostType == 3 && !(gSystemRecord.mOptions & 8)) {
        mSelPanes[0].cursor->changeTexture("Mozi_One_More_Race.bti", 0);
        mSelPanes[1].cursor->changeTexture("Mozi_Change_Course.bti", 0);
        mSelPanes[2].cursor->changeTexture("Mozi_Change_Character.bti", 0);
        if (RCMGetManager()->isReplayMode()) {
            mSelPanes[3].cursor->changeTexture("Mozi_First.bti", 0);
        } else {
            mSelPanes[3].cursor->changeTexture("Mozi_Replay.bti", 0);
        }
        mSceneQuitPicture->changeTexture("Mozi_SaveGhost.bti", 0);
        mSceneResultPicture2->changeTexture("Mozi_Quit.bti", 0);

        for (int i = 0; i < 6; i++) {
            mWinScreens[i]->draw(_1988, 144.0f + 32.0f * i, mOrtho);
        }
    } else {
        mSelPanes[0].cursor->changeTexture("Mozi_One_More_Race.bti", 0);
        mSelPanes[1].cursor->changeTexture("Mozi_Change_Course.bti", 0);
        mSelPanes[2].cursor->changeTexture("Mozi_Change_Character.bti", 0);
        if (RCMGetManager()->isReplayMode()) {
            mSelPanes[3].cursor->changeTexture("Mozi_First.bti", 0);
        } else {
            mSelPanes[3].cursor->changeTexture("Mozi_Replay.bti", 0);
        }
        mSceneQuitPicture->changeTexture("Mozi_Quit.bti", 0);

        for (int i = 0; i < 5; i++) {
            mWinScreens[i]->draw(_1988, 160.0f + 32.0f * i, mOrtho);
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
            mScoreScreens[row][i]->draw(ypos[i], _e6c, mOrtho);
        }
    }
    for (int i = 0; i < num; i++) {
        mPointScreens[i]->draw(ypos[i], _e64 + mMGPointYPos[i], mOrtho);
    }

    mSelPanes[0].cursor->changeTexture("Mozi_One_More_Race.bti", 0);
    mSelPanes[1].cursor->changeTexture("Mozi_Change_Map.bti", 0);
    mSelPanes[2].cursor->changeTexture("Mozi_Change_Character.bti", 0);
    mSelPanes[3].cursor->changeTexture("Mozi_Change_MiniGame.bti", 0);
    mSceneQuitPicture->changeTexture("Mozi_Quit.bti", 0);

    f32 ybase = 288.0f;
    f32 xstep = 32.0f;
    for (int i = 0; i < 5; i++) {
        mWinScreens[i]->draw(_1988, ybase + xstep * i, mOrtho);
    }
}

void Result2D::calc(KartGamePad *pad) {
    if (!mDrawFlag) {
        return;
    }

    if (pad != NULL && mResultState == 2 && _1988 == 0.0f && _1afc == 0) {
        (this->*mAutoFunc)();
    }
    (this->*mCalcFunc)();

    switch (mResultState) {
    case 0:
        if (_197c < 0 || _197c > 0x168) {
            _197c = 0;
        }
        calcRaceRank();
        setRaceRank();
        if (_197c == 0xd7) {
            _197c--;
        }
        if (pad->testTrigger(JUTGamePad::A)) {
            if (_197c >= 0xcd && _197c < 0x127) {
                _197c = 0x127;
            }
        }
        if (_197c == 0x168) {
            mResultState = 1;
            _197c = -1;
        }
        break;
    case 1:
        if (_197c < 0 || _197c > 0xe6) {
            _197c = 0;
        }
        calcGPRank();
        setGPRank();
        if (_197c == 0x55) {
            _197c--;
        }
        if (pad->testTrigger(JUTGamePad::A)) {
            if (_197c >= 0x4b && _197c < 0xa5) {
                _197c = 0xa5;
            }
        }
        if (_197c == 0xe6) {
            mResultState = 3;
            _197c = -1;
        }
        break;
    case 2:
        switch (RCMGetManager()->mRaceInfo->getRaceMode()) {
        case TIME_ATTACK:
            if (_197c < 0) {
                _197c = 0;
            }
            if (_197c == 0xb4) {
                _197c--;
            }
            if (pad->testTrigger(JUTGamePad::A)) {
                if (_197c >= 0x5a && _197c < 0xb4) {
                    _197c = 0xb4;
                }
            }
            if (_197c == 0xf0) {
                CardAgent::ask(CardAgent::mcCommand3, 0);
            }
            if (_197c == 0xf1) {
                if (CardAgent::msState == 0) {
                    CardAgent::getPrintMemoryCard()->setStuff(0, 0, 0);
                } else {
                    _197c--;
                }
            }
            calcTA(pad);
            break;
        case VERSUS_RACE:
            calcVS();
            setVS();
            break;
        case BALLOON_BATTLE:
        case BOMB_BATTLE:
        case ESCAPE_BATTLE:
            calcMG();
            setMG();
            break;
        case GRAND_PRIX:
        case AWARD_DEMO:
        case STAFF_ROLL:
            calcResult();
            break;
        default:
            break;
        }
        break;
    default:
        mResultState = 2;
        _197c = -1;
        if (gSequenceInfo.isEndGP() && RCMGetManager()->mRaceInfo->getPlayerKartNumber() == 2) {
            struct PauseState {
                u8 _0[0xf];
                u8 _f;
            };
            ((PauseState *)PauseManager::getManager())->_f = 1;
        }
        break;
    }
    _197c++;
    if (_1980 != 0) {
        _1980++;
    }
    int c90 = _1990 + 1;
    _1990 = c90;
    if (c90 >= 50) {
        _1990 = 0;
    }
    int c94 = _1994 + 1;
    _1994 = c94;
    if (c94 >= 40) {
        _1994 = 0;
    }
    int c98 = _1998 + 1;
    _1998 = c98;
    if (c98 >= 60) {
        _1998 = 0;
    }
    _1984 += 10;
    if (_1984 >= 128) {
        _1984 = 128;
    }
    if (_1980 != 0) {
        _1984 -= (_1980 - 1) * 10;
        if (_1984 < 0) {
            _1984 = 0;
        }
    }

    J2DPane *overBack = mScreen->search(mOverBackTag);
    overBack->setAlpha((u8)_1984);

    if (_1988 == 0.0f) {
        int anm = _198c;
        _198c = anm + 1;
        if (anm >= 30) {
            _198c = 30;
        }
    }
    if (_1afc < 8) {
        f32 diff = _1b08 - _1b00;
        _1b04 = _1b00 + (f32)_1afc * diff * 0.125f;
    } else {
        _1b00 = _1b08;
        _1b04 = _1b08;
    }
    if (_1afc != 0) {
        if (_1afc >= 8) {
            _1afc = 0;
        } else {
            _1afc++;
        }
    }
    _1af4++;
    _1af8 = getScale(_1af4);
}

void Result2D::calcRaceRank() {
    int frame = _197c;
    getRaceRankLayoutAlpha(frame, mLayoutAlpha);
    for (int rank = 0; rank < 8; rank++) {
        getRaceRankLinePos(frame, rank, mLineYPos[rank]);
        getRaceRankLineScale(frame, rank, mLineScale[rank].a, mLineScale[rank].b);
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 20; j++) {
            JGeometry::TVec2f a = mLineScale[i].a;
            J2DPane *pane = mLines[i][j].mPane;
            pane->updateScale(a.x, a.y);
        }
    }
    for (int i = 0; i < 8; i++) {
        JGeometry::TVec2f *sb = &mLineScale[i].b;
        J2DPane *pane2 = mRankRows[i].main.mPane;
        pane2->updateScale(sb->x, sb->y);
        for (int j = 0; j < 3; j++) {
            J2DPane *sub = mRankRows[i].sub[j].mPane;
            sub->updateScale(mLineScale[i].a.x, mLineScale[i].a.y);
        }
    }
    int f = frame - 0x4b;
    if (f >= 0 && f < 0x64 && f % 10 == 9) {
        GameAudio::Main::getAudio()->startSystemSe(0x20033);
        for (int i = 0; i < 8; i++) {
            if (mLinePoint[i] > 0) {
                mLinePoint[i]--;
            }
        }
    }
}

void Result2D::calcGPRank() {
    int frame = _197c;
    getGPRankLayoutAlpha(frame, mLayoutAlpha);
    for (int rank = 0; rank < 8; rank++) {
        getGPRankLinePos(frame, rank, mLineYPos[rank]);
        getGPRankLineScale(frame, rank, mLineScale[rank].a, mLineScale[rank].b);
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 20; j++) {
            JGeometry::TVec2f a = mLineScale[i].a;
            J2DPane *pane = mLines[i][j].mPane;
            pane->updateScale(a.x, a.y);
        }
    }
    for (int i = 0; i < 8; i++) {
        JGeometry::TVec2f &sb = mLineScale[i].b;
        J2DPane *pane = mRankRows[i].main.mPane;
        pane->updateScale(sb.x, sb.y);
        for (int j = 0; j < 3; j++) {
            J2DPane *sub = mRankRows[i].sub[j].mPane;
            sub->updateScale(mLineScale[i].a.x, mLineScale[i].a.y);
        }
        J2DPane *last = mLines[i][19].mPane;
        last->updateScale(mLineScale[i].a.x, mLineScale[i].a.y);
    }
}

void Result2D::calcResult() {
    getResultLinePos(_197c, _1988);
}

void Result2D::calcVS() {
    int frame = _197c;
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
    getResultLinePos(frame, _1988);
}

void Result2D::calcTA(KartGamePad *pad) {
    getResultLinePos(_197c, _1988);
    f32 one = 1.0f;
    if (_1ab0 || _1ab8) {
        if (_197c < 60) {
            _1744[0].mVal = (f32)_197c;
        } else if (_197c < 180) {
            _1744[0].mVal = 70.0f;
        } else if (_197c < 240) {
            _1744[0].mVal = (f32)(_197c - 80);
        } else {
            _1744[0].mVal = 170.0f;
        }
        f32 vzero = _17ac[0].mVal;
        f32 v0 = vzero + one;
        _17ac[0].mVal = v0;
        if (v0 >= 80.0f) {
            _17ac[0].mVal = 0.0f;
        }
        if (_1ab8) {
            f32 v1a = _1744[1].mVal;
                f32 v1 = v1a + one;
            _1744[1].mVal = v1;
            if (v1 >= 80.0f) {
                _1744[1].mVal = 0.0f;
            }
            f32 v2a = _17ac[1].mVal;
                f32 v2 = v2a + one;
            _17ac[1].mVal = v2;
            if (v2 >= 80.0f) {
                _17ac[1].mVal = 0.0f;
            }
        } else {
            _1744[1].mVal = 0.0f;
            _17ac[1].mVal = 0.0f;
        }
        for (int i = 0; i < 5; i++) {
            if (_1ab0 && i == _1ab4) {
                f32 s0a = _1754[i].mVal;
                f32 s0 = s0a + one;
                _1754[i].mVal = s0;
                if (s0 >= 80.0f) {
                    _1754[i].mVal = 0.0f;
                }
                f32 s1a = _17bc[i].mVal;
                f32 s1 = s1a + one;
                _17bc[i].mVal = s1;
                if (s1 >= 80.0f) {
                    _17bc[i].mVal = 0.0f;
                }
            } else {
                _1754[i].mVal = 0.0f;
                _17bc[i].mVal = 0.0f;
            }
        }
        for (int i = 0; i < 6; i++) {
            f32 t0a = _177c[i].mVal;
                f32 t0 = t0a + one;
            _177c[i].mVal = t0;
            if (t0 >= 160.0f) {
                _177c[i].mVal = 0.0f;
            }
            if (_197c < 60) {
                _17e4[i].mVal = 0.0f;
            } else if (_197c < 180) {
                f32 t1a = _17e4[i].mVal;
                f32 t1 = t1a + one;
                _17e4[i].mVal = t1;
                if (t1 > 100.0f) {
                    _17e4[i].mVal = 100.0f;
                }
            } else if (_197c < 240) {
                f32 t1a = _17e4[i].mVal;
                f32 t1 = t1a - one;
                _17e4[i].mVal = t1;
                if (t1 < 0.0f) {
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
    int frame = _197c;
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
    getResultLinePos(_197c, _1988);
}

void Result2D::calcPASSWD(KartGamePad *pad) {
    if (pad != NULL && getAnmEnd() && mPASSWDEnabled && !mPASSWDAccepted) {
        u16 code[9] = { 0x40, 0x20, 0x40, 0x20, 0x400, 0x800, 0x400, 0x800, 0x10 };
        int idx = mPASSWDCount;
        u32 trigger = pad->getTrigger();
        if (trigger != 0) {
            if (code[idx] == trigger) {
                mPASSWDCount = idx + 1;
                if (idx + 1 >= 9) {
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
        J2DPicture *pane = *(J2DPicture **)((u8 *)this + 0x350 + i * 12);
        pane->setAlpha(mLayoutAlpha);
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 11; j++) {
            mLines[i][j].mPane->mIsVisible = true;
        }
    }

    for (int i = 0; i < 8; i++) {
        int kartNo = gSequenceInfo.get_2b8(i);
        int player;
        if (kartNo < RaceMgr::getManager()->mRaceInfo->getStatusNumber()) {
            player = -1;
            if (kartNo == J2DManager::getStatus2Kart(0)) {
                player = 0;
            } else if (kartNo == J2DManager::getStatus2Kart(1)) {
                if (gRaceInfo.getKartInfo(0)->getPad(1) != NULL) {
                    player = 2;
                } else {
                    player = 1;
                }
            }
            JUTColor color;
            u8 alpha;
            getFrameClr(player, color, alpha);
            for (int j = 0; j < 3; j++) {
                mLines[i][j].mPane->mIsVisible = true;
                mLines[i][j].mPane->setAlpha(alpha);
                mLines[i][j].mPane->setBlack(mWhiteColor[player][0]);
            }
            mLines[i][5].mPane->setBlack(color);
            mLines[i][3].mPane->setBlack(color);
        } else {
            for (int j = 0; j < 3; j++) {
                mLines[i][j].mPane->mIsVisible = false;
            }
            mLines[i][5].mPane->setBlack(mWhiteColor[3][0]);
            mLines[i][3].mPane->setBlack(mWhiteColor[3][0]);
        }

        ECharID charID0 = gRaceInfo.getKartInfo(kartNo)->getDriverCharID(0);
        if (charID0 > 0 && charID0 < 0x15) {
            mLines[i][6].mPane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(charID0), 0);
        }
        ECharID charID1 = gRaceInfo.getKartInfo(kartNo)->getDriverCharID(1);
        if (charID1 > 0 && charID1 < 0x15) {
            mLines[i][4].mPane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(charID1), 0);
        }

        for (int j = 0; j < 8; j++) {
            if (gSequenceInfo.getCourse() == 1) {
                mLines[j][19].mPane->mIsVisible = false;
            } else {
                mLines[j][19].mPane->mIsVisible = true;
                mLines[j][19].mPane->mScale.set(0.0f, 0.0f);
                mLines[j][19].mPane->calcMtx();
                int kart = gSequenceInfo.get_2d8(j);
                int idx = 0;
                while (idx < 8) {
                    if (kart == gSequenceInfo.get_2f8(idx)) {
                        break;
                    }
                    idx++;
                }
                int icon;
                if (player < idx) {
                    icon = 0;
                } else if (player > idx) {
                    icon = 2;
                } else {
                    icon = 1;
                }
                mLines[j][19].mPane->changeTexture(mRankIconPics[icon], 0);
            }
        }

        int ms, sec, min;
        RaceTime time = RaceMgr::getManager()->getKartChecker(kartNo)->getTotalTime();
        if (time.get() != -1) {
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
            for (int j = 8; j < 15; j++) {
                mLines[i][j].mPane->changeTexture(dash, 0);
            }
        }

        int point = gSequenceInfo.getRankPoint(kartNo) - mLinePoint[i];
        int hund = point / 100;
        int tens = (point / 10) % 10;
        int ones = point % 10;

        if (hund != 0) {
            if (hund >= 0 && hund < 10) {
                mRankRows[i].sub[2].mPane->mIsVisible = true;
                mRankRows[i].sub[2].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)hund), 0);
            }
        } else {
            mRankRows[i].sub[2].mPane->mIsVisible = false;
        }
        if (tens != 0) {
            if (tens >= 0 && tens < 10) {
                mRankRows[i].sub[1].mPane->mIsVisible = true;
                mRankRows[i].sub[1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)tens), 0);
            }
        } else if (hund != 0) {
            mRankRows[i].sub[1].mPane->mIsVisible = true;
            mRankRows[i].sub[1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)tens), 0);
        } else {
            mRankRows[i].sub[1].mPane->mIsVisible = false;
        }
        if (ones >= 0 && ones < 10) {
            mRankRows[i].sub[0].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)ones), 0);
        }
    }
}

void Result2D::setGPRank() {
    int j;
    for (int i = 0; i < 11; i++) {
        J2DPicture *pane = *(J2DPicture **)((u8 *)this + 0x350 + i * 12);
        pane->setAlpha(mLayoutAlpha);
    }
    for (int i = 0; i < 8; i++) {
        for (j = 0; j < 11; j++) {
            mLines[i][j].mPane->mIsVisible = false;
        }
    }

    for (int i = 0; i < 8; i++) {
        int kartNo = gSequenceInfo.get_2d8(i);
        if (kartNo < RaceMgr::getManager()->mRaceInfo->getStatusNumber()) {
            int player = -1;
            if (kartNo == J2DManager::getStatus2Kart(0)) {
                player = 0;
            } else if (kartNo == J2DManager::getStatus2Kart(1)) {
                if (gRaceInfo.getKartInfo(0)->getPad(1) != NULL) {
                    player = 2;
                } else {
                    player = 1;
                }
            }
            JUTColor color;
            u8 alpha;
            getFrameClr(player, color, alpha);
            for (j = 0; j < 3; j++) {
                mLines[i][j].mPane->mIsVisible = true;
                mLines[i][j].mPane->setAlpha(alpha);
                mLines[i][j].mPane->setBlack(mWhiteColor[player][0]);
            }
            mLines[i][5].mPane->setBlack(color);
            mLines[i][3].mPane->setBlack(color);
        } else {
            for (j = 0; j < 3; j++) {
                mLines[i][j].mPane->mIsVisible = false;
            }
            mLines[i][5].mPane->setBlack(mWhiteColor[3][0]);
            mLines[i][3].mPane->setBlack(mWhiteColor[3][0]);
        }

        ECharID charID0 = gRaceInfo.getKartInfo(kartNo)->getDriverCharID(0);
        if (charID0 > 0 && charID0 < 0x15) {
            mLines[i][6].mPane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(charID0), 0);
        }
        ECharID charID1 = gRaceInfo.getKartInfo(kartNo)->getDriverCharID(1);
        if (charID1 > 0 && charID1 < 0x15) {
            mLines[i][4].mPane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(charID1), 0);
        }

        int point = gSequenceInfo.get_298(kartNo);
        int hund = point / 100;
        int tens = (point / 10) % 10;
        int ones = point % 10;

        if (hund != 0) {
            if (hund >= 0 && hund < 10) {
                mRankRows[i].sub[2].mPane->mIsVisible = true;
                mRankRows[i].sub[2].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)hund), 0);
            }
        } else {
            mRankRows[i].sub[2].mPane->mIsVisible = false;
        }
        if (tens != 0) {
            if (tens >= 0 && tens < 10) {
                mRankRows[i].sub[1].mPane->mIsVisible = true;
                mRankRows[i].sub[1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)tens), 0);
            }
        } else if (hund != 0) {
            mRankRows[i].sub[1].mPane->mIsVisible = true;
            mRankRows[i].sub[1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)tens), 0);
        } else {
            mRankRows[i].sub[1].mPane->mIsVisible = false;
        }
        if (ones >= 0 && ones < 10) {
            mRankRows[i].sub[0].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)ones), 0);
        }
    }
}

void Result2D::setVS() {
    int num = RaceMgr::getManager()->getStatusNumber();
    setBattleCharacter();

    int num1 = num - 1;
    for (int i = 0; i < num; i++) {
        int point = gSequenceInfo.get_40(i, 0);
        u8 *p1 = (u8 *)this + i * 48;
        data2D *row = (data2D *)(p1 + 0x1418);
        int digit = point % 10;
        row[0].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)digit), 0);
        row[1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(point / 10)), 0);

        int kartNo = J2DManager::getStatus2Kart(i);
        if (gSequenceInfo.getStartNo(kartNo) == 0) {
            JUTColor color;
            getVSColor(i, color);
            row[0].mPane->setBlack(color);
            row[1].mPane->setBlack(color);
            getFrameColor(color);
            ((data2D *)((u8 *)this + 0xee4 + i * 168))->mPane->setBlack(color);
        } else {
            JUTColor color(0);
            row[0].mPane->setBlack(color);
            row[1].mPane->setBlack(color);
        }

        for (int j = 0; j < num1; j++) {
            int p = gSequenceInfo.get_40(i, j + 1);
            u8 *p2 = (u8 *)this + i * 144 + j * 48;
            data2D *row2 = (data2D *)(p2 + 0x14d8);
            int d = p % 10;
            row2[0].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)d), 0);
            row2[1].mPane->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(p / 10)), 0);
        }
    }
}

void Result2D::setTA() {
    int kartNo = J2DManager::getStatus2Kart(0);
    KartInfo *info = gRaceInfo.getKartInfo(kartNo);
    ECharID charID0 = info->getDriverCharID(0);
    ECharID charID1 = info->getDriverCharID(1);
    EKartID kartID = info->getKartID();
    ECourseID course = ResMgr::msCourseID;

    TARecord rec((u8)charID0, (u8)charID1, (u8)kartID, (u8)course);
    rec.setName(&gSystemRecord.mDefaultName[0]);
    TARecord lap((u8)charID0, (u8)charID1, (u8)kartID, (u8)course);
    lap.setName(&gSystemRecord.mDefaultName[0]);

    rec.setRaceTime(RaceMgr::getManager()->getKartChecker(kartNo)->getTotalTime());
    lap.setRaceTime(*RaceMgr::getManager()->getKartChecker(kartNo)->getBestLapTime());

    if (RaceMgr::getManager()->getKartChecker(kartNo)->isBestTotalTimeRenewal(4)) {
        int rank = gSystemRecord.rankTARecord(course, rec);
        if (rank >= 1 && rank <= 5) {
            _1ab0 = 1;
            _1ab4 = rank - 1;
        }
    }

    TARecord bestLap = *gSystemRecord.getBestLap(course);
    if (bestLap.isValid() && bestLap.less(lap)) {
        gSystemRecord.setBestLap(course, lap);
        _1ab8 = 1;
    } else if (bestLap.getRaceTime().get() != -1) {
        gSystemRecord.setBestLap(course, lap);
        _1ab8 = 1;
    }

    if (_1ab0 || _1ab8) {
        for (int i = 0; i < 5; i++) {
            TARecord *r = gSystemRecord.getTARecord(course, i);
            TALinePics &pics = mTALines[i];
            if (r->isValid()) {
                Kart2DCommon::ptr()->changeStringTexture((const char *)r, pics.namePics, 3);
                for (int j = 0; j < 3; j++) {
                    pics.idPics[j]->mIsVisible = true;
                }
                pics.charPic1->changeTexture(Kart2DCommon::ptr()->getCharIconTextureChecked(r->getCharID(0)), 0);
                pics.charPic2->changeTexture(Kart2DCommon::ptr()->getCharIconTextureChecked(r->getCharID(1)), 0);
                pics.kartPic->changeTexture(Kart2DCommon::ptr()->getKartIconTexture(r->getKartID()), 0);
                RaceTime time = r->getRaceTime();
                if (time.get() != -1) {
                    int min, sec, ms;
                    time.get(&min, &sec, &ms);
                    Kart2DCommon::ptr()->changeNumberTexture(ms, pics.msPics, 3, true, false);
                    Kart2DCommon::ptr()->changeNumberTexture(sec, pics.secPics, 2, true, false);
                    Kart2DCommon::ptr()->changeNumberTexture(min, pics.minPics, 2, true, false);
                } else {
                    Kart2DCommon::ptr()->changeNumberTexture(0, pics.msPics, 3, true, true);
                    Kart2DCommon::ptr()->changeNumberTexture(0, pics.secPics, 2, true, true);
                    Kart2DCommon::ptr()->changeNumberTexture(0, pics.minPics, 2, true, true);
                }
            } else {
                Kart2DCommon::ptr()->changeStringTexture("---", pics.namePics, 3);
                for (int j = 0; j < 3; j++) {
                    pics.idPics[j]->mIsVisible = false;
                }
                Kart2DCommon::ptr()->changeNumberTexture(0, pics.msPics, 3, true, true);
                Kart2DCommon::ptr()->changeNumberTexture(0, pics.secPics, 2, true, true);
                Kart2DCommon::ptr()->changeNumberTexture(0, pics.minPics, 2, true, true);
            }
        }

        TARecord *best = gSystemRecord.getBestLap(course);
        if (best->isValid()) {
            Kart2DCommon::ptr()->changeStringTexture((const char *)best, mTABest.namePics, 3);
            RaceTime time = best->getRaceTime();
            if (time.get() != -1) {
                int min, sec, ms;
                time.get(&min, &sec, &ms);
                Kart2DCommon::ptr()->changeNumberTexture(ms, mTABest.msPics, 3, true, false);
                Kart2DCommon::ptr()->changeNumberTexture(sec, mTABest.secPics, 2, true, false);
                Kart2DCommon::ptr()->changeNumberTexture(min, mTABest.minPics, 2, true, false);
            } else {
                Kart2DCommon::ptr()->changeNumberTexture(0, mTABest.msPics, 3, true, true);
                Kart2DCommon::ptr()->changeNumberTexture(0, mTABest.secPics, 2, true, true);
                Kart2DCommon::ptr()->changeNumberTexture(0, mTABest.minPics, 2, true, true);
            }
        } else {
            Kart2DCommon::ptr()->changeStringTexture("---", mTABest.namePics, 3);
            Kart2DCommon::ptr()->changeNumberTexture(0, mTABest.msPics, 3, true, true);
            Kart2DCommon::ptr()->changeNumberTexture(0, mTABest.secPics, 2, true, true);
            Kart2DCommon::ptr()->changeNumberTexture(0, mTABest.minPics, 2, true, true);
        }
    } else {
        _197c = 0xf2;
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
            JUTColor color(0);
            mPointRows[i][0].mPane->setBlack(color);
            mPointRows[i][1].mPane->setBlack(color);
        }
    }
}

void Result2D::setBattleCharacter() {
    int num = RaceMgr::getManager()->getStatusNumber();
    int kartNo = -1;

    for (int i = 0; i < num; i++) {
        int k = J2DManager::getStatus2Kart(i);
        if (gSequenceInfo.getStartNo(k) == 0) {
            mScoreScreens[0][i]->show();
            mScoreScreens[1][i]->hide();
            kartNo = k;
        } else {
            mScoreScreens[0][i]->hide();
            mScoreScreens[1][i]->show();
        }
    }

    int player = -1;
    KartInfo *info = gRaceInfo.getKartInfo(kartNo);
    if (*(KartGamePad **)((u8 *)info + 0xc) != NULL) {
        player = gSequenceInfo.getPad2Player(gRaceInfo.getKartInfo(kartNo)->getPad());
        if (player == -1) {
            player = 0;
        }
        if (player < 2) {
            player = 0;
        } else if (player < 4) {
            player = 2;
        } else {
#line 2799
            JUT_ASSERT(0);
        }
    } else {
        player = gSequenceInfo.getPad2Player(info->getPad());
        if (player == -1) {
            player = 0;
        }
    }

    J2DPicture *pic = (J2DPicture *)mScoreScreens[3][0]->search((u64)'C4_1P' + (player << 8));
    J2DPicture::TCornerColor corner;
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
    if (_1980 != 0) {
        frame = _1980 - 1;
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
        scaleA.x = mLineScaleTblA[rem].x;
        scaleA.y = mLineScaleTblA[rem].y;
        scaleB.x = mLineScaleTblB[rem].x;
        scaleB.y = mLineScaleTblB[rem].y;
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
        scaleA.x = mLineScaleTblA[idx].x;
        scaleA.y = mLineScaleTblA[idx].y;
        scaleB.x = mLineScaleTblB[idx].x;
        scaleB.y = mLineScaleTblB[idx].y;
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

    f32 wave = -0.5f * JMACosShort((s16)(_1990 * (0x10000 - 1) / 50 + 0x7fff)) + 0.5f;

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
    f32 h = _3cc[0] - _e64;
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

    int kartNo = J2DManager::getStatus2Kart(0);
    struct TPadCheck { // mirrors: KartInfo::mKartCharacters[1].mKartGamePad
        u8 _0[0xc];
        KartGamePad *mPad;
    };
    TPadCheck *info = (TPadCheck *)RaceMgr::getManager()->getKartInfo(kartNo);
    if (info->mPad != NULL && player != 0) {
        player++;
        if (player >= 4) {
            player = 3;
        }
    }
    f32 wave = -0.5f * JMASCos((s16)(_1994 * (0x10000 - 1) / 40)) + 0.5f;
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

    int kartNo = J2DManager::getStatus2Kart(0);
    KartInfo *info = RaceMgr::getManager()->getKartInfo(kartNo);
    if (info->isTagMode() && player != 0) {
        player++;
        if (player >= 4) {
            player = 3;
        }
    }
    f32 wave = -0.5f * JMASCos((s16)(_1994 * (0x10000 - 1) / 40)) + 0.5f;
    color.r = wave * colors[player].r;
    color.g = wave * colors[player].g;
    color.b = wave * colors[player].b;
    color.a = 0;
}

void Result2D::getFrameColor(JUTColor &color) {
    f32 fade = -0.5f * JMASCos((s16)(_1998 * (0x10000 - 1) / 60)) + 0.5f;
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
            J2DPane *cursor = mSelPanes[i].cursor;
            cursor->mScale.x = sway;
            cursor->mScale.y = sway;
            cursor->calcMtx();
            J2DPane *icon = mSelPanes[0].icon;
            icon->mScale.x = sway;
            icon->mScale.y = sway;
            icon->calcMtx();
            J2DPane *num = mSelPanes[0].num;
            num->mScale.x = sway;
            num->mScale.y = sway;
            num->calcMtx();
        } else {
            int arrowT = --mArrowTimer[i];
            if (arrowT < 0) {
                mArrowTimer[i] = 0;
            }
            J2DPane *cursor = mSelPanes[i].cursor;
            cursor->mScale.x = 1.0f;
            cursor->mScale.y = 1.0f;
            cursor->calcMtx();
        }
        f32 s = mArrowTimer[i] * 0.125f;
        for (int j = 0; j <= 5; j++) {
            J2DPane *pane = mSelPanes[i].parts[j].mPane;
            pane->mScale.x = s;
            pane->mScale.y = 1.0f;
            pane->calcMtx();
        }
    }
    J2DPane *icon = mSelPanes[0].icon;
    icon->mOffset.y = _1b04;
    icon->calcMtx();
    J2DPane *num = mSelPanes[0].num;
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
    if (_1988 == 0.0f && _198c >= 30) {
        ret = true;
    }
    return ret;
}

#include "JSystem/JAudio/JASFakeMatch2.h"
J2DPicture *Race2D::getRankPic(int rank, int no) {
#line 490
    JUT_MINMAX_ASSERT(0, rank, 4);
    JUT_MINMAX_ASSERT(0, no, 8);
    return mRankDigits1[rank][no];
}


bool J2DScreen::set(const char *path, JKRArchive *archive) {
    return set(path, 0, archive);
}

Result2DHioNode::Result2DHioNode(const char *name)
: HioNode(name, &mParam, 0, 0) {}
