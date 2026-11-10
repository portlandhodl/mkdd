#include "Kameda/SceneCourseSelect.h"

#include "Kameda/Cup2D.h"
#include "Kameda/Scene.h"
#include "Kameda/SequenceInfo.h"
#include "Kaneshige/RaceInfo.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "Osako/AppMgr.h"
#include "Osako/Kart2DCommon.h"
#include "Osako/MenuBackground.h"
#include "Osako/MenuTitleLine.h"
#include "Osako/RaceApp.h"
#include "Osako/ResMgr.h"
#include "Osako/SequenceApp.h"
#include "Osako/SystemRecord.h"
#include "Osako/system.h"
#include "kartEnums.h"
#include "mathHelper.h"

int SceneCourseSelect::mCup; // ERaceGpCup?
int SceneCourseSelect::mCourse; // ECourseID?

const ERaceGpCup SceneCourseSelect::mRaceGpCup[5] = {
    MUSHROOM_CUP, FLOWER_CUP, STAR_CUP, SPECIAL_CUP,
    ALL_CUP_TOUR
};

const char *SceneCourseSelect::mCupName[] = {
    "CupName_MUSHROOM_CUP.bti",
    "CupName_FLOWER_CUP.bti",
    "CupName_STAR_CUP.bti",
    "CupName_SPECIAL_CUP.bti",
    "CupName_REVERSE2_CUP.bti",
};

const char *SceneCourseSelect::mSelectCupName[] = {
    "SelCourse_Kinoko1.bti",
    "SelCourse_Flower1.bti",
    "SelCourse_Star1.bti",
    "SelCourse_Special1.bti",
    "SelCourse_Reverse2.bti",
};

#define CourseDataBuilder(course_id, course_name) \
    course_id, "CoName_" #course_name ".bti", "COP_" #course_name ".bti" 

const SceneCourseSelect::CourseData SceneCourseSelect::mCourseData[][4] = {
    CourseDataBuilder(LUIGI_CIRCUIT, LUIGI_CIRCUIT),
    CourseDataBuilder(PEACH_BEACH, PEACH_BEACH),
    CourseDataBuilder(BABY_PARK, BABY_PARK),
    CourseDataBuilder(DRY_DRY_DESERT, KARA_KARA_DESERT),
    CourseDataBuilder(MUSHROOM_BRIDGE, KINOKO_BRIDGE),
    CourseDataBuilder(MARIO_CIRCUIT, MARIO_CIRCUIT),
    CourseDataBuilder(DAISY_CRUISER, DAISY_SHIP),
    CourseDataBuilder(WALUIGI_STADIUM, WALUIGI_STADIUM),
    CourseDataBuilder(SHERBET_LAND, SHERBET_LAND),
    MUSHROOM_CITY,  "CoName_KINOKO_CITY.bti", "COP_KONOKO_CITY.bti", 
    CourseDataBuilder(YOSHI_CIRCUIT, YOSHI_CIRCUIT),
    CourseDataBuilder(DK_MOUNTAIN, DK_MOUNTAIN),
    CourseDataBuilder(WARIO_COLOSSEUM, WARIO_COLOSSEUM),
    CourseDataBuilder(DINO_DINO_JUNGLE, DINO_DINO_JUNGLE),
    CourseDataBuilder(BOWSER_CASTLE, BOWSER_CASTLE),
    CourseDataBuilder(RAINBOW_ROAD, RAINBOW_ROAD),
    LUIGI_CIRCUIT, "", "",
    CRS_ID_INV, "", "",
    CRS_ID_INV, "", "",
    CRS_ID_INV, "", "",
};

const u64 SceneCourseSelect::mCourseSelectBckTag[] = {
    'NCo_Pict', 'NWCourse', 'NBest1', 'NCoCurs1', 
    'N_OK', 'CupTro', 'CupName', 'Course01', 
    'Course02', 'Course03', 'Course04', 'EfCoCs01', 
    'EfCoCs02', 'EfCoCs03', 'EfCoCs04', 'EfCoCs05', 
    'EfCoCs06', 'EfCoCs07', 'EfCoCs08', 'EfOKBx01', 
    'EfOKBx02', 'EfOKBx03', 'EfOKBx04', 'EfOKBx05', 
    'EfOKBx06', 'WCourse', 'BestT0', 'BestT1', 
    'BestT2', 'BestT3', 'BestT4', 'BestT5', 
    'BestT6', 'BestL0', 'BestL1', 'BestL2', 
    'BestL3', 'BestL4', 'BestL5', 'BestL6', 
    'Cwhite01', 'Cwhite02',
};

