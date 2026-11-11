#include "Kameda/SceneMenu.h"
#include "Kameda/Scene.h"

#include "kartEnums.h"
#include "mathHelper.h"

#include "JSystem/J2D/J2DPicture.h"
#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/JUtility/JUTDbg.h"
#include "Kameda/SequenceInfo.h"
#include "Kameda/CharacterSelect3D.h"
#include "JSystem/JUtility/JUTGamePad.h"
#include "Kameda/ReadPrintMessage.h"
#include "Kaneshige/RaceInfo.h"
#include "Kaneshige/KartInfo.h"
#include "Kaneshige/KartChecker.h"
#include "Inagaki/GameAudioMain.h"
#include "Sato/JPEffectMgr.h"
#include "Osako/Kart2DCommon.h"
#include "Osako/MenuBackground.h"
#include "Osako/MenuTitleLine.h"
#include "Osako/NameEntryHelper.h"
#include "Osako/SequenceApp.h"
#include "Osako/SystemRecord.h"
#include "Osako/system.h"
#include "Osako/kartPad.h"
#include "dolphin/gx/GXTransform.h"

const u32 SceneMenu::mCharVoice[] = {
    0x00020014, 0x00020015, 0x00020012, 0x00020013, 
    0x00020018, 0x00020019, 0x0002000E, 0x0002000F, 
    0x0002001E, 0x0002001F, 0x00020011, 0x00020010, 
    0x0002001A, 0x0002001B, 0x0002001C, 0x0002001D, 
    0x00020016, 0x00020017, 0x00020020, 0x00020021, 
};

const u32 SceneMenu::mCharGoVoice[] = {
    0x0002004E, 0x0002004F, 0x0002004C, 0x0002004D, 
    0x00020052, 0x00020053, 0x00020048, 0x00020049, 
    0x00020058, 0x00020059, 0x0002004B, 0x0002004A, 
    0x00020054, 0x00020055, 0x00020056, 0x00020057, 
    0x00020050, 0x00020051, 0x0002005A, 0x0002005B, 
};

const int SceneMenu::mCharIndex[] = {
    0x00000007, 0x00000008, 0x00000005, 0x00000006, 
    0x0000000B, 0x0000000C, 0x00000001, 0x00000002, 
    0x00000011, 0x00000012, 0x00000004, 0x00000003, 
    0x0000000D, 0x0000000E, 0x0000000F, 0x00000010, 
    0x00000009, 0x0000000A, 0x00000014, 0x00000013, 
};

const char *SceneMenu::mCharName[] = {
    "ChaName_MARIO.bti", "ChaName_LUIGI.bti",
    "ChaName_PEACH.bti", "ChaName_DAISY.bti",
    "ChaName_YOSHI.bti", "ChaName_BIRDO.bti",
    "ChaName_BABY_MARIO.bti", "ChaName_BABY_LUIGI.bti",
    "ChaName_KINOPIO.bti", "ChaName_KINOPICO.bti",
    "ChaName_KOOPA.bti", "ChaName_PARATOOPA.bti",
    "ChaName_DONKEY_KONG.bti", "ChaName_DIDDY_KONG.bti",
    "ChaName_BOWSER.bti", "ChaName_BOWSER_JR.bti",
    "ChaName_WARIO.bti", "ChaName_WALUIGI.bti",
    "ChaName_PAKKUN.bti", "ChaName_TERESA.bti"
};

const int SceneMenu::mKartIndex[] = {
    0x00000005, 0x0000000D, 0x00000003, 0x0000000B, 
    0x00000009, 0x0000000F, 0x00000010, 0x00000011, 
    0x00000000, 0x00000008, 0x00000004, 0x0000000C, 
    0x00000002, 0x0000000A, 0x0000000E, 0x00000006, 
    0x00000001, 0x00000007, 0x00000013, 0x00000012, 
    0x00000014, 
};

const u8 SceneMenu::mKartPerf[] = {
    0x01, 0x05, 0x02, 0x02, 0x04, 0x02, 0x02, 0x04, 
    0x02, 0x01, 0x05, 0x02, 0x04, 0x02, 0x03, 0x04, 
    0x03, 0x01, 0x02, 0x04, 0x02, 0x01, 0x05, 0x02, 
    0x03, 0x03, 0x03, 0x04, 0x02, 0x02, 0x02, 0x04, 
    0x03, 0x03, 0x03, 0x02, 0x02, 0x04, 0x03, 0x03, 
    0x03, 0x04, 0x03, 0x03, 0x03, 0x04, 0x02, 0x04, 
    0x04, 0x02, 0x04, 0x05, 0x01, 0x05, 0x04, 0x02, 
    0x05, 0x02, 0x04, 0x05, 0x04, 0x03, 0x04,
};

const ERaceLevel SceneMenu::mLevelID[] = {
    LVL_50CC, LVL_100CC, LVL_150CC, LVL_MIRROR
};

const u64 SceneMenu::mPlayerIconBckTag[] = {
    'N_Stok', 'Cstok_p', 'Cstok_pb', 'Ns1234', 
    'Ns12_3_4', 'Ns12_34', 'SMpb1', 'SMpb2', 
    'SMpb3', 'SMpb4', 'SMpb5', 'SMpb6', 
    'SMpb7', 'SMpb8', 'SMpb9', 'SMpm1', 
    'SMpm2', 'SMpm3', 'SMpm4', 'SMpm5', 
    'SMpm6', 'SMpm7', 'SMpm8', 'SMpm9', 
    'SMpm10', 'SMpm11', 'SMpm12', 'SMm1', 
    'SMm2', 'SMm3', 'SMm4', 'SMm5', 
    'SMm6', 
};

const char *SceneMenu::mPlayerIconPlayerName[] = {
    "Player1.bti", 
    "Player2.bti",
    "Player3.bti",
    "Player4.bti"
};

const u64 SceneMenu::mHowManyDriversBckTag[] = {
    'N_Entry', 'Ecrsr1', 'Ecrsr2', 'Ecrsr3', 
    'Ecrsr4', 'ENplay1', 'ENplay2', 'ENplay3', 
    'ENplay4', 'Eplay1b', 'Eplay2b', 'Eplay3b', 
    'Eplay4b', 
};

const u64 SceneMenu::mHowManyDriversBrkTag[] = {
    'Ebar1', 'Ebar2', 'Ebar3', 'Ebar4', 
    'Eplay1', 'Eplay2', 'Eplay3', 'Eplay4', 
    'Eplay1b', 'Eplay2b', 'Eplay3b', 'Eplay4b', 
    'Emario11', 'Emario21', 'Emario22', 'Emario31', 
    'Emario32', 'Emario33', 'Emario41', 'Emario42', 
    'Emario43', 'Emario44',
};

const u64 SceneMenu::mSingleTagSelectBckTag[] = {
    'N_CoVs', 'N_C2p', 'C2crsr1', 'C2crsr2', 
    'C2chaw1N', 'C2chaw2N', 'C2chaw3N', 'CNP2M1', 
    'CNP2M2', 'CNm22p', 'C2pb1', 'C2pb2', 
    'N_C3p', 'C3crsr1', 'C3crsr2', 'CPcN31', 
    'CPcN32', 'CNP3M1', 'CNP3M2', 'C3pb1', 
    'C3pb2', 'C3pb3', 'C3pb4', 'C3pb5', 
    'N_C4p', 'C4crsr1', 'C4crsr2', 'C4crsr3', 
    'CPcN41', 'CPcN42', 'CPcN43', 'CNP4M1', 
    'CNP4M2', 'CNP4M3', 'C4pb1', 'C4pb2', 
    'C4pb3', 'C4pb4', 'C4pb5', 'C4pb6', 
    'C4pb7', 'C4pb8', 'C4pb9',
};

const u64 SceneMenu::mSingleTagSelectBtkTag[] = {
    'C2bar1', 'C2bar2', 'C2cha_w1', 'C2cha_w2', 
    'C2cha_w3', 'C3bar1', 'C3bar2', 'C3cha_w1', 
    'C3cha_w2', 'C3cha_w3', 'C3cha_w4', 'C3cha_w5', 
    'C4bar1', 'C4bar2', 'C4bar3', 'C4cha_w1', 
    'C4cha_w2', 'C4cha_w3', 'C4cha_w4', 'C4cha_w5', 
    'C4cha_w6', 'C4cha_w7', 'C4cha_w8', 'C4cha_w9', 
};

const u64 SceneMenu::mSingleTagSelectBrkTag[] = {
    'C2bar1', 'C2bar2', 'C2cha1', 'C2cha2', 
    'C2cha3', 'C2cha4', 'CNm22p', 'C2pm1', 
    'C2pm2', 'C2pm3', 'C2pm4', 'C2pb1', 
    'C2pb2', 'C2pvsm1', 'C3bar1', 'C3bar2', 
    'C3cha1', 'C3cha2', 'C3cha3', 'C3cha4', 
    'C3cha5', 'C3cha6', 'C3pm1', 'C3pm2', 
    'C3pm3', 'C3pm4', 'C3pm5', 'C3pm6', 
    'C3pb1', 'C3pb2', 'C3pb3', 'C3pb4', 
    'C3pb5', 'C3vsm1', 'C3vsm2', 'C3vsm4', 
    'C4bar1', 'C4bar2', 'C4bar3', 'C4cha1', 
    'C4cha2', 'C4cha3', 'C4cha4', 'C4cha5', 
    'C4cha6', 'C4cha7', 'C4cha8', 'C4cha9', 
    'C4cha10', 'C4cha11', 'C4cha12', 'C4pm1', 
    'C4pm2', 'C4pm3', 'C4pm4', 'C4pm5', 
    'C4pm6', 'C4pm7', 'C4pm8', 'C4pm9', 
    'C4pm10', 'C4pm11', 'C4pm12', 'C4pb1', 
    'C4pb2', 'C4pb3', 'C4pb4', 'C4pb5', 
    'C4pb6', 'C4pb7', 'C4pb8', 'C4pb9', 
    'C4vsm1', 'C4vsm2', 'C4vsm3', 'C4vsm4', 
    'C4vsm5', 'C4vsm6',
};

const u64 SceneMenu::mSingleTagSelectBpkTag[] = {
    'C2crsr1', 'C2crsr2', 'C3crsr1', 'C3crsr2', 
    'C4crsr1', 'C4crsr2', 'C4crsr3',
};

const u64 SceneMenu::mModeBckTag[] = {
    'N_Mode2', 'M4crsr1', 'M4crsr2', 'M4crsr3', 
    'MNgpP21', 'MNvsP21', 'MvsPN22', 'Mvsb2', 
    'MNminP21', 'MNminP22', 'MMinP21', 'MMinP22', 
    'MMinP23', 'Mgpm1N', 'MgpM21', 'MvsN', 
    'MvsM21', 'MminiN', 'MminM21', 'N_Pw_gp2', 
    'Mpow1', 'NPowL1', 'NPowR1', 'Mp2s1', 
    'Mp2s2', 'Mpow1m', 'N_Pw_vs', 'Mpow2', 
    'Mpow2m', 'Mp2s3', 'Mp2s4', 'NPowL2', 
    'NPowR2', 'N_Mode1', 'M4crsr4', 'M4crsr5', 
    'N_Mode11', 'MNgpP11', 'MNtaP11', 'MNtaPic', 
    'MNtaNe', 'MtaEye1', 'MtaEye2', 'MtaLi', 
    'MgpmN2', 'MgpM11', 'MtamN', 'MtaM11', 
    'N_Mode12', 'MNVspc1', 'MvsPN12', 'Mvsb1', 
    'MMinPN11', 'MMinPN12', 'MMinP11', 'MMinP12', 
    'MMinP13', 'MvsN1', 'MvsM12', 'MminiN1', 
    'MminiM12', 'N_Pw_gp1', 'NPowL3', 'NPowR3', 
    'Mp2s7', 'Mp2s8', 'Mp2s17', 'Mp2s18', 
    'Mpow4', 'Mpow1m1', 'MtaName', 'NNM1', 
    'NNM2', 'NNM3', 'OK_wb11',
};

const u64 SceneMenu::mModeBtkTag[] = {
    'M4bar1', 'M4bar2', 'M4bar3', 'Mgpbb', 
    'Mgpb1', 'C4cha_10', 'C4cha_11', 'C4cha_12', 
    'C4cha_13', 'MMinb1', 'Mp2s11', 'Mp2s12', 
    'Mp2s13', 'Mp2s14', 'Mgpbb11', 'Mgpb2', 
    'M4bar4', 'M4bar5', 'MMinb2', 'C4cha_01', 
    'C4cha_02', 'C4cha_03', 'C4cha_04', 'Mp2s17', 
    'Mp2s18', 'OK_w11', 
};

const u64 SceneMenu::mModeBrkTag[] = {
    'M4bar1', 'M4bar2', 'M4bar3', 'MMinb1', 
    'MgpM21', 'Mgpm1', 'MvsM21', 'Mvs', 
    'MminM21', 'Mmini', 'Mp2s1', 'Mp2s2', 
    'Mp2s3', 'Mp2s4', 'Mp2s11', 'Mp2s12', 
    'MgpM11', 'Mgpm2', 'MtaM11', 'Mtam', 
    'M4bar4', 'M4bar5', 'MMinb2', 'MvsM12', 
    'Mvs1', 'MminiM12', 'Mmini1', 'Mp2s7', 
    'Mp2s8', 'OK_wb11',
};

const u64 SceneMenu::mModeBpkTag[] = {
    'M4crsr1', 'M4crsr2', 'M4crsr3', 'M4crsr4', 
    'M4crsr5', 'MtaLi', 
};

const char *SceneMenu::mPowerMojiName[] = {
    "Power_Mozi.1.bti",
    "Power_Mozi.2.bti",
    "Power_Mozi.3.bti",
    "Power_Mozi.4.bti"
};

const char *SceneMenu::mPowerPicName[] = {
    "Entry_Engine1.bti",
    "Entry_Engine2.bti",
    "Entry_Engine3.bti",
    "Entry_Engine4.bti",
};

const char *SceneMenu::mBattleName[] = {
    "BattleSnap1.bti",
    "BattleSnap2.bti",
    "BattleSnap3.bti"
};

const u64 SceneMenu::mBattleBckTag[] = {
    'NSlBtl', 'Curs1', 'Curs2', 'Curs3', 
    'Bar1', 'Bar2', 'Bar3', 'Word1', 
    'Word2', 'Word3', 'BtlP1', 'BtlP2', 
    'BtlP3', 'NPict', 'BtlPict',
};

const u64 SceneMenu::mBattleBrkTag[] = {
    'Bar1', 'Bar2', 'Bar3'
};

const u64 SceneMenu::mCharCCBckTag[] = {
    'Cha1', 'Cha2', 
};

const u64 SceneMenu::mCharCCBrkTag[] = {
    'CW1', 'CW2', 'CWC1', 'CWC2', 
};

const u64 SceneMenu::mCharCCBpkTag[] = {
    'CW1', 'CW2', 'CP1', 'CP2', 
    'CC1', 'CC2', 
};

const char *SceneMenu::mCharCCName[] = {
    "SC_P1_u.bti",
    "SC_P2_u.bti",
    "SC_P3_u.bti",
    "SC_P4_u.bti",
    "SC_P1_s.bti",
    "SC_P2_s.bti",
    "SC_P3_s.bti",
    "SC_P4_s.bti",
};

const u64 SceneMenu::mChar1BckTag[] = {
    'N_HC1', 'C11P1', 'C11P2', 'HC11w', 
    'N_P1', 'ArL11', 'ArR11', 'Wait11', 
    'NRandom',
};

const u64 SceneMenu::mChar1PlusBckTag[] = {
    'N11F', 'N11H', 'N11B', 'N_MP11', 
    'MP11S1', 'MP11S2', 'MP11S3', 'MP11S4', 
    'MP11S5', 'MP11A1', 'MP11A2', 'MP11A3', 
    'MP11A4', 'MP11A5', 'MP11W1', 'MP11W2', 
    'MP11W3', 'MP11W4', 'MP11W5',
};

const u64 SceneMenu::mChar1PlusBrkTag[] = {
    'N11F', 'N11B', 
};

const u64 SceneMenu::mChar2BckTag[] = {
    'N_HC2', 'C21P1', 'C21P2', 'C22P1', 
    'C22P2', 'HC21w', 'HC22w', 'N_P21', 
    'N_P22', 'OK_wb21', 'OK_wb22', 'OK_w21', 
    'OK_w22', 'ArL21', 'ArL22', 'ArR21', 
    'ArR22', 'Wait21', 'Wait22', 'NRandom', 
};

const u64 SceneMenu::mChar2BrkTag[] = {
   'C22P1', 'C22P2', 'HC22w', 'OK_w22', 
    'Ksb22', 
};

const u64 SceneMenu::mChar3BckTag[] = {
    'N_HC3', 'C31P1', 'C31P2', 'C32P1', 
    'C33P1', 'HC31w', 'HC32w', 'HC33w', 
    'N_P31', 'N_P32', 'N_P33', 'OK_wb31', 
    'OK_wb32', 'OK_wb33', 'OK_w31', 'OK_w32', 
    'OK_w33', 'ArL31', 'ArL32', 'ArL33', 
    'ArR31', 'ArR32', 'ArR33', 'Wait31', 
    'Wait32', 'Wait33', 'NRandom', 
};

const u64 SceneMenu::mChar3BrkTag[] = {
    'C32P1', 'C33P1', 'HC32w', 'HC33w', 
    'Ksb32', 'Ksb33', 'OK_w32', 'OK_w33', 
};

const u64 SceneMenu::mChar4BckTag[] = {
    'N_HC4', 'C41P1', 'C42P1', 'C43P1', 
    'C44P1', 'HC41w', 'HC42w', 'HC43w', 
    'HC44w', 'N_P41', 'N_P42', 'N_P43', 
    'N_P44', 'ArL41', 'ArL42', 'ArL43', 
    'ArL44', 'ArR41', 'ArR42', 'ArR43', 
    'ArR44', 'Wait41', 'Wait42', 'Wait43', 
    'Wait44', 'NRandom', 
};

