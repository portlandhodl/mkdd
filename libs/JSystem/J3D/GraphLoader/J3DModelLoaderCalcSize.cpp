#include "JSystem/J3D/J3DJoint.h"
#include "JSystem/J3D/J3DMaterialFactory.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DModelLoader.h"
#include "JSystem/J3D/J3DShapeFactory.h"
#include "JSystem/JSupport.h"
#include "macros.h"

u16 J3DModelLoader::countMaterialNum(const void* stream)
{
    const J3DFileHeader* header = (const J3DFileHeader*)stream;
    u32 count = header->mBlockCount;
    const J3DFileBlockBase* block = header->getFirstBlock();
    for (int i = 0; i < count; block = block->getNext(), i++) {
        if (block->mBlockType == J3DFBT_Material) {
            return ((const J3DMaterialBlock*)block)->mCount;
        }
    }
    return 0;
}

int J3DModelLoader::calcLoadSize(const void* stream, u32 flags)
{
    size_t size = 0;
    size += 0xE4;

    const J3DFileHeader* header = static_cast<const J3DFileHeader*>(stream);
    const J3DFileBlockBase* nextBlock = (const J3DFileBlockBase*)(header + 1);
    for (u32 i = 0; i < header->mBlockCount; i++) {
        switch (nextBlock->mBlockType) {
        case J3DFBT_Info:
            size += calcSizeInformation((const J3DModelInfoBlock*)nextBlock, flags);
            break;
        case J3DFBT_Joint:
            size += calcSizeJoint((const J3DJointBlock*)nextBlock);
            break;
        case J3DFBT_MaterialV21:
            break;
        case J3DFBT_Material:
            size += calcSizeMaterial((const J3DMaterialBlock*)nextBlock, flags);
            break;
        case J3DFBT_Shape:
            size += calcSizeShape((const J3DShapeBlock*)nextBlock, flags);
            break;
        case J3DFBT_Texture:
            size += calcSizeTexture((const J3DTextureBlock*)nextBlock);
            break;
        case J3DFBT_Envelope:
            size += calcSizeEnvelope((const J3DEnvelopeBlock*)nextBlock);
            break;
        case J3DFBT_Draw:
            size += calcSizeDraw((const J3DDrawBlock*)nextBlock);
            break;
        case J3DFBT_Vertex:
            break;
        default:
            break;
        }
        nextBlock = nextBlock->getNext();
    }
    return size;
}

int J3DModelLoader::calcLoadMaterialTableSize(const void* stream)
{
    u32 flags = J3DMLF_21 | J3DMLF_Material_UseIndirect | J3DMLF_Material_PE_Full | J3DMLF_Material_Color_LightOn;
    size_t size = 0;

    size += 0x20;
    const J3DFileBlockBase* nextBlock;
    const J3DFileHeader* header = reinterpret_cast<const J3DFileHeader*>(stream);
    bool hasTextureTable = false;
    nextBlock = header->getFirstBlock();
    for (u32 i = 0; i < header->mBlockCount; i++) {
        switch (nextBlock->mBlockType) {
        case J3DFBT_MaterialV21:
            break;
        case J3DFBT_Material:
            size += calcSizeMaterialTable((const J3DMaterialBlock*)nextBlock, flags);
            break;
        case J3DFBT_Texture:
            size += calcSizeTextureTable((const J3DTextureBlock*)nextBlock);
            hasTextureTable = true;
            break;
        }
        nextBlock = nextBlock->getNext();
    }

    if (!hasTextureTable) {
        size += 0xC;
    }
    return size;
}

inline u32 getBdlFlag_MaterialType(u32 flags)
{
    return flags & (J3DMLF_13 | J3DMLF_DoBdlMaterialCalc);
}

int J3DModelLoader::calcLoadBinaryDisplayListSize(const void* stream, u32 flags)
{
    u32 size = 0;
    size += sizeof(J3DModelData);
    const J3DFileHeader* header = (const J3DFileHeader*)stream;
    const J3DFileBlockBase* nextBlock = (const J3DFileBlockBase*)(header + 1);

    for (u32 i = 0; i < header->mBlockCount; i++) {
        switch (nextBlock->mBlockType) {
        case J3DFBT_Info:
            size += calcSizeInformation((const J3DModelInfoBlock*)nextBlock, flags);
            break;
        case J3DFBT_Joint:
            size += calcSizeJoint((const J3DJointBlock*)nextBlock);
            break;
        case J3DFBT_Shape:
            size += calcSizeShape((const J3DShapeBlock*)nextBlock, flags);
            break;
        case J3DFBT_Texture:
            size += calcSizeTexture((const J3DTextureBlock*)nextBlock);
            break;
        case J3DFBT_MaterialDL:
            size += calcSizeMaterialDL((const J3DMaterialDLBlock*)nextBlock, flags);
            break;
        case J3DFBT_MaterialV21:
            break;
        case J3DFBT_Material:
            u32 flags2 = J3DMLF_21 | J3DMLF_Material_PE_Full | J3DMLF_Material_Color_LightOn;
            flags2 |= (u32)flags & (J3DMLF_Material_UseIndirect | J3DMLF_26);
            mMaterialBlock = (const J3DMaterialBlock*)nextBlock;
            u32 flag_mtl_type = getBdlFlag_MaterialType(flags);
            if (flag_mtl_type == 0) {
                _18 = 1;
                size += calcSizeMaterial((const J3DMaterialBlock*)nextBlock, flags2);
            } else if (flag_mtl_type == J3DMLF_DoBdlMaterialCalc) {
                _18 = 1;
                size += calcSizePatchedMaterial((const J3DMaterialBlock*)nextBlock, flags2);
            }
            break;
        case J3DFBT_Envelope:
            size += calcSizeEnvelope((const J3DEnvelopeBlock*)nextBlock);
            break;
        case J3DFBT_Draw:
            size += calcSizeDraw((const J3DDrawBlock*)nextBlock);
            break;
        case J3DFBT_Vertex:
            break;
        default:
            break;
        }
        nextBlock = nextBlock->getNext();
    }
    return size;
}

