#include "Kaneshige/LightMgr.h"
#include "Kaneshige/RaceMgr.h"
#include "Shiraiwa/Objects/MapObjDemoObj.h"
#include "Sato/ObjUtility.h"

const u32 TMapObjDemoObj::scCameraID[11] = {
    'firs', 'park', 'fron', 'down', 'farr', 'mont',
    'mare', 'item', 'topp', 'endd', 'next'
};
f32 TMapObjDemoObj::sTurnSpeed = 0.03f;

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
        diff.y = 0.0f;
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
    JGeometry::TVec3f dir;
    dir.sub(mKartPos, mPos);
    dir.y = 0.0f;
    dir.normalize();

    JGeometry::TVec3f side;
    JGeometry::TVec3f up;
    JGeometry::TVec3f front;
    mRotMtx.getXDir(side);
    mRotMtx.getYDir(up);
    mRotMtx.getZDir(front);
    if (dir.isZero())
        dir.set(front);

    JGeometry::TVec3f diff;
    diff.sub(dir, front);
    diff.y = 0.0f;
    if (diff.length() > sTurnSpeed)
        diff.setLength(sTurnSpeed);
    if (front.dot(dir) < 0.0f)
        diff.scale(3.0f);
    front.add(diff);
    front.normalize();
    side.cross(up, front);
    side.normalize();
    mRotMtx.setXYZDir(side, up, front);
}

#include "JSystem/JAudio/JASFakeMatch2.h"