const u64 SceneCourseSelect::mCourseSelectBtkTag[] = {
    'CoCurs1', 'CoOKBox', 'WCo_Pict', 'WCourse', 
};

const u64 SceneCourseSelect::mCourseSelectBpkTag[] = {
    'CoOKB', 'Co_Pict2', 
};

SceneCourseSelect::SceneCourseSelect(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap) {

}

void SceneCourseSelect::reset() {
    for (int i = 0; i < 42; i++)
        mBck[i].mFrame = 0.0f;
    for (int i = 0; i < 4; i++)
        mBtk[i].mFrame = 0.0f;
    mUnderBrk.mFrame = 0.0f;
    for (int i = 0; i < 2; i++)
        mBpk[i].mFrame = 0.0f;
    mCupCursorBck.mFrame = 0.0f;
    mCupCursorBtk.mFrame = 0.0f;
    mCupCursorBrk.mFrame = 0.0f;
    mStateA = 0;
    mStateB = 0;
    mStateC = 0;
    mTimer = 0;
    mTimer2 = 0;
    mTimer3 = 0;
    mNextScene = SCENE_MAX;
    mTimer = 0;
    mGpStage = 0;
    mFade = 600.0f;
    for (int i = 0; i < mCupCount; i++) {
        mPos[i].x = mCupPos[i].x;
        mPos[i].y = mCupPos[i].y;
    }
    mState = 0;
    mField388 = 0;
    mField38c = 0;
    mField394 = 0;
    if (gRaceInfo.getRaceMode() == 1)
        mBck[2].mPane->show();
    else
        mBck[2].mPane->hide();
    mBck[5].mPane->hide();
    mBck[4].mPane->hide();
    for (int i = 0; i < mCupCount; i++) {
        J2DScreen *screen = mScreens[i + 2];
        screen->mScale.x = 1.0f;
        screen->mScale.y = 1.0f;
        screen->calcMtx();
    }

    mCupCount = 3;
    if (gSystemRecord.isGameUnlocked(SystemRecord::SpecialCup))
        mCupCount++;
    if (gSystemRecord.isGameUnlocked(SystemRecord::AllCupTour))
        mCupCount++;

    switch (gRaceInfo.getRaceMode()) {
    case 2:
        if (mMode != 1) {
            mCup = 0;
            mCourse = 0;
        }
        mMode = 1;
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcCupSelect);
        break;
    case 1:
    case 3:
        if (mMode != 2) {
            mCup = 0;
            mCourse = 0;
        }
        mMode = 2;
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcCourseSelect);
        if (mCupCount == 5) {
            mCupCount = 4;
            if (mCup == 4)
                mCup = 0;
        }
        break;
    default:
#line 383
        JUT_ASSERT(0);
        break;
    }

    for (int i = 0; i < mCupCount; i++) {
        mCupPos[i].x = 85.0f + 432.0f / (2.0f * mCupCount) * (2 * i + 1);
        mCupPos[i].y = 140.0f;
    }
    for (int i = 0; i < mCupCount; i++) {
        mPos[i].x = mCupPos[i].x;
        mPos[i].y = mCupPos[i].y;
    }
    calcAnm();
}