SceneMenu::SceneMenu(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap) {
    void (J2DGrafContext::*scissorFn)(f32, f32, f32, f32) = &J2DGrafContext::scissor;
    (mOrtho->*scissorFn)(0.0f, 0.0f, 0.0f, 0.0f);

    void (J2DTextBox::*fontColorFn)(JUtility::TColor, JUtility::TColor) = &J2DTextBox::setFontColor;
    (mTextBox->*fontColorFn)(JUtility::TColor(0xFFFFFFFF), JUtility::TColor(0xFFFFFFFF));

    void (J2DTextBox::*fontSizeFn)(J2DTextBox::TFontSize &) const = &J2DTextBox::getFontSize;
    (mTextBox->*fontSizeFn)(mFontSize);

    ResTIMG *(Kart2DCommon::*charIconFn)(ECharID) = &Kart2DCommon::getCharIconTexture;
    (Kart2DCommon::ptr()->*charIconFn)((ECharID)0);

    MenuTitleLine *(*createTitleFn)(JKRArchive *, JKRHeap *) = &MenuTitleLine::create;
    createTitleFn(archive, heap);
    MenuBackground *(*createBgFn)(JKRArchive *) = &MenuBackground::create;
    createBgFn(archive);

    void (*createJpaFn)(JKRHeap *, bool) = &CreateJPAMgr;
    createJpaFn(heap, true);

    CharacterSelect3D *(*createCharFn)(JKRHeap *) = &CharacterSelect3D::create;
    createCharFn(heap);
}

void SceneMenu::reset()
{
    for (int i = 0; i < 2; i++)
    {
        J2DPane *pane = mScreenCharLayout->search(0x0053657443686131ull + i);
        JGeometry::TBox2f *bounds = pane->getBounds();
        f32 bx = bounds->i.x;
        f32 by = bounds->i.y;
        mCharBasePos[i].x = bx;
        mCharBasePos[i].y = by;
    }

    int n = 10;
    if (!(gSystemRecord.mGameFlag & 1))
    {
        mCharScreens[4]->mIsVisible = false;
        n = 9;
    }
    else
    {
        mCharScreens[4]->mIsVisible = true;
    }
    if (!(gSystemRecord.mGameFlag & 2))
    {
        mCharScreens[9]->mIsVisible = false;
        n--;
    }
    else
    {
        mCharScreens[9]->mIsVisible = true;
    }

    int k = -2;
    f32 scale = 2.0f * n;
    f32 dx = (mCharBasePos[1].x - mCharBasePos[0].x) / scale;
    f32 dy = (mCharBasePos[1].y - mCharBasePos[0].y) / scale;

    for (int i = 0; i < 10; i++)
    {
        if (i != 4 || (gSystemRecord.mGameFlag & 1))
            k += 2;
        int idx = k + 1;
        mCharPos[i].x = mCharBasePos[0].x + dx * idx;
        mCharPos[i].y = mCharBasePos[0].y + dy * idx;
    }

    for (int i = 0; i < 20; i++)
    {
        J2DPicture *pane = (J2DPicture *)mCharCCBpk[i / 2][i % 2 + 2].mPane;
#line 1440
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        pane->changeTexture(Kart2DCommon::ptr()->getCharIconTextureChecked((ECharID)mCharIndex[i]), 0);
    }

    for (int i = 0; i < 20; i++)
    {
        mChara[i].mCharId = mCharIndex[i];
        mChara[i].mWeight = KartInfo::getDriverWeight((ECharID)mChara[i].mCharId);
        mChara[i].mKartId = KartInfo::getDefaultKartID((ECharID)mChara[i].mCharId);
        if (!SystemRecord::tstSecretKart((EKartID)mChara[i].mKartId, gSystemRecord.mSecretKart))
        {
            int charId = mChara[i].mCharId;
            switch (charId)
            {
            case 14:
                mChara[i].mKartId = 5;
                break;
            case 16:
                mChara[i].mKartId = 3;
                break;
            case 10:
                mChara[i].mKartId = 0;
                break;
            default:
                mChara[i].mKartId = KartInfo::getPartnerKartID((ECharID)charId);
                break;
            }
        }
    }

    for (int i = 0; i < 21; i++)
    {
        mKartList[i].mKartId = mKartIndex[i];
        mKartList[i].mKartWeight = KartInfo::getKartWeight((EKartID)mKartList[i].mKartId);
    }

    for (int i = 0; i < 33; i++)
        mIconBck[i].mFrame = 0.0f;
    for (int i = 0; i < 13; i++)
        mHowManyDriversBck[i].mFrame = 0.0f;
    mHowManyDriversEnd.mFrame = 0.0f;
    for (int i = 0; i < 22; i++)
        mHowManyDriversBrk[i].mFrame = 0.0f;
    mSingleTagSelectEndBck.mFrame = 0.0f;
    for (int i = 0; i < 43; i++)
        mSingleTagSelectBck[i].mFrame = 0.0f;
    for (int i = 0; i < 24; i++)
        mSingleTagSelectBtk[i].mFrame = 0.0f;
    for (int i = 0; i < 78; i++)
        mSingleTagSelectBrk[i].mFrame = 0.0f;
    for (int i = 0; i < 7; i++)
        mSingleTagSelectBpk[i].mFrame = 0.0f;
    for (int i = 0; i < 75; i++)
        mModeBck[i].mFrame = 0.0f;
    for (int i = 0; i < 26; i++)
        mModeBtk[i].mFrame = 0.0f;
    for (int i = 0; i < 30; i++)
        mModeBrk[i].mFrame = 0.0f;
    for (int i = 0; i < 6; i++)
        mModeBpk[i].mFrame = 0.0f;
    for (int i = 0; i < 15; i++)
        mBattleBck[i].mFrame = 0.0f;
    mBattleBrk.mFrame = 0.0f;

    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 2; j++)
            mCharPane[i][j].mFrame = 0.0f;
        mCharCCBck[i].mFrame = 0.0f;
        for (int j = 0; j < 4; j++)
            mCharCCBrk[i][j].mFrame = 0.0f;
        for (int j = 0; j < 6; j++)
            mCharCCBpk[i][j].mFrame = 0.0f;
    }
    for (int i = 0; i < 9; i++)
        mCharPaneBck[i].mFrame = 0.0f;
    mCharCCEndBck.mFrame = 0.0f;
    mCharCCEndBrk.mFrame = 0.0f;
    for (int i = 0; i < 19; i++)
        mChar1PlusBck[i].mFrame = 0.0f;
    for (int i = 0; i < 2; i++)
        mChar1PlusBrk[i].mFrame = 0.0f;
    mChar1PlusBpk.mFrame = 0.0f;
    for (int i = 0; i < 20; i++)
        mChar2Bck[i].mFrame = 0.0f;
    mChar2EndBck.mFrame = 0.0f;
    for (int i = 0; i < 5; i++)
        mChar2Brk[i].mFrame = 0.0f;
    mChar2EndBrk.mFrame = 0.0f;
    for (int i = 0; i < 27; i++)
        mChar3Bck[i].mFrame = 0.0f;
    mChar3EndBck.mFrame = 0.0f;
    for (int i = 0; i < 8; i++)
        mChar3Brk[i].mFrame = 0.0f;
    mChar3EndBrk.mFrame = 0.0f;
    for (int i = 0; i < 26; i++)
        mChar4Bck[i].mFrame = 0.0f;
    mChar4EndBck.mFrame = 0.0f;
    mChar4EndBrk.mFrame = 0.0f;

    mInfo.mode = 0;
    mInfo.oldMode = 0;
    mInfo.nextMode = 0;
    mInfo.timer = 0;
    mInfo.nextScene = Scene::SCENE_MAX;
    mInfo.playerNum = 0;
    mInfo._18 = 0;
    mInfo.entryNum = 0;
    mInfo._20 = 0;
    mInfo.engineType = 0;
    _2108[0] = 0;
    _2108[1] = 0;
    _2108[2] = 0;
    _210c = 0;
    _2110 = 0;
    _2114 = 0;
    mMsgId = 0;
    for (int i = 0; i < 4; i++)
        mPlayerPadNo[i] = -1;
    for (int i = 0; i < 4; i++)
    {
        mPlayerData[i].mCursor = -1;
        mPlayerData[i].mCharNoF = -1;
        mPlayerData[i].mCharNoB = -1;
        mPlayerData[i].mCharF = -1;
        mPlayerData[i].mCharB = -1;
        mPlayerData[i].mKart = -1;
        mPlayerData[i].mState = 0;
    }
    for (int i = 0; i < 20; i++)
        mCharState[i] = 0;

    for (int i = 0; i < 33; i++)
        mIconBck[i].mPane->hide();

    mSingleTagSelectBck[0].mPane->hide();
    mSingleTagSelectBck[1].mPane->hide();
    mSingleTagSelectBck[12].mPane->hide();
    mSingleTagSelectBck[24].mPane->hide();
    mModeBck[33].mPane->hide();
    mModeBck[61].mPane->hide();
    mModeBck[36].mPane->hide();
    mModeBck[48].mPane->hide();
    mModeBck[34].mPane->hide();
    mModeBck[35].mPane->hide();
    mModeBck[0].mPane->hide();
    mModeBck[19].mPane->hide();
    mModeBck[26].mPane->hide();
    mModeBck[1].mPane->hide();
    mModeBck[2].mPane->hide();
    mModeBck[3].mPane->hide();

    strncpy(mName, gSystemRecord.mDefaultName, 4);
    mNameEntry->init(mName);
    mRouletteTimer = 0;
}

