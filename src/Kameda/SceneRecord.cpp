#include "Kameda/SceneRecord.h"
#include "Kameda/Scene.h"
#include "Kameda/CourseName2D.h"
#include "Kameda/Cup2D.h"
#include "Kameda/SceneCourseSelect.h"

#include "JSystem/J2D/J2DPicture.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JUtility/JUTDbg.h"
#include "JSystem/JUtility/JUTFader.h"
#include "Inagaki/GameAudioMain.h"
#include "Osako/AppMgr.h"
#include "Osako/MainMenuApp.h"
#include "Osako/SequenceApp.h"
#include "Osako/SystemRecord.h"
#include "Osako/GPRecord.h"
#include "Osako/Kart2DCommon.h"
#include "Osako/ResMgr.h"
#include "Osako/system.h"
#include "Osako/kartPad.h"
#include "dolphin/os.h"


const u64 SceneRecord::mTagRecord_cursor[] = {
    'TrCurs1', 'TrCurs2', 'TrCurs3', 
};

const u64 SceneRecord::mTagRecord_menu[] = {
    'RecM1', 'RecM2', 'RecM3',
};

const u64 SceneRecord::mTagRecord_menubar[] = {
    'RecBar1', 'RecBar2', 'RecBar3',
};

const u64 SceneRecord::mTagRecord_submenu[] = {
    'NRecCo1', 'NRecCo2',
};

const u64 SceneRecord::mTagRecord_submenu2[][3] = {
    {'RecCo1', 'NRecS1Ls', 'NRecS1Rs'},
    {'RecCo2', 'NRecS2Ls', 'NRecS2Rs'},
};

const u64 SceneRecord::mTagRecord_submenucursor[][2] = {
    {'NRecS1L', 'NRecS1R'},
    {'NRecS2L', 'NRecS2R'},
};

const u64 SceneRecord::mTagRecord_submenucursor2[][2] = {
    {'RecSL1L', 'RecSL1R'},
    {'RecSL2L', 'RecSL2R'},
};

const u64 SceneRecord::mTagRecord_submenucursor3[][2] = {
    {'RecSB1L', 'RecSB1R'},
    {'RecSB2L', 'RecSB2R'},
};

const u64 SceneRecord::mTagRecordGP_all[] = {
    'NGPM', 'NCoN', 'NReOp',
};

const u64 SceneRecord::mTagRecordGP_cursor[] = {
    'NCoTL', 'NCoTR',
};

const u64 SceneRecord::mTagRecordGP_cursor2[] = {
    'NCoSL', 'NCoSR',
};

const u64 SceneRecord::mTagRecordGP_cursor3[] = {
    'CoSBL', 'CoSBR',
};

const u64 SceneRecord::mTagRecordGP_cursorbtk[] = {
    'CoSLL', 'CoSLR',
};

const u64 SceneRecord::mTagRecordGP_linepostag[] = {
    'PowTag1', 'PowTag2', 'PowTag3', 'PowTag4',
};

const u64 SceneRecord::mTagRecordGP_linepostag2[] = {
    'PowSgl1', 'PowSgl2', 'PowSgl3', 'PowSgl4',
};

const u64 SceneRecord::mTagRecordGPLine_time[] = {
    'GPT0', 'GPT1', 'GPT2', 'GPT3',
    'GPT4', 'GPT5', 'GPT6',
};

const u64 SceneRecord::mTagRecordGPLine_quotation[] = {
    'GDTt02', 'GDTt01',
};

const u64 SceneRecord::mTagRecordGPLine_point[] = {
    'Point1', 'Point2', 'Point3',
};

const u64 SceneRecord::mTagRecordGPLine_charawindow[] = {
    'GPChaB1', 'GPChaB2', 'GPChaB3',
};

const u64 SceneRecord::mTagRecordGPLine_name[] = {
    'GPNm3', 'GPNm2', 'GPNm1',
};

const u64 SceneRecord::mTagRecordTA_all[] = {
    'N_BL', 'N_TA', 'NReOp',
};

const u64 SceneRecord::mTagRecordTA_cursor[] = {
    'NCoTL', 'NCoTR',
};

const u64 SceneRecord::mTagRecordTA_cursor2[] = {
    'NCoSL', 'NCoSR',
};

const u64 SceneRecord::mTagRecordTA_cursor3[] = {
    'CoSBL', 'CoSBR',
};

const u64 SceneRecord::mTagRecordTA_cursorbtk[] = {
    'CoSLL', 'CoSLR',
};

const u64 SceneRecord::mTagRecordTA_character[] = {
    'BTCha3', 'BTCha2', 'BTCha1',
};

const u64 SceneRecord::mTagRecordTA_characterwindow[] = {
    'BTChaB3', 'BTChaB2', 'BTChaB1',
};

const u64 SceneRecord::mTagRecordTA_bestlap[] = {
    'BLTm0', 'BLTm1', 'BLTm2', 'BLTm3',
    'BLTm4', 'BLTm5', 'BLTm6',
};

const u64 SceneRecord::mTagRecordTA_name[] = {
    'BLNm3', 'BLNm2', 'BLNm1',
};

const u64 SceneRecord::mTagRecordTA_quotation[] = {
    'BLTmt2', 'BLTmt1',
};

const u64 SceneRecord::mTagRecordTA_linepostag[] = {
    'Rank1', 'Rank2', 'Rank3', 'Rank4',
    'Rank5',
};

const u64 SceneRecord::mTagRecordTA_linecolor[] = {
    'Tex_1', 'Tex_2', 'Tex_3', 'Tex_4',
    'Tex_5',
};

const u64 SceneRecord::mTagRecordTALine_time[] = {
    'TTTm0', 'TTTm1', 'TTTm2', 'TTTm3',
    'TTTm4', 'TTTm5', 'TTTm6',
};

const u64 SceneRecord::mTagRecordTALine_quotation[] = {
    'TTTmt2', 'TTTmt1',
};

const u64 SceneRecord::mTagRecordTALine_character[] = {
    'TTCha3', 'TTCha2', 'TTCha1',
};

const u64 SceneRecord::mTagRecordTALine_characterwindow[] = {
    'TTChaB3', 'TTChaB2', 'TTChaB1',
};

const u64 SceneRecord::mTagRecordTALine_name[] = {
    'TTNm3', 'TTNm2', 'TTNm1',
};

const char *SceneRecord::mBtiName_Cup[] = {
    "CupWithPict_MushroomCup.bti",
    "CupWithPict_FlowerCup.bti",
    "CupWithPict_StarCup.bti",
    "CupWithPict_SpecialCup.bti",
    "CupWithPict_Reverse2Cup.bti"
};

const char *SceneRecord::mBtiName_Level[] = {
    "Power_Mozi.1.bti",
    "Power_Mozi.2.bti",
    "Power_Mozi.3.bti",
    "Power_Mozi.4.bti",
};

const char *SceneRecord::mBtiName_Trophy[3] = {
    "Trophy1.bti",
    "Trophy2.bti",
    "Trophy3.bti"
};

const char *SceneRecord::mBtiName_Rank[] = {
    "position1_1.bti",
    "position1_2.bti",
    "position1_3.bti",
    "position1_4.bti",
    "position1_5.bti",
};

const char *SceneRecord::mBtiName_characterwindow = "chara_window1.bti";
const char *SceneRecord::mBtiName_quotation = "kazu3Dten.bti";
const char *SceneRecord::mBtiName_colon1 = "timg/MarioFontColon1.bti";
const char *SceneRecord::mBtiName_colon2 = "timg/MarioFontColon2.bti";
const char *SceneRecord::mBtiName_minus = "MarioFont_Number_XLine.bti";

