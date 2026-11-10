#include "Kameda/SceneRecord.h"
#include "Kameda/Scene.h"
#include "Kameda/CourseName2D.h"
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

#pragma dont_inline on
static J2DPicture::TCornerColor makeCornerColor() { return J2DPicture::TCornerColor(); }
#pragma dont_inline off


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

const u64 SceneRecord::mTagRecord_submenu2[] = {
    'RecCo1', 'NRecS1Ls', 'NRecS1Rs', 'RecCo2',
    'NRecS2Ls', 'NRecS2Rs',
};

const u64 SceneRecord::mTagRecord_submenucursor[] = {
    'NRecS1L', 'NRecS1R', 'NRecS2L', 'NRecS2R',
};

const u64 SceneRecord::mTagRecord_submenucursor2[] = {
    'RecSL1L', 'RecSL1R', 'RecSL2L', 'RecSL2R',
};

const u64 SceneRecord::mTagRecord_submenucursor3[] = {
    'RecSB1L', 'RecSB1R', 'RecSB2L', 'RecSB2R',
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

SceneRecord::SceneRecord(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap), mTitleLine(heap) {
    J2DPicture::TCornerColor color = makeCornerColor();
}

void SceneRecord::SceneRecord_ConstructerMenu(JKRHeap *) {}

void SceneRecord::SceneRecord_ConstructerGP(JKRHeap *) {}

void SceneRecord::SceneRecord_ConstructerGPLine(JKRHeap *) {}

void SceneRecord::SceneRecord_ConstructerTA(JKRHeap *) {}

void SceneRecord::SceneRecord_ConstructerTALine(JKRHeap *) {}

void SceneRecord::SceneRecord_ConstructerBack(JKRHeap *) {}

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
    mTitleLine.init_titleline(7);

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
    mModeCursor = -1;
    mMatCursorTimer = 0;
    for (int i = 0; i < 3; i++)
        mMatMenuTimer[i] = 0;
    for (int i = 0; i < 2; i++) {
        mSubCursorTimer[i] = 20;
        for (int j = 0; j < 3; j++)
            mSubCursorTimer2[i * 3 + j] = 29;
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
    mGpLinePhase = 0.0f;
    mGpLineDir = 0;
    for (int i = 0; i < 4; i++) {
        mGpLineCursorPanes[i]->changeTexture(mBtiName_Level[i], 0);
        mGpLinePanes3[i]->changeTexture(Kart2DCommon::ptr()->getPartsLocName(0), 0);
        mGpLineColon[i * 2]->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, mBtiName_colon1), 0);
        mGpLineColon[i * 2 + 1]->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, mBtiName_colon2), 0);
        for (int j = 0; j < 3; j++)
            mGpLineCharWin[i * 3 + j]->changeTexture(mBtiName_characterwindow, 0);
    }
    if (gSystemRecord.isGameUnlocked(SystemRecord::Mirror))
        mGpLineScreen[3]->show();
    else
        mGpLineScreen[3]->hide();
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
    mTaLineTimer = -1;
    mTaLinePhase = 0.0f;
    mTaLineDir = 0;
    for (int i = 0; i < 5; i++) {
        mTaLineRank[i]->changeTexture(mBtiName_Rank[i], 0);
        mTaLineColon[i * 2]->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, mBtiName_colon1), 0);
        mTaLineColon[i * 2 + 1]->changeTexture((ResTIMG *)ResMgr::getPtr(ResMgr::mcArcMRAMLoc, mBtiName_colon2), 0);
        for (int j = 0; j < 3; j++)
            mTaLineCharWin[i * 3 + j]->changeTexture(mBtiName_characterwindow, 0);
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
    mOrtho->setPort();
    mBackScreen->draw(0.0f, 0.0f, mOrtho);
    mMenuScreen->draw(0.0f, 0.0f, mOrtho);
    mGpScreen->draw(0.0f, 0.0f, mOrtho);

    int count = 0;
    for (int i = 0; i < 4; i++) {
        GPRecord rec = gSystemRecord.getGPRecord((ERaceGpCup)ConvNum2ERace(mGpIndex), (ERaceLevel)i);
        if ((rec.mFlags & 2) && rec.isValid())
            count++;
    }

    for (int i = 0; i < 4; i++) {
        if (count != 0)
            mGpLineScreen[i]->draw(mGpLineX[i * 2], mGpLineX[i * 2 + 1], mOrtho);
        else
            mGpLineScreen[i]->draw(mGpLineY[i * 2], mGpLineY[i * 2 + 1], mOrtho);
    }

    mTaScreen->draw(0.0f, 0.0f, mOrtho);
    for (int i = 0; i < 5; i++)
        mTaLineScreen[i]->draw(mTaLineX[i * 2], mTaLineX[i * 2 + 1], mOrtho);
    mTitleLine.drawTitleLine(mOrtho);
}

void SceneRecord::calcAnmRecordAll() {}