void SceneMenu::init()
{
    if (System::mspDisplay->getFader())
        System::mspDisplay->getFader()->startFadeIn(15);

    mCharacterSelect3D->init();
    j3dSys.reinitGX();

    Scene::SceneType seq = SequenceApp::ptr()->getPrevScene();
    if (gSequenceInfo._28)
        seq = (Scene::SceneType)12;
    gSequenceInfo._28 = 0;

    if (seq == Scene::SCENE_NONE)
    {
        reset();
        return;
    }

    if (GameAudio::Main::getAudio()->getPlayingSequenceID() != 0x1000001)
        GameAudio::Main::getAudio()->startSequenceBgm(0x1000001);

    if (seq == Scene::SCENE_TITLE)
    {
        reset();
        return;
    }

    mInfo.timer = 0;
    mInfo.mode = mInfo.oldMode;

    if (mInfo.mode == 14)
    {
        for (int i = 0; i < 4; i++)
        {
            if (mPlayerData[i].mState == 2)
                mPlayerData[i].mState = 1;
        }
        mInfo.mode = 12;
        return;
    }

    if (seq == Scene::SCENE_GHOST_LOAD_SAVE)
    {
        if (gSequenceInfo._30 == 0)
            mInfo.mode = 12;
        else
            mInfo.mode = 6;
        return;
    }

    if (seq == (Scene::SceneType)12 || seq == Scene::SCENE_COURSE || seq == Scene::SCENE_MAP_SELECT)
    {
        reset();
        mInfo.nextMode = 13;
        mInfo.mode = 12;
        mInfo.oldMode = 12;
        mInfo.nextScene = Scene::SCENE_MAX;
        if (seq == (Scene::SceneType)12 && gSequenceInfo._2c == 1)
        {
            mInfo.nextMode = 10;
            mInfo.mode = 9;
            mInfo.oldMode = 9;
        }
    }

    switch (gRaceInfo.mRaceMode)
    {
    case 0:
        mInfo._20 = 0;
        break;
    case 1:
        mInfo._20 = 2;
        break;
    case 2:
        mInfo._20 = 1;
        break;
    case 3:
        mInfo._20 = 3;
        mMsgId = 0;
        break;
    case 4:
        mInfo._20 = 3;
        mMsgId = 2;
        break;
    case 5:
        mInfo._20 = 3;
        mMsgId = 1;
        break;
    case 6:
    case 7:
    default:
#line 1702
        JUT_ASSERT(0);
    }

    switch (gRaceInfo.mRaceLevel)
    {
    case 0:
        mInfo.engineType = 0;
        break;
    case 1:
        mInfo.engineType = 1;
        break;
    case 2:
        mInfo.engineType = 2;
        break;
    case 3:
        mInfo.engineType = 3;
        break;
    default:
#line 1720
        JUT_ASSERT(0);
    }

    for (int i = 0; i < gRaceInfo.mStatusNum; i++)
    {
        KartGamePad *pad = gRaceInfo.getKartInfo(i)->getPad();
        if (pad)
        {
            for (int j = 0; j < 4; j++)
            {
                if (pad == gpaKartPad[j])
                {
                    mPlayerData[i].mCharNoF = j;
                    mPlayerPadNo[j] = i;
                }
            }
        }
    }

    for (int i = 0; i < gRaceInfo.mStatusNum; i++)
    {
        KartGamePad *pad = gRaceInfo.getKartInfo(i)->getPad(1);
        if (pad)
        {
            for (int j = 0; j < 4; j++)
            {
                if (pad == gpaKartPad[j])
                {
                    mPlayerData[i].mCharNoB = j;
                    mPlayerPadNo[j] = i;
                }
            }
        }
    }

    mInfo.playerNum = 0;
    for (int i = 0; i < gRaceInfo.mStatusNum; i++)
    {
        if (gRaceInfo.getKartInfo(i)->getPad(1))
            mInfo.playerNum += 2;
        else
        {
            mInfo.playerNum += 1;
            mPlayerData[i].mCharNoB = mPlayerData[i].mCharNoF;
        }
    }
    mInfo.playerNum -= 1;

    mInfo.entryNum = gRaceInfo.mStatusNum;
    mInfo.entryNum -= 1;

    switch (mInfo.entryNum)
    {
    case 0:
        switch (mInfo.playerNum)
        {
        case 0:
            mInfo._18 = 0;
            break;
        case 1:
            mInfo._18 = 1;
            break;
        default:
#line 1773
            JUT_ASSERT(0);
        }
        break;
    case 1:
        switch (mInfo.playerNum)
        {
        case 1:
            mInfo._18 = 0;
            break;
        case 2:
            mInfo._18 = 1;
            break;
        default:
#line 1789
            JUT_ASSERT(0);
        }
        break;
    case 2:
        switch (mInfo.playerNum)
        {
        case 2:
            mInfo._18 = 0;
            break;
        case 3:
            mInfo._18 = 2;
            break;
        default:
#line 1802
            JUT_ASSERT(0);
        }
        break;
    case 3:
        if (mInfo.playerNum == 3)
            mInfo._18 = 0;
        else
        {
#line 1812
            JUT_ASSERT(0);
        }
        break;
    default:
#line 1817
        JUT_ASSERT(0);
    }

    if (seq == Scene::SCENE_COURSE || seq == Scene::SCENE_MAP_SELECT)
    {
        for (int i = 0; i < gRaceInfo.mStatusNum; i++)
        {
            if (gRaceInfo.getKartInfo(i)->getPad())
            {
                ECharID charId = gRaceInfo.getKartInfo(i)->getDriverCharID(0);
                for (int j = 0; j < 20; j++)
                {
                    if (charId == mChara[j].mCharId)
                    {
                        mPlayerData[i].mCharF = j;
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < gRaceInfo.mStatusNum; i++)
        {
            if (gRaceInfo.getKartInfo(i)->getPad(1))
            {
                ECharID charId = gRaceInfo.getKartInfo(i)->getDriverCharID(1);
                for (int j = 0; j < 20; j++)
                {
                    if (charId == mChara[j].mCharId)
                    {
                        mPlayerData[i].mCharB = j;
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < gRaceInfo.mStatusNum; i++)
        {
            mPlayerData[i].mCursor = -1;
            if (mPlayerData[i].mCharF != -1 && mPlayerData[i].mCharB != -1)
            {
                mPlayerData[i].mState = 1;
                EKartID kartId = gRaceInfo.getKartInfo(i)->getKartID();
                int kartNo = -1;
                for (int j = 0; j < 21; j++)
                {
                    if (kartId == mKartIndex[j])
                    {
                        kartNo = j;
                        break;
                    }
                }
                if (kartNo < 21)
                    mPlayerData[i].mKart = kartNo;
                else
                    mPlayerData[i].mKart = getKart(mPlayerData[i].mCharF, mPlayerData[i].mCharB);
            }
            mPlayerData[i].mState = 1;
        }
    }
}

SceneMenu::~SceneMenu() {}

void SceneMenu::draw()
{
    mOrtho->setPort();
    // the MenuBackground's J2DScreen is embedded at offset 0
    ((J2DScreen *)MenuBackground::ptr())->draw(0.0f, 0.0f, mOrtho);

    switch (mInfo.mode)
    {
    default:
    case 12:
    case 13:
    case 14:
        break;
    case 0:
    case 1:
    case 2:
        mScreenHowManyDrivers->draw(0.0f, 0.0f, mOrtho);
        break;
    case 3:
    case 4:
    case 5:
        mScreenSingleTagSelect->draw(0.0f, 0.0f, mOrtho);
        break;
    case 6:
    case 7:
    case 8:
        mScreenSelectMode->draw(0.0f, 0.0f, mOrtho);
        mNameEntry->draw(mOrtho);
        break;
    case 9:
    case 10:
    case 11:
        mScreenSelectBattle->draw(0.0f, 0.0f, mOrtho);
        break;
    }

    if (mInfo.mode < 15)
    {
    if (mInfo.mode < 12) { }
    else
    {
        for (int i = 0; i < 10; i++)
        {
            mCharScreens[i]->draw(mCharPos[i].x, mCharPos[i].y, mOrtho);
        }
        switch (mInfo.entryNum)
        {
        case 0:
            mScreenChar1->draw(0.0f, 0.0f, mOrtho);
            break;
        case 1:
            mScreenChar2->draw(0.0f, 0.0f, mOrtho);
            break;
        case 2:
            mScreenChar3->draw(0.0f, 0.0f, mOrtho);
            break;
        case 3:
            mScreenChar4->draw(0.0f, 0.0f, mOrtho);
            break;
        default:
#line 1938
            JUT_ASSERT(0);
        }
    }
    }

    mScreenPlayerIcon->draw(0.0f, 0.0f, mOrtho);
    MenuTitleLine::ptr()->draw(mOrtho);

    if (mInfo.mode < 15)
    {
    if (mInfo.mode < 12) { }
    else
    {
        for (int i = 0; i <= mInfo.entryNum; i++)
        {
            f32 ratio;
            switch (mInfo.entryNum)
            {
            case 0:
                mGrafs[0]->place(mCharRect[0].i.x, mCharRect[0].i.y + (mCharPaneBck[0].mPane->mOffset.y - 325.0f), mCharRect[0].f.x, mCharRect[0].f.y);
                mGrafs[0]->setPort();
                ratio = mCharRect[0].f.x / mCharRect[0].f.y;
                break;
            case 1:
                mGrafs[1 + i]->place(mCharRect[1 + i].i.x, mCharRect[1 + i].i.y + (mChar2Bck[0].mPane->mOffset.y - 325.0f), mCharRect[1 + i].f.x, mCharRect[1 + i].f.y);
                mGrafs[1 + i]->setPort();
                ratio = mCharRect[1 + i].f.x / mCharRect[1 + i].f.y;
                if (i == 0)
                    GXSetScissorBoxOffset(76, 0);
                else
                    GXSetScissorBoxOffset(0, 0);
                break;
            case 2:
                mGrafs[3 + i]->place(mCharRect[3 + i].i.x, mCharRect[3 + i].i.y + (mChar3Bck[0].mPane->mOffset.y - 325.0f), mCharRect[3 + i].f.x, mCharRect[3 + i].f.y);
                mGrafs[3 + i]->setPort();
                ratio = mCharRect[3 + i].f.x / mCharRect[3 + i].f.y;
                if (i == 0)
                    GXSetScissorBoxOffset(46, 0);
                else
                    GXSetScissorBoxOffset(0, 0);
                break;
            case 3:
                mGrafs[6 + i]->place(mCharRect[6 + i].i.x, mCharRect[6 + i].i.y + (mChar4Bck[0].mPane->mOffset.y - 325.0f), mCharRect[6 + i].f.x, mCharRect[6 + i].f.y);
                mGrafs[6 + i]->setPort();
                ratio = mCharRect[6 + i].f.x / mCharRect[6 + i].f.y;
                if (i == 0)
                    GXSetScissorBoxOffset(25, 0);
                else
                    GXSetScissorBoxOffset(0, 0);
                break;
            default:
#line 2010
                JUT_ASSERT(0);
            }
            mCharacterSelect3D->draw(i, ratio);
        }
        mOrtho->setPort();
    }
    }

    j3dSys.reinitGX();
    JPEffectMgr::mThis->draw_for2D();
    mOrtho->setPort();

    switch (mInfo.mode)
    {
    case 13:
        switch (mInfo.entryNum)
        {
        case 0:
            mScreenChar1Plus->draw(0.0f, 0.0f, mOrtho);
            break;
        case 1:
        case 2:
        case 3:
            break;
        default:
#line 2038
            JUT_ASSERT(0);
        }
        break;
    }
}

void SceneMenu::calc()
{
    switch (mInfo.mode)
    {
    case 0:
        entry_init();
        break;
    case 1:
        entry();
        break;
    case 2:
        entry_end();
        break;
    case 3:
        vs_init();
        break;
    case 4:
        vs();
        break;
    case 5:
        vs_end();
        break;
    case 6:
        mode_init();
        break;
    case 7:
        mode();
        break;
    case 8:
        mode_end();
        break;
    case 9:
        battle_init();
        break;
    case 10:
        battle();
        break;
    case 11:
        battle_end();
        break;
    case 12:
        character_init();
        for (int i = 0; i < 19; i++)
        {
            mChar1PlusBck[i].mFrame = 0.0f;
        }
        for (int i = 0; i < 2; i++)
        {
            mChar1PlusBrk[i].mFrame = 0.0f;
        }
        mChar1PlusBpk.mFrame = 0.0f;
        mChar1PlusBck[0].mPane->hide();
        mChar1PlusBck[1].mPane->hide();
        mChar1PlusBck[2].mPane->hide();
        mChar1PlusBck[3].mPane->hide();
        break;
    case 13:
        character();
        setChar1Plus();
        break;
    case 14:
        character_end();
        break;
    case 15:
        nextScene();
        break;
    default:
#line 2114
        JUT_ASSERT(0);
    }

    MenuBackground::ptr()->calc();
    MenuTitleLine::ptr()->calc();

    for (int i = 0; i < 9; i++)
    {
        mIconBck[6 + i].mFrame += 1.0f;
        if (mIconBck[6 + i].mFrame >= 75.0f)
            mIconBck[6 + i].mFrame = 14.0f;
    }
    for (int i = 0; i < 33; i++)
    {
        mIconBck[i].mAnm->setFrame(mIconBck[i].mFrame);
    }
}

void SceneMenu::calcModeAnm()
{
    calcEnginePic(&mModeBck[20]);
    caleEngine(&mModeBck[23]);
    caleEngine(&mModeBck[24]);
    calcEngineMoji(&mModeBck[25]);
    calcEnginePic(&mModeBck[27]);
    caleEngine(&mModeBck[29]);
    caleEngine(&mModeBck[30]);
    calcEngineMoji(&mModeBck[28]);
    calcEnginePic(&mModeBck[68]);
    caleEngine(&mModeBck[64]);
    caleEngine(&mModeBck[65]);
    calcEngineMoji(&mModeBck[69]);
    calcEngineBrkAnm(&mModeBrk[10]);
    calcEngineBrkAnm(&mModeBrk[11]);
    calcEngineBrkAnm(&mModeBrk[12]);
    calcEngineBrkAnm(&mModeBrk[13]);
    calcEngineBrkAnm(&mModeBrk[27]);
    calcEngineBrkAnm(&mModeBrk[28]);
}

void SceneMenu::caleEngine(SceneMenu::BckData *data)
{
    if (data->mFrame <= 20.0f)
    {
        data->mFrame = 20.0f;
    }
    else if (data->mFrame < 28.0f)
    {
        data->mFrame += 1.0f;
    }
    else
    {
        data->mFrame = 20.0f;
    }
}

void SceneMenu::calcEngineMoji(SceneMenu::BckData *data)
{
    if (data->mFrame <= 20.0f)
    {
        data->mFrame = 20.0f;
    }
    else if (data->mFrame < 28.0f)
    {
        if (data->mFrame == 24.0f)
        {
            J2DPane *pane = data->mPane;
#line 2366
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->changeTexture(mPowerMojiName[mInfo.engineType], 0);
        }
        data->mFrame += 1.0f;
    }
    else
    {
        data->mFrame = 20.0f;
    }
}

void SceneMenu::calcEnginePic(SceneMenu::BckData *data)
{
    if (data->mFrame <= 20.0f)
    {
        data->mFrame = 20.0f;
    }
    else if (data->mFrame < 28.0f)
    {
        if (data->mFrame == 24.0f)
        {
            J2DPane *pane = data->mPane;
#line 2383
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->changeTexture(mPowerPicName[mInfo.engineType], 0);
        }
        data->mFrame += 1.0f;
    }
    else
    {
        data->mFrame = 20.0f;
    }
}

void SceneMenu::calcEngineBrkAnm(SceneMenu::BrkData *data)
{
    if (data->mFrame <= 0.0f)
    {
        data->mFrame = 0.0f;
    }
    else if (data->mFrame < 29.0f)
    {
        data->mFrame += 1.0f;
    }
    else
    {
        data->mFrame = 0.0f;
    }
}

void SceneMenu::calcModeOrder()
{
    J2DPane *panes[3];
    for (int i = 0; i < 3; i++)
        panes[i] = mModeBck[10 + i].mPane;
    for (int i = 0; i < 2; i++)
    {
        for (int j = 2; j > i; j--)
        {
            J2DPane *a = panes[i];
            J2DPane *b = panes[j];
            if (a->mScale.x > b->mScale.x)
            {
                panes[i] = b;
                panes[j] = a;
            }
        }
    }
    for (int i = 0; i < 3; i++)
    {
        mModeBck[9].mPane->appendChild(panes[i]);
    }

    for (int i = 0; i < 3; i++)
        panes[i] = mModeBck[54 + i].mPane;
    for (int i = 0; i < 2; i++)
    {
        for (int j = 2; j > i; j--)
        {
            J2DPane *a = panes[i];
            J2DPane *b = panes[j];
            if (a->mScale.x > b->mScale.x)
            {
                panes[i] = b;
                panes[j] = a;
            }
        }
    }
    for (int i = 0; i < 3; i++)
    {
        mModeBck[53].mPane->appendChild(panes[i]);
    }
}

void SceneMenu::entry_init()
{
    if (mInfo.timer < 15)
    {
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcHowManyPlayers);
        mHowManyDriversBck[0].mFrame = mInfo.timer;

        switch (mInfo.playerNum)
        {
        case 0:
            mHowManyDriversBrk[0].mFrame = -1.0f;
            mHowManyDriversBrk[1].mFrame = 0.0f;
            mHowManyDriversBrk[2].mFrame = 0.0f;
            mHowManyDriversBrk[3].mFrame = 0.0f;
            mHowManyDriversBck[5].mFrame = 22.0f;
            mHowManyDriversBck[6].mFrame = 14.0f;
            mHowManyDriversBck[7].mFrame = 14.0f;
            mHowManyDriversBck[8].mFrame = 14.0f;
            mHowManyDriversBrk[4].mFrame = -1.0f;
            mHowManyDriversBrk[5].mFrame = 0.0f;
            mHowManyDriversBrk[6].mFrame = 0.0f;
            mHowManyDriversBrk[7].mFrame = 0.0f;
            mHowManyDriversBrk[8].mFrame = -1.0f;
            mHowManyDriversBrk[9].mFrame = 0.0f;
            mHowManyDriversBrk[10].mFrame = 0.0f;
            mHowManyDriversBrk[11].mFrame = 0.0f;
            mHowManyDriversBrk[12].mFrame = -1.0f;
            mHowManyDriversBrk[13].mFrame = 0.0f;
            mHowManyDriversBrk[14].mFrame = 0.0f;
            mHowManyDriversBrk[15].mFrame = 0.0f;
            mHowManyDriversBrk[16].mFrame = 0.0f;
            mHowManyDriversBrk[17].mFrame = 0.0f;
            mHowManyDriversBrk[18].mFrame = 0.0f;
            mHowManyDriversBrk[19].mFrame = 0.0f;
            mHowManyDriversBrk[20].mFrame = 0.0f;
            mHowManyDriversBrk[21].mFrame = 0.0f;
            break;
        case 1:
            mHowManyDriversBrk[0].mFrame = 0.0f;
            mHowManyDriversBrk[1].mFrame = -1.0f;
            mHowManyDriversBrk[2].mFrame = 0.0f;
            mHowManyDriversBrk[3].mFrame = 0.0f;
            mHowManyDriversBck[5].mFrame = 14.0f;
            mHowManyDriversBck[6].mFrame = 22.0f;
            mHowManyDriversBck[7].mFrame = 14.0f;
            mHowManyDriversBck[8].mFrame = 14.0f;
            mHowManyDriversBrk[4].mFrame = 0.0f;
            mHowManyDriversBrk[5].mFrame = -1.0f;
            mHowManyDriversBrk[6].mFrame = 0.0f;
            mHowManyDriversBrk[7].mFrame = 0.0f;
            mHowManyDriversBrk[8].mFrame = 0.0f;
            mHowManyDriversBrk[9].mFrame = -1.0f;
            mHowManyDriversBrk[10].mFrame = 0.0f;
            mHowManyDriversBrk[11].mFrame = 0.0f;
            mHowManyDriversBrk[12].mFrame = 0.0f;
            mHowManyDriversBrk[13].mFrame = -1.0f;
            mHowManyDriversBrk[14].mFrame = -1.0f;
            mHowManyDriversBrk[15].mFrame = 0.0f;
            mHowManyDriversBrk[16].mFrame = 0.0f;
            mHowManyDriversBrk[17].mFrame = 0.0f;
            mHowManyDriversBrk[18].mFrame = 0.0f;
            mHowManyDriversBrk[19].mFrame = 0.0f;
            mHowManyDriversBrk[20].mFrame = 0.0f;
            mHowManyDriversBrk[21].mFrame = 0.0f;
            break;
        case 2:
            mHowManyDriversBrk[0].mFrame = 0.0f;
            mHowManyDriversBrk[1].mFrame = 0.0f;
            mHowManyDriversBrk[2].mFrame = -1.0f;
            mHowManyDriversBrk[3].mFrame = 0.0f;
            mHowManyDriversBck[5].mFrame = 14.0f;
            mHowManyDriversBck[6].mFrame = 14.0f;
            mHowManyDriversBck[7].mFrame = 22.0f;
            mHowManyDriversBck[8].mFrame = 14.0f;
            mHowManyDriversBrk[4].mFrame = 0.0f;
            mHowManyDriversBrk[5].mFrame = 0.0f;
            mHowManyDriversBrk[6].mFrame = -1.0f;
            mHowManyDriversBrk[7].mFrame = 0.0f;
            mHowManyDriversBrk[8].mFrame = 0.0f;
            mHowManyDriversBrk[9].mFrame = 0.0f;
            mHowManyDriversBrk[10].mFrame = -1.0f;
            mHowManyDriversBrk[11].mFrame = 0.0f;
            mHowManyDriversBrk[12].mFrame = 0.0f;
            mHowManyDriversBrk[13].mFrame = 0.0f;
            mHowManyDriversBrk[14].mFrame = 0.0f;
            mHowManyDriversBrk[15].mFrame = -1.0f;
            mHowManyDriversBrk[16].mFrame = -1.0f;
            mHowManyDriversBrk[17].mFrame = -1.0f;
            mHowManyDriversBrk[18].mFrame = 0.0f;
            mHowManyDriversBrk[19].mFrame = 0.0f;
            mHowManyDriversBrk[20].mFrame = 0.0f;
            mHowManyDriversBrk[21].mFrame = 0.0f;
            break;
        case 3:
            mHowManyDriversBrk[0].mFrame = 0.0f;
            mHowManyDriversBrk[1].mFrame = 0.0f;
            mHowManyDriversBrk[2].mFrame = 0.0f;
            mHowManyDriversBrk[3].mFrame = -1.0f;
            mHowManyDriversBck[5].mFrame = 14.0f;
            mHowManyDriversBck[6].mFrame = 14.0f;
            mHowManyDriversBck[7].mFrame = 14.0f;
            mHowManyDriversBck[8].mFrame = 22.0f;
            mHowManyDriversBrk[4].mFrame = 0.0f;
            mHowManyDriversBrk[5].mFrame = 0.0f;
            mHowManyDriversBrk[6].mFrame = 0.0f;
            mHowManyDriversBrk[7].mFrame = -1.0f;
            mHowManyDriversBrk[8].mFrame = 0.0f;
            mHowManyDriversBrk[9].mFrame = 0.0f;
            mHowManyDriversBrk[10].mFrame = 0.0f;
            mHowManyDriversBrk[11].mFrame = -1.0f;
            mHowManyDriversBrk[12].mFrame = 0.0f;
            mHowManyDriversBrk[13].mFrame = 0.0f;
            mHowManyDriversBrk[14].mFrame = 0.0f;
            mHowManyDriversBrk[15].mFrame = 0.0f;
            mHowManyDriversBrk[16].mFrame = 0.0f;
            mHowManyDriversBrk[17].mFrame = 0.0f;
            mHowManyDriversBrk[18].mFrame = -1.0f;
            mHowManyDriversBrk[19].mFrame = -1.0f;
            mHowManyDriversBrk[20].mFrame = -1.0f;
            mHowManyDriversBrk[21].mFrame = -1.0f;
            break;
        default:
#line 2598
            JUT_ASSERT(0);
        }

        mHowManyDriversBck[9].mFrame = mInfo.timer;
        mHowManyDriversBck[10].mFrame = mInfo.timer;
        mHowManyDriversBck[11].mFrame = mInfo.timer;
        mHowManyDriversBck[12].mFrame = mInfo.timer;
        mHowManyDriversBck[1].mFrame = mInfo.timer;
        mHowManyDriversBck[2].mFrame = mInfo.timer;
        mHowManyDriversBck[3].mFrame = mInfo.timer;
        mHowManyDriversBck[4].mFrame = mInfo.timer;
    }
    else
    {
        mInfo.oldMode = mInfo.mode;
        mInfo.mode = 1;
        mInfo.timer = 0;
    }
}

void SceneMenu::entry()
{
    if (gpaGamePad[0]->mButtons.mTrigger & 0x100)
    {
        GameAudio::Main::getAudio()->startSystemSe(0x20003);
        switch (mInfo.playerNum)
        {
        case 0:
            mInfo.nextMode = 6;
            mInfo._18 = 0;
            mInfo.entryNum = 0;
            break;
        case 1:
        case 2:
        case 3:
            mInfo.nextMode = 3;
            break;
        default:
#line 2641
            JUT_ASSERT(0);
        }
        mInfo.mode = 2;
        mInfo.oldMode = 1;
        mInfo.timer = 0;
    }
    else if ((gpaGamePad[0]->mButtons.mTrigger & 0x200) && System::mspDisplay->getFader()->getStatus() == JUTFader::Status_In)
    {
        GameAudio::Main::getAudio()->startSystemSe(0x20004);
        if (System::mspDisplay->getFader())
            System::mspDisplay->getFader()->startFadeOut(15);
        mInfo.nextMode = 15;
        mInfo.mode = 2;
        mInfo.oldMode = 1;
        mInfo.timer = 0;
        mInfo.nextScene = Scene::SCENE_TITLE;
    }
    else if (gpaGamePad[0]->mButtons.mRepeat & 0x08000000)
    {
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
        mInfo.playerNum = (mInfo.playerNum + 3) % 4;
    }
    else if (gpaGamePad[0]->mButtons.mRepeat & 0x04000000)
    {
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
        mInfo.playerNum = (mInfo.playerNum + 1) % 4;
    }

    switch (mInfo.playerNum)
    {
    case 0:
    {
        f32 val1 = mHowManyDriversBck[1].mFrame + 1.0f;
        mHowManyDriversBck[1].mFrame = val1;
        if (val1 > 22.0f)
            mHowManyDriversBck[1].mFrame = 22.0f;
        f32 val2 = mHowManyDriversBck[2].mFrame - 1.0f;
        mHowManyDriversBck[2].mFrame = val2;
        if (val2 < 14.0f)
            mHowManyDriversBck[2].mFrame = 14.0f;
        f32 val3 = mHowManyDriversBck[3].mFrame - 1.0f;
        mHowManyDriversBck[3].mFrame = val3;
        if (val3 < 14.0f)
            mHowManyDriversBck[3].mFrame = 14.0f;
        f32 val4 = mHowManyDriversBck[4].mFrame - 1.0f;
        mHowManyDriversBck[4].mFrame = val4;
        if (val4 < 14.0f)
            mHowManyDriversBck[4].mFrame = 14.0f;

        mHowManyDriversBrk[0].mFrame = 1.0f;
        mHowManyDriversBrk[1].mFrame = 0.0f;
        mHowManyDriversBrk[2].mFrame = 0.0f;
        mHowManyDriversBrk[3].mFrame = 0.0f;

        f32 val5 = mHowManyDriversBck[5].mFrame + 1.0f;
        mHowManyDriversBck[5].mFrame = val5;
        if (val5 > 22.0f)
            mHowManyDriversBck[5].mFrame = 22.0f;
        f32 val6 = mHowManyDriversBck[6].mFrame - 1.0f;
        mHowManyDriversBck[6].mFrame = val6;
        if (val6 < 14.0f)
            mHowManyDriversBck[6].mFrame = 14.0f;
        f32 val7 = mHowManyDriversBck[7].mFrame - 1.0f;
        mHowManyDriversBck[7].mFrame = val7;
        if (val7 < 14.0f)
            mHowManyDriversBck[7].mFrame = 14.0f;
        f32 val8 = mHowManyDriversBck[8].mFrame - 1.0f;
        mHowManyDriversBck[8].mFrame = val8;
        if (val8 < 14.0f)
            mHowManyDriversBck[8].mFrame = 14.0f;

        mHowManyDriversBrk[4].mFrame = 1.0f;
        mHowManyDriversBrk[5].mFrame = 0.0f;
        mHowManyDriversBrk[6].mFrame = 0.0f;
        mHowManyDriversBrk[7].mFrame = 0.0f;

        f32 val9 = mHowManyDriversBck[9].mFrame + 1.0f;
        mHowManyDriversBck[9].mFrame = val9;
        if (val9 >= 75.0f)
            mHowManyDriversBck[9].mFrame = 14.0f;
        mHowManyDriversBck[10].mFrame = 14.0f;
        mHowManyDriversBck[11].mFrame = 14.0f;
        mHowManyDriversBck[12].mFrame = 14.0f;

        mHowManyDriversBrk[4].mFrame = 1.0f;
        mHowManyDriversBrk[5].mFrame = 0.0f;
        mHowManyDriversBrk[6].mFrame = 0.0f;
        mHowManyDriversBrk[7].mFrame = 0.0f;
        mHowManyDriversBrk[8].mFrame = 1.0f;
        mHowManyDriversBrk[9].mFrame = 0.0f;
        mHowManyDriversBrk[10].mFrame = 0.0f;
        mHowManyDriversBrk[11].mFrame = 0.0f;
        mHowManyDriversBrk[12].mFrame = 1.0f;
        mHowManyDriversBrk[13].mFrame = 0.0f;
        mHowManyDriversBrk[14].mFrame = 0.0f;
        mHowManyDriversBrk[15].mFrame = 0.0f;
        mHowManyDriversBrk[16].mFrame = 0.0f;
        mHowManyDriversBrk[17].mFrame = 0.0f;
        mHowManyDriversBrk[18].mFrame = 0.0f;
        mHowManyDriversBrk[19].mFrame = 0.0f;
        mHowManyDriversBrk[20].mFrame = 0.0f;
        mHowManyDriversBrk[21].mFrame = 0.0f;
        break;
    }
    case 1:
    {
        f32 val10 = mHowManyDriversBck[1].mFrame - 1.0f;
        mHowManyDriversBck[1].mFrame = val10;
        if (val10 < 14.0f)
            mHowManyDriversBck[1].mFrame = 14.0f;
        f32 val11 = mHowManyDriversBck[2].mFrame + 1.0f;
        mHowManyDriversBck[2].mFrame = val11;
        if (val11 > 22.0f)
            mHowManyDriversBck[2].mFrame = 22.0f;
        f32 val12 = mHowManyDriversBck[3].mFrame - 1.0f;
        mHowManyDriversBck[3].mFrame = val12;
        if (val12 < 14.0f)
            mHowManyDriversBck[3].mFrame = 14.0f;
        f32 val13 = mHowManyDriversBck[4].mFrame - 1.0f;
        mHowManyDriversBck[4].mFrame = val13;
        if (val13 < 14.0f)
            mHowManyDriversBck[4].mFrame = 14.0f;

        mHowManyDriversBrk[0].mFrame = 0.0f;
        mHowManyDriversBrk[1].mFrame = 1.0f;
        mHowManyDriversBrk[2].mFrame = 0.0f;
        mHowManyDriversBrk[3].mFrame = 0.0f;

        f32 val14 = mHowManyDriversBck[5].mFrame - 1.0f;
        mHowManyDriversBck[5].mFrame = val14;
        if (val14 < 14.0f)
            mHowManyDriversBck[5].mFrame = 14.0f;
        f32 val15 = mHowManyDriversBck[6].mFrame + 1.0f;
        mHowManyDriversBck[6].mFrame = val15;
        if (val15 > 22.0f)
            mHowManyDriversBck[6].mFrame = 22.0f;
        f32 val16 = mHowManyDriversBck[7].mFrame - 1.0f;
        mHowManyDriversBck[7].mFrame = val16;
        if (val16 < 14.0f)
            mHowManyDriversBck[7].mFrame = 14.0f;
        f32 val17 = mHowManyDriversBck[8].mFrame - 1.0f;
        mHowManyDriversBck[8].mFrame = val17;
        if (val17 < 14.0f)
            mHowManyDriversBck[8].mFrame = 14.0f;

        mHowManyDriversBrk[4].mFrame = 0.0f;
        mHowManyDriversBrk[5].mFrame = 1.0f;
        mHowManyDriversBrk[6].mFrame = 0.0f;
        mHowManyDriversBrk[7].mFrame = 0.0f;

        mHowManyDriversBck[9].mFrame = 14.0f;
        f32 val18 = mHowManyDriversBck[10].mFrame + 1.0f;
        mHowManyDriversBck[10].mFrame = val18;
        if (val18 >= 75.0f)
            mHowManyDriversBck[10].mFrame = 14.0f;
        mHowManyDriversBck[11].mFrame = 14.0f;
        mHowManyDriversBck[12].mFrame = 14.0f;

        mHowManyDriversBrk[4].mFrame = 0.0f;
        mHowManyDriversBrk[5].mFrame = 1.0f;
        mHowManyDriversBrk[6].mFrame = 0.0f;
        mHowManyDriversBrk[7].mFrame = 0.0f;
        mHowManyDriversBrk[8].mFrame = 0.0f;
        mHowManyDriversBrk[9].mFrame = 1.0f;
        mHowManyDriversBrk[10].mFrame = 0.0f;
        mHowManyDriversBrk[11].mFrame = 0.0f;
        mHowManyDriversBrk[12].mFrame = 0.0f;
        mHowManyDriversBrk[13].mFrame = 1.0f;
        mHowManyDriversBrk[14].mFrame = 1.0f;
        mHowManyDriversBrk[15].mFrame = 0.0f;
        mHowManyDriversBrk[16].mFrame = 0.0f;
        mHowManyDriversBrk[17].mFrame = 0.0f;
        mHowManyDriversBrk[18].mFrame = 0.0f;
        mHowManyDriversBrk[19].mFrame = 0.0f;
        mHowManyDriversBrk[20].mFrame = 0.0f;
        mHowManyDriversBrk[21].mFrame = 0.0f;
        break;
    }
    case 2:
    {
        f32 val19 = mHowManyDriversBck[1].mFrame - 1.0f;
        mHowManyDriversBck[1].mFrame = val19;
        if (val19 < 14.0f)
            mHowManyDriversBck[1].mFrame = 14.0f;
        f32 val20 = mHowManyDriversBck[2].mFrame - 1.0f;
        mHowManyDriversBck[2].mFrame = val20;
        if (val20 < 14.0f)
            mHowManyDriversBck[2].mFrame = 14.0f;
        f32 val21 = mHowManyDriversBck[3].mFrame + 1.0f;
        mHowManyDriversBck[3].mFrame = val21;
        if (val21 > 22.0f)
            mHowManyDriversBck[3].mFrame = 22.0f;
        f32 val22 = mHowManyDriversBck[4].mFrame - 1.0f;
        mHowManyDriversBck[4].mFrame = val22;
        if (val22 < 14.0f)
            mHowManyDriversBck[4].mFrame = 14.0f;

        mHowManyDriversBrk[0].mFrame = 0.0f;
        mHowManyDriversBrk[1].mFrame = 0.0f;
        mHowManyDriversBrk[2].mFrame = 1.0f;
        mHowManyDriversBrk[3].mFrame = 0.0f;

        f32 val23 = mHowManyDriversBck[5].mFrame - 1.0f;
        mHowManyDriversBck[5].mFrame = val23;
        if (val23 < 14.0f)
            mHowManyDriversBck[5].mFrame = 14.0f;
        f32 val24 = mHowManyDriversBck[6].mFrame - 1.0f;
        mHowManyDriversBck[6].mFrame = val24;
        if (val24 < 14.0f)
            mHowManyDriversBck[6].mFrame = 14.0f;
        f32 val25 = mHowManyDriversBck[7].mFrame + 1.0f;
        mHowManyDriversBck[7].mFrame = val25;
        if (val25 > 22.0f)
            mHowManyDriversBck[7].mFrame = 22.0f;
        f32 val26 = mHowManyDriversBck[8].mFrame - 1.0f;
        mHowManyDriversBck[8].mFrame = val26;
        if (val26 < 14.0f)
            mHowManyDriversBck[8].mFrame = 14.0f;

        mHowManyDriversBrk[4].mFrame = 0.0f;
        mHowManyDriversBrk[5].mFrame = 0.0f;
        mHowManyDriversBrk[6].mFrame = 1.0f;
        mHowManyDriversBrk[7].mFrame = 0.0f;

        mHowManyDriversBck[9].mFrame = 14.0f;
        mHowManyDriversBck[10].mFrame = 14.0f;
        f32 val27 = mHowManyDriversBck[11].mFrame + 1.0f;
        mHowManyDriversBck[11].mFrame = val27;
        if (val27 >= 75.0f)
            mHowManyDriversBck[11].mFrame = 14.0f;
        mHowManyDriversBck[12].mFrame = 14.0f;

        mHowManyDriversBrk[4].mFrame = 0.0f;
        mHowManyDriversBrk[5].mFrame = 0.0f;
        mHowManyDriversBrk[6].mFrame = 1.0f;
        mHowManyDriversBrk[7].mFrame = 0.0f;
        mHowManyDriversBrk[8].mFrame = 0.0f;
        mHowManyDriversBrk[9].mFrame = 0.0f;
        mHowManyDriversBrk[10].mFrame = 1.0f;
        mHowManyDriversBrk[11].mFrame = 0.0f;
        mHowManyDriversBrk[12].mFrame = 0.0f;
        mHowManyDriversBrk[13].mFrame = 0.0f;
        mHowManyDriversBrk[14].mFrame = 0.0f;
        mHowManyDriversBrk[15].mFrame = 1.0f;
        mHowManyDriversBrk[16].mFrame = 1.0f;
        mHowManyDriversBrk[17].mFrame = 0.0f;
        mHowManyDriversBrk[18].mFrame = 0.0f;
        mHowManyDriversBrk[19].mFrame = 0.0f;
        mHowManyDriversBrk[20].mFrame = 0.0f;
        mHowManyDriversBrk[21].mFrame = 0.0f;
        break;
    }
    case 3:
    {
        f32 val28 = mHowManyDriversBck[1].mFrame - 1.0f;
        mHowManyDriversBck[1].mFrame = val28;
        if (val28 < 14.0f)
            mHowManyDriversBck[1].mFrame = 14.0f;
        f32 val29 = mHowManyDriversBck[2].mFrame - 1.0f;
        mHowManyDriversBck[2].mFrame = val29;
        if (val29 < 14.0f)
            mHowManyDriversBck[2].mFrame = 14.0f;
        f32 val30 = mHowManyDriversBck[3].mFrame - 1.0f;
        mHowManyDriversBck[3].mFrame = val30;
        if (val30 < 14.0f)
            mHowManyDriversBck[3].mFrame = 14.0f;
        f32 val31 = mHowManyDriversBck[4].mFrame + 1.0f;
        mHowManyDriversBck[4].mFrame = val31;
        if (val31 > 22.0f)
            mHowManyDriversBck[4].mFrame = 22.0f;

        mHowManyDriversBrk[0].mFrame = 0.0f;
        mHowManyDriversBrk[1].mFrame = 0.0f;
        mHowManyDriversBrk[2].mFrame = 0.0f;
        mHowManyDriversBrk[3].mFrame = 1.0f;

        f32 val32 = mHowManyDriversBck[5].mFrame - 1.0f;
        mHowManyDriversBck[5].mFrame = val32;
        if (val32 < 14.0f)
            mHowManyDriversBck[5].mFrame = 14.0f;
        f32 val33 = mHowManyDriversBck[6].mFrame - 1.0f;
        mHowManyDriversBck[6].mFrame = val33;
        if (val33 < 14.0f)
            mHowManyDriversBck[6].mFrame = 14.0f;
        f32 val34 = mHowManyDriversBck[7].mFrame - 1.0f;
        mHowManyDriversBck[7].mFrame = val34;
        if (val34 < 14.0f)
            mHowManyDriversBck[7].mFrame = 14.0f;
        f32 val35 = mHowManyDriversBck[8].mFrame + 1.0f;
        mHowManyDriversBck[8].mFrame = val35;
        if (val35 > 22.0f)
            mHowManyDriversBck[8].mFrame = 22.0f;

        mHowManyDriversBrk[4].mFrame = 0.0f;
        mHowManyDriversBrk[5].mFrame = 0.0f;
        mHowManyDriversBrk[6].mFrame = 0.0f;
        mHowManyDriversBrk[7].mFrame = 1.0f;

        mHowManyDriversBck[9].mFrame = 14.0f;
        mHowManyDriversBck[10].mFrame = 14.0f;
        mHowManyDriversBck[11].mFrame = 14.0f;
        f32 val36 = mHowManyDriversBck[12].mFrame + 1.0f;
        mHowManyDriversBck[12].mFrame = val36;
        if (val36 >= 75.0f)
            mHowManyDriversBck[12].mFrame = 14.0f;

        mHowManyDriversBrk[4].mFrame = 0.0f;
        mHowManyDriversBrk[5].mFrame = 0.0f;
        mHowManyDriversBrk[6].mFrame = 0.0f;
        mHowManyDriversBrk[7].mFrame = 1.0f;
        mHowManyDriversBrk[8].mFrame = 0.0f;
        mHowManyDriversBrk[9].mFrame = 0.0f;
        mHowManyDriversBrk[10].mFrame = 0.0f;
        mHowManyDriversBrk[11].mFrame = 1.0f;
        mHowManyDriversBrk[12].mFrame = 0.0f;
        mHowManyDriversBrk[13].mFrame = 0.0f;
        mHowManyDriversBrk[14].mFrame = 0.0f;
        mHowManyDriversBrk[15].mFrame = 0.0f;
        mHowManyDriversBrk[16].mFrame = 0.0f;
        mHowManyDriversBrk[17].mFrame = 1.0f;
        mHowManyDriversBrk[18].mFrame = 1.0f;
        mHowManyDriversBrk[19].mFrame = 0.0f;
        mHowManyDriversBrk[20].mFrame = 0.0f;
        mHowManyDriversBrk[21].mFrame = 0.0f;
        break;
    }
    default:
#line 2849
        JUT_ASSERT(0);
    }
}

void SceneMenu::entry_end()
{
    if (mInfo.timer < 15)
    {
        MenuTitleLine::ptr()->lift();
        mHowManyDriversBck[0].mFrame = mInfo.timer + 75;
    }
    else
    {
        mInfo.oldMode = mInfo.mode;
        mInfo.mode = mInfo.nextMode;
        mInfo.timer = 0;
        mInfo._18 = 0;
    }
}

void SceneMenu::vs_init()
{
    if (mInfo.timer < 15)
    {
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcVsCoop);
        mIconBck[0].mPane->show();
        mIconBck[1].mPane->show();
        mIconBck[2].mPane->show();
        mSingleTagSelectBck[0].mPane->show();
        mIconBck[0].mFrame = mInfo.timer;
        mIconBck[1].mFrame = mInfo.timer;
        mIconBck[2].mFrame = mInfo.timer;
        mSingleTagSelectBck[0].mFrame = mInfo.timer;

        switch (mInfo.playerNum)
        {
        case 1:
        {
            J2DPane *pane = mIconBck[1].mPane;
#line 2900
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->changeTexture(mPlayerIconPlayerName[1], 0);
            mSingleTagSelectBck[1].mPane->show();
            mSingleTagSelectBck[12].mPane->hide();
            mSingleTagSelectBck[24].mPane->hide();
            switch (mInfo._18)
            {
            case 0:
                mSingleTagSelectBck[2].mFrame = 22.0f;
                mSingleTagSelectBck[3].mFrame = 14.0f;
                mSingleTagSelectBck[7].mFrame = 22.0f;
                mSingleTagSelectBck[8].mFrame = 14.0f;
                mSingleTagSelectBrk[0].mFrame = 1.0f;
                mSingleTagSelectBrk[1].mFrame = 0.0f;
                mSingleTagSelectBrk[2].mFrame = 1.0f;
                mSingleTagSelectBrk[3].mFrame = 1.0f;
                mSingleTagSelectBrk[4].mFrame = 0.0f;
                mSingleTagSelectBrk[5].mFrame = 0.0f;
                mSingleTagSelectBrk[6].mFrame = 0.0f;
                mSingleTagSelectBrk[7].mFrame = 1.0f;
                mSingleTagSelectBrk[8].mFrame = 1.0f;
                mSingleTagSelectBrk[9].mFrame = 0.0f;
                mSingleTagSelectBrk[10].mFrame = 0.0f;
                mSingleTagSelectBrk[11].mFrame = 1.0f;
                mSingleTagSelectBrk[12].mFrame = 1.0f;
                mSingleTagSelectBrk[13].mFrame = 1.0f;
                break;
            case 1:
                mSingleTagSelectBck[2].mFrame = 14.0f;
                mSingleTagSelectBck[3].mFrame = 22.0f;
                mSingleTagSelectBck[7].mFrame = 14.0f;
                mSingleTagSelectBck[8].mFrame = 22.0f;
                mSingleTagSelectBrk[0].mFrame = 0.0f;
                mSingleTagSelectBrk[1].mFrame = 1.0f;
                mSingleTagSelectBrk[2].mFrame = 0.0f;
                mSingleTagSelectBrk[3].mFrame = 0.0f;
                mSingleTagSelectBrk[4].mFrame = 1.0f;
                mSingleTagSelectBrk[5].mFrame = 1.0f;
                mSingleTagSelectBrk[6].mFrame = 1.0f;
                mSingleTagSelectBrk[7].mFrame = 0.0f;
                mSingleTagSelectBrk[8].mFrame = 0.0f;
                mSingleTagSelectBrk[9].mFrame = 1.0f;
                mSingleTagSelectBrk[10].mFrame = 1.0f;
                mSingleTagSelectBrk[11].mFrame = 0.0f;
                mSingleTagSelectBrk[12].mFrame = 0.0f;
                mSingleTagSelectBrk[13].mFrame = 0.0f;
                break;
            default:
#line 2952
                JUT_ASSERT(0);
            }
            mSingleTagSelectBck[4].mFrame = 10.0f;
            mSingleTagSelectBck[5].mFrame = 10.0f;
            mSingleTagSelectBck[6].mFrame = 10.0f;
            mSingleTagSelectBck[10].mFrame = 14.0f;
            mSingleTagSelectBck[11].mFrame = 14.0f;
            mSingleTagSelectBck[9].mFrame = 14.0f;
            mSingleTagSelectBtk[2].mFrame = 0.0f;
            mSingleTagSelectBtk[3].mFrame = 0.0f;
            mSingleTagSelectBtk[4].mFrame = 0.0f;
            mSingleTagSelectBpk[0].mFrame = 0.0f;
            mSingleTagSelectBpk[1].mFrame = 0.0f;
            break;
        }
        case 2:
        {
            J2DPane *pane = mIconBck[1].mPane;
#line 2972
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->changeTexture(mPlayerIconPlayerName[2], 0);
            mSingleTagSelectBck[1].mPane->hide();
            mSingleTagSelectBck[12].mPane->show();
            mSingleTagSelectBck[24].mPane->hide();
            switch (mInfo._18)
            {
            case 0:
                mSingleTagSelectBck[13].mFrame = 22.0f;
                mSingleTagSelectBck[14].mFrame = 14.0f;
                mSingleTagSelectBck[17].mFrame = 22.0f;
                mSingleTagSelectBck[18].mFrame = 14.0f;
                mSingleTagSelectBrk[14].mFrame = 1.0f;
                mSingleTagSelectBrk[15].mFrame = 0.0f;
                mSingleTagSelectBrk[16].mFrame = 1.0f;
                mSingleTagSelectBrk[17].mFrame = 1.0f;
                mSingleTagSelectBrk[18].mFrame = 1.0f;
                mSingleTagSelectBrk[19].mFrame = 0.0f;
                mSingleTagSelectBrk[20].mFrame = 0.0f;
                mSingleTagSelectBrk[21].mFrame = 0.0f;
                mSingleTagSelectBrk[22].mFrame = 1.0f;
                mSingleTagSelectBrk[23].mFrame = 1.0f;
                mSingleTagSelectBrk[24].mFrame = 1.0f;
                mSingleTagSelectBrk[25].mFrame = 0.0f;
                mSingleTagSelectBrk[26].mFrame = 0.0f;
                mSingleTagSelectBrk[27].mFrame = 0.0f;
                mSingleTagSelectBrk[28].mFrame = 1.0f;
                mSingleTagSelectBrk[29].mFrame = 1.0f;
                mSingleTagSelectBrk[30].mFrame = 1.0f;
                mSingleTagSelectBrk[31].mFrame = 0.0f;
                mSingleTagSelectBrk[32].mFrame = 0.0f;
                mSingleTagSelectBrk[33].mFrame = 1.0f;
                mSingleTagSelectBrk[34].mFrame = 1.0f;
                mSingleTagSelectBrk[35].mFrame = 0.0f;
                break;
            case 1:
                mSingleTagSelectBck[13].mFrame = 14.0f;
                mSingleTagSelectBck[14].mFrame = 22.0f;
                mSingleTagSelectBck[17].mFrame = 14.0f;
                mSingleTagSelectBck[18].mFrame = 22.0f;
                mSingleTagSelectBrk[14].mFrame = 0.0f;
                mSingleTagSelectBrk[15].mFrame = 1.0f;
                mSingleTagSelectBrk[16].mFrame = 0.0f;
                mSingleTagSelectBrk[17].mFrame = 0.0f;
                mSingleTagSelectBrk[18].mFrame = 0.0f;
                mSingleTagSelectBrk[19].mFrame = 1.0f;
                mSingleTagSelectBrk[20].mFrame = 1.0f;
                mSingleTagSelectBrk[21].mFrame = 1.0f;
                mSingleTagSelectBrk[22].mFrame = 0.0f;
                mSingleTagSelectBrk[23].mFrame = 0.0f;
                mSingleTagSelectBrk[24].mFrame = 0.0f;
                mSingleTagSelectBrk[25].mFrame = 1.0f;
                mSingleTagSelectBrk[26].mFrame = 1.0f;
                mSingleTagSelectBrk[27].mFrame = 1.0f;
                mSingleTagSelectBrk[28].mFrame = 0.0f;
                mSingleTagSelectBrk[29].mFrame = 0.0f;
                mSingleTagSelectBrk[30].mFrame = 0.0f;
                mSingleTagSelectBrk[31].mFrame = 1.0f;
                mSingleTagSelectBrk[32].mFrame = 1.0f;
                mSingleTagSelectBrk[33].mFrame = 0.0f;
                mSingleTagSelectBrk[34].mFrame = 0.0f;
                mSingleTagSelectBrk[35].mFrame = 1.0f;
                break;
            default:
#line 3041
                JUT_ASSERT(0);
            }
            mSingleTagSelectBck[15].mFrame = 10.0f;
            mSingleTagSelectBck[16].mFrame = 10.0f;
            mSingleTagSelectBck[19].mFrame = 14.0f;
            mSingleTagSelectBck[20].mFrame = 14.0f;
            mSingleTagSelectBck[21].mFrame = 14.0f;
            mSingleTagSelectBck[22].mFrame = 14.0f;
            mSingleTagSelectBck[23].mFrame = 14.0f;
            mSingleTagSelectBtk[7].mFrame = 0.0f;
            mSingleTagSelectBtk[8].mFrame = 0.0f;
            mSingleTagSelectBtk[9].mFrame = 0.0f;
            mSingleTagSelectBtk[10].mFrame = 0.0f;
            mSingleTagSelectBtk[11].mFrame = 0.0f;
            mSingleTagSelectBpk[2].mFrame = 0.0f;
            mSingleTagSelectBpk[3].mFrame = 0.0f;
            break;
        }
        case 3:
        {
            J2DPane *pane = mIconBck[1].mPane;
#line 3064
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->changeTexture(mPlayerIconPlayerName[3], 0);
            mSingleTagSelectBck[1].mPane->hide();
            mSingleTagSelectBck[12].mPane->hide();
            mSingleTagSelectBck[24].mPane->show();
            switch (mInfo._18)
            {
            case 0:
                mSingleTagSelectBck[25].mFrame = 22.0f;
                mSingleTagSelectBck[26].mFrame = 14.0f;
                mSingleTagSelectBck[27].mFrame = 14.0f;
                mSingleTagSelectBck[31].mFrame = 22.0f;
                mSingleTagSelectBck[32].mFrame = 14.0f;
                mSingleTagSelectBck[33].mFrame = 14.0f;
                mSingleTagSelectBrk[36].mFrame = 1.0f;
                mSingleTagSelectBrk[37].mFrame = 0.0f;
                mSingleTagSelectBrk[38].mFrame = 0.0f;
                mSingleTagSelectBrk[39].mFrame = 1.0f;
                mSingleTagSelectBrk[40].mFrame = 1.0f;
                mSingleTagSelectBrk[41].mFrame = 1.0f;
                mSingleTagSelectBrk[42].mFrame = 1.0f;
                mSingleTagSelectBrk[43].mFrame = 0.0f;
                mSingleTagSelectBrk[44].mFrame = 0.0f;
                mSingleTagSelectBrk[45].mFrame = 0.0f;
                mSingleTagSelectBrk[46].mFrame = 0.0f;
                mSingleTagSelectBrk[47].mFrame = 0.0f;
                mSingleTagSelectBrk[48].mFrame = 0.0f;
                mSingleTagSelectBrk[49].mFrame = 0.0f;
                mSingleTagSelectBrk[50].mFrame = 0.0f;
                mSingleTagSelectBrk[51].mFrame = 1.0f;
                mSingleTagSelectBrk[52].mFrame = 1.0f;
                mSingleTagSelectBrk[53].mFrame = 1.0f;
                mSingleTagSelectBrk[54].mFrame = 1.0f;
                mSingleTagSelectBrk[55].mFrame = 0.0f;
                mSingleTagSelectBrk[56].mFrame = 0.0f;
                mSingleTagSelectBrk[57].mFrame = 0.0f;
                mSingleTagSelectBrk[58].mFrame = 0.0f;
                mSingleTagSelectBrk[59].mFrame = 0.0f;
                mSingleTagSelectBrk[60].mFrame = 0.0f;
                mSingleTagSelectBrk[61].mFrame = 0.0f;
                mSingleTagSelectBrk[62].mFrame = 0.0f;
                mSingleTagSelectBrk[63].mFrame = 1.0f;
                mSingleTagSelectBrk[64].mFrame = 1.0f;
                mSingleTagSelectBrk[65].mFrame = 1.0f;
                mSingleTagSelectBrk[66].mFrame = 1.0f;
                mSingleTagSelectBrk[67].mFrame = 0.0f;
                mSingleTagSelectBrk[68].mFrame = 0.0f;
                mSingleTagSelectBrk[69].mFrame = 0.0f;
                mSingleTagSelectBrk[70].mFrame = 0.0f;
                mSingleTagSelectBrk[71].mFrame = 0.0f;
                mSingleTagSelectBrk[72].mFrame = 1.0f;
                mSingleTagSelectBrk[73].mFrame = 1.0f;
                mSingleTagSelectBrk[74].mFrame = 1.0f;
                mSingleTagSelectBrk[75].mFrame = 0.0f;
                mSingleTagSelectBrk[76].mFrame = 0.0f;
                mSingleTagSelectBrk[77].mFrame = 0.0f;
                break;
            case 1:
                mSingleTagSelectBck[25].mFrame = 14.0f;
                mSingleTagSelectBck[26].mFrame = 22.0f;
                mSingleTagSelectBck[27].mFrame = 14.0f;
                mSingleTagSelectBck[31].mFrame = 14.0f;
                mSingleTagSelectBck[32].mFrame = 22.0f;
                mSingleTagSelectBck[33].mFrame = 14.0f;
                mSingleTagSelectBrk[36].mFrame = 0.0f;
                mSingleTagSelectBrk[37].mFrame = 1.0f;
                mSingleTagSelectBrk[38].mFrame = 0.0f;
                mSingleTagSelectBrk[39].mFrame = 0.0f;
                mSingleTagSelectBrk[40].mFrame = 0.0f;
                mSingleTagSelectBrk[41].mFrame = 0.0f;
                mSingleTagSelectBrk[42].mFrame = 0.0f;
                mSingleTagSelectBrk[43].mFrame = 1.0f;
                mSingleTagSelectBrk[44].mFrame = 1.0f;
                mSingleTagSelectBrk[45].mFrame = 1.0f;
                mSingleTagSelectBrk[46].mFrame = 1.0f;
                mSingleTagSelectBrk[47].mFrame = 0.0f;
                mSingleTagSelectBrk[48].mFrame = 0.0f;
                mSingleTagSelectBrk[49].mFrame = 0.0f;
                mSingleTagSelectBrk[50].mFrame = 0.0f;
                mSingleTagSelectBrk[51].mFrame = 0.0f;
                mSingleTagSelectBrk[52].mFrame = 0.0f;
                mSingleTagSelectBrk[53].mFrame = 0.0f;
                mSingleTagSelectBrk[54].mFrame = 0.0f;
                mSingleTagSelectBrk[55].mFrame = 1.0f;
                mSingleTagSelectBrk[56].mFrame = 1.0f;
                mSingleTagSelectBrk[57].mFrame = 1.0f;
                mSingleTagSelectBrk[58].mFrame = 1.0f;
                mSingleTagSelectBrk[59].mFrame = 0.0f;
                mSingleTagSelectBrk[60].mFrame = 0.0f;
                mSingleTagSelectBrk[61].mFrame = 0.0f;
                mSingleTagSelectBrk[62].mFrame = 0.0f;
                mSingleTagSelectBrk[63].mFrame = 0.0f;
                mSingleTagSelectBrk[64].mFrame = 0.0f;
                mSingleTagSelectBrk[65].mFrame = 0.0f;
                mSingleTagSelectBrk[66].mFrame = 0.0f;
                mSingleTagSelectBrk[67].mFrame = 1.0f;
                mSingleTagSelectBrk[68].mFrame = 1.0f;
                mSingleTagSelectBrk[69].mFrame = 0.0f;
                mSingleTagSelectBrk[70].mFrame = 0.0f;
                mSingleTagSelectBrk[71].mFrame = 0.0f;
                mSingleTagSelectBrk[72].mFrame = 0.0f;
                mSingleTagSelectBrk[73].mFrame = 0.0f;
                mSingleTagSelectBrk[74].mFrame = 0.0f;
                mSingleTagSelectBrk[75].mFrame = 1.0f;
                mSingleTagSelectBrk[76].mFrame = 0.0f;
                mSingleTagSelectBrk[77].mFrame = 0.0f;
                break;
            case 2:
                mSingleTagSelectBck[25].mFrame = 14.0f;
                mSingleTagSelectBck[26].mFrame = 14.0f;
                mSingleTagSelectBck[27].mFrame = 22.0f;
                mSingleTagSelectBck[31].mFrame = 14.0f;
                mSingleTagSelectBck[32].mFrame = 14.0f;
                mSingleTagSelectBck[33].mFrame = 22.0f;
                mSingleTagSelectBrk[36].mFrame = 1.0f;
                mSingleTagSelectBrk[37].mFrame = 1.0f;
                mSingleTagSelectBrk[38].mFrame = 0.0f;
                mSingleTagSelectBrk[39].mFrame = 1.0f;
                mSingleTagSelectBrk[40].mFrame = 1.0f;
                mSingleTagSelectBrk[41].mFrame = 1.0f;
                mSingleTagSelectBrk[42].mFrame = 1.0f;
                mSingleTagSelectBrk[43].mFrame = 1.0f;
                mSingleTagSelectBrk[44].mFrame = 1.0f;
                mSingleTagSelectBrk[45].mFrame = 1.0f;
                mSingleTagSelectBrk[46].mFrame = 1.0f;
                mSingleTagSelectBrk[47].mFrame = 0.0f;
                mSingleTagSelectBrk[48].mFrame = 0.0f;
                mSingleTagSelectBrk[49].mFrame = 0.0f;
                mSingleTagSelectBrk[50].mFrame = 0.0f;
                mSingleTagSelectBrk[51].mFrame = 1.0f;
                mSingleTagSelectBrk[52].mFrame = 1.0f;
                mSingleTagSelectBrk[53].mFrame = 1.0f;
                mSingleTagSelectBrk[54].mFrame = 1.0f;
                mSingleTagSelectBrk[55].mFrame = 1.0f;
                mSingleTagSelectBrk[56].mFrame = 1.0f;
                mSingleTagSelectBrk[57].mFrame = 1.0f;
                mSingleTagSelectBrk[58].mFrame = 1.0f;
                mSingleTagSelectBrk[59].mFrame = 0.0f;
                mSingleTagSelectBrk[60].mFrame = 0.0f;
                mSingleTagSelectBrk[61].mFrame = 0.0f;
                mSingleTagSelectBrk[62].mFrame = 0.0f;
                mSingleTagSelectBrk[63].mFrame = 1.0f;
                mSingleTagSelectBrk[64].mFrame = 1.0f;
                mSingleTagSelectBrk[65].mFrame = 1.0f;
                mSingleTagSelectBrk[66].mFrame = 1.0f;
                mSingleTagSelectBrk[67].mFrame = 1.0f;
                mSingleTagSelectBrk[68].mFrame = 1.0f;
                mSingleTagSelectBrk[69].mFrame = 0.0f;
                mSingleTagSelectBrk[70].mFrame = 0.0f;
                mSingleTagSelectBrk[71].mFrame = 0.0f;
                mSingleTagSelectBrk[72].mFrame = 1.0f;
                mSingleTagSelectBrk[73].mFrame = 1.0f;
                mSingleTagSelectBrk[74].mFrame = 1.0f;
                mSingleTagSelectBrk[75].mFrame = 1.0f;
                mSingleTagSelectBrk[76].mFrame = 0.0f;
                mSingleTagSelectBrk[77].mFrame = 0.0f;
                break;
            default:
#line 3229
                JUT_ASSERT(0);
            }
            mSingleTagSelectBck[28].mFrame = 10.0f;
            mSingleTagSelectBck[29].mFrame = 10.0f;
            mSingleTagSelectBck[30].mFrame = 10.0f;
            mSingleTagSelectBck[34].mFrame = 14.0f;
            mSingleTagSelectBck[35].mFrame = 14.0f;
            mSingleTagSelectBck[36].mFrame = 14.0f;
            mSingleTagSelectBck[37].mFrame = 14.0f;
            mSingleTagSelectBck[38].mFrame = 14.0f;
            mSingleTagSelectBck[39].mFrame = 14.0f;
            mSingleTagSelectBck[40].mFrame = 14.0f;
            mSingleTagSelectBck[41].mFrame = 14.0f;
            mSingleTagSelectBck[42].mFrame = 14.0f;
            mSingleTagSelectBtk[15].mFrame = 0.0f;
            mSingleTagSelectBtk[16].mFrame = 0.0f;
            mSingleTagSelectBtk[17].mFrame = 0.0f;
            mSingleTagSelectBtk[18].mFrame = 0.0f;
            mSingleTagSelectBtk[19].mFrame = 0.0f;
            mSingleTagSelectBtk[20].mFrame = 0.0f;
            mSingleTagSelectBtk[21].mFrame = 0.0f;
            mSingleTagSelectBtk[22].mFrame = 0.0f;
            mSingleTagSelectBtk[23].mFrame = 0.0f;
            mSingleTagSelectBpk[4].mFrame = 0.0f;
            mSingleTagSelectBpk[5].mFrame = 0.0f;
            mSingleTagSelectBpk[6].mFrame = 0.0f;
            break;
        }
        case 0:
        default:
#line 3262
            JUT_ASSERT(0);
            break;
        }
    }
    else
    {
        mInfo.oldMode = mInfo.mode;
        mInfo.mode = 4;
        mInfo.timer = 0;
    }
}

void SceneMenu::vs() {}

void SceneMenu::vs_end()
{
    if (mInfo.timer < 15)
    {
        MenuTitleLine::ptr()->lift();
        mIconBck[0].mFrame = mInfo.timer + 75;
        mSingleTagSelectBck[0].mFrame = mInfo.timer + 75;
    }
    else
    {
        mIconBck[0].mPane->hide();
        mIconBck[1].mPane->hide();
        mIconBck[2].mPane->hide();
        mSingleTagSelectBck[0].mPane->hide();

        switch (mInfo.playerNum)
        {
        case 0:
            mInfo.entryNum = 0;
            break;
        case 1:
            switch (mInfo._18)
            {
            case 0:
                mInfo.entryNum = 1;
                break;
            case 1:
                mInfo.entryNum = 0;
                break;
            default:
#line 3814
                JUT_ASSERT(0);
            }
            break;
        case 2:
            switch (mInfo._18)
            {
            case 0:
                mInfo.entryNum = 2;
                break;
            case 1:
                mInfo.entryNum = 1;
                break;
            default:
#line 3827
                JUT_ASSERT(0);
            }
            break;
        case 3:
            switch (mInfo._18)
            {
            case 0:
                mInfo.entryNum = 3;
                break;
            case 1:
                mInfo.entryNum = 1;
                break;
            case 2:
                mInfo.entryNum = 2;
                break;
            default:
#line 3843
                JUT_ASSERT(0);
            }
            break;
        default:
#line 3848
            JUT_ASSERT(0);
        }

        mInfo.oldMode = mInfo.mode;
        mInfo.mode = mInfo.nextMode;
        mInfo.timer = 0;
        if (mInfo._20 == 2 && mInfo.playerNum != 0)
            mInfo._20 = 0;
    }
}

void SceneMenu::mode_init() {}

void SceneMenu::mode() {}

void SceneMenu::mode_end() {}

void SceneMenu::battle_init()
{
    if (mInfo.timer < 15)
    {
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcBattleSelect);
        for (int i = 0; i < 15; i++)
            mBattleBck[i].mPane->show();
        if (mInfo.timer < 10)
            mBattleBck[0].mFrame = mInfo.timer;

        for (int i = 0; i < 3; i++)
        {
            if (i == mMsgId)
            {
                mBattleBck[1 + i].mFrame = 19.0f;
                mBattleBck[4 + i].mFrame = 19.0f;
                mBattleBck[7 + i].mFrame = 19.0f;
                mBattleBck[10 + i].mFrame = 19.0f;

                J2DPane *pane = mBattleBck[4 + i].mPane;
#line 5491
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                static_cast<J2DPicture *>(pane)->setWhite(mBattleWhite);
                static_cast<J2DPicture *>(pane)->setBlack(mBattleBlack);

                pane = mBattleBck[7 + i].mPane;
#line 5496
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                static_cast<J2DPicture *>(pane)->setWhite(mBattleWhite2);

                pane = mBattleBck[10 + i].mPane;
#line 5500
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                static_cast<J2DPicture *>(pane)->setWhite(mBattleWhite3);
            }
            else
            {
                mBattleBck[1 + i].mFrame = 14.0f;
                mBattleBck[4 + i].mFrame = 14.0f;
                mBattleBck[7 + i].mFrame = 14.0f;
                mBattleBck[10 + i].mFrame = 14.0f;

                J2DPane *pane = mBattleBck[4 + i].mPane;
#line 5510
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                static_cast<J2DPicture *>(pane)->setWhite(mBattleWhiteInactive);
                static_cast<J2DPicture *>(pane)->setBlack(mBattleBlackInactive);

                pane = mBattleBck[7 + i].mPane;
#line 5515
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                static_cast<J2DPicture *>(pane)->setWhite(mBattleWhite2Inactive);

                pane = mBattleBck[10 + i].mPane;
#line 5519
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                static_cast<J2DPicture *>(pane)->setWhite(mBattleWhite3Inactive);
            }
        }

        mBattleBck[13].mFrame = 24.0f;
        J2DPane *pane = mBattleBck[14].mPane;
#line 5526
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        static_cast<J2DPicture *>(pane)->changeTexture(mBattleName[mMsgId], 0);
        changeText();
    }
    else
    {
        mInfo.oldMode = mInfo.mode;
        mInfo.mode = 10;
        mInfo.timer = 0;
    }
}

void SceneMenu::battle()
{
    if (mBattleBck[13].mFrame == 24.0f)
    {
        JUTGamePad *pad = gpaGamePad[0];
        u32 trigger = pad->getTrigger();
        if (trigger & JUTGamePad::A)
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20003);
            mInfo.nextMode = 12;
            mInfo.mode = 11;
            mInfo.oldMode = 10;
            mInfo.timer = 0;
        }
        else if (trigger & JUTGamePad::B)
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20005);
            mInfo.nextMode = 6;
            mInfo.mode = 11;
            mInfo.oldMode = 10;
            mInfo.timer = 0;
        }
        else
        {
            u32 repeat = pad->getRepeat();
            if (repeat & JUTGamePad::MAINSTICK_UP)
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20000);
                mMsgId = (mMsgId + 2) % 3;
                mBattleBck[13].mFrame = 34.0f;
            }
            else if (repeat & JUTGamePad::MAINSTICK_DOWN)
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20000);
                mMsgId = (mMsgId + 1) % 3;
                mBattleBck[13].mFrame = 34.0f;
            }
        }
    }

    for (int i = 0; i < 3; i++)
    {
        if (i == mMsgId)
        {
            f32 f0 = mBattleBck[1 + i].mFrame + -1.0f;
            mBattleBck[1 + i].mFrame = f0;
            if (f0 > 19.0f)
                mBattleBck[1 + i].mFrame = 19.0f;
            f32 f1 = mBattleBck[4 + i].mFrame + -1.0f;
            mBattleBck[4 + i].mFrame = f1;
            if (f1 > 19.0f)
                mBattleBck[4 + i].mFrame = 19.0f;
            f32 f2 = mBattleBck[7 + i].mFrame + -1.0f;
            mBattleBck[7 + i].mFrame = f2;
            if (f2 > 19.0f)
                mBattleBck[7 + i].mFrame = 19.0f;
            f32 f3 = mBattleBck[10 + i].mFrame + -1.0f;
            mBattleBck[10 + i].mFrame = f3;
            if (f3 > 19.0f)
                mBattleBck[10 + i].mFrame = 19.0f;

            J2DPane *pane = mBattleBck[4 + i].mPane;
#line 5585
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->setWhite(mBattleWhite);
            static_cast<J2DPicture *>(pane)->setBlack(mBattleBlack);

            pane = mBattleBck[7 + i].mPane;
#line 5592
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->setWhite(mBattleWhite2);

            pane = mBattleBck[10 + i].mPane;
#line 5597
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->setWhite(mBattleWhite3);
        }
        else
        {
            f32 f0 = mBattleBck[1 + i].mFrame - -1.0f;
            mBattleBck[1 + i].mFrame = f0;
            if (f0 < 14.0f)
                mBattleBck[1 + i].mFrame = 14.0f;
            f32 f1 = mBattleBck[4 + i].mFrame - -1.0f;
            mBattleBck[4 + i].mFrame = f1;
            if (f1 < 14.0f)
                mBattleBck[4 + i].mFrame = 14.0f;
            f32 f2 = mBattleBck[7 + i].mFrame - -1.0f;
            mBattleBck[7 + i].mFrame = f2;
            if (f2 < 14.0f)
                mBattleBck[7 + i].mFrame = 14.0f;
            f32 f3 = mBattleBck[10 + i].mFrame - -1.0f;
            mBattleBck[10 + i].mFrame = f3;
            if (f3 < 14.0f)
                mBattleBck[10 + i].mFrame = 14.0f;

            J2DPane *pane = mBattleBck[4 + i].mPane;
#line 5607
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->setWhite(mBattleWhiteInactive);
            static_cast<J2DPicture *>(pane)->setBlack(mBattleBlackInactive);

            pane = mBattleBck[7 + i].mPane;
#line 5614
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->setWhite(mBattleWhite2Inactive);

            pane = mBattleBck[10 + i].mPane;
#line 5619
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->setWhite(mBattleWhite3Inactive);
        }
    }

    f32 frame = mBattleBck[13].mFrame;
    if (24.0f != frame)
    {
        mBattleBck[13].mFrame = frame + -1.0f;
        if (frame == 29.0f)
        {
            J2DPane *pane = mBattleBck[14].mPane;
#line 5628
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->changeTexture(mBattleName[mMsgId], 0);
            changeText();
        }
        if (frame >= 27.0f)
            mBattleBck[13].mFrame = 24.0f;
    }
}