const u64 SceneRecord::mModeTag[] = {
    'mt_1gp', 'mt_2gp', 'ms_1gp', 'ms_2gp',
    'mc_betu', 'm_pice',
};

SceneRecord::SceneRecord(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap), SceneTitleLine(archive, heap, "record_title_line") {
    OSInitFastCast();
    SceneRecord_ConstructerMenu(heap);
    SceneRecord_ConstructerGP(heap);
    SceneRecord_ConstructerGPLine(heap);
    SceneRecord_ConstructerTA(heap);
    SceneRecord_ConstructerTALine(heap);
    SceneRecord_ConstructerBack(heap);

    if (System::getDisplay()->getFader()->getStatus() == JUTFader::Status_Out) {
        mState = 0;
        mPrevState = 0;
        if (System::getDisplay()->getFader())
            System::getDisplay()->getFader()->startFadeIn(15);
        mFadeTimer = 0;
    } else {
        mState = 1;
        mPrevState = 1;
    }

    _094 = 0;
    mMenuCursor = 0;
    mGpCursor = 0;
    mGpIndex = 0;
    mTaCursor = 0;
    mTaIndex = 0;
}

void SceneRecord::SceneRecord_ConstructerMenu(JKRHeap *heap) {
    int i;
    int j;
    mMenuScreen = new (heap, 0) J2DScreen();
    mMenuScreen->set("SelectRecord.blo", 0x40000, mArchive);
    mMenuAnm0 = J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("SelectRecord.bck", mArchive));
    mMenuScreen->setAnimation((J2DAnmTransform *)mMenuAnm0);
    mMenuAnm1 = J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("SelectRecord.bpk", mArchive));
    mMenuAnm1->searchUpdateMaterialID(mMenuScreen);
    void *brk = JKRFileLoader::getGlbResource("SelectRecord.brk", mArchive);
    void *btk = JKRFileLoader::getGlbResource("SelectRecord.btk", mArchive);
    mSubAnm2 = J2DAnmLoaderDataBase::load(btk);
    mSubAnm2->searchUpdateMaterialID(mMenuScreen);
    for (i = 0; i < 3; i++) {
        mMenuAnm2[i] = J2DAnmLoaderDataBase::load(brk);
        mMenuAnm2[i]->searchUpdateMaterialID(mMenuScreen);
    }
    for (i = 0; i < 2; i++) {
        mSubAnm3[i] = J2DAnmLoaderDataBase::load(btk);
        mSubAnm3[i]->searchUpdateMaterialID(mMenuScreen);
        for (j = 0; j < 2; j++) {
            mSubAnm[i][j] = J2DAnmLoaderDataBase::load(brk);
            mSubAnm[i][j]->searchUpdateMaterialID(mMenuScreen);
        }
    }

    mRecordPane = (J2DPicture *)mMenuScreen->search('NRecord');
    mRecordPane->setAnimation((J2DAnmTransform *)mMenuAnm0);

    for (i = 0; i < 3; i++) {
        mMenuCursorPanes[i] = (J2DPicture *)mMenuScreen->search(mTagRecord_cursor[i]);
        mMenuCursorPanes[i]->setAnimation((J2DAnmTransform *)mMenuAnm0);
        mMenuCursorPanes[i]->setAnimation((J2DAnmColor *)mMenuAnm1);
        mMenuBarPanes[i] = (J2DPicture *)mMenuScreen->search(mTagRecord_menu[i]);
        mMenuBarPanes[i]->setAnimation((J2DAnmTransform *)mMenuAnm0);
        mMenuBarPanes[i]->setAnimation((J2DAnmTevRegKey *)mMenuAnm2[i]);
        mMenuSubPanes[i] = (J2DPicture *)mMenuScreen->search(mTagRecord_menubar[i]);
        mMenuSubPanes[i]->setAnimation((J2DAnmTextureSRTKey *)mSubAnm2);
        mMenuSubPanes[i]->setAnimation((J2DAnmTevRegKey *)mMenuAnm2[i]);
    }

    for (i = 0; i < 2; i++) {
        mSubCursorPanes0[i] = (J2DPicture *)mMenuScreen->search(mTagRecord_submenu[i]);
        mSubCursorPanes0[i]->setAnimation((J2DAnmTransform *)mMenuAnm0);
        for (j = 0; j < 3; j++) {
            mSubCursorPanes1[i][j] = (J2DPicture *)mMenuScreen->search(mTagRecord_submenu2[i][j]);
            mSubCursorPanes1[i][j]->setAnimation((J2DAnmTransform *)mMenuAnm0);
        }
        for (j = 0; j < 2; j++) {
            mSubCursorPanes2[i][j] = (J2DPicture *)mMenuScreen->search(mTagRecord_submenucursor[i][j]);
            mSubCursorPanes2[i][j]->setAnimation((J2DAnmTransform *)mMenuAnm0);
            mSubCursorPanes3[i][j] = (J2DPicture *)mMenuScreen->search(mTagRecord_submenucursor2[i][j]);
            mSubCursorPanes3[i][j]->setAnimation((J2DAnmTextureSRTKey *)mSubAnm3[i]);
            mSubCursorPanes4[i][j] = (J2DPicture *)mMenuScreen->search(mTagRecord_submenucursor3[i][j]);
            mSubCursorPanes4[i][j]->setAnimation((J2DAnmTevRegKey *)mSubAnm[i][j]);
        }
    }
}

void SceneRecord::SceneRecord_ConstructerGP(JKRHeap *heap) {
    int i;
    mGpScreen = new (heap, 0) J2DScreen();
    mGpScreen->set("GPLayout.blo", 0x40000, mArchive);
    mGpAnm0 = J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("GPLayout.bck", mArchive));
    mGpScreen->setAnimation((J2DAnmTransform *)mGpAnm0);
    mGpAnm1 = J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("GPLayout.btk", mArchive));
    mGpAnm1->searchUpdateMaterialID(mGpScreen);
    void *brk = JKRFileLoader::getGlbResource("GPLayout.brk", mArchive);
    for (i = 0; i < 2; i++) {
        mGpAnm2[i] = J2DAnmLoaderDataBase::load(brk);
        mGpAnm2[i]->searchUpdateMaterialID(mGpScreen);
    }

    for (i = 0; i < 3; i++) {
        mGpAllPanes[i] = (J2DPicture *)mGpScreen->search(mTagRecordGP_all[i]);
        mGpAllPanes[i]->setAnimation((J2DAnmTransform *)mGpAnm0);
    }

    for (i = 0; i < 2; i++) {
        mGpCursorPanes[i] = (J2DPicture *)mGpScreen->search(mTagRecordGP_cursor[i]);
        mGpCursorPanes[i]->setAnimation((J2DAnmTransform *)mGpAnm0);
        mGpCursorPanes2[i] = (J2DPicture *)mGpScreen->search(mTagRecordGP_cursor2[i]);
        mGpCursorPanes2[i]->setAnimation((J2DAnmTransform *)mGpAnm0);
        mGpPanes[i] = (J2DPicture *)mGpScreen->search(mTagRecordGP_cursor3[i]);
        mGpPanes[i]->setAnimation((J2DAnmTevRegKey *)mGpAnm2[i]);
        mGpPanes2[i] = (J2DPicture *)mGpScreen->search(mTagRecordGP_cursorbtk[i]);
        mGpPanes2[i]->setAnimation((J2DAnmTextureSRTKey *)mGpAnm1);
    }

    mGpReOpPane = (J2DPicture *)mGpScreen->search('GDCoName');
    mGpReOpPane->setAnimation((J2DAnmTransform *)mGpAnm0);

    for (i = 0; i < 4; i++) {
        mGpLinePanes[i] = (J2DPicture *)mGpScreen->search(mTagRecordGP_linepostag[i]);
        mGpLinePos[i].set(mGpLinePanes[i]->getTranslateX(), mGpLinePanes[i]->getTranslateY());
        mGpLinePanes2[i] = (J2DPicture *)mGpScreen->search(mTagRecordGP_linepostag2[i]);
        mGpLinePos2[i].set(mGpLinePanes2[i]->getTranslateX(), mGpLinePanes2[i]->getTranslateY());
    }
}

