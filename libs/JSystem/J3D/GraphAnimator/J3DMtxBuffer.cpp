#pragma push
#pragma auto_inline off

#include "JSystem/J3D/J3DMtxBuffer.h"
#include "JSystem/J3D/J3DJoint.h"
#include "JSystem/J3D/J3DJointTree.h"
#include "JSystem/J3D/J3DMaterial.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DTransform.h"
#include "JSystem/J3D/J3DTypes.h"
#include "JSystem/JKernel/JKRHeap.h"

Mtx J3DMtxBuffer::sNoUseDrawMtx;
Mtx33 J3DMtxBuffer::sNoUseNrmMtx;
Mtx* J3DMtxBuffer::sNoUseDrawMtxPtr = &J3DMtxBuffer::sNoUseDrawMtx;
Mtx33* J3DMtxBuffer::sNoUseNrmMtxPtr = &J3DMtxBuffer::sNoUseNrmMtx;

void J3DCalcViewBaseMtx(Mtx view, Vec const& scale, const Mtx& base, Mtx dst);
extern "C" int countBumpMtxNum__8J3DShapeCFv(J3DShape*);
extern "C" void* __nwa__FUl(u32);
extern "C" void* __nwa__FUli(u32, int);

void J3DMtxBuffer::initialize()
{
    mJointTree = NULL;
    mScaleFlags = NULL;
    mEnvelopeScaleFlags = NULL;
    mWorldMatrices = NULL;
    mWeightEnvelopeMatrices = NULL;
    mDrawMatrices[0] = NULL;
    mDrawMatrices[1] = NULL;
    mNormMatrices[0] = NULL;
    mNormMatrices[1] = NULL;
    mBumpMatrices[0] = NULL;
    mBumpMatrices[1] = NULL;
    mModelType = 1;
    mCurrentViewNumber = 0;
    mUserAnmMtx = NULL;
}

#pragma pop
int J3DMtxBuffer::create(J3DModelData* pModelData, u32 mtxNum)
{
    s32 ret = 0;
    mModelType = mtxNum;
    mJointTree = &pModelData->getJointTree();

    ret = createAnmMtx(pModelData);
    if (ret != 0)
        return ret;

    ret = createWeightEnvelopeMtx(pModelData);
    if (ret != 0)
        return ret;

    if (pModelData->checkFlag(0x100)) {
        setNoUseDrawMtx();
    } else {
        u32 loadType = pModelData->getFlag() & 0x10;
        switch (loadType) {
        case 0x10:
            ret = setNoUseDrawMtx();
            break;
        case 0:
        default:
            ret = createDoubleDrawMtx(pModelData, mtxNum);
            break;
        }
    }

    if (ret != 0)
        return ret;

    if (pModelData->getFlag() & 0x10) {
        pModelData->setBumpFlag(0);
    } else {
        ret = createBumpMtxArray(pModelData, mtxNum);

        if (ret != 0)
            return ret;
    }

    return ret;
}

int J3DMtxBuffer::createAnmMtx(J3DModelData* pModelData)
{
    u32 jointNum = pModelData->getJointNum();
    if (jointNum != 0) {
        mScaleFlags = new u8[jointNum];
        mWorldMatrices = new Mtx[pModelData->getJointNum()];
        mUserAnmMtx = mWorldMatrices;
    }

    return 0;
}

int J3DMtxBuffer::createWeightEnvelopeMtx(J3DModelData* pModelData)
{
    u32 wEvlpMtxNum = pModelData->getWEvlpMtxNum();
    if (wEvlpMtxNum != 0) {
        mEnvelopeScaleFlags = new u8[wEvlpMtxNum];
        mWeightEnvelopeMatrices = new Mtx[pModelData->getWEvlpMtxNum()];
    }

    return 0;
}

int J3DMtxBuffer::setNoUseDrawMtx()
{
    mDrawMatrices[0] = mDrawMatrices[1] = &sNoUseDrawMtxPtr;
    mNormMatrices[0] = mNormMatrices[1] = &sNoUseNrmMtxPtr;
    mBumpMatrices[0] = mBumpMatrices[1] = NULL;
    return 0;
}

