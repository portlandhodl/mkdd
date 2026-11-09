#include "Kaneshige/LightMgr.h"
#include "Kaneshige/RaceMgr.h"
#include "Shiraiwa/Objects/MapObjDemoObj.h"
#include "Sato/ObjUtility.h"

const u32 TMapObjDemoObj::scCameraID[11] = {
    'firs', 'park', 'fron', 'down', 'farr', 'mont',
    'mare', 'item', 'topp', 'endd', 'next'
};
f32 TMapObjDemoObj::sTurnSpeed = 0.03f;
static const f32 cZero = 0.0f;
static const f32 c32 = 32.0f;
static const f32 cHalf = 0.5f;
static const f32 cThree = 3.0f;
static const f32 asmF_3cf5c28f = 0.03f;
static const f32 asmF_00000000 = 0.0;
static const f32 asmF_3f000000 = 0.5;
static const f32 asmF_40400000 = 3.0;
static const f32 asmF_42000000 = 32.0;


TMapObjDemoObj::TMapObjDemoObj(const CrsData::SObject &obj) : TMapObjHioNode(obj)
{
    flagReset();
}

TMapObjDemoObj::TMapObjDemoObj(u32 id) : TMapObjHioNode(id)
{
    flagReset();
}

TMapObjDemoObj::~TMapObjDemoObj() {}

void TMapObjDemoObj::flagReset()
{
    mFlag = 0;
    mFlag |= 1;
}

void TMapObjDemoObj::reset()
{
    resetObject();
    mGeoObjFlag &= ~0x20;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;

    if (mFlag & 1)
    {
        mKartNo = RCMGetManager()->searchNRankKart(1);

        f32 length = (f32)mObjData->mParam2;
        if (length <= 0.0f)
            length = 1000.0f;
        mLengthSq = length * length;
    }
    else
    {
        mKartNo = -1;
        mLengthSq = 0.0f;
    }

    mLength2 = 1.0e11f;
    mCounter = 0;
}

void TMapObjDemoObj::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createDifferedModel(heap, viewNo, 0x1000200, true);
}

void TMapObjDemoObj::calc()
{
    if (mKartNo != -1)
    {
        ObjUtility::getKartPos(mKartNo, &mKartPos);
        if (checkKartLength())
        {
            turnToKart();
        }
        else
        {
            mKartNo = -1;
        }
    }
    else if (mFlag & 1)
    {
        mKartNo = RCMGetManager()->searchNRankKart(1);
    }

    if (!checkVisible())
    {
        mGeoObjFlag |= 0x20;
        if (mShadowMdl)
            mShadowMdl->clrVisibleAll();
    }
    else
    {
        mGeoObjFlag &= ~0x20;
        if (mShadowMdl)
            mShadowMdl->setVisibleAll();
    }

    if (mShadowMdl)
        moveShadowModel();
}

asm bool TMapObjDemoObj::checkKartLength()
{
    nofralloc
    stwu r1, -0x20(r1)
    lbz r0, 0x154(r3)
    cmplwi r0, 0
    bne L_check0
    psq_l f1, 0x160(r3), 0, 0
    psq_l f0, 0x4(r3), 0, 0
    psq_l f2, 0x168(r3), 1, 0
    ps_sub f3, f1, f0
    psq_l f1, 0xc(r3), 1, 0
    lfs f0, cZero
    ps_sub f1, f2, f1
    psq_st f3, 0x8(r1), 0, 0
    stfs f0, 0xc(r1)
    psq_l f2, 0x8(r1), 0, 0
    psq_st f1, 0x10(r1), 1, 0
    ps_mul f2, f2, f2
    lfs f4, 0x10(r1)
    ps_madd f0, f4, f4, f2
    ps_sum0 f0, f0, f2, f2
    stfs f0, 0x150(r3)
L_check0:
    lbz r4, 0x154(r3)
    addi r0, r4, 1
    stb r0, 0x154(r3)
    lbz r0, 0x154(r3)
    cmplwi r0, 0x1e
    ble L_check1
    li r0, 0
    stb r0, 0x154(r3)
L_check1:
    lfs f1, 0x150(r3)
    lfs f0, 0x14c(r3)
    fcmpo cr0, f1, f0
    mfcr r0
    srwi r3, r0, 31
    addi r1, r1, 0x20
    blr
}

