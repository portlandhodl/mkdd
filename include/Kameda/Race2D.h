#ifndef RACE2D_H
#define RACE2D_H

#include "kartEnums.h"

#include "JSystem/J2D/J2DPicture.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JORReflexible.h"
#include "Kameda/Task.h"
#include "Kaneshige/HioMgr.h"
#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/J2D/J2DScreen.h"
#include "JSystem/JGeometry/Vec.h"

class LapGhost2D;
class JPABaseEmitter;
class CanNotSaveG2D;
class BombStart2D;

#define RACE2D_ASSERT_RANGE(line, name, val, max) \
    (((0 <= (val)) && ((val) < (max))) ? (void)0 : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "Race2D.h", line, "range over: %d <= " #name "=%d < %d", 0, (val), (max)), OSPanic("Race2D.h", line, "Halt")))

class Race2DParam : public JORReflexible {
public:
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
    J2DPicture *getLapTimePic(int status, int lap, int tag) {
#line 484
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, lap, 9);
        JUT_MINMAX_ASSERT(0, tag, 11);
        return mLapDigits[status][lap][tag];
    }
    J2DPicture *getRankPic(int status, int rank) {
#line 490
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, rank, 8);
        return mRankDigits1[status][rank];
    }
    void setRaceTime(int status, bool available, int lap, int d0, int d1, int d2, int d3, int d4, int d5, int d6) {
#line 502
        JUT_MINMAX_ASSERT(0, status, 4);
        mPlayerHUD[status].mRaceTimeAvailable = available;
        mPlayerHUD[status].mLap = lap;
        mPlayerHUD[status].mDigits[0] = d0;
        mPlayerHUD[status].mDigits[1] = d1;
        mPlayerHUD[status].mDigits[2] = d2;
        mPlayerHUD[status].mDigits[3] = d3;
        mPlayerHUD[status].mDigits[4] = d4;
        mPlayerHUD[status].mDigits[5] = d5;
        mPlayerHUD[status].mDigits[6] = d6;
    }
    void setLapTime(int status, int lap, bool available, int d0, int d1, int d2, int d3, int d4, int d5, int d6) {
#line 520
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, lap, 9);
        mPlayerHUD[status].mLapSlots[lap].mAvailable = available;
        mPlayerHUD[status].mLapSlots[lap].mDigits[0] = d0;
        mPlayerHUD[status].mLapSlots[lap].mDigits[1] = d1;
        mPlayerHUD[status].mLapSlots[lap].mDigits[2] = d2;
        mPlayerHUD[status].mLapSlots[lap].mDigits[3] = d3;
        mPlayerHUD[status].mLapSlots[lap].mDigits[4] = d4;
        mPlayerHUD[status].mLapSlots[lap].mDigits[5] = d5;
        mPlayerHUD[status].mLapSlots[lap].mDigits[6] = d6;
    }
    J2DPicture *getRankGoalPic(int status, int rank) {
#line 495
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, rank, 8);
        return mRankDigits2[status][rank];
    }
    f32 getTimePosY(int status) {
#line 514
        JUT_MINMAX_ASSERT(0, status, 4);
        return mPlayerHUD[status].mTimePos[0].y;
    }
    JGeometry::TVec2f getLapTimePos(int status, int lap, int tag) {
#line 532
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, lap, 9);
        JUT_MINMAX_ASSERT(0, tag, 11);
        return mPlayerHUD[status].mLapTimePos[lap][tag];
    }
    void setLapTimePos2(int status, int lap, int tag, JGeometry::TVec2f pos) {
#line 538
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, lap, 9);
        JUT_MINMAX_ASSERT(0, tag, 11);
        mPlayerHUD[status].mLapTimePos2[lap][tag] = pos;
    }
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
    static const u64 mLapTag[4][4];        // multidimensional?                                       // 0x8036fb10
    static const u64 mItemExTag[4][4];     // multidimensional?                                       // 0x8036fb90
    static const u64 mItemExDrawTag[4][2][6]; // multidimensional?                                       // 0x8036fc10
    static const u64 mRankTag[4][8];                                                                  // 0x8036ff80
    static const u64 mRankGoalTag[2][8];                                                              // 0x80370080
    static const u64 mTimeDrawTag[2][10];                                                              // 0x80370100
    static const u64 mLapTimeDrawTag[2][9][11];                                                          // 0x803701a0
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
    int getItem(int status, int driver, int no) {
#line 544
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, driver, 2);
        JUT_MINMAX_ASSERT(0, no, 2);
        return mPlayerHUD[status].mItem[driver][no];
    }
    void setItem(int status, int driver, int no, int item) {
#line 550
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, driver, 2);
        JUT_MINMAX_ASSERT(0, no, 2);
        JUT_MINMAX_ASSERT(0, item, 31);
        mPlayerHUD[status].mItem[driver][no] = item;
    }
    void setItemFlag(int status, int driver, bool flag) {
#line 557
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, driver, 2);
        mPlayerHUD[status]._7f8[driver] = flag;
    }
    JGeometry::TVec2f getItemPos(int status, int driver) {
#line 562
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, driver, 2);
        return mPlayerHUD[status].mItemPos[driver];
    }
    JGeometry::TVec2f getItemPos2(int status, int driver) {
#line 567
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, driver, 2);
        return mPlayerHUD[status].mItemPos2[driver];
    }
    void setItemPos3(int status, int driver, JGeometry::TVec2f pos) {
#line 572
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, driver, 2);
        mPlayerHUD[status].mItemPos3[driver] = pos;
    }
    void setItemFrame(int status, int driver, int interval, int count) {
#line 577
        JUT_MINMAX_ASSERT(0, status, 4);
        JUT_MINMAX_ASSERT(0, driver, 2);
        mPlayerHUD[status].mItemInterval[driver] = interval;
        mPlayerHUD[status].mItemCount[driver] = count;
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
    const ResTIMG *mDriverTex[20];      // 0x0
    const ResTIMG *mPlayerTex[8];       // 0x50
    const ResTIMG *mCharacterTex[20];   // 0x70
    const ResTIMG *mLapNumTex[9];       // 0xc0
    const ResTIMG *mItemTex[31];        // 0xe4
    const ResTIMG *mNumberTex[10];      // 0x160
    const ResTIMG *mSpeedTex[10];       // 0x188
    Race2DHioNode *mHio;                // 0x1b0
    J2DOrthoGraph *mGrafContext;        // 0x1b4
    LapGhost2D *mLapGhost;              // 0x1b8
    CanNotSaveG2D *mCanNotSave;         // 0x1bc
    BombStart2D *mBombStart;            // 0x1c0
    struct {
        u32 mBase; // +0x0
        u32 _4;    // +0x4
        u32 mStep; // +0x8
        u32 _c;    // +0xc
    } mAlarm[4];                        // 0x1c4
    J2DScreen *mAlarmScreen;            // 0x204
    J2DPicture *mAlarmPic0;             // 0x208
    J2DPicture *mAlarmPic1;             // 0x20c
    JGeometry::TVec2f mAlarmPos0;       // 0x210
    JGeometry::TVec2f mAlarmPos1;       // 0x218
    f32 mAlarmX[4];                     // 0x220
    J2DScreen *mScreen;                 // 0x230
    J2DScreen *mMaskScreen;             // 0x234
    J2DPicture *mMark[4][8];            // 0x238
    int mMarkOrder[4][8];               // 0x2b8
    J2DScreen *mShineScreen[4];         // 0x338
    J2DPicture *mDemoPic;               // 0x348
    J2DPicture *mCoursePic;             // 0x34c
    J2DPicture *mStartLinePic;          // 0x350
    u8 _354[0x364 - 0x354];             // 0x354
    J2DPicture *mLapPic[4];             // 0x364
    J2DPicture *mLapPics[4][4];         // 0x374
    J2DPicture *mItemExPics[4][4];      // 0x3b4
    J2DPicture *mItemExDraw[4][2][6];   // 0x3f4
    J2DPicture *mSpecialItem[4][2];     // 0x4b4
    J2DPicture *mKiraPics[4][2][6];     // 0x4d4
    J2DPicture *mRankDigits1[4][8];     // 0x594
    J2DPicture *mRankDigits2[4][8];     // 0x614
    J2DPicture *_694x[2][10];           // 0x694
    J2DPicture *mLapDigits[2][9][11];   // 0x6e4
    J2DPicture *mSpeedPics[3];          // 0x9fc
    J2DPicture *mSpeedChipPics[7];      // 0xa08
    J2DPicture *mSpeedDrawPics[11];     // 0xa24
    J2DPicture *mMiniPics[4][3][4];     // 0xa50
    JGeometry::TVec2f mMiniPos[4][3][4]; // 0xb10
    J2DPicture *mStarPics[4][2];        // 0xc90
    J2DPicture *mShinePics[4][5];       // 0xcb0
    JGeometry::TVec2f mShineBase[4];    // 0xd00
    JGeometry::TVec2f mShinePos[5];     // 0xd20
    int mBattleTimeCount[4];            // 0xd48 (calcTimeFrame state counters)
    int mShineKind[4];                  // 0xd58
    int mTimeColorFrame;                // 0xd68
    int mBombColorFrame;                // 0xd6c
    int mBombWinFrame[4];               // 0xd70
    int mBombMarkFrame[4][4];           // 0xd80
    JGeometry::TVec2f _dc0[4][4];       // 0xdc0
    JGeometry::TVec2f _e40[4][4];       // 0xe40
    JPABaseEmitter *_ec0[4][4][3];      // 0xec0
    JPABaseEmitter *_f80[4][4][2];      // 0xf80
    JGeometry::TVec2f mMarkPos[4][8];   // 0x1000
    JGeometry::TVec2f mDemoPos;         // 0x1100
    JGeometry::TVec2f mCoursePos;       // 0x1108
    struct {
        JGeometry::TVec2f mMin; // +0x0
        JGeometry::TVec2f mMax; // +0x8
        int mRot;               // +0x10
    } mMapInfo;                         // 0x1110
    JGeometry::TVec2f mStartLinePos;    // 0x1124
    f32 mStartLineRot;                  // 0x112c
    J2DPicture *mKartIcon[8];           // 0x1130
    J2DPicture *mPlayerIcon[8];         // 0x1150
    J2DPicture *mPlayerGIcon;           // 0x1170
    J2DPicture *mPlayerSGIcon;          // 0x1174
    J2DPicture *mMapShinePic;           // 0x1178
    JGeometry::TVec2f mEscape;          // 0x117c
    J2DPicture *mBattleLightPic;        // 0x1184
    int mEscapeFrame;                   // 0x1188
    JGeometry::TVec2f mEscapeTarget[8]; // 0x118c
    JGeometry::TVec2f mItemExPos[4][2][6]; // 0x11cc
    JGeometry::TVec2f _134c[4][2][6];   // 0x134c
    JUTColor mSpeedChipColor[7];        // 0x14cc
    JGeometry::TVec2f mSpeedDrawPos[11]; // 0x14e8
    J2DPicture::TCornerColor mTimeCornerColor;    // 0x1540
    J2DPicture::TCornerColor mLapTimeCornerColor; // 0x1550
    struct {
        int mCount[4];
        int mNum;
    } mAnmTA[2]; // 0x1560

    struct {
        int mLap;                           // +0x000
        JGeometry::TVec2f mLapPos[4];       // +0x004
        int _24;                            // +0x024
        u8 mRaceTimeAvailable;              // +0x028
        int mDigits[7];                     // +0x02c
        JGeometry::TVec2f mTimePos[10];     // +0x048
        struct {
            u8 mAvailable;  // +0x00
            int mDigits[7]; // +0x04
        } mLapSlots[9];                     // +0x098
        JGeometry::TVec2f mLapTimePos[9][11];  // +0x1b8
        JGeometry::TVec2f mLapTimePos2[9][11]; // +0x4d0
        int mItem[2][2];                    // +0x7e8
        bool _7f8[2];                       // +0x7f8
        bool _7fa[2];                       // +0x7fa
        JGeometry::TVec2f mItemPos[2];      // +0x7fc
        JGeometry::TVec2f mItemPos2[2];     // +0x80c
        JGeometry::TVec2f mItemPos3[2];     // +0x81c
        int mItemInterval[2];               // +0x82c
        int mItemCount[2];                  // +0x834
        u8 _83c[0x848 - 0x83c];             // +0x83c
        int _848;                           // +0x848
        int _84c[2];                        // +0x84c
        JGeometry::TVec2f mRankPos[8];      // +0x854
        JGeometry::TVec2f mRankPos2[8];     // +0x894
        u8 _8d4;                            // +0x8d4
        bool mIsAlarm;                      // +0x8d5
        int mAlarmAlpha[4];                 // +0x8d8
    } mPlayerHUD[4];                        // 0x1588
    int mGoalCount;                         // 0x3928
    Vec mItemPos[4][2][39];                 // 0x392c
    Vec mCharPos[2][39];                    // 0x47cc
    f32 mSpecialItemScale[50];              // 0x4b74
    struct {
        int _0;
        int _4;
        int _8;
    } mKartSlot[8];                         // 0x4c3c
    struct {
        J2DScreen *mScreen;           // +0x00
        J2DPicture *mPics[2][2];      // +0x04
        J2DPane *mPane;               // +0x14
        int _18;                      // +0x18
        JGeometry::TVec2f _1c;        // +0x1c
        JGeometry::TVec2f mPos;       // +0x24
        JGeometry::TVec2f _2c;        // +0x2c
        u8 _34[4];                    // +0x34
    } mCharWindow[8];                       // 0x4c9c
    JGeometry::TVec2f mCharIconPos[8];      // 0x4e5c
    int _4e9c[2];                           // 0x4e9c
    JGeometry::TVec2f mCharWindowOfs;       // 0x4ea4
    JGeometry::TVec2f mCharWindowCenter;    // 0x4eac
    int mEscapeTimer;                       // 0x4eb4
    int mEscapeTimer4;                      // 0x4eb8
    int mHideFrame;                         // 0x4ebc
    bool mDrawFlag;                         // 0x4ec0
    bool mIsEndBombDemo;                    // 0x4ec1
};

#endif // RACE2D_H
