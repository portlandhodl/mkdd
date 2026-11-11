#ifndef DRIVER_H
#define DRIVER_H

#include "Inagaki/GameSoundMgr.h"

#include "Kaneshige/ExModel.h"
#include "Kaneshige/KartInfo.h"
#include "Kawano/driverData.h"
#include "Sato/ItemObj.h"
#include "JSystem/J3D/J3DJoint.h"

class J3DAnmTransform; // TODO: JSystem
class J3DJoint;
class KartLoader;

class NullAnimator
{
public:
    f32 mCurrentValue; // 0x0
    NullAnimator() : mCurrentValue(0.0f) {}
    virtual void start() {}  // v1
    virtual void stop() {}   // v2
    virtual void reset() {}  // v3
    virtual void update() {} // v4
}; // Size: 0x8

class AnimatorWind : public NullAnimator
{
public:
    s32 mFrameCounter; // 0x8
    s32 mAnimationState; // 0xC
    AnimatorWind() : mFrameCounter(0), mAnimationState(0) {}
    virtual void start();
    virtual void stop();
    virtual void reset();
}; // Size: 0x10

class LoopAnimatorWind : public AnimatorWind
{
public:
    f32 mMinValue;     // 0x10
    f32 mMaxValue;     // 0x14
    f32 mSpeed;        // 0x18
    f32 mCurrentPhase; // 0x1C
    LoopAnimatorWind() : mMinValue(0.0f), mMaxValue(0.0f), mSpeed(0.0f), mCurrentPhase(0.0f) {}
    virtual void update();
}; // Size: 0x20

class OneShotAnimator2 : public NullAnimator
{
public:
    f32 mMinValue;   // 0x8
    f32 mMaxValue;   // 0xC
    f32 mRate;       // 0x10
    f32 mTime;       // 0x14
    s32 mFrame;      // 0x18
    s32 mMaxFrame;   // 0x1C
    s32 mStepFrame;  // 0x20
    s32 mIsFinished; // 0x24

    OneShotAnimator2()
    {
        mMinValue = 0.0f;
        mMaxValue = 0.0f;
        mRate = 0.0f;
        mTime = 0.0f;
        mFrame = 0;
        mMaxFrame = 0;
        mStepFrame = 0x10;
        mIsFinished = 1;
    }

    virtual void start() { mIsFinished = 0; }
    virtual void reset()
    {
        mFrame = 0;
        mIsFinished = 1;
    }
    virtual void update();
}; // Size: 0x28
class DriversAnimator
{
public:
    DriversAnimator() { reset(); }
    ~DriversAnimator();

    void reset()
    {
        mLoopAnim.reset();
        mOneShotAnm[0].reset();
        mOneShotAnm[1].reset();
    }

    LoopAnimatorWind mLoopAnim;      // 0x0
    OneShotAnimator2 mOneShotAnm[2]; // 0x20
}; // Size: 0x70

class DriverAnimBufMgr;
class DriverTPAnim;
class DriverLoader;
class KartMtxCalcBlendAnm;
class KartMtxCalcAdditiveAnm;

class NeckCtrl;
class LumbarCtrl;
class ArmCtrl;
class FootCtrl;

class DriverModel : public ExModel
{
public:
    enum StateEnum
    {
        mcDrive = 0,
        mcDriveBack = 1,
        mcDriveC = 2,
        mcTandemB = 3,
        mcTandemAlone = 4,
        mcChangeAtoB = 5,
        mcChangeBtoA = 6,
        mcS007 = 7, // dummy

        mcS008 = 8, // dummy

        mcS009 = 9, // b_throw_a1 / b_throw_a1

        mcS010 = 10, // b_throw_a1 / b_throw_a1

        mcS011 = 11, // c_throw_a1 / c_throw_a1

        mcS012 = 12, // c_throw_a1 / c_throw_a1

        mcS013 = 13, // b_throw_a1

        mcS014 = 14, // b_throw_a1

        mcS015 = 15, // b_throw_b1

        mcS016 = 16, // b_throw_b1

        mcS017 = 17, // c_throw_a1

        mcS018 = 18, // c_throw_a1

        mcS019 = 19, // c_throw_b1

        mcS020 = 20, // c_throw_b1

        mcS021 = 21, // a_drive_a

        mcS022 = 22, // a_drive_a

        mcS023 = 23, // a_drive_a

