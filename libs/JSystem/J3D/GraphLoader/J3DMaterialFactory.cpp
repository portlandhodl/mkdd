#include "JSystem/J3D/J3DMaterialFactory.h"
#include "JSystem/J3D/J3DColorBlock.h"
#include "JSystem/J3D/J3DTexGenBlock.h"
#include "JSystem/JSupport.h"

J3DMaterialFactory::J3DMaterialFactory(const J3DMaterialBlock& block)
{
    _00 = block.mCount;
    _04 = JSUConvertOffsetToPtr<J3DMaterialInitData>(&block, block._0C);
    _08 = JSUConvertOffsetToPtr<u16>(&block, block._10);
    if (block._18 != nullptr && (u32)block._18 - (u32)block._14 > 4)
        _0C = JSUConvertOffsetToPtr<J3DIndInitData>(&block, block._18);
    else
        _0C = nullptr;
    _3C = JSUConvertOffsetToPtr<_GXCullMode>(&block, block._1C);
    _10 = JSUConvertOffsetToPtr<GXColor>(&block, block._20);
    _14 = JSUConvertOffsetToPtr<u8>(&block, block._24);
    _18 = JSUConvertOffsetToPtr<J3DColorChanInfo>(&block, block._28);
    _1C = JSUConvertOffsetToPtr<GXColor>(&block, block._2C);
    _20 = JSUConvertOffsetToPtr<J3DLightInfo>(&block, block._30);
    _24 = JSUConvertOffsetToPtr<u8>(&block, block._34);
    _28 = JSUConvertOffsetToPtr<J3DTexCoordInfo>(&block, block._38);
    _2C = JSUConvertOffsetToPtr<J3DTexCoord2Info>(&block, block._3C);
    _30 = JSUConvertOffsetToPtr<J3DTexMtxInfo>(&block, block._40);
    _34 = JSUConvertOffsetToPtr<J3DTexMtxInfo>(&block, block._44);
    _38 = JSUConvertOffsetToPtr<u16>(&block, block._48);
    _40 = JSUConvertOffsetToPtr<J3DTevOrderInfo>(&block, block._4C);
    _44 = JSUConvertOffsetToPtr<GXColorS10>(&block, block._50);
    _48 = JSUConvertOffsetToPtr<GXColor>(&block, block._54);
    _4C = JSUConvertOffsetToPtr<u8>(&block, block._58);
    _50 = JSUConvertOffsetToPtr<J3DTevStageInfo>(&block, block._5C);
    _54 = JSUConvertOffsetToPtr<J3DTevSwapModeInfo>(&block, block._60);
    _58 = JSUConvertOffsetToPtr<J3DTevSwapModeTableInfo>(&block, block._64);
    _5C = JSUConvertOffsetToPtr<J3DFogInfo>(&block, block._68);
    _60 = JSUConvertOffsetToPtr<J3DAlphaCompInfo>(&block, block._6C);
    _64 = JSUConvertOffsetToPtr<J3DBlendInfo>(&block, block._70);
    _68 = JSUConvertOffsetToPtr<J3DZModeInfo>(&block, block._74);
    _6C = JSUConvertOffsetToPtr<u8>(&block, block._78);
    _70 = JSUConvertOffsetToPtr<u8>(&block, block._7C);
    _74 = JSUConvertOffsetToPtr<J3DNBTScaleInfo>(&block, block._80);
    _78 = nullptr;
    _7C = nullptr;
    _80 = nullptr;
    _84 = nullptr;
}

J3DMaterialFactory::J3DMaterialFactory(const J3DMaterialDLBlock& block)
{
    _00 = block._08;
    _04 = nullptr;
    _78 = JSUConvertOffsetToPtr<J3DDisplayListInit>(&block, block._0C);
    _7C = JSUConvertOffsetToPtr<J3DPatchingInfo>(&block, block._10);
    _80 = JSUConvertOffsetToPtr<J3DCurrentMtxInfo>(&block, block._14);
    _84 = JSUConvertOffsetToPtr<u8>(&block, block._18);
}

u32 J3DMaterialFactory::countUniqueMaterials()
{
    u32 count = 0;
    int maxID = -1;
    for (u16 i = 0; i < _00; i++) {
        if (maxID < _08[i]) {
            count++;
            maxID = _08[i];
        }
    }
    return count;
}

J3DMaterial* J3DMaterialFactory::create(J3DMaterial* material, MaterialType type, int matID, u32 flags) const
{
    switch (type) {
    case NORMAL:
        material = createNormalMaterial(material, matID, flags);
        break;
    case LOCKED:
        material = createLockedMaterial(material, matID, flags);
        break;
    case PATCHED:
        material = createPatchedMaterial(material, matID, flags);
        break;
    }
    return material;
}

