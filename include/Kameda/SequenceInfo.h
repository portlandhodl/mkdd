#ifndef SEQUENCEINFO_H
#define SEQUENCEINFO_H

#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceTime.h"
#include "Osako/SystemRecord.h"
#include "Osako/kartPad.h"
#include "kartEnums.h"
#include "types.h"

class SequenceInfo
{
public:
    enum GhostSaveLoad {

    };

    void init(); // 0x801515f0
    void setRandomSeed(); // 0x80151760
    u32 getRandom(); // 0x80151790
    KartGamePad *getDecidePad(); // 0x801517b4
    void setClrGPCourse(); // 0x801517d0
    bool isEndGP(); // 0x80151df0
    bool isGameClr(); // 0x80151e34
    int getPlayerRankGP(); // 0x80151f7c
    int getPad2Player(KartGamePad *); // 0x801520c8
    void setSecretFlag(); // 0x80152110
    void setSecretGameAppear(SystemRecord::GameFlag); // 0x80152890
    void setSecretKartAppear(SystemRecord::SecretKartID); // 0x801528bc
    void rndDemo(u32); // 0x801528e8
    EKartID getDemoKart(ECharID); // 0x80152a4c
    void rndAllCupCourse(); // 0x80152b10
    static const int RANKPOINT[8]; // 0x8036f410
    // Inline/Unused
    SequenceInfo();
    void setGPRank(int, int);

    void setStartNo(int kartNo, int startNo) {
#line 167
        JUT_MINMAX_ASSERT(0, kartNo, 8);
        JUT_MINMAX_ASSERT(0, startNo, 8);
        mStartNo[kartNo] = startNo;
    }

    void set_318_charId(int kart, int driverNo, ECharID charId) {
#line 278
        JUT_MINMAX_ASSERT(0, kart, 2);        
        //JUT_MINMAX_ASSERT(0, kart, 8);
        JUT_MINMAX_ASSERT(1, charId, cCharIDMax);
        _318[kart]._0[driverNo] = charId;
    }

    void set_318_kartId(int kart, EKartID kartId) {
#line 288
        JUT_MINMAX_ASSERT(0, kart, 2);        
        JUT_MINMAX_ASSERT(0, kartId, cKartIDMax);
        _318[kart]._8 = kartId;
    }

    RaceTime get_318_RaceTime(int kart) {
#line 301
        JUT_MINMAX_ASSERT(0, kart, 2);
        return _318[kart].mTime;
    }

    void set_318_RaceTime(int kart, RaceTime time) {
#line 305
        JUT_MINMAX_ASSERT(0, kart, 2);
        _318[kart].mTime = time;
    }

    void set_318_14(int kart, int val) {
#line 313
        JUT_MINMAX_ASSERT(0, kart, 2);
        JUT_MINMAX_ASSERT(0, kart, 8); // compared against the wrong value?
        _318[kart]._14 = val;
    }

    void set_318_coop(int kart, bool coop) {
#line 297
        JUT_MINMAX_ASSERT(0, kart, 2);
        _318[kart]._C[0] = coop;
    }

    void set_318_Pad(int kart, KartGamePad *pad) {
#line 322
        JUT_MINMAX_ASSERT(0, kart, 2);
        _318[kart]._18 = pad;
    }
    ECharID get_318_charId0(int kart) {
#line 273
        JUT_MINMAX_ASSERT(0, kart, 2);
        return _318[kart]._0[0];
    }

    ECharID get_318_charId1(int kart) {
#line 273
        JUT_MINMAX_ASSERT(0, kart, 2);
        return _318[kart]._0[1];
    }

    EKartID get_318_kartId(int kart) {
#line 284
        JUT_MINMAX_ASSERT(0, kart, 2);
        return _318[kart]._8;
    }

    bool get_318_coop(int kart) {
#line 293
        JUT_MINMAX_ASSERT(0, kart, 2);
        return _318[kart]._C[0];
    }

    int get_318_rankPoint(int kart) {
#line 284
        JUT_MINMAX_ASSERT(0, kart, 2);
        return mRankPoint[kart];
    }

    int get_318_14(int kart) {
#line 309
        JUT_MINMAX_ASSERT(0, kart, 2);
        return _318[kart]._14;
    }

    KartGamePad *get_318_Pad(int kart) {
#line 318
        JUT_MINMAX_ASSERT(0, kart, 2);
        return _318[kart]._18;
    }


    void getDemoKart(int no, ECharID &charID1, ECharID &charID2, EKartID &kartID)
    {
#line 413
        JUT_MINMAX_ASSERT(0, no, 8);

        charID1 = mDemoKart[no].char1;
        charID2 = mDemoKart[no].char2;
        kartID = mDemoKart[no].kart;
    }

    void set_354(int val) { _354 =  val; }
    void setRaceLevel(ERaceLevel level) { mLevel = level; }
    void setGhostFlag(u8 flags) { mGhostFlags = flags; }

    void lockGame(int i) { // fabricated
        mGameFlag &= ~(1 << i);
    }

    void lockKart(int i) { // fabricated
        mSecretKart &= ~(1 << i);
    }

    bool tstGameUnlocked(int i) {
        return (mGameFlag & (1 << i)) != 0;
    }

