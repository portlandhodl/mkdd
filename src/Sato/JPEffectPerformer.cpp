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
    const KartOffsetData sKartOffsetData[cKartIDMax] = {
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

    const f32 sCharStartSmokeOffsetData[cKartIDMax][2] = {
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
    virtual void execute(JPABaseEmitter *emt)
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
    virtual void draw(JPABaseEmitter *);
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

u8 TstEmitterModeAndClipNotAvailableFlg(int kartNo, JPABaseEmitter *emt)
{
    bool unavailableFlg = TstEmitterModeNotAvailableFlg(kartNo, emt);

    if (!unavailableFlg)
    {
        if (emt->mResource->getDyn()->getResUserWork() & 1)
        {
            if (!GetStEfctMgr()->tstIsAvailableDistance(kartNo))
            {
                GetJPAMgr()->deleteEmt(emt);
                unavailableFlg = true;
            }
        }
    }

    return unavailableFlg;
}


void JPTraceEmitterCallBack::execute(JPABaseEmitter *emt)
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
        EKartID kartID = RCMGetManager()->getKartInfo(kartNo)->getKartID();
        JUT_MINMAX_ASSERT(0, kartID, 21);

        f32 scale = GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale();

        JGeometry::TVec3f dir(0.0f, sKartOffsetData[kartID].x, sKartOffsetData[kartID].z);
        JGeometry::TVec3f sclVec;
        sclVec.setAll(sKartOffsetData[kartID].scale);
        sclVec.scale(scale);

        JGeometry::TVec3f out;
        const Mtx &mtx = ObjUtility::getKartBodyMtx(kartNo);
        PSMTXMultVec(mtx, &dir, &out);

        emt->setGlobalScale(sclVec);
        emt->setGlobalTranslation(out.x, out.y, out.z);
        emt->setGlobalRMatrix(mtx);
    }
}

void JPRotateOffsetEmitterCallBack::draw(JPABaseEmitter *) {}

void JPRotateXZEmitterCallBack::execute(JPABaseEmitter *emt)
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
        EKartID kartID = RCMGetManager()->getKartInfo(kartNo)->getKartID();
        JUT_MINMAX_ASSERT(0, kartID, 21);

        JGeometry::TVec3f dir(0.0f, sKartOffsetData[kartID].x, sKartOffsetData[kartID].z);
        JGeometry::TVec3f sclVec;
        sclVec.setAll(sKartOffsetData[kartID].scale);
        sclVec.scale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());

        JGeometry::TVec3f out;
        const Mtx * kartMtx = &ObjUtility::getKartBodyMtx(kartNo);
        PSMTXMultVec(*kartMtx, &dir, &out);

        emt->setGlobalScale(sclVec);
        emt->setGlobalTranslation(out.x, out.y, out.z);

        JGeometry::TVec3f zdir;
        ObjUtility::getKartZdir(kartNo, &zdir);

        JGeometry::TVec3f pos;
        ObjUtility::getKartPos(kartNo, &pos);

        JGeometry::TPos3f dirMtx;
        stMakeDirectionMtx(&dirMtx, zdir, 'Z');
        emt->setGlobalRMatrix(dirMtx);
        emt->setGlobalTranslation(pos.x, pos.y, pos.z);
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
        const Mtx &mtx = ObjUtility::getKartBodyMtx(kartNo);
        JPASetRMtxSTVecfromMtx(mtx, emt->mGlobalRot, &emt->mGlobalScl, &emt->mGlobalTrs);
        emt->setGlobalParticleScale(emt->mGlobalScl);
    }
}

void JPRotateEmitterCallBack::draw(JPABaseEmitter *) {}

void JPBonusKartEfctEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = getKartIndex(emt);
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
        JPASetRMtxSTVecfromMtx(ObjUtility::getKartBodyMtx(kartNo), emt->mGlobalRot, &emt->mGlobalScl, &emt->mGlobalTrs);
        emt->setGlobalParticleScale(emt->mGlobalScl);
    }

    if (GetKartCtrl()->GetGameStatus(getKartIndex(emt)) & 0x2000)
        emt->playCreateParticle();
    else
        emt->stopCreateParticle();
}

