#include "Shiraiwa/Coord3DUtil.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Kaneshige/RaceMgr.h"
#include "Shiraiwa/SiUtil.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

extern JGeometry::TVec3f axisY;

s16 TPathWalk::sNearlyReachTargetLength = 300;
s16 TFreeFly::sNearlyReachTargetLength = 500;

TPathUtil::TPathUtil(const CrsData::SObject *obj) {
    mNode = 0;
    mDir = 1;
    mObj = obj;
}

TPathUtil::TPathUtil() {
    mObj = nullptr;
    mNode = 0;
    mDir = 1;
}

void TPathUtil::reset() {
    mDistanceDirty = true;
    mDistance = 0.0f;
    mNode = 0;
    mDir = 1;
}
u16 TPathUtil::getNextNode() {
    int node = (s16)mNode;
    node = (s16)node + (s8)mDir;
    CrsData::PathData *path = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPathData(mObj->pathID);
    if ((s16)node >= path->pointCount) {
        path = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPathData(mObj->pathID);
        if (path->isClosed()) {
            node = 0;
        } else {
            node = mNode - 1;
            mDir = -1;
            node = (s16)node;
        }
    } else if ((s16)node < 0) {
        node = mNode + 1;
        mDir = 1;
        node = (s16)node;
    }
    return node;
}
void TPathUtil::getDirection(JGeometry::TVec3f &dst) {
    JGeometry::TVec3f p1pos;
    JGeometry::TVec3f p2pos;
    JGeometry::TVec3f res;
    CrsData::PointData *p1 = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mObj->pathID, mNode);
    p1pos.x = p1->pos.x;
    p1pos.y = p1->pos.y;
    p1pos.z = p1->pos.z;
    u16 next = getNextNode();
    CrsData::PointData *p2 = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mObj->pathID, next);
    p2pos.x = p2->pos.x;
    p2pos.y = p2->pos.y;
    p2pos.z = p2->pos.z;
    res.sub(p2pos, p1pos);
    res.normalize();
    dst.set(res);
}
f32 TPathUtil::getDistance() {
    if (mDistanceDirty) {
        JGeometry::TVec3f p1pos;
        JGeometry::TVec3f p2pos;
        CrsData::PointData *p1 = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mObj->pathID, mNode);
        p1pos.x = p1->pos.x;
        p1pos.y = p1->pos.y;
        p1pos.z = p1->pos.z;
        u16 next = getNextNode();
        CrsData::PointData *p2 = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mObj->pathID, next);
        p2pos.x = p2->pos.x;
        p2pos.y = p2->pos.y;
        p2pos.z = p2->pos.z;
        JGeometry::TVec3f dir;
        dir.sub(p2pos, p1pos);
        mDistance = dir.length();
        mDistanceDirty = false;
    }
    return mDistance;
}
void TPathUtilInterrupt::reset() {
    mDistanceDirty = true;
    mDistance = 0.0f;
    mNode = 0;
    mDir = 1;
    _20 = false;
}

void TPathUtilInterrupt::getCurrentNodePos(JGeometry::TVec3f *dst) {
    dst->set(mInterruptPos);
}

void TPathUtilInterrupt::getNextNodePos(JGeometry::TVec3f *dst) {
    CrsData::PointData *p = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(mObj->pathID, getNextNode());
    dst->set(p->pos);
}

void TPathUtilInterrupt::updateNode() {
    mNode = getNextNode();
}

TPathWalk::TPathWalk(const CrsData::SObject *obj) {
    mpUtil = new TPathUtilInterrupt(obj);
    mpRotZ = new TFreeRotate();
    mpRotY = new TFreeRotate();
    mpGround = new CrsGround(RaceMgr::sRaceManager->getCourse());
    mTarget.set(obj->position);
}
TPathWalk::~TPathWalk() {
    delete mpUtil;
    delete mpRotZ;
    delete mpRotY;
    delete mpGround;
}

