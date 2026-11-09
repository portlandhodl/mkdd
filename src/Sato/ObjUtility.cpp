#include "Sato/ObjUtility.h"
#include "JSystem/JGeometry/Matrix.h"
#include "JSystem/JGeometry/Vec.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/KartLoader.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/ResMgr.h"
#include "Yamamoto/kartCtrl.h"
#include "Kameda/MotorManager.h"
#include "JSystem/JMath/JMath.h"
#include "Sato/stMath.h"
#include "Yamamoto/kartCamera.h"
#include "dolphin/mtx.h"
#include "macros.h"

static const f32 sPoolGuard[4] __attribute__((section(".rodata"))) = {0.0f, 0.0f, 0.0f, 1.0f};

#line 12 "ObjUtility.cpp"
void *ObjUtility::getPtrMainArc(char *path) { return ResMgr::getPtr(ResMgr::mcArcMRAM, path); }

void *ObjUtility::getPtrCourseArc(char *path) { return ResMgr::getPtr(ResMgr::mcArcCourse, path); }

void ObjUtility::getPtrMainLocaleArc(char *path) { ResMgr::getPtr(ResMgr::mcArcMRAMLoc, path); }

void ObjUtility::setRMtx(MtxPtr mtx, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2)
{
    JGeometry::TVec3f mX;
    JGeometry::TVec3f mZ;

    PSVECCrossProduct(&vec1, &vec2, &mX);
    stVecNormalize(&mX);
    PSVECCrossProduct(&mX, &vec1, &mZ);
    stVecNormalize(&mZ);
    stMakeRMtx(mtx, mX, vec1, mZ);
}

void ObjUtility::executeShakeCamera(const JGeometry::TVec3f &inputVec, f32 f1, f32 f2, f32 f3)
{
    for (int kart = 0; kart < RCMGetManager()->mRaceInfo->mKartNum; kart++)
    {
        if (KartCtrl::getKartCtrl()->CheckCamera(kart) != 1)
            continue;

        int camNo = KartCtrl::getKartCtrl()->GetCameraNum(kart);
        JGeometry::TVec3f kartPos;
        getKartPos(kart, &kartPos);

        if (fabs(kartPos.y - inputVec.y) < f3)
        {
            JGeometry::TVec3f diff;
            diff.sub(kartPos, inputVec);
            f32 mag = PSVECMag(&diff);
            f32 length = (1.0f - mag / f1) * f2;
            f32 scale = 0.0f;
            if (length >= 0.0f)
                scale = length;

            if (scale > 0.0f)
            {
                KartCtrl::getKartCtrl()->getKartCam(camNo)->MakeShaker(scale);

                f32 power = 1.0f;
                u8 dur = (int)(10.0f * scale + power);
                if (10 < dur)
                    dur = 10;

                MotorManager::setPowerMotor(kart, power, dur, 10);
            }
        }
    }
}

Mtx &ObjUtility::getKartBodyMtx(int kart)
{






















































































#line 120 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart, 8);
    return (Mtx &)RCMGetManager()->getKartLoader(kart)->getExModelBody()->getBaseTRMtx();
}

void ObjUtility::getKartPos(int kart_index, JGeometry::TVec3f *kartPos)
{








#line 134 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart_index, 8);
    JGeometry::TVec3f *p = kartPos;
    Mtx &mtx = getKartBodyMtx(kart_index);
    p->set(mtx[0][3], mtx[1][3], mtx[2][3]);
}

void ObjUtility::getKartXdir(int kart, JGeometry::TVec3f *kartXdir)
{








#line 149 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart, 8);
    Mtx &mtx = getKartBodyMtx(kart);
    kartXdir->set(mtx[0][0], mtx[1][0], mtx[2][0]);
    stVecNormalize(*kartXdir);
}

void ObjUtility::getKartYdir(int kart, JGeometry::TVec3f *kartYdir)
{








#line 165 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart, 8);
    Mtx &mtx = getKartBodyMtx(kart);
    kartYdir->set(mtx[0][1], mtx[1][1], mtx[2][1]);
    stVecNormalize(*kartYdir);
}

void ObjUtility::getKartZdir(int kart, JGeometry::TVec3f *kartZdir)
{








#line 181 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart, 8);
    Mtx &mtx = getKartBodyMtx(kart);
    kartZdir->set(mtx[0][2], mtx[1][2], mtx[2][2]);
    stVecNormalize(*kartZdir);
}

void ObjUtility::getKartVel(int kart, JGeometry::TVec3f *kartVel)
{







#line 196 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart, 8);
    KartCtrl::getKartCtrl()->GetBodyVel(kart, kartVel);
}

void ObjUtility::getKartEfctVel(int kart, JGeometry::TVec3f *kartEfctVel)
{







#line 209 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart, 8);
    KartCtrl::getKartCtrl()->GetKartEffctVel(kart, kartEfctVel);
}

