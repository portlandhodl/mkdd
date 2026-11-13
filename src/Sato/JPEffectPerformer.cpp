#include "Sato/JPEffectPerformer.h"

#include <math.h>
#include <stdio.h>

#include <dolphin/os.h>

#include "JSystem/J3D/J3DModel.h"
#include "JSystem/JUtility/JUTAssert.h"

#include "Kaneshige/KartInfo.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/ResMgr.h"

#include "Sato/ItemObjMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/JPEffectMgr.h"

#include "Yamamoto/kartCamera.h"
#include "Yamamoto/kartCtrl.h"

static const f32 sDummy[4] = {0.0f, 0.0f, 0.0f, 1.0f};

struct stWaterTrack
{
    u8 _00[0x2C];
    JGeometry::TVec3f mPosition; // 0x2C
};

class stEffectMgr
{
public:
    bool tstIsAvailableDistance(int) const;
    MtxPtr getKartExhaustMtxL(int);
    MtxPtr getKartExhaustMtxR(int);
    stWaterTrack *getWaterTrack(int);
    bool tstUseMufllerL(int);
    bool tstUseMufllerR(int);
    void setExhaustStopFlg(int, bool);

    static stEffectMgr *mMgr; // 0x80416b40

    static stEffectMgr *get() { return mMgr; } // helper for friendly access
};

#define GetStEfctMgr() (stEffectMgr::get())

namespace
{
    int sPlayerNum;

    f32 sWaterWaveMinSpd = 15.0f;
    f32 sWaterWaveMaxSpd = 70.0f;
    f32 sWaterWaveHeightMin = -50.0f;
    f32 sWaterWaveHeightMax = 30.0f;

    s16 sStarTraceSpdRatio = 0x50;
    s16 sCharFireSmokeSpdRatio = 0x5D;
    s16 sFallLavaSmokeSpdRatio = 0x64;
    s16 sAccelExhaustSpdRatio = 0x5F;

    struct KartOffsetData
    {
        f32 scale;
        f32 x;
        f32 z;
    };

    // clang-format off
    KartOffsetData sKartOffsetData[cKartIDMax] = {
        { 1.00f,   0.0f,  0.0f },
        { 0.80f,  20.0f, 15.0f },
        { 1.00f,   0.0f, 15.0f },
        { 0.80f, -15.0f,  0.0f },
        { 1.00f,   0.0f, 15.0f },
        { 0.85f, -45.0f,  0.0f },
        { 1.40f,  20.0f, 35.0f },
        { 0.80f,  10.0f,  0.0f },
        { 1.00f,   0.0f,  0.0f },
        { 1.00f,   0.0f, 35.0f },
        { 1.00f,   0.0f, 15.0f },
        { 0.80f, -15.0f,  0.0f },
        { 1.00f,   0.0f, 15.0f },
        { 0.85f, -45.0f,  0.0f },
        { 1.15f,   0.0f,  0.0f },
        { 0.95f, -45.0f, 20.0f },
        { 0.90f, -10.0f,  0.0f },
        { 0.90f, -10.0f,  0.0f },
        { 1.20f,   0.0f,  0.0f },
        { 1.20f,   0.0f,  0.0f },
        { 1.40f,  10.0f,  0.0f },
    };

    f32 sCharStartSmokeOffsetData[cKartIDMax][2] = {
        { 0.0f,  0.0f },
        { 0.6f, 10.0f },
        { 0.6f, 10.0f },
        { 0.6f, 10.0f },
        { 0.6f, 10.0f },
        { 0.6f, 10.0f },
        { 0.6f, 10.0f },
        { 0.8f, 20.0f },
        { 0.8f, 20.0f },
        { 1.0f, 20.0f },
        { 1.0f, 20.0f },
        { 0.8f, 20.0f },
        { 0.8f, 20.0f },
        { 1.0f, 20.0f },
        { 1.0f, 20.0f },
        { 1.0f, 20.0f },
        { 0.6f, 10.0f },
        { 0.6f, 10.0f },
        { 0.6f, 10.0f },
        { 0.0f,  0.0f },
        { 0.8f, 20.0f },
    };
    // clang-format on
} // namespace

class JPTraceEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPRotateOffsetEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPRotateXZEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPRotateEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);

    virtual int getKartIndex(JPABaseEmitter *emt) { return emt->mpUserWork; }

    void calc_common(JPABaseEmitter *);
};

class JPBonusKartEfctEmitterCallBack : public JPRotateEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
};

class JPKinokoDashEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPKinokoDash2EmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPPtclTraceEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
    int execute_common(JPABaseEmitter *);
};

class JPStarEmitterCallBack : public JPPtclTraceEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
};

class JPTireTraceEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *emt) { execute_common(emt); }
    virtual void draw(JPABaseEmitter *);
    void execute_common(JPABaseEmitter *);
};

class JPWheelSpinEmitterCallBack : public JPTireTraceEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPWaterWaveEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPWaterRippleEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPKoukasenEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);

    u8 mCounter;
};

class JPCharFireEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPTireSandEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPKartWetEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPTraceParticleCallBack : public JPAParticleCallBack
{
public:
    virtual void execute(JPABaseEmitter *, JPABaseParticle *);
    virtual void draw(JPABaseEmitter *, JPABaseParticle *);
};

class JPStartFootSmokeEmitterCallBack : public JPAEmitterCallBack
{
public:
    virtual void execute(JPABaseEmitter *);
    virtual void draw(JPABaseEmitter *);
};

class JPRotateFollowEmitterCallBack : public JPRotateEmitterCallBack
{
public:
    virtual int getKartIndex(JPABaseEmitter *emt) { return emt->mpUserWork & 0xF; }
};

static JPTraceEmitterCallBack JPTraceECB;
static JPRotateOffsetEmitterCallBack JPRotateOffsetECB;
static JPRotateXZEmitterCallBack JPRotateXZECB;
static JPRotateEmitterCallBack JPRotateECB;
static JPBonusKartEfctEmitterCallBack JPBonusKartECB;
static JPRotateFollowEmitterCallBack JPRotateFollowECB;
static JPKinokoDashEmitterCallBack JPKinokoDashECB;
static JPKinokoDash2EmitterCallBack JPKinokoDashECB2;
static JPPtclTraceEmitterCallBack JPPtclTraceECB;
static JPStarEmitterCallBack JPStarECB;
static JPTireTraceEmitterCallBack JPTireTraceECB;
static JPWheelSpinEmitterCallBack JPWheelSpinECB;
static JPWaterWaveEmitterCallBack JPWaterWaveECB;
static JPWaterRippleEmitterCallBack JPWaterRippleECB;
static JPKoukasenEmitterCallBack *JPKoukasenECB;
static JPCharFireEmitterCallBack JPCharFireECB;
static JPTireSandEmitterCallBack JPTireSandECB;
static JPKartWetEmitterCallBack JPKartWetECB;
static JPTraceParticleCallBack JPTracePCB;
static JPStartFootSmokeEmitterCallBack JPStartSmokeECB;

u8 TstEmitterModeNotAvailableFlg(int kartNo, JPABaseEmitter *emt)
{
    bool unavailableFlg = false;

    if (RCMGetManager()->isLANMode())
        return false;

    u32 resFlags = emt->mResource->getDyn()->getResUserWork();

    if ((resFlags & 0x2) && sPlayerNum > 1)
        unavailableFlg = true;

    if ((resFlags & (1 << (sPlayerNum + 3))) && !GetKartCtrl()->CheckCamera(kartNo))
        unavailableFlg = true;

    if (unavailableFlg)
    {
        GetJPAMgr()->deleteEmt(emt);
    }

    return unavailableFlg;
}

u8 TstEmitterClipNotAvailableFlg(int kartNo, JPABaseEmitter *emt)
{
    bool unavailableFlg = false;
    if (emt->mResource->getDyn()->getResUserWork() & 1)
    {
        if (!GetStEfctMgr()->tstIsAvailableDistance(kartNo))
        {
            GetJPAMgr()->deleteEmt(emt);
            unavailableFlg = true;
        }
    }
    return unavailableFlg;
}

u8 TstEmitterModeAndClipNotAvailableFlg(int kartNo, JPABaseEmitter *emt)
{
    u8 unavailableFlg = TstEmitterModeNotAvailableFlg(kartNo, emt);
    if (!unavailableFlg)
    {
        unavailableFlg = false;
        if (emt->mResource->getDyn()->getResUserWork() & 1)
        {
            if (!GetStEfctMgr()->tstIsAvailableDistance(kartNo))
            {
                GetJPAMgr()->deleteEmt(emt);
                unavailableFlg = true;
            }
        }
        unavailableFlg = (u8)unavailableFlg;
    }
    return unavailableFlg;
}

void setKartThunderScale(int kartNo, JPABaseEmitter *emt)
{
    emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
}

