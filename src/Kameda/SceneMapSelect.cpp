#include "JSystem/J3D/J3DPacket.h"
#include "Kameda/SceneMapSelect.h"
#include "Kameda/BattleName2D.h"
#include "Kaneshige/RaceInfo.h"
#include "Kameda/Scene.h"
#include "Kameda/SceneFactory.h"
#include "Kameda/SequenceInfo.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "JSystem/JUtility/JUTAssert.h"
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

const SceneMapSelect::MapData SceneMapSelect::mMapData[] = {
    MINI_7, "BattleMapSnap1.bti", "Mozi_Map1.bti",
    MINI_2, "BattleMapSnap3.bti", "Mozi_Map3.bti",
    MINI_3, "BattleMapSnap2.bti", "Mozi_Map2.bti",
    MINI_8, "BattleMapSnap4.bti", "Mozi_Map4.bti",
    MINI_1, "BattleMapSnap6.bti", "Mozi_Map6.bti",
    MINI_5, "BattleMapSnap5.bti", "Mozi_Map5.bti",
};

const u64 SceneMapSelect::mMapSelectBckTag[3] = {
    'NSlMap', 'OK_wb11', 'NRandom', 
};

const u64 SceneMapSelect::mMapBckTag[3] = {
    'NMap', 'Name', 'HighL', 
};

const u64 SceneMapSelect::mMapBrkTag[2] = {
    'MapPict', 'Name',
};

const u64 SceneMapSelect::mMapBpkTag = 'HighL';

SceneMapSelect::SceneMapSelect(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap) {
    mMainScreen = new (heap, 0) J2DScreen();
    for (int i = 0; i < 6; i++)
        mScreens[i] = new (heap, 0) J2DScreen();

    MenuBackground::create(SceneFactory::ptr()->getArchive(9));
    MenuTitleLine::create(SceneFactory::ptr()->getArchive(8), nullptr);

    mMainScreen->set("SelectMapLayout.blo", 0x104, mArchive);
    for (int i = 0; i < 6; i++)
        mScreens[i]->set("SelectMap1.blo", 0x104, mArchive);

    J2DScreen **screens = mScreens;
    const u64 *bckTag = mMapBckTag;
    const u64 *brkTag = mMapBrkTag;
    J2DAnmColor **bpkAnim = &mMapBpk[0].mAnm;
    J2DPicture **bpkPane = &mMapBpk[0].mPane;

    for (int i = 0; i < 3; i++) {
        mMainBck[i].mPane = (J2DPicture *)mMainScreen->search(mMapSelectBckTag[i]);
        mMainBck[i].mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("SelectMapLayout.bck", mArchive));
        mMainBck[i].mPane->setAnimation(mMainBck[i].mAnm);
    }

    mMainBtk.mAnm = (J2DAnmTextureSRTKey *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("SelectMapLayout.btk", mArchive));
    mMainBtk.mAnm->searchUpdateMaterialID(mMainScreen);
    mMainScreen->setAnimation(mMainBtk.mAnm);

    mMainBpk.mAnm = (J2DAnmColor *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("SelectMapLayout.bpk", mArchive));
    mMainBpk.mAnm->searchUpdateMaterialID(mMainScreen);
    mMainScreen->setAnimation(mMainBpk.mAnm);

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 3; j++) {
            mMapBck[i][j].mPane = (J2DPicture *)screens[i]->search(bckTag[j]);
            mMapBck[i][j].mAnm = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("SelectMap1.bck", mArchive));
            mMapBck[i][j].mPane->setAnimation(mMapBck[i][j].mAnm);
        }
        for (int j = 0; j < 2; j++) {
            mMapBrk[i][j].mPane = (J2DPicture *)screens[i]->search(brkTag[j]);
            mMapBrk[i][j].mAnm = (J2DAnmTevRegKey *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("SelectMap1.brk", mArchive));
            mMapBrk[i][j].mAnm->searchUpdateMaterialID(screens[i]);
            mMapBrk[i][j].mPane->setAnimation(mMapBrk[i][j].mAnm);
        }
        bpkPane[i * 3] = (J2DPicture *)screens[i]->search(*(const u64 *)&mMapBpkTag);
        bpkAnim[i * 3] = (J2DAnmColor *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("SelectMap1.bpk", mArchive));
        bpkAnim[i * 3]->searchUpdateMaterialID(screens[i]);
        bpkPane[i * 3]->setAnimation(bpkAnim[i * 3]);
    }

    mMapIndex = 0;
    reset();
}

