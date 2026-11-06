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
    __asm {
        mr r27,r3
        mr r28,r4
        li r3,40
        bl __nw__FUl
        mr. r0,r3
        beq L_load_skip
        bl __ct__13J3DDeformDataFv
        mr r0,r3
    L_load_skip:
        lis r3,17228
        stw r0,4(r27)
        lwz r31,12(r28)
        addi r29,r28,32
        addi r30,r3,21297
        li r28,0
        b L_load_cond
    L_load_body:
        lwz r0,0(r29)
        cmpw r0,r30
        beq L_load_call
        b L_load_next
    L_load_call:
        mr r3,r27
        mr r4,r29
        bl readCluster
    L_load_next:
        lwz r0,4(r29)
        addi r28,r28,1
        add r29,r29,r0
    L_load_cond:
        cmplw r28,r31
        blt L_load_body
        lwz r3,4(r27)
    }
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

	u8* clusterPointer = (u8*)block->mClusterPointer;
	u32 keySize        = (u32)((u8*)block->mClusterKeyPointer - clusterPointer);
	u32 vertexSize     = (u32)((u8*)block->mClusterVertex - clusterPointer);
	J3DCluster* buffer;
	u32 clusterSize = (u32)((u8*)block->mVtxPos - clusterPointer);
	buffer          = (J3DCluster*)new (32) u8[clusterSize];
	memcpy(buffer, JSUConvertOffsetToPtr<J3DCluster>(block, clusterPointer), clusterSize);

	mpDeformData->mClusterPointer    = buffer;
	mpDeformData->mClusterKeyPointer = (J3DClusterKey*)((u8*)buffer + keySize);
	mpDeformData->mClusterVertex     = (J3DClusterVertex*)((u8*)buffer + vertexSize);

	u32 diff = (u32)buffer - (u32)clusterPointer;

	for (int i = 0; i < mpDeformData->mClusterNum; i++) {
		J3DCluster* cluster = &mpDeformData->mClusterPointer[i];
		cluster->_08        = (int)JSUConvertOffsetToPtr<J3DClusterKey>(
            (const void*)diff, (const void*)cluster->_08);
		*(u32*)&cluster->_18[0]
		    = (u32)JSUConvertOffsetToPtr<u16>(block, (const void*)*(u32*)&cluster->_18[0]);
		cluster->_1C = (int)JSUConvertOffsetToPtr<J3DClusterVertex>(
		    (const void*)diff, (const void*)cluster->_1C);

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