u8 TstEmitterStopKartState(int kartNo, JPABaseEmitter *emt)
{
    if (!(GetKartCtrl()->GetKartStatus(kartNo) & 0x800))
    {
        emt->stopCreateParticle();
        emt->mMaxFrame = 1;
        return true;
    }
    return false;
}

void setEmitterKartOffsetData(int kartNo, JPABaseEmitter *emt)
{
    u32 kartID = RCMGetManager()->getKartInfo(kartNo)->getKartID();
#line 193
    JUT_MINMAX_ASSERT(0, kartID, 21);
#line 1

    KartOffsetData *data = &sKartOffsetData[kartID];
    JGeometry::TVec3f dir(0.0f, data->x, data->z);
    JGeometry::TVec3f out;
    JGeometry::TVec3f sclVec;
    sclVec.setAll(data->scale);
    sclVec.scale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());

    PSMTXMultVec(ObjUtility::getKartBodyMtx(kartNo), &dir, &out);

    emt->setGlobalScale(sclVec);
    emt->setGlobalTranslation(out);
}

void JPTraceEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = emt->mpUserWork;
    u8 unavailableFlg = TstEmitterClipNotAvailableFlg(kartNo, emt);

    if (!unavailableFlg)
    {
        const Mtx &mtx = ObjUtility::getKartBodyMtx(kartNo);
        f32 ty = mtx[1][3];
        f32 tz = mtx[2][3];
        f32 tx = mtx[0][3];
        emt->setGlobalTranslation(tx, ty, tz);
        emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
    }
}

void JPTraceEmitterCallBack::draw(JPABaseEmitter *) {}



void JPRotateOffsetEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = emt->mpUserWork;
    bool unavailableFlg = false;

    if (emt->mResource->getDyn()->getResUserWork() & 1)
    {
        if (!GetStEfctMgr()->tstIsAvailableDistance(kartNo))
        {
            GetJPAMgr()->deleteEmt(emt);
            unavailableFlg = true;
        }
    }

    if (!unavailableFlg)
    {
        u32 kartID = RCMGetManager()->getKartInfo(kartNo)->getKartID();
#line 193
        JUT_MINMAX_ASSERT(0, kartID, 21);
#line 1

        JGeometry::TVec3f sclVec;
        JGeometry::TVec3f out;
        KartOffsetData *data = &sKartOffsetData[kartID];
        JGeometry::TVec3f dir(0.0f, data->x, data->z);
        sclVec.setAll(data->scale);
        sclVec.scale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());

        PSMTXMultVec(ObjUtility::getKartBodyMtx(kartNo), &dir, &out);

        emt->setGlobalScale(sclVec);
        emt->setGlobalTranslation(out);
        emt->setGlobalRMatrix(ObjUtility::getKartBodyMtx(kartNo));
    }
}

void JPRotateOffsetEmitterCallBack::draw(JPABaseEmitter *) {}

void JPRotateXZEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = emt->mpUserWork;
    u8 unavailableFlg = TstEmitterClipNotAvailableFlg(kartNo, emt);

    if (!unavailableFlg)
    {
        JGeometry::TPos3f dirMtx;
        JGeometry::TVec3f zdir;
        JGeometry::TVec3f pos;
        setEmitterKartOffsetData(kartNo, emt);

        ObjUtility::getKartZdir(kartNo, &zdir);
        zdir.y = 0.0f;
        ObjUtility::getKartPos(kartNo, &pos);
        stMakeDirectionMtx(&dirMtx, zdir, 'Z');
        emt->setGlobalRMatrix(dirMtx);
        emt->setGlobalTranslation(pos);
    }
}

void JPRotateXZEmitterCallBack::draw(JPABaseEmitter *) {}

void JPRotateEmitterCallBack::execute(JPABaseEmitter *emt)
{
    calc_common(emt);
}

void JPRotateEmitterCallBack::calc_common(JPABaseEmitter *emt)
{
    int kartNo = getKartIndex(emt);
    u8 unavailableFlg = TstEmitterClipNotAvailableFlg(kartNo, emt);

    if (!unavailableFlg)
    {
        const Mtx &mtx = ObjUtility::getKartBodyMtx(kartNo);
        JPASetRMtxSTVecfromMtx(mtx, emt->mGlobalRot, &emt->mGlobalScl, &emt->mGlobalTrs);
        emt->setGlobalParticleScale(emt->mGlobalScl);
    }
}

void JPRotateEmitterCallBack::draw(JPABaseEmitter *) {}

static inline void setCreateParticleByStatus(JPABaseEmitter *emt, int kartNo)
{
    if (GetKartCtrl()->GetGameStatus(kartNo) & 0x2000)
        emt->playCreateParticle();
    else
        emt->stopCreateParticle();
}

void JPBonusKartEfctEmitterCallBack::execute(JPABaseEmitter *emt)
{
    calc_common(emt);
    setCreateParticleByStatus(emt, getKartIndex(emt));
}

void JPKinokoDashEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = (emt->mpUserWork >> 4) & 0xF;

    if (!TstEmitterClipNotAvailableFlg(kartNo, emt))
    {
        MtxPtr exhaustMtx;
        if ((emt->mpUserWork & 0xF) == 0)
            exhaustMtx = GetStEfctMgr()->getKartExhaustMtxL(kartNo);
        else
            exhaustMtx = GetStEfctMgr()->getKartExhaustMtxR(kartNo);

        f32 x0 = -exhaustMtx[0][0];
        f32 x1 = -exhaustMtx[1][0];
        f32 x2 = -exhaustMtx[2][0];
        f32 y0 = exhaustMtx[0][1];
        f32 y1 = exhaustMtx[1][1];
        f32 y2 = exhaustMtx[2][1];
        f32 z0 = -exhaustMtx[0][2];
        f32 z1 = -exhaustMtx[1][2];
        f32 z2 = -exhaustMtx[2][2];

        Mtx negMtx;
        PSMTXCopy(exhaustMtx, negMtx);
        negMtx[0][0] = x0;
        negMtx[1][0] = x1;
        negMtx[2][0] = x2;
        negMtx[0][1] = y0;
        negMtx[1][1] = y1;
        negMtx[2][1] = y2;
        negMtx[0][2] = z0;
        negMtx[1][2] = z1;
        negMtx[2][2] = z2;
        JGeometry::TVec3f trans(exhaustMtx[0][3], exhaustMtx[1][3], exhaustMtx[2][3]);
        negMtx[0][3] = trans.x;
        negMtx[1][3] = trans.y;
        negMtx[2][3] = trans.z;

        JPASetRMtxTVecfromMtx(negMtx, emt->mGlobalRot, &emt->mGlobalTrs);
        emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
    }
}

void JPKinokoDashEmitterCallBack::draw(JPABaseEmitter *) {}

void JPKinokoDash2EmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = (u32)emt->mpUserWork >> 8;
    u32 side = emt->mpUserWork & 0xF;

    if (emt->isEnableDeleteEmitter())
    {
        GetJPAMgr()->deleteEmt(emt);
        u8 idx = (emt->mpUserWork >> 4) & 0xF;
        if (side == 0)
        {
            JPABaseEmitter **p = JPEffectPerformer::sKartEfctEmts[kartNo].kinokoDashL;
            p[idx] = NULL;
        }
        else
        {
            JPABaseEmitter **p = JPEffectPerformer::sKartEfctEmts[kartNo].kinokoDashR;
            p[idx] = NULL;
        }
    }
    else
    {
        MtxPtr exhaustMtx;
        if (side == 0)
            exhaustMtx = GetStEfctMgr()->getKartExhaustMtxL(kartNo);
        else
            exhaustMtx = GetStEfctMgr()->getKartExhaustMtxR(kartNo);

        f32 x0 = -exhaustMtx[0][0];
        f32 x1 = -exhaustMtx[1][0];
        f32 x2 = -exhaustMtx[2][0];
        f32 y0 = exhaustMtx[0][1];
        f32 y1 = exhaustMtx[1][1];
        f32 y2 = exhaustMtx[2][1];
        f32 z0 = -exhaustMtx[0][2];
        f32 z1 = -exhaustMtx[1][2];
        f32 z2 = -exhaustMtx[2][2];

        Mtx negMtx;
        PSMTXCopy(exhaustMtx, negMtx);
        negMtx[0][0] = x0;
        negMtx[1][0] = x1;
        negMtx[2][0] = x2;
        negMtx[0][1] = y0;
        negMtx[1][1] = y1;
        negMtx[2][1] = y2;
        negMtx[0][2] = z0;
        negMtx[1][2] = z1;
        negMtx[2][2] = z2;
        JGeometry::TVec3f trans(exhaustMtx[0][3], exhaustMtx[1][3], exhaustMtx[2][3]);
        negMtx[0][3] = trans.x;
        negMtx[1][3] = trans.y;
        negMtx[2][3] = trans.z;

        JPASetRMtxTVecfromMtx(negMtx, emt->mGlobalRot, &emt->mGlobalTrs);
        emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
    }
}

