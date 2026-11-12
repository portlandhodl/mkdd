#include "Shiraiwa/BoidsManager.h"

#include "JSystem/JMath/JMath.h"
#include "JSystem/JMath/Inline.h"
#include "mathHelper.h"

TBoidParam TBoidsManager::sParams[3] = {
    {0.0001f, 1.0f, 500.0f, 0.8f, 0.5f, 0.6f, 50.0f},
    {0.001f, 1.0f, 500.0f, 0.5f, 0.5f, 0.6f, 30.0f},
    {0.001f, 1.0f, 500.0f, 0.0f, 2.0f, 0.2f, 20.0f},
};

f32 TBoid::sUpPow = 2.0f;

static inline void normalizeDir(JGeometry::TVec3<f32> &v) {
    v.normalize();
}

TBoidsManager::TBoidsManager(u32 num, u8 type) {
    mBoids = new TBoid *[num];
    mNum = num;
    u32 t = type;
    if (t >= 3)
        t = 0;
    mParam = &sParams[t];
    for (u32 i = 0; i < mNum; i++)
        mBoids[i] = new TBoid(mParam->_14, mParam->_18);
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
            normalizeDir(dir);
            dir.normalize();
            mGlbDir.add(dir);
        }
    }
    mGlbDir.normalize();
}

void TBoidsManager::getPow(u32 idx, JGeometry::TVec3<f32> *out) {
    out->zero();
    JGeometry::TVec3<f32> pos;
    pos.x = mBoids[idx]->mPos->x;
    pos.y = mBoids[idx]->mPos->y;
    pos.z = mBoids[idx]->mPos->z;
    JGeometry::TVec3<f32> center;
    center.sub(mCenter, pos);
    f32 dist = center.length();
    center.normalize();
    center.scale(mParam->_0 * (dist * dist));

    JGeometry::TVec3<f32> sep;
    sep.zero();
    TBoid *nearest = nullptr;
    for (u32 i = 0; i < mNum; i++) {
        if (i != idx) {
            TBoid *boid = mBoids[i];
            if (boid) {
                JGeometry::TVec3<f32> d;
                d.x = boid->mPos->x;
                d.y = boid->mPos->y;
                d.z = boid->mPos->z;
                d.sub(pos);
                f32 sq = d.squared();
                if (nearest == nullptr || dist > sq) {
                    nearest = boid;
                    dist = sq;
                }
            }
        }
    }
    JGeometry::TVec3<f32> np;
    np.x = nearest->mPos->x;
    np.y = nearest->mPos->y;
    np.z = nearest->mPos->z;
    sep.sub(pos, np);
    f32 len = mParam->_8 - sep.length();
    if (len < 0.0f)
        len = 0.0f;
    sep.normalize();
    sep.scale(len * mParam->_4);
    if (sep.isZero()) {
        sep.set(1.0f, 1.0f, 1.0f);
        sep.scale(mParam->_4);
    }

    JGeometry::TVec3<f32> glb;
    glb.scale(mParam->_c, mGlbDir);
    out->x = glb.x + (center.x + sep.x);
    out->y = mParam->_10 * (glb.y + (center.y + sep.y));
    out->z = glb.z + (center.z + sep.z);
    out->normalize();
}

void TBoid::update(const JGeometry::TVec3<f32> &pow, f32 up) {
    JGeometry::TVec3<f32> acc;
    acc.scale(_c, pow);
    if (up > mPos->y)
        acc.y += sUpPow;
    mVel->add(acc);
    f32 speed = mVel->length();
    if (speed > mMaxSpeed)
        mVel->scale(mMaxSpeed / speed);
    mPos->add(*mVel);

    JGeometry::TVec3<f32> front;
    if (mVel->isZero()) {
        front.set(mMat[0][2], mMat[1][2], mMat[2][2]);
        if (front.isZero())
            front.set(0.0f, 0.0f, 1.0f);
    } else {
        front.set(*mVel);
    }
    front.normalize();
    JGeometry::TVec3<f32> side;
    JGeometry::TVec3<f32> upDir(0.0f, 1.0f, 0.0f);
    upDir.normalize();
    side.cross(upDir, front);
    side.normalize();
    MtxPtr m = mMat;
    m[0][0] = side.x;
    m[1][0] = side.y;
    m[2][0] = side.z;
    m[0][1] = upDir.x;
    m[1][1] = upDir.y;
    m[2][1] = upDir.z;
    m[0][2] = front.x;
    m[1][2] = front.y;
    m[2][2] = front.z;
}