void J3DMaterialFactory::modifyPatchedCurrentMtx(J3DMaterial* material, int matID) const
{
    J3DTexCoord coords[8];
    u32 texGenNum = countTexGens(matID);
    for (u8 i = 0; i < texGenNum; i++)
        coords[i] = newTexCoord(matID, i);
    material->mCurrentMtx.mMtxIdxRegA = ((u32)coords[0].getTexGenMtx() << 6) | ((u32)coords[1].getTexGenMtx() << 12)
                                         | ((u32)coords[2].getTexGenMtx() << 18) | ((u32)coords[3].getTexGenMtx() << 24);
    material->mCurrentMtx.mMtxIdxRegB
        = coords[4].getTexGenMtx() | (coords[5].getTexGenMtx() << 6) | (coords[6].getTexGenMtx() << 12)
          | (coords[7].getTexGenMtx() << 18);
}

size_t J3DMaterialFactory::calcSize(J3DMaterial* material, MaterialType type, int matID, u32 flags) const
{
    size_t size = 0;
    switch (type) {
    case NORMAL:
        size = calcSizeNormalMaterial(material, matID, flags);
        break;
    case LOCKED:
        size = calcSizeLockedMaterial(material, matID, flags);
        break;
    case PATCHED:
        size = calcSizePatchedMaterial(material, matID, flags);
        break;
    }
    return size;
}

size_t J3DMaterialFactory::calcSizeLockedMaterial(J3DMaterial* material, int matID, u32 flags) const
{
    u32 size = 0;
    if (material == nullptr) {
        size = sizeof(J3DLockedMaterial) + sizeof(J3DColorBlockNull) + sizeof(J3DTexGenBlockNull) + sizeof(J3DTevBlockNull)
             + sizeof(J3DIndBlockNull) + sizeof(J3DPEBlockNull);
    }
    return size + sizeof(J3DDisplayListObj);
}

J3DGXColor J3DMaterialFactory::newMatColor(int idx, int stage) const
{
    J3DGXColor dflt;
    dflt = j3dDefaultColInfo;

    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mMatColorIndices[stage] != 0xFFFF)
        return _10[initData->mMatColorIndices[stage]];
    else
        return dflt;
}

u8 J3DMaterialFactory::newColorChanNum(int idx) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->_02 != 0xFF)
        return _14[initData->_02];
    else
        return 0;
}

J3DColorChan J3DMaterialFactory::newColorChan(int idx, int stage) const
{
    u32 unused = 0;

    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mColorChanIndices[stage] != 0xFFFF)
        return J3DColorChan(_18[initData->mColorChanIndices[stage]]);
    else
        return J3DColorChan();
}

J3DGXColor J3DMaterialFactory::newAmbColor(int idx, int stage) const
{
    J3DGXColor dflt;
    dflt = j3dDefaultAmbInfo;

    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mAmbColorIndices[stage] != 0xFFFF)
        return _1C[initData->mAmbColorIndices[stage]];
    else
        return dflt;
}

u8 J3DMaterialFactory::newTexGenNum(int idx) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->_03 != 0xFF)
        return _24[initData->_03];
    else
        return 0;
}

J3DTexCoord J3DMaterialFactory::newTexCoord(int idx, int stage) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mTexCoordInfoIndices[stage] != 0xFFFF)
        return J3DTexCoord(_28[initData->mTexCoordInfoIndices[stage]]);
    else
        return J3DTexCoord();
}

J3DTexMtx* J3DMaterialFactory::newTexMtx(int idx, int stage) const
{
    J3DTexMtx* ret = nullptr;
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mTexMtxIndices[stage] != 0xFFFF)
        ret = new J3DTexMtx(_30[initData->mTexMtxIndices[stage]]);

    return ret;
}

u8 J3DMaterialFactory::newCullMode(int idx) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->_01 != 0xFF)
        return _3C[initData->_01];
    else
        return 0xFF;
}

u16 J3DMaterialFactory::newTexNo(int idx, int stage) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mTexNoIndices[stage] != 0xFFFF)
        return _38[initData->mTexNoIndices[stage]];
    else
        return 0xFFFF;
}

J3DTevOrder J3DMaterialFactory::newTevOrder(int idx, int stage) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mTevOrderInfoIndices[stage] != 0xFFFF)
        return J3DTevOrder(_40[initData->mTevOrderInfoIndices[stage]]);
    else
        return J3DTevOrder();
}

J3DGXColorS10 J3DMaterialFactory::newTevColor(int idx, int stage) const
{
    J3DGXColorS10 dflt = j3dDefaultTevColor;

    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mTevColorIndices[stage] != 0xFFFF)
        return _44[initData->mTevColorIndices[stage]];
    else
        return dflt;
}

