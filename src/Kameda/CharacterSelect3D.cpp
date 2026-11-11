#include "Kameda/CharacterSelect3D.h"
#include "dolphin/mtx.h"

#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DModelLoader.h"
#include "JSystem/J3D/J3DTransform.h"
#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DTypes.h"
#include "JSystem/J3D/J3DPE.h"
#include "Kameda/SceneFactory.h"
#include "Kaneshige/RaceLight.h"
#include "Kaneshige/SysDebug.h"
#include "Kawano/accessory.h"
#include "Kawano/osage.h"
#include "Osako/ResMgr.h"
#include "JSystem/JFramework/JFWDisplay.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/LightMgr.h"
#include "Kaneshige/SimpleDrawer.h"
#include "mathHelper.h"

const f32 CharacterSelect3D::mCharacterDefScale[20] = {
    1.0f, 1.0f, 1.15f, 1.15f, 
    1.0f, 1.0f, 1.0f, 1.0f, 
    0.95f, 1.0f, 1.05f, 1.05f, 
    1.45f, 1.05f, 1.18f, 1.3f, 
    1.0f, 1.0f, 1.0f, 0.8f,
};

const f32 CharacterSelect3D::mKartDefScale[21] = {
    1.0f, 0.92f, 1.0f, 1.05f, 
    1.0f, 1.05f, 0.85f, 0.92f, 
    1.0f, 1.05f, 1.0f, 1.05f, 
    1.0f, 1.05f, 1.0f, 1.05f, 
    1.05f, 1.05f, 0.92f, 0.92f, 
    0.92f, 
};

