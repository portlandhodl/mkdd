#include <dolphin/gx.h>
#include <dolphin/gx/GXPriv.h>
#include <dolphin/os.h>
#include <dolphin/base/PPCArch.h>

static GXFifoObj* CPUFifo;
static GXFifoObj* GPFifo;
static OSThread* __GXCurrentThread;
static u8 CPGPLinked;
static BOOL GXOverflowSuspendInProgress;
static GXBreakPtCallback BreakPointCB;
static u32 __GXOverflowCount;
static void* __GXCurrentBP;

void GXOverflowHandler(void)
{
  __GXOverflowCount += 1;
  __GXWriteFifoIntEnable(0, 1);
  __GXWriteFifoIntReset(1, 0);
  GXOverflowSuspendInProgress = TRUE;
  OSSuspendThread(__GXCurrentThread);
}

void GXUnderflowHandler(void)
{
  OSResumeThread(__GXCurrentThread);
  GXOverflowSuspendInProgress = FALSE;
  __GXWriteFifoIntReset(1, 1);
  __GXWriteFifoIntEnable(1, 0);
}

void GXBreakPointHandler(OSContext* context)
{
  OSContext bpContext;
  GX_SET_REG(gx->cpEnable, 0, 26, 26);
  GX_SET_CP_REG(1, gx->cpEnable);

  if (BreakPointCB) {
    OSClearContext(&bpContext);
    OSSetCurrentContext(&bpContext);
    BreakPointCB();
    OSClearContext(&bpContext);
    OSSetCurrentContext(context);
  }
}

static void GXCPInterruptHandler(s16 p1, OSContext* context)
{
  gx->cpStatus = GX_GET_CP_REG(0);

  if ((gx->cpEnable >> 3 & 1) && (gx->cpStatus >> 1 & 1)) {
    GXUnderflowHandler();
  }

  if ((gx->cpEnable >> 2 & 1) && (gx->cpStatus >> 0 & 1)) {
    GXOverflowHandler();
  }

  if ((gx->cpEnable >> 5 & 1) && (gx->cpStatus >> 4 & 1)) {
    GXBreakPointHandler(context);
  }
}

void GXInitFifoBase(GXFifoObj* fifo, void* base, u32 size)
{
  GXFifoObjPriv* pFifo = (GXFifoObjPriv*)fifo;
  pFifo->base = base;
  pFifo->end = (void*)((u32)base + size - 4);
  pFifo->size = size;
  pFifo->rwDistance = 0;
  GXInitFifoLimits(fifo, size - 0x4000, OSRoundDown32B(size / 2));
  GXInitFifoPtrs(fifo, base, base);
}

void GXInitFifoPtrs(GXFifoObj* fifo, void* readPtr, void* writePtr)
{
  GXFifoObjPriv* pFifo = (GXFifoObjPriv*)fifo;
  int interrupts = OSDisableInterrupts();
  pFifo->readPtr = readPtr;
  pFifo->writePtr = writePtr;
  pFifo->rwDistance = (u32)writePtr - (u32)readPtr;
  if (pFifo->rwDistance < 0) {
    pFifo->rwDistance += pFifo->size;
  }
  OSRestoreInterrupts(interrupts);
}

void GXInitFifoLimits(GXFifoObj* fifo, u32 highWatermark, u32 lowWatermark)
{
  GXFifoObjPriv* pFifo = (GXFifoObjPriv*)fifo;
  pFifo->highWatermark = highWatermark;
  pFifo->lowWatermark = lowWatermark;
}

void GXSetCPUFifo(GXFifoObj* fifo)
{
  BOOL interrupts;
  interrupts = OSDisableInterrupts();

  CPUFifo = fifo;

  if (fifo == GPFifo) {
    u32 reg;
    GX_SET_PI_REG(3, (u32)((GXFifoObjPriv*)fifo)->base & 0x3FFFFFFF);
    GX_SET_PI_REG(4, (u32)((GXFifoObjPriv*)fifo)->end & 0x3FFFFFFF);
    reg = 0;
    GX_SET_REG(reg, (u32)((GXFifoObjPriv*)fifo)->writePtr >> 5, 6, 26);
    GX_SET_REG(reg, 0, 5, 5);
    GX_SET_PI_REG(5, reg);

    CPGPLinked = GX_TRUE;

    __GXWriteFifoIntReset(1, 1);
    __GXWriteFifoIntEnable(1, 0);
    __GXFifoLink(1);
  } else {
    u32 reg;
    if (CPGPLinked) {
      __GXFifoLink(0);
      CPGPLinked = GX_FALSE;
    }
    __GXWriteFifoIntEnable(0, 0);

    GX_SET_PI_REG(3, (u32)((GXFifoObjPriv*)fifo)->base & 0x3FFFFFFF);
    GX_SET_PI_REG(4, (u32)((GXFifoObjPriv*)fifo)->end & 0x3FFFFFFF);
    reg = 0;
    GX_SET_REG(reg, (u32)((GXFifoObjPriv*)fifo)->writePtr >> 5, 6, 26);
    GX_SET_REG(reg, 0, 5, 5);
    GX_SET_PI_REG(5, reg);
  }

  PPCSync();

  OSRestoreInterrupts(interrupts);
}

