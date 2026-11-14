#ifndef _DOLPHIN_GXPRIV
#define _DOLPHIN_GXPRIV

#include <dolphin/gx/GXEnum.h>
#include <dolphin/gx/GXCommandList.h>
#include <dolphin/gx/GXStruct.h>
#include <dolphin/gx/GXTexture.h>
#include <dolphin/gx/GXVert.h>
#include <dolphin/gx/GXPixel.h>
#include <dolphin/gx/GXPerf.h>
#include <dolphin/gx/GXFifo.h>

typedef struct GXLightObj_ {
  u32 padding[3];
  u32 color;
  float a0;
  float a1;
  float a2;
  float k0;
  float k1;
  float k2;
  float px;
  float py;
  float pz;
  float nx;
  float ny;
  float nz;
} GXLightObj_;

#define XF_LIGHT_BASE 0x0600
#define XF_LIGHT_SIZE 0x10

#define GX_FIFO_ADDR 0xCC008000

#define GX_WRITE_U8(v) GXWGFifo.u8 = (u8)(v)
#define GX_WRITE_U16(v) GXWGFifo.u16 = (u16)(v)
#define GX_WRITE_U32(v) GXWGFifo.u32 = (u32)v
#define GX_WRITE_F32(v) (GXWGFifo.f32 = (f32)(v))

#define GX_BITFIELD(field, pos, size, value) \
  (__rlwimi((field), (value), 31 - (pos) - (size) + 1, (pos), (pos) + (size) - 1))
#define GX_BITFIELD_SET(field, pos, size, value) \
  ((field) = GX_BITFIELD(field, pos, size, value))
#define GX_BITFIELD_TRUNC(field, pos, size, value) \
  (__rlwimi((field), (value), 0, (pos), (pos) + (size) - 1))
#define GX_BITGET(field, pos, size) \
  ((field) >> (31 - (pos) - (size) + 1) & ((1 << (size)) - 1))

#define GX_REG_MASK(st, end) (((1 << ((end) - (st) + 1)) - 1) << 31 - (end))
#define GX_GET_REG(reg, st, end) GX_BITGET((reg), (st), ((end) - (st) + 1))
#define GX_SET_REG(reg, x, st, end) GX_BITFIELD_SET((reg), (st), ((end) - (st) + 1), (x))
#define GX_SET_TRUNC(reg, x, st, end) GX_BITFIELD_TRUNC((reg), (st), ((end) - (st) + 1), (x))

#define GX_BP_LOAD_REG(data) \
  GXWGFifo.s8 = GX_LOAD_BP_REG; \
  GXWGFifo.s32 = (data);
#define GX_CP_LOAD_REG(addr, data) \
  GXWGFifo.s8 = GX_LOAD_CP_REG; \
  GXWGFifo.s8 = (addr); \
  GXWGFifo.s32 = (data);
#define GX_XF_LOAD_REG_HDR(addr) \
  GXWGFifo.s8 = GX_LOAD_XF_REG; \
  GXWGFifo.s32 = (addr);
#define GX_XF_LOAD_REG(addr, data) \
  GX_XF_LOAD_REG_HDR(addr); \
  GXWGFifo.s32 = (data);
#define GX_XF_LOAD_REGS(size, addr) \
  { \
    u32 cmd = (size) << 16 | addr; \
    GX_XF_LOAD_REG_HDR(cmd); \
  }

