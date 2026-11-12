#ifndef KARTCANNON_H
#define KARTCANNON_H

#include <JSystem/JGeometry/Vec.h>

#include "types.h"

class KartBody;
class JugemPoint;

class KartCannon
{
public:
    KartCannon() {}
    void Init(int);            // 0x8030273c
    void MakeCannon();         // 0x80302788
    void DoKeep();             // 0x80302abc
    void SetPose();            // 0x80302b30
    void DoCatch();            // 0x80302f20
    int DoLaunch();           // 0x80302fac
    bool DoFlight();           // 0x80303058
    bool DoTouchDown();        // 0x803031cc
    bool DoLanding();          // 0x80303294
    bool DoCheckEnd();         // 0x8030330c
    int DoRainbowLaunch();    // 0x80303474
    bool DoRainbowFlight();    // 0x80303520
    bool DoRainbowTouchDown(); // 0x803036c8
    bool DoRainbowLanding();   // 0x80303794
    bool DoRainbowCheckEnd();  // 0x80303810
    void DoCannonCrl();        // 0x80303900
    void DoDonkyCannonCrl();   // 0x80303934
    void DoRainbowCannonCrl(); // 0x80303b80
    void DoAfterCannonCrl();   // 0x80303dd4

    JugemPoint *mPoint;    // 0x00
    KartBody *mBody;       // 0x04
    u8 mFlags;             // 0x08
    u8 _9;                 // 0x09
    u8 mState;             // 0x0a
    u8 mCounter;           // 0x0b
    u16 mTimer;            // 0x0c
    u8 _e[0x14 - 0xe];     //
    f32 _14;               // 0x14
    u8 _18[0x1c - 0x18];   //
    JGeometry::TVec3f _1c; // 0x1c
    JGeometry::TVec3f _28; // 0x28
    JGeometry::TVec3f _34; // 0x34
};
#endif // KARTCANNON_H