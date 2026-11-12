#ifndef JPEFFECTPERFORMER_H
#define JPEFFECTPERFORMER_H

#include "JSystem/JParticle/JPAEmitter.h"
#include "JSystem/JSupport/JSUList.h"
#include "JSystem/JGeometry.h"
#include "types.h"

class CrsGround;

class PermissionCounter
{
public:
    PermissionCounter();
    ~PermissionCounter();

    u8 mCount;        // 0x0
    u8 mPermission;   // 0x1
    JSULink<PermissionCounter> mLink; // 0x4
};

class PermissionCounterObserver : public JSUList<PermissionCounter>
{
public:
    PermissionCounterObserver() { initiate(); }
    ~PermissionCounterObserver();

    void init() { initiate(); }
    void reset();
    void calc();
};

class JPEffectPerformer
{
public:
    enum EffectType
    {
        // Original ordering from efctFuncTbl; stray names kept for caller compat.
        Effect_Unknown0 = 0x0,
        Effect_Unknown1 = 0x1,
        Effect_Unknown2 = 0x2,
        Effect_Unknown4 = 0x4,
        Effect_Unknown5 = 0x5,
        Effect_Unknown6 = 0x6,
        Effect_UnknownD = 0xD,
        Effect_UnknownF = 0xF,
        Effect_Unknown12 = 0x12,
        Effect_Burn = 0x13,
        Effect_Star = 0x1c,
        Effect_Cannon = 0x23,
        Effect_Fireball = 0x24,
    };

    static void init();
    static void reset();
    static void loadCourseParam(u32);
    static void calc();
    static void deleteKartEfctKoukasen(int, u8);
    static void deleteKartEfctWet(int);
    static Mtx &getNearestCamPtr(const JGeometry::TVec3f &);
    static void setEffect(EffectType, int, const JGeometry::TVec3f &, u8);
    static void setEffectSplash(CrsGround *, int, const JGeometry::TVec3f &);
    static void setEffectEachCam(EffectType, int, u8, u8);
    static void setLandEffect(const JGeometry::TVec3f &, CrsGround *, int);
    static const char *getLandingEfctName(CrsGround *);

    struct KartEfctEmts
    {
        JPABaseEmitter *star;           // 0x00
        JPABaseEmitter *waterWave;      // 0x04
        JPABaseEmitter *waterRipple;    // 0x08
        JPABaseEmitter *koukasen;       // 0x0C
        JPABaseEmitter *charFire;       // 0x10
        JPABaseEmitter *kinokoDashL[3]; // 0x14
        JPABaseEmitter *kinokoDashR[3]; // 0x20
        JPABaseEmitter *tireSand[2];    // 0x2C
        JPABaseEmitter *wet;            // 0x34
    };

    static KartEfctEmts *getKartEfctEmts(int);
    static void deleteKartEfctTireSand(int);

    JPEffectPerformer() {}
    ~JPEffectPerformer() {}

    static KartEfctEmts sKartEfctEmts[8];       // 0x804119d0
    static PermissionCounter sPCounters[8][22]; // 0x8041299c
    static PermissionCounterObserver sPCounterObserver; // 0x8041375c
    static f32 sKoukasenLength;                // 0x80414d40
    static u8 *sCrs_Params;                    // 0x80416c74
    static u8 *sCrs_ParamNum;                  // 0x80416c78
    static u8 sCharSmokeParamSpecialFlg;       // 0x80416c7c
    static u8 sStartFootSmokeIndex;            // 0x80416c7d
};

u8 TstEmitterModeNotAvailableFlg(int, JPABaseEmitter *);
u8 TstEmitterModeAndClipNotAvailableFlg(int, JPABaseEmitter *);

#endif // JPEFFECTPERFORMER_H
