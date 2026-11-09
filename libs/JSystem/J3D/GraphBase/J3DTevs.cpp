#include <dolphin/gd.h>
#include <dolphin/gx.h>

#include "JSystem/J3D/J3DGD.h"
#include "JSystem/J3D/J3DPE.h"
#include "JSystem/J3D/J3DInd.h"
#include "JSystem/J3D/J3DPacket.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DTevBlock.h"
#include "JSystem/J3D/J3DTexMtx.h"
#include "JSystem/J3D/J3DTransform.h"
#include "JSystem/ResTIMG.h"
#include "types.h"

void J3DLightObj::load(u32 lightIdx) const
{
	GDOverflowCheck(0x48);
	J3DGDSetLightPos(GXLightID(1 << lightIdx), mPosition.x, mPosition.y, mPosition.z);
	J3DGDSetLightAttn(GXLightID(1 << lightIdx), _1C, _20, _24, _28, _2C, _30);
	J3DGDSetLightColor(GXLightID(1 << lightIdx), mLightColor);
	J3DGDSetLightDir(GXLightID(1 << lightIdx), mDirection.x, mDirection.y, mDirection.z);
}

void loadTexCoordGens(u32 texGenNum, J3DTexCoord* texCoords)
{
	u32 texMtx = 61;
	GDOverflowCheck(texGenNum * 4 * 2 + 10);
	J3DGDWriteXFCmdHdr(0x1040, texGenNum);

	for (int i = 0; i < texGenNum; i++) {
		J3DGDSetTexCoordGen(GXTexGenType(texCoords[i].getTexGenType()), GXTexGenSrc(texCoords[i].getTexGenSrc()));
	}

	J3DGDWriteXFCmdHdr(0x1050, texGenNum);

	if (j3dSys.checkFlag(0x40000000)) {
		for (int i = 0; i < texGenNum; i++) {
			if (texCoords[i].getTexGenMtx() != 60) {
				texMtx = i * 3;
			} else {
				texMtx = 61;
			}
			J3DGDWrite_u32(texMtx);
		}
	} else {
		for (int i = 0; i < texGenNum; i++) {
			J3DGDWrite_u32(texMtx);
		}
	}
}

void J3DTexMtx::load(u32 mtxIdx) const
{
	if (j3dSys.checkFlag(0x40000000)) {
		loadPostTexMtx(mtxIdx);
	} else {
		loadTexMtx(mtxIdx);
	}
}


inline void J3DTexMtx::loadTexMtx(u32 param_0) const
{
	GDOverflowCheck(0x35);
	J3DGDLoadTexMtxImm((MtxPtr)mMtx, param_0 * 3 + 30, (_GXTexMtxType)mTexMtxInfo.mProjection);
}

inline void J3DTexMtx::loadPostTexMtx(u32 param_0) const
{
	GDOverflowCheck(0x35);
	J3DGDLoadPostTexMtxImm((MtxPtr)mMtx, param_0 * 3 + 0x40);
}

void J3DTexMtx::calc(f32 const (*param_0)[4])
{
	calcTexMtx(param_0);
}

void J3DTexMtx::calcTexMtx(const Mtx param_0)
{
	Mtx44 mtx1;
	Mtx44 mtx2;

	static Mtx qMtx = {
		0.5f, 0.0f, 0.5f, 0.0f, 0.0f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	};
	static Mtx qMtx2 = {
		0.5f, 0.0f, 0.0f, 0.5f, 0.0f, -0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
	};

	u32 r28 = mTexMtxInfo.mInfo & 0x3f;
	u32 r30 = (mTexMtxInfo.mInfo >> 7) & 1;
	switch (r28) {
	case 8:
	case 9:
	case 11:
		if (r30 == 0) {
			J3DGetTextureMtx(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
		} else if (r30 == 1) {
			J3DGetTextureMtxMaya(mTexMtxInfo.mSRT, mtx2);
		}
		PSMTXConcat(mtx2, qMtx, mtx2);
		J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
		PSMTXConcat(mtx1, param_0, mMtx);
		break;
	case 7:
		if (r30 == 0) {
			J3DGetTextureMtx(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx1);
		} else if (r30 == 1) {
			J3DGetTextureMtxMaya(mTexMtxInfo.mSRT, mtx1);
		}
		PSMTXConcat(mtx1, qMtx, mtx1);
		PSMTXConcat(mtx1, param_0, mMtx);
		break;
	case 10:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
		}
		PSMTXConcat(mtx2, qMtx2, mtx2);
		J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
		PSMTXConcat(mtx1, param_0, mMtx);
		break;
	case 6:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx1);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx1);
		}
		PSMTXConcat(mtx1, qMtx2, mtx1);
		PSMTXConcat(mtx1, param_0, mMtx);
		break;
	case 1:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx1);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx1);
		}
		PSMTXConcat(mtx1, param_0, mMtx);
		break;
	case 2:
	case 3:
	case 5:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
		}
		J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
		PSMTXConcat(mtx1, param_0, mMtx);
		break;
	case 4:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
		}
		J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mMtx);
		break;
	default:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mMtx);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mMtx);
		}
		break;
	}
}

