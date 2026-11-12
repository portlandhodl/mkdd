#include "JSystem/J3D/J3DJointFactory.h"
#include "JSystem/J3D/J3DMaterialFactory.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DModelLoader.h"
#include "JSystem/J3D/J3DMtxCalc.h"
#include "JSystem/J3D/J3DShapeFactory.h"
#include "JSystem/J3D/J3DTypes.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JSupport.h"
#include "macros.h"

DECL_WEAK J3DModelLoader::~J3DModelLoader()
{
}





DECL_WEAK J3DModelLoader_v21::~J3DModelLoader_v21()
{
}





DECL_WEAK J3DModelLoader_v26::~J3DModelLoader_v26()
{
}





DECL_WEAK J3DModelLoader::J3DModelLoader()
    : mModelData(nullptr)
    , mMaterialTable(nullptr)
    , mShapeBlock(nullptr)
    , mMaterialBlock(nullptr)
    , mHierarchy(nullptr)
    , _18(0)
    , _1A(0)
{
}





J3DModelData* J3DModelLoaderDataBase::load(const void* stream, u32 flags)
{
    if (stream == nullptr) {
        return nullptr;
    }
    const J3DFileHeader* header = (const J3DFileHeader*)stream;
    if (header->mJ3dVersion == 'J3D1' && header->mFileVersion == 'bmd1') {
        return nullptr;
    }
    if (header->mJ3dVersion == 'J3D2' && header->mFileVersion == 'bmd2') {
        J3DModelLoader_v21 loader;
        return loader.load(stream, flags);
    }
    if (header->mJ3dVersion == 'J3D2' && header->mFileVersion == 'bmd3') {
        J3DModelLoader_v26 loader;
        return loader.load(stream, flags);
    }
    return nullptr;
}





J3DMaterialTable* J3DModelLoaderDataBase::loadMaterialTable(const void* stream)
{
    if (stream == nullptr) {
        return nullptr;
    }
    const J3DFileHeader* header = (const J3DFileHeader*)stream;
    if (header->mFileVersion == 'bmt1' && header->mJ3dVersion == 'J3D1') {
        return nullptr;
    }
    if (header->mFileVersion == 'bmt3' && header->mJ3dVersion == 'J3D2') {
        J3DModelLoader_v26 loader;
        return loader.loadMaterialTable(stream);
    }
    return nullptr;
}





J3DModelData* J3DModelLoaderDataBase::loadBinaryDisplayList(const void* stream, u32 flags)
{
    if (stream != nullptr) {
        const J3DFileHeader* header = (const J3DFileHeader*)stream;
        if (header->mJ3dVersion == 'J3D2' && header->mFileVersion == 'bdl1') {
            return nullptr;
        }
        if (header->mJ3dVersion == 'J3D2' && (header->mFileVersion == 'bdl3' || header->mFileVersion == 'bdl4')) {
            J3DModelLoader_v26 loader;
            return loader.loadBinaryDisplayList(stream, flags);
        }
    }
    return nullptr;
}





