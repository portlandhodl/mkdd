#ifndef PAUSEMANAGER_H
#define PAUSEMANAGER_H

#include "JSystem/JKernel/JKRHeap.h"
#include "Osako/kartPad.h"
#include "types.h"

class JKRArchive;

class PauseManager {
public:
    PauseManager(JKRHeap *);     // 0x801343d0
    void reset();                // 0x801345fc
    void draw();                 // 0x801346d0
    void exec();                 // 0x801348f8
    static bool tstPause();      // 0x80135288
    void wipeOut(int);           // 0x80135290
    static int getPauseChoice(); // 0x801352d0
    void setTA();                // 0x8013541c
    // Inline/Unused
    ~PauseManager();
    void setPause();
    static void clrPause();
    void wipeIn();
    void getCharRankPos(int);

    void setResultStartFlag(bool flag) {mResultStart = flag; }
    void setGameOverFlag(bool flag) { mGameOver = flag; }
    void setExecFlag(bool flag) { mExec = flag; }
    void setPauseEndFlag(bool flag) { mPauseEnd = flag; }
    
    void enablePause() { _14 = 0; }

    static PauseManager *getManager() {
        return mThis;
    }

    bool isResultStart() const { return mResultStart; }
    JKRArchive *getArchive() const { return mArchive; }

private:
    static PauseManager *mThis;    // 0x80416298
    static bool mIsPause;          // 0x8041629c
    static int mPauseSelector;     // 0x804162a0
    static int mPauseChoice;       // 0x804162a4
    static int mPauseNextChoice;   // 0x804162a8
    static KartGamePad *mPausePad; // 0x804162ac

    bool _0;
    JKRArchive *mArchive;                    // 0x04
    class PrintMemoryCard *mPrintMemoryCard; // 0x08
    bool mMemCard;                           // 0x0c
    bool _d;                                 // 0x0d
    bool mResultStart;                       // 0x0e
    bool _f;                                 // 0x0f
    bool mGameOver;                          // 0x10
    bool _11;                                // 0x11
    bool _12;                                // 0x12
    bool mExec;                              // 0x13
    int _14;                                 // 0x14
    bool mPauseEnd;                          // 0x18
    class Pause2D *mPause2D;                 // 0x1c
    class Result2D *mResult2D;               // 0x20
    class J2DOrthoGraph *mOrtho;             // 0x24
    int _28;                                 // 0x28
    int mWipeState;                          // 0x2c
    int mWipeType;                           // 0x30
    int _34;                                 // 0x34
    bool _38;                                // 0x38
};

inline PauseManager *GETPauseManager() { return PauseManager::getManager(); }

#endif
