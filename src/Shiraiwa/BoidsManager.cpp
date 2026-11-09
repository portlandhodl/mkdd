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
extern u8 __vt__13TBoidsManager[];
extern u8 __vt__5TBoid[];
extern TBoidParam sParams__13TBoidsManager[];
extern void *__nw__FUl(u32);
extern void *__nwa__FUl(u32);
extern f32 sUpPow__5TBoid;
static const f32 asmF_00000000 = 0.0f;
static const f32 asmF_3f000000 = 0.5f;
static const f32 asmF_3f800000 = 1.0f;
static const f32 asmF_40400000 = 3.0f;
static const f32 asmF_42000000 = 32.0f;

TBoid::TBoid(const TBoidParam &param) {
    f32 speed = param._18;
    f32 c = param._14;
    _c = c;
    mMaxSpeed = speed;
}

asm TBoidsManager::TBoidsManager(u32 num, u8 type)
{
    nofralloc
    stwu r1, -0x20(r1)
    mflr r0
    lis r6, __vt__13TBoidsManager@ha
    stw r0, 0x24(r1)
    addi r0, r6, __vt__13TBoidsManager@l
    stmw r28, 0x10(r1)
    mr r28, r3
    mr r30, r4
    mr r29, r5
    stw r0, 0x0(r3)
    slwi r3, r4, 2
    bl __nwa__FUl
    stw r3, 0x8(r28)
    clrlwi r0, r29, 24
    cmplwi r0, 0x3
    stw r30, 0xc(r28)
    blt L_802D1AE8
    li r0, 0x0
    L_802D1AE8:
    mulli r5, r0, 0x1c
    lis r4, sParams__13TBoidsManager@ha
    lis r3, __vt__5TBoid@ha
    addi r0, r4, sParams__13TBoidsManager@l
    add r0, r0, r5
    addi r30, r3, __vt__5TBoid@l
    stw r0, 0x4(r28)
    li r29, 0x0
    li r31, 0x0
    b L_802D1B48
    L_802D1B10:
    li r3, 0x1c
    bl __nw__FUl
    cmplwi r3, 0x0
    beq L_802D1B38
    lwz r4, 0x4(r28)
    lfs f1, 0x18(r4)
    lfs f0, 0x14(r4)
    stw r30, 0x0(r3)
    stfs f0, 0xc(r3)
    stfs f1, 0x8(r3)
    L_802D1B38:
    lwz r4, 0x8(r28)
    addi r29, r29, 0x1
    stwx r3, r4, r31
    addi r31, r31, 0x4
    L_802D1B48:
    lwz r0, 0xc(r28)
    cmplw r29, r0
    blt L_802D1B10
    mr r3, r28
    lmw r28, 0x10(r1)
    lwz r0, 0x24(r1)
    mtlr r0
    addi r1, r1, 0x20
    blr
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

asm void TBoidsManager::getPow(u32 idx, JGeometry::TVec3<f32> *out)
{
    nofralloc
    stwu r1, -0x110(r1)
    mflr r0
    stw r0, 0x114(r1)
    stfd f31, 0x100(r1)
    psq_st f31, 0x108(r1), 0, 0
    stfd f30, 0xf0(r1)
    psq_st f30, 0xf8(r1), 0, 0
    stfd f29, 0xe0(r1)
    psq_st f29, 0xe8(r1), 0, 0
    stfd f28, 0xd0(r1)
    psq_st f28, 0xd8(r1), 0, 0
    stfd f27, 0xc0(r1)
    psq_st f27, 0xc8(r1), 0, 0
    stfd f26, 0xb0(r1)
    psq_st f26, 0xb8(r1), 0, 0
    stfd f25, 0xa0(r1)
    psq_st f25, 0xa8(r1), 0, 0
    stfd f24, 0x90(r1)
    psq_st f24, 0x98(r1), 0, 0
    stfd f23, 0x80(r1)
    psq_st f23, 0x88(r1), 0, 0
    stfd f22, 0x70(r1)
    psq_st f22, 0x78(r1), 0, 0
    stfd f21, 0x60(r1)
    psq_st f21, 0x68(r1), 0, 0
    stmw r29, 0x54(r1)
    lfs f0, asmF_00000000
    mr r29, r3
    mr r30, r4
    slwi r0, r4, 2
    stfs f0, 0x8(r5)
    addi r3, r1, 0x38
    mr r31, r5
    stfs f0, 0x4(r5)
    stfs f0, 0x0(r5)
    lwz r4, 0x8(r29)
    psq_l f2, 0x10(r29), 0, 0
    lwzx r4, r4, r0
    lwz r4, 0x10(r4)
    lfs f0, 0x0(r4)
    lfs f1, 0x4(r4)
    stfs f0, 0x44(r1)
    lfs f0, 0x8(r4)
    stfs f1, 0x48(r1)
    psq_l f1, 0x44(r1), 0, 0
    stfs f0, 0x4c(r1)
    ps_sub f1, f2, f1
    psq_l f0, 0x4c(r1), 1, 0
    psq_st f1, 0x0(r3), 0, 0
    psq_l f1, 0x18(r29), 1, 0
    ps_sub f0, f1, f0
    psq_st f0, 0x8(r3), 1, 0
    bl PSVECMag
    psq_l f3, 0x38(r1), 0, 0
    lis r3, __float_epsilon@ha
    lfs f24, 0x40(r1)
    ps_mul f3, f3, f3
    lfs f2, asmF_42000000
    lfs f0, __float_epsilon@l(r3)
    ps_madd f5, f24, f24, f3
    fmuls f0, f2, f0
    ps_sum0 f5, f5, f3, f3
    fcmpo cr0, f5, f0
    cror eq, lt, eq
    beq L_802D20E0
    lfs f0, asmF_00000000
    fcmpo cr0, f5, f0
    cror eq, lt, eq
    bne L_802D20A4
    fmr f28, f5
    b L_802D20C8
    L_802D20A4:
    frsqrte f4, f5
    lfs f3, asmF_3f000000
    lfs f0, asmF_40400000
    frsp f4, f4
    fmuls f2, f4, f4
    fmuls f3, f3, f4
    fnmsubs f0, f5, f2, f0
    fmuls f0, f3, f0
    fmr f28, f0
    L_802D20C8:
    psq_l f2, 0x38(r1), 0, 0
    psq_l f0, 0x40(r1), 1, 0
    ps_muls0 f2, f2, f28
    ps_muls0 f0, f0, f28
    psq_st f2, 0x38(r1), 0, 0
    psq_st f0, 0x40(r1), 1, 0
    L_802D20E0:
    lwz r3, 0x4(r29)
    fmuls f2, f1, f1
    psq_l f4, 0x38(r1), 0, 0
    li r5, 0x0
    lfs f3, 0x0(r3)
    li r6, 0x0
    lfs f0, asmF_00000000
    fmuls f31, f3, f2
    psq_l f2, 0x40(r1), 1, 0
    stfs f0, 0x34(r1)
    li r3, 0x0
    ps_muls0 f3, f4, f31
    stfs f0, 0x30(r1)
    ps_muls0 f2, f2, f31
    stfs f0, 0x2c(r1)
    psq_st f3, 0x38(r1), 0, 0
    psq_st f2, 0x40(r1), 1, 0
    b L_802D21AC
    L_802D2128:
    cmplw r6, r30
    beq L_802D21A4
    lwz r4, 0x8(r29)
    lwzx r7, r4, r3
    cmplwi r7, 0x0
    beq L_802D21A4
    lwz r4, 0x10(r7)
    cmplwi r5, 0x0
    psq_l f4, 0x4c(r1), 1, 0
    lfs f0, 0x8(r4)
    lfs f3, 0x0(r4)
    lfs f2, 0x4(r4)
    stfs f0, 0x28(r1)
    psq_l f5, 0x44(r1), 0, 0
    psq_l f0, 0x28(r1), 1, 0
    stfs f3, 0x20(r1)
    ps_sub f0, f0, f4
    stfs f2, 0x24(r1)
    psq_l f2, 0x20(r1), 0, 0
    psq_st f0, 0x28(r1), 1, 0
    ps_sub f0, f2, f5
    lfs f27, 0x28(r1)
    ps_mul f2, f0, f0
    psq_st f0, 0x20(r1), 0, 0
    ps_madd f0, f27, f27, f2
    ps_sum0 f0, f0, f2, f2
    beq L_802D219C
    fcmpo cr0, f1, f0
    ble L_802D21A4
    L_802D219C:
    mr r5, r7
    fmr f1, f0
    L_802D21A4:
    addi r6, r6, 0x1
    addi r3, r3, 0x4
    L_802D21AC:
    lwz r0, 0xc(r29)
    cmplw r6, r0
    blt L_802D2128
    lwz r4, 0x10(r5)
    addi r3, r1, 0x2c
    psq_l f3, 0x44(r1), 0, 0
    lfs f2, 0x0(r4)
    lfs f1, 0x4(r4)
    lfs f0, 0x8(r4)
    stfs f2, 0x14(r1)
    psq_l f2, 0x4c(r1), 1, 0
    stfs f1, 0x18(r1)
    stfs f0, 0x1c(r1)
    psq_l f1, 0x14(r1), 0, 0
    psq_l f0, 0x1c(r1), 1, 0
    ps_sub f1, f3, f1
    ps_sub f0, f2, f0
    psq_st f1, 0x0(r3), 0, 0
    psq_st f0, 0x8(r3), 1, 0
    bl PSVECMag
    lwz r3, 0x4(r29)
    lfs f0, asmF_00000000
    lfs f2, 0x8(r3)
    fsubs f5, f2, f1
    fcmpo cr0, f5, f0
    bge L_802D2218
    fmr f5, f0
    L_802D2218:
    psq_l f2, 0x2c(r1), 0, 0
    lis r3, __float_epsilon@ha
    lfs f23, 0x34(r1)
    ps_mul f2, f2, f2
    lfs f1, asmF_42000000
    lfs f0, __float_epsilon@l(r3)
    ps_madd f4, f23, f23, f2
    fmuls f0, f1, f0
    ps_sum0 f4, f4, f2, f2
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    beq L_802D229C
    lfs f0, asmF_00000000
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    bne L_802D2260
    fmr f26, f4
    b L_802D2284
    L_802D2260:
    frsqrte f3, f4
    lfs f2, asmF_3f000000
    lfs f0, asmF_40400000
    frsp f3, f3
    fmuls f1, f3, f3
    fmuls f2, f2, f3
    fnmsubs f0, f4, f1, f0
    fmuls f0, f2, f0
    fmr f26, f0
    L_802D2284:
    psq_l f1, 0x2c(r1), 0, 0
    psq_l f0, 0x34(r1), 1, 0
    ps_muls0 f1, f1, f26
    ps_muls0 f0, f0, f26
    psq_st f1, 0x2c(r1), 0, 0
    psq_st f0, 0x34(r1), 1, 0
    L_802D229C:
    lwz r4, 0x4(r29)
    lis r3, __float_epsilon@ha
    psq_l f3, 0x34(r1), 1, 0
    lfs f2, 0x4(r4)
    psq_l f4, 0x2c(r1), 0, 0
    fmuls f30, f5, f2
    lfs f1, asmF_42000000
    lfs f0, __float_epsilon@l(r3)
    ps_muls0 f3, f3, f30
    ps_muls0 f4, f4, f30
    fmuls f0, f1, f0
    psq_st f3, 0x34(r1), 1, 0
    ps_mul f3, f4, f4
    lfs f22, 0x34(r1)
    psq_st f4, 0x2c(r1), 0, 0
    ps_madd f1, f22, f22, f3
    ps_sum0 f1, f1, f3, f3
    fcmpo cr0, f1, f0
    cror eq, lt, eq
    bne L_802D2314
    lfs f0, asmF_3f800000
    stfs f0, 0x34(r1)
    psq_l f1, 0x34(r1), 1, 0
    stfs f0, 0x2c(r1)
    ps_muls0 f1, f1, f2
    stfs f0, 0x30(r1)
    psq_l f0, 0x2c(r1), 0, 0
    psq_st f1, 0x34(r1), 1, 0
    ps_muls0 f0, f0, f2
    psq_st f0, 0x2c(r1), 0, 0
    L_802D2314:
    lwz r4, 0x4(r29)
    lis r3, __float_epsilon@ha
    psq_l f0, 0x1c(r29), 0, 0
    lfs f29, 0xc(r4)
    psq_l f1, 0x24(r29), 1, 0
    ps_muls0 f4, f0, f29
    lfs f2, 0x38(r1)
    ps_muls0 f3, f1, f29
    lfs f0, 0x2c(r1)
    lfs f1, asmF_42000000
    psq_st f4, 0x8(r1), 0, 0
    fadds f0, f2, f0
    lfs f2, 0x8(r1)
    psq_st f3, 0x10(r1), 1, 0
    fadds f0, f2, f0
    lfs f4, 0xc(r1)
    lfs f3, 0x10(r1)
    stfs f0, 0x0(r31)
    lfs f2, 0x3c(r1)
    lfs f0, 0x30(r1)
    lwz r4, 0x4(r29)
    fadds f0, f2, f0
    lfs f2, 0x10(r4)
    fadds f0, f4, f0
    fmuls f0, f2, f0
    stfs f0, 0x4(r31)
    lfs f2, 0x40(r1)
    lfs f0, 0x34(r1)
    fadds f0, f2, f0
    fadds f0, f3, f0
    stfs f0, 0x8(r31)
    psq_l f2, 0x0(r31), 0, 0
    lfs f21, 0x8(r31)
    ps_mul f2, f2, f2
    lfs f0, __float_epsilon@l(r3)
    fmuls f0, f1, f0
    ps_madd f4, f21, f21, f2
    ps_sum0 f4, f4, f2, f2
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    beq L_802D240C
    lfs f0, asmF_00000000
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    bne L_802D23D0
    fmr f25, f4
    b L_802D23F4
    L_802D23D0:
    frsqrte f3, f4
    lfs f2, asmF_3f000000
    lfs f0, asmF_40400000
    frsp f3, f3
    fmuls f1, f3, f3
    fmuls f2, f2, f3
    fnmsubs f0, f4, f1, f0
    fmuls f0, f2, f0
    fmr f25, f0
    L_802D23F4:
    psq_l f1, 0x0(r31), 0, 0
    psq_l f0, 0x8(r31), 1, 0
    ps_muls0 f1, f1, f25
    ps_muls0 f0, f0, f25
    psq_st f1, 0x0(r31), 0, 0
    psq_st f0, 0x8(r31), 1, 0
    L_802D240C:
    psq_l f31, 0x108(r1), 0, 0
    lfd f31, 0x100(r1)
    psq_l f30, 0xf8(r1), 0, 0
    lfd f30, 0xf0(r1)
    psq_l f29, 0xe8(r1), 0, 0
    lfd f29, 0xe0(r1)
    psq_l f28, 0xd8(r1), 0, 0
    lfd f28, 0xd0(r1)
    psq_l f27, 0xc8(r1), 0, 0
    lfd f27, 0xc0(r1)
    psq_l f26, 0xb8(r1), 0, 0
    lfd f26, 0xb0(r1)
    psq_l f25, 0xa8(r1), 0, 0
    lfd f25, 0xa0(r1)
    psq_l f24, 0x98(r1), 0, 0
    lfd f24, 0x90(r1)
    psq_l f23, 0x88(r1), 0, 0
    lfd f23, 0x80(r1)
    psq_l f22, 0x78(r1), 0, 0
    lfd f22, 0x70(r1)
    psq_l f21, 0x68(r1), 0, 0
    lfd f21, 0x60(r1)
    lmw r29, 0x54(r1)
    lwz r0, 0x114(r1)
    mtlr r0
    addi r1, r1, 0x110
    blr
}
asm void TBoid::update(const JGeometry::TVec3<f32> &pos, f32 f)
{
    nofralloc
    stwu r1, -0xb0(r1)
    mflr r0
    stw r0, 0xb4(r1)
    stfd f31, 0xa0(r1)
    psq_st f31, 0xa8(r1), 0, 0
    stfd f30, 0x90(r1)
    psq_st f30, 0x98(r1), 0, 0
    stfd f29, 0x80(r1)
    psq_st f29, 0x88(r1), 0, 0
    stfd f28, 0x70(r1)
    psq_st f28, 0x78(r1), 0, 0
    stfd f27, 0x60(r1)
    psq_st f27, 0x68(r1), 0, 0
    stfd f26, 0x50(r1)
    psq_st f26, 0x58(r1), 0, 0
    stfd f25, 0x40(r1)
    psq_st f25, 0x48(r1), 0, 0
    stw r31, 0x3c(r1)
    mr r31, r3
    psq_l f3, 0x0(r4), 0, 0
    lfs f0, 0xc(r3)
    psq_l f2, 0x8(r4), 1, 0
    lwz r3, 0x10(r3)
    ps_muls0 f3, f3, f0
    ps_muls0 f2, f2, f0
    lfs f0, 0x4(r3)
    psq_st f3, 0x2c(r1), 0, 0
    fcmpo cr0, f1, f0
    psq_st f2, 0x34(r1), 1, 0
    ble L_802D2500
    lfs f1, 0x30(r1)
    lfs f0, sUpPow__5TBoid
    fadds f0, f1, f0
    stfs f0, 0x30(r1)
    L_802D2500:
    lwz r3, 0x14(r31)
    psq_l f1, 0x2c(r1), 0, 0
    psq_l f2, 0x0(r3), 0, 0
    psq_l f0, 0x34(r1), 1, 0
    ps_add f1, f2, f1
    psq_st f1, 0x0(r3), 0, 0
    psq_l f1, 0x8(r3), 1, 0
    ps_add f0, f1, f0
    psq_st f0, 0x8(r3), 1, 0
    lwz r3, 0x14(r31)
    bl PSVECMag
    lfs f0, 0x8(r31)
    fcmpo cr0, f1, f0
    ble L_802D2558
    fdivs f25, f0, f1
    lwz r3, 0x14(r31)
    psq_l f1, 0x0(r3), 0, 0
    psq_l f0, 0x8(r3), 1, 0
    ps_muls0 f1, f1, f25
    ps_muls0 f0, f0, f25
    psq_st f1, 0x0(r3), 0, 0
    psq_st f0, 0x8(r3), 1, 0
    L_802D2558:
    lwz r4, 0x10(r31)
    lis r3, __float_epsilon@ha
    lwz r5, 0x14(r31)
    psq_l f2, 0x0(r4), 0, 0
    psq_l f0, 0x0(r5), 0, 0
    lfs f1, asmF_42000000
    ps_add f0, f2, f0
    psq_st f0, 0x0(r4), 0, 0
    psq_l f2, 0x8(r4), 1, 0
    psq_l f0, 0x8(r5), 1, 0
    ps_add f0, f2, f0
    psq_st f0, 0x8(r4), 1, 0
    lwz r4, 0x14(r31)
    lfs f0, __float_epsilon@l(r3)
    psq_l f2, 0x0(r4), 0, 0
    lfs f28, 0x8(r4)
    fmuls f1, f1, f0
    ps_mul f2, f2, f2
    ps_madd f0, f28, f28, f2
    ps_sum0 f0, f0, f2, f2
    fcmpo cr0, f0, f1
    cror eq, lt, eq
    bne L_802D2604
    lwz r3, 0x18(r31)
    lfs f3, 0x28(r3)
    lfs f2, 0x18(r3)
    lfs f0, 0x8(r3)
    stfs f0, 0x20(r1)
    stfs f2, 0x24(r1)
    psq_l f2, 0x20(r1), 0, 0
    stfs f3, 0x28(r1)
    ps_mul f2, f2, f2
    ps_madd f0, f3, f3, f2
    ps_sum0 f0, f0, f2, f2
    fcmpo cr0, f0, f1
    cror eq, lt, eq
    bne L_802D261C
    lfs f1, asmF_00000000
    lfs f0, asmF_3f800000
    stfs f1, 0x20(r1)
    stfs f1, 0x24(r1)
    stfs f0, 0x28(r1)
    b L_802D261C
    L_802D2604:
    lfs f0, 0x0(r4)
    stfs f0, 0x20(r1)
    lfs f0, 0x4(r4)
    stfs f0, 0x24(r1)
    lfs f0, 0x8(r4)
    stfs f0, 0x28(r1)
    L_802D261C:
    psq_l f2, 0x20(r1), 0, 0
    lis r3, __float_epsilon@ha
    lfs f27, 0x28(r1)
    ps_mul f2, f2, f2
    lfs f1, asmF_42000000
    lfs f0, __float_epsilon@l(r3)
    ps_madd f4, f27, f27, f2
    fmuls f0, f1, f0
    ps_sum0 f4, f4, f2, f2
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    beq L_802D26A0
    lfs f0, asmF_00000000
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    bne L_802D2664
    fmr f31, f4
    b L_802D2688
    L_802D2664:
    frsqrte f3, f4
    lfs f2, asmF_3f000000
    lfs f0, asmF_40400000
    frsp f3, f3
    fmuls f1, f3, f3
    fmuls f2, f2, f3
    fnmsubs f0, f4, f1, f0
    fmuls f0, f2, f0
    fmr f31, f0
    L_802D2688:
    psq_l f1, 0x20(r1), 0, 0
    psq_l f0, 0x28(r1), 1, 0
    ps_muls0 f1, f1, f31
    ps_muls0 f0, f0, f31
    psq_st f1, 0x20(r1), 0, 0
    psq_st f0, 0x28(r1), 1, 0
    L_802D26A0:
    lfs f2, asmF_00000000
    lis r3, __float_epsilon@ha
    lfs f0, asmF_3f800000
    stfs f2, 0x8(r1)
    lfs f1, asmF_42000000
    stfs f0, 0xc(r1)
    lfs f0, __float_epsilon@l(r3)
    psq_l f3, 0x8(r1), 0, 0
    fmuls f0, f1, f0
    stfs f2, 0x10(r1)
    ps_mul f3, f3, f3
    ps_madd f4, f2, f2, f3
    ps_sum0 f4, f4, f3, f3
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    beq L_802D2730
    fcmpo cr0, f4, f2
    cror eq, lt, eq
    bne L_802D26F4
    fmr f30, f4
    b L_802D2718
    L_802D26F4:
    frsqrte f3, f4
    lfs f2, asmF_3f000000
    lfs f0, asmF_40400000
    frsp f3, f3
    fmuls f1, f3, f3
    fmuls f2, f2, f3
    fnmsubs f0, f4, f1, f0
    fmuls f0, f2, f0
    fmr f30, f0
    L_802D2718:
    psq_l f1, 0x8(r1), 0, 0
    psq_l f0, 0x10(r1), 1, 0
    ps_muls0 f1, f1, f30
    ps_muls0 f0, f0, f30
    psq_st f1, 0x8(r1), 0, 0
    psq_st f0, 0x10(r1), 1, 0
    L_802D2730:
    addi r3, r1, 0x8
    addi r4, r1, 0x20
    addi r5, r1, 0x14
    bl PSVECCrossProduct
    psq_l f2, 0x14(r1), 0, 0
    lis r3, __float_epsilon@ha
    lfs f26, 0x1c(r1)
    ps_mul f2, f2, f2
    lfs f1, asmF_42000000
    lfs f0, __float_epsilon@l(r3)
    ps_madd f4, f26, f26, f2
    fmuls f0, f1, f0
    ps_sum0 f4, f4, f2, f2
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    beq L_802D27C4
    lfs f0, asmF_00000000
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    bne L_802D2788
    fmr f29, f4
    b L_802D27AC
    L_802D2788:
    frsqrte f3, f4
    lfs f2, asmF_3f000000
    lfs f0, asmF_40400000
    frsp f3, f3
    fmuls f1, f3, f3
    fmuls f2, f2, f3
    fnmsubs f0, f4, f1, f0
    fmuls f0, f2, f0
    fmr f29, f0
    L_802D27AC:
    psq_l f1, 0x14(r1), 0, 0
    psq_l f0, 0x1c(r1), 1, 0
    ps_muls0 f1, f1, f29
    ps_muls0 f0, f0, f29
    psq_st f1, 0x14(r1), 0, 0
    psq_st f0, 0x1c(r1), 1, 0
    L_802D27C4:
    lwz r3, 0x18(r31)
    lfs f0, 0x14(r1)
    stfs f0, 0x0(r3)
    lfs f0, 0x18(r1)
    stfs f0, 0x10(r3)
    lfs f0, 0x1c(r1)
    stfs f0, 0x20(r3)
    lfs f0, 0x8(r1)
    stfs f0, 0x4(r3)
    lfs f0, 0xc(r1)
    stfs f0, 0x14(r3)
    lfs f0, 0x10(r1)
    stfs f0, 0x24(r3)
    lfs f0, 0x20(r1)
    stfs f0, 0x8(r3)
    lfs f0, 0x24(r1)
    stfs f0, 0x18(r3)
    lfs f0, 0x28(r1)
    stfs f0, 0x28(r3)
    psq_l f31, 0xa8(r1), 0, 0
    lfd f31, 0xa0(r1)
    psq_l f30, 0x98(r1), 0, 0
    lfd f30, 0x90(r1)
    psq_l f29, 0x88(r1), 0, 0
    lfd f29, 0x80(r1)
    psq_l f28, 0x78(r1), 0, 0
    lfd f28, 0x70(r1)
    psq_l f27, 0x68(r1), 0, 0
    lfd f27, 0x60(r1)
    psq_l f26, 0x58(r1), 0, 0
    lfd f26, 0x50(r1)
    psq_l f25, 0x48(r1), 0, 0
    lfd f25, 0x40(r1)
    lwz r0, 0xb4(r1)
    lwz r31, 0x3c(r1)
    mtlr r0
    addi r1, r1, 0xb0
    blr
}

#include "JSystem/JAudio/JASFakeMatch2.h"
