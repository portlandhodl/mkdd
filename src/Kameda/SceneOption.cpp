#include "Kameda/SceneOption.h"
#include "Kameda/Scene.h"
#include "Osako/kartPad.h"
#include "JSystem/JUtility/JUTGamePad.h"
#include "mathHelper.h"
#include "JSystem/J2D/J2DScreen.h"
#include "JSystem/J2D/J2DPane.h"
#include "JSystem/J2D/J2DPicture.h"
#include "Osako/Kart2DCommon.h"
#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "JSystem/JFramework/JFWDisplay.h"
#include "JSystem/JUtility/JUTFader.h"
#include "Inagaki/GameAudioMain.h"
#include "Osako/system.h"
#include "Osako/AppMgr.h"
#include "Osako/MainMenuApp.h"
#include "Osako/SequenceApp.h"
#include "Osako/SystemRecord.h"
#include "Osako/CardAgent.h"
#include "dolphin/os.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kameda/PrintMemoryCard.h"

const u64 SceneOption::mTag_menu[] = {
    'Sd_M', 'BGM_M', 'Rm_M', 'Gh_M', 
    'Is_M', 'Lp_M', 'ReTtl_M', 
};

const u64 SceneOption::mTag_secret[] = {
    'Ques1', 'Ques2', 'Ques3', 
};

const u64 SceneOption::mTag_cursor[] = {
    'SdCs', 'BGMCs', 'RmCs', 'GhCs', 
    'IsCs', 'LpCs', 'ReTtlCs', 
};

const u64 SceneOption::mTag_submenu[] = {
    'NSdSb', 'NSbBGM', 'NSbRm', 'NSbGh', 
    'NSbIs', 'NSbLp', 
};

const u64 SceneOption::mTag_submenu_bgms[] = {
    'SbBGMw0', 'SbBGMw1', 'SbBGMw2', 
};

const u64 SceneOption::mTag_cursorl1[] = {
    'NSdSbL1', 'NSbBGML1', 'NSbRmL1', 'NSbGhL1', 
    'NSbIsL1', 'NSbLpL1',
};

const u64 SceneOption::mTag_cursorr1[] = {
    'NSdSbR1', 'NSbBGMR1', 'NSbRmR1', 'NSbGhR1', 
    'NSbIsR1', 'NSbLpR1', 
};

const u64 SceneOption::mTag_cursorl2[] = {
    'NSdSbL2', 'NSbBGML2', 'NSbRmL2', 'NSbGhL2', 
    'NSbIsL2', 'NSbLpL2',
};

const u64 SceneOption::mTag_cursorr2[] = {
    'NSdSbR2', 'NSbBGMR2', 'NSbRmR2', 'NSbGhR2', 
    'NSbIsR2', 'NSbLpR2',
};

const u64 SceneOption::mTag_cursorl3[] = {
   'SdSbL1', 'SbBGML1', 'SbRmL1', 'SbGhL1', 
    'SbIsL1', 'SbLpL1', 
};

const u64 SceneOption::mTag_cursorr3[] = {
    'SdSbR1', 'SbBGMR1', 'SbRmR1', 'SbGhR1', 
    'SbIsR1', 'SbLpR1', 
};

const u64 SceneOption::mTag_cursorl4[] = {
    'SdSbL2', 'SbBGML2', 'SbRmL2', 'SbGhL2', 
    'SbIsL2', 'SbLpL2', 
};
const u64 SceneOption::mTag_cursorr4[] = {
    'SdSbR2', 'SbBGMR2', 'SbRmR2', 'SbGhR2', 
    'SbIsR2', 'SbLpR2',
};

const u64 SceneOption::mTag_submenumoji[] = {
    'SbSd_M', 'XXXXXXXX', 'SbRm_M', 'SbGh_M', 
    'SbIs_M', 'SbLp_M', 
};

const u64 SceneOption::mTag_color[] = {
    'CSel1', 'CSel2', 
};

const u64 SceneOption::mTag_knob[] = {
    'BGMKnob', 'SEKnob', 
};

const u64 SceneOption::mTag_volume[] = {
    'BGMBar2', 'SEBar2',
};

const char *SceneOption::mBtiName_sound[] = {
    "Mozi_Mono.bti",
    "Mozi_Stereo.bti",
    "Mozi_DolbySurround.bti"
};

const char *SceneOption::mBtiName_onOff[] = {
    "Mozi_Off.bti",
    "Mozi_On.bti",
};

const char *SceneOption::mBtiName_itemSlot[] = {
    "Mozi_Standard.bti",
    "Mozi_Smash.bti",
    "Mozi_Super.bti",
    "Mozi_Simple.bti"
};

const char *SceneOption::mBtiName_bgmsign[] = {
    "MarioFont_Number_XLine.bti",
    "MarioFont_Number_Plus.bti",
};

const char *SceneOption::mBtiName_default = "Mozi_Question.bti";

const u64 SceneOption::mTagSP_menu[] = {
    'SPM1', 'SPM2', 'SPM3', 'SPM4', 
};

const u64 SceneOption::mTagSP_submenu[] = {
    'NSb1', 'NSb2', 'NSb3', 
};

const u64 SceneOption::mTagSP_cursor[] = {
    'Crs1', 'Crs2', 'Crs3', 'Crs4', 
};

const u64 SceneOption::mTagSP_cursorl1[] = {
    'NSbStL1', 'NSbStL2', 'NSbStL3', 
};