J3DGXColor J3DMaterialFactory::newTevKColor(int idx, int stage) const
{
    J3DGXColor dflt;
    dflt = j3dDefaultTevKColor;

    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mTevKColorIndices[stage] != 0xFFFF)
        return _48[initData->mTevKColorIndices[stage]];
    else
        return dflt;
}

u8 J3DMaterialFactory::newTevStageNum(int idx) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->_04 != 0xFF)
        return _4C[initData->_04];
    else
        return 0xFF;
}

J3DTevStage J3DMaterialFactory::newTevStage(int idx, int stage) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mTevStageIndices[stage] != 0xFFFF)
        return J3DTevStage(_50[initData->mTevStageIndices[stage]]);
    else
        return J3DTevStage();
}

J3DTevSwapModeTable J3DMaterialFactory::newTevSwapModeTable(int idx, int stage) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mTevSwapModeTableIndices[stage] != 0xFFFF)
        return J3DTevSwapModeTable(_58[initData->mTevSwapModeTableIndices[stage]]);
    else
        return J3DTevSwapModeTable(j3dDefaultTevSwapModeTable);
}

u8 J3DMaterialFactory::newIndTexStageNum(int idx) const
{
    if (_0C[idx].mEnabled == 1)
        return _0C[idx].mIndTexStageNum;
    else
        return 0;
}

J3DIndTexOrder J3DMaterialFactory::newIndTexOrder(int idx, int stage) const
{
    J3DIndTexOrder dflt = j3dDefaultIndTexOrderNull;

    if (_0C[idx].mEnabled == 1)
        return J3DIndTexOrder(_0C[idx].mIndTexOrders[stage]);
    else
        return J3DIndTexOrder(j3dDefaultIndTexOrderNull);
}

J3DIndTexMtx J3DMaterialFactory::newIndTexMtx(int idx, int stage) const
{
    J3DIndTexMtx dflt = j3dDefaultIndTexMtxInfo;
    J3DIndInitData* ind = &_0C[idx];

    if (ind->mEnabled == 1) {
        J3DIndTexMtxInfo& info = ind->mIndTexMatrices[stage];
        return J3DIndTexMtx(info);
    } else
        return dflt;
}

J3DIndTevStage J3DMaterialFactory::newIndTevStage(int idx, int stage) const
{
    J3DIndTevStage ret;
    if (_0C[idx].mEnabled == 1)
        return J3DIndTevStage(_0C[idx].mIndTevStageInfos[stage]);
    return ret;
}

J3DIndTexCoordScale J3DMaterialFactory::newIndTexCoordScale(int idx, int stage) const
{
    J3DIndTexCoordScale dflt = j3dDefaultIndTexCoordScaleInfo;

    if (_0C[idx].mEnabled == 1)
        return J3DIndTexCoordScale(_0C[idx].mIndTexCoordScales[stage]);
    else
        return J3DIndTexCoordScale(j3dDefaultIndTexCoordScaleInfo);
}

J3DFog J3DMaterialFactory::newFog(int idx) const
{
    J3DFog fog;
    J3DMaterialInitData& data = _04[_08[idx]];
    if (data.mFogInfoIndex != 0xFFFF)
        fog.setFogInfo(_5C[data.mFogInfoIndex]);
    return fog;
}

J3DAlphaComp J3DMaterialFactory::newAlphaComp(int idx) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mAlphaCompInfoIndex != 0xFFFF)
        return J3DAlphaComp(_60[initData->mAlphaCompInfoIndex]);
    else
        return J3DAlphaComp(0xFFFF);
}

J3DBlend J3DMaterialFactory::newBlend(int idx) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mBlendInfoIndex != 0xFFFF)
        return J3DBlend(_64[initData->mBlendInfoIndex]);
    else
        return J3DBlend();
}

J3DZMode J3DMaterialFactory::newZMode(int idx) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->_06 != 0xFF)
        return J3DZMode(_68[initData->_06]);
    else {
        J3DZMode dflt;
        dflt.mZModeID = j3dDefaultZModeID;
        return dflt;
    }
}

u8 J3DMaterialFactory::newZCompLoc(int idx) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->_05 != 0xFF)
        return _6C[initData->_05];
    else
        return 0;
}

u8 J3DMaterialFactory::newDither(int idx) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->_07 != 0xFF)
        return _70[initData->_07];
    else
        return 1;
}

J3DNBTScale J3DMaterialFactory::newNBTScale(int idx) const
{
    J3DNBTScale dflt;

    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mNbtScaleInfoIndex != 0xFFFF)
        return J3DNBTScale(_74[initData->mNbtScaleInfoIndex]);
    else
        return dflt;
}
