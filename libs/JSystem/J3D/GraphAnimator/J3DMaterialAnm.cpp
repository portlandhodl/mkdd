#include "JSystem/J3D/J3DAnmTexPattern.h"
#include "JSystem/J3D/J3DAnmTextureSRTKey.h"
#include "JSystem/J3D/J3DAnmTevRegKey.h"
#include "JSystem/J3D/J3DColorBlock.h"
#include "JSystem/J3D/J3DMaterial.h"
#include "JSystem/J3D/J3DMaterialAnm.h"
#include "JSystem/J3D/J3DTevBlock.h"
#include "JSystem/J3D/J3DTexGenBlock.h"
#include "JSystem/J3D/J3DTexMtx.h"
#include "macros.h"

J3DMaterialAnm::~J3DMaterialAnm()
{
}

J3DTevKColorAnm::~J3DTevKColorAnm()
{
}

J3DTevColorAnm::~J3DTevColorAnm()
{
}

J3DTexNoAnm::~J3DTexNoAnm()
{
}

void J3DTexNoAnm::calc(u16* pNo) const
{
	J3DAnmTexPattern* anm = mAnm;
	u16* idxp             = (u16*)((u8*)this + 4);
	anm->getTexNo(*idxp, pNo);
}

J3DTexMtxAnm::~J3DTexMtxAnm()
{
}

J3DMatColorAnm::~J3DMatColorAnm()
{
}

void J3DMaterialAnm::initialize()
{
	for (int i = 0; i < 2; i++) {
		mMatColAnmList[i].setAnmFlag(false);
	}
	for (int i = 0; i < 8; i++) {
		mTexNoAnmList[i].setAnmFlag(false);
	}
	for (int i = 0; i < 4; i++) {
		mTevColAnmList[i].setAnmFlag(false);
	}
	for (int i = 0; i < 4; i++) {
		mTevKColAnmList[i].setAnmFlag(false);
	}
	for (int i = 0; i < 8; i++) {
		mTexMtxAnmList[i].setAnmFlag(false);
	}
}

void J3DMaterialAnm::calc(J3DMaterial* material) const
{
	for (u32 i = 0; i < 2; i++) {
		if (mMatColAnmList[i].getAnmFlag()) {
			GXColor* color = material->mColorBlock->getMatColor(i);
			J3DAnmColor** anmp = (J3DAnmColor**)((u8*)&mMatColAnmList[i] + 4);
			u16* idxp = (u16*)((u8*)&mMatColAnmList[i]);
			(*anmp)->getColor(*idxp, color);
		}
	}

	for (u32 i = 0; i < 8; i++) {
		if (mTexNoAnmList[i].getAnmFlag()) {
			u16 tmp;
			mTexNoAnmList[i].calc(&tmp);
			material->mTevBlock->setTexNo(i, tmp);
		}
	}

	for (u32 i = 0; i < 3; i++) {
		if (mTevColAnmList[i].getAnmFlag()) {
			GXColorS10* color = material->mTevBlock->getTevColor(i);
			J3DAnmTevRegKey** anmp = (J3DAnmTevRegKey**)((u8*)&mTevColAnmList[i] + 4);
			u16* idxp = (u16*)((u8*)&mTevColAnmList[i]);
			(*anmp)->getTevColorReg(*idxp, color);
		}
	}

	for (u32 i = 0; i < 4; i++) {
		if (mTevKColAnmList[i].getAnmFlag()) {
			GXColor* color = material->mTevBlock->getTevKColor(i);
			J3DAnmTevRegKey** anmp = (J3DAnmTevRegKey**)((u8*)&mTevKColAnmList[i] + 4);
			u16* idxp = (u16*)((u8*)&mTevKColAnmList[i]);
			(*anmp)->getTevKonstReg(*idxp, color);
		}
	}

	for (u32 i = 0; i < 8; i++) {
		if (mTexMtxAnmList[i].getAnmFlag()) {
			mTexMtxAnmList[i].calc(&material->mTexGenBlock->getTexMtx(i)->getTexMtxInfo().mSRT);
		}
	}
}

void J3DMaterialAnm::setMatColorAnm(int idx, J3DMatColorAnm* pAnm)
{
	if (pAnm == NULL) {
		mMatColAnmList[idx].setAnmFlag(false);
	} else {
		mMatColAnmList[idx] = *pAnm;
	}
}

void J3DMaterialAnm::setTexMtxAnm(int idx, J3DTexMtxAnm* pAnm)
{
	if (pAnm == NULL) {
		mTexMtxAnmList[idx].setAnmFlag(false);
	} else {
		mTexMtxAnmList[idx] = *pAnm;
	}
}

void J3DMaterialAnm::setTexNoAnm(int idx, J3DTexNoAnm* pAnm)
{
	if (pAnm == NULL) {
		mTexNoAnmList[idx].setAnmFlag(false);
	} else {
		mTexNoAnmList[idx] = *pAnm;
	}
}

void J3DMaterialAnm::setTevColorAnm(int idx, J3DTevColorAnm* pAnm)
{
	if (pAnm == NULL) {
		mTevColAnmList[idx].setAnmFlag(false);
	} else {
		mTevColAnmList[idx] = *pAnm;
	}
}

void J3DMaterialAnm::setTevKColorAnm(int idx, J3DTevKColorAnm* pAnm)
{
	if (pAnm == NULL) {
		mTevKColAnmList[idx].setAnmFlag(false);
	} else {
		mTevKColAnmList[idx] = *pAnm;
	}
}
