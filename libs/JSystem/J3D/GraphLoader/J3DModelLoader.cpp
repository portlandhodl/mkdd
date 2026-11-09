#include "JSystem/J3D/J3DJointFactory.h"
#include "JSystem/J3D/J3DMaterialFactory.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DModelLoader.h"
#include "JSystem/JSupport.h"
#include "macros.h"

DECL_WEAK void J3DModelLoader::readMaterial_v21(const J3DMaterialBlock_v21*, u32)
{
}

DECL_WEAK void J3DModelLoader::readMaterial(const J3DMaterialBlock*, u32)
{
}

DECL_WEAK void J3DModelLoader::readMaterialTable_v21(const J3DMaterialBlock_v21*, u32)
{
}

DECL_WEAK void J3DModelLoader::readMaterialTable(const J3DMaterialBlock*, u32)
{
}

void J3DModelLoader::readEnvelop(const J3DEnvelopeBlock* block)
{
    mModelData->getJointTree().mEnvelopeCnt = block->mCount;
    mModelData->getJointTree().mEnvelopeMixCnt = JSUConvertOffsetToPtr<u8>(block, block->_0C);
    mModelData->getJointTree().mEnvelopeMixIdx = JSUConvertOffsetToPtr<u16>(block, block->_10);
    mModelData->getJointTree().mEnvelopeMixWeight = JSUConvertOffsetToPtr<f32>(block, block->_14);
    mModelData->getJointTree().mInvJointMtx = JSUConvertOffsetToPtr<Mtx>(block, block->_18);
}

void J3DModelLoader::readDraw(const J3DDrawBlock* block)
{
    J3DJointTree& tree = mModelData->getJointTree();
    tree.mMtxData.mCount = block->mCount - tree.mEnvelopeCnt;
    tree.mMtxData.mDrawMtxFlag = JSUConvertOffsetToPtr<u8>(block, block->_0C);
    tree.mMtxData.mDrawMtxIdx = JSUConvertOffsetToPtr<u16>(block, block->_14);

    u16 i;
    for (i = 0; i < tree.mMtxData.mCount; i++) {
        if (tree.mMtxData.mDrawMtxFlag[i] == 1) {
            break;
        }
    }
    tree.mMtxData.mDrawMtxCount = i;
    mModelData->getJointTree().mEnvelopeImptIdx = new u16[tree.mMtxData.mCount];
}

void J3DModelLoader::readJoint(const J3DJointBlock* block)
{
    J3DJointFactory factory(*block);
    mModelData->getJointTree().mJointCnt = block->_08;
    if (block->_14) {
        mModelData->getJointTree().mNametab =
            new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, (const void*)block->_14));
    } else {
        mModelData->getJointTree().mNametab = nullptr;
    }
    mModelData->getJointTree().mJoints = new J3DJoint*[mModelData->getJointTree().mJointCnt];
    for (u16 i = 0; i < mModelData->getJointNum(); i++) {
        mModelData->getJointTree().mJoints[i] = factory.create(i);
    }
}

void J3DModelLoader::readTexture(const J3DTextureBlock* block)
{
    const u16 count = block->_08;
    ResTIMG* resTextureImage = JSUConvertOffsetToPtr<ResTIMG>(block, block->_0C);
    if (block->_10) {
        mMaterialTable->mTextureNames = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, block->_10));
    } else {
        mMaterialTable->mTextureNames = nullptr;
    }
    mMaterialTable->mTextures = new J3DTexture(count, resTextureImage);
}

void J3DModelLoader::readTextureTable(const J3DTextureBlock* block)
{
    const u16 count = block->_08;
    ResTIMG* resTextureImage = JSUConvertOffsetToPtr<ResTIMG>(block, block->_0C);
    if (block->_10) {
        mMaterialTable->mTextureNames = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, block->_10));
    } else {
        mMaterialTable->mTextureNames = nullptr;
    }
    mMaterialTable->mTextures = new J3DTexture(count, resTextureImage);
}

void J3DModelLoader::modifyMaterial(u32 flags)
{
    if ((flags & J3DMLF_DoBdlMaterialCalc) != 0) {
        J3DMaterialFactory factory(*mMaterialBlock);
        for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
            factory.modifyPatchedCurrentMtx(mMaterialTable->mMaterials[i], i);
        }
    }
}

DECL_WEAK size_t J3DModelLoader::calcSizeMaterial(const J3DMaterialBlock*, u32)
{
    return 0;
}

DECL_WEAK size_t J3DModelLoader::calcSizeMaterialTable(const J3DMaterialBlock*, u32)
{
    return 0;
}