void TPathWalk::init(JGeometry::TVec3f *pos, JGeometry::TVec3f *vel, JGeometry::TPos3f *mtx) {
    mpPos = pos;
    mpMtx = mtx;
    mpVel = vel;
    PSMTXIdentity(mMtxZ.mMtx);
    PSMTXIdentity(mMtxY.mMtx);
    mpRotZ->init(&mMtxZ);
    mpRotY->init(&mMtxY);
    reset();
}
void TPathWalk::reset() {
    mpUtil->reset();
    _94 = false;
    PSMTXIdentity(mMtxZ.mMtx);
    PSMTXIdentity(mMtxY.mMtx);
    JGeometry::TVec3f v(mpMtx->mMtx[0][1], mpMtx->mMtx[1][1], mpMtx->mMtx[2][1]);
    mpRotY->setTargetVec(v, 1.0f, 1.0f, 1.0f, 121);
    mpRotY->update();
    v.set(mpMtx->mMtx[0][2], mpMtx->mMtx[1][2], mpMtx->mMtx[2][2]);
    mpRotZ->setTargetVec(v, 1.0f, 1.0f, 1.0f, 122);
    mpRotZ->update();
    _90 = 0.0f;
    _a0 = 0.0f;
    _98 = 0.0f;
    _a4 = 0.0f;
    _9c = 0.0f;
    mCheckLength = (f32)sNearlyReachTargetLength;
}
void TPathWalk::setParameters(f32 p1, f32 p2, f32 p3, f32 p4, f32 p5) {
    _90 = p1;
    _98 = p2;
    _9c = p3;
    _a0 = p4 == 0.0f ? _98 : p4;
    _a4 = p5 == 0.0f ? _9c : p5;
}
void TPathWalk::setCheckLength(f32 len) {
    if (len <= 0.0f)
        len = (f32)sNearlyReachTargetLength;
    mCheckLength = len;
}
s16 TPathWalk::update() {
    TPathUtilInterrupt *util;
    s16 node = -1;
    if (_94) {
        dirUpdate();
        posUpdate();
        if (checkReachTarget()) {
            util = mpUtil;
            node = (s16)util->mNode;
            if (util->_20) {
                util->_20 = false;
            } else {
                util->mDistanceDirty = true;
                u16 next = util->getNextNode();
                util->mNode = next;
            }
        }
    }
    return node;
}
void TPathWalk::dirUpdate() {
    mpRotZ->update();
    mpRotY->update();
    PSMTXConcat(mMtxY, mMtxZ, *mpMtx);
    if (!mpRotZ->_28) {
        TPathUtilInterrupt *util = mpUtil;
        JGeometry::TVec3f target;
        if (util->_20) {
            target.x = util->mInterruptPos.x;
            target.y = util->mInterruptPos.y;
            target.z = util->mInterruptPos.z;
        } else {
            CrsData::PointData *p = RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(util->mObj->pathID, util->mNode);
            target.x = p->pos.x;
            target.y = p->pos.y;
            target.z = p->pos.z;
        }
        target.sub(*mpPos);
        target.y = 0.0f;
        target.normalize();
        mpRotZ->setTargetVec(target, _a0, _98, 0.0f, 122);
    }
    if (!mpRotY->_28) {
        mpGround->search(*mpPos);
        JGeometry::TVec3f normal;
        mpGround->getNormal(&normal);
        normal.x = 0.0f;
        normal.normalize();
        mpRotY->setTargetVec(normal, _a4, _9c, 0.0f, 121);
    }
}
void TPathWalk::posUpdate() {
    JGeometry::TVec3f dir;
    dir.y = mpMtx->mMtx[1][2];
    dir.x = mpMtx->mMtx[0][2];
    dir.z = mpMtx->mMtx[2][2];
    f32 y = mpPos->y;
    dir.normalize();
    mpVel->scale(_90, dir);
    mpPos->add(*mpVel);
    mpGround->search(*mpPos);
    JGeometry::TVec3f normal;
    mpGround->getNormal(&normal);
    if (mpGround->getAttribute() == CrsGround::Attr_255)
        mpPos->y = y;
    else
        mpPos->y = mpGround->getHeight();
}
bool TPathWalk::checkReachTarget() {
    TPathUtilInterrupt *util = mpUtil;
    JGeometry::TVec3f target;
    bool reach = false;
    if (util->_20)
        target.set(util->mInterruptPos);
    else
        target.set(RaceMgr::sRaceManager->getCourse()->getCrsData()->getPointData(util->mObj->pathID, util->mNode)->pos);
    target.y = 0.0f;
    target.sub(*mpPos);
    if (target.squared() < mCheckLength * mCheckLength)
        reach = true;
    return reach;
}
#pragma dont_inline on
void TPathWalk::forceTurn(const JGeometry::TVec3f &vec) {
    mpRotZ->setTargetVec(vec, 1.0f, 1.0f, 1.0f, 122);
    dirUpdate();
}
void TPathWalk::forceTurn(const JGeometry::TVec3f &a, const JGeometry::TVec3f &b) {
    mpRotY->setTargetVec(a, 1.0f, 1.0f, 1.0f, 121);
    mpRotZ->setTargetVec(b, 1.0f, 1.0f, 1.0f, 122);
    dirUpdate();
}

#pragma dont_inline reset

bool TPathWalkSphere::checkReachTarget() {
    return false;
}