void SceneMenu::battle_end()
{
    if (mInfo.timer < 15)
    {
        MenuTitleLine::ptr()->lift();
        mHowManyDriversBck[0].mFrame = mInfo.timer + 75;
        if (mInfo.timer >= 5)
        {
            mBattleBck[0].mFrame = 14 - mInfo.timer;
        }
    }
    else
    {
        for (int i = 0; i < 15; i++)
        {
            mBattleBck[i].mPane->hide();
        }
        mInfo.oldMode = mInfo.mode;
        mInfo.mode = mInfo.nextMode;
        mInfo.timer = 0;
        for (int i = 0; i < 4; i++)
        {
            mPlayerData[i].mCursor = -1;
            mPlayerData[i].mCharNoF = -1;
            mPlayerData[i].mCharNoB = -1;
            mPlayerData[i].mCharF = -1;
            mPlayerData[i].mCharB = -1;
            mPlayerData[i].mKart = -1;
            mPlayerData[i].mState = 0;
        }
    }
}

void SceneMenu::character_init() {}

void SceneMenu::character()
{
    if (mRouletteTimer != 0)
    {
        mRouletteTimer++;

        bool pressed = false;
        for (int i = 0; i <= mInfo.playerNum; i++)
        {
            u32 button = gpaGamePad[i]->mButtons.mButton;
            if ((button & JUTGamePad::R) && (button & JUTGamePad::L))
            {
                pressed = true;
                break;
            }
        }

        if (!pressed && mRouletteTimer < 30)
        {
            rndRoulette();
        }
        else
        {
            rndSelect();
            mRouletteTimer = 0;
            GameAudio::Main::getAudio()->startSystemSe(0x20028);
        }
    }
    else
    {
        for (int i = 0; i <= mInfo.playerNum; i++)
        {
            KartGamePad **pads = &gpaGamePad[0];
            if (mInfo.mode == 14)
                break;

            int done = 0;
            for (int j = 0; j <= mInfo.entryNum; j++)
            {
                if (mPlayerData[j].mState == 2)
                    done++;
            }
            if (mInfo.entryNum + 1 == done)
                continue;

            KartGamePad *pad = pads[i];
            u32 trigger = pad->getTrigger();

            if (trigger & JUTGamePad::A)
            {
                selectA(i);
            }
            else if (trigger & JUTGamePad::B)
            {
                selectB(i);
            }
            else
            {
                u32 repeat = pad->getRepeat();
                if (repeat & JUTGamePad::MAINSTICK_UP)
                    selectUp(i);
                else if (repeat & JUTGamePad::MAINSTICK_DOWN)
                    selectDown(i);
                else if (repeat & JUTGamePad::MAINSTICK_LEFT)
                    selectLeft(i);
                else if (repeat & JUTGamePad::MAINSTICK_RIGHT)
                    selectRight(i);
                else if (trigger & JUTGamePad::Z)
                    selectZ(i);
                else if ((pad->mButtons.mButton & JUTGamePad::R) && (pad->mButtons.mButton & JUTGamePad::L) && i == 0)
                {
                    if (isRndRoulette())
                        mRouletteTimer = 1;
                }
            }
        }
    }

    f32 dec = -1.0f;
    f32 endFrame = 26.0f;
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            f32 frame = mCharPane[i][j].mFrame + dec;
            mCharPane[i][j].mFrame = frame;
            if (endFrame == frame)
            {
                if (mSelectAnm[i][j].mEnable == 0)
                    continue;
                mSelectAnm[i][j].mEnable = 0;
                int no = mSelectAnm[i][j].mNo;
                const int padNo = mPlayerPadNo[no];
                int cursor = mPlayerData[no].mCursor;

                if (mPlayerData[padNo].mCharNoF == no && mPlayerData[padNo].mCharF == -1)
                {
#line 6075
                    JUT_ASSERT(0);
                }
                else if (mPlayerData[padNo].mCharNoB == no && mPlayerData[padNo].mCharB == -1)
                {
                    setCharacterFlag();
                    do
                    {
                        cursor = (cursor + 1) % 20;
                    } while (mCharacterFlag[cursor] == 0);
                    mPlayerData[no].mCursor = cursor;
                }
                else
                {
                    mPlayerData[no].mCursor = -1;
                }

                if (mPlayerData[padNo].mCharF != -1 && mPlayerData[padNo].mCharB != -1)
                {
                    mPlayerData[padNo].mState = 1;
                    mPlayerData[padNo].mKart = getKart(mPlayerData[padNo].mCharF, mPlayerData[padNo].mCharB);
                }
            }
            else if (frame > 27.0f)
            {
                mCharPane[i][j].mFrame = 27.0f;
            }
        }
    }

    characterAnm();

    int ready = 0;
    for (int i = 0; i <= mInfo.entryNum; i++)
    {
        if (mPlayerData[i].mState == 2)
        {
            mCharacterSelect3D->setCharacterStatus(i, 0, CharacterSelect3D::STATUS_2);
            mCharacterSelect3D->setCharacterStatus(i, 1, CharacterSelect3D::STATUS_2);
        }
        else
        {
            mCharacterSelect3D->setCharacterStatus(i, 0, CharacterSelect3D::STATUS_1);
            mCharacterSelect3D->setCharacterStatus(i, 1, CharacterSelect3D::STATUS_1);
        }

        if (mPlayerData[i].mState == 2 && (u8)mCharacterSelect3D->isNext(i))
            ready++;
    }

    if (mInfo.entryNum + 1 == ready)
        setRaceData();
    else
        characterOkWaitAnm();
}

