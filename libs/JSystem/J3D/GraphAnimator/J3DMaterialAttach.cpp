#include "JSystem/J3D/J3DMaterial.h"
#include "JSystem/J3D/J3DTexMtx.h"
#include "macros.h"

extern "C" void* __vt__11J3DTexNoAnm[];

void J3DMaterialTable::clear()
{
	mMaterialNum     = 0;
	mMaterialInitNum = 0;
	mMaterials       = NULL;
	mMaterialNames   = NULL;
	mUniqueMaterials = NULL;
	mTextures        = NULL;
	mTextureNames    = NULL;
	_1C              = 0;
}

J3DMaterialTable::J3DMaterialTable()
{
	clear();
}

J3DMaterialTable::~J3DMaterialTable()
{
}

DECL_WEAK J3DMatColorAnm::J3DMatColorAnm()
    : mIndex(0)
    , mAnmFlag(1)
    , mAnm(NULL)
{
}

DECL_WEAK J3DTexNoAnm::J3DTexNoAnm()
    : _04(0)
    , mAnmFlag(1)
    , mAnm(NULL)
{
}

DECL_WEAK J3DTexMtxAnm::J3DTexMtxAnm()
    : mIndex(0)
    , mAnmFlag(1)
    , mAnm(NULL)
{
}

DECL_WEAK J3DTevKColorAnm::J3DTevKColorAnm()
    : mIndex(0)
    , mAnmFlag(1)
    , mAnm(NULL)
{
}

DECL_WEAK J3DTevColorAnm::J3DTevColorAnm()
    : mIndex(0)
    , mAnmFlag(1)
    , mAnm(NULL)
{
}

int J3DMaterialTable::removeMatColorAnimator(J3DAnmColor* anm)
{
	u16 count;
	int result = 0;
	count = anm->mUpdateMaterialNum;

	for (u16 i = 0; i < count; i++) {
		u16 matID = anm->mUpdateMaterialID[i];
		if (matID != 0xFFFF) {
			J3DMaterialAnm* materialAnm = getMaterialNodePointer(matID)->getMaterialAnm();
			if (materialAnm == NULL) {
				result = 1;
			} else {
				materialAnm->setMatColorAnm(0, NULL);
			}
		}
	}

	return result;
}

int J3DMaterialTable::removeTexNoAnimator(J3DAnmTexPattern* anm)
{
	int result = 0;
	J3DAnmTexPatternFullTable* table;
	const u16 count = anm->mUpdateMaterialNum;
	table           = anm->mAnmTable;

	for (u16 i = 0; i < count; i++) {
		u16 matID = anm->mUpdateMaterialID[i];
		if (matID != 0xFFFF) {
			J3DMaterialAnm* materialAnm = getMaterialNodePointer(matID)->getMaterialAnm();
			u8 texNo                    = ((u8*)&table[i])[4];
			if (materialAnm == NULL) {
				result = 1;
			} else {
				materialAnm->setTexNoAnm(texNo, NULL);
			}
		}
	}

	return result;
}

int J3DMaterialTable::removeTexMtxAnimator(J3DAnmTextureSRTKey* anm)
{
	int result = 0;
	u16 count  = anm->getUpdateMaterialNum();

	for (u16 i = 0; i < count; i++) {
		u16 matID = anm->getUpdateMaterialID(i);
		if (matID != 0xFFFF) {
			J3DMaterial* material       = getMaterialNodePointer(matID);
			J3DMaterialAnm* materialAnm = material->getMaterialAnm();
			u8 texMtxID                 = anm->mUpdateTexMtxID[i];
			if (materialAnm == NULL) {
				result = 1;
			} else if (texMtxID != 0xFF) {
				materialAnm->setTexMtxAnm(texMtxID, NULL);
			}
		}
	}

	return result;
}

