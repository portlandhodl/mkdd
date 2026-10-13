#ifndef DRIVER_H
#define DRIVER_H

#include "Inagaki/GameSoundMgr.h"

#include "Kaneshige/ExModel.h"
#include "Kaneshige/KartInfo.h"
#include "Sato/ItemObj.h"

class J3DAnmTransform; // TODO: JSystem
class KartLoader;

class DriverDataChild
{
    static int mInitCounter;
    static f32 mDriverDataDefault[91][14];

    f32 data[91][14];

public:
    DriverDataChild();
    virtual ~DriverDataChild();

}; // Size: 0x180

class DriverData : public JKRDisposer
{
    u8 _18[0x7c - 0x18];

public:
    DriverData(); // Auto Inlined
    virtual ~DriverData();
    static DriverData *create();

    static DriverDataChild msDriverData[14];
    static DriverData *mspDriverData;
}; // Size: 0x7c

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
    virtual ~DriverModel();

    // -- driver.cpp (this unit) --
    static int convCharIDToDataID(ECharID);
    void init_ik(int);
    void init(int);
    void initMtxCalc();
    void initAnmTrans_BossPakkun(J3DAnmTransform *);
    void InitMpaMtxCalc(StateEnum, J3DAnmTransform *, J3DAnmTransform *, J3DAnmTransform *, J3DAnmTransform *);
    void restart(int);
    void setDriveAnm(f32);
    void setTandemAnm(f32, f32);
    void Start1ShotAnm_base(DriversAnimator *, StateEnum);
    void StartAnimation2();
    void StartAnimation(StateEnum);
    void updateFrame();
    void updateFrame_base(DriversAnimator *, StateEnum);
    void updateSetFrame();
    void updateLoad_Animation();
    void updateLoad();
    int updateCheckRenew_Animation();
    int updateCheckRenew();
    void updateBlendCount();
    void updateSetBlendWeight_GetBlendTime(StateEnum);
    void updateSetBlendWeight();
    void BacktoDWait() { StartAnimation((StateEnum)0x95); }
    void setItem(ItemObj *);
    void resetItem();
    Mtx *getMtxP_HandR();
    Mtx *getMtxP_HandR2();
    Mtx *getMtxP(int);
    J3DAnmTransform *getAnmTransformPtr(int);
    int IsAnmResInMRAM(int);
    int IsAnmSoundResInMRAM(int);

    virtual void update(u16);                           // override
    virtual bool createModel(JKRSolidHeap *, u32, u32); // override
    virtual void setCurrentViewNo(u32);                 // override

    // -- functions defined in sibling units (driverAnim.cpp etc) --
    void goBack();
    void goFront();
    void TurnFront();
    void StartThrow(int, int, int);
    void StartDriverThrow(int, int);
    void StartJump(int);
    void StartSit();
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
    int IsDemo();
    int IsDrive();
    int IsDanger();
    int IsDangerPlus();
    int IsDash();
    int IsDamage();
    int IsDamageL();
    int IsAttack();
    int IsThrow();
    int IsTandem();
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
    void StartAnimationWC(StateEnum);
    void InitDriverOffsetPosition();
    void SetDriverOffsetPosition();
    void InitMpaMtxCalcThrow(J3DAnmTransform *);
    void updateLoadCommand(int);
    void updateLoadCommand_Sound(int);
    void updateLoad_Sound();
    void updateCheckRenew_Sound();
    void Change_next_mState(StateEnum);
    void update_mIsSpeakBCA();

    static u8 msNext_mState_HokanDamages[2];
    static const u32 msAnmBlendList[152][2]; // state -> 2 animation ids
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
    u16 m_be;                                    // 0xBE
    u16 m_c0;                                    // 0xC0
    u16 m_c2;                                    // 0xC2
    u16 m_c4;                                    // 0xC4
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
    s32 mNext_mState_Hokan;                      // 0x508
    s32 m_50c;                                   // 0x50C
    s32 m_510;                                   // 0x510
    s32 m_514;                                   // 0x514
    DriversAnimator mDriveAnim;                  // 0x518..0x588
    DriversAnimator mTandemAnim;                 // 0x588..0x5F8
    DriversAnimator mAdditiveAnim;               // 0x5F8..0x668
    LoopAnimatorWind m_668;                      // 0x668..0x688
    LoopAnimatorWind m_688;                      // 0x688..0x6A8
    u8 _6a8[0xc];                                // 0x6A8..0x6B4
    Vec mPos;                                    // 0x6B4..0x6C0
    GameAudio::CharacterSoundMgr *mCharSoundMgr; // 0x6C0
    JGeometry::TVec3f m_6c4[4];                  // 0x6C4..0x6F4
    JGeometry::TVec3f m_6f4[4];                  // 0x6F4..0x724
    JGeometry::TVec3f m_724[4];                  // 0x724..0x754
    f32 m_754[4];                                // 0x754..0x764
    f32 mAnmWorkFrame[8];                        // 0x764..0x784
    s32 mAnmWorkID[8];                           // 0x784..0x7A4
    s32 mAnmSabunNum;                            // 0x7A4
}; // Size: 0x7A8

#endif // DRIVER_H
