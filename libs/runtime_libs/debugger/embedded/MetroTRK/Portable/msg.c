#include "PowerPC_EABI_Support/MetroTRK/trk.h"

void TRKMessageAdd(void)
{
}

void TRKMessageGet(void)
{
}

TRKResult TRKMessageSend(TRK_Msg* msg)
{
	TRKResult writeErr = TRKWriteUARTN(&msg->mMsg, msg->mMsgLength);
	MWTRACE(1, "MessageSend : cc_write returned %ld\n", writeErr);
	return TRKSuccess;
}