void SceneMenu::character_end()
{
    if (mInfo.timer < 25)
    {
        MenuTitleLine::ptr()->lift();
        if (mInfo.timer >= 5)
        {
            for (int i = 0; i < 10; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    mCharPane[i][j].mFrame = 24 - mInfo.timer;
                }
            }

            switch (mInfo.entryNum)
            {
            case 0:
                for (int i = 0; i < 4; i++)
                    mCharPaneBck[i].mFrame = 24 - mInfo.timer;
                break;
            case 1:
                for (int i = 0; i < 7; i++)
                    mChar2Bck[i].mFrame = 24 - mInfo.timer;
                break;
            case 2:
                for (int i = 0; i < 8; i++)
                    mChar3Bck[i].mFrame = 24 - mInfo.timer;
                break;
            case 3:
                for (int i = 0; i < 9; i++)
                    mChar4Bck[i].mFrame = 24 - mInfo.timer;
                break;
            default:
#line 6155
                JUT_ASSERT(0);
            }
        }
    }
    else
    {
        for (int i = 0; i < 10; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                mCharPane[i][j].mPane->hide();
            }
        }

        switch (mInfo.entryNum)
        {
        case 0:
            for (int i = 0; i < 9; i++)
                mCharPaneBck[i].mPane->hide();
            break;
        case 1:
            for (int i = 0; i < 20; i++)
                mChar2Bck[i].mPane->hide();
            break;
        case 2:
            for (int i = 0; i < 27; i++)
                mChar3Bck[i].mPane->hide();
            break;
        case 3:
            for (int i = 0; i < 26; i++)
                mChar4Bck[i].mPane->hide();
            break;
        default:
#line 6181
            JUT_ASSERT(0);
        }

        mInfo.oldMode = mInfo.mode;
        mInfo.mode = mInfo.nextMode;
        mInfo.timer = 0;
    }

    characterAnm();
    characterOkWaitAnm();
}