void J3DTexMtx::calcPostTexMtx(const Mtx param_0)
{
	Mtx44 mtx1;
	Mtx44 mtx2;

	static Mtx qMtx = {
		0.5f, 0.0f, 0.5f, 0.0f, 0.0f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	};
	static Mtx qMtx2 = {
		0.5f, 0.0f, 0.0f, 0.5f, 0.0f, -0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
	};

	u32 r29 = mTexMtxInfo.mInfo & 0x3f;
	u32 r30 = (mTexMtxInfo.mInfo >> 7) & 1;
	switch (r29) {
	case 8:
	case 11:
		if (r30 == 0) {
			J3DGetTextureMtx(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
		} else if (r30 == 1) {
			J3DGetTextureMtxMaya(mTexMtxInfo.mSRT, mtx2);
		}
		PSMTXConcat(mtx2, qMtx, mtx2);
		J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
		PSMTXConcat(mtx1, param_0, mMtx);
		break;
	case 9:
		if (r30 == 0) {
			J3DGetTextureMtx(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
		} else if (r30 == 1) {
			J3DGetTextureMtxMaya(mTexMtxInfo.mSRT, mtx2);
		}
		PSMTXConcat(mtx2, qMtx, mtx2);
		J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mMtx);
		break;
	case 7:
		if (r30 == 0) {
			J3DGetTextureMtx(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx1);
		} else if (r30 == 1) {
			J3DGetTextureMtxMaya(mTexMtxInfo.mSRT, mtx1);
		}
		PSMTXConcat(mtx1, qMtx, mMtx);
		break;
	case 10:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
		}
		PSMTXConcat(mtx2, qMtx2, mtx2);
		J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
		PSMTXConcat(mtx1, param_0, mMtx);
		break;
	case 6:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx1);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx1);
		}
		PSMTXConcat(mtx1, qMtx2, mMtx);
		break;
	case 1:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mMtx);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mMtx);
		}
		break;
	case 2:
	case 5:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
		}
		J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mtx1);
		PSMTXConcat(mtx1, param_0, mMtx);
		break;
	case 3:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
		}
		J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mMtx);
		break;
	case 4:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mtx2);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mtx2);
		}
		J3DMtxProjConcat(mtx2, mTexMtxInfo.mEffectMtx, mMtx);
		break;
	default:
		if (r30 == 0) {
			J3DGetTextureMtxOld(mTexMtxInfo.mSRT, mTexMtxInfo.mCenter, mMtx);
		} else if (r30 == 1) {
			J3DGetTextureMtxMayaOld(mTexMtxInfo.mSRT, mMtx);
		}
		break;
	}
}

bool isTexNoReg(void* pDL)
{
	u8 r31 = ((u8*)pDL)[1];
	if (r31 >= 0x80 && r31 <= 0xbb) {
		return true;
	}
	return false;
}

u16 getTexNoReg(void* pDL)
{
	u32 var_r31 = *(u32*)((u8*)pDL + 1);
	return var_r31 & 0xFFFFFF;
}

