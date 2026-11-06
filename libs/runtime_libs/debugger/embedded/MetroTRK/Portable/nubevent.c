#include "PowerPC_EABI_Support/MetroTRK/trk.h"

TRKEventQueue gTRKEventQueue;

TRKResult TRKInitializeEventQueue()
{
	TRKInitializeMutex(&gTRKEventQueue);
	TRKAcquireMutex(&gTRKEventQueue);
	gTRKEventQueue.mCurrEvtID          = 0;
	gTRKEventQueue.mNextSlotToOverwrite = 0;
	gTRKEventQueue._24                 = 0x100;
	TRKReleaseMutex(&gTRKEventQueue);
	return TRKSuccess;
}

void TRKCopyEvent(void)
{
	// UNUSED FUNCTION
}

BOOL TRKGetNextEvent(TRKEvent* ev)
{
	BOOL ret = FALSE;

	TRKAcquireMutex(&gTRKEventQueue);

	if (gTRKEventQueue.mCurrEvtID > 0) {
		TRK_memcpy(ev, &gTRKEventQueue.mEvents[gTRKEventQueue.mNextSlotToOverwrite], sizeof(TRKEvent));
		gTRKEventQueue.mCurrEvtID--;

		if (++gTRKEventQueue.mNextSlotToOverwrite == 2) {
			gTRKEventQueue.mNextSlotToOverwrite = 0;
		}

		ret = TRUE;
	}

	TRKReleaseMutex(&gTRKEventQueue);
	return ret;
}

TRKResult TRKPostEvent(TRKEvent* event)
{
	TRKResult ret = TRKSuccess;
	int nextEventID;

	TRKAcquireMutex(&gTRKEventQueue);

	if (gTRKEventQueue.mCurrEvtID == 2) {
		ret = 0x100;

	} else {
		nextEventID = (gTRKEventQueue.mNextSlotToOverwrite + gTRKEventQueue.mCurrEvtID) % 2;
		TRK_memcpy(&gTRKEventQueue.mEvents[nextEventID], event, sizeof(TRKEvent));
		gTRKEventQueue.mEvents[nextEventID]._04 = gTRKEventQueue._24;

		if (++gTRKEventQueue._24 < 0x100) {
			gTRKEventQueue._24 = 0x100;
		}

		gTRKEventQueue.mCurrEvtID++;
	}

	TRKReleaseMutex(&gTRKEventQueue);
	return ret;
}

void TRKConstructEvent(TRKEvent* event, int eventType)
{
	event->mEventType  = eventType;
	event->_04         = 0;
	event->mBufferIndex = -1;
}

void TRKDestructEvent(TRKEvent* event)
{
	TRKReleaseBuffer(event->mBufferIndex);
}