void JPKinokoDashEmitterCallBack::execute(JPABaseEmitter *emt)
{
    bool unavailableFlg = false;
    int kartNo = emt->mpUserWork >> 4;

    if (emt->mResource->getDyn()->getResUserWork() & 1)
    {
        if (!TstEmitterModeNotAvailableFlg(kartNo, emt))
            /* nothing */;
        if (!GetStEfctMgr()->tstIsAvailableDistance(kartNo))
        {
            GetJPAMgr()->deleteEmt(emt);
            unavailableFlg = true;
        }
    }

    if (!unavailableFlg)
    {
        MtxPtr exhaustMtx;
        if (((emt->mpUserWork >> 4) & 0xF) == 0)
            exhaustMtx = GetStEfctMgr()->getKartExhaustMtxL(kartNo);
        else
            exhaustMtx = GetStEfctMgr()->getKartExhaustMtxR(kartNo);

        f32 n00 = -exhaustMtx[0][0];
        f32 n10 = -exhaustMtx[1][0];
        f32 n20 = -exhaustMtx[2][0];
        f32 v01 = exhaustMtx[0][1];
        f32 v11 = exhaustMtx[1][1];
        f32 v21 = exhaustMtx[2][1];
        f32 n02 = -exhaustMtx[0][2];
        f32 n12 = -exhaustMtx[1][2];
        f32 n22 = -exhaustMtx[2][2];
        f32 tz = exhaustMtx[2][3];
        f32 ty = exhaustMtx[1][3];
        f32 tx = exhaustMtx[0][3];

        Mtx negMtx;
        PSMTXCopy(exhaustMtx, negMtx);
        negMtx[0][0] = n00;
        negMtx[1][0] = n10;
        negMtx[2][0] = n20;
        negMtx[0][1] = v01;
        negMtx[1][1] = v11;
        negMtx[2][1] = v21;
        negMtx[0][2] = n02;
        negMtx[1][2] = n12;
        negMtx[2][2] = n22;
        negMtx[0][3] = tx;
        negMtx[1][3] = ty;
        negMtx[2][3] = tz;

        JPASetRMtxTVecfromMtx(negMtx, emt->mGlobalRot, &emt->mGlobalTrs);
        emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
    }
}

void JPKinokoDashEmitterCallBack::draw(JPABaseEmitter *) {}

void JPKinokoDash2EmitterCallBack::execute(JPABaseEmitter *emt)
{
    bool unavailableFlg = false;
    int kartNo = emt->mpUserWork >> 4;

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
        MtxPtr exhaustMtx;
        if (((emt->mpUserWork >> 4) & 0xF) == 0)
            exhaustMtx = GetStEfctMgr()->getKartExhaustMtxL(kartNo);
        else
            exhaustMtx = GetStEfctMgr()->getKartExhaustMtxR(kartNo);

        f32 n00 = -exhaustMtx[0][0];
        f32 n10 = -exhaustMtx[1][0];
        f32 n20 = -exhaustMtx[2][0];
        f32 v01 = exhaustMtx[0][1];
        f32 v11 = exhaustMtx[1][1];
        f32 v21 = exhaustMtx[2][1];
        f32 n02 = -exhaustMtx[0][2];
        f32 n12 = -exhaustMtx[1][2];
        f32 n22 = -exhaustMtx[2][2];
        f32 tz = exhaustMtx[2][3];
        f32 ty = exhaustMtx[1][3];
        f32 tx = exhaustMtx[0][3];

        Mtx negMtx;
        PSMTXCopy(exhaustMtx, negMtx);
        negMtx[0][0] = n00;
        negMtx[1][0] = n10;
        negMtx[2][0] = n20;
        negMtx[0][1] = v01;
        negMtx[1][1] = v11;
        negMtx[2][1] = v21;
        negMtx[0][2] = n02;
        negMtx[1][2] = n12;
        negMtx[2][2] = n22;
        negMtx[0][3] = tx;
        negMtx[1][3] = ty;
        negMtx[2][3] = tz;

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
    emt->setGlobalTranslation(kartMtx[0][3], kartMtx[1][3], kartMtx[2][3]);
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
    int kartNo = ((u32)emt->mpUserWork >> 4) & 0xF;

    const Mtx &kartMtx = ObjUtility::getKartBodyMtx(kartNo);
    emt->setGlobalTranslation(kartMtx[0][3], kartMtx[1][3], kartMtx[2][3]);
    emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());

    if (GetKartCtrl()->GetGameStatus(kartNo) & 0x2000)
        emt->playCreateParticle();
    else
        emt->stopCreateParticle();

    if (!(GetKartCtrl()->GetKartStatus(kartNo) & 0x40000))
    {
        emt->stopCreateParticle();
        emt->mMaxFrame = 1;
        JPEffectPerformer::sKartEfctEmts[kartNo].star = NULL;
    }
}

void JPWheelSpinEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = (u32)emt->mpUserWork >> 4;

    if (!(GetKartCtrl()->GetKartStatus(kartNo) & 0x800))
    {
        emt->stopCreateParticle();
        emt->mMaxFrame = 1;
    }

    JPTireTraceEmitterCallBack::execute(emt);
}

void JPWheelSpinEmitterCallBack::draw(JPABaseEmitter *) {}

void JPWaterWaveEmitterCallBack::execute(JPABaseEmitter *emt)
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

    stWaterTrack *track = GetStEfctMgr()->getWaterTrack(kartNo);
    if (track == NULL)
    {
        JPEffectPerformer::sKartEfctEmts[kartNo].waterWave = NULL;
        emt->stopCreateParticle();
        emt->mMaxFrame = 1;
    }
    else
    {
        if ((GetKartCtrl()->GetKartStatus(kartNo) & 3) != 0 ||
            GetKartCtrl()->GetWaterHeight(kartNo) <= sWaterWaveHeightMin ||
            GetKartCtrl()->GetWaterHeight(kartNo) >= sWaterWaveHeightMax ||
            unavailableFlg)
        {
            emt->stopCreateParticle();
        }
        else
        {
            f32 absSpd = JMAAbs(GetKartCtrl()->GetCarSpeed(kartNo));

            if (absSpd < sWaterWaveMinSpd)
            {
                emt->stopCreateParticle();
            }
            else
            {
                f32 adjSpd = absSpd;
                if (adjSpd > sWaterWaveMaxSpd)
                    adjSpd = sWaterWaveMaxSpd;

                f32 e = (f32)exp(-(3.0f * (adjSpd - sWaterWaveMinSpd) / (sWaterWaveMaxSpd - sWaterWaveMinSpd)));

                f32 pscale = (1.0f - e) * GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale();
                const Mtx &kartMtx = ObjUtility::getKartBodyMtx(kartNo);
                emt->mGlobalPScl.x = pscale;
                emt->mGlobalPScl.y = pscale;
                emt->playCreateParticle();
                emt->mGlobalTrs.x = track->mPosition.x;
                emt->mGlobalTrs.y = track->mPosition.y;
                emt->mGlobalTrs.z = track->mPosition.z;
                emt->setGlobalRMatrix(kartMtx);
            }
        }
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
        JPEffectPerformer::sKartEfctEmts[kartNo].waterRipple = NULL;
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

    JUT_MINMAX_ASSERT(0, slot, 4);

    const float (*camMtx)[4] = GetKartCtrl()->getKartCam(slot)->GetMtx();

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

    JUT_MINMAX_ASSERT(0, slot, 4);

    JGeometry::TVec3f pos = *GetKartCtrl()->getKartCam(slot)->GetCameraPos();
    pos += dir;
    mat[0][3] = pos.x;
    mat[1][3] = pos.y;
    mat[2][3] = pos.z;

    JPASetRMtxTVecfromMtx(mat, emt->mGlobalRot, &emt->mGlobalTrs);

    int spState = 0;
    if (sideIdx == 0)
        spState = !(GetKartCtrl()->GetKartStatus(kartNo) & 0x8000);
    else if (sideIdx == 1)
        spState = !(GetKartCtrl()->GetKartStatus(kartNo) & 0x100000000);

    if (spState)
    {
        emt->setGlobalAlpha((10 - mCounter) * 255 / 10);
        mCounter++;
    }

    if (mCounter >= 10)
    {
        GetJPAMgr()->deleteEmtCam(emt, slot);
        JPEffectPerformer::sKartEfctEmts[kartNo].koukasen = NULL;
    }
}

