#ifndef SCENEMENU_H
#define SCENEMENU_H

#include "Kameda/Scene.h"
#include "kartEnums.h"
#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/JGeometry/Box.h"

class J2DAnmBase;
class J2DPane;
class J2DPicture;
class J2DScreen;
class J2DOrthoGraph;
class NameEntryHelper;
class CharacterSelect3D;

class SceneMenu : public Scene
{
public:
    struct BckData
    {
        J2DAnmBase *mAnm; // 00
        J2DPane *mPane;   // 04
        f32 mFrame;       // 08
    };

    struct BrkData
    {
        J2DAnmBase *mAnm; // 00
        J2DPane *mPane;   // 04
        f32 mFrame;       // 08
    };

    struct PaneData // fabricated
    {
        J2DPane *mPane; // 00
        f32 mFrame;     // 04
        u32 _8;         // 08
    };

    struct CharInfo // fabricated
    {
        int mCharId; // 00
        int mWeight; // 04
        int mKartId; // 08
    };

    struct KartInfoPair // fabricated
    {
        int mKartId;    // 00
        int mKartWeight;// 04
    };

    struct PlayerData // fabricated
    {
        int mCursor;  // 00
        int mCharNoF; // 04
        int mCharNoB; // 08
        int mCharF;   // 0C
        int mCharB;   // 10
        int mKart;    // 14
        int mState;   // 18
    };

    struct SelectAnm // fabricated
    {
        u8 mEnable; // 00
        u8 _pad[3]; // 01
        int mNo;    // 04
    };

    enum Cursor
    {
        CURSOR_UP,        // 0
        CURSOR_UP_LEFT,   // 1
        CURSOR_UP_RIGHT,  // 2
        CURSOR_DOWN,      // 3
        CURSOR_DOWN_LEFT, // 4
        CURSOR_DOWN_RIGHT,// 5
        CURSOR_LEFT,      // 6
        CURSOR_RIGHT,     // 7
    };

    SceneMenu(JKRArchive *, JKRHeap *);  // 0x8016f78c
    virtual ~SceneMenu();                // 0x80173908
    virtual void init();                 // 0x80172e6c
    virtual void draw();                 // 0x80173960
    virtual void calc();                 // 0x80173f08

