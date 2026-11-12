#ifndef CHARACTERSELECT3D_H
#define CHARACTERSELECT3D_H

#include "JSystem/J3D/J3DAnmTexPattern.h"
#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/J3D/J3DAnmTransform.h"
#include "JSystem/J3D/J3DDrawBuffer.h"
#include "JSystem/J3D/J3DMtxCalc.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/JGeometry.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JORReflexible.h"

#include "Kaneshige/ExModel.h"
#include "Kaneshige/HioMgr.h"
#include "Kaneshige/RaceLight.h"
#include "Kaneshige/RaceMgr.h"

class CharacterSelect3DParam : public JORReflexible
{
public:
    CharacterSelect3DParam();
    virtual ~CharacterSelect3DParam() {}

    u8 mAdjust;                       // 0x04
    u8 _5[3];                         // 0x05
    f32 mFovy;                        // 0x08
    JGeometry::TVec3f mEye;           // 0x0c
    JGeometry::TVec3f mTarget;        // 0x18
    JGeometry::TVec3f mBodyPos[2];    // 0x24
    JGeometry::TVec3f mBodyTarget[2]; // 0x3c
    f32 mBodyScale[2];                // 0x54
    JGeometry::TVec3f mKartPos;       // 0x5c
    JGeometry::TVec3f mKartTarget;    // 0x68
    f32 mKartScale;                   // 0x74
};

class CharacterSelect3D // oh great, unnamed classes
{
public:
    enum CHARACTER_STATUS
    {
        STATUS_0,
        STATUS_1,
        STATUS_2
    };

    struct CharacterModel
    {
        J3DModelData *mModelData;         // 0x00
        ExModel *mModel;                  // 0x04
        struct
        {
            J3DAnmTransform *mAnmTransform;
            J3DMtxCalc *mMtxCalc;
        } mAnm[3];                        // 0x08
        J3DMtxCalc *mMtxCalc01;           // 0x20
        J3DMtxCalc *mMtxCalc02;           // 0x24
        J3DMtxCalc *mMtxCalc12;           // 0x28
        J3DAnmTexPattern *mAnmTexPattern; // 0x2c
    }; // Size: 0x30


    CharacterSelect3D(JKRHeap *);                                                     // 0x80183a60
    void init();                                                                      // 0x80184910
    ~CharacterSelect3D();                                                             // 0x801849f0
    void draw(int, f32);                                                              // 0x80184a44
    void draw(int, f32, JGeometry::TVec3f, JGeometry::TVec3f);                        // 0x80184e60
    void calc();                                                                      // 0x801851d0
    void setCharacter(int, int, ECharID, JGeometry::TVec3f, JGeometry::TVec3f, f32);  // 0x80185b58
    void setCharacterStatus(int player, int position, CHARACTER_STATUS);              // 0x80185d20
    void setKart(int, EKartID, JGeometry::TVec3f, JGeometry::TVec3f, f32);            // 0x80185ef0
    int isCancel(int);                                                               // 0x80186054
    int isNext(int);                                                                 // 0x801860a8
    MtxPtr getCameraMatrix();                                                         // 0x801860fc
    f32 getFovy();                                                                    // 0x80186104
    void calcCharacter(int, int, ECharID, JGeometry::TVec3f, JGeometry::TVec3f, f32); // 0x80186114
    void drawKart(EKartID);                                                           // 0x80186938
    void calcRotTrans(Mtx, JGeometry::TVec3f, JGeometry::TVec3f);                     // 0x80186b90

    static CharacterSelect3D *ptr() { return mCharacterSelect3D; }

    static CharacterSelect3D *create(JKRHeap *heap) {
        if (mCharacterSelect3D == nullptr) {
            mCharacterSelect3D = new (heap, 0) CharacterSelect3D(heap);
        }
        return mCharacterSelect3D;
    }