void SceneRecord::calcAnmRecordCursor() {
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
        for (int j = 0; j < 2; j++)
            mSubCursorPanes2[i * 2 + j]->animationTransform();

        for (int j = 0; j < 3; j++) {
            int *t = &mSubCursorTimer2[i * 3 + j];
            if (*t <= 29) {
                mMenuAnm0->setFrame(30.0f);
                mSubCursorPanes1[i * 3 + j]->animationTransform();
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
                mSubCursorPanes1[i * 3 + j]->animationTransform();
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

void SceneRecord::calcTexChangeRecordGPLine(ERaceLevel) {}

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

void SceneRecord::calcTexChangeRecordTA() {}

void SceneRecord::calcTexChangeRecordTALine(int) {}

void SceneRecord::calcAnm() {
    if (mState >= 2)
        goto dec;
    if (mState >= 0) {
        mModeCursor++;
        if (mModeCursor > 7)
            mModeCursor = 7;
    } else {
dec:
        mModeCursor--;
        if (mModeCursor < 0)
            mModeCursor = 0;
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
    for (int i = 0; i < 3; i++)
        mGpAllPanes[i]->animationTransform();
    calcAnmRecordGPCursor();
    calcAnmRecordGPLineAll();

    for (int i = 0; i < 4; i++) {
        GPRecord rec = gSystemRecord.getGPRecord((ERaceGpCup)ConvNum2ERace(mGpIndex), (ERaceLevel)i);
        bool valid = (rec.mFlags & 2) && rec.isValid();
        if (valid)
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
    for (int i = 0; i < 3; i++)
        mTaAllPanes[i]->animationTransform();
    calcAnmRecordTACursor();
    calcAnmRecordTALineAll();

    mSubCursorPanes1[0]->changeTexture(mBtiName_Cup[mGpCursor], 0);
    mSubCursorPanes1[3]->changeTexture((ResTIMG *)CourseName2D::ptr()->getCourseNameTexture(
                                           SceneCourseSelect::mCourseData[mTaCursor / 4][mTaCursor % 4].courseID),
                                       0);
    mGpReOpPane->changeTexture(mBtiName_Cup[mGpCursor], 0);

    for (int i = 0; i < 4; i++)
        calcTexChangeRecordGPLine((ERaceLevel)i);
    calcTexChangeRecordTA();

    if (mState == 6)
        mTitleLine.calcAnmTitleLine(-1);
    else
        mTitleLine.calcAnmTitleLine(1);
}

void SceneRecord::calcMatAnmRecordCursor() {
    mMatCursorTimer++;
    if (mMatCursorTimer > 119)
        mMatCursorTimer = 0;
    mMenuAnm1->setFrame((f32)mMatCursorTimer);
    mSubAnm2->setFrame((f32)mMatCursorTimer);

    for (int i = 0; i < 2; i++) {
        if (mMenuCursor == i) {
            if (++mSubCursorTimer4[i] > 119)
                mSubCursorTimer4[i] = 0;
        } else {
            mSubCursorTimer4[i] = 0;
        }
        mSubAnm3[i]->setFrame((f32)mSubCursorTimer4[i]);

        for (int j = 0; j < 2; j++) {
            int *t = &mSubMatTimer[i * 2 + j];
            if (mMenuCursor != i) {
                *t = 0;
            } else if (*t > 1) {
                (*t)++;
            } else {
                *t = 1;
            }
            if (*t < 0)
                *t = 0;
            if (*t > 19)
                *t = 1;
            mSubAnm[i * 2 + j]->setFrame((f32)*t);
        }
    }
}

void SceneRecord::calcMatAnmRecordMenu() {}

void SceneRecord::calcMatAnmRecordGPCursor() {}

void SceneRecord::calcMatAnmRecordTACursor() {}

void SceneRecord::calcMatAnmRecordBack() {}

void SceneRecord::calcMatAnm() {
    calcMatAnmRecordCursor();
    for (int i = 0; i < 3; i++) {
        if (mMenuCursor == i)
            mMatMenuTimer[i] = 1;
        else
            mMatMenuTimer[i] = 0;
        mMenuAnm2[i]->setFrame((f32)mMatMenuTimer[i]);
    }
    mMenuScreen->animation();

    mGpMatTimer++;
    if (mGpMatTimer > 119)
        mGpMatTimer = 0;
    mGpAnm1->setFrame((f32)mGpMatTimer);
    for (int i = 0; i < 2; i++) {
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

    mTaMatTimer++;
    if (mTaMatTimer > 119)
        mTaMatTimer = 0;
    mTaAnm1->setFrame((f32)mTaMatTimer);
    for (int i = 0; i < 2; i++) {
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

    mBackFlag++;
    if (mBackFlag > 540)
        mBackFlag = 0;
    mBackAnm->setFrame((f32)mBackFlag);
    mBackScreen->animation();

    mTitleLine.calcMatAnmTitleLine();
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
        mSubCursorTimer2[mMenuCursor * 3] = 30;
        mSubCursorTimer2[mMenuCursor * 3 + 1] = 30;
        mSubMatTimer[mMenuCursor * 2] = 2;
        for (int i = 0; i < 2; i++) {
            switch (mMenuCursor) {
            case 0:
                if (--mGpCursor < 0)
                    mGpCursor = mModeCount - 1;
                mGpIndex = mGpCursor;
                break;
            case 1:
                if (--mTaCursor < 0)
                    mTaCursor = mModeCount - 1;
                mTaIndex = mTaCursor;
                break;
            }
        }
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT)) {
        if (mMenuCursor >= 2)
            return;
        GameAudio::Main::getAudio()->startSystemSe(0x2000c);
        mSubCursorTimer2[mMenuCursor * 3] = 30;
        mSubCursorTimer2[mMenuCursor * 3 + 2] = 30;
        mSubMatTimer[mMenuCursor * 2 + 1] = 2;
        for (int i = 0; i < 2; i++) {
            switch (mMenuCursor) {
            case 0:
                if (++mGpCursor >= mModeCount)
                    mGpCursor = 0;
                mGpIndex = mGpCursor;
                break;
            case 1:
                if (++mTaCursor >= mModeCount)
                    mTaCursor = 0;
                mTaIndex = mTaCursor;
                break;
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
