#include "JSystem/JUtility/JUTNameTab.h"
#include "JSystem/JSupport.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/J3D/J3DCluster.h"
#include "JSystem/J3D/J3DClusterLoader.h"
#include "JSystem/J3D/J3DSkinDeform.h"
#include <dolphin/os.h>
#include "macros.h"
extern "C" void* __nw__FUl(u32);
extern "C" void __ct__13J3DDeformDataFv(void*);














void* J3DClusterLoaderDataBase::load(const void* data)
{
	const JUTDataFileHeader* header = (const JUTDataFileHeader*)data;
	u32 magic                        = *(const u32*)data;
	if (magic == 0x4A334431 && header->mType == 0x626C7331) {
		J3DClusterLoader_v15 loader;
		return (void*)loader.load(data);
	} else if (magic == 0x4A334432 && header->mType == 0x626C7332) {
		return NULL;
	}
	return NULL;
}

J3DClusterLoader_v15::J3DClusterLoader_v15()
{
}

J3DClusterLoader_v15::~J3DClusterLoader_v15()
{
}

void* J3DClusterLoader_v15::load(const void* data)
{
	mpDeformData = new J3DDeformData;

	const JUTDataFileHeader* header = (const JUTDataFileHeader*)data;
	const JUTDataBlockHeader* block = &header->mFirstBlock;
	for (u32 i = 0; i < header->mBlockNum; i++) {
		switch (block->mType) {
		case 0x434C5331:
			readCluster((const J3DClusterBlock*)block);
			break;
		}
		block = block->getNext();
	}

	return mpDeformData;
}

#pragma dont_inline on

void J3DClusterLoader_v15::readCluster(const J3DClusterBlock* block)
{
	mpDeformData->mClusterNum      = block->mClusterNum;
	mpDeformData->mClusterKeyNum   = block->mClusterKeyNum;
	mpDeformData->mVtxPosNum       = block->mVtxPosNum;
	mpDeformData->mVtxNrmNum       = block->mVtxNrmNum;
	mpDeformData->mClusterVertexNum = block->mClusterVertexNum;

	if (block->mClusterName != NULL) {
		mpDeformData->mClusterName
		    = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, block->mClusterName));
	} else {
		mpDeformData->mClusterName = NULL;
	}

	if (block->mClusterKeyName != NULL) {
		mpDeformData->mClusterKeyName
		    = new JUTNameTab(JSUConvertOffsetToPtr<ResNTAB>(block, block->mClusterKeyName));
	} else {
		mpDeformData->mClusterKeyName = NULL;
	}

	mpDeformData->mVtxPos = JSUConvertOffsetToPtr<f32>(block, block->mVtxPos);
	mpDeformData->mVtxNrm = JSUConvertOffsetToPtr<f32>(block, block->mVtxNrm);

	void* clusterPointer = block->mClusterPointer;
	int keySize = (int)block->mClusterKeyPointer - (int)block->mClusterPointer;
	int vertexSize = (int)block->mClusterVertex - (int)block->mClusterPointer;
	int clusterSize = (int)block->mVtxPos - (int)block->mClusterPointer;
	u8* buffer = new (32) u8[clusterSize];
	memcpy(buffer, JSUConvertOffsetToPtr<J3DCluster>(block, block->mClusterPointer), clusterSize);
	mpDeformData->mClusterPointer = (J3DCluster*)buffer;
	mpDeformData->mClusterKeyPointer = (J3DClusterKey*)&buffer[keySize];
	mpDeformData->mClusterVertex = (J3DClusterVertex*)&buffer[vertexSize];

	for (int i = 0; i < mpDeformData->mClusterNum; i++) {
		J3DCluster* cluster = &mpDeformData->mClusterPointer[i];
		cluster->_08 = (int)JSUConvertOffsetToPtr<J3DClusterKey>(
		    buffer - (int)clusterPointer, (const void*)cluster->_08);
		*(u32*)&cluster->_18[0]
		    = (u32)JSUConvertOffsetToPtr<u16>(block, (const void*)*(u32*)&cluster->_18[0]);
		cluster->_1C = (int)JSUConvertOffsetToPtr<J3DClusterVertex>(
		    buffer - (int)clusterPointer, (const void*)cluster->_1C);

		J3DDeformer* deformer = new J3DDeformer(mpDeformData);
		if (cluster->_14 != 0) {
			deformer->_0C = (int)new f32[cluster->_14 * 3];
		} else {
			deformer->_0C = NULL;
		}
		deformer->mFlags                  = cluster->_0C;
		deformer->_08                     = (int)new f32[cluster->_10];
		cluster->mDeformer                = deformer;
	}

	for (int i = 0; i < mpDeformData->mClusterKeyNum; i++) {
		J3DClusterKey* key = &mpDeformData->mClusterKeyPointer[i];
		key->_04 = JSUConvertOffsetToPtr<u16>(block, (const void*)key->_04);
		key->_08 = JSUConvertOffsetToPtr<u16>(block, (const void*)key->_08);
	}

	for (int i = 0; i < mpDeformData->mClusterVertexNum; i++) {
		J3DClusterVertex* vertex = &mpDeformData->mClusterVertex[i];
		vertex->_04              = JSUConvertOffsetToPtr<u16>(block, (const void*)vertex->_04);
		vertex->_08              = JSUConvertOffsetToPtr<u16>(block, (const void*)vertex->_08);
	}

	DCStoreRange(buffer, clusterSize);
}

#pragma dont_inline off
