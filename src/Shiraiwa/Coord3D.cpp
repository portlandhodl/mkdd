#include "Shiraiwa/Coord3D.h"
#include "JSystem/JGeometry/Matrix.h"
#include "JSystem/JGeometry/Quat.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"

TFreeMove::TFreeMove() {
    _18 = false;
    _1c = 0.0f;
}

void TFreeMove::init(JGeometry::TVec3f *pos, JGeometry::TVec3f *vel, f32 p3) {
    mpPos = pos;
    mpVel = vel;
    _24 = p3;
    _1c = 0.0f;
    reset();
#line 84
    JUT_ASSERT(pos != 0 && vel != 0);
}

void TFreeMove::reset() {
    _18 = false;
    mTagret.zero();
    _20 = 0.0f;
}

void TFreeMove::setTargetPos(const JGeometry::TVec3f &pos, f32 a2, f32 a3) {
    mTagret.set(pos);
    _20 = a2;
    _1c = a3;
    _18 = true;
}

void TFreeMove::setTargetOffset(const JGeometry::TVec3f &offset, f32 a2, f32 a3) {
    mTagret.add(offset, *mpPos);
    _20 = a2;
    _1c = a3;
    _18 = true;
}

void TFreeMove::setTargetOffsetUniform(const JGeometry::TVec3f &offset, int mag) {
    _20 = 0.0f;
    mpVel->scale(1.0f / mag, offset);
    _1c = mpVel->length();
    mTagret.add(offset, *mpPos);
    _18 = true;
}

bool TFreeMove::update() {
    if (_18) {
        if (checkReachTarget()) {
            mpPos->set(mTagret);
            _18 = false;
        } else {
            mpPos->add(*mpVel);
            velUpdate(*mpVel, _20, _1c);
        }
    }
    return _18;
}

bool TFreeMove::checkReachTarget() {
    JGeometry::TVec3f diff;
    diff.sub(mTagret, *mpPos);
    return diff.length() <= _1c;
}

void TFreeMove::velUpdate(JGeometry::TVec3f &, f32, f32) {}

TPathMove::TPathMove(const CrsData::SObject *obj) {
    _8 = 0;
    _0a = 1;
    _1c = false;
    _1d = false;
    mpObj = obj;
}

bool TPathMove::update() {
    if (_1c) {
        updatePos();
        if (checkReachTarget()) {
            if (_1d & 1) {
                setTargetNode();
            } else {
                _1c = false;
            }
        }
    }
    return _1c;
}

void TPathMove::init(JGeometry::TVec3f *, JGeometry::TVec3f *) {}

void TPathMove::reset() {
    _1d = false;
    _1c = false;
    _0a = 1;
    _8 = 0;
}

void TPathMove::setTargetNode() {
    setTargetNode(getNextNode(), _14, _18);
}

void TPathMove::setTargetNode(u16 node, f32 a1, f32 a2) {
    _8 = node;
    _14 = a1;
    _18 = a2;
    _1c = true;
}

void TPathMove::getNodePosition(JGeometry::TVec3f *pos, u16 node) {
    pos->set(RCMGetCourse()->getCrsData()->getPointData(mpObj->pathID, node)->pos);
}

u16 TPathMove::getNextNode() {
    u16 next = _8;
    next += _0a;
    if (next >= RCMGetCourse()->getCrsData()->getPathData(mpObj->pathID)->getPointNumber() - 1) {
        if (!RCMGetCourse()->getCrsData()->getPathData(mpObj->pathID)->isClosed()) {
            _0a = -1;
        } else {
            next = 0;
        }
    } else if (next == 0) {
        _0a = 1;
    }
    return next;
}

void TPathMove::updatePos() {}

bool TPathMove::checkReachTarget() {
    JGeometry::TVec3f nodePos;
    getNodePosition(&nodePos, _8);
    nodePos.sub(*mpPos);
    bool result;
    if (nodePos.squared() < _20) {
        result = true;
    } else {
        result = false;
    }
    return result;
}

TFreeRotate::TFreeRotate() {
    mpMatrix = nullptr;
    _28 = false;
    _3c = false;
}

void TFreeRotate::init(JGeometry::TPos3f *matrix) {
    mpMatrix = matrix;
    _28 = false;
    JUT_ASSERT(mpMatrix !=0);
    mpMatrix->getQuat(_18);
    _8 = _18;
}

void TFreeRotate::setTargetVec(const JGeometry::TVec3f &, const JGeometry::TVec3f &, f32, f32, f32) {}

void TFreeRotate::setTargetVec(const JGeometry::TVec3f &, f32, f32, f32, u8) {}

void TFreeRotate::setTargetQuat(const JGeometry::TQuat4f &q, f32 inc, f32 maxSpd, f32 target) {
    mpMatrix->getQuat(_18);
    if (!_18.equals(q)) {
        _8 = q;
        mSpeedInc = inc;
        mMaxSpeed = maxSpd;
        mSpeed = 0.0f;
        mTarget = target;
        _28 = true;
    }
}

bool TFreeRotate::update() {
    if (_28) {
        if (checkReachTarget()) {
            _28 = false;
        }
        else {
            angleUpdate();
            velUpdate();
        }
    }
    return _28;
}

void TFreeRotate::angleUpdate() {
    JGeometry::TQuat4f q;
    if (_3c & 1) {
        q.slerp(_18, _8, mTarget);
    }
    
    mTarget += mSpeed;
}

void TFreeRotate::velUpdate() {
    if (mSpeed < mMaxSpeed) {
         mSpeed += mSpeedInc;
        if (mSpeed > mMaxSpeed) {
            mSpeed = mMaxSpeed;
        }
    }   
}

bool TFreeRotate::checkReachTarget() {
    return mTarget > 1.0f;
}

void TFreeRotate::setSpeed(f32 speed) {
    if (speed > mMaxSpeed) {
        mMaxSpeed = speed;
    }
    mSpeed = speed;
}

void TFreeRotate::restart() {
    f32 f = mSpeed / (1.0f - mTarget);
    setTargetQuat(_8, f, f, 0.0f);
    _28 = true;
}
