#include "Kameda/SceneMap.h"
#include "Kameda/Scene.h"
#include "Kameda/SceneCourseSelect.h"
#include "Kameda/SequenceInfo.h"
#include "Kaneshige/KartInfo.h"
#include "Kaneshige/RaceInfo.h"
#include "Osako/AppMgr.h"
#include "Osako/AwardApp.h"
#include "Osako/RaceApp.h"
#include "Osako/ResMgr.h"
#include "Osako/SequenceApp.h"
#include "Osako/SystemFile.h"
#include "Osako/SystemRecord.h"
#include "kartEnums.h"
#include "mathHelper.h"

SceneMap::SceneMap(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap) {
    init();
}

void SceneMap::init() { _c = 0; }

SceneMap::~SceneMap() {}

void SceneMap::draw() {}

void SceneMap::calc() {
    switch (_c) {
    case 0:
        map();
        break;
    case 1:
        nextScene();
        break;
    case 2:
        nextRace();
        break;
    case 3:
        nextAward();
        break;
    case 4:
        nextEnding();
        break;
    case 5:
        nextTrueEnding();
        break;
    default:
#line 108
        JUT_ASSERT(0);
        break;
    }
}

void SceneMap::map() {
    switch (gSequenceInfo._364) {
    case 0:
        if (!gSequenceInfo.isEndGP()) {
            int cup = gSequenceInfo.mMapInfo.cup;
            int course = gSequenceInfo.mMapInfo.course;
            int row = cup;
            int col = course;
            if (cup == ALL_CUP_TOUR) {
                int idx = gSequenceInfo.get_3cc(course);
                row = idx / 4;
                col = idx % 4;
            }
            _c = 2;
            if (gRaceInfo.getStatusNumber() < 2)
                ResMgr::loadCourseData(SceneCourseSelect::mCourseData[row][col].courseID, ResMgr::mcOrder_2);
            else
                ResMgr::loadCourseData(SceneCourseSelect::mCourseData[row][col].courseID, ResMgr::mcOrder_1);
        }
        else {
            _c = 3;
            ResMgr::loadCourseData(AWARD, ResMgr::mcOrder_2);
        }
        break;
    case 1:
        if (gSequenceInfo._360)
            gSystemFile._6024 = 1;
        if (gSequenceInfo._35e) {
            _c = 4;
            ResMgr::loadCourseData(ENDING, ResMgr::mcOrder_2);
        }
        else if (gSequenceInfo._35f) {
            _c = 5;
            ResMgr::loadCourseData(ENDING, ResMgr::mcOrder_2);
        }
        else {
            _c = 1;
            gSequenceInfo._364 = 0;
            if (gSequenceInfo._35d)
                mNextScene = SCENE_SECRET;
            else
                mNextScene = SCENE_TITLE;
        }
        break;
    case 2:
        _c = 1;
        gSequenceInfo._364 = 0;
        if (gSequenceInfo._35d)
            mNextScene = SCENE_SECRET;
        else
            mNextScene = SCENE_TITLE;
        break;
    default:
#line 182
        JUT_ASSERT(0);
        break;
    }
}

void SceneMap::nextScene() {
    if (SequenceApp::ptr()->ready(mNextScene)) {
        SequenceApp::ptr()->setNextScene(mNextScene);
    }
}

void SceneMap::nextRace() {
    if (ResMgr::isFinishLoadingArc(ResMgr::mcArcCourse) && SequenceApp::ptr()->checkFinishAllLoading()) {
        AppMgr::deleteCurrentApp();
        for (int i = 0; i < gRaceInfo.getKartNumber(); i++) {
            gRaceInfo.setStartPosIdx(i, gSequenceInfo.getStartNo(i));
            gRaceInfo.setPointTable(i, gSequenceInfo.getRankPoint(i));
        }
        if (gRaceInfo.getRaceMode() == GRAND_PRIX)
            gRaceInfo.mGpStageNo = gSequenceInfo.mMapInfo.course;
        u16 vs = gSystemRecord.mVsLapNum;
        gRaceInfo.mItemSlotType = gSystemRecord.mItemSlotType;
        gRaceInfo.mVsLapNum = vs;
        gRaceInfo.mGpCup = SceneCourseSelect::mRaceGpCup[gSequenceInfo.mMapInfo.cup];
        RaceApp::call();
    }
}

void SceneMap::nextAward() {
    if (ResMgr::isFinishLoadingArc(ResMgr::mcArcCourse) && SequenceApp::ptr()->checkFinishAllLoading()) {
        AppMgr::deleteCurrentApp();
        EKartID kartIDs[8];
        ECharID charIDs[8][2];
        ERaceLevel level = gRaceInfo.getRaceLevel();
        ERaceGpCup cup = SceneCourseSelect::mRaceGpCup[gSequenceInfo.mMapInfo.cup];

        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 2; j++) {
                charIDs[i][j] = gRaceInfo.getKartInfo(i)->getDriverCharID(j);
            }
            kartIDs[i] = gRaceInfo.getKartInfo(i)->getKartID();
        }

        int rank = gSequenceInfo.getPlayerRankGP();
        gSequenceInfo.setSecretFlag();

        if ((cup == ALL_CUP_TOUR || cup == SPECIAL_CUP) && rank == 0 &&
            gSystemRecord.isGameUnlocked(SystemRecord::SpecialEnding))
            gSequenceInfo._35f = 1;
        else if ((cup == ALL_CUP_TOUR || cup == SPECIAL_CUP) && rank >= 0 && rank < 3)
            gSequenceInfo._35e = 1;

        gRaceInfo.setRace(AWARD_DEMO, 3, 0, 1, 1);
        gRaceInfo.mDemoNextPhase = (ERacePhase)11;
        gSequenceInfo._364 = 1;
        gRaceInfo.mGpCup = cup;
        gRaceInfo.setRaceLevel(level);
        if (rank >= 0 && rank < 3)
            gRaceInfo.mAwardKartNo = rank;

        for (int i = 0; i < gRaceInfo.getKartNumber(); i++) {
            gRaceInfo.setKart(i, i != 0 ? kartIDs[gSequenceInfo.get_2d8(i)] : PARADE_KART,
                              charIDs[gSequenceInfo.get_2d8(i)][0], 0,
                              charIDs[gSequenceInfo.get_2d8(i)][1], 0);
        }
        AwardApp::call();
    }
}

void SceneMap::nextEnding() {
    if (ResMgr::isFinishLoadingArc(ResMgr::mcArcCourse) && SequenceApp::ptr()->checkFinishAllLoading()) {
        AppMgr::deleteCurrentApp();
        gRaceInfo.settingForStaffRoll(false);
        gRaceInfo.mDemoNextPhase = (ERacePhase)11;
        gSequenceInfo._364 = 2;
        RaceApp::call();
    }
}

void SceneMap::nextTrueEnding() {
    if (ResMgr::isFinishLoadingArc(ResMgr::mcArcCourse) && SequenceApp::ptr()->checkFinishAllLoading()) {
        AppMgr::deleteCurrentApp();
        gRaceInfo.settingForStaffRoll(true);
        gRaceInfo.mDemoNextPhase = (ERacePhase)11;
        gSequenceInfo._364 = 2;
        RaceApp::call();
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
