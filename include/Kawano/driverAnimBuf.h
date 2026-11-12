#ifndef DRIVERANIMBUF_H
#define DRIVERANIMBUF_H

#include "JSystem/J3D/J3DAnmTransform.h"
#include "types.h"

class DriverBuf {
public:
    DriverBuf();
    ~DriverBuf();
    bool readResAsync(char *, char *);
    void setID(int);
    int CheckBufID(int);

    void *mBuf;      // 0x00
    u32 mSize;       // 0x04
    int mID;         // 0x08
    int mLoadState;  // 0x0c
    int mUsed;       // 0x10
}; // Size: 0x14

class DriverAnimBuf : public DriverBuf {
public:
    DriverAnimBuf();
    void reset();
    ~DriverAnimBuf();
    void changeResource();

    J3DAnmTransform *mAnm; // 0x14
}; // Size: 0x18

class DriverSoundBuf : public DriverBuf {
public:
    DriverSoundBuf();
    void reset();
    ~DriverSoundBuf();
}; // Size: 0x14

class DriverAnimBufMgr {
public:
    DriverAnimBufMgr();
    void reset();
    int CheckAndLoadAnimAvailable(int, int);
    void CheckAndLoadAnimSoundAvailable(int, int);
    void LoadAnimation(int, int, int);
    int loadAnimation(int, int);
    void LoadAnimationSound(int, int);
    int CheckRenew();
    int CheckRenewSound();
    bool IsARAMDMAFinished();
    void Check();
    void resetFrame();
    void resetFrameSound();
    DriverAnimBuf *searchBuffer(int);
    void *GetPtr(int);
    void *GetSoundPtr(int);

    int mRenew;                 // 0x00
    int mRenewSound;            // 0x04
    DriverAnimBuf mAnimBufs[4]; // 0x08
    DriverSoundBuf mSoundBuf;   // 0x68
}; // Size: 0x7c

#endif // DRIVERANIMBUF_H
