#include "JSystem/J3D/J3DShapeFactory.h"
#include "JSystem/J3D/J3DShape.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JSupport.h"

enum {
    J3DMdlDataFlag_ConcatView = 0x10,
};

enum {
    J3DShapeMtxType_Mtx = 0x00,
    J3DShapeMtxType_BBoard = 0x01,
    J3DShapeMtxType_YBBoard = 0x02,
    J3DShapeMtxType_Multi = 0x03,
};

struct J3DShapeMtxInitData {
    u16 mUseMtxIndex;
    u16 mUseMtxCount;
    u32 mFirstUseMtxIndex;
};

struct J3DShapeDrawInitData {
    u32 mDisplayListSize;
    u32 mDisplayListIndex;
};

static inline u32 getMdlDataFlag_MtxLoadType(u32 flag)
{
    return flag & J3DMdlDataFlag_ConcatView;
}

J3DShapeFactory::J3DShapeFactory(const J3DShapeBlock& block)
{
    mInitData = JSUConvertOffsetToPtr<J3DShapeInitData>(&block, block._0C);
    mInitDataIndices = JSUConvertOffsetToPtr<u16>(&block, block._10);
    mVtxDescLists = JSUConvertOffsetToPtr<GXVtxDescList>(&block, block._18);
    mMtxTable = JSUConvertOffsetToPtr<u16>(&block, block._1C);
    mDisplayListData = JSUConvertOffsetToPtr<u8>(&block, block._20);
    mMtxInitData = JSUConvertOffsetToPtr<J3DShapeMtxInitData>(&block, block._24);
    mDrawInitData = JSUConvertOffsetToPtr<J3DShapeDrawInitData>(&block, block._28);
    mVcdVatCmdBuffer = NULL;
}

J3DShape* J3DShapeFactory::create(int no, u32 flag, GXVtxDescList* vtxDesc)
{
    J3DShape* shape = new J3DShape;
    shape->mMtxGroupNum = getMtxGroupNum(no);
    shape->mRadius = mInitData[mInitDataIndices[no]].mRadius;
    shape->mVtxDesc = getVtxDescList(no);
    shape->mShapeMtx = new J3DShapeMtx*[shape->mMtxGroupNum];
    shape->mShapeDraw = new J3DShapeDraw*[shape->mMtxGroupNum];
    shape->mMin = getMin(no);
    shape->mMax = getMax(no);
    shape->mVcdVatCmd = mVcdVatCmdBuffer + no * J3DShape::kVcdVatDLSize;

    for (int i = 0; i < shape->mMtxGroupNum; i++) {
        shape->mShapeMtx[i] = newShapeMtx(flag, no, i);
        shape->mShapeDraw[i] = newShapeDraw(no, i);
    }

    shape->mId = no;
    return shape;
}

J3DShapeMtx* J3DShapeFactory::newShapeMtx(u32 flag, int shapeNo, int mtxGroupNo) const
{
    J3DShapeMtx* ret = NULL;
    J3DShapeInitData& shapeInitData = mInitData[mInitDataIndices[shapeNo]];
    J3DShapeMtxInitData& mtxInitData = (&mMtxInitData[shapeInitData.mShapeMtxInitDataIndex])[mtxGroupNo];

    switch (getMdlDataFlag_MtxLoadType(flag)) {
    case J3DMdlDataFlag_ConcatView:
        switch (shapeInitData.mShapeMtxType) {
        case J3DShapeMtxType_Mtx:
            ret = new J3DShapeMtxConcatView(mtxInitData.mUseMtxIndex);
            break;
        case J3DShapeMtxType_BBoard:
            ret = new J3DShapeMtxBBoardConcatView(mtxInitData.mUseMtxIndex);
            break;
        case J3DShapeMtxType_YBBoard:
            ret = new J3DShapeMtxYBBoardConcatView(mtxInitData.mUseMtxIndex);
            break;
        case J3DShapeMtxType_Multi:
            ret = new J3DShapeMtxMultiConcatView(mtxInitData.mUseMtxIndex, mtxInitData.mUseMtxCount, &mMtxTable[mtxInitData.mFirstUseMtxIndex]);
            break;
        default:
            break;
        }
        break;

    case 0:
    default:
        switch (shapeInitData.mShapeMtxType) {
        case J3DShapeMtxType_Mtx:
        case J3DShapeMtxType_BBoard:
        case J3DShapeMtxType_YBBoard:
            ret = new J3DShapeMtx(mtxInitData.mUseMtxIndex);
            break;
        case J3DShapeMtxType_Multi:
            ret = new J3DShapeMtxMulti(mtxInitData.mUseMtxIndex, mtxInitData.mUseMtxCount, &mMtxTable[mtxInitData.mFirstUseMtxIndex]);
            break;
        default:
            break;
        }
        break;
    }

    return ret;
}

