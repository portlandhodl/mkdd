#include "JSystem/J3D/J3DMaterialFactory.h"
#include "JSystem/J3D/J3DColorBlock.h"
#include "JSystem/J3D/J3DTexGenBlock.h"
#include "JSystem/JSupport.h"

static inline u32 getMdlDataFlag_TevStageNum(u32 flags) { return (flags & 0x001F0000) >> 0x10; }
static inline u32 getMdlDataFlag_TexGenFlag(u32 flags) { return flags & 0x0C000000; }
static inline u32 getMdlDataFlag_ColorFlag(u32 flags) { return flags & 0xC0000000; }
static inline u32 getMdlDataFlag_PEFlag(u32 flags) { return flags & 0x30000000; }

inline J3DMaterial::J3DMaterial() { initialize(); }

J3DMaterialFactory_v21::J3DMaterialFactory_v21(const J3DMaterialBlock_v21& block)
{
    mMaterialNum           = block.mCount;
    mpMaterialInitData     = JSUConvertOffsetToPtr<J3DMaterialInitData_v21>(&block, block._0C);
    mpMaterialID           = JSUConvertOffsetToPtr<u16>(&block, block._10);
    mpCullMode             = JSUConvertOffsetToPtr<_GXCullMode>(&block, block._18);
    mpMatColor             = JSUConvertOffsetToPtr<GXColor>(&block, block._1C);
    mpColorChanNum         = JSUConvertOffsetToPtr<u8>(&block, block._20);
    mpColorChanInfo        = JSUConvertOffsetToPtr<J3DColorChanInfo>(&block, block._24);
    mpTexGenNum            = JSUConvertOffsetToPtr<u8>(&block, block._28);
    mpTexCoordInfo         = JSUConvertOffsetToPtr<J3DTexCoordInfo>(&block, block._2C);
    mpTexCoord2Info        = JSUConvertOffsetToPtr<J3DTexCoord2Info>(&block, block._30);
    mpTexMtxInfo           = JSUConvertOffsetToPtr<J3DTexMtxInfo>(&block, block._34);
    field_0x28             = JSUConvertOffsetToPtr<J3DTexMtxInfo>(&block, block._38);
    mpTexNo                = JSUConvertOffsetToPtr<u16>(&block, block._3C);
    mpTevOrderInfo         = JSUConvertOffsetToPtr<J3DTevOrderInfo>(&block, block._40);
    mpTevColor             = JSUConvertOffsetToPtr<GXColorS10>(&block, block._44);
    mpTevKColor            = JSUConvertOffsetToPtr<GXColor>(&block, block._48);
    mpTevStageNum          = JSUConvertOffsetToPtr<u8>(&block, block._4C);
    mpTevStageInfo         = JSUConvertOffsetToPtr<J3DTevStageInfo>(&block, block._50);
    mpTevSwapModeInfo      = JSUConvertOffsetToPtr<J3DTevSwapModeInfo>(&block, block._54);
    mpTevSwapModeTableInfo = JSUConvertOffsetToPtr<J3DTevSwapModeTableInfo>(&block, block._58);
    mpFogInfo              = JSUConvertOffsetToPtr<J3DFogInfo>(&block, block._5C);
    mpAlphaCompInfo        = JSUConvertOffsetToPtr<J3DAlphaCompInfo>(&block, block._60);
    mpBlendInfo            = JSUConvertOffsetToPtr<J3DBlendInfo>(&block, block._64);
    mpZModeInfo            = JSUConvertOffsetToPtr<J3DZModeInfo>(&block, block._68);
    mpZCompLoc             = JSUConvertOffsetToPtr<u8>(&block, block._6C);
    mpDither               = JSUConvertOffsetToPtr<u8>(&block, block._70);
    mpNBTScaleInfo         = JSUConvertOffsetToPtr<J3DNBTScaleInfo>(&block, block._74);
}

u16 J3DMaterialFactory_v21::countUniqueMaterials()
{
    u16 i;
    for (i = 0; i < mMaterialNum; i++) {
    }
    return i;
}

u32 J3DMaterialFactory_v21::countTexGens(int matID) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[matID]];
    if (initData->mTexGenNumIdx != 0xFF)
        return mpTexGenNum[initData->mTexGenNumIdx];
    return 0;
}

u32 J3DMaterialFactory_v21::countStages(int matID) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[matID]];
    u32 texNoCount = 0;
    u32 stageNum   = 0;
    if (initData->mTevStageNumIdx != 0xFF)
        stageNum = mpTevStageNum[initData->mTevStageNumIdx];
    for (int i = 0; i < 8; i++) {
        if (initData->mTexNoIdx[i] != 0xFFFF)
            texNoCount++;
    }
    if (stageNum != texNoCount && texNoCount != 0)
        return stageNum > texNoCount ? stageNum : texNoCount;
    return stageNum;
}