J3DModelData* J3DModelLoader::load(const void* stream, u32 flags)
{
    JKRHeap::sCurrentHeap->getTotalFreeSize();
    mModelData = new J3DModelData();
    mModelData->clear();
    const J3DFileHeader* header       = reinterpret_cast<const J3DFileHeader*>(stream);
    mModelData->mBmd                  = stream;
    const J3DFileBlockBase* nextBlock = header->getFirstBlock();
    mModelData->mJointTree.mModelDataType = 0;
    mMaterialTable                        = &mModelData->mMaterialTable;
    for (u32 i = 0; i < header->mBlockCount; i++) {
        switch (nextBlock->mBlockType) {
        case J3DFBT_Info:
            readInformation((const J3DModelInfoBlock*)nextBlock, flags);
            break;
        case J3DFBT_Vertex:
            readVertex((const J3DVertexBlock*)nextBlock);
            break;
        case J3DFBT_Envelope:
            readEnvelop((const J3DEnvelopeBlock*)nextBlock);
            break;
        case J3DFBT_Draw:
            readDraw((const J3DDrawBlock*)nextBlock);
            break;
        case J3DFBT_Joint:
            readJoint((const J3DJointBlock*)nextBlock);
            break;
        case J3DFBT_Material:
            readMaterial((const J3DMaterialBlock*)nextBlock, flags);
            break;
        case J3DFBT_MaterialV21:
            readMaterial_v21((const J3DMaterialBlock_v21*)nextBlock, flags);
            break;
        case J3DFBT_Shape:
            readShape((const J3DShapeBlock*)nextBlock, flags);
            break;
        case J3DFBT_Texture:
            readTexture((const J3DTextureBlock*)nextBlock);
            break;
        }
        nextBlock = nextBlock->getNext();
    }
    mModelData->init(mModelData->mJointTree.mHierarchy);
    mModelData->mShapeTable.sortVcdVatCmd();
    mModelData->mJointTree.findImportantMtxIndex();
    setupBBoardInfo();
    if (mModelData->mModelLoaderFlags & J3DMLF_NoMatrixTransform) {
        for (u16 i = 0; i < mModelData->getShapeNum(); i++) {
            mModelData->mShapeTable.getShapeNodePointer(i)->onFlag(J3DShape::NoMtx);
        }
    }
    return mModelData;
}





J3DMaterialTable* J3DModelLoader::loadMaterialTable(const void* stream)
{
    mMaterialTable = new J3DMaterialTable();
    mMaterialTable->clear();
    const J3DFileHeader* header       = reinterpret_cast<const J3DFileHeader*>(stream);
    const J3DFileBlockBase* nextBlock = header->getFirstBlock();
    for (u32 i = 0; i < header->mBlockCount; i++) {
        switch (nextBlock->mBlockType) {
        case J3DFBT_Material:
            readMaterialTable((const J3DMaterialBlock*)nextBlock, 0x51100000);
            break;
        case J3DFBT_MaterialV21:
            readMaterialTable_v21((const J3DMaterialBlock_v21*)nextBlock, 0x51100000);
            break;
        case J3DFBT_Texture:
            readTextureTable((const J3DTextureBlock*)nextBlock);
            break;
        default:
            break;
        }
        nextBlock = nextBlock->getNext();
    }
    if (mMaterialTable->mTextures == nullptr) {
        mMaterialTable->mTextures = new J3DTexture(0, nullptr);
    }
    return mMaterialTable;
}





J3DModelData* J3DModelLoader::loadBinaryDisplayList(const void* stream, u32 flags)
{
    mModelData = new J3DModelData();
    mModelData->clear();
    const J3DFileHeader* header       = reinterpret_cast<const J3DFileHeader*>(stream);
    mModelData->mBmd                  = stream;
    const J3DFileBlockBase* nextBlock = header->getFirstBlock();
    mModelData->mJointTree.mModelDataType = 0x1;
    mMaterialTable                        = &mModelData->mMaterialTable;
    for (u32 i = 0; i < header->mBlockCount; i++) {
        switch (nextBlock->mBlockType) {
        case J3DFBT_Info:
            readInformation((const J3DModelInfoBlock*)nextBlock, flags);
            break;
        case J3DFBT_Vertex:
            readVertex((const J3DVertexBlock*)nextBlock);
            break;
        case J3DFBT_Envelope:
            readEnvelop((const J3DEnvelopeBlock*)nextBlock);
            break;
        case J3DFBT_Draw:
            readDraw((const J3DDrawBlock*)nextBlock);
            break;
        case J3DFBT_Joint:
            readJoint((const J3DJointBlock*)nextBlock);
            break;
        case J3DFBT_Shape:
            readShape((const J3DShapeBlock*)nextBlock, flags);
            break;
        case J3DFBT_Texture:
            readTexture((const J3DTextureBlock*)nextBlock);
            break;
        case J3DFBT_MaterialDL:
            readMaterialDL((const J3DMaterialDLBlock*)nextBlock, flags);
            modifyMaterial(flags);
            break;
        case J3DFBT_Material: {
            u32 matFlags = 0x50100000;
            matFlags |= flags & 0x03000000;
            const J3DMaterialBlock* matBlock = (const J3DMaterialBlock*)nextBlock;
            mMaterialBlock                   = matBlock;
            u32 matType                      = flags & 0x3000;
            if (matType == 0) {
                readMaterial(matBlock, matFlags);
            } else if (matType == 0x2000) {
                readPatchedMaterial(matBlock, matFlags);
            }
            break;
        }
        }
        nextBlock = nextBlock->getNext();
    }
    mModelData->init(mModelData->mJointTree.mHierarchy);
    mModelData->mShapeTable.sortVcdVatCmd();
    mModelData->mJointTree.findImportantMtxIndex();
    setupBBoardInfo();
    mModelData->indexToPtr();
    return mModelData;
}