void SceneCourseSelect::init() {
    SceneType prevScene = SequenceApp::ptr()->getPrevScene();
    if (gSequenceInfo._28)
        prevScene = SCENE_12;
    gSequenceInfo._28 = 0;

    if (prevScene == SCENE_NONE) {
        reset();
        return;
    }

    u32 id = GetGameAudioMain()->getPlayingSequenceID();
    if (id - 0x1000000 != 1)
        GetGameAudioMain()->startSequenceBgm(0x1000001);
    reset();
    mStateA = mStateB;
    mNextScene = prevScene;

    if (mNextScene == SCENE_12 || mNextScene == SCENE_GHOST_LOAD_SAVE) {
        mNextScene = SCENE_MENU;
        mCup = gSequenceInfo.mMapInfo.cup;
        mCourse = gSequenceInfo.mMapInfo.course;
        mField38c = 0;
        mState = 1;
        f32 zero = 0.0f;
        f32 selX = 301.0f;
        f32 selY = 140.0f;
        for (int i = 0; i < mCupCount; i++) {
            if (mCup != i) {
                J2DScreen *screen = mScreens[i + 2];
                screen->mScale.x = zero;
                screen->mScale.y = zero;
                screen->calcMtx();
            } else {
                mPos[i].x = selX;
                mPos[i].y = selY;
            }
        }
        mBck[2].mFrame = 9.0f;
    }

    SequenceApp::ptr()->ready(mNextScene);
    if (gRaceInfo.getRaceMode() == 1 && !(gSystemRecord.mOptions & 0x8))
        SequenceApp::ptr()->ready(SCENE_GHOST_LOAD_SAVE);

    for (int i = 0; i < 2; i++) {
        if (mCup == 4)
            mCoPict[i]->changeTexture(mCourseData[mGpStage][mCourse].copName, 0);
        else
            mCoPict[i]->changeTexture(mCourseData[mCup][mCourse].copName, 0);
    }

    setTexture();
    calcAnm();
    if (System::getDisplay()->getFader())
        System::getDisplay()->getFader()->startFadeIn(15);
}

SceneCourseSelect::~SceneCourseSelect() {
    delete mScreens[0];
    delete mScreens[1];
}

void SceneCourseSelect::draw() {
    mOrtho->setPort();
    ((J2DScreen *)MenuBackground::ptr())->draw(0.0f, 0.0f, mOrtho);
    MenuTitleLine::ptr()->draw(mOrtho);
    mScreens[0]->draw(0.0f, 0.0f, mOrtho);
    for (int i = 0; i < mCupCount; i++) {
        if (mCup != i)
            mScreens[i + 2]->draw(mFade + mPos[i].x, mPos[i].y, mOrtho);
    }
    mScreens[1]->draw(mFade + mPos[mCup].x, mPos[mCup].y, mOrtho);
    mScreens[mCup + 2]->draw(mFade + mPos[mCup].x, mPos[mCup].y, mOrtho);
}

#define UPD(field, maxv) do { f32 v = 1.0f; v = field + v; field = v; if (v >= maxv) field = 0.0f; } while (0)
void SceneCourseSelect::calc() {
    switch (mStateA) {
    case 0:
        course_init();
        break;
    case 1:
        course();
        break;
    case 2:
        course_end();
        break;
    case 3:
        nextScene();
        break;
    case 4:
        nextRace();
        break;
    default:
#line 528
        JUT_ASSERT(0);
        break;
    }

    MenuBackground::ptr()->calc();
    MenuTitleLine::ptr()->calc();

    UPD(mBck[11].mFrame, 80.0f);
    UPD(mBck[12].mFrame, 80.0f);
    UPD(mBck[13].mFrame, 80.0f);
    UPD(mBck[14].mFrame, 80.0f);
    UPD(mBck[15].mFrame, 80.0f);
    UPD(mBck[16].mFrame, 80.0f);
    UPD(mBck[17].mFrame, 80.0f);
    UPD(mBck[18].mFrame, 80.0f);
    UPD(mBck[19].mFrame, 80.0f);
    UPD(mBck[20].mFrame, 80.0f);
    UPD(mBck[21].mFrame, 80.0f);
    UPD(mBck[22].mFrame, 80.0f);
    UPD(mBck[23].mFrame, 80.0f);
    UPD(mBck[24].mFrame, 80.0f);
    UPD(mBtk[0].mFrame, 60.0f);
    UPD(mBtk[1].mFrame, 120.0f);
    UPD(mBtk[2].mFrame, 180.0f);
    UPD(mBtk[3].mFrame, 180.0f);
    UPD(mBpk[0].mFrame, 180.0f);

    for (int i = 0; i < 42; i++)
        mBck[i].mAnm->mCurrentFrame = mBck[i].mFrame;
    for (int i = 0; i < 4; i++)
        mBtk[i].mAnm->mCurrentFrame = mBtk[i].mFrame;
    mUnderBrk.mAnm->mCurrentFrame = mUnderBrk.mFrame;
    for (int i = 0; i < 2; i++)
        mBpk[i].mAnm->mCurrentFrame = mBpk[i].mFrame;

    mScreens[0]->animation();

    UPD(mCupCursorBck.mFrame, 80.0f);
    UPD(mCupCursorBtk.mFrame, 120.0f);
    mCupCursorBck.mAnm->mCurrentFrame = mCupCursorBck.mFrame;
    mCupCursorBtk.mAnm->mCurrentFrame = mCupCursorBtk.mFrame;
    mCupCursorBrk.mAnm->mCurrentFrame = mCupCursorBrk.mFrame;

    mScreens[1]->animation();

    register f32 frame0 = mBck[0].mFrame;
    register f32 resetFrame = 10.0f;
    if (resetFrame != frame0) {
        f32 frame = 1.0f + frame0;
        mBck[0].mFrame = frame;
        f32 maxFrame = 16.0f;
        if (maxFrame == frame)
            mBck[0].mFrame = resetFrame;
    }

    if (mTimer2 < 21)
        mBpk[1].mFrame = mTimer2;
    else
        mBpk[1].mFrame = 0.0f;

    if (mCup == 4)
        mCoPict[1]->changeTexture(mCourseData[mGpStage][mCourse].copName, 0);
    else
        mCoPict[1]->changeTexture(mCourseData[mCup][mCourse].copName, 0);

    mTimer++;

    if (mMode == 1) {
        int timer2 = mTimer2 + 1;
        mTimer2 = timer2;
        if (timer2 >= 90) {
            mCourse = (mCourse + 1) % 4;
            mTimer2 = 0;
            if (mCup == 4)
                mCoPict[0]->changeTexture(mCourseData[mGpStage][mCourse].copName, 0);
            else
                mCoPict[0]->changeTexture(mCourseData[mCup][mCourse].copName, 0);
        }
        int timer3 = mTimer3 + 1;
        mTimer3 = timer3;
        if (timer3 >= 360) {
            mGpStage = (mGpStage + 1) % 4;
            mTimer3 = 0;
            if (mCup == 4)
                mCoPict[0]->changeTexture(mCourseData[mGpStage][mCourse].copName, 0);
            else
                mCoPict[0]->changeTexture(mCourseData[mCup][mCourse].copName, 0);
        }
    }
}
#undef UPD