int J3DMaterialTable::removeTevRegAnimator(J3DAnmTevRegKey* anm)
{
	u16 cCount;
	u16 kCount;
	int result = 0;
	cCount = anm->mCRegUpdateMaterialNum;
	kCount = anm->mKRegUpdateMaterialNum;

	for (u16 i = 0; i < cCount; i++) {
		u16 matID = anm->mCRegUpdateMaterialID[i];
		if (matID != 0xFFFF) {
			J3DMaterialAnm* materialAnm = getMaterialNodePointer(matID)->getMaterialAnm();
			u8 colorID                  = anm->mCRegKeyTable[i]._18[0];
			if (materialAnm == NULL) {
				result = 1;
			} else {
				materialAnm->setTevColorAnm(colorID, NULL);
			}
		}
	}

	for (u16 i = 0; i < kCount; i++) {
		u16 matID = anm->mKRegUpdateMaterialID[i];
		if (matID != 0xFFFF) {
			J3DMaterialAnm* materialAnm = getMaterialNodePointer(matID)->getMaterialAnm();
			u8 colorID                  = anm->mKRegKeyTable[i]._18[0];
			if (materialAnm == NULL) {
				result = 1;
			} else {
				materialAnm->setTevKColorAnm(colorID, NULL);
			}
		}
	}

	return result;
}

J3DErrType J3DMaterialTable::entryMatColorAnimator(J3DAnmColor* anm)
{
	u16 count;
	int result = 0;
	count = anm->mUpdateMaterialNum;
	if (isLocked()) {
		return JET_LockedModelData;
	}

	for (u16 i = 0; i < count; i++) {
		if (anm->isValidUpdateMaterialID(i)) {
			u16 matID                  = anm->getUpdateMaterialID(i);
			J3DMaterialAnm* materialAnm = getMaterialNodePointer(matID)->getMaterialAnm();
			if (materialAnm == NULL) {
				result = JET_NoMatAnm;
			} else {
				struct {
					u16 index;
					u16 flag;
					J3DAnmColor* anm;
				} matColorAnm;
				matColorAnm.index = i;
				matColorAnm.flag  = 1;
				matColorAnm.anm   = anm;
				materialAnm->setMatColorAnm(0, (J3DMatColorAnm*)&matColorAnm);
			}
		}
	}

	return (J3DErrType)result;
}

J3DErrType J3DMaterialTable::entryTexNoAnimator(J3DAnmTexPattern* anm)
{
	u16 count;
	int result = 0;
	count = anm->mUpdateMaterialNum;
	if (isLocked()) {
		return JET_LockedModelData;
	}

	for (u16 i = 0; i < count; i++) {
		u16 matID = anm->mUpdateMaterialID[i];
		if (matID != 0xFFFF) {
			J3DMaterialAnm* materialAnm = getMaterialNodePointer(matID)->getMaterialAnm();
			u8 texNo                    = ((u8*)&anm->mAnmTable[i])[4];
			if (materialAnm == NULL) {
				result = JET_NoMatAnm;
			} else {
				struct {
					void* vtable;
					u16 index;
					u16 flag;
					J3DAnmTexPattern* anm;
				} texNoAnm;
				texNoAnm.vtable = __vt__11J3DTexNoAnm;
				texNoAnm.index  = i;
				texNoAnm.flag   = 1;
				texNoAnm.anm    = anm;
				materialAnm->setTexNoAnm(texNo, (J3DTexNoAnm*)&texNoAnm);
				texNoAnm.vtable = __vt__11J3DTexNoAnm;
			}
		}
	}

	return (J3DErrType)result;
}

inline int J3DMaterialTable::createTexMtxForAnimator(J3DAnmTextureSRTKey* anm)
{
	u16 matID;
	J3DMaterialAnm* materialAnm;
	u8 texMtxID;
	J3DTexMtx* mtx;
	int result = 0;
	u16 count  = anm->getUpdateMaterialNum();
	if (isLocked()) {
		return 2;
	}

	for (u16 i = 0; i < count; i++) {
		matID = anm->getUpdateMaterialID(i);
		if (matID != 0xFFFF) {
			J3DMaterial* material       = getMaterialNodePointer(matID);
			texMtxID                    = anm->mUpdateTexMtxID[i];
			materialAnm                 = material->getMaterialAnm();
			if (materialAnm == NULL) {
				result = 1;
			} else if (texMtxID != 0xFF && material->getTexMtx(texMtxID) == NULL) {
				mtx = new J3DTexMtx();
				result = 4;
				material->mTexGenBlock->setTexMtx(texMtxID, mtx);
			}
		}
	}

	return result;
}