void SceneRecord::SceneRecord_ConstructerGPLine(JKRHeap *heap) {
    int i;
    int j;
    for (i = 0; i < 4; i++) {
        mGpLineScreen[i] = new (heap, 0) J2DScreen();
        mGpLineScreen[i]->set("GPLine.blo", 0x40000, mArchive);
    }
    mGpLineAnm = J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("GPLine.bck", mArchive));

    for (i = 0; i < 4; i++) {
        mGpLineScreen[i]->setAnimation((J2DAnmTransform *)mGpLineAnm);
        mGpLineAllPanes[i] = (J2DPicture *)mGpLineScreen[i]->search('NLine');
        mGpLineAllPanes[i]->setAnimation((J2DAnmTransform *)mGpLineAnm);
        mGpLineCursorPanes[i] = (J2DPicture *)mGpLineScreen[i]->search('PowM');
        mGpLineCursorPanes2[i] = (J2DPicture *)mGpLineScreen[i]->search('Trophy');
        for (j = 0; j < 7; j++)
            mGpLinePts[i][j] = (J2DPicture *)mGpLineScreen[i]->search(mTagRecordGPLine_time[j]);
        for (j = 0; j < 2; j++)
            mGpLineColon[i][j] = (J2DPicture *)mGpLineScreen[i]->search(mTagRecordGPLine_quotation[j]);
        for (j = 0; j < 3; j++)
            mGpLineChar[i][j] = (J2DPicture *)mGpLineScreen[i]->search(mTagRecordGPLine_point[j]);
        mGpLinePanes3[i] = (J2DPicture *)mGpLineScreen[i]->search('PointM');
        mGpLineName[i] = (J2DPicture *)mGpLineScreen[i]->search('GPCha3');
        mGpLineName2[i] = (J2DPicture *)mGpLineScreen[i]->search('GPCha2');
        mGpLineName3[i] = (J2DPicture *)mGpLineScreen[i]->search('GPCha1');
        for (j = 0; j < 3; j++)
            mGpLineCharWin[i][j] = (J2DPicture *)mGpLineScreen[i]->search(mTagRecordGPLine_charawindow[j]);
        for (j = 0; j < 3; j++)
            mGpLineCharWin2[i][j] = (J2DPicture *)mGpLineScreen[i]->search(mTagRecordGPLine_name[j]);
        mGpLineTag[i] = (J2DPicture *)mGpLineScreen[i]->search('TagMark');
    }
}

void SceneRecord::SceneRecord_ConstructerTA(JKRHeap *heap) {
    int i;
    mTaScreen = new (heap, 0) J2DScreen();
    mTaScreen->set("TALayout.blo", 0x40000, mArchive);
    mTaAnm0 = J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("TALayout.bck", mArchive));
    mTaScreen->setAnimation((J2DAnmTransform *)mTaAnm0);
    mTaAnm1 = J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("TALayout.btk", mArchive));
    mTaAnm1->searchUpdateMaterialID(mTaScreen);
    void *brk = JKRFileLoader::getGlbResource("TALayout.brk", mArchive);
    for (i = 0; i < 2; i++) {
        mTaAnm2[i] = J2DAnmLoaderDataBase::load(brk);
        mTaAnm2[i]->searchUpdateMaterialID(mTaScreen);
    }

    for (i = 0; i < 3; i++) {
        mTaAllPanes[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_all[i]);
        mTaAllPanes[i]->setAnimation((J2DAnmTransform *)mTaAnm0);
    }

    for (i = 0; i < 2; i++) {
        mTaCursorPanes[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_cursor[i]);
        mTaCursorPanes[i]->setAnimation((J2DAnmTransform *)mTaAnm0);
        mTaCursorPanes2[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_cursor2[i]);
        mTaCursorPanes2[i]->setAnimation((J2DAnmTransform *)mTaAnm0);
        mTaPanes[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_cursor3[i]);
        mTaPanes[i]->setAnimation((J2DAnmTevRegKey *)mTaAnm2[i]);
        mTaPanes2[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_cursorbtk[i]);
        mTaPanes2[i]->setAnimation((J2DAnmTextureSRTKey *)mTaAnm1);
    }

    mTaReOpPane = (J2DPicture *)mTaScreen->search('TACoN');
    mTaReOpPane->setAnimation((J2DAnmTransform *)mTaAnm0);

    for (i = 0; i < 3; i++) {
        mTaChar[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_character[i]);
        mTaCharWin[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_characterwindow[i]);
    }
    for (i = 0; i < 7; i++)
        mTaBestLap[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_bestlap[i]);
    for (i = 0; i < 3; i++)
        mTaName[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_name[i]);
    for (i = 0; i < 2; i++)
        mTaColon[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_quotation[i]);

    for (i = 0; i < 5; i++) {
        mTaRank[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_linepostag[i]);
        mTaLinePos[i].set(mTaRank[i]->getTranslateX(), mTaRank[i]->getTranslateY());
        mTaLineColorPanes[i] = (J2DPicture *)mTaScreen->search(mTagRecordTA_linecolor[i]);
        mTaLineColorPanes[i]->getCornerColor(mTaLineColor[i]);
    }
}

void SceneRecord::SceneRecord_ConstructerTALine(JKRHeap *heap) {
    int i;
    int j;
    for (i = 0; i < 5; i++) {
        mTaLineScreen[i] = new (heap, 0) J2DScreen();
        mTaLineScreen[i]->set("TALine.blo", 0x40000, mArchive);
    }
    mTaLineAnm = J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("TALine.bck", mArchive));

    for (i = 0; i < 5; i++) {
        mTaLineScreen[i]->setAnimation((J2DAnmTransform *)mTaLineAnm);
        mTaLineRank[i] = (J2DPicture *)mTaLineScreen[i]->search('TTRank');
        mTaLineRank[i]->setAnimation((J2DAnmTransform *)mTaLineAnm);
        mTaLineRank[i]->setCornerColor(mTaLineColor[i]);
        for (j = 0; j < 7; j++)
            mTaLineChar[i][j] = (J2DPicture *)mTaLineScreen[i]->search(mTagRecordTALine_time[j]);
        for (j = 0; j < 2; j++)
            mTaLineColon[i][j] = (J2DPicture *)mTaLineScreen[i]->search(mTagRecordTALine_quotation[j]);
        for (j = 0; j < 3; j++) {
            mTaLineTime[i][j] = (J2DPicture *)mTaLineScreen[i]->search(mTagRecordTALine_character[j]);
            mTaLineQuote[i][j] = (J2DPicture *)mTaLineScreen[i]->search(mTagRecordTALine_characterwindow[j]);
        }
        for (j = 0; j < 3; j++)
            mTaLineCharWin[i][j] = (J2DPicture *)mTaLineScreen[i]->search(mTagRecordTALine_name[j]);
    }
}