int J3DMtxBuffer::createDoubleDrawMtx(J3DModelData* pModelData, u32 mtxNum)
{
    if (mtxNum != 0) {
        for (s32 i = 0; i < 2; i++) {
            mDrawMatrices[i] = new Mtx*[mtxNum];
            mNormMatrices[i] = new Mtx33*[mtxNum];
            mBumpMatrices[i] = NULL;
        }
    }

    for (s32 i = 0; i < 2; i++) {
        for (u32 j = 0; j < mtxNum; j++) {
            u32 drawMtxNum = pModelData->getDrawMtxNum();
            if (drawMtxNum != 0) {
                mDrawMatrices[i][j] = new (0x20) Mtx[drawMtxNum];
                mNormMatrices[i][j] = new (0x20) Mtx33[pModelData->getDrawMtxNum()];
            }
        }
    }

    return 0;
}

int J3DMtxBuffer::createBumpMtxArray(J3DModelData* i_modelData, u32 mtxNum)
{
    __asm {
        mr r26,r4
        mr r25,r3
        mr r27,r5
        lwz r0,28(r4)
        cmplwi r0,0
        bne L_bump_end
        lhz r28,92(r26)
        li r20,0
        li r31,0
        li r21,0
        b L_bump1_cond

    L_bump1_body:
        lwz r3,96(r26)
        rlwinm r0,r21,2,14,29
        lwzx r29,r3,r0
        lwz r3,40(r29)
        lwz r12,0(r3)
        lwz r12,92(r12)
        mtctr r12
        bctrl
        lbz r0,0(r3)
        cmplwi r0,1
        bne L_bump1_next
        lwz r3,8(r29)
        bl countBumpMtxNum__8J3DShapeCFv
        add r20,r20,r3
        addi r31,r31,1
    L_bump1_next:
        addi r21,r21,1
    L_bump1_cond:
        clrlwi r0,r21,16
        cmplw r0,r28
        blt L_bump1_body

        clrlwi. r0,r20,16
        beq L_bump2_pre
        cmplwi r27,0
        beq L_bump2_pre
        mr r23,r25
        rlwinm r24,r31,2,14,29
        li r20,0
    L_bump2a_body:
        mr r3,r24
        bl __nwa__FUl
        addi r20,r20,1
        stw r3,36(r23)
        cmpwi r20,2
        addi r23,r23,4
        blt L_bump2a_body

    L_bump2_pre:
        mr r21,r25
        slwi r20,r27,2
        li r30,0
    L_bump2_outer:
        lhz r24,92(r26)
        li r29,0
        li r28,0
        li r22,0
        b L_bump2_cond

    L_bump2_body:
        lwz r3,96(r26)
        rlwinm r0,r28,2,14,29
        lwzx r23,r3,r0
        lwz r3,40(r23)
        lwz r12,0(r3)
        lwz r12,92(r12)
        mtctr r12
        bctrl
        lbz r0,0(r3)
        cmplwi r0,1
        bne L_bump2_next
        mr r3,r20
        bl __nwa__FUl
        lwz r4,36(r21)
        stwx r3,r4,r22
        addi r22,r22,4
        lwz r3,8(r23)
        stw r29,100(r3)
        addi r29,r29,1
    L_bump2_next:
        addi r28,r28,1
    L_bump2_cond:
        clrlwi r0,r28,16
        cmplw r0,r24
        blt L_bump2_body
        addi r30,r30,1
        addi r21,r21,4
        cmpwi r30,2
        blt L_bump2_outer

        li r24,0
        mr r22,r25
    L_bump3_outer:
        lhz r23,92(r26)
        li r25,0
        li r21,0
        b L_bump3_cond

    L_bump3_body:
        lwz r3,96(r26)
        rlwinm r0,r25,2,14,29
        lwzx r3,r3,r0
        lwz r3,40(r3)
        lwz r12,0(r3)
        lwz r12,92(r12)
        mtctr r12
        bctrl
        lbz r0,0(r3)
        cmplwi r0,1
        bne L_bump3_next
        li r28,0
        li r20,0
        b L_bump3_inner_cond

    L_bump3_inner:
        lhz r0,68(r26)
        li r4,32
        mulli r3,r0,36
        bl __nwa__FUl
        lwz r0,36(r22)
        addi r28,r28,1
        lwzx r4,r21,r0
        stwx r3,r4,r20
        addi r20,r20,4
    L_bump3_inner_cond:
        cmplw r28,r27
        blt L_bump3_inner
        addi r21,r21,4
    L_bump3_next:
        addi r25,r25,1
    L_bump3_cond:
        clrlwi r0,r25,16
        cmplw r0,r23
        blt L_bump3_body
        addi r24,r24,1
        addi r22,r22,4
        cmpwi r24,2
        blt L_bump3_outer

        clrlwi. r0,r31,16
        beq L_bump_end
        li r0,1
        sth r0,12(r26)
    L_bump_end:
        li r3,0
    }
}