int J3DModelLoader::calcSizeInformation(const J3DModelInfoBlock* block, u32 flags)
{
    int size = 0;
    switch ((flags | block->_08) & J3DMLF_MtxTypeMask) {
    case 0:
        size = 4;
        break;
    case J3DMLF_MtxSoftImageCalc:
        size = 4;
        break;
    case J3DMLF_MtxMayaCalc:
        size = 4;
        break;
    default:
        break;
    }
    mHierarchy = JSUConvertOffsetToPtr<J3DModelHierarchy>(block, block->_14);
    return size;
}

int J3DModelLoader::calcSizeJoint(const J3DJointBlock* block)
{
    int size = 0;
    if (block->_14) {
        size = 0x10;
    }
    size += block->_08 * sizeof(J3DJoint*);
    size += block->_08 * sizeof(J3DJoint);
    return size;
}

int J3DModelLoader::calcSizeEnvelope(const J3DEnvelopeBlock* block)
{
    _1A = block->mCount;
    return 0;
}

int J3DModelLoader::calcSizeDraw(const J3DDrawBlock* block)
{
    return (block->mCount - _1A) * 2;
}

size_t J3DModelLoader_v26::calcSizeMaterial(const J3DMaterialBlock* block, u32 flags)
{
    size_t size = 0;
    J3DMaterialFactory factory(*block);
    u32 count = block->mCount;
    u16 uniqueCount = factory.countUniqueMaterials();
    if (block->_14 != nullptr) {
        size += 0x10;
    }
    size += count * sizeof(J3DMaterial*);
    if ((flags & J3DMLF_UseUniqueMaterials) != 0) {
        size += ALIGN_NEXT((u16)uniqueCount * sizeof(J3DMaterial), 0x20);
    }
    if ((flags & J3DMLF_UseUniqueMaterials) != 0) {
        for (u32 i = 0; i < uniqueCount; i++) {
            size += factory.calcSize(nullptr, J3DMaterialFactory::NORMAL, i, flags);
        }
    }
    for (u32 i = 0; i < count; i++) {
        size += factory.calcSize(nullptr, J3DMaterialFactory::NORMAL, i, flags);
    }
    return size;
}

int J3DModelLoader::calcSizeShape(const J3DShapeBlock* block, u32 flags)
{
    int size = 0;
    J3DShapeFactory factory(*block);
    int count = block->_08;
    if (block->_14) {
        size = 0x10;
    }
    size += count * sizeof(J3DShape*);
    size += factory.calcSizeVcdVatCmdBuffer(count);
    for (J3DModelHierarchy* hierarchy = mHierarchy; hierarchy->_00 != 0; hierarchy = hierarchy + 1) {
        if (hierarchy->_00 == 0x12) {
            size += factory.calcSize(hierarchy->_02, flags);
        }
    }
    return size;
}

int J3DModelLoader::calcSizeTexture(const J3DTextureBlock* block)
{
    int padding = 0;
    if (block->_10) {
        padding = 0x10;
    }
    return padding + 0xC;
}

size_t J3DModelLoader_v26::calcSizeMaterialTable(const J3DMaterialBlock* block, u32 flags)
{
    int size = 0;
    u32 count = block->mCount;
    J3DMaterialFactory factory(*block);
    if (block->_14) {
        size = 0x10;
    }
    size += count * sizeof(J3DMaterial*);
    for (u16 i = 0; (u16)i < (u16)count; i++) {
        size += factory.calcSize(nullptr, J3DMaterialFactory::NORMAL, (u16)i, flags);
    }
    return size;
}

int J3DModelLoader::calcSizeTextureTable(const J3DTextureBlock* block)
{
    int padding = 0;
    if (block->_10) {
        padding = 0x10;
    }
    return padding + 0xC;
}

int J3DModelLoader::calcSizePatchedMaterial(const J3DMaterialBlock* block, u32 flags)
{
    int padding = 0;
    u32 count = block->mCount;
    J3DMaterialFactory factory(*block);
    if (block->_14) {
        padding = 0x10;
    }
    u32 size = padding + count * sizeof(J3DMaterial*);
    for (u16 i = 0; (u16)i < (u16)count; i++) {
        size += factory.calcSize(nullptr, J3DMaterialFactory::PATCHED, (u16)i, flags);
    }
    return size;
}

int J3DModelLoader::calcSizeMaterialDL(const J3DMaterialDLBlock* block, u32 flags)
{
    size_t size = 0;
    J3DMaterialFactory factory(*block);
    if (!_18) {
        u32 count = block->_08;
        if (block->_20) {
            size = 0x10;
        }
        size += count * sizeof(J3DLockedMaterial*);
        for (u16 i = 0; (u16)i < (u16)count; i++) {
            size += factory.calcSize(nullptr, J3DMaterialFactory::LOCKED, (u16)i, flags);
        }
    } else {
        u32 count = block->_08;
        for (u16 i = 0; (u16)i < (u16)count; i++) {
            size += factory.calcSize((J3DMaterial*)this, J3DMaterialFactory::LOCKED, (u16)i, flags);
        }
    }
    return size;
}
