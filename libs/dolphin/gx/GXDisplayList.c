#include <string.h>
#include <dolphin/gx.h>
#include <dolphin/gx/GXPriv.h>
#include <dolphin/os.h>

static GXFifoObj DisplayListFifo;
static GXFifoObj* OldCPUFifo;
static GXData __savedGXdata;

void GXBeginDisplayList(void* list, u32 size)
{
	if (gx->dirtyState) {
		__GXSetDirtyState();
	}

	if (gx->dlSaveContext) {
		memcpy(&__savedGXdata, gx, sizeof(*gx));
	}

	GXInitFifoBase(&DisplayListFifo, list, size);
	gx->inDispList = GX_TRUE;

	OldCPUFifo = GXGetCPUFifo();
	GXSetCPUFifo(&DisplayListFifo);
	GXResetWriteGatherPipe();
}

u32 GXEndDisplayList(void)
{
	GXBool ov;
	BOOL enabled;
	u32 cpenable;
	u32 count;
	GXBool overhi, underlow, cpuWrite, gpRead;

	GXGetFifoStatus(&DisplayListFifo, &overhi, &underlow, &count, &cpuWrite, &gpRead, &ov);

	GXSetCPUFifo(OldCPUFifo);

	if (gx->dlSaveContext) {
		enabled  = OSDisableInterrupts();
		cpenable = gx->cpEnable;
		memcpy(gx, &__savedGXdata, sizeof(*gx));
		gx->cpEnable = cpenable;
		OSRestoreInterrupts(enabled);
	}

	gx->inDispList = GX_FALSE;

	if (!ov) {
		return count;
	}

	return 0;
}

void GXCallDisplayList(const void* dl, u32 byteCnt)
{
	if (gx->dirtyState) {
		__GXSetDirtyState();
	}
	if (GX_CHECK_FLUSH()) {
		__GXSendFlushPrim();
	}
	GX_WRITE_U8(GX_CMD_CALL_DL);
	GX_WRITE_U32((u32)dl);
	GX_WRITE_U32(byteCnt);
}
