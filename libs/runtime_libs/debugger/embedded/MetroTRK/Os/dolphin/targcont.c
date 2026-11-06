#include "PowerPC_EABI_Support/MetroTRK/trk.h"

extern u32 gTRKCPUState[];

void OutputGPR(void)
{
	u32* gpr = gTRKCPUState;
	int i;

	for (i = 0; i < 32; i++) {
		MWTRACE(1, "r%d = 0x%08x\n", i, gpr[i]);
	}
}

TRKResult TRKTargetContinue(void)
{
	TRKTargetSetStopped(0);
	UnreserveEXI2Port();
	TRKSwapAndGo();
	ReserveEXI2Port();
	return TRKSuccess;
}