void SceneRecord::SceneRecord_ConstructerBack(JKRHeap *heap) {
    mBackScreen = new (heap, 0) J2DScreen();
    mBackScreen->set("RecordBack.blo", 0x1040000, mArchive);
    mBackAnm = J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("RecordBack.btk", mArchive));
    mBackScreen->setAnimation((J2DAnmTextureSRTKey *)mBackAnm);
}

void SceneRecord::init() {
    GameAudio::Main::getAudio()->startSequenceBgm(0x1000003);
    mState = mPrevState;
    mSceneType = SequenceApp::ptr()->getPrevScene();

    JUTFader *fader = System::getDisplay()->getFader();
    if (fader->mStatus == JUTFader::Status_Out) {
        mState = 0;
        mPrevState = 0;
        mFadeTimer = 0;
        if (System::getDisplay()->getFader())
            System::getDisplay()->getFader()->startFadeIn(15);
    }

    if (mSceneType != Scene::SCENE_MAX)
        SequenceApp::ptr()->ready((Scene::SceneType)mSceneType);
    else
        mSceneType = Scene::SCENE_MAX;

    init_mode();
    init_gp();
    init_gpline();
    init_ta();
    init_taline();
    init_back();
    init_titleline(7);

    mModeCount = 3;
    mLevelCount = 12;
    if (gSystemRecord.isGameUnlocked(SystemRecord::SpecialCup)) {
        mModeCount = 4;
        mLevelCount = 16;
    }
    if (gSystemRecord.isGameUnlocked(SystemRecord::AllCupTour))
        mModeCount = 5;
}

void SceneRecord::init_mode() {
    int i;
    int j;
    mModeCursor = -1;
    mMatCursorTimer = 0;
    for (i = 0; i < 3; i++)
        mMatMenuTimer[i] = 0;
    for (i = 0; i < 2; i++) {
        mSubCursorTimer[i] = 20;
        for (j = 0; j < 3; j++)
            mSubCursorTimer2[i][j] = 29;
        mSubCursorTimer3[i] = 40;
        mSubCursorTimer4[i] = 0;
    }
}

void SceneRecord::init_gp() {
    mGpTimer = -1;
    mGpMatTimer = 0;
    mGpCursorTimer = 10;
    for (int i = 0; i < 2; i++) {
        mGpCursorFrame[i] = 49;
        mGpTextFrame[i] = 0;
    }
    mGpTextTimer = 49;
}

void SceneRecord::init_gpline() {
    mGpLineTimer = -1;
    mGpLineDir = 0;
    mGpLinePhase = 0.0f;
    for (int i = 0; i < 4; i++) {
        mGpLineCursorPanes[i]->changeTexture(mBtiName_Level[i], 0);
        mGpLinePanes3[i]->changeTexture(Kart2DCommon::ptr()->getPartsLocName(0), 0);
        mGpLineColon[i][0]->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, mBtiName_colon1), 0);
        mGpLineColon[i][1]->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, mBtiName_colon2), 0);
        for (int j = 0; j < 3; j++)
            mGpLineCharWin[i][j]->changeTexture(mBtiName_characterwindow, 0);
    }
    if (!gSystemRecord.isGameUnlocked(SystemRecord::Mirror))
        mGpLineScreen[3]->hide();
    else
        mGpLineScreen[3]->show();
}

void SceneRecord::init_ta() {
    mTaTimer = -1;
    mTaMatTimer = 0;
    mTaCursorTimer = 10;
    for (int i = 0; i < 2; i++) {
        mTaCursorFrame[i] = 49;
        mTaTextFrame[i] = 0;
    }
    mTaTextTimer = 49;
    for (int i = 0; i < 3; i++)
        mTaCharWin[i]->changeTexture(mBtiName_characterwindow, 0);
    mTaColon[0]->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, mBtiName_colon1), 0);
    mTaColon[1]->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, mBtiName_colon2), 0);
}

void SceneRecord::init_taline() {
    int i;
    int j;
    mTaLineTimer = -1;
    mTaLineDir = 0;
    mTaLinePhase = 0.0f;
    for (i = 0; i < 5; i++)
        mTaLineRank[i]->changeTexture(mBtiName_Rank[i], 0);
    for (i = 0; i < 5; i++) {
        mTaLineColon[i][0]->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, mBtiName_colon1), 0);
        mTaLineColon[i][1]->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, mBtiName_colon2), 0);
        for (j = 0; j < 3; j++)
            mTaLineQuote[i][j]->changeTexture(mBtiName_characterwindow, 0);
    }
}

void SceneRecord::init_back() {
    mBackFlag = 0;
}

SceneRecord::~SceneRecord() {
    delete mMenuScreen;
    delete mGpScreen;
    for (int i = 0; i < 4; i++)
        delete mGpLineScreen[i];
    delete mTaScreen;
    for (int i = 0; i < 5; i++)
        delete mTaLineScreen[i];
    delete mBackScreen;
}

void SceneRecord::draw() {
    int i;
    mOrtho->setPort();
    mBackScreen->draw(0.0f, 0.0f, mOrtho);
    mMenuScreen->draw(0.0f, 0.0f, mOrtho);
    mGpScreen->draw(0.0f, 0.0f, mOrtho);

    int count = 0;
    for (i = 0; i < 4; i++) {
        GPRecord rec = gSystemRecord.getGPRecord((ERaceGpCup)ConvNum2ERace(mGpIndex), (ERaceLevel)i);
        if ((rec.mFlags & 2) && rec.isValid())
            count++;
    }

    for (i = 0; i < 4; i++) {
        if (count != 0)
            mGpLineScreen[i]->draw(mGpLinePos[i].x, mGpLinePos[i].y, mOrtho);
        else
            mGpLineScreen[i]->draw(mGpLinePos2[i].x, mGpLinePos2[i].y, mOrtho);
    }

    mTaScreen->draw(0.0f, 0.0f, mOrtho);
    for (i = 0; i < 5; i++)
        mTaLineScreen[i]->draw(mTaLinePos[i].x, mTaLinePos[i].y, mOrtho);
    drawTitleLine(mOrtho);
}

void SceneRecord::calcAnmRecordAll() {}

void SceneRecord::calcAnmRecordCursor() {
    int j;
    int i;
    for (i = 0; i < 3; i++) {
        if (mMenuCursor == i)
            mMenuTimers[i]++;
        else
            mMenuTimers[i] = 10;
        if (mMenuTimers[i] < 10)
            mMenuTimers[i] = 10;
        if (mMenuTimers[i] > 17)
            mMenuTimers[i] = 17;
        mMenuAnm0->setFrame((f32)mMenuTimers[i]);
        mMenuCursorPanes[i]->animationTransform();
        mMenuBarPanes[i]->animationTransform();
    }

    for (i = 0; i < 2; i++) {
        if (mMenuCursor == i)
            mSubCursorTimer[i]++;
        else
            mSubCursorTimer[i]--;
        if (mSubCursorTimer[i] < 20)
            mSubCursorTimer[i] = 20;
        if (mSubCursorTimer[i] > 23)
            mSubCursorTimer[i] = 23;
        mMenuAnm0->setFrame((f32)mSubCursorTimer[i]);
        mSubCursorPanes0[i]->animationTransform();

        if (mMenuCursor == i)
            mSubCursorTimer3[i]++;
        else
            mSubCursorTimer3[i] = 40;
        if (mSubCursorTimer3[i] < 40)
            mSubCursorTimer3[i] = 40;
        if (mSubCursorTimer3[i] > 69)
            mSubCursorTimer3[i] = 40;
        mMenuAnm0->setFrame((f32)mSubCursorTimer3[i]);
        for (j = 0; j < 2; j++)
            mSubCursorPanes2[i][j]->animationTransform();

        for (j = 0; j < 3; j++) {
            int *t = &mSubCursorTimer2[i][j];
            if (*t <= 29) {
                mMenuAnm0->setFrame(30.0f);
                mSubCursorPanes1[i][j]->animationTransform();
                *t = 29;
            } else {
                (*t)++;
                if (*t < 30)
                    *t = 30;
                if (*t > 38) {
                    *t = 29;
                    mMenuAnm0->setFrame(30.0f);
                } else {
                    mMenuAnm0->setFrame((f32)*t);
                }
                mSubCursorPanes1[i][j]->animationTransform();
            }
        }
    }
}