void GXSetGPFifo(GXFifoObj* fifo)
{
  GXFifoObjPriv* pFifo = (GXFifoObjPriv*)fifo;
  int interrupts = OSDisableInterrupts();
  __GXFifoReadDisable();
  __GXWriteFifoIntEnable(0, 0);
  GPFifo = fifo;
  GX_SET_CP_REG(16, (u16)(u32)pFifo->base);
  GX_SET_CP_REG(18, (u16)(u32)pFifo->end);
  GX_SET_CP_REG(24, (u16)pFifo->rwDistance);
  GX_SET_CP_REG(26, (u16)(u32)pFifo->writePtr);
  GX_SET_CP_REG(28, (u16)(u32)pFifo->readPtr);
  GX_SET_CP_REG(20, (u16)pFifo->highWatermark);
  GX_SET_CP_REG(22, (u16)pFifo->lowWatermark);

  GX_SET_CP_REG(17, ((u32)pFifo->base & 0x3FFFFFFF) >> 16);
  GX_SET_CP_REG(19, ((u32)pFifo->end & 0x3FFFFFFF) >> 16);
  GX_SET_CP_REG(25, (pFifo->rwDistance) >> 16);
  GX_SET_CP_REG(27, ((u32)pFifo->writePtr & 0x3FFFFFFF) >> 16);
  GX_SET_CP_REG(29, ((u32)pFifo->readPtr & 0x3FFFFFFF) >> 16);
  GX_SET_CP_REG(21, (pFifo->highWatermark) >> 16);
  GX_SET_CP_REG(23, (pFifo->lowWatermark) >> 16);

  PPCSync();

  if (CPUFifo == GPFifo) {
    CPGPLinked = 1;
    __GXWriteFifoIntEnable(1, 0);
    __GXFifoLink(1);
  } else {
    CPGPLinked = 0;
    __GXWriteFifoIntEnable(0, 0);
    __GXFifoLink(0);
  }
  __GXWriteFifoIntReset(1, 1);
  __GXFifoReadEnable();
  OSRestoreInterrupts(interrupts);
}

void GXSaveCPUFifo(GXFifoObj* fifo)
{
  __GXSaveCPUFifoAux(fifo);
}

void __GXSaveCPUFifoAux(GXFifoObj* fifo)
{
  GXFifoObjPriv* pFifo = (GXFifoObjPriv*)fifo;
  int interrupts = OSDisableInterrupts();
  GXFlush();
  pFifo->base = OSPhysicalToCached(GX_GET_PI_REG(3));
  pFifo->end = OSPhysicalToCached(GX_GET_PI_REG(4));
  pFifo->writePtr = OSPhysicalToCached(GX_GET_PI_REG(5) & ~0x4000000);
  if (CPGPLinked != 0) {
    u32 reg = GX_GET_CP_REG(29) << 16;
    u32 reg2 = GX_GET_CP_REG(28) | reg;
    pFifo->readPtr = (void*)(reg2 + -0x80000000);
    reg = GX_GET_CP_REG(25) << 16;
    pFifo->rwDistance = (((u32)GX_GET_CP_REG(24) | reg));
  } else {
    pFifo->rwDistance = (u32)pFifo->writePtr - (u32)pFifo->readPtr;
    if (pFifo->rwDistance < 0) {
      pFifo->rwDistance += pFifo->size;
    }
  }
  OSRestoreInterrupts(interrupts);
}

void GXSaveGPFifo(GXFifoObj* fifo)
{
  GXFlush();
  __GXSaveCPUFifoAux(fifo);
}

void GXGetGPStatus(GXBool* overhi, GXBool* underlow, GXBool* readIdle, GXBool* cmdIdle, GXBool* brkpt)
{
  gx->cpStatus = GX_GET_CP_REG(0);
  *overhi = gx->cpStatus & 1;
  *underlow = (gx->cpStatus >> 1) & 1;
  *readIdle = (gx->cpStatus >> 2) & 1;
  *cmdIdle = (gx->cpStatus >> 3) & 1;
  *brkpt = (gx->cpStatus >> 4) & 1;
}

