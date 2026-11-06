#include "PowerPC_EABI_Support/MetroTRK/trk.h"

TRKResult TRKInitializeDispatcher(void)
{
	return TRKSuccess;
}

TRKResult TRKDispatchMessage(TRKBuffer* msg)
{
	u32 err;

	err = 0x500;
	TRKSetBufferPosition(msg, 0);
	MWTRACE(1, "Dispatch command 0x%08x\n", msg->mBuffer[0]);
	switch (msg->mBuffer[0]) {
	case TRK_DISPATCH_CMD_CONNECT:
		err = TRKDoConnect(msg);
		break;
	case TRK_DISPATCH_CMD_DISCONNECT:
		err = TRKDoDisconnect(msg);
		break;
	case TRK_DISPATCH_CMD_RESET:
		err = TRKDoReset(msg);
		break;
	case TRK_DISPATCH_CMD_OVERRIDE:
		err = TRKDoOverride(msg);
		break;
	case TRK_DISPATCH_CMD_GETVERSION:
		err = TRKDoVersions(msg);
		break;
	case TRK_DISPATCH_CMD_GETSUPPORTMASK:
		err = TRKDoSupportMask(msg);
		break;
	case TRK_DISPATCH_CMD_READMEM:
		err = TRKDoReadMemory(msg);
		break;
	case TRK_DISPATCH_CMD_WRITEMEM:
		err = TRKDoWriteMemory(msg);
		break;
	case TRK_DISPATCH_CMD_READREGS:
		err = TRKDoReadRegisters(msg);
		break;
	case TRK_DISPATCH_CMD_WRITEREGS:
		err = TRKDoWriteRegisters(msg);
		break;
	case TRK_DISPATCH_CMD_CONTINUE:
		err = TRKDoContinue(msg);
		break;
	case TRK_DISPATCH_CMD_STEP:
		err = TRKDoStep(msg);
		break;
	case TRK_DISPATCH_CMD_STOP:
		err = TRKDoStop(msg);
		break;
	case TRK_DISPATCH_CMD_SETOPTION:
		err = TRKDoSetOption(msg);
		break;
	}
	MWTRACE(1, "Dispatch complete err = %ld\n", err);
	return err;
}