void J3DModelLoader::setupBBoardInfo()
{
    for (u16 i = 0; i < mModelData->getJointNum(); i++) {
        J3DMaterial* material = mModelData->mJointTree.mJoints[i]->mMaterial;
        if (material) {
            u16 id                     = material->mShape->mId;
            u16* initDataIndexToIDMap  = JSUConvertOffsetToPtr<u16>(mShapeBlock, mShapeBlock->_10);
            J3DShapeInitData* initData = JSUConvertOffsetToPtr<J3DShapeInitData>(mShapeBlock, mShapeBlock->_0C);
            J3DJoint* joint;
            switch (initData[initDataIndexToIDMap[id]].mShapeMtxType) {
            case J3DShapeMtx_Base:
                joint      = mModelData->getJointNodePointer(i);
                joint->mKind = joint->getKind();
                break;
            case J3DShapeMtx_BBoard:
                joint = mModelData->getJointNodePointer(i);
                joint->mKind =  (joint->mKind & ~0xf0) | 0x10;
                mModelData->mBillboardFlag = 1;
                break;
            case J3DShapeMtx_Y_BBoard:
                joint = mModelData->getJointNodePointer(i);
                joint->mKind =  (joint->mKind & ~0xf0) | 0x20;
                mModelData->mBillboardFlag = 1;
                break;
            case J3DShapeMtx_Multi:
                joint      = mModelData->getJointNodePointer(i);
                joint->mKind = joint->getKind();
                break;
            }
        }
    }
}





void J3DModelLoader::readInformation(const J3DModelInfoBlock* block, u32 flags)
{
    J3DMtxCalc* calc              = nullptr;
    mModelData->mModelLoaderFlags = flags | block->_08;
    mModelData->mJointTree.mFlags = mModelData->mModelLoaderFlags;
    switch (mModelData->mModelLoaderFlags & J3DMLF_MtxTypeMask) {
    case 0:
        calc = new J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformBasic, J3DMtxCalcJ3DSysInitBasic>;
        break;
    case J3DMLF_MtxSoftImageCalc:
        calc = new J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformSoftimage, J3DMtxCalcJ3DSysInitSoftimage>;
        break;
    case J3DMLF_MtxMayaCalc:
        calc = new J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformMaya, J3DMtxCalcJ3DSysInitMaya>;
        break;
    }
    mModelData->mJointTree.mTransformCalc = calc;
    mModelData->mVertexData.mPacketNum    = block->_0C;
    mModelData->mVertexData.mVtxNum       = block->_10;
    mModelData->mJointTree.mHierarchy     = JSUConvertOffsetToPtr<J3DModelHierarchy>(block, block->_14);
}





GXCompType getFmtType(GXVtxAttrFmtList* fmtList, GXAttr attr)
{
    for (; fmtList->attr != GX_VA_NULL; fmtList++) {
        if (fmtList->attr == attr) {
            return fmtList->type;
        }
    }
    return GX_F32;
}





