#include "PowerPC_EABI_Support/MetroTRK/trk.h"

typedef struct TRKFramingState {
	int msgBufID;
	void* buffer;
	int receiveState;
	BOOL isEscape;
	u8 fcsType;
} TRKFramingState;

static TRKFramingState gTRKFramingState;
void* gTRKInputPendingPtr;

extern TRKResult TRKGetFreeBuffer(int* bufIdx, TRKBuffer** msg);
extern int TRKPollUART();
extern UARTError TRKReadUARTN(void*, u32);

int TRKTestForPacket(void)
{
	u8 payloadBuf[0x880];
	u8 packetBuf[0x40];
	int bufID;
	TRKBuffer* msg;
	int result;

	if (TRKPollUART() <= 0) {
		return -1;
	}

	result = TRKGetFreeBuffer(&bufID, &msg);

	MWTRACE(4, "TestForPacket : FreeBuffer is  %ld\n", result);

	TRKSetBufferPosition(msg, 0);
	if (TRKReadUARTN(packetBuf, 0x40) == kUARTNoError) {
		int readSize;

		TRKAppendBuffer_ui8(msg, packetBuf, 0x40);
		readSize = ((u32*)packetBuf)[0] - 0x40;
		result   = bufID;
		if (readSize > 0) {
			MWTRACE(1, "Reading payload %ld bytes\n", readSize);
			if (TRKReadUARTN(payloadBuf, ((u32*)packetBuf)[0] - 0x40) == kUARTNoError) {
				TRKAppendBuffer_ui8(msg, payloadBuf, ((u32*)packetBuf)[0]);
			} else {
				MWTRACE(8, "TestForPacket : Invalid size of packet hdr.size\n");
				TRKReleaseBuffer(result);
				result = -1;
			}
		}
	} else {
		MWTRACE(8, "TestForPacket : Invalid size of packet\n");
		TRKReleaseBuffer(result);
		result = -1;
	}

	MWTRACE(1, "TestForPacket returning %ld\n", result);
	return result;
}

void TRKGetInput(void)
{
	int id = TRKTestForPacket();
	if (id != -1) {
		TRKEvent event;
		TRKGetBuffer(id);
		TRKConstructEvent(&event, 2);
		event.mBufferIndex        = id;
		gTRKFramingState.msgBufID = -1;
		TRKPostEvent(&event);
	}
}

void TRKProcessInput(int bufferIdx)
{
	TRKEvent event;

	TRKConstructEvent(&event, 2);
	event.mBufferIndex        = bufferIdx;
	gTRKFramingState.msgBufID = -1;
	TRKPostEvent(&event);
}

TRKResult TRKInitializeSerialHandler()
{
	gTRKFramingState.msgBufID     = -1;
	gTRKFramingState.receiveState = 0;
	gTRKFramingState.isEscape     = FALSE;

	MWTRACE(1, "TRK_Packet_Header \t    %ld bytes\n", 0x40);
	MWTRACE(1, "TRK_CMD_ReadMemory     %ld bytes\n", 0x40);
	MWTRACE(1, "TRK_CMD_WriteMemory    %ld bytes\n", 0x40);
	MWTRACE(1, "TRK_CMD_Connect \t    %ld bytes\n", 0x40);
	MWTRACE(1, "TRK_CMD_ReplyAck\t    %ld bytes\n", 0x40);
	MWTRACE(1, "TRK_CMD_ReadRegisters\t%ld bytes\n", 0x40);

	return TRKSuccess;
}

TRKResult TRKTerminateSerialHandler(void)
{
	return TRKSuccess;
}