void SceneMapSelect::reset() {
    mMapCount = 4;
    if (gSystemRecord.isGameUnlocked(SystemRecord::MiniLuigi))
        mMapCount++;
    if (gSystemRecord.isGameUnlocked(SystemRecord::MiniMario))
        mMapCount++;
    if (mMapCount < 4 || mMapCount > 6) {
#line 151
        JUT_ASSERT(0);
    }

    if (mMapCount == 4) {
        for (int i = 0; i < 4; i++) {
            J2DPicture *pane2 = (J2DPicture *)mMainScreen->search('SetA1' + (u64)i);
            mPos[i].x = pane2->mOffset.x;
            mPos[i].y = pane2->mOffset.y;
        }
    }
    else {
        for (int i = 0; i < 6; i++) {
            J2DPicture *pane2 = (J2DPicture *)mMainScreen->search('Set1' + (u64)i);
            mPos[i].x = pane2->mOffset.x;
            mPos[i].y = pane2->mOffset.y;
        }
    }

    for (int i = 0; i < 6; i++) {
        J2DPicture *pane2 = (J2DPicture *)mScreens[i]->search('MapPict');
#line 175
        JUT_ASSERT_MSG(pane2->getTypeID() == J2DPane_Picture, "pane2->getTypeID() == J2DPane_Picture");
        pane2->changeTexture(mMapData[i].coName, 0);
        pane2 = mMapBck[i][1].mPane;
#line 179
        JUT_ASSERT_MSG(pane2->getTypeID() == J2DPane_Picture, "pane2->getTypeID() == J2DPane_Picture");
        pane2->changeTexture(mMapData[i].copName, 0);
    }

    if (mMapCount == 5 && gSystemRecord.isGameUnlocked(SystemRecord::MiniMario)) {
        J2DPicture *pane2 = (J2DPicture *)mScreens[4]->search('MapPict');
#line 186
        JUT_ASSERT_MSG(pane2->getTypeID() == J2DPane_Picture, "pane2->getTypeID() == J2DPane_Picture");
        pane2->changeTexture(mMapData[5].coName, 0);
        pane2 = mMapBck[4][1].mPane;
#line 190
        JUT_ASSERT_MSG(pane2->getTypeID() == J2DPane_Picture, "pane2->getTypeID() == J2DPane_Picture");
        pane2->changeTexture(mMapData[5].copName, 0);
    }

    J2DPicture *pane2 = (J2DPicture *)mMainScreen->search('SubM');
#line 197
    JUT_ASSERT_MSG(pane2->getTypeID() == J2DPane_Picture, "pane2->getTypeID() == J2DPane_Picture");
    pane2->changeTexture(BattleName2D::ptr()->getBattleNameTexture((BattleName2D::BattleName)gSequenceInfo.mBattleType), 0);

    pane2 = (J2DPicture *)mMainScreen->search('BtlPict');
#line 203
    JUT_ASSERT_MSG(pane2->getTypeID() == J2DPane_Picture, "pane2->getTypeID() == J2DPane_Picture");
    switch (gRaceInfo.getRaceMode()) {
    case BALLOON_BATTLE:
        pane2->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(0), 0);
        break;
    case BOMB_BATTLE:
        pane2->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(1), 0);
        break;
    case ESCAPE_BATTLE:
        pane2->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(2), 0);
        break;
    default:
        break;
    }

    for (int i = 0; i < 3; i++)
        mMainBck[i].mFrame = 0.0f;
    mMainBtk.mFrame = 0.0f;
    mMainBpk.mFrame = 0.0f;

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 3; j++)
            mMapBck[i][j].mFrame = 0.0f;
        for (int j = 0; j < 2; j++)
            mMapBrk[i][j].mFrame = 0.0f;
        (*(BpkData *)((u8 *)mMapBpk + i * 12)).mFrame = 0.0f;
    }

    mState = 0;
    mNextStateA = 0;
    mNextStateB = 0;
    mTimer = 0;
    mNextScene = SCENE_MAX;
    mTimer = 0;
    mField264 = 0;
    mConfirm = 0;
    calcAnm();
}

