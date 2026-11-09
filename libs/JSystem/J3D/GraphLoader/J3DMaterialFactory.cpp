#include "JSystem/J3D/J3DMaterialFactory.h"
#include "JSystem/JSupport.h"

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

u8 J3DMaterialFactory::newCullMode(int idx) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->_01 != 0xFF)
        return _3C[initData->_01];
    else
        return 0xFF;
}

J3DTexCoord J3DMaterialFactory::newTexCoord(int idx, int stage) const
{
    J3DMaterialInitData* initData = &_04[_08[idx]];

    if (initData->mTexCoordInfoIndices[stage] != 0xFFFF)
        return J3DTexCoord(_28[initData->mTexCoordInfoIndices[stage]]);
    else
        return J3DTexCoord();
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
    if (_0C[idx].mEnabled)
        return _0C[idx].mIndTexStageNum;
    else
        return 0;
}

J3DIndTexOrder J3DMaterialFactory::newIndTexOrder(int idx, int stage) const
{
    J3DIndTexOrder dflt;

    if (_0C[idx].mEnabled)
        return J3DIndTexOrder(_0C[idx].mIndTexOrders[stage]);
    else
        return dflt;
}

J3DIndTexCoordScale J3DMaterialFactory::newIndTexCoordScale(int idx, int stage) const
{
    J3DIndTexCoordScale dflt;

    if (_0C[idx].mEnabled)
        return J3DIndTexCoordScale(_0C[idx].mIndTexCoordScales[stage]);
    else
        return dflt;
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