void SceneCourseSelect::course_init() {
    if (mTimer < 15) {
        mBck[1].mPane->show();
        if (mTimer < 10)
            mBck[1].mFrame = mTimer;
        mFade = 600.0f - 600.0f * mTimer / 15.0f;
        if (mMode == 1) {
            MenuTitleLine::ptr()->drop(MenuTitleLine::mcCupSelect);
            mBck[5].mPane->show();
            mBck[4].mPane->show();
            mBck[5].mFrame = 0.0f;
            mBck[4].mFrame = 0.0f;
            mScreens[0]->search('NRandom')->hide();
        } else {
            MenuTitleLine::ptr()->drop(MenuTitleLine::mcCourseSelect);
            if (gRaceInfo.getRaceMode() != 1)
                mScreens[0]->search('NRandom')->show();
            else
                mScreens[0]->search('NRandom')->hide();
        }
    } else {
        mStateB = mStateA;
        mStateA = 1;
        mTimer = 0;
        mTimer2 = 0;
        mTimer3 = 0;
        mFade = 0.0f;
    }
    mBck[0].mFrame = 10.0f;
    mBpk[1].mFrame = 0.0f;
}

void SceneCourseSelect::course() {
    if (mMode == 2 && mState == 0 && gRaceInfo.getRaceMode() != 1)
        mScreens[0]->search('NRandom')->show();
    else
        mScreens[0]->search('NRandom')->hide();

    if (mField394 != 0) {
        if (mField388 == 0) {
        mField394++;
        switch (mState) {
        case 0: {
            u32 button;
            bool start = false;
            button = gpaGamePad[0]->getButton();
            if ((button & 0x20) && (button & 0x40))
                start = true;
            if (start || mField394 < 30)
                rndRoulette();
            else {
                buttonA();
                GetGameAudioMain()->startSystemSe(0x20028);
                mField394 = 1;
            }
            break;
        }
        case 1:
            if (mField394 < 3)
                rndRoulette();
            else {
                buttonA();
                mField394 = 0;
            }
            break;
        default:
#line 706
            JUT_ASSERT(0);
            break;
        }
        }
    } else if (mField388 == 0 && mBck[0].mFrame == 9.0f) {
        RaceInfo *raceInfo = &gRaceInfo;
        u32 trigger = gpaGamePad[0]->getTrigger();
        if ((trigger & 0x100) && System::getDisplay()->getFader()->getStatus() == 1) {
            buttonA();
            if (mState == 2)
                GetGameAudioMain()->startSystemSe(0x20002);
            else
                GetGameAudioMain()->startSystemSe(0x20003);
        } else if ((trigger & 0x200) && System::getDisplay()->getFader()->getStatus() == 1) {
            buttonB();
        } else {
            u32 repeat = gpaGamePad[0]->getRepeat();
            if (repeat & 0x08000000) {
                if (mMode == 2 && mState == 1) {
                    GetGameAudioMain()->startSystemSe(0x20000);
                    mCourse = (mCourse + 3) % 4;
                    mBck[0].mFrame = 1.0f;
                }
            } else if (repeat & 0x04000000) {
                if (mMode == 2 && mState == 1) {
                    GetGameAudioMain()->startSystemSe(0x20000);
                    mCourse = (mCourse + 1) % 4;
                    mBck[0].mFrame = 1.0f;
                }
            } else if (repeat & 0x01000000) {
                if (mState == 0) {
                    GetGameAudioMain()->startSystemSe(0x20000);
                    int cupCount = mCupCount;
                    int cup;
                    cup = mCup;
                    cup += cupCount;
                    mCup = (cup - 1) % cupCount;
                    mGpStage = 0;
                    mCourse = 0;
                    mTimer2 = 0;
                    mTimer3 = 0;
                    mBck[0].mFrame = 1.0f;
                }
            } else if (repeat & 0x02000000) {
                if (mState == 0) {
                    GetGameAudioMain()->startSystemSe(0x20000);
                    mCup = (mCup + 1) % mCupCount;
                    mGpStage = 0;
                    mCourse = 0;
                    mTimer2 = 0;
                    mTimer3 = 0;
                    mBck[0].mFrame = 1.0f;
                }
            } else {
                u32 button = gpaGamePad[0]->getButton();
                if ((button & 0x20) && (button & 0x40)) {
                    if (mMode == 2) {
                        if (mState != 0) {
                            if (mState < 0 || mState >= 3) {
#line 779
                                JUT_ASSERT(0);
                            }
                        } else {
                            if (raceInfo->getRaceMode() != 1)
                                mField394 = 1;
                        }
                    }
                }
            }
        }
    }
    calcAnm();
}

