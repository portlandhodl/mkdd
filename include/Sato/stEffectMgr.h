#ifndef STEFFECTMGR_H
#define STEFFECTMGR_H

#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JGeometry/Vec.h"
#include "dolphin/mtx.h"
#include "types.h"

struct CrsGround;
struct stEmitterCallBack;
struct stGndAttrDataIndex;
struct stGndAttrParam;
struct stTireEfctStatusSet;
struct stTireEffectStatus;

struct stEffectMgr {
    struct stExhaustAccelCounter {
        u8 _00[0x10];
    };

    struct stWaterEffectStatus {
        u8 _00[0x38];
    };

    struct stTireEffectStatus {
        u8 _00[0x2c];
    };

    struct stTireEfctStatusSet {
        stTireEffectStatus mStatus[2];
    };

    struct stClipParam {
        u8 _00[2];
    };

    stEffectMgr();                                                                // 0x802360d4
    void createEmitterAndParticles();                                             // 0x8023647c
    void createKartExhaustMtx();                                                  // 0x8023677c
    void model_init();                                                            // 0x80236964
    void createModel(JKRSolidHeap *, u32);                                        // 0x80236c78
    void init();                                                                  // 0x80236c98
    void initCallBackPtr(stEmitterCallBack *, s16, s16);                          // 0x80237664
    void reset();                                                                 // 0x80237670
    void calc();                                                                  // 0x80237ab8
    void calcGndAttrEffect();                                                     // 0x80237b98
    void calcGndCenterEffect(int, bool);                                          // 0x80237cf8
    void calcGndTireEffect(int, bool, stTireEfctStatusSet *);                     // 0x80237ea4
    void calcDriftSparkEfct(int, u8, stTireEffectStatus *, u32);                  // 0x802382cc
    void calcSideGEfct(int, int, stTireEffectStatus *, bool);                     // 0x80238398
    void calcSpinTurnEfct(int, int, stTireEffectStatus *, bool);                  // 0x80238538
    void calcDriftEfct(int, int, stTireEffectStatus *, bool);                     // 0x8023864c
    void createDriftEfct(int, int, stTireEffectStatus *, int);                    // 0x80238788
    void calcGndCharEffect(int, bool);                                            // 0x80238864
    void calcWaterAttrEffect(int);                                                // 0x80238b00
    void findParamPtrTire(int, CrsGround *, stTireEffectStatus *, u8);            // 0x80238ebc
    void findParamPtrCenter(int, CrsGround *, stTireEffectStatus *, u8);          // 0x802394bc
    void findParamPtrChar(int, CrsGround *, stTireEffectStatus *, u8);            // 0x80239ad8
    void findParamPtrCommon(int);                                                 // 0x80239fc4
    void chkAvailablePtrList(int, int);                                           // 0x8023a060
    void calcAttrRoad(int, int, stTireEffectStatus *, u8);                        // 0x8023a1d8
    void calcAttrRoadCenter(int, stTireEffectStatus *, u8);                       // 0x8023a49c
    void calcAttrRoadCharCenter(int, stTireEffectStatus *, u8);                   // 0x8023a5e0
    void calcAttrWeedCharCenter(int, stTireEffectStatus *, u8);                   // 0x8023a6e8
    void setDriftStatus(int, int, stTireEffectStatus *);                          // 0x8023a7f0
    void draw(u32);                                                               // 0x8023a910
    void update();                                                                // 0x8023a930
    void viewCalc(u32);                                                           // 0x8023a934
    void setCurrentViewNo(u32);                                                   // 0x8023a938
    stWaterEffectStatus *getWaterTrack(int);                                      // 0x8023a93c
    Mtx *getKartExhaustMtxL(int);                                                 // 0x8023a9d4
    Mtx *getKartExhaustMtxR(int);                                                 // 0x8023aa6c
    f32 getEfctCarSpeed(int) const;                                               // 0x8023ab04
    stExhaustAccelCounter *getExhaustAccelCounter(int);                           // 0x8023abb4
    void getExhaustPosDir(JGeometry::TVec3f *, JGeometry::TVec3f *, int, u8);     // 0x8023ac64
    void createEmitter(stEmitterCallBack *, u16);                                 // 0x8023adb8
    void createKartCommonEmitter(int, stEmitterCallBack *, u16, bool);            // 0x8023ae88
    void createKinokoDashEmt(int);                                                // 0x8023b05c
    u8 tstUseMufllerL(int);                                                       // 0x8023b294
    u8 tstUseMufllerR(int);                                                       // 0x8023b328
    void setExhaustStopFlg(int, bool);                                            // 0x8023b3bc
    u8 tstIsAvailableDistance(int) const;                                       // 0x8023b4f0
    void resetIndex();                                                            // 0x8023b584
    void loadCourseParam(u32);                                                    // 0x8023b834
    void setIndexToList_Common(u8, const stGndAttrParam &);                       // 0x8023bc04
    void setIndexToList_StdGnd(u8, const stGndAttrParam &, stGndAttrDataIndex &); // 0x8023bccc
    void *getEmitterPtr(char *);                                                  // 0x8023bd74
    void *getBtiPtr(char *);                                                      // 0x8023bdc0
    char *getSnpName(u32);                                                        // 0x8023be0c

    static void createMgr() { mMgr = new stEffectMgr(); }
    static stEffectMgr *getMgr() { return mMgr; }
    static stEffectMgr *mMgr; // 0x80416b40

    u8 _00[0xf0];
    stExhaustAccelCounter *mpExhaustAccelCounter; // 0xf0
    u8 *_f4;                                      // 0xf4
    u8 _f8[0x14];                                 // 0xf8
    u8 **mpExhaustStopL;                          // 0x10c
    u8 **mpExhaustStopR;                          // 0x110
    u8 _114[0x268];                               // 0x114
    stWaterEffectStatus *mpWaterEffectStatus;     // 0x37c
    u8 _380[0xff8];                               // 0x380
    Mtx *mpKartExhaustMtxL;                       // 0x1378
    Mtx *mpKartExhaustMtxR;                       // 0x137c
    u8 *mpMufllerL;                               // 0x1380
    u8 *mpMufllerR;                               // 0x1384
    f32 *mpEfctCarSpeed;                          // 0x1388
    u32 _138c;                                    // 0x138c
    u8 mIsAvailableDistance[8];                   // 0x1390
};

inline void CreateStEfctMgr() { stEffectMgr::createMgr(); }
inline stEffectMgr *GetStEfctMgr() { return stEffectMgr::getMgr(); }

#endif