TFreeFly::TFreeFly() {
    mpRotate = nullptr;
    mpPos = nullptr;
    mpVel = nullptr;
    mpMtx = nullptr;
    mpRotate = new TFreeRotate();
}
TFreeFly::~TFreeFly() {
    delete mpRotate;
}
void TFreeFly::init(JGeometry::TVec3f *pos, JGeometry::TVec3f *vel, JGeometry::TPos3f *mtx) {
    mpPos = pos;
    mpMtx = mtx;
    mpVel = vel;
    PSMTXIdentity(mMtx.mMtx);
    mpRotate->init(&mMtx);
}
void TFreeFly::reset() {
    _54 = false;
}
void TFreeFly::setParameters(f32 p1, f32 p2, f32 p3) {
    mSpeed = p1;
    _58 = p2;
    _5c = p3;
}
bool TFreeFly::update() {
    if (_54) {
        if (checkReachTarget())
            _54 = false;
        else {
            dirUpdate();
            posUpdate();
        }
    }
    return _54;
}
void TFreeFly::dirUpdate() {
    mpRotate->update();
    PSMTXCopy(mMtx.mMtx, mpMtx->mMtx);
    if (!mpRotate->_28) {
        JGeometry::TVec3f dir;
        dir.sub(mTarget, *mpPos);
        dir.y = 0.0f;
        dir.normalize();
        mpRotate->setTargetVec(dir, _5c, _58, 0.0f, 122);
    }
}
void TFreeFly::posUpdate() {
    JGeometry::TVec3f fwd;
    JGeometry::TVec3f dir;
    dir.sub(mTarget, *mpPos);
    dir.normalize();
    fwd.y = mpMtx->mMtx[1][2];
    fwd.x = mpMtx->mMtx[0][2];
    fwd.z = mpMtx->mMtx[2][2];
    fwd.normalize();
    fwd.y = dir.y;
    fwd.normalize();
    mpVel->scale(mSpeed, fwd);
    mpPos->add(*mpVel);
}
bool TFreeFly::checkReachTarget() {
    JGeometry::TVec3f dir;
    bool reach = false;
    dir.sub(mTarget, *mpPos);
    if (dir.squared() < (f32)(sNearlyReachTargetLength * sNearlyReachTargetLength))
        reach = true;
    return reach;
}

void TFreeFly::forceTurn(JGeometry::TVec3f &vec) {
    mpRotate->setTargetVec(vec, 1.0f, 1.0f, 1.0f, 122);
    dirUpdate();
}

TFreeFall::TFreeFall() {
}

TFreeFall::TFreeFall(JGeometry::TVec3f *pos, JGeometry::TVec3f *vel, JGeometry::TPos3f *mtx) {
    init(pos, vel, mtx);
}

TFreeFall::~TFreeFall() {
}