const u64 SceneOption::mTagSP_cursorr1[] = {
    'NSbStR1', 'NSbStR2', 'NSbStR3',
};

const u64 SceneOption::mTagSP_cursorl2[] = {
    'NSbSsL1', 'NSbSsL2', 'NSbSsL3', 
};

const u64 SceneOption::mTagSP_cursorr2[] = {
    'NSbSsR1', 'NSbSsR2', 'NSbSsR3', 
};

const u64 SceneOption::mTagSP_cursorl3[] = {
    'SbL1', 'SbL2', 'SbL3', 
};

const u64 SceneOption::mTagSP_cursorr3[] = {
    'SbR1', 'SbR2', 'SbR3', 
};

const u64 SceneOption::mTagSP_cursorl4[] = {
    'SbLL1', 'SbLL2', 'SbLL3',
};

const u64 SceneOption::mTagSP_cursorr4[] = {
    'SbLR1', 'SbLR2', 'SbLR3', 
};

const u64 SceneOption::mTagSP_numeral1[] = {
    'Sb1M11', 'Sb2M11', 'Sb3M11',
};

const u64 SceneOption::mTagSP_numeral21[] = {
    'Sb1M21', 'Sb2M21', 'Sb3M21', 
};

const u64 SceneOption::mTagSP_numeral22[] = {
    'Sb1M22', 'Sb2M22', 'Sb3M22',
};

const u64 SceneOption::mTagSP_numeral31[] = {
    'Sb1M31', 'Sb2M31', 'Sb3M31', 
};

const u64 SceneOption::mTagSP_numeral32[] = {
    'Sb1M32', 'Sb2M32', 'Sb3M32', 
};

const u64 SceneOption::mTagSP_numeral33[] = {
    'Sb1M33', 'Sb2M33', 'Sb3M33', 
};

const u64 SceneOption::mTagSP_color[] = {
    'CSel1', 'CSel2',
};

const u64 SceneOption::mTag_all = 'NOpAll';
const u64 SceneOption::mTag_submenu_bgm = 'NSbBGM1';
const u64 SceneOption::mTag_osusume = 'SbLp_M1';

SceneOption::SceneOption(JKRArchive *archive, JKRHeap *heap)
    : Scene(archive, heap), SceneTitleLine(archive, heap, "option_title_line") {
    OSInitFastCast();
    SceneOption_ConstructerOption(heap);
    SceneOption_ConstructerBack(heap);
}

void SceneOption::SceneOption_ConstructerOption(JKRHeap *heap) {
    int i;
    int j;

    mScreen = new (heap, 0) J2DScreen();
    mScreen->set("option.blo", 0x40000, mArchive);
    mAnmTransform = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("option.bck", mArchive));
    mScreen->setAnimation(mAnmTransform);

    void *brk = JKRFileLoader::getGlbResource("option.brk", mArchive);
    void *btk = JKRFileLoader::getGlbResource("option.btk", mArchive);
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 2; j++) {
            mCursorAnm[i].mAnm[j] = (J2DAnmTevRegKey *)J2DAnmLoaderDataBase::load(brk);
            mCursorAnm[i].mAnm[j]->searchUpdateMaterialID(mScreen);
        }
        mMatAnm[i] = (J2DAnmTextureSRTKey *)J2DAnmLoaderDataBase::load(btk);
        mMatAnm[i]->searchUpdateMaterialID(mScreen);
    }
    for (i = 0; i < 2; i++) {
        mAnm10c[i] = (J2DAnmTextureSRTKey *)J2DAnmLoaderDataBase::load(btk);
        mAnm10c[i]->searchUpdateMaterialID(mScreen);
    }

    mAllPane = mScreen->search(mTag_all);
    mAllPane->setAnimation(mAnmTransform);
    for (i = 0; i < 7; i++) {
        mMenuPanes[i] = (J2DPicture *)mScreen->search(mTag_menu[i]);
        mMenuPanes[i]->setAnimation(mAnmTransform);
        mCursorPanes[i] = (J2DPicture *)mScreen->search(mTag_cursor[i]);
        mCursorPanes[i]->setAnimation(mAnmTransform);
    }
    for (i = 0; i < 3; i++)
        mSecretPanes[i] = mScreen->search(mTag_secret[i]);
    for (i = 0; i < 6; i++) {
        mSubmenuPanes[i] = (J2DPicture *)mScreen->search(mTag_submenu[i]);
        mSubmenuPanes[i]->setAnimation(mAnmTransform);
        mPane1b4[i] = mScreen->search(mTag_cursorl1[i]);
        mPane1b4[i]->setAnimation(mAnmTransform);
        mPane1cc[i] = mScreen->search(mTag_cursorr1[i]);
        mPane1cc[i]->setAnimation(mAnmTransform);
        mPane1e4[i] = mScreen->search(mTag_cursorl2[i]);
        mPane1e4[i]->setAnimation(mAnmTransform);
        mPane1fc[i] = mScreen->search(mTag_cursorr2[i]);
        mPane1fc[i]->setAnimation(mAnmTransform);
        mPane214[i] = mScreen->search(mTag_cursorl3[i]);
        mPane214[i]->setAnimation(mCursorAnm[i].mAnm[0]);
        mPane22c[i] = mScreen->search(mTag_cursorr3[i]);
        mPane22c[i]->setAnimation(mCursorAnm[i].mAnm[1]);
        mPane244[i] = mScreen->search(mTag_cursorl4[i]);
        mPane244[i]->setAnimation(mMatAnm[i]);
        mPane25c[i] = mScreen->search(mTag_cursorr4[i]);
        mPane25c[i]->setAnimation(mMatAnm[i]);
        if (i != 1) {
            mPane274[i] = (J2DPicture *)mScreen->search(mTag_submenumoji[i]);
            mPane274[i]->setAnimation(mAnmTransform);
        }
    }
    mPane1a4 = (J2DPicture *)mScreen->search(mTag_submenu_bgm);
    mPane1a4->setAnimation(mAnmTransform);
    for (i = 0; i < 3; i++)
        mPane1a8[i] = (J2DPicture *)mScreen->search(mTag_submenu_bgms[i]);
    mPane28c = (J2DPicture *)mScreen->search(mTag_osusume);
    mPane28c->setAnimation(mAnmTransform);
    for (i = 0; i < 2; i++)
        mColorPanes[i] = (J2DPicture *)mScreen->search(mTag_color[i]);
    for (i = 0; i < 2; i++) {
        mKnobPanes[i] = (J2DPicture *)mScreen->search(mTag_knob[i]);
        mKnobPanes[i]->setAnimation(mAnmTransform);
        mVolumePanes[i] = (J2DPicture *)mScreen->search(mTag_volume[i]);
        mVolumePanes[i]->setAnimation(mAnm10c[i]);
    }

    if (System::getDisplay()->getFader()->mStatus == JUTFader::Status_Out) {
        mState = 0;
        mPrevState = 0;
        mFadeTimer = 0;
        if (System::getDisplay()->getFader())
            System::getDisplay()->getFader()->startFadeIn(15);
    } else {
        mState = 1;
        mPrevState = 1;
    }
    mMenuCursor = 0;
    mSoundMode = 0;
    mRumble = 1;
    mOnOff2 = 1;
    mBgmVolume = 20;
    mSeVolume = 100;
    mSPCursor = 0;
    for (i = 0; i < 3; i++)
        mSPValue[i] = 0;

    mColor1[0] = mColorPanes[0]->getBlack();
    mColor1[1] = mColorPanes[0]->getWhite();
    mColor0[0] = mColorPanes[1]->getBlack();
    mColor0[1] = mColorPanes[1]->getWhite();
}