void SceneRecord::calcTexChangeRecord() {}

void SceneRecord::calcAnmRecordGPAll() {}

void SceneRecord::calcAnmRecordGPCursor() {
    mGpCursorTimer++;
    if (mGpCursorTimer < 10)
        mGpCursorTimer = 10;
    if (mGpCursorTimer > 40)
        mGpCursorTimer = 10;
    mGpAnm0->setFrame((f32)mGpCursorTimer);
    int i;
    for (i = 0; i < 2; i++)
        mGpCursorPanes[i]->animationTransform();

    for (i = 0; i < 2; i++) {
        int *t = &mGpCursorFrame[i];
        if (*t < 50) {
            mGpAnm0->setFrame(50.0f);
            mGpCursorPanes2[i]->animationTransform();
            *t = 49;
        } else {
            (*t)++;
            if (*t > 58) {
                mGpAnm0->setFrame(50.0f);
                *t = 49;
            } else {
                mGpAnm0->setFrame((f32)*t);
            }
            mGpCursorPanes2[i]->animationTransform();
        }
    }

    if (mGpTextTimer < 50) {
        mGpAnm0->setFrame(50.0f);
        mGpReOpPane->animationTransform();
        mGpTextTimer = 49;
    } else {
        mGpTextTimer++;
        if (mGpTextTimer > 58) {
            mGpAnm0->setFrame(50.0f);
            mGpTextTimer = 49;
        } else {
            mGpAnm0->setFrame((f32)mGpTextTimer);
        }
        mGpReOpPane->animationTransform();
    }
}

void SceneRecord::calcAnmRecordGPLineAll() {
    switch (mState) {
    case 2:
        if (mGpCursor == mGpIndex) {
            if (mGpLineTimer < 4) {
                mGpLineTimer++;
                if (mGpLineTimer > 4)
                    mGpLineTimer = 4;
            } else if (mGpLineTimer > 4) {
                mGpLineTimer--;
                if (mGpLineTimer < 4)
                    mGpLineTimer = 4;
            }
            if (mGpLinePhase < 4.0f) {
                f32 d = 4.0f - mGpLinePhase;
                d *= 0.5f;
                if (d < 0.2f)
                    d = 0.2f;
                if (d > 1.0f)
                    d = 1.0f;
                mGpLinePhase += d;
                if (mGpLinePhase > 4.0f)
                    mGpLinePhase = 4.0f;
            } else if (mGpLinePhase > 4.0f) {
                f32 d = mGpLinePhase - 4.0f;
                d *= 0.5f;
                if (d < 0.2f)
                    d = 0.2f;
                if (d > 1.0f)
                    d = 1.0f;
                mGpLinePhase -= d;
                if (mGpLinePhase < 4.0f)
                    mGpLinePhase = 4.0f;
            }
        } else {
            mGpLineTimer += mGpLineDir;
            if (mGpLineTimer < 0) {
                mGpLineTimer = 8;
                mGpIndex = mGpCursor;
            }
            if (mGpLineTimer > 8) {
                mGpLineTimer = 0;
                mGpIndex = mGpCursor;
            }
            mGpLinePhase = (f32)mGpLineTimer;
        }
        break;
    default:
        if (mGpLineTimer > 0) {
            mGpLineTimer--;
            if (mGpLineTimer < 0)
                mGpLineTimer = 0;
        }
        mGpLinePhase = (f32)mGpLineTimer;
        break;
    }

    mGpLineAnm->setFrame(mGpLinePhase);
    for (int i = 0; i < 4; i++)
        mGpLineAllPanes[i]->animationTransform();
}

void SceneRecord::calcAnmRecordGPLineEtc() {}

void SceneRecord::calcTexChangeRecordGP() {}

void SceneRecord::calcTexChangeRecordGPLine(ERaceLevel level) {
    int i;
    int d;
    u8 char1, char2;
#line 1279
    JUT_MINMAX_ASSERT(0, level, 4);
    GPRecord rec = gSystemRecord.getGPRecord((ERaceGpCup)ConvNum2ERace(mGpIndex), level);
    if (rec.isValid()) {
        u8 rank = rec.mRank;
        if (rank >= 3) {
            mGpLineCursorPanes2[level]->hide();
        } else {
            mGpLineCursorPanes2[level]->show();
            mGpLineCursorPanes2[level]->changeTexture((ResTIMG *)Cup2D::ptr()->getCupTexture((ERaceGpCup)ConvNum2ERace(mGpIndex), rank), 0);
        }

        int time = rec.mTotalTime.get();
        if (!rec.mTotalTime.isAvailable()) {
            for (i = 0; i < 7; i++) {
                mGpLinePts[level][i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture('-'), 0);
                mGpLinePts[level][i]->show();
            }
        } else {
            mGpLinePts[level][0]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(time % 10)), 0);
            mGpLinePts[level][1]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 10) % 10)), 0);
            mGpLinePts[level][2]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 100) % 10)), 0);
            mGpLinePts[level][3]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 1000) % 10)), 0);
            mGpLinePts[level][4]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 10000) % 6)), 0);
            mGpLinePts[level][5]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 60000) % 10)), 0);
            mGpLinePts[level][6]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 600000) % 10)), 0);
            if ((time / 600000) % 10 != 0)
                mGpLinePts[level][6]->show();
            else
                mGpLinePts[level][6]->hide();
        }

        u8 pts = rec.mPoints;
        mGpLineChar[level][0]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(pts % 10)), 0);
        u8 tens = pts / 10u;
        d = tens % 10;
        mGpLineChar[level][1]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)d), 0);
        if (d != 0)
            mGpLineChar[level][1]->show();
        else if ((tens / 10) % 10 != 0)
            mGpLineChar[level][1]->show();
        else
            mGpLineChar[level][1]->hide();
        u8 hundreds = pts / 100u;
        d = hundreds % 10;
        mGpLineChar[level][2]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)d), 0);
        if (d != 0)
            mGpLineChar[level][2]->show();
        else
            mGpLineChar[level][2]->hide();

        u8 c1 = rec.mCharIDs[0];
        u8 c2 = rec.mCharIDs[1];
        u8 kart = rec.mKartID;
        char1 = c1;
        char2 = c2;
        if (c1 == 0)
            char1 = 1;
        if (c2 == 0)
            char2 = 1;
        mGpLineName[level]->show();
        mGpLineName2[level]->show();
        mGpLineName3[level]->show();
        for (i = 0; i < 3; i++)
            mGpLineCharWin[level][i]->show();
        mGpLineName[level]->changeTexture(Kart2DCommon::ptr()->getCharIconTexture((ECharID)char1), 0);
        mGpLineName2[level]->changeTexture(Kart2DCommon::ptr()->getCharIconTexture((ECharID)char2), 0);
        mGpLineName3[level]->changeTexture(Kart2DCommon::ptr()->getKartIconTexture(kart), 0);
        char *name = rec.mName;
        for (i = 0; i < 3; i++)
            mGpLineCharWin2[level][i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture(name[i]), 0);
    } else {
        mGpLineCursorPanes2[level]->hide();
        for (i = 0; i < 7; i++) {
            mGpLinePts[level][i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture('-'), 0);
            mGpLinePts[level][i]->show();
        }
        for (i = 0; i < 3; i++) {
            mGpLineChar[level][i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture('-'), 0);
            mGpLineChar[level][i]->show();
        }
        mGpLineName[level]->hide();
        mGpLineName2[level]->hide();
        mGpLineName3[level]->hide();
        for (i = 0; i < 3; i++)
            mGpLineCharWin[level][i]->hide();
        for (i = 0; i < 3; i++)
            mGpLineCharWin2[level][i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture('-'), 0);
    }
}

