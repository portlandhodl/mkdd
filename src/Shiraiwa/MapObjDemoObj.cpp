#include "Kaneshige/LightMgr.h"
#include "Kaneshige/RaceMgr.h"
#include "Shiraiwa/Objects/MapObjDemoObj.h"
#include "Sato/ObjUtility.h"

const u32 TMapObjDemoObj::scCameraID[11] = {
    'firs', 'park', 'fron', 'down', 'farr', 'mont',
    'mare', 'item', 'topp', 'endd', 'next'
};
f32 TMapObjDemoObj::sTurnSpeed = 0.03f;
static const f32 cZero = 0.0f;
static const f32 c32 = 32.0f;
static const f32 cHalf = 0.5f;
static const f32 cThree = 3.0f;


TMapObjDemoObj::TMapObjDemoObj(const CrsData::SObject &obj) : TMapObjHioNode(obj)
{
    flagReset();
}

TMapObjDemoObj::TMapObjDemoObj(u32 id) : TMapObjHioNode(id)
{
    flagReset();
}

TMapObjDemoObj::~TMapObjDemoObj() {}

void TMapObjDemoObj::flagReset()
{
    mFlag = 0;
    mFlag |= 1;
}

void TMapObjDemoObj::reset()
{
    resetObject();
    mGeoObjFlag &= ~0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;

    if (mFlag & 1)
    {
        mKartNo = RCMGetManager()->searchNRankKart(1);

        f32 length = (f32)mObjData->mParam2;
        if (length <= 0.0f)
            length = 1000.0f;
        mLengthSq = length * length;
    }
    else
    {
        mKartNo = -1;
        mLengthSq = 0.0f;
    }

    mLength2 = 1.0e11f;
    mCounter = 0;
}

void TMapObjDemoObj::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x1000200, true);
}

void TMapObjDemoObj::calc()
{
    if (mKartNo != -1)
    {
        ObjUtility::getKartPos(mKartNo, &mKartPos);
        if (checkKartLength())
        {
            turnToKart();
        }
        else
        {
            mKartNo = -1;
        }
    }
    else if (mFlag & 1)
    {
        mKartNo = RCMGetManager()->searchNRankKart(1);
    }

    if (!checkVisible())
    {
        mGeoObjFlag |= 0x20;
        if (mShadowMdl)
            mShadowMdl->clrVisibleAll();
    }
    else
    {
        mGeoObjFlag &= ~0x20;
        if (mShadowMdl)
            mShadowMdl->setVisibleAll();
    }

    if (mShadowMdl)
        moveShadowModel();
}

bool TMapObjDemoObj::checkKartLength()
{
    if (mCounter == 0)
    {
        JGeometry::TVec3f diff;
        diff.sub(mKartPos, mPos);
        mLength2 = diff.squared();
    }

    mCounter++;
    if (mCounter > 30)
        mCounter = 0;

    return mLength2 < mLengthSq;
}

void TMapObjDemoObj::setCurrentViewNo(u32 viewNo)
{
    mModel.setCurrentViewNo(viewNo);

    LightObj *light = LightMgr::getManager()->searchLight('SCN0' + viewNo);
    MtxPtr mtx = light->getEffectMtx();
    mModel.setEffectMtx(mtx, 1);
}

bool TMapObjDemoObj::checkVisible()
{
    u32 tag = RCMGetManager()->mRaceDirector->getDemoCameraTagName();
    s16 param = mObjData->mParam3;
    bool ret = true;

    for (int i = 0; i < 11; i++)
    {
        if (tag == scCameraID[i])
        {
            if (param & (1 << i))
                ret = false;
            break;
        }
    }

    return ret;
}

void TMapObjDemoObj::turnToKart()
{
    JGeometry::TVec3f target;
    target.sub(mKartPos, mPos);
    target.y = 0.0f;
    f32 sq = target.squaredZX();

    if (sq > JGeometry::TUtilf::epsilon())
        target.scale(JGeometry::TUtilf::inv_sqrt(sq));

    JGeometry::TVec3f right(mRotMtx[0][0], mRotMtx[1][0], mRotMtx[2][0]);
    JGeometry::TVec3f up(mRotMtx[0][1], mRotMtx[1][1], mRotMtx[2][1]);
    JGeometry::TVec3f forward(mRotMtx[0][2], mRotMtx[1][2], mRotMtx[2][2]);

    if (target.squaredZX() <= 0.5f * JGeometry::TUtilf::epsilon())
        target = forward;

    JGeometry::TVec3f cross;
    cross.cross(target, forward);
    f32 sin = cross.length();
    if (sin > sTurnSpeed)
    {
        cross.scale(sTurnSpeed / sin);
        forward.add(cross);
    }

    forward.normalize();
    right.cross(up, forward);
    right.normalize();
    up.cross(forward, right);
    up.normalize();

    mRotMtx[0][0] = right.x;
    mRotMtx[1][0] = right.y;
    mRotMtx[2][0] = right.z;
    mRotMtx[0][1] = up.x;
    mRotMtx[1][1] = up.y;
    mRotMtx[2][1] = up.z;
    mRotMtx[0][2] = forward.x;
    mRotMtx[1][2] = forward.y;
    mRotMtx[2][2] = forward.z;
}

#include "JSystem/JAudio/JASFakeMatch2.h"