extern u16 sTexCoordScaleTable__6J3DSys[8][4];
static void OverflowWrap() { GDOverflowed(); }
static void ImgPtrWrap(GXTexMapID id, void* p) { J3DGDSetTexImgPtr(id, p); }
static void ImgAttrWrap(GXTexMapID id, u16 w, u16 h, GXTexFmt f) { J3DGDSetTexImgAttr(id, w, h, f); }
static void LookupWrap(GXTexMapID id, GXTexWrapMode a, GXTexWrapMode b, GXTexFilter c, GXTexFilter d, f32 e, f32 f, f32 g, u8 h, u8 i, GXAnisotropy j) { J3DGDSetTexLookupMode(id, a, b, c, d, e, f, g, h, i, j); }
static void LoadTlutWrap(void* p, u32 a, GXTlutSize b) { J3DGDLoadTlut(p, a, b); }
static void SetTlut32(GXTexMapID id, u32 addr, GXTlutFmt fmt)
{
	J3DGDSetTexTlut(id, addr, fmt);
}

static const f32 kTexLodScale = 0.125f;
static const f32 kTexBiasScale = 0.01f;
static const f32 kTexU32ToFloat = 176.0f;
static const u64 kTexIntMagic = 0x80000000ffffffffULL;

asm void loadTexNo(u32 param_0, const u16& texNo)
{
		nofralloc
		stwu r1,-48(r1)
		mflr r0
		lis r5,j3dSys@ha
		stw r0,52(r1)
		lhz r0,0(r4)
		addi r4,r5,j3dSys@l
		stw r31,44(r1)
		slwi r0,r0,5
		stw r30,40(r1)
		mr r30,r3
		lis r3,sTexCoordScaleTable__6J3DSys@ha
		stw r29,36(r1)
		slwi r6,r30,3
		stw r28,32(r1)
		lwz r4,88(r4)
		lwz r7,__GDCurrentDL
		lwz r5,4(r4)
		addi r4,r3,sTexCoordScaleTable__6J3DSys@l
		add r3,r4,r6
		add r31,r5,r0
		lhz r0,2(r31)
		sthx r0,r4,r6
		lhz r0,4(r31)
		sth r0,2(r3)
		lwz r3,8(r7)
		lwz r0,12(r7)
		addi r3,r3,20
		cmplw r3,r0
		ble L_ltn_1
		bl OverflowWrap
	L_ltn_1:
		lwz r0,28(r31)
		mr r3,r30
		add r4,r31,r0
		bl ImgPtrWrap
		lbz r0,0(r31)
		mr r3,r30
		lhz r4,2(r31)
		lhz r5,4(r31)
		clrlwi r6,r0,28
		bl ImgAttrWrap
		lbz r6,22(r31)
		lis r5,17200
		lbz r4,23(r31)
		mr r3,r30
		lha r0,26(r31)
		extsb r6,r6
		extsb r4,r4
		stw r5,8(r1)
		xoris r6,r6,32768
		xoris r0,r0,32768
		xoris r4,r4,32768
		stw r6,12(r1)
		lfd f4,kTexIntMagic
		lfd f0,8(r1)
		stw r4,20(r1)
		fsubs f1,f0,f4
		lfs f5,kTexBiasScale
		stw r5,16(r1)
		lfs f3,kTexU32ToFloat
		lfd f0,16(r1)
		fmuls f1,f5,f1
		stw r0,28(r1)
		fsubs f2,f0,f4
		lbz r4,6(r31)
		stw r5,24(r1)
		lbz r5,7(r31)
		lfd f0,24(r1)
		fmuls f2,f5,f2
		lbz r6,20(r31)
		fsubs f0,f0,f4
		lbz r7,21(r31)
		lbz r8,18(r31)
		lbz r9,17(r31)
		fmuls f3,f3,f0
		lbz r10,19(r31)
		bl LookupWrap
		lbz r0,8(r31)
		cmplwi r0,1
		bne L_ltn_end
		lhz r0,10(r31)
		li r29,1
		cmplwi r0,16
		ble L_ltn_2
		li r29,16
	L_ltn_2:
		lwz r4,__GDCurrentDL
		lwz r3,8(r4)
		lwz r0,12(r4)
		addi r3,r3,20
		cmplw r3,r0
		ble L_ltn_3
		bl OverflowWrap
	L_ltn_3:
		slwi r3,r30,13
		lwz r0,12(r31)
		addis r28,r3,15
		mr r5,r29
		mr r4,r28
		add r3,r31,r0
		bl LoadTlutWrap
		lbz r5,9(r31)
		mr r3,r30
		mr r4,r28
		bl SetTlut32
	L_ltn_end:
		lwz r0,52(r1)
		lwz r31,44(r1)
		lwz r30,40(r1)
		lwz r29,36(r1)
		lwz r28,32(r1)
		mtlr r0
		addi r1,r1,48
		blr
}
void patchTexNo_PtrToIdx(u32 texID, const u16& idx)
{
	ResTIMG* timg = j3dSys.getTexture()->getResTIMG(idx);
	J3DGDSetTexImgPtrRaw(GXTexMapID(texID), idx);
}