void SceneRecord::calcAnmRecordTAAll() {}

void SceneRecord::calcAnmRecordTACursor() {
    mTaCursorTimer++;
    if (mTaCursorTimer < 10)
        mTaCursorTimer = 10;
    if (mTaCursorTimer > 40)
        mTaCursorTimer = 10;
    mTaAnm0->setFrame((f32)mTaCursorTimer);
    int i;
    for (i = 0; i < 2; i++)
        mTaCursorPanes[i]->animationTransform();

    for (i = 0; i < 2; i++) {
        int *t = &mTaCursorFrame[i];
        if (*t < 50) {
            mTaAnm0->setFrame(50.0f);
            mTaCursorPanes2[i]->animationTransform();
            *t = 49;
        } else {
            (*t)++;
            if (*t > 58) {
                mTaAnm0->setFrame(50.0f);
                *t = 49;
            } else {
                mTaAnm0->setFrame((f32)*t);
            }
            mTaCursorPanes2[i]->animationTransform();
        }
    }

    if (mTaTextTimer < 50) {
        mTaAnm0->setFrame(50.0f);
        mTaReOpPane->animationTransform();
        mTaTextTimer = 49;
    } else {
        mTaTextTimer++;
        if (mTaTextTimer > 58) {
            mTaAnm0->setFrame(50.0f);
            mTaTextTimer = 49;
        } else {
            mTaAnm0->setFrame((f32)mTaTextTimer);
        }
        mTaReOpPane->animationTransform();
    }
}

void SceneRecord::calcAnmRecordTALineAll() {
    switch (mState) {
    case 3:
        if (mTaCursor == mTaIndex) {
            if (mTaLineTimer < 4) {
                mTaLineTimer++;
                if (mTaLineTimer > 4)
                    mTaLineTimer = 4;
            } else if (mTaLineTimer > 4) {
                mTaLineTimer--;
                if (mTaLineTimer < 4)
                    mTaLineTimer = 4;
            }
            if (mTaLinePhase < 4.0f) {
                f32 d = 4.0f - mTaLinePhase;
                d *= 0.5f;
                if (d < 0.2f)
                    d = 0.2f;
                if (d > 1.0f)
                    d = 1.0f;
                mTaLinePhase += d;
                if (mTaLinePhase > 4.0f)
                    mTaLinePhase = 4.0f;
            } else if (mTaLinePhase > 4.0f) {
                f32 d = mTaLinePhase - 4.0f;
                d *= 0.5f;
                if (d < 0.2f)
                    d = 0.2f;
                if (d > 1.0f)
                    d = 1.0f;
                mTaLinePhase -= d;
                if (mTaLinePhase < 4.0f)
                    mTaLinePhase = 4.0f;
            }
        } else {
            mTaLineTimer += mTaLineDir;
            if (mTaLineTimer < 0) {
                mTaLineTimer = 8;
                mTaIndex = mTaCursor;
            }
            if (mTaLineTimer > 8) {
                mTaLineTimer = 0;
                mTaIndex = mTaCursor;
            }
            mTaLinePhase = (f32)mTaLineTimer;
        }
        break;
    default:
        if (mTaLineTimer > 0) {
            mTaLineTimer--;
            if (mTaLineTimer < 0)
                mTaLineTimer = 0;
        }
        mTaLinePhase = (f32)mTaLineTimer;
        break;
    }

    mTaLineAnm->setFrame(mTaLinePhase);
    for (int i = 0; i < 5; i++)
        mTaLineRank[i]->animationTransform();
}

void SceneRecord::calcTexChangeRecordTA() {
    int i;
    ECourseID courseID = SceneCourseSelect::mCourseData[mTaCursor / 4][mTaCursor % 4].courseID;
    mTaReOpPane->changeTexture((ResTIMG *)CourseName2D::ptr()->getCourseNameTexture(courseID), 0);
    TARecord rec = *gSystemRecord.getBestLap(courseID);
    bool valid = rec.mKartID < 0x15 && rec.mCharIDs[0] != 0 && rec.mCharIDs[0] < 0x15 && rec.mCharIDs[1] != 0 && rec.mCharIDs[1] < 0x15;
    if (valid) {
        u8 c[2];
        c[0] = rec.mCharIDs[0];
        c[1] = rec.mCharIDs[1];
        u8 kart = rec.mKartID;
        u8 char1 = c[0];
        u8 char2 = c[1];
        if (c[0] == 0)
            char1 = 1;
        if (c[1] == 0)
            char2 = 1;
        for (i = 0; i < 3; i++) {
            mTaChar[i]->show();
            mTaCharWin[i]->show();
        }
        mTaChar[0]->changeTexture(Kart2DCommon::ptr()->getCharIconTexture((ECharID)char1), 0);
        mTaChar[1]->changeTexture(Kart2DCommon::ptr()->getCharIconTexture((ECharID)char2), 0);
        mTaChar[2]->changeTexture(Kart2DCommon::ptr()->getKartIconTexture(kart), 0);

        int time = rec.getRaceTime().get();
        if (!rec.getRaceTime().isAvailable()) {
            for (i = 0; i < 7; i++) {
                mTaBestLap[i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture('-'), 0);
                mTaBestLap[i]->show();
            }
        } else {
            mTaBestLap[0]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(time % 10)), 0);
            mTaBestLap[1]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 10) % 10)), 0);
            mTaBestLap[2]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 100) % 10)), 0);
            mTaBestLap[3]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 1000) % 10)), 0);
            mTaBestLap[4]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 10000) % 6)), 0);
            mTaBestLap[5]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 60000) % 10)), 0);
            mTaBestLap[6]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 600000) % 10)), 0);
            if ((time / 600000) % 10 != 0)
                mTaBestLap[6]->show();
            else
                mTaBestLap[6]->hide();
        }
        char *name = rec.mName;
        for (i = 0; i < 3; i++)
            mTaName[i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture(name[i]), 0);
    } else {
        for (i = 0; i < 3; i++) {
            mTaChar[i]->hide();
            mTaCharWin[i]->hide();
        }
        for (i = 0; i < 7; i++) {
            mTaBestLap[i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture('-'), 0);
            mTaBestLap[i]->show();
        }
        for (i = 0; i < 3; i++)
            mTaName[i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture('-'), 0);
    }

    for (i = 0; i < 5; i++)
        calcTexChangeRecordTALine(i);
}