void GXGetFifoStatus(GXFifoObj* obj, GXBool* isOverHi, GXBool* isUnderLo, u32* fifoCount,
                     GXBool* isCpuWrite, GXBool* isGPRead, GXBool* isFifoWrap)
{
  GXFifoObjPriv* pFifo = (GXFifoObjPriv*)obj;
  *isOverHi = GX_FALSE;
  *isUnderLo = GX_FALSE;
  *isFifoWrap = GX_FALSE;
  *isCpuWrite = GX_FALSE;
  *isGPRead = GX_FALSE;

  if (obj == CPUFifo) {
    u32 reg = GX_GET_PI_REG(5);
    if (reg & 0x04000000) {
      *isFifoWrap = GX_TRUE;
    }
    if (reg & 0x02000000) {
      *isCpuWrite = GX_TRUE;
    }
    *fifoCount = pFifo->rwDistance;
  }

  if (obj == GPFifo) {
    u32 status = GX_GET_CP_REG(0);
    if (status & 1) {
      *isOverHi = GX_TRUE;
    }
    if (status & 2) {
      *isUnderLo = GX_TRUE;
    }
    if (status & 0x10) {
      *isGPRead = GX_TRUE;
    }
    *fifoCount = pFifo->rwDistance;
  }
}

void GXGetFifoPtrs(GXFifoObj* obj, void** readPtr, void** writePtr)
{
  GXFifoObjPriv* pFifo = (GXFifoObjPriv*)obj;

  if (obj == CPUFifo) {
    pFifo->writePtr = OSPhysicalToCached(GX_GET_PI_REG(5) & ~0x4000000);
  }
  if (obj == GPFifo) {
    u32 reg = GX_GET_CP_REG(29) << 16;
    pFifo->readPtr = (void*)((GX_GET_CP_REG(28) | reg) + -0x80000000);
    reg = GX_GET_CP_REG(25) << 16;
    pFifo->rwDistance = (GX_GET_CP_REG(24) | reg);
  } else {
    pFifo->rwDistance = (u32)pFifo->writePtr - (u32)pFifo->readPtr;
    if (pFifo->rwDistance < 0) {
      pFifo->rwDistance += pFifo->size;
    }
  }
  *readPtr = pFifo->readPtr;
  *writePtr = pFifo->writePtr;
}

void* GXGetFifoBase(GXFifoObj* obj)
{
  return ((GXFifoObjPriv*)obj)->base;
}

u32 GXGetFifoSize(GXFifoObj* obj)
{
  return ((GXFifoObjPriv*)obj)->size;
}

void GXGetFifoLimits(GXFifoObj* obj, u32* hi, u32* lo)
{
  *hi = ((GXFifoObjPriv*)obj)->highWatermark;
  *lo = ((GXFifoObjPriv*)obj)->lowWatermark;
}

GXBreakPtCallback GXSetBreakPtCallback(GXBreakPtCallback cb)
{
  GXBreakPtCallback oldCallback = BreakPointCB;
  int interrupts = OSDisableInterrupts();
  BreakPointCB = cb;
  OSRestoreInterrupts(interrupts);
  return oldCallback;
}

void GXEnableBreakPt(void* breakPtr)
{
  int interrupts = OSDisableInterrupts();
  __GXFifoReadDisable();
  GX_SET_CP_REG(30, (u16)(u32)breakPtr);
  GX_SET_CP_REG(31, ((u32)breakPtr >> 16) & 0x3FFF);
  GX_SET_REG(gx->cpEnable, 1, 30, 30);
  GX_SET_REG(gx->cpEnable, 1, 26, 26);
  GX_SET_CP_REG(1, gx->cpEnable);
  __GXCurrentBP = breakPtr;
  __GXFifoReadEnable();
  OSRestoreInterrupts(interrupts);
}

void GXDisableBreakPt(void)
{
  int interrupts = OSDisableInterrupts();
  GX_SET_REG(gx->cpEnable, 0, 30, 30);
  GX_SET_REG(gx->cpEnable, 0, 26, 26);
  GX_SET_CP_REG(1, gx->cpEnable);
  __GXCurrentBP = NULL;
  OSRestoreInterrupts(interrupts);
}

void __GXFifoInit(void)
{
  __OSSetInterruptHandler(0x11, GXCPInterruptHandler);
  __OSUnmaskInterrupts(0x4000);
  __GXCurrentThread = OSGetCurrentThread();
  GXOverflowSuspendInProgress = 0;
  CPUFifo = NULL;
  GPFifo = NULL;
}

static void __GXFifoReadEnable(void)
{
  GX_SET_REG(gx->cpEnable, GX_TRUE, 31, 31);
  GX_SET_CP_REG(1, gx->cpEnable);
}

static void __GXFifoReadDisable(void)
{
  GX_SET_REG(gx->cpEnable, GX_FALSE, 31, 31);
  GX_SET_CP_REG(1, gx->cpEnable);
}

