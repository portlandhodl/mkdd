#include "Kameda/CharacterSelect3D.h"
#include "dolphin/mtx.h"

#include "JSystem/J3D/J3DSys.h"
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



static const Vec sUpVector = {0.0f, 1.0f, 0.0f};
static const Vec sUpVector2 = {0.0f, 1.0f, 0.0f};

void *CharacterSelect3D::mDiddyDump;
void *CharacterSelect3D::mKinopioDump;
CharacterSelect3D *CharacterSelect3D::mCharacterSelect3D;

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

CharacterSelect3D::CharacterSelect3D(JKRHeap *) {}

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

        LightObj *light = i == 1 ? (LightObj *)mSubLight[1] : (LightObj *)mSceneLight;
        MtxPtr effectMtx = light->getEffectMtx();
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

    JGeometry::TVec3f up = sUpVector2;

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

        LightObj *light = i == 1 ? (LightObj *)mSubLight[1] : (LightObj *)mSceneLight;
        MtxPtr effectMtx = light->getEffectMtx();
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

void CharacterSelect3D::calc() {}

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

void CharacterSelect3D::calcCharacter(int, int, ECharID, JGeometry::TVec3f, JGeometry::TVec3f, float) {}

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