void JPKinokoDash2EmitterCallBack::draw(JPABaseEmitter *) {}

void JPPtclTraceEmitterCallBack::execute(JPABaseEmitter *emt)
{
    execute_common(emt);
}

int JPPtclTraceEmitterCallBack::execute_common(JPABaseEmitter *emt)
{
    u8 kartNou = (u32)emt->mpUserWork >> 4;
    int kartNo = kartNou & 0xF;

    const Mtx &kartMtx = ObjUtility::getKartBodyMtx(kartNo);
    f32 ty = kartMtx[1][3];
    f32 tz = kartMtx[2][3];
    f32 tx = kartMtx[0][3];
    emt->setGlobalTranslation(tx, ty, tz);
    emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());

    if (GetKartCtrl()->GetGameStatus(kartNo) & 0x2000)
        emt->playCreateParticle();
    else
        emt->stopCreateParticle();

    return kartNo;
}

void JPPtclTraceEmitterCallBack::draw(JPABaseEmitter *) {}

void JPStarEmitterCallBack::execute(JPABaseEmitter *emt)
{
    s32 kartNo = execute_common(emt);

    if (!(GetKartCtrl()->GetKartStatus(kartNo) & 0x40000))
    {
        emt->stopCreateParticle();
        emt->mMaxFrame = 1;
        JPEffectPerformer::sKartEfctEmts[kartNo].star = NULL;
    }
}

void JPTireTraceEmitterCallBack::execute_common(JPABaseEmitter *emt)
{
    Vec tirePos;
    int kartNo = (u32)emt->mpUserWork >> 4;

    if ((emt->mpUserWork & 0xF) == 0)
        GetKartCtrl()->GetLeftTirePos(kartNo, &tirePos);
    else
        GetKartCtrl()->GetRightTirePos(kartNo, &tirePos);

    emt->setGlobalRMatrix(ObjUtility::getKartBodyMtx(kartNo));
    emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
    emt->mGlobalTrs.x = tirePos.x;
    emt->mGlobalTrs.y = tirePos.y;
    emt->mGlobalTrs.z = tirePos.z;
}

void JPTireTraceEmitterCallBack::draw(JPABaseEmitter *) {}

void JPWheelSpinEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = (u32)emt->mpUserWork >> 4;
    TstEmitterStopKartState(kartNo, emt);
    execute_common(emt);
}

void JPWheelSpinEmitterCallBack::draw(JPABaseEmitter *) {}

void JPWaterWaveEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = emt->mpUserWork;
    u8 unavailableFlg = TstEmitterClipNotAvailableFlg(kartNo, emt);

    stWaterTrack *track = GetStEfctMgr()->getWaterTrack(kartNo);
    if (track != NULL)
    {
        if (!(GetKartCtrl()->GetKartStatus(kartNo) & 3) && GetKartCtrl()->GetWaterHeight(kartNo) > sWaterWaveHeightMin &&
            GetKartCtrl()->GetWaterHeight(kartNo) < sWaterWaveHeightMax && !unavailableFlg)
        {
            f32 spd = JMAAbs(GetKartCtrl()->GetCarSpeed(kartNo));
            if (spd < sWaterWaveMinSpd)
            {
                emt->stopCreateParticle();
            }
            else
            {
                f32 v = (spd > sWaterWaveMaxSpd) ? sWaterWaveMaxSpd : spd;
                f32 e = exp(-(3.0f * (v - sWaterWaveMinSpd) / 55.0f));
                const Mtx &kartMtx = ObjUtility::getKartBodyMtx(kartNo);
                f32 pscale = (1.0f - e) * GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale();
                emt->mGlobalPScl.x = pscale;
                emt->mGlobalPScl.y = pscale;
                emt->playCreateParticle();
                emt->setGlobalTranslation(track->mPosition);
                emt->setGlobalRMatrix(kartMtx);
            }
        }
        else
        {
            emt->stopCreateParticle();
        }
    }
    else
    {
        emt->stopCreateParticle();
        emt->mMaxFrame = 1;
        JPEffectPerformer::KartEfctEmts *ef = JPEffectPerformer::sKartEfctEmts + kartNo;
        ef->waterWave = NULL;
    }
}

void JPWaterWaveEmitterCallBack::draw(JPABaseEmitter *) {}

void JPWaterRippleEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = emt->mpUserWork;

    f32 waterHeight = GetKartCtrl()->GetBodyGround(kartNo)->getWaterHeight();

    JGeometry::TVec3f kartPos;
    ObjUtility::getKartPos(kartNo, &kartPos);

    f32 kartY = kartPos.y;

    if (GetKartCtrl()->GetBodyGround(kartNo)->getAttribute() == CrsGround::Attr_Water &&
        waterHeight > kartY - 20.0f && waterHeight < kartY + 100.0f)
    {
        if (JMAAbs(GetKartCtrl()->GetCarSpeed(kartNo)) > 20.0f)
        {
            emt->stopCreateParticle();
        }
        else
        {
            emt->playCreateParticle();
            kartPos.y = GetKartCtrl()->GetBodyGround(kartNo)->getWaterHeight();
            emt->mGlobalTrs.x = kartPos.x;
            emt->mGlobalTrs.y = kartPos.y;
            emt->mGlobalTrs.z = kartPos.z;
            emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
        }
    }
    else
    {
        JPEffectPerformer::KartEfctEmts *emts = &JPEffectPerformer::sKartEfctEmts[kartNo];
        emts->waterRipple = NULL;
        emt->stopCreateParticle();
        emt->mMaxFrame = 1;
    }
}

void JPWaterRippleEmitterCallBack::draw(JPABaseEmitter *) {}

void JPKoukasenEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int slot = ((u32)emt->mpUserWork >> 4) & 0xF;
    int sideIdx = emt->mpUserWork & 0xF;
    int kartNo = (u32)emt->mpUserWork >> 8;

    const float (*camMtx)[4] = RCMGetCamera(slot)->GetMtx();

    JGeometry::TVec3f dir;
    dir.x = -camMtx[2][0];
    dir.y = -camMtx[2][1];
    dir.z = -camMtx[2][2];
    dir.scale(JPEffectPerformer::sKoukasenLength);

    JGeometry::TPos3f mat;
    mat[0][0] = -camMtx[0][0];
    mat[1][0] = -camMtx[0][1];
    mat[2][0] = -camMtx[0][2];
    mat[0][1] = camMtx[1][0];
    mat[1][1] = camMtx[1][1];
    mat[2][1] = camMtx[1][2];
    mat[0][2] = -camMtx[2][0];
    mat[1][2] = -camMtx[2][1];
    mat[2][2] = -camMtx[2][2];

    JGeometry::TVec3f pos = *RCMGetCamera(slot)->GetCameraPos();
    pos += dir;
    mat[0][3] = pos.x;
    mat[1][3] = pos.y;
    mat[2][3] = pos.z;

    JPASetRMtxTVecfromMtx(mat, emt->mGlobalRot, &emt->mGlobalTrs);

    if ((sideIdx == 0 && !(GetKartCtrl()->GetKartStatus(kartNo) & 0x8000)) ||
        (sideIdx == 1 && !(GetKartCtrl()->GetKartStatus(kartNo) & 0x100000000)))
    {
        emt->setGlobalAlpha((10 - mCounter) * 255 / 10);
        mCounter++;
    }

    if (mCounter >= 10)
    {
        GetJPAMgr()->deleteEmtCam(emt, slot);
        JPEffectPerformer::sKartEfctEmts[(u8)kartNo].koukasen = NULL;
    }
}

void JPKoukasenEmitterCallBack::draw(JPABaseEmitter *) {}

void JPCharFireEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = ((u32)emt->mpUserWork >> 4) & 0xF;

    if (TstEmitterClipNotAvailableFlg(kartNo, emt))
    {
        JPEffectPerformer::KartEfctEmts *emts = &JPEffectPerformer::sKartEfctEmts[kartNo];
        emts->charFire = NULL;
        return;
    }

    DriverModel *driver = RCMGetManager()->getKartLoader(kartNo)->getExModelDriver(ItemObjMgr::getNowTandemDriverNum(kartNo));
    int jointIdx = driver->getModelData()->getJointName()->getIndex("skl_root");
    MtxPtr psm = driver->getModel()->getAnmMtx(jointIdx);
    f32 x = psm[0][3];
    f32 y = psm[1][3];
    f32 z = psm[2][3];

    JPASetRMtxfromMtx(driver->getBaseTRMtx(), emt->mGlobalRot);
    emt->mGlobalTrs.x = x;
    emt->mGlobalTrs.y = y;
    emt->mGlobalTrs.z = z;
    setKartThunderScale(kartNo, emt);

    if (GetKartCtrl()->GetGameStatus(kartNo) & 0x2000)
        emt->playCreateParticle();
    else
        emt->stopCreateParticle();

    if (!GetKartCtrl()->IsBurn(kartNo))
    {
        JPABaseEmitter **charFire = &JPEffectPerformer::sKartEfctEmts[kartNo].charFire;
        if (*charFire != NULL)
        {
            emt->stopCreateParticle();
            emt->mMaxFrame = 1;
            *charFire = NULL;
        }
    }
}