J3DMaterial* J3DMaterialFactory_v21::create(J3DMaterial* material, int matID, u32 flags) const
{
    u32 stageNum    = countStages(matID);
    u32 tevFlag     = getMdlDataFlag_TevStageNum(flags);
    u32 tevStageNum = stageNum > tevFlag ? stageNum : tevFlag;
    u32 texNum      = tevStageNum > 8 ? 8 : tevStageNum;

    u32 texGenNum  = countTexGens(matID);
    u32 texGenFlag = texGenNum > 4 ? 0 : getMdlDataFlag_TexGenFlag(flags);
    u32 colorFlag  = getMdlDataFlag_ColorFlag(flags);
    u32 peFlag     = getMdlDataFlag_PEFlag(flags);
    BOOL indFlag   = (flags & J3DMLF_Material_UseIndirect) ? TRUE : FALSE;

    if (!material) {
        material = new J3DMaterial();
    }

    material->mColorBlock  = J3DMaterial::createColorBlock(colorFlag);
    material->mTexGenBlock = J3DMaterial::createTexGenBlock(texGenFlag);
    material->mTevBlock    = J3DMaterial::createTevBlock((u16)tevStageNum);
    material->mIndBlock    = J3DMaterial::createIndBlock(indFlag);
    material->mPeBlock     = J3DMaterial::createPEBlock(peFlag, getMaterialMode(matID));
    material->mIndex       = matID;
    material->mDrawMode    = getMaterialMode(matID);
    material->mColorBlock->setColorChanNum(newColorChanNum(matID));
    material->mColorBlock->setCullMode(newCullMode(matID));
    material->mTexGenBlock->setTexGenNum(newTexGenNum(matID));
    material->mTexGenBlock->setNBTScale(newNBTScale(matID));
    material->mPeBlock->setFog(newFog(matID));
    material->mPeBlock->setAlphaComp(newAlphaComp(matID));
    material->mPeBlock->setBlend(newBlend(matID));
    material->mPeBlock->setZMode(newZMode(matID));
    material->mPeBlock->setZCompLoc(newZCompLoc(matID));
    material->mPeBlock->setDither(newDither(matID));
    material->mTevBlock->setTevStageNum(newTevStageNum(matID));

    for (u8 i = 0; i < texNum; i++)
        material->mTevBlock->setTexNo(i, newTexNo(matID, i));
    for (u8 i = 0; i < tevStageNum; i++)
        material->mTevBlock->setTevOrder(i, newTevOrder(matID, i));
    for (u8 i = 0; i < tevStageNum; i++) {
        J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[matID]];
        material->mTevBlock->setTevStage(i, newTevStage(matID, i));
        if (initData->mTevSwapModeIdx[i] != 0xFFFF) {
            material->mTevBlock->getTevStage(i)->setTexSel(mpTevSwapModeInfo[initData->mTevSwapModeIdx[i]].mTexSel);
            material->mTevBlock->getTevStage(i)->setRasSel(mpTevSwapModeInfo[initData->mTevSwapModeIdx[i]].mRasSel);
        }
    }
    for (u8 i = 0; i < 4; i++)
        material->mTevBlock->setTevKColor(i, newTevKColor(matID, i));
    for (u8 i = 0; i < 4; i++)
        material->mTevBlock->setTevColor(i, newTevColor(matID, i));
    for (u8 i = 0; i < 4; i++)
        material->mTevBlock->setTevSwapModeTable(i, newTevSwapModeTable(matID, i));
    for (u8 i = 0; i < 2; i++)
        material->mColorBlock->setMatColor(i, newMatColor(matID, i));
    for (u8 i = 0; i < 4; i++) {
        J3DColorChan colorChan = newColorChan(matID, i);
        material->mColorBlock->setColorChan(i, colorChan);
    }
    for (u8 i = 0; i < texGenNum; i++) {
        J3DTexCoord texCoord = newTexCoord(matID, i);
        material->mTexGenBlock->setTexCoord(i, &texCoord);
    }
    for (u8 i = 0; i < 8; i++) {
        material->mTexGenBlock->setTexMtx(i, newTexMtx(matID, i));
    }
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[matID]];
    for (u8 i = 0; i < tevStageNum; i++) {
        if (initData->mTevKColorSel[i] != 0xFF) {
            material->mTevBlock->setTevKColorSel(i, initData->mTevKColorSel[i]);
        } else {
            material->mTevBlock->setTevKColorSel(i, 0xC);
        }
    }
    for (u8 i = 0; i < tevStageNum; i++) {
        if (initData->mTevKAlphaSel[i] != 0xFF) {
            material->mTevBlock->setTevKAlphaSel(i, initData->mTevKAlphaSel[i]);
        } else {
            material->mTevBlock->setTevKAlphaSel(i, 0x1C);
        }
    }
    return material;
}