void SceneCourseSelect::buttonA() {
    if (mState == 2) {
        if (mBck[4].mFrame != 9.0f)
            return;

        if (gRaceInfo.getRaceMode() != 1 || (gSystemRecord.mOptions & 0x8)) {
            System::getDisplay()->startFadeOut(15);
            GetGameAudioMain()->fadeOutAll(15);
        }

        if (gRaceInfo.getRaceMode() == 1 && !(gSystemRecord.mOptions & 0x8)) {
            mStateC = 3;
            mNextScene = SCENE_GHOST_LOAD_SAVE;
            gSequenceInfo.setGhostFlag(1);
        } else {
            mStateC = 4;
        }
        mStateA = 2;
        mStateB = 1;
        mTimer = 0;

        if (mMode == 1) {
            if (gRaceInfo.getStatusNumber() < 2)
                ResMgr::loadCourseData(mCourseData[mCup][0].courseID, ResMgr::mcOrder_2);
            else
                ResMgr::loadCourseData(mCourseData[mCup][0].courseID, ResMgr::mcOrder_1);
            gSequenceInfo.mMapInfo.cup = mCup;
            gSequenceInfo.mMapInfo.course = 0;
        } else if (mMode == 2) {
            if (gRaceInfo.getStatusNumber() < 2)
                ResMgr::loadCourseData(mCourseData[mCup][mCourse].courseID, ResMgr::mcOrder_2);
            else
                ResMgr::loadCourseData(mCourseData[mCup][mCourse].courseID, ResMgr::mcOrder_1);
            gSequenceInfo.mMapInfo.cup = mCup;
            gSequenceInfo.mMapInfo.course = mCourse;
        }
    } else {
        mField388 = 1;
        mField38c = 0;
    }
}

void SceneCourseSelect::buttonB() {
    if (mState == 0) {
        GetGameAudioMain()->startSystemSe(0x20005);
        mStateC = 3;
        mStateA = 2;
        mStateB = 1;
        mTimer = 0;
    } else {
        GetGameAudioMain()->startSystemSe(0x20005);
        mField388 = 1;
        mField38c = 1;
    }
}

