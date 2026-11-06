#include "types.h"
extern BOOL gTRKBigEndian __attribute__((section(".data")));
#include "PowerPC_EABI_Support/MetroTRK/trk.h"

#define DS_NoError                     TRKSuccess
#define DS_MessageBufferOverflow       TRKError301
#define DS_MessageBufferReadError      TRKError302
#define DS_NoMessageBufferAvailable    0x300

typedef struct MessageBuffer {
	u32 _00;
	BOOL isInUse;
	u32 length;
	u32 position;
	u8 data[0x880];
} MessageBuffer;

extern BOOL usr_puts_serial(const char* msg);
void* TRK_memset(void* dst, int val, size_t n);
TRKResult TRKReadBuffer(MessageBuffer* msg, void* data, size_t length);
TRKResult TRKAppendBuffer(MessageBuffer* msg, const void* data, size_t length);
TRKResult TRKAppendBuffer_ui32(MessageBuffer* buffer, const u32* data, int count);
TRKResult TRKReadBuffer1_ui32(MessageBuffer* buffer, u32* data);
TRKResult TRKReadBuffer_ui8(MessageBuffer* buffer, u8* data, int count);
void TRKResetBuffer(MessageBuffer* msg, BOOL keepData);

MessageBuffer gTRKMsgBufs[3];

static inline TRKResult TRKAppendBuffer1_ui8(MessageBuffer* buffer, const u8 data)
{
	if (buffer->position >= 0x880) {
		return DS_MessageBufferOverflow;
	}

	buffer->data[buffer->position++] = data;
	buffer->length++;
	return TRKSuccess;
}

void TRKSetBufferUsed(MessageBuffer* msg, BOOL state)
{
	msg->isInUse = state;
}

TRKResult TRKInitializeMessageBuffers(void)
{
	int i;
	for (i = 0; i < 3; i++) {
		TRKInitializeMutex(&gTRKMsgBufs[i]);
		TRKAcquireMutex(&gTRKMsgBufs[i]);
		TRKSetBufferUsed(&gTRKMsgBufs[i], FALSE);
		TRKReleaseMutex(&gTRKMsgBufs[i]);
	}

	return TRKSuccess;
}

TRKResult TRKGetFreeBuffer(int* msgID, TRKBuffer** outMsg)
{
	MessageBuffer* buf;
	TRKResult error = DS_NoMessageBufferAvailable;
	int i;

	*outMsg = nullptr;

	for (i = 0; i < 3; i++) {
		buf = TRKGetBuffer(i);

		TRKAcquireMutex(buf);
		if (!buf->isInUse) {
			TRKResetBuffer(buf, TRUE);
			TRKSetBufferUsed(buf, TRUE);
			error   = TRKSuccess;
			*outMsg = (TRKBuffer*)buf;
			*msgID  = i;
			i       = 3;
		}
		TRKReleaseMutex(buf);
	}

	if (error == DS_NoMessageBufferAvailable) {
		usr_puts_serial("ERROR : No buffer available\n");
	}

	return error;
}

void* TRKGetBuffer(int idx)
{
	MessageBuffer* buf = nullptr;
	if (idx >= 0 && idx < 3) {
		buf = &gTRKMsgBufs[idx];
	}

	return buf;
}

void TRKReleaseBuffer(int idx)
{
	MessageBuffer* msg;
	if (idx != -1 && idx >= 0 && idx < 3) {
		msg = &gTRKMsgBufs[idx];
		TRKAcquireMutex(msg);
		TRKSetBufferUsed(msg, FALSE);
		TRKReleaseMutex(msg);
	}
}

void TRKResetBuffer(MessageBuffer* msg, BOOL keepData)
{
	msg->length   = 0;
	msg->position = 0;

	if (!keepData) {
		TRK_memset(msg->data, 0, 0x880);
	}
}

TRKResult TRKSetBufferPosition(TRKBuffer* buffer, u32 pos)
{
	MessageBuffer* msg = (MessageBuffer*)buffer;
	TRKResult error = TRKSuccess;

	if (pos > 0x880) {
		error = DS_MessageBufferOverflow;
	} else {
		msg->position = pos;
		if (pos > msg->length) {
			msg->length = pos;
		}
	}

	return error;
}

TRKResult TRKAppendBuffer(MessageBuffer* msg, const void* data, size_t length)
{
	TRKResult error = TRKSuccess;
	u32 bytesLeft;

	if (length == 0) {
		return TRKSuccess;
	}

	bytesLeft = 0x880 - msg->position;

	if (bytesLeft < length) {
		error  = DS_MessageBufferOverflow;
		length = bytesLeft;
	}

	if (length == 1) {
		msg->data[msg->position] = ((u8*)data)[0];
	} else {
		TRK_memcpy(msg->data + msg->position, data, length);
	}

	msg->position += length;
	msg->length = msg->position;

	return error;
}

TRKResult TRKReadBuffer(MessageBuffer* msg, void* data, size_t length)
{
	TRKResult error = TRKSuccess;
	uint bytesLeft;

	if (length == 0) {
		return TRKSuccess;
	}

	bytesLeft = msg->length - msg->position;

	if (length > bytesLeft) {
		error  = DS_MessageBufferReadError;
		length = bytesLeft;
	}

	TRK_memcpy(data, msg->data + msg->position, length);
	msg->position += length;
	return error;
}

void TRKAppendBuffer1_ui16(void)
{
}