static f32 J3DUnit01[] = {0.0f, 1.0f};

void J3DMtxBuffer::calcWeightEnvelopeMtx()
{
    register MtxPtr weightAnmMtx;
    register MtxPtr worldMtx;
    register MtxPtr invMtx;
    register f32 weight;
    int idx;
    int j;
    int mixNum;
    int i;
    int max;
    u16* indices;
    register f32* weights;
    u8* pScale;

    register f32 var_f1;
    register f32 var_f2;
    register f32 var_f3;
    register f32 var_f4;
    register f32 var_f5;
    register f32 var_f6;
    register f32 var_f7;
    register f32 var_f8;
    register f32 var_f9;
    register f32 var_f9b;
    register f32 var_f10;
    register f32 var_f11;
    register f32 var_f12;
    register f32 var_f13;
    register f32 var_f31;
    register f32 var_f30;
    register f32 var_f29;
    register f32 var_f28;
    register f32 var_f27;
    register f32 var_f26;
    register f32 var_f25;
    register f32 var_f24;
    register f32* var_r7 = J3DUnit01;

    i = -1;
    max = mJointTree->getWEvlpMtxNum();
    indices = mJointTree->getWEvlpMixIndex() - 1;
    weights = mJointTree->getWEvlpMixWeight() - 1;

    asm {
        psq_l var_f24, 0(var_r7), 0, 0
        ps_merge00 var_f11, var_f24, var_f24
        ps_merge00 var_f13, var_f24, var_f24
        ps_merge00 var_f30, var_f24, var_f24
    }

    while (++i < max) {
        pScale = &mEnvelopeScaleFlags[i];
        *pScale = 1;
        weightAnmMtx = mWeightEnvelopeMatrices[i];

        asm {
            ps_merge00 var_f10, var_f24, var_f24
            ps_merge00 var_f12, var_f24, var_f24
            ps_merge00 var_f31, var_f24, var_f24
        }

        j = 0;
        mixNum = mJointTree->getWEvlpMixMtxNum(i);
        do {
            idx = *++indices;
            invMtx = mJointTree->getInvJointMtx((u16)idx);
            worldMtx = mWorldMatrices[idx];

            asm {
                lfsu weight, 0x4(weights)
                psq_l var_f2, 0x0(invMtx), 0, 0
                psq_l var_f1, 0x0(worldMtx), 0, 0
                psq_l var_f3, 0x10(worldMtx), 0, 0
                ps_muls0 var_f29, var_f2, var_f1
                psq_l var_f8, 0x10(invMtx), 0, 0
                ps_muls0 var_f27, var_f2, var_f3
                psq_l var_f5, 0x20(worldMtx), 0, 0
                psq_l var_f7, 0x8(invMtx), 0, 0
                ps_muls0 var_f25, var_f2, var_f5
                ps_madds1 var_f29, var_f8, var_f1, var_f29
                psq_l var_f9, 0x20(invMtx), 0, 0
                psq_l var_f2, 0x8(worldMtx), 0, 0
                ps_madds1 var_f27, var_f8, var_f3, var_f27
                psq_l var_f4, 0x18(worldMtx), 0, 0
                ps_madds0 var_f29, var_f9, var_f2, var_f29
                ps_madds1 var_f25, var_f8, var_f5, var_f25
                psq_l var_f6, 0x28(worldMtx), 0, 0
                ps_madds0 var_f27, var_f9, var_f4, var_f27
                psq_l var_f8, 0x18(invMtx), 0, 0
                ps_muls0 var_f28, var_f7, var_f1
                ps_muls0 var_f26, var_f7, var_f3
                ps_madds0 var_f25, var_f9, var_f6, var_f25
                psq_l var_f9b, 0x28(invMtx), 0, 0
                ps_madds0 var_f10, var_f29, weight, var_f10
                ps_muls0 var_f7, var_f7, var_f5
                ps_madds1 var_f28, var_f8, var_f1, var_f28
                ps_madds1 var_f26, var_f8, var_f3, var_f26
                psq_st var_f10, 0x0(weightAnmMtx), 0, 0
                ps_madds0 var_f12, var_f27, weight, var_f12
                ps_madds1 var_f7, var_f8, var_f5, var_f7
                ps_madds0 var_f28, var_f9b, var_f2, var_f28
                ps_madds0 var_f26, var_f9b, var_f4, var_f26
                psq_st var_f12, 0x10(weightAnmMtx), 0, 0
                ps_madds0 var_f31, var_f25, weight, var_f31
                ps_madds0 var_f7, var_f9b, var_f6, var_f7
                ps_madd var_f28, var_f24, var_f2, var_f28
                psq_st var_f31, 0x20(weightAnmMtx), 0, 0
                ps_madd var_f26, var_f24, var_f4, var_f26
                ps_madd var_f7, var_f24, var_f6, var_f7
                ps_madds0 var_f11, var_f28, weight, var_f11
                ps_madds0 var_f13, var_f26, weight, var_f13
                ps_madds0 var_f30, var_f7, weight, var_f30
            }

            *pScale &= mScaleFlags[idx];
        } while (++j < mixNum);
        asm {
            psq_st var_f11, 8(weightAnmMtx), 0, 0
            ps_merge00 var_f11, var_f24, var_f24
            psq_st var_f13, 24(weightAnmMtx), 0, 0
            ps_merge00 var_f13, var_f24, var_f24
            psq_st var_f30, 40(weightAnmMtx), 0, 0
            ps_merge00 var_f30, var_f24, var_f24
        }
    }
}

