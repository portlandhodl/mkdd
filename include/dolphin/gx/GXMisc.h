#ifndef RVL_SDK_GX_MISC_H
#define RVL_SDK_GX_MISC_H
#include <dolphin/types.h>
#include <dolphin/gx/GXEnum.h>
#include <dolphin/gx/GXStruct.h>
#include <dolphin/gx/GXManage.h>

#ifdef __cplusplus
extern "C"
{
#endif

void GXSetMisc(GXMiscToken token, u32 val);
void GXFlush();
void GXResetWriteGatherPipe();
u16 GXReadDrawSync(void);
void GXWaitDrawDone(void);
void GXTexModeSync(void);
void GXPeekARGB(u16 x, u16 y, u32 *color);
void GXPokeARGB(u16 x, u16 y, u32 color);
void GXPeekZ(u16 x, u16 y, u32 *z);
void GXPokeZ(u16 x, u16 y, u32 z);
u32 GXCompressZ16(u32 z24, GXZFmt16 zFormat);
u32 GXDecompressZ16(u32 z16, GXZFmt16 zFormat);

void GXAbortFrame();
void __GXAbort(void);
void __GXPEInit(void);
void GXPokeAlphaMode(GXCompare func, u8 threshold);
void GXPokeAlphaRead(GXAlphaReadMode mode);
void GXPokeAlphaUpdate(GXBool update_enable);
void GXPokeBlendMode(GXBlendMode type, GXBlendFactor src_factor, GXBlendFactor dst_factor, GXLogicOp op);
void GXPokeColorUpdate(GXBool update_enable);
void GXPokeDstAlpha(GXBool enable, u8 alpha);
void GXPokeDither(GXBool dither);
void GXPokeZMode(GXBool compare_enable, GXCompare func, GXBool update_enable);
typedef void (*GXDrawSyncCallback)(u16 token);
GXDrawSyncCallback GXSetDrawSyncCallback(GXDrawSyncCallback cb);
GXDrawDoneCallback GXSetDrawDoneCallback(GXDrawDoneCallback cb);

#ifdef __cplusplus
}
#endif
#endif