#include "types.h"
#include "dolphin/db.h"
#include "PowerPC_EABI_Support/MetroTRK/trk.h"
#include "PowerPC_EABI_Support/MetroTRK/custconn/CircleBuffer.h"

#define GDEV_BUF_SIZE (0x500)

#define DDH_ERR_NOT_INITIALIZED     -0x2711
#define DDH_ERR_ALREADY_INITIALIZED -0x2715
#define DDH_ERR_READ_ERROR          -0x2719

extern void DBInitInterrupts();
extern u32 DBQueryData();
extern BOOL DBRead(void*, u32);
extern BOOL DBWrite(const void*, u32);
extern void DBOpen();
extern void DBClose();

static CircleBuffer gRecvCB;
static u8 gRecvBuf[GDEV_BUF_SIZE];
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

int gdev_cc_initialize(void* inputPendingPtrRef, void* monitorCallback)
{
	MWTRACE(1, "CALLING EXI2_Init\n");
	DBInitComm(inputPendingPtrRef, monitorCallback);
	MWTRACE(1, "DONE CALLING EXI2_Init\n");
	CircleBufferInitialize(&gRecvCB, gRecvBuf, GDEV_BUF_SIZE);
	return 0;
}

int gdev_cc_shutdown()
{
	return 0;
}

int gdev_cc_open()
{
	if (gIsInitialized) {
		return DDH_ERR_ALREADY_INITIALIZED;
	}

	gIsInitialized = TRUE;
	return 0;
}

int gdev_cc_close()
{
	return 0;
}

int gdev_cc_read(u8* data, int size)
{
	u8 buff[GDEV_BUF_SIZE];
	int p1;
	u32 retval;
	int p2;
	int poll;
	retval = 0;
	if (!gIsInitialized) {
		return DDH_ERR_NOT_INITIALIZED;
	}

	MWTRACE(1, "Expected packet size : 0x%08x (%ld)\n", size, size);

	p1 = size;
	p2 = size;
	while ((u32)CBGetBytesAvailableForRead(&gRecvCB) < p2) {
		retval = 0;
		poll   = DBQueryData();
		if (poll != 0) {
			retval = DBRead(buff, p2);
			if (retval == 0) {
				CircleBufferWriteBytes(&gRecvCB, buff, poll);
			}
		}
	}

	if (retval == 0) {
		CircleBufferReadBytes(&gRecvCB, data, p1);
	} else {
		MWTRACE(8, "cc_read : error reading bytes from EXI2 %ld\n", retval);
	}

	return retval;
}

int gdev_cc_write(const u8* bytes, int length)
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
		exi2Len = DBWrite((const void*)hexCopy, n_copy);
		if (exi2Len == 0) {
			break;
		}
		hexCopy += exi2Len;
		n_copy -= exi2Len;
	}

	return 0;
}

int gdev_cc_pre_continue()
{
	DBClose();
	return 0;
}

int gdev_cc_post_stop()
{
	DBOpen();
	return 0;
}

int gdev_cc_peek()
{
	int poll;
	u8 buff[GDEV_BUF_SIZE];

	poll = DBQueryData();
	if (poll <= 0) {
		return 0;
	}

	if (DBRead(buff, poll) == 0) {
		CircleBufferWriteBytes(&gRecvCB, buff, poll);
	} else {
		return DDH_ERR_READ_ERROR;
	}

	return poll;
}

int gdev_cc_initinterrupts()
{
	DBInitInterrupts();
	return 0;
}