void SceneOption::SceneOption_ConstructerSoundPlayer(JKRHeap *heap) {
    int i;
    int j;

    mSPScreen = new (heap, 0) J2DScreen();
    mSPScreen->set("soundplayer.blo", 0x40000, mArchive);
    mSPAnmTransform = (J2DAnmTransform *)J2DAnmLoaderDataBase::load(JKRFileLoader::getGlbResource("soundplayer.bck", mArchive));
    mSPScreen->setAnimation(mSPAnmTransform);

    void *brk = JKRFileLoader::getGlbResource("soundplayer.brk", mArchive);
    void *btk = JKRFileLoader::getGlbResource("soundplayer.btk", mArchive);
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 2; j++) {
            mSPCursorAnm[i].mAnm[j] = (J2DAnmTevRegKey *)J2DAnmLoaderDataBase::load(brk);
            mSPCursorAnm[i].mAnm[j]->searchUpdateMaterialID(mSPScreen);
        }
        mSPMatAnm[i] = (J2DAnmTextureSRTKey *)J2DAnmLoaderDataBase::load(btk);
        mSPMatAnm[i]->searchUpdateMaterialID(mSPScreen);
    }

    for (i = 0; i < 4; i++) {
        mSPMenuPanes[i] = (J2DPicture *)mSPScreen->search(mTagSP_menu[i]);
        mSPMenuPanes[i]->setAnimation(mSPAnmTransform);
        mSPCursorPanes[i] = (J2DPicture *)mSPScreen->search(mTagSP_cursor[i]);
        mSPCursorPanes[i]->setAnimation(mSPAnmTransform);
    }
    for (i = 0; i < 3; i++) {
        mSPSubmenuPanes[i] = (J2DPicture *)mSPScreen->search(mTagSP_submenu[i]);
        mSPSubmenuPanes[i]->setAnimation(mSPAnmTransform);
        mSPPaneL1[i] = mSPScreen->search(mTagSP_cursorl1[i]);
        mSPPaneL1[i]->setAnimation(mSPAnmTransform);
        mSPPaneR1[i] = mSPScreen->search(mTagSP_cursorr1[i]);
        mSPPaneR1[i]->setAnimation(mSPAnmTransform);
        mSPPaneL2[i] = mSPScreen->search(mTagSP_cursorl2[i]);
        mSPPaneL2[i]->setAnimation(mSPAnmTransform);
        mSPPaneR2[i] = mSPScreen->search(mTagSP_cursorr2[i]);
        mSPPaneR2[i]->setAnimation(mSPAnmTransform);
        mSPPaneL3[i] = mSPScreen->search(mTagSP_cursorl3[i]);
        mSPPaneL3[i]->setAnimation(mSPCursorAnm[i].mAnm[0]);
        mSPPaneR3[i] = mSPScreen->search(mTagSP_cursorr3[i]);
        mSPPaneR3[i]->setAnimation(mSPCursorAnm[i].mAnm[1]);
        mSPPaneL4[i] = mSPScreen->search(mTagSP_cursorl4[i]);
        mSPPaneL4[i]->setAnimation(mSPMatAnm[i]);
        mSPPaneR4[i] = mSPScreen->search(mTagSP_cursorr4[i]);
        mSPPaneR4[i]->setAnimation(mSPMatAnm[i]);
        mSPNumeral1[i] = (J2DPicture *)mSPScreen->search(mTagSP_numeral1[i]);
        mSPNumeral21[i] = (J2DPicture *)mSPScreen->search(mTagSP_numeral21[i]);
        mSPNumeral22[i] = (J2DPicture *)mSPScreen->search(mTagSP_numeral22[i]);
        mSPNumeral31[i] = (J2DPicture *)mSPScreen->search(mTagSP_numeral31[i]);
        mSPNumeral32[i] = (J2DPicture *)mSPScreen->search(mTagSP_numeral32[i]);
        mSPNumeral33[i] = (J2DPicture *)mSPScreen->search(mTagSP_numeral33[i]);
    }
    for (i = 0; i < 2; i++)
        mSPColorPanes[i] = (J2DPicture *)mSPScreen->search(mTagSP_color[i]);
}