        mcS024 = 24, // a_drive_a

        mcS025 = 25, // a_drive_a

        mcS026 = 26, // c_attack_rb

        mcS027 = 27, // b_jump_a

        mcS028 = 28, // b_jump_b

        mcS029 = 29, // c_jump_b

        mcS030 = 30, // b_sit_a / b_sit_b

        mcS031 = 31, // c_sit_b

        mcS032 = 32, // d_wait_a

        mcS033 = 33, // d_run

        mcS034 = 34, // d_normal

        mcS035 = 35, // a_drive_b

        mcS036 = 36, // a_drive_b

        mcS037 = 37, // a_drive_b / a_drive_b

        mcS038 = 38, // a_drive_b / a_drive_b

        mcS039 = 39, // a_drive_b / a_drive_b

        mcS040 = 40, // a_drive_b / a_drive_b

        mcS041 = 41, // a_drive_b / a_drive_b

        mcS042 = 42, // a_drive_b / d_normal

        mcS043 = 43, // a_drive_b

        mcS044 = 44, // a_drive_b

        mcS045 = 45, // a_drive_b

        mcS046 = 46, // a_drive_b

        mcS047 = 47, // a_drive_b

        mcS048 = 48, // a_drive_b / d_normal

        mcS049 = 49, // b_tandem_al / b_tandem_bl

        mcS050 = 50, // b_tandem_al / b_tandem_bl

        mcS051 = 51, // b_tandem_al / b_tandem_bl

        mcS052 = 52, // b_tandem_ar / b_tandem_br

        mcS053 = 53, // b_tandem_ar / b_tandem_br

        mcS054 = 54, // b_tandem_ar / b_tandem_br

        mcS055 = 55, // b_tandem_a

        mcS056 = 56, // b_tandem_a

        mcS057 = 57, // b_tandem_a

        mcS058 = 58, // c_tandem_br

        mcS059 = 59, // c_tandem_br

        mcS060 = 60, // c_tandem_br

        mcS061 = 61, // b_tandem_a

        mcS062 = 62, // b_tandem_a

        mcS063 = 63, // b_throw_b1

        mcS064 = 64, // b_throw_b1

        mcS065 = 65, // e_good_b

        mcS066 = 66, // e_good_b2

        mcS067 = 67, // e_good_a

        mcS068 = 68, // e_good_b

        mcS069 = 69, // b_dash_a / b_dash_b

        mcS070 = 70, // b_tandem_as / b_tandem_bs

        mcS071 = 71, // b_sit_a / b_sit_b

        mcS072 = 72, // c_dash_b

        mcS073 = 73, // b_tandem_a

        mcS074 = 74, // c_sit_b

        mcS075 = 75, // b_dash_a / b_dash_b

        mcS076 = 76, // c_dash_b

        mcS077 = 77, // b_wait_a / b_wait_b

        mcS078 = 78, // c_wait_b

        mcS079 = 79, // b_tandem_as / b_tandem_bs

        mcS080 = 80, // b_tandem_a

        mcS081 = 81, // b_tandem_as / b_tandem_bs

        mcS082 = 82, // b_tandem_as / b_tandem_bs

        mcS083 = 83, // b_tandem_a

        mcS084 = 84, // b_tandem_a

        mcS085 = 85, // a_drive_a / a_drive_a

        mcS086 = 86, // a_drive_a / a_drive_a

        mcS087 = 87, // a_drive_a

        mcS088 = 88, // c_attack_rb

        mcS089 = 89, // b_jump_a / b_jump_b

        mcS090 = 90, // c_jump_b

        mcS091 = 91, // b_sit_a / b_sit_b

        mcS092 = 92, // c_sit_b

        mcS093 = 93, // d_run

        mcS094 = 94, // a_drive_b

        mcS095 = 95, // a_drive_b

        mcS096 = 96, // a2b_change

        mcS097 = 97, // b2a_change

        mcS098 = 98, // S_a2b_change

        mcS099 = 99, // S_b2a_change

        mcS100 = 100, // dummy

        mcS101 = 101, // dummy

        mcS102 = 102, // dummy

        mcS103 = 103, // dummy

        mcS104 = 104, // dummy

        mcS105 = 105, // dummy

        mcS106 = 106, // a_drive_b / a_drive_b

        mcS107 = 107, // a_drive_b / a_drive_b

        mcS108 = 108, // b_tandem_a