static void __GXFifoLink(u8 link)
{
  u32 b;
  if (link) {
    b = 1;
  } else {
    b = 0;
  }
  GX_SET_REG(gx->cpEnable, b, 27, 27);
  GX_SET_CP_REG(1, gx->cpEnable);
}

static void __GXWriteFifoIntEnable(u32 p1, u32 p2)
{
  GX_SET_REG(gx->cpEnable, p1, 29, 29);
  GX_SET_REG(gx->cpEnable, (u8)p2, 28, 28);
  GX_SET_CP_REG(1, gx->cpEnable);
}

static void __GXWriteFifoIntReset(u32 p1, u32 p2)
{
  GX_SET_REG(gx->cpClr, p1, 31, 31);
  GX_SET_REG(gx->cpClr, (u8)p2, 30, 30);
  GX_SET_CP_REG(2, gx->cpClr);
}

void __GXInsaneWatermark(void)
{
  GXFifoObjPriv* pFifo = (GXFifoObjPriv*)GXGetGPFifo();
  if (pFifo) {
    GX_SET_CP_REG(20, 0xFFFF);
    GX_SET_CP_REG(21, 0xFFFF);
    GX_SET_CP_REG(22, 0);
    GX_SET_CP_REG(23, 0);
  }
}

void __GXCleanGPFifo(void)
{
  BOOL interrupts;
  GXFifoObjPriv tempObj;
  u32 i, j, k;
  GXFifoObjPriv* gpFifo;
  GXFifoObjPriv* cpuFifo;
  void* tempPtr;

  gpFifo = (GXFifoObjPriv*)GXGetGPFifo();
  if (!gpFifo) {
    return;
  }

  cpuFifo = (GXFifoObjPriv*)GXGetCPUFifo();
  tempPtr = gpFifo->base;

  tempObj = *gpFifo;

  interrupts = OSDisableInterrupts();

  tempObj.readPtr = tempPtr;
  tempObj.writePtr = tempPtr;
  tempObj.rwDistance = 0;
  if (tempObj.rwDistance < 0) {
    tempObj.rwDistance += tempObj.size;
  }

  OSRestoreInterrupts(interrupts);

  GXSetGPFifo((GXFifoObj*)&tempObj);

  if (cpuFifo == gpFifo) {
    GXSetCPUFifo((GXFifoObj*)&tempObj);
  }

  interrupts = OSDisableInterrupts();

  gpFifo->readPtr = tempPtr;
  gpFifo->writePtr = tempPtr;
  gpFifo->rwDistance = 0;

  if (gpFifo->rwDistance < 0) {
    gpFifo->rwDistance += gpFifo->size;
  }
  OSRestoreInterrupts(interrupts);

  GXSetGPFifo((GXFifoObj*)gpFifo);
  if (cpuFifo == gpFifo) {
    GXSetCPUFifo((GXFifoObj*)cpuFifo);
  }
}

OSThread* GXSetCurrentGXThread(void)
{
  __GXCurrentThread = OSGetCurrentThread();
  return __GXCurrentThread;
}

OSThread* GXGetCurrentGXThread(void)
{
  return __GXCurrentThread;
}

GXFifoObj* GXGetCPUFifo(void)
{
  return CPUFifo;
}

GXFifoObj* GXGetGPFifo(void)
{
  return GPFifo;
}

u32 GXGetOverflowCount(void)
{
  return __GXOverflowCount;
}

u32 GXResetOverflowCount(void)
{
  u32 old = __GXOverflowCount;
  __GXOverflowCount = 0;
  return old;
}

void* GXRedirectWriteGatherPipe(void* ptr)
{
  GXFifoObjPriv* pFifo = (GXFifoObjPriv*)CPUFifo;
  void* old;
  u32 count;
  int interrupts;

  if (!pFifo) {
    return NULL;
  }

  interrupts = OSDisableInterrupts();
  old = pFifo->writePtr;
  count = (u32)pFifo->writePtr - (u32)pFifo->base;
  GX_SET_PI_REG(3, (u32)pFifo->base + count);
  GX_SET_PI_REG(5, (u32)ptr);
  pFifo->writePtr = ptr;
  pFifo->rwDistance = (u32)ptr - (u32)pFifo->readPtr;
  OSRestoreInterrupts(interrupts);
  return old;
}

void GXRestoreWriteGatherPipe(void)
{
  GXFifoObjPriv* pFifo = (GXFifoObjPriv*)CPUFifo;
  u32 count;
  int interrupts;

  if (!pFifo) {
    return;
  }

  interrupts = OSDisableInterrupts();
  count = (u32)pFifo->writePtr - (u32)pFifo->base;
  pFifo->writePtr = (void*)((u32)pFifo->base + count);
  GX_SET_PI_REG(3, (u32)pFifo->base + count);
  GX_SET_PI_REG(5, (u32)pFifo->writePtr);
  OSRestoreInterrupts(interrupts);
}