void loadNBTScale(J3DNBTScale& NBTScale)
{
	if (NBTScale.mHasScale == true) {
		j3dSys.setNBTScale(&NBTScale.mScale);
	} else {
		j3dSys.setNBTScale(NULL);
	}
}

J3DTexCoordInfo const j3dDefaultTexCoordInfo[8] = {
	{GX_TG_MTX3X4, GX_TG_TEX0, GX_IDENTITY, 0}, {GX_TG_MTX3X4, GX_TG_TEX1, GX_IDENTITY, 0},
	{GX_TG_MTX3X4, GX_TG_TEX2, GX_IDENTITY, 0}, {GX_TG_MTX3X4, GX_TG_TEX3, GX_IDENTITY, 0},
	{GX_TG_MTX3X4, GX_TG_TEX4, GX_IDENTITY, 0}, {GX_TG_MTX3X4, GX_TG_TEX5, GX_IDENTITY, 0},
	{GX_TG_MTX3X4, GX_TG_TEX6, GX_IDENTITY, 0}, {GX_TG_MTX3X4, GX_TG_TEX7, GX_IDENTITY, 0},
};

J3DTexMtxInfo const j3dDefaultTexMtxInfo = {
	0x01,
	0x00,
	0xFF,
	0xFF,
	{0.0f, 0.0f, 0.0f},
	{1.0f, 1.0f, 0, 0.0f, 0.0f},
	{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}},
};

J3DIndTexMtxInfo const j3dDefaultIndTexMtxInfo = {0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 1};

const GXColorS10 j3dDefaultTevColor = {0xFF, 0xFF, 0xFF, 0xFF};

J3DTevStageInfo const j3dDefaultTevStageInfo = {
	0x04, 0x0A, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
};

J3DIndTevStageInfo const j3dDefaultIndTevStageInfo = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

J3DFogInfo const j3dDefaultFogInfo = {
	0x00,   0x00,   0x0140, 0.0f,   0.0f,   0.1f,   10000.0f, 0xFF,   0xFF,   0xFF,   0x00,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   0x0000, 0x0000, 0x0000,
};

J3DNBTScaleInfo const j3dDefaultNBTScaleInfo = {
	0x00,
	1.0f,
	1.0f,
	1.0f,
};

static u8 j3dTexCoordTable[7623];

static u8 texMtxVals[11] = {
	GX_TEXMTX0, GX_TEXMTX1, GX_TEXMTX2, GX_TEXMTX3, GX_TEXMTX4, GX_TEXMTX5, GX_TEXMTX6, GX_TEXMTX7, GX_TEXMTX8, GX_TEXMTX9, GX_IDENTITY,
};