void SceneOption::SceneOption_ConstructerBack(JKRHeap *heap) {
    mBackScreen = new (heap, 0) J2DScreen();
    mBackScreen->set("OptionBack.blo", 0x1040000, mArchive);
    mBackAnm = (J2DAnmTextureSRTKey *)J2DAnmLoaderDataBase::load(
        JKRFileLoader::getGlbResource("OptionBack.btk", mArchive));
    mBackScreen->setAnimation(mBackAnm);
}

void SceneOption::init_blo() {}

void SceneOption::init() {
    GameAudio::Main::getAudio()->startSequenceBgm(0x1000002);
    mState = mPrevState;
    mSceneType = SequenceApp::ptr()->getPrevScene();
    if (System::getDisplay()->getFader()->mStatus == JUTFader::Status_Out) {
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
    init_option();
    init_back();
    init_titleline(8);
}

void SceneOption::init_option() {
    if (!(gSystemRecord.mOptions & 4))
        mRumble = 1;
    else
        mRumble = 0;
    switch (gSystemRecord.mOptions & 3) {
    case 1:
        mSoundMode = 0;
        break;
    case 0:
        mSoundMode = 1;
        break;
    case 2:
        mSoundMode = 2;
        break;
    }
    if (!(gSystemRecord.mOptions & 8))
        mOnOff2 = 1;
    else
        mOnOff2 = 0;
    switch (gSystemRecord.mItemSlotType) {
    case 0:
        mItemSlot = 0;
        break;
    case 1:
        mItemSlot = 3;
        break;
    case 2:
        mItemSlot = 1;
        break;
    case 3:
        mItemSlot = 2;
        break;
    }
    mVsLap = gSystemRecord.mVsLapNum;
    mBgmVolume = (s8)gSystemRecord.mVolume + 10;
    mSeVolume = 100;
    mPrevSoundMode = mSoundMode;
    mPrevRumble = mRumble;
    mPrevOnOff2 = mOnOff2;
    mPrevItemSlot = mItemSlot;
    mPrevVsLap = mVsLap;
    mPrevBgmVolume = mBgmVolume;
    mPrevSeVolume = mSeVolume;
    mMatCursorTimer = -1;
    mMenuTimer = 0;
    for (int i = 0; i < 7; i++)
        mMatMenuTimer[i] = 20;
    for (int i = 0; i < 6; i++) {
        mCursorTimer[i] = 40;
        mSubCursorTimer[i][0] = 30;
        mSubCursorTimer[i][1] = 30;
        mSubCursorTimer[i][2] = 30;
        mSubCursorTimer2[i][0] = 0;
        mSubCursorTimer2[i][1] = 0;
        mSubCursorTimer3[i] = 0;
    }
    mRumbleTimer[0] = 0;
    for (int i = 0; i < 3; i++)
        mAnmEnable[i] = 1;
}

void SceneOption::init_soundplayer() {}

void SceneOption::init_back() {
    mBackAnmFrame = 0;
}

SceneOption::~SceneOption() {
    delete mScreen;
    delete mBackScreen;
}

void SceneOption::draw() {
    mOrtho->setPort();
    mBackScreen->draw(0.0f, 0.0f, mOrtho);
    mScreen->draw(0.0f, 0.0f, mOrtho);
    drawTitleLine(mOrtho);
}

void SceneOption::calcAnmAll() {
    switch (mState) {
    case 0:
    case 1:
        if (mMatCursorTimer > 80) {
            mMatCursorTimer--;
            if (mMatCursorTimer <= 80)
                mMatCursorTimer = 9;
        } else {
            mMatCursorTimer++;
            if (mMatCursorTimer < 0)
                mMatCursorTimer = 0;
            if (mMatCursorTimer > 9)
                mMatCursorTimer = 9;
        }
        break;
    case 2:
        mMatCursorTimer++;
        if (mMatCursorTimer > 9) {
            if (mMatCursorTimer < 80)
                mMatCursorTimer = 80;
            if (mMatCursorTimer > 89)
                mMatCursorTimer = 89;
        }
        break;
    case 3:
    case 4:
        mMatCursorTimer++;
        if (mMatCursorTimer < 80)
            mMatCursorTimer = 80;
        if (mMatCursorTimer > 89)
            mMatCursorTimer = 89;
        break;
    }

    mAnmTransform->setFrame((f32)mMatCursorTimer);
    mAllPane->animationTransform();
}

void SceneOption::calcAnmMenu() {
    int menuTimer = ++mMenuTimer;
    if (menuTimer >= 90)
        mMenuTimer = 0;
    for (int i = 0; i < 7; i++) {
        if (i == mMenuCursor) {
            mMatMenuTimer[i]++;
            mMenuPanes[i]->setBlackWhite(mColor1[0], mColor1[1]);
        } else {
            mMatMenuTimer[i]--;
            mMenuPanes[i]->setBlackWhite(mColor0[0], mColor0[1]);
        }
        if (mMatMenuTimer[i] < 20)
            mMatMenuTimer[i] = 20;
        if (mMatMenuTimer[i] > 25)
            mMatMenuTimer[i] = 25;
        mAnmTransform->setFrame((f32)mMatMenuTimer[i]);
        mMenuPanes[i]->animationTransform();
        if (i < 6)
            mSubmenuPanes[i]->animationTransform();
        if (i == 1)
            mPane1a4->animationTransform();
        mAnmTransform->setFrame((f32)mMenuTimer);
        mCursorPanes[i]->animationTransform();
    }
}

void SceneOption::calcAnmCursor() {
    for (int i = 0; i < 6; i++) {
        if (i == mMenuCursor) {
            mCursorTimer[i]++;
            if (i != 1)
                mPane274[i]->setBlackWhite(mColor1[0], mColor1[1]);
            switch (i) {
            case 5:
                mPane28c->setBlackWhite(mColor1[0], mColor1[1]);
                break;
            case 1:
                for (int j = 0; j < 3; j++)
                    mPane1a8[j]->setBlackWhite(mColor1[0], mColor1[1]);
                break;
            }
        } else {
            mCursorTimer[i] = 40;
            if (i != 1)
                mPane274[i]->setBlackWhite(mColor0[0], mColor0[1]);
            switch (i) {
            case 5:
                mPane28c->setBlackWhite(mColor0[0], mColor0[1]);
                break;
            case 1:
                for (int j = 0; j < 3; j++)
                    mPane1a8[j]->setBlackWhite(mColor0[0], mColor0[1]);
                break;
            }
        }

        if (mCursorTimer[i] < 40)
            mCursorTimer[i] = 40;
        if (mCursorTimer[i] > 70)
            mCursorTimer[i] = 40;

        for (int j = 0; j < 3; j++) {
            int *timer = &mSubCursorTimer[i][j];
            if (*timer > 30)
                (*timer)++;
            else
                *timer = 30;
            if (*timer < 30)
                *timer = 30;
            if (*timer > 39)
                *timer = 30;
        }

        mAnmTransform->setFrame((f32)mCursorTimer[i]);
        mPane1b4[i]->animationTransform();
        mPane1cc[i]->animationTransform();
        mAnmTransform->setFrame((f32)mSubCursorTimer[i][0]);
        mPane1e4[i]->animationTransform();
        mAnmTransform->setFrame((f32)mSubCursorTimer[i][1]);
        mPane1fc[i]->animationTransform();
        mAnmTransform->setFrame((f32)mSubCursorTimer[i][2]);
        if (i != 1)
            mPane274[i]->animationTransform();
        if (i == 5)
            mPane28c->animationTransform();
    }
}

void SceneOption::calcAnmVolume() {
    mAnmTransform->setFrame((f32)(mBgmVolume * 99) / 20.0f + 100.0f);
    mKnobPanes[0]->animationTransform();
    mAnmTransform->setFrame((f32)(mSeVolume * 99) / 100.0f + 100.0f);
    mKnobPanes[1]->animationTransform();
}

void SceneOption::calcTexChange() {
    mPane274[0]->changeTexture(mBtiName_sound[mSoundMode], 0);
    mPane274[2]->changeTexture(mBtiName_onOff[mRumble], 0);
    mPane274[3]->changeTexture(mBtiName_onOff[mOnOff2], 0);
    mPane274[4]->changeTexture(mBtiName_itemSlot[mItemSlot], 0);

    if (mVsLap == 0) {
        mPane274[5]->mIsVisible = 0;
        mPane28c->mIsVisible = 1;
    } else {
        mPane274[5]->mIsVisible = 1;
        mPane28c->mIsVisible = 0;
        mPane274[5]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)mVsLap), 0);
    }

    if (mBgmVolume == 10) {
        mPane1a8[0]->mIsVisible = 1;
        mPane1a8[1]->mIsVisible = 0;
        mPane1a8[2]->mIsVisible = 0;
    } else {
        mPane1a8[0]->mIsVisible = 0;
        mPane1a8[1]->mIsVisible = 1;
        mPane1a8[2]->mIsVisible = 1;
    }

    int num;
    if (mBgmVolume < 10) {
        mPane1a8[2]->changeTexture(mBtiName_bgmsign[0], 0);
        num = 10 - mBgmVolume;
    } else {
        mPane1a8[2]->changeTexture(mBtiName_bgmsign[1], 0);
        num = mBgmVolume - 10;
    }
    mPane1a8[1]->changeTexture(Kart2DCommon::ptr()->getNumberTexture((Kart2DCommon::NumberTextureID)num), 0);
}