void JPCharFireEmitterCallBack::draw(JPABaseEmitter *) {}

void JPTireSandEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = (u32)emt->mpUserWork >> 4;
    Vec tirePos;

    if ((emt->mpUserWork & 0xF) == 0)
        GetKartCtrl()->GetLeftTirePos(kartNo, &tirePos);
    else
        GetKartCtrl()->GetRightTirePos(kartNo, &tirePos);

    emt->setGlobalRMatrix(ObjUtility::getKartBodyMtx(kartNo));
    RCMGetManager()->getKartInfo(kartNo);

    emt->mGlobalTrs.x = tirePos.x;
    emt->mGlobalTrs.y = tirePos.y;
    emt->mGlobalTrs.z = tirePos.z;
    emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
}

void JPTireSandEmitterCallBack::draw(JPABaseEmitter *) {}

void JPKartWetEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = emt->mpUserWork;
    u8 unavailableFlg = TstEmitterClipNotAvailableFlg(kartNo, emt);

    if (!unavailableFlg)
    {
        const Mtx &kartMtx = ObjUtility::getKartBodyMtx(kartNo);
        f32 yoff = 0.0f;

        switch (RCMGetManager()->getKartInfo(kartNo)->getKartID())
        {
        case GOO_GOO_BUGGY:
        case RATTLE_BUGGY:
            yoff = 20.0f;
            break;
        case TURBO_YOSHI:
        case TURBO_BIRDO:
            yoff = -15.0f;
            break;
        case RED_FIRE:
        case GREEN_FIRE:
        case WALUIGI_RACER:
            yoff = -10.0f;
            break;
        case PARADE_KART:
            yoff = -20.0f;
            break;
        }

        JGeometry::TVec3f kartPos;
        ObjUtility::getKartPos(kartNo, &kartPos);
        JGeometry::TVec3f kartYdir;
        ObjUtility::getKartYdir(kartNo, &kartYdir);
        kartYdir.scale(yoff);
        kartPos += kartYdir;

        emt->mGlobalTrs.x = kartPos.x;
        emt->mGlobalTrs.y = kartPos.y;
        emt->mGlobalTrs.z = kartPos.z;
        emt->setGlobalRMatrix(kartMtx);
    }
}

void JPKartWetEmitterCallBack::draw(JPABaseEmitter *) {}

void JPTraceParticleCallBack::execute(JPABaseEmitter *emt, JPABaseParticle *ptcl)
{
    if (ptcl->mAge > 0)
    {
        u32 uw = (u32)emt->mpUserWork;
        f32 ratio = ((f32)(uw >> 8) * 0.01f);
        JGeometry::TVec3f vel;
        ObjUtility::getKartEfctVel(((uw >> 4) & 0xF), &vel);

        JGeometry::TVec3f offset;
        ptcl->getOffsetPosition(offset);
        vel.scale(ratio);
        offset.add(vel);
        ptcl->setOffsetPosition(offset);
    }
}

void JPTraceParticleCallBack::draw(JPABaseEmitter *, JPABaseParticle *) {}

void JPStartFootSmokeEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = emt->mpUserWork;
    KartGamePad *coPad = GetKartCtrl()->GetCoDriveCont(kartNo);
    KartPad *pad = GetKartCtrl()->getKartPad(kartNo);
    u32 coBits = *(u32 *)((u8 *)coPad + 0x18);
    if (pad->mAccelBtn & coBits)
        emt->playCreateParticle();
    else
        emt->stopCreateParticle();

    if (GetKartCtrl()->GetKartStatus(kartNo) & 0x02000000)
    {
        emt->stopCreateParticle();
        emt->mMaxFrame = 1;
    }
}

void JPStartFootSmokeEmitterCallBack::draw(JPABaseEmitter *) {}

void PermissionCounterObserver::reset()
{
    JSUPtrLink *link = getFirstLink();
    while (link != NULL)
    {
        remove(&((PermissionCounter *)link->getObjectPtr())->mLink);
        link = getFirstLink();
    }
    initiate();
}

void PermissionCounterObserver::calc()
{
    JSUPtrLink *link = getFirstLink();
    while (link != NULL)
    {
        static_cast<PermissionCounter *>(link->getObjectPtr())->mCount++;
        if (static_cast<PermissionCounter *>(link->getObjectPtr())->mCount >
            static_cast<PermissionCounter *>(link->getObjectPtr())->mPermission)
        {
            static_cast<PermissionCounter *>(link->getObjectPtr())->mCount = 0;
            remove(&static_cast<PermissionCounter *>(link->getObjectPtr())->mLink);
            if (link == NULL)
                break;
        }
        link = link->getNext();
    }
}

static void calcEfct_KartOverTurn(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emts[3] = {NULL, NULL, NULL};
    MtxPtr camMtx = JPEffectPerformer::getNearestCamPtr(pos);

    emts[0] = GetJPAMgr()->createEmt("mk_collKart_a", pos, camMtx);
    if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emts[0]))
        return;

    emts[1] = GetJPAMgr()->createEmt("mk_collKart_b", pos, camMtx);
    emts[2] = GetJPAMgr()->createEmt("mk_collKart_c", pos, camMtx);

    JPABaseEmitter **p = emts;
    for (u8 i = 0; i < 3; i++, p++)
    {
        if (*p != NULL)
        {
            (*p)->mEmitterCallback = &JPTraceECB;
            (*p)->mpUserWork = kartNo;
        }
    }
}

static inline void calcEfct_KartSpinBase(int kartNo, const JGeometry::TVec3f &pos, const char *name)
{
    JPABaseEmitter *emts[3] = {NULL, NULL, NULL};
    MtxPtr camMtx = JPEffectPerformer::getNearestCamPtr(pos);

    emts[0] = GetJPAMgr()->createEmt("mk_collKart_a", pos, camMtx);
    if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emts[0]))
        return;

    emts[1] = GetJPAMgr()->createEmt("mk_collKart_b", pos, camMtx);
    emts[2] = GetJPAMgr()->createEmt(name, pos, camMtx);

    JPABaseEmitter **p = emts;
    for (u8 i = 0; i < 3; i++, p++)
    {
        if (*p != NULL)
        {
            (*p)->mEmitterCallback = &JPRotateOffsetECB;
            (*p)->mpUserWork = kartNo;
        }
    }
}

static void calcEfct_KartSpin(int kartNo, const JGeometry::TVec3f &pos, u8 dir)
{
    const char *name = "mk_kartSpinL";
    if (dir != 0)
        name = "mk_kartSpinR";
    calcEfct_KartSpinBase(kartNo, pos, name);
}

static void calcEfct_KartHalfSpin(int kartNo, const JGeometry::TVec3f &pos, u8 dir)
{
    const char *name = "mk_kartSpinL_half";
    if (dir != 0)
        name = "mk_kartSpinR_half";
    calcEfct_KartSpinBase(kartNo, pos, name);
}

static void calcEfct_KartLanding(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPEffectPerformer::setLandEffect(pos, GetKartCtrl()->GetBodyGround(kartNo), kartNo);
}

static void calcEfct_ColKartAndKart(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emts[2] = {NULL, NULL};

    emts[0] = GetJPAMgr()->createEmt("mk_collKart_a", pos);
    if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emts[0]))
        return;

    emts[1] = GetJPAMgr()->createEmt("mk_collKart_c", pos);

    JPABaseEmitter **p = emts;
    for (u8 i = 0; i < 2; i++, p++)
    {
        if (*p != NULL)
        {
            (*p)->mEmitterCallback = &JPTraceECB;
            (*p)->mpUserWork = kartNo;
        }
    }
}

static void calcEfct_ColKartAndObject(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_collKart_a", pos);
    if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emt))
        return;

    GetJPAMgr()->createEmt("mk_collKart_c", pos);
}

static void calcEfct_KartWaterSplashSmall(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *names[3] = {"mk_landingWtr_a", "mk_landingWtr_b", "mk_landingWtr_c"};

    TstEmitterClipNotAvailableFlg(kartNo, GetJPAMgr()->createEmt(names[0], pos));
    GetJPAMgr()->createEmt(names[1], pos);
    TstEmitterModeAndClipNotAvailableFlg(kartNo, GetJPAMgr()->createEmt(names[2], pos));
}