void J3DMtxBuffer::calcDrawMtx(u32 mdlFlag, Vec const& param_1, Mtx const& param_2)
{
    Mtx viewBaseMtx;
    __asm {
        cmpwi r4,1
        mr r28,r3
        beq L_draw_case1
        bge L_draw_ge1
        cmpwi r4,0
        bge L_draw_case0
        b L_draw_end
    L_draw_ge1:
        cmpwi r4,3
        bge L_draw_end
        b L_draw_case2

    L_draw_case0:
        lwz r3,0(r28)
        lis r4,j3dSys@ha
        addi r0,r4,j3dSys@l
        li r29,0
        lhz r31,54(r3)
        mr r30,r0
        b L_draw0_cond
    L_draw0_body:
        lwz r4,0(r28)
        clrlwi r0,r29,16
        lwz r3,48(r28)
        rlwinm r6,r29,1,15,30
        lwz r7,60(r4)
        mulli r0,r0,48
        lwz r5,24(r28)
        slwi r4,r3,2
        lhzx r6,r7,r6
        mr r3,r30
        lwzx r4,r5,r4
        mulli r6,r6,48
        lwz r7,12(r28)
        add r5,r4,r0
        add r4,r7,r6
        bl PSMTXConcat
        addi r29,r29,1
    L_draw0_cond:
        clrlwi r0,r29,16
        cmplw r0,r31
        blt L_draw0_body
        lwz r7,0(r28)
        lhz r0,52(r7)
        cmplw r0,r31
        ble L_draw_end
        lwz r4,48(r28)
        mulli r0,r31,48
        lwz r6,24(r28)
        mr r3,r30
        slwi r5,r4,2
        lwz r4,16(r28)
        lwzx r5,r6,r5
        lhz r6,30(r7)
        add r5,r5,r0
        bl J3DPSMtxArrayConcat
        b L_draw_end

    L_draw_case1:
        lwz r3,0(r28)
        li r30,0
        lhz r29,54(r3)
        b L_draw1_cond
    L_draw1_body:
        lwz r4,0(r28)
        clrlwi r0,r30,16
        lwz r3,48(r28)
        rlwinm r5,r30,1,15,30
        lwz r6,60(r4)
        mulli r0,r0,48
        lwz r4,24(r28)
        slwi r3,r3,2
        lhzx r5,r6,r5
        lwzx r3,r4,r3
        mulli r5,r5,48
        lwz r6,12(r28)
        add r4,r3,r0
        add r3,r6,r5
        bl PSMTXCopy
        addi r30,r30,1
    L_draw1_cond:
        clrlwi r0,r30,16
        cmplw r0,r29
        blt L_draw1_body
        lwz r3,0(r28)
        li r30,0
        lhz r29,54(r3)
        b L_draw1b_cond
    L_draw1b_body:
        lwz r3,0(r28)
        clrlwi r6,r30,16
        lwz r5,48(r28)
        mulli r0,r6,48
        lhz r4,54(r3)
        lwz r3,16(r28)
        slwi r5,r5,2
        add r4,r6,r4
        lwz r6,24(r28)
        mulli r4,r4,48
        add r3,r3,r0
        lwzx r0,r6,r5
        add r4,r0,r4
        bl PSMTXCopy
        addi r30,r30,1
    L_draw1b_cond:
        clrlwi r0,r30,16
        cmplw r0,r29
        blt L_draw1b_body
        b L_draw_end

    L_draw_case2:
        lis r3,j3dSys@ha
        mr r4,r5
        mr r5,r6
        la r6,viewBaseMtx(r1)
        addi r3,r3,j3dSys@l
        bl J3DCalcViewBaseMtx
        lwz r3,0(r28)
        li r30,0
        lhz r29,54(r3)
        b L_draw2_cond
    L_draw2_body:
        lwz r4,0(r28)
        clrlwi r0,r30,16
        lwz r3,48(r28)
        rlwinm r6,r30,1,15,30
        lwz r7,60(r4)
        mulli r0,r0,48
        lwz r5,24(r28)
        slwi r4,r3,2
        lhzx r6,r7,r6
        la r3,viewBaseMtx(r1)
        lwzx r4,r5,r4
        mulli r6,r6,48
        lwz r7,12(r28)
        add r5,r4,r0
        add r4,r7,r6
        bl PSMTXConcat
        addi r30,r30,1
    L_draw2_cond:
        clrlwi r0,r30,16
        cmplw r0,r29
        blt L_draw2_body
        lwz r7,0(r28)
        lhz r3,54(r7)
        lhz r0,52(r7)
        cmplw r0,r3
        ble L_draw_end
        lwz r4,48(r28)
        mulli r0,r3,48
        lwz r6,24(r28)
        la r3,viewBaseMtx(r1)
        slwi r5,r4,2
        lwz r4,16(r28)
        lwzx r5,r6,r5
        lhz r6,30(r7)
        add r5,r5,r0
        bl J3DPSMtxArrayConcat
    L_draw_end:
    }
}

