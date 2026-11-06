#include "math.h"

#include <dolphin/gx.h>

#include "dolphin/gx/GXPriv.h"

#define PI 3.14159265358979323846f
#define GX_LARGE_NUMBER 1.0e+18f

typedef GXLightObj_ GXLightObjPriv;

void GXInitLightAttn(GXLightObj* obj, f32 a0, f32 a1, f32 a2, f32 k0, f32 k1, f32 k2)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	pObj->a0           = a0;
	pObj->a1           = a1;
	pObj->a2           = a2;

	pObj->k0 = k0;
	pObj->k1 = k1;
	pObj->k2 = k2;
}

void GXInitLightAttnA(GXLightObj* obj, f32 a0, f32 a1, f32 a2)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	pObj->a0           = a0;
	pObj->a1           = a1;
	pObj->a2           = a2;
}

void GXGetLightAttnA(GXLightObj* obj, f32* a0, f32* a1, f32* a2)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	*a0                  = pObj->a0;
	*a1                  = pObj->a1;
	*a2                  = pObj->a2;
}

void GXInitLightAttnK(GXLightObj* obj, f32 k0, f32 k1, f32 k2)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	pObj->k0           = k0;
	pObj->k1           = k1;
	pObj->k2           = k2;
}

void GXGetLightAttnK(GXLightObj* obj, f32* k0, f32* k1, f32* k2)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	*k0                  = pObj->k0;
	*k1                  = pObj->k1;
	*k2                  = pObj->k2;
}

void GXInitLightSpot(GXLightObj* obj, f32 cutoff, GXSpotFn spotFunc)
{
	f32 a0, a1, a2, r, d, cr;
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;

	if (cutoff <= 0.0f || cutoff > 90.0f) {
		spotFunc = GX_SP_OFF;
	}

	r  = cutoff * PI / 180.0f;
	cr = cosf(r);

	switch (spotFunc) {
	case GX_SP_FLAT:
		a0 = -1000.0f * cr;
		a1 = 1000.0f;
		a2 = 0.0f;
		break;
	case GX_SP_COS:
		a1 = 1.0f / (1.0f - cr);
		a0 = -cr * a1;
		a2 = 0.0f;
		break;
	case GX_SP_COS2:
		a2 = 1.0f / (1.0f - cr);
		a0 = 0.0f;
		a1 = -cr * a2;
		break;
	case GX_SP_SHARP:
		d  = 1.0F / ((1.0F - cr) * (1.0F - cr));
		a0 = cr * (cr - 2.0F) * d;
		a1 = 2.0F * d;
		a2 = -d;
		break;
	case GX_SP_RING1:
		d  = 1.0f / ((1.0f - cr) * (1.0F - cr));
		a2 = -4.0f * d;
		a0 = a2 * cr;
		a1 = 4.0f * (1.0f + cr) * d;
		break;
	case GX_SP_RING2:
		d  = 1.0f / ((1.0f - cr) * (1.0F - cr));
		a0 = 1.0f - 2.0f * cr * cr * d;
		a1 = 4.0f * cr * d;
		a2 = -2.0f * d;
		break;
	case GX_SP_OFF:
	default:
		a0 = 1.0f;
		a1 = 0.0f;
		a2 = 0.0f;
		break;
	}

	pObj->a0 = a0;
	pObj->a1 = a1;
	pObj->a2 = a2;
}

void GXInitLightDistAttn(GXLightObj* obj, f32 refDist, f32 refBrightness, GXDistAttnFn distFunc)
{
	f32 k0, k1, k2;
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;

	if (refDist < 0.0F) {
		distFunc = GX_DA_OFF;
	}

	if (refBrightness <= 0.0F || refBrightness >= 1.0F) {
		distFunc = GX_DA_OFF;
	}

	switch (distFunc) {
	case GX_DA_GENTLE:
		k0 = 1.0F;
		k1 = (1.0F - refBrightness) / (refBrightness * refDist);
		k2 = 0.0F;
		break;
	case GX_DA_MEDIUM:
		k0 = 1.0f;
		k1 = 0.5f * (1.0f - refBrightness) / (refBrightness * refDist);
		k2 = 0.5f * (1.0f - refBrightness) / (refBrightness * refDist * refDist);
		break;
	case GX_DA_STEEP:
		k0 = 1.0f;
		k1 = 0.0f;
		k2 = (1.0f - refBrightness) / (refBrightness * refDist * refDist);
		break;
	case GX_DA_OFF:
	default:
		k0 = 1.0f;
		k1 = 0.0f;
		k2 = 0.0f;
		break;
	}

	pObj->k0 = k0;
	pObj->k1 = k1;
	pObj->k2 = k2;
}

void GXInitLightPos(GXLightObj* obj, f32 x, f32 y, f32 z)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	pObj->px        = x;
	pObj->py        = y;
	pObj->pz        = z;
}

void GXGetLightPos(const GXLightObj* obj, f32* x, f32* y, f32* z)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	*x                   = pObj->px;
	*y                   = pObj->py;
	*z                   = pObj->pz;
}