void SceneRecord::calcTexChangeRecordTALine(int rank) {
    int i;
#line 1685
    JUT_MINMAX_ASSERT(0, rank, 5);
    TARecord rec = *gSystemRecord.getTARecord(SceneCourseSelect::mCourseData[mTaCursor / 4][mTaCursor % 4].courseID, rank);
    bool valid = rec.mKartID < 0x15 && rec.mCharIDs[0] != 0 && rec.mCharIDs[0] < 0x15 && rec.mCharIDs[1] != 0 && rec.mCharIDs[1] < 0x15;
    if (valid) {
        u8 c[2];
        c[0] = rec.mCharIDs[0];
        c[1] = rec.mCharIDs[1];
        u8 kart = rec.mKartID;
        u8 char1 = c[0];
        u8 char2 = c[1];
        if (c[0] == 0)
            char1 = 1;
        if (c[1] == 0)
            char2 = 1;
        for (i = 0; i < 3; i++) {
            mTaLineTime[rank][i]->show();
            mTaLineQuote[rank][i]->show();
        }
        mTaLineTime[rank][0]->changeTexture(Kart2DCommon::ptr()->getCharIconTexture((ECharID)char1), 0);
        mTaLineTime[rank][1]->changeTexture(Kart2DCommon::ptr()->getCharIconTexture((ECharID)char2), 0);
        mTaLineTime[rank][2]->changeTexture(Kart2DCommon::ptr()->getKartIconTexture(kart), 0);

        RaceTime rt = rec.getRaceTime();
        int time = rt.get();
        if (!rt.isAvailable()) {
            for (i = 0; i < 7; i++) {
                Kart2DCommon::ptr()->changeNumberTexture(0, &mTaLineChar[rank][i], 1, false, true);
                mTaLineChar[rank][i]->show();
            }
        } else {
            mTaLineChar[rank][0]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)(time % 10)), 0);
            mTaLineChar[rank][1]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 10) % 10)), 0);
            mTaLineChar[rank][2]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 100) % 10)), 0);
            mTaLineChar[rank][3]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 1000) % 10)), 0);
            mTaLineChar[rank][4]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 10000) % 6)), 0);
            mTaLineChar[rank][5]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 60000) % 10)), 0);
            mTaLineChar[rank][6]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)((time / 600000) % 10)), 0);
            if ((time / 600000) % 10 != 0)
                mTaLineChar[rank][6]->show();
            else
                mTaLineChar[rank][6]->hide();
        }
        char *name = rec.mName;
        for (i = 0; i < 3; i++)
            mTaLineCharWin[rank][i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture(name[i]), 0);
    } else {
        for (i = 0; i < 3; i++) {
            mTaLineTime[rank][i]->hide();
            mTaLineQuote[rank][i]->hide();
        }
        for (i = 0; i < 7; i++) {
            Kart2DCommon::ptr()->changeNumberTexture(0, &mTaLineChar[rank][i], 1, false, true);
            mTaLineChar[rank][i]->show();
        }
        for (i = 0; i < 3; i++)
            mTaLineCharWin[rank][i]->changeTexture(Kart2DCommon::ptr()->getAsciiTexture('-'), 0);
    }
}

void SceneRecord::calcAnm() {
    int i;
    switch (mState) {
    case 0:
    case 1:
        mModeCursor++;
        if (mModeCursor > 7)
            mModeCursor = 7;
        break;
    default:
        mModeCursor--;
        if (mModeCursor < 0)
            mModeCursor = 0;
        break;
    }
    mMenuAnm0->setFrame((f32)mModeCursor);
    mRecordPane->animationTransform();
    calcAnmRecordCursor();

    switch (mState) {
    case 2:
        mGpTimer++;
        if (mGpTimer > 7)
            mGpTimer = 7;
        break;
    default:
        mGpTimer--;
        if (mGpTimer < 0)
            mGpTimer = 0;
        break;
    }
    mGpAnm0->setFrame((f32)mGpTimer);
    for (i = 0; i < 3; i++)
        mGpAllPanes[i]->animationTransform();
    calcAnmRecordGPCursor();
    calcAnmRecordGPLineAll();

    for (i = 0; i < 4; i++) {
        GPRecord rec = gSystemRecord.getGPRecord((ERaceGpCup)ConvNum2ERace(mGpIndex), (ERaceLevel)i);
        if ((rec.mFlags & 2) && rec.isValid())
            mGpLineTag[i]->show();
        else
            mGpLineTag[i]->hide();
    }

    switch (mState) {
    case 3:
        mTaTimer++;
        if (mTaTimer > 7)
            mTaTimer = 7;
        break;
    default:
        mTaTimer--;
        if (mTaTimer < 0)
            mTaTimer = 0;
        break;
    }
    mTaAnm0->setFrame((f32)mTaTimer);
    for (i = 0; i < 3; i++)
        mTaAllPanes[i]->animationTransform();
    calcAnmRecordTACursor();
    calcAnmRecordTALineAll();

    mSubCursorPanes1[0][0]->changeTexture(mBtiName_Cup[mGpCursor], 0);
    mSubCursorPanes1[1][0]->changeTexture((ResTIMG *)CourseName2D::ptr()->getCourseNameTexture(
                                           SceneCourseSelect::mCourseData[mTaCursor / 4][mTaCursor % 4].courseID),
                                       0);
    mGpReOpPane->changeTexture(mBtiName_Cup[mGpCursor], 0);

    for (i = 0; i < 4; i++)
        calcTexChangeRecordGPLine((ERaceLevel)i);
    calcTexChangeRecordTA();

    int dir;
    switch (mState) {
    case 6:
        dir = -1;
        break;
    default:
        dir = 1;
        break;
    }
    calcAnmTitleLine(dir);
}

void SceneRecord::calcMatAnmRecordCursor() {
    int i;
    int j;
    if (++mMatCursorTimer > 119)
        mMatCursorTimer = 0;
    mMenuAnm1->setFrame((f32)mMatCursorTimer);
    mSubAnm2->setFrame((f32)mMatCursorTimer);

    for (i = 0; i < 2; i++) {
        if (mMenuCursor == i) {
            if (++mSubCursorTimer4[i] > 119)
                mSubCursorTimer4[i] = 0;
        } else {
            mSubCursorTimer4[i] = 0;
        }
        mSubAnm3[i]->setFrame((f32)mSubCursorTimer4[i]);

        for (j = 0; j < 2; j++) {
            if (i != mMenuCursor)
                mSubMatTimer[i][j] = 0;
            else if (mSubMatTimer[i][j] > 1)
                mSubMatTimer[i][j]++;
            else
                mSubMatTimer[i][j] = 1;
            if (mSubMatTimer[i][j] < 0)
                mSubMatTimer[i][j] = 0;
            if (mSubMatTimer[i][j] > 19)
                mSubMatTimer[i][j] = 1;
            mSubAnm[i][j]->setFrame((f32)mSubMatTimer[i][j]);
        }
    }
}

void SceneRecord::calcMatAnmRecordMenu() {}

void SceneRecord::calcMatAnmRecordGPCursor() {}

void SceneRecord::calcMatAnmRecordTACursor() {}

void SceneRecord::calcMatAnmRecordBack() {}

void SceneRecord::calcMatAnm() {
    int i;
    calcMatAnmRecordCursor();
    for (i = 0; i < 3; i++) {
        if (mMenuCursor == i)
            mMatMenuTimer[i] = 1;
        else
            mMatMenuTimer[i] = 0;
        mMenuAnm2[i]->setFrame((f32)mMatMenuTimer[i]);
    }
    mMenuScreen->animation();

    if (++mGpMatTimer > 119)
        mGpMatTimer = 0;
    mGpAnm1->setFrame((f32)mGpMatTimer);
    for (i = 0; i < 2; i++) {
        if (mGpTextFrame[i] >= 1)
            mGpTextFrame[i]++;
        else
            mGpTextFrame[i] = 0;
        if (mGpTextFrame[i] < 0)
            mGpTextFrame[i] = 0;
        if (mGpTextFrame[i] > 19)
            mGpTextFrame[i] = 0;
        mGpAnm2[i]->setFrame((f32)mGpTextFrame[i]);
    }
    mGpScreen->animation();

    if (++mTaMatTimer > 119)
        mTaMatTimer = 0;
    mTaAnm1->setFrame((f32)mTaMatTimer);
    for (i = 0; i < 2; i++) {
        if (mTaTextFrame[i] >= 1)
            mTaTextFrame[i]++;
        else
            mTaTextFrame[i] = 0;
        if (mTaTextFrame[i] < 0)
            mTaTextFrame[i] = 0;
        if (mTaTextFrame[i] > 19)
            mTaTextFrame[i] = 0;
        mTaAnm2[i]->setFrame((f32)mTaTextFrame[i]);
    }
    mTaScreen->animation();

    if (++mBackFlag > 540)
        mBackFlag = 0;
    mBackAnm->setFrame((f32)mBackFlag);
    mBackScreen->animation();

    calcMatAnmTitleLine();
}