void JPKoukasenEmitterCallBack::draw(JPABaseEmitter *) {}

void JPCharFireEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = ((u32)emt->mpUserWork >> 4) & 0xF;

    if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emt))
    {
        JPEffectPerformer::sKartEfctEmts[kartNo].charFire = NULL;
        return;
    }

    u8 driverIdx = ItemObjMgr::getNowTandemDriverNum(kartNo);
    KartLoader *loader = RCMGetManager()->getKartLoader(kartNo);
    DriverModel *driver = loader->getExModelDriver(driverIdx);
    s16 jointIdx = driver->mModel[0]->getModelData()->getJointName()->getIndex("skl_root");
    MtxPtr psm = driver->mModel[0]->getAnmMtx(jointIdx);

    JPASetRMtxfromMtx(ObjUtility::getKartBodyMtx(kartNo), emt->mGlobalRot);
    emt->mGlobalTrs.x = psm[0][3];
    emt->mGlobalTrs.y = psm[1][3];
    emt->mGlobalTrs.z = psm[2][3];
    emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());

    if (GetKartCtrl()->GetGameStatus(kartNo) & 0x2000)
        emt->playCreateParticle();
    else
        emt->stopCreateParticle();

    if (!GetKartCtrl()->IsBurn(kartNo))
    {
        if (JPEffectPerformer::sKartEfctEmts[kartNo].charFire != NULL)
        {
            emt->stopCreateParticle();
            emt->mMaxFrame = 1;
            JPEffectPerformer::sKartEfctEmts[kartNo].charFire = NULL;
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
    JUT_MINMAX_ASSERT(0, kartNo, 8);

    emt->mGlobalTrs.x = tirePos.x;
    emt->mGlobalTrs.y = tirePos.y;
    emt->mGlobalTrs.z = tirePos.z;
    emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
}

void JPTireSandEmitterCallBack::draw(JPABaseEmitter *) {}

void JPKartWetEmitterCallBack::execute(JPABaseEmitter *emt)
{
    bool unavailableFlg = false;
    int kartNo = emt->mpUserWork;

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

        JGeometry::TVec3f velfrac;
        velfrac.x = vel.x * ratio;
        velfrac.y = vel.y * ratio;
        velfrac.z = vel.z * ratio;

        ptcl->mVelocity.x += velfrac.x;
        ptcl->mVelocity.y += velfrac.y;
        ptcl->mVelocity.z += velfrac.z;
    }
}

void JPTraceParticleCallBack::draw(JPABaseEmitter *, JPABaseParticle *) {}