void SceneCourseSelect::course_end() {
    if (mState == 0) {
        mBck[2].mPane->hide();
    } else if (gRaceInfo.getRaceMode() == 1) {
        mBck[2].mPane->show();
    } else {
        mBck[2].mPane->hide();
    }

    if (mTimer < 15) {
        MenuTitleLine::ptr()->lift();
        if (mTimer > 6) {
            mBck[1].mFrame = 15 - mTimer;
            mBck[2].mFrame = 15 - mTimer;
            if ((mBck[5].mFrame -= 1.0f) < 0.0f)
                mBck[5].mFrame = 0.0f;
            if ((mBck[4].mFrame -= 1.0f) < 0.0f)
                mBck[4].mFrame = 0.0f;
        }
        mFade = -600.0f * mTimer / 15.0f;
    } else {
        mBck[1].mPane->hide();
        mStateB = mStateA;
        mStateA = mStateC;
        mTimer = 0;
        mFade = -600.0f;
    }
}

void SceneCourseSelect::nextScene() {
    if (mNextScene == SCENE_GHOST_LOAD_SAVE) {
        if (!ResMgr::isFinishLoadingArc(ResMgr::mcArcCourse) || !SequenceApp::ptr()->checkFinishAllLoading())
            return;
    }
    if (!SequenceApp::ptr()->ready(mNextScene))
        return;
    SequenceApp::ptr()->setNextScene(mNextScene);
}

void SceneCourseSelect::nextRace() {
    if (ResMgr::isFinishLoadingArc(ResMgr::mcArcCourse) && SequenceApp::ptr()->checkFinishAllLoading()) {
        gSequenceInfo.setRandomSeed();
        if (mMode == 1)
            setRivalKart();
        else
            setStartNo();

        AppMgr::msRequest |= 1;
        for (int kart = 0; kart < gRaceInfo.getKartNumber(); kart++) {
#line 163
            JUT_MINMAX_ASSERT(0, kart, 8);
            gRaceInfo.setStartPosIdx(kart, gSequenceInfo.mStartNo[kart]);
#line 226
            JUT_MINMAX_ASSERT(0, kart, 8);
            gRaceInfo.setPointTable(kart, gSequenceInfo.mRankPoint[kart]);
        }
        if (gRaceInfo.getRaceMode() == 2)
            gRaceInfo.mGpStageNo = gSequenceInfo.mMapInfo.course;
        int vsLapNum = gSystemRecord.mVsLapNum;
        int itemSlotType = gSystemRecord.mItemSlotType;
        gRaceInfo.mItemSlotType = itemSlotType;
        gRaceInfo.mVsLapNum = vsLapNum;
        gRaceInfo.mGpCup = mRaceGpCup[mCup];
        RaceApp::call();
    }
}

