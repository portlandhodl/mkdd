#include "Shiraiwa/Coord3D.h"
#include "JSystem/JGeometry/Matrix.h"
#include "JSystem/JGeometry/Quat.h"
#include "JSystem/JUtility/JUTAssert.h"

TFreeMove::TFreeMove() {
    _18 = false;
    _1c = 0.0f;
}

void TFreeMove::init(JGeometry::TVec3f *, JGeometry::TVec3f *, f32) {}

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
    mTagret.add(*mpPos, offset);
    _20 = a2;
    _1c = a3;
    _18 = true;
}

void TFreeMove::setTargetOffsetUniform(const JGeometry::TVec3f &, int) {}

void TFreeMove::update() {}

void TFreeMove::checkReachTarget() {}

void TFreeMove::velUpdate(JGeometry::TVec3f &, f32, f32) {}

TPathMove::TPathMove(const CrsData::SObject *obj) {
    _8 = 0;
    _0a = true;
    _1c = false;
    _1d = false;
    mpObj = obj;
}

void TPathMove::update() {}

void TPathMove::init(JGeometry::TVec3f *, JGeometry::TVec3f *) {}

void TPathMove::reset() {
    _1d = false;
    _1c = false;
    _0a = true;
    _8 = 0;
}

void TPathMove::setTargetNode() {}

void TPathMove::setTargetNode(u16 node, f32 a1, f32 a2) {
    _8 = node;
    _14 = a1;
    _18 = a2;
    _1c = true;
}

void TPathMove::getNodePosition(JGeometry::TVec3f *, u16) {}

void TPathMove::getNextNode() {}

void TPathMove::updatePos() {}

void TPathMove::checkReachTarget() {}

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