J3DGXColor J3DMaterialFactory_v21::newMatColor(int idx, int stage) const
{
    GXColor dflt = {0xFF, 0xFF, 0xFF, 0xFF};

    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mMatColorIdx[stage] != 0xFFFF)
        return mpMatColor[initData->mMatColorIdx[stage]];
    else
        return dflt;
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

J3DColorChan J3DMaterialFactory_v21::newColorChan(int idx, int stage) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mColorChanIdx[stage] != 0xFFFF)
        return J3DColorChan(mpColorChanInfo[initData->mColorChanIdx[stage]]);
    else
        return J3DColorChan();
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

J3DTexCoord J3DMaterialFactory_v21::newTexCoord(int idx, int stage) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mTexCoordIdx[stage] != 0xFFFF)
        return J3DTexCoord(mpTexCoordInfo[initData->mTexCoordIdx[stage]]);
    else
        return J3DTexCoord();
}

J3DTexMtx* J3DMaterialFactory_v21::newTexMtx(int idx, int stage) const
{
    J3DTexMtx* ret = nullptr;
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mTexMtxIdx[stage] != 0xFFFF)
        ret = new J3DTexMtx(mpTexMtxInfo[initData->mTexMtxIdx[stage]]);

    return ret;
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

J3DGXColorS10 J3DMaterialFactory_v21::newTevColor(int idx, int stage) const
{
    GXColorS10 dflt = {0, 0, 0, 0};

    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mTevColorIdx[stage] != 0xFFFF)
        return mpTevColor[initData->mTevColorIdx[stage]];
    else
        return dflt;
}

J3DGXColor J3DMaterialFactory_v21::newTevKColor(int idx, int stage) const
{
    GXColor dflt = {0xFF, 0xFF, 0xFF, 0xFF};

    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mTevKColorIdx[stage] != 0xFFFF)
        return mpTevKColor[initData->mTevKColorIdx[stage]];
    else
        return dflt;
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

J3DTevStage J3DMaterialFactory_v21::newTevStage(int idx, int stage) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mTevStageIdx[stage] != 0xFFFF)
        return J3DTevStage(mpTevStageInfo[initData->mTevStageIdx[stage]]);
    else
        return J3DTevStage();
}

J3DTevSwapModeTable J3DMaterialFactory_v21::newTevSwapModeTable(int idx, int stage) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mTevSwapModeTableIdx[stage] != 0xFFFF)
        return J3DTevSwapModeTable(mpTevSwapModeTableInfo[initData->mTevSwapModeTableIdx[stage]]);
    else
        return J3DTevSwapModeTable(j3dDefaultTevSwapModeTable);
}

J3DMaterialInitData_v21& J3DMaterialFactory_v21::getMaterialInitData_v21(u16 idx) const
{
    return mpMaterialInitData[mpMaterialID[idx]];
}

J3DFog J3DMaterialFactory_v21::newFog(int idx) const
{
    J3DFog fog;
    J3DMaterialInitData_v21& data = mpMaterialInitData[mpMaterialID[idx]];
    if (data.mFogIdx != 0xFFFF)
        fog.setFogInfo(mpFogInfo[data.mFogIdx]);
    return fog;
}

J3DAlphaComp J3DMaterialFactory_v21::newAlphaComp(int idx) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mAlphaCompIdx != 0xFFFF)
        return J3DAlphaComp(mpAlphaCompInfo[initData->mAlphaCompIdx]);
    else
        return J3DAlphaComp(0xFFFF);
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

J3DZMode J3DMaterialFactory_v21::newZMode(int idx) const
{
    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mZModeIdx != 0xFF)
        return J3DZMode(mpZModeInfo[initData->mZModeIdx]);
    else {
        J3DZMode dflt;
        dflt.mZModeID = j3dDefaultZModeID;
        return dflt;
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

J3DNBTScale J3DMaterialFactory_v21::newNBTScale(int idx) const
{
    J3DNBTScale dflt;

    J3DMaterialInitData_v21* initData = &mpMaterialInitData[mpMaterialID[idx]];

    if (initData->mNBTScaleIdx != 0xFFFF)
        return J3DNBTScale(mpNBTScaleInfo[initData->mNBTScaleIdx]);
    else
        return dflt;
}
