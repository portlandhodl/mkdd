#include "JSystem/J3D/J3DMaterialFactory.h"

namespace {

struct MaterialInitDataV21Layout {
    u8 mMaterialMode;
    u8 mCullModeIdx;
    u8 mColorChanNumIdx;
    u8 mTexGenNumIdx;
    u8 mTevStageNumIdx;
    u8 mZCompLocIdx;
    u8 mZModeIdx;
    u8 mDitherIdx;
    u16 mMatColorIdx[2];
    u16 mColorChanIdx[4];
    u16 mTexCoordIdx[8];
    u8 _24[0x10];
    u16 mTexMtxIdx[8];
    u8 _44[0x2C];
    u16 mTexNoIdx[8];
    u16 mTevKColorIdx[4];
    u8 mTevKColorSel[0x10];
    u8 mTevKAlphaSel[0x10];
    u16 mTevOrderIdx[0x10];
    u16 mTevColorIdx[4];
    u16 mTevStageIdx[0x10];
    u16 mTevSwapModeIdx[0x10];
    u16 mTevSwapModeTableIdx[4];
    u8 _118[0x18];
    u16 mFogIdx;
    u16 mAlphaCompIdx;
    u16 mBlendIdx;
    u16 mNBTScaleIdx;
};

}

extern "C" u16 countUniqueMaterials__22J3DMaterialFactory_v21Fv(
    J3DMaterialFactory_v21* self)
{
    u16 i;
    for (i = 0; i < *reinterpret_cast<const u16*>(self); i++) {
    }
    return i;
}

extern "C" u8 newColorChanNum__22J3DMaterialFactory_v21CFi(
    const J3DMaterialFactory_v21* self, int idx)
{
    const MaterialInitDataV21Layout* initData =
        reinterpret_cast<const MaterialInitDataV21Layout*>(
            self->_04 + reinterpret_cast<const u16*>(self->_08)[idx]);
    if (initData->mColorChanNumIdx != 0xFF) {
        return reinterpret_cast<const u8*>(self->_10)[initData->mColorChanNumIdx];
    } else {
        return 0;
    }
}

extern "C" u32 newTexGenNum__22J3DMaterialFactory_v21CFi(
    const J3DMaterialFactory_v21* self, int idx)
{
    const MaterialInitDataV21Layout* initData =
        reinterpret_cast<const MaterialInitDataV21Layout*>(
            self->_04 + reinterpret_cast<const u16*>(self->_08)[idx]);
    if (initData->mTexGenNumIdx != 0xFF) {
        return reinterpret_cast<const u8*>(self->_18)[initData->mTexGenNumIdx];
    } else {
        return 0;
    }
}

extern "C" u8 newCullMode__22J3DMaterialFactory_v21CFi(
    const J3DMaterialFactory_v21* self, int idx)
{
    const MaterialInitDataV21Layout* initData =
        reinterpret_cast<const MaterialInitDataV21Layout*>(
            self->_04 + reinterpret_cast<const u16*>(self->_08)[idx]);
    if (initData->mCullModeIdx != 0xFF) {
        return reinterpret_cast<const _GXCullMode*>(self->_30)[initData->mCullModeIdx];
    } else {
        return 0xFF;
    }
}

extern "C" u16 newTexNo__22J3DMaterialFactory_v21CFii(
    const J3DMaterialFactory_v21* self, int idx, int stage)
{
    const MaterialInitDataV21Layout* initData =
        reinterpret_cast<const MaterialInitDataV21Layout*>(
            self->_04 + reinterpret_cast<const u16*>(self->_08)[idx]);
    if (initData->mTexNoIdx[stage] != 0xFFFF) {
        return reinterpret_cast<const u16*>(self->_2C)[initData->mTexNoIdx[stage]];
    } else {
        return 0xFFFF;
    }
}

extern "C" J3DTevOrder newTevOrder__22J3DMaterialFactory_v21CFii(
    const J3DMaterialFactory_v21* self, int idx, int stage)
{
    const MaterialInitDataV21Layout* initData =
        reinterpret_cast<const MaterialInitDataV21Layout*>(
            self->_04 + reinterpret_cast<const u16*>(self->_08)[idx]);
    if (initData->mTevOrderIdx[stage] != 0xFFFF) {
        return J3DTevOrder(reinterpret_cast<const J3DTevOrderInfo*>(self->_34)[initData->mTevOrderIdx[stage]]);
    } else {
        return J3DTevOrder();
    }
}

extern "C" u8 newTevStageNum__22J3DMaterialFactory_v21CFi(
    const J3DMaterialFactory_v21* self, int idx)
{
    const MaterialInitDataV21Layout* initData =
        reinterpret_cast<const MaterialInitDataV21Layout*>(
            self->_04 + reinterpret_cast<const u16*>(self->_08)[idx]);
    if (initData->mTevStageNumIdx != 0xFF) {
        return reinterpret_cast<const u8*>(self->_40)[initData->mTevStageNumIdx];
    } else {
        return 0xFF;
    }
}

extern "C" J3DBlend newBlend__22J3DMaterialFactory_v21CFi(
    const J3DMaterialFactory_v21* self, int idx)
{
    const MaterialInitDataV21Layout* initData =
        reinterpret_cast<const MaterialInitDataV21Layout*>(
            self->_04 + reinterpret_cast<const u16*>(self->_08)[idx]);
    if (initData->mBlendIdx != 0xFFFF) {
        return J3DBlend(reinterpret_cast<const J3DBlendInfo*>(self->_58)[initData->mBlendIdx]);
    } else {
        return J3DBlend();
    }
}

extern "C" u8 newZCompLoc__22J3DMaterialFactory_v21CFi(
    const J3DMaterialFactory_v21* self, int idx)
{
    const MaterialInitDataV21Layout* initData =
        reinterpret_cast<const MaterialInitDataV21Layout*>(
            self->_04 + reinterpret_cast<const u16*>(self->_08)[idx]);
    if (initData->mZCompLocIdx != 0xFF) {
        return reinterpret_cast<const u8*>(self->_60)[initData->mZCompLocIdx];
    } else {
        return 0;
    }
}

extern "C" u8 newDither__22J3DMaterialFactory_v21CFi(
    const J3DMaterialFactory_v21* self, int idx)
{
    const MaterialInitDataV21Layout* initData =
        reinterpret_cast<const MaterialInitDataV21Layout*>(
            self->_04 + reinterpret_cast<const u16*>(self->_08)[idx]);
    if (initData->mDitherIdx != 0xFF) {
        return reinterpret_cast<const u8*>(self->_64)[initData->mDitherIdx];
    } else {
        return 1;
    }
}
