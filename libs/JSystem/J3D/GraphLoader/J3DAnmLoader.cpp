#include "JSystem/J3D/J3DAnmLoader.h"
#include "JSystem/J3D/J3DFileBlock.h"
#include "JSystem/JSupport.h"
#include "JSystem/JUtility/JUTNameTab.h"
#include "types.h"

J3DAnmBase* J3DAnmLoaderDataBase::load(const void* data, J3DAnmLoaderDataBaseFlag flag)
{
	if (!data) {
		return NULL;
	}

	const J3DFileHeader* header = (const J3DFileHeader*)data;
	if (header->mJ3dVersion == 'J3D1') {
		switch (header->mFileVersion) {
		case 'bck1': {
			J3DAnmKeyLoader_v15 loader;
			loader.mAnimation = new J3DAnmTransformKey();
			return loader.load(data);
		}
		case 'bpk1': {
			J3DAnmKeyLoader_v15 loader;
			loader.mAnimation = new J3DAnmColorKey();
			return loader.load(data);
		}
		case 'blk1': {
			J3DAnmKeyLoader_v15 loader;
			loader.mAnimation = new J3DAnmClusterKey();
			return loader.load(data);
		}
		case 'btk1': {
			J3DAnmKeyLoader_v15 loader;
			loader.mAnimation = new J3DAnmTextureSRTKey();
			return loader.load(data);
		}
		case 'brk1': {
			J3DAnmKeyLoader_v15 loader;
			loader.mAnimation = new J3DAnmTevRegKey();
			return loader.load(data);
		}
		case 'bxk1': {
			J3DAnmKeyLoader_v15 loader;
			loader.mAnimation = new J3DAnmVtxColorKey();
			return loader.load(data);
		}
		case 'bca1': {
			J3DAnmFullLoader_v15 loader;
			if (flag & J3DLOADER_UNK_FLAG1) {
				loader.mAnimation = new J3DAnmTransformFullWithLerp();
			} else {
				loader.mAnimation = new J3DAnmTransformFull();
			}
			return loader.load(data);
		}
		case 'bpa1': {
			J3DAnmFullLoader_v15 loader;
			loader.mAnimation = new J3DAnmColorFull();
			return loader.load(data);
		}
		case 'btp1': {
			J3DAnmFullLoader_v15 loader;
			loader.mAnimation = new J3DAnmTexPattern();
			return loader.load(data);
		}
		case 'bva1': {
			J3DAnmFullLoader_v15 loader;
			loader.mAnimation = new J3DAnmVisibilityFull();
			return loader.load(data);
		}
		case 'bla1': {
			J3DAnmFullLoader_v15 loader;
			loader.mAnimation = new J3DAnmClusterFull();
			return loader.load(data);
		}
		case 'bxa1': {
			J3DAnmFullLoader_v15 loader;
			loader.mAnimation = new J3DAnmVtxColorFull();
			return loader.load(data);
		}
		default:
			return NULL;
		}
	}
	return NULL;
}

