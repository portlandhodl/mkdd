#ifndef DRIVERMGR_H
#define DRIVERMGR_H

#include "JSystem/J3D/J3DModel.h"
#include "JSystem/JKernel/JKRDisposer.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Osako/ResMgr.h"
#include "macros.h"
#include "types.h"

class DriverModel;

class DriverLoader {
public:

    enum AnmEnum {};
    enum AnmSoundEnum {};

    DriverLoader(int);
    static int convAnmIDToKeepID(DriverLoader::AnmEnum);
    void *getAnmFile(int);
    static int convSndIDToKeepID(DriverLoader::AnmSoundEnum);
    void *getSoundDataFile(int);
    static J3DModelData *createModelData(int dataID, int level);

    // Inline/Unused
    static void *getBmdFile(int, int);

    inline static const char *getDriverName(int no) {
#line 206
        JUT_MINMAX_ASSERT(0, no, 20);
        return mscpaDriverName[no];
    }

    inline static const char *getDriverFolderName2(int no) {
#line 212
        JUT_MINMAX_ASSERT(0, no, 20);
        return mscpaDriverFolderName2[no];
    }

    J3DModelData *getMdlData(int level) { return mMdlData[level]; }

    u16 getJntBindNo(u16 ctrlNo) {
#line 179
        JUT_MINMAX_ASSERT(0, ctrlNo, 27);
        return _c[ctrlNo];
    }

    u16 getJntNo(u16 i) const { return _c[i]; }

    static void checkAnmID(u8 anmID)
    {
#line 231
        JUT_MINMAX_ASSERT(0, anmID, 82);
    }


    static const u8 mscAnm2SoundList[82];
    static const u8 msAnmMRAMList[11];
    static const char mscaaMRAMAnmFileName[11][32];
    static const char mscaaMRAMAnmSoundFileName[1][32];
    static const char *sAnmFileNameTable[82];
    static const char *sAnmSoundFileNameTable[12];
    static const char *mscpaDriverName[20];
    static const char *mscpaDriverFolderName2[20];
    static const char *mscaaJntName[27];
    static const char *sModelFileNameTable[2];
    static const char *sDriverFolderName3;
    static const u8 msAnmSoundMRAMList[1];
    static const u16 mscaBoneJnt[4];

private:
    int mDataID;
    J3DModelData *mMdlData[2];
    u16 _c[27];
    f32 _44[4];
}; // class DriverLoader

class DriverManager : JKRDisposer {
public:
    DriverManager();
    ~DriverManager();
    static DriverManager *ptr();
    const char *getFolderName_Sound(int, int);
    void SetDriverModelPtr(DriverModel *);

    u8 getGoodB2Flag(int dataID) { return _7e0[dataID]; }
    u8 getDriverFlag(int dataID) { return _7b8[dataID]; }
    u8 getSpeakFlag(int dataID) { return _7cc[dataID]; }
    u8 getHokanDamageCnt() { return _24; }
    int getBlendTime(int i) { return mBlendTimes[i]; }

    // Inline/Unused
    bool IsAllARAMDMAFinished();
    void CheckARAMDMA(f64, int);

    static DriverManager *mspDriverManager;
private:
    f32 _18;
    f32 _1c;
    f32 _20;
    u8 _24;
public:
    f32 _28;
private:
    u8 _2c;
    J3DModelData *mMdlData;
public:
    u8 mBlendTimes[0x27];
private:
public:
    u8 _5b;
    u8 _5c;
    u8 _5d;
    u8 _5e;
    u8 _5f;
private:
    u8 _60[20][82];
    u8 _6c8[20][12];
    u8 _7b8[20];
    u8 _7cc[20];
    u8 _7e0[20];
    u8 mMdlCnt;                      // 7f4
    DriverModel *mMdls[16];          // 7f8, unknown count as of now
public:
    u16 _838;
    s16 _83a;
    s16 _83c;
    f64 _840;
    int _848;
    PLACEHOLDER_BYTES(0x84c, 0x858);
    // Size: 0x858 
}; // class DriverManager


#endif // DRIVERMGR_H