void SceneMapSelect::init() {
    SceneType prev = SequenceApp::ptr()->getPrevScene();
    if (gSequenceInfo._28)
        prev = SCENE_12;
    gSequenceInfo._28 = 0;

    if (prev == SCENE_NONE) {
        reset();
        return;
    }

    if (GetGameAudioMain()->getPlayingSequenceID() != 0x1000001)
        GetGameAudioMain()->startSequenceBgm(0x1000001);

    reset();
    mState = mNextStateA;
    mNextScene = SCENE_MENU;
    if (prev == SCENE_12)
        mMapIndex = gSequenceInfo.mMapIndex;
    SequenceApp::ptr()->ready(mNextScene);
    calcAnm();

    System::getDisplay()->startFadeIn(15);
}

SceneMapSelect::~SceneMapSelect() {
    delete mMainScreen;
    for (int i = 0; i < 6; i++)
        delete mScreens[i];
}

void SceneMapSelect::draw() {
    mOrtho->setPort();
    ((J2DScreen *)MenuBackground::ptr())->draw(0.0f, 0.0f, mOrtho);
    MenuTitleLine::ptr()->draw(mOrtho);
    mMainScreen->draw(0.0f, 0.0f, mOrtho);
    for (int i = 0; i < mMapCount; i++) {
        mScreens[i]->draw(mMainX + mPos[i].x, mMainY + mPos[i].y, mOrtho);
    }
}

void SceneMapSelect::calc() {
    switch (mState) {
    case 0:
        map_init();
        break;
    case 1:
        map();
        break;
    case 2:
        map_end();
        break;
    case 3:
        nextScene();
        break;
    case 4:
        nextBattle();
        break;
    default:
#line 343
        JUT_ASSERT(0);
        break;
    }

    MenuBackground::ptr()->calc();
    MenuTitleLine::ptr()->calc();

    f32 f;
    f = 1.0f + mMainBtk.mFrame;
    mMainBtk.mFrame = f;
    if (f >= 120.0f)
        mMainBtk.mFrame = 0.0f;
    f = 1.0f + mMainBpk.mFrame;
    mMainBpk.mFrame = f;
    if (f >= 120.0f)
        mMainBpk.mFrame = 0.0f;

    for (int i = 0; i < 3; i++)
        mMainBck[i].mAnm->setFrame(mMainBck[i].mFrame);
    mMainBtk.mAnm->setFrame(mMainBtk.mFrame);
    mMainBpk.mAnm->setFrame(mMainBpk.mFrame);
    mMainScreen->animation();

    for (int i = 0; i < mMapCount; i++) {
        for (int j = 0; j < 3; j++)
            mMapBck[i][j].mAnm->setFrame(mMapBck[i][j].mFrame);
        for (int j = 0; j < 2; j++)
            mMapBrk[i][j].mAnm->setFrame(mMapBrk[i][j].mFrame);
        mMapBpk[i].mAnm->setFrame(mMapBpk[i].mFrame);
        mScreens[i]->animation();
    }

    mMainX = mMainBck[0].mPane->mOffset.x;
    mMainY = mMainBck[0].mPane->mOffset.y;
    mTimer++;
}

void SceneMapSelect::map_init() {
    if (mTimer < 15) {
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcMapSelect);
        mMainBck[0].mPane->show();
        mMainBck[1].mPane->show();
        mMainBck[2].mPane->show();
        if (mTimer < 10) {
            mMainBck[0].mFrame = mTimer;
            mMainBck[1].mFrame = 0.0f;
            mMainBck[2].mFrame = 0.0f;
        }
    }
    else {
        mNextStateA = mState;
        mState = 1;
        mTimer = 0;
    }
    calcAnm();
}