asm void makeTexCoordTable()
{
		nofralloc
		stwu r1,-64(r1)
		lis r3,texMtxVals@ha
		addi r8,r3,texMtxVals@l
		lis r4,j3dTexCoordTable@ha
		stmw r24,32(r1)
		addi r29,r4,j3dTexCoordTable@l
		li r3,0
		li r7,0
		lwz r6,0(r8)
		lwz r5,4(r8)
		lhz r4,8(r8)
		lbz r0,10(r8)
		stw r6,8(r1)
		stw r5,12(r1)
		sth r4,16(r1)
		stb r0,18(r1)
		li r4,0
		li r27,0
		li r6,2
		addi r28,r1,8
	L_tcbody:
		add r0,r27,r7
		li r5,0
		mtctr r6
	L_tcinner:
		add r24,r5,r0
		lbz r30,0(r28)
		mulli r6,r24,3
		lbz r31,1(r28)
		addi r8,r24,1
		lbz r11,2(r28)
		addi r12,r24,2
		lbz r9,3(r28)
		add r26,r29,r6
		lbz r6,4(r28)
		stb r3,0(r26)
		mulli r10,r8,3
		addi r8,r24,4
		stb r4,1(r26)
		addi r25,r24,3
		mulli r12,r12,3
		addi r28,r28,5
		stb r30,2(r26)
		add r26,r29,r10
		addi r5,r5,5
		stb r3,0(r26)
		mulli r10,r25,3
		add r12,r29,r12
		stb r4,1(r26)
		stb r31,2(r26)
		mulli r8,r8,3
		add r10,r29,r10
		stb r3,0(r12)
		add r8,r29,r8
		stb r4,1(r12)
		stb r11,2(r12)
		stb r3,0(r10)
		stb r4,1(r10)
		stb r9,2(r10)
		stb r3,0(r8)
		stb r4,1(r8)
		stb r6,2(r8)
		bdnz L_tcinner
		addi r9,r1,8
		subfic r6,r5,11
		add r9,r9,r5
		mtctr r6
		cmpwi r5,11
		bge L_tctail_done
	L_tctail:
		add r8,r5,r0
		lbz r6,0(r9)
		mulli r8,r8,3
		addi r9,r9,1
		addi r5,r5,1
		add r8,r29,r8
		stb r3,0(r8)
		stb r4,1(r8)
		stb r6,2(r8)
		bdnz L_tctail
	L_tctail_done:
		addi r4,r4,1
		addi r27,r27,11
		cmplwi r4,21
		blt L_tcbody
		addi r3,r3,1
		addi r7,r7,0xe7
		cmplwi r3,11
		blt L_tcbody
		lmw r24,32(r1)
		addi r1,r1,64
		blr
}

u8 j3dTevSwapTableTable[1024];

u8 j3dAlphaCmpTable[768];

asm void makeAlphaCmpTable()
{
		nofralloc
		stwu r1,-32(r1)
		lis r4,j3dAlphaCmpTable@ha
		li r3,0
		li r5,0
		stmw r26,8(r1)
		addi r30,r4,j3dAlphaCmpTable@l
		li r0,4
		li r4,0
		li r6,0
		mtctr r0
	L_alpha_body:
		add r11,r5,r6
		li r8,0
		mulli r0,r11,3
		li r10,1
		addi r7,r11,1
		addi r12,r11,2
		add r9,r30,r0
		addi r29,r11,3
		stb r3,0(r9)
		mulli r7,r7,3
		addi r28,r11,4
		stb r4,1(r9)
		li r31,3
		mulli r0,r12,3
		addi r27,r11,5
		stb r8,2(r9)
		add r12,r30,r7
		addi r7,r11,7
		stb r3,0(r12)
		mulli r8,r29,3
		add r29,r30,r0
		stb r4,1(r12)
		addi r26,r11,6
		li r9,2
		stb r10,2(r12)
		mulli r0,r28,3
		add r28,r30,r8
		stb r3,0(r29)
		li r12,4
		li r10,5
		stb r4,1(r29)
		mulli r11,r27,3
		add r27,r30,r0
		stb r9,2(r29)
		li r8,6
		li r0,7
		stb r3,0(r28)
		mulli r9,r26,3
		add r11,r30,r11
		stb r4,1(r28)
		addi r6,r6,8
		stb r31,2(r28)
		mulli r7,r7,3
		add r9,r30,r9
		stb r3,0(r27)
		add r7,r30,r7
		stb r4,1(r27)
		stb r12,2(r27)
		stb r3,0(r11)
		stb r4,1(r11)
		stb r10,2(r11)
		stb r3,0(r9)
		stb r4,1(r9)
		stb r8,2(r9)
		stb r3,0(r7)
		stb r4,1(r7)
		addi r4,r4,1
		stb r0,2(r7)
		bdnz L_alpha_body
		addi r3,r3,1
		addi r5,r5,32
		cmplwi r3,8
		blt L_alpha_body
		lmw r26,8(r1)
		addi r1,r1,32
		blr
}