        mcS109 = 109, // b_tandem_a

        mcS110 = 110, // b_tandem_a / b_tandem_a

        mcS111 = 111, // b_tandem_as / b_tandem_bs

        mcS112 = 112, // b_tandem_a

        mcS113 = 113, // b_tandem_al / b_tandem_bl

        mcS114 = 114, // b_tandem_ar / b_tandem_br

        mcS115 = 115, // b_tandem_a

        mcS116 = 116, // c_tandem_br

        mcS117 = 117, // a_drive_b / a_drive_b

        mcS118 = 118, // a_damage

        mcS119 = 119, // a_piyo

        mcS120 = 120, // a_drive_a / a_drive_b

        mcS121 = 121, // b2a_change_p

        mcS122 = 122, // a2b_change_p

        mcS123 = 123, // b_fire

        mcS124 = 124, // c_fire

        mcS125 = 125, // d_normal

        mcS126 = 126, // b_tandem_al / b_tandem_bl

        mcS127 = 127, // b_tandem_ar / b_tandem_br

        mcS128 = 128, // e_1st_b

        mcS129 = 129, // e_1st_b

        mcS130 = 130, // e_good_a

        mcS131 = 131, // e_good_a

        mcS132 = 132, // e_good_b

        mcS133 = 133, // b_tandem_b

        mcS134 = 134, // b_tandem_b

        mcS135 = 135, // b_tandem_b

        mcS136 = 136, // b_tandem_al / b_tandem_bl

        mcS137 = 137, // b_tandem_ar / b_tandem_br

        mcS138 = 138, // b_tandem_a

        mcS139 = 139, // c_tandem_br

        mcS140 = 140, // a_drive_a

        mcS141 = 141, // a_drive_a

        mcS142 = 142, // a_drive_a

        mcS143 = 143, // a_drive_a

        mcS144 = 144, // a_drive_a

        mcS145 = 145, // c_attack_rb

        mcS146 = 146, // b_tandem_b

        mcS147 = 147, // b_tandem_b

        mcS148 = 148, // b_tandem_b

        mcS149 = 149, // d_wait_a

        mcS150 = 150, // all_jug_a

        mcS151 = 151, // all_speak
    };

    DriverModel(KartLoader *, int, u16, bool, int);
    virtual __declspec(weak) ~DriverModel();

    // -- driver.cpp (this unit) --
    static int convCharIDToDataID(ECharID);
    void init_ik(int);
    void init(int);
    void initMtxCalc();
    void initAnmTrans_BossPakkun(J3DAnmTransform *);
    void InitMpaMtxCalc(StateEnum, J3DAnmTransform *, J3DAnmTransform *, J3DAnmTransform *, J3DAnmTransform *);
    void InitMpaMtxCalcThrow(J3DAnmTransform *);
    void restart(int);
    void setDriveAnm(f32);
    void setTandemAnm(f32, f32);
    void setTandemAnm(f32);
    void Start1ShotAnm_base(DriversAnimator *, StateEnum);
    void StartAnimation2();
    void StartAnimationWC(StateEnum);
    void StartAnimation(StateEnum);
    void setShakeAnm(StateEnum, f32);
    void setShakeAnm2(StateEnum, f32, int, int);
    void setShakeAnm3(StateEnum, f32);
    void setCommon1ShotAnm_Single(f32, int);
    void setCommon1ShotAnm(f32);
    void setCommon1ShotAnm_Single2(int, f32, int);
    void setCommon1ShotAnm2(int, f32);
    void setCommonAnm_Base(int, f32, int);
    void setCommonAnm2(int, f32);
    void setCommonAnm(f32);
    void setCommonAnm_Blend(f32, int);
    void Change_next_mState(StateEnum);
    void updateFrame();
    void updateFrame_base(DriversAnimator *, StateEnum);
    void updateSetFrame_base(StateEnum);
    void updateSetFrame();
    void updateLoad_Animation();
    void updateLoadCommand(int);
    void updateLoadCommand_Sound(int);
    void updateLoad_Sound();
    void updateLoad();
    int updateCheckRenew_Animation();
    int updateCheckRenew_Sound();
    int updateCheckRenew();
    f64 BlendValue(f32);
    void updateBlendCount_Func1(u8, u8);
    void updateBlendCount_Func2(u8);
    void updateBlendCount();
    int updateSetBlendWeight_GetBlendTime(StateEnum);
    f32 updateSetBlendWeight_Func1(u8, u8);
    void updateSetBlendWeight_FuncABCD(f32, f32, f32, f32);
    void updateSetBlendWeight_mStateThrow();
    void updateSetBlendWeight();
    void update_mIsSpeakBCA();
    void BacktoDWait() { StartAnimation((StateEnum)0x95); }
    void TurnBack() { StartAnimation((StateEnum)104); }
    void StartNormalStart() { StartAnimation((StateEnum)34); }
    void StartRocketStart() { StartAnimation((StateEnum)94); }
    void StartRocketStartFailure() { StartAnimation((StateEnum)95); }
    void setItem(ItemObj *);
    void resetItem();
    void setLODLevel(int);
    Mtx *getMtxP_HandR();
    Mtx *getMtxP_HandR2();
    Mtx *getMtxP(int);
    J3DAnmTransform *getAnmTransformPtr(int);
    int IsAnmResInMRAM(int);
    int IsAnmSoundResInMRAM(int);
    bool IsARAMDMAFinished();