void SceneOption::calcAnmSPAll() {}

void SceneOption::calcAnmSPMenu() {}

void SceneOption::calcAnmSPCursor() {}

void SceneOption::calcAnmSPNumeral() {}

void SceneOption::calcAnm() {
    calcAnmAll();
    calcAnmMenu();
    calcAnmCursor();
    calcAnmVolume();
    calcTexChange();

    int dir;
    switch (mState) {
    case 3:
    case 4:
        dir = -1;
        break;
    default:
        dir = 1;
        break;
    }
    calcAnmTitleLine(dir);
}

void SceneOption::calcMatAnmCursor() {
    for (int i = 0; i < 7; i++) {
        if (i == mMenuCursor) {
            mCursorPanes[i]->mIsVisible = 1;
            if (i < 6) {
                for (int j = 0; j < 2; j++) {
                    int *timer = &mSubCursorTimer2[i][j];
                    if (*timer > 1)
                        (*timer)++;
                    else
                        *timer = 1;
                    if (*timer < 1)
                        *timer = 1;
                    if (*timer > 19)
                        *timer = 1;
                    mCursorAnm[i].mAnm[j]->setFrame((f32)*timer);
                }
                int *timer = &mSubCursorTimer3[i];
                (*timer)++;
                if (*timer > 119)
                    *timer = 0;
                mMatAnm[i]->setFrame((f32)*timer);
            }
        } else {
            mCursorPanes[i]->mIsVisible = 0;
            if (i < 6) {
                mCursorAnm[i].mAnm[0]->setFrame(0.0f);
                mCursorAnm[i].mAnm[1]->setFrame(0.0f);
                mMatAnm[i]->setFrame(0.0f);
            }
        }
    }
}

