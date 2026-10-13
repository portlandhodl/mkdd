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
#include "Kaneshige/RaceInfo.h"
#include "Kaneshige/KartInfo.h"
#include "Inagaki/GameAudioMain.h"
#include "Sato/JPEffectMgr.h"
#include "Osako/MenuBackground.h"
#include "Osako/MenuTitleLine.h"
#include "Osako/NameEntryHelper.h"
#include "Osako/SequenceApp.h"
#include "Osako/SystemRecord.h"
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
    /*
    J2DGrafContext::scissor(float, float, float, float) {}
    J2DTextBox::setFontColor(JUtility::TColor, JUtility::TColor) {}
    J2DTextBox::getFontSize(J2DTextBox::TFontSize &) const {}
    Kart2DCommon::getCharIconTexture(ECharID) {}
    MenuTitleLine::create(JKRArchive *, JKRHeap *) {}
    MenuBackground::create(JKRArchive *) {}
    CreateJPAMgr(JKRHeap *, bool) {}
    CharacterSelect3D::create(JKRHeap *) {}
    */
}

void SceneMenu::reset() {}

void SceneMenu::init() {}

SceneMenu::~SceneMenu() {}

void SceneMenu::draw()
{
    mOrtho->setPort();
    // the MenuBackground's J2DScreen is embedded at offset 0
    ((J2DScreen *)MenuBackground::ptr())->draw(0.0f, 0.0f, mOrtho);

    switch (mInfo.mode)
    {
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
    default:
        break;
    }

    if (mInfo.mode >= 12 && mInfo.mode < 15)
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
            mScreenChar1Plus->draw(0.0f, 0.0f, mOrtho);
            break;
        case 2:
            mScreenChar2->draw(0.0f, 0.0f, mOrtho);
            break;
        case 3:
            mScreenChar3->draw(0.0f, 0.0f, mOrtho);
            break;
        default:
#line 1938
            JUT_ASSERT(0);
        }
    }

    mScreenPlayerIcon->draw(0.0f, 0.0f, mOrtho);
    MenuTitleLine::ptr()->draw(mOrtho);

    if (mInfo.mode >= 12 && mInfo.mode < 15)
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

    j3dSys.reinitGX();
    JPEffectMgr::mThis->draw_for2D();
    mOrtho->setPort();

    if (mInfo.mode == 13)
    {
        if (mInfo.entryNum == 0)
        {
            mScreenChar1Plus->draw(0.0f, 0.0f, mOrtho);  // hmm wait: 0x2004
        }
        else if (mInfo.entryNum < 0 || mInfo.entryNum >= 4)
        {
#line 2038
            JUT_ASSERT(0);
        }
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
    else if (data->mFrame >= 28.0f)
    {
        data->mFrame = 20.0f;
    }
    else
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
}

void SceneMenu::calcEnginePic(SceneMenu::BckData *data)
{
    if (data->mFrame <= 20.0f)
    {
        data->mFrame = 20.0f;
    }
    else if (data->mFrame >= 28.0f)
    {
        data->mFrame = 20.0f;
    }
    else
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

void SceneMenu::calcModeOrder() {}

void SceneMenu::entry_init() {}

void SceneMenu::entry() {}

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

void SceneMenu::vs_init() {}

void SceneMenu::vs() {}

void SceneMenu::vs_end() {}

void SceneMenu::mode_init() {}

void SceneMenu::mode() {}

void SceneMenu::mode_end() {}

void SceneMenu::battle_init() {}

void SceneMenu::battle() {}

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

void SceneMenu::character() {}

void SceneMenu::character_end() {}

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
            mCharPane[cursor / 2][cursor % 2].mFrame = 19.0f;
            mSelectAnm[cursor / 2][cursor % 2].mEnable = 1;
            mSelectAnm[cursor / 2][cursor % 2].mNo = no;
        }
        else if (mPlayerData[padNo].mCharNoB == no && mPlayerData[padNo].mCharB == -1)
        {
            mPlayerData[padNo].mCharB = cursor;
            mCharPane[cursor / 2][cursor % 2].mFrame = 19.0f;
            mSelectAnm[cursor / 2][cursor % 2].mEnable = 1;
            mSelectAnm[cursor / 2][cursor % 2].mNo = no;
        }
        break;
    }
    case 1:
    {
        int charF = mPlayerData[padNo].mCharF;
        int charB = mPlayerData[padNo].mCharB;
        if (charF >= 0 && charF < 20)
        {
            GameAudio::Main::getAudio()->startSystemSe(mCharGoVoice[charF]);
        }
        if (charB >= 0 && charB < 20)
        {
            GameAudio::Main::getAudio()->startSystemSe(mCharGoVoice[charB]);
        }
        mPlayerData[padNo].mState = 2;
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

    int padNo = mPlayerPadNo[no];
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
        // NOTE: CharacterSelect3D::isCancel is (currently) stubbed as void in its
        // shared header; original code branched on its boolean result here.
        CharacterSelect3D::ptr()->isCancel(padNo);
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

    int padNo = mPlayerPadNo[no];
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

    int padNo = mPlayerPadNo[no];
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
        mCharPane[charF / 2][charF % 2].mFrame = 19.0f;
        mSelectAnm[charF / 2][charF % 2].mEnable = 1;
        mSelectAnm[charF / 2][charF % 2].mNo = mPlayerData[i].mCharNoF;
        mPlayerData[i].mCharB = charB;
        mCharPane[charB / 2][charB % 2].mFrame = 19.0f;
        mSelectAnm[charB / 2][charB % 2].mEnable = 1;
        mSelectAnm[charB / 2][charB % 2].mNo = mPlayerData[i].mCharNoB;
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

void SceneMenu::characterAnm() {}

void SceneMenu::characterOkWaitAnm() {}

void SceneMenu::setChar1Plus() {}

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
        num = 0;
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

void SceneMenu::changeText() {}

#include "JSystem/JAudio/JASFakeMatch2.h"
