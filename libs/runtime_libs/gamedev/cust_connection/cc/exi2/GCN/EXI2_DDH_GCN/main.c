#include "types.h"
#include "PowerPC_EABI_Support/MetroTRK/trk.h"
#include "PowerPC_EABI_Support/MetroTRK/custconn/CircleBuffer.h"

#define DDH_BUF_SIZE (0x800)

#define DDH_ERR_NOT_INITIALIZED     -0x2711
#define DDH_ERR_ALREADY_INITIALIZED -0x2715
#define DDH_ERR_READ_ERROR          -0x2719

extern void EXI2_Init(void*, void*);
extern void EXI2_EnableInterrupts(void);
extern int EXI2_Poll(void);
extern int EXI2_ReadN(void*, u32);
extern int EXI2_WriteN(const void*, u32);
extern void EXI2_Reserve(void);
extern void EXI2_Unreserve(void);

static CircleBuffer gRecvCB;
static u8 gRecvBuf[DDH_BUF_SIZE];
static BOOL gIsInitialized;

void OutputData(void* data, int length)
{
	int i;
	u8* datapointer = data;

	for (i = 0; i < length; i++) {
		MWTRACE(8, "%02x ", datapointer[i]);
		if (i % 16 == 15) {
			MWTRACE(8, "\n");
		}
	}

	MWTRACE(8, "\n");
}

BOOL IsInitialized()
{
	return gIsInitialized;
}

int ddh_cc_initialize(void* inputPendingPtrRef, void* monitorCallback)
{
	MWTRACE(1, "CALLING EXI2_Init\n");
	EXI2_Init(inputPendingPtrRef, monitorCallback);
	MWTRACE(1, "DONE CALLING EXI2_Init\n");
	CircleBufferInitialize(&gRecvCB, gRecvBuf, DDH_BUF_SIZE);
	return 0;
}

int ddh_cc_shutdown()
{
	return 0;
}

int ddh_cc_open()
{
	if (gIsInitialized) {
		return DDH_ERR_ALREADY_INITIALIZED;
	}

	gIsInitialized = TRUE;
	return 0;
}

int ddh_cc_close()
{
	return 0;
}

int ddh_cc_read(u8* data, int size)
{
	u8 buff[DDH_BUF_SIZE];
	int originalDataSize;
	u32 result;
	int expectedDataSize;
	int poll;

	result = 0;
	if (!gIsInitialized) {
		return DDH_ERR_NOT_INITIALIZED;
	}

	MWTRACE(1, "Expected packet size : 0x%08x (%ld)\n", size, size);

	originalDataSize = expectedDataSize = size;
	while ((u32)CBGetBytesAvailableForRead(&gRecvCB) < expectedDataSize) {
		result = 0;

		poll = EXI2_Poll();
		if (poll != 0) {
			result = EXI2_ReadN(buff, poll);
			if (result == 0) {
				CircleBufferWriteBytes(&gRecvCB, buff, poll);
			}
		}
	}

	if (result == 0) {
		CircleBufferReadBytes(&gRecvCB, data, originalDataSize);
	} else {
		MWTRACE(8, "cc_read : error reading bytes from EXI2 %ld\n", result);
	}

	return result;
}

int ddh_cc_write(const u8* bytes, int length)
{
	int exi2Len;
	int n_copy;
	u32 hexCopy;

	hexCopy = (u32)bytes;
	n_copy  = length;

	if (gIsInitialized == FALSE) {
		MWTRACE(8, "cc not initialized\n");
		return DDH_ERR_NOT_INITIALIZED;
	}

	MWTRACE(8, "cc_write : Output data 0x%08x %ld bytes\n", bytes, length);

	while (n_copy > 0) {
		MWTRACE(1, "cc_write sending %ld bytes\n", n_copy);
		exi2Len = EXI2_WriteN((const void*)hexCopy, n_copy);
		if (exi2Len == 0) {
			break;
		}
		hexCopy += exi2Len;
		n_copy -= exi2Len;
	}

	return 0;
}

int ddh_cc_pre_continue()
{
	EXI2_Unreserve();
	return 0;
}

int ddh_cc_post_stop()
{
	EXI2_Reserve();
	return 0;
}

int ddh_cc_peek()
{
	int poll;
	u8 buff[DDH_BUF_SIZE];

	poll = EXI2_Poll();
	if (poll <= 0) {
		return 0;
	}

	if (EXI2_ReadN(buff, poll) == 0) {
		CircleBufferWriteBytes(&gRecvCB, buff, poll);
	} else {
		return DDH_ERR_READ_ERROR;
	}

	return poll;
}

int ddh_cc_initinterrupts()
{
	EXI2_EnableInterrupts();
	return 0;
}