void J3DModelLoader::readVertex(const J3DVertexBlock* block)
{
    J3DModelData* model       = mModelData;
    J3DVertexData* vertData   = model->getVertexData();
    vertData->mVtxAttrFmtList = JSUConvertOffsetToPtr<GXVtxAttrFmtList>(block, block->_08);
    vertData->mVtxPos         = JSUConvertOffsetToPtr<void>(block, block->_0C);
    vertData->mVtxNorm        = JSUConvertOffsetToPtr<void>(block, block->_10);
    vertData->mVtxNBT         = JSUConvertOffsetToPtr<void>(block, block->_14);
    for (int i = 0; i < 2; i++) {
        vertData->mVtxColor[i] = (GXColor*)JSUConvertOffsetToPtr<void>(block, block->_18[i]);
    }
    for (int i = 0; i < 8; i++) {
        vertData->mVtxTexCoord[i] = JSUConvertOffsetToPtr<void>(block, block->_20[i]);
    }

    GXCompType nrm_type = getFmtType(vertData->mVtxAttrFmtList, GX_VA_NRM);
    u32 nrm_size        = nrm_type == GX_F32 ? 12 : 6;

    void* nrm_end = nullptr;
    if (vertData->mVtxNBT != nullptr) {
        nrm_end = vertData->mVtxNBT;
    } else if (vertData->mVtxColor[0] != nullptr) {
        nrm_end = vertData->mVtxColor[0];
    } else if (vertData->mVtxTexCoord[0] != nullptr) {
        nrm_end = vertData->mVtxTexCoord[0];
    }

    if (vertData->mVtxNorm == nullptr) {
        vertData->mNormNum = 0;
    } else if (nrm_end != nullptr) {
        vertData->mNormNum = ((u32)nrm_end - (u32)vertData->mVtxNorm) / nrm_size + 1;
    } else {
        vertData->mNormNum = (block->mSize - (u32)block->_10) / nrm_size + 1;
    }

    void* color0_end = nullptr;
    if (vertData->mVtxColor[1] != nullptr) {
        color0_end = vertData->mVtxColor[1];
    } else if (vertData->mVtxTexCoord[0] != nullptr) {
        color0_end = vertData->mVtxTexCoord[0];
    }

    if (vertData->mVtxColor[0] == nullptr) {
        vertData->mColorNum = 0;
    } else if (color0_end != nullptr) {
        vertData->mColorNum = ((u32)color0_end - (u32)vertData->mVtxColor[0]) / 4 + 1;
    } else {
        vertData->mColorNum = (block->mSize - (u32)block->_18[0]) / 4 + 1;
    }

    if (vertData->mVtxTexCoord[0] == nullptr) {
        vertData->mTexCoordNum = 0;
    } else {
        vertData->mTexCoordNum = (block->mSize - (u32)block->_20[0]) / 8 + 1;
    }
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





void J3DModelLoader_v26::readMaterial(const J3DMaterialBlock* block, u32 flags)
{
    J3DMaterialFactory factory(*block);
    mMaterialTable->mMaterialNum       = block->mCount;
    mMaterialTable->mMaterialInitNum   = factory.countUniqueMaterials();
    if (block->_14) {
        mMaterialTable->mMaterialNames = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, block->_14));
    } else {
        mMaterialTable->mMaterialNames = nullptr;
    }

    mMaterialTable->mMaterials = new J3DMaterial*[mMaterialTable->mMaterialNum];
    if (flags & J3DMLF_UseUniqueMaterials) {
        mMaterialTable->mUniqueMaterials = new (0x20) J3DMaterial[mMaterialTable->mMaterialInitNum];
    } else {
        mMaterialTable->mUniqueMaterials = nullptr;
    }
    if (flags & J3DMLF_UseUniqueMaterials) {
        for (u16 i = 0; i < mMaterialTable->mMaterialInitNum; i++) {
            u16 idx = i;
            factory.create(&mMaterialTable->mUniqueMaterials[idx], J3DMaterialFactory::NORMAL, idx, flags);
            mMaterialTable->mUniqueMaterials[idx].mMaterialID = (u32)&mMaterialTable->mUniqueMaterials[idx] >> 4;
        }
    }
    for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
        u16 idx                       = i;
        mMaterialTable->mMaterials[i] = factory.create(nullptr, J3DMaterialFactory::NORMAL, idx, flags);
    }
    if (flags & J3DMLF_UseUniqueMaterials) {
        for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
            u16 idx = i;
            mMaterialTable->mMaterials[i]->mMaterialID = (u32)&mMaterialTable->mUniqueMaterials[factory._08[idx]] >> 4;
            mMaterialTable->mMaterials[i]->mOrigMaterial = &mMaterialTable->mUniqueMaterials[factory._08[idx]];
        }
    } else {
        for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
            u16 idx = i;
            mMaterialTable->mMaterials[i]->mMaterialID = ((u32)mMaterialTable->mMaterials >> 4) + factory._08[idx];
        }
    }
}