void SceneMapSelect::map() {
    if (mConfirm != 0) {
        mConfirm++;
        u32 button = gpaGamePad[0]->getButton();
        u8 rl = 0;
        if (button & KartGamePad::R) {
            if (button & KartGamePad::L)
                rl = 1;
        }
        if (rl || mConfirm < 30) {
            rndRoulette();
        }
        else {
            buttonA();
            GetGameAudioMain()->startSystemSe(0x20028);
            mConfirm = 0;
        }
    }
    else {
        u32 trigger = gpaGamePad[0]->getTrigger();
        if ((trigger & KartGamePad::A) && System::getDisplay()->getFader()->getStatus() == JUTFader::Status_In) {
            if (mField264) {
                if (mMainBck[1].mFrame == 9.0f)
                    GetGameAudioMain()->startSystemSe(0x20002);
            }
            else {
                if (mMainBck[1].mFrame == 0.0f)
                    GetGameAudioMain()->startSystemSe(0x20003);
            }
            buttonA();
        }
        else if ((trigger & KartGamePad::B) && System::getDisplay()->getFader()->getStatus() == JUTFader::Status_In) {
            buttonB();
        }
        else {
            u32 repeat = gpaGamePad[0]->getRepeat();
            if (repeat & (KartGamePad::MAINSTICK_UP | KartGamePad::MAINSTICK_DOWN)) {
                if (mMainBck[1].mFrame == 0.0f) {
                    if (!(mMapCount == 5 && mMapIndex == 4)) {
                        GetGameAudioMain()->startSystemSe(0x20000);
                        if (mMapIndex % 2)
                            mMapIndex--;
                        else
                            mMapIndex++;
                    }
                }
            }
            else if (repeat & KartGamePad::MAINSTICK_LEFT) {
                if (mMainBck[1].mFrame == 0.0f) {
                    GetGameAudioMain()->startSystemSe(0x20000);
                    if (mMapCount == 5) {
                        switch (mMapIndex) {
                        case 0: mMapIndex = 4; break;
                        case 1: mMapIndex = 3; break;
                        case 2: mMapIndex = 0; break;
                        case 3: mMapIndex = 1; break;
                        case 4: mMapIndex = 2; break;
                        default:
#line 467
                            JUT_ASSERT(0);
                        }
                    }
                    else {
                        int idx = mMapCount;
                        idx = mMapIndex + idx;
                        mMapIndex = (idx - 2) % mMapCount;
                    }
                }
            }
            else if (repeat & KartGamePad::MAINSTICK_RIGHT) {
                if (mMainBck[1].mFrame == 0.0f) {
                    GetGameAudioMain()->startSystemSe(0x20000);
                    if (mMapCount == 5) {
                        switch (mMapIndex) {
                        case 0: mMapIndex = 2; break;
                        case 1: mMapIndex = 3; break;
                        case 2: mMapIndex = 4; break;
                        case 3: mMapIndex = 1; break;
                        case 4: mMapIndex = 0; break;
                        default:
#line 483
                            JUT_ASSERT(0);
                        }
                    }
                    else {
                        mMapIndex = (mMapIndex + 2) % mMapCount;
                    }
                }
            }
            else {
                u32 button = gpaGamePad[0]->getButton();
                if (button & KartGamePad::R) {
                    if (button & KartGamePad::L) {
                        if (mField264 == 0 && mMainBck[1].mFrame == 0.0f)
                            mConfirm = 1;
                    }
                }
            }
        }
    }
    calcAnm();
}

void SceneMapSelect::buttonA() {
    if (mField264) {
        if (mMainBck[1].mFrame == 9.0f) {
            System::getDisplay()->startFadeOut(15);
            GetGameAudioMain()->fadeOutAll(15);
            mNextStateB = 4;
            mState = 2;
            mNextStateA = 1;
            mTimer = 0;
            if (mMapCount == 5 && mMapIndex == 4 && gSystemRecord.isGameUnlocked(SystemRecord::MiniMario))
                ResMgr::loadCourseData(mMapData[5].courseID, ResMgr::mcOrder_2);
            else
                ResMgr::loadCourseData(mMapData[mMapIndex].courseID, ResMgr::mcOrder_2);
            gSequenceInfo.mMapIndex = mMapIndex;
        }
    }
    else {
        if (mMainBck[1].mFrame == 0.0f)
            mField264 = 1;
    }
}

void SceneMapSelect::buttonB() {
    if (mField264) {
        if (mMainBck[1].mFrame == 0.0f) {
            GetGameAudioMain()->startSystemSe(0x20005);
            mField264 = 0;
        }
    }
    else {
        if (mMainBck[1].mFrame == 0.0f) {
            GetGameAudioMain()->startSystemSe(0x20005);
            mNextStateB = 3;
            mNextScene = SCENE_MENU;
            mState = 2;
            mNextStateA = 1;
            mTimer = 0;
        }
    }
}

void SceneMapSelect::map_end() {
    if (mTimer < 15) {
        MenuTitleLine::ptr()->lift();
        if (mTimer > 6) {
            mMainBck[0].mFrame = 15 - mTimer;
            f32 frame = mMainBck[1].mFrame - 1.0f;
            mMainBck[1].mFrame = frame;
            if (frame < 0.0f)
                mMainBck[1].mFrame = 0.0f;
            frame = mMainBck[2].mFrame - 1.0f;
            mMainBck[2].mFrame = frame;
            if (frame < 0.0f)
                mMainBck[2].mFrame = 0.0f;
        }
    }
    else {
        mMainBck[0].mPane->hide();
        mMainBck[1].mPane->hide();
        mMainBck[2].mPane->hide();
        mNextStateA = mState;
        mState = mNextStateB;
        mTimer = 0;
    }
    calcAnm();
}

