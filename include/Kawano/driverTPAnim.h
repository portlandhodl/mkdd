#ifndef DRIVER_TPANIM_H
#define DRIVER_TPANIM_H

#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DAnmTexPattern.h"
#include "JSystem/JKernel/JKRDisposer.h"
#include "Kawano/driver.h"
#include "Sato/J3DAnmObject.h"
#include "types.h"

class DriverTPAnim : public JKRDisposer
{
public:
    enum TPAStateEnum
    {
        TPA_WINK = 0,
        TPA_SMILE,
        TPA_SPEAK,
        TPA_PIYO,
        TPA_ADAMAGE,
        TPA_BDAMAGEA,
        TPA_BDAMAGEB,
        TPA_FIRE,
        TPA_1STB,
        TPA_BADA,
        TPA_BADB1,
        TPA_BADB2,
        TPA_GOODA,
        TPA_GOODB,

        TPA_NONE = 19,
    };

    DriverTPAnim(DriverModel *, int);
    virtual ~DriverTPAnim();
    void LoadBTP();
    void setDriverTPAnim(TPAStateEnum);
    void startDriverTPAnim(TPAStateEnum);
    void update();
    void update_TPAWink();
    void update_TPASmile();
    void update_TPASpeak();
    void update_TPAFire();
    void update_TPAPiyo();
    void update_TPAADamage();
    void update_TPABDamageA();
    void update_TPABDamageB();
    void update_basic();
    void setup(DriverModel *);

    static const char *mscpaBTPFileName[14];
    static u32 mProbabilityWink;
    static u32 mMinFrameWink;
    static u32 mFrameSmile;

public:
    J3DAnmTexPattern *mAnmTrans[14]; // 0x18
    J3DAnmObjMatTexPattern mAnm[14]; // 0x50
    DriverModel *mDriver;            // 0x210
    u32 mCounter;                    // 0x214
    volatile int mAnimIdx;           // 0x218
    int mDataID;                     // 0x21C
}; // Size: 0x220

#endif // DRIVER_TPANIM_H