TRKResult TRKAppendBuffer1_ui32(MessageBuffer* buffer, const u32 data)
{
	u8* bigEndianData;
	u8* byteData;
	u8 swapBuffer[sizeof(data)];

	if (gTRKBigEndian) {
		bigEndianData = (u8*)&data;
	} else {
		byteData      = (u8*)&data;
		bigEndianData = swapBuffer;

		bigEndianData[0] = byteData[3];
		bigEndianData[1] = byteData[2];
		bigEndianData[2] = byteData[1];
		bigEndianData[3] = byteData[0];
	}

	return TRKAppendBuffer(buffer, (const void*)bigEndianData, sizeof(data));
}

TRKResult TRKAppendBuffer1_ui64(MessageBuffer* buffer, const u64 data)
{
	u8* bigEndianData;
	u8* byteData;
	u8 swapBuffer[sizeof(data)];
	if (gTRKBigEndian) {
		bigEndianData = (u8*)&data;
	} else {
		byteData      = (u8*)&data;
		bigEndianData = swapBuffer;

		bigEndianData[0] = byteData[7];
		bigEndianData[1] = byteData[6];
		bigEndianData[2] = byteData[5];
		bigEndianData[3] = byteData[4];
		bigEndianData[4] = byteData[3];
		bigEndianData[5] = byteData[2];
		bigEndianData[6] = byteData[1];
		bigEndianData[7] = byteData[0];
	}

	return TRKAppendBuffer(buffer, (const void*)bigEndianData, sizeof(data));
}

void TRKAppendBuffer1_ui128(void)
{
}

TRKResult TRKAppendBuffer_ui8(TRKBuffer* buffer, u8* data, int count)
{
	MessageBuffer* msg = (MessageBuffer*)buffer;
	TRKResult err;
	int i;

	for (i = 0, err = TRKSuccess; err == TRKSuccess && i < count; i++) {
		err = TRKAppendBuffer1_ui8(msg, data[i]);
	}

	return err;
}

void TRKAppendBuffer_ui16(void)
{
}

TRKResult TRKAppendBuffer_ui32(MessageBuffer* buffer, const u32* data, int count)
{
	TRKResult err;
	int i;

	for (i = 0, err = TRKSuccess; err == TRKSuccess && i < count; i++) {
		err = TRKAppendBuffer1_ui32(buffer, data[i]);
	}

	return err;
}

void TRKAppendBuffer_ui64(void)
{
}

void TRKAppendBuffer_ui128(void)
{
}

TRKResult TRKReadBuffer1_ui8(MessageBuffer* buffer, u8* data)
{
	return TRKReadBuffer(buffer, (void*)data, 1);
}

void TRKReadBuffer1_ui16(void)
{
}

TRKResult TRKReadBuffer1_ui32(MessageBuffer* buffer, u32* data)
{
	TRKResult err;

	u8* bigEndianData;
	u8* byteData;
	u8 swapBuffer[sizeof(data)];

	if (gTRKBigEndian) {
		bigEndianData = (u8*)data;
	} else {
		bigEndianData = swapBuffer;
	}

	err = TRKReadBuffer(buffer, (void*)bigEndianData, sizeof(*data));

	if (!gTRKBigEndian && err == TRKSuccess) {
		byteData = (u8*)data;

		byteData[0] = bigEndianData[3];
		byteData[1] = bigEndianData[2];
		byteData[2] = bigEndianData[1];
		byteData[3] = bigEndianData[0];
	}

	return err;
}

TRKResult TRKReadBuffer1_ui64(MessageBuffer* buffer, u64* data)
{
	TRKResult err;

	u8* bigEndianData;
	u8* byteData;
	u8 swapBuffer[sizeof(data)];

	if (gTRKBigEndian) {
		bigEndianData = (u8*)data;
	} else {
		bigEndianData = swapBuffer;
	}

	err = TRKReadBuffer(buffer, (void*)bigEndianData, sizeof(*data));

	if (!gTRKBigEndian && err == 0) {
		byteData = (u8*)data;

		byteData[0] = bigEndianData[7];
		byteData[1] = bigEndianData[6];
		byteData[2] = bigEndianData[5];
		byteData[3] = bigEndianData[4];
		byteData[4] = bigEndianData[3];
		byteData[5] = bigEndianData[2];
		byteData[6] = bigEndianData[1];
		byteData[7] = bigEndianData[0];
	}

	return err;
}

void TRKReadBuffer1_ui128(MessageBuffer* buffer, u8* p2, int p3)
{
}

TRKResult TRKReadBuffer_ui8(MessageBuffer* buffer, u8* data, int count)
{
	TRKResult err;
	int i;

	for (i = 0, err = TRKSuccess; err == TRKSuccess && i < count; i++) {
		err = TRKReadBuffer1_ui8(buffer, &(data[i]));
	}

	return err;
}

void TRKReadBuffer_ui16(MessageBuffer* buffer, u8* p2, int p3)
{
}

TRKResult TRKReadBuffer_ui32(MessageBuffer* buffer, u32* data, int count)
{
	TRKResult err;
	s32 i;

	for (i = 0, err = TRKSuccess; err == TRKSuccess && i < count; i++) {
		err = TRKReadBuffer1_ui32(buffer, &(data[i]));
	}

	return err;
}

void TRKReadBuffer_ui64(MessageBuffer* buffer, u8* p2, int p3)
{
}

void TRKReadBuffer_ui128(MessageBuffer* buffer, u8* p2, int p3)
{
}
