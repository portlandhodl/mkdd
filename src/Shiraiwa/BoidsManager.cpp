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
static const f32 asmF_00000000 = 0.0f;
static const f32 asmF_3f000000 = 0.5f;
static const f32 asmF_40400000 = 3.0f;
static const f32 asmF_42000000 = 32.0f;

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

asm void TBoidsManager::getGlbDir()
{
    nofralloc
    stwu r1, -0x20(r1)
    li r7, 0x0
    lfs f4, asmF_00000000
    li r4, 0x0
    stfs f4, 0x24(r3)
    stfs f4, 0x20(r3)
    stfs f4, 0x1c(r3)
    b L_802D1EF0
    L_802D1D98:
    lwz r5, 0x8(r3)
    lwzx r5, r5, r4
    cmplwi r5, 0x0
    beq L_802D1EE8
    lwz r6, 0x14(r5)
    lis r5, __float_epsilon@ha
    lfs f5, asmF_42000000
    lfs f6, 0x0(r6)
    lfs f4, 0x4(r6)
    stfs f6, 0x8(r1)
    lfs f6, 0x8(r6)
    stfs f4, 0xc(r1)
    lfs f4, __float_epsilon@l(r5)
    psq_l f8, 0x8(r1), 0, 0
    fmuls f4, f5, f4
    stfs f6, 0x10(r1)
    ps_mul f8, f8, f8
    ps_madd f9, f6, f6, f8
    ps_sum0 f9, f9, f8, f8
    fcmpo cr0, f9, f4
    cror eq, lt, eq
    beq L_802D1E44
    lfs f4, asmF_00000000
    fcmpo cr0, f9, f4
    cror eq, lt, eq
    bne L_802D1E08
    fmr f1, f9
    b L_802D1E2C
    L_802D1E08:
    frsqrte f8, f9
    lfs f6, asmF_3f000000
    lfs f4, asmF_40400000
    frsp f8, f8
    fmuls f5, f8, f8
    fmuls f6, f6, f8
    fnmsubs f4, f9, f5, f4
    fmuls f4, f6, f4
    fmr f1, f4
    L_802D1E2C:
    psq_l f5, 0x8(r1), 0, 0
    psq_l f4, 0x10(r1), 1, 0
    ps_muls0 f5, f5, f1
    ps_muls0 f4, f4, f1
    psq_st f5, 0x8(r1), 0, 0
    psq_st f4, 0x10(r1), 1, 0
    L_802D1E44:
    psq_l f6, 0x8(r1), 0, 0
    lis r5, __float_epsilon@ha
    lfs f2, 0x10(r1)
    ps_mul f6, f6, f6
    lfs f5, asmF_42000000
    lfs f4, __float_epsilon@l(r5)
    ps_madd f9, f2, f2, f6
    fmuls f4, f5, f4
    ps_sum0 f9, f9, f6, f6
    fcmpo cr0, f9, f4
    cror eq, lt, eq
    beq L_802D1EC8
    lfs f4, asmF_00000000
    fcmpo cr0, f9, f4
    cror eq, lt, eq
    bne L_802D1E8C
    fmr f7, f9
    b L_802D1EB0
    L_802D1E8C:
    frsqrte f8, f9
    lfs f6, asmF_3f000000
    lfs f4, asmF_40400000
    frsp f8, f8
    fmuls f5, f8, f8
    fmuls f6, f6, f8
    fnmsubs f4, f9, f5, f4
    fmuls f4, f6, f4
    fmr f7, f4
    L_802D1EB0:
    psq_l f5, 0x8(r1), 0, 0
    psq_l f4, 0x10(r1), 1, 0
    ps_muls0 f5, f5, f7
    ps_muls0 f4, f4, f7
    psq_st f5, 0x8(r1), 0, 0
    psq_st f4, 0x10(r1), 1, 0
    L_802D1EC8:
    psq_l f6, 0x1c(r3), 0, 0
    psq_l f5, 0x8(r1), 0, 0
    psq_l f4, 0x10(r1), 1, 0
    ps_add f5, f6, f5
    psq_st f5, 0x1c(r3), 0, 0
    psq_l f5, 0x24(r3), 1, 0
    ps_add f4, f5, f4
    psq_st f4, 0x24(r3), 1, 0
    L_802D1EE8:
    addi r7, r7, 0x1
    addi r4, r4, 0x4
    L_802D1EF0:
    lwz r0, 0xc(r3)
    cmplw r7, r0
    blt L_802D1D98
    psq_l f4, 0x1c(r3), 0, 0
    lis r4, __float_epsilon@ha
    lfs f3, 0x24(r3)
    ps_mul f4, f4, f4
    lfs f2, asmF_42000000
    lfs f1, __float_epsilon@l(r4)
    ps_madd f5, f3, f3, f4
    fmuls f1, f2, f1
    ps_sum0 f5, f5, f4, f4
    fcmpo cr0, f5, f1
    cror eq, lt, eq
    beq L_802D1F80
    lfs f1, asmF_00000000
    fcmpo cr0, f5, f1
    cror eq, lt, eq
    bne L_802D1F44
    fmr f0, f5
    b L_802D1F68
    L_802D1F44:
    frsqrte f4, f5
    lfs f3, asmF_3f000000
    lfs f1, asmF_40400000
    frsp f4, f4
    fmuls f2, f4, f4
    fmuls f3, f3, f4
    fnmsubs f1, f5, f2, f1
    fmuls f1, f3, f1
    fmr f0, f1
    L_802D1F68:
    psq_l f1, 0x1c(r3), 0, 0
    psq_l f2, 0x24(r3), 1, 0
    ps_muls0 f1, f1, f0
    ps_muls0 f0, f2, f0
    psq_st f1, 0x1c(r3), 0, 0
    psq_st f0, 0x24(r3), 1, 0
    L_802D1F80:
    addi r1, r1, 0x20
    blr
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

#include "JSystem/JAudio/JASFakeMatch2.h"