u8 j3dZModeTable[96];

asm void makeZModeTable()
{
		nofralloc
		stwu r1,-16(r1)
		lis r3,j3dZModeTable@ha
		addi r8,r3,j3dZModeTable@l
		stw r31,12(r1)
		li r31,0
		li r3,0
		stw r30,8(r1)
		li r9,0
		li r0,2
		mr r6,r9
		li r30,0
		li r4,1
		mtctr r0
	L_zbody:
		add r11,r9,r3
		addi r9,r9,2
		mulli r7,r11,3
		addi r5,r11,1
		add r11,r9,r3
		add r10,r8,r7
		addi r9,r9,2
		stb r31,0(r10)
		mulli r0,r5,3
		addi r5,r11,1
		stb r30,1(r10)
		add r12,r8,r0
		stb r6,2(r10)
		mulli r7,r11,3
		add r11,r9,r3
		stb r31,0(r12)
		addi r9,r9,2
		add r10,r8,r7
		stb r30,1(r12)
		mulli r0,r5,3
		addi r30,r30,1
		stb r4,2(r12)
		addi r5,r11,1
		add r12,r8,r0
		stb r31,0(r10)
		mulli r7,r11,3
		add r11,r9,r3
		stb r30,1(r10)
		addi r9,r9,2
		mulli r0,r5,3
		addi r5,r11,1
		stb r6,2(r10)
		add r10,r8,r7
		stb r31,0(r12)
		mulli r7,r11,3
		stb r30,1(r12)
		addi r30,r30,1
		stb r4,2(r12)
		add r12,r8,r0
		mulli r0,r5,3
		stb r31,0(r10)
		stb r30,1(r10)
		stb r6,2(r10)
		add r10,r8,r7
		stb r31,0(r12)
		stb r30,1(r12)
		addi r30,r30,1
		stb r4,2(r12)
		add r12,r8,r0
		stb r31,0(r10)
		stb r30,1(r10)
		stb r6,2(r10)
		stb r31,0(r12)
		stb r30,1(r12)
		addi r30,r30,1
		stb r4,2(r12)
		bdnz L_zbody
		addi r31,r31,1
		addi r3,r3,16
		cmpwi r31,2
		blt L_zbody
		lwz r31,12(r1)
		lwz r30,8(r1)
		addi r1,r1,16
		blr
}

void makeTevSwapTable()
{
	u8* table = j3dTevSwapTableTable;
	int i     = 0;
	do {
		table[0] = i >> 6;
		table[1] = (i >> 4) & 3;
		table[2] = (i >> 2) & 3;
		table[3] = i & 3;
		i++;
		table += 4;
	} while (i < 256);
}

const GXColor j3dDefaultColInfo = {0xFF, 0xFF, 0xFF, 0xFF};

const GXColor j3dDefaultAmbInfo = {0x32, 0x32, 0x32, 0x32};

const u8 j3dDefaultColorChanNum = 1;

const J3DTevOrderInfo j3dDefaultTevOrderInfoNull = {0xFF, 0xFF, 0xFF, 0x00};

const J3DIndTexOrderInfo j3dDefaultIndTexOrderNull = {0xFF, 0xFF, 0x00, 0x00};

const GXColor j3dDefaultTevKColor = {0xFF, 0xFF, 0xFF, 0xFF};

const J3DIndTexCoordScaleInfo j3dDefaultIndTexCoordScaleInfo = {};

J3DTevSwapModeInfo const j3dDefaultTevSwapMode = {
	0x00,
	0x00,
	0x00,
	0x00,
};

const J3DTevSwapModeTableInfo j3dDefaultTevSwapModeTable = {0x00, 0x01, 0x02, 0x03};

const J3DBlendInfo j3dDefaultBlendInfo = {GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP};

const u8 j3dDefaultTevSwapTableID = 0x1B;

const u16 j3dDefaultAlphaCmpID = 0x00E7;

const u16 j3dDefaultZModeID = 0x0017;
