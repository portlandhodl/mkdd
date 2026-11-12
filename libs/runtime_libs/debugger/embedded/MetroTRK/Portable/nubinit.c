#include "PowerPC_EABI_Support/MetroTRK/trk.h"

BOOL gTRKBigEndian;

extern void TRK_board_display(char*);

TRKResult TRKInitializeNub(void)
{
	TRKResult ret;
	TRKResult uartErr;

	ret = TRKInitializeEndian();

	MWTRACE(1, "Initialize NUB\n");
	if (ret == TRKSuccess) {
		usr_put_initialize();
	}
	if (ret == TRKSuccess) {
		ret = TRKInitializeEventQueue();
	}
	if (ret == TRKSuccess) {
		ret = TRKInitializeMessageBuffers();
	}
	if (ret == TRKSuccess) {
		ret = TRKInitializeDispatcher();
	}
	InitializeProgramEndTrap();
	if (ret == TRKSuccess) {
		ret = TRKInitializeSerialHandler();
	}
	if (ret == TRKSuccess) {
		ret = TRKInitializeTarget();
	}
	if (ret == TRKSuccess) {

		uartErr = TRKInitializeIntDrivenUART(0x0000e100, 1, 0, &gTRKInputPendingPtr);
		TRKTargetSetInputPendingPtr(gTRKInputPendingPtr);
		if (uartErr != TRKSuccess) {
			ret = uartErr;
		}
	}
	return ret;
}

TRKResult TRKTerminateNub(void)
{
	TRKTerminateSerialHandler();
	return TRKSuccess;
}

void TRKNubWelcome(void)
{
	TRK_board_display("MetroTRK for GAMECUBE v2.0");
	return;
}

BOOL TRKInitializeEndian(void)
{
	u8 bendian[4];
	BOOL result   = FALSE;
	gTRKBigEndian = TRUE;

	bendian[0] = 0x12;
	bendian[1] = 0x34;
	bendian[2] = 0x56;
	bendian[3] = 0x78;

	if (*(u32*)bendian == 0x12345678) {
		gTRKBigEndian = TRUE;
	} else if (*(u32*)bendian == 0x78563412) {
		gTRKBigEndian = FALSE;
	} else {
		result = TRUE;
	}
	return result;
}
