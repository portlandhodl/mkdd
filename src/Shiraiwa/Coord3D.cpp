#include "Shiraiwa/Coord3D.h"
#include "JSystem/JGeometry/Matrix.h"
#include "JSystem/JGeometry/Quat.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/RaceMgr.h"
#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"

template <>
inline void JGeometry::TRotation3<JGeometry::TMtx34f>::getQuat(JGeometry::TQuat4f &rDest) const {
    f32 z, y, x;
    x = this->mMtx[0][0];
    y = this->mMtx[1][1];
    z = this->mMtx[2][2];
    f32 len = x + y + z;
    if (len >= 0.0f) {
        len = JGeometry::TUtilf::sqrt(len + JGeometry::TUtilf::one());
        f32 invS = 0.5f / len;

        rDest.w = 0.5f * len;
        rDest.x = (this->mMtx[2][1] - this->mMtx[1][2]) * invS;
        rDest.y = (this->mMtx[0][2] - this->mMtx[2][0]) * invS;
        rDest.z = (this->mMtx[1][0] - this->mMtx[0][1]) * invS;
        return;
    }

    f32 maxDiagxyz = x >= y ? x : y;
    maxDiagxyz = maxDiagxyz >= z ? maxDiagxyz : z;

    if (maxDiagxyz == x) {
        len = JGeometry::TUtilf::sqrt(x - (y + z) + 1.0f);
        f32 invS = 0.5f / len;

        rDest.x = 0.5f * len;
        rDest.y = (this->mMtx[0][1] + this->mMtx[1][0]) * invS;
        rDest.z = (this->mMtx[2][0] + this->mMtx[0][2]) * invS;
        rDest.w = (this->mMtx[2][1] - this->mMtx[1][2]) * invS;
    } else if (maxDiagxyz == y) {
        f32 s = JGeometry::TUtilf::sqrt((y - (z + x)) + 1.0f);
        f32 invS = 0.5f / s;

        rDest.y = 0.5f * s;
        rDest.z = (this->mMtx[1][2] + this->mMtx[2][1]) * invS;
        rDest.x = (this->mMtx[0][1] + this->mMtx[1][0]) * invS;
        rDest.w = (this->mMtx[0][2] - this->mMtx[2][0]) * invS;
    } else {
        f32 sum = x;
        sum += y;
        f32 s = JGeometry::TUtilf::sqrt((z - sum) + 1.0f);
        f32 invS = 0.5f / s;

        rDest.z = 0.5f * s;
        rDest.x = (this->mMtx[2][0] + this->mMtx[0][2]) * invS;
        rDest.y = (this->mMtx[1][2] + this->mMtx[2][1]) * invS;
        rDest.w = (this->mMtx[1][0] - this->mMtx[0][1]) * invS;
    }
}

inline f32 JMath::TAsinAcosTable<1024, f32>::acos_(f32 x) const {
    if (x >= 1.0f) {
        return 0.0f;
    } else if (x <= -1.0f) {
        return TAngleConstant_<f32>::RADIAN_DEG180();
    } else if (x < 0.0f) {
        return mTable[(u32)(-x * 1023.5f)] + TAngleConstant_<f32>::RADIAN_DEG090();
    } else {
        return TAngleConstant_<f32>::RADIAN_DEG090() - mTable[(u32)(x * 1023.5f)];
    }
}

template <>
inline void JGeometry::TQuat4<f32>::slerp(const JGeometry::TQuat4<f32> &other, f32 t) {
    const f32 ratio = t;
    f32 angle;
    f32 sinAngle;
    f32 weight;
    JGeometry::TQuat4f q1;
    q1.normalize(*this);

    JGeometry::TQuat4f q2;
    q2.normalize(other);

    f32 dot = q1.squared(q2);
    bool reverse;
    if (dot < 0.0f) {
        dot = -dot;
        reverse = true;
    } else {
        reverse = false;
    }

    if (1.0f - dot <= JGeometry::TUtilf::epsilon()) {
        weight = 1.0f - ratio;
    } else {
        angle = JMath::asinAcosTable_.acos_(dot);
        sinAngle = sin(angle);
        weight = (f32)sin((1.0f - ratio) * angle) / sinAngle;
        t = (f32)sin(ratio * angle) / sinAngle;
    }

    if (reverse) {
        t = -t;
    }

    this->set(weight * q1.x + t * q2.x, weight * q1.y + t * q2.y,
              weight * q1.z + t * q2.z, weight * q1.w + t * q2.w);
}

