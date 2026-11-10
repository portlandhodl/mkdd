#include "JSystem/J3D/J3DMaterialFactory.h"
#include "JSystem/J3D/J3DColorBlock.h"
#include "JSystem/J3D/J3DInd.h"
#include "JSystem/J3D/J3DPE.h"
#include "JSystem/J3D/J3DTexGenBlock.h"
#include "JSystem/JSupport.h"

static inline u32 getMdlDataFlag_TevStageNum(u32 flags) { return (flags & 0x001F0000) >> 0x10; }
static inline u32 getMdlDataFlag_TexGenFlag(u32 flags) { return flags & 0x0C000000; }
static inline u32 getMdlDataFlag_ColorFlag(u32 flags) { return flags & 0xC0000000; }
static inline u32 getMdlDataFlag_PEFlag(u32 flags) { return flags & 0x30000000; }

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

J3DMaterial* J3DMaterialFactory::createNormalMaterial(J3DMaterial* material, int matID, u32 flags) const
{
	if (_78) {
		return createLockedMaterial(material, matID, flags);
	}

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

	material->mColorBlock   = J3DMaterial::createColorBlock(colorFlag);
	material->mTexGenBlock  = J3DMaterial::createTexGenBlock(texGenFlag);
	material->mTevBlock     = J3DMaterial::createTevBlock((u16)tevStageNum);
	material->mIndBlock     = J3DMaterial::createIndBlock(indFlag);
	material->mPeBlock      = J3DMaterial::createPEBlock(peFlag, getMaterialMode(matID));
	material->mIndex        = matID;
	material->mDrawMode = getMaterialMode(matID);
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
		J3DMaterialInitData& initData = _04[_08[matID]];
		material->mTevBlock->setTevStage(i, newTevStage(matID, i));
		if (initData._104[i] != 0xFFFF) {
			material->mTevBlock->getTevStage(i)->setTexSel(_54[initData._104[i]].mTexSel);
			material->mTevBlock->getTevStage(i)->setRasSel(_54[initData._104[i]].mRasSel);
		}
	}
	for (u8 i = 0; i < 4; i++)
		material->mTevBlock->setTevKColor(i, newTevKColor(matID, i));
	for (u8 i = 0; i < 4; i++)
		material->mTevBlock->setTevColor(i, newTevColor(matID, i));
	for (u8 i = 0; i < 4; i++)
		material->mTevBlock->setTevSwapModeTable(i, newTevSwapModeTable(matID, i));
	for (u8 i = 0; i < 2; i++)
		material->mColorBlock->setAmbColor(i, newAmbColor(matID, i));
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
	J3DMaterialInitData& initData = _04[_08[matID]];
	for (u8 i = 0; i < tevStageNum; i++) {
		if (initData.mTevKColorSelIndices[i] != 0xff) {
			material->mTevBlock->setTevKColorSel(i, initData.mTevKColorSelIndices[i]);
		} else {
			material->mTevBlock->setTevKColorSel(i, 0xc);
		}
	}
	for (u8 i = 0; i < tevStageNum; i++) {
		if (initData.mTevKAlphaSelIndices[i] != 0xff) {
			material->mTevBlock->setTevKAlphaSel(i, initData.mTevKAlphaSelIndices[i]);
		} else {
			material->mTevBlock->setTevKAlphaSel(i, 0x1C);
		}
	}
	if (_0C) {
		u8 indTexStageNum = newIndTexStageNum(matID);
		material->mIndBlock->setIndTexStageNum(newIndTexStageNum(matID));
		for (u8 i = 0; i < indTexStageNum; i++)
			material->mIndBlock->setIndTexMtx(i, newIndTexMtx(matID, i));
		for (u8 i = 0; i < indTexStageNum; i++)
			material->mIndBlock->setIndTexOrder(i, newIndTexOrder(matID, i));
		for (u8 i = 0; i < indTexStageNum; i++)
			material->mIndBlock->setIndTexCoordScale(i, newIndTexCoordScale(matID, i));
		for (u8 i = 0; i < tevStageNum; i++)
			material->mTevBlock->setIndTevStage(i, newIndTevStage(matID, i));
	}
	return material;
}

