#ifndef _DOLPHIN_GXSTRUCT
#define _DOLPHIN_GXSTRUCT

#include <dolphin/gx/GXEnum.h>
#include <dolphin/vi.h>
#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GXRenderModeObj {
  VITVMode viTVmode;
  u16 fbWidth;
  u16 efbHeight;
  u16 xfbHeight;
  u16 viXOrigin;
  u16 viYOrigin;
  u16 viWidth;
  u16 viHeight;
  VIXFBMode xFBmode;
  u8 field_rendering;
  u8 aa;
  u8 sample_pattern[12][2];
  u8 vfilter[7];
} GXRenderModeObj;

typedef struct _GXColor {
  u8 r;
  u8 g;
  u8 b;
  u8 a;
} GXColor;

typedef struct _GXTexObj {
  u32 dummy[8];
} GXTexObj;

typedef struct _GXTexObjPriv {
  u32 mode0;
  u32 mode1;
  u32 image0;
  u32 image3;
  void* userData;
  GXTexFmt format;
  u32 tlutName;
  u16 loadCount;
  u8 loadFormat;
  u8 flags;
} GXTexObjPriv;

typedef struct _GXTlutObj {
  u32 dummy[3];
} GXTlutObj;

typedef struct _GXTlutObjPriv {
  u32 unk0;
  u32 unk4;
  u16 numEntries;
  u8 padding[2];
} GXTlutObjPriv;

typedef struct _GXLightObj {
  u32 dummy[16];
} GXLightObj;

typedef struct _GXVtxDescList {
  GXAttr attr;
  GXAttrType type;
} GXVtxDescList;

typedef struct _GXVtxAttrFmtList {
	GXAttr attr;
  GXCompCnt cnt;
  GXCompType type;
	u8 frac;
} GXVtxAttrFmtList;

typedef struct _GXColorS10 {
  s16 r;
  s16 g;
  s16 b;
  s16 a;
} GXColorS10;

typedef struct _GXTexRegion {
  u32 dummy[4];
} GXTexRegion;

typedef struct _GXTexRegionPriv {
  u32 unk0;
  u32 unk4;
  u16 sizeEven;
  u16 sizeOdd;
  u8 unkC;
  u8 unkD;
  u8 padding[2];
} GXTexRegionPriv;

typedef struct _GXTlutRegion {
  u32 dummy[4];
} GXTlutRegion;

typedef struct _GXTlutRegionPriv {
  u32 unk0;
  GXTlutObjPriv tlutObj;
} GXTlutRegionPriv;

typedef struct _GXFogAdjTable {
	u16 fogVals[10]; // _00
} GXFogAdjTable;

#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_GXSTRUCT
