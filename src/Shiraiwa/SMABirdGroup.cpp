#include "Shiraiwa/Objects/SMABird.h"

#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

static inline void getNodePos(TPathUtil *util, JGeometry::TVec3f &pos)
{
    u16 node = util->getNextNode();
    pos.set(RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(util->mObj->pathID, (u16)node)->pos);
}

TSMABirdGroup::TSMABirdGroup(u8 num, const CrsData::SObject *obj) : mBirdNum(0), mMaxBirdNum(0)
{
#line 19
    JUT_ASSERT(mMaxBirdNum < 16);
    mMaxBirdNum = num;
    mPathUtil = new TPathUtil(obj);
    mPassMask = 0;
    PSMTXIdentity(mMtx);
    mAllMask = 0;
    for (int i = 0; i < mMaxBirdNum; i++) {
        mAllMask |= 1 << i;
    }
}

TSMABirdGroup::~TSMABirdGroup()
{
    delete mPathUtil;
}

void TSMABirdGroup::reset()
{
    mPassMask = 0;
    mBirdNum = 0;
    mPathUtil->reset();
    replaceMatrix();
}

void TSMABirdGroup::attachBird(TSMABird *bird)
{
    bird->mGroup = this;
    bird->mGroupIdx = mBirdNum;
    JGeometry::TVec3f pos;
    getTargetPos(mBirdNum, pos);
    bird->mBasePos.set(pos);
    mBirdNum++;
}

void TSMABirdGroup::getTargetPos(s16 idx, JGeometry::TVec3f &pos)
{
    JGeometry::TVec3f point;
    JGeometry::TVec3f offset;
    getOffset(idx, offset);
    PSMTXMultVecSR(mMtx, &offset, &offset);
    getNodePos(mPathUtil, point);
    pos.add(point, offset);
    mPassMask |= 1 << idx;
}

void TSMABirdGroup::checkAllPass()
{
    if (mMaxBirdNum != 0 && mPassMask == mAllMask) {
        TPathUtil *util = mPathUtil;
        util->mDistanceDirty = true;
        util->mNode = util->getNextNode();
        replaceMatrix();
        mPassMask = 0;
    }
}

void TSMABirdGroup::replaceMatrix()
{
    JGeometry::TVec3f xDir;
    JGeometry::TVec3f yDir;
    JGeometry::TVec3f zDir;
    mPathUtil->getDirection(zDir);
    yDir.set(0.0f, 1.0f, 0.0f);
    xDir.cross(yDir, zDir);
    xDir.normalize();
    yDir.cross(zDir, xDir);
    yDir.normalize();
    mMtx.setXYZDir(xDir, yDir, zDir);
}

void TSMABirdGroup::getOffset(s16 idx, JGeometry::TVec3f &offset)
{
    offset.set<f32>(mOffset[idx].x, 0.0f, mOffset[idx].y);
}

static inline int getSide(int i, int row)
{
    int col = row;
    if (i % 2 == 0) {
        col = -col;
    }
    return col;
}

void TSMABirdGroup::makeOffsetTable(f32 scale)
{
    int row;
    int col;
    for (int i = 0; i < 16; i++) {
        row = (i + 1) / 2;
        if (i < 1) {
            col = 0;
        } else {
            col = (i % 2 == 0) ? -row : row;
        }
        mOffset[i].x = (int)(scale * (col * 300));
        mOffset[i].y = (int)(scale * (row * -480));
    }
}

TSMABirdGen::TSMABirdGen(const CrsData::SObject &obj, u32 id) : TSmallAnimalGen(obj, id)
{
    mGroupNum = 0;
    mGroups = nullptr;
    makeBird();
}

TSMABirdGen::~TSMABirdGen()
{
    for (int i = 0; i < mGroupNum; i++) {
        delete mGroups[i];
    }
    delete mGroups;
}

void TSMABirdGen::reset()
{
    for (int i = 0; i < mGroupNum; i++) {
        mGroups[i]->reset();
        TPathUtil *util = mGroups[i]->mPathUtil;
        util->mDistanceDirty = true;
        util->mNode = i;
    }
    for (int i = 0; i < mCount; i++) {
        TSMABirdGroup *group = mGroups[i % mGroupNum];
        TSMABird *bird = (TSMABird *)mObjects[i];
        bird->mGroup = group;
        bird->mGroupIdx = group->mBirdNum;
        JGeometry::TVec3f pos;
        group->getTargetPos(group->mBirdNum, pos);
        bird->mBasePos.set(pos);
        group->mBirdNum++;
    }
}


TSMADonkyBirdGen::TSMADonkyBirdGen(const CrsData::SObject &obj) : TSMABirdGen(obj, 0x119d)
{
    makeBird();
}

void TSMADonkyBirdGen::makeBird()
{
    mGroupNum = mObjData->mParam2;
    if (mGroupNum == 0) {
        mGroupNum = 1;
    }
    f32 scale = 0.1f * mObjData->mParam3;
    if (0.0f == scale) {
        scale = 1.0f;
    }
    f32 speedRate = 0.1f * mObjData->mParam4;
    if (0.0f == speedRate) {
        speedRate = 1.0f;
    }
    for (int i = 0; i < mCount; i++) {
        ((TSMABird *)mObjects[i])->mSpeedRate = speedRate;
    }
    mGroups = new TSMABirdGroup *[mGroupNum];
    int num = mCount / mGroupNum;
    int rest = mCount % mGroupNum;
    for (int i = 0; i < mGroupNum; i++) {
        u8 birdNum = num + (i < rest);
        mGroups[i] = new TSMABirdGroup(birdNum, mObjData);
        mGroups[i]->makeOffsetTable(scale);
    }
}
