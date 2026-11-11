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
#include "JSystem/J2D/J2DAnmLoader.h"
#include "JSystem/JKernel/JKRFileLoader.h"
#include "Kameda/BattleName2D.h"
#include "Kameda/SceneFactory.h"
#include "Kaneshige/SysDebug.h"
#include "Osako/CardAgent.h"
#include "Osako/FontMgr.h"
#include "Osako/ResMgr.h"

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

const char *SceneMenu::mCharCCName[2][4] = {
    {
        "SC_P1_u.bti",
        "SC_P2_u.bti",
        "SC_P3_u.bti",
        "SC_P4_u.bti",
    },
    {
        "SC_P1_s.bti",
        "SC_P2_s.bti",
        "SC_P3_s.bti",
        "SC_P4_s.bti",
    },
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

SceneMenu::SceneMenu(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap)
{
    // The split load statements grow the ctor past MWCC's inline budget, which keeps the
    // small creator/accessor inlines (create, getFontSize, ...) out-of-line as in the target.
    void *res;
    J2DAnmBase *anm;
    mScreenPlayerIcon = new (heap, 0) J2DScreen();
    mScreenHowManyDrivers = new (heap, 0) J2DScreen();
    mScreenSingleTagSelect = new (heap, 0) J2DScreen();
    mScreenSelectMode = new (heap, 0) J2DScreen();
    mScreenSelectBattle = new (heap, 0) J2DScreen();
    mScreenCharLayout = new (heap, 0) J2DScreen();
    for (int i = 0; i < 10; i++)
        mCharScreens[i] = new (heap, 0) J2DScreen();
    mScreenChar1 = new (heap, 0) J2DScreen();
    mScreenChar1Plus = new (heap, 0) J2DScreen();
    mScreenChar2 = new (heap, 0) J2DScreen();
    mScreenChar3 = new (heap, 0) J2DScreen();
    mScreenChar4 = new (heap, 0) J2DScreen();

    SysDebug::getManager()->setHeapGroup("CharacterSelect3D", heap);
    mCharacterSelect3D = CharacterSelect3D::create(heap);
    SysDebug::getManager()->setHeapGroup("Menu", heap);
    CreateJPAMgr(heap, true);

    MenuBackground::create(SceneFactory::ptr()->getArchive(9));
    MenuTitleLine::create(SceneFactory::ptr()->getArchive(8), nullptr);

    mScreenPlayerIcon->set("PlayerIcon.blo", 0x1040000, mArchive);
    mScreenHowManyDrivers->set("HowManyDrivers.blo", 0x1040000, mArchive);
    mScreenSingleTagSelect->set("SingleTagSelect.blo", 0x1040000, mArchive);
    mScreenSelectMode->set("SelectMode.blo", 0x1040000, mArchive);
    mScreenSelectBattle->set("SelectBattle.blo", 0x1040000, mArchive);
    mScreenCharLayout->set("select_character_cc_layout.blo", 0x1040000, mArchive);
    for (int i = 0; i < 10; i++)
        mCharScreens[i]->set("select_character_cc.blo", 0x1040000, mArchive);
    mScreenChar1->set("select_character1.blo", 0x1040000, mArchive);
    mScreenChar1Plus->set("select_character_plus1.blo", 0x1040000, mArchive);
    mScreenChar2->set("select_character2.blo", 0x1040000, mArchive);
    mScreenChar3->set("select_character3.blo", 0x1040000, mArchive);
    mScreenChar4->set("select_character4.blo", 0x1040000, mArchive);

    SequenceApp::ptr()->ready(Scene::SCENE_TITLE);
    SequenceApp::ptr()->ready(Scene::SCENE_COURSE);
    SequenceApp::ptr()->ready(Scene::SCENE_MAP_SELECT);
    SequenceApp::ptr()->ready(Scene::SCENE_GHOST_LOAD_SAVE);

    for (int i = 0; i < 33; i++)
    {
        mIconBck[i].mPane = mScreenPlayerIcon->search(mPlayerIconBckTag[i]);
        res = JKRFileLoader::getGlbResource("PlayerIcon.bck", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mIconBck[i].mAnm = anm;
        mIconBck[i].mPane->setAnimation((J2DAnmTransform *)mIconBck[i].mAnm);
        mIconBck[i].mPane->hide();
    }

    for (int i = 0; i < 13; i++)
    {
        mHowManyDriversBck[i].mPane = mScreenHowManyDrivers->search(mHowManyDriversBckTag[i]);
        res = JKRFileLoader::getGlbResource("HowManyDrivers.bck", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mHowManyDriversBck[i].mAnm = anm;
        mHowManyDriversBck[i].mPane->setAnimation((J2DAnmTransform *)mHowManyDriversBck[i].mAnm);
    }
    res = JKRFileLoader::getGlbResource("HowManyDrivers.btk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mHowManyDriversEnd.mAnm = anm;
    mHowManyDriversEnd.mAnm->searchUpdateMaterialID(mScreenHowManyDrivers);
    mScreenHowManyDrivers->setAnimation((J2DAnmTextureSRTKey *)mHowManyDriversEnd.mAnm);
    for (int i = 0; i < 22; i++)
    {
        mHowManyDriversBrk[i].mPane = mScreenHowManyDrivers->search(mHowManyDriversBrkTag[i]);
        res = JKRFileLoader::getGlbResource("HowManyDrivers.brk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mHowManyDriversBrk[i].mAnm = anm;
        mHowManyDriversBrk[i].mAnm->searchUpdateMaterialID(mScreenHowManyDrivers);
        mHowManyDriversBrk[i].mPane->setAnimation((J2DAnmTevRegKey *)mHowManyDriversBrk[i].mAnm);
    }
    res = JKRFileLoader::getGlbResource("HowManyDrivers.bpk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mSingleTagSelectEndBck.mAnm = anm;
    mSingleTagSelectEndBck.mAnm->searchUpdateMaterialID(mScreenHowManyDrivers);
    mScreenHowManyDrivers->setAnimation((J2DAnmColor *)mSingleTagSelectEndBck.mAnm);

    for (int i = 0; i < 43; i++)
    {
        mSingleTagSelectBck[i].mPane = mScreenSingleTagSelect->search(mSingleTagSelectBckTag[i]);
        res = JKRFileLoader::getGlbResource("SingleTagSelect.bck", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mSingleTagSelectBck[i].mAnm = anm;
        mSingleTagSelectBck[i].mPane->setAnimation((J2DAnmTransform *)mSingleTagSelectBck[i].mAnm);
    }
    for (int i = 0; i < 24; i++)
    {
        mSingleTagSelectBtk[i].mPane = mScreenSingleTagSelect->search(mSingleTagSelectBtkTag[i]);
        res = JKRFileLoader::getGlbResource("SingleTagSelect.btk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mSingleTagSelectBtk[i].mAnm = anm;
        mSingleTagSelectBtk[i].mAnm->searchUpdateMaterialID(mScreenSingleTagSelect);
        mSingleTagSelectBtk[i].mPane->setAnimation((J2DAnmTextureSRTKey *)mSingleTagSelectBtk[i].mAnm);
    }
    for (int i = 0; i < 78; i++)
    {
        mSingleTagSelectBrk[i].mPane = mScreenSingleTagSelect->search(mSingleTagSelectBrkTag[i]);
        res = JKRFileLoader::getGlbResource("SingleTagSelect.brk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mSingleTagSelectBrk[i].mAnm = anm;
        mSingleTagSelectBrk[i].mAnm->searchUpdateMaterialID(mScreenSingleTagSelect);
        mSingleTagSelectBrk[i].mPane->setAnimation((J2DAnmTevRegKey *)mSingleTagSelectBrk[i].mAnm);
    }
    for (int i = 0; i < 7; i++)
    {
        mSingleTagSelectBpk[i].mPane = mScreenSingleTagSelect->search(mSingleTagSelectBpkTag[i]);
        res = JKRFileLoader::getGlbResource("SingleTagSelect.bpk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mSingleTagSelectBpk[i].mAnm = anm;
        mSingleTagSelectBpk[i].mAnm->searchUpdateMaterialID(mScreenSingleTagSelect);
        mSingleTagSelectBpk[i].mPane->setAnimation((J2DAnmColor *)mSingleTagSelectBpk[i].mAnm);
    }

    for (int i = 0; i < 75; i++)
    {
        mModeBck[i].mPane = mScreenSelectMode->search(mModeBckTag[i]);
        res = JKRFileLoader::getGlbResource("SelectMode.bck", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mModeBck[i].mAnm = anm;
        mModeBck[i].mPane->setAnimation((J2DAnmTransform *)mModeBck[i].mAnm);
    }
    for (int i = 0; i < 26; i++)
    {
        mModeBtk[i].mPane = mScreenSelectMode->search(mModeBtkTag[i]);
        res = JKRFileLoader::getGlbResource("SelectMode.btk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mModeBtk[i].mAnm = anm;
        mModeBtk[i].mAnm->searchUpdateMaterialID(mScreenSelectMode);
        mModeBtk[i].mPane->setAnimation((J2DAnmTextureSRTKey *)mModeBtk[i].mAnm);
    }
    for (int i = 0; i < 30; i++)
    {
        mModeBrk[i].mPane = mScreenSelectMode->search(mModeBrkTag[i]);
        res = JKRFileLoader::getGlbResource("SelectMode.brk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mModeBrk[i].mAnm = anm;
        mModeBrk[i].mAnm->searchUpdateMaterialID(mScreenSelectMode);
        mModeBrk[i].mPane->setAnimation((J2DAnmTevRegKey *)mModeBrk[i].mAnm);
    }
    for (int i = 0; i < 6; i++)
    {
        mModeBpk[i].mPane = mScreenSelectMode->search(mModeBpkTag[i]);
        res = JKRFileLoader::getGlbResource("SelectMode.bpk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mModeBpk[i].mAnm = anm;
        mModeBpk[i].mAnm->searchUpdateMaterialID(mScreenSelectMode);
        mModeBpk[i].mPane->setAnimation((J2DAnmColor *)mModeBpk[i].mAnm);
    }

    for (int i = 0; i < 15; i++)
    {
        mBattleBck[i].mPane = mScreenSelectBattle->search(mBattleBckTag[i]);
        res = JKRFileLoader::getGlbResource("SelectBattle.bck", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mBattleBck[i].mAnm = anm;
        mBattleBck[i].mPane->setAnimation((J2DAnmTransform *)mBattleBck[i].mAnm);
        mBattleBck[i].mPane->hide();
    }
    res = JKRFileLoader::getGlbResource("SelectBattle.bpk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mBattleBrk.mAnm = anm;
    mBattleBrk.mAnm->searchUpdateMaterialID(mScreenSelectBattle);
    mScreenSelectBattle->setAnimation((J2DAnmColor *)mBattleBrk.mAnm);

    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            mCharPane[i][j].mPane = mCharScreens[i]->search(mCharCCBckTag[j]);
            res = JKRFileLoader::getGlbResource("select_character_cc.bck", mArchive);
            anm = J2DAnmLoaderDataBase::load(res);
            mCharPane[i][j].mAnm = anm;
            mCharPane[i][j].mPane->setAnimation((J2DAnmTransform *)mCharPane[i][j].mAnm);
            mCharPane[i][j].mPane->hide();
        }
        res = JKRFileLoader::getGlbResource("select_character_cc.btk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mCharCCBck[i].mAnm = anm;
        mCharCCBck[i].mAnm->searchUpdateMaterialID(mCharScreens[i]);
        mCharScreens[i]->setAnimation((J2DAnmTextureSRTKey *)mCharCCBck[i].mAnm);
        for (int j = 0; j < 4; j++)
        {
            mCharCCBrk[i][j].mPane = mCharScreens[i]->search(mCharCCBrkTag[j]);
            res = JKRFileLoader::getGlbResource("select_character_cc.brk", mArchive);
            anm = J2DAnmLoaderDataBase::load(res);
            mCharCCBrk[i][j].mAnm = anm;
            mCharCCBrk[i][j].mAnm->searchUpdateMaterialID(mCharScreens[i]);
            mCharCCBrk[i][j].mPane->setAnimation((J2DAnmTevRegKey *)mCharCCBrk[i][j].mAnm);
        }
        for (int j = 0; j < 6; j++)
        {
            mCharCCBpk[i][j].mPane = mCharScreens[i]->search(mCharCCBpkTag[j]);
            res = JKRFileLoader::getGlbResource("select_character_cc.bpk", mArchive);
            anm = J2DAnmLoaderDataBase::load(res);
            mCharCCBpk[i][j].mAnm = anm;
            mCharCCBpk[i][j].mAnm->searchUpdateMaterialID(mCharScreens[i]);
            mCharCCBpk[i][j].mPane->setAnimation((J2DAnmColor *)mCharCCBpk[i][j].mAnm);
        }
    }

    for (int i = 0; i < 9; i++)
    {
        mCharPaneBck[i].mPane = mScreenChar1->search(mChar1BckTag[i]);
        res = JKRFileLoader::getGlbResource("select_character1.bck", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mCharPaneBck[i].mAnm = anm;
        mCharPaneBck[i].mPane->setAnimation((J2DAnmTransform *)mCharPaneBck[i].mAnm);
        mCharPaneBck[i].mPane->hide();
    }
    res = JKRFileLoader::getGlbResource("select_character1.btk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mCharCCEndBck.mAnm = anm;
    mCharCCEndBck.mAnm->searchUpdateMaterialID(mScreenChar1);
    mScreenChar1->setAnimation((J2DAnmTextureSRTKey *)mCharCCEndBck.mAnm);
    res = JKRFileLoader::getGlbResource("select_character1.bpk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mCharCCEndBrk.mAnm = anm;
    mCharCCEndBrk.mAnm->searchUpdateMaterialID(mScreenChar1);
    mScreenChar1->setAnimation((J2DAnmColor *)mCharCCEndBrk.mAnm);

    for (int i = 0; i < 19; i++)
    {
        mChar1PlusBck[i].mPane = mScreenChar1Plus->search(mChar1PlusBckTag[i]);
        res = JKRFileLoader::getGlbResource("select_character_plus1.bck", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mChar1PlusBck[i].mAnm = anm;
        mChar1PlusBck[i].mPane->setAnimation((J2DAnmTransform *)mChar1PlusBck[i].mAnm);
    }
    for (int i = 0; i < 2; i++)
    {
        mChar1PlusBrk[i].mPane = mScreenChar1Plus->search(mChar1PlusBrkTag[i]);
        res = JKRFileLoader::getGlbResource("select_character_plus1.brk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mChar1PlusBrk[i].mAnm = anm;
        mChar1PlusBrk[i].mAnm->searchUpdateMaterialID(mScreenChar1Plus);
        mChar1PlusBrk[i].mPane->setAnimation((J2DAnmTevRegKey *)mChar1PlusBrk[i].mAnm);
    }
    res = JKRFileLoader::getGlbResource("select_character_plus1.bpk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mChar1PlusBpk.mAnm = anm;
    mChar1PlusBpk.mAnm->searchUpdateMaterialID(mScreenChar1Plus);
    mScreenChar1Plus->setAnimation((J2DAnmColor *)mChar1PlusBpk.mAnm);

    for (int i = 0; i < 20; i++)
    {
        mChar2Bck[i].mPane = mScreenChar2->search(mChar2BckTag[i]);
        res = JKRFileLoader::getGlbResource("select_character2.bck", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mChar2Bck[i].mAnm = anm;
        mChar2Bck[i].mPane->setAnimation((J2DAnmTransform *)mChar2Bck[i].mAnm);
        mChar2Bck[i].mPane->hide();
    }
    res = JKRFileLoader::getGlbResource("select_character2.btk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mChar2EndBck.mAnm = anm;
    mChar2EndBck.mAnm->searchUpdateMaterialID(mScreenChar2);
    mScreenChar2->setAnimation((J2DAnmTextureSRTKey *)mChar2EndBck.mAnm);
    for (int i = 0; i < 5; i++)
    {
        mChar2Brk[i].mPane = mScreenChar2->search(mChar2BrkTag[i]);
        res = JKRFileLoader::getGlbResource("select_character2.brk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mChar2Brk[i].mAnm = anm;
        mChar2Brk[i].mAnm->searchUpdateMaterialID(mScreenChar2);
        mChar2Brk[i].mPane->setAnimation((J2DAnmTevRegKey *)mChar2Brk[i].mAnm);
    }
    res = JKRFileLoader::getGlbResource("select_character2.bpk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mChar2EndBrk.mAnm = anm;
    mChar2EndBrk.mAnm->searchUpdateMaterialID(mScreenChar2);
    mScreenChar2->setAnimation((J2DAnmColor *)mChar2EndBrk.mAnm);

    for (int i = 0; i < 27; i++)
    {
        mChar3Bck[i].mPane = mScreenChar3->search(mChar3BckTag[i]);
        res = JKRFileLoader::getGlbResource("select_character3.bck", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mChar3Bck[i].mAnm = anm;
        mChar3Bck[i].mPane->setAnimation((J2DAnmTransform *)mChar3Bck[i].mAnm);
        mChar3Bck[i].mPane->hide();
    }
    res = JKRFileLoader::getGlbResource("select_character3.btk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mChar3EndBck.mAnm = anm;
    mChar3EndBck.mAnm->searchUpdateMaterialID(mScreenChar3);
    mScreenChar3->setAnimation((J2DAnmTextureSRTKey *)mChar3EndBck.mAnm);
    for (int i = 0; i < 8; i++)
    {
        mChar3Brk[i].mPane = mScreenChar3->search(mChar3BrkTag[i]);
        res = JKRFileLoader::getGlbResource("select_character3.brk", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mChar3Brk[i].mAnm = anm;
        mChar3Brk[i].mAnm->searchUpdateMaterialID(mScreenChar3);
        mChar3Brk[i].mPane->setAnimation((J2DAnmTevRegKey *)mChar3Brk[i].mAnm);
    }
    res = JKRFileLoader::getGlbResource("select_character3.bpk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mChar3EndBrk.mAnm = anm;
    mChar3EndBrk.mAnm->searchUpdateMaterialID(mScreenChar3);
    mScreenChar3->setAnimation((J2DAnmColor *)mChar3EndBrk.mAnm);

    for (int i = 0; i < 26; i++)
    {
        mChar4Bck[i].mPane = mScreenChar4->search(mChar4BckTag[i]);
        res = JKRFileLoader::getGlbResource("select_character4.bck", mArchive);
        anm = J2DAnmLoaderDataBase::load(res);
        mChar4Bck[i].mAnm = anm;
        mChar4Bck[i].mPane->setAnimation((J2DAnmTransform *)mChar4Bck[i].mAnm);
        mChar4Bck[i].mPane->hide();
    }
    res = JKRFileLoader::getGlbResource("select_character4.btk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mChar4EndBck.mAnm = anm;
    mChar4EndBck.mAnm->searchUpdateMaterialID(mScreenChar4);
    mScreenChar4->setAnimation((J2DAnmTextureSRTKey *)mChar4EndBck.mAnm);
    res = JKRFileLoader::getGlbResource("select_character4.bpk", mArchive);
    anm = J2DAnmLoaderDataBase::load(res);
    mChar4EndBrk.mAnm = anm;
    mChar4EndBrk.mAnm->searchUpdateMaterialID(mScreenChar4);
    mScreenChar4->setAnimation((J2DAnmColor *)mChar4EndBrk.mAnm);

    J2DPicture *pane;
    pane = (J2DPicture *)mSingleTagSelectBrk[2].mPane;
#line 1116
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(MARIO), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[3].mPane;
#line 1119
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(PEACH), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[4].mPane;
#line 1122
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(MARIO), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[5].mPane;
#line 1125
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(PEACH), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[16].mPane;
#line 1129
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(MARIO), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[17].mPane;
#line 1132
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(PEACH), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[18].mPane;
#line 1135
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(YOSHI), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[19].mPane;
#line 1138
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(MARIO), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[20].mPane;
#line 1141
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(PEACH), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[21].mPane;
#line 1144
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(YOSHI), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[39].mPane;
#line 1148
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(MARIO), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[40].mPane;
#line 1151
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(PEACH), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[41].mPane;
#line 1154
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(YOSHI), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[42].mPane;
#line 1157
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(BOWSER), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[43].mPane;
#line 1160
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(MARIO), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[44].mPane;
#line 1163
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(YOSHI), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[45].mPane;
#line 1166
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(PEACH), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[46].mPane;
#line 1169
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(BOWSER), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[47].mPane;
#line 1172
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(MARIO), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[48].mPane;
#line 1175
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(YOSHI), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[49].mPane;
#line 1178
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(PEACH), 0);

    pane = (J2DPicture *)mSingleTagSelectBrk[50].mPane;
#line 1181
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getCharIconTexture(BOWSER), 0);

    pane = (J2DPicture *)mModeBck[12].mPane;
#line 1189
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(0), 0);

    pane = (J2DPicture *)mModeBck[10].mPane;
#line 1192
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(2), 0);

    pane = (J2DPicture *)mModeBck[11].mPane;
#line 1195
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(1), 0);

    pane = (J2DPicture *)mModeBck[56].mPane;
#line 1199
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(0), 0);

    pane = (J2DPicture *)mModeBck[54].mPane;
#line 1202
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(2), 0);

    pane = (J2DPicture *)mModeBck[55].mPane;
#line 1205
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(1), 0);

    pane = (J2DPicture *)mBattleBck[10].mPane;
#line 1213
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(0), 0);

    pane = (J2DPicture *)mBattleBck[11].mPane;
#line 1217
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(2), 0);

    pane = (J2DPicture *)mBattleBck[12].mPane;
#line 1221
    JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
    pane->changeTexture(Kart2DCommon::ptr()->getBattleIconTexture(1), 0);

    {
        J2DTextBox *pane = (J2DTextBox *)mScreenSelectBattle->search('TBtl');
#line 1227
        JUT_ASSERT(pane->getTypeID() == J2DPane_TextBox);
        mTextBox = pane;
    }
    mTextBox->setFont(FontMgr::mspResFont);
    mTextBox->getFontSize(mFontSize);
    mColorRes = ResMgr::getPtr(ResMgr::mcColor);
    mMsgRes = ResMgr::getPtr(ResMgr::mcBattle);
    JUtility::TColor color = ReadPrintMessage::getColor(mColorRes, ReadPrintMessage::getDefaultColor(mMsgRes));
    mTextBox->setFontColor(color, color);

    char buffer[512];
    for (int i = 0; i < 512; i++)
        buffer[i] = ' ';
    buffer[511] = '\0';
    mTextBox->setString(buffer);

    for (int i = 0; i < 3; i++)
    {
        J2DPicture *pane = (J2DPicture *)mBattleBck[7 + i].mPane;
#line 1251
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        pane->changeTexture(BattleName2D::ptr()->getBattleNameTexture((BattleName2D::BattleName)i), 0);
    }

    for (int i = 0; i < 2; i++)
    {
        J2DPicture *pane = (J2DPicture *)mScreenSelectBattle->search('CCBar1' + i);
#line 1258
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mBarWhite[i] = pane->getWhite();
        mBarBlack[i] = pane->getBlack();
    }
    for (int i = 0; i < 2; i++)
    {
        J2DPicture *pane = (J2DPicture *)mScreenSelectBattle->search('CCWord1' + i);
#line 1265
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mWordWhite[i] = pane->getWhite();
    }
    for (int i = 0; i < 2; i++)
    {
        J2DPicture *pane = (J2DPicture *)mScreenSelectBattle->search('CCBtlP1' + i);
#line 1271
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        mBtlPWhite[i] = pane->getWhite();
    }

    f32 vpW = System::get2DVpW();
    f32 vpH = System::get2DVpH();
    f32 cy = vpH / 2.0f;
    f32 top = cy - 124.0f;
    f32 bottom = 124.0f + cy;
    mCharRect[0].x = 0.0f;
    mCharRect[0].y = top;
    mCharRect[0].w = vpW;
    mCharRect[0].h = bottom;

    JGeometry::TBox2f box21(*mChar2Bck[5].mPane->getBounds());
    JGeometry::TBox2f box22(*mChar2Bck[6].mPane->getBounds());
    f32 x22 = (box22.i.x + box22.f.x) / 2.0f;
    mCharRect[1].x = 0.0f;
    mCharRect[1].y = top;
    mCharRect[1].w = 480.0f;
    mCharRect[1].h = bottom;
    mCharRect[2].x = x22 - 240.0f;
    mCharRect[2].y = top;
    mCharRect[2].w = 480.0f;
    mCharRect[2].h = bottom;

    JGeometry::TBox2f box31(*mChar3Bck[5].mPane->getBounds());
    JGeometry::TBox2f box32(*mChar3Bck[6].mPane->getBounds());
    f32 x32 = (box32.i.x + box32.f.x) / 2.0f;
    JGeometry::TBox2f box33(*mChar3Bck[7].mPane->getBounds());
    f32 x33 = (box33.i.x + box33.f.x) / 2.0f;
    mCharRect[3].x = 0.0f;
    mCharRect[3].y = top;
    mCharRect[3].w = 320.0f;
    mCharRect[3].h = bottom;
    mCharRect[4].x = x32 - 160.0f;
    mCharRect[4].y = top;
    mCharRect[4].w = 320.0f;
    mCharRect[4].h = bottom;
    mCharRect[5].x = x33 - 160.0f;
    mCharRect[5].y = top;
    mCharRect[5].w = 320.0f;
    mCharRect[5].h = bottom;

    JGeometry::TBox2f box41(*mChar4Bck[5].mPane->getBounds());
    JGeometry::TBox2f box42(*mChar4Bck[6].mPane->getBounds());
    f32 x42 = (box42.i.x + box42.f.x) / 2.0f;
    JGeometry::TBox2f box43(*mChar4Bck[7].mPane->getBounds());
    f32 x43 = (box43.i.x + box43.f.x) / 2.0f;
    JGeometry::TBox2f box44(*mChar4Bck[8].mPane->getBounds());
    f32 x44 = (box44.i.x + box44.f.x) / 2.0f;
    mCharRect[6].x = 0.0f;
    mCharRect[6].y = top;
    mCharRect[6].w = 230.0f;
    mCharRect[6].h = bottom;
    mCharRect[7].x = x42 - 115.0f;
    mCharRect[7].y = top;
    mCharRect[7].w = 230.0f;
    mCharRect[7].h = bottom;
    mCharRect[8].x = x43 - 115.0f;
    mCharRect[8].y = top;
    mCharRect[8].w = 230.0f;
    mCharRect[8].h = bottom;
    mCharRect[9].x = x44 - 115.0f;
    mCharRect[9].y = top;
    mCharRect[9].w = 230.0f;
    mCharRect[9].h = bottom;

    for (int i = 0; i < 10; i++)
    {
        mGrafs[i] = new (heap, 0) J2DOrthoGraph(mCharRect[i].x, mCharRect[i].y, mCharRect[i].w, mCharRect[i].h, -1.0f, 1.0f);
        mGrafs[i]->scissor(mCharRect[i].x, mCharRect[i].y, mCharRect[i].w, mCharRect[i].h);
    }

    strncpy(mName, gSystemRecord.mDefaultName, 4);
    J2DPane *namePanes[3];
    for (int i = 0; i < 3; i++)
        namePanes[i] = mModeBck[71 + i].mPane;
    mNameEntry = new (heap, 0) NameEntryHelper(namePanes, mName, heap, gpaGamePad[0], true);
    reset();
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
    System::getDisplay()->startFadeIn(15);

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
    case GRAND_PRIX:
        mInfo._20 = 0;
        break;
    case TIME_ATTACK:
        mInfo._20 = 2;
        break;
    case VERSUS_RACE:
        mInfo._20 = 1;
        break;
    case BALLOON_BATTLE:
        mInfo._20 = 3;
        mMsgId = 0;
        break;
    case BOMB_BATTLE:
        mInfo._20 = 3;
        mMsgId = 2;
        break;
    case ESCAPE_BATTLE:
        mInfo._20 = 3;
        mMsgId = 1;
        break;
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
        case 3:
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
        switch (mInfo.playerNum)
        {
        case 3:
            mInfo._18 = 0;
            break;
        default:
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

        for (int i = 0; i < gRaceInfo.mStatusNum; i++)
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

        for (int i = 0; i < gRaceInfo.mStatusNum; i++)
        {
            mPlayerData[i].mCursor = -1;
            if (mPlayerData[i].mCharF != -1 && mPlayerData[i].mCharB != -1)
            {
                mPlayerData[i].mState = 1;
                EKartID kartId = gRaceInfo.getKartInfo(i)->getKartID();
                int kartNo;
                for (kartNo = 0; kartNo < 21; kartNo++)
                {
                    if (kartId == mKartIndex[kartNo])
                        break;
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

    switch (mInfo.mode)
    {
    case 12:
    case 13:
    case 14:
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
        break;
    }
    }

    mScreenPlayerIcon->draw(0.0f, 0.0f, mOrtho);
    MenuTitleLine::ptr()->draw(mOrtho);

    switch (mInfo.mode)
    {
    case 12:
    case 13:
    case 14:
    {
        for (int i = 0; i <= mInfo.entryNum; i++)
        {
            f32 ratio;
            switch (mInfo.entryNum)
            {
            case 0:
                mGrafs[0]->place(mCharRect[0].x, mCharRect[0].y + (mCharPaneBck[0].mPane->mOffset.y - 325.0f), mCharRect[0].w, mCharRect[0].h);
                mGrafs[0]->setPort();
                ratio = mCharRect[0].w / mCharRect[0].h;
                break;
            case 1:
                mGrafs[1 + i]->place(mCharRect[1 + i].x, mCharRect[1 + i].y + (mChar2Bck[0].mPane->mOffset.y - 325.0f), mCharRect[1 + i].w, mCharRect[1 + i].h);
                mGrafs[1 + i]->setPort();
                ratio = mCharRect[1 + i].w / mCharRect[1 + i].h;
                if (i == 0)
                    GXSetScissorBoxOffset(76, 0);
                else
                    GXSetScissorBoxOffset(0, 0);
                break;
            case 2:
                mGrafs[3 + i]->place(mCharRect[3 + i].x, mCharRect[3 + i].y + (mChar3Bck[0].mPane->mOffset.y - 325.0f), mCharRect[3 + i].w, mCharRect[3 + i].h);
                mGrafs[3 + i]->setPort();
                ratio = mCharRect[3 + i].w / mCharRect[3 + i].h;
                if (i == 0)
                    GXSetScissorBoxOffset(46, 0);
                else
                    GXSetScissorBoxOffset(0, 0);
                break;
            case 3:
                mGrafs[6 + i]->place(mCharRect[6 + i].x, mCharRect[6 + i].y + (mChar4Bck[0].mPane->mOffset.y - 325.0f), mCharRect[6 + i].w, mCharRect[6 + i].h);
                mGrafs[6 + i]->setPort();
                ratio = mCharRect[6 + i].w / mCharRect[6 + i].h;
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
        break;
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

    if ((mIconBck[6].mFrame += 1.0f) >= 75.0f)
        mIconBck[6].mFrame = 14.0f;
    if ((mIconBck[7].mFrame += 1.0f) >= 75.0f)
        mIconBck[7].mFrame = 14.0f;
    if ((mIconBck[8].mFrame += 1.0f) >= 75.0f)
        mIconBck[8].mFrame = 14.0f;
    if ((mIconBck[9].mFrame += 1.0f) >= 75.0f)
        mIconBck[9].mFrame = 14.0f;
    if ((mIconBck[10].mFrame += 1.0f) >= 75.0f)
        mIconBck[10].mFrame = 14.0f;
    if ((mIconBck[11].mFrame += 1.0f) >= 75.0f)
        mIconBck[11].mFrame = 14.0f;
    if ((mIconBck[12].mFrame += 1.0f) >= 75.0f)
        mIconBck[12].mFrame = 14.0f;
    if ((mIconBck[13].mFrame += 1.0f) >= 75.0f)
        mIconBck[13].mFrame = 14.0f;
    if ((mIconBck[14].mFrame += 1.0f) >= 75.0f)
        mIconBck[14].mFrame = 14.0f;
    for (int i = 0; i < 33; i++)
        mIconBck[i].mAnm->setFrame(mIconBck[i].mFrame);
    mScreenPlayerIcon->animation();

    if ((mHowManyDriversEnd.mFrame += 1.0f) >= 180.0f)
        mHowManyDriversEnd.mFrame = 0.0f;
    if ((mSingleTagSelectEndBck.mFrame += 1.0f) >= 120.0f)
        mSingleTagSelectEndBck.mFrame = 0.0f;
    for (int i = 0; i < 13; i++)
        mHowManyDriversBck[i].mAnm->setFrame(mHowManyDriversBck[i].mFrame);
    mHowManyDriversEnd.mAnm->setFrame(mHowManyDriversEnd.mFrame);
    for (int i = 0; i < 22; i++)
        mHowManyDriversBrk[i].mAnm->setFrame(mHowManyDriversBrk[i].mFrame);
    mSingleTagSelectEndBck.mAnm->setFrame(mSingleTagSelectEndBck.mFrame);
    mScreenHowManyDrivers->animation();

#define SM_INC(x, max, min) \
    if ((x.mFrame += 1.0f) >= max) \
        x.mFrame = min;
    SM_INC(mSingleTagSelectBtk[0], 180.0f, 0.0f)
    SM_INC(mSingleTagSelectBtk[1], 180.0f, 0.0f)
    SM_INC(mSingleTagSelectBtk[5], 180.0f, 0.0f)
    SM_INC(mSingleTagSelectBtk[6], 180.0f, 0.0f)
    SM_INC(mSingleTagSelectBtk[12], 180.0f, 0.0f)
    SM_INC(mSingleTagSelectBtk[13], 180.0f, 0.0f)
    SM_INC(mSingleTagSelectBtk[14], 180.0f, 0.0f)
    for (int i = 0; i < 43; i++)
        mSingleTagSelectBck[i].mAnm->setFrame(mSingleTagSelectBck[i].mFrame);
    for (int i = 0; i < 24; i++)
        mSingleTagSelectBtk[i].mAnm->setFrame(mSingleTagSelectBtk[i].mFrame);
    for (int i = 0; i < 78; i++)
        mSingleTagSelectBrk[i].mAnm->setFrame(mSingleTagSelectBrk[i].mFrame);
    for (int i = 0; i < 7; i++)
        mSingleTagSelectBpk[i].mAnm->setFrame(mSingleTagSelectBpk[i].mFrame);
    mScreenSingleTagSelect->animation();

    SM_INC(mModeBck[21], 50.0f, 20.0f)
    SM_INC(mModeBck[31], 50.0f, 20.0f)
    SM_INC(mModeBck[62], 50.0f, 20.0f)
    SM_INC(mModeBck[22], 50.0f, 20.0f)
    SM_INC(mModeBck[32], 50.0f, 20.0f)
    SM_INC(mModeBck[63], 50.0f, 20.0f)
    SM_INC(mModeBrk[29], 260.0f, 0.0f)
    SM_INC(mModeBtk[25], 120.0f, 0.0f)
    SM_INC(mModeBtk[0], 180.0f, 0.0f)
    SM_INC(mModeBtk[1], 180.0f, 0.0f)
    SM_INC(mModeBtk[2], 180.0f, 0.0f)
    SM_INC(mModeBtk[16], 180.0f, 0.0f)
    SM_INC(mModeBtk[17], 180.0f, 0.0f)
    SM_INC(mModeBpk[0], 120.0f, 0.0f)
    SM_INC(mModeBpk[1], 120.0f, 0.0f)
    SM_INC(mModeBpk[2], 120.0f, 0.0f)
    SM_INC(mModeBpk[3], 120.0f, 0.0f)
    SM_INC(mModeBpk[4], 120.0f, 0.0f)
    for (int i = 0; i < 75; i++)
        mModeBck[i].mAnm->setFrame(mModeBck[i].mFrame);
    for (int i = 0; i < 26; i++)
        mModeBtk[i].mAnm->setFrame(mModeBtk[i].mFrame);
    for (int i = 0; i < 30; i++)
        mModeBrk[i].mAnm->setFrame(mModeBrk[i].mFrame);
    for (int i = 0; i < 6; i++)
        mModeBpk[i].mAnm->setFrame(mModeBpk[i].mFrame);
    mScreenSelectMode->animation();
    calcModeAnm();
    calcModeOrder();

    SM_INC(mBattleBrk, 120.0f, 0.0f)
    for (int i = 0; i < 15; i++)
        mBattleBck[i].mAnm->setFrame(mBattleBck[i].mFrame);
    mBattleBrk.mAnm->setFrame(mBattleBrk.mFrame);
    mScreenSelectBattle->animation();

    switch (mInfo.mode)
    {
    case 12:
    case 13:
    case 14:
        for (int i = 0; i < 10; i++)
        {
            SM_INC(mCharCCBck[i], 120.0f, 0.0f)
            for (int j = 0; j < 2; j++)
                mCharPane[i][j].mAnm->setFrame(mCharPane[i][j].mFrame);
            mCharCCBck[i].mAnm->setFrame(mCharCCBck[i].mFrame);
            for (int j = 0; j < 4; j++)
                mCharCCBrk[i][j].mAnm->setFrame(mCharCCBrk[i][j].mFrame);
            for (int j = 0; j < 6; j++)
                mCharCCBpk[i][j].mAnm->setFrame(mCharCCBpk[i][j].mFrame);
            mCharScreens[i]->animation();
        }

        switch (mInfo.entryNum)
        {
        case 0:
            SM_INC(mCharPaneBck[5], 30.0f, 0.0f)
            SM_INC(mCharPaneBck[6], 30.0f, 0.0f)
            SM_INC(mCharCCEndBck, 120.0f, 0.0f)
            SM_INC(mCharCCEndBrk, 120.0f, 0.0f)
            for (int i = 0; i < 9; i++)
                mCharPaneBck[i].mAnm->setFrame(mCharPaneBck[i].mFrame);
            mCharCCEndBck.mAnm->setFrame(mCharCCEndBck.mFrame);
            mCharCCEndBrk.mAnm->setFrame(mCharCCEndBrk.mFrame);
            mScreenChar1->animation();
            for (int i = 0; i < 19; i++)
                mChar1PlusBck[i].mAnm->setFrame(mChar1PlusBck[i].mFrame);
            for (int i = 0; i < 2; i++)
                mChar1PlusBrk[i].mAnm->setFrame(mChar1PlusBrk[i].mFrame);
            mChar1PlusBpk.mAnm->setFrame(mChar1PlusBpk.mFrame);
            mScreenChar1Plus->animation();
            break;
        case 1:
            SM_INC(mChar2Bck[13], 30.0f, 0.0f)
            SM_INC(mChar2Bck[15], 30.0f, 0.0f)
            SM_INC(mChar2Bck[14], 30.0f, 0.0f)
            SM_INC(mChar2Bck[16], 30.0f, 0.0f)
            SM_INC(mChar2EndBck, 120.0f, 0.0f)
            SM_INC(mChar2EndBrk, 120.0f, 0.0f)
            for (int i = 0; i < 20; i++)
                mChar2Bck[i].mAnm->setFrame(mChar2Bck[i].mFrame);
            mChar2EndBck.mAnm->setFrame(mChar2EndBck.mFrame);
            for (int i = 0; i < 5; i++)
                mChar2Brk[i].mAnm->setFrame(mChar2Brk[i].mFrame);
            mChar2EndBrk.mAnm->setFrame(mChar2EndBrk.mFrame);
            mScreenChar2->animation();
            break;
        case 2:
            SM_INC(mChar3Bck[17], 30.0f, 0.0f)
            SM_INC(mChar3Bck[20], 30.0f, 0.0f)
            SM_INC(mChar3Bck[18], 30.0f, 0.0f)
            SM_INC(mChar3Bck[21], 30.0f, 0.0f)
            SM_INC(mChar3Bck[19], 30.0f, 0.0f)
            SM_INC(mChar3Bck[22], 30.0f, 0.0f)
            SM_INC(mChar3EndBck, 120.0f, 0.0f)
            SM_INC(mChar3EndBrk, 120.0f, 0.0f)
            for (int i = 0; i < 27; i++)
                mChar3Bck[i].mAnm->setFrame(mChar3Bck[i].mFrame);
            mChar3EndBck.mAnm->setFrame(mChar3EndBck.mFrame);
            for (int i = 0; i < 8; i++)
                mChar3Brk[i].mAnm->setFrame(mChar3Brk[i].mFrame);
            mChar3EndBrk.mAnm->setFrame(mChar3EndBrk.mFrame);
            mScreenChar3->animation();
            break;
        case 3:
            SM_INC(mChar4Bck[13], 30.0f, 0.0f)
            SM_INC(mChar4Bck[17], 30.0f, 0.0f)
            SM_INC(mChar4Bck[14], 30.0f, 0.0f)
            SM_INC(mChar4Bck[18], 30.0f, 0.0f)
            SM_INC(mChar4Bck[15], 30.0f, 0.0f)
            SM_INC(mChar4Bck[19], 30.0f, 0.0f)
            SM_INC(mChar4Bck[16], 30.0f, 0.0f)
            SM_INC(mChar4Bck[20], 30.0f, 0.0f)
            SM_INC(mChar4EndBck, 120.0f, 0.0f)
            SM_INC(mChar4EndBrk, 120.0f, 0.0f)
            for (int i = 0; i < 26; i++)
                mChar4Bck[i].mAnm->setFrame(mChar4Bck[i].mFrame);
            mChar4EndBck.mAnm->setFrame(mChar4EndBck.mFrame);
            mChar4EndBrk.mAnm->setFrame(mChar4EndBrk.mFrame);
            mScreenChar4->animation();
            break;
        default:
#line 2276
            JUT_ASSERT(0);
        }
        break;
    }
#undef SM_INC

    if (isRndRoulette())
    {
        mCharPaneBck[8].mPane->show();
        mChar2Bck[19].mPane->show();
        mChar3Bck[26].mPane->show();
        mChar4Bck[25].mPane->show();
    }
    else
    {
        mCharPaneBck[8].mPane->hide();
        mChar2Bck[19].mPane->hide();
        mChar3Bck[26].mPane->hide();
        mChar4Bck[25].mPane->hide();
    }

    switch (mInfo.mode)
    {
    case 12:
    case 13:
    case 14:
        mCharacterSelect3D->calc();
        break;
    }
    mNameEntry->calc();
    JPEffectMgr::mThis->calc_forMenu();
    mInfo.timer++;
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

void SceneMenu::vs()
{
    JUTGamePad *pad = gpaGamePad[0];
    u32 trigger = pad->getTrigger();
    if (trigger & JUTGamePad::A)
    {
        GameAudio::Main::getAudio()->startSystemSe(0x20003);
        mInfo.nextMode = 6;
        mInfo.mode = 5;
        mInfo.oldMode = 4;
        mInfo.timer = 0;
    }
    else if (trigger & JUTGamePad::B)
    {
        GameAudio::Main::getAudio()->startSystemSe(0x20005);
        mInfo.nextMode = 0;
        mInfo.mode = 5;
        mInfo.oldMode = 4;
        mInfo.timer = 0;
    }
    else
    {
        u32 repeat = pad->getRepeat();
        if (repeat & JUTGamePad::MAINSTICK_UP)
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            switch (mInfo.playerNum)
            {
            case 1:
            case 2:
                mInfo._18 = (mInfo._18 + 1) % 2;
                break;
            case 3:
                mInfo._18 = (mInfo._18 + 2) % 3;
                break;
            case 0:
            default:
#line 3307
                JUT_ASSERT(0);
            }
        }
        else if (repeat & JUTGamePad::MAINSTICK_DOWN)
        {
            GameAudio::Main::getAudio()->startSystemSe(0x20000);
            switch (mInfo.playerNum)
            {
            case 1:
            case 2:
                mInfo._18 = (mInfo._18 + 1) % 2;
                break;
            case 3:
                mInfo._18 = (mInfo._18 + 1) % 3;
                break;
            case 0:
            default:
#line 3322
                JUT_ASSERT(0);
            }
        }
    }

    switch (mInfo.playerNum)
    {
    case 1:
        switch (mInfo._18)
        {
        case 0:
            if ((mSingleTagSelectBck[2].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[2].mFrame = 22.0f;
            if ((mSingleTagSelectBck[3].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[3].mFrame = 14.0f;
            if ((mSingleTagSelectBpk[0].mFrame += 1.0f) >= 120.0f)
                mSingleTagSelectBpk[0].mFrame = 0.0f;
            mSingleTagSelectBpk[1].mFrame = 0.0f;
            if ((mSingleTagSelectBck[4].mFrame += 1.0f) >= 60.0f)
                mSingleTagSelectBck[4].mFrame = 10.0f;
            if ((mSingleTagSelectBck[5].mFrame += 1.0f) >= 60.0f)
                mSingleTagSelectBck[5].mFrame = 10.0f;
            mSingleTagSelectBck[6].mFrame = 10.0f;
            if ((mSingleTagSelectBtk[2].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[2].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[3].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[3].mFrame = 0.0f;
            mSingleTagSelectBtk[4].mFrame = 0.0f;
            if ((mSingleTagSelectBck[7].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[7].mFrame = 22.0f;
            if ((mSingleTagSelectBck[8].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[8].mFrame = 14.0f;
            if ((mSingleTagSelectBck[10].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[10].mFrame = 14.0f;
            if ((mSingleTagSelectBck[11].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[11].mFrame = 14.0f;
            mSingleTagSelectBck[9].mFrame = 14.0f;
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
            if ((mSingleTagSelectBck[2].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[2].mFrame = 14.0f;
            if ((mSingleTagSelectBck[3].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[3].mFrame = 22.0f;
            mSingleTagSelectBpk[0].mFrame = 0.0f;
            if ((mSingleTagSelectBpk[1].mFrame += 1.0f) >= 120.0f)
                mSingleTagSelectBpk[1].mFrame = 0.0f;
            mSingleTagSelectBck[4].mFrame = 10.0f;
            mSingleTagSelectBck[5].mFrame = 10.0f;
            if ((mSingleTagSelectBck[6].mFrame += 1.0f) >= 60.0f)
                mSingleTagSelectBck[6].mFrame = 10.0f;
            mSingleTagSelectBtk[2].mFrame = 0.0f;
            mSingleTagSelectBtk[3].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[4].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[4].mFrame = 0.0f;
            if ((mSingleTagSelectBck[7].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[7].mFrame = 14.0f;
            if ((mSingleTagSelectBck[8].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[8].mFrame = 22.0f;
            mSingleTagSelectBck[10].mFrame = 14.0f;
            mSingleTagSelectBck[11].mFrame = 14.0f;
            if ((mSingleTagSelectBck[9].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[9].mFrame = 14.0f;
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
#line 3410
            JUT_ASSERT(0);
        }
        break;
    case 2:
        switch (mInfo._18)
        {
        case 0:
            if ((mSingleTagSelectBck[13].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[13].mFrame = 22.0f;
            if ((mSingleTagSelectBck[14].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[14].mFrame = 14.0f;
            if ((mSingleTagSelectBpk[2].mFrame += 1.0f) >= 120.0f)
                mSingleTagSelectBpk[2].mFrame = 0.0f;
            mSingleTagSelectBpk[3].mFrame = 0.0f;
            if ((mSingleTagSelectBck[15].mFrame += 1.0f) >= 60.0f)
                mSingleTagSelectBck[15].mFrame = 10.0f;
            mSingleTagSelectBck[16].mFrame = 10.0f;
            if ((mSingleTagSelectBtk[7].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[7].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[8].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[8].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[9].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[9].mFrame = 0.0f;
            mSingleTagSelectBtk[10].mFrame = 0.0f;
            mSingleTagSelectBtk[11].mFrame = 0.0f;
            if ((mSingleTagSelectBck[17].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[17].mFrame = 22.0f;
            if ((mSingleTagSelectBck[18].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[18].mFrame = 14.0f;
            if ((mSingleTagSelectBck[19].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[19].mFrame = 14.0f;
            if ((mSingleTagSelectBck[20].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[20].mFrame = 14.0f;
            if ((mSingleTagSelectBck[21].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[21].mFrame = 14.0f;
            mSingleTagSelectBck[22].mFrame = 14.0f;
            mSingleTagSelectBck[23].mFrame = 14.0f;
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
            if ((mSingleTagSelectBck[13].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[13].mFrame = 14.0f;
            if ((mSingleTagSelectBck[14].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[14].mFrame = 22.0f;
            mSingleTagSelectBpk[2].mFrame = 0.0f;
            if ((mSingleTagSelectBpk[3].mFrame += 1.0f) >= 120.0f)
                mSingleTagSelectBpk[3].mFrame = 0.0f;
            mSingleTagSelectBck[15].mFrame = 10.0f;
            if ((mSingleTagSelectBck[16].mFrame += 1.0f) >= 60.0f)
                mSingleTagSelectBck[16].mFrame = 10.0f;
            mSingleTagSelectBtk[7].mFrame = 0.0f;
            mSingleTagSelectBtk[8].mFrame = 0.0f;
            mSingleTagSelectBtk[9].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[10].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[10].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[11].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[11].mFrame = 0.0f;
            if ((mSingleTagSelectBck[17].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[17].mFrame = 14.0f;
            if ((mSingleTagSelectBck[18].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[18].mFrame = 22.0f;
            mSingleTagSelectBck[19].mFrame = 14.0f;
            mSingleTagSelectBck[20].mFrame = 14.0f;
            mSingleTagSelectBck[21].mFrame = 14.0f;
            if ((mSingleTagSelectBck[22].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[22].mFrame = 14.0f;
            if ((mSingleTagSelectBck[23].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[23].mFrame = 14.0f;
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
#line 3516
            JUT_ASSERT(0);
        }
        break;
    case 3:
        switch (mInfo._18)
        {
        case 0:
            if ((mSingleTagSelectBck[25].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[25].mFrame = 22.0f;
            if ((mSingleTagSelectBck[26].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[26].mFrame = 14.0f;
            if ((mSingleTagSelectBck[27].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[27].mFrame = 14.0f;
            if ((mSingleTagSelectBpk[4].mFrame += 1.0f) >= 120.0f)
                mSingleTagSelectBpk[4].mFrame = 0.0f;
            mSingleTagSelectBpk[5].mFrame = 0.0f;
            mSingleTagSelectBpk[6].mFrame = 0.0f;
            if ((mSingleTagSelectBck[28].mFrame += 1.0f) >= 60.0f)
                mSingleTagSelectBck[28].mFrame = 10.0f;
            mSingleTagSelectBck[29].mFrame = 10.0f;
            mSingleTagSelectBck[30].mFrame = 10.0f;
            if ((mSingleTagSelectBtk[15].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[15].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[16].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[16].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[17].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[17].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[18].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[18].mFrame = 0.0f;
            mSingleTagSelectBtk[19].mFrame = 0.0f;
            mSingleTagSelectBtk[20].mFrame = 0.0f;
            mSingleTagSelectBtk[21].mFrame = 0.0f;
            mSingleTagSelectBtk[22].mFrame = 0.0f;
            mSingleTagSelectBtk[23].mFrame = 0.0f;
            if ((mSingleTagSelectBck[31].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[31].mFrame = 22.0f;
            if ((mSingleTagSelectBck[32].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[32].mFrame = 14.0f;
            if ((mSingleTagSelectBck[33].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[33].mFrame = 14.0f;
            if ((mSingleTagSelectBck[34].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[34].mFrame = 14.0f;
            if ((mSingleTagSelectBck[35].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[35].mFrame = 14.0f;
            if ((mSingleTagSelectBck[36].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[36].mFrame = 14.0f;
            if ((mSingleTagSelectBck[37].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[37].mFrame = 14.0f;
            mSingleTagSelectBck[38].mFrame = 14.0f;
            mSingleTagSelectBck[39].mFrame = 14.0f;
            mSingleTagSelectBck[40].mFrame = 14.0f;
            mSingleTagSelectBck[41].mFrame = 14.0f;
            mSingleTagSelectBck[42].mFrame = 14.0f;
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
            if ((mSingleTagSelectBck[25].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[25].mFrame = 14.0f;
            if ((mSingleTagSelectBck[26].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[26].mFrame = 22.0f;
            if ((mSingleTagSelectBck[27].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[27].mFrame = 14.0f;
            mSingleTagSelectBpk[4].mFrame = 0.0f;
            if ((mSingleTagSelectBpk[5].mFrame += 1.0f) >= 120.0f)
                mSingleTagSelectBpk[5].mFrame = 0.0f;
            mSingleTagSelectBpk[6].mFrame = 0.0f;
            mSingleTagSelectBck[28].mFrame = 10.0f;
            if ((mSingleTagSelectBck[29].mFrame += 1.0f) >= 60.0f)
                mSingleTagSelectBck[29].mFrame = 10.0f;
            mSingleTagSelectBck[30].mFrame = 10.0f;
            mSingleTagSelectBtk[15].mFrame = 0.0f;
            mSingleTagSelectBtk[16].mFrame = 0.0f;
            mSingleTagSelectBtk[17].mFrame = 0.0f;
            mSingleTagSelectBtk[18].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[19].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[19].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[20].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[20].mFrame = 0.0f;
            mSingleTagSelectBtk[21].mFrame = 0.0f;
            mSingleTagSelectBtk[22].mFrame = 0.0f;
            mSingleTagSelectBtk[23].mFrame = 0.0f;
            if ((mSingleTagSelectBck[31].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[31].mFrame = 14.0f;
            if ((mSingleTagSelectBck[32].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[32].mFrame = 22.0f;
            if ((mSingleTagSelectBck[33].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[33].mFrame = 14.0f;
            mSingleTagSelectBck[34].mFrame = 14.0f;
            mSingleTagSelectBck[35].mFrame = 14.0f;
            mSingleTagSelectBck[36].mFrame = 14.0f;
            mSingleTagSelectBck[37].mFrame = 14.0f;
            if ((mSingleTagSelectBck[38].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[38].mFrame = 14.0f;
            if ((mSingleTagSelectBck[39].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[39].mFrame = 14.0f;
            mSingleTagSelectBck[40].mFrame = 14.0f;
            mSingleTagSelectBck[41].mFrame = 14.0f;
            mSingleTagSelectBck[42].mFrame = 14.0f;
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
            if ((mSingleTagSelectBck[25].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[25].mFrame = 14.0f;
            if ((mSingleTagSelectBck[26].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[26].mFrame = 14.0f;
            if ((mSingleTagSelectBck[27].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[27].mFrame = 22.0f;
            mSingleTagSelectBpk[4].mFrame = 0.0f;
            mSingleTagSelectBpk[5].mFrame = 0.0f;
            if ((mSingleTagSelectBpk[6].mFrame += 1.0f) >= 120.0f)
                mSingleTagSelectBpk[6].mFrame = 0.0f;
            mSingleTagSelectBck[28].mFrame = 10.0f;
            mSingleTagSelectBck[29].mFrame = 10.0f;
            if ((mSingleTagSelectBck[30].mFrame += 1.0f) >= 60.0f)
                mSingleTagSelectBck[30].mFrame = 10.0f;
            mSingleTagSelectBtk[15].mFrame = 0.0f;
            mSingleTagSelectBtk[16].mFrame = 0.0f;
            mSingleTagSelectBtk[17].mFrame = 0.0f;
            mSingleTagSelectBtk[18].mFrame = 0.0f;
            mSingleTagSelectBtk[19].mFrame = 0.0f;
            mSingleTagSelectBtk[20].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[21].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[21].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[22].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[22].mFrame = 0.0f;
            if ((mSingleTagSelectBtk[23].mFrame += 1.0f) >= 180.0f)
                mSingleTagSelectBtk[23].mFrame = 0.0f;
            if ((mSingleTagSelectBck[31].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[31].mFrame = 14.0f;
            if ((mSingleTagSelectBck[32].mFrame -= 1.0f) < 14.0f)
                mSingleTagSelectBck[32].mFrame = 14.0f;
            if ((mSingleTagSelectBck[33].mFrame += 1.0f) > 22.0f)
                mSingleTagSelectBck[33].mFrame = 22.0f;
            mSingleTagSelectBck[34].mFrame = 14.0f;
            mSingleTagSelectBck[35].mFrame = 14.0f;
            mSingleTagSelectBck[36].mFrame = 14.0f;
            mSingleTagSelectBck[37].mFrame = 14.0f;
            mSingleTagSelectBck[38].mFrame = 14.0f;
            mSingleTagSelectBck[39].mFrame = 14.0f;
            if ((mSingleTagSelectBck[40].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[40].mFrame = 14.0f;
            if ((mSingleTagSelectBck[41].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[41].mFrame = 14.0f;
            if ((mSingleTagSelectBck[42].mFrame += 1.0f) >= 75.0f)
                mSingleTagSelectBck[42].mFrame = 14.0f;
            mSingleTagSelectBrk[36].mFrame = 0.0f;
            mSingleTagSelectBrk[37].mFrame = 0.0f;
            mSingleTagSelectBrk[38].mFrame = 1.0f;
            mSingleTagSelectBrk[39].mFrame = 0.0f;
            mSingleTagSelectBrk[40].mFrame = 0.0f;
            mSingleTagSelectBrk[41].mFrame = 0.0f;
            mSingleTagSelectBrk[42].mFrame = 0.0f;
            mSingleTagSelectBrk[43].mFrame = 0.0f;
            mSingleTagSelectBrk[44].mFrame = 0.0f;
            mSingleTagSelectBrk[45].mFrame = 0.0f;
            mSingleTagSelectBrk[46].mFrame = 0.0f;
            mSingleTagSelectBrk[47].mFrame = 1.0f;
            mSingleTagSelectBrk[48].mFrame = 1.0f;
            mSingleTagSelectBrk[49].mFrame = 1.0f;
            mSingleTagSelectBrk[50].mFrame = 1.0f;
            mSingleTagSelectBrk[51].mFrame = 0.0f;
            mSingleTagSelectBrk[52].mFrame = 0.0f;
            mSingleTagSelectBrk[53].mFrame = 0.0f;
            mSingleTagSelectBrk[54].mFrame = 0.0f;
            mSingleTagSelectBrk[55].mFrame = 0.0f;
            mSingleTagSelectBrk[56].mFrame = 0.0f;
            mSingleTagSelectBrk[57].mFrame = 0.0f;
            mSingleTagSelectBrk[58].mFrame = 0.0f;
            mSingleTagSelectBrk[59].mFrame = 1.0f;
            mSingleTagSelectBrk[60].mFrame = 1.0f;
            mSingleTagSelectBrk[61].mFrame = 1.0f;
            mSingleTagSelectBrk[62].mFrame = 1.0f;
            mSingleTagSelectBrk[63].mFrame = 0.0f;
            mSingleTagSelectBrk[64].mFrame = 0.0f;
            mSingleTagSelectBrk[65].mFrame = 0.0f;
            mSingleTagSelectBrk[66].mFrame = 0.0f;
            mSingleTagSelectBrk[67].mFrame = 0.0f;
            mSingleTagSelectBrk[68].mFrame = 0.0f;
            mSingleTagSelectBrk[69].mFrame = 1.0f;
            mSingleTagSelectBrk[70].mFrame = 1.0f;
            mSingleTagSelectBrk[71].mFrame = 1.0f;
            mSingleTagSelectBrk[72].mFrame = 0.0f;
            mSingleTagSelectBrk[73].mFrame = 0.0f;
            mSingleTagSelectBrk[74].mFrame = 0.0f;
            mSingleTagSelectBrk[75].mFrame = 0.0f;
            mSingleTagSelectBrk[76].mFrame = 1.0f;
            mSingleTagSelectBrk[77].mFrame = 1.0f;
            break;
        default:
#line 3767
            JUT_ASSERT(0);
        }
        break;
    case 0:
    default:
#line 3773
        JUT_ASSERT(0);
    }

    if ((mIconBck[2].mFrame += 1.0f) >= 75.0f)
        mIconBck[2].mFrame = 14.0f;
}

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

void SceneMenu::mode_init()
{
    if (mInfo.timer < 15)
    {
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcSelectMode);
        mIconBck[0].mPane->show();
        mIconBck[0].mFrame = mInfo.timer;
        switch (mInfo.entryNum)
        {
        case 0:
        case 2:
        case 3:
            mModeBck[33].mPane->show();
            mModeBck[61].mPane->show();
            mModeBck[34].mPane->show();
            mModeBck[35].mPane->show();
            mModeBck[33].mFrame = mInfo.timer;
            mModeBck[61].mFrame = 4.0f;
            mModeBrk[20].mFrame = 0.0f;
            mModeBrk[21].mFrame = 0.0f;
            switch (mInfo.entryNum)
            {
            case 0:
                mModeBck[36].mPane->show();
                mModeBck[37].mPane->show();
                mModeBck[38].mPane->show();
                mModeBck[70].mPane->show();
                mModeBck[36].mFrame = mInfo.timer;
                mModeBck[37].mFrame = 250.0f;
                mModeBck[38].mFrame = 250.0f;
                mModeBck[70].mFrame = 4.0f;
                mModeBck[74].mFrame = 20.0f;
                break;
            case 2:
            case 3:
                mModeBck[48].mPane->show();
                mModeBck[49].mPane->show();
                mModeBck[52].mPane->show();
                mModeBck[48].mFrame = mInfo.timer;
                mModeBck[49].mFrame = 250.0f;
                mModeBck[52].mFrame = 250.0f;
                break;
            default:
#line 3916
                JUT_ASSERT(0);
            }
            break;
        case 1:
            mModeBck[0].mPane->show();
            mModeBck[19].mPane->show();
            mModeBck[26].mPane->show();
            mModeBck[1].mPane->show();
            mModeBck[2].mPane->show();
            mModeBck[3].mPane->show();
            mModeBck[0].mFrame = mInfo.timer;
            mModeBck[4].mFrame = 250.0f;
            mModeBck[5].mFrame = 250.0f;
            mModeBck[8].mFrame = 250.0f;
            mModeBck[19].mFrame = 4.0f;
            mModeBck[26].mFrame = 4.0f;
            break;
        default:
#line 3936
            JUT_ASSERT(0);
        }

        switch (mInfo.entryNum)
        {
        case 0:
            switch (mInfo._20)
            {
            case 0:
            case 1:
            case 3:
                mInfo._20 = 0;
                mModeBck[34].mFrame = 17.0f;
                mModeBck[35].mFrame = 9.0f;
                mModeBck[44].mFrame = 19.0f;
                mModeBck[46].mFrame = 14.0f;
                mModeBrk[20].mFrame = 1.0f;
                mModeBrk[21].mFrame = 0.0f;
                mModeBrk[16].mFrame = 1.0f;
                mModeBrk[17].mFrame = 1.0f;
                mModeBrk[18].mFrame = 0.0f;
                mModeBrk[19].mFrame = 0.0f;
                break;
            case 2:
                mInfo._20 = 2;
                mModeBck[34].mFrame = 9.0f;
                mModeBck[35].mFrame = 17.0f;
                mModeBck[44].mFrame = 14.0f;
                mModeBck[46].mFrame = 19.0f;
                mModeBrk[20].mFrame = 0.0f;
                mModeBrk[21].mFrame = 1.0f;
                mModeBrk[16].mFrame = 0.0f;
                mModeBrk[17].mFrame = 0.0f;
                mModeBrk[18].mFrame = 1.0f;
                mModeBrk[19].mFrame = 1.0f;
                break;
            default:
#line 3977
                JUT_ASSERT(0);
            }
            mModeBck[45].mFrame = 10.0f;
            mModeBck[47].mFrame = 10.0f;
            mModeBck[39].mFrame = 9.0f;
            mModeBck[40].mFrame = 10.0f;
            mModeBck[41].mFrame = 10.0f;
            mModeBck[42].mFrame = 10.0f;
            mModeBck[43].mFrame = 10.0f;
            mModeBtk[14].mFrame = 0.0f;
            mModeBtk[15].mFrame = 0.0f;
            mModeBpk[5].mFrame = 10.0f;
            break;
        case 1:
            switch (mInfo._20)
            {
            case 0:
            case 2:
                mInfo._20 = 0;
                mModeBck[1].mFrame = 17.0f;
                mModeBck[2].mFrame = 9.0f;
                mModeBck[3].mFrame = 9.0f;
                mModeBck[13].mFrame = 19.0f;
                mModeBck[15].mFrame = 14.0f;
                mModeBck[17].mFrame = 14.0f;
                mModeBrk[0].mFrame = 1.0f;
                mModeBrk[1].mFrame = 0.0f;
                mModeBrk[2].mFrame = 0.0f;
                mModeBrk[4].mFrame = 1.0f;
                mModeBrk[5].mFrame = 1.0f;
                mModeBrk[6].mFrame = 0.0f;
                mModeBrk[7].mFrame = 0.0f;
                mModeBrk[8].mFrame = 0.0f;
                mModeBrk[9].mFrame = 0.0f;
                mModeBrk[3].mFrame = 0.0f;
                break;
            case 1:
                mInfo._20 = 1;
                mModeBck[1].mFrame = 9.0f;
                mModeBck[2].mFrame = 17.0f;
                mModeBck[3].mFrame = 9.0f;
                mModeBck[13].mFrame = 14.0f;
                mModeBck[15].mFrame = 19.0f;
                mModeBck[17].mFrame = 14.0f;
                mModeBrk[0].mFrame = 0.0f;
                mModeBrk[1].mFrame = 1.0f;
                mModeBrk[2].mFrame = 0.0f;
                mModeBrk[4].mFrame = 0.0f;
                mModeBrk[5].mFrame = 0.0f;
                mModeBrk[6].mFrame = 1.0f;
                mModeBrk[7].mFrame = 1.0f;
                mModeBrk[8].mFrame = 0.0f;
                mModeBrk[9].mFrame = 0.0f;
                mModeBrk[3].mFrame = 0.0f;
                break;
            case 3:
                mInfo._20 = 3;
                mModeBck[1].mFrame = 9.0f;
                mModeBck[2].mFrame = 9.0f;
                mModeBck[3].mFrame = 17.0f;
                mModeBck[13].mFrame = 14.0f;
                mModeBck[15].mFrame = 14.0f;
                mModeBck[17].mFrame = 19.0f;
                mModeBrk[0].mFrame = 0.0f;
                mModeBrk[1].mFrame = 0.0f;
                mModeBrk[2].mFrame = 1.0f;
                mModeBrk[4].mFrame = 0.0f;
                mModeBrk[5].mFrame = 0.0f;
                mModeBrk[6].mFrame = 0.0f;
                mModeBrk[7].mFrame = 0.0f;
                mModeBrk[8].mFrame = 1.0f;
                mModeBrk[9].mFrame = 1.0f;
                mModeBrk[3].mFrame = 1.0f;
                break;
            default:
#line 4061
                JUT_ASSERT(0);
            }
            mModeBck[6].mFrame = 10.0f;
            mModeBck[7].mFrame = 10.0f;
            mModeBck[10].mFrame = 10.0f;
            mModeBck[11].mFrame = 10.0f;
            mModeBck[12].mFrame = 10.0f;
            mModeBck[14].mFrame = 10.0f;
            mModeBck[16].mFrame = 10.0f;
            mModeBck[18].mFrame = 10.0f;
            mModeBtk[3].mFrame = 0.0f;
            mModeBtk[4].mFrame = 0.0f;
            mModeBtk[5].mFrame = 0.0f;
            mModeBtk[6].mFrame = 0.0f;
            mModeBtk[7].mFrame = 0.0f;
            mModeBtk[8].mFrame = 0.0f;
            mModeBtk[9].mFrame = 0.0f;
            mModeBtk[10].mFrame = 0.0f;
            mModeBtk[11].mFrame = 0.0f;
            mModeBtk[12].mFrame = 0.0f;
            mModeBtk[13].mFrame = 0.0f;
            break;
        case 2:
        case 3:
            switch (mInfo._20)
            {
            case 0:
            case 1:
            case 2:
                mInfo._20 = 1;
                mModeBck[34].mFrame = 17.0f;
                mModeBck[35].mFrame = 9.0f;
                mModeBck[57].mFrame = 19.0f;
                mModeBck[59].mFrame = 14.0f;
                mModeBrk[20].mFrame = 1.0f;
                mModeBrk[21].mFrame = 0.0f;
                mModeBrk[23].mFrame = 1.0f;
                mModeBrk[24].mFrame = 1.0f;
                mModeBrk[25].mFrame = 0.0f;
                mModeBrk[26].mFrame = 0.0f;
                mModeBrk[22].mFrame = 0.0f;
                break;
            case 3:
                mInfo._20 = 3;
                mModeBck[34].mFrame = 9.0f;
                mModeBck[35].mFrame = 17.0f;
                mModeBck[57].mFrame = 14.0f;
                mModeBck[59].mFrame = 19.0f;
                mModeBrk[20].mFrame = 0.0f;
                mModeBrk[21].mFrame = 1.0f;
                mModeBrk[23].mFrame = 0.0f;
                mModeBrk[24].mFrame = 0.0f;
                mModeBrk[25].mFrame = 1.0f;
                mModeBrk[26].mFrame = 1.0f;
                mModeBrk[22].mFrame = 1.0f;
                break;
            default:
#line 4124
                JUT_ASSERT(0);
            }
            mModeBck[58].mFrame = 10.0f;
            mModeBck[50].mFrame = 10.0f;
            mModeBck[51].mFrame = 10.0f;
            mModeBck[60].mFrame = 10.0f;
            mModeBck[54].mFrame = 10.0f;
            mModeBck[55].mFrame = 10.0f;
            mModeBck[56].mFrame = 10.0f;
            mModeBtk[19].mFrame = 0.0f;
            mModeBtk[20].mFrame = 0.0f;
            mModeBtk[21].mFrame = 0.0f;
            mModeBtk[22].mFrame = 0.0f;
            mModeBtk[23].mFrame = 0.0f;
            mModeBtk[24].mFrame = 0.0f;
            mModeBtk[18].mFrame = 0.0f;
            break;
        default:
#line 4145
            JUT_ASSERT(0);
        }

        switch (mInfo.playerNum)
        {
        case 0:
            mIconBck[3].mPane->show();
            mIconBck[6].mPane->show();
            mIconBck[7].mPane->hide();
            mIconBck[8].mPane->hide();
            mIconBck[9].mPane->hide();
            mIconBck[15].mPane->show();
            mIconBck[16].mPane->hide();
            mIconBck[17].mPane->hide();
            mIconBck[18].mPane->hide();
            mIconBck[27].mPane->hide();
            mIconBck[28].mPane->hide();
            mIconBck[29].mPane->hide();
            mIconBck[3].mFrame = mInfo.timer;
            mIconBck[15].mFrame = mInfo.timer;
            break;
        case 1:
            switch (mInfo._18)
            {
            case 0:
                mIconBck[3].mPane->show();
                mIconBck[6].mPane->show();
                mIconBck[7].mPane->show();
                mIconBck[8].mPane->hide();
                mIconBck[9].mPane->hide();
                mIconBck[15].mPane->show();
                mIconBck[16].mPane->show();
                mIconBck[17].mPane->hide();
                mIconBck[18].mPane->hide();
                mIconBck[27].mPane->show();
                mIconBck[28].mPane->hide();
                mIconBck[29].mPane->hide();
                mIconBck[3].mFrame = mInfo.timer;
                mIconBck[15].mFrame = mInfo.timer;
                mIconBck[16].mFrame = mInfo.timer;
                mIconBck[27].mFrame = mInfo.timer;
                break;
            case 1:
                mIconBck[4].mPane->show();
                mIconBck[10].mPane->show();
                mIconBck[11].mPane->hide();
                mIconBck[12].mPane->hide();
                mIconBck[19].mPane->show();
                mIconBck[20].mPane->show();
                mIconBck[21].mPane->hide();
                mIconBck[22].mPane->hide();
                mIconBck[30].mPane->hide();
                mIconBck[31].mPane->hide();
                mIconBck[4].mFrame = mInfo.timer;
                mIconBck[19].mFrame = mInfo.timer;
                mIconBck[20].mFrame = mInfo.timer;
                break;
            default:
#line 4206
                JUT_ASSERT(0);
            }
            break;
        case 2:
            switch (mInfo._18)
            {
            case 0:
                mIconBck[3].mPane->show();
                mIconBck[6].mPane->show();
                mIconBck[7].mPane->show();
                mIconBck[8].mPane->show();
                mIconBck[9].mPane->hide();
                mIconBck[15].mPane->show();
                mIconBck[16].mPane->show();
                mIconBck[17].mPane->show();
                mIconBck[18].mPane->hide();
                mIconBck[27].mPane->show();
                mIconBck[28].mPane->show();
                mIconBck[29].mPane->hide();
                mIconBck[3].mFrame = mInfo.timer;
                mIconBck[15].mFrame = mInfo.timer;
                mIconBck[16].mFrame = mInfo.timer;
                mIconBck[17].mFrame = mInfo.timer;
                mIconBck[27].mFrame = mInfo.timer;
                mIconBck[28].mFrame = mInfo.timer;
                break;
            case 1:
                mIconBck[4].mPane->show();
                mIconBck[10].mPane->show();
                mIconBck[11].mPane->show();
                mIconBck[12].mPane->hide();
                mIconBck[19].mPane->show();
                mIconBck[20].mPane->show();
                mIconBck[21].mPane->show();
                mIconBck[22].mPane->hide();
                mIconBck[30].mPane->show();
                mIconBck[31].mPane->hide();
                mIconBck[4].mFrame = mInfo.timer;
                mIconBck[19].mFrame = mInfo.timer;
                mIconBck[20].mFrame = mInfo.timer;
                mIconBck[21].mFrame = mInfo.timer;
                mIconBck[30].mFrame = mInfo.timer;
                break;
            default:
#line 4252
                JUT_ASSERT(0);
            }
            break;
        case 3:
            switch (mInfo._18)
            {
            case 0:
                mIconBck[3].mPane->show();
                mIconBck[6].mPane->show();
                mIconBck[7].mPane->show();
                mIconBck[8].mPane->show();
                mIconBck[9].mPane->show();
                mIconBck[15].mPane->show();
                mIconBck[16].mPane->show();
                mIconBck[17].mPane->show();
                mIconBck[18].mPane->show();
                mIconBck[27].mPane->show();
                mIconBck[28].mPane->show();
                mIconBck[29].mPane->show();
                mIconBck[3].mFrame = mInfo.timer;
                mIconBck[15].mFrame = mInfo.timer;
                mIconBck[16].mFrame = mInfo.timer;
                mIconBck[17].mFrame = mInfo.timer;
                mIconBck[18].mFrame = mInfo.timer;
                mIconBck[27].mFrame = mInfo.timer;
                mIconBck[28].mFrame = mInfo.timer;
                mIconBck[29].mFrame = mInfo.timer;
                break;
            case 1:
                mIconBck[5].mPane->show();
                mIconBck[13].mPane->show();
                mIconBck[14].mPane->show();
                mIconBck[23].mPane->show();
                mIconBck[24].mPane->show();
                mIconBck[25].mPane->show();
                mIconBck[26].mPane->show();
                mIconBck[32].mPane->show();
                mIconBck[5].mFrame = mInfo.timer;
                mIconBck[23].mFrame = mInfo.timer;
                mIconBck[24].mFrame = mInfo.timer;
                mIconBck[25].mFrame = mInfo.timer;
                mIconBck[26].mFrame = mInfo.timer;
                mIconBck[32].mFrame = mInfo.timer;
                break;
            case 2:
                mIconBck[4].mPane->show();
                mIconBck[10].mPane->show();
                mIconBck[11].mPane->show();
                mIconBck[12].mPane->show();
                mIconBck[19].mPane->show();
                mIconBck[20].mPane->show();
                mIconBck[21].mPane->show();
                mIconBck[22].mPane->show();
                mIconBck[30].mPane->show();
                mIconBck[31].mPane->show();
                mIconBck[4].mFrame = mInfo.timer;
                mIconBck[19].mFrame = mInfo.timer;
                mIconBck[20].mFrame = mInfo.timer;
                mIconBck[21].mFrame = mInfo.timer;
                mIconBck[22].mFrame = mInfo.timer;
                mIconBck[30].mFrame = mInfo.timer;
                mIconBck[31].mFrame = mInfo.timer;
                break;
            default:
#line 4319
                JUT_ASSERT(0);
            }
            break;
        default:
#line 4324
            JUT_ASSERT(0);
        }

        if (mInfo.entryNum == 0 && mInfo.playerNum == 1)
        {
            mModeBck[38].mPane->hide();
            mModeBck[46].mPane->hide();
            mModeBck[35].mPane->hide();
            mModeBrk[21].mPane->hide();
        }
        else
        {
            mModeBck[38].mPane->show();
            mModeBck[46].mPane->show();
            mModeBck[35].mPane->show();
            mModeBrk[21].mPane->show();
        }
    }
    else
    {
        mInfo.oldMode = mInfo.mode;
        mInfo.mode = 7;
        mInfo.timer = 0;
        _2108[0] = 0;
        _2108[1] = 0;
        _2108[2] = 0;
        _210c = 0;
        _2110 = 0;
        _2114 = 0;
        mNameEntry->init(mName);
        J2DPicture *pane;
        pane = (J2DPicture *)mModeBck[20].mPane;
#line 4354
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        pane->changeTexture(mPowerPicName[mInfo.engineType], 0);
        pane = (J2DPicture *)mModeBck[27].mPane;
#line 4357
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        pane->changeTexture(mPowerPicName[mInfo.engineType], 0);
        pane = (J2DPicture *)mModeBck[68].mPane;
#line 4360
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        pane->changeTexture(mPowerPicName[mInfo.engineType], 0);
        pane = (J2DPicture *)mModeBck[25].mPane;
#line 4363
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        pane->changeTexture(mPowerMojiName[mInfo.engineType], 0);
        pane = (J2DPicture *)mModeBck[28].mPane;
#line 4366
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        pane->changeTexture(mPowerMojiName[mInfo.engineType], 0);
        pane = (J2DPicture *)mModeBck[69].mPane;
#line 4369
        JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
        pane->changeTexture(mPowerMojiName[mInfo.engineType], 0);
    }
}

void SceneMenu::mode()
{
    SystemRecord *sysRecord = &gSystemRecord;
    char *defaultName = sysRecord->mDefaultName;
    SequenceInfo *seqInfo = &gSequenceInfo;
    int i;
    if (_2108[2])
    {
        if (CardAgent::msState == 0)
        {
            if (CardAgent::mspPrintMemoryCard->_d == 0)
                GameAudio::Main::getAudio()->startSystemSe(0x20003);
            CardAgent::mspPrintMemoryCard->setStuff(0, 0, 0);
            mInfo.nextMode = 12;
            mInfo.mode = 8;
            mInfo.oldMode = 7;
            mInfo.timer = 0;
            if (!(gSystemRecord.mOptions & 8))
            {
                mInfo.nextMode = 15;
                mInfo.nextScene = Scene::SCENE_GHOST_LOAD_SAVE;
                seqInfo->setGhostFlag(3);
            }
        }
    }
    else if (_210c == 0 && _2110 == 0 && _2114 == 0)
    {
        if (mInfo._20 == 3 && _2108[0])
        {
            mInfo.nextMode = 9;
            mInfo.mode = 8;
            mInfo.oldMode = 7;
            mInfo.timer = 0;
            for (i = 0; i < 4; i++)
                gSequenceInfo._80[i] = 0;
        }
        else if (mNameEntry->mAction != 1 && mNameEntry->mAction != 2)
        {
            JUTGamePad *pad = gpaGamePad[0];
            u32 trigger = pad->getTrigger();
            if (trigger & JUTGamePad::A)
            {
                if (_2108[0])
                {
                    if (mInfo._20 == 2)
                    {
                        if (_2108[1])
                        {
                            if (strncmp(mName, defaultName, 4) != 0)
                            {
                                gSystemRecord.setDefaultName(mName);
                                CardAgent::ask(CardAgent::mcCommand3, 0);
                                _2108[2] = 1;
                            }
                            else
                            {
                                GameAudio::Main::getAudio()->startSystemSe(0x20003);
                                mInfo.nextMode = 12;
                                mInfo.mode = 8;
                                mInfo.oldMode = 7;
                                mInfo.timer = 0;
                                if (!(gSystemRecord.mOptions & 8))
                                {
                                    mInfo.nextMode = 15;
                                    mInfo.nextScene = Scene::SCENE_GHOST_LOAD_SAVE;
                                    seqInfo->setGhostFlag(3);
                                }
                            }
                        }
                        else
                        {
                            GameAudio::Main::getAudio()->startSystemSe(0x20003);
                            _2108[1] = 1;
                            _2110 = 1;
                        }
                    }
                    else
                    {
                        GameAudio::Main::getAudio()->startSystemSe(0x20003);
                        mInfo.nextMode = 12;
                        mInfo.mode = 8;
                        mInfo.oldMode = 7;
                        mInfo.timer = 0;
                        switch (mInfo._20)
                        {
                        case 1:
                            for (i = 0; i < 4; i++)
                            {
                                for (int j = 0; j < 4; j++)
                                    gSequenceInfo._40[i][j] = 0;
                            }
                            break;
                        case 3:
                            mInfo.nextMode = 9;
                            for (i = 0; i < 4; i++)
                                gSequenceInfo._80[i] = 0;
                            break;
                        }
                    }
                }
                else
                {
                    GameAudio::Main::getAudio()->startSystemSe(0x20003);
                    _2108[0] = 1;
                    _210c = 1;
                }
            }
            else if (trigger & JUTGamePad::B)
            {
                GameAudio::Main::getAudio()->startSystemSe(0x20005);
                if (_2108[0])
                {
                    if (_2108[1])
                    {
                        mNameEntry->startEdit();
                        _2108[1] = 0;
                        _2110 = 1;
                    }
                    else
                    {
                        _2108[0] = 0;
                        _210c = 1;
                    }
                }
                else if (mInfo.playerNum == 0)
                {
                    mInfo.nextMode = 0;
                    mInfo.mode = 8;
                    mInfo.oldMode = 7;
                    mInfo.timer = 0;
                }
                else
                {
                    mInfo.nextMode = 3;
                    mInfo.mode = 8;
                    mInfo.oldMode = 7;
                    mInfo.timer = 0;
                }
            }
            else
            {
                u32 repeat = pad->getRepeat();
                if ((repeat & JUTGamePad::MAINSTICK_UP) && !_2108[0])
                {
                    switch (mInfo.entryNum)
                    {
                    case 0:
                        if (mInfo.playerNum == 1)
                        {
                            mInfo._20 = 0;
                        }
                        else
                        {
                            GameAudio::Main::getAudio()->startSystemSe(0x20000);
                            switch (mInfo._20)
                            {
                            case 0:
                                mInfo._20 = 2;
                                break;
                            case 2:
                                mInfo._20 = 0;
                                break;
                            case 1:
                            default:
#line 4521
                                JUT_ASSERT(0);
                            }
                        }
                        break;
                    case 1:
                        GameAudio::Main::getAudio()->startSystemSe(0x20000);
                        switch (mInfo._20)
                        {
                        case 0:
                            mInfo._20 = 3;
                            break;
                        case 1:
                            mInfo._20 = 0;
                            break;
                        case 3:
                            mInfo._20 = 1;
                            break;
                        case 2:
                        default:
#line 4556
                            JUT_ASSERT(0);
                        }
                        break;
                    case 2:
                    case 3:
                        GameAudio::Main::getAudio()->startSystemSe(0x20000);
                        switch (mInfo._20)
                        {
                        case 1:
                            mInfo._20 = 3;
                            break;
                        case 3:
                            mInfo._20 = 1;
                            break;
                        case 2:
                        default:
#line 4576
                            JUT_ASSERT(0);
                        }
                        break;
                    default:
#line 4582
                        JUT_ASSERT(0);
                    }
                }
                else if ((repeat & JUTGamePad::MAINSTICK_DOWN) && !_2108[0])
                {
                    switch (mInfo.entryNum)
                    {
                    case 0:
                        if (mInfo.playerNum == 1)
                        {
                            mInfo._20 = 0;
                        }
                        else
                        {
                            GameAudio::Main::getAudio()->startSystemSe(0x20000);
                            switch (mInfo._20)
                            {
                            case 0:
                                mInfo._20 = 2;
                                break;
                            case 2:
                                mInfo._20 = 0;
                                break;
                            case 1:
                            default:
#line 4605
                                JUT_ASSERT(0);
                            }
                        }
                        break;
                    case 1:
                        GameAudio::Main::getAudio()->startSystemSe(0x20000);
                        switch (mInfo._20)
                        {
                        case 0:
                            mInfo._20 = 1;
                            break;
                        case 1:
                            mInfo._20 = 3;
                            break;
                        case 3:
                            mInfo._20 = 0;
                            break;
                        case 2:
                        default:
#line 4640
                            JUT_ASSERT(0);
                        }
                        break;
                    case 2:
                    case 3:
                        GameAudio::Main::getAudio()->startSystemSe(0x20000);
                        switch (mInfo._20)
                        {
                        case 1:
                            mInfo._20 = 3;
                            break;
                        case 3:
                            mInfo._20 = 1;
                            break;
                        case 2:
                        default:
#line 4660
                            JUT_ASSERT(0);
                        }
                        break;
                    default:
#line 4666
                        JUT_ASSERT(0);
                    }
                }
                else if ((repeat & JUTGamePad::MAINSTICK_LEFT) && _2108[0])
                {
                    mModeBck[20].mFrame = 21.0f;
                    mModeBck[27].mFrame = 21.0f;
                    mModeBck[68].mFrame = 21.0f;
                    mModeBck[25].mFrame = 21.0f;
                    mModeBck[28].mFrame = 21.0f;
                    mModeBck[69].mFrame = 21.0f;
                    mModeBck[23].mFrame = 21.0f;
                    mModeBrk[10].mFrame = 1.0f;
                    mModeBck[29].mFrame = 21.0f;
                    mModeBrk[12].mFrame = 1.0f;
                    mModeBck[64].mFrame = 21.0f;
                    mModeBrk[27].mFrame = 1.0f;
                    if (mInfo._20 != 2 && mInfo._20 != 3)
                    {
                        GameAudio::Main::getAudio()->startSystemSe(0x2000c);
                        mInfo.engineType = (mInfo.engineType + 3) % 4;
                        if (!sysRecord->isGameUnlocked(SystemRecord::Mirror) && mInfo.engineType == 3)
                            mInfo.engineType = 2;
                    }
                }
                else if ((repeat & JUTGamePad::MAINSTICK_RIGHT) && _2108[0])
                {
                    mModeBck[20].mFrame = 21.0f;
                    mModeBck[27].mFrame = 21.0f;
                    mModeBck[68].mFrame = 21.0f;
                    mModeBck[25].mFrame = 21.0f;
                    mModeBck[28].mFrame = 21.0f;
                    mModeBck[69].mFrame = 21.0f;
                    mModeBck[24].mFrame = 21.0f;
                    mModeBrk[11].mFrame = 1.0f;
                    mModeBck[30].mFrame = 21.0f;
                    mModeBrk[13].mFrame = 1.0f;
                    mModeBck[65].mFrame = 21.0f;
                    mModeBrk[28].mFrame = 1.0f;
                    if (mInfo._20 != 2 && mInfo._20 != 3)
                    {
                        GameAudio::Main::getAudio()->startSystemSe(0x2000c);
                        mInfo.engineType = (mInfo.engineType + 1) % 4;
                        if (!sysRecord->isGameUnlocked(SystemRecord::Mirror) && mInfo.engineType == 3)
                            mInfo.engineType = 0;
                    }
                }
            }
        }
    }

    if (mInfo._20 == 2 && _2108[0] && !_2108[1])
    {
        switch (mNameEntry->mAction)
        {
        case 3:
            _2108[1] = 1;
            _2110 = 1;
            break;
        case 4:
            _2108[0] = 0;
            _210c = 1;
            break;
        }
    }
    if (_210c != 0)
    {
        if (++_210c > 15)
        {
            _210c = 0;
            if (mInfo._20 == 2 && _2108[0] && !_2108[1] && mNameEntry->mAction != 2)
                mNameEntry->startEdit();
            if (mInfo._20 == 2 && !_2108[0] && !_2108[1])
                mNameEntry->init(mName);
        }
    }
    if (_2110 != 0)
    {
        if (++_2110 > 10)
        {
            _2110 = 0;
            if (mInfo._20 == 2 && _2108[0] && !_2108[1] && mNameEntry->mAction != 2)
                mNameEntry->startEdit();
            if (mInfo._20 == 2 && !_2108[0] && !_2108[1])
                mNameEntry->init(mName);
        }
    }
    if (_2114 != 0)
    {
        if (++_2114 > 10)
        {
            _2114 = 0;
            if (mInfo._20 == 2 && _2108[0] && !_2108[1] && mNameEntry->mAction != 2)
                mNameEntry->startEdit();
            if (mInfo._20 == 2 && !_2108[0] && !_2108[1])
                mNameEntry->init(mName);
        }
    }

    switch (mInfo.entryNum)
    {
    case 0:
        switch (mInfo._20)
        {
        case 0:
            if ((mModeBck[34].mFrame += 1.0f) > 17.0f)
                mModeBck[34].mFrame = 17.0f;
            if ((mModeBck[35].mFrame -= 1.0f) < 10.0f)
                mModeBck[35].mFrame = 10.0f;
            if (_2108[0])
            {
                if ((mModeBck[61].mFrame += 1.0f) > 19.0f)
                    mModeBck[61].mFrame = 19.0f;
            }
            else
            {
                if ((mModeBck[61].mFrame -= 1.0f) < 4.0f)
                    mModeBck[61].mFrame = 4.0f;
            }
            if ((mModeBck[70].mFrame -= 1.0f) < 4.0f)
                mModeBck[70].mFrame = 4.0f;
            if ((mModeBck[44].mFrame += 1.0f) > 19.0f)
                mModeBck[44].mFrame = 19.0f;
            if ((mModeBck[45].mFrame += 1.0f) >= 70.0f)
                mModeBck[45].mFrame = 10.0f;
            if ((mModeBck[46].mFrame -= 1.0f) < 14.0f)
                mModeBck[46].mFrame = 14.0f;
            mModeBck[47].mFrame = 10.0f;
            mModeBck[39].mFrame = 9.0f;
            mModeBck[40].mFrame = 10.0f;
            mModeBck[41].mFrame = 10.0f;
            mModeBck[42].mFrame = 10.0f;
            mModeBck[43].mFrame = 10.0f;
            if ((mModeBtk[14].mFrame += 1.0f) >= 100.0f)
                mModeBtk[14].mFrame = 0.0f;
            if ((mModeBtk[15].mFrame += 1.0f) >= 50.0f)
                mModeBtk[15].mFrame = 0.0f;
            mModeBrk[20].mFrame = 1.0f;
            mModeBrk[21].mFrame = 0.0f;
            mModeBrk[16].mFrame = 1.0f;
            mModeBrk[17].mFrame = 1.0f;
            mModeBrk[18].mFrame = 0.0f;
            mModeBrk[19].mFrame = 0.0f;
            mModeBpk[5].mFrame = 10.0f;
            break;
        case 2:
            if ((mModeBck[34].mFrame -= 1.0f) < 9.0f)
                mModeBck[34].mFrame = 9.0f;
            if ((mModeBck[35].mFrame += 1.0f) > 17.0f)
                mModeBck[35].mFrame = 17.0f;
            if ((mModeBck[61].mFrame -= 1.0f) < 4.0f)
                mModeBck[61].mFrame = 4.0f;
            if (_2108[0])
            {
                if ((mModeBck[70].mFrame += 1.0f) > 19.0f)
                    mModeBck[70].mFrame = 19.0f;
            }
            else
            {
                if ((mModeBck[70].mFrame -= 1.0f) < 4.0f)
                    mModeBck[70].mFrame = 4.0f;
            }
            if (_2108[1])
            {
                if ((mModeBck[74].mFrame += 1.0f) > 30.0f)
                    mModeBck[74].mFrame = 30.0f;
            }
            else
            {
                if ((mModeBck[74].mFrame -= 1.0f) < 20.0f)
                    mModeBck[74].mFrame = 20.0f;
            }
            if ((mModeBck[44].mFrame -= 1.0f) < 14.0f)
                mModeBck[44].mFrame = 14.0f;
            mModeBck[45].mFrame = 10.0f;
            if ((mModeBck[46].mFrame += 1.0f) > 19.0f)
                mModeBck[46].mFrame = 19.0f;
            if ((mModeBck[47].mFrame += 1.0f) >= 70.0f)
                mModeBck[47].mFrame = 10.0f;
            if ((mModeBck[39].mFrame += 1.0f) >= 70.0f)
                mModeBck[39].mFrame = 10.0f;
            if ((mModeBck[40].mFrame += 1.0f) >= 70.0f)
                mModeBck[40].mFrame = 10.0f;
            if ((mModeBck[41].mFrame += 1.0f) >= 70.0f)
                mModeBck[41].mFrame = 10.0f;
            if ((mModeBck[42].mFrame += 1.0f) >= 70.0f)
                mModeBck[42].mFrame = 10.0f;
            if ((mModeBck[43].mFrame += 1.0f) >= 70.0f)
                mModeBck[43].mFrame = 10.0f;
            mModeBtk[14].mFrame = 0.0f;
            mModeBtk[15].mFrame = 0.0f;
            mModeBrk[20].mFrame = 0.0f;
            mModeBrk[21].mFrame = 1.0f;
            mModeBrk[16].mFrame = 0.0f;
            mModeBrk[17].mFrame = 0.0f;
            mModeBrk[18].mFrame = 1.0f;
            mModeBrk[19].mFrame = 1.0f;
            if ((mModeBpk[5].mFrame += 1.0f) >= 70.0f)
                mModeBpk[5].mFrame = 10.0f;
            break;
        case 1:
        default:
#line 4851
            JUT_ASSERT(0);
        }
        if (_2108[0])
        {
            if ((mModeBck[37].mFrame += 1.0f) > 265.0f)
                mModeBck[37].mFrame = 265.0f;
            if ((mModeBck[38].mFrame += 1.0f) > 265.0f)
                mModeBck[38].mFrame = 265.0f;
        }
        else
        {
            if ((mModeBck[37].mFrame -= 1.0f) < 250.0f)
                mModeBck[37].mFrame = 250.0f;
            if ((mModeBck[38].mFrame -= 1.0f) < 250.0f)
                mModeBck[38].mFrame = 250.0f;
        }
        break;
    case 1:
        switch (mInfo._20)
        {
        case 0:
            if ((mModeBck[1].mFrame += 1.0f) > 17.0f)
                mModeBck[1].mFrame = 17.0f;
            if ((mModeBck[2].mFrame -= 1.0f) < 9.0f)
                mModeBck[2].mFrame = 9.0f;
            if ((mModeBck[3].mFrame -= 1.0f) < 9.0f)
                mModeBck[3].mFrame = 9.0f;
            if (_2108[0])
            {
                if ((mModeBck[19].mFrame += 1.0f) > 19.0f)
                    mModeBck[19].mFrame = 19.0f;
            }
            else
            {
                if ((mModeBck[19].mFrame -= 1.0f) < 4.0f)
                    mModeBck[19].mFrame = 4.0f;
            }
            if ((mModeBck[26].mFrame -= 1.0f) < 4.0f)
                mModeBck[26].mFrame = 4.0f;
            mModeBck[6].mFrame = 10.0f;
            mModeBck[7].mFrame = 10.0f;
            mModeBck[10].mFrame = 10.0f;
            mModeBck[11].mFrame = 10.0f;
            mModeBck[12].mFrame = 10.0f;
            if ((mModeBck[13].mFrame += 1.0f) > 19.0f)
                mModeBck[13].mFrame = 19.0f;
            if ((mModeBck[14].mFrame += 1.0f) >= 70.0f)
                mModeBck[14].mFrame = 10.0f;
            if ((mModeBck[15].mFrame -= 1.0f) < 14.0f)
                mModeBck[15].mFrame = 14.0f;
            mModeBck[16].mFrame = 10.0f;
            if ((mModeBck[17].mFrame -= 1.0f) < 14.0f)
                mModeBck[17].mFrame = 14.0f;
            mModeBck[18].mFrame = 10.0f;
            if ((mModeBtk[3].mFrame += 1.0f) >= 100.0f)
                mModeBtk[3].mFrame = 0.0f;
            if ((mModeBtk[4].mFrame += 1.0f) >= 50.0f)
                mModeBtk[4].mFrame = 0.0f;
            mModeBtk[5].mFrame = 0.0f;
            mModeBtk[6].mFrame = 0.0f;
            mModeBtk[7].mFrame = 0.0f;
            mModeBtk[8].mFrame = 0.0f;
            mModeBtk[9].mFrame = 0.0f;
            if ((mModeBtk[10].mFrame += 1.0f) >= 120.0f)
                mModeBtk[10].mFrame = 0.0f;
            if ((mModeBtk[11].mFrame += 1.0f) >= 120.0f)
                mModeBtk[11].mFrame = 0.0f;
            mModeBtk[12].mFrame = 0.0f;
            mModeBtk[13].mFrame = 0.0f;
            mModeBrk[0].mFrame = 1.0f;
            mModeBrk[1].mFrame = 0.0f;
            mModeBrk[2].mFrame = 0.0f;
            mModeBrk[4].mFrame = 1.0f;
            mModeBrk[5].mFrame = 1.0f;
            mModeBrk[6].mFrame = 0.0f;
            mModeBrk[7].mFrame = 0.0f;
            mModeBrk[8].mFrame = 0.0f;
            mModeBrk[9].mFrame = 0.0f;
            mModeBrk[3].mFrame = 0.0f;
            break;
        case 1:
            if ((mModeBck[1].mFrame -= 1.0f) < 9.0f)
                mModeBck[1].mFrame = 9.0f;
            if ((mModeBck[2].mFrame += 1.0f) > 17.0f)
                mModeBck[2].mFrame = 17.0f;
            if ((mModeBck[3].mFrame -= 1.0f) < 9.0f)
                mModeBck[3].mFrame = 9.0f;
            if ((mModeBck[19].mFrame -= 1.0f) < 4.0f)
                mModeBck[19].mFrame = 4.0f;
            if (_2108[0])
            {
                if ((mModeBck[26].mFrame += 1.0f) > 19.0f)
                    mModeBck[26].mFrame = 19.0f;
            }
            else
            {
                if ((mModeBck[26].mFrame -= 1.0f) < 4.0f)
                    mModeBck[26].mFrame = 4.0f;
            }
            if ((mModeBck[6].mFrame += 1.0f) >= 60.0f)
                mModeBck[6].mFrame = 10.0f;
            if ((mModeBck[7].mFrame += 1.0f) >= 60.0f)
                mModeBck[7].mFrame = 10.0f;
            mModeBck[10].mFrame = 10.0f;
            mModeBck[11].mFrame = 10.0f;
            mModeBck[12].mFrame = 10.0f;
            if ((mModeBck[13].mFrame -= 1.0f) < 14.0f)
                mModeBck[13].mFrame = 14.0f;
            mModeBck[14].mFrame = 10.0f;
            if ((mModeBck[15].mFrame += 1.0f) > 19.0f)
                mModeBck[15].mFrame = 19.0f;
            if ((mModeBck[16].mFrame += 1.0f) >= 70.0f)
                mModeBck[16].mFrame = 10.0f;
            if ((mModeBck[17].mFrame -= 1.0f) < 14.0f)
                mModeBck[17].mFrame = 14.0f;
            mModeBck[18].mFrame = 10.0f;
            mModeBtk[3].mFrame = 0.0f;
            mModeBtk[4].mFrame = 0.0f;
            if ((mModeBtk[5].mFrame += 1.0f) >= 180.0f)
                mModeBtk[5].mFrame = 0.0f;
            if ((mModeBtk[6].mFrame += 1.0f) >= 180.0f)
                mModeBtk[6].mFrame = 0.0f;
            if ((mModeBtk[7].mFrame += 1.0f) >= 180.0f)
                mModeBtk[7].mFrame = 0.0f;
            if ((mModeBtk[8].mFrame += 1.0f) >= 180.0f)
                mModeBtk[8].mFrame = 0.0f;
            mModeBtk[9].mFrame = 0.0f;
            mModeBtk[10].mFrame = 0.0f;
            mModeBtk[11].mFrame = 0.0f;
            if ((mModeBtk[12].mFrame += 1.0f) >= 120.0f)
                mModeBtk[12].mFrame = 0.0f;
            if ((mModeBtk[13].mFrame += 1.0f) >= 120.0f)
                mModeBtk[13].mFrame = 0.0f;
            mModeBrk[0].mFrame = 0.0f;
            mModeBrk[1].mFrame = 1.0f;
            mModeBrk[2].mFrame = 0.0f;
            mModeBrk[4].mFrame = 0.0f;
            mModeBrk[5].mFrame = 0.0f;
            mModeBrk[6].mFrame = 1.0f;
            mModeBrk[7].mFrame = 1.0f;
            mModeBrk[8].mFrame = 0.0f;
            mModeBrk[9].mFrame = 0.0f;
            mModeBrk[3].mFrame = 0.0f;
            break;
        case 3:
            if ((mModeBck[1].mFrame -= 1.0f) < 9.0f)
                mModeBck[1].mFrame = 9.0f;
            if ((mModeBck[2].mFrame -= 1.0f) < 9.0f)
                mModeBck[2].mFrame = 9.0f;
            if ((mModeBck[3].mFrame += 1.0f) > 17.0f)
                mModeBck[3].mFrame = 17.0f;
            if ((mModeBck[19].mFrame -= 1.0f) < 4.0f)
                mModeBck[19].mFrame = 4.0f;
            if ((mModeBck[26].mFrame -= 1.0f) < 4.0f)
                mModeBck[26].mFrame = 4.0f;
            mModeBck[6].mFrame = 10.0f;
            mModeBck[7].mFrame = 10.0f;
            if ((mModeBck[10].mFrame += 1.0f) >= 250.0f)
                mModeBck[10].mFrame = 10.0f;
            if ((mModeBck[11].mFrame += 1.0f) >= 250.0f)
                mModeBck[11].mFrame = 10.0f;
            if ((mModeBck[12].mFrame += 1.0f) >= 250.0f)
                mModeBck[12].mFrame = 10.0f;
            if ((mModeBck[13].mFrame -= 1.0f) < 14.0f)
                mModeBck[13].mFrame = 14.0f;
            mModeBck[14].mFrame = 10.0f;
            if ((mModeBck[15].mFrame -= 1.0f) < 14.0f)
                mModeBck[15].mFrame = 14.0f;
            mModeBck[16].mFrame = 10.0f;
            if ((mModeBck[17].mFrame += 1.0f) > 19.0f)
                mModeBck[17].mFrame = 19.0f;
            if ((mModeBck[18].mFrame += 1.0f) >= 70.0f)
                mModeBck[18].mFrame = 10.0f;
            mModeBtk[3].mFrame = 0.0f;
            mModeBtk[4].mFrame = 0.0f;
            mModeBtk[5].mFrame = 0.0f;
            mModeBtk[6].mFrame = 0.0f;
            mModeBtk[7].mFrame = 0.0f;
            mModeBtk[8].mFrame = 0.0f;
            if ((mModeBtk[9].mFrame += 1.0f) >= 260.0f)
                mModeBtk[9].mFrame = 0.0f;
            mModeBtk[10].mFrame = 0.0f;
            mModeBtk[11].mFrame = 0.0f;
            mModeBtk[12].mFrame = 0.0f;
            mModeBtk[13].mFrame = 0.0f;
            mModeBrk[0].mFrame = 0.0f;
            mModeBrk[1].mFrame = 0.0f;
            mModeBrk[2].mFrame = 1.0f;
            mModeBrk[4].mFrame = 0.0f;
            mModeBrk[5].mFrame = 0.0f;
            mModeBrk[6].mFrame = 0.0f;
            mModeBrk[7].mFrame = 0.0f;
            mModeBrk[8].mFrame = 1.0f;
            mModeBrk[9].mFrame = 1.0f;
            mModeBrk[3].mFrame = 1.0f;
            break;
        case 2:
        default:
#line 5005
            JUT_ASSERT(0);
        }
        if (_2108[0])
        {
            if ((mModeBck[4].mFrame += 1.0f) > 265.0f)
                mModeBck[4].mFrame = 265.0f;
            if ((mModeBck[5].mFrame += 1.0f) > 265.0f)
                mModeBck[5].mFrame = 265.0f;
            if (mInfo._20 == 3)
            {
                if ((mModeBck[8].mFrame -= 1.0f) < 250.0f)
                    mModeBck[8].mFrame = 250.0f;
            }
            else
            {
                if ((mModeBck[8].mFrame += 1.0f) > 265.0f)
                    mModeBck[8].mFrame = 265.0f;
            }
        }
        else
        {
            if ((mModeBck[4].mFrame -= 1.0f) < 250.0f)
                mModeBck[4].mFrame = 250.0f;
            if ((mModeBck[5].mFrame -= 1.0f) < 250.0f)
                mModeBck[5].mFrame = 250.0f;
            if ((mModeBck[8].mFrame -= 1.0f) < 250.0f)
                mModeBck[8].mFrame = 250.0f;
        }
        break;
    case 2:
    case 3:
        switch (mInfo._20)
        {
        case 1:
            if ((mModeBck[34].mFrame += 1.0f) > 17.0f)
                mModeBck[34].mFrame = 17.0f;
            if ((mModeBck[35].mFrame -= 1.0f) < 9.0f)
                mModeBck[35].mFrame = 9.0f;
            if (_2108[0])
            {
                if ((mModeBck[61].mFrame += 1.0f) > 19.0f)
                    mModeBck[61].mFrame = 19.0f;
            }
            else
            {
                if ((mModeBck[61].mFrame -= 1.0f) < 4.0f)
                    mModeBck[61].mFrame = 4.0f;
            }
            if ((mModeBck[57].mFrame += 1.0f) > 19.0f)
                mModeBck[57].mFrame = 19.0f;
            if ((mModeBck[58].mFrame += 1.0f) >= 70.0f)
                mModeBck[58].mFrame = 10.0f;
            if ((mModeBck[50].mFrame += 1.0f) >= 60.0f)
                mModeBck[50].mFrame = 10.0f;
            if ((mModeBck[51].mFrame += 1.0f) >= 60.0f)
                mModeBck[51].mFrame = 10.0f;
            if ((mModeBck[59].mFrame -= 1.0f) < 14.0f)
                mModeBck[59].mFrame = 14.0f;
            mModeBck[60].mFrame = 10.0f;
            mModeBck[54].mFrame = 10.0f;
            mModeBck[55].mFrame = 10.0f;
            mModeBck[56].mFrame = 10.0f;
            if ((mModeBtk[19].mFrame += 1.0f) >= 180.0f)
                mModeBtk[19].mFrame = 0.0f;
            if ((mModeBtk[20].mFrame += 1.0f) >= 180.0f)
                mModeBtk[20].mFrame = 0.0f;
            if ((mModeBtk[21].mFrame += 1.0f) >= 180.0f)
                mModeBtk[21].mFrame = 0.0f;
            if ((mModeBtk[22].mFrame += 1.0f) >= 180.0f)
                mModeBtk[22].mFrame = 0.0f;
            if ((mModeBtk[23].mFrame += 1.0f) >= 120.0f)
                mModeBtk[23].mFrame = 0.0f;
            if ((mModeBtk[24].mFrame += 1.0f) >= 120.0f)
                mModeBtk[24].mFrame = 0.0f;
            mModeBtk[18].mFrame = 0.0f;
            mModeBrk[20].mFrame = 1.0f;
            mModeBrk[21].mFrame = 0.0f;
            mModeBrk[23].mFrame = 1.0f;
            mModeBrk[24].mFrame = 1.0f;
            mModeBrk[25].mFrame = 0.0f;
            mModeBrk[26].mFrame = 0.0f;
            mModeBrk[22].mFrame = 0.0f;
            break;
        case 3:
            if ((mModeBck[34].mFrame -= 1.0f) < 9.0f)
                mModeBck[34].mFrame = 9.0f;
            if ((mModeBck[35].mFrame += 1.0f) > 17.0f)
                mModeBck[35].mFrame = 17.0f;
            if ((mModeBck[61].mFrame -= 1.0f) < 4.0f)
                mModeBck[61].mFrame = 4.0f;
            if ((mModeBck[57].mFrame -= 1.0f) < 14.0f)
                mModeBck[57].mFrame = 14.0f;
            mModeBck[58].mFrame = 10.0f;
            mModeBck[50].mFrame = 10.0f;
            mModeBck[51].mFrame = 10.0f;
            if ((mModeBck[59].mFrame += 1.0f) > 19.0f)
                mModeBck[59].mFrame = 19.0f;
            if ((mModeBck[60].mFrame += 1.0f) >= 70.0f)
                mModeBck[60].mFrame = 10.0f;
            if ((mModeBck[54].mFrame += 1.0f) >= 250.0f)
                mModeBck[54].mFrame = 10.0f;
            if ((mModeBck[55].mFrame += 1.0f) >= 250.0f)
                mModeBck[55].mFrame = 10.0f;
            if ((mModeBck[56].mFrame += 1.0f) >= 250.0f)
                mModeBck[56].mFrame = 10.0f;
            mModeBtk[19].mFrame = 0.0f;
            mModeBtk[20].mFrame = 0.0f;
            mModeBtk[21].mFrame = 0.0f;
            mModeBtk[22].mFrame = 0.0f;
            mModeBtk[23].mFrame = 0.0f;
            mModeBtk[24].mFrame = 0.0f;
            if ((mModeBtk[18].mFrame += 1.0f) >= 260.0f)
                mModeBtk[18].mFrame = 0.0f;
            mModeBrk[20].mFrame = 0.0f;
            mModeBrk[21].mFrame = 1.0f;
            mModeBrk[23].mFrame = 0.0f;
            mModeBrk[24].mFrame = 0.0f;
            mModeBrk[25].mFrame = 1.0f;
            mModeBrk[26].mFrame = 1.0f;
            mModeBrk[22].mFrame = 1.0f;
            break;
        case 2:
        default:
#line 5096
            JUT_ASSERT(0);
        }
        if (_2108[0])
        {
            if ((mModeBck[49].mFrame += 1.0f) > 265.0f)
                mModeBck[49].mFrame = 265.0f;
            if (mInfo._20 == 3)
            {
                if ((mModeBck[52].mFrame -= 1.0f) < 250.0f)
                    mModeBck[52].mFrame = 250.0f;
            }
            else
            {
                if ((mModeBck[52].mFrame += 1.0f) > 265.0f)
                    mModeBck[52].mFrame = 265.0f;
            }
        }
        else
        {
            if ((mModeBck[49].mFrame -= 1.0f) < 250.0f)
                mModeBck[49].mFrame = 250.0f;
            if ((mModeBck[52].mFrame -= 1.0f) < 250.0f)
                mModeBck[52].mFrame = 250.0f;
        }
        break;
    default:
#line 5113
        JUT_ASSERT(0);
    }
}

void SceneMenu::mode_end()
{
    if (mInfo.timer < 15)
    {
        MenuTitleLine::ptr()->lift();
        mIconBck[0].mFrame = mInfo.timer + 75;
        switch (mInfo.entryNum)
        {
        case 0:
            mModeBck[33].mFrame = mInfo.timer + 75;
            mModeBck[36].mFrame = mInfo.timer + 75;
        case 1:
            mModeBck[0].mFrame = mInfo.timer + 75;
            break;
        case 2:
        case 3:
            mModeBck[33].mFrame = mInfo.timer + 75;
            mModeBck[48].mFrame = mInfo.timer + 75;
            break;
        default:
#line 5145
            JUT_ASSERT(0);
        }

        switch (mInfo.entryNum)
        {
        case 0:
            switch (mInfo._20)
            {
            case 0:
            case 1:
            case 3:
                if ((mModeBck[61].mFrame -= 1.0f) < 4.0f)
                    mModeBck[61].mFrame = 4.0f;
                break;
            case 2:
                if ((mModeBck[70].mFrame -= 1.0f) < 4.0f)
                    mModeBck[70].mFrame = 4.0f;
                break;
            default:
#line 5160
                JUT_ASSERT(0);
            }
            break;
        case 1:
            switch (mInfo._20)
            {
            case 0:
            case 2:
                if ((mModeBck[19].mFrame -= 1.0f) < 4.0f)
                    mModeBck[19].mFrame = 4.0f;
                break;
            case 1:
                if ((mModeBck[26].mFrame -= 1.0f) < 4.0f)
                    mModeBck[26].mFrame = 4.0f;
                break;
            case 3:
                break;
            default:
#line 5176
                JUT_ASSERT(0);
            }
            break;
        case 2:
        case 3:
            switch (mInfo._20)
            {
            case 0:
            case 1:
            case 2:
                if ((mModeBck[61].mFrame -= 1.0f) < 4.0f)
                    mModeBck[61].mFrame = 4.0f;
                break;
            case 3:
                break;
            default:
#line 5191
                JUT_ASSERT(0);
            }
            break;
        default:
#line 5196
            JUT_ASSERT(0);
        }

        switch (mInfo.playerNum)
        {
        case 0:
            mIconBck[3].mFrame = mInfo.timer + 75;
            mIconBck[15].mFrame = mInfo.timer + 75;
            break;
        case 1:
            switch (mInfo._18)
            {
            case 0:
                mIconBck[3].mFrame = mInfo.timer + 75;
                mIconBck[15].mFrame = mInfo.timer + 75;
                mIconBck[16].mFrame = mInfo.timer + 75;
                mIconBck[27].mFrame = mInfo.timer + 75;
                break;
            case 1:
                mIconBck[4].mFrame = mInfo.timer + 75;
                mIconBck[19].mFrame = mInfo.timer + 75;
                mIconBck[20].mFrame = mInfo.timer + 75;
                break;
            default:
#line 5220
                JUT_ASSERT(0);
            }
            break;
        case 2:
            switch (mInfo._18)
            {
            case 0:
                mIconBck[3].mFrame = mInfo.timer + 75;
                mIconBck[15].mFrame = mInfo.timer + 75;
                mIconBck[16].mFrame = mInfo.timer + 75;
                mIconBck[17].mFrame = mInfo.timer + 75;
                mIconBck[27].mFrame = mInfo.timer + 75;
                mIconBck[28].mFrame = mInfo.timer + 75;
                break;
            case 1:
                mIconBck[3].mFrame = mInfo.timer + 75;
                mIconBck[15].mFrame = mInfo.timer + 75;
                mIconBck[16].mFrame = mInfo.timer + 75;
                mIconBck[17].mFrame = mInfo.timer + 75;
                mIconBck[27].mFrame = mInfo.timer + 75;
                mIconBck[28].mFrame = mInfo.timer + 75;
                break;
            default:
#line 5243
                JUT_ASSERT(0);
            }
            break;
        case 3:
            switch (mInfo._18)
            {
            case 0:
                mIconBck[3].mFrame = mInfo.timer + 75;
                mIconBck[15].mFrame = mInfo.timer + 75;
                mIconBck[16].mFrame = mInfo.timer + 75;
                mIconBck[17].mFrame = mInfo.timer + 75;
                mIconBck[18].mFrame = mInfo.timer + 75;
                mIconBck[27].mFrame = mInfo.timer + 75;
                mIconBck[28].mFrame = mInfo.timer + 75;
                mIconBck[29].mFrame = mInfo.timer + 75;
                break;
            case 1:
                mIconBck[5].mFrame = mInfo.timer + 75;
                mIconBck[23].mFrame = mInfo.timer + 75;
                mIconBck[24].mFrame = mInfo.timer + 75;
                mIconBck[25].mFrame = mInfo.timer + 75;
                mIconBck[26].mFrame = mInfo.timer + 75;
                mIconBck[32].mFrame = mInfo.timer + 75;
                break;
            case 2:
                mIconBck[4].mFrame = mInfo.timer + 75;
                mIconBck[19].mFrame = mInfo.timer + 75;
                mIconBck[20].mFrame = mInfo.timer + 75;
                mIconBck[21].mFrame = mInfo.timer + 75;
                mIconBck[22].mFrame = mInfo.timer + 75;
                mIconBck[30].mFrame = mInfo.timer + 75;
                mIconBck[31].mFrame = mInfo.timer + 75;
                break;
            default:
#line 5277
                JUT_ASSERT(0);
            }
            break;
        default:
#line 5282
            JUT_ASSERT(0);
        }
    }
    else
    {
        mIconBck[0].mPane->hide();
        switch (mInfo.entryNum)
        {
        case 0:
        case 2:
        case 3:
            mModeBck[33].mPane->hide();
            mModeBck[61].mPane->hide();
            mModeBck[34].mPane->hide();
            mModeBck[35].mPane->hide();
            switch (mInfo.entryNum)
            {
            case 0:
                mModeBck[36].mPane->hide();
                mModeBck[37].mPane->hide();
                mModeBck[38].mPane->hide();
                mModeBck[70].mPane->hide();
                break;
            case 2:
            case 3:
                mModeBck[48].mPane->hide();
                mModeBck[49].mPane->hide();
                mModeBck[52].mPane->hide();
                break;
            default:
#line 5311
                JUT_ASSERT(0);
            }
            break;
        case 1:
            mModeBck[0].mPane->hide();
            mModeBck[19].mPane->hide();
            mModeBck[26].mPane->hide();
            mModeBck[1].mPane->hide();
            mModeBck[2].mPane->hide();
            mModeBck[3].mPane->hide();
            break;
        default:
#line 5324
            JUT_ASSERT(0);
        }

        switch (mInfo.playerNum)
        {
        case 0:
            mIconBck[3].mPane->hide();
            mIconBck[6].mPane->hide();
            mIconBck[7].mPane->hide();
            mIconBck[8].mPane->hide();
            mIconBck[9].mPane->hide();
            mIconBck[15].mPane->hide();
            mIconBck[16].mPane->hide();
            mIconBck[17].mPane->hide();
            mIconBck[18].mPane->hide();
            mIconBck[27].mPane->hide();
            mIconBck[28].mPane->hide();
            mIconBck[29].mPane->hide();
            break;
        case 1:
            switch (mInfo._18)
            {
            case 0:
                mIconBck[3].mPane->hide();
                mIconBck[6].mPane->hide();
                mIconBck[7].mPane->hide();
                mIconBck[8].mPane->hide();
                mIconBck[9].mPane->hide();
                mIconBck[15].mPane->hide();
                mIconBck[16].mPane->hide();
                mIconBck[17].mPane->hide();
                mIconBck[18].mPane->hide();
                mIconBck[27].mPane->hide();
                mIconBck[28].mPane->hide();
                mIconBck[29].mPane->hide();
                break;
            case 1:
                mIconBck[4].mPane->hide();
                mIconBck[10].mPane->hide();
                mIconBck[11].mPane->hide();
                mIconBck[12].mPane->hide();
                mIconBck[19].mPane->hide();
                mIconBck[20].mPane->hide();
                mIconBck[21].mPane->hide();
                mIconBck[22].mPane->hide();
                mIconBck[30].mPane->hide();
                mIconBck[31].mPane->hide();
                break;
            default:
#line 5373
                JUT_ASSERT(0);
            }
            break;
        case 2:
            switch (mInfo._18)
            {
            case 0:
                mIconBck[3].mPane->hide();
                mIconBck[6].mPane->hide();
                mIconBck[7].mPane->hide();
                mIconBck[8].mPane->hide();
                mIconBck[9].mPane->hide();
                mIconBck[15].mPane->hide();
                mIconBck[16].mPane->hide();
                mIconBck[17].mPane->hide();
                mIconBck[18].mPane->hide();
                mIconBck[27].mPane->hide();
                mIconBck[28].mPane->hide();
                mIconBck[29].mPane->hide();
                break;
            case 1:
                mIconBck[4].mPane->hide();
                mIconBck[10].mPane->hide();
                mIconBck[11].mPane->hide();
                mIconBck[12].mPane->hide();
                mIconBck[19].mPane->hide();
                mIconBck[20].mPane->hide();
                mIconBck[21].mPane->hide();
                mIconBck[22].mPane->hide();
                mIconBck[30].mPane->hide();
                mIconBck[31].mPane->hide();
                break;
            default:
#line 5406
                JUT_ASSERT(0);
            }
            break;
        case 3:
            switch (mInfo._18)
            {
            case 0:
                mIconBck[3].mPane->hide();
                mIconBck[6].mPane->hide();
                mIconBck[7].mPane->hide();
                mIconBck[8].mPane->hide();
                mIconBck[9].mPane->hide();
                mIconBck[15].mPane->hide();
                mIconBck[16].mPane->hide();
                mIconBck[17].mPane->hide();
                mIconBck[18].mPane->hide();
                mIconBck[27].mPane->hide();
                mIconBck[28].mPane->hide();
                mIconBck[29].mPane->hide();
                break;
            case 1:
                mIconBck[5].mPane->hide();
                mIconBck[13].mPane->hide();
                mIconBck[14].mPane->hide();
                mIconBck[23].mPane->hide();
                mIconBck[24].mPane->hide();
                mIconBck[25].mPane->hide();
                mIconBck[26].mPane->hide();
                mIconBck[32].mPane->hide();
                break;
            case 2:
                mIconBck[4].mPane->hide();
                mIconBck[10].mPane->hide();
                mIconBck[11].mPane->hide();
                mIconBck[12].mPane->hide();
                mIconBck[19].mPane->hide();
                mIconBck[20].mPane->hide();
                mIconBck[21].mPane->hide();
                mIconBck[22].mPane->hide();
                mIconBck[30].mPane->hide();
                mIconBck[31].mPane->hide();
                break;
            default:
#line 5449
                JUT_ASSERT(0);
            }
            break;
        default:
#line 5454
            JUT_ASSERT(0);
        }
        mInfo.oldMode = mInfo.mode;
        mInfo.mode = mInfo.nextMode;
        mInfo.timer = 0;
    }
}

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
                static_cast<J2DPicture *>(pane)->setWhite(mBarWhite[0]);
                static_cast<J2DPicture *>(pane)->setBlack(mBarBlack[0]);

                pane = mBattleBck[7 + i].mPane;
#line 5496
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                static_cast<J2DPicture *>(pane)->setWhite(mWordWhite[0]);

                pane = mBattleBck[10 + i].mPane;
#line 5500
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                static_cast<J2DPicture *>(pane)->setWhite(mBtlPWhite[0]);
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
                static_cast<J2DPicture *>(pane)->setWhite(mBarWhite[1]);
                static_cast<J2DPicture *>(pane)->setBlack(mBarBlack[1]);

                pane = mBattleBck[7 + i].mPane;
#line 5515
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                static_cast<J2DPicture *>(pane)->setWhite(mWordWhite[1]);

                pane = mBattleBck[10 + i].mPane;
#line 5519
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                static_cast<J2DPicture *>(pane)->setWhite(mBtlPWhite[1]);
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
            static_cast<J2DPicture *>(pane)->setWhite(mBarWhite[0]);
            static_cast<J2DPicture *>(pane)->setBlack(mBarBlack[0]);

            pane = mBattleBck[7 + i].mPane;
#line 5592
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->setWhite(mWordWhite[0]);

            pane = mBattleBck[10 + i].mPane;
#line 5597
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->setWhite(mBtlPWhite[0]);
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
            static_cast<J2DPicture *>(pane)->setWhite(mBarWhite[1]);
            static_cast<J2DPicture *>(pane)->setBlack(mBarBlack[1]);

            pane = mBattleBck[7 + i].mPane;
#line 5614
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->setWhite(mWordWhite[1]);

            pane = mBattleBck[10 + i].mPane;
#line 5619
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            static_cast<J2DPicture *>(pane)->setWhite(mBtlPWhite[1]);
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

void SceneMenu::character_init()
{
    if (mInfo.timer < 25)
    {
        MenuTitleLine::ptr()->drop(MenuTitleLine::mcCharacterSelect);
        if (mInfo.timer < 19)
        {
            for (int i = 0; i < 10; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    mCharPane[i][j].mPane->show();
                    mCharPane[i][j].mFrame = mInfo.timer;
                }
            }
        }
        if (mInfo.timer < 20)
        {
            switch (mInfo.entryNum)
            {
            case 0:
                for (int i = 0; i < 4; i++)
                    mCharPaneBck[i].mPane->show();
                for (int i = 0; i < 4; i++)
                    mCharPaneBck[i].mFrame = mInfo.timer;
                break;
            case 1:
                for (int i = 0; i < 7; i++)
                    mChar2Bck[i].mPane->show();
                for (int i = 0; i < 7; i++)
                    mChar2Bck[i].mFrame = mInfo.timer;
                break;
            case 2:
                for (int i = 0; i < 8; i++)
                    mChar3Bck[i].mPane->show();
                for (int i = 0; i < 8; i++)
                    mChar3Bck[i].mFrame = mInfo.timer;
                break;
            case 3:
                for (int i = 0; i < 9; i++)
                    mChar4Bck[i].mPane->show();
                for (int i = 0; i < 9; i++)
                    mChar4Bck[i].mFrame = mInfo.timer;
                break;
            default:
#line 5723
                JUT_ASSERT(0);
            }
        }

        J2DPicture *pane;
        switch (mInfo.playerNum)
        {
        case 0:
            mCharPaneBck[2].mPane->hide();
            if (mPlayerData[0].mState == 0)
            {
                mPlayerPadNo[0] = 0;
                mPlayerData[0].mCharNoF = 0;
                mPlayerData[0].mCharNoB = 0;
            }
            break;
        case 1:
            switch (mInfo._18)
            {
            case 0:
                mChar2Bck[2].mPane->hide();
                pane = (J2DPicture *)mChar2Bck[3].mPane;
#line 5745
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                pane->changeTexture(mCharCCName[0][1], 0);
                mChar2Bck[4].mPane->hide();
                mChar2Brk[0].mFrame = 0.0f;
                mChar2Brk[1].mFrame = 0.0f;
                mChar2Brk[2].mFrame = 0.0f;
                mChar2Brk[3].mFrame = 0.0f;
                mChar2Brk[4].mFrame = 0.0f;
                if (mPlayerData[0].mState == 0)
                {
                    mPlayerPadNo[0] = 0;
                    mPlayerData[0].mCharNoF = 0;
                    mPlayerData[0].mCharNoB = 0;
                }
                if (mPlayerData[1].mState == 0)
                {
                    mPlayerPadNo[1] = 1;
                    mPlayerData[1].mCharNoF = 1;
                    mPlayerData[1].mCharNoB = 1;
                }
                break;
            case 1:
                if (mPlayerData[0].mState == 0)
                {
                    mPlayerPadNo[0] = 0;
                    mPlayerPadNo[1] = 0;
                    mPlayerData[0].mCharNoF = 0;
                    mPlayerData[0].mCharNoB = 1;
                }
                break;
            default:
#line 5773
                JUT_ASSERT(0);
            }
            break;
        case 2:
            switch (mInfo._18)
            {
            case 0:
                mChar3Bck[2].mPane->hide();
                pane = (J2DPicture *)mChar3Bck[3].mPane;
#line 5782
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                pane->changeTexture(mCharCCName[0][1], 0);
                pane = (J2DPicture *)mChar3Bck[4].mPane;
#line 5785
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                pane->changeTexture(mCharCCName[0][2], 0);
                mChar3Brk[0].mFrame = 0.0f;
                mChar3Brk[1].mFrame = 0.0f;
                mChar3Brk[2].mFrame = 0.0f;
                mChar3Brk[3].mFrame = 0.0f;
                mChar3Brk[4].mFrame = 0.0f;
                mChar3Brk[5].mFrame = 0.0f;
                mChar3Brk[6].mFrame = 0.0f;
                mChar3Brk[7].mFrame = 0.0f;
                if (mPlayerData[0].mState == 0)
                {
                    mPlayerPadNo[0] = 0;
                    mPlayerData[0].mCharNoF = 0;
                    mPlayerData[0].mCharNoB = 0;
                }
                if (mPlayerData[1].mState == 0)
                {
                    mPlayerPadNo[1] = 1;
                    mPlayerData[1].mCharNoF = 1;
                    mPlayerData[1].mCharNoB = 1;
                }
                if (mPlayerData[2].mState == 0)
                {
                    mPlayerPadNo[2] = 2;
                    mPlayerData[2].mCharNoF = 2;
                    mPlayerData[2].mCharNoB = 2;
                }
                break;
            case 1:
                pane = (J2DPicture *)mChar2Bck[3].mPane;
#line 5813
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                pane->changeTexture(mCharCCName[0][2], 0);
                mChar2Bck[4].mPane->hide();
                mChar2Brk[0].mFrame = 1.0f;
                mChar2Brk[1].mFrame = 1.0f;
                mChar2Brk[2].mFrame = 1.0f;
                mChar2Brk[3].mFrame = 1.0f;
                mChar2Brk[4].mFrame = 1.0f;
                if (mPlayerData[0].mState == 0)
                {
                    mPlayerPadNo[0] = 0;
                    mPlayerPadNo[1] = 0;
                    mPlayerData[0].mCharNoF = 0;
                    mPlayerData[0].mCharNoB = 1;
                }
                if (mPlayerData[1].mState == 0)
                {
                    mPlayerPadNo[2] = 1;
                    mPlayerData[1].mCharNoF = 2;
                    mPlayerData[1].mCharNoB = 2;
                }
                break;
            default:
#line 5834
                JUT_ASSERT(0);
            }
            break;
        case 3:
            switch (mInfo._18)
            {
            case 0:
                if (mPlayerData[0].mState == 0)
                {
                    mPlayerPadNo[0] = 0;
                    mPlayerData[0].mCharNoF = 0;
                    mPlayerData[0].mCharNoB = 0;
                }
                if (mPlayerData[1].mState == 0)
                {
                    mPlayerPadNo[1] = 1;
                    mPlayerData[1].mCharNoF = 1;
                    mPlayerData[1].mCharNoB = 1;
                }
                if (mPlayerData[2].mState == 0)
                {
                    mPlayerPadNo[2] = 2;
                    mPlayerData[2].mCharNoF = 2;
                    mPlayerData[2].mCharNoB = 2;
                }
                if (mPlayerData[3].mState == 0)
                {
                    mPlayerPadNo[3] = 3;
                    mPlayerData[3].mCharNoF = 3;
                    mPlayerData[3].mCharNoB = 3;
                }
                break;
            case 1:
                pane = (J2DPicture *)mChar2Bck[3].mPane;
#line 5864
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                pane->changeTexture(mCharCCName[0][2], 0);
                mChar2Brk[0].mFrame = 1.0f;
                mChar2Brk[1].mFrame = 1.0f;
                mChar2Brk[2].mFrame = 1.0f;
                mChar2Brk[3].mFrame = 1.0f;
                mChar2Brk[4].mFrame = 1.0f;
                if (mPlayerData[0].mState == 0)
                {
                    mPlayerPadNo[0] = 0;
                    mPlayerPadNo[1] = 0;
                    mPlayerData[0].mCharNoF = 0;
                    mPlayerData[0].mCharNoB = 1;
                }
                if (mPlayerData[1].mState == 0)
                {
                    mPlayerPadNo[2] = 1;
                    mPlayerPadNo[3] = 1;
                    mPlayerData[1].mCharNoF = 2;
                    mPlayerData[1].mCharNoB = 3;
                }
                break;
            case 2:
                pane = (J2DPicture *)mChar3Bck[3].mPane;
#line 5886
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                pane->changeTexture(mCharCCName[0][2], 0);
                pane = (J2DPicture *)mChar3Bck[4].mPane;
#line 5889
                JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
                pane->changeTexture(mCharCCName[0][3], 0);
                mChar3Brk[0].mFrame = 1.0f;
                mChar3Brk[1].mFrame = 1.0f;
                mChar3Brk[2].mFrame = 1.0f;
                mChar3Brk[3].mFrame = 1.0f;
                mChar3Brk[4].mFrame = 1.0f;
                mChar3Brk[5].mFrame = 1.0f;
                mChar3Brk[6].mFrame = 1.0f;
                mChar3Brk[7].mFrame = 1.0f;
                if (mPlayerData[0].mState == 0)
                {
                    mPlayerPadNo[0] = 0;
                    mPlayerPadNo[1] = 0;
                    mPlayerData[0].mCharNoF = 0;
                    mPlayerData[0].mCharNoB = 1;
                }
                if (mPlayerData[1].mState == 0)
                {
                    mPlayerPadNo[2] = 1;
                    mPlayerData[1].mCharNoF = 2;
                    mPlayerData[1].mCharNoB = 2;
                }
                if (mPlayerData[2].mState == 0)
                {
                    mPlayerPadNo[3] = 2;
                    mPlayerData[2].mCharNoF = 3;
                    mPlayerData[2].mCharNoB = 3;
                }
                break;
            default:
#line 5917
                JUT_ASSERT(0);
            }
            break;
        default:
#line 5922
            JUT_ASSERT(0);
        }
    }
    else
    {
        for (int i = 0; i < 10; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                mCharPane[i][j].mFrame = 27.0f;
                mSelectAnm[i][j].mEnable = false;
            }
        }

#define SM_SET_CURSOR(pad, pos)                                              \
    for (int k = 0; k < 4; k++)                                              \
    {                                                                        \
        if (mPlayerData[k].mCharNoF == pad && mPlayerData[k].mCharF == -1)   \
            mPlayerData[pad].mCursor = pos;                                  \
        if (mPlayerData[k].mCharNoB == pad && mPlayerData[k].mCharB == -1)   \
            mPlayerData[pad].mCursor = pos;                                  \
    }
        switch (mInfo.playerNum)
        {
        case 0:
            SM_SET_CURSOR(0, 0)
            break;
        case 1:
            SM_SET_CURSOR(0, 0)
            SM_SET_CURSOR(1, 16)
            break;
        case 2:
            SM_SET_CURSOR(0, 0)
            SM_SET_CURSOR(1, 2)
            SM_SET_CURSOR(2, 16)
            break;
        case 3:
            SM_SET_CURSOR(0, 0)
            SM_SET_CURSOR(1, 2)
            SM_SET_CURSOR(2, 14)
            SM_SET_CURSOR(3, 16)
            break;
        default:
#line 5983
            JUT_ASSERT(0);
        }
#undef SM_SET_CURSOR
        mInfo.oldMode = mInfo.mode;
        mInfo.mode = 13;
        mInfo.timer = 0;
    }
    characterAnm();
    characterOkWaitAnm();
}

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
            mCharCCBpk[row][col + 4].mPane->show();
            J2DPicture *pane = (J2DPicture *)mCharCCBpk[row][col + 4].mPane;
#line 6960
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            pane->changeTexture(mCharCCName[col][no], 0);
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
            mCharCCBpk[row][col + 4].mPane->show();
            J2DPicture *pane = (J2DPicture *)mCharCCBpk[row][col + 4].mPane;
#line 6976
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            pane->changeTexture(mCharCCName[col][no], 0);
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
            mCharCCBpk[row][col + 4].mPane->show();
            J2DPicture *pane = (J2DPicture *)mCharCCBpk[row][col + 4].mPane;
#line 6991
            JUT_ASSERT(pane->getTypeID() == J2DPane_Picture);
            pane->changeTexture(mCharCCName[col][i], 0);
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

void SceneMenu::setRaceData()
{
    int n;
    switch (mInfo._20)
    {
    case 0:
        switch (mInfo.entryNum)
        {
        case 0:
            gRaceInfo.setRace(GRAND_PRIX, 8, 1, 1, 1);
            break;
        case 1:
            gRaceInfo.setRace(GRAND_PRIX, 8, 2, 2, 2);
            break;
        default:
#line 7436
            JUT_ASSERT(0);
        }
        gRaceInfo.setRaceLevel(mLevelID[mInfo.engineType]);
        break;
    case 1:
        switch (mInfo.entryNum)
        {
        case 1:
            gRaceInfo.setRace(VERSUS_RACE, 2, 2, 2, 2);
            break;
        case 2:
            gRaceInfo.setRace(VERSUS_RACE, 3, 3, 4, 3);
            break;
        case 3:
            gRaceInfo.setRace(VERSUS_RACE, 4, 4, 4, 4);
            break;
        default:
#line 7454
            JUT_ASSERT(0);
        }
        gRaceInfo.setRaceLevel(mLevelID[mInfo.engineType]);
        break;
    case 2:
        gRaceInfo.setRace(TIME_ATTACK, 4, 4, 1, 1);
        gRaceInfo.setRaceLevel(LVL_150CC);
        break;
    case 3:
    {
        ERaceMode mode;
        switch (mMsgId)
        {
        case 0:
            gSequenceInfo.mBattleType = 0;
            mode = BALLOON_BATTLE;
            break;
        case 1:
            gSequenceInfo.mBattleType = 1;
            mode = ESCAPE_BATTLE;
            break;
        case 2:
            gSequenceInfo.mBattleType = 2;
            mode = BOMB_BATTLE;
            break;
        default:
#line 7481
            JUT_ASSERT(0);
        }
        switch (mInfo.entryNum)
        {
        case 1:
            gRaceInfo.setRace(mode, 2, 2, 2, 2);
            break;
        case 2:
            gRaceInfo.setRace(mode, 3, 3, 4, 3);
            break;
        case 3:
            gRaceInfo.setRace(mode, 4, 4, 4, 4);
            break;
        default:
#line 7495
            JUT_ASSERT(0);
        }
        gRaceInfo.setRaceLevel(LVL_50CC);
        break;
    }
    default:
#line 7502
        JUT_ASSERT(0);
    }

    setCharSelFlag();

    struct KartEntry
    {
        EKartID kart;
        ECharID charF;
        KartGamePad *padF;
        ECharID charB;
        KartGamePad *padB;
    };

    KartEntry entries[4];
    n = 0;
    for (int i = 0; i < 4; i++)
    {
        if (mPlayerData[i].mCharNoF != -1)
        {
            KartGamePad *padF = gpaKartPad[mPlayerData[i].mCharNoF];
            KartGamePad *padB;
            if (mPlayerData[i].mCharNoF == mPlayerData[i].mCharNoB)
                padB = nullptr;
            else
                padB = gpaKartPad[mPlayerData[i].mCharNoB];
            int charF = mPlayerData[i].mCharF;
            int charB = mPlayerData[i].mCharB;
            entries[n].kart = (EKartID)mKartList[mPlayerData[i].mKart].mKartId;
            entries[n].charF = (ECharID)mChara[charF].mCharId;
            entries[n].padF = padF;
            entries[n].charB = (ECharID)mChara[charB].mCharId;
            entries[n].padB = padB;
            useCharSel(charF, charB);
            n++;
        }
    }

    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - 1 - i; j++)
        {
            int player[2][2];
            player[0][0] = gSequenceInfo.getPad2Player(entries[j].padF);
            player[0][1] = gSequenceInfo.getPad2Player(entries[j].padB);
            player[1][0] = gSequenceInfo.getPad2Player(entries[j + 1].padF);
            player[1][1] = gSequenceInfo.getPad2Player(entries[j + 1].padB);
            int minPlayer[2];
            for (int k = 0; k < 2; k++)
            {
                if (player[k][1] == -1)
                    minPlayer[k] = player[k][0];
                else if (player[k][0] < player[k][1])
                    minPlayer[k] = player[k][0];
                else
                    minPlayer[k] = player[k][1];
            }
            if (minPlayer[0] > minPlayer[1])
            {
                KartEntry tmp;
                tmp.kart = entries[j].kart;
                tmp.charF = entries[j].charF;
                tmp.padF = entries[j].padF;
                tmp.charB = entries[j].charB;
                tmp.padB = entries[j].padB;
                entries[j].kart = entries[j + 1].kart;
                entries[j].charF = entries[j + 1].charF;
                entries[j].padF = entries[j + 1].padF;
                entries[j].charB = entries[j + 1].charB;
                entries[j].padB = entries[j + 1].padB;
                entries[j + 1].kart = tmp.kart;
                entries[j + 1].charF = tmp.charF;
                entries[j + 1].padF = tmp.padF;
                entries[j + 1].charB = tmp.charB;
                entries[j + 1].padB = tmp.padB;
            }
        }
    }

    for (int i = 0; i < n; i++)
        gRaceInfo.setKart(i, entries[i].kart, entries[i].charF, entries[i].padF, entries[i].charB, entries[i].padB);

    if (gRaceInfo.getRaceMode() == GRAND_PRIX)
    {
        for (int i = 0; i < n; i++)
        {
            gSequenceInfo.set_318_charId(i, 0, entries[i].charF);
            gSequenceInfo.set_318_charId(i, 1, entries[i].charB);
            gSequenceInfo.set_318_kartId(i, entries[i].kart);
            gSequenceInfo.set_318_coop(i, entries[i].padB != nullptr);
            gSequenceInfo.set_318_RaceTime(i, RaceTime(0));
            gSequenceInfo.set_318_14(i, 0);
            gSequenceInfo.set_318_Pad(i, entries[i].padF);
        }
        gSequenceInfo._350 = mInfo.playerNum + 1;
        gSequenceInfo._354 = n;
        gSequenceInfo.mLevel = mLevelID[mInfo.engineType];
    }

    for (int i = mInfo.entryNum + 1; i < gRaceInfo.getKartNumber(); i++)
    {
        int charF;
        int charB;
        getCharSel(charF, charB);
        gRaceInfo.setKart(i, (EKartID)mKartList[getKart(charF, charB)].mKartId, (ECharID)mChara[charF].mCharId, nullptr, (ECharID)mChara[charB].mCharId, nullptr);
    }

    int startNo[8];
    for (int i = 0; i < gRaceInfo.getKartNumber(); i++)
        startNo[i] = i;
    gSequenceInfo.setRandomSeed();
    for (int i = 0; i < gRaceInfo.getKartNumber(); i++)
    {
        int r = i + gSequenceInfo.getRandom() % (gRaceInfo.getKartNumber() - i);
        int tmp = startNo[i];
        startNo[i] = startNo[r];
        startNo[r] = tmp;
    }
    for (int i = 0; i < gRaceInfo.getKartNumber(); i++)
        gSequenceInfo.setStartNo(i, startNo[i]);

    if (mInfo._20 == 0)
    {
        for (int i = 0; i < gRaceInfo.getKartNumber(); i++)
            gSequenceInfo.setStartNo(i, gRaceInfo.getKartNumber() - 1 - i);
        if (mInfo.entryNum == 1)
        {
            gSequenceInfo.setStartNo(0, gRaceInfo.getKartNumber() - 2);
            gSequenceInfo.setStartNo(1, gRaceInfo.getKartNumber() - 1);
        }
    }

    gRaceInfo.setRandomSeed(OSGetTime());
    for (int i = 0; i < mInfo.entryNum + 1; i++)
        gRaceInfo.setConsoleTarget(i, i, false);
    if (gRaceInfo.getRaceMode() == GRAND_PRIX && mInfo.entryNum == 0)
        gRaceInfo.setConsoleTarget(1, 0, false);

    switch (gRaceInfo.getRaceMode())
    {
    case VERSUS_RACE:
    case BALLOON_BATTLE:
    case BOMB_BATTLE:
    case ESCAPE_BATTLE:
        if (mInfo.entryNum + 1 == 3)
            gRaceInfo.setConsoleTarget(3, 0, true);
        break;
    }

    gSequenceInfo.mDecidePad = gpaGamePad[0];
    mInfo.timer = 0;
    switch (mInfo._20)
    {
    case 0:
    case 1:
    case 2:
        mInfo.nextMode = 15;
        mInfo.mode = 14;
        mInfo.oldMode = 13;
        mInfo.timer = 0;
        mInfo.nextScene = Scene::SCENE_COURSE;
        break;
    case 3:
        mInfo.nextMode = 15;
        mInfo.mode = 14;
        mInfo.oldMode = 13;
        mInfo.timer = 0;
        mInfo.nextScene = Scene::SCENE_MAP_SELECT;
        break;
    default:
#line 7690
        JUT_ASSERT(0);
    }
}

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