void J3DAnmLoaderDataBase::setResource(J3DAnmBase* resource, const void* data)
{
	if (!data) {
		return;
	}

	const J3DFileHeader* header = (const J3DFileHeader*)data;
	if (header->mJ3dVersion == 'J3D1') {
		switch (header->mFileVersion) {
		case 'bck1': {
			J3DAnmKeyLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		case 'bpk1': {
			J3DAnmKeyLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		case 'blk1': {
			J3DAnmKeyLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		case 'btk1': {
			J3DAnmKeyLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		case 'brk1': {
			J3DAnmKeyLoader_v15 loader;
			if (resource->getKind() == J3DAnmKind_TevRegKey) {
				loader.setResource(resource, data);
			}
			break;
		}
		case 'bxk1': {
			J3DAnmKeyLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		case 'bca1': {
			J3DAnmFullLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		case 'bpa1': {
			J3DAnmFullLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		case 'btp1': {
			J3DAnmFullLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		case 'bla1': {
			J3DAnmFullLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		case 'bxa1': {
			J3DAnmFullLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		case 'bva1': {
			J3DAnmFullLoader_v15 loader;
			loader.setResource(resource, data);
			break;
		}
		default:
			break;
		}
	}
}

J3DAnmFullLoader_v15::J3DAnmFullLoader_v15()
{
}

inline J3DAnmLoader::~J3DAnmLoader()
{
}

J3DAnmFullLoader_v15::~J3DAnmFullLoader_v15()
{
}

J3DAnmKeyLoader_v15::J3DAnmKeyLoader_v15()
{
}

J3DAnmKeyLoader_v15::~J3DAnmKeyLoader_v15()
{
}

J3DAnmBase* J3DAnmFullLoader_v15::load(const void* data)
{
	const J3DFileHeader* header       = (const J3DFileHeader*)data;
	const J3DFileBlockBase* nextBlock = header->getFirstBlock();
	for (u32 i = 0; i < header->mBlockCount; i++) {
		switch (nextBlock->mBlockType) {
		case J3DFBT_AnmTransformFull:
			readAnmTransform((const J3DAnmTransformFullData*)nextBlock);
			break;
		case J3DFBT_AnmColorFull:
			readAnmColor((const J3DAnmColorFullData*)nextBlock);
			break;
		case J3DFBT_AnmTexPattern:
			readAnmTexPattern((const J3DAnmTexPatternFullData*)nextBlock);
			break;
		case J3DFBT_AnmClusterFull:
			readAnmCluster((const J3DAnmClusterFullData*)nextBlock);
			break;
		case J3DFBT_AnmVisibilityFull:
			readAnmVisibility((const J3DAnmVisibilityFullData*)nextBlock);
			break;
		case J3DFBT_AnmVtxColorFull:
			readAnmVtxColor((const J3DAnmVtxColorFullData*)nextBlock);
			break;
		}
		nextBlock = nextBlock->getNext();
	}
	return mAnimation;
}

void J3DAnmFullLoader_v15::setResource(J3DAnmBase* resource, const void* data)
{
	const J3DFileHeader* header       = (const J3DFileHeader*)data;
	const J3DFileBlockBase* nextBlock = header->getFirstBlock();
	for (u32 i = 0; i < header->mBlockCount; i++) {
		switch (nextBlock->mBlockType) {
		case J3DFBT_AnmTransformFull:
			setAnmTransform((J3DAnmTransformFull*)resource, (const J3DAnmTransformFullData*)nextBlock);
			break;
		case J3DFBT_AnmColorFull:
			setAnmColor((J3DAnmColorFull*)resource, (const J3DAnmColorFullData*)nextBlock);
			break;
		case J3DFBT_AnmTexPattern:
			setAnmTexPattern((J3DAnmTexPattern*)resource, (const J3DAnmTexPatternFullData*)nextBlock);
			break;
		case J3DFBT_AnmClusterFull:
			setAnmCluster((J3DAnmClusterFull*)resource, (const J3DAnmClusterFullData*)nextBlock);
			break;
		case J3DFBT_AnmVisibilityFull:
			setAnmVisibility((J3DAnmVisibilityFull*)resource, (const J3DAnmVisibilityFullData*)nextBlock);
			break;
		case J3DFBT_AnmVtxColorFull:
			setAnmVtxColor((J3DAnmVtxColorFull*)resource, (const J3DAnmVtxColorFullData*)nextBlock);
			break;
		}
		nextBlock = nextBlock->getNext();
	}
}

void J3DAnmFullLoader_v15::readAnmTransform(const J3DAnmTransformFullData* data)
{
	setAnmTransform((J3DAnmTransformFull*)mAnimation, data);
}

void J3DAnmFullLoader_v15::setAnmTransform(J3DAnmTransformFull* animation, const J3DAnmTransformFullData* data)
{
	animation->_1E          = data->mExtraInfo1;
	animation->mMaxFrame    = data->mTotalFrameCount;
	animation->mAttribute   = data->mAttribute;
	animation->mCurrentFrame = 0.0f;
	animation->mTable       = JSUConvertOffsetToPtr<J3DAnmTransformFullTable>(data, (void*)data->mTableOffset);
	animation->mScaleVals   = JSUConvertOffsetToPtr<f32>(data, (void*)data->mScaleValOffset);
	animation->mRotationVals = JSUConvertOffsetToPtr<s16>(data, (void*)data->mRotValOffset);
	animation->mTranslationVals = JSUConvertOffsetToPtr<f32>(data, (void*)data->mTransValOffset);
}

void J3DAnmFullLoader_v15::readAnmColor(const J3DAnmColorFullData* data)
{
	setAnmColor((J3DAnmColorFull*)mAnimation, data);
}

void J3DAnmFullLoader_v15::setAnmColor(J3DAnmColorFull* animation, const J3DAnmColorFullData* data)
{
	animation->mMaxFrame    = data->mFrameMax;
	animation->mAttribute   = data->_08;
	animation->mCurrentFrame = 0.0f;
	animation->mUpdateMaterialNum = data->mUpdateMaterialNum;
	animation->mTable       = JSUConvertOffsetToPtr<J3DAnmColorFullTable>(data, (void*)data->mTableOffset);
	animation->_2C          = JSUConvertOffsetToPtr<u8>(data, (void*)data->mRValuesOffset);
	animation->_30          = JSUConvertOffsetToPtr<u8>(data, (void*)data->mGValuesOffset);
	animation->_34          = JSUConvertOffsetToPtr<u8>(data, (void*)data->mBValuesOffset);
	animation->_38          = JSUConvertOffsetToPtr<u8>(data, (void*)data->mAValuesOffset);
	animation->mUpdateMaterialID = JSUConvertOffsetToPtr<u16>(data, (void*)data->mUpdateMaterialIDOffset);
	animation->mNameTab.setResource(JSUConvertOffsetToPtr<ResNTAB>(data, (void*)data->mNameTabOffset));
}

void J3DAnmFullLoader_v15::readAnmTexPattern(const J3DAnmTexPatternFullData* data)
{
	setAnmTexPattern((J3DAnmTexPattern*)mAnimation, data);
}

void J3DAnmFullLoader_v15::setAnmTexPattern(J3DAnmTexPattern* animation, const J3DAnmTexPatternFullData* data)
{
	animation->mMaxFrame    = data->mTotalFrameCount;
	animation->mAttribute   = data->mAttribute;
	animation->mCurrentFrame = 0.0f;
	animation->mUpdateMaterialNum = data->mExtraInfo1;
	animation->_14          = data->mExtraInfo2;
	animation->mAnmTable    = JSUConvertOffsetToPtr<J3DAnmTexPatternFullTable>(data, (void*)data->mTableOffset);
	animation->_0C          = JSUConvertOffsetToPtr<u16>(data, (void*)data->mValuesOffset);
	animation->mUpdateMaterialID = JSUConvertOffsetToPtr<u16>(data, (void*)data->mUpdateMaterialIDOffset);
	animation->mNameTab.setResource(JSUConvertOffsetToPtr<ResNTAB>(data, (void*)data->mNameTabOffset));
}

void J3DAnmFullLoader_v15::readAnmVisibility(const J3DAnmVisibilityFullData* data)
{
	setAnmVisibility((J3DAnmVisibilityFull*)mAnimation, data);
}

void J3DAnmFullLoader_v15::setAnmVisibility(J3DAnmVisibilityFull* animation, const J3DAnmVisibilityFullData* data)
{
	animation->mMaxFrame    = data->mTotalFrameCount;
	animation->mAttribute   = data->mAttribute;
	animation->mCurrentFrame = 0.0f;
	animation->_0C          = data->mExtraInfo1;
	animation->_0E          = data->mExtraInfo2;
	animation->mTable       = JSUConvertOffsetToPtr<J3DAnmVisibilityFullTable>(data, (void*)data->mTableOffset);
	animation->mValues      = JSUConvertOffsetToPtr<u8>(data, (void*)data->mValuesOffset);
}

void J3DAnmFullLoader_v15::readAnmCluster(const J3DAnmClusterFullData* data)
{
	setAnmCluster((J3DAnmClusterFull*)mAnimation, data);
}

void J3DAnmFullLoader_v15::setAnmCluster(J3DAnmClusterFull* animation, const J3DAnmClusterFullData* data)
{
	animation->mMaxFrame    = data->mTotalFrameCount;
	animation->mAttribute   = data->mAttribute;
	animation->mCurrentFrame = 0.0f;
	animation->mTables      = JSUConvertOffsetToPtr<J3DAnmClusterFullTable>(data, data->_10);
	animation->_0C          = JSUConvertOffsetToPtr<f32>(data, data->_14);
}

void J3DAnmFullLoader_v15::readAnmVtxColor(const J3DAnmVtxColorFullData* data)
{
	setAnmVtxColor((J3DAnmVtxColorFull*)mAnimation, data);
}

void J3DAnmFullLoader_v15::setAnmVtxColor(J3DAnmVtxColorFull* animation, const J3DAnmVtxColorFullData* data)
{
	animation->mMaxFrame    = data->mTotalFrameCount;
	animation->mAttribute   = data->mAttribute;
	animation->mCurrentFrame = 0.0f;
	animation->mAnmTableNum[0] = data->mExtraInfo1;
	animation->mAnmTableNum[1] = data->mExtraInfo2;
	animation->mTable[0]    = JSUConvertOffsetToPtr<J3DAnmColorFullTable>(data, (void*)data->mTableOffsets[0]);
	animation->mTable[1]    = JSUConvertOffsetToPtr<J3DAnmColorFullTable>(data, (void*)data->mTableOffsets[1]);
	animation->mAnmVtxColorIndexData[0] = JSUConvertOffsetToPtr<J3DAnmVtxColorIndexData>(data, (void*)data->mVtxColorIndexDataOffsets[0]);
	animation->mAnmVtxColorIndexData[1] = JSUConvertOffsetToPtr<J3DAnmVtxColorIndexData>(data, (void*)data->mVtxColorIndexDataOffsets[1]);
	u16* indexPtr0 = JSUConvertOffsetToPtr<u16>(data, (void*)data->mVtxColorIndexPointerOffsets[0]);
	u16* indexPtr1 = JSUConvertOffsetToPtr<u16>(data, (void*)data->mVtxColorIndexPointerOffsets[1]);

	for (int i = 0; i < animation->mAnmTableNum[0]; i++) {
		animation->mAnmVtxColorIndexData[0][i].mData = (void*)(indexPtr0 + (u32)animation->mAnmVtxColorIndexData[0][i].mData);
	}
	for (int i = 0; i < animation->mAnmTableNum[1]; i++) {
		animation->mAnmVtxColorIndexData[1][i].mData = (void*)(indexPtr1 + (u32)animation->mAnmVtxColorIndexData[1][i].mData);
	}

	animation->mRedVals     = JSUConvertOffsetToPtr<u8>(data, (void*)data->mRValuesOffset);
	animation->mGreenVals   = JSUConvertOffsetToPtr<u8>(data, (void*)data->mGValuesOffset);
	animation->mBlueVals    = JSUConvertOffsetToPtr<u8>(data, (void*)data->mBValuesOffset);
	animation->mAlphaVals   = JSUConvertOffsetToPtr<u8>(data, (void*)data->mAValuesOffset);
}

J3DAnmBase* J3DAnmKeyLoader_v15::load(const void* data)
{
	const J3DFileHeader* header       = (const J3DFileHeader*)data;
	const J3DFileBlockBase* nextBlock = header->getFirstBlock();
	for (u32 i = 0; i < header->mBlockCount; i++) {
		switch (nextBlock->mBlockType) {
		case J3DFBT_AnmTransformKey:
			readAnmTransform((const J3DAnmTransformKeyData*)nextBlock);
			break;
		case J3DFBT_AnmColorKey:
			readAnmColor((const J3DAnmColorKeyData*)nextBlock);
			break;
		case J3DFBT_AnmClusterKey:
			readAnmCluster((const J3DAnmClusterKeyData*)nextBlock);
			break;
		case J3DFBT_AnmTextureSRTKey:
			readAnmTextureSRT((const J3DAnmTextureSRTKeyData*)nextBlock);
			break;
		case J3DFBT_AnmTevRegKey:
			readAnmTevReg((const J3DAnmTevRegKeyData*)nextBlock);
			break;
		case J3DFBT_AnmVtxColorKey:
			readAnmVtxColor((const J3DAnmVtxColorKeyData*)nextBlock);
			break;
		}
		nextBlock = nextBlock->getNext();
	}
	return mAnimation;
}

void J3DAnmKeyLoader_v15::setResource(J3DAnmBase* resource, const void* data)
{
	const J3DFileHeader* header       = (const J3DFileHeader*)data;
	const J3DFileBlockBase* nextBlock = header->getFirstBlock();
	for (u32 i = 0; i < header->mBlockCount; i++) {
		switch (nextBlock->mBlockType) {
		case J3DFBT_AnmTransformKey:
			setAnmTransform((J3DAnmTransformKey*)resource, (const J3DAnmTransformKeyData*)nextBlock);
			break;
		case J3DFBT_AnmColorKey:
			setAnmColor((J3DAnmColorKey*)resource, (const J3DAnmColorKeyData*)nextBlock);
			break;
		case J3DFBT_AnmClusterKey:
			setAnmCluster((J3DAnmClusterKey*)resource, (const J3DAnmClusterKeyData*)nextBlock);
			break;
		case J3DFBT_AnmTextureSRTKey:
			setAnmTextureSRT((J3DAnmTextureSRTKey*)resource, (const J3DAnmTextureSRTKeyData*)nextBlock);
			break;
		case J3DFBT_AnmTevRegKey:
			setAnmTevReg((J3DAnmTevRegKey*)resource, (const J3DAnmTevRegKeyData*)nextBlock);
			break;
		case J3DFBT_AnmVtxColorKey:
			setAnmVtxColor((J3DAnmVtxColorKey*)resource, (const J3DAnmVtxColorKeyData*)nextBlock);
			break;
		}
		nextBlock = nextBlock->getNext();
	}
}

void J3DAnmKeyLoader_v15::readAnmTransform(const J3DAnmTransformKeyData* data)
{
	setAnmTransform((J3DAnmTransformKey*)mAnimation, data);
}

void J3DAnmKeyLoader_v15::setAnmTransform(J3DAnmTransformKey* animation, const J3DAnmTransformKeyData* data)
{
	animation->_1E          = data->mExtraInfo1;
	animation->mMaxFrame    = data->mTotalFrameCount;
	animation->mAttribute   = data->mAttribute;
	animation->_20          = data->mRotationScale;
	animation->mCurrentFrame = 0.0f;
	animation->mTable       = JSUConvertOffsetToPtr<J3DAnmTransformKeyTable>(data, (void*)data->mTableOffset);
	animation->mScaleVals   = JSUConvertOffsetToPtr<f32>(data, (void*)data->mScaleOffset);
	animation->mRotationVals = JSUConvertOffsetToPtr<s16>(data, (void*)data->mRotationOffset);
	animation->mTranslationVals = JSUConvertOffsetToPtr<f32>(data, (void*)data->mTranslateOffset);
}

void J3DAnmKeyLoader_v15::readAnmTextureSRT(const J3DAnmTextureSRTKeyData* data)
{
	setAnmTextureSRT((J3DAnmTextureSRTKey*)mAnimation, data);
}

void J3DAnmKeyLoader_v15::setAnmTextureSRT(J3DAnmTextureSRTKey* animation, const J3DAnmTextureSRTKeyData* data)
{
	animation->mUpdateMaterialNum = data->mExtraInfo1;
	animation->mMaxFrame          = data->mTotalFrameCount;
	animation->mAttribute         = data->mAttribute;
	animation->_0C                = data->mRotationScale;
	animation->mCurrentFrame      = 0.0f;
	animation->mUpdateMaterialNum = data->mExtraInfo1;
	animation->_16                = data->mExtraInfo2;
	animation->_18                = data->mRotationNum;
	animation->_1A                = data->mTranslationNum;
	animation->_10                = JSUConvertOffsetToPtr<J3DAnmTransformKeyTable>(data, (void*)data->mTableOffset);
	animation->mUpdateMaterialID  = JSUConvertOffsetToPtr<u16>(data, (void*)data->mUpdateMatIDOffset);
	animation->_30.setResource(JSUConvertOffsetToPtr<ResNTAB>(data, (void*)data->mNameTab1Offset));
	animation->mUpdateTexMtxID    = JSUConvertOffsetToPtr<u8>(data, (void*)data->mUpdateTexMtxIDOffset);
	animation->_40                = JSUConvertOffsetToPtr<Vec>(data, (void*)data->mSrtCenterOffset);
	animation->_1C                = JSUConvertOffsetToPtr<f32>(data, (void*)data->mScaleValOffset);
	animation->_20                = JSUConvertOffsetToPtr<s16>(data, (void*)data->mRotValOffset);
	animation->_24                = JSUConvertOffsetToPtr<f32>(data, (void*)data->mTransValOffset);
	if (data->mNameTab2Offset) {
		animation->_64.setResource(JSUConvertOffsetToPtr<ResNTAB>(data, (void*)data->mNameTab2Offset));
	}
	animation->_4A                = data->mPostTrackNum;
	animation->_44                = data->_36;
	animation->_46                = data->_38;
	animation->_48                = data->_3A;
	animation->_58                = JSUConvertOffsetToPtr<J3DAnmTransformKeyTable>(data, (void*)data->mTransformKeyTableOffset);
	animation->_60                = JSUConvertOffsetToPtr<u16>(data, (void*)data->mPostUpdateMaterialIDOffset);
	animation->_5C                = JSUConvertOffsetToPtr<u8>(data, (void*)data->mPostUpdateTexMtxIDOffset);
	animation->_74                = JSUConvertOffsetToPtr<Vec>(data, (void*)data->mPostSRTCenterOffset);
	animation->_4C                = JSUConvertOffsetToPtr<f32>(data, (void*)data->_50);
	animation->_50                = JSUConvertOffsetToPtr<s16>(data, (void*)data->_54);
	animation->_54                = JSUConvertOffsetToPtr<f32>(data, (void*)data->_58);

	switch (data->mTexMtxCalcType) {
	case 0:
	case 1:
		animation->mTexMtxCalcType = data->mTexMtxCalcType;
		break;
	default:
		animation->mTexMtxCalcType = 0;
	}
}

void J3DAnmKeyLoader_v15::readAnmColor(const J3DAnmColorKeyData* data)
{
	setAnmColor((J3DAnmColorKey*)mAnimation, data);
}

void J3DAnmKeyLoader_v15::setAnmColor(J3DAnmColorKey* animation, const J3DAnmColorKeyData* data)
{
	animation->mMaxFrame    = data->mTotalFrameCount;
	animation->mAttribute   = data->mAttribute;
	animation->mCurrentFrame = 0.0f;
	animation->mUpdateMaterialNum = data->mUpdateMaterialNum;
	animation->_0C          = data->_10;
	animation->_0E          = data->_12;
	animation->_10          = data->_14;
	animation->_12          = data->_16;
	animation->mTable       = JSUConvertOffsetToPtr<J3DAnmColorKeyTable>(data, (void*)data->mTableOffset);
	animation->_2C          = JSUConvertOffsetToPtr<s16>(data, (void*)data->mRValOffset);
	animation->_30          = JSUConvertOffsetToPtr<s16>(data, (void*)data->mGValOffset);
	animation->_34          = JSUConvertOffsetToPtr<s16>(data, (void*)data->mBValOffset);
	animation->_38          = JSUConvertOffsetToPtr<s16>(data, (void*)data->mAValOffset);
	animation->mUpdateMaterialID = JSUConvertOffsetToPtr<u16>(data, (void*)data->mUpdateMaterialIDOffset);
	animation->mNameTab.setResource(JSUConvertOffsetToPtr<ResNTAB>(data, (void*)data->mNameTabOffset));
}

void J3DAnmKeyLoader_v15::readAnmCluster(const J3DAnmClusterKeyData* data)
{
	setAnmCluster((J3DAnmClusterKey*)mAnimation, data);
}

void J3DAnmKeyLoader_v15::setAnmCluster(J3DAnmClusterKey* animation, const J3DAnmClusterKeyData* data)
{
	animation->mMaxFrame    = data->_0A;
	animation->mAttribute   = data->_08;
	animation->mCurrentFrame = 0.0f;
	animation->mTables      = JSUConvertOffsetToPtr<J3DAnmClusterKeyTable>(data, data->_10);
	animation->_0C          = JSUConvertOffsetToPtr<f32>(data, data->_14);
}

void J3DAnmKeyLoader_v15::readAnmTevReg(const J3DAnmTevRegKeyData* data)
{
	setAnmTevReg((J3DAnmTevRegKey*)mAnimation, data);
}

void J3DAnmKeyLoader_v15::setAnmTevReg(J3DAnmTevRegKey* animation, const J3DAnmTevRegKeyData* data)
{
	animation->mMaxFrame    = data->mTotalFrameCount;
	animation->mAttribute   = data->mAttribute;
	animation->mCurrentFrame = 0.0f;
	animation->mCRegUpdateMaterialNum = data->mCRegUpdateMaterialNum;
	animation->mCRegKeyTable = JSUConvertOffsetToPtr<J3DAnmCRegKeyTable>(data, (void*)data->mCRegTableOffset);
	animation->mCRegUpdateMaterialID = JSUConvertOffsetToPtr<u16>(data, (void*)data->mCRegUpdateMaterialIDOffset);
	animation->mCRegNameTable.setResource(JSUConvertOffsetToPtr<ResNTAB>(data, (void*)data->mCRegNameTabOffset));
	animation->mKRegUpdateMaterialNum = data->mKRegUpdateMaterialNum;
	animation->mKRegKeyTable = JSUConvertOffsetToPtr<J3DAnmKRegKeyTable>(data, (void*)data->mKRegTableOffset);
	animation->mKRegUpdateMaterialID = JSUConvertOffsetToPtr<u16>(data, (void*)data->mKRegUpdateMaterialIDOffset);
	animation->mKRegNameTable.setResource(JSUConvertOffsetToPtr<ResNTAB>(data, (void*)data->mKRegNameTabOffset));
	animation->_10          = data->_10;
	animation->_12          = data->_12;
	animation->_14          = data->_14;
	animation->_16          = data->_16;
	animation->mCRedVals    = JSUConvertOffsetToPtr<s16>(data, (void*)data->mCRValuesOffset);
	animation->mCGreenVals  = JSUConvertOffsetToPtr<s16>(data, (void*)data->mCGValuesOffset);
	animation->mCBlueVals   = JSUConvertOffsetToPtr<s16>(data, (void*)data->mCBValuesOffset);
	animation->mCAlphaVals  = JSUConvertOffsetToPtr<s16>(data, (void*)data->mCAValuesOffset);
	animation->_18          = data->_18;
	animation->_1A          = data->_1A;
	animation->_1C          = data->_1C;
	animation->_1E          = data->_1E;
	animation->mKRedVals    = JSUConvertOffsetToPtr<s16>(data, (void*)data->mKRValuesOffset);
	animation->mKGreenVals  = JSUConvertOffsetToPtr<s16>(data, (void*)data->mKGValuesOffset);
	animation->mKBlueVals   = JSUConvertOffsetToPtr<s16>(data, (void*)data->mKBValuesOffset);
	animation->mKAlphaVals  = JSUConvertOffsetToPtr<s16>(data, (void*)data->mKAValuesOffset);
}

void J3DAnmKeyLoader_v15::readAnmVtxColor(const J3DAnmVtxColorKeyData* data)
{
	setAnmVtxColor((J3DAnmVtxColorKey*)mAnimation, data);
}

void J3DAnmKeyLoader_v15::setAnmVtxColor(J3DAnmVtxColorKey* animation, const J3DAnmVtxColorKeyData* data)
{
	animation->mMaxFrame    = data->mTotalFrameCount;
	animation->mAttribute   = data->mAttribute;
	animation->mCurrentFrame = 0.0f;
	animation->mAnmTableNum[0] = data->mAnmTableNum[0];
	animation->mAnmTableNum[1] = data->mAnmTableNum[1];
	animation->mTable[0]    = JSUConvertOffsetToPtr<J3DAnmColorKeyTable>(data, (void*)data->mTableOffsets[0]);
	animation->mTable[1]    = JSUConvertOffsetToPtr<J3DAnmColorKeyTable>(data, (void*)data->mTableOffsets[1]);
	animation->mAnmVtxColorIndexData[0] = JSUConvertOffsetToPtr<J3DAnmVtxColorIndexData>(data, (void*)data->mVtxColoIndexDataOffset[0]);
	animation->mAnmVtxColorIndexData[1] = JSUConvertOffsetToPtr<J3DAnmVtxColorIndexData>(data, (void*)data->mVtxColoIndexDataOffset[1]);
	u16* indexPtr0 = JSUConvertOffsetToPtr<u16>(data, (void*)data->mVtxColorIndexPointerOffset[0]);
	u16* indexPtr1 = JSUConvertOffsetToPtr<u16>(data, (void*)data->mVtxColorIndexPointerOffset[1]);

	for (int i = 0; i < animation->mAnmTableNum[0]; i++) {
		animation->mAnmVtxColorIndexData[0][i].mData = (void*)(indexPtr0 + (u32)animation->mAnmVtxColorIndexData[0][i].mData);
	}
	for (int i = 0; i < animation->mAnmTableNum[1]; i++) {
		animation->mAnmVtxColorIndexData[1][i].mData = (void*)(indexPtr1 + (u32)animation->mAnmVtxColorIndexData[1][i].mData);
	}

	animation->mRedVals     = JSUConvertOffsetToPtr<s16>(data, (void*)data->mRValOffset);
	animation->mGreenVals   = JSUConvertOffsetToPtr<s16>(data, (void*)data->mGValOffset);
	animation->mBlueVals    = JSUConvertOffsetToPtr<s16>(data, (void*)data->mBValOffset);
	animation->mAlphaVals   = JSUConvertOffsetToPtr<s16>(data, (void*)data->mAValOffset);
}
