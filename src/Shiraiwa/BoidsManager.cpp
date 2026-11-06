#include "Shiraiwa/BoidsManager.h"

#include "JSystem/JMath/JMath.h"
#include "JSystem/JMath/Inline.h"
#include "mathHelper.h"

TBoidParam TBoidsManager::sParams[3] = {
    {0.0001f, 1.0f, 500.0f, 0.8f, 0.5f, 0.6f, 50.0f},
    {1.0f, 500.0f, 0.5f, 0.5f, 0.6f, 30.0f, 0.001f},
    {0.001f, 1.0f, 500.0f, 0.0f, 2.0f, 0.2f, 20.0f},
};

f32 TBoid::sUpPow = 2.0f;

TBoid::TBoid(const TBoidParam &param) {
    f32 speed = param._18;
    f32 c = param._14;
    _c = c;
    mMaxSpeed = speed;
}

TBoidsManager::TBoidsManager(u32 num, u8 type) {
    mBoids = new TBoid *[num];
    mNum = num;
    u32 t = type;
    if (t >= 3)
        t = 0;
    mParam = &sParams[t];
    for (u32 i = 0; i < mNum; i++) {
        mBoids[i] = new TBoid(*mParam);
    }
}

TBoidsManager::~TBoidsManager() {
    delete[] mBoids;
}

void TBoidsManager::reset(const JGeometry::TVec3<f32> &center) {
    for (u32 i = 0; i < mNum; i++) {}
    mCenter.x = center.x;
    mCenter.y = center.y;
    mCenter.z = center.z;
    mGlbDir.zero();
}

void TBoidsManager::update() {
    getCenter();
    getGlbDir();
    f32 up = mBoids[0]->mPos->y;
    for (u32 i = 1; i < mNum; i++) {
        TBoid *boid = mBoids[i];
        if (boid) {
            JGeometry::TVec3<f32> pow;
            getPow(i, &pow);
            boid->update(pow, up);
        }
    }
}

void TBoidsManager::getCenter() {
    mCenter.zero();
    u32 count = 0;
    for (u32 i = 0; i < mNum; i++) {
        TBoid *boid = mBoids[i];
        if (boid) {
            JGeometry::TVec3<f32> pos;
            pos.x = boid->mPos->x;
            pos.y = boid->mPos->y;
            pos.z = boid->mPos->z;
            mCenter += pos;
            count++;
        }
    }
    if (count != 0)
        mCenter.div(count);
}

void TBoidsManager::getGlbDir() {
    mGlbDir.zero();
    for (u32 i = 0; i < mNum; i++) {
        TBoid *boid = mBoids[i];
        if (boid) {
            JGeometry::TVec3<f32> dir;
            dir.x = boid->mVel->x;
            dir.y = boid->mVel->y;
            dir.z = boid->mVel->z;
            dir.normalize();
            dir.normalize();
            mGlbDir += dir;
        }
    }
    if (!mGlbDir.isZero())
        mGlbDir.normalize();
}

void TBoidsManager::getPow(u32 idx, JGeometry::TVec3<f32> *out) {
    out->zero();
    TBoid *boid = mBoids[idx];
    JGeometry::TVec3<f32> selfPos = *boid->mPos;
    JGeometry::TVec3<f32> centerDir = mCenter - selfPos;

    f32 centerDist = centerDir.length();
    centerDir.normalize();
    f32 sepForce = mParam->_0 * centerDist * centerDist;

    JGeometry::TVec3<f32> dir;
    dir.zero();
    TBoid *nearest = nullptr;
    f32 nearestDist = 0.0f;

    for (u32 i = 0; i < mNum; i++) {
        if (i == idx)
            continue;
        TBoid *other = mBoids[i];
        if (!other)
            continue;
        JGeometry::TVec3<f32> cand = *other->mPos;
        f32 d = cand.squared() + selfPos.squared() - 2.0f * cand.dot(selfPos);
        if (!nearest || d < nearestDist) {
            nearest = other;
            nearestDist = d;
        }
    }

    if (nearest)
        dir.sub(*nearest->mPos, selfPos);
    dir.normalize();

    f32 repulse = mParam->_8 - dir.length();
    if (repulse < 0.0f)
        repulse = 0.0f;
    repulse *= mParam->_4;

    JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
    up.normalize();

    out->x = centerDir.x + dir.x + up.x;
    out->y = mParam->_10 * (centerDir.y + dir.y + up.y);
    out->z = centerDir.z + dir.z + up.z;
    out->normalize();
}
void TBoid::update(const JGeometry::TVec3<f32> &pos, f32 f) {
    JGeometry::TVec3<f32> p = pos;
    if (p.y > mPos->y)
        p.y += sUpPow;

    mVel->add(p);

    f32 mag = mVel->length();
    if (mag > mMaxSpeed)
        mVel->scale(mMaxSpeed / mag);

    JGeometry::TVec3<f32> forward;
    JGeometry::TVec3<f32> right;
    JGeometry::TVec3<f32> up;
    if (mVel->isZero()) {
        forward.set(mMat[0][2], mMat[1][2], mMat[2][2]);
        if (forward.isZero())
            forward.set(0.0f, 0.0f, 1.0f);
    }
    else {
        forward.x = mVel->x;
        forward.y = mVel->y;
        forward.z = mVel->z;
    }
    forward.normalize();

    up.set(0.0f, 1.0f, 0.0f);
    up.normalize();

    right.cross(up, forward);
    right.normalize();

    MtxPtr mat = mMat;
    mat[0][0] = right.x;
    mat[1][0] = right.y;
    mat[2][0] = right.z;
    mat[0][1] = up.x;
    mat[1][1] = up.y;
    mat[2][1] = up.z;
    mat[0][2] = forward.x;
    mat[1][2] = forward.y;
    mat[2][2] = forward.z;
}

#include "JSystem/JAudio/JASFakeMatch2.h"