J3DShapeDraw* J3DShapeFactory::newShapeDraw(int shapeNo, int mtxGroupNo) const
{
    J3DShapeInitData& shapeInitData = mInitData[mInitDataIndices[shapeNo]];
    const J3DShapeDrawInitData& drawInitData = (&mDrawInitData[shapeInitData.mShapeDrawInitDataIndex])[mtxGroupNo];
    return new J3DShapeDraw(&mDisplayListData[drawInitData.mDisplayListIndex], drawInitData.mDisplayListSize);
}

void J3DShapeFactory::allocVcdVatCmdBuffer(u32 count)
{
    mVcdVatCmdBuffer = new (0x20) u8[J3DShape::kVcdVatDLSize * count];
    for (u32 i = 0; i < (J3DShape::kVcdVatDLSize * count) / 4; i++)
        ((u32*)mVcdVatCmdBuffer)[i] = 0;
}

int J3DShapeFactory::calcSize(int shapeNo, u32 flag)
{
#ifndef __MWERKS__
    u32 mtxGroupNum = mInitData[mInitDataIndices[shapeNo]].mMtxGroupNum;
    int size = 0x68 + mtxGroupNum * 4 + mtxGroupNum * 4;
    for (u32 i = 0; i < mtxGroupNum; i++) {
        size += calcSizeShapeMtx(flag, shapeNo, i);
        size += 0xc;
    }
    return size;
#else
    __asm {
        slwi r0,r4,1
        mr r26,r3
        mr r27,r4
        mr r28,r5
        li r29,0
        lwz r3,4(r3)
        lwz r4,0(r26)
        lhzx r0,r3,r0
        mulli r0,r0,40
        add r3,r4,r0
        lhz r30,2(r3)
        slwi r3,r30,2
        addi r31,r3,104
        add r31,r31,r3
        b L_calcSize_loop
    L_calcSize_body:
        mr r3,r26
        mr r4,r28
        mr r5,r27
        mr r6,r29
        bl calcSizeShapeMtx
        add r31,r31,r3
        addi r29,r29,1
        addi r31,r31,12
    L_calcSize_loop:
        cmplw r29,r30
        blt L_calcSize_body
        mr r3,r31
    }
#endif
}

int J3DShapeFactory::calcSizeVcdVatCmdBuffer(u32 count)
{
    return ALIGN_NEXT(count * J3DShape::kVcdVatDLSize, 0x20);
}

int J3DShapeFactory::calcSizeShapeMtx(u32 flag, int shapeNo, int mtxGroupNo) const
{
    J3DShapeInitData& shapeInitData = mInitData[mInitDataIndices[shapeNo]];
    u32 ret = 0;

    u32 mtxLoadType = getMdlDataFlag_MtxLoadType(flag);
    switch (mtxLoadType) {
    case J3DMdlDataFlag_ConcatView:
        switch (shapeInitData.mShapeMtxType) {
        case J3DShapeMtxType_Mtx:
            ret = 0x08;
            break;
        case J3DShapeMtxType_BBoard:
            ret = 0x08;
            break;
        case J3DShapeMtxType_YBBoard:
            ret = 0x08;
            break;
        case J3DShapeMtxType_Multi:
            ret = 0x10;
            break;
        default:
            break;
        }
        break;

    case 0:
    default:
        switch (shapeInitData.mShapeMtxType) {
        case J3DShapeMtxType_Mtx:
        case J3DShapeMtxType_BBoard:
        case J3DShapeMtxType_YBBoard:
            ret = 0x08;
            break;
        case J3DShapeMtxType_Multi:
            ret = 0x10;
            break;
        default:
            break;
        }
        break;
    }

    return ret;
}