void JPStartFootSmokeEmitterCallBack::execute(JPABaseEmitter *emt)
{
    int kartNo = emt->mpUserWork;
    KartGamePad *coPad = GetKartCtrl()->GetCoDriveCont(kartNo);
    u32 coBits = *(u32 *)((u8 *)coPad + 0x18);
    if (GetKartCtrl()->getKartPad(kartNo)->mAccelBtn & coBits)
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

static void calcEfct_KartWaterMudSplashSmall(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_landingWtr_wa", pos);
    TstEmitterModeAndClipNotAvailableFlg(kartNo, emt);
    emt = GetJPAMgr()->createEmt("mk_landingWtr_wb", pos);
    emt = GetJPAMgr()->createEmt("mk_landingWtr_wc", pos);
    TstEmitterModeNotAvailableFlg(kartNo, emt);
    TstEmitterModeAndClipNotAvailableFlg(kartNo, emt);
}

static void calcEfct_KartWaterDrainSplashSmall(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_landingWtr_sa", pos);
    TstEmitterModeAndClipNotAvailableFlg(kartNo, emt);
    emt = GetJPAMgr()->createEmt("mk_landingWtr_sb", pos);
    emt = GetJPAMgr()->createEmt("mk_landingWtr_sc", pos);
    TstEmitterModeNotAvailableFlg(kartNo, emt);
    TstEmitterModeAndClipNotAvailableFlg(kartNo, emt);
}

static void calcEfct_KartWaterMudDiddySplashSmall(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_landingWtr_ya", pos);
    TstEmitterModeAndClipNotAvailableFlg(kartNo, emt);
    emt = GetJPAMgr()->createEmt("mk_landingWtr_yb", pos);
    emt = GetJPAMgr()->createEmt("mk_landingWtr_yc", pos);
    TstEmitterModeNotAvailableFlg(kartNo, emt);
    TstEmitterModeAndClipNotAvailableFlg(kartNo, emt);
}

static void calcEfct_KartFallLava(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    static const char *sLavaNames[3] = {"mk_yougan_a", "mk_yougan_b", "mk_yougan_c"};

    for (int i = 0; i < 3; i++)
    {
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(sLavaNames[i], pos);
        if (emt == NULL)
            continue;
        TstEmitterModeNotAvailableFlg(kartNo, emt);
    }
}
static void calcEfct_KartAccelExhaustL(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *sExhaustNames[2] = {"mk_acceleOn_a", "mk_acceleOn_b"};

    for (u8 i = 0; i < 2; i++)
    {
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(sExhaustNames[i], pos);
        if (emt != NULL)
        {
            emt->mEmitterCallback = &JPKinokoDashECB;
            emt->mpUserWork = (sAccelExhaustSpdRatio << 8) | (kartNo << 4);
            if (i == 0)
                emt->mParticleCallback = &JPTracePCB;
        }
    }
}

static void calcEfct_KartAccelExhaustR(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *sExhaustNames[2] = {"mk_acceleOn_a", "mk_acceleOn_b"};

    for (u8 i = 0; i < 2; i++)
    {
        JPABaseEmitter *emt = GetJPAMgr()->createEmt(sExhaustNames[i], pos);
        if (emt != NULL)
        {
            emt->mEmitterCallback = &JPKinokoDashECB;
            emt->mpUserWork = (sAccelExhaustSpdRatio << 8) | (kartNo << 4) | 1;
            if (i == 0)
                emt->mParticleCallback = &JPTracePCB;
        }
    }
}
static void calcEfct_KartSlide(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    const char *name = "mk_slideAt_r";
    if (true)
        name = "mk_slideAt_l";

    name = "mk_slideAt_l";

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (emt != NULL)
        emt->mpUserWork = kartNo;
}

static void calcEfct_KartStar(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    if (JPEffectPerformer::getKartEfctEmts(kartNo)->star != NULL)
    {
        GetJPAMgr()->deleteEmt(JPEffectPerformer::getKartEfctEmts(kartNo)->star);
        JPEffectPerformer::getKartEfctEmts(kartNo)->star = NULL;
    }

    const char *name = "mk_star_a";
    if (RCMGetManager()->getTotalLapNumber() > 2)
        name = "mk_star_b";

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (emt != NULL)
    {
        JPEffectPerformer::getKartEfctEmts(kartNo)->star = emt;
        emt->mEmitterCallback = &JPStarECB;
        emt->mParticleCallback = &JPTracePCB;
        emt->mpUserWork = kartNo | (sStarTraceSpdRatio << 8);
    }
}

static void calcEfct_KartKinokoDash1(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_kinokoDash_a", pos);
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;
    if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emt))
        return;
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPKinokoDashECB;
        emt->mpUserWork = kartNo | (0 << 4);
    }
}

static void calcEfct_KartKinokoDash2(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_kinokoDash_b", pos);
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;
    if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emt))
        return;
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPKinokoDashECB;
        emt->mpUserWork = kartNo | (1 << 4);
    }
}