TFreeMove::TFreeMove() {
    _18 = false;
    _1c = -1.0f;
}

void TFreeMove::init(JGeometry::TVec3f *pos, JGeometry::TVec3f *vel, f32 p3) {
    mpPos = pos;
    mpVel = vel;
    _24 = p3;
    _1c = -1.0f;
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
    mTagret.add(*mpPos, offset);
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

bool TFreeMove::velUpdate(JGeometry::TVec3f &vel, f32 speed, f32 maxSpeed) {
    if (speed == 0.0f) return false;
    bool result = false;
    if (maxSpeed <= 0.0f || vel.length() < maxSpeed) {
        JGeometry::TVec3f diff;
        diff.sub(mTagret, *mpPos);
        diff.normalize();
        diff.scale(speed);
        vel.add(diff);
        if (maxSpeed > 0.0f && vel.length() > maxSpeed) {
            vel.normalize();
            vel.scale(maxSpeed);
        }
        result = true;
    }
    return result;
}

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

void TPathMove::init(JGeometry::TVec3f *pos, JGeometry::TVec3f *vel) {
    mpPos = pos;
    mpVel = vel;
    reset();
#line 448
    JUT_ASSERT(pos != 0 && vel != 0);
}

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

void TPathMove::updatePos() {
    JGeometry::TVec3f nodePos;
    JGeometry::TVec3f diff;
    getNodePosition(&nodePos, _8);
    diff.sub(nodePos, *mpPos);
    diff.normalize();
    diff.scale(_14);
    mpVel->add(diff);
    if (mpVel->length() > _18) {
        mpVel->normalize();
        mpVel->scale(_18);
    }
    mpPos->add(*mpVel);
}

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
#line 712
    JUT_ASSERT(mpMatrix != 0);
    mpMatrix->getQuat(_18);
    _8 = _18;
}

void TFreeRotate::setTargetVec(const JGeometry::TVec3f &from, const JGeometry::TVec3f &to, f32 inc, f32 maxSpd, f32 target) {
    mpMatrix->getQuat(_18);
    JGeometry::TQuat4f q;
    q.setRotate(from, to);
    PSQUATMultiply(&q, &_18, &_8);
    mSpeedInc = inc;
    mMaxSpeed = maxSpd;
    mSpeed = 0.0f;
    mTarget = target;
    _28 = true;
}

void TFreeRotate::setTargetVec(const JGeometry::TVec3f &vec, f32 inc, f32 maxSpd, f32 target, u8 axis) {
    JGeometry::TVec3f dir;
    switch (axis) {
    case 'X':
    case 'x':
        mpMatrix->getXDir(dir);
        break;
    case 'Y':
    case 'y':
        mpMatrix->getYDir(dir);
        break;
    case 'Z':
    case 'z':
    default:
        mpMatrix->getZDir(dir);
        break;
    }
    dir.normalize();
    setTargetVec(dir, vec, inc, maxSpd, target);
}

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
    } else {
        q.x = _18.x + mTarget * (_8.x - _18.x);
        q.y = _18.y + mTarget * (_8.y - _18.y);
        q.z = _18.z + mTarget * (_8.z - _18.z);
        q.w = _18.w + mTarget * (_8.w - _18.w);
    }

    f32 sq = q.squared();
    if (sq <= JGeometry::TUtilf::epsilon()) {
        q.set(0.0f, 0.0f, 0.0f, 1.0f);
    } else {
        q.scale(JGeometry::TUtilf::inv_sqrt(sq));
    }

    mpMatrix->setQuat(q);
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