#pragma schedule off
void J3DMtxBuffer::calcNrmMtx()
{
    __asm {
        li r30,0
        mr r29,r3
        lwz r3,0(r3)
        lhz r31,52(r3)
        b L_nrm_loop

    L_nrm_noskip:
        lwz r4,0(r29)
        clrlwi r0,r30,16
        lwz r3,56(r4)
        lbzx r3,r3,r0
        cmplwi r3,0
        bne L_nrm_env

        lwz r4,60(r4)
        slwi r3,r0,1
        lwz r5,4(r29)
        lhzx r3,r4,r3
        lbzx r3,r5,r3
        cmplwi r3,1
        bne L_nrm_inv

        lwz r3,48(r29)
        mulli r4,r0,48
        lwz r5,24(r29)
        slwi r6,r3,2
        lwz r3,32(r29)
        lwzx r5,r5,r6
        mulli r0,r0,36
        lwzx r3,r3,r6
        add r4,r5,r4
        psq_l f5,0(r4),0,0
        add r3,r3,r0
        lfs f4,8(r4)
        psq_l f3,16(r4),0,0
        lfs f2,24(r4)
        psq_l f1,32(r4),0,0
        lfs f0,40(r4)
        psq_st f5,0(r3),0,0
        stfs f4,8(r3)
        psq_st f3,12(r3),0,0
        stfs f2,20(r3)
        psq_st f1,24(r3),0,0
        stfs f0,32(r3)
        b L_nrm_inc

    L_nrm_inv:
        lwz r3,48(r29)
        mulli r5,r0,48
        lwz r4,24(r29)
        slwi r7,r3,2
        lwz r3,32(r29)
        lwzx r6,r4,r7
    }
    __asm {
        lwzx r4,r3,r7
        add r3,r6,r5
        mulli r0,r0,36
        add r4,r4,r0
        bl J3DPSCalcInverseTranspose
        b L_nrm_inc

    L_nrm_env:
        lwz r4,60(r4)
        slwi r3,r0,1
        lwz r5,8(r29)
        lhzx r3,r4,r3
        lbzx r3,r5,r3
        cmplwi r3,1
        bne L_nrm_inv2

        lwz r3,48(r29)
        mulli r4,r0,48
        lwz r5,24(r29)
        slwi r6,r3,2
        lwz r3,32(r29)
        lwzx r5,r5,r6
        mulli r0,r0,36
        lwzx r3,r3,r6
        add r4,r5,r4
        psq_l f5,0(r4),0,0
        add r3,r3,r0
        lfs f4,8(r4)
        psq_l f3,16(r4),0,0
        lfs f2,24(r4)
        psq_l f1,32(r4),0,0
        lfs f0,40(r4)
        psq_st f5,0(r3),0,0
        stfs f4,8(r3)
        psq_st f3,12(r3),0,0
        stfs f2,20(r3)
        psq_st f1,24(r3),0,0
        stfs f0,32(r3)
        b L_nrm_inc

    L_nrm_inv2:
        lwz r3,48(r29)
        mulli r5,r0,48
        lwz r4,24(r29)
        slwi r7,r3,2
        lwz r3,32(r29)
        lwzx r6,r4,r7
    }
    __asm {
        lwzx r4,r3,r7
        add r3,r6,r5
        mulli r0,r0,36
        add r4,r4,r0
        bl J3DPSCalcInverseTranspose

    L_nrm_inc:
        addi r30,r30,1
    L_nrm_loop:
        clrlwi r0,r30,16
        cmplw r0,r31
        blt L_nrm_noskip
    }
}
#pragma schedule on

