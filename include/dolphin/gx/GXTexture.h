#ifndef _DOLPHIN_GXTEXTURE
#define _DOLPHIN_GXTEXTURE

#include <dolphin/gx/GXEnum.h>
#include <dolphin/gx/GXStruct.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef GXTexRegion* (*GXTexRegionCallback)(const GXTexObj* obj, GXTexMapID id);
typedef GXTlutRegion* (*GXTlutRegionCallback)(u32 idx);

void GXInitTexObj(GXTexObj* obj, const void* data, u16 width, u16 height, u32 format,
                  GXTexWrapMode wrapS, GXTexWrapMode wrapT, GXBool mipmap);
void GXInitTexObjCI(GXTexObj* obj, const void* data, u16 width, u16 height, GXCITexFmt format,
                    GXTexWrapMode wrapS, GXTexWrapMode wrapT, GXBool mipmap, u32 tlut);
void GXInitTexObjData(GXTexObj* obj, const void* data);
void GXInitTexObjWrapMode(GXTexObj* obj, GXTexWrapMode s, GXTexWrapMode t);
void GXInitTexObjTlut(GXTexObj* obj, u32 tlut);
void GXInitTexObjFilter(GXTexObj* obj, GXTexFilter min_filt, GXTexFilter mag_filt);
void GXInitTexObjMaxLOD(GXTexObj* obj, f32 max_lod);
void GXInitTexObjMinLOD(GXTexObj* obj, f32 min_lod);
void GXInitTexObjLODBias(GXTexObj* obj, f32 lod_bias);
void GXInitTexObjBiasClamp(GXTexObj* obj, GXBool bias_clamp);
void GXInitTexObjEdgeLOD(GXTexObj* obj, GXBool do_edge_lod);
void GXInitTexObjMaxAniso(GXTexObj* obj, GXAnisotropy max_aniso);
void GXInitTexObjUserData(GXTexObj* obj, void* userData);
void* GXGetTexObjUserData(const GXTexObj* obj);
void GXInitTexObjLOD(GXTexObj* obj, GXTexFilter min_filt, GXTexFilter mag_filt, f32 min_lod,
                     f32 max_lod, f32 lod_bias, GXBool bias_clamp, GXBool do_edge_lod,
                     GXAnisotropy max_aniso);
void GXGetTexObjAll(const GXTexObj* obj, void** data, u16* width, u16* height, GXTexFmt* format,
                    GXTexWrapMode* wrapS, GXTexWrapMode* wrapT, GXBool* mipmap);
void* GXGetTexObjData(const GXTexObj* obj);
u16 GXGetTexObjWidth(const GXTexObj* obj);
u16 GXGetTexObjHeight(const GXTexObj* obj);
GXTexFmt GXGetTexObjFmt(const GXTexObj* obj);
GXTexWrapMode GXGetTexObjWrapS(const GXTexObj* obj);
GXTexWrapMode GXGetTexObjWrapT(const GXTexObj* obj);
GXBool GXGetTexObjMipMap(const GXTexObj* obj);
void GXGetTexObjLODAll(const GXTexObj* obj, GXTexFilter* min_filt, GXTexFilter* mag_filt,
                       f32* min_lod, f32* max_lod, f32* lod_bias, GXBool* bias_clamp,
                       GXBool* do_edge_lod, GXAnisotropy* max_aniso);
GXTexFilter GXGetTexObjMinFilt(const GXTexObj* obj);
GXTexFilter GXGetTexObjMagFilt(const GXTexObj* obj);
f32 GXGetTexObjMinLOD(const GXTexObj* obj);
f32 GXGetTexObjMaxLOD(const GXTexObj* obj);
f32 GXGetTexObjLODBias(const GXTexObj* obj);
GXBool GXGetTexObjBiasClamp(const GXTexObj* obj);
GXBool GXGetTexObjEdgeLOD(const GXTexObj* obj);
GXAnisotropy GXGetTexObjMaxAniso(const GXTexObj* obj);
u32 GXGetTexObjTlut(const GXTexObj* obj);
void GXLoadTexObjPreLoaded(GXTexObj* obj, GXTexRegion* region, GXTexMapID id);
void GXLoadTexObj(GXTexObj* obj, GXTexMapID id);
u32 GXGetTexBufferSize(u16 width, u16 height, u32 format, GXBool mipmap, u8 max_lod);
void GXInvalidateTexAll();
void GXInitTlutObj(GXTlutObj* obj, const void* data, GXTlutFmt format, u16 entries);
void GXGetTlutObjAll(const GXTlutObj* obj, void** data, GXTlutFmt* format, u16* entries);
void* GXGetTlutObjData(const GXTlutObj* obj);
GXTlutFmt GXGetTlutObjFmt(const GXTlutObj* obj);
u16 GXGetTlutObjNumEntries(const GXTlutObj* obj);
void GXLoadTlut(GXTlutObj* obj, u32 idx);
void GXInitTexCacheRegion(GXTexRegion* region, GXBool is_32b_mipmap, u32 tmem_even,
                          GXTexCacheSize size_even, u32 tmem_odd, GXTexCacheSize size_odd);
void GXInitTexPreLoadRegion(GXTexRegion* region, u32 tmem_even, u32 size_even, u32 tmem_odd,
                            u32 size_odd);
void GXGetTexRegionAll(const GXTexRegion* region, GXBool* is_cached, GXBool* is_32b_mipmap,
                       u32* tmem_even, GXTexCacheSize* size_even, u32* tmem_odd,
                       GXTexCacheSize* size_odd);
void GXGetTlutRegionAll(const GXTlutRegion* region, u32* memAddr, GXTlutSize* tlutSize);
void GXInvalidateTexRegion(const GXTexRegion* region);
void GXInitTlutRegion(GXTlutRegion* region, u32 tmem_addr, GXTlutSize tlut_size);
GXTexRegionCallback GXSetTexRegionCallback(GXTexRegionCallback callback);
GXTlutRegionCallback GXSetTlutRegionCallback(GXTlutRegionCallback callback);
void GXPreLoadEntireTexture(const GXTexObj* obj, GXTexRegion* region);
void GXSetTexCoordScaleManually(GXTexCoordID coord, GXBool enable, u16 ss, u16 ts);
void GXSetTexCoordCylWrap(GXTexCoordID coord, GXBool s_enable, GXBool t_enable);
void GXSetTexCoordBias(GXTexCoordID coord, GXBool s_enable, GXBool t_enable);

#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_GXTEXTURE
