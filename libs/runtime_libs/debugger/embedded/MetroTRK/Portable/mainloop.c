#include "types.h"
#include "PowerPC_EABI_Support/MetroTRK/trk.h"

void TRKHandleRequestEvent(void)
{
	// UNUSED FUNCTION
}

void TRKHandleSupportEvent(void)
{
	// UNUSED FUNCTION
}

void TRKIdle(void)
{
	// UNUSED FUNCTION
}

void TRKNubMainLoop(void)
{
	void* msg;
	TRKEvent event;
	BOOL isShutdownRequested;
	BOOL isNewInput;

	isShutdownRequested = FALSE;
	isNewInput          = FALSE;
	while (isShutdownRequested == FALSE) {
		if (TRKGetNextEvent(&event) != FALSE) {
			isNewInput = FALSE;

			switch (event.mEventType) {
			case 0:
				break;

			case 2:
				msg = TRKGetBuffer(event.mBufferIndex);
				TRKDispatchMessage(msg);
				break;

			case 1:
				isShutdownRequested = TRUE;
				break;

			case 3:
			case 4:
				TRKTargetInterrupt(&event);
				break;

			case 5:
				TRKTargetSupportRequest();
				break;
			}

			TRKDestructEvent(&event);
			continue;
		}

		if ((isNewInput == FALSE) || (*(u8*)gTRKInputPendingPtr != '\0')) {
			isNewInput = TRUE;
			TRKGetInput();
			continue;
		}

		if (TRKTargetStopped() == FALSE) {
			TRKTargetContinue();
		}
		isNewInput = FALSE;
	}
}