void J3DMtxBuffer::calcBBoardMtx()
{
    u16 i = 0;
    u32 drawMtxNum = mJointTree->getDrawMtxNum();
    for (; i < drawMtxNum; i++) {
        if (!mJointTree->getDrawMtxFlag(i)) {
            u16 index = mJointTree->getDrawMtxIndex(i);
            if (mJointTree->getJointNodePointer(index)->getMtxType() == 1) {
                MtxPtr drawMtx = *getDrawMtx(i);
                J3DCalcBBoardMtx(drawMtx);

                Mtx33* nrmMtx = getNrmMtx(i);
                (*nrmMtx)[0][0] = 1.0f / drawMtx[0][0];
                (*nrmMtx)[0][1] = 0.0f;
                (*nrmMtx)[0][2] = 0.0f;
                (*nrmMtx)[1][0] = 0.0f;
                (*nrmMtx)[1][1] = 1.0f / drawMtx[1][1];
                (*nrmMtx)[1][2] = 0.0f;
                (*nrmMtx)[2][0] = 0.0f;
                (*nrmMtx)[2][1] = 0.0f;
                (*nrmMtx)[2][2] = 1.0f / drawMtx[2][2];
            } else if (mJointTree->getJointNodePointer(index)->getMtxType() == 2) {
                MtxPtr drawMtx = *getDrawMtx(i);
                J3DCalcYBBoardMtx(drawMtx);
                Mtx33* nrmMtx = getNrmMtx(i);
                J3DPSCalcInverseTranspose(drawMtx, *nrmMtx);
            }
        }
    }
}

void J3DCalcViewBaseMtx(Mtx view, Vec const& scale, const Mtx& base, Mtx dst)
{
    Mtx m;

    m[0][0] = base[0][0] * scale.x;
    m[0][1] = base[0][1] * scale.y;
    m[0][2] = base[0][2] * scale.z;
    m[0][3] = base[0][3];

    m[1][0] = base[1][0] * scale.x;
    m[1][1] = base[1][1] * scale.y;
    m[1][2] = base[1][2] * scale.z;
    m[1][3] = base[1][3];

    m[2][0] = base[2][0] * scale.x;
    m[2][1] = base[2][1] * scale.y;
    m[2][2] = base[2][2] * scale.z;
    m[2][3] = base[2][3];

    PSMTXConcat(view, m, dst);
}