void J3DModelLoader_v21::readMaterial_v21(const J3DMaterialBlock_v21* block, u32 flags)
{
    J3DMaterialFactory_v21 factory(*block);
    mMaterialTable->mMaterialNum       = block->mCount;
    mMaterialTable->mMaterialInitNum   = factory.countUniqueMaterials();
    if (block->_14) {
        mMaterialTable->mMaterialNames = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, block->_14));
    } else {
        mMaterialTable->mMaterialNames = nullptr;
    }

    mMaterialTable->mMaterials = new J3DMaterial*[mMaterialTable->mMaterialNum];
    if (flags & J3DMLF_UseUniqueMaterials) {
        mMaterialTable->mUniqueMaterials = new (0x20) J3DMaterial[mMaterialTable->mMaterialInitNum];
    } else {
        mMaterialTable->mUniqueMaterials = nullptr;
    }
    if (flags & J3DMLF_UseUniqueMaterials) {
        for (u16 i = 0; i < mMaterialTable->mMaterialInitNum; i++) {
            u16 idx = i;
            factory.create(&mMaterialTable->mUniqueMaterials[idx], idx, flags);
            mMaterialTable->mUniqueMaterials[idx].mMaterialID = (u32)&mMaterialTable->mUniqueMaterials[idx] >> 4;
        }
    }
    for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
        u16 idx                       = i;
        mMaterialTable->mMaterials[i] = factory.create(nullptr, idx, flags);
    }
    if (flags & J3DMLF_UseUniqueMaterials) {
        for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
            u16 idx = i;
            mMaterialTable->mMaterials[i]->mMaterialID = (u32)&mMaterialTable->mUniqueMaterials[factory.mpMaterialID[idx]] >> 4;
            mMaterialTable->mMaterials[i]->mOrigMaterial = &mMaterialTable->mUniqueMaterials[factory.mpMaterialID[idx]];
        }
    } else {
        for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
            mMaterialTable->mMaterials[i]->mMaterialID = 0xC0000000;
        }
    }
}





void J3DModelLoader::readShape(const J3DShapeBlock* block, u32 flags)
{
    mShapeBlock = (J3DShapeBlock*)block;

    J3DModelData* model = mModelData;
    J3DShapeFactory factory(*block);

    model->mShapeTable.mCount = block->_08;
    if (block->_14) {
        model->mShapeTable.mNames = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, (const void*)block->_14));
    } else {
        model->mShapeTable.mNames = nullptr;
    }
    model->mShapeTable.mItems = new J3DShape*[model->mShapeTable.mCount];
    factory.allocVcdVatCmdBuffer(model->mShapeTable.mCount);

    GXVtxDescList* vtxDescList = nullptr;
    for (J3DModelHierarchy* hier = mModelData->mJointTree.mHierarchy; hier->_00; hier++) {
        if (hier->_00 == 0x12) {
            model->mShapeTable.mItems[hier->_02] = factory.create(hier->_02, flags, vtxDescList);
            vtxDescList                          = factory.getVtxDescList(hier->_02);
        }
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





void J3DModelLoader_v26::readMaterialTable(const J3DMaterialBlock* block, u32 flags)
{
    J3DMaterialFactory factory(*block);
    mMaterialTable->mMaterialNum = block->mCount;
    if (block->_14) {
        mMaterialTable->mMaterialNames = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, block->_14));
    } else {
        mMaterialTable->mMaterialNames = nullptr;
    }
    mMaterialTable->mMaterials = new J3DMaterial*[mMaterialTable->mMaterialNum];
    for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
        u16 idx                       = i;
        mMaterialTable->mMaterials[i] = factory.create(nullptr, J3DMaterialFactory::NORMAL, idx, flags);
    }
    for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
        mMaterialTable->mMaterials[i]->mMaterialID = (u32)mMaterialTable->mMaterials + factory._08[(u16)i];
    }
}