void SceneOption::calcMatAnmVolume() {}

void SceneOption::calcMatAnmSPCursor() {}

void SceneOption::calcMatAnmBack() {}

void SceneOption::calcMatAnm() {}

void SceneOption::calc() {
    switch (mState) {
    case 0:
        fadeIn();
        break;
    case 1:
        option();
        break;
    case 2:
        soundplayer();
        break;
    case 4:
        nextScene();
        break;
    case 3:
        if (CardAgent::msState != 0)
            break;
        if (CardAgent::mspPrintMemoryCard->_d == 0)
            GameAudio::Main::getAudio()->startSystemSe(0x20002);
        PrintMemoryCard *print = CardAgent::mspPrintMemoryCard;
        print->_c = 0;
        print->_d = 0;
        print->_e = 0;
        mState = 4;
        if (System::getDisplay()->getFader())
            System::getDisplay()->getFader()->startFadeOut(15);
        GameAudio::Main::getAudio()->fadeOutAll(15);
        break;
    default:
#line 1259
        JUT_ASSERT_F(0, "%s", "0");
        break;
    }
    calcMatAnmCursor();
    mAnm10c[0]->setFrame((f32)(mBgmVolume * 99 / 20));
    mAnm10c[1]->setFrame((f32)(mSeVolume * 99 / 20));
    mScreen->animation();
    mBackAnmFrame++;
    if (mBackAnmFrame < 0)
        mBackAnmFrame = 0;
    if (mBackAnmFrame >= 540)
        mBackAnmFrame = 0;
    mBackAnm->setFrame((f32)mBackAnmFrame);
    mBackScreen->animation();
    calcMatAnmTitleLine();
    calcAnm();
    if (mRumbleTimer[0] > 0) {
        mRumbleTimer[0]--;
        if (mRumbleTimer[0] == 0)
            rumbleOff(0);
    }
}

void SceneOption::fadeIn() {
    if (++mFadeTimer > 15) {
        mState = 1;
        mPrevState = 0;
        mFadeTimer = 0;
    }
}

bool SceneOption::isMenuExist(int menu) {
    if (menu < 4)
        return true;
    if (menu == 6)
        return true;
    if (mAnmEnable[menu - 4])
        return true;
    return false;
}