void SceneCourseSelect::calcAnm() {
    if (mBck[0].mFrame == 13.0f)
        setTexture();

    for (int i = 0; i < mCupCount; i++) {
        JUtility::TColor color = mCupColor[i == mCup ? 1 : 0];
        mCupPict[i]->setWhite(color);
    }

    if (mMode == 1) {
        mBck[3].mPane->hide();
        for (int i = 0; i < 4; i++) {
            JUtility::TColor color = mCupColor[1];
            mCupIcon[i]->setWhite(color);
        }
    }
    else if (mState == 0) {
        mBck[3].mPane->hide();
        for (int i = 0; i < 4; i++) {
            JUtility::TColor color = mCupColor[0];
            mCupIcon[i]->setWhite(color);
        }
    }
    else {
        mBck[3].mPane->show();
        for (int i = 0; i < 4; i++) {
            JUtility::TColor color = mCupColor[i == mCourse ? 1 : 0];
            mCupIcon[i]->setWhite(color);
        }
    }
    mBck[3].mFrame = mCourse;

    if (mField388 != 0) {
        mField388++;
        if (mField38c == 0) {
            switch (mState) {
            case 0:
                if (mMode == 1) {
                    if ((f32)mField388 > 15.0f) {
                        mState = 2;
                        mField388 = 0;
                    }
                }
                else if ((f32)mField388 > 15.0f) {
                    mState = 1;
                    mField388 = 0;
                }
                break;
            case 1:
                if (mMode == 1) {
#line 1041
                    JUT_ASSERT(0);
                }
                else {
                    mState = 2;
                    mField388 = 0;
                }
                break;
            default:
#line 1047
                JUT_ASSERT(0);
                break;
            }
        }
        else {
            switch (mState) {
            case 1:
                if (mMode == 1) {
#line 1054
                    JUT_ASSERT(0);
                }
                else if ((f32)mField388 > 15.0f) {
                    mState = 0;
                    mField388 = 0;
                }
                break;
            case 2:
                if (mMode == 1) {
                    if ((f32)mField388 > 15.0f) {
                        mState = 0;
                        mField388 = 0;
                    }
                }
                else {
                    mState = 1;
                    mField388 = 0;
                }
                break;
            default:
#line 1054
                JUT_ASSERT(0);
                break;
            }
        }
    }

    if (mField388 != 0) {
        f32 t;
        if (mField38c == 0)
            t = 1.0f - (f32)mField388 / 15.0f;
        else
            t = (f32)mField388 / 15.0f;

        for (int i = 0; i < mCupCount; i++) {
            if (i != mCup) {
                mScreens[2 + i]->mScale.x = t;
                mScreens[2 + i]->mScale.y = t;
                mScreens[2 + i]->calcMtx();
            }
            else {
                f32 x = mCupPos[i].x;
                f32 y = mCupPos[i].y;
                f32 dx = x - 301.0f;
                f32 dy = y - 140.0f;
                mPos[i].x = 301.0f + dx * t;
                mPos[i].y = 140.0f + dy * t;
            }
        }

        f32 f = 9.0f * (1.0f - t);
        mBck[2].mFrame = f;
        if (mMode == 1) {
            mBck[4].mFrame = f;
            mBck[5].mFrame = f;
        }
    }

    if (mMode == 1) {
        GPRecord rec = gSystemRecord.getGPRecord(mRaceGpCup[mCup], gRaceInfo.getRaceLevel());
        if (rec.isValid() && rec.mRank < 3) {
            J2DPicture *pane = mBck[5].mPane;
            pane->show();
#line 1114
            JUT_ASSERT_MSG(pane->getTypeID() == J2DPane_Picture, "pane->getTypeID() == J2DPane_Picture");
            pane->changeTexture((const ResTIMG *)Cup2D::ptr()->getCupTexture(mRaceGpCup[mCup], rec.mRank), 0);
        }
        else {
            mBck[5].mPane->hide();
        }
    }
    else if (mState == 2) {
        mBck[4].mPane->show();
        f32 f = mBck[4].mFrame + 1.0f;
        mBck[4].mFrame = f;
        if (f > 9.0f)
            mBck[4].mFrame = 9.0f;
    }
    else {
        f32 f = mBck[4].mFrame - 1.0f;
        mBck[4].mFrame = f;
        if (f < 0.0f)
            mBck[4].mFrame = 0.0f;
        if (mBck[4].mFrame == 0.0f)
            mBck[4].mPane->hide();
    }
}