void TMapObjDemoObj::setCurrentViewNo(u32 viewNo)
{
    mModel.setCurrentViewNo(viewNo);

    LightObj *light = LightMgr::getManager()->searchLight('SCN0' + viewNo);
    MtxPtr mtx = light->getEffectMtx();
    mModel.setEffectMtx(mtx, 1);
}

bool TMapObjDemoObj::checkVisible()
{
    u32 tag = RCMGetManager()->mRaceDirector->getDemoCameraTagName();
    s16 param = mObjData->mParam3;
    bool ret = true;

    for (int i = 0; i < 11; i++)
    {
        if (tag == scCameraID[i])
        {
            if (param & (1 << i))
                ret = false;
            break;
        }
    }

    return ret;
}

asm void TMapObjDemoObj::turnToKart()
{
    nofralloc
    stwu r1, -0xc0(r1)
    mflr r0
    stw r0, 0xc4(r1)
    stfd f31, 0xb0(r1)
    psq_st f31, 0xb8(r1), 0, 0
    stfd f30, 0xa0(r1)
    psq_st f30, 0xa8(r1), 0, 0
    stfd f29, 0x90(r1)
    psq_st f29, 0x98(r1), 0, 0
    stfd f28, 0x80(r1)
    psq_st f28, 0x88(r1), 0, 0
    stfd f27, 0x70(r1)
    psq_st f27, 0x78(r1), 0, 0
    stfd f26, 0x60(r1)
    psq_st f26, 0x68(r1), 0, 0
    stfd f25, 0x50(r1)
    psq_st f25, 0x58(r1), 0, 0
    stw r31, 0x4c(r1)
    mr r31, r3
    lis r3, __float_epsilon@ha
    psq_l f3, 0x160(r31), 0, 0
    psq_l f1, 0x4(r31), 0, 0
    psq_l f2, 0x168(r31), 1, 0
    ps_sub f5, f3, f1
    psq_l f1, 0xc(r31), 1, 0
    lfs f3, asmF_00000000
    ps_sub f4, f2, f1
    lfs f2, asmF_42000000
    psq_st f5, 0x38(r1), 0, 0
    lfs f1, __float_epsilon@l(r3)
    stfs f3, 0x3c(r1)
    fmuls f1, f2, f1
    psq_l f2, 0x38(r1), 0, 0
    psq_st f4, 0x40(r1), 1, 0
    ps_mul f2, f2, f2
    lfs f7, 0x40(r1)
    ps_madd f5, f7, f7, f2
    ps_sum0 f5, f5, f2, f2
    fcmpo cr0, f5, f1
    cror eq, lt, eq
    beq L_802CB314
    fcmpo cr0, f5, f3
    cror eq, lt, eq
    bne L_802CB2D8
    fmr f6, f5
    b L_802CB2FC
    L_802CB2D8:
    frsqrte f4, f5
    lfs f3, asmF_3f000000
    lfs f1, asmF_40400000
    frsp f4, f4
    fmuls f2, f4, f4
    fmuls f3, f3, f4
    fnmsubs f1, f5, f2, f1
    fmuls f1, f3, f1
    fmr f6, f1
    L_802CB2FC:
    psq_l f2, 0x38(r1), 0, 0
    psq_l f1, 0x40(r1), 1, 0
    ps_muls0 f2, f2, f6
    ps_muls0 f1, f1, f6
    psq_st f2, 0x38(r1), 0, 0
    psq_st f1, 0x40(r1), 1, 0
    L_802CB314:
    psq_l f4, 0x38(r1), 0, 0
    lis r3, __float_epsilon@ha
    lfs f0, 0x40(r1)
    ps_mul f4, f4, f4
    lfs f1, asmF_42000000
    lfs f5, 0x30(r31)
    lfs f6, 0x20(r31)
    ps_madd f3, f0, f0, f4
    lfs f2, 0x10(r31)
    lfs f0, __float_epsilon@l(r3)
    stfs f2, 0x2c(r1)
    ps_sum0 f3, f3, f4, f4
    fmuls f0, f1, f0
    stfs f6, 0x30(r1)
    stfs f5, 0x34(r1)
    fcmpo cr0, f3, f0
    lfs f1, 0x34(r31)
    lfs f2, 0x24(r31)
    lfs f0, 0x14(r31)
    stfs f0, 0x20(r1)
    stfs f2, 0x24(r1)
    stfs f1, 0x28(r1)
    lfs f1, 0x38(r31)
    lfs f2, 0x28(r31)
    lfs f0, 0x18(r31)
    stfs f0, 0x14(r1)
    stfs f2, 0x18(r1)
    stfs f1, 0x1c(r1)
    cror eq, lt, eq
    bne L_802CB398
    stfs f0, 0x38(r1)
    stfs f2, 0x3c(r1)
    stfs f1, 0x40(r1)
    L_802CB398:
    psq_l f1, 0x38(r1), 0, 0
    addi r3, r1, 0x8
    psq_l f0, 0x14(r1), 0, 0
    psq_l f2, 0x40(r1), 1, 0
    ps_sub f3, f1, f0
    psq_l f1, 0x1c(r1), 1, 0
    lfs f0, asmF_00000000
    ps_sub f1, f2, f1
    psq_st f3, 0x0(r3), 0, 0
    psq_st f1, 0x8(r3), 1, 0
    stfs f0, 0xc(r1)
    bl PSVECMag
    lfs f4, asmF_3cf5c28f
    fcmpo cr0, f1, f4
    ble L_802CB454
    psq_l f2, 0x8(r1), 0, 0
    lis r3, __float_epsilon@ha
    lfs f27, 0x10(r1)
    ps_mul f2, f2, f2
    lfs f1, asmF_42000000
    lfs f0, __float_epsilon@l(r3)
    ps_madd f5, f27, f27, f2
    fmuls f0, f1, f0
    ps_sum0 f5, f5, f2, f2
    fcmpo cr0, f5, f0
    cror eq, lt, eq
    beq L_802CB454
    lfs f0, asmF_00000000
    fcmpo cr0, f5, f0
    cror eq, lt, eq
    bne L_802CB418
    b L_802CB438
    L_802CB418:
    frsqrte f3, f5
    lfs f2, asmF_3f000000
    lfs f0, asmF_40400000
    frsp f3, f3
    fmuls f1, f3, f3
    fmuls f2, f2, f3
    fnmsubs f0, f5, f1, f0
    fmuls f5, f2, f0
    L_802CB438:
    fmuls f31, f5, f4
    psq_l f1, 0x8(r1), 0, 0
    psq_l f0, 0x10(r1), 1, 0
    ps_muls0 f1, f1, f31
    ps_muls0 f0, f0, f31
    psq_st f1, 0x8(r1), 0, 0
    psq_st f0, 0x10(r1), 1, 0
    L_802CB454:
    psq_l f3, 0x18(r1), 0, 0
    psq_l f2, 0x3c(r1), 0, 0
    psq_l f1, 0x14(r1), 0, 0
    ps_mul f3, f3, f2
    psq_l f2, 0x38(r1), 0, 0
    lfs f0, asmF_00000000
    ps_madd f2, f1, f2, f3
    ps_sum0 f1, f2, f3, f3
    fcmpo cr0, f1, f0
    bge L_802CB498
    lfs f30, asmF_40400000
    psq_l f1, 0x8(r1), 0, 0
    psq_l f0, 0x10(r1), 1, 0
    ps_muls0 f1, f1, f30
    ps_muls0 f0, f0, f30
    psq_st f1, 0x8(r1), 0, 0
    psq_st f0, 0x10(r1), 1, 0
    L_802CB498:
    psq_l f1, 0x1c(r1), 1, 0
    lis r3, __float_epsilon@ha
    psq_l f0, 0x10(r1), 1, 0
    psq_l f4, 0x14(r1), 0, 0
    ps_add f2, f1, f0
    psq_l f3, 0x8(r1), 0, 0
    lfs f1, asmF_42000000
    ps_add f3, f4, f3
    lfs f0, __float_epsilon@l(r3)
    psq_st f2, 0x1c(r1), 1, 0
    fmuls f0, f1, f0
    ps_mul f1, f3, f3
    lfs f26, 0x1c(r1)
    psq_st f3, 0x14(r1), 0, 0
    ps_madd f4, f26, f26, f1
    ps_sum0 f4, f4, f1, f1
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    beq L_802CB538
    lfs f0, asmF_00000000
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    bne L_802CB4FC
    fmr f29, f4
    b L_802CB520
    L_802CB4FC:
    frsqrte f3, f4
    lfs f2, asmF_3f000000
    lfs f0, asmF_40400000
    frsp f3, f3
    fmuls f1, f3, f3
    fmuls f2, f2, f3
    fnmsubs f0, f4, f1, f0
    fmuls f0, f2, f0
    fmr f29, f0
    L_802CB520:
    psq_l f1, 0x14(r1), 0, 0
    psq_l f0, 0x1c(r1), 1, 0
    ps_muls0 f1, f1, f29
    ps_muls0 f0, f0, f29
    psq_st f1, 0x14(r1), 0, 0
    psq_st f0, 0x1c(r1), 1, 0
    L_802CB538:
    addi r3, r1, 0x20
    addi r4, r1, 0x14
    addi r5, r1, 0x2c
    bl PSVECCrossProduct
    psq_l f2, 0x2c(r1), 0, 0
    lis r3, __float_epsilon@ha
    lfs f25, 0x34(r1)
    ps_mul f2, f2, f2
    lfs f1, asmF_42000000
    lfs f0, __float_epsilon@l(r3)
    ps_madd f4, f25, f25, f2
    fmuls f0, f1, f0
    ps_sum0 f4, f4, f2, f2
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    beq L_802CB5CC
    lfs f0, asmF_00000000
    fcmpo cr0, f4, f0
    cror eq, lt, eq
    bne L_802CB590
    fmr f28, f4
    b L_802CB5B4
    L_802CB590:
    frsqrte f3, f4
    lfs f2, asmF_3f000000
    lfs f0, asmF_40400000
    frsp f3, f3
    fmuls f1, f3, f3
    fmuls f2, f2, f3
    fnmsubs f0, f4, f1, f0
    fmuls f0, f2, f0
    fmr f28, f0
    L_802CB5B4:
    psq_l f1, 0x2c(r1), 0, 0
    psq_l f0, 0x34(r1), 1, 0
    ps_muls0 f1, f1, f28
    ps_muls0 f0, f0, f28
    psq_st f1, 0x2c(r1), 0, 0
    psq_st f0, 0x34(r1), 1, 0
    L_802CB5CC:
    lfs f0, 0x2c(r1)
    stfs f0, 0x10(r31)
    lfs f0, 0x30(r1)
    stfs f0, 0x20(r31)
    lfs f0, 0x34(r1)
    stfs f0, 0x30(r31)
    lfs f0, 0x20(r1)
    stfs f0, 0x14(r31)
    lfs f0, 0x24(r1)
    stfs f0, 0x24(r31)
    lfs f0, 0x28(r1)
    stfs f0, 0x34(r31)
    lfs f0, 0x14(r1)
    stfs f0, 0x18(r31)
    lfs f0, 0x18(r1)
    stfs f0, 0x28(r31)
    lfs f0, 0x1c(r1)
    stfs f0, 0x38(r31)
    psq_l f31, 0xb8(r1), 0, 0
    lfd f31, 0xb0(r1)
    psq_l f30, 0xa8(r1), 0, 0
    lfd f30, 0xa0(r1)
    psq_l f29, 0x98(r1), 0, 0
    lfd f29, 0x90(r1)
    psq_l f28, 0x88(r1), 0, 0
    lfd f28, 0x80(r1)
    psq_l f27, 0x78(r1), 0, 0
    lfd f27, 0x70(r1)
    psq_l f26, 0x68(r1), 0, 0
    lfd f26, 0x60(r1)
    psq_l f25, 0x58(r1), 0, 0
    lfd f25, 0x50(r1)
    lwz r0, 0xc4(r1)
    lwz r31, 0x4c(r1)
    mtlr r0
    addi r1, r1, 0xc0
    blr
}

#include "JSystem/JAudio/JASFakeMatch2.h"
