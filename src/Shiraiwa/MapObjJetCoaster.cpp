#include "Shiraiwa/Objects/MapObjJetCoaster.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Sato/AnmController.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ObjUtility.h"

#include <JSystem/JAudio/JASFakeMatch2.h>

f32 TMapObjJetCoaster::sFrameRate = 1.0f;
J3DAnmTransform *TMapObjJetCoaster::sJetCoasterBckAnmTrans;
J3DMtxCalc *TMapObjJetCoaster::sJetCoasterBckMtxCalc;

int TMapObjJetCoasterHead::sJointNo = -1;
int TMapObjJetCoasterBody::sJointNo = -1;

template <typename T>
inline T getLoopValue(T val, T min, int max)
{
    if (min >= max)
        return min;
    while (val < min)
        val += max - min;
    while (val >= max)
        val -= max - min;
    return val;
}

TMapObjJetCoaster::TMapObjJetCoaster(const CrsData::SObject &obj) : TMapObjHioNode(obj)
{
    NewAnmCtrl();
}

TMapObjJetCoaster::TMapObjJetCoaster(u32 id) : TMapObjHioNode(id)
{
    NewAnmCtrl();
}

TMapObjJetCoaster::~TMapObjJetCoaster() {}

void TMapObjJetCoaster::reset()
{
    GeographyObj::resetObject();
    mPos.zero();
    mCurPos.zero();
    mPrevPos.zero();
    clrObjFlagHidding();
    setObjFlagCheckItemHitting();
    setAllCheckKartHitFlag();
    mAnmCtrl->Reset();
    mAnmCtrl->getFrameCtrl(0)->setRate(sFrameRate);
    _58 = 4;
    mIsFirst = 1;
}

void TMapObjJetCoaster::setAnimationFrame(f32 frame)
{
    mAnmCtrl->getFrameCtrl(0)->setFrame(frame);
}

void TMapObjJetCoaster::loadAnimation()
{
    J3DModelData *modelData = mModel.getModelData();
    void *bck = ObjUtility::getPtrCourseArc("/Objects/Babyjet.bck");
    J3DAnmObjTrans::setupTransAnmData(&sJetCoasterBckAnmTrans, &sJetCoasterBckMtxCalc, modelData, bck);
}

void TMapObjJetCoaster::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createModel(heap, viewNo, 0);
    getAnmCtrl()->InitRegistration(1, &mModel);
    getAnmCtrl()->RegisterTrans(0, sJetCoasterBckAnmTrans, sJetCoasterBckMtxCalc);
    getAnmCtrl()->getFrameCtrl(0)->setAttribute(2);
    setJointNo();
}

__declspec(weak) void TMapObjJetCoaster::setJointNo() {}

void TMapObjJetCoaster::createColModel(J3DModelData *)
{
    createBoundsSphere(360.0f, mScale.y);
}

void TMapObjJetCoaster::getColPos(JGeometry::TVec3f *pos, long jointNo)
{
    JGeometry::TVec3f dir;
    if (mIsFirst) {
        pos->zero();
        dir.set(0.0f, 1.0f, 0.0f);
    }
    else {
        Mtx &mtx = mModel.getModel()->getAnmMtx(jointNo);
        pos->set(mtx[0][3], mtx[1][3], mtx[2][3]);
        dir.set(mtx[0][1], mtx[1][1], mtx[2][1]);
    }

    dir.normalize();
    dir.scale(200.0f);
    pos->add(dir);
}

void TMapObjJetCoaster::calc()
{
    JGeometry::TVec3f pos;
    getColPos(&pos, getJointNo());
    setColObjPos(pos, 0);

    mPrevPos.set(mCurPos);
    mCurPos.set(pos);
    if (mIsFirst)
        mIsFirst = 0;
}

__declspec(weak) int TMapObjJetCoaster::getJointNo() { return -1; }

TMapObjJetCoasterHead::TMapObjJetCoasterHead(const CrsData::SObject &obj) : TMapObjJetCoaster(obj)
{
    u8 bodyNum = obj.mParam1;
    mFrame = obj.mParam2;
    if (mFrame < 0.0f)
        mFrame = 0.0f;
    mCurPos.zero();
    mCoasterSound = new GameAudio::CoasterSoundMgr(&mCurPos, nullptr);
    createBody(bodyNum);
}

void TMapObjJetCoasterHead::createBody(u8 num)
{
#line 217
    JUT_ASSERT(num < scBodyMax);
    mBodyNum = num != 0 ? num : 1;
    mBodies = new TMapObjJetCoaster *[mBodyNum];
    for (int i = 0; i < mBodyNum; i++) {
        mBodies[i] = (TMapObjJetCoaster *)GeographyObjManager::getGeographyObjManager()->createSubObj(0xce7);
    }
}

void TMapObjJetCoasterHead::reset()
{
    TMapObjJetCoaster::reset();
    setAnimationFrame(mFrame);
    for (int i = 0; i < mBodyNum; i++) {
        f32 ofs = 8.0f * (i + 1);
        J3DFrameCtrl *ctrl = mAnmCtrl->getFrameCtrl(0);
        f32 frame = getLoopValue(mFrame - ofs, 0.0f, ctrl->getEnd());
        mBodies[i]->setAnimationFrame(frame);
    }
}

void TMapObjJetCoasterHead::calc()
{
    TMapObjJetCoaster::calc();

    JGeometry::TVec3f vel;
    vel.sub(mCurPos, mPrevPos);
    f32 speed;
    if (mPrevPos.isZero() || mCurPos.isZero())
        speed = 0.0f;
    else
        speed = vel.length();
    mCoasterSound->frameWork(speed);
}

TMapObjJetCoasterHead::~TMapObjJetCoasterHead()
{
    delete[] mBodies;
}

void TMapObjJetCoasterHead::setJointNo()
{
    sJointNo = mModel.getModelData()->getJointName()->getIndex("yoshi_jet_head1");
}

void TMapObjJetCoasterBody::setJointNo()
{
    sJointNo = mModel.getModelData()->getJointName()->getIndex("yoshi_jet_body1");
}