static void calcEfct_KartWaterMudSplashSmall(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *names[3] = {"mk_landingWtr_wa", "mk_landingWtr_wb", "mk_landingWtr_wc"};

    TstEmitterClipNotAvailableFlg(kartNo, GetJPAMgr()->createEmt(names[0], pos));
    GetJPAMgr()->createEmt(names[1], pos);
    TstEmitterModeAndClipNotAvailableFlg(kartNo, GetJPAMgr()->createEmt(names[2], pos));
}

static void calcEfct_KartWaterDrainSplashSmall(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *names[3] = {"mk_landingWtr_sa", "mk_landingWtr_sb", "mk_landingWtr_sc"};

    TstEmitterClipNotAvailableFlg(kartNo, GetJPAMgr()->createEmt(names[0], pos));
    GetJPAMgr()->createEmt(names[1], pos);
    TstEmitterModeAndClipNotAvailableFlg(kartNo, GetJPAMgr()->createEmt(names[2], pos));
}

static void calcEfct_KartWaterMudDiddySplashSmall(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *names[3] = {"mk_landingWtr_ya", "mk_landingWtr_yb", "mk_landingWtr_yc"};

    TstEmitterClipNotAvailableFlg(kartNo, GetJPAMgr()->createEmt(names[0], pos));
    GetJPAMgr()->createEmt(names[1], pos);
    TstEmitterModeAndClipNotAvailableFlg(kartNo, GetJPAMgr()->createEmt(names[2], pos));
}

static void calcEfct_KartFallLava(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *names[3] = {"mk_yougan_a", "mk_yougan_b", "mk_yougan_c"};
    JPABaseEmitter *emt;

    for (u8 i = 0; i < 3; i++)
    {
        emt = GetJPAMgr()->createEmt(names[i], pos);
        if (emt != NULL)
            TstEmitterModeNotAvailableFlg(kartNo, emt);
    }
}
static inline void calcEfct_KartAccelExhaustBase(int kartNo, const JGeometry::TVec3f &pos, u8 side)
{
    const char *names[2] = {"mk_acceleOn_a", "mk_acceleOn_b"};

    for (u8 i = 0; i < 2; i++)
    {
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(names[i], pos);
        if (emt != NULL)
        {
            u32 userWork = (sAccelExhaustSpdRatio << 8) | (kartNo << 4) | side;
            emt->mEmitterCallback = &JPKinokoDashECB;
            emt->mpUserWork = userWork;
            if (i == 0)
                emt->mParticleCallback = &JPTracePCB;
        }
    }
}

static void calcEfct_KartAccelExhaustL(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    calcEfct_KartAccelExhaustBase(kartNo, pos, 0);
}

static void calcEfct_KartAccelExhaustR(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    calcEfct_KartAccelExhaustBase(kartNo, pos, 1);
}
static void calcEfct_KartSlide(int kartNo, const JGeometry::TVec3f &pos, u8 dir)
{
    const char *name = !dir ? "mk_slideAt_l" : "mk_slideAt_r";

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPRotateECB;
        emt->mpUserWork = kartNo;
    }
}

static void calcEfct_KartStar(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    if (JPEffectPerformer::getKartEfctEmts(kartNo)->star != NULL)
        GetJPAMgr()->deleteEmt(JPEffectPerformer::getKartEfctEmts(kartNo)->star);

    const char *name = "mk_star_a";
    if (RCMGetManager()->getConsoleNumber() >= 2)
        name = "mk_star_b";

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (emt != NULL)
    {
        JPEffectPerformer::getKartEfctEmts(kartNo)->star = emt;
        emt->mEmitterCallback = &JPStarECB;
        emt->mParticleCallback = &JPTracePCB;
        emt->mpUserWork = (sStarTraceSpdRatio << 8) | (kartNo << 4);
    }
}

static void calcEfct_KartKinokoDash1(int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    u32 userWork;
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_kinokoDash_a", pos);
    userWork = (kartNo << 4) | flag;
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPKinokoDashECB;
        emt->mpUserWork = userWork;
    }
}

static void calcEfct_KartKinokoDash2(int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    u32 userWork;
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_kinokoDash_b", pos);
    userWork = (kartNo << 4) | flag;
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPKinokoDashECB;
        emt->mpUserWork = userWork;
    }
}

void calcEfct_KartKinokoDash3Base(int kartNo, const JGeometry::TVec3f &pos, u8 sideIdx, char **names)
{
    JPABaseEmitter **emts;
    if (sideIdx == 0)
        emts = JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashL;
    else
        emts = JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashR;

    for (u8 i = 0; i < 3; i++)
    {
        if (emts[i] != NULL)
        {
            GetJPAMgr()->deleteEmt(emts[i]);
            if (sideIdx == 0)
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashL[i] = NULL;
            else
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashR[i] = NULL;
        }

        JPABaseEmitter *emt = GetJPAMgr()->createEmt(names[i], pos);
        u32 userWork = (kartNo << 8) | ((u32)i << 4) | sideIdx;
        if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emt))
            continue;

        if (emt != NULL)
        {
            if (sideIdx == 0)
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashL[i] = emt;
            else
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashR[i] = emt;
            emt->mEmitterCallback = &JPKinokoDashECB2;
            emt->mFlags |= 0x40;
            emt->mpUserWork = userWork;
        }
    }
}

static void calcEfct_KartKinokoDash3(int kartNo, const JGeometry::TVec3f &pos, u8 sideIdx)
{
    const char *names[3] = {"mk_kinokoDash_e", "mk_kinokoDash_f", "mk_kinokoDash_g"};
    if (RCMGetManager()->getConsoleNumber() >= 2)
        names[0] = "mk_kinokoDash_el";

    JPABaseEmitter **emts;
    if (sideIdx == 0)
        emts = JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashL;
    else
        emts = JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashR;

    for (u8 i = 0; i < 3; i++)
    {
        if (emts[i] != NULL)
        {
            GetJPAMgr()->deleteEmt(emts[i]);
            if (sideIdx == 0)
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashL[i] = NULL;
            else
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashR[i] = NULL;
        }

        JPABaseEmitter *emt = GetJPAMgr()->createEmt(names[i], pos);
        u32 userWork = (kartNo << 8) | ((u32)i << 4) | sideIdx;
        if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emt))
            continue;

        if (emt != NULL)
        {
            if (sideIdx == 0)
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashL[i] = emt;
            else
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashR[i] = emt;
            emt->mEmitterCallback = &JPKinokoDashECB2;
            emt->mFlags |= 0x40;
            emt->mpUserWork = userWork;
        }
    }
}

static void calcEfct_KartMiniDash1(int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    u32 userWork;
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_miniDash_a", pos);
    userWork = (kartNo << 4) | flag;
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPKinokoDashECB;
        emt->mpUserWork = userWork;
    }
}

static void calcEfct_KartMiniDash2(int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    u32 userWork;
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_miniDash_b", pos);
    userWork = (kartNo << 4) | flag;
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPKinokoDashECB;
        emt->mpUserWork = userWork;
    }
}

static void calcEfct_KartWheelSpin(int kartNo, const JGeometry::TVec3f &pos, u8 sideIdx)
{
    JPABaseEmitter *emts[2] = {};
    const char *names[2] = {"mk_wheelSpin_a", "mk_wheelSpin_b"};

    u32 userWork = (kartNo << 4) | sideIdx;
    for (u8 i = 0; i < 2; i++)
    {
        emts[i] = GetJPAMgr()->createEmt(names[i], pos);
        if (emts[i] != NULL)
        {
            emts[i]->mEmitterCallback = &JPWheelSpinECB;
            emts[i]->mpUserWork = userWork;
        }
    }
}

static void calcEfct_KartWaterWave(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    if (JPEffectPerformer::getKartEfctEmts(kartNo)->waterWave == NULL)
    {
        JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_inWater_nami", pos);
        if (emt != NULL)
        {
            emt->mEmitterCallback = &JPWaterWaveECB;
            emt->mpUserWork = kartNo;
            JPEffectPerformer::getKartEfctEmts(kartNo)->waterWave = emt;
        }
    }
}

static void calcEfct_KartWaterRipple(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    if (JPEffectPerformer::getKartEfctEmts(kartNo)->waterRipple == NULL)
    {
        JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_hamon_a", pos);
        if (emt != NULL)
        {
            emt->mEmitterCallback = &JPWaterRippleECB;
            emt->mpUserWork = kartNo;
            JPEffectPerformer::getKartEfctEmts(kartNo)->waterRipple = emt;
        }
    }
}