void SceneOption::option() {
    int prevSoundMode = mSoundMode;
    mFadeTimer = 0;
    switch (mMenuCursor) {
    case 1:
        int bgmVolume = mBgmVolume;
        if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_LEFT)) {
            if (mBgmVolume > 0)
                mBgmVolume--;
        } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT)) {
            if (mBgmVolume < 20)
                mBgmVolume++;
        }
        if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT)) {
            {
                int &timer = mSubCursorTimer[mMenuCursor][1];
                timer = 31;
            }
            {
                int &timer = mSubCursorTimer[mMenuCursor][2];
                timer = 31;
            }
            {
                int &timer = mSubCursorTimer2[mMenuCursor][1];
                timer = 2;
            }
        }
        if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_LEFT)) {
            {
                int &timer = mSubCursorTimer[mMenuCursor][0];
                timer = 31;
            }
            {
                int &timer = mSubCursorTimer[mMenuCursor][2];
                timer = 31;
            }
            {
                int &timer = mSubCursorTimer2[mMenuCursor][0];
                timer = 2;
            }
        }
        if (mBgmVolume != bgmVolume) {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            GameAudio::Main::getAudio()->setMasterVolume(mBgmVolume - 10);
        }
        break;
    }

    if (gpaGamePad[0]->testTrigger(JUTGamePad::A)) {
        switch (mMenuCursor) {
        case 6:
            if (!SceneChange_ExitOption(0))
                GameAudio::Main::getAudio()->startSystemSe(0x20002);
            break;
        }
    } else if (gpaGamePad[0]->testTrigger(JUTGamePad::B)) {
        GameAudio::Main::getAudio()->startSystemSe(0x20004);
        SceneChange_ExitOption(1);
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_UP)) {
        while (true) {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            if (--mMenuCursor < 0)
                mMenuCursor = 6;
            if (isMenuExist(mMenuCursor))
                break;
        }
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
        while (true) {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            if (++mMenuCursor >= 7)
                mMenuCursor = 0;
            if (isMenuExist(mMenuCursor))
                break;
        }
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_LEFT)) {
        if (mMenuCursor != 1 && mMenuCursor < 6) {
            if (mMenuCursor != 0)
                GameAudio::Main::getAudio()->startSystemSe(0x2000c);
            {
                int &timer = mSubCursorTimer[mMenuCursor][0];
                timer = 31;
            }
            {
                int &timer = mSubCursorTimer[mMenuCursor][2];
                timer = 31;
            }
            {
                int &timer = mSubCursorTimer2[mMenuCursor][0];
                timer = 2;
            }
            switch (mMenuCursor) {
            case 2:
                if (--mRumble < 0)
                    mRumble = 1;
                rumble(0);
                break;
            case 0:
                if (--mSoundMode < 0)
                    mSoundMode = 2;
                break;
            case 3:
                if (--mOnOff2 < 0)
                    mOnOff2 = 1;
                break;
            case 4:
                if (--mItemSlot < 0)
                    mItemSlot = 3;
                break;
            case 5:
                if (--mVsLap < 0)
                    mVsLap = 9;
                break;
            case 1:
            case 6:
                break;
            default:
#line 1519
                JUT_ASSERT_F(0, "%s", "0");
                break;
            }
        }
    } else if (gpaGamePad[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT)) {
        if (mMenuCursor != 1 && mMenuCursor < 6) {
            if (mMenuCursor != 0)
                GameAudio::Main::getAudio()->startSystemSe(0x2000c);
            {
                int &timer = mSubCursorTimer[mMenuCursor][1];
                timer = 31;
            }
            {
                int &timer = mSubCursorTimer[mMenuCursor][2];
                timer = 31;
            }
            {
                int &timer = mSubCursorTimer2[mMenuCursor][1];
                timer = 2;
            }
            switch (mMenuCursor) {
            case 2:
                if (++mRumble >= 2)
                    mRumble = 0;
                rumble(0);
                break;
            case 0:
                if (++mSoundMode >= 3)
                    mSoundMode = 0;
                break;
            case 3:
                if (++mOnOff2 >= 2)
                    mOnOff2 = 0;
                break;
            case 4:
                if (++mItemSlot >= 4)
                    mItemSlot = 0;
                break;
            case 5:
                if (++mVsLap >= 10)
                    mVsLap = 0;
                break;
            case 1:
            case 6:
                break;
            default:
#line 1574
                JUT_ASSERT_F(0, "%s", "0");
                break;
            }
        }
    }

    if (prevSoundMode != mSoundMode) {
        GameAudio::Main::getAudio()->setOutputMode(getJASoundMode(mSoundMode));
        GameAudio::Main::getAudio()->startSystemSe(0x20045);
    }
    switch (mState) {
    case 1:
        break;
    case 3:
        save();
        break;
    default:
        nosave();
        break;
    }
}

void SceneOption::soundplayer() {
    KartGamePad **pads = gpaGamePad;
    if (pads[0]->testButton(JUTGamePad::MAINSTICK_RIGHT)) {
        mSPHoldRight++;
        if (mSPHoldRight > 168)
            mSPHoldRight = 168;
        if (mSPHoldRight > 84 && mSPCursor < 3) {
            {
                int add = mSPHoldRight >= 168 ? 2 : 1;
                int &value = mSPValue[mSPCursor];
                value += add;
            }
            {
                int &value = mSPValue[mSPCursor];
                int &max = mSPMax[mSPCursor];
                if (value >= max)
                    value = max;
            }
        }
    } else {
        mSPHoldRight = 0;
    }
    if (pads[0]->testButton(JUTGamePad::MAINSTICK_LEFT)) {
        mSPHoldLeft++;
        if (mSPHoldLeft > 168)
            mSPHoldLeft = 168;
        if (mSPHoldLeft > 84 && mSPCursor < 3) {
            {
                int sub = mSPHoldLeft >= 168 ? 2 : 1;
                int &value = mSPValue[mSPCursor];
                value -= sub;
            }
            int &value = mSPValue[mSPCursor];
            if (value < 0)
                value = 0;
        }
    } else {
        mSPHoldLeft = 0;
    }

    if (pads[0]->testTrigger(JUTGamePad::A | JUTGamePad::START)) {
        if (mSPCursor == 3) {
            GameAudio::Main::getAudio()->startSystemSe(0x20004);
            mState = 1;
            mPrevState = 2;
        }
    } else if (pads[0]->testTrigger(JUTGamePad::B)) {
        GameAudio::Main::getAudio()->startSystemSe(0x20004);
        mState = 1;
        mPrevState = 2;
    } else if (pads[0]->testRepeat(JUTGamePad::MAINSTICK_UP)) {
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
        if (--mSPCursor < 0)
            mSPCursor = 3;
    } else if (pads[0]->testRepeat(JUTGamePad::MAINSTICK_DOWN)) {
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
        if (++mSPCursor >= 4)
            mSPCursor = 0;
    } else if (pads[0]->testRepeat(JUTGamePad::MAINSTICK_LEFT)) {
        if (mSPCursor < 3) {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            {
                int &timer = mSPSubCursorTimer[mSPCursor][0];
                timer = 31;
            }
            {
                int &timer = mSPSubCursorTimer[mSPCursor][2];
                timer = 31;
            }
            {
                int &timer = mSPSubCursorTimer2[mSPCursor][0];
                timer = 2;
            }
            if (mSPHoldLeft < 84) {
                int &value = mSPValue[mSPCursor];
                if (--value < 0)
                    mSPValue[mSPCursor] = mSPMax[mSPCursor];
            }
        }
    } else if (pads[0]->testRepeat(JUTGamePad::MAINSTICK_RIGHT)) {
        if (mSPCursor < 3) {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            {
                int &timer = mSPSubCursorTimer[mSPCursor][1];
                timer = 31;
            }
            {
                int &timer = mSPSubCursorTimer[mSPCursor][2];
                timer = 31;
            }
            {
                int &timer = mSPSubCursorTimer2[mSPCursor][1];
                timer = 2;
            }
            if (mSPHoldRight < 84) {
                if (++mSPValue[mSPCursor] >= mSPMax[mSPCursor]) {
                    int &value = mSPValue[mSPCursor];
                    value = 0;
                }
            }
        }
    }
}

