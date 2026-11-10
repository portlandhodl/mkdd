#include "Kameda/Result2D.h"
#include "mathHelper.h"
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
    _1740[2] = 0.0f;
    _17b0[0] = 0.0f;
    _1740[4] = 0.0f;
    _17b0[2] = 0.0f;
    for (int i = 0; i < 5; i++) {
        _1758[i].x = 0.0f;
        _17c0[i].x = 0.0f;
    }
    for (int i = 0; i < 6; i++) {
        _1780[i].x = 0.0f;
        _17e8[i].x = 0.0f;
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
    PauseState *pause = (PauseState *)PauseManager::getManager();
    if ((mNode->mParam.mFlag & 1) && !pause->_f && !pause->mGameOver && mDrawFlag
        && (RaceMgr::getManager()->isReplayMode() || Goal2D::mDrawEndFlag)) {
        mOrtho->setPort();
        (this->*mDrawFunc)();
    }
}

void Result2D::drawGP() {}

void Result2D::drawVS() {}

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
        J2DPane *pane2 = mRankRows[i].main.mPane;
        pane2->updateScale(mLineScale[i].b.x, mLineScale[i].b.y);
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

void Result2D::calcTA(KartGamePad *) {}

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

void Result2D::setRaceRank() {}

void Result2D::setGPRank() {}

void Result2D::setVS() {}

void Result2D::setTA() {}

void Result2D::setMG() {}

void Result2D::setBattleCharacter() {}

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
    f32 h = _3cc - _e64;
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
    if (((TPadCheck *)RaceMgr::getManager()->getKartInfo(kartNo))->mPad != NULL && player != 0) {
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
    if (RaceMgr::getManager()->getKartInfo(kartNo)->isTagMode() && player != 0) {
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
KartInfo *RaceMgr::getKartInfo(int index) {
#line 170
    JUT_MINMAX_ASSERT(0, index, 8);
    return &mRaceInfo->mKartInfo[index];
}

J2DPicture *Race2D::getRankPic(int rank, int no) {
#line 490
    JUT_MINMAX_ASSERT(0, rank, 4);
    JUT_MINMAX_ASSERT(0, no, 8);
    return mRankDigits1[rank][no];
}

ECharID KartInfo::getDriverCharID(int driverNo) const {
#line 142
    JUT_MINMAX_ASSERT(0, driverNo, 2);
    return mKartCharacters[driverNo].getCharID();
}

void J2DPicture::setCornerColor(JUtility::TColor color) {
    JUtility::TColor c0 = color;
    JUtility::TColor c1 = color;
    JUtility::TColor c2 = color;
    JUtility::TColor c3 = color;
    mCornerColors[0].set(c0);
    mCornerColors[1].set(c1);
    mCornerColors[2].set(c2);
    mCornerColors[3].set(c3);
}

void J2DPicture::setCornerColor(J2DPicture::TCornerColor colors) {
    JUtility::TColor c0 = colors.mColor0;
    JUtility::TColor c1 = colors.mColor1;
    JUtility::TColor c2 = colors.mColor2;
    JUtility::TColor c3 = colors.mColor3;
    mCornerColors[0].set(c0);
    mCornerColors[1].set(c1);
    mCornerColors[2].set(c2);
    mCornerColors[3].set(c3);
}

void J2DPicture::getCornerColor(J2DPicture::TCornerColor &colors) {
    colors.mColor0.r = mCornerColors[0].r;
    colors.mColor0.g = mCornerColors[0].g;
    colors.mColor0.b = mCornerColors[0].b;
    colors.mColor0.a = mCornerColors[0].a;
    colors.mColor1.r = mCornerColors[1].r;
    colors.mColor1.g = mCornerColors[1].g;
    colors.mColor1.b = mCornerColors[1].b;
    colors.mColor1.a = mCornerColors[1].a;
    colors.mColor2.r = mCornerColors[2].r;
    colors.mColor2.g = mCornerColors[2].g;
    colors.mColor2.b = mCornerColors[2].b;
    colors.mColor2.a = mCornerColors[2].a;
    colors.mColor3.r = mCornerColors[3].r;
    colors.mColor3.g = mCornerColors[3].g;
    colors.mColor3.b = mCornerColors[3].b;
    colors.mColor3.a = mCornerColors[3].a;
}

KartGamePad *KartInfo::getPad(int driverNo) {
#line 126
    JUT_MINMAX_ASSERT(0, driverNo, 2);
    return mKartCharacters[driverNo].getPad();
}

KartInfo *RaceInfo::getKartInfo(int kartNo) {
#line 173
    JUT_MINMAX_ASSERT(0, kartNo, 8);
    return mKartInfo + kartNo;
}

bool J2DScreen::set(const char *path, JKRArchive *archive) {
    return set(path, 0, archive);
}

void *JKRGetNameResource(const char *path, JKRFileLoader *loader) {
    return JKRFileLoader::getGlbResource(path, loader);
}

Result2DHioNode::Result2DHioNode(const char *name)
: HioNode(name, &mParam, 0, 0) {}
