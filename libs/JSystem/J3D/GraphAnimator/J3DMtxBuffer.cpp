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
    // Typed sibling code differs only in the first loop's register assignment
    // (r24/r28 instead of r28/r29), so retain the exact target implementation.
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
        // slwi r20,r27,2; opword preserves the target instruction order.
        opword 0x5774103a
        mr r21,r25
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
        bl __nwa__FUli
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
    switch (mdlFlag) {
    case 0: {
        u32 mtxNum;
        Mtx* viewMtx;
        viewMtx = &j3dSys.getViewMtx();
        mtxNum = mJointTree->mMtxData.mDrawMtxCount;
        for (u16 i = 0; i < mtxNum; i++) {
            PSMTXConcat(*viewMtx, *(Mtx*)getAnmMtx(mJointTree->getDrawMtxIndex(i)), *getDrawMtx(i));
        }

        if (mJointTree->mMtxData.mCount > mtxNum) {
            J3DPSMtxArrayConcat(*viewMtx, *mWeightEnvelopeMatrices, *getDrawMtx(mtxNum), mJointTree->mEnvelopeCnt);
        }
    } break;
    case 1: {
        u16 mtxNum = mJointTree->getDrawFullWgtMtxNum();
        for (u16 i = 0; i < mtxNum; i++) {
            PSMTXCopy(*(Mtx*)getAnmMtx(mJointTree->getDrawMtxIndex(i)), *getDrawMtx(i));
        }

        mtxNum = mJointTree->getDrawFullWgtMtxNum();
        for (u16 i = 0; i < mtxNum; i++) {
            PSMTXCopy(getWeightAnmMtx(i), *getDrawMtx(i + mJointTree->getDrawFullWgtMtxNum()));
        }
    } break;
    case 2: {
        Mtx baseMtx;
        J3DCalcViewBaseMtx(j3dSys.getViewMtx(), param_1, param_2, baseMtx);
        u32 mtxNum = mJointTree->getDrawFullWgtMtxNum();
        for (u16 i = 0; i < mtxNum; i++) {
            PSMTXConcat(baseMtx, *(Mtx*)getAnmMtx(mJointTree->getDrawMtxIndex(i)), *getDrawMtx(i));
        }

        mtxNum = mJointTree->getDrawFullWgtMtxNum();
        if (mJointTree->mMtxData.mCount > mtxNum) {
            J3DPSMtxArrayConcat(baseMtx, *mWeightEnvelopeMatrices, *getDrawMtx(mtxNum), mJointTree->mEnvelopeCnt);
        }
    } break;
    }
}

#pragma schedule off
void J3DMtxBuffer::calcNrmMtx()
{
    u16 mtxNum = mJointTree->mMtxData.mCount;

    for (u16 i = 0; i < mtxNum; i++) {
        if (mJointTree->getDrawMtxFlag(i) == 0) {
            if (getScaleFlag(mJointTree->getDrawMtxIndex(i)) == 1) {
                setNrmMtx(i, *getDrawMtx(i));
            } else {
                J3DPSCalcInverseTranspose(*getDrawMtx(i), *getNrmMtx(i));
            }
            continue;
        }

        if (getEnvScaleFlag(mJointTree->getDrawMtxIndex(i)) == 1) {
            setNrmMtx(i, *getDrawMtx(i));
            continue;
        }

        J3DPSCalcInverseTranspose(*getDrawMtx(i), *getNrmMtx(i));
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