void GXInitLightDir(GXLightObj* obj, f32 nX, f32 nY, f32 nZ)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	pObj->nx        = -nX;
	pObj->ny        = -nY;
	pObj->nz        = -nZ;
}

void GXGetLightDir(GXLightObj* obj, f32* nX, f32* nY, f32* nZ)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	*nX                  = -pObj->nx;
	*nY                  = -pObj->ny;
	*nZ                  = -pObj->nz;
}

void GXInitSpecularDir(GXLightObj* obj, f32 nX, f32 nY, f32 nZ)
{
	f32 mag;
	f32 vx, vy, vz;
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;

	vx  = -nX;
	vy  = -nY;
	vz  = (-nZ + 1.0F);
	mag = vx * vx + vy * vy + vz * vz;

	if (mag != 0.0f) {
		mag = 1.0f / sqrtf(mag);
	}

	pObj->nx = vx * mag;
	pObj->ny = vy * mag;
	pObj->nz = vz * mag;

	pObj->px = nX * -GX_LARGE_NUMBER;
	pObj->py = nY * -GX_LARGE_NUMBER;
	pObj->pz = nZ * -GX_LARGE_NUMBER;
}

void GXInitSpecularDirHA(GXLightObj* obj, f32 nX, f32 nY, f32 nZ, f32 hX, f32 hY, f32 hZ)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;

	pObj->nx = hX;
	pObj->ny = hY;
	pObj->nz = hZ;
	pObj->px = nX * -GX_LARGE_NUMBER;
	pObj->py = nY * -GX_LARGE_NUMBER;
	pObj->pz = nZ * -GX_LARGE_NUMBER;
}

void GXInitLightColor(GXLightObj* obj, GXColor color)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	pObj->color          = (*(u32*)&color);
}

void GXGetLightColor(const GXLightObj* obj, GXColor* color)
{
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;
	*(u32*)color = pObj->color;
}

static inline void PushLight(const register GXLightObjPriv* lt_obj, register void* dest)
{
	register u32 zero, color;
	register f32 a0_a1, a2_k0, k1_k2;
	register f32 px_py, pz_dx, dy_dz;
#ifdef __MWERKS__ // clang-format off
	asm {
		lwz     color, 12(lt_obj)
		xor     zero, zero, zero
		psq_l   a0_a1, 16(lt_obj), 0, 0
		psq_l   a2_k0, 24(lt_obj), 0, 0
		psq_l   k1_k2, 32(lt_obj), 0, 0
		psq_l   px_py, 40(lt_obj), 0, 0
		psq_l   pz_dx, 48(lt_obj), 0, 0
		psq_l   dy_dz, 56(lt_obj), 0, 0

		stw     zero,  0(dest)
		stw     zero,  0(dest)
		stw     zero,  0(dest)
		stw     color, 0(dest)
		psq_st  a0_a1, 0(dest), 0, 0
		psq_st  a2_k0, 0(dest), 0, 0
		psq_st  k1_k2, 0(dest), 0, 0
		psq_st  px_py, 0(dest), 0, 0
		psq_st  pz_dx, 0(dest), 0, 0
		psq_st  dy_dz, 0(dest), 0, 0
	}
#endif // clang-format on
}

void GXLoadLightObjImm(GXLightObj* obj, GXLightID light)
{
	u32 addr;
	u32 idx;
	GXLightObjPriv* pObj = (GXLightObjPriv*)obj;

	idx = 31 - __cntlzw(light);
	idx &= 7;

	addr = 0x600 + idx * 0x10;

	GX_XF_LOAD_REG_HDR(addr | (0x10 - 1) << 16);

	PushLight(pObj, (void*)GX_FIFO_ADDR);
	gx->bpSentNot = 1;
}

void GXLoadLightObjIndx(u32 objIndex, GXLightID light)
{
	u32 reg;
	u32 addr;
	u32 idx;

	idx = 31 - __cntlzw(light);
	idx &= 7;

	addr = 0x600 + idx * 0x10;
	reg  = 0;
	GX_SET_REG(reg, addr, 0, 11);
	GX_SET_REG(reg, 0xF, 12, 15);
	GX_SET_REG(reg, objIndex, 16, 31);
	GX_WRITE_U8(GX_LOAD_INDX_A);
	GX_WRITE_U32(reg);
	gx->bpSentNot = 0;
}

