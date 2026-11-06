#ifndef _DOLPHIN_GXGEOMETRY
#define _DOLPHIN_GXGEOMETRY

#include <dolphin/gx/GXEnum.h>

#ifdef __cplusplus
extern "C" {
#endif

void __GXSetDirtyState(void);
void __GXSendFlushPrim(void);
void __GXSetGenMode(void);

void GXSetVtxDesc(GXAttr attr, GXAttrType type);
void GXSetVtxDescv(GXVtxDescList* list);
void GXClearVtxDesc(void);
void GXSetVtxAttrFmt(GXVtxFmt vtxfmt, GXAttr attr, GXCompCnt cnt, GXCompType type, u8 frac);
void GXSetVtxAttrFmtv(GXVtxFmt vtxfmt, GXVtxAttrFmtList* list);
void GXGetVtxDesc(GXAttr attr, GXAttrType* type);
void GXGetVtxDescv(GXVtxDescList* list);
void GXGetVtxAttrFmtv(GXVtxFmt vtxfmt, GXVtxAttrFmtList* list);
void __GXSetVCD(void);
void __GXSetVAT(void);
void __GXCalculateVLim(void);
void GXSetNumTexGens(u8 nTexGens);
void GXBegin(GXPrimitive type, GXVtxFmt vtxfmt, u16 nverts);
void GXSetTexCoordGen2(GXTexCoordID dst_coord, GXTexGenType func, GXTexGenSrc src_param, u32 mtx,
                       GXBool normalize, u32 postmtx);
void GXSetLineWidth(u8 width, GXTexOffset texOffsets);
void GXGetLineWidth(u8* width, GXTexOffset* texOffsets);
void GXSetPointSize(u8 pointSize, GXTexOffset texOffsets);
void GXGetPointSize(u8* pointSize, GXTexOffset* texOffsets);
void GXEnableTexOffsets(GXTexCoordID coord, GXBool line_enable, GXBool point_enable);
void GXSetCullMode(GXCullMode mode);
void GXGetCullMode(GXCullMode* mode);
void GXSetCoPlanar(GXBool enable);
void GXSetArray(GXAttr attr, const void* data, u8 stride);
void GXInvalidateVtxCache(void);

static inline void GXSetTexCoordGen(GXTexCoordID dst_coord, GXTexGenType func,
                                    GXTexGenSrc src_param, u32 mtx) {
  GXSetTexCoordGen2(dst_coord, func, src_param, mtx, GX_FALSE, GX_PTIDENTITY);
}

#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_GXGEOMETRY