    void reset();                        // 0x801725b8
    void calcModeAnm();                  // 0x801750b0
    void caleEngine(BckData *);          // 0x801751ac
    void calcEngineMoji(BckData *);      // 0x801751ec
    void calcEnginePic(BckData *);       // 0x801752f0
    void calcEngineBrkAnm(BrkData *);    // 0x801753f4
    void calcModeOrder();                // 0x80175434
    void entry_init();                   // 0x801755b4
    void entry();                        // 0x80175970
    void entry_end();                    // 0x80176268
    void vs_init();                      // 0x801762ec
    void vs();                           // 0x80176c5c
    void vs_end();                       // 0x801780bc
    void mode_init();                    // 0x8017836c
    void mode();                         // 0x801797d0
    void mode_end();                     // 0x8017b78c
    void battle_init();                  // 0x8017c834
    void battle();                       // 0x8017cd14
    void battle_end();                   // 0x8017d39c
    void character_init();               // 0x8017d4a0
    void character();                    // 0x8017e3b8
    void character_end();                // 0x8017e86c
    void nextScene();                    // 0x8017ebf4
    void selectA(int);                   // 0x8017ece8
    void selectB(int);                   // 0x8017eefc
    void selectUp(int);                  // 0x8017f17c
    void selectDown(int);                // 0x8017f28c
    void selectLeft(int);                // 0x8017f39c
    void selectRight(int);               // 0x8017f4a0
    void selectZ(int);                   // 0x8017f5a4
    bool isSelectAnm(int);               // 0x8017f6f0
    int moveCursor(int, Cursor);         // 0x8017f754
    bool isRndRoulette();                // 0x8017fb78
    void rndRoulette();                  // 0x8017fc50
    void rndSelect();                    // 0x8017fce0
    void setCharacterFlag();             // 0x8017fe00
    int getKart(int, int);               // 0x8017ff04
    int changeKart(int, int, int, int);  // 0x80180060
    void characterAnm();                 // 0x8018029c
    void characterOkWaitAnm();           // 0x80180ef8
    void setChar1Plus();                 // 0x80181450
    void setRaceData();                  // 0x801817f0
    void setCharSelFlag();               // 0x80182954
    void useCharSel(int, int);           // 0x80182954
    void getCharSel(int &, int &);       // 0x8018296c
    void getRndCharSel(int &, int &);    // 0x80182d14
    void changeText();                   // 0x80182e8c

private:
    static const u32 mCharVoice[20];             // 0x80371338
    static const u32 mCharGoVoice[20];           // 0x80371388
    static const int mCharIndex[20];             // 0x803713d8
    static const int mKartIndex[21];             // 0x803715d4
    static const u8 mKartPerf[63];               // 0x80371628
    static const ERaceLevel mLevelID[4];         // 0x80371668
    static const u64 mPlayerIconBckTag[33];      // 0x80371678
    static const u64 mHowManyDriversBckTag[13];  // 0x803717b0
    static const u64 mHowManyDriversBrkTag[22];  // 0x80371818
    static const u64 mSingleTagSelectBckTag[43]; // 0x803718c8
    static const u64 mSingleTagSelectBtkTag[24]; // 0x80371a20
    static const u64 mSingleTagSelectBrkTag[78]; // 0x80371ae0
    static const u64 mSingleTagSelectBpkTag[7];  // 0x80371d50
    static const u64 mModeBckTag[75];            // 0x80371d88
    static const u64 mModeBtkTag[26];            // 0x80371fe0
    static const u64 mModeBrkTag[30];            // 0x803720b0
    static const u64 mModeBpkTag[6];             // 0x803721a0
    static const u64 mBattleBckTag[15];          // 0x803722a0
    static const u64 mBattleBrkTag[3];           // 0x80372318
    static const u64 mCharCCBckTag[2];           // 0x80372330
    static const u64 mCharCCBrkTag[4];           // 0x80372340
    static const u64 mCharCCBpkTag[6];           // 0x80372360
    static const u64 mChar1BckTag[9];            // 0x803723f0
    static const u64 mChar1PlusBckTag[19];       // 0x80372438
    static const u64 mChar1PlusBrkTag[2];        // 0x803724d0
    static const u64 mChar2BckTag[20];           // 0x803724e0
    static const u64 mChar2BrkTag[5];            // 0x80372580
    static const u64 mChar3BckTag[27];           // 0x803725a8
    static const u64 mChar3BrkTag[8];            // 0x80372680
    static const u64 mChar4BckTag[26];           // 0x803726c0
    static const char *mCharName[20];            // 0x80395648
    static const char *mPlayerIconPlayerName[4]; // 0x80395698
    static const char *mPowerMojiName[4];        // 0x803956a8
    static const char *mPowerPicName[4];         // 0x803956b8
    static const char *mBattleName[3];           // 0x803956c8
    static const char *mCharCCName[8];           // 0x803956d4