static void calcEfct_CharFire(int kartNo, const JGeometry::TVec3f &pos, u8 sideIdx)
{
    if (JPEffectPerformer::getKartEfctEmts(kartNo)->charFire != NULL)
        GetJPAMgr()->deleteEmt(JPEffectPerformer::getKartEfctEmts(kartNo)->charFire);

    const char *name = "mk_ketsumoe";
    u8 dir = 0;
    if (!JPEffectPerformer::sCharSmokeParamSpecialFlg)
    {
        if (GetKartCtrl()->GetTandemDir(kartNo) == 1)
            dir = 1;
    }
    else if (GetKartCtrl()->GetTandemDir(kartNo) == 1)
        dir = 3;
    else
        dir = 2;

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(ObjUtility::concatSimpleFileName((char *)name, dir), pos);
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;

    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPCharFireECB;
        emt->mParticleCallback = &JPTracePCB;
        emt->mpUserWork = (sCharFireSmokeSpdRatio << 8) | (kartNo << 4);
        JPEffectPerformer::getKartEfctEmts(kartNo)->charFire = emt;
    }
}

static void calcEfct_HitFireBall(int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    JPABaseEmitter *emts[2] = {NULL, NULL};
    const char *names[2] = {NULL, NULL};
    u32 sel = flag;

    switch (sel)
    {
    case 0:
        names[0] = "mk_fbrHit_a";
        names[1] = "mk_fbrHit_b";
        break;
    case 1:
        names[0] = "mk_fbgHit_a";
        names[1] = "mk_fbgHit_b";
        break;
    }

    emts[0] = GetJPAMgr()->createEmt(names[0], pos);
    if (TstEmitterModeNotAvailableFlg(kartNo, emts[0]))
        return;

    emts[1] = GetJPAMgr()->createEmt(names[1], pos);

    JPABaseEmitter **p = emts;
    for (u8 i = 0; i < 2; i++, p++)
    {
        if (*p != NULL)
        {
            (*p)->mEmitterCallback = &JPTraceECB;
            (*p)->mpUserWork = kartNo;
        }
    }
}
static inline void calcEfct_KartTireSandBase(int kartNo, const JGeometry::TVec3f &pos, u8 num, const char *name)
{
    for (u8 i = 0; i < num; i++)
    {
        if (JPEffectPerformer::getKartEfctEmts(kartNo)->tireSand[i] != NULL)
            continue;

        JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
        u32 userWork = (kartNo << 4) | i;
        if (TstEmitterModeNotAvailableFlg(kartNo, emt))
            continue;

        if (emt != NULL)
        {
            emt->mEmitterCallback = &JPTireSandECB;
            emt->mpUserWork = userWork;
            JPEffectPerformer::getKartEfctEmts(kartNo)->tireSand[i] = emt;
        }
    }
}

static void calcEfct_KartTireSand(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    calcEfct_KartTireSandBase(kartNo, pos, 2, "mk_sunamakiage_a");
}

static void calcEfct_KartMiniDash(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    if (GetStEfctMgr()->tstUseMufllerL(kartNo))
    {
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x25, kartNo, pos, 0);
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x26, kartNo, pos, 0);
    }
    if (GetStEfctMgr()->tstUseMufllerR(kartNo))
    {
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x25, kartNo, pos, 1);
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x26, kartNo, pos, 1);
    }
}

static void calcEfct_KartStartDash(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    if (GetStEfctMgr()->tstUseMufllerL(kartNo))
    {
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x25, kartNo, pos, 0);
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x26, kartNo, pos, 0);
    }
    if (GetStEfctMgr()->tstUseMufllerR(kartNo))
    {
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x25, kartNo, pos, 1);
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)0x26, kartNo, pos, 1);
    }
}

static void calcEfct_KartTagStartDash(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    char *names[3] = {"mk_wDash_a", "mk_wDash_b", "mk_wDash_c"};

    if (GetStEfctMgr()->tstUseMufllerL(kartNo))
        calcEfct_KartKinokoDash3Base(kartNo, pos, 0, names);
    if (GetStEfctMgr()->tstUseMufllerR(kartNo))
        calcEfct_KartKinokoDash3Base(kartNo, pos, 1, names);

    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_2pDash_a", pos);
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPRotateXZECB;
        emt->mpUserWork = kartNo;
    }
}

static void calcEfct_KartDash(int kartNo, const JGeometry::TVec3f &pos, u8)
{
}

static void calcEfct_KartDisappear(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *name = "mk_lose";
    for (u8 i = 0; i < 2; i++)
    {
        const char *emtName = ObjUtility::concatSimpleFileName((char *)name, i);
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(emtName, pos);
        if (emt == NULL)
            continue;

        u32 kartID = RCMGetManager()->getKartInfo(kartNo)->getKartID();
#line 193
        JUT_MINMAX_ASSERT(0, kartID, 21);
#line 1

        JGeometry::TVec3f sclVec;
        JGeometry::TVec3f out;
        KartOffsetData *data = &sKartOffsetData[kartID];
        JGeometry::TVec3f dir(0.0f, data->x, data->z);
        sclVec.setAll(data->scale);
        sclVec.scale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());

        PSMTXMultVec(ObjUtility::getKartBodyMtx(kartNo), &dir, &out);

        emt->setGlobalScale(sclVec);
        emt->setGlobalTranslation(out);
    }
}

static void calcEfct_KartInvalidity(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_muteki_a", pos);
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPRotateECB;
        emt->mpUserWork = kartNo;
    }
}

static void calcEfct_KartWet(int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    if (JPEffectPerformer::getKartEfctEmts(kartNo)->wet != NULL)
        return;

    const char *prefix = "mk_mizupota";
    char sizeChar[3] = {'S', 'M', 'L'};
    char colChar[3] = {'a', 'b', 'c'};
    u8 weight = KartInfo::getKartWeight(RCMGetManager()->getKartInfo(kartNo)->getKartID());
    EKartID kartID = RCMGetManager()->getKartInfo(kartNo)->getKartID();
    if (kartID == BOO_PIPES || kartID == PIRANHA_PIPES)
        weight = 1;

    u8 col = 0;
    if (flag == 1)
    {
        switch (RCMGetManager()->getCourse()->getCourseID())
        {
        case WALUIGI_STADIUM: col = 1; break;
        case BOWSER_CASTLE:   col = 2; break;
        }
    }

    char name[64];
    snprintf(name, 64, "%s%c_%c", prefix, sizeChar[weight], colChar[col]);

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (emt != NULL && !TstEmitterModeNotAvailableFlg(kartNo, emt))
    {
        emt->mEmitterCallback = &JPKartWetECB;
        emt->mpUserWork = kartNo;
        JPEffectPerformer::getKartEfctEmts(kartNo)->wet = emt;
    }

    emt = GetJPAMgr()->createEmt("mk_mizupota_ato", pos);
    if (emt == NULL)
        return;
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;

    CrsGround *ground = GetKartCtrl()->GetBodyGround(kartNo);
    JGeometry::TVec3f normal;
    ground->getNormal(&normal);
    JGeometry::TPos3f mtx;
    stMakeDirectionMtx(&mtx, normal, 'Y');
    f32 height = ground->getHeight();
    JGeometry::TVec3f p(pos);
    p.y = height;
    normal.scale(5.0f);
    p.add(normal);
    mtx[0][3] = p.x;
    mtx[1][3] = p.y;
    mtx[2][3] = p.z;
    JPASetRMtxTVecfromMtx(mtx, emt->mGlobalRot, &emt->mGlobalTrs);
}

static void calcEfct_KartFireSmoke(int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    const char *name;
    if (JPEffectPerformer::sCharSmokeParamSpecialFlg)
        name = "mk_pusupusu_b";
    else
        name = "mk_pusupusu_a";

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (emt == NULL)
        return;

    u8 unavailableFlg = TstEmitterClipNotAvailableFlg(kartNo, emt);

    if (!unavailableFlg)
    {
        emt->mEmitterCallback = &JPPtclTraceECB;
        emt->mParticleCallback = &JPTracePCB;
        emt->mpUserWork = (sFallLavaSmokeSpdRatio << 8) | (kartNo << 4);
    }
}

static void calcEfct_BonusKartEfct(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *name = RCMGetManager()->isAwardDemoMode() ? "mk_extra_kira_demo" : "mk_extra_kira";

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (emt != NULL && !TstEmitterModeNotAvailableFlg(kartNo, emt))
    {
        emt->mEmitterCallback = &JPBonusKartECB;
        emt->mpUserWork = kartNo;
    }
}

static void calcEfct_KartDriftFireChipL(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_dftFirechip_a", pos);
    if (emt == NULL)
        return;
    emt->mEmitterCallback = &JPTireTraceECB;
    emt->mpUserWork = kartNo << 4;
}

static void calcEfct_KartDriftFireChipR(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_dftFirechip_a", pos);
    if (emt == NULL)
        return;
    emt->mEmitterCallback = &JPTireTraceECB;
    emt->mpUserWork = (kartNo << 4) | 1;
}

