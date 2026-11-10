#ifndef LANPLAYINFO_H
#define LANPLAYINFO_H

#include "JSystem/JMath/JMath.h"
#include "Osako/LANSelectMode.h"
#include "Osako/ResMgr.h"
#include "kartEnums.h"

class LANPlayInfo {
public:
    void init();                       // 0x80218580
    ECourseID getCrsID();              // 0x8021865c
    ResMgr::CourseOrder getCrsOrder(); // 0x80218904
    void resumeInfo(LANSelectMode *);  // 0x8021890c
    void saveInfo(LANSelectMode *);    // 0x802189a4
    u8 getRandomMap(int, u32);         // 0x80218a48
    void shuffleKart(int);             // 0x80218ab4
    u8 getMaskedRandom(int, u32);      // 0x80218be4
    EKartID getRideKart(u8);           // 0x80218d10
    ECharID getFront(int);             // 0x80218ec4
    ECharID getBack(int);              // 0x80218ee4
    int getDisplayNum();               // 0x80218f04
    // Inline/Unused
    LANPlayInfo();

private:
    friend class LANEntry;

    struct Player {
        u8 mKart;
        u8 mFront;
        u8 mBack;
        u8 mUnk;
    };

    u8 mGameMode;          // 0x00
    u8 mPower;             // 0x01
    u8 mCourseSelect;      // 0x02
    u8 mCourse;            // 0x03
    u8 mCourseSel;         // 0x04
    u8 mOrder;             // 0x05
    u8 mTag;               // 0x06
    u8 mDisplay;           // 0x07
    u8 mIdSw;              // 0x08
    u8 mCrsCount;          // 0x09
    u8 _0a;                // 0x0a
    u8 _0b;                // 0x0b
    u32 mMask;             // 0x0c
    u32 mBattleMask;       // 0x10
    u32 mRaceMask;         // 0x14
    JMath::TRandom_fast_ mRandom; // 0x18
    Player mPlayers[8];    // 0x1c
    u8 mActive[8];         // 0x3c
    PLACEHOLDER_BYTES(0x44, 0x84);
};
extern LANPlayInfo gLANPlayInfo;

#endif // LANPLAYINFO_H