static void calcEfct_KartKinokoDash3(int kartNo, const JGeometry::TVec3f &pos, u8 sideIdx)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_kinokoDash_e", pos);
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;
    if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emt))
        return;
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPKinokoDashECB2;
        u8 idx = sideIdx & 0xF;
        emt->mpUserWork = kartNo | (idx << 8);
    }
}

static void calcEfct_KartMiniDash1(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_miniDash_a", pos);
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;
    if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emt))
        return;
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPKinokoDashECB;
        emt->mpUserWork = kartNo | (2 << 4);
    }
}

static void calcEfct_KartMiniDash2(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_miniDash_b", pos);
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;
    if (TstEmitterModeAndClipNotAvailableFlg(kartNo, emt))
        return;
    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPKinokoDashECB;
        emt->mpUserWork = kartNo | (3 << 4);
    }
}

static void calcEfct_KartWheelSpin(int kartNo, const JGeometry::TVec3f &pos, u8 sideIdx)
{
    JPABaseEmitter *emt;

    emt = GetJPAMgr()->createEmt("mk_wheelSpin_a", pos);
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;

    if (emt != NULL)
    {
        emt->mEmitterCallback = &JPWheelSpinECB;
        emt->mpUserWork = (kartNo << 4) | (sideIdx & 0xF);
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
    u8 dir = ItemObjMgr::getNowTandemDriverNum(kartNo);
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_ketsumoe", pos);
    emt->mEmitterCallback = &JPCharFireECB;
    emt->mpUserWork = kartNo | (dir << 4);
}

static void calcEfct_HitFireBall(int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    JPABaseEmitter *emts[2] = {NULL, NULL};

    if (TstEmitterModeNotAvailableFlg(kartNo, GetJPAMgr()->createEmt("mk_fbrHit_a", pos)))
        return;
    GetJPAMgr()->createEmt("mk_fbrHit_b", pos);
}
static void calcEfct_KartTireSand(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt;
    for (int i = 0; i < 2; i++)
    {
        if (JPEffectPerformer::getKartEfctEmts(kartNo)->tireSand[i] != NULL)
            continue;

        emt = GetJPAMgr()->createEmt("mk_sunamakiage_a", pos);
        u8 flg = TstEmitterModeNotAvailableFlg(kartNo, emt);
        if (flg)
            continue;

        if (emt != NULL)
        {
            emt->mEmitterCallback = &JPTireSandECB;
            emt->mpUserWork = kartNo | (i << 4);
            JPEffectPerformer::getKartEfctEmts(kartNo)->tireSand[i] = emt;
        }
    }
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
    const char *names[3] = {"mk_wDash_a", "mk_wDash_b", "mk_wDash_c"};

    if (GetStEfctMgr()->tstUseMufllerL(kartNo))
    {
        for (int i = 0; i < 3; i++)
        {
            if (JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashL[i] != NULL)
            {
                GetJPAMgr()->deleteEmt(JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashL[i]);
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashL[i] = NULL;
            }
            JPABaseEmitter *emt = GetJPAMgr()->createEmt(names[i], pos);
            if (TstEmitterModeNotAvailableFlg(kartNo, emt))
                continue;
            if (emt != NULL)
            {
                emt->mEmitterCallback = &JPKinokoDashECB2;
                emt->setFlag(0x40);  // enableDelete
                emt->mpUserWork = (kartNo << 8) | (i << 4);
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashL[i] = emt;
            }
        }
    }

    if (GetStEfctMgr()->tstUseMufllerR(kartNo))
    {
        for (int i = 0; i < 3; i++)
        {
            if (JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashR[i] != NULL)
            {
                GetJPAMgr()->deleteEmt(JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashR[i]);
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashR[i] = NULL;
            }
            JPABaseEmitter *emt = GetJPAMgr()->createEmt(names[i], pos);
            if (TstEmitterModeNotAvailableFlg(kartNo, emt))
                continue;
            if (emt != NULL)
            {
                emt->mEmitterCallback = &JPKinokoDashECB2;
                emt->setFlag(0x40);
                emt->mpUserWork = (kartNo << 8) | (i << 4) | 1;
                JPEffectPerformer::getKartEfctEmts(kartNo)->kinokoDashR[i] = emt;
            }
        }
    }

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
    JPABaseEmitter *emt = NULL;
    for (u8 i = 0; i < 3; i++)
    {
        emt = GetJPAMgr()->createEmt("mk_lose", pos);
        TstEmitterModeNotAvailableFlg(kartNo, emt);

        if (emt != NULL)
        {
            const Mtx &kartMtx = ObjUtility::getKartBodyMtx(kartNo);
            f32 ty = kartMtx[1][3];
            f32 tz = kartMtx[2][3];
            f32 tx = kartMtx[0][3];
            emt->setGlobalTranslation(tx, ty, tz);
            emt->setScale(GetKartCtrl()->getKartBody(kartNo)->getThunder()->getScale());
        }
    }
}

static void calcEfct_KartInvalidity(int kartNo, const JGeometry::TVec3f &pos, u8)
{
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_muteki_a", pos);
    if (emt != NULL)
    {
        emt->mEmitterCallback = NULL;
        emt->mpUserWork = kartNo;
    }
}

static void calcEfct_KartFireSmoke(int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    const char *name = "mk_pusupusu_b";
    if (JPEffectPerformer::sCharSmokeParamSpecialFlg)
        name = "mk_pusupusu_a";

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (emt == NULL)
        return;
    if ((emt->mResource->getDyn()->getResUserWork() & 1) &&
        !TstEmitterModeAndClipNotAvailableFlg(kartNo, emt))
        GetJPAMgr()->deleteEmt(emt);

    emt->mEmitterCallback = &JPPtclTraceECB;
    emt->mParticleCallback = &JPTracePCB;
    emt->mpUserWork = (kartNo << 4) | (sCharFireSmokeSpdRatio << 8);
}

static void calcEfct_BonusKartEfct(int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
    const char *name = flag ? "mk_extra_kira_demo" : "mk_extra_kira";

    JPABaseEmitter *emt = GetJPAMgr()->createEmt(name, pos);
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;

    if (emt != NULL)
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
    JPABaseEmitter *emt = GetJPAMgr()->createEmt("mk_starHit", pos);
    if (TstEmitterModeNotAvailableFlg(kartNo, emt))
        return;
    if (emt != NULL)
        emt->mpUserWork = kartNo;
}

static void calcEfct_StartFootSmoke(int kartNo, const JGeometry::TVec3f &pos, u8)
{
}

void JPEffectPerformer::init()
{
    JPKoukasenECB = new JPKoukasenEmitterCallBack[RCMGetManager()->getConsoleNumber()];

    sCharSmokeParamSpecialFlg = false;
    sStartFootSmokeIndex = 0;

    ECourseID courseID = RCMGetManager()->getCourse()->getCourseID();
    if (courseID == DK_MOUNTAIN || courseID == PEACH_BEACH ||
        (RAINBOW_ROAD <= courseID && courseID <= SHERBET_LAND))
        sCharSmokeParamSpecialFlg = true;

    switch (courseID)
    {
    case BABY_PARK:
    case LUIGI_CIRCUIT:
    case MARIO_CIRCUIT:
    case YOSHI_CIRCUIT:
    case MUSHROOM_BRIDGE:
    case MUSHROOM_CITY:
    case WARIO_COLOSSEUM:
    case SHERBET_LAND:
        sStartFootSmokeIndex = 0;
        break;
    case WALUIGI_STADIUM:
    case DINO_DINO_JUNGLE:
    case DK_MOUNTAIN:
    case RAINBOW_ROAD:
    case DRY_DRY_DESERT:
        sStartFootSmokeIndex = 1;
        break;
    case PEACH_BEACH:
    case DAISY_CRUISER:
        sStartFootSmokeIndex = 2;
        break;
    default:
        break;
    }

    reset();
}

void JPEffectPerformer::reset()
{
    for (int k = 0; k < 8; k++)
    {
        for (int i = 0; i < 22; i++)
        {
            sPCounters[k][i].mCount = 0;
            if (i == 0x13)
                sPCounters[k][i].mPermission = 0xd2;
            else if (i >= 0x10 && i < 0x13)
                sPCounters[k][i].mPermission = 0x3c;
            else
                sPCounters[k][i].mPermission = 0x1e;
        }

        sKartEfctEmts[k].star = NULL;
        sKartEfctEmts[k].waterWave = NULL;
        sKartEfctEmts[k].waterRipple = NULL;
        sKartEfctEmts[k].koukasen = NULL;
        sKartEfctEmts[k].charFire = NULL;
        sKartEfctEmts[k].wet = NULL;
        for (int t = 0; t < 2; t++)
            sKartEfctEmts[k].tireSand[t] = NULL;
        for (int j = 0; j < 3; j++)
        {
            sKartEfctEmts[k].kinokoDashL[j] = NULL;
            sKartEfctEmts[k].kinokoDashR[j] = NULL;
        }
    }

    sPlayerNum = RCMGetManager()->getStatusNumber();
}

void JPEffectPerformer::loadCourseParam(u32 courseID)
{
}

void JPEffectPerformer::calc()
{
    sPCounterObserver.calc();
}

void JPEffectPerformer::deleteKartEfctKoukasen(int kartNo, u8 camNo)
{
    KartEfctEmts *ef = &sKartEfctEmts[kartNo];
    // FIXME: ldup
    JPABaseEmitter *emitter = ef->koukasen;
    if (emitter)
    {
        GetJPAMgr()->deleteEmtCam(emitter, camNo);
        ef->koukasen = NULL;
    }
}

void JPEffectPerformer::deleteKartEfctTireSand(int kartNo)
{
    KartEfctEmts *ef = &sKartEfctEmts[kartNo];
    for (u8 i = 0; i < 2; i++)
    {
        if (ef->tireSand[i])
        {
            GetJPAMgr()->deleteEmt(ef->tireSand[i]);
            ef->tireSand[i] = NULL;
        }
    }
}

Mtx &JPEffectPerformer::getNearestCamPtr(const JGeometry::TVec3f &pos)
{
    int camNum = RCMGetManager()->getConsoleNumber();

    MtxPtr bestMtx = GetKartCtrl()->getKartCam(0)->GetMtx();

    JGeometry::TVec3f camPos = *GetKartCtrl()->getKartCam(0)->GetCameraPos();
    JGeometry::TVec3f d = pos;
    d.sub(camPos);
    f32 best = d.squared();

    for (int i = 1; i < camNum; i++)
    {
        JUT_MINMAX_ASSERT(0, i, 4);
        KartCam *cam = GetKartCtrl()->getKartCam(i);
        camPos = *cam->GetCameraPos();
        d = pos;
        d.sub(camPos);
        f32 dist = d.squared();
        if (dist < best)
        {
            best = dist;
            bestMtx = cam->GetMtx();
        }
    }

    return *(Mtx *)bestMtx;
}

void JPEffectPerformer::setEffect(EffectType type, int kartNo, const JGeometry::TVec3f &pos, u8 flag)
{
}

void JPEffectPerformer::setEffectSplash(CrsGround *, int, const JGeometry::TVec3f &)
{
}

void JPEffectPerformer::setEffectEachCam(EffectType type, int kartNo, u8 flag, u8)
{
}

void JPEffectPerformer::setLandEffect(const JGeometry::TVec3f &pos, CrsGround *ground, int kartNo)
{
}

const char *JPEffectPerformer::getLandingEfctName(CrsGround *ground)
{
}

JPEffectPerformer::KartEfctEmts *JPEffectPerformer::getKartEfctEmts(int index)
{
    JUT_MINMAX_ASSERT(0, index, 8);

    return &sKartEfctEmts[index];
}

void JPEffectPerformer::deleteKartEfctWet(int kartNo)
{
    KartEfctEmts *ef = &sKartEfctEmts[kartNo];
    if (ef->wet)
    {
        GetJPAMgr()->deleteEmt(ef->wet);
        ef->wet = NULL;
    }
}
