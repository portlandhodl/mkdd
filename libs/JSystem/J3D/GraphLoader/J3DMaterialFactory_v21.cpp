#include "JSystem/J3D/J3DMaterialFactory.h"

u16 J3DMaterialFactory_v21::countUniqueMaterials()
{
    u16 i;
    for (i = 0; i < mMaterialNum; i++) {
    }
    return i;
}

u8 J3DMaterialFactory_v21::newColorChanNum(int idx) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];
    if (initData->mColorChanNumIdx != 0xFF) {
        return mpColorChanNum[initData->mColorChanNumIdx];
    } else {
        return 0;
    }
}

u32 J3DMaterialFactory_v21::newTexGenNum(int idx) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];
    if (initData->mTexGenNumIdx != 0xFF) {
        return mpTexGenNum[initData->mTexGenNumIdx];
    } else {
        return 0;
    }
}

u8 J3DMaterialFactory_v21::newCullMode(int idx) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];
    if (initData->mCullModeIdx != 0xFF) {
        return mpCullMode[initData->mCullModeIdx];
    } else {
        return 0xFF;
    }
}

u16 J3DMaterialFactory_v21::newTexNo(int idx, int stage) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];
    if (initData->mTexNoIdx[stage] != 0xFFFF) {
        return mpTexNo[initData->mTexNoIdx[stage]];
    } else {
        return 0xFFFF;
    }
}

J3DTevOrder J3DMaterialFactory_v21::newTevOrder(int idx, int stage) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];
    if (initData->mTevOrderIdx[stage] != 0xFFFF) {
        return J3DTevOrder(mpTevOrderInfo[initData->mTevOrderIdx[stage]]);
    } else {
        return J3DTevOrder();
    }
}

u8 J3DMaterialFactory_v21::newTevStageNum(int idx) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];
    if (initData->mTevStageNumIdx != 0xFF) {
        return mpTevStageNum[initData->mTevStageNumIdx];
    } else {
        return 0xFF;
    }
}

J3DBlend J3DMaterialFactory_v21::newBlend(int idx) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];
    if (initData->mBlendIdx != 0xFFFF) {
        return J3DBlend(mpBlendInfo[initData->mBlendIdx]);
    } else {
        return J3DBlend();
    }
}

u8 J3DMaterialFactory_v21::newZCompLoc(int idx) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];
    if (initData->mZCompLocIdx != 0xFF) {
        return mpZCompLoc[initData->mZCompLocIdx];
    } else {
        return 0;
    }
}

u8 J3DMaterialFactory_v21::newDither(int idx) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];
    if (initData->mDitherIdx != 0xFF) {
        return mpDither[initData->mDitherIdx];
    } else {
        return 1;
    }
}