    bool tstSecretKartUnlocked(int i) {
        return (mSecretKart & (1 << i)) != 0;
    }

    bool checkAnyGameUnlocked() {
        return mGameFlag != 0;
    }

    bool checkAnySecretKartUnlocked() {
        return mSecretKart != 0;
    }

    bool checkAnyUnlocked() { // fabricated
        return mGameFlag != 0 || mSecretKart != 0;
    }

    int getStartNo(int kartNo) {
#line 163
        JUT_MINMAX_ASSERT(0, kartNo, 8);
        return mStartNo[kartNo];
    }

    int getRankPoint(int kart) {
#line 226
        JUT_MINMAX_ASSERT(0, kart, 8);
        return mRankPoint[kart];
    }

    int get_80(int kart) {
#line 188
        JUT_MINMAX_ASSERT(0, kart, 4);
        return _80[kart];
    }

    int get_40(int kart, int rank) {
#line 200
        JUT_MINMAX_ASSERT(0, kart, 4);
#line 201
        JUT_MINMAX_ASSERT(0, rank, 4);
        return _40[kart][rank];
    }

    void checkKart(int kart) {
#line 192
        JUT_MINMAX_ASSERT(0, kart, 4);
    }

    void checkKart2(int kart) {
#line 205
        JUT_MINMAX_ASSERT(0, kart, 4);
    }

    void checkRank(int rank) {
#line 206
        JUT_MINMAX_ASSERT(0, rank, 4);
    }

    void inc_80(int kart) {
#line 188
        JUT_MINMAX_ASSERT(0, kart, 4);
        int v = _80[kart];
        checkKart(kart);
        int val = v + 1;
        if (val > 99)
            _80[kart] = 99;
        else
            _80[kart] = val;
    }

    void inc_40(int kart, int rank) {
#line 200
        JUT_MINMAX_ASSERT(0, kart, 4);
#line 201
        JUT_MINMAX_ASSERT(0, rank, 4);
        int v = _40[kart][rank];
        checkKart2(kart);
        checkRank(rank);
        int val = v + 1;
        if (val > 99)
            _40[kart][rank] = 99;
        else
            _40[kart][rank] = val;
    }

    int getCourse() { return mMapInfo.course; }
    int getCup() const { return mMapInfo.cup; }

    int get_298(int kart) {
#line 226
        return mRankPoint[kart];
    }

    int get_2d8(int rank) {
#line 240
        JUT_MINMAX_ASSERT(0, rank, 8);
        return _2d8[rank];
    }

    int get_2b8(int rank) {
#line 235
        JUT_MINMAX_ASSERT(0, rank, 8);
        return _2b8[rank];
    }

    int get_2f8(int rank) {
#line 245
        JUT_MINMAX_ASSERT(0, rank, 8);
        return _2f8[rank];
    }

    int getPrevRank(int rank) { // fabricated
#line 245
        JUT_MINMAX_ASSERT(0, rank, 8);
        int kart = _2d8[rank];
        int i;
        for (i = 0; i < 8; i++) {
            if (kart == _2f8[i]) {
                break;
            }
        }
        return i;
    }

    int get_3cc(int course) {
#line 424
        JUT_MINMAX_ASSERT(0, course, 16);
        return _3cc[course];
    }

    friend class SceneMap;
    friend class SceneMenu;
    friend class Goal2D;
    friend class Award2D;
    friend class SceneMapSelect;
    friend class PauseManager;

private:
    friend class SceneGhostLoadSave;
    friend class SceneCourseSelect;
    friend class SceneTitle;
    JMARandFast mRnd;          // 00
    int mStartNo[8];           // 004
    KartGamePad *mDecidePad;   // 024
    u8 _28;                    // 028
    int _2c;                   // 02C
    int _30;                   // 030
    u8 mGhostFlags;            // 034
    u8 _35[3];                 // 035
    int mBattleType;           // 038
    int mMapIndex;             // 03C
    int _40[4][4];             // 040
    int _80[4];                // 080
    struct {    
        int cup;               // 090
        int course;            // 094
    } mMapInfo;
    int _98[8][16];            // 098
    int mRankPoint[8];         // 298
    int _2b8[8];               // 2B8
    int _2d8[8];               // 2D8
    int _2f8[8];               // 2F8
#ifdef __MWERKS__
    union {
#endif
        struct {
            ECharID _0[2];     // 00
            EKartID _8;        // 08
            bool _C[0x10 - 0xc]; // 0C
            RaceTime mTime;    // 10
            int _14;           // 14
            KartGamePad *_18;  // 18
            // Size: 0x1c
        } _318[2];             // 318
#ifdef __MWERKS__
    };
#endif

public:
    int _350;                  // 350 (number of [human] players?)

private:
    int _354;                  //
    ERaceLevel mLevel;         //
    u8 _35c;                   //
    u8 _35d;                   //
    u8 _35e;                   //
    u8 _35f;                   //
    u8 _360;                   //
    int _364;                  //
    u16 mGameFlag;             // 368
    u16 mSecretKart;           // 36a
    struct {
        ECharID char1;
        ECharID char2;
        EKartID kart;
        // Size: 0xc
    } mDemoKart[8];            // 36C
    int _3cc[16];              // 3CC
};

extern SequenceInfo gSequenceInfo;

#endif
