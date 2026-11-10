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

SceneOption::SceneOption(JKRArchive *archive, JKRHeap *heap)
    : Scene(archive, heap), mTitleLine(archive, heap, (char *)"option_title_line") {
    SceneOption_ConstructerOption(heap);
    SceneOption_ConstructerBack(heap);
}

void SceneOption::SceneOption_ConstructerOption(JKRHeap *) {}

void SceneOption::SceneOption_ConstructerSoundPlayer(JKRHeap *) {}

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
    mTitleLine.init_titleline(8);
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

 SceneOption::~SceneOption() {}

void SceneOption::draw() {
    mOrtho->setPort();
    mBackScreen->draw(0.0f, 0.0f, mOrtho);
    mScreen->draw(0.0f, 0.0f, mOrtho);
    mTitleLine.drawTitleLine(mOrtho);
}

void SceneOption::calcAnmAll() {}

void SceneOption::calcAnmMenu() {}

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

void SceneOption::calcAnmVolume() {}

void SceneOption::calcTexChange() {}

void SceneOption::calcAnmSPAll() {}

void SceneOption::calcAnmSPMenu() {}

void SceneOption::calcAnmSPCursor() {}

void SceneOption::calcAnmSPNumeral() {}

void SceneOption::calcAnm() {
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
    mSoundPlayerScreen->animationTransform();
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

    calcAnmCursor();
    mAnmTransform->setFrame((f32)(mBgmVolume * 99) / 20.0f + 100.0f);
    mKnobPanes[0]->animationTransform();
    mAnmTransform->setFrame((f32)(mSeVolume * 99) / 20.0f + 100.0f);
    mKnobPanes[1]->animationTransform();

    mPane274[0]->changeTexture(mBtiName_sound[mSoundMode], 0);
    mPane274[1]->changeTexture(mBtiName_onOff[mRumble], 0);
    mPane274[2]->changeTexture(mBtiName_onOff[mOnOff2], 0);
    mPane274[3]->changeTexture(mBtiName_itemSlot[mItemSlot], 0);

    if (mVsLap == 0) {
        mPane274[4]->mIsVisible = 0;
        mPane274[5]->mIsVisible = 1;
    } else {
        mPane274[4]->mIsVisible = 1;
        mPane274[5]->mIsVisible = 0;
        mPane274[4]->changeTexture(Kart2DCommon::ptr()->getPartsLocName(mVsLap), 0);
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
    mPane1a8[1]->changeTexture(Kart2DCommon::ptr()->getPartsLocName(num), 0);

    int dir;
    if (mState < 5) {
        if (mState >= 3)
            dir = -1;
        else
            dir = 1;
    } else {
        dir = 1;
    }
    mTitleLine.calcAnmTitleLine(dir);
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
        JUT_ASSERT_F(0, "0", 0);
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
    mTitleLine.calcMatAnmTitleLine();
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

void SceneOption::isMenuExist(int) {}

void SceneOption::option() {}

void SceneOption::soundplayer() {}

void SceneOption::nextScene() {
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
    if (mRumbleTimer[0] != 0)
        rumbleOff(0);
}

int SceneOption::getJASoundMode(int i) {
    int ret = 1;
    switch (i) {
    case 1:
        return 1;
    case 0:
        ret = 0;
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
    case 1:
        gSystemRecord.mItemSlotType = 1;
        break;
    case 2:
        gSystemRecord.mItemSlotType = 2;
        break;
    case 3:
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