    static const f32 mCharacterDefScale[20];      // 0x80372ec0
    static const f32 mKartDefScale[21];           // 0x80372f10
    static const f32 mMarioBody[9];               // 0x80372f64
    static const f32 mDkBody[9];                  // 0x80372f88
    static const f32 mYoshiBody[9];               // 0x80372fac
    static const f32 mNokoBody[9];                // 0x80372fd0
    static const f32 mPeachBody[9];               // 0x80372ff4
    static const f32 mBabyBody[9];                // 0x80373018
    static const f32 mWarioBody[9];               // 0x8037303c
    static const f32 mKoopaBody[9];               // 0x80373060
    static const f32 mDiddyBody[9];               // 0x80373084
    static const f32 mKoopaJrBody[9];             // 0x803730a8
    static const f32 mKinopioBody[9];             // 0x803730cc
    static const f32 mCatherineBody[9];           // 0x803730f0
    static const f32 mWaluigiBody[9];             // 0x80373114
    static const f32 mKingTeresaBody[9];          // 0x80373138
    static const f32 mBonusBody[9];               // 0x8037315c
    static const f32 mMarioArm[4][9];             // 0x80373180
    static const f32 mDkArm[4][9];                // 0x80373210
    static const f32 mYoshiArm[4][9];             // 0x803732a0
    static const f32 mNokoArm[4][9];              // 0x80373330
    static const f32 mPeachArm[4][9];             // 0x803733c0
    static const f32 mBabyArm[4][9];              // 0x80373450
    static const f32 mWarioArm[4][9];             // 0x803734e0
    static const f32 mKoopaArm[6][9];             // 0x80373570
    static const f32 mDiddyArm[6][9];             // 0x80373648
    static const f32 mKoopaJrArm[4][9];           // 0x80373720
    static const f32 mKinopioArm[4][9];           // 0x803737b0
    static const f32 mCatherineArm[4][9];         // 0x80373840
    static const f32 mWaluigiArm[4][9];           // 0x803738d0
    static const f32 mKingTeresaArm[4][9];        // 0x80373960
    static const f32 mBonusArm[4][9];             // 0x803739f0
    static const f32 mMarioDump[4][9];            // 0x80373a80
    static const f32 mDkDump[4][9];               // 0x80373b10
    static const f32 mYoshiDump[4][9];            // 0x80373ba0
    static const f32 mNokoDump[4][9];             // 0x80373c30
    static const f32 mPeachDump[4][9];            // 0x80373cc0
    static const f32 mBabyDump[4][9];             // 0x80373d50
    static const f32 mWarioDump[4][9];            // 0x80373de0
    static const f32 mKoopaDump[6][9];            // 0x80373e70
    static const f32 mKoopaJrDump[4][9];          // 0x80373f48
    static const f32 mCatherineDump[4][9];        // 0x80373fd8
    static const f32 mWaluigiDump[4][9];          // 0x80374068
    static const f32 mKingTeresaDump[4][9];       // 0x803740f8
    static const f32 mBonusDump[4][9];            // 0x80374188
    static const f32 mMarioTire[4][9];            // 0x80374218
    static const f32 mDkTire[4][9];               // 0x803742a8
    static const f32 mYoshiTire[4][9];            // 0x80374338
    static const f32 mNokoTire[4][9];             // 0x803743c8
    static const f32 mPeachTire[4][9];            // 0x80374458
    static const f32 mBabyTire[4][9];             // 0x803744e8
    static const f32 mWarioTire[4][9];            // 0x80374578
    static const f32 mKoopaTire[6][9];            // 0x80374608
    static const f32 mDiddyTire[6][9];            // 0x803746e0
    static const f32 mKoopaJrTire[4][9];          // 0x803747b8
    static const f32 mKinopioTire[4][9];          // 0x80374848
    static const f32 mCatherineTire[4][9];        // 0x803748d8
    static const f32 mWaluigiTire[4][9];          // 0x80374968
    static const f32 mKingTeresaTire[4][9];       // 0x803749f8
    static const f32 mBonusTire[4][9];            // 0x80374a88
    static void *mDiddyDump;                      // 0x80416358
    static void *mKinopioDump;                    // 0x8041635c
    static CharacterSelect3D *mCharacterSelect3D; // 0x80416360
private:
    HioNode *mNode;                   // 0x0000
    int mSceneID;                     // 0x0004
    J3DDrawBuffer *mDrawBufferA[4];   // 0x0008
    J3DDrawBuffer *mDrawBufferB[4];   // 0x0018
    CharacterModel mCharacterModel[21]; // 0x0028
    J3DModelData *mAccessoryModelData;  // 0x0418
    ExModel *mAccessoryModel;           // 0x041c
    J3DAnmTextureSRTKey *mAccessoryAnm; // 0x0420
    f32 mAccessoryScale;                // 0x0424
    J3DModelData *mKartModelData[21][5]; // 0x0428
    ExModel *mKartModel[21];             // 0x05cc
    ExModel *mKartPartsA[21][6];         // 0x0620
    ExModel *mKartPartsB[21][6];         // 0x0818
    ExModel *mKartPartsC[21][6];         // 0x0a10
    struct
    {
        u8 mActive;                       // 0x00
        u8 _1[3];                         // 0x01
        ECharID mCharID;                  // 0x04
        JGeometry::TVec3f mPos;           // 0x08
        JGeometry::TVec3f mTarget;        // 0x14
        f32 mScale;                       // 0x20
        int mState;                       // 0x24
        int mPrevState;                   // 0x28
        f32 mTimer[3];                    // 0x2c
    } mCharacterStatus[4][2]; // Size: 0x38

    struct
    {
        u8 mActive;                       // 0x00
        u8 _1[3];                         // 0x01
        EKartID mKartID;                  // 0x04
        JGeometry::TVec3f mPos;           // 0x08
        JGeometry::TVec3f mTarget;        // 0x14
        f32 mScale;                       // 0x20
    } mKartStatus[4]; // Size: 0x24

    const f32 *mDefaultTable[84];           // 0x0e58
    f32 mKartHeight;                        // 0x0fa8
    MenuSceneLight *mSceneLight;            // 0x0fac
    MenuSubLight *mSubLight[2];             // 0x0fb0
    Mtx mCameraMatrix;                      // 0x0fb8
    Mtx mEffectMatrix;                      // 0x0fe8
}; // Size: 0x1018

#endif // CHARACTERSELECT3D_H