void SceneMapSelect::nextScene() {
    if (SequenceApp::ptr()->ready(mNextScene)) {
        SequenceApp::ptr()->setNextScene(mNextScene);
    }
}

void SceneMapSelect::nextBattle() {
    if (ResMgr::isFinishLoadingArc(ResMgr::mcArcCourse) && SequenceApp::ptr()->checkFinishAllLoading()) {
        gSequenceInfo.setRandomSeed();
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

        AppMgr::deleteCurrentApp();
        for (int i = 0; i < gRaceInfo.getKartNumber(); i++) {
            gRaceInfo.setStartPosIdx(i, gSequenceInfo.getStartNo(i));
            gRaceInfo.setPointTable(i, gSequenceInfo.getRankPoint(i));
        }
        if (gRaceInfo.getRaceMode() == GRAND_PRIX)
            gRaceInfo.mGpStageNo = gSequenceInfo.mMapInfo.course;
        gRaceInfo.mItemSlotType = gSystemRecord.mItemSlotType;
        RaceApp::call();
    }
}

void SceneMapSelect::calcAnm() {
    for (int i = 0; i < mMapCount; i++) {
        if (mMapIndex == i) {
            f32 f = 1.0f + mMapBck[i][0].mFrame;
            mMapBck[i][0].mFrame = f;
            if (f >= 8.0f)
                mMapBck[i][0].mFrame = 8.0f;
            f = 1.0f + mMapBck[i][1].mFrame;
            mMapBck[i][1].mFrame = f;
            if (f >= 8.0f)
                mMapBck[i][1].mFrame = 8.0f;
            f = 1.0f + mMapBck[i][2].mFrame;
            mMapBck[i][2].mFrame = f;
            if (f >= 60.0f)
                mMapBck[i][2].mFrame = 0.0f;
            if (mField264)
                mMapBrk[i][0].mFrame = 1.0f;
            else
                mMapBrk[i][0].mFrame = 1.0f;
            f = 1.0f + mMapBrk[i][1].mFrame;
            mMapBrk[i][1].mFrame = f;
            if (f >= 60.0f)
                mMapBrk[i][1].mFrame = 1.0f;
            f = 1.0f + mMapBpk[i].mFrame;
            mMapBpk[i].mFrame = f;
            if (f >= 60.0f)
                mMapBpk[i].mFrame = 0.0f;
        }
        else {
            f32 f = mMapBck[i][0].mFrame - 1.0f;
            mMapBck[i][0].mFrame = f;
            if (f <= 0.0f)
                mMapBck[i][0].mFrame = 0.0f;
            f = mMapBck[i][1].mFrame - 1.0f;
            mMapBck[i][1].mFrame = f;
            if (f <= 0.0f)
                mMapBck[i][1].mFrame = 0.0f;
            mMapBck[i][2].mFrame = 0.0f;
            if (mField264)
                mMapBrk[i][0].mFrame = 0.0f;
            else
                mMapBrk[i][0].mFrame = 1.0f;
            mMapBrk[i][1].mFrame = 0.0f;
            mMapBpk[i].mFrame = 0.0f;
        }
    }

    if (mField264) {
        f32 f = 1.0f + mMainBck[1].mFrame;
        mMainBck[1].mFrame = f;
        if (f > 9.0f)
            mMainBck[1].mFrame = 9.0f;
        f = 1.0f + mMainBck[2].mFrame;
        mMainBck[2].mFrame = f;
        if (f > 9.0f)
            mMainBck[2].mFrame = 9.0f;
    }
    else {
        f32 f = mMainBck[1].mFrame - 1.0f;
        mMainBck[1].mFrame = f;
        if (f < 0.0f)
            mMainBck[1].mFrame = 0.0f;
        f = mMainBck[2].mFrame - 1.0f;
        mMainBck[2].mFrame = f;
        if (f < 0.0f)
            mMainBck[2].mFrame = 0.0f;
    }
}

void SceneMapSelect::rndRoulette() {
    if (mTimer % 5 == 0) {
        GetGameAudioMain()->startSystemSe(0x20000);
        u32 time = OSGetTime();
        u32 rem = time % (mMapCount - 1);
        mMapIndex = (mMapIndex + (int)rem + 1) % mMapCount;
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