    // -- driver_ik.cpp (this unit) --
    void setDirMtx(JGeometry::TVec3f &, JGeometry::TVec3f &, JGeometry::TVec3f &, JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > *);
    void calcMPos(JGeometry::TVec3f &, JGeometry::TVec3f &, JGeometry::TVec3f &, float, float, JGeometry::TVec3f *);
    void calc1stJnt(J3DModel *, u16, u16, u16, u16, u16);
    void calc2ndJnt(J3DModel *, u16, u16, u16);
    void calc3rdJnt(J3DModel *, u16, u16, bool);
    bool CheckNoIK();
    static bool koshiCallBack(J3DJoint *, int);
    static bool spinCallBack(J3DJoint *, int);
    static bool footL1CallBack(J3DJoint *, int);
    static bool footL2CallBack(J3DJoint *, int);
    static bool footL3CallBack(J3DJoint *, int);
    static bool footLPosCallBack(J3DJoint *, int);
    static bool footR1CallBack(J3DJoint *, int);
    static bool footR2CallBack(J3DJoint *, int);
    static bool footR3CallBack(J3DJoint *, int);
    static bool footRPosCallBack(J3DJoint *, int);
    static bool armL1CallBack(J3DJoint *, int);
    static bool armL2CallBack(J3DJoint *, int);
    static bool armL3CallBack(J3DJoint *, int);
    static bool armR1CallBack(J3DJoint *, int);
    static bool armR2CallBack(J3DJoint *, int);
    static bool armR3CallBack(J3DJoint *, int);
    static bool handRCallBack(J3DJoint *, int);
    static bool faceCallBack(J3DJoint *, int);
    static bool mouthCallBack(J3DJoint *, int);
    static bool skirtCallBack(J3DJoint *, int);
    static bool skirtPosCallBack(J3DJoint *, int);
    static bool dHair1CallBack(J3DJoint *, int);
    static bool dHair2CallBack(J3DJoint *, int);
    static bool pHair1CallBack(J3DJoint *, int);
    static bool pHair2CallBack(J3DJoint *, int);
    static bool tie1CallBack(J3DJoint *, int);

    static int IsFootLIKOn(DriverModel *);
    static int IsFootRIKOn(DriverModel *);
    static int IsArmLIKOn(DriverModel *);
    static int IsArmRIKOn(DriverModel *);
    static int IsAllIKOff(DriverModel *);
    static int IsAllIKOffWithoutFace(DriverModel *);
    void SetOffset(J3DModel *, DriverModel *, u16);
    void SetOffset2(DriverModel *, u16);

    static DriverModel *sCurDriver;
    static J3DJointCallBack mscaCallBackTbl[27];

    virtual void update(u16);                           // override
    virtual bool createModel(JKRSolidHeap *, u32, u32); // override
    virtual void setCurrentViewNo(u32);                 // override