void SceneOption::nextScene() {
    if (mSceneType == Scene::SCENE_MAX) {
        AppMgr::deleteCurrentApp();
        MainMenuApp::call();
        return;
    }
    if (++mFadeTimer > 15) {
        mSceneType = 0;
        if (SequenceApp::ptr()->ready((Scene::SceneType)mSceneType)) {
            GameAudio::Main::getAudio()->stopSequenceBgm(1);
            GameAudio::Main::getAudio()->stopSystemSe(0x20045);
            SequenceApp::ptr()->setNextScene((Scene::SceneType)mSceneType);
        }
        if (mRumbleTimer[0] != 0)
            rumbleOff(0);
    }
}

int SceneOption::getJASoundMode(int i) {
    int ret = 1;
    switch (i) {
    case 0:
        ret = 0;
        break;
    case 1:
        ret = 1;
        break;
    case 2:
        ret = 2;
        break;
    }
    return ret;
}

void SceneOption::IsChangeOptions() {}

bool SceneOption::SceneChange_ExitOption(int arg) {
    bool changed;
    if (mPrevSoundMode != mSoundMode)
        changed = 1;
    else if (mPrevRumble != mRumble)
        changed = 1;
    else if (mPrevOnOff2 != mOnOff2)
        changed = 1;
    else if (mPrevItemSlot != mItemSlot)
        changed = 1;
    else if (mPrevVsLap != mVsLap)
        changed = 1;
    else if (mPrevBgmVolume != mBgmVolume)
        changed = 1;
    else if (mPrevSeVolume != mSeVolume)
        changed = 1;
    else
        changed = 0;
    if (changed && arg == 0) {
        mState = 3;
        mPrevState = 1;
        return true;
    }
    mState = 4;
    if (System::getDisplay()->getFader())
        System::getDisplay()->getFader()->startFadeOut(15);
    GameAudio::Main::getAudio()->fadeOutAll(15);
    return false;
}

void SceneOption::rumble(int pad) {
    if (mRumble == 1) {
        JUTGamePad::CRumble::startMotor(gpaGamePad[pad]->mPortNum);
        mRumbleTimer[pad] = 10;
    }
}

void SceneOption::rumbleOff(int kart) {
    JUTGamePad::CRumble::stopMotor(gpaGamePad[kart]->mPortNum, true);
}

void SceneOption::save() {
    switch (mRumble) {
    case 1:
        gSystemRecord.mOptions &= ~4;
        break;
    case 0:
        gSystemRecord.mOptions |= 4;
        break;
    }
    osSetSound(mSoundMode);
    switch (mSoundMode) {
    case 0:
        gSystemRecord.mOptions &= ~3;
        gSystemRecord.mOptions |= 1;
        break;
    case 1:
        gSystemRecord.mOptions &= ~3;
        break;
    case 2:
        gSystemRecord.mOptions &= ~3;
        gSystemRecord.mOptions |= 2;
        break;
    }
    switch (mOnOff2) {
    case 1:
        gSystemRecord.mOptions &= ~8;
        break;
    case 0:
        gSystemRecord.mOptions |= 8;
        break;
    }
    switch (mItemSlot) {
    case 0:
        gSystemRecord.mItemSlotType = 0;
        break;
    case 3:
        gSystemRecord.mItemSlotType = 1;
        break;
    case 1:
        gSystemRecord.mItemSlotType = 2;
        break;
    case 2:
        gSystemRecord.mItemSlotType = 3;
        break;
    }
    gSystemRecord.mVsLapNum = mVsLap;
    gSystemRecord.mVolume = mBgmVolume - 10;
    CardAgent::ask(CardAgent::mcCommand3, 0);
}

void SceneOption::nosave() {
    GameAudio::Main::getAudio()->setOutputMode(getJASoundMode(mPrevSoundMode));
    GameAudio::Main::getAudio()->setMasterVolume(mPrevBgmVolume - 10);
}

void SceneOption::osSetSound(int mode) {
    switch (mode) {
    case 0:
        OSSetSoundMode(0);
        break;
    default:
        OSSetSoundMode(1);
        break;
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