void SceneRecord::calc() {
    switch (mState) {
    case 0:
        fadeIn();
        break;
    case 1:
        mode();
        break;
    case 2:
        gp();
        break;
    case 3:
        ta();
        break;
    case 4:
        record();
        break;
    case 5:
        piece();
        break;
    case 6:
        nextScene();
        break;
    default:
#line 2009
        JUT_ASSERT(0);
    }
    calcMatAnm();
    calcAnm();
}

void SceneRecord::fadeIn() {
    if (++mFadeTimer > 15) {
        mState = 1;
        mPrevState = 0;
        mFadeTimer = 0;
    }
}

void SceneRecord::mode() {
    if (gpaGamePad[0]->testTrigger(JUTGamePad::A)) {
        switch (mMenuCursor) {
        case 0:
            mState = 2;
            mPrevState = 1;
            GameAudio::Main::getAudio()->startSystemSe(0x20003);
            break;
        case 1:
            mState = 3;
            mPrevState = 1;
            GameAudio::Main::getAudio()->startSystemSe(0x20003);
            break;
        case 2:
            mState = 6;
            mPrevState = 1;
            System::getDisplay()->startFadeOut(15);
            GameAudio::Main::getAudio()->startSystemSe(0x20004);
            GameAudio::Main::getAudio()->fadeOutAll(15);
            break;
        }
    } else if (gpaGamePad[0]->testTrigger(JUTGamePad::B)) {
        mState = 6;
        mPrevState = 1;
        System::getDisplay()->startFadeOut(15);
        GameAudio::Main::getAudio()->startSystemSe(0x20004);
        GameAudio::Main::getAudio()->fadeOutAll(15);
    } else if (gpaGamePad[0]->testTrigger(JUTGamePad::MAINSTICK_UP)) {
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
        if (--mMenuCursor < 0)
            mMenuCursor = 2;
    } else if (gpaGamePad[0]->testTrigger(JUTGamePad::MAINSTICK_DOWN)) {
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
        if (++mMenuCursor >= 3)
            mMenuCursor = 0;
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_LEFT)) {
        if (mMenuCursor >= 2)
            return;
        GameAudio::Main::getAudio()->startSystemSe(0x2000c);
        int &t0 = mSubCursorTimer2[mMenuCursor][0];
        t0 = 30;
        int &t1 = mSubCursorTimer2[mMenuCursor][1];
        t1 = 30;
        int &t2 = mSubMatTimer[mMenuCursor][0];
        t2 = 2;
        for (int i = 0; i < 2; i++) {
            if (mMenuCursor == i) {
                switch (mMenuCursor) {
                case 0:
                    if (--mGpCursor < 0)
                        mGpCursor = mModeCount - 1;
                    mGpIndex = mGpCursor;
                    break;
                case 1:
                    if (--mTaCursor < 0)
                        mTaCursor = mLevelCount - 1;
                    mTaIndex = mTaCursor;
                    break;
                }
            }
        }
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT)) {
        if (mMenuCursor >= 2)
            return;
        GameAudio::Main::getAudio()->startSystemSe(0x2000c);
        int &t0 = mSubCursorTimer2[mMenuCursor][0];
        t0 = 30;
        int &t1 = mSubCursorTimer2[mMenuCursor][2];
        t1 = 30;
        int &t2 = mSubMatTimer[mMenuCursor][1];
        t2 = 2;
        for (int i = 0; i < 2; i++) {
            if (mMenuCursor == i) {
                switch (mMenuCursor) {
                case 0:
                    if (++mGpCursor >= mModeCount)
                        mGpCursor = 0;
                    mGpIndex = mGpCursor;
                    break;
                case 1:
                    if (++mTaCursor >= mLevelCount)
                        mTaCursor = 0;
                    mTaIndex = mTaCursor;
                    break;
                }
            }
        }
    }
}

void SceneRecord::gp() {
    if (gpaGamePad[0]->testTrigger(JUTGamePad::B)) {
        GameAudio::Main::getAudio()->startSystemSe(0x20005);
        mState = 1;
        mPrevState = 2;
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_LEFT)) {
        GameAudio::Main::getAudio()->startSystemSe(0x2000c);
        mGpTextTimer = 50;
        mGpCursorFrame[0] = 50;
        mGpTextFrame[0] = 1;
        mGpLineDir = -1;
        if (--mGpCursor < 0)
            mGpCursor = mModeCount - 1;
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT)) {
        GameAudio::Main::getAudio()->startSystemSe(0x2000c);
        mGpTextTimer = 50;
        mGpCursorFrame[1] = 50;
        mGpTextFrame[1] = 1;
        mGpLineDir = 1;
        if (++mGpCursor >= mModeCount)
            mGpCursor = 0;
    }
}

void SceneRecord::ta() {
    if (gpaGamePad[0]->testTrigger(JUTGamePad::B)) {
        GameAudio::Main::getAudio()->startSystemSe(0x20005);
        mState = 1;
        mPrevState = 3;
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_LEFT)) {
        GameAudio::Main::getAudio()->startSystemSe(0x2000c);
        mTaTextTimer = 50;
        mTaCursorFrame[0] = 50;
        mTaTextFrame[0] = 1;
        mTaLineDir = -1;
        if (--mTaCursor < 0)
            mTaCursor = mLevelCount - 1;
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT)) {
        GameAudio::Main::getAudio()->startSystemSe(0x2000c);
        mTaTextTimer = 50;
        mTaCursorFrame[1] = 50;
        mTaTextFrame[1] = 1;
        mTaLineDir = 1;
        if (++mTaCursor >= mLevelCount)
            mTaCursor = 0;
    }
}

void SceneRecord::record() {}

void SceneRecord::piece() {}

void SceneRecord::nextScene() {
    if (mSceneType == Scene::SCENE_MAX) {
        AppMgr::deleteCurrentApp();
        MainMenuApp::call();
    } else if (++mFadeTimer > 15) {
        mSceneType = 0;
        if (SequenceApp::ptr()->ready((Scene::SceneType)mSceneType)) {
            GameAudio::Main::getAudio()->stopSequenceBgm(1);
            GameAudio::Main::getAudio()->stopSystemSe(0x20045);
            SequenceApp::ptr()->setNextScene((Scene::SceneType)mSceneType);
        }
    }
}

void SceneRecord::getKartID(EKartID) {}

int SceneRecord::ConvNum2ERace(int num) {
#line 2390
    JUT_MINMAX_ASSERT(0, num, 5);
    int level;
    switch (num) {
    case 0:
        level = 0;
        break;
    case 1:
        level = 1;
        break;
    case 2:
        level = 2;
        break;
    case 3:
        level = 3;
        break;
    case 4:
        level = 4;
        break;
    }
    return level;
}

#include "JSystem/JAudio/JASFakeMatch.h"