void J3DModelLoader_v21::readMaterialTable_v21(const J3DMaterialBlock_v21* block, u32 flags)
{
    J3DMaterialFactory_v21 factory(*block);
    mMaterialTable->mMaterialNum = block->mCount;
    if (block->_14) {
        mMaterialTable->mMaterialNames = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, block->_14));
    } else {
        mMaterialTable->mMaterialNames = nullptr;
    }
    mMaterialTable->mMaterials = new J3DMaterial*[mMaterialTable->mMaterialNum];

    for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
        u16 idx                       = i;
        mMaterialTable->mMaterials[i] = factory.create(nullptr, idx, flags);
    }

    for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
        mMaterialTable->mMaterials[i]->mMaterialID = ((u32)mMaterialTable->mMaterials >> 4) + factory.mpMaterialID[(u16)i];
    }
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





void J3DModelLoader::readPatchedMaterial(const J3DMaterialBlock* block, u32 flags)
{
    J3DMaterialFactory factory(*block);
    mMaterialTable->mMaterialNum       = block->mCount;
    mMaterialTable->mMaterialInitNum   = factory.countUniqueMaterials();
    if (block->_14) {
        mMaterialTable->mMaterialNames = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, block->_14));
    } else {
        mMaterialTable->mMaterialNames = nullptr;
    }
    mMaterialTable->mMaterials       = new J3DMaterial*[mMaterialTable->mMaterialNum];
    mMaterialTable->mUniqueMaterials = nullptr;

    for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
        u16 idx                                  = (u16)i;
        mMaterialTable->mMaterials[i]            = factory.create(nullptr, J3DMaterialFactory::PATCHED, idx, flags);
        mMaterialTable->mMaterials[i]->mMaterialID = ((u32)mMaterialTable->mMaterials >> 4) + factory._08[idx];
    }
}





void J3DModelLoader::readMaterialDL(const J3DMaterialDLBlock* block, u32 flags)
{
    J3DMaterialFactory factory(*block);
    if (mMaterialTable->getMaterialNum() == 0) {
        mMaterialTable->_1C          = 1;
        mMaterialTable->mMaterialNum = block->_08;
        mMaterialTable->mMaterialInitNum = block->_08;
        if (block->_20) {
            mMaterialTable->mMaterialNames = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, (const void*)block->_20));
        } else {
            mMaterialTable->mMaterialNames = nullptr;
        }
        mMaterialTable->mMaterials       = new J3DMaterial*[mMaterialTable->mMaterialNum];
        mMaterialTable->mUniqueMaterials = nullptr;
        for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
            mMaterialTable->mMaterials[i] = factory.create(nullptr, J3DMaterialFactory::LOCKED, i, flags);
        }
        for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
            mMaterialTable->mMaterials[i]->mMaterialID = 0xc0000000;
        }
        return;
    }

    for (u16 i = 0; i < mMaterialTable->mMaterialNum; i++) {
        mMaterialTable->mMaterials[i] = factory.create(mMaterialTable->mMaterials[i], J3DMaterialFactory::LOCKED, i, flags);
    }
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





DECL_WEAK size_t J3DModelLoader::calcSizeMaterial(const J3DMaterialBlock*, u32)
{
    return 0;
}





DECL_WEAK size_t J3DModelLoader::calcSizeMaterialTable(const J3DMaterialBlock*, u32)
{
    return 0;
}






DECL_WEAK J3DMtxCalcNoAnmBase::~J3DMtxCalcNoAnmBase()
{
}





template <>
DECL_WEAK void J3DMtxCalcNoAnm<J3DMtxCalcCalcTransformSoftimage, J3DMtxCalcJ3DSysInitSoftimage>::init(const Vec& p1, const f32 (&p2)[3][4])
{
    J3DSys::mCurrentS.x = p1.x;
    J3DSys::mCurrentS.y = p1.y;
    J3DSys::mCurrentS.z = p1.z;
    PSMTXCopy(p2, J3DSys::mCurrentMtx);
}





DECL_WEAK J3DMaterial::J3DMaterial()
{
    initialize();
}





DECL_WEAK J3DTexture::~J3DTexture()
{
}