void SceneMenu::nextScene()
{
#line 6206
    JUT_ASSERT(mInfo.nextScene == Scene::SCENE_TITLE || mInfo.nextScene == Scene::SCENE_COURSE || mInfo.nextScene == Scene::SCENE_MAP_SELECT || mInfo.nextScene == Scene::SCENE_GHOST_LOAD_SAVE);

    if (SEQUENCEAPP_isReady(mInfo.nextScene))
    {
        SequenceApp::ptr()->setNextScene(mInfo.nextScene);
    }
}

void SceneMenu::selectA(int no)
{
    if (isSelectAnm(no))
        return;

    int padNo = mPlayerPadNo[no];
    switch (mPlayerData[padNo].mState)
    {
    case 0:
    {
        int cursor = mPlayerData[no].mCursor;
        if (cursor == -1)
            break;
        GameAudio::Main::getAudio()->startSystemSe(mCharVoice[cursor]);
        if (mPlayerData[padNo].mCharNoF == no && mPlayerData[padNo].mCharF == -1)
        {
            mPlayerData[padNo].mCharF = cursor;
            int row = cursor / 2;
            int col = cursor % 2;
            mCharPane[row][col].mFrame = 19.0f;
            mSelectAnm[row][col].mEnable = 1;
            mSelectAnm[row][col].mNo = no;
        }
        else if (mPlayerData[padNo].mCharNoB == no && mPlayerData[padNo].mCharB == -1)
        {
            mPlayerData[padNo].mCharB = cursor;
            int row = cursor / 2;
            int col = cursor % 2;
            mCharPane[row][col].mFrame = 19.0f;
            mSelectAnm[row][col].mEnable = 1;
            mSelectAnm[row][col].mNo = no;
        }
        break;
    }
    case 1:
    {
        PlayerData &pd = mPlayerData[padNo];
        int charF = pd.mCharF;
        int charB = pd.mCharB;
        if (charF >= 0 && charF < 20)
        {
            GameAudio::Main::getAudio()->startSystemSe(mCharGoVoice[charF]);
        }
        if (charB >= 0 && charB < 20)
        {
            GameAudio::Main::getAudio()->startSystemSe(mCharGoVoice[charB]);
        }
        pd.mState = 2;
        break;
    }
    case 2:
        break;
    default:
#line 6268
        JUT_ASSERT(0);
    }
}

void SceneMenu::selectB(int no)
{
    if (isSelectAnm(no))
        return;

    const int padNo = mPlayerPadNo[no];
    switch (mPlayerData[padNo].mState)
    {
    case 0:
        if (no == 0)
        {
            if ((mPlayerData[0].mCharNoF != 0 || mPlayerData[0].mCharF == -1) &&
                (mPlayerData[0].mCharNoB != 0 || mPlayerData[0].mCharB == -1))
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20005);
                switch (mInfo._20)
                {
                case 0:
                case 1:
                case 2:
                    mInfo.nextMode = 6;
                    break;
                case 3:
                    mInfo.nextMode = 9;
                    break;
                default:
#line 6303
                    JUT_ASSERT(0);
                }
                mInfo.mode = 14;
                mInfo.oldMode = 13;
                mInfo.timer = 0;
                for (int i = 0; i < 4; i++)
                {
                    mPlayerData[i].mCursor = -1;
                    mPlayerData[i].mCharNoF = -1;
                    mPlayerData[i].mCharNoB = -1;
                    mPlayerData[i].mCharF = -1;
                    mPlayerData[i].mCharB = -1;
                    mPlayerData[i].mKart = -1;
                    mPlayerData[i].mState = 0;
                }
            }
        }
        {
            int cellNo = -1;
            if (mPlayerData[padNo].mCharNoB == no && mPlayerData[padNo].mCharB != -1)
            {
                GameAudio::Main::getAudio()->startSystemSe(0x2000d);
                cellNo = mPlayerData[padNo].mCharB;
                mPlayerData[padNo].mCharB = -1;
            }
            else if (mPlayerData[padNo].mCharNoF == no && mPlayerData[padNo].mCharF != -1)
            {
                GameAudio::Main::getAudio()->startSystemSe(0x2000d);
                cellNo = mPlayerData[padNo].mCharF;
                mPlayerData[padNo].mCharF = -1;
            }
            if (cellNo != -1)
            {
                mPlayerData[no].mCursor = cellNo;
                mPlayerData[padNo].mState = 0;
                mPlayerData[padNo].mKart = -1;
            }
        }
        break;
    case 2:
        if ((u8)mCharacterSelect3D->isCancel(padNo))
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20005);
            mPlayerData[padNo].mState = 1;
        }
        break;
    default:
