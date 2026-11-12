#ifndef STEFFECTMGR_H
#define STEFFECTMGR_H

#include "Sato/stEmitter.h"
#include "Sato/stEfctExhaust.h"
#include "Sato/stParticle.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JGeometry/Vec.h"
#include "dolphin/mtx.h"
#include "types.h"

struct CrsGround;
class JUTTexture;
struct stEfctWaterTrack;
struct stEfctWaterSpray;
struct stEfctWaterSprayCenter;
struct stEfctWaterBubble;
struct stEfctCrsWaterSpray;
struct stEfctCrsWaterBubble;

struct stEmitterPtclMgrEx : stEmitterPtclMgr {
    stEmitterPtclMgrEx()
    {
        mFreeList = nullptr;
        mList.initiate();
    }
};

struct stEffectMgr {
    struct stWaterEffectStatus {
        stWaterEffectStatus();
        void init()
        {
            for (u8 i = 0; i < 2; i++) {
                mTrack[i] = nullptr;
                mBubble[i] = nullptr;
                mSpray[i] = nullptr;
            }
        }
        stEmitter *mSpray[2];  // 0x00
        stEmitter *mBubble[2]; // 0x08
        stEmitter *mTrack[2];  // 0x10
    }; // Size: 0x18

    struct stTireEffectStatus {
        stTireEffectStatus();
        void init()
        {
            mAttr = 0xff;
            _25 = 0;
            _24 = 0;
            mAttrIdx = 0;
            for (u8 i = 0; i < 3; i++)
                mDriftEmitters[i] = nullptr;
            for (u8 i = 0; i < 2; i++) {
                mEmitter[i] = nullptr;
                mIndex[i] = 0xffff;
                mMaterial[i] = 0;
            }
            mSideGEmitter = nullptr;
            mSpinTurnEmitter = nullptr;
        }
        stEmitter *mEmitter[2];        // 0x00
        stEmitter *mSideGEmitter;      // 0x08
        stEmitter *mSpinTurnEmitter;   // 0x0c
        stEmitter *mDriftEmitters[3];  // 0x10
        int mAttr;                     // 0x1c
        u32 mAttrIdx;                  // 0x20
        u8 _24;
        u8 _25;
        u16 mIndex[2];   // 0x26
        u8 mMaterial[2]; // 0x2a
    }; // Size: 0x2c

    struct stTireEfctStatusSet {
        stTireEfctStatusSet();
        stTireEffectStatus mStatus[2];
    };

    struct stGndAttrParam {
        u8 mAttr;      // 0x00
        u8 mAttrIdx;   // 0x01
        u8 mKind;      // 0x02
        char mEmitterName[6][0x18]; // 0x03
        char mBtiName[0x18];        // 0x93
        u8 mMaterial;  // 0xab
        u8 mClip0;     // 0xac
        u8 mClip1;     // 0xad
    }; // Size: 0xae

    struct stGndAttrDataIndex {
        void init()
        {
            for (u8 i = 0; i < 20; i++) {
                mChar[i] = -1;
                mCenter[1][i] = -1;
                mCenter[0][i] = -1;
                mTire[1][i] = -1;
                mTire[0][i] = -1;
                mMaterial[1][i] = 0xff;
                mMaterial[0][i] = 0xff;
            }
        }
        int mTire[2][20];    // 0x000
        int mCenter[2][20];  // 0x0a0
        int mChar[20];       // 0x140
        u8 mMaterial[2][20]; // 0x190
    }; // Size: 0x1b8

    struct stClipParam {
        stClipParam();
        u8 _00;
        u8 _01;
    };

    enum cKindTexture {
        cKindTexture_Tool01,
        cKindTexture_Tool02,
        cKindTexture_Tool03,
        cKindTexture_Max
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
    bool calcWaterAttrEffect(int);                                                // 0x80238b00
    stEmitterCallBack::stFileParamPtrList *findParamPtrTire(int, CrsGround *, stTireEffectStatus *, u8);   // 0x80238ebc
    stEmitterCallBack::stFileParamPtrList *findParamPtrCenter(int, CrsGround *, stTireEffectStatus *, u8); // 0x802394bc
    stEmitterCallBack::stFileParamPtrList *findParamPtrChar(int, CrsGround *, stTireEffectStatus *, u8);   // 0x80239ad8
    stEmitterCallBack::stFileParamPtrList *findParamPtrCommon(int);                                        // 0x80239fc4
    stEmitterCallBack::stFileParamPtrList *chkAvailablePtrList(int, int);                                  // 0x8023a060
    stEmitterCallBack *getCallBackPtr(u8, int, int);                              // UNUSED
    void calcAttrRoad(int, int, stTireEffectStatus *, u8);                        // 0x8023a1d8
    void calcAttrRoadCenter(int, stTireEffectStatus *, u8);                       // 0x8023a49c
    void calcAttrRoadCharCenter(int, stTireEffectStatus *, u8);                   // 0x8023a5e0
    void calcAttrWeedCharCenter(int, stTireEffectStatus *, u8);                   // 0x8023a6e8
    int setDriftStatus(int, int, stTireEffectStatus *);                          // 0x8023a7f0
    void draw(u32);                                                               // 0x8023a910
    void update();                                                                // 0x8023a930
    void viewCalc(u32);                                                           // 0x8023a934
    void setCurrentViewNo(u32);                                                   // 0x8023a938
    JUTTexture *getTexture(cKindTexture) const;                                   // UNUSED
    stEfctWaterTrack *getWaterTrack(int);                                         // 0x8023a93c
    Mtx *getKartExhaustMtxL(int);                                                 // 0x8023a9d4
    Mtx *getKartExhaustMtxR(int);                                                 // 0x8023aa6c
    f32 getEfctCarSpeed(int) const;                                               // 0x8023ab04
    stExhaustAccelCounter *getExhaustAccelCounter(int);                           // 0x8023abb4
    void getExhaustPosDir(JGeometry::TVec3f *, JGeometry::TVec3f *, int, u8);     // 0x8023ac64
    stEmitter *createEmitter(stEmitterCallBack *, u16);                                 // 0x8023adb8
    stEmitter *createKartCommonEmitter(int, stEmitterCallBack *, u16, bool);            // 0x8023ae88
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
    char *getBtiName(cKindTexture);                                               // UNUSED
    char *getSnpName(u32);                                                        // 0x8023be0c

