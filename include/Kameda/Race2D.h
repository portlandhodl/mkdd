#ifndef RACE2D_H
#define RACE2D_H

#include "kartEnums.h"

#include "JSystem/J2D/J2DPicture.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JORReflexible.h"
#include "Kameda/Task.h"
#include "Kaneshige/HioMgr.h"

#define RACE2D_ASSERT_RANGE(line, name, val, max) \
    (((0 <= (val)) && ((val) < (max))) ? (void)0 : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "Race2D.h", line, "range over: %d <= " #name "=%d < %d", 0, (val), (max)), OSPanic("Race2D.h", line, "Halt")))

class Race2DParam : public JORReflexible {
public:
    // Inline/Unused
    Race2DParam();
    virtual ~Race2DParam() {}

    void setX(f32 v) { mX = v; }
    void setY(f32 v) { mY = v; }
    void setS(f32 v) { mS = v; }

    u8 mFlag;   // +0x04
    f32 mUnk8;  // +0x08
    f32 mX;     // +0x0c
    f32 mY;     // +0x10
    f32 mS;     // +0x14
};

class Race2DHioNode : public HioNode {
public:
    Race2DHioNode(const char *name) : HioNode(name, &mParam, 0, 0) {}
    virtual ~Race2DHioNode() {}

    Race2DParam *getParam() { return &mParam; }

    Race2DParam mParam;
};

class Race2D
{ 
public:
    Race2D(JKRHeap *);                                                                            // 0x801531b8
    J2DPicture *getRankPic(int, int);
    void init();                                                                                  // 0x80157714
    void drawPlayerMark();                                                                        // 0x8015838c
    void drawMask();                                                                              // 0x801588b0
    void draw();                                                                                  // 0x80158998
    void drawAlarm();                                                                             // 0x80159a90
    void anmTA(int);                                                                              // 0x80159f00
    void getAlphaAnmTA(int, u8 &);                                                     // 0x8015a688
    void getBestTimeColor(int, JUTColor &, J2DPicture::TCornerColor &);                   // 0x8015a758
    void getBestLapColor(int, JUTColor &);                                                // 0x8015a874
    void drawMiniGame();                                                                          // 0x8015a90c
    void drawBomb();                                                                              // 0x8015a978
    f32 getBombScale(int, int, int);                                                              // 0x8015af3c
    f32 getReachBombScale(int, int);                                                              // 0x8015b13c
    void drawEscape();                                                                            // 0x8015b1e8
    f32 getStartShineTimeScale(int);                                                              // 0x8015b42c
    void drawCourse();                                                                            // 0x8015b4b8
    void calc();                                                                                  // 0x8015c7ec
    void calcPlayerMark();                                                                        // 0x8015cfdc
    void calcParticle();                                                                          // 0x8015d394
    void setLayoutData();                                                                         // 0x8015da18
    void calcMiniGame();                                                                          // 0x8015f688
    void calcBomb();                                                                              // 0x8015f6f4
    JUTColor getBombColor(JUTColor, JUTColor);                                        // 0x80160590
    bool isBombReach(int);                                                                        // 0x80160708
    bool isBombWinner(int);                                                                       // 0x80160864
    void setEffectPos(int);                                                                       // 0x80160980
    void calcEscape();                                                                            // 0x80160c14
    void calcTimeFrame(bool, int);                                                                // 0x80161114
    static J2DPicture::TCornerColor getCornerColor(int);                                          // 0x8016152c
    void setEscapeColor(int);                                                                     // 0x80161894
    JUTColor getTimeColor(JUTColor, JUTColor);                                        // 0x80162844
    void startBombDemo();                                                                         // 0x801629bc
    bool isAlarm(int);                                                                            // 0x801629e0
    void getMapPos(int, JGeometry::TVec3f &, JGeometry::TVec2f &);                    // 0x80162a40
    void getCharacterInfo(int, int, f32 &, f32 &, f32 &);                                   // 0x80162dac
    void getCharacterClr(int, int, int, JUTColor &, JUTColor &, u8 &); // 0x80162ec8
    void getItemInfo(int, int, int, f32 &, f32 &, f32 &);                                   // 0x801635d0
    void decideItem(int, u8, int &);                                                   // 0x8016374c
    static void calcLapInit();                                                         // 0x80163b74
    static void calcLap();                                                                        // 0x80163c00
    static void lapInit();                                                                        // 0x80164128
    static void lapMain();                                                                        // 0x80164168
    static void lapTimeInit();                                                                           // 0x80164188
    static void lapTimeMain();                                                                           // 0x8016451c
    static void calcItem();                                                                              // 0x801651b4
    static void itemStartInit();                                                                         // 0x801655d0
    static void itemStart();                                                                             // 0x80165620
    static void itemBlinkInit();                                                                         // 0x80166348
    static void itemBlink();                                                                             // 0x80166398
    static void itemGetInit();                                                                    // 0x80166dd8
    static void itemGetMain();                                                                    // 0x80166e2c
    static void itemSpecialInit();                                                                // 0x80167464
    static void itemSpecialMain();                                                                // 0x801674a4
    void setUsisi(int, ECharID);                                                                  // 0x80167b08
    void alarmX(int, f32 &);                                                                    // 0x80167b0c
    void getStartScaleA(int, f32 &);                                                            // 0x80167b5c
    void getStartScaleB(int, f32 &);                                                            // 0x80167c50
    void getStartSpeedMeterScale(int, int, f32 &);                                              // 0x80167d4c
    void getGoalSpeedMeterPos(int, f32 &);                                                      // 0x80167df8
    void getStartLapPos(int, int, f32 &);                                                       // 0x80167e48
    void getGoalLapPos(int, f32 &);                                                             // 0x8016804c
    void getStartCharPos(int, int, f32 &);                                                      // 0x8016809c
    void getGoalCharPos(int, int, f32 &);                                                       // 0x8016819c
    f32 getThunderCharScale(int);                                                                // 0x80168214
    void getStartRankPos(int, int, JGeometry::TVec2f &);                                    // 0x801682ec
    f32 getRaceRankScale(int);                                                                   // 0x8016851c
    void getGoalRankPos(int, int, JGeometry::TVec2f &);                                     // 0x801685e4
    void getGoalRankScale(int, int, f32 &);                                                     // 0x801689e4
    void getGoalRankRot(int, f32 &);                                                            // 0x80168cb4
    void getStartLapTimePos(int, int, f32 &);                                                   // 0x80168d1c
    void getGoalLapTimePos(int, int, f32 &);                                                    // 0x80168e30
    static const u64 mLapTag[16];        // multidimensional?                                       // 0x8036fb10
    static const u64 mItemExTag[16];     // multidimensional?                                       // 0x8036fb90
    static const u64 mItemExDrawTag[48]; // multidimensional?                                       // 0x8036fc10
    static const u64 mRankTag[32];                                                                  // 0x8036ff80
    static const u64 mRankGoalTag[16];                                                              // 0x80370080
    static const u64 mTimeDrawTag[20];                                                              // 0x80370100
    static const u64 mLapTimeDrawTag[198];                                                          // 0x803701a0
    static const u64 mSpeedTag[3];                                                                  // 0x803707d0
    static const u64 mSpeedChipTag[7];                                                              // 0x803707e8
    static const u64 mSpeedDrawTag[11];                                                             // 0x80370820
    static const u64 mShineTimeTag[5];                                                              // 0x803708f0
    static const char *mPlayerNumberName[8];                                                        // 0x803950b8
    static const char *mLANPlayerNumberName[8];                                                     // 0x803950d8
    static const char *mDriverName[20];                                                             // 0x803950f8
    static const char *mPlayerName[8];                                                              // 0x80395148
    static const char *mLANPlayerName[8];                                                           // 0x80395168
    static const char *mCharacterName[20];                                                          // 0x80395188
    static const char *mItemName[31];                                                               // 0x803951d8
    static const char *mSpeedName[10];                                                              // 0x80395254
    static JUTColor mPlayerNumberColor[10];                                                 // 0x803fbecc
    static const char *mTimeLapName[2];                                                             // 0x804145d8
    static Race2D *mThis;                                                                           // 0x80416300
    static Task *mTask;                                                                             // 0x80416304
 public:
     static int getWinBombPoint(); // helper mirroring KartChecker::getWinBombPoint

    u8 *getSpecialItem(int status, int pos)
    {
        u8 *thisp = (u8 *)mThis;
        RACE2D_ASSERT_RANGE(463, status, status, 4);
        RACE2D_ASSERT_RANGE(464, pos, pos, 2);
        return *(u8 **)(thisp + 0x4b4 + status * 8 + pos * 4);
    }
    int getItemShow(int status, int driver)
    {
        Race2D *thisp = mThis;
        RACE2D_ASSERT_RANGE(544, status, status, 4);
        RACE2D_ASSERT_RANGE(545, driver, driver, 2);
        return thisp->mPlayerHUD[status].mItem[driver].mShown;
    }
    // Inline/Unused
    ~Race2D();
    void getItemPos(int, int, JGeometry::TVec2f &);
    void itemUseInit();
    void itemUseMain();

    void reset() { init(); }
    void hide() { mHideFrame = 1; }
    
    int getHideFrame() const { return mHideFrame; }
    void setDrawFlag(bool enable) { mDrawFlag = enable; }

    void endBombDemo() { mIsEndBombDemo = true; }
public:
    PLACEHOLDER_BYTES(0, 0xe4);
    const class ResTIMG *mItemTex[32]; // 0xe4
    PLACEHOLDER_BYTES(0x164, 0x1b0);
    Race2DHioNode *mHio;   // 0x1b0
    class J2DGrafContext *mGrafContext;   // 0x1b4
    PLACEHOLDER_BYTES(0x1b8, 0x1c0);
    class BombStart2D *mBombStart; // 0x1c0
    struct Race2DAlarmParam {
        u32 mBase; // +0x0
        u32 _4;    // +0x4
        u32 mStep; // +0x8
        u32 _c;    // +0xc
    } mAlarm[4];                   // 0x1c4
    PLACEHOLDER_BYTES(0x204, 0x208);
    class J2DPicture *mAlarmPic0; // 0x208
    class J2DPicture *mAlarmPic1; // 0x20c
    f32 mAlarmPic0X; // 0x210
    f32 mAlarmPic0Y; // 0x214
    f32 mAlarmPic1X; // 0x218
    f32 mAlarmPic1Y; // 0x21c
    f32 mAlarmX[4];  // 0x220
    PLACEHOLDER_BYTES(0x230, 0x234);
    class J2DScreen *mMaskScreen;         // 0x234
    class J2DPicture *mMark[4][8]; // 0x238
    int mMarkOrder[4][8];          // 0x2b8
    PLACEHOLDER_BYTES(0x338, 0x594);
    class J2DPicture *mRankDigits1[4][8]; // 0x594
    class J2DPicture *mRankDigits2[4][8]; // 0x614
    class J2DPicture *_694x[20];        // 0x694 (unconfirmed)
    // The lap digit table owner's tail overlaps the shine-vector storage
    // (mLapDigits[3][8][6..10+] aliases 0xd00..): only laps 0..8, digits 0..10 are real pictures.
    union Race2DLapShine {
        class J2DPicture *mLapDigits[4][9][11]; // 0x6e4 lap time pane table
        struct
        {
            u8 _0[0x61c];
            JGeometry::TVec2f mShineBase[4]; // 0xd00
            JGeometry::TVec2f mShinePos[5];  // 0xd20
            int mBattleTimeCount[4];         // 0xd48 (calcTimeFrame state counters)
            int mShineKind[4];               // 0xd58
            int mTimeColorFrame;             // 0xd68
            int mBombColorFrame;             // 0xd6c
            int mBombWinFrame[4];            // 0xd70
            int mBombMarkFrame[4][4];        // 0xd80
        } mShine;
    } mLapShine;                         // 0x6e4..0xdc0
#define mLapDigits mLapShine.mLapDigits
#define mShineBase mLapShine.mShine.mShineBase
#define mShinePos mLapShine.mShine.mShinePos
#define mBattleTimeCount mLapShine.mShine.mBattleTimeCount
#define mShineKind mLapShine.mShine.mShineKind
#define mTimeColorFrame mLapShine.mShine.mTimeColorFrame
#define mBombColorFrame mLapShine.mShine.mBombColorFrame
#define mBombWinFrame mLapShine.mShine.mBombWinFrame
#define mBombMarkFrame mLapShine.mShine.mBombMarkFrame
    PLACEHOLDER_BYTES(0xdc0, 0x1000);
    JGeometry::TVec2f mMarkPos[4][8]; // 0x1000
    PLACEHOLDER_BYTES(0x1100, 0x1588);
    struct Race2DLapSlot {
        u8 mAvailable;  // +0x00
        u8 _1[3];       // +0x01
        int mDigits[7]; // +0x04
    };                        // size 0x20
    struct Race2DPlayerHUD {
        int mLap;              // +0x000
        u8 _4[0x24];           // +0x004
        u8 mRaceTimeAvailable; // +0x028
        u8 _29[3];             // +0x029
        int mDigits[7];        // +0x02c
        u8 _48[0x30];          // +0x048
        Race2DLapSlot mLapSlots[9]; // +0x078
        u8 _198[0x7e8 - 0x198];
        struct Race2DItemState { int mShown; int mNew; } mItem[2]; // +0x7e8
        JGeometry::TVec2f mItemPos[2]; // +0x7f8
        u8 _808[0x854 - 0x808];
        f32 mGoalX;      // +0x854 (abs 0x1ddc)
        f32 mGoalY;      // +0x858
        u8 _85c[0x894 - 0x85c];
        f32 mGoalX2;     // +0x894 (abs 0x1e1c)
        f32 mGoalY2;     // +0x898
        u8 _89c[0x8d5 - 0x89c];
        bool mIsAlarm;   // +0x8d5 (abs 0x1e5d)
        u8 _8d6[0x8d8 - 0x8d6];
        int mAlarmAlpha; // +0x8d8
        u8 _8dc[0x8e8 - 0x8dc];
    };                              // size 0x8e8
    Race2DPlayerHUD mPlayerHUD[4];  // 0x1588
    int mGoalCount;                 // 0x3928
    JGeometry::TVec3f mItemPos[4][2][39]; // 0x392c
    JGeometry::TVec3f mCharPos[2][39];    // 0x47cc
    f32 mSpecialItemScale[73];        // 0x4b74
    PLACEHOLDER_BYTES(0x4c98, 0x4eb4);
    int mEscapeTimer;                 // 0x4eb4
    int mEscapeTimer4;                // 0x4eb8
    int mHideFrame;  // 4ebc
    bool mDrawFlag; // 4ec0
    bool mIsEndBombDemo; // 4ec1
    PLACEHOLDER_BYTES(0x4ec2, 0x4ec4);
};

#endif // RACE2D_H
