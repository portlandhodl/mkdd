#include "types.h"
#include "PowerPC_EABI_Support/MetroTRK/trk.h"

extern TRKResult TRKGetFreeBuffer(int* bufIdx, TRKBuffer** msg);
extern TRKResult TRKRequestSend(TRKBuffer* msg, int* reqIdx, int p1, int p2, int p3);
extern void TRKTargetAddStopInfo(TRKBuffer* msg);
extern void TRKTargetAddExceptionInfo(TRKBuffer* msg);

void TRKWaitForACK(void)
{
	// UNUSED FUNCTION
}

TRKResult TRKDoNotifyStopped(int cmd)
{
	int reqIdx;
	int bufIdx;
	TRKBuffer* msg;
	TRKResult err;
	TRKResult bufError;

	bufError = TRKGetFreeBuffer(&bufIdx, &msg);
	if ((err = bufError) == FALSE) {
		if (err == TRKSuccess) {
			if (cmd == 0x90) {
				TRKTargetAddStopInfo(msg);
			} else {
				TRKTargetAddExceptionInfo(msg);
			}
		}
		bufError = TRKRequestSend(msg, &reqIdx, 2, 3, 1);
		err      = bufError;
		if (err == TRKSuccess) {
			TRKReleaseBuffer(reqIdx);
		}
		TRKReleaseBuffer(bufIdx);
	}
	return err;
}