J3DPatchedMaterial* J3DMaterialFactory::createPatchedMaterial(J3DMaterial* material, int matID, u32 flags) const
{
	if (material == nullptr) {
		material = new J3DPatchedMaterial();
	}

	bool indFlag          = (flags & 0x03000000) ? true : false; // fakematch?
	material->mColorBlock = J3DMaterial::createColorBlock(0x40000000);

	material->mTexGenBlock  = new J3DTexGenBlockPatched();
	material->mTevBlock     = new J3DTevBlockPatched();
	material->mIndBlock     = J3DMaterial::createIndBlock(indFlag);
	material->mPeBlock      = J3DMaterial::createPEBlock(0x10000000, getMaterialMode(matID));
	material->mIndex        = matID;
	material->mDrawMode = getMaterialMode(matID);
	material->mTevBlock->setTevStageNum(newTevStageNum(matID));
	material->mColorBlock->setColorChanNum(newColorChanNum(matID));
	material->mColorBlock->setCullMode(newCullMode(matID));
	material->mPeBlock->setFog(newFog(matID));
	material->mPeBlock->setAlphaComp(newAlphaComp(matID));
	material->mPeBlock->setBlend(newBlend(matID));
	material->mPeBlock->setZMode(newZMode(matID));
	material->mPeBlock->setZCompLoc(newZCompLoc(matID));
	material->mPeBlock->setDither(newDither(matID));

	u8 tevStageNum = material->mTevBlock->getTevStageNum();
	for (u8 i = 0; i < 8; i++)
		material->mTevBlock->setTexNo(i, newTexNo(matID, i));
	for (u8 i = 0; i < tevStageNum; i++)
		material->mTevBlock->setTevOrder(i, newTevOrder(matID, i));
	for (u8 i = 0; i < 4; i++)
		material->mTevBlock->setTevKColor(i, newTevKColor(matID, i));
	for (u8 i = 0; i < 4; i++)
		material->mTevBlock->setTevColor(i, newTevColor(matID, i));
	for (u8 i = 0; i < tevStageNum; i++) {
		J3DMaterialInitData& initData = _04[_08[matID]];
		material->mTevBlock->setTevStage(i, newTevStage(matID, i));
		if (initData._104[i] != 0xFFFF) {
			material->mTevBlock->getTevStage(i)->setTexSel(_54[initData._104[i]].mTexSel);
			material->mTevBlock->getTevStage(i)->setRasSel(_54[initData._104[i]].mRasSel);
		}
	}
	J3DMaterialInitData& initData = _04[_08[matID]];
	for (u8 i = 0; i < tevStageNum; i++) {
		if (initData.mTevKColorSelIndices[i] != 0xff) {
			material->mTevBlock->setTevKColorSel(i, initData.mTevKColorSelIndices[i]);
		} else {
			material->mTevBlock->setTevKColorSel(i, 0xc);
		}
	}
	for (u8 i = 0; i < 2; i++)
		material->mColorBlock->setMatColor(i, newMatColor(matID, i));
	for (u8 i = 0; i < 4; i++) {
		J3DColorChan colorChan = newColorChan(matID, i);
		material->mColorBlock->setColorChan(i, colorChan);
	}

	u32 texGenNum = countTexGens(matID);
	material->mTexGenBlock->setTexGenNum(newTexGenNum(matID));
	for (u8 i = 0; i < 8; i++)
		material->mTexGenBlock->setTexMtx(i, newTexMtx(matID, i));
	for (u8 i = 0; i < texGenNum; i++) {
		J3DTexCoord texCoord = newTexCoord(matID, i);
		material->mTexGenBlock->setTexCoord(i, &texCoord);
	}
	if (indFlag && _0C) {
		u8 indTexStageNum = newIndTexStageNum(matID);
		material->mIndBlock->setIndTexStageNum(newIndTexStageNum(matID));
		for (u8 i = 0; i < indTexStageNum; i++)
			material->mIndBlock->setIndTexMtx(i, newIndTexMtx(matID, i));
		for (u8 i = 0; i < indTexStageNum; i++)
			material->mIndBlock->setIndTexOrder(i, newIndTexOrder(matID, i));
		for (u8 i = 0; i < indTexStageNum; i++)
			material->mIndBlock->setIndTexCoordScale(i, newIndTexCoordScale(matID, i));
		for (u8 i = 0; i < tevStageNum; i++)
			material->mTevBlock->setIndTevStage(i, newIndTevStage(matID, i));
	}
	return (J3DPatchedMaterial*)material;
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

J3DLockedMaterial* J3DMaterialFactory::createLockedMaterial(J3DMaterial* material, int matID, u32 flags) const
{
    if (material == nullptr) {
        material = new J3DLockedMaterial();
        material->mColorBlock = new J3DColorBlockNull();
        material->mTexGenBlock = new J3DTexGenBlockNull();
        material->mTevBlock = new J3DTevBlockNull();
        material->mIndBlock = new J3DIndBlockNull();
        material->mPeBlock = new J3DPEBlockNull();
        material->mIndex = matID;
        material->mDrawMode = _84[matID];
    }

    J3DCurrentMtxInfo* mtx = &_80[matID];
    material->mCurrentMtx.mMtxIdxRegA = mtx->mMtxIdxRegA;
    material->mCurrentMtx.mMtxIdxRegB = mtx->mMtxIdxRegB;
    material->getColorBlock()->setMatColorOffset(_7C[matID]._00);
    material->getColorBlock()->setColorChanOffset(_7C[matID]._02);
    material->getTexGenBlock()->setTexMtxOffset(_7C[matID]._04);
    material->getTevBlock()->setTexNoOffset(_7C[matID]._06);
    material->getTevBlock()->setTevRegOffset(_7C[matID]._08);
    material->getPEBlock()->setFogOffset(_7C[matID]._0A);
    if (material->mSharedDLObj == nullptr) {
        material->mSharedDLObj = new J3DDisplayListObj();
        material->mSharedDLObj->setSingleDisplayList((void*)(_78[matID]._00 + (u32)&_78[matID]), _78[matID]._04);
    }

    return (J3DLockedMaterial*)material;
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

size_t J3DMaterialFactory::calcSizeNormalMaterial(J3DMaterial* material, int matID, u32 flags) const
{
    u32 size = 0;
    if (_78 != nullptr)
        return calcSizeLockedMaterial(material, matID, flags);

    u32 stages         = countStages(matID);
    u32 tevStageNum    = getMdlDataFlag_TevStageNum(flags);
    u32 tevStageNumMax = stages > tevStageNum ? stages : tevStageNum;
    u32 texGenNum      = countTexGens(matID);
    u32 texGenFlag     = texGenNum > 4 ? getMdlDataFlag_TexGenFlag(0) : getMdlDataFlag_TexGenFlag(flags);
    u32 colorFlag      = getMdlDataFlag_ColorFlag(flags);
    u32 peFlag         = getMdlDataFlag_PEFlag(flags);
    BOOL indFlag       = (flags & 0x1000000) ? TRUE : FALSE;
    if (material == nullptr)
        size += sizeof(J3DMaterial);

    size += J3DMaterial::calcSizeColorBlock(colorFlag);
    size += J3DMaterial::calcSizeTexGenBlock(texGenFlag);
    size += J3DMaterial::calcSizeTevBlock((u16)tevStageNumMax);
    size += J3DMaterial::calcSizeIndBlock(indFlag);
    size += J3DMaterial::calcSizePEBlock(peFlag, getMaterialMode(matID));
    J3DMaterialInitData* initData = &_04[_08[matID]];
    for (u32 i = 0; i < 8; i++) {
        if (initData->mTexMtxIndices[i] != 0xFFFF)
            size += sizeof(J3DTexMtx);
    }

    return size;
}

size_t J3DMaterialFactory::calcSizePatchedMaterial(J3DMaterial* material, int matID, u32 flags) const
{
    u32 size = 0;
    if (material == nullptr)
        size += sizeof(J3DPatchedMaterial);

    u8 indFlag = (flags & 0x3000000) != 0 ? (u8)1 : (u8)0;
    size += J3DMaterial::calcSizeColorBlock(0x40000000);
    size += sizeof(J3DTexGenBlockPatched);
    size += sizeof(J3DTevBlockPatched);
    size += J3DMaterial::calcSizeIndBlock(indFlag);
    size += J3DMaterial::calcSizePEBlock(0x10000000, getMaterialMode(matID));
    J3DMaterialInitData* initData = &_04[_08[matID]];
    for (u32 i = 0; i < 8; i++) {
        if (initData->mTexMtxIndices[i] != 0xFFFF)
            size += sizeof(J3DTexMtx);
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
    J3DMaterialInitData& initData = _04[_08[idx]];
    u16 id = initData.mColorChanIndices[stage];
    if (id != 0xFFFF)
        return J3DColorChan(_18[id]);
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

u32 J3DMaterialFactory::newTexGenNum(int idx) const
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
