#ifndef _DOLPHIN_GXFIFO
#define _DOLPHIN_GXFIFO

#include <dolphin/gx/GXEnum.h>
#include <dolphin/os/OSThread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  u8 pad[128];
} GXFifoObj;

typedef struct {
  void* base;
  void* end;
  u32 size;
  u32 highWatermark;
  u32 lowWatermark;
  void* readPtr;
  void* writePtr;
  s32 rwDistance;
  u8 pad[0x60];
} GXFifoObjPriv;

typedef void (*GXBreakPtCallback)(void);

void __GXFifoInit(void);
void __GXSaveCPUFifoAux(GXFifoObj* fifo);
void __GXFifoReadEnable(void);
void __GXFifoReadDisable(void);
void __GXFifoLink(u8 link);
void __GXWriteFifoIntEnable(u32 hi, u32 lo);
void __GXWriteFifoIntReset(u32 hi, u32 lo);
void __GXInsaneWatermark(void);
void __GXCleanGPFifo(void);

void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size);
void GXInitFifoPtrs(GXFifoObj* fifo, void* readPtr, void* writePtr);
void GXGetFifoPtrs(GXFifoObj* fifo, void** readPtr, void** writePtr);
void* GXGetFifoBase(GXFifoObj* fifo);
u32 GXGetFifoSize(GXFifoObj* fifo);
void GXGetFifoLimits(GXFifoObj* fifo, u32* hi, u32* lo);
GXFifoObj* GXGetCPUFifo(void);
GXFifoObj* GXGetGPFifo(void);
void GXSetCPUFifo(GXFifoObj* fifo);
void GXSetGPFifo(GXFifoObj* fifo);
void GXSaveCPUFifo(GXFifoObj* fifo);
void GXSaveGPFifo(GXFifoObj* fifo);
GXBool GXGetFifoWrap(GXFifoObj* fifo);
u32 GXGetFifoCount(GXFifoObj* fifo);
void GXGetFifoStatus(GXFifoObj* fifo, GXBool* overhi, GXBool* underlow, u32* fifoCount,
                     GXBool* cpu_write, GXBool* gp_read, GXBool* fifowrap);
void GXGetGPStatus(GXBool* overhi, GXBool* underlow, GXBool* readIdle, GXBool* cmdIdle,
                   GXBool* brkpt);
void GXInitFifoLimits(GXFifoObj* fifo, u32 hiWaterMark, u32 loWaterMark);
GXBreakPtCallback GXSetBreakPtCallback(GXBreakPtCallback cb);
void GXEnableBreakPt(void* breakPt);
void GXDisableBreakPt(void);

OSThread* GXSetCurrentGXThread(void);
OSThread* GXGetCurrentGXThread(void);
u32 GXGetOverflowCount(void);
u32 GXResetOverflowCount(void);
void* GXRedirectWriteGatherPipe(void* ptr);
void GXRestoreWriteGatherPipe(void);

#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_GXFIFO