static void calcEfct_KartStartHit(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *name = "mk_starHit";

    for (u8 i = 0; i < 3; i++)
    {
        const char *emtName = ObjUtility::concatSimpleFileName((char *)name, i);
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(emtName, pos);
        if (emt == NULL)
            continue;

        emt->mEmitterCallback = &JPTraceECB;

        if (TstEmitterModeNotAvailableFlg(kartNo, emt))
            continue;

        emt->mpUserWork = kartNo;
    }
}

static void calcEfct_StartFootSmoke(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    int driver = ItemObjMgr::getNowTandemDriverNum(kartNo);

    KartInfo *info = RCMGetManager()->getKartInfo(kartNo);
    int charID = info->getDriverCharID(driver);
#line 2046
    JUT_MINMAX_ASSERT(0, charID, 21);
#line 1
    if (charID == 19)
        return;

    const char *name = ObjUtility::concatSimpleFileName("mk_ashikemu", JPEffectPerformer::sStartFootSmokeIndex);
    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (emt == NULL)
        return;

    KartLoader *loader = RCMGetManager()->getKartLoader(kartNo);
    DriverModel *model = loader->getExModelDriver(driver);
    int jointNo = model->getModelData()->getJointName()->getIndex("skl_root");
    const Mtx &jointMtx = model->getModel()->mMtxBuffer->getAnmMtx(jointNo);

    JGeometry::TVec3f p(jointMtx[0][3], jointMtx[1][3], jointMtx[2][3]);
    CrsGround ground(RCMGetCourse());
    ground.search(p);
    JGeometry::TVec3f efctPos(p);
    efctPos.y = ground.getHeight();

    emt->mpUserWork = kartNo;
    emt->mEmitterCallback = &JPStartSmokeECB;

    const f32 *ofs = sCharStartSmokeOffsetData[charID];
    f32 scale = ofs[0];
    f32 zOff = 0.0f;
    if (charID == 5 || charID == 6)
        zOff = -30.0f;

    JGeometry::TVec3f zdir, ydir;
    ObjUtility::getKartYdir(kartNo, &ydir);
    ObjUtility::getKartZdir(kartNo, &zdir);

    ydir.scale(ofs[1]);
    zdir.scale(zOff);
    efctPos.add(ydir);
    efctPos.add(zdir);

    emt->setGlobalTranslation(efctPos);
    emt->setGlobalRMatrix(ObjUtility::getKartBodyMtx(kartNo));
    emt->setScale(scale);
}

namespace
{
    typedef void (*EfctFunc)(int, const JGeometry::TVec3f &, u8);

    struct EfctFuncEntry
    {
        u8 mEnable;
        EfctFunc mFunc;
    };

    EfctFuncEntry efctFuncTbl[] = {
        {0, calcEfct_KartOverTurn},
        {0, calcEfct_KartSpin},
        {0, calcEfct_KartHalfSpin},
        {0, calcEfct_KartLanding},
        {0, calcEfct_ColKartAndKart},
        {0, calcEfct_ColKartAndObject},
        {0, calcEfct_ColKartAndObject},
        {0, calcEfct_KartWaterSplashSmall},
        {0, calcEfct_KartWaterMudSplashSmall},
        {0, calcEfct_KartWaterDrainSplashSmall},
        {0, calcEfct_KartWaterMudDiddySplashSmall},
        {0, calcEfct_KartFallLava},
        {0, calcEfct_KartDisappear},
        {0, calcEfct_KartInvalidity},
        {0, calcEfct_KartWet},
        {0, calcEfct_KartFireSmoke},
        {1, calcEfct_KartDriftFireChipL},
        {1, calcEfct_KartDriftFireChipR},
        {0, calcEfct_KartStartHit},
        {0, calcEfct_CharFire},
        {0, calcEfct_KartAccelExhaustL},
        {0, calcEfct_KartAccelExhaustR},
        {0, NULL},
        {1, calcEfct_KartDash},
        {1, calcEfct_KartStartDash},
        {1, calcEfct_KartTagStartDash},
        {1, calcEfct_KartMiniDash},
        {0, calcEfct_KartSlide},
        {0, calcEfct_KartStar},
        {1, calcEfct_KartKinokoDash1},
        {1, calcEfct_KartKinokoDash2},
        {1, calcEfct_KartKinokoDash3},
        {0, calcEfct_KartWheelSpin},
        {0, calcEfct_KartWaterWave},
        {0, calcEfct_KartWaterRipple},
        {0, NULL},
        {0, calcEfct_HitFireBall},
        {1, calcEfct_KartMiniDash1},
        {1, calcEfct_KartMiniDash2},
        {0, calcEfct_KartTireSand},
        {0, calcEfct_BonusKartEfct},
        {0, calcEfct_StartFootSmoke},
    };
} // namespace

inline PermissionCounterObserver::~PermissionCounterObserver() {}

JPEffectPerformer::KartEfctEmts JPEffectPerformer::sKartEfctEmts[8];
PermissionCounter JPEffectPerformer::sPCounters[8][22];
PermissionCounterObserver JPEffectPerformer::sPCounterObserver;
f32 JPEffectPerformer::sKoukasenLength = 2500.0f;
u8 *JPEffectPerformer::sCrs_Params;
u8 *JPEffectPerformer::sCrs_ParamNum;
u8 JPEffectPerformer::sCharSmokeParamSpecialFlg;
u8 JPEffectPerformer::sStartFootSmokeIndex;

void JPEffectPerformer::init()
{
    JPKoukasenECB = new JPKoukasenEmitterCallBack[RCMGetManager()->getConsoleNumber()];

    sCharSmokeParamSpecialFlg = false;

    switch (RCMGetManager()->getCourse()->getCourseID())
    {
    case PEACH_BEACH:
    case DK_MOUNTAIN:
    case RAINBOW_ROAD:
    case DRY_DRY_DESERT:
    case SHERBET_LAND:
        sCharSmokeParamSpecialFlg = true;
        break;
    }

    switch (RCMGetManager()->getCourse()->getCourseID())
    {
    case BABY_PARK:
    case LUIGI_CIRCUIT:
    case MARIO_CIRCUIT:
    case YOSHI_CIRCUIT:
    case MUSHROOM_BRIDGE:
    case MUSHROOM_CITY:
    case WARIO_COLOSSEUM:
        sStartFootSmokeIndex = 0;
        break;
    case WALUIGI_STADIUM:
    case DINO_DINO_JUNGLE:
    case DK_MOUNTAIN:
    case BOWSER_CASTLE:
    case DRY_DRY_DESERT:
        sStartFootSmokeIndex = 1;
        break;
    case PEACH_BEACH:
    case DAISY_CRUISER:
    case RAINBOW_ROAD:
    case SHERBET_LAND:
        sStartFootSmokeIndex = 2;
        break;
    default:
        break;
    }

    reset();
}

void JPEffectPerformer::reset()
{
    sPCounterObserver.reset();

    for (int k = 0; k < 8; k++)
    {
        for (int i = 0; i < 22; i++)
        {
            PermissionCounter &pc = sPCounters[k][i];
            pc.mCount = 0;
            switch (i)
            {
            case 0x10:
            case 0x11:
                pc.mPermission = 0x3c;
                break;
            case 0x13:
                pc.mPermission = 0xd2;
                break;
            case 0x14:
            case 0x15:
                pc.mPermission = 0x1e;
                break;
            }
        }

        sKartEfctEmts[k].waterWave = NULL;
        sKartEfctEmts[k].waterRipple = NULL;
        sKartEfctEmts[k].koukasen = NULL;
        sKartEfctEmts[k].charFire = NULL;
        sKartEfctEmts[k].wet = NULL;
        sKartEfctEmts[k].star = NULL;
        for (u8 t = 0; t < 2; t++)
            sKartEfctEmts[k].tireSand[t] = NULL;
        for (u8 j = 0; j < 3; j++)
        {
            sKartEfctEmts[k].kinokoDashL[j] = NULL;
            sKartEfctEmts[k].kinokoDashR[j] = NULL;
        }
    }

    sPlayerNum = RCMGetManager()->getStatusNumber();
}

void JPEffectPerformer::loadCourseParam(u32 id)
{
    u8 *data = (u8 *)ResMgr::getPtr((ResMgr::ArchiveId)0, "/Effect/JPAEffectData.pel");
    u8 *courseID = data;
    u8 *pCount = data + 1;
#line 2326
    JUT_ASSERT(*courseID == 0x00);

    sCrs_ParamNum = pCount;
    sCrs_Params = NULL;

    while (id != *courseID)
    {
        if (*courseID == 0xff)
        {
            sCrs_ParamNum = NULL;
            sCrs_Params = NULL;
            break;
        }

        courseID = &courseID[*sCrs_ParamNum * 75];
        sCrs_ParamNum = courseID + 3;
        sCrs_Params = sCrs_ParamNum + 1;
        courseID += 2;
    }
}

