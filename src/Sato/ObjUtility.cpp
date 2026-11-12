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
#include "Sato/EffectScreen.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/J3DEffectMgr.h"
#include "Sato/J3DEfctBomb.h"
#include "Sato/JPEffectMgr.h"
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
    stVecNormalize(mX);
    PSVECCrossProduct(&mX, &vec1, &mZ);
    stVecNormalize(mZ);
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

        if (JGeometry::TUtilf::abs(kartPos.y - inputVec.y) < f3)
        {
            JGeometry::TVec3f diff;
            diff.sub(kartPos, inputVec);
            f32 mag = PSVECMag(&diff);
            f32 length = (1.0f - mag / f1) * f2;
            f32 scale = length < 0.0f ? 0.0f : length;

            if (scale > 0.0f)
            {
                KartCtrl::getKartCtrl()->getKartCam(camNo)->MakeShaker(scale);
                u8 dur = 10.0f * scale + 1.0f;
                MotorManager::setPowerMotor(kart, 1.0f, (dur > 10) ? (u8)10 : dur, 10);
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

void ObjUtility::getKartPos(const int kart_index, JGeometry::TVec3f *kartPos)
{








#line 134 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart_index, 8);
    Mtx &mtx = getKartBodyMtx(kart_index);
    kartPos->set(mtx[0][3], mtx[1][3], mtx[2][3]);
}

void ObjUtility::getKartXdir(const int kart, JGeometry::TVec3f *kartXdir)
{








#line 149 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart, 8);
    Mtx &mtx = getKartBodyMtx(kart);
    kartXdir->set(mtx[0][0], mtx[1][0], mtx[2][0]);
    stVecNormalize(*kartXdir);
}

void ObjUtility::getKartYdir(const int kart, JGeometry::TVec3f *kartYdir)
{








#line 165 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart, 8);
    Mtx &mtx = getKartBodyMtx(kart);
    kartYdir->set(mtx[0][1], mtx[1][1], mtx[2][1]);
    stVecNormalize(*kartYdir);
}

void ObjUtility::getKartZdir(const int kart, JGeometry::TVec3f *kartZdir)
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

f32 ObjUtility::getKartDistance(const int kart, const JGeometry::TVec3f &pos)
{
#line 223 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, kart, 8);
    JGeometry::TVec3f kartPos;
    getKartPos(kart, &kartPos);
    return PSVECDistance(&pos, &kartPos);
}

int ObjUtility::findNearKart(const JGeometry::TVec3f &pos, f32 dist, int exclude)
{
    int result = -1;
    for (int kart = 0; kart < RCMGetManager()->mRaceInfo->mKartNum; kart++)
    {
        if (getKartDistance(kart, pos) <= dist)
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

namespace
{
inline f32 getVecDegAngle(const JGeometry::TVec3f &dir, const JGeometry::TVec3f &vec)
{
    JGeometry::TVec3f cross;
    PSVECCrossProduct(&dir, &vec, &cross);
    f32 mag = PSVECMag(&cross);
    f32 at = JMAAtan2Radian(mag, dir.dot(vec));
    return 57.29578f * JGeometry::TUtilf::abs(at);
}
}

int ObjUtility::findNearKartAngle(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &dir, f32 dist, f32 angle)
{
    int result = -1;
    for (int kart = 0; kart < RCMGetManager()->mRaceInfo->mKartNum; kart++)
    {
        if (getKartDistance(kart, pos) <= dist)
        {
            JGeometry::TVec3f diff;
            JGeometry::TVec3f kpos;
            getKartPos(kart, &kpos);
            if (kpos.equals(pos))
                continue;

            diff.sub(kpos, pos);
            if (getVecDegAngle(dir, diff) < angle)
            {
                result = kart;
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

bool ObjUtility::getReflectVec(JGeometry::TVec3f vel, JGeometry::TVec3f normal, JGeometry::TVec3f *out, bool flat)
{
    bool ret = false;
    JGeometry::TVec3f v(vel);
    JGeometry::TVec3f n(normal);
    out->zero();
    if (PSVECMag(&n) > 0.0f)
    {
        v.normalize();
        n.normalize();
        n.scale(-1.0f);
        f32 d = n.dot(v);
        if (d < 0.0f)
            return false;
        v.scale(d);
        JGeometry::TVec3f tmp;
        tmp.sub(v, n);
        out->add(v, tmp);
        if (flat)
            out->y = 0.0f;
        stVecNormalize(*out);
        ret = true;
    }
    return ret;
}

namespace
{
char sConcatReturnFileName[64];
}

void ObjUtility::chkAvailableEnemyByDistande(bool *available, f32 dist)
{
    static int chkCamKart[8];
    int camCnt = 0;
    for (int i = 0; i < RCMGetManager()->mRaceInfo->mKartNum; i++)
    {
        if (KartCtrl::getKartCtrl()->CheckCamera(i))
        {
            chkCamKart[camCnt] = i;
            camCnt++;
        }
    }

    for (int kart = 0; kart < RCMGetManager()->mRaceInfo->mKartNum; kart++)
    {
        bool avail = true;
        if (RCMGetManager()->getKartInfo(kart)->getPad() == nullptr && !KartCtrl::getKartCtrl()->CheckCamera(kart))
        {
            int farCnt = 0;
            JGeometry::TVec3f kartPos;
            getKartPos(kart, &kartPos);
            for (int j = 0; j < camCnt; j++)
            {
                JGeometry::TVec3f camKartPos;
                getKartPos(chkCamKart[j], &camKartPos);
                if (PSVECDistance(&camKartPos, &kartPos) > dist)
                    farCnt++;
            }
            if (farCnt == camCnt)
                avail = false;
        }
        available[kart] = avail;
    }
}

const char *ObjUtility::concatSimpleFileName(char *name, const u8 num)
{








































































    static char *addName[] = {"a", "b", "c", "d"};
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
const char *sBombNormalJPAName[2][3] = {
    {"mk_bomb_a", "mk_bomb_b", "mk_bomb_c"},
    {"mk_aogameBomb_a", "mk_aogameBomb_b", "mk_aogameBomb_c"},
};
const char *sBombMiniJPAName[4][3] = {
    {"mk_bomb_1P_a", "mk_bomb_1P_b", "mk_bomb_1P_c"},
    {"mk_bomb_2P_a", "mk_bomb_2P_b", "mk_bomb_2P_c"},
    {"mk_bomb_3P_a", "mk_bomb_3P_b", "mk_bomb_3P_c"},
    {"mk_bomb_4P_a", "mk_bomb_4P_b", "mk_bomb_4P_c"},
};
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

void BombEffectObj::setInitialize(JGeometry::TPos3f &mtx, const JGeometry::TPos3f &pos, u8 color, f32 scale)
{
    reset();
    JGeometry::TVec3f trans;
    pos.getTrans(trans);
    _10 = trans;
    JGeometry::TVec3f scaleVec(0.8f, 0.8f, 0.8f);
    mBlastBound->setScale(scale);
    mSphere2->setScale(scale);
    scaleVec.scale(scale);
    PSMTXScale(mtx, scaleVec.x, scaleVec.y, scaleVec.z);
    mtx.ref(0, 3) = trans.x;
    mtx.ref(1, 3) = trans.y;
    mtx.ref(2, 3) = trans.z;
    setJPAEfct(trans, scaleVec, color);
    ObjUtility::executeShakeCamera(trans, sShakeMaxLength, 1.0f, 3000.0f);
    EffectScreenMgr::getThis()->flashBombExplosion(color, trans);
    mObjSndMgr->setSe(0x40020);
}

void BombEffectObj::setJPAEfct(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &scale, u8 color)
{
    const char **names;
    if (GeographyObjManager::getGeographyObjManager()->isBombBattle())
        names = sBombMiniJPAName[color];
    else
        names = sBombNormalJPAName[color];

    for (u8 i = 0; i < 3; i++, names++)
    {
        JPABaseEmitter *emt = JPEffectMgr::getEffectMgr()->createEmt(*names, pos);
        if (emt)
        {
            emt->setGlobalScale(scale);
        }
    }
}

void BombEffectObj::startEfct(const JGeometry::TPos3f &mtx, u8 color)
{
    int c = color;
    ((J3DEfctBomb *)J3DEffectMgr::getJ3DEffectMgr()->setEffect(0, (MtxPtr)&mtx, -1))->changeColor(c);
}

bool BombEffectObj::execute()
{
    f32 stdR = (f32)sStdBlastRadius;
    f32 addR = (f32)sAddBlastRadius;
    u8 add = (u8)sBlastAddFrame;
    u8 cont = (u8)sBlastContinueFrame;
    return calc(stdR, addR, add, cont);
}

bool BombEffectObj::calc(f32 stdR, f32 addR, u8 addFrame, u8 contFrame)
{
    bool ret = false;
    f32 rate = (f32)_4 / (f32)addFrame;
    f32 radius = stdR + addR * (rate > 1.0f ? 1.0f : rate);
    mBlastBound->setRadius(radius);
    mSphere2->setRadius(5.0f * radius);
    _4++;
    if (_4 >= addFrame + contFrame)
        ret = true;
    mObjSndMgr->frameWork();
    return ret;
}

namespace
{
JGeometry::TVec3f sFlareScale(0.5f, 0.5f, 0.5f);
}

void FlareEffectObj::init(ObjColBase *&col, const JGeometry::TVec3f &pos)
{
    JPABaseEmitter *emt = JPEffectMgr::getEffectMgr()->createEmt("fireball03", pos);
    if (emt)
        emt->setGlobalScale(sFlareScale);
    emt = JPEffectMgr::getEffectMgr()->createEmt("fireball01", pos);
    if (emt)
        emt->setGlobalScale(sFlareScale);
}


DECL_WEAK BombEffectObj::~BombEffectObj() {}

void CLPointController::reset()
{
    mPrev = nullptr;
    mNext = nullptr;
    _c.zero();
    _18.zero();
    mIsReverse = false;
}

void CLPointController::init_ByKartIndex(int kart)
{
    mPrev = KartCtrl::getKartCtrl()->GetKartCenterPtr(kart);
    mNext = calcNextPoint(mPrev);
    calcCLDir();
}

void CLPointController::init_ByCLPointPtr(CLPoint *point)
{
    mPrev = point->getPrevPoint(0);
    mNext = point;
    calcCLDir();
}

bool CLPointController::chkArriveAtFwPoint(const JGeometry::TVec3f &pos, f32 dist)
{
    bool arrived = false;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f next;
    mNext->getPosition(&next);
    tmp.sub(next, pos);
    JGeometry::TVec3f diff(tmp);
    diff.y = 0.0f;
    if (PSVECMag(&diff) < dist)
        arrived = true;
    return arrived;
}

bool CLPointController::chkIsOverFwPoint(const JGeometry::TVec3f &pos)
{
    bool over = false;
    JGeometry::TVec3f next;
    mNext->getPosition(&next);
    JGeometry::TVec3f v;
    v.sub(pos, next);
    v.y = 0.0f;
    JGeometry::TVec3f cross;
    cross.cross(_c, v);
    f32 mag = PSVECMag(&cross);
    f32 dot = _c.dot(v);
    f32 at = JGeometry::TUtilf::atan2(mag, dot);
    at = JGeometry::TUtilf::abs(at) * 57.29578f;
    if (at < 90.0f)
        over = true;
    return over;
}

bool CLPointController::doNextChkIsOverFwPoint(const JGeometry::TVec3f &pos)
{
    bool over = false;
    JGeometry::TVec3f v;
    JGeometry::TVec3f next;
    mNext->getPosition(&next);
    v.sub(pos, next);
    v.y = 0.0f;
    JGeometry::TVec3f cross;
    cross.cross(_c, v);
    f32 mag = PSVECMag(&cross);
    f32 dot = _c.dot(v);
    f32 at = JGeometry::TUtilf::atan2(mag, dot);
    at = JGeometry::TUtilf::abs(at) * 57.29578f;
    if (at < 90.0f)
        over = true;
    if (over)
        changeNextPoint();
    return over;
}

f32 CLPointController::getLengthFromCLDir(const JGeometry::TVec3f &dir)
{
    JGeometry::TVec3f diff;
    JGeometry::TVec3f prev;
    mPrev->getPosition(&prev);
    diff.sub(dir, prev);
    diff.y = 0.0f;
    return JGeometry::TUtilf::abs(_18.dot(diff));
}

void CLPointController::changeNextPoint()
{
    mPrev = mNext;
    mNext = calcNextPoint(mPrev);
    calcCLDir();
}

void CLPointController::calcCLDir()
{
    JGeometry::TVec3f next;
    JGeometry::TVec3f prev;
    JGeometry::TVec3f up;
    up.set(0.0f, 1.0f, 0.0f);
    mNext->getPosition(&next);
    mPrev->getPosition(&prev);
    _c.sub(next, prev);
    _c.y = 0.0f;
    stVecNormalize(_c);
    PSVECCrossProduct(&up, &_c, &_18);
}

void CLPointController::calcFirstCLbyAngle(const JGeometry::TVec3f &dir, int maxCnt, f32 threshold)
{
    CLPoint *next;
    CLPoint *point;
    int i;
    JGeometry::TVec3f d2;
    JGeometry::TVec3f d1;
    point = mPrev;
    d1.set(0.0f, 0.0f, 0.0f);
    for (i = 0; i < maxCnt; i++)
    {
        next = calcNextPoint(point);
        JGeometry::TVec3f prevPos;
        JGeometry::TVec3f nextPos;
        mNext->getPosition(&prevPos);
        next->getPosition(&nextPos);
        d1.sub(nextPos, prevPos);
        d1.y = 0.0f;
        stVecNormalize(d1);
        d2.sub(prevPos, dir);
        d2.y = 0.0f;
        stVecNormalize(d2);
        if (i == maxCnt - 1 || d2.dot(d1) < threshold)
        {
            mPrev = point;
            mNext = next;
            break;
        }
        point = next;
    }
    calcCLDir();
}

CLPoint *CLPointController::calcNextPoint(CLPoint *point)
{
    int no = findNextPointNo(point);
    if (mIsReverse)
        return point->getPrevPoint(no);
    return point->getNextPoint(no);
}


int CLPointController::findNextPointNo(CLPoint *) { return 0; }

int CLPointController::findRndPointNoExceptForShortCut(CLPoint *point, stRandom *rnd)
{
    int nextNum = mIsReverse ? point->getPrevPointNumber() : point->getNextPointNumber();
    if (nextNum == 1)
        return 0;

#line 1271 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, nextNum, 5);

    int candidates[6];
    int shortcutCnt = 0;
    int candCnt = 0;
    int offset = 0;
    candidates[0] = 0;
    u8 i = 0;
    while (i < nextNum)
    {
        CLPoint *p = mIsReverse ? point->getPrevPoint(i) : point->getNextPoint(i);
        if (p->getPointData()->groupSetting != 0)
            shortcutCnt++;
        else
        {
            candidates[candCnt] = i;
            candCnt++;
            offset += 4;
        }
        i++;
    }

    int ret;
    if (candCnt > 1)
        ret = candidates[rnd->getRandomMax(candCnt - 1)];
    else
    {
        if (shortcutCnt > 0)
            candidates[0] = 0;
        ret = candidates[0];
    }

#line 1302 "ObjUtility.cpp"
    JUT_MINMAX_ASSERT(0, ret, nextNum);
    return ret;
}


void CLPointController::recursiveTargetCalc(CLPoint *point, int groupID, int *pPointNo, u8 *pCnt, bool *pFound)
{
    (*pCnt)++;
    if (*pCnt > 2)
        return;

    int num = tstReverse() ? point->getPrevPointNumber() : point->getNextPointNumber();
    for (int i = 0; i < num; i++)
    {
        if (*pCnt == 1)
            *pPointNo = i;

        CLPoint *p = tstReverse() ? point->getPrevPoint(i) : point->getNextPoint(i);
        int id = p->getGroupID();
        if (id == groupID)
        {
            *pFound = true;
        }
        else
        {
            CLPoint *div = p->getDivPoint();
            if (div)
            {
                recursiveTargetCalc(div, groupID, pPointNo, pCnt, pFound);
                (*pCnt)--;
            }
        }

        if (*pFound)
            break;
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