#line 6352
        JUT_ASSERT(0);
    }
}

void SceneMenu::selectUp(int no)
{
    if (isSelectAnm(no))
        return;
    switch (mPlayerData[mPlayerPadNo[no]].mState)
    {
    case 0:
        if (mPlayerData[no].mCursor != -1)
        {
            if (gpaGamePad[no]->testRepeat(JUTGamePad::MAINSTICK_LEFT))
                mPlayerData[no].mCursor = moveCursor(mPlayerData[no].mCursor, CURSOR_UP_LEFT);
            else if (gpaGamePad[no]->testRepeat(JUTGamePad::MAINSTICK_RIGHT))
                mPlayerData[no].mCursor = moveCursor(mPlayerData[no].mCursor, CURSOR_UP_RIGHT);
            else
                mPlayerData[no].mCursor = moveCursor(mPlayerData[no].mCursor, CURSOR_UP);
        }
        break;
    case 1:
    case 2:
        break;
    default:
#line 6392
        JUT_ASSERT(0);
    }
}

void SceneMenu::selectDown(int no)
{
    if (isSelectAnm(no))
        return;
    switch (mPlayerData[mPlayerPadNo[no]].mState)
    {
    case 0:
        if (mPlayerData[no].mCursor != -1)
        {
            if (gpaGamePad[no]->testRepeat(JUTGamePad::MAINSTICK_LEFT))
                mPlayerData[no].mCursor = moveCursor(mPlayerData[no].mCursor, CURSOR_DOWN_LEFT);
            else if (gpaGamePad[no]->testRepeat(JUTGamePad::MAINSTICK_RIGHT))
                mPlayerData[no].mCursor = moveCursor(mPlayerData[no].mCursor, CURSOR_DOWN_RIGHT);
            else
                mPlayerData[no].mCursor = moveCursor(mPlayerData[no].mCursor, CURSOR_DOWN);
        }
        break;
    case 1:
    case 2:
        break;
    default:
#line 6432
        JUT_ASSERT(0);
    }
}

void SceneMenu::selectLeft(int no)
{
    if (isSelectAnm(no))
        return;

    const int padNo = mPlayerPadNo[no];
    switch (mPlayerData[padNo].mState)
    {
    case 0:
        if (mPlayerData[no].mCursor != -1)
        {
            mPlayerData[no].mCursor = moveCursor(mPlayerData[no].mCursor, CURSOR_LEFT);
        }
        break;
    case 1:
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
        mPlayerData[padNo].mKart = changeKart(mPlayerData[padNo].mCharF, mPlayerData[padNo].mCharB, mPlayerData[padNo].mKart, -1);
        break;
    case 2:
        break;
    default:
#line 6468
        JUT_ASSERT(0);
    }
}

void SceneMenu::selectRight(int no)
{
    if (isSelectAnm(no))
        return;

    const int padNo = mPlayerPadNo[no];
    switch (mPlayerData[padNo].mState)
    {
    case 0:
        if (mPlayerData[no].mCursor != -1)
        {
            mPlayerData[no].mCursor = moveCursor(mPlayerData[no].mCursor, CURSOR_RIGHT);
        }
        break;
    case 1:
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
        mPlayerData[padNo].mKart = changeKart(mPlayerData[padNo].mCharF, mPlayerData[padNo].mCharB, mPlayerData[padNo].mKart, 1);
        break;
    case 2:
        break;
    default:
#line 6504
        JUT_ASSERT(0);
    }
}

void SceneMenu::selectZ(int no)
{
    if (isSelectAnm(no))
        return;

    int padNo = mPlayerPadNo[no];
    int i;
    for (i = 0; i <= mInfo.playerNum; i++)
    {
        if (no != i && mPlayerPadNo[i] == padNo)
            break;
    }

    switch (mPlayerData[padNo].mState)
    {
    case 0:
        break;
    case 1:
    {
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
        int charF = mPlayerData[padNo].mCharF;
        mPlayerData[padNo].mCharF = mPlayerData[padNo].mCharB;
        mPlayerData[padNo].mCharB = charF;
        int charNoF = mPlayerData[padNo].mCharNoF;
        mPlayerData[padNo].mCharNoF = mPlayerData[padNo].mCharNoB;
        mPlayerData[padNo].mCharNoB = charNoF;
        if (i <= mInfo.playerNum)
        {
            int cursor = mPlayerData[no].mCursor;
            mPlayerData[no].mCursor = mPlayerData[i].mCursor;
            mPlayerData[i].mCursor = cursor;
        }
        break;
    }
    case 2:
        break;
    default:
#line 6552
        JUT_ASSERT(0);
    }
}

bool SceneMenu::isSelectAnm(int no)
{
    for (int i = 0; i < 20; i++)
    {
        if (no == mSelectAnm[i / 2][i % 2].mNo && mSelectAnm[i / 2][i % 2].mEnable)
            return true;
    }
    return false;
}

int SceneMenu::moveCursor(int no, SceneMenu::Cursor cursor)
{
    int keep = no;
    int out = no;
    setCharacterFlag();
    mCharacterFlag[no] = 1;
    switch (cursor)
    {
    case CURSOR_UP:
    case CURSOR_DOWN:
        if (no % 2 != 0)
        {
            if (mCharacterFlag[no - 1] != 0)
                out = no - 1;
        }
        else
        {
            if (mCharacterFlag[no + 1] != 0)
                out = no + 1;
        }
        break;
    case CURSOR_UP_LEFT:
    {
        no = (no + 18) % 20;
        if (no == 8 || no == 9)
        {
            if (!gSystemRecord.isGameUnlocked(SystemRecord::KinopioCombi))
                no = (no + 18) % 20;
        }
        else
        {
            if (no == 18 || no == 19)
            {
                if (!gSystemRecord.isGameUnlocked(SystemRecord::BossCombi))
                    no = (no + 18) % 20;
            }
        }
        if (no % 2 != 0)
        {
            if (mCharacterFlag[no - 1] != 0)
                out = no - 1;
        }
        break;
    }
    case CURSOR_UP_RIGHT:
    {
        int sel = (no + 22) % 20;
        if (sel == 8 || sel == 9)
        {
            if (!gSystemRecord.isGameUnlocked(SystemRecord::KinopioCombi))
                sel = (sel + 22) % 20;
        }
        else
        {
            if (sel == 18 || sel == 19)
            {
                if (!gSystemRecord.isGameUnlocked(SystemRecord::BossCombi))
                    sel = (sel + 22) % 20;
            }
        }
        if (sel % 2 != 0)
        {
            if (mCharacterFlag[sel - 1] != 0)
                out = sel - 1;
        }
        break;
    }
    case CURSOR_DOWN_LEFT:
    {
        int sel = (no + 18) % 20;
        if (sel == 8 || sel == 9)
        {
            if (!gSystemRecord.isGameUnlocked(SystemRecord::KinopioCombi))
                sel = (sel + 18) % 20;
        }
        else
        {
            if (sel == 18 || sel == 19)
            {
                if (!gSystemRecord.isGameUnlocked(SystemRecord::BossCombi))
                    sel = (sel + 18) % 20;
            }
        }
        if (sel % 2 == 0)
        {
            if (mCharacterFlag[sel + 1] != 0)
                out = sel + 1;
        }
        break;
    }
    case CURSOR_DOWN_RIGHT:
    {
        int sel = (no + 22) % 20;
        if (sel == 8 || sel == 9)
        {
            if (!gSystemRecord.isGameUnlocked(SystemRecord::KinopioCombi))
                sel = (sel + 22) % 20;
        }
        else
        {
            if (sel == 18 || sel == 19)
            {
                if (!gSystemRecord.isGameUnlocked(SystemRecord::BossCombi))
                    sel = (sel + 22) % 20;
            }
        }
        if (sel % 2 == 0)
        {
            if (mCharacterFlag[sel + 1] != 0)
                out = sel + 1;
        }
        break;
    }
    case CURSOR_LEFT:
        do
        {
            no = (no + 18) % 20;
        } while (mCharacterFlag[no] == 0);
        out = no;
        break;
    case CURSOR_RIGHT:
        do
        {
            no = (no + 22) % 20;
        } while (mCharacterFlag[no] == 0);
        out = no;
        break;
    default:
#line 6702
        JUT_ASSERT(0);
    }
    if (keep != out)
    {
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
    }
    return out;
}

bool SceneMenu::isRndRoulette()
{
    bool ret = true;
    for (int i = 0; i <= mInfo.playerNum; i++)
    {
        if (isSelectAnm(i))
        {
            ret = false;
            break;
        }
    }
    for (int i = 0; i <= mInfo.entryNum; i++)
    {
        if (mPlayerData[i].mCharF != -1 || mPlayerData[i].mCharB != -1)
        {
            ret = false;
            break;
        }
    }
    return ret;
}

void SceneMenu::rndRoulette()
{
    if (mInfo.timer % 5 == 0)
    {
        GameAudio::Main::getAudio()->startSystemSe(0x20000);
        setCharSelFlag();
        for (int i = 0; i <= mInfo.playerNum; i++)
        {
            int charF, charB;
            getRndCharSel(charF, charB);
            mPlayerData[i].mCursor = charB;
        }
    }
}

void SceneMenu::rndSelect()
{
    setCharSelFlag();
    for (int i = 0; i <= mInfo.entryNum; i++)
    {
        int charF, charB;
        getRndCharSel(charF, charB);
        mPlayerData[i].mCharF = charF;
        int fRow = charF / 2;
        int fCol = charF % 2;
        mCharPane[fRow][fCol].mFrame = 19.0f;
        mSelectAnm[fRow][fCol].mEnable = 1;
        mSelectAnm[fRow][fCol].mNo = mPlayerData[i].mCharNoF;
        mPlayerData[i].mCharB = charB;
        int bRow = charB / 2;
        int bCol = charB % 2;
        mCharPane[bRow][bCol].mFrame = 19.0f;
        mSelectAnm[bRow][bCol].mEnable = 1;
        mSelectAnm[bRow][bCol].mNo = mPlayerData[i].mCharNoB;
    }
}

void SceneMenu::setCharacterFlag()
{
    for (int i = 0; i < 20; i++)
    {
        mCharacterFlag[i] = 1;
    }
    for (int i = 0; i < 4; i++)
    {
        int no = mPlayerData[i].mCursor;
        if (no >= 0 && no < 20)
            mCharacterFlag[no] = 0;
    }
    for (int i = 0; i < 4; i++)
    {
        int no = mPlayerData[i].mCharF;
        if (no >= 0 && no < 20)
            mCharacterFlag[no] = 0;
    }
    for (int i = 0; i < 4; i++)
    {
        int no = mPlayerData[i].mCharB;
        if (no >= 0 && no < 20)
            mCharacterFlag[no] = 0;
    }
    if (!gSystemRecord.isGameUnlocked(SystemRecord::KinopioCombi))
    {
        mCharacterFlag[8] = 0;
        mCharacterFlag[9] = 0;
    }
    if (gSystemRecord.isGameUnlocked(SystemRecord::BossCombi))
        return;
    mCharacterFlag[18] = 0;
    mCharacterFlag[19] = 0;
}

int SceneMenu::getKart(int charF, int charB)
{
#line 6850
    JUT_MINMAX_ASSERT(0, charF, 20);
#line 6851
    JUT_MINMAX_ASSERT(0, charB, 20);

    int sel;
    if (mChara[charF].mWeight > mChara[charB].mWeight)
    {
        sel = charF;
    }
    else if (mChara[charF].mWeight < mChara[charB].mWeight)
    {
        sel = charB;
    }
    else
    {
        sel = charF;
    }

    for (int i = 0; i < 21; i++)
    {
        if (mChara[sel].mKartId == mKartIndex[i])
            return i;
    }
    return 0;
}

int SceneMenu::changeKart(int charF, int charB, int kart, int dir)
{
#line 6887
    JUT_MINMAX_ASSERT(0, charF, 20);
#line 6888
    JUT_MINMAX_ASSERT(0, charB, 20);
#line 6889
    JUT_MINMAX_ASSERT(0, kart, 21);

    int best = charB;
    if (mChara[charF].mWeight > mChara[charB].mWeight)
        best = charF;

    int weight = mChara[best].mWeight;
    int kartNo = kart;
    while (true)
    {
        if (dir > 0)
        {
            for (int i = 0; i < dir; i++)
            {
                do
                {
                    kartNo = (kartNo + 1) % 21;
                } while (mKartList[kartNo].mKartWeight != weight && kartNo != 20);
            }
        }
        else if (dir < 0)
        {
            for (int i = 0; i < -dir; i++)
            {
                do
                {
                    kartNo = (kartNo + 20) % 21;
                } while (mKartList[kartNo].mKartWeight != weight && kartNo != 20);
            }
        }
        else
        {
            kartNo = kart;
        }
        if (SystemRecord::tstSecretKart((EKartID)mKartIndex[kartNo], gSystemRecord.mSecretKart))
            return kartNo;
    }
}