void GXSetChanAmbColor(GXChannelID channel, GXColor color)
{
	u32 reg;
	u32 rgb;
	u32 colorID;
	u8 alpha;

	switch (channel) {
	case GX_COLOR0:
		rgb     = gx->ambColor[GX_COLOR0];
		reg     = GX_SET_TRUNC((*(u32*)&color) & ~0xff, rgb, 24, 31);
		colorID = GX_COLOR0;
		break;
	case GX_COLOR1:
		rgb     = gx->ambColor[GX_COLOR1];
		reg     = GX_SET_TRUNC((*(u32*)&color) & ~0xff, rgb, 24, 31);
		colorID = GX_COLOR1;
		break;
	case GX_ALPHA0:
		reg     = gx->ambColor[GX_COLOR0];
		reg     = GX_SET_TRUNC(reg, color.a, 24, 31);
		colorID = GX_COLOR0;
		break;
	case GX_ALPHA1:
		reg     = gx->ambColor[GX_COLOR1];
		reg     = GX_SET_TRUNC(reg, color.a, 24, 31);
		colorID = GX_COLOR1;
		break;
	case GX_COLOR0A0:
		reg     = (*(u32*)&color);
		colorID = GX_COLOR0;
		break;
	case GX_COLOR1A1:
		reg     = (*(u32*)&color);
		colorID = GX_COLOR1;
		break;
	default:
		return;
	}

	GX_XF_LOAD_REG(GX_XF_REG_AMBIENT0 + colorID, reg);
	gx->bpSentNot         = GX_TRUE;
	gx->ambColor[colorID] = reg;
}

void GXSetChanMatColor(GXChannelID channel, GXColor color)
{
	u32 reg = 0;
	u32 rgb;
	GXChannelID colorID;

	switch (channel) {
	case GX_COLOR0:
		rgb     = gx->matColor[GX_COLOR0];
		reg     = GX_SET_TRUNC((*(u32*)&color) & ~0xff, rgb, 24, 31);
		colorID = GX_COLOR0;
		break;
	case GX_COLOR1:
		rgb     = gx->matColor[GX_COLOR1];
		reg     = GX_SET_TRUNC((*(u32*)&color) & ~0xff, rgb, 24, 31);
		colorID = GX_COLOR1;
		break;
	case GX_ALPHA0:
		reg     = gx->matColor[GX_COLOR0];
		reg     = GX_SET_TRUNC(reg, color.a, 24, 31);
		colorID = GX_COLOR0;
		break;
	case GX_ALPHA1:
		reg     = gx->matColor[GX_COLOR1];
		reg     = GX_SET_TRUNC(reg, color.a, 24, 31);
		colorID = GX_COLOR1;
		break;
	case GX_COLOR0A0:
		reg     = (*(u32*)&color);
		colorID = GX_COLOR0;
		break;
	case GX_COLOR1A1:
		reg     = (*(u32*)&color);
		colorID = GX_COLOR1;
		break;
	default:
		return;
	}

	GX_XF_LOAD_REG(GX_XF_REG_MATERIAL0 + colorID, reg);
	gx->bpSentNot         = GX_TRUE;
	gx->matColor[colorID] = reg;
}

void GXSetNumChans(u8 count)
{
	GX_SET_REG(gx->genMode, count, 25, 27);
	GX_XF_LOAD_REG(GX_XF_REG_NUMCOLORS, count);
	gx->dirtyState |= GX_DIRTY_GEN_MODE;
}

void GXSetChanCtrl(GXChannelID channel, GXBool doEnable, GXColorSrc ambSrc, GXColorSrc matSrc, u32 mask, GXDiffuseFn diffFunc,
                   GXAttnFn attnFunc)
{
	const u32 colorID = (u32)channel & 0x3;
	u32 reg           = 0;

	GX_SET_REG(reg, doEnable, GX_XF_CLR0CTRL_LIGHT_ST, GX_XF_CLR0CTRL_LIGHT_END);
	GX_SET_REG(reg, matSrc, GX_XF_CLR0CTRL_MTXSRC_ST, GX_XF_CLR0CTRL_MTXSRC_END);
	GX_SET_REG(reg, ambSrc, GX_XF_CLR0CTRL_AMBSRC_ST, GX_XF_CLR0CTRL_AMBSRC_END);
	GX_SET_REG(reg, (attnFunc == GX_AF_SPEC ? GX_DF_NONE : diffFunc), GX_XF_CLR0CTRL_DIFATTN_ST, GX_XF_CLR0CTRL_DIFATTN_END);
	GX_SET_REG(reg, (attnFunc != GX_AF_NONE), GX_XF_CLR0CTRL_ATTNENABLE_ST, GX_XF_CLR0CTRL_ATTNENABLE_END);
	GX_SET_REG(reg, (attnFunc != GX_AF_SPEC), GX_XF_CLR0CTRL_ATTNSEL_ST, GX_XF_CLR0CTRL_ATTNSEL_END);

	reg = (reg & ~(0xf << 2)) | ((mask & 0xf) << 2);

	reg = (reg & ~(0xf << 11)) | (((mask >> 4) & 0xf) << 11);

	GX_XF_LOAD_REG(GX_XF_REG_COLOR0CNTRL + colorID, reg);

	if (channel == GX_COLOR0A0) {
		GX_XF_LOAD_REG(GX_XF_REG_ALPHA0CNTRL, reg);
	} else if (channel == GX_COLOR1A1) {
		GX_XF_LOAD_REG(GX_XF_REG_ALPHA1CNTRL, reg);
	}

	gx->bpSentNot = GX_TRUE;
}