    static void createMgr() { mMgr = new stEffectMgr(); }
    static stEffectMgr *getMgr() { return mMgr; }
    static stEffectMgr *mMgr; // 0x80416b40

    stEmitterPtclMgrEx mEmtMgr0;                 // 0x000
    stEmitterPtclMgrEx mFreeEmt0;                // 0x010
    stParticleDynamics mDynamics0;               // 0x020
    stParticle *mParticles0;                     // 0x04c
    stEmitterPtclMgrEx mEmtMgr1;                 // 0x050
    stEmitterPtclMgrEx mFreeEmt1;                // 0x060
    stParticleDynamics mDynamics1;               // 0x070
    stParticle *mParticles1;                     // 0x09c
    stEmitterPtclMgrEx mEmtMgr2;                 // 0x0a0
    stEmitterPtclMgrEx mFreeEmt2;                // 0x0b0
    stParticleDynamics mDynamics2;               // 0x0c0
    stParticle *mParticles2;                     // 0x0ec
    stExhaustAccelCounter *mpExhaustAccelCounter; // 0xf0
    stEmitter **mpEmitterTemplate;                // 0xf4
    stClipParam *mpClipParam;                     // 0xf8
    JUTTexture *mpTexture[3];                     // 0xfc
    u32 *mpTexList;                               // 0x108
    stEfctExhaust **mpExhaustL;                   // 0x10c
    stEfctExhaust **mpExhaustR;                   // 0x110
    stEfctKinokoDashBase *mpKinokoDash[2][3];     // 0x114
    stEmitterCallBack *mpRoadCb[8][2];            // 0x12c
    stEmitterCallBack *mpDriftCreateCb[8][2];     // 0x16c
    stEmitterCallBack *mpDriftChipCb[8][2];       // 0x1ac
    stEmitterCallBack *mpDriftCb[8][2];           // 0x1ec
    stEmitterCallBack *mpTireSpinCb[8][2];        // 0x22c
    stEmitterCallBack *mpSpinTurnCb[8][2];        // 0x26c
    stEmitterCallBack *mpWeedCb[8][2];            // 0x2ac
    stEmitterCallBack *mpRoadCenterCb[2][8];      // 0x2ec
    stEmitterCallBack *mpRoadCharCenterCb[8];     // 0x32c
    stEmitterCallBack *mpWeedCharCenterCb[8];     // 0x34c
    stEfctWaterSpray *mpWaterSpray[2];            // 0x36c
    stEfctWaterBubble *mpWaterBubble[2];          // 0x374
    stEfctWaterTrack *mpWaterTrack;               // 0x37c
    stEfctWaterSprayCenter *mpWaterSprayCenter;   // 0x380
    stEfctCrsWaterSpray *mpCrsWaterSpray[2];      // 0x384
    stEfctCrsWaterBubble *mpCrsWaterBubble[2];    // 0x38c
    int mDriftCnt[8];                             // 0x394
    u32 _3b4;                                     // 0x3b4
    u32 _3b8;                                     // 0x3b8
    u32 mDriftParam[3];                           // 0x3bc
    u32 _3c8;                                     // 0x3c8
    u32 _3cc;                                     // 0x3cc
    u32 _3d0;                                     // 0x3d0
    u8 _3d4;
    u8 _3d5;
    u8 _3d6;
    u8 _3d7;
    u32 _3d8;
    u32 _3dc;
    u8 _3e0;
    u8 _3e1;
    u8 _3e2;
    u8 _3e3;
    stGndAttrDataIndex mAttrData[9];              // 0x3e4
    stEmitterCallBack::stFileParamPtrList *mpPtrList; // 0x135c
    stTireEfctStatusSet *mpTireStatusSet;         // 0x1360
    stTireEffectStatus *mpCenterStatus;           // 0x1364
    stTireEffectStatus *mpCharStatus;             // 0x1368
    stWaterEffectStatus *mpWaterStatus;           // 0x136c
    Mtx *mpKartBodyMtxA;                          // 0x1370
    Mtx *mpKartBodyMtxB;                          // 0x1374
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