int J3DMaterialTable::entryTexMtxAnimator(J3DAnmTextureSRTKey* anm)
{
	J3DMaterial* material;
	int result = 0;
	u16 count  = anm->getUpdateMaterialNum();
	result     = createTexMtxForAnimator(anm);
	if (result != 0) {
		return result;
	}

	if (isLocked()) {
		return 2;
	}

	for (u16 no = 0; no < count; no++) {
		const u16 matID = anm->getUpdateMaterialID(no);
		if (matID != 0xFFFF) {
			material                    = getMaterialNodePointer(matID);
			J3DMaterialAnm* materialAnm = material->getMaterialAnm();
			u8 texMtxID                 = anm->mUpdateTexMtxID[no];
			if (materialAnm == NULL) {
				result = 1;
			} else if (texMtxID != 0xFF) {
				if (material->mTexGenBlock->getTexCoord(texMtxID) != NULL) {
					material->mTexGenBlock->getTexCoord(texMtxID)->setTexGenMtx(texMtxID * 3 + 0x1E);
				}
				J3DTexMtxInfo& info = material->getTexMtx(texMtxID)->getTexMtxInfo();
				info.mInfo          = (info.mInfo & 0x3F) | (anm->mTexMtxCalcType << 7);
				info.mCenter.x      = anm->_40[no].x;
				info.mCenter.y      = anm->_40[no].y;
				info.mCenter.z      = anm->_40[no].z;
				struct {
					u16 index;
					u16 flag;
					J3DAnmTextureSRTKey* anm;
				} texMtxAnm = { no, 1, anm };
				materialAnm->setTexMtxAnm(texMtxID, (J3DTexMtxAnm*)&texMtxAnm);
			}
		}
	}

	return result;
}

J3DErrType J3DMaterialTable::entryTevRegAnimator(J3DAnmTevRegKey* anm)
{
	u16 cCount;
	u16 kCount;
	int result = 0;
	cCount = anm->mCRegUpdateMaterialNum;
	kCount = anm->mKRegUpdateMaterialNum;
	if (isLocked()) {
		return JET_LockedModelData;
	}

	for (u16 i = 0; i < cCount; i++) {
		u16 matID = anm->mCRegUpdateMaterialID[i];
		if (matID != 0xFFFF) {
			J3DMaterialAnm* materialAnm = getMaterialNodePointer(matID)->getMaterialAnm();
			u8 colorID                  = anm->mCRegKeyTable[i]._18[0];
			if (materialAnm == NULL) {
				result = JET_NoMatAnm;
			} else {
				struct {
					u16 index;
					u16 flag;
					J3DAnmTevRegKey* anm;
				} tevColorAnm;
				tevColorAnm.index = i;
				tevColorAnm.flag  = 1;
				tevColorAnm.anm   = anm;
				materialAnm->setTevColorAnm(colorID, (J3DTevColorAnm*)&tevColorAnm);
			}
		}
	}

	for (u16 i = 0; i < kCount; i++) {
		u16 matID = anm->mKRegUpdateMaterialID[i];
		if (matID != 0xFFFF) {
			J3DMaterialAnm* materialAnm = getMaterialNodePointer(matID)->getMaterialAnm();
			u8 colorID                  = anm->mKRegKeyTable[i]._18[0];
			if (materialAnm == NULL) {
				result = JET_NoMatAnm;
			} else {
				struct {
					u16 index;
					u16 flag;
					J3DAnmTevRegKey* anm;
				} tevKColorAnm;
				tevKColorAnm.index = i;
				tevKColorAnm.flag  = 1;
				tevKColorAnm.anm   = anm;
				materialAnm->setTevKColorAnm(colorID, (J3DTevKColorAnm*)&tevKColorAnm);
			}
		}
	}

	return (J3DErrType)result;
}
