#ifndef RIVALCTRLBASE_H
#define RIVALCTRLBASE_H

#include "types.h"

class RivalKart;
class RivalBodyCtrl;
class KartBody;
struct stRandom;

class RivalCtrlBase
{
public:
    class RankParam
    {
    public:
        RankParam(int);
        void reset();

        int getFirstRank() const { return mRankId; }
        void setRankId(const int &id) { mRankId = id; }
        void setKeepRank(const int &rank) { mKeepRank = rank; }

        int mFirstRank; // 0
        int mRankId;    // 4
        int mKeepRank;  // 8
        f32 mKeepSpd;   // c
    };

    RivalCtrlBase(RivalKart *);
    int getMyID();
    RivalBodyCtrl *getBodyCtrl();
    KartBody *getBody();
    stRandom *getRandom();
    u8 getRankParamID();
    bool tstEndRace();
    bool isRival() const;
    int getMyRank();
    // Inline/Unused
    // void getSpeedCtrl();
    // void getItemCtrl();

    virtual ~RivalCtrlBase() {} // 0x8027ea9c

    RivalKart *mRival;   // 4
    RankParam *mRankParam; // 8
}; // Size: 0xc

#endif