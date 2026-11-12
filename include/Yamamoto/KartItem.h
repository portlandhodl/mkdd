#ifndef KARTITEMSTRAT_H
#define KARTITEMSTRAT_H

#include "types.h"

class KartItem
{
public:
    KartItem() {}
    void Init(int);
    void DoRabbit();
    void SetSucMashItem();
    void CheckHitItem();
    void FallStealItem(int, int);
    void FallItem();
    void FallAllItem();
    void ReleseWanWan();
    bool CompulsionReleseWanWan();
    void KillWanWan();
    bool IsMyItem();
    bool IsChanging();
    void DoTurtleReleaseDir(int, int);
    void DoBananaReleaseDir();
    void DoTurtleReleaseDynamicDir(u32);
    void DoBananaReleaseDynamicDir();
    void TeamRobItm();
    bool SuspendItem();
    void CheckOtherAnime();
    void DoOtherAnime();
    void DoTandemItemAnime();
    void DoTandemItemRelease();
    void DoItem();
    // Inline/Unused
    // void DoDriverTurtleReleaseDir(int);
    // void DoDriverBananaReleaseDir();
    // void DoBarrelReleaseDir(int, int);
    // void DoJumperReleaseDir();
    // void DoTurtleSpReleaseDir(int);
    // void DoBananaSpReleaseDir();
    // void DoDriverTurtleReleaseDynamicDir();
    // void DoDriverBananaReleaseDynamicDir();
    // void DoBarrelReleaseDynamicDir();
    // void DoDriverItemAnime();
    // void DoDriverItemRelease();
    class KartBody *mBody; // 0
    u8 _4;                 // 4
    u16 _6;                // 6
    u8 _8;                 // 8
    u8 _9;                 // 9
    u8 _a[2];              // a

}; // 0xc

#endif // KARTITEMSTRAT_H
