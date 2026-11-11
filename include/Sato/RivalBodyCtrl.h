#ifndef RIVALBODYCTRL_H
#define RIVALBODYCTRL_H

#include <JSystem/JKernel/JKRDisposer.h>

#include "macros.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/CenterLine.h"
#include "Sato/RivalCtrlBase.h"

class RivalBodyCtrl;

class CLPointControllerRivalKart : public CLPointController
{
public:
    CLPointControllerRivalKart(RivalBodyCtrl *);
    virtual int findNextPointNo(CLPoint *);

    RivalBodyCtrl *mBody; // 28
};


class RivalBodyCtrl : public RivalCtrlBase, JKRDisposer
{
public:
    RivalBodyCtrl(RivalKart *);                    // 0x802804f4
    void init();                                   // 0x802805e4
    static void reset();                           // 0x80280724
    static void setRankArray();                    // 0x80280864
    static void chkPlayerKartCondition();          // 0x80280aec
    void chkBodyCondition();                       // 0x80280bec
    static int getKartIndexByRank(int);                  // 0x80280ca0
    static f32 getKartTotalDist(int);                    // 0x80285fb0
    void calcTargetPosDrift();                     // 0x80280ea4
    void calcTargetPosNormal(float);               // 0x80280ea8
    void getFwPointPos(JGeometry::TVec3f *);       // 0x80281068
    void getPointRSideDir(JGeometry::TVec3f *);    // 0x802810ac
    f32 getLengthFCenter();                       // 0x802811b4
    bool isNeighborhoodCenter(float);              // 0x8028135c
    void getDriftTargetDir(JGeometry::TVec3f *);   // 0x802813d0
    void getTargetCurveLR(float *, short);         // 0x80281558
    void chkDriftPoint();                          // 0x802818a8
    bool chkArrivePoint(CLPoint *);                // 0x80281a54
    void initDrift(bool, CLPoint *);               // 0x80281b24    
    void reviseAfterDriftVtTargetPos();            // 0x80282364    
    void doColWallCtrl();                          // 0x80282888
    int calcMaxChangeWaitCnt();                   // 0x80282cb4    
    void comebackRescure(CLPoint *, bool);         // 0x80282cf8
    void resetBodyData();                          // 0x80283054
    bool isOneUpperLowerPlayer(bool);              // 0x802830e4
    bool getLowestPlayerDist(float *);             // 0x802831f4
    int findMyRankOnlyCom();                      // 0x802832cc
    int getTopComKartIndex(RivalBodyCtrl *);       // 0x80283324
    int getMyFowardComKartIndex();                // 0x802833e0
    int getMyBackwardComKartIndex(unsigned char); // 0x802834e0
    void setDriftInfo();                           // 0x802835f4
    void doRotPose(JGeometry::TVec3f, float);      // 0x80283858

    // Vtable
    virtual ~RivalBodyCtrl();                              // 0x8028056c, 14
    virtual void doSterrCtrl();                            // 0x8028251c, 18
    virtual void doDriftCtrl();                            // 0x80282070, 1c
    virtual void doStateFlgCtrl();                         // 0x80282cf4, 20
    virtual void doChangeDriverCtrl();                     // 0x80282aa8, 24
    virtual void doRotPosSterr(const JGeometry::TVec3f &); // 0x80282650, 28
    virtual bool doNextPoint(bool);                        // 0x80280d5c, 2c
    virtual void resetPointData();                         // 0x80282dd4, 30

    static s32 sKartRankArray[24];                       // 0x804138d4
    static s32 sComKartRankArray[8];                     // 0x80413994
    static f32 sKartTotalDist[8];                        // 0x804139b4
    static s32 sPlayerRank[8];                           // 0x804139d4
    static s32 sTopPlayerIndex;                          // 0x8041503c
    static s32 sLowestPlayerKartIndex;                   // 0x80415040
    static s32 sLowestComKartIndex;                      // 0x80415044
    static f32 sCrsStdDistRatio;                         // 0x80415048
    static void *sEngineBParamPtr;                       // 0x80416d20
    static u32 sCommonFlgs;                              // 0x80416d24
    static f32 sCheckTopDistRatio;                       // 0x80416d28

    // void getNthCLPoint(CLPoint *, short);
    // void getVtLengthFCenter();
    // void calcDriftTargetDir();
    // void getPointDir(const CLPoint *, JGeometry::TVec3f *);
    // void getLRKartTargetDir();
    // void getAngleKartZdirTargetDir();
    // void doDriftStageCount();
    // void setDriftStageWaitFrame();
    // void doRotTurnAndChangeState(RivalCtrlBase::RivalMoveState);
    // void isRank1st();
    // void isLowerPlayer(const float &);
    // void isLowestRankOfCom();
    // void getFowardPlayerDist();
    // void getBackwardPlayerDist();
    // void getTopPlayerDist();
    // void findRivalDistanceFrontAngle(float, float);
    // void findRivalDistanceRearAngle(float, float);
    
    static bool isPlayerFirst() { return (sCommonFlgs & 2) != 0; }


    CLPointControllerRivalKart *mPointCtrl;  // 24
    JGeometry::TVec3f _28;                   // 28
    JGeometry::TVec3f _34;                   // 34
    JGeometry::TVec3f _40;                   // 40
    JGeometry::TVec3f _4c;                   // 4c
    JGeometry::TVec3f _58;                   // 58
    JGeometry::TVec3f _64;                   // 64
    JGeometry::TVec3f _70;                   // 70
    f32 _7c;                                 // 7c
    CLPoint * _80;                           // 80
    s32 _84;                                 // 84
    f32 mWidth;                              // 88
    f32 _8c;                                 // 8c
    f32 _90;                                 // 90
    s32 _94;                                 // 94
    s32 _98;                                 // 98
    u8 _9c;                                  // 9c
    u8 _9d;                                  // 9d
    f32 _a0;                                 // a0
    u8 _a4;                                  // a4
    u8 _a5;                                  // a5
    u8 _a6;                                  // a6
    u8 _a7;                                  // a7
    s32 _a8;                                 // a8
    u32 mFlags;                              // ac
    s32 _b0;                                 // b0
    s32 _b4;                                 // b4
    s32 _b8;                                 // b8
    f32 _bc;                                 // bc
    f32 _c0;                                 // c0
    u8 _c4;                                  // c4
    u8 _c5;                                  // c5
    u8 _c6;                                  // c6
    u8 _c7;                                  // c7
}; // Size: 0xc8

class RivalAwardBodyCtrl : public RivalBodyCtrl
{
public:
    RivalAwardBodyCtrl(RivalKart *);
    virtual bool doNextPoint(bool);
    virtual void doRotPosSterr(const JGeometry::TVec3f &);
    virtual ~RivalAwardBodyCtrl() {} // 0x8028416c
    virtual void doDriftCtrl() {}     // 0x80284438
    virtual void doChangeDriverCtrl() {} // 0x8028443c
};

class RivalAward2ndBodyCtrl : public RivalAwardBodyCtrl
{
public:
    RivalAward2ndBodyCtrl(RivalKart *);
    virtual void doSterrCtrl();
    void calcVtTargetPos();
    bool doNextPoint(bool);
    virtual ~RivalAward2ndBodyCtrl() {} // 0x80284440
};

#endif // RIVALBODYCTRL_H