void SceneMenu::characterAnm()
{
    int charPlayer[20];

    for (int i = 0; i < 20; i++)
    {
        charPlayer[i] = -1;
        mCharCCBpk[i / 2][i % 2 + 4].mPane->hide();
        mCharState[i] = 0;
    }

    for (int i = 0; i < 4; i++)
    {
        int no = mPlayerData[i].mCharNoF;
        int charF = mPlayerData[i].mCharF;
        if (charF >= 0 && charF < 20 && no >= 0 && no < 4)
        {
            int col = charF % 2;
            int row = charF / 2;
            charPlayer[charF] = no;
            J2DPicture *pane = (J2DPicture *)mCharCCBpk[row][col + 4].mPane;
            pane->show();
#line 6960
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            pane->changeTexture(mCharCCName[no + col * 4], 0);
            mCharState[charF] = 1;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        int no = mPlayerData[i].mCharNoB;
        int charB = mPlayerData[i].mCharB;
        if (charB >= 0 && charB < 20 && no >= 0 && no < 4)
        {
            int col = charB % 2;
            int row = charB / 2;
            charPlayer[charB] = no;
            J2DPicture *pane = (J2DPicture *)mCharCCBpk[row][col + 4].mPane;
            pane->show();
#line 6976
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            pane->changeTexture(mCharCCName[no + col * 4], 0);
            mCharState[charB] = 2;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        int cursor = mPlayerData[i].mCursor;
        if (cursor >= 0 && cursor < 20)
        {
            int col = cursor % 2;
            int row = cursor / 2;
            charPlayer[cursor] = i;
            J2DPicture *pane = (J2DPicture *)mCharCCBpk[row][col + 4].mPane;
            pane->show();
#line 6991
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            pane->changeTexture(mCharCCName[i + col * 4], 0);
            mCharState[cursor] = 2;
        }
    }

    for (int i = 0; i < 20; i++)
    {
        int row = i / 2;
        int col = i % 2;

        switch (mCharState[i])
        {
        case 0:
            mCharCCBrk[row][col].mPane->show();
            mCharCCBrk[row][col].mFrame = 0.0f;
            mCharCCBrk[row][col + 2].mPane->show();
            mCharCCBrk[row][col + 2].mFrame = 0.0f;
            mCharCCBpk[row][col].mFrame = 0.0f;
            mCharCCBpk[row][col + 2].mFrame = 1.0f;
            mCharCCBpk[row][col + 4].mFrame = 0.0f;
            break;
        case 1:
            mCharCCBrk[row][col].mPane->show();
            mCharCCBrk[row][col].mFrame = (f32)(charPlayer[i] + 1);
            mCharCCBrk[row][col + 2].mPane->hide();
            mCharCCBrk[row][col + 2].mFrame = (f32)(charPlayer[i] + 1);
            mCharCCBpk[row][col].mFrame = (f32)(charPlayer[i] + 1);
            mCharCCBpk[row][col + 2].mFrame = 0.0f;
            mCharCCBpk[row][col + 4].mFrame = 0.0f;
            break;
        case 2:
            mCharCCBrk[row][col].mPane->hide();
            mCharCCBrk[row][col].mFrame = (f32)(charPlayer[i] + 1);
            mCharCCBrk[row][col + 2].mPane->show();
            mCharCCBrk[row][col + 2].mFrame = (f32)(charPlayer[i] + 1);
            mCharCCBpk[row][col].mFrame = (f32)(charPlayer[i] + 1);
            mCharCCBpk[row][col + 2].mFrame = 1.0f;
            mCharCCBpk[row][col + 4].mFrame = (f32)(charPlayer[i] + 1);
            break;
        default:
#line 7042
            JUT_ASSERT(0);
        }
    }

    switch (mInfo.entryNum)
    {
    case 0:
    {
        int charF = mPlayerData[0].mCharF;
        int charB = mPlayerData[0].mCharB;
        int kart = mPlayerData[0].mKart;

        if (charF >= 0 && charF < 20)
        {
            mCharacterSelect3D->setCharacter(0, 0, (ECharID)mChara[charF].mCharId,
                                             JGeometry::TVec3f(170.0f, 110.0f, 380.0f),
                                             JGeometry::TVec3f(352.0f, 346.0f, 4.0f), 0.95f);
        }
        else
        {
            mCharacterSelect3D->setCharacterStatus(0, 0, CharacterSelect3D::STATUS_0);
        }

        if (charB >= 0 && charB < 20)
        {
            mCharacterSelect3D->setCharacter(0, 1, (ECharID)mChara[charB].mCharId,
                                             JGeometry::TVec3f(-385.0f, -150.0f, -380.0f),
                                             JGeometry::TVec3f(353.0f, 13.6f, 356.0f), 1.85f);
        }
        else
        {
            mCharacterSelect3D->setCharacterStatus(0, 1, CharacterSelect3D::STATUS_0);
        }

        if (kart >= 0 && kart < 21)
        {
            mCharacterSelect3D->setKart(0, (EKartID)mKartList[kart].mKartId,
                                        JGeometry::TVec3f(0.0f, 5.0f, 0.0f),
                                        JGeometry::TVec3f(0.0f, 320.0f, 0.0f), 1.2f);
        }
        break;
    }
    case 1:
        for (int i = 0; i < 2; i++)
        {
            int charF = mPlayerData[i].mCharF;
            int charB = mPlayerData[i].mCharB;
            int kart = mPlayerData[i].mKart;

            if (charF >= 0 && charF < 20)
            {
                mCharacterSelect3D->setCharacter(i, 0, (ECharID)mChara[charF].mCharId,
                                                 JGeometry::TVec3f(110.0f, 58.0f, 265.0f),
                                                 JGeometry::TVec3f(353.0f, 353.0f, 2.0f), 1.0f);
            }
            else
            {
                mCharacterSelect3D->setCharacterStatus(i, 0, CharacterSelect3D::STATUS_0);
            }

            if (charB >= 0 && charB < 20)
            {
                mCharacterSelect3D->setCharacter(i, 1, (ECharID)mChara[charB].mCharId,
                                                 JGeometry::TVec3f(-190.0f, -25.0f, -380.0f),
                                                 JGeometry::TVec3f(355.0f, 7.5f, 0.0f), 1.4f);
            }
            else
            {
                mCharacterSelect3D->setCharacterStatus(i, 1, CharacterSelect3D::STATUS_0);
            }

            if (kart >= 0 && kart < 21)
            {
                mCharacterSelect3D->setKart(i, (EKartID)mKartList[kart].mKartId,
                                            JGeometry::TVec3f(-10.0f, 0.0f, 0.0f),
                                            JGeometry::TVec3f(0.0f, 320.0f, 0.0f), 1.13f);
            }
        }
        break;
    case 2:
        for (int i = 0; i < 3; i++)
        {
            int charF = mPlayerData[i].mCharF;
            int charB = mPlayerData[i].mCharB;
            int kart = mPlayerData[i].mKart;

            if (charF >= 0 && charF < 20)
            {
                mCharacterSelect3D->setCharacter(i, 0, (ECharID)mChara[charF].mCharId,
                                                 JGeometry::TVec3f(64.0f, 62.0f, 265.0f),
                                                 JGeometry::TVec3f(354.0f, 354.0f, 0.5f), 0.85f);
            }
            else
            {
                mCharacterSelect3D->setCharacterStatus(i, 0, CharacterSelect3D::STATUS_0);
            }

            if (charB >= 0 && charB < 20)
            {
                mCharacterSelect3D->setCharacter(i, 1, (ECharID)mChara[charB].mCharId,
                                                 JGeometry::TVec3f(-90.0f, -15.0f, -380.0f),
                                                 JGeometry::TVec3f(357.0f, 2.5f, 0.0f), 1.3f);
            }
            else
            {
                mCharacterSelect3D->setCharacterStatus(i, 1, CharacterSelect3D::STATUS_0);
            }

            if (kart >= 0 && kart < 21)
            {
                mCharacterSelect3D->setKart(i, (EKartID)mKartList[kart].mKartId,
                                            JGeometry::TVec3f(-10.0f, 10.0f, 0.0f),
                                            JGeometry::TVec3f(0.0f, 320.0f, 0.0f), 1.0f);
            }
        }
        break;
    case 3:
        for (int i = 0; i < 4; i++)
        {
            int charF = mPlayerData[i].mCharF;
            int charB = mPlayerData[i].mCharB;
            int kart = mPlayerData[i].mKart;

            if (charF >= 0 && charF < 20)
            {
                mCharacterSelect3D->setCharacter(i, 0, (ECharID)mChara[charF].mCharId,
                                                 JGeometry::TVec3f(45.0f, 60.0f, 265.0f),
                                                 JGeometry::TVec3f(351.0f, 353.5f, 1.0f), 0.75f);
            }
            else
            {
                mCharacterSelect3D->setCharacterStatus(i, 0, CharacterSelect3D::STATUS_0);
            }

            if (charB >= 0 && charB < 20)
            {
                mCharacterSelect3D->setCharacter(i, 1, (ECharID)mChara[charB].mCharId,
                                                 JGeometry::TVec3f(-65.0f, 5.0f, -380.0f),
                                                 JGeometry::TVec3f(356.0f, 2.4f, 0.0f), 1.15f);
            }
            else
            {
                mCharacterSelect3D->setCharacterStatus(i, 1, CharacterSelect3D::STATUS_0);
            }

            if (kart >= 0 && kart < 21)
            {
                mCharacterSelect3D->setKart(i, (EKartID)mKartList[kart].mKartId,
                                            JGeometry::TVec3f(-10.0f, 15.0f, 0.0f),
                                            JGeometry::TVec3f(0.0f, 320.0f, 0.0f), 0.85f);
            }
        }
        break;
    default:
#line 7197
        JUT_ASSERT(0);
    }
}

void SceneMenu::characterOkWaitAnm()
{
    switch (mInfo.entryNum)
    {
    case 0:
        switch (mPlayerData[0].mState)
        {
        case 0:
        case 2:
            mCharPaneBck[4].mPane->hide();
            mCharPaneBck[5].mPane->hide();
            mCharPaneBck[6].mPane->hide();
            mCharPaneBck[7].mPane->hide();
            break;
        case 1:
            mCharPaneBck[4].mPane->show();
            mCharPaneBck[5].mPane->show();
            mCharPaneBck[6].mPane->show();
            mCharPaneBck[7].mPane->hide();
            break;
        default:
#line 7228
            JUT_ASSERT(0);
        }
        break;
    case 1:
        for (int i = 0; i < 2; i++)
        {
            switch (mPlayerData[i].mState)
            {
            case 0:
                mChar2Bck[7 + i].mPane->hide();
                mChar2Bck[9 + i].mPane->hide();
                mChar2Bck[11 + i].mPane->hide();
                mChar2Bck[13 + i].mPane->hide();
                mChar2Bck[15 + i].mPane->hide();
                mChar2Bck[17 + i].mPane->hide();
                break;
            case 1:
                mChar2Bck[7 + i].mPane->show();
                mChar2Bck[9 + i].mPane->show();
                mChar2Bck[11 + i].mPane->show();
                mChar2Bck[13 + i].mPane->show();
                mChar2Bck[15 + i].mPane->show();
                mChar2Bck[17 + i].mPane->hide();
                break;
            case 2:
                mChar2Bck[7 + i].mPane->hide();
                mChar2Bck[9 + i].mPane->hide();
                mChar2Bck[11 + i].mPane->hide();
                mChar2Bck[13 + i].mPane->hide();
                mChar2Bck[15 + i].mPane->hide();
                mChar2Bck[17 + i].mPane->show();
                break;
            default:
#line 7260
                JUT_ASSERT(0);
            }
        }
        break;
    case 2:
        for (int i = 0; i < 3; i++)
        {
            switch (mPlayerData[i].mState)
            {
            case 0:
                mChar3Bck[8 + i].mPane->hide();
                mChar3Bck[11 + i].mPane->hide();
                mChar3Bck[14 + i].mPane->hide();
                mChar3Bck[17 + i].mPane->hide();
                mChar3Bck[20 + i].mPane->hide();
                mChar3Bck[23 + i].mPane->hide();
                break;
            case 1:
                mChar3Bck[8 + i].mPane->show();
                mChar3Bck[11 + i].mPane->show();
                mChar3Bck[14 + i].mPane->show();
                mChar3Bck[17 + i].mPane->show();
                mChar3Bck[20 + i].mPane->show();
                mChar3Bck[23 + i].mPane->hide();
                break;
            case 2:
                mChar3Bck[8 + i].mPane->hide();
                mChar3Bck[11 + i].mPane->hide();
                mChar3Bck[14 + i].mPane->hide();
                mChar3Bck[17 + i].mPane->hide();
                mChar3Bck[20 + i].mPane->hide();
                mChar3Bck[23 + i].mPane->show();
                break;
            default:
#line 7293
                JUT_ASSERT(0);
            }
        }
        break;
    case 3:
        for (int i = 0; i < 4; i++)
        {
            switch (mPlayerData[i].mState)
            {
            case 0:
                mChar4Bck[9 + i].mPane->hide();
                mChar4Bck[13 + i].mPane->hide();
                mChar4Bck[17 + i].mPane->hide();
                mChar4Bck[21 + i].mPane->hide();
                break;
            case 1:
                mChar4Bck[9 + i].mPane->show();
                mChar4Bck[13 + i].mPane->show();
                mChar4Bck[17 + i].mPane->show();
                mChar4Bck[21 + i].mPane->hide();
                break;
            case 2:
                mChar4Bck[9 + i].mPane->hide();
                mChar4Bck[13 + i].mPane->hide();
                mChar4Bck[17 + i].mPane->hide();
                mChar4Bck[21 + i].mPane->show();
                break;
            default:
#line 7320
                JUT_ASSERT(0);
            }
        }
        break;
    default:
#line 7326
        JUT_ASSERT(0);
    }

    switch (mInfo.mode)
    {
    case 14:
        for (int i = 0; i < 2; i++)
            mChar2Bck[17 + i].mPane->hide();
        for (int i = 0; i < 3; i++)
            mChar3Bck[23 + i].mPane->hide();
        for (int i = 0; i < 4; i++)
            mChar4Bck[21 + i].mPane->hide();
        break;
    }
}

void SceneMenu::setChar1Plus()
{
    int charF = mPlayerData[0].mCharF;
    int charB = mPlayerData[0].mCharB;
    int kart = mPlayerData[0].mKart;
    bool busy = mPlayerData[0].mState == 2;
    f32 dec = -1.0f;

    if (charF >= 0 && charF < 20 && !busy)
    {
        mChar1PlusBck[0].mPane->show();
        
        f32 f1 = dec + mChar1PlusBck[0].mFrame;
        mChar1PlusBck[0].mFrame = f1;

        if (f1 > 9.0f)

            mChar1PlusBck[0].mFrame = 9.0f;
        
        f32 f2 = dec + mChar1PlusBrk[0].mFrame;
        mChar1PlusBrk[0].mFrame = f2;

        if (f2 > 9.0f)

            mChar1PlusBrk[0].mFrame = 9.0f;

        J2DPane *pane = mChar1PlusBck[0].mPane;
#line 7362
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        static_cast<J2DPicture *>(pane)->changeTexture(mCharName[charF], 0);

        mChar1PlusBck[1].mPane->show();
        
        f32 f3 = dec + mChar1PlusBck[1].mFrame;
        mChar1PlusBck[1].mFrame = f3;

        if (f3 > 9.0f)

            mChar1PlusBck[1].mFrame = 9.0f;
        
        f32 f4 = dec + mChar1PlusBpk.mFrame;
        mChar1PlusBpk.mFrame = f4;

        if (f4 > 9.0f)

            mChar1PlusBpk.mFrame = 9.0f;
    }
    else
    {
        mChar1PlusBck[0].mPane->hide();
        mChar1PlusBck[0].mFrame = -1.0f;
        mChar1PlusBrk[0].mFrame = -1.0f;
        mChar1PlusBck[1].mPane->hide();
        mChar1PlusBck[1].mFrame = -1.0f;
        mChar1PlusBpk.mFrame = -1.0f;
    }

    if (charB >= 0 && charB < 20 && !busy)
    {
        mChar1PlusBck[2].mPane->show();
        
        f32 f5 = dec + mChar1PlusBck[2].mFrame;
        mChar1PlusBck[2].mFrame = f5;

        if (f5 > 9.0f)

            mChar1PlusBck[2].mFrame = 9.0f;
        
        f32 f6 = dec + mChar1PlusBrk[1].mFrame;
        mChar1PlusBrk[1].mFrame = f6;

        if (f6 > 9.0f)

            mChar1PlusBrk[1].mFrame = 9.0f;

        J2DPane *pane = mChar1PlusBck[2].mPane;
#line 7384
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        static_cast<J2DPicture *>(pane)->changeTexture(mCharName[charB], 0);
    }
    else
    {
        mChar1PlusBck[2].mPane->hide();
        mChar1PlusBck[2].mFrame = -1.0f;
        mChar1PlusBrk[1].mFrame = -1.0f;
    }

    if (kart >= 0 && kart < 21 && !busy)
    {
        const s8 *p0 = (const s8 *)mKartPerf + kart * 3;
        mChar1PlusBck[3].mPane->show();
        const s8 *p1 = p0 + 1;
        const s8 *p2 = p0 + 2;
        for (int i = 0; i < 5; i++)
        {
            if (i < p0[0])
                mChar1PlusBck[4 + i].mPane->show();
            else
                mChar1PlusBck[4 + i].mPane->hide();
            if (i < p1[0])
                mChar1PlusBck[9 + i].mPane->show();
            else
                mChar1PlusBck[9 + i].mPane->hide();
            if (i < p2[0])
                mChar1PlusBck[14 + i].mPane->show();
            else
                mChar1PlusBck[14 + i].mPane->hide();
        }
    }
    else
    {
        mChar1PlusBck[3].mPane->hide();
    }
}

void SceneMenu::setRaceData() {}

void SceneMenu::setCharSelFlag()
{
    for (int i = 0; i < 20; i++)
    {
        mCharSelFlag[i] = true;
    }
    if (!gSystemRecord.isGameUnlocked(SystemRecord::KinopioCombi))
    {
        mCharSelFlag[8] = false;
        mCharSelFlag[9] = false;
    }
    if (gSystemRecord.isGameUnlocked(SystemRecord::BossCombi))
        return;
    mCharSelFlag[18] = false;
    mCharSelFlag[19] = false;
}

void SceneMenu::useCharSel(int charF, int charB)
{
    mCharSelFlag[charF] = false;
    mCharSelFlag[charB] = false;
}

void SceneMenu::getCharSel(int &charF, int &charB)
{
    int num = 0;
    for (int i = 0; i < 20; i++)
    {
        if (mCharSelFlag[i] != 0)
            num++;
    }
    if (num < 2)
    {
#line 7746
        JUT_ASSERT(0);
    }

    gSequenceInfo.setRandomSeed();
    int no1 = gSequenceInfo.getRandom() % num-- + 1;
    int no2 = gSequenceInfo.getRandom() % num + 1;

    switch (gRaceInfo.getRaceLevel())
    {
    case LVL_50CC:
    case LVL_100CC:
    {
        int cnt = 0;
        for (int no = 0; no < 20; no++)
        {
            if (mCharSelFlag[no] != 0)
            {
                cnt++;
                if (cnt == no1)
                {
                    charF = no;
                    mCharSelFlag[no] = 0;
                    break;
                }
            }
        }
        int pair = charF % 2 ? charF - 1 : charF + 1;
        if (mCharSelFlag[pair] != 0)
        {
            charB = pair;
            mCharSelFlag[pair] = 0;
            break;
        }
        int num = 0;
        for (int no = 0; no < 10; no++)
        {
            if ((mCharSelFlag[no * 2] != 0 && mCharSelFlag[no * 2 + 1] == 0) || (mCharSelFlag[no * 2] == 0 && mCharSelFlag[no * 2 + 1] != 0))
                num++;
        }

        if (num == 0)
        {
#line 7790
            JUT_ASSERT(0);
        }

        int pairSel = gSequenceInfo.getRandom() % num + 1;
        int n = 0;
        for (int no = 0; no < 10; no++)
        {
            if ((mCharSelFlag[no * 2] != 0 && mCharSelFlag[no * 2 + 1] == 0) || (mCharSelFlag[no * 2] == 0 && mCharSelFlag[no * 2 + 1] != 0))
            {
                n++;
                if (n == pairSel)
                {
                    if (mCharSelFlag[no * 2] != 0)
                    {
                        charB = no * 2;
                        mCharSelFlag[no * 2] = 0;
                        break;
                    }
                    if (mCharSelFlag[no * 2 + 1] != 0)
                    {
                        charB = no * 2 + 1;
                        mCharSelFlag[no * 2 + 1] = 0;
                        break;
                    }
                }
            }
        }
        break;
    }
    case LVL_150CC:
    case LVL_MIRROR:
    {
        int cnt = 0;
        for (int no = 0; no < 20; no++)
        {
            if (mCharSelFlag[no] != 0)
            {
                cnt++;
                if (cnt == no1)
                {
                    charF = no;
                    mCharSelFlag[no] = 0;
                    break;
                }
            }
        }
        cnt = 0;
        for (int no = 0; no < 20; no++)
        {
            if (mCharSelFlag[no] != 0)
            {
                cnt++;
                if (cnt == no2)
                {
                    charB = no;
                    mCharSelFlag[no] = 0;
                    break;
                }
            }
        }
        break;
    }
    default:
#line 7836
        JUT_ASSERT(0);
    }
}

void SceneMenu::getRndCharSel(int &charF, int &charB)
{
    int num = 0;
    for (int i = 0; i < 20; i++)
    {
        if (mCharSelFlag[i] != 0)
            num++;
    }
    if (num < 2)
    {
#line 7858
        JUT_ASSERT(0);
    }

    gSequenceInfo.setRandomSeed();
    int no1 = gSequenceInfo.getRandom() % num-- + 1;
    int no2 = gSequenceInfo.getRandom() % num + 1;

    int cnt = 0;
    for (int no = 0; no < 20; no++)
    {
        if (mCharSelFlag[no] != 0)
        {
            cnt++;
            if (cnt == no1)
            {
                charF = no;
                mCharSelFlag[no] = 0;
                break;
            }
        }
    }
    cnt = 0;
    for (int no = 0; no < 20; no++)
    {
        if (mCharSelFlag[no] != 0)
        {
            cnt++;
            if (cnt == no2)
            {
                charB = no;
                mCharSelFlag[no] = 0;
                break;
            }
        }
    }
}

void SceneMenu::changeText()
{
    if (mInfo.entryNum + 1 <= 2)
        ReadPrintMessage::mNum[0] = KartChecker::sBombPointFullS;
    else
        ReadPrintMessage::mNum[0] = KartChecker::sBombPointFullL;

    int messageId = ReadPrintMessage::getBmgMessageID(mMsgId);
    char *message = ReadPrintMessage::getMessage(mMsgRes, messageId);

    mTextBox->setFontSize(
        mFontSize.x * ReadPrintMessage::getRate(mMsgRes, messageId) / 100.0f,
        mFontSize.y * ReadPrintMessage::getRate(mMsgRes, messageId) / 100.0f);

    mTextBox->setCharSpace(ReadPrintMessage::getCharSpace(mMsgRes, messageId));

    J2DTextBox::TFontSize fontSize;
    mTextBox->getFontSize(fontSize);
    char buf[512];
    ReadPrintMessage::tagCnv(message, mColorRes, fontSize, sizeof(buf), buf);
    strncpy(mTextBox->getStringPtr(), buf, sizeof(buf));
}

#include "JSystem/JAudio/JASFakeMatch2.h"