    struct MenuInfo // fabricated (only "mInfo.nextScene" is known)
    {
        int mode;                   // 00
        int oldMode;                // 04
        int nextMode;               // 08
        int timer;                  // 0C
        Scene::SceneType nextScene; // 10
        int playerNum;              // 14
        int _18;                    // 18
        int entryNum;               // 1C
        int _20;                    // 20
        int engineType;             // 24
    };

private:
    CharInfo mChara[20];                   // 0x00C
    KartInfoPair mKartList[21];            // 0x0FC
    BckData mIconBck[33];                  // 0x1A4 (PlayerIcon.bck tags)
    BckData mHowManyDriversBck[13];        // 0x330
    BckData mHowManyDriversEnd;            // 0x3CC
    BckData mHowManyDriversBrk[22];        // 0x3D8
    BckData mSingleTagSelectEndBck;        // 0x4E0 (fabricated)
    BckData mSingleTagSelectBck[43];       // 0x4EC
    BckData mSingleTagSelectBtk[24];       // 0x6F0
    BckData mSingleTagSelectBrk[78];       // 0x810
    BckData mSingleTagSelectBpk[7];        // 0xBB8
    BckData mModeBck[75];                  // 0xC0C
    BrkData mModeBtk[26];                  // 0xF90
    BrkData mModeBrk[30];                  // 0x10C8
    BrkData mModeBpk[6];                   // 0x1230
    BckData mBattleBck[15];                // 0x1278
    BckData mBattleBrk[3];                 // 0x132C
    u8 _1350[0x136C - 0x1350];             // 0x1350
    BckData mCharPane[10][2];              // 0x136C (select_character.bck, per-char cell anm)
    BckData mCharCCBck[10];                // 0x145C
    BckData mCharCCBrk[10][4];             // 0x14D4
    BckData mCharCCBpk[10][6];             // 0x16B4
    BckData mCharPaneBck[9];               // 0x1984
    BckData mCharCCEndBck;                 // 0x19F0 (select_character_cc anm)
    BckData mCharCCEndBrk;                 // 0x19FC
    BckData mChar1PlusBck[19];             // 0x1A08
    BckData mChar1PlusBrk[2];              // 0x1AEC
    BckData mChar1PlusBpk;                 // 0x1B04
    BckData mChar2Bck[20];                 // 0x1B10
    BckData mChar2EndBck;                  // 0x1C00
    BckData mChar2Brk[5];                  // 0x1C0C
    BckData mChar2EndBrk;                  // 0x1C48
    BckData mChar3Bck[27];                 // 0x1C54
    BckData mChar3EndBck;                  // 0x1D98
    BckData mChar3Brk[8];                  // 0x1DA4
    BckData mChar3EndBrk;                  // 0x1E04
    BckData mChar4Bck[26];                 // 0x1E10
    BckData mChar4EndBck;                  // 0x1F48
    BckData mChar4EndBrk;                  // 0x1F54
    J2DScreen *mScreenPlayerIcon;          // 0x1F60 (PlayerIcon.blo)
    J2DScreen *mScreenHowManyDrivers;      // 0x1F64
    J2DScreen *mScreenSingleTagSelect;     // 0x1F68
    J2DScreen *mScreenSelectMode;          // 0x1F6C
    J2DScreen *mScreenSelectBattle;        // 0x1F70
    J2DScreen *mScreenCharLayout;          // 0x1F74
    u8 _1f78[0x1F88 - 0x1F78];             // 0x1F78
    J2DScreen *mCharScreens[10];           // 0x1F88 (select_character_cc.blo copies)
    JGeometry::TVec2f mCharPos[10];        // 0x1FB0
    J2DScreen *mScreenChar1;               // 0x2000 (select_character1.blo)
    J2DScreen *mScreenChar1Plus;           // 0x2004
    J2DScreen *mScreenChar2;               // 0x2008
    J2DScreen *mScreenChar3;               // 0x200C
    J2DScreen *mScreenChar4;               // 0x2010
    CharacterSelect3D *mCharacterSelect3D; // 0x2014
    J2DOrthoGraph *mGrafs[10];             // 0x2018
    JGeometry::TBox2f mCharRect[10];       // 0x2040
    MenuInfo mInfo;                        // 0x20E0
    u8 _2108[3];                           // 0x2108
    u8 _210b;                              // 0x210B
    int _210c;                             // 0x210C
    int _2110;                             // 0x2110
    int _2114;                             // 0x2114
    int _2118;                             // 0x2118
    int mPlayerPadNo[4];                   // 0x211C
    PlayerData mPlayerData[4];             // 0x212C
    u8 _219c[0x21EC - 0x219C];             // 0x219C
    SelectAnm mSelectAnm[10][2];           // 0x21EC
    u8 mCharacterFlag[20];                 // 0x228C
    u8 mCharSelFlag[20];                   // 0x22A0
    NameEntryHelper *mNameEntry;           // 0x22B4
    u8 _22b8[0x22C0 - 0x22B8];             // 0x22B8
}; // Size: 0x22c0

#endif