    // -- functions defined in sibling units (driverAnim.cpp etc) --
    void goBack();
    void goFront();
    void TurnFront();
    void StartThrow(int, int, int);
    void StartThrow(int);
    void StartDriverThrow(int, int);
    void StartJump(int);
    void StartJump();
    void StartSit();
    void StartSit(int);
    void StartStand();
    void StartStart();
    void StartAttackL();
    void StartAttackR();
    void StartLoopAnm(StateEnum, DriversAnimator *, f32, f32, f32, f32);
    void StartLoopAnm2(DriversAnimator *, f32, f32, f32, f32);
    void StartDWait();
    void StartDemo();
    void StartDemo2();
    void StartDemo2Front();
    void StartDemo2Back();
    void StartDrift(int);
    void StartDriftEnd(int);
    void StartDamage(int, int);
    void StartDamage_GetUp(int, int);
    void StartDash();
    void StartHang();
    void StartDashEnd();
    void StartHang2(f32, u8);
    void StartDashEnd2(f32, u8);
    void StartDangerL();
    void StartDangerR();
    void StartDangerEnd(int);
    void StartDriverDamage();
    void StartDriverDrive();
    void StartGoalMotion();
    void StartGood();
    void StartBad();
    void StartFire();
    void StartFireEnd();
    void StartJuggle();
    void StartSpeak();
    void SetDriverFlags(StateEnum);
    void CheckDriverFlags();
    int CheckDriverFlags_Loop(StateEnum);
    int CheckDriverFlags_Hokan(StateEnum);
    int IsDrive(StateEnum);
    int IsDemo(StateEnum);
    int IsDrift(StateEnum);
    int IsTandem(StateEnum);
    int IsChange(StateEnum);
    int IsSit(StateEnum);
    int IsMove();
    int IsSit();
    int IsStand();
    int IsBack();
    int IsFront();
    int IsDemo();
    int IsDemoBack();
    int IsDemoFront();
    int IsDrive();
    int IsDanger();
    int IsDangerPlus();
    int IsDangerEnd();
    int IsDash();
    int IsDash1();
    int IsDash2();
    int IsTire();
    int IsWait();
    int IsWait(StateEnum);
    int IsTurn();
    int IsJuggle();
    int IsBeforeStart();
    int IsDamage();
    int IsDamageL();
    int IsDamageL12();
    int IsDamageS12();
    int IsAttack();
    int IsThrow();
    int IsTandem();
    int IsDrift();
    int IsDriftLeft();
    int IsDriftEnd();
    int IsDriftFinish();
    int IsHappy();
    int IsHappyFront();
    int IsHappyBack();
    int IsGoal();
    int IsGoalFront();
    int IsGoalBack();
    int IsTandemAnmStartOK();
    int IsItemWithTurn();
    int IsRocketStart();
    int IsPiyo();
    int IsChange();
    int IsChangeBack();
    int IsChangeFront();
    int IsStart();
    int IsNormalStart();
    int IsTurnBack();
    int IsTurnFront();
    int IsDamageS();
    int IsItemThrowOK();
    int IsAppeal();
    int IsDriftStart();
    int IsDriftRight();
    int IsRun();
    int IsJump();
    int IsFire();
    int IsDriverDamage();
    void SetFreeze();
    void ResetFreeze();
    void StartHappy();
    void StartGood1st();
    void StartAppeal();
    void StartHokanDWait();
    void StartAttack(int, int);
    void StartAttack(int);
    void StartWait();
    void StartWaitBack();
    void StartRun2();
    void StartDRunAnim(DriversAnimator *);
    void Check4StartDrift();
    void StartDrift();
    void StartDriftEnd();
    void StartHokanDash();
    void StartHokanDash2();
    void SetHangRate(f32);
    void StartDanger(int);
    void StartDriverPiyo();
    void StartSetTire(int);
    void StartSetTireL();
    void StartSetTireR();
    int IsThrowBack();
    int IsTire2();
    int IsDamage2();
    int IsDamageS2();
    int IsDamageL2();
    int IsAppeal(StateEnum);
    int IsDemoFront(StateEnum);
    int IsDemoBack(StateEnum);
    void InitDriverOffsetPosition();
    void SetDriverOffsetPosition();
    void SetDriverOffsetPosition_Func1(JGeometry::TVec3<f32> *);
    void SetDriverOffsetPosition_Change(DriverData *);
    void SetDriverOffsetPosition_Demo(DriverData *);

    static const s8 msNext_mState_HokanDamages[2];
    static const s32 msAnmBlendList[152][2]; // state -> 2 animation ids
    static void *msAnmSabunList;