int ObjUtility::findNearKart(const JGeometry::TVec3f &pos, f32 dist, int exclude)
{
    int result = -1;
    for (int kart = 0; kart < RCMGetManager()->mRaceInfo->mKartNum; kart++)
    {




#line 223 "ObjUtility.cpp"
        JUT_MINMAX_ASSERT(0, kart, 8);
        JGeometry::TVec3f kartPos;
        getKartPos(kart, &kartPos);
        if (PSVECDistance(&pos, &kartPos) <= dist)
        {
            if (exclude < 0 || exclude != kart)
            {
                result = kart;
                break;
            }
        }
    }
    return result;
}

int ObjUtility::findNearKartAngle(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &dir, f32 dist, f32 angle)
{
    int result = -1;
    for (int kart = 0; kart < RCMGetManager()->mRaceInfo->mKartNum; kart++)
    {
        int i = (int)kart;
#line 223 "ObjUtility.cpp"
        JUT_MINMAX_ASSERT(0, kart, 8);
        JGeometry::TVec3f kartPos;
        getKartPos(i, &kartPos);

        if (PSVECDistance(&pos, &kartPos) <= dist)
        {
            const Mtx &mtx = getKartBodyMtx(i);
            JGeometry::TVec3f kpos;
            kpos.set(mtx[0][3], mtx[1][3], mtx[2][3]);

            if (kpos.x == pos.x && kpos.y == pos.y && kpos.z == pos.z)
                continue;

            JGeometry::TVec3f diff;
            diff.sub(pos, kpos);
            JGeometry::TVec3f cross;
            cross.cross(dir, diff);

            f32 mag = PSVECMag(&cross);
            f32 dot = dir.dot(kpos);
            f32 at = JMAAtan2Radian(mag, dot);
            at = fabs(at) * 57.29578f;
            if (at < angle)
            {
                result = i;
                break;
            }
        }
    }
    return result;
}

void ObjUtility::getCamDependLightMtx(u32 viewNo, const JGeometry::TVec3f &pos, Mtx mtx)
{
#line 323 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, viewNo, 4);
    JGeometry::TVec3f *camPos = RCMGetManager()->getCamera(viewNo)->GetCameraPos();

    JGeometry::TVec3f dir;
    dir.sub(pos, *camPos);
    stVecNormalize(dir);

    JGeometry::TVec3f eye;
    JGeometry::TVec3f up;
    up.set(0.0f, 1.0f, 0.0f);
    eye.set(0.0f, 0.0f, 0.0f);
    C_MTXLookAt(mtx, &eye, &up, &dir);
}

bool ObjUtility::isPlayerDriver(const int &kart)
{





























































































































#line 342 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart, 8);
    return RCMGetManager()->getKartInfo(kart)->isPlayerKart();
}

bool ObjUtility::getReflectVec(JGeometry::TVec3f, JGeometry::TVec3f, JGeometry::TVec3f *, bool) { return false; }

void ObjUtility::chkAvailableEnemyByDistande(bool *, f32) {}

namespace
{
char *addName[] = {"a", "b", "c", "d"};
char sConcatReturnFileName[64];
}

const char *ObjUtility::concatSimpleFileName(char *name, const u8 num)
{








































































#line 430 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, num, 4);
    snprintf(sConcatReturnFileName, 64, "%s_%s", name, addName[num]);
    return sConcatReturnFileName;
}

namespace
{
u8 sBlastAddFrame = 3;
u8 sBlastContinueFrame = 50;
f32 sShakeMaxLength = 10000.0f;
f32 sStdBlastRadius = 200.0f;
f32 sAddBlastRadius = 650.0f;
}

BombEffectObj::BombEffectObj()
{
    _4 = 0;
    mBlastBound = new ObjColSphere(0.0f, 1.0f);
    mSphere2 = new ObjColSphere(0.0f, 1.0f);
    _10.z = 0.0f;
    _10.y = 0.0f;
    _10.x = 0.0f;
    mObjSndMgr = new GameAudio::ObjectSoundMgr(&_10, nullptr);
}

void BombEffectObj::reset()
{
    _4 = 0;
    mBlastBound->setRadius(0.0f);
    mBlastBound->setScale(1.0f);
    mSphere2->setRadius(0.0f);
    mSphere2->setScale(1.0f);
}

void BombEffectObj::init(const JGeometry::TPos3f &pos, u8 color, f32 scale)
{
    JGeometry::TPos3f tmp;
    setInitialize(tmp, pos, color, scale);
    startEfct(tmp, color);
}


bool BombEffectObj::execute()
{
    f32 stdR = (f32)sStdBlastRadius;
    f32 addR = (f32)sAddBlastRadius;
    u8 add = (u8)sBlastAddFrame;
    u8 cont = (u8)sBlastContinueFrame;
    return calc(stdR, addR, add, cont);
}


DECL_WEAK BombEffectObj::~BombEffectObj() {}
