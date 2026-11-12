#ifndef AWARD2D_H
#define AWARD2D_H

#include "JSystem/J2D/J2DScreen.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "Osako/GPRecord.h"
#include "types.h"

class NameEntryHelper;

class Award2D {
public:
    Award2D(JKRHeap *); // 0x8020f884
    void calc();        // 0x80210f48
    void draw();        // 0x80211254

    class OKButton {
        friend class Award2D;
    public:
        void calcAnm(); // 0x802112cc
        void zoomIn();  // 0x802113d8
        void zoomOut(); // 0x802113f4
    private:
        int _0;
        s16 _4;
        s16 _6;
        u8 *_8;
        u8 *_c;
        u8 *_10;
    };

    void still() { mState = 0; }
    void start() { mState = 1; }
    bool isFinish() { return mState == 6; }

    J2DScreen *mScreen;      // 0x00
    s16 _4;                  // 0x04
    s16 _6;                  // 0x06
    u8 _8;                   // 0x08
    char _9[7];              // 0x09
    int mState;              // 0x10
    GPRecord _14;            // 0x14
    u32 _24;                 // 0x24
    u32 _28;                 // 0x28
    u8 *_2c;                 // 0x2c
    NameEntryHelper *_30;    // 0x30
    OKButton _34;            // 0x34
    u8 *_48;                 // 0x48
    u8 *_4c;                 // 0x4c
    u8 *_50;                 // 0x50
    u8 *_54;                 // 0x54
};

#endif