    GameAudio::CharacterSoundMgr *getCharSoundMgr() { return mCharSoundMgr; }
    GameAudio::CharacterSoundMgr *getGameAudioCharacterSoundMgr() { return mCharSoundMgr; } // legacy name used by Yamamoto units

public:
    s32 mCharID;                                 // 0x8C
    s32 mDataID;                                 // 0x90
    KartLoader *mKartLoader;                     // 0x94
    bool m_98;                                    // 0x98
    s32 m_9c;                                    // 0x9C
    s32 mKartNum;                                // 0xA0
    s32 mDriverNo;                               // 0xA4
    s32 mState;                                  // 0xA8
    u8 m_ac;                                     // 0xAC
    u8 m_ad;                                     // 0xAD
    f32 m_b0;                                    // 0xB0
    ItemObj *mItem;                              // 0xB4
    u8 m_b8;                                     // 0xB8
    u8 m_b9;                                     // 0xB9
    u8 m_ba;                                     // 0xBA
    u8 m_bb;                                     // 0xBB
    u8 mHokanDamageCnt;                          // 0xBC
    s16 m_be;                                    // 0xBE
    s16 m_c0;                                    // 0xC0
    s16 m_c2;                                    // 0xC2
    s16 m_c4;                                    // 0xC4
    s32 m_c8;                                    // 0xC8
    s32 m_cc;                                    // 0xCC
    s32 m_d0;                                    // 0xD0
    s32 m_d4;                                    // 0xD4
    s32 m_d8;                                    // 0xD8
    s32 m_dc;                                    // 0xDC
    s32 m_e0;                                    // 0xE0
    s32 m_e4;                                    // 0xE4
    s32 m_e8;                                    // 0xE8
    DriverLoader *mDriverLoader;                 // 0xEC
    DriverAnimBufMgr *mAnimBufMgr;               // 0xF0
    J3DAnmTransform *mKeepAnmTrans[11];          // 0xF4..0x120
    f32 mAnmFrame[82];                           // 0x120..0x268
    s32 mAnmID;                                  // 0x268
    void *mKeepSndFile[1];                        // 0x26C (sound keep-res slots)
    DriverTPAnim *mTPAnim;                       // 0x270
    KartMtxCalcBlendAnm *mMtxCalcs[0x98];           // 0x274..0x4D4
    void *mMtxCalcsThrow[2];                     // 0x4D4,0x4D8 ([0] = KartMtxCalcAdditiveAnm*)
    NeckCtrl *mNeckCtrl;                         // 0x4DC
    LumbarCtrl *mLumbarCtrl;                     // 0x4E0
    ArmCtrl *mArmCtrl;                           // 0x4E4
    FootCtrl *mFootCtrlL;                        // 0x4E8
    FootCtrl *mFootCtrlR;                        // 0x4EC
    void *mHairCtrl;                             // 0x4F0
    s32 m_4f4;                                   // 0x4F4
    s32 m_4f8;                                   // 0x4F8
    s32 mBeforeState;                            // 0x4FC
    s32 m_500;                                   // 0x500
    s32 m_504;                                   // 0x504
    s32 next_mState_Hokan;                       // 0x508
    s32 next_before_mState_Hokan;                // 0x50C
    s32 m_510;                                   // 0x510
    s32 m_514;                                   // 0x514
    DriversAnimator mDriveAnim;                  // 0x518..0x588
    DriversAnimator mTandemAnim;                 // 0x588..0x5F8
    DriversAnimator mAdditiveAnim;               // 0x5F8..0x668
    LoopAnimatorWind m_668;                      // 0x668..0x688
    LoopAnimatorWind m_688;                      // 0x688..0x6A8
    s32 mOffsetMode;                             // 0x6A8
    s32 mOffsetDir;                              // 0x6AC
    f32 mOffsetBlend;                            // 0x6B0
    Vec mPos;                                    // 0x6B4..0x6C0
    GameAudio::CharacterSoundMgr *mCharSoundMgr; // 0x6C0
    JGeometry::TVec3f m_6c4[4];                  // 0x6C4..0x6F4
    JGeometry::TVec3f m_6f4[4];                  // 0x6F4..0x724
    JGeometry::TVec3f m_724[4];                  // 0x724..0x754
    f32 m_754[4];                                // 0x754..0x764
    f32 mAnmWorkFrame[8];                        // 0x764..0x784
    s32 mAnmWorkID[8];                           // 0x784..0x7A4
    s32 mAnmID_debug_index;                            // 0x7A4
}; // Size: 0x7A8

#endif // DRIVER_H