typedef struct GXData {
  u16 vNumNot;
  u16 bpSentNot;
  u16 vNum;
  u16 vLim;

  u32 cpEnable;
  u32 cpStatus;
  u32 cpClr;
  u32 vcdLo;
  u32 vcdHi;
  u32 vatA[8];
  u32 vatB[8];
  u32 vatC[8];
  u32 lpSize;
  u32 matIdxA;
  u32 matIdxB;

  u32 indexBase[4];
  u32 indexStride[4];

  u32 ambColor[2];
  u32 matColor[2];

  u32 suTs0[8];
  u32 suTs1[8];
  u32 suScis0;
  u32 suScis1;

  u32 tref[8];
  u32 iref;

  u32 bpMask;
  u32 IndTexScale0;
  u32 IndTexScale1;

  u32 tevc[16];
  u32 teva[16];
  u32 tevKsel[8];

  u32 cmode0;
  u32 cmode1;
  u32 zmode;
  u32 peCtrl;

  u32 cpDispSrc;
  u32 cpDispSize;
  u32 cpDispStride;
  u32 cpDisp;

  u32 cpTexSrc;
  u32 cpTexSize;
  u32 cpTexStride;
  u32 cpTex;
  GXBool cpTexZ;

  u32 genMode;

  GXTexRegion TexRegions0[GX_MAX_TEXMAP];
  GXTexRegion TexRegions1[GX_MAX_TEXMAP];
  GXTexRegion TexRegions2[GX_MAX_TEXMAP];

  GXTlutRegion TlutRegions[GX_MAX_TLUT_ALL];
  GXTexRegionCallback texRegionCallback;
  GXTlutRegionCallback tlutRegionCallback;

  GXAttrType nrmType;
  GXBool hasNrms;
  GXBool hasBiNrms;
  u32 projType;
  f32 projMtx[6];

  f32 vpLeft;
  f32 vpTop;
  f32 vpWd;
  f32 vpHt;
  f32 vpNearz;
  f32 vpFarz;
  f32 zOffset;
  f32 zScale;

  u32 tImage0[8];
  u32 tMode0[8];
  u32 texmapId[16];
  u32 tcsManEnab;
  u32 tevTcEnab;

  GXPerf0 perf0;
  GXPerf1 perf1;
  u32 perfSel;

  GXBool inDispList;
  GXBool dlSaveContext;
  GXBool abtWaitPECopy;
  u8 dirtyVAT;
  u32 dirtyState;
} GXData;

extern GXData* const __GXData;
#define gx __GXData

void __GXSetSUTexRegs(void);
void __GXUpdateBPMask(void);
void __GXSetVCD(void);
void __GXSetVAT(void);
void __GXCalculateVLim(void);

#define GX_CHECK_FLUSH() (!(*(u32*)(&gx->vNumNot)))

extern void* __cpReg;
extern void* __piReg;
extern void* __memReg;
extern void* __peReg;

#define GX_CP_ADDR  (0x0C000000)
#define GX_PE_ADDR  (0x0C001000)
#define GX_PI_ADDR  (0x0C003000)
#define GX_MEM_ADDR (0x0C004000)

#define GX_GET_MEM_REG(offset) (*(vu16*)((vu16*)(__memReg) + (offset)))
#define GX_GET_CP_REG(offset)  (*(vu16*)((vu16*)(__cpReg) + (offset)))
#define GX_GET_PE_REG(offset)  (*(vu16*)((vu16*)(__peReg) + (offset)))
#define GX_GET_PI_REG(offset)  (*(vu32*)((vu32*)(__piReg) + (offset)))

#define GX_SET_MEM_REG(offset, val) (*(vu16*)((vu16*)(__memReg) + (offset)) = val)
#define GX_SET_CP_REG(offset, val)  (*(vu16*)((vu16*)(__cpReg) + (offset)) = val)
#define GX_SET_PE_REG(offset, val)  (*(vu16*)((vu16*)(__peReg) + (offset)) = val)
#define GX_SET_PI_REG(offset, val)  (*(vu32*)((vu32*)(__piReg) + (offset)) = val)

static inline u32 GXReadMEMReg(u32 addrLo, u32 addrHi) {
  u32 hiStart, hiNew, lo;
  hiStart = GX_GET_MEM_REG(addrHi);
  do {
    hiNew = hiStart;
    lo = GX_GET_MEM_REG(addrLo);
    hiStart = GX_GET_MEM_REG(addrHi);
  } while (hiStart != hiNew);
  return ((hiStart << 16) | lo);
}

#endif // _DOLPHIN_GXPRIV