void TFreeFall::init(JGeometry::TVec3f *pos, JGeometry::TVec3f *vel, JGeometry::TPos3f *mtx) {
    JUT_ASSERT_F(pos != 0, "%s", "pos != 0");
    JUT_ASSERT_F(vel != 0, "%s", "vel != 0");
    JUT_ASSERT_F(mtx != 0, "%s", "mtx != 0");
    mPos = pos;
    mVel = vel;
    mMtx = mtx;
}
void TFreeFall::reset() {
    _04 = 0.99f;
    _08 = 1.0f;
    _0C = 0.3f;
    _10 = 1.0f;
    _14 = 50.0f;
    _18 = 0.0f;
    _1C = 0;
}
u8 TFreeFall::update() {
    if (_1C)
        roll();
    return _1C;
}
void TFreeFall::roll() {
    JGeometry::TVec3f pos;
    pos.x = mPos->x;
    pos.y = mPos->y;
    pos.z = mPos->z;
    mVel->add(JGeometry::TVec3f(_04, _04, _04));
    JGeometry::TVec3f vec;
    getGravity(&vec);
    addAccel(vec);
    mPos->add(vec);
    reflect(pos);
    rotate();
}
void TFreeFall::reflect(const JGeometry::TVec3f &vec) {
    CrsGround ground(RaceMgr::sRaceManager->getCourse());
    ground.search(*mPos, vec);
    CrsGround::EAttr attr = ground.getAttribute();
    if (attr == CrsGround::Attr_2 || attr == CrsGround::Attr_10 || mPos->y < ground.getHeight() + _18) {
        if (attr == CrsGround::Attr_2) {
            JGeometry::TVec3f normal;
            f32 d = ground.getWallNormal(&normal, nullptr);
            normal.normalize();
            normal.scale(d);
            mPos->add(normal);
        } else {
            JGeometry::TVec3f normal;
            ground.getNormal(&normal);
            mPos->y = ground.getHeight() + _18;
            mVel->x += _08;
            mVel->y += _08;
            mVel->z += _08;
        }
        JGeometry::TVec3f tmp;
        JGeometry::TVec3f out;
        tmp.set(*mVel);
        if (tmp.length() > _0C)
            getReflect(*mPos, &out);
        else
            out.zero();
        tmp.scale(-(1.0f + _10), out);
        mVel->add(tmp);
    }
}
void TFreeFall::getReflect(const JGeometry::TVec3f &vec, JGeometry::TVec3f *out) {
    JGeometry::TVec3f t;
    JGeometry::TVec3f n;
    n.x = mVel->x;
    n.y = mVel->y;
    n.z = mVel->z;
    n.normalize();
    t.cross(vec, n);
    t.normalize();
    n.cross(t, vec);
    n.normalize();
    if (n.squared() == 0.0f)
        n.set(0.0f, 0.0f, 1.0f);
    out->set(n);
}
void TFreeFall::rotate() {
    JGeometry::TPos3f mtx;
    JGeometry::TVec3f vel;
    vel.x = mVel->x;
    vel.y = 0.0f;
    vel.z = mVel->z;
    if (!vel.isZero()) {
        f32 ang = SiUtil::getNormalRange(vel.length(), 0.0f, 50.0f) * _14;
        JGeometry::TVec3f cross;
        JGeometry::TVec3f axis;
        axis.set(0.0f, 1.0f, 0.0f);
        cross.cross(vel, axis);
        JGeometry::TVec3f n;
        n.normalize(cross);
        mtx.setRotate(n, ang);
        PSMTXConcat(mtx, *mMtx, *mMtx);
    }
}
void TFreeFall::addAccel(const JGeometry::TVec3f &acc) {
    CrsGround ground(RaceMgr::sRaceManager->getCourse());
    ground.search(*mPos);
    f32 height = ground.getHeight();
    if (mPos->y - acc.y < height + _18) {
        JGeometry::TVec3f normal;
        ground.getNormal(&normal);
        JGeometry::TVec3f accel;
        accel.scaleAdd(-normal.dot(acc), normal, acc);
        mVel->add(accel);
    } else {
        mVel->add(acc);
    }
}
void TFreeFall::getGravity(JGeometry::TVec3f *out) {
    out->set(0.0f, -1.0f, 0.0f);
    JGeometry::TVec3f accel(_0C, _0C, _0C);
    out->add(accel);
}
void TFreeFallShakeSky::getGravity(JGeometry::TVec3f *out) {
    JGeometry::TVec3f *gravity = (JGeometry::TVec3f *)((u8 *)RaceMgr::sRaceManager->getCourse() + 0x234);
    out->set(*gravity);
    out->normalize();
    out->negate();
    JGeometry::TVec3f accel(_0C, _0C, _0C);
    out->add(accel);
}
void TFreeFallShakeSky::reset() {
    _04 = 0.99f;
    _08 = 1.0f;
    _0C = 0.3f;
    _10 = 1.0f;
    _14 = 50.0f;
    _18 = 0.0f;
    _1C = 0;
    _34 = 5.0f;
    _38 = 0;
    _30 = 0.0f;
    _2C = nullptr;
}
void TFreeFallShakeSky::getReflect(const JGeometry::TVec3f &vec, JGeometry::TVec3f *out) {
    JGeometry::TVec3f t;
    JGeometry::TVec3f n;
    n.x = mVel->x;
    n.y = mVel->y;
    n.z = mVel->z;
    n.normalize();
    t.cross(vec, n);
    t.normalize();
    n.cross(t, vec);
    n.normalize();
    if (n.squared() == 0.0f)
        n.set(0.0f, 0.0f, 1.0f);
    out->set(n);
}
void TFreeFallShakeSky::reflect(const JGeometry::TVec3f &vec) {
    TFreeFall::reflect(vec);
}
void TFreeFallShakeSky::rotate() {
    JGeometry::TPos3f mtx;
    JGeometry::TVec3f vel;
    vel.x = mVel->x;
    vel.y = 0.0f;
    vel.z = mVel->z;
    if (!vel.isZero()) {
        f32 ang = SiUtil::getNormalRange(vel.length(), 0.0f, 50.0f) * _14;
        JGeometry::TVec3f cross;
        JGeometry::TVec3f axis;
        axis.set(0.0f, 1.0f, 0.0f);
        cross.cross(vel, axis);
        JGeometry::TVec3f n;
        n.normalize(cross);
        mtx.setRotate(n, ang);
        PSMTXConcat(mtx, *mMtx, *mMtx);
        _30 += ang;
    }
}

void TFreeFallDonkyRock::reflect(const JGeometry::TVec3f &vec) {
    TFreeFallShakeSky::reflect(vec);
}

void TFreeFallDonkyRock::reset() {
    TFreeFallShakeSky::reset();
}

void TFreeFallDonkyRock::getReflect(const JGeometry::TVec3f &vec, JGeometry::TVec3f *out) {
    TFreeFallShakeSky::getReflect(vec, out);
}

TFreeFallDonkyRock::~TFreeFallDonkyRock() {
}