void SceneCourseSelect::setTexture() {
    mPane2c0->changeTexture(mCupName[mCup], 0);

    if (mMode == 1) {
        if (mCup == 4) {
            for (int i = 0; i < 4; i++)
                mCupIcon[i]->changeTexture(mCupName[i], 0);
            for (int i = 0; i < 2; i++)
                mCoPict[i]->changeTexture(mCourseData[mGpStage][mCourse].copName, 0);
        }
        else {
            for (int i = 0; i < 4; i++)
                mCupIcon[i]->changeTexture(mCourseData[mCup][i].coName, 0);
            for (int i = 0; i < 2; i++)
                mCoPict[i]->changeTexture(mCourseData[mCup][mCourse].copName, 0);
        }
    }
    else {
        for (int i = 0; i < 4; i++)
            mCupIcon[i]->changeTexture(mCourseData[mCup][i].coName, 0);
        mCoPict[1]->changeTexture(mCourseData[mCup][mCourse].copName, 0);
    }

    const ResTIMG *timg = (const ResTIMG *)J2DScreen::getNameResource(mSelectCupName[mCup]);
    ((JUTTexture *)mPane2f8)->storeTIMG(timg, (u8)0);
    mUnderBrk.mFrame = mCup;

    if (gRaceInfo.getRaceMode() == TIME_ATTACK) {
        Kart2DCommon *kart = Kart2DCommon::ptr();
        ECourseID courseID = mCourseData[mCup][mCourse].courseID;

        TARecord record;
        int min, sec, ms;

        record = *gSystemRecord.getTARecord(courseID, 0);
        if (record.isValid()) {
            if (record.getRaceTime().isAvailable()) {
                record.getRaceTime().get(&min, &sec, &ms);
                kart->changeNumberTexture(min, &mBestL[5], 2, false, false);
                kart->changeNumberTexture(sec, &mBestL[3], 2, true, false);
                kart->changeNumberTexture(ms, &mBestL[0], 3, true, false);
            }
            else {
                kart->changeNumberTexture(0, &mBestL[5], 2, true, true);
                kart->changeNumberTexture(0, &mBestL[3], 2, true, true);
                kart->changeNumberTexture(0, &mBestL[0], 3, true, true);
            }
        }
        else {
            kart->changeNumberTexture(0, &mBestL[5], 2, true, true);
            kart->changeNumberTexture(0, &mBestL[3], 2, true, true);
            kart->changeNumberTexture(0, &mBestL[0], 3, true, true);
        }

        record = *gSystemRecord.getBestLap(courseID);
        if (record.isValid()) {
            if (record.getRaceTime().isAvailable()) {
                record.getRaceTime().get(&min, &sec, &ms);
                kart->changeNumberTexture(min, &mBestT[5], 2, false, false);
                kart->changeNumberTexture(sec, &mBestT[3], 2, true, false);
                kart->changeNumberTexture(ms, &mBestT[0], 3, true, false);
            }
            else {
                kart->changeNumberTexture(0, &mBestT[5], 2, true, true);
                kart->changeNumberTexture(0, &mBestT[3], 2, true, true);
                kart->changeNumberTexture(0, &mBestT[0], 3, true, true);
            }
        }
        else {
            kart->changeNumberTexture(0, &mBestT[5], 2, true, true);
            kart->changeNumberTexture(0, &mBestT[3], 2, true, true);
            kart->changeNumberTexture(0, &mBestT[0], 3, true, true);
        }
    }
}

void SceneCourseSelect::setRivalKart() {
    int num = gRaceInfo.getPlayerKartNumber();
    u32 order[8];
    for (int i = 0; i < 8; i++)
        order[i] = i;

    int count = 8 - num;
    for (int i = 0; i < count; i++) {
        int j = i + gSequenceInfo.getRandom() % (count - i);
        u32 tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }

    int rank[4][3] = {
        {0, 0, 0},
        {0, 1, 1},
        {0, 1, 2},
        {0, 1, 2},
    };
    int *prank = rank[gRaceInfo.getRaceLevel()];

    for (int i = 0; i < count; i++) {
        for (int j = count - 1; j > i; j--) {
            int kart1 = order[i] + num;
            int kart2 = order[j] + num;
            EKartID id1 = gRaceInfo.getKartInfo(kart1)->getKartID();
            EKartID id2 = gRaceInfo.getKartInfo(kart2)->getKartID();
            if (prank[KartInfo::getKartWeight(id1)] < prank[KartInfo::getKartWeight(id2)]) {
                u32 tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }

    for (int i = 0; i < 2; i++)
        gRaceInfo.setRivalKartNo(i, order[i] + num);
}

void SceneCourseSelect::setStartNo() {
    int num = gRaceInfo.getKartNumber();
    u32 order[8];
    for (int i = 0; i < num; i++)
        order[i] = i;
    for (int i = 0; i < num; i++) {
        int j = i + gSequenceInfo.getRandom() % (num - i);
        u32 tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }
    for (int i = 0; i < num; i++)
        gSequenceInfo.setStartNo(i, order[i]);
}

void SceneCourseSelect::rndRoulette() {
    u32 time = OSGetTime();
    switch (mState) {
    case 0:
        if (mTimer % 7 == 0) {
            GetGameAudioMain()->startSystemSe(0x20000);
            int cup = mCup;
            mCup = (int)(cup + time % (mCupCount - 1) + 1) % mCupCount;
            mBck[0].mFrame = 1.0f;
        }
        break;
    case 1: {
        mCourse = (int)(mCourse + time % 3 + 1) % 4;
        mBck[0].mFrame = 1.0f;
    } break;
    default:
#line 1329
        JUT_ASSERT(0);
        break;
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
