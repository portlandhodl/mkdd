#include "PowerPC_EABI_Support/MetroTRK/custconn/CircleBuffer.h"
#include "string.h"

extern void MWInitializeCriticalSection(uint* section);
extern void MWEnterCriticalSection(uint* section);
extern void MWExitCriticalSection(uint* section);

u32 CBGetBytesAvailableForRead(CircleBuffer* cb)
{
	return cb->mBytesToRead;
}

u32 CBGetBytesAvailableForWrite(CircleBuffer* cb)
{
	return cb->mBytesToWrite;
}

void CircleBufferInitialize(CircleBuffer* cb, u8* buf, u32 size)
{
	cb->unk8        = buf;
	cb->unkC        = size;
	cb->unk0        = cb->unk8;
	cb->unk4        = cb->unk8;
	cb->mBytesToRead  = 0;
	cb->mBytesToWrite = cb->unkC;
	MWInitializeCriticalSection(&cb->mSection);
}

void CircleBufferTerminate(CircleBuffer* cb)
{
	// UNUSED FUNCTION
}

int CircleBufferWriteBytes(CircleBuffer* cb, u8* buf, u32 size)
{
	int availSize;

	if (size > cb->mBytesToWrite) {
		return -1;
	}
	MWEnterCriticalSection(&cb->mSection);
	availSize = cb->unkC - (cb->unk4 - cb->unk8);
	if (availSize >= size) {
		memcpy(cb->unk4, buf, size);
		cb->unk4 += size;
	} else {
		memcpy(cb->unk4, buf, availSize);
		memcpy(cb->unk8, buf + availSize, size - availSize);
		cb->unk4 = cb->unk8 + size - availSize;
	}

	if (cb->unkC == (cb->unk4 - cb->unk8)) {
		cb->unk4 = cb->unk8;
	}

	cb->mBytesToWrite -= size;
	cb->mBytesToRead += size;
	MWExitCriticalSection(&cb->mSection);
	return 0;
}

int CircleBufferReadBytes(CircleBuffer* cb, u8* buf, u32 size)
{
	int availSize;

	if (size > cb->mBytesToRead) {
		return -1;
	}
	MWEnterCriticalSection(&cb->mSection);
	availSize = cb->unkC - (cb->unk0 - cb->unk8);
	if (size < availSize) {
		memcpy(buf, cb->unk0, size);
		cb->unk0 += size;
	} else {
		memcpy(buf, cb->unk0, availSize);
		memcpy(buf + availSize, cb->unk8, size - availSize);
		cb->unk0 = cb->unk8 + size - availSize;
	}

	if (cb->unkC == (cb->unk0 - cb->unk8)) {
		cb->unk0 = cb->unk8;
	}

	cb->mBytesToWrite += size;
	cb->mBytesToRead -= size;
	MWExitCriticalSection(&cb->mSection);
	return 0;
}