const f32 CharacterSelect3D::mMarioBody[9] = {
    0.f, 57.3f, -6.f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mDkBody[9] = {
    0.f, 59.1f, -17.2f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mYoshiBody[9] = {
    0.f, 56.6f, -22.f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mNokoBody[9] = {
    0.f, 45.f, -9.5f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f,
};

const f32 CharacterSelect3D::mPeachBody[9] = {
    0.f, 63.f, -14.2f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mBabyBody[9] = {
    0.f, 53.3f, -2.f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mWarioBody[9] = {
    0.f, 60.2f, -39.5f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mKoopaBody[9] = {
    0.f, 66.3f, -6.f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mDiddyBody[9] = {
    0.f, 54.f, -35.f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mKoopaJrBody[9] = {
    0.f, 53.8f, -20.f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mKinopioBody[9] = {
    0.f, 30.f, 4.f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mCatherineBody[9] = {
    0.f, 56.6f, -22.f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mWaluigiBody[9] = {
    0.f, 55.7f, -9.8f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mKingTeresaBody[9] = {
    0.f, 71.3f, -2.f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mBonusBody[9] = {
    0.f, 64.7f, 25.f, 0.f, 
    0.f, 0.f, 1.f, 1.f, 
    1.f, 
};

const f32 CharacterSelect3D::mMarioArm[4][9] = {
    0.f, 55.3f, 49, 0.f, 
    0.f, 22.f, 1.f, 1.f, 
    1.f, 0.f, 55.3f, 49.f, 
    0.f, 180.f, -22.f, 1.f, 
    1.f, 1.f, 0.f, 55.3f, 
    -61.f, 0.f, 0.f, 19.f, 
    1.2f, 1.f, 1.f, 0.f, 
    55.3f, -61.f, 0.f, 180.f, 
    -19.f, 1.2f, 1.f, 1.f, 
};

const f32 CharacterSelect3D::mDkArm[4][9] = {
    0.f, 48.1f, 52.8, 0.f, 
    0.f, 7.f, 1.f, 1.f, 
    1.f, 0.f, 48.1f, 52.8f, 
    0.f, 180.f, -7.f, 1.f, 
    1.f, 1.f, 0.f, 48.1f, 
    -87.2f, 0.f, 0.f, 7.f, 
    1.f, 1.f, 1.f, 0.f, 
    48.1f, -87.2f, 0.f, 180.f, 
    -7.f, 1.f, 1.f, 1.f, 
};

const f32 CharacterSelect3D::mYoshiArm[4][9] = {
    0, 51.6, 58, 0, 
    0, 18, 1, 1, 
    1, 0, 51.6, 58, 
    0, 180, -18, 1, 
    1, 1, 0, 51.6, 
    -77, 0, 0, 16, 
    1.1, 1, 1, 0, 
    51.6, -77, 0, 180, 
    -16, 1.1, 1, 1, 
};

const f32 CharacterSelect3D::mNokoArm[4][9] = {
    0, 30.8, 34.5, 0, 
    0, 12, 1, 1, 
    1, 0, 30.8, 34.5, 
    0, 180, -12, 1, 
    1, 1, 0, 31.8, 
    -53.5, 0, 0, 12, 
    1, 1, 1, 0, 
    31.8, -53.5, 0, 180, 
    -12, 1, 1, 1,
};

const f32 CharacterSelect3D::mPeachArm[4][9]  = {
    0, 46.5, 60.8, 0, 
    0, 16, 1, 1, 
    1, 0, 46.5, 60.8, 
    0, 180, -16, 1, 
    1, 1, 0, 46.5, 
    -69.2, 0, 0, 13, 
    1, 1, 1, 0, 
    46.5, -69.2, 0, 180, 
    -13, 1, 1, 1, 
};

const f32 CharacterSelect3D::mBabyArm[4][9] = {
    0, 28.3, 32, 0, 
    0, 10, 1, 1, 
    1, 0, 28.3, 32, 
    0, 180, -10, 1, 
    1, 1, 0, 28.3, 
    -36, 0, 0, 7, 
    1, 1, 1, 0, 
    28.3, -36, 0, 180, 
    -7, 1, 1, 1, 
};

const f32 CharacterSelect3D::mWarioArm[4][9] = {
    -30, 54.2, 80.5, 0, 
    0, 0, 1, 1, 
    1, 30, 54.2, 80.5, 
    0, 180, 0, 1, 
    1, 1, -30, 54.2, 
    -109.5, 0, 0, 0, 
    1, 1, 1, 30, 
    54.2, -109.5, 0, 180, 
    0, 1, 1, 1,
};

const f32 CharacterSelect3D::mKoopaArm[6][9] = {
    0, 55.3, 87.5, 0, 
    0, 20, 1, 1, 
    1, 0, 55.3, 87.5, 
    0, 180, -20, 1, 
    1, 1, 0, 55.3, 
    14, 0, 0, 20, 
    1, 1, 1, 0, 
    55.3, 14, 0, 180, 
    -20, 1, 1, 1, 
    0, 55.3, -76, 0, 
    0, 10, 1, 1, 
    1, 0, 55.3, -76, 
    0, 180, -10, 1, 
    1, 1,
};

const f32 CharacterSelect3D::mDiddyArm[6][9] = {
    -14, 30, 55, 0, 
    0, 20, 1, 1, 
    1, 14, 30, 55, 
    0, 180, -20, 1, 
    1, 1, -14, 30, 
    -23, 0, 0, 20, 
    1, 1, 1, 14, 
    30, -23, 0, 180, 
    -20, 1, 1, 1, 
    -14, 30, -82, 0, 
    0, 5, 1, 1, 
    1, 14, 30, -82, 
    0, 180, -5, 1, 
    1, 1,
};

const f32 CharacterSelect3D::mKoopaJrArm[4][9] = {
    0, 31.8, 36, 0, 
    0, 13, 1, 1, 
    1, 0, 31.8, 36, 
    0, 180, -13, 1, 
    1, 1, 0, 31.8, 
    -53, 0, 0, 11, 
    1.1, 1, 1, 0, 
    31.8, -53, 0, 180, 
    -11, 1.1, 1, 1,
};

const f32 CharacterSelect3D::mKinopioArm[4][9] = {
    -20, 17, 56, 0, 
    0, 0, 1, 1, 
    1, 20, 17, 56, 
    0, 180, 0, 1, 
    1, 1, -27, 27, 
    -48, 0, 0, 0, 
    1, 1, 1, 27, 
    27, -48, 0, 180, 
    0, 1, 1, 1,
};

const f32 CharacterSelect3D::mCatherineArm[4][9] = {
    0, 51.6, 58, 0, 
    0, 18, 1, 1, 
    1, 0, 51.6, 58, 
    0, 180, -18, 1, 
    1, 1, 0, 51.6, 
    -77, 0, 0, 16, 
    1.1, 1, 1, 0, 
    51.6, -77, 0, 180, 
    -16, 1.1, 1, 1,
};

const f32 CharacterSelect3D::mWaluigiArm[4][9]  = {
    0, 44.2, 80.2, 0, 
    0, 20, 0.8, 1, 
    1, 0, 44.2, 80.2, 
    0, 180, -20, 0.8, 
    1, 1, 0, 50.7, 
    -64.8, 0, 0, 16, 
    1, 1, 1, 0, 
    50.7, -64.8, 0, 180, 
    -16, 1, 1, 1, 
};

const f32 CharacterSelect3D::mKingTeresaArm[4][9] = {
    0, 41.3, 68, 0, 
    0, 18, 1, 1, 
    1, 0, 41.3, 68, 
    0, 180, -18, 1, 
    1, 1, 0, 41.3, 
    -77.5, 0, 0, 18, 
    0.8, 1, 1, 0, 
    41.3, -77.5, 0, 180, 
    -18, 0.8, 1, 1,
};

const f32 CharacterSelect3D::mBonusArm[4][9] = {
    -5, 52.2, 130, 0, 
    0, 0, 1, 1, 
    1, 5, 52.2, 130, 
    0, 180, 0, 1, 
    1, 1, -5, 54.7, 
    -70, 0, 0, 0, 
    1, 1, 1, 5, 
    54.7, -70, 0, 180, 
    0, 1, 1, 1, 
};

const f32 CharacterSelect3D::mMarioDump[4][9] = {
    -28.6f, 67.3f, 49.f, 0.f, 
    0.f, -20.f, 1.f, 1.2f, 
    1.f, 28.6f, 67.3f, 49.f, 
    0.f, 0.f, 20.f, 1.f, 
    1.2f, 1.f, -28.6f, 67.3f, 
    -61.f, 0.f, 0.f, -20.f, 
    1.f, 1.1, 1, 28.6f, 
    67.3f, -61.f, 0.f, 0.f, 
    20.f, 1.f, 1.1f, 1.f, 
};

const f32 CharacterSelect3D::mDkDump[4][9] = {
    -45, 70.1, 52.8, 0, 
    0, -20, 1, 1.3, 
    1, 45, 70.1, 52.8, 
    0, 0, 20, 1, 
    1.3, 1, -45, 70.1, 
    -87.2, 0, 0, -20, 
    1, 1.3, 1, 45, 
    70.1, -87.2, 0, 0, 
    20, 1, 1.3, 1,  
};

const f32 CharacterSelect3D::mYoshiDump[4][9] = {
    -28.6, 66.6, 58, 0, 
    0, -20, 1, 1.25, 
    1, 28.6, 66.6, 58, 
    0, 0, 20, 1, 
    1.25, 1, -28.6, 66.6, 
    -77, 0, 0, -20, 
    1, 1.15, 1, 28.6, 
    66.6, -77, 0, 0, 
    20, 1, 1.15, 1,
};

const f32 CharacterSelect3D::mNokoDump[4][9] = {
    -15, 38.8, 34.5, 0, 
    0, -50, 1, 1.2, 
    1, 15, 38.8, 34.5, 
    0, 0, 50, 1, 
    1.2, 1, -15, 39.8, 
    -53.5, 0, 0, -50, 
    1, 1.2, 1, 15, 
    39.8, -53.5, 0, 0, 
    50, 1, 1.2, 1,  
};

const f32 CharacterSelect3D::mPeachDump[4][9] = {
    -27, 69.5, 60.8, 0, 
    0, -20, 1, 1.5, 
    1, 27, 69.5, 60.8, 
    0, 0, 20, 1, 
    1.5, 1, -27, 69.5, 
    -69.2, 0, 0, -20, 
    1, 1.4, 1, 27, 
    69.5, -69.2, 0, 0, 
    20, 1, 1.4, 1,
};

const f32 CharacterSelect3D::mBabyDump[4][9] = {
    -24, 58.3, 32, 0, 
    0, -10, 1, 1.2, 
    1, 24, 58.3, 32, 
    0, 0, 10, 1, 
    1.2, 1, -28, 58.3, 
    -36, 0, 0, -10, 
    1, 1.15, 1, 28, 
    58.3, -36, 0, 0, 
    10, 1, 1.15, 1, 
};

const f32 CharacterSelect3D::mWarioDump[4][9] = {
    -42, 90.2, 80.5, 0, 
    0, -20, 1, 1, 
    1, 42, 90.2, 80.5, 
    0, 0, 20, 1, 
    1, 1, -42, 90.2, 
    -109.5, 0, 0, -20, 
    1, 1, 1, 42, 
    90.2, -109.5, 0, 0, 
    20, 1, 1, 1, 
};

const f32 CharacterSelect3D::mKoopaDump[6][9] = {
     -45, 77.3, 87.5, 0, 
    0, -5, 1, 1.65, 
    1, 45, 77.3, 87.5, 
    0, 0, 5, 1, 
    1.65, 1, -45, 77.3, 
    14, 0, 0, -5, 
    1, 1.65, 1, 45, 
    77.3, 14, 0, 0, 
    5, 1, 1.65, 1, 
    -38, 77.3, -76, 0, 
    0, -15, 1, 1.25, 
    1, 38, 77.3, -76, 
    0, 0, 15, 1, 
    1.25, 1, 
};

const f32 CharacterSelect3D::mKoopaJrDump[4][9] = {
    -30, 43.8, 36, 0, 
    0, -20, 1, 1.8, 
    1, 30, 43.8, 36, 
    0, 0, 20, 1, 
    1.8, 1, -30, 43.8, 
    -53, 0, 0, -20, 
    1, 1.5, 1, 30, 
    43.8, -53, 0, 0, 
    20, 1, 1.5, 1, 
};

const f32 CharacterSelect3D::mCatherineDump[4][9] = {
    -28.6, 66.6, 58, 0, 
    0, -20, 1, 1.25, 
    1, 28.6, 66.6, 58, 
    0, 0, 20, 1, 
    1.25, 1, -28.6, 66.6, 
    -77, 0, 0, -20, 
    1, 1.15, 1, 28.6, 
    66.6, -77, 0, 0, 
    20, 1, 1.15, 1,
};

const f32 CharacterSelect3D::mWaluigiDump[4][9] = {
    -23, 62.2, 80.2, 0, 
    0, -20, 1, 1.6, 
    1, 23, 62.2, 80.2, 
    0, 0, 20, 1, 
    1.6, 1, -37, 62.2, 
    -64.8, 0, 0, -20, 
    1, 1.4, 1, 37, 
    62.2, -64.8, 0, 0, 
    20, 1, 1.4, 1,
};

const f32 CharacterSelect3D::mKingTeresaDump[4][9] = {
    -48, 63.3, 68, 0, 
    0, -14, 1, 1.7, 
    1, 48, 63.3, 68, 
    0, 0, 14, 1, 
    1.7, 1, -39, 63.3, 
    -77.5, 0, 0, -14, 
    1, 1.6, 1, 39, 
    63.3, -77.5, 0, 0, 
    14, 1, 1.6, 1, 
};

const f32 CharacterSelect3D::mBonusDump[4][9] = {
     -55, 80.7, 130, 0, 
    0, -12, 1, 1, 
    1, 55, 80.7, 130, 
    0, 0, 12, 1, 
    1, 1, -55, 84.7, 
    -70, 0, 0, -12, 
    1, 1, 1, 55, 
    84.7, -70, 0, 0, 
    12, 1, 1, 1,
};

const f32 CharacterSelect3D::mMarioTire[4][9] = {
    -57.7f, 33.f, 49.f, 0.f, 
    0.f, -6.8f, 1.f, 1.f, 
    1.f, 57.7f, 33, 49.f, 
    0.f, 0.f, 6.8f, 1.f, 
    1.f, 1.f, -61.7f, 35.f, 
    -61.f, 0.f, 0.f, -7.6f, 
    1.06f, 1.06f, 1.06f, 61.7f, 
    35.f, -61.f, 0.f, 0.f, 
    7.6f, 1.06f, 1.06f, 1.06f, 
};

const f32 CharacterSelect3D::mDkTire[4][9] = {
    -75.1, 40.5, 52.8, 0, 
    0, -4.4, 1, 1, 
    1, 75.1, 40.5, 52.8, 
    0, 0, 4.4, 1, 
    1, 1, -71.900002, 40.5, 
    -87.2, 0, 0, -4.2, 
    1, 1, 1, 71.900002, 
    40.5, -87.2, 0, 0, 
    4.2, 1, 1, 1,
};

const f32 CharacterSelect3D::mYoshiTire[4][9] = {
    -59.5, 33, 58, 0, 
    0, -6.6, 1, 1, 
    1, 59.5, 33, 58, 
    0, 0, 6.6, 1, 
    1, 1, -59.9, 35, 
    -77, 0, 0, -7.2, 
    1.06, 1.06, 1.06, 59.9, 
    35, -77, 0, 0, 
    7.2, 1.06, 1.06, 1.06, 
};

const f32 CharacterSelect3D::mNokoTire[4][9] = {
    -52.3, 24, 34.5, 0, 
    0, -8.3, 1, 1, 
    1, 52.3, 24, 34.5, 
    0, 0, 8.3, 1, 
    1, 1, -50, 24, 
    -53.5, 0, 0, -8.3, 
    1, 1, 1, 50, 
    24, -53.5, 0, 0, 
    8.3, 1, 1, 1,
};

const f32 CharacterSelect3D::mPeachTire[4][9] = {
    -57.3, 32.5, 60.8, 0, 
    0, -6.7, 1, 1, 
    1, 57.3, 32.5, 60.8, 
    0, 0, 6.7, 1, 
    1, 1, -54.8, 35.5, 
    -69.2, 0, 0, -7, 
    1, 1.1, 1.1, 54.8, 
    35.5, -69.2, 0, 0, 
    7, 1, 1.1, 1.1, 
};

const f32 CharacterSelect3D::mBabyTire[4][9] = {
    -47, 20.5, 32, 0, 
    0, -7.6, 1, 1, 
    1, 47, 20.5, 32, 
    0, 0, 7.6, 1, 
    1, 1, -48, 22.5, 
    -36, 0, 0, -8.3999996, 
    1.1, 1.1, 1.1, 48, 
    22.5, -36, 0, 0, 
    8.3999996, 1.1, 1.1, 1.1 
};

const f32 CharacterSelect3D::mWarioTire[4][9] = {
    -78.2, 53.5, 80.5, 0, 
    0, -8.7, 1, 1, 
    1, 78.2, 53.5, 80.5, 
    0, 0, 8.7, 1, 
    1, 1, -74.8, 53.5, 
    -109.5, 0, 0, -9.2, 
    1, 1, 1, 74.8, 
    53.5, -109.5, 0, 0, 
    9.2, 1, 1, 1,
};

const f32 CharacterSelect3D::mKoopaTire[6][9] = {
    -75.1, 35.5, 87.5, 0, 
    0, -3.9, 1, 1, 
    1, 75.1, 35.5, 87.5, 
    0, 0, 3.9, 1, 
    1, 1, -75.1, 35.5, 
    14, 0, 0, -3.9, 
    1, 1, 1, 75.1, 
    35.5, 14, 0, 0, 
    3.9, 1, 1, 1, 
    -74.8, 47, -76, 0, 
    0, -5.8, 1.2, 1.33, 
    1.33, 74.8, 47, -76, 
    0, 0, 5.8, 1.2, 
    1.33, 1.33,
};

const f32 CharacterSelect3D::mDiddyTire[6][9] = {
    -40, 21, 55, 0, 
    0, -2.5, 1, 1, 
    1, 40, 21, 55, 
    0, 0, 2.5, 1, 
    1, 1, -41.7, 26.8, 
    -23, 0, 0, -1.6, 
    1.2, 1.27, 1.27, 41.7, 
    26.8, -23, 0, 0, 
    1.6, 1.2, 1.27, 1.27, 
    -41.7, 26.8, -82, 0, 
    0, -1.6, 1.2, 1.27, 
    1.27, 41.7, 26.8, -82, 
    0, 0, 1.6, 1.2, 
    1.27, 1.27,
};

const f32 CharacterSelect3D::mKoopaJrTire[4][9] = {
    -50.2, 20.7, 36, 0, 
    0, -5.9, 1, 1, 
    1, 50.2, 20.7, 36, 
    0, 0, 5.9, 1, 
    1, 1, -51, 22.7, 
    -53, 0, 0, -6.9, 
    1.1, 1.1, 1.1, 51, 
    22.7, -53, 0, 0, 
    6.9, 1.1, 1.1, 1.1,
};

const f32 CharacterSelect3D::mKinopioTire[4][9] = {
   -47, 17, 56, 0, 
    0, -2.3, 1, 1, 
    1, 47, 17, 56, 
    0, 0, 2.3, 1, 
    1, 1, -55, 27, 
    -48, 0, 0, -2.3, 
    1.55, 1.55, 1.55, 55, 
    27, -48, 0, 0, 
    2.3, 1.55, 1.55, 1.55,
};

const f32 CharacterSelect3D::mCatherineTire[4][9] = {
    -59.5, 33, 58, 0, 
    0, -6.6, 1, 1, 
    1, 59.5, 33, 58, 
    0, 0, 6.6, 1, 
    1, 1, -59.9, 35, 
    -77, 0, 0, -7.2, 
    1.06, 1.06, 1.06, 59.9, 
    35, -77, 0, 0, 
    7.2, 1.06, 1.06, 1.06,
};

const f32 CharacterSelect3D::mWaluigiTire[4][9] = {
    -52, 26.2, 80.2, 0, 
    0, -2.5, 1, 1, 
    1, 52, 26.2, 80.2, 
    0, 0, 2.5, 1, 
    1, 1, -70, 32.7, 
    -64.8, 0, 0, -2.4, 
    1.6, 1.25, 1.25, 70, 
    32.7, -64.8, 0, 0, 
    2.4, 1.6, 1.25, 1.25,
};

const f32 CharacterSelect3D::mKingTeresaTire[4][9] = {
    -73.5, 20.8, 68, 0, 
    0, -2.5, 1, 1, 
    1, 73.5, 20.8, 68, 
    0, 0, 2.5, 1, 
    1, 1, -70, 24.8, 
    -77.5, 0, 0, -2.6, 
    1.4, 1.2, 1.2, 70, 
    24.8, -77.5, 0, 0, 
    2.6, 1.4, 1.2, 1.2,
};

const f32 CharacterSelect3D::mBonusTire[4][9] = {
    -80.1, 50.7, 130, 0, 
    0, -2.8, 1, 1, 
    1, 80.1, 50.7, 130, 
    0, 0, 2.8, 1, 
    1, 1, -86.6, 52.7, 
    -70, 0, 0, -1.9, 
    1.1, 1.1, 1.1, 86.6, 
    52.7, -70, 0, 0, 
    1.9, 1.1, 1.1, 1.1, 
};



void *CharacterSelect3D::mDiddyDump;
void *CharacterSelect3D::mKinopioDump;
CharacterSelect3D *CharacterSelect3D::mCharacterSelect3D;


CharacterSelect3D::CharacterSelect3D(JKRHeap *heap)
{
    CharacterSelect3DParam *param = new (heap, 0) CharacterSelect3DParam();
    mNode = new (heap, 0) HioNode("キャラクターセレクト３Ｄ表示調整", param, 0, 0);
    mSceneID = (int)SceneFactory::ptr()->getArchive(13);

    for (int i = 0; i < 4; ++i)
    {
        mDrawBufferA[i] = new (heap, 0) J3DDrawBuffer(8);
        j3dSys.setDrawBuffer(mDrawBufferA[i], 0);
        mDrawBufferB[i] = new (heap, 0) J3DDrawBuffer(8);
        j3dSys.setDrawBuffer(mDrawBufferB[i], 1);
    }

    SysDebug::getManager()->setHeapGroup("3DModel", heap);

    for (int i = 0; i < 21; ++i)
    {
        if (i != 20)
        {
            mCharacterModel[i].mModelData = J3DModelLoaderDataBase::load(
                ResMgr::getPtr(ResMgr::mcDriverDataBmd, (ResMgr::DriverId)i), 0x11020030);
        }
        else
        {
            mCharacterModel[i].mModelData = J3DModelLoaderDataBase::load(
                ResMgr::getPtr(ResMgr::mcArcMRAM, OsageMgr::mscaOsageBmdFileName), 0x11020030);
        }

        ExModel::disableTexLOD(mCharacterModel[i].mModelData);
        for (u16 j = 0; j < mCharacterModel[i].mModelData->getShapeNum(); ++j)
        {
            J3DShape *shape = mCharacterModel[i].mModelData->getShapeNodePointer(j);
            shape->mFlags = (shape->mFlags & ~0xF000) | 0x2000;
        }
        mCharacterModel[i].mModelData->newSharedDisplayList(0x40000);

        mCharacterModel[i].mModel = new (heap, 0) ExModel(1);
        mCharacterModel[i].mModel->setModelData(mCharacterModel[i].mModelData);
    }

    JKRSolidHeap *solidHeap = JKRSolidHeap::create(heap->getFreeSize(), heap, false);
    JKRHeap *currentHeap = solidHeap->becomeCurrentHeap();

    Mtx mtx;
    PSMTXIdentity(mtx);
    j3dSys.setViewMtx(mtx);

    for (int i = 0; i < 21; ++i)
    {
        mCharacterModel[i].mModel->createDifferedModel(solidHeap, 1, 0x1080202, true);
        mCharacterModel[i].mModel->makeSharedDL();
        mCharacterModel[i].mModel->lock();
    }

    solidHeap->adjustSize();
    currentHeap->becomeCurrentHeap();

    const char *anmFile[3][21] = {
        {
            "BabyMario/enter.bck", "BabyLuigi/enter.bck", "Patapata/enter.bck",
            "Nokonoko/enter.bck", "Peach/enter.bck", "Daisy/enter.bck",
            "Mario/enter.bck", "Luigi/enter.bck", "Wario/enter.bck",
            "Waluigi/enter.bck", "Yoshi/enter.bck", "Catherine/enter.bck",
            "DK/enter.bck", "DKjr/enter.bck", "Koopa/enter.bck",
            "KoopaJr/enter.bck", "Kinopio/enter.bck", "Kinopico/enter.bck",
            "KingTeresa/enter.bck", "BossPakkun/enter.bck", "Kinopico/hair_enter.bck",
        },
        {
            "BabyMario/wait.bck", "BabyLuigi/wait.bck", "Patapata/wait.bck",
            "Nokonoko/wait.bck", "Peach/wait.bck", "Daisy/wait.bck",
            "Mario/wait.bck", "Luigi/wait.bck", "Wario/wait.bck",
            "Waluigi/wait.bck", "Yoshi/wait.bck", "Catherine/wait.bck",
            "DK/wait.bck", "DKjr/wait.bck", "Koopa/wait.bck",
            "KoopaJr/wait.bck", "Kinopio/wait.bck", "Kinopico/wait.bck",
            "KingTeresa/wait.bck", "BossPakkun/wait.bck", "Kinopico/hair_wait.bck",
        },
        {
            "BabyMario/gut.bck", "BabyLuigi/gut.bck", "Patapata/gut.bck",
            "Nokonoko/gut.bck", "Peach/gut.bck", "Daisy/gut.bck",
            "Mario/gut.bck", "Luigi/gut.bck", "Wario/gut.bck",
            "Waluigi/gut.bck", "Yoshi/gut.bck", "Catherine/gut.bck",
            "DK/gut.bck", "DKjr/gut.bck", "Koopa/gut.bck",
            "KoopaJr/gut.bck", "Kinopio/gut.bck", "Kinopico/gut.bck",
            "KingTeresa/gut.bck", "BossPakkun/gut.bck", "Kinopico/hair_gut.bck",
        },
    };

    for (int i = 0; i < 21; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            J3DAnmTransform *anm = (J3DAnmTransform *)J3DAnmLoaderDataBase::load(
                ((JKRArchive *)mSceneID)->getResource(anmFile[j][i]), J3DLOADER_UNK_FLAG0);
            mCharacterModel[i].mAnm[j].mAnmTransform = anm;
            mCharacterModel[i].mAnm[j].mMtxCalc = J3DNewMtxCalcAnm(
                mCharacterModel[i].mModelData->getMtxCalcType(),
                mCharacterModel[i].mAnm[j].mAnmTransform);
        }

        mCharacterModel[i].mMtxCalc01 = J3DUNewMtxCalcAnm(
            mCharacterModel[i].mModelData->getMtxCalcType(),
            mCharacterModel[i].mAnm[0].mAnmTransform, mCharacterModel[i].mAnm[1].mAnmTransform,
            nullptr, nullptr, MTXCalc_Blend);
        mCharacterModel[i].mMtxCalc02 = J3DUNewMtxCalcAnm(
            mCharacterModel[i].mModelData->getMtxCalcType(),
            mCharacterModel[i].mAnm[1].mAnmTransform, mCharacterModel[i].mAnm[2].mAnmTransform,
            nullptr, nullptr, MTXCalc_Blend);
        mCharacterModel[i].mMtxCalc12 = J3DUNewMtxCalcAnm(
            mCharacterModel[i].mModelData->getMtxCalcType(),
            mCharacterModel[i].mAnm[0].mAnmTransform, mCharacterModel[i].mAnm[2].mAnmTransform,
            nullptr, nullptr, MTXCalc_Blend);
    }

        const char *texFile[21] = {
        "BabyMario/gut.btp", "BabyLuigi/gut.btp", "Patapata/gut.btp",
        "Nokonoko/gut.btp", "Peach/gut.btp", "Daisy/gut.btp",
        "Mario/gut.btp", "Luigi/gut.btp", "Wario/gut.btp",
        "Waluigi/gut.btp", "Yoshi/gut.btp", "Catherine/gut.btp",
        "DK/gut.btp", "DKjr/gut.btp", "Koopa/gut.btp",
        "KoopaJr/gut.btp", "Kinopio/gut.btp", "Kinopico/gut.btp",
        "KingTeresa/gut.btp",
        "Mario/gut.btp", "Mario/gut.btp",
    };

    for (int i = 0; i < 21; ++i)
    {
        mCharacterModel[i].mAnmTexPattern = (J3DAnmTexPattern *)J3DAnmLoaderDataBase::load(
            ((JKRArchive *)mSceneID)->getResource(texFile[i]), J3DLOADER_UNK_FLAG0);
        mCharacterModel[i].mAnmTexPattern->searchUpdateMaterialID(mCharacterModel[i].mModelData);

        for (u16 j = 0; j < mCharacterModel[i].mModelData->getMaterialNum(); ++j)
        {
            J3DMaterialAnm *anm = new (heap, 0) J3DMaterialAnm();
            mCharacterModel[i].mModelData->getMaterialNodePointer(j)->change();
            mCharacterModel[i].mModelData->getMaterialNodePointer(j)->setMaterialAnm(anm);
        }
        mCharacterModel[i].mModelData->getMaterialTable().entryTexNoAnimator(mCharacterModel[i].mAnmTexPattern);
    }

        const ResMgr::KartDataId kartDataId[5] = {
        ResMgr::mcKartDataBody, ResMgr::mcKartDataArm, ResMgr::mcKartDataShock,
        ResMgr::mcKartTireRight, ResMgr::mcKartTireLeft,
    };


    for (int kartID = 0; kartID < 21; ++kartID)
    {
        for (int j = 0; j < 5; ++j)
        {
            if ((kartID == 9 || kartID == 16 || kartID == 17) && j == 2)
            {
                mKartModelData[kartID][j] = nullptr;
            }
            else
            {
                mKartModelData[kartID][j] = J3DModelLoaderDataBase::load(
                    ResMgr::getPtr(kartDataId[j], (EKartID)kartID), 0x18020010);
                ExModel::disableTexLOD(mKartModelData[kartID][j]);
                ExModel::setLightMask(mKartModelData[kartID][j], GX_LIGHT1);
                mKartModelData[kartID][j]->newSharedDisplayList(0x40000);
                mKartModelData[kartID][j]->simpleCalcMaterial(0, *(Mtx *)&j3dDefaultMtx);
                mKartModelData[kartID][j]->makeSharedDL();
            }
        }
    }

    for (int kartID = 0; kartID < 21; ++kartID)
    {
        mKartModel[kartID] = nullptr;
        for (int j = 0; j < 6; ++j)
        {
            mKartPartsA[kartID][j] = nullptr;
            mKartPartsB[kartID][j] = nullptr;
            mKartPartsC[kartID][j] = nullptr;
        }
    }

    JKRSolidHeap *kartHeap = JKRSolidHeap::create(heap->getFreeSize(), heap, false);
    JKRHeap *kartCurrentHeap = kartHeap->becomeCurrentHeap();

    for (int kartID = 0; kartID < 21; ++kartID)
    {
        mKartModel[kartID] = new (kartHeap, 0) ExModel(1);
        mKartModel[kartID]->setModelData(mKartModelData[kartID][0]);

        int count;
        if (kartID == 7 || kartID == 9)
            count = 6;
        else
            count = 4;
        for (int i = 0; i < count; ++i)
        {
            mKartPartsA[kartID][i] = new (kartHeap, 0) ExModel(1);
            mKartPartsA[kartID][i]->setModelData(mKartModelData[kartID][1]);

            if (mKartModelData[kartID][2] != nullptr)
            {
                mKartPartsB[kartID][i] = new (kartHeap, 0) ExModel(1);
                mKartPartsB[kartID][i]->setModelData(mKartModelData[kartID][2]);
            }

            mKartPartsC[kartID][i] = new (kartHeap, 0) ExModel(1);
            if (i % 2 == 0)
                mKartPartsC[kartID][i]->setModelData(mKartModelData[kartID][3]);
            else
                mKartPartsC[kartID][i]->setModelData(mKartModelData[kartID][4]);
        }
    }

    kartHeap->adjustSize();
    kartCurrentHeap->becomeCurrentHeap();

    mDefaultTable[0] = mMarioBody;
    mDefaultTable[4] = mDkBody;
    mDefaultTable[8] = mYoshiBody;
    mDefaultTable[12] = mNokoBody;
    mDefaultTable[16] = mPeachBody;
    mDefaultTable[20] = mBabyBody;
    mDefaultTable[24] = mWarioBody;
    mDefaultTable[28] = mKoopaBody;
    mDefaultTable[32] = mMarioBody;
    mDefaultTable[36] = mDiddyBody;
    mDefaultTable[40] = mCatherineBody;
    mDefaultTable[44] = mNokoBody;
    mDefaultTable[48] = mPeachBody;
    mDefaultTable[52] = mBabyBody;
    mDefaultTable[56] = mWaluigiBody;
    mDefaultTable[60] = mKoopaJrBody;
    mDefaultTable[64] = mKinopioBody;
    mDefaultTable[68] = mKinopioBody;
    mDefaultTable[72] = mKingTeresaBody;
    mDefaultTable[76] = mKingTeresaBody;
    mDefaultTable[80] = mBonusBody;
    mDefaultTable[1] = mMarioArm[0];
    mDefaultTable[5] = mDkArm[0];
    mDefaultTable[9] = mYoshiArm[0];
    mDefaultTable[13] = mNokoArm[0];
    mDefaultTable[17] = mPeachArm[0];
    mDefaultTable[21] = mBabyArm[0];
    mDefaultTable[25] = mWarioArm[0];
    mDefaultTable[29] = mKoopaArm[0];
    mDefaultTable[33] = mMarioArm[0];
    mDefaultTable[37] = mDiddyArm[0];
    mDefaultTable[41] = mCatherineArm[0];
    mDefaultTable[45] = mNokoArm[0];
    mDefaultTable[49] = mPeachArm[0];
    mDefaultTable[53] = mBabyArm[0];
    mDefaultTable[57] = mWaluigiArm[0];
    mDefaultTable[61] = mKoopaJrArm[0];
    mDefaultTable[65] = mKinopioArm[0];
    mDefaultTable[69] = mKinopioArm[0];
    mDefaultTable[73] = mKingTeresaArm[0];
    mDefaultTable[77] = mKingTeresaArm[0];
    mDefaultTable[81] = mBonusArm[0];
    mDefaultTable[2] = mMarioDump[0];
    mDefaultTable[6] = mDkDump[0];
    mDefaultTable[10] = mYoshiDump[0];
    mDefaultTable[14] = mNokoDump[0];
    mDefaultTable[18] = mPeachDump[0];
    mDefaultTable[22] = mBabyDump[0];
    mDefaultTable[26] = mWarioDump[0];
    mDefaultTable[30] = mKoopaDump[0];
    mDefaultTable[34] = mMarioDump[0];
    mDefaultTable[38] = (const f32 *)mDiddyDump;
    mDefaultTable[42] = mCatherineDump[0];
    mDefaultTable[46] = mNokoDump[0];
    mDefaultTable[50] = mPeachDump[0];
    mDefaultTable[54] = mBabyDump[0];
    mDefaultTable[58] = mWaluigiDump[0];
    mDefaultTable[62] = mKoopaJrDump[0];
    mDefaultTable[66] = (const f32 *)mKinopioDump;
    mDefaultTable[70] = (const f32 *)mKinopioDump;
    mDefaultTable[74] = mKingTeresaDump[0];
    mDefaultTable[78] = mKingTeresaDump[0];
    mDefaultTable[82] = mBonusDump[0];
    mDefaultTable[3] = mMarioTire[0];
    mDefaultTable[7] = mDkTire[0];
    mDefaultTable[11] = mYoshiTire[0];
    mDefaultTable[15] = mNokoTire[0];
    mDefaultTable[19] = mPeachTire[0];
    mDefaultTable[23] = mBabyTire[0];
    mDefaultTable[27] = mWarioTire[0];
    mDefaultTable[31] = mKoopaTire[0];
    mDefaultTable[35] = mMarioTire[0];
    mDefaultTable[39] = mDiddyTire[0];
    mDefaultTable[43] = mCatherineTire[0];
    mDefaultTable[47] = mNokoTire[0];
    mDefaultTable[51] = mPeachTire[0];
    mDefaultTable[55] = mBabyTire[0];
    mDefaultTable[59] = mWaluigiTire[0];
    mDefaultTable[63] = mKinopioTire[0];
    mDefaultTable[67] = mKoopaJrTire[0];
    mDefaultTable[71] = mKoopaJrTire[0];
    mDefaultTable[75] = mKingTeresaTire[0];
    mDefaultTable[79] = mKingTeresaTire[0];
    mDefaultTable[83] = mBonusTire[0];

    mAccessoryModelData = J3DModelLoaderDataBase::load(
        ResMgr::getPtr(ResMgr::mcArcMRAM, AccessoryMgr::mscaAccessoryBmdFileName), 0x18020010);

    for (u16 i = 0; i < mAccessoryModelData->getMaterialNum(); ++i)
    {
        mAccessoryModelData->getMaterialNodePointer(i)->getPEBlock()->getZMode()->setUpdateEnable(0);
    }

    ExModel::disableTexLOD(mAccessoryModelData);
    mAccessoryModelData->newSharedDisplayList(0x40000);
    mAccessoryModelData->simpleCalcMaterial(0, *(Mtx *)&j3dDefaultMtx);
    mAccessoryModelData->makeSharedDL();

    JKRSolidHeap *accessoryHeap = JKRSolidHeap::create(heap->getFreeSize(), heap, false);
    JKRHeap *accessoryCurrentHeap = accessoryHeap->becomeCurrentHeap();

    mAccessoryModel = new (accessoryHeap, 0) ExModel(1);
    mAccessoryModel->setModelData(mAccessoryModelData);
    mAccessoryModel->_18 |= 2;

    accessoryHeap->adjustSize();
    accessoryCurrentHeap->becomeCurrentHeap();

    mAccessoryAnm = (J3DAnmTextureSRTKey *)J3DAnmLoaderDataBase::load(
        ResMgr::getPtr(ResMgr::mcArcMRAM, AccessoryMgr::mscaAccessoryBtkFileName), J3DLOADER_UNK_FLAG0);
    mAccessoryAnm->searchUpdateMaterialID(mAccessoryModelData);

    for (u16 i = 0; i < mAccessoryModelData->getMaterialNum(); ++i)
    {
        J3DMaterialAnm *anm = new (heap, 0) J3DMaterialAnm();
        mAccessoryModelData->getMaterialNodePointer(i)->change();
        mAccessoryModelData->getMaterialNodePointer(i)->setMaterialAnm(anm);
    }
    mAccessoryModelData->entryTexMtxAnimator(mAccessoryAnm);

    SysDebug::getManager()->setHeapGroup("CharacterSelect3D", heap);

    PSMTXIdentity(mCameraMatrix);
    PSMTXIdentity(mEffectMatrix);

    LightMgr::createManager();
    mSceneLight = MenuSceneLight::createAndAppend(heap, mCameraMatrix);
    for (int i = 0; i < 2; ++i)
    {
        mSubLight[i] = MenuSubLight::createAndAppend(heap, i, mSceneLight, i + 1);
    }

    JUtility::TColor color(200, 185, 180, 255);
    mSceneLight->setColor(color);

    mSceneLight->mOffsetPos.set(-10300.0f, 15600.0f, 21000.0f);
    mSubLight[0]->mOffsetPos.set(-10300.0f, 15600.0f, 21000.0f);
    mSubLight[1]->mOffsetPos.set(3000.0f, 11500.0f, 18000.0f);

    JUtility::TColor ambient(100, 100, 100, 255);
    MenuAmbient::createAndAppend(heap, ambient);

    init();
}

CharacterSelect3DParam::CharacterSelect3DParam()
{
    mAdjust = 0;
    mFovy = 30.0f;
    mEye.set(0.0f, 135.0f, 1000.0f);
    mTarget.set(0.0f, 135.0f, 0.0f);
    mBodyPos[0].set(0.0f, 0.0f, 0.0f);
    mBodyTarget[0].set(0.0f, 0.0f, 0.0f);
    mBodyScale[0] = 1.0f;
    mBodyPos[1].set(0.0f, 0.0f, 0.0f);
    mBodyTarget[1].set(0.0f, 0.0f, 0.0f);
    mBodyScale[1] = 1.0f;
    mKartPos.set(0.0f, 135.0f, 0.0f);
    mKartTarget.set(0.0f, 0.0f, 0.0f);
    mKartScale = 1.0f;
}

static const Vec sUpVector = {0.0f, 1.0f, 0.0f};
static const Vec sUpVector2 = {0.0f, 1.0f, 0.0f};

void CharacterSelect3D::init()
{
    JGeometry::TVec3f zero(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            mCharacterStatus[i][j].mActive = 0;
            mCharacterStatus[i][j].mCharID = (ECharID)0;
            mCharacterStatus[i][j].mPos = zero;
            mCharacterStatus[i][j].mScale = 1.0f;
            mCharacterStatus[i][j].mState = 0;
            mCharacterStatus[i][j].mPrevState = 0;
            for (int k = 0; k < 3; ++k)
            {
                mCharacterStatus[i][j].mTimer[k] = 0.0f;
            }
        }

        mKartStatus[i].mActive = 0;
        mKartStatus[i].mKartID = (EKartID)0xff;
        mKartStatus[i].mPos = zero;
        mKartStatus[i].mScale = 1.0f;
    }

    mAccessoryScale = 0.0f;
    mKartHeight = 0.0f;
}

CharacterSelect3D::~CharacterSelect3D()
{
    if (mCharacterSelect3D != nullptr)
    {
        mCharacterSelect3D = nullptr;
    }
}

void CharacterSelect3D::draw(int player, f32 aspect)
{
    CharacterSelect3DParam *param = (CharacterSelect3DParam *)mNode->getReflexible();
    j3dSys.drawInit();

    JGeometry::TVec3f target;
    JGeometry::TVec3f up = sUpVector;
    JGeometry::TVec3f eye;
    Mtx tmp;
    Mtx projMtx;

    C_MTXPerspective(projMtx, param->mFovy, aspect, 10.0f, 200000.0f);
    GXSetProjection(projMtx, GX_PERSPECTIVE);

    eye = param->mEye;
    target = param->mTarget;

    if (player == 0)
    {
        C_MTXLookAt(mCameraMatrix, &eye, &up, &target);

        if (ExModel::isMtxCombinationOn())
        {
            PSMTXCopy(mCameraMatrix, tmp);
            for (int i = 0; i < 3; ++i)
            {
                for (int k = 0; k < 4; ++k)
                {
                    mCameraMatrix[i][k] = (tmp[i][k] - mEffectMatrix[i][k]) *
                                              JFWDisplay::getManager()->getCombinationRatio() +
                                          mEffectMatrix[i][k];
                }
            }
            PSMTXCopy(tmp, mEffectMatrix);
        }
    }

    mSceneLight->mCameraPos.set(eye.x, eye.y, eye.z);
    mSceneLight->mTargetPos.set(target.x, target.y, target.z);
    j3dSys.setViewMtx(mCameraMatrix);

    LightMgr::getManager()->setCurrentViewNo(0);
    LightMgr::getManager()->draw();

    if (mKartStatus[player].mActive)
    {
        drawKart(mKartStatus[player].mKartID);
    }

    for (int i = 0; i < 2; ++i)
    {
        if (!mCharacterStatus[player][i].mActive)
            continue;

        int index = mCharacterStatus[player][i].mCharID - 1;
        mCharacterModel[index].mModel->setCurrentViewNo(0);

        MtxPtr effectMtx = mSceneLight->getEffectMtx();
        if (i == 1)
            effectMtx = ((LightObj *)mSubLight[1])->getEffectMtx();
        mCharacterModel[index].mModel->setEffectMtx(effectMtx, 1);

        if (index + 1 == 17)
        {
            mCharacterModel[20].mModel->setCurrentViewNo(0);
            mCharacterModel[20].mModel->setEffectMtx(effectMtx, 1);
        }
    }

    j3dSys.setDrawModeOpaTexEdge();
    mDrawBufferA[player]->draw();
    j3dSys.setDrawModeXlu();
    mDrawBufferB[player]->draw();

    for (int i = 0; i < 2; ++i)
    {
        if (mCharacterStatus[player][i].mActive && mCharacterStatus[player][i].mCharID == 19)
        {
            SimpleDrawer drawer;
            drawer.drawInit(mAccessoryModel);
            while (drawer.loadPreDrawSetting())
            {
                mAccessoryModel->simpleDraw(0, nullptr, 1);
            }
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        mCharacterStatus[player][i].mActive = 0;
    }
    mKartStatus[player].mActive = 0;
}

void CharacterSelect3D::draw(int player, f32 aspect, JGeometry::TVec3f eye, JGeometry::TVec3f target)
{
    CharacterSelect3DParam *param = (CharacterSelect3DParam *)mNode->getReflexible();
    j3dSys.drawInit();

    Mtx projMtx;
    Mtx tmp;
    C_MTXPerspective(projMtx, param->mFovy, aspect, 10.0f, 200000.0f);
    GXSetProjection(projMtx, GX_PERSPECTIVE);

    Vec up = sUpVector2;

    if (player == 0)
    {
        C_MTXLookAt(mCameraMatrix, &eye, &up, &target);

        if (ExModel::isMtxCombinationOn())
        {
            PSMTXCopy(mCameraMatrix, tmp);
            for (int i = 0; i < 3; ++i)
            {
                for (int k = 0; k < 4; ++k)
                {
                    mCameraMatrix[i][k] = (tmp[i][k] - mEffectMatrix[i][k]) *
                                              JFWDisplay::getManager()->getCombinationRatio() +
                                          mEffectMatrix[i][k];
                }
            }
            PSMTXCopy(tmp, mEffectMatrix);
        }
    }

    mSceneLight->mCameraPos.set(eye.x, eye.y, eye.z);
    mSceneLight->mTargetPos.set(target.x, target.y, target.z);
    j3dSys.setViewMtx(mCameraMatrix);

    LightMgr::getManager()->setCurrentViewNo(0);
    LightMgr::getManager()->draw();

    if (mKartStatus[player].mActive)
    {
        drawKart(mKartStatus[player].mKartID);
    }

    for (int i = 0; i < 2; ++i)
    {
        if (!mCharacterStatus[player][i].mActive)
            continue;

        int index = mCharacterStatus[player][i].mCharID - 1;
        mCharacterModel[index].mModel->setCurrentViewNo(0);

        MtxPtr effectMtx = mSceneLight->getEffectMtx();
        if (i == 1)
            effectMtx = ((LightObj *)mSubLight[1])->getEffectMtx();
        mCharacterModel[index].mModel->setEffectMtx(effectMtx, 1);

        if (index + 1 == 17)
        {
            mCharacterModel[20].mModel->setCurrentViewNo(0);
            mCharacterModel[20].mModel->setEffectMtx(effectMtx, 1);
        }
    }

    j3dSys.setDrawModeOpaTexEdge();
    mDrawBufferA[player]->draw();
    j3dSys.setDrawModeXlu();
    mDrawBufferB[player]->draw();

    for (int i = 0; i < 2; ++i)
    {
        if (mCharacterStatus[player][i].mActive && mCharacterStatus[player][i].mCharID == 19)
        {
            SimpleDrawer drawer;
            drawer.drawInit(mAccessoryModel);
            while (drawer.loadPreDrawSetting())
            {
                mAccessoryModel->simpleDraw(0, nullptr, 1);
            }
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        mCharacterStatus[player][i].mActive = 0;
    }
    mKartStatus[player].mActive = 0;
}

void CharacterSelect3D::calc()
{
    for (int i = 0; i < 4; ++i)
    {
        mDrawBufferA[i]->frameInit();
        mDrawBufferB[i]->frameInit();
    }

    for (int player = 0; player < 4; ++player)
    {
        j3dSys.setDrawBuffer(mDrawBufferA[player], 0);
        j3dSys.setDrawBuffer(mDrawBufferB[player], 1);

        for (int position = 0; position < 2; ++position)
        {
            if (!mCharacterStatus[player][position].mActive)
                continue;

            GXLightID lightMask = position ? GX_LIGHT2 : GX_LIGHT0;
            int index = mCharacterStatus[player][position].mCharID - 1;
            mCharacterModel[index].mModel->setLightMask(
                mCharacterModel[index].mModelData, lightMask);
            if (mCharacterStatus[player][position].mCharID == 17)
            {
                mCharacterModel[20].mModel->setLightMask(
                    mCharacterModel[20].mModelData, lightMask);
            }

            calcCharacter(player, position, mCharacterStatus[player][position].mCharID,
                          mCharacterStatus[player][position].mPos,
                          mCharacterStatus[player][position].mTarget,
                          mCharacterStatus[player][position].mScale);

            if (mCharacterStatus[player][position].mCharID == 19)
            {
                mAccessoryAnm->setFrame(mAccessoryScale);

                int rootIdx = mCharacterModel[index].mModelData->getJointName()->getIndex("skl_root");
                Mtx mtx;
                PSMTXCopy(mCharacterModel[index].mModel->getModel()->getAnmMtx(rootIdx), mtx);

                f32 scale = mCharacterStatus[player][position].mScale *
                            mCharacterDefScale[index] * 0.5f;
                JGeometry::TVec3f scaleVec;
                scaleVec.set(scale, scale, scale);

                mAccessoryModel->setCurrentViewNo(0);
                mAccessoryModel->setBaseTRMtx(mtx);
                mAccessoryModel->setBaseScale(scaleVec);
                mAccessoryModel->update(0);
            }
        }
    }

    f32 oldAccessoryScale = mAccessoryScale;
    mAccessoryScale = oldAccessoryScale + 1.0f;
    if (oldAccessoryScale >= mAccessoryAnm->getFrameMax())
        mAccessoryScale = 0.0f;

    for (int player = 0; player < 4; ++player)
    {
        if (!mKartStatus[player].mActive)
            continue;

        EKartID kartID = mKartStatus[player].mKartID;
        CharacterSelect3DParam *param = (CharacterSelect3DParam *)mNode->getReflexible();

        JGeometry::TVec3f pos = mKartStatus[player].mPos;
        JGeometry::TVec3f target = mKartStatus[player].mTarget;
        f32 scale = mKartStatus[player].mScale;

        int count;
        if (kartID == KOOPA_KING || kartID == BARREL_TRAIN)
            count = 6;
        else
            count = 4;

        const f32 *body = mDefaultTable[kartID * 4];
        scale *= mKartDefScale[kartID];

        Mtx mtx;
        PSMTXTrans(mtx, body[0] * scale, body[1] * scale, body[2] * scale);

        if (param->mAdjust & 1)
            calcRotTrans(mtx, param->mKartPos, param->mKartTarget);
        else
            calcRotTrans(mtx, pos, target);

        mKartModel[kartID]->setCurrentViewNo(0);
        mKartModel[kartID]->setBaseTRMtx(mtx);
        JGeometry::TVec3f scaleVec;
        scaleVec.set(scale, scale, scale);
        mKartModel[kartID]->setBaseScale(scaleVec);

        for (int i = 0; i < count; ++i)
        {
            const f32 *part = mDefaultTable[kartID * 4 + 1] + i * 9;
            JGeometry::TVec3f partPos;
            JGeometry::TVec3f partTarget;
            JGeometry::TVec3f partScale;
            partPos.set(part[0] * scale, part[1] * scale, part[2] * scale);
            partTarget.set(part[3] * scale, part[4] * scale, part[5] * scale);

            JGeometry::TVec3f p;
            JGeometry::TVec3f t;
            PSMTXTrans(mtx, 0.0f, 0.0f, 0.0f);
            if (param->mAdjust & 1)
            {
                p.set(param->mKartPos.x + partPos.x, param->mKartPos.y + partPos.y,
                      param->mKartPos.z + partPos.z);
                t.set(param->mKartTarget.x + partTarget.x, param->mKartTarget.y + partTarget.y,
                      param->mKartTarget.z + partTarget.z);
            }
            else
            {
                p.set(pos.x + partPos.x, pos.y + partPos.y, pos.z + partPos.z);
                t.set(target.x + partTarget.x, target.y + partTarget.y, target.z + partTarget.z);
            }
            calcRotTrans(mtx, p, t);
            mKartPartsA[kartID][i]->setCurrentViewNo(0);
            mKartPartsA[kartID][i]->setBaseTRMtx(mtx);
            partScale.set(part[6] * scale, part[7] * scale, part[8] * scale);
            mKartPartsA[kartID][i]->setBaseScale(partScale);
        }

        if (kartID != BARREL_TRAIN && (u32)(kartID - 16) > 1)
        {
            for (int i = 0; i < count; ++i)
            {
                const f32 *part = mDefaultTable[kartID * 4 + 2] + i * 9;
                JGeometry::TVec3f partPos;
                JGeometry::TVec3f partTarget;
                JGeometry::TVec3f partScale;
                partPos.set(part[0] * scale, part[1] * scale, part[2] * scale);
                partTarget.set(part[3] * scale, part[4] * scale, part[5] * scale);

                JGeometry::TVec3f p;
                JGeometry::TVec3f t;
                PSMTXTrans(mtx, 0.0f, 0.0f, 0.0f);
                if (param->mAdjust & 1)
                {
                    p.set(param->mKartPos.x + partPos.x, param->mKartPos.y + partPos.y,
                          param->mKartPos.z + partPos.z);
                    t.set(param->mKartTarget.x + partTarget.x, param->mKartTarget.y + partTarget.y,
                          param->mKartTarget.z + partTarget.z);
                }
                else
                {
                    p.set(pos.x + partPos.x, pos.y + partPos.y, pos.z + partPos.z);
                    t.set(target.x + partTarget.x, target.y + partTarget.y, target.z + partTarget.z);
                }
                calcRotTrans(mtx, p, t);
                mKartPartsB[kartID][i]->setCurrentViewNo(0);
                mKartPartsB[kartID][i]->setBaseTRMtx(mtx);
                partScale.set(part[6] * scale, part[7] * scale, part[8] * scale);
                mKartPartsB[kartID][i]->setBaseScale(partScale);
            }
        }

        for (int i = 0; i < count; ++i)
        {
            const f32 *part = mDefaultTable[kartID * 4 + 3] + i * 9;
            JGeometry::TVec3f partPos;
            JGeometry::TVec3f partTarget;
            JGeometry::TVec3f partScale;
            partPos.set(part[0] * scale, part[1] * scale, part[2] * scale);
            partTarget.set(part[3] * scale, part[4] * scale, part[5] * scale);

            JGeometry::TVec3f p;
            JGeometry::TVec3f t;
            PSMTXTrans(mtx, 0.0f, 0.0f, 0.0f);
            if (param->mAdjust & 1)
            {
                p.set(param->mKartPos.x + partPos.x, param->mKartPos.y + partPos.y,
                      param->mKartPos.z + partPos.z);
                t.set(param->mKartTarget.x + partTarget.x, param->mKartTarget.y + partTarget.y,
                      param->mKartTarget.z + partTarget.z);
            }
            else
            {
                p.set(pos.x + partPos.x, pos.y + partPos.y, pos.z + partPos.z);
                t.set(target.x + partTarget.x, target.y + partTarget.y, target.z + partTarget.z);
            }
            calcRotTrans(mtx, p, t);
            mKartPartsC[kartID][i]->setCurrentViewNo(0);
            mKartPartsC[kartID][i]->setBaseTRMtx(mtx);
            partScale.set(part[6] * scale, part[7] * scale, part[8] * scale);
            mKartPartsC[kartID][i]->setBaseScale(partScale);
        }
    }

    for (int kartID = 0; kartID < 21; ++kartID)
    {
        mKartModel[kartID]->update(0);
        for (int i = 0; i < 6; ++i)
        {
            if (mKartPartsA[kartID][i] != nullptr)
                mKartPartsA[kartID][i]->update(0);
            if (mKartPartsB[kartID][i] != nullptr)
                mKartPartsB[kartID][i]->update(0);
            if (mKartPartsC[kartID][i] != nullptr)
                mKartPartsC[kartID][i]->update(0);
        }
    }

    LightMgr::getManager()->calc();

    mKartHeight += 1.0f;
    if (mKartHeight >= 360.0f)
        mKartHeight -= 360.0f;
}

void CharacterSelect3D::setCharacter(int player, int position, ECharID character,
                                     JGeometry::TVec3f pos, JGeometry::TVec3f target, f32 scale)
{
    CharacterSelect3DParam *param = (CharacterSelect3DParam *)mNode->getReflexible();

#line 1448
    JUT_MINMAX_ASSERT(0, player, 4);
    JUT_MINMAX_ASSERT(0, position, 2);
    JUT_ASSERT(cCharIDNone < character && character < cCharIDMax);
#line 723

    mCharacterStatus[player][position].mActive = 1;
    mCharacterStatus[player][position].mCharID = character;
    mCharacterStatus[player][position].mPos = pos;
    mCharacterStatus[player][position].mTarget = target;
    mCharacterStatus[player][position].mScale = scale;

    if (param->mAdjust & 1)
    {
        mCharacterStatus[player][position].mScale = param->mBodyScale[position];
    }
}

void CharacterSelect3D::setCharacterStatus(int player, int position, CharacterSelect3D::CHARACTER_STATUS status)
{
#line 1475
    JUT_MINMAX_ASSERT(0, player, 4);
    JUT_MINMAX_ASSERT(0, position, 2);
#line 745

    switch (status)
    {
    case STATUS_0:
        mCharacterStatus[player][position].mState = status;
        mCharacterStatus[player][position].mPrevState = status;
        mCharacterStatus[player][position].mTimer[0] = 0.0f;
        mCharacterStatus[player][position].mTimer[1] = 0.0f;
        mCharacterStatus[player][position].mTimer[2] = 0.0f;
        break;
    case STATUS_1:
        if (mCharacterStatus[player][position].mState != STATUS_0 &&
            mCharacterStatus[player][position].mState == STATUS_2)
        {
            mCharacterStatus[player][position].mPrevState = mCharacterStatus[player][position].mState;
            mCharacterStatus[player][position].mState = status;
            mCharacterStatus[player][position].mTimer[1] = 0.0f;
        }
        break;
    case STATUS_2:
        if (mCharacterStatus[player][position].mState != STATUS_2)
        {
            mCharacterStatus[player][position].mPrevState = mCharacterStatus[player][position].mState;
            mCharacterStatus[player][position].mState = status;
            mCharacterStatus[player][position].mTimer[2] = 0.0f;
        }
        break;
    default:
#line 1503
        JUT_ASSERT(0);
    }
}

void CharacterSelect3D::setKart(int player, EKartID kartType, JGeometry::TVec3f pos,
                                JGeometry::TVec3f target, f32 scale)
{
    CharacterSelect3DParam *param = (CharacterSelect3DParam *)mNode->getReflexible();

#line 1523
    JUT_MINMAX_ASSERT(0, player, 4);
    JUT_MINMAX_ASSERT(0, kartType, 21);
#line 784

    mKartStatus[player].mActive = 1;
    mKartStatus[player].mKartID = kartType;
    mKartStatus[player].mPos = pos;
    mKartStatus[player].mTarget = target;
    mKartStatus[player].mTarget.y = mKartHeight;
    mKartStatus[player].mScale = scale;

    if (param->mAdjust & 1)
    {
        mKartStatus[player].mScale = param->mKartScale;
    }
}

int CharacterSelect3D::isCancel(int player)
{
    if (mCharacterStatus[player][0].mState == STATUS_2 &&
        mCharacterStatus[player][1].mState == STATUS_2 &&
        mCharacterStatus[player][0].mTimer[2] >= 0.0f &&
        mCharacterStatus[player][1].mTimer[2] >= 0.0f)
    {
        return 1;
    }
    return 0;
}

int CharacterSelect3D::isNext(int player)
{
    if (mCharacterStatus[player][0].mState == STATUS_2 &&
        mCharacterStatus[player][1].mState == STATUS_2 &&
        mCharacterStatus[player][0].mTimer[2] >= 0.0f &&
        mCharacterStatus[player][1].mTimer[2] >= 0.0f)
    {
        return 1;
    }
    return 0;
}

MtxPtr CharacterSelect3D::getCameraMatrix() {
    return mCameraMatrix;
}

f32 CharacterSelect3D::getFovy()
{
    return ((CharacterSelect3DParam *)mNode->getReflexible())->mFovy;
}

void CharacterSelect3D::calcCharacter(int player, int position, ECharID character,
                                      JGeometry::TVec3f pos, JGeometry::TVec3f target, f32 scale)
{
    CharacterSelect3DParam *param = (CharacterSelect3DParam *)mNode->getReflexible();

#line 1622
    JUT_MINMAX_ASSERT(0, player, 4);
    JUT_MINMAX_ASSERT(0, position, 2);
    JUT_ASSERT(cCharIDNone < character && character < cCharIDMax);
#line 1670

    int index = character - 1;
    Mtx mtx;
    PSMTXTrans(mtx, 0.0f, 0.0f, 0.0f);
    if (param->mAdjust & 1)
        calcRotTrans(mtx, param->mBodyPos[position], param->mBodyTarget[position]);
    else
        calcRotTrans(mtx, pos, target);

    mCharacterModel[index].mModel->setBaseTRMtx(mtx);
    if (index + 1 == 18)
        mCharacterModel[20].mModel->setBaseTRMtx(mtx);

    scale *= mCharacterDefScale[index];
    JGeometry::TVec3f scaleVec;
    scaleVec.set(scale, scale, scale);
    mCharacterModel[index].mModel->setBaseScale(scaleVec);
    if (index + 1 == 18)
        mCharacterModel[20].mModel->setBaseScale(scaleVec);

    switch (mCharacterStatus[player][position].mState)
    {
    case STATUS_0:
    {
        f32 old = mCharacterStatus[player][position].mTimer[0];
        mCharacterStatus[player][position].mTimer[0] = old + 1.0f;
        if (old >= mCharacterModel[index].mAnm[0].mAnmTransform->getFrameMax())
        {
            mCharacterStatus[player][position].mPrevState = STATUS_0;
            mCharacterStatus[player][position].mState = STATUS_1;
        }
        break;
    }
    case STATUS_1:
    {
        f32 old = mCharacterStatus[player][position].mTimer[1];
        mCharacterStatus[player][position].mTimer[1] = old + 1.0f;
        if (old >= mCharacterModel[index].mAnm[1].mAnmTransform->getFrameMax())
        {
            mCharacterStatus[player][position].mPrevState = STATUS_1;
            mCharacterStatus[player][position].mTimer[1] = 0.0f;
        }
        break;
    }
    case STATUS_2:
    {
        f32 old = mCharacterStatus[player][position].mTimer[2];
        mCharacterStatus[player][position].mTimer[2] = old + 1.0f;
        if (old >= mCharacterModel[index].mAnm[2].mAnmTransform->getFrameMax())
        {
            mCharacterStatus[player][position].mTimer[2] =
                mCharacterModel[index].mAnm[2].mAnmTransform->getFrameMax() - 1.0f;
        }
        break;
    }
    default:
#line 1670
        JUT_ASSERT(0);
    }

    int state = mCharacterStatus[player][position].mState;
    int prevState = mCharacterStatus[player][position].mPrevState;
    f32 frame = mCharacterStatus[player][position].mTimer[state];

    mCharacterModel[index].mAnm[state].mAnmTransform->setFrame(frame);
    if (index + 1 == 18)
        mCharacterModel[20].mAnm[state].mAnmTransform->setFrame(frame);

    if (frame < 0.0f && prevState != state)
    {
        if (prevState == STATUS_0 && state == STATUS_1)
        {
            mCharacterModel[index].mModelData->getJointNodePointer(0)->setMtxCalc(
                mCharacterModel[index].mAnm[1].mMtxCalc);
            if (index + 1 == 18)
                mCharacterModel[20].mModelData->getJointNodePointer(0)->setMtxCalc(
                    mCharacterModel[20].mAnm[1].mMtxCalc);
        }
        else if (prevState == STATUS_1 && state == STATUS_2)
        {
            mCharacterModel[index].mModelData->getJointNodePointer(0)->setMtxCalc(
                mCharacterModel[index].mMtxCalc02);
            mCharacterModel[index].mMtxCalc02->setWeight(0, 1.0f - frame);
            mCharacterModel[index].mMtxCalc02->setWeight(1, 0.5f * frame);
            if (index + 1 == 18)
            {
                mCharacterModel[20].mModelData->getJointNodePointer(0)->setMtxCalc(
                    mCharacterModel[20].mMtxCalc02);
                mCharacterModel[20].mMtxCalc02->setWeight(0, 1.0f - frame);
                mCharacterModel[20].mMtxCalc02->setWeight(1, 0.5f * frame);
            }
        }
        else if (prevState == STATUS_2 && state == STATUS_1)
        {
            mCharacterModel[index].mModelData->getJointNodePointer(0)->setMtxCalc(
                mCharacterModel[index].mMtxCalc02);
            mCharacterModel[index].mMtxCalc02->setWeight(1, 1.0f - frame);
            mCharacterModel[index].mMtxCalc02->setWeight(0, 0.5f * frame);
            if (index + 1 == 18)
            {
                mCharacterModel[20].mModelData->getJointNodePointer(0)->setMtxCalc(
                    mCharacterModel[20].mMtxCalc02);
                mCharacterModel[20].mMtxCalc02->setWeight(1, 1.0f - frame);
                mCharacterModel[20].mMtxCalc02->setWeight(0, 0.5f * frame);
            }
        }
        else if (prevState == STATUS_0 && state == STATUS_2)
        {
            mCharacterModel[index].mModelData->getJointNodePointer(0)->setMtxCalc(
                mCharacterModel[index].mMtxCalc12);
            mCharacterModel[index].mMtxCalc12->setWeight(0, 1.0f - frame);
            mCharacterModel[index].mMtxCalc12->setWeight(1, 0.5f * frame);
            if (index + 1 == 18)
            {
                mCharacterModel[20].mModelData->getJointNodePointer(0)->setMtxCalc(
                    mCharacterModel[20].mMtxCalc12);
                mCharacterModel[20].mMtxCalc12->setWeight(0, 1.0f - frame);
                mCharacterModel[20].mMtxCalc12->setWeight(1, 0.5f * frame);
            }
        }
        else
        {
#line 1724
            JUT_ASSERT(0);
        }
    }
    else
    {
        mCharacterModel[index].mModelData->getJointNodePointer(0)->setMtxCalc(
            mCharacterModel[index].mAnm[state].mMtxCalc);
        if (index + 1 == 18)
            mCharacterModel[20].mModelData->getJointNodePointer(0)->setMtxCalc(
                mCharacterModel[20].mAnm[state].mMtxCalc);
    }

    if (state == STATUS_2)
        mCharacterModel[index].mAnmTexPattern->setFrame(frame);
    else
        mCharacterModel[index].mAnmTexPattern->setFrame(0.0f);

    if (index + 1 == 18)
    {
        if (state == STATUS_2)
            mCharacterModel[20].mAnmTexPattern->setFrame(frame);
        else
            mCharacterModel[20].mAnmTexPattern->setFrame(0.0f);
    }

    mCharacterModel[index].mModel->update(0);
    if (index + 1 == 18)
        mCharacterModel[20].mModel->update(0);
}

void CharacterSelect3D::drawKart(EKartID kartType)
{
#line 1765
    JUT_MINMAX_ASSERT(0, kartType, 21);
#line 837

    SimpleDrawer drawer;
    int count;
    if (kartType == 7 || kartType == 9)
    {
        count = 6;
    }
    else
    {
        count = 4;
    }

    drawer.drawInit(mKartModel[kartType]);
    while (drawer.loadPreDrawSetting())
    {
        mKartModel[kartType]->simpleDraw(0, mSubLight[0]->getEffectMtx(), 1);
    }

    for (int i = 0; i < count; ++i)
    {
        drawer.drawInit(mKartPartsA[kartType][i]);
        while (drawer.loadPreDrawSetting())
        {
            mKartPartsA[kartType][i]->simpleDraw(0, mSubLight[0]->getEffectMtx(), 1);
        }
    }

    if (kartType != 9 && (u32)(kartType - 16) > 1)
    {
        for (int i = 0; i < count; ++i)
        {
            drawer.drawInit(mKartPartsB[kartType][i]);
            while (drawer.loadPreDrawSetting())
            {
                mKartPartsB[kartType][i]->simpleDraw(0, mSubLight[0]->getEffectMtx(), 1);
            }
        }
    }

    for (int i = 0; i < count; ++i)
    {
        drawer.drawInit(mKartPartsC[kartType][i]);
        while (drawer.loadPreDrawSetting())
        {
            mKartPartsC[kartType][i]->simpleDraw(0, mSubLight[0]->getEffectMtx(), 1);
        }
    }
}

void CharacterSelect3D::calcRotTrans(Mtx m, JGeometry::TVec3f pos, JGeometry::TVec3f rot)
{
    Mtx rotX, rotY, rotZ, trans;

    PSMTXRotRad(rotX, 'X', 0.017453292f * rot.x);
    PSMTXRotRad(rotY, 'Y', 0.017453292f * rot.y);
    PSMTXRotRad(rotZ, 'Z', 0.017453292f * rot.z);
    PSMTXTrans(trans, pos.x, pos.y, pos.z);

    PSMTXConcat(rotX, m, m);
    PSMTXConcat(rotY, m, m);
    PSMTXConcat(rotZ, m, m);
    PSMTXConcat(trans, m, m);
}


#include "JSystem/JAudio/JASFakeMatch2.h"