void JPEffectPerformer::calc()
{
    sPCounterObserver.calc();
}

void JPEffectPerformer::deleteKartEfctKoukasen(int kartNo, u8 camNo)
{
    JPABaseEmitter *emitter = sKartEfctEmts[kartNo].koukasen;
    if (emitter)
    {
        GetJPAMgr()->deleteEmtCam(emitter, camNo);
        sKartEfctEmts[kartNo].koukasen = NULL;
    }
}

void JPEffectPerformer::deleteKartEfctTireSand(int kartNo)
{
    for (u8 i = 0; i < 2; i++)
    {
        JPABaseEmitter *emt = sKartEfctEmts[kartNo].tireSand[i];
        if (emt)
        {
            GetJPAMgr()->deleteEmt(emt);
            sKartEfctEmts[kartNo].tireSand[i] = NULL;
        }
    }
}

#ifndef __MWERKS__
static inline f32 JPSquareDistance(const Vec *a, const Vec *b)
{
    f32 dx = a->x - b->x;
    f32 dy = a->y - b->y;
    f32 dz = a->z - b->z;
    return (dx * dx + dy * dy) + dz * dz;
}
#else
static inline f32 JPSquareDistance(register const Vec *a, register const Vec *b)
{
    register f32 v0yz, v1yz, v0xy, v1xy, dyz, dxy, sqdist;
    // clang-format off
    __asm {
        psq_l v0xy, 0(a), 0, 0
        psq_l v1xy, 0(b), 0, 0
        ps_sub dxy, v0xy, v1xy
        psq_l v0yz, 4(a), 0, 0
        psq_l v1yz, 4(b), 0, 0
        ps_sub dyz, v0yz, v1yz
        ps_mul dyz, dyz, dyz
        ps_madd sqdist, dxy, dxy, dyz
        ps_sum0 sqdist, sqdist, dyz, dyz
    }
    // clang-format on
    return sqdist;
}
#endif

static inline KartCam *JPGetCamera(int cameraNo)
{
    ((0 <= cameraNo) && (cameraNo < 4))
        ? (void)0
        : (JUTAssertion::showAssert_f(JUTAssertion::getSDevice(), "RaceMgr.h", 262, "range over: %d <= cameraNo=%d < %d", 0,
                                      cameraNo, 4),
           OSPanic("RaceMgr.h", 262, "Halt"));
    return KartCtrl::getKartCtrl()->getKartCam(cameraNo);
}

Mtx &JPEffectPerformer::getNearestCamPtr(const JGeometry::TVec3f &pos)
{
    KartCam *cam = JPGetCamera(0);
    MtxPtr bestMtx = cam->GetMtx();

    if (RCMGetManager()->getConsoleNumber() >= 2)
    {
        JGeometry::TVec3f camPos(*cam->GetCameraPos());
        f32 best = JPSquareDistance(&pos, &camPos);

        for (int i = 1; i < RCMGetManager()->getConsoleNumber(); i++)
        {
            camPos = *JPGetCamera(i)->GetCameraPos();
            if (best > JPSquareDistance(&pos, &camPos))
            {
                best = JPSquareDistance(&pos, &camPos);
                bestMtx = JPGetCamera(i)->GetMtx();
            }
        }
    }

    return *(Mtx *)bestMtx;
}

void JPEffectPerformer::setEffect(EffectType type, int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    KartInfo *info = RCMGetManager()->getKartInfo(kartNo);
    if (KartInfo::KartCharacter::convPlayerKind(info->getPad()) == 4)
    {
        if (!efctFuncTbl[type].mEnable)
            return;
    }

    if (type < 22)
    {
        if (sPCounters[kartNo][type].mLink.getSupervisor() != NULL)
            return;
        sPCounterObserver.append(&sPCounters[kartNo][type].mLink);
    }

    efctFuncTbl[type].mFunc(kartNo, pos, flag);
}

void JPEffectPerformer::setEffectSplash(CrsGround *ground, int kartNo, const JGeometry::TVec3f &pos)
{
    KartInfo *info = RCMGetManager()->getKartInfo(kartNo);
    if (KartInfo::KartCharacter::convPlayerKind(info->getPad()) == 4)
        return;

    int splashID = 42;
    switch (ground->getSplashID())
    {
    case 1:
    case 2:
        splashID = 7;
        break;
    case 3:
        splashID = 8;
        break;
    case 4:
        splashID = 9;
        break;
    case 6:
        splashID = 10;
        break;
    case 5:
        splashID = 11;
        break;
    }

    JGeometry::TVec3f p(pos);
    p.y = ground->getSplashHeight();

    if (splashID == 42)
        return;

    info = RCMGetManager()->getKartInfo(kartNo);
    if (KartInfo::KartCharacter::convPlayerKind(info->getPad()) == 4)
    {
        if (!efctFuncTbl[splashID].mEnable)
            return;
    }

    if (splashID < 22)
    {
        if (sPCounters[kartNo][splashID].mLink.getSupervisor() != NULL)
            return;
        sPCounterObserver.append(&sPCounters[kartNo][splashID].mLink);
    }

    efctFuncTbl[splashID].mFunc(kartNo, p, 0);
}

void JPEffectPerformer::setEffectEachCam(EffectType type, int kartNo, u8 camNo, u8 flag)
{
    if (RCMGetManager()->getStatusNumber() > 1)
        return;

    KartInfo *info = RCMGetManager()->getKartInfo(kartNo);
    if (KartInfo::KartCharacter::convPlayerKind(info->getPad()) == 4)
        return;

    switch (type)
    {
    case Effect_Cannon:
    {
        if (sKartEfctEmts[kartNo].koukasen != NULL)
            GetJPAMgr()->deleteEmtCam(sKartEfctEmts[kartNo].koukasen, (u8)camNo);

        JGeometry::TVec3f pos(0.0f, 0.0f, 0.0f);
        int camIndex = camNo;
        u32 userWork = flag | ((kartNo << 8) | (camNo << 4));
        JPABaseEmitter *emt = GetJPAMgr()->createEmtCam("mk_koukasen", pos, camIndex);
        if (emt == NULL)
            return;

        JPKoukasenEmitterCallBack *cb = &JPKoukasenECB[camIndex];
        cb->mCounter = 0;
        emt->mEmitterCallback = &JPKoukasenECB[camIndex];
        emt->mpUserWork = userWork;
        sKartEfctEmts[kartNo].koukasen = emt;
        break;
    }
    }
}

void JPEffectPerformer::setLandEffect(const JGeometry::TVec3f &pos, CrsGround *ground, int kartNo)
{
    if (KartInfo::KartCharacter::convPlayerKind(RCMGetManager()->getKartInfo(kartNo)->getPad()) == 4)
        return;

    const char *name = getLandingEfctName(ground);
    if (name == NULL)
        return;

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (emt == NULL)
        return;
    if (kartNo < 0)
        return;

    JGeometry::TVec3f normal;
    ground->getNormal(&normal);
    JGeometry::TPos3f mtx;
    stMakeDirectionMtx(&mtx, normal, 'Y');
    JPASetRMtxfromMtx(mtx, emt->mGlobalRot);
    emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
}

const char *JPEffectPerformer::getLandingEfctName(CrsGround *ground)
{
    if (sCrs_Params == NULL)
        return NULL;

    u8 num = *sCrs_ParamNum;
    u8 *p;
    p = sCrs_Params;
    for (u8 i = 0; i < num; i++, p += 75)
    {
        if (p[0] == ground->getAttribute() && p[1] == ground->getAttrIndex())
        {
            switch (RCMGetManager()->getConsoleNumber())
            {
            case 1:
                return (const char *)(p + 3);
            case 2:
                return (const char *)(p + 27);
            case 3:
            case 4:
                return (const char *)(p + 51);
            }
        }
    }
    return NULL;
}

JPEffectPerformer::KartEfctEmts *JPEffectPerformer::getKartEfctEmts(int kart_index)
{
#line 2586
    JUT_MINMAX_ASSERT(0, kart_index, 8);
#line 1

    return &sKartEfctEmts[kart_index];
}

void JPEffectPerformer::deleteKartEfctWet(int kart_index)
{
#line 2598
    JUT_MINMAX_ASSERT(0, kart_index, 8);
#line 1
    KartEfctEmts *ef = getKartEfctEmts(kart_index);
    JPABaseEmitter *wet = ef->wet;
    if (wet)
    {
        wet->stopCreateParticle();
        wet->mMaxFrame = 1;
        ef->wet = NULL;
    }
}


#include "JSystem/JAudio/JASFakeMatch2.h"
