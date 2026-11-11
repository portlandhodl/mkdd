#include "types.h"
#include "PowerPC_EABI_Support/MetroTRK/trk.h"
#include "string.h"

#define DATA(p) ((u8*)(p) + 0x10)

static const char _s_optmsg[] __attribute__((section(".rodata"))) = "\nMetroTRK Option : SerialIO - ";
static const char _s_enable[] __attribute__((section(".rodata"))) = "Enable\n";
static const char _s_disable[] __attribute__((section(".rodata"))) = "Disable\n";

typedef struct MessageBuffer {
	u32 _00;
	BOOL isInUse;
	u32 length;
	u32 position;
	u8 data[0x880];
} MessageBuffer;

typedef enum {
	DS_NoError                  = 0x0,
	DS_StepError                = 0x1,
	DS_ParameterError           = 0x2,
	DS_NoMessageBufferAvailable = 0x300,
	DS_MessageBufferOverflow    = 0x301,
	DS_MessageBufferReadError   = 0x302,
	DS_InvalidMemory            = 0x700,
	DS_InvalidRegister          = 0x701,
	DS_CWDSException            = 0x702,
	DS_UnsupportedError         = 0x703,
	DS_InvalidProcessID         = 0x704,
	DS_InvalidThreadID          = 0x705,
	DS_OSError                  = 0x706,
} DSError;

typedef enum {
	DSMSG_Ping        = 0x0,
	DSMSG_Connect     = 0x1,
	DSMSG_Disconnect  = 0x2,
	DSMSG_Reset       = 0x3,
	DSMSG_Versions    = 0x4,
	DSMSG_SupportMask = 0x5,
	DSMSG_Override    = 0x7,
	DSMSG_ReadMemory     = 0x10,
	DSMSG_WriteMemory    = 0x11,
	DSMSG_ReadRegisters  = 0x12,
	DSMSG_WriteRegisters = 0x13,
	DSMSG_SetOption      = 0x17,
	DSMSG_Continue       = 0x18,
	DSMSG_Step           = 0x19,
	DSMSG_Stop           = 0x1A,
	DSMSG_ReplyACK = 0x80,
} MessageCommandID;

typedef enum {
	DSREG_Default   = 0,
	DSREG_FP        = 1,
	DSREG_Extended1 = 2,
	DSREG_Extended2 = 3,
} DSMessageRegisterOptions;

typedef enum {
	DSSTEP_IntoCount = 0x0,
	DSSTEP_IntoRange = 0x1,
	DSSTEP_OverCount = 0x10,
	DSSTEP_OverRange = 0x11,
} DSMessageStepOptions;

typedef enum {
	DSREPLY_NoError         = 0x0,
	DSREPLY_Error           = 0x1,
	DSREPLY_PacketSizeError = 0x2,
	DSREPLY_CWDSError       = 0x3,
	DSREPLY_UnsupportedCommandError = 0x10,
	DSREPLY_ParameterError          = 0x11,
	DSREPLY_UnsupportedOptionError  = 0x12,
	DSREPLY_InvalidMemoryRange      = 0x13,
	DSREPLY_InvalidRegisterRange    = 0x14,
	DSREPLY_CWDSException           = 0x15,
	DSREPLY_NotStopped              = 0x16,
	DSREPLY_OSError            = 0x20,
	DSREPLY_InvalidProcessID   = 0x21,
	DSREPLY_InvalidThreadID    = 0x22,
} DSReplyError;

enum {
	DSMSGMEMORY_Segmented     = 0x01,
	DSMSGMEMORY_Extended      = 0x02,
	DSMSGMEMORY_Protected     = 0x04,
	DSMSGMEMORY_Userview      = 0x08,
	DSMSGMEMORY_Space_program = 0x00,
	DSMSGMEMORY_Space_data    = 0x40,
	DSMSGMEMORY_Space_io      = 0x80
};

typedef struct CommandReply {
	u32 _00;
	union {
		u8 b;
		MessageCommandID m;
	} commandID;
	union {
		u8 b;
		DSReplyError r;
	} replyError;
	u32 _0C;
	u8 _10[0x30];
} CommandReply;

extern BOOL usr_puts_serial(const char* msg);
void SetUseSerialIO(u8);
TRKResult TRKMessageSend(TRK_Msg*);
TRKResult TRKReadBuffer(MessageBuffer* msg, void* data, size_t length);
TRKResult TRKAppendBuffer(MessageBuffer* msg, const void* data, size_t length);
void TRKResetBuffer(MessageBuffer* msg, BOOL keepData);
TRKResult TRKAppendBuffer_ui8(TRKBuffer* buffer, u8* data, int count);
TRKResult TRKSetBufferPosition(TRKBuffer* msg, u32 pos);
TRKResult TRKStandardACK(TRKBuffer*, MessageCommandID, DSReplyError);
TRKResult TRKSendACK(MessageBuffer*);

static BOOL IsTRKConnected __attribute__((section(".data")));
static u32 g_CurrentSequence __attribute__((section(".data")));

void OutputData(void* data, int length)
{
	int i;
	u8* datapointer = data;

	for (i = 0; i < length; i++) {
		MWTRACE(8, "%02x ", datapointer[i]);
		if (i % 16 == 15) {
			MWTRACE(8, "\n");
		}
	}

	MWTRACE(8, "\n");
}
int GetTRKConnected(void)
{
	return IsTRKConnected;
}
void SetTRKConnected(int isConnected)
{
	IsTRKConnected = isConnected;
}
void TRKMessageIntoReply(void)
{
}
TRKResult TRKSendACK(MessageBuffer* buffer)
{
	TRKResult err;
	MWTRACE(1, "SendACK : Calling MessageSend\n");
	err = TRKMessageSend((TRK_Msg*)buffer);
	MWTRACE(1, "MessageSend err : %ld\n", err);
	return err;
}
TRKResult TRKStandardACK(TRKBuffer* buffer, MessageCommandID commandID, DSReplyError replyError)
{
	CommandReply reply;

	memset(&reply, 0, sizeof(CommandReply));
	reply.commandID.b  = commandID;
	reply._00          = 0x40;
	reply.replyError.b = replyError;
	TRKWriteUARTN(&reply, sizeof(CommandReply));
	return DS_NoError;
}
void TRKDoError(void)
{
}
void TRKDoUnsupported(void)
{
}
u32 TRKDoConnect(TRKBuffer* buffer)
{
	IsTRKConnected = TRUE;
	return TRKStandardACK(buffer, 0x80, DSREPLY_NoError);
}
u32 TRKDoDisconnect(TRKBuffer* buffer)
{
	TRKEvent event;

	IsTRKConnected = FALSE;
	TRKStandardACK(buffer, 0x80, DSREPLY_NoError);
	TRKConstructEvent(&event, 1);
	TRKPostEvent(&event);
	return DS_NoError;
}
u32 TRKDoReset(TRKBuffer* buffer)
{
	TRKStandardACK(buffer, 0x80, DSREPLY_NoError);
	__TRK_reset();
	return DS_NoError;
}
u32 TRKDoOverride(TRKBuffer* buffer)
{
	TRKStandardACK(buffer, 0x80, DSREPLY_NoError);
	__TRK_copy_vectors();
	return DS_NoError;
}
u32 TRKDoVersions(TRKBuffer*)
{
	return DS_NoError;
}
u32 TRKDoSupportMask(TRKBuffer*)
{
	return DS_NoError;
}
void TRKDoCPUType(void)
{
}
u32 TRKDoReadMemory(TRKBuffer* buffer)
{
	u8 buf[0x800] ;
	size_t tempLength;
	int result;
	int replyErr;
	int options;
	size_t length;
	u32 start;

	start   = *(u32*)(DATA(buffer) + 16);
	length  = *(u16*)(DATA(buffer) + 12);
	options = DATA(buffer)[8];

	MWTRACE(1, "ReadMemory (0x%02x) : 0x%08x 0x%08x 0x%08x\n", DATA(buffer)[4], start, length, options);

	if (options & DSMSGMEMORY_Extended) {
		return TRKStandardACK(buffer, DSMSG_ReplyACK, DSREPLY_UnsupportedOptionError);
	}

	tempLength = length;

	result = TRKTargetAccessMemory(buf, start, &tempLength, options & DSMSGMEMORY_Userview ? 0 : 1, TRUE);

	TRKResetBuffer((MessageBuffer*)buffer, 0);

	if (result == DS_NoError) {
		CommandReply reply;
		memset(&reply, 0, sizeof(CommandReply));
		reply.replyError.b = result;
		reply._00          = tempLength + 0x40;
		reply.commandID.b  = DSMSG_ReplyACK;
		TRKAppendBuffer((MessageBuffer*)buffer, &reply, sizeof(CommandReply));

		result = TRKAppendBuffer((MessageBuffer*)buffer, buf, tempLength);
	}

	if (result) {
		switch (result) {
		case DS_CWDSException:
			replyErr = DSREPLY_CWDSException;
			break;
		case DS_InvalidMemory:
			replyErr = DSREPLY_InvalidMemoryRange;
			break;
		case DS_InvalidProcessID:
			replyErr = DSREPLY_InvalidProcessID;
			break;
		case DS_InvalidThreadID:
			replyErr = DSREPLY_InvalidThreadID;
			break;
		case DS_OSError:
			replyErr = DSREPLY_OSError;
			break;
		default:
			replyErr = DSREPLY_CWDSError;
			break;
		}
		return TRKStandardACK(buffer, DSMSG_ReplyACK, replyErr);
	}

	return TRKSendACK((MessageBuffer*)buffer);
}
u32 TRKDoWriteMemory(TRKBuffer* buffer)
{
	u8 buf[0x800] ;
	size_t tempLength;
	int options;
	int result;
	int replyErr;
	size_t length;
	u32 start;

	start   = *(u32*)(DATA(buffer) + 16);
	length  = *(u16*)(DATA(buffer) + 12);
	options = DATA(buffer)[8];

	MWTRACE(1, "WriteMemory (0x%02x) : 0x%08x 0x%08x 0x%08x\n", (uint)DATA(buffer)[0x4], start, length, options);

	if (options & DSMSGMEMORY_Extended) {
		return TRKStandardACK(buffer, DSMSG_ReplyACK, DSREPLY_UnsupportedOptionError);
	}

	tempLength = length;

	TRKSetBufferPosition(buffer, DSMSGMEMORY_Space_data);
	TRKReadBuffer((MessageBuffer*)buffer, buf, tempLength);
	result = TRKTargetAccessMemory(buf, start, &tempLength, options & DSMSGMEMORY_Userview ? 0 : 1, FALSE);

	TRKResetBuffer((MessageBuffer*)buffer, 0);

	if (result == DS_NoError) {
		CommandReply reply;
		memset(&reply, 0, sizeof(CommandReply));
		reply._00          = 0x40;
		reply.commandID.b  = DSMSG_ReplyACK;
		reply.replyError.b = result;
		result             = TRKAppendBuffer((MessageBuffer*)buffer, &reply, sizeof(CommandReply));
	}

	if (result != DS_NoError) {
		switch (result) {
		case DS_CWDSException:
			replyErr = DSREPLY_CWDSException;
			break;
		case DS_InvalidMemory:
			replyErr = DSREPLY_InvalidMemoryRange;
			break;
		case DS_InvalidProcessID:
			replyErr = DSREPLY_InvalidProcessID;
			break;
		case DS_InvalidThreadID:
			replyErr = DSREPLY_InvalidThreadID;
			break;
		case DS_OSError:
			replyErr = DSREPLY_OSError;
			break;
		default:
			replyErr = DSREPLY_CWDSError;
			break;
		}
		return TRKStandardACK(buffer, DSMSG_ReplyACK, replyErr);
	}

	return TRKSendACK((MessageBuffer*)buffer);
}
u32 TRKDoReadRegisters(TRKBuffer* buffer)
{
	int error;
	u8 options;
	u16 firstRegister;
	u16 lastRegister;
	size_t registersLength;
	CommandReply local_50;

	options       = DATA(buffer)[8];
	firstRegister = *(u16*)(DATA(buffer) + 12);
	lastRegister  = *(u16*)(DATA(buffer) + 16);

	if (firstRegister > lastRegister) {
		return TRKStandardACK(buffer, DSMSG_ReplyACK, DSREPLY_InvalidRegisterRange);
	}

	local_50.commandID.b = DSMSG_ReplyACK;
	local_50._00         = 0x468;

	TRKResetBuffer((MessageBuffer*)buffer, 0);
	MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", ((MessageBuffer*)buffer)->length);

	TRKAppendBuffer_ui8(buffer, (u8*)&local_50, sizeof(CommandReply));
	MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", ((MessageBuffer*)buffer)->length);

	error = TRKTargetAccessDefault(0, 36, buffer, &registersLength, TRUE);
	MWTRACE(4, "DoReadRegisters : Error reading  default regs 0x%08x\n", error);
	MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", ((MessageBuffer*)buffer)->length);

	if (error == DS_NoError) {
		error = TRKTargetAccessFP(0, 33, buffer, &registersLength, TRUE);
	}
	MWTRACE(4, "DoReadRegisters : Error FP regs 0x%08x\n", error);
	MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", ((MessageBuffer*)buffer)->length);
	if (error == DS_NoError) {
		error = TRKTargetAccessExtended1(0, 0x60, buffer, &registersLength, TRUE);
	}
	MWTRACE(4, "DoReadRegisters : Error extended1 regs 0x%08x\n", error);
	MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", ((MessageBuffer*)buffer)->length);
	if (error == DS_NoError) {
		error = TRKTargetAccessExtended2(0, 31, buffer, &registersLength, TRUE);
	}
	MWTRACE(4, "DoReadRegisters : Error extended2 regs 0x%08x\n", error);
	MWTRACE(4, "DoReadRegisters : Buffer length 0x%08x\n", ((MessageBuffer*)buffer)->length);

	if (error != DS_NoError) {
		int replyError;
		switch (error) {
		case DS_UnsupportedError:
			replyError = DSREPLY_UnsupportedOptionError;
			break;
		case DS_InvalidRegister:
			replyError = DSREPLY_InvalidRegisterRange;
			break;
		case DS_CWDSException:
			replyError = DSREPLY_CWDSException;
			break;
		case DS_InvalidProcessID:
			replyError = DSREPLY_InvalidProcessID;
			break;
		case DS_InvalidThreadID:
			replyError = DSREPLY_InvalidThreadID;
			break;
		case DS_OSError:
			replyError = DSREPLY_OSError;
			break;
		default:
			replyError = DSREPLY_CWDSError;
		}

		return TRKStandardACK(buffer, DSMSG_ReplyACK, replyError);
	} else {
		return TRKSendACK((MessageBuffer*)buffer);
	}
}
u32 TRKDoWriteRegisters(TRKBuffer* buffer)
{
	int error;
	int replyError;
	u8 options;
	u16 firstRegister;
	u16 lastRegister;
	size_t registersLength;

	options       = DATA(buffer)[8];
	firstRegister = *(u16*)(DATA(buffer) + 12);
	lastRegister  = *(u16*)(DATA(buffer) + 16);

	TRKSetBufferPosition(buffer, 0);

	if (firstRegister > lastRegister) {
		return TRKStandardACK(buffer, DSMSG_ReplyACK, DSREPLY_InvalidRegisterRange);
	}

	TRKSetBufferPosition(buffer, 0x40);

	switch (options) {
	case DSREG_Default:
		error = TRKTargetAccessDefault(firstRegister, lastRegister, buffer, &registersLength, FALSE);
		break;
	case DSREG_FP:
		error = TRKTargetAccessFP(firstRegister, lastRegister, buffer, &registersLength, FALSE);
		break;
	case DSREG_Extended1:
		error = TRKTargetAccessExtended1(firstRegister, lastRegister, buffer, &registersLength, FALSE);
		break;
	case DSREG_Extended2:
		error = TRKTargetAccessExtended2(firstRegister, lastRegister, buffer, &registersLength, FALSE);
		break;
	default:
		error = DS_UnsupportedError;
		break;
	}

	TRKResetBuffer((MessageBuffer*)buffer, 0);

	if (error == DS_NoError) {
		CommandReply local_50;
		memset(&local_50, 0, sizeof(CommandReply));
		local_50._00          = 0x40;
		local_50.commandID.b  = DSMSG_ReplyACK;
		local_50.replyError.b = error;
		error                 = TRKAppendBuffer((MessageBuffer*)buffer, (u8*)&local_50, sizeof(CommandReply));
	}

	if (error != DS_NoError) {
		switch (error) {
		case DS_UnsupportedError:
			replyError = DSREPLY_UnsupportedOptionError;
			break;
		case DS_InvalidRegister:
			replyError = DSREPLY_InvalidRegisterRange;
			break;
		case DS_MessageBufferReadError:
			replyError = DSREPLY_PacketSizeError;
			break;
		case DS_CWDSException:
			replyError = DSREPLY_CWDSException;
			break;
		case DS_InvalidProcessID:
			replyError = DSREPLY_InvalidProcessID;
			break;
		case DS_InvalidThreadID:
			replyError = DSREPLY_InvalidThreadID;
			break;
		case DS_OSError:
			replyError = DSREPLY_OSError;
			break;
		default:
			replyError = DSREPLY_CWDSError;
		}

		return TRKStandardACK(buffer, DSMSG_ReplyACK, replyError);
	} else {
		return TRKSendACK((MessageBuffer*)buffer);
	}
}
void TRKDoFlushCache(void)
{
	MWTRACE(1, "DoFlushCache unimplemented!!!\n");
}
u32 TRKDoContinue(TRKBuffer*)
{
	MWTRACE(1, "DoContinue\n");
	if (!TRKTargetStopped()) {
		u8 arr[0x40];
		memset(arr, 0, 0x40);

		arr[4]     = 0x80;
		*(u32*)arr = 0x40;
		arr[8]     = 0x16;

		TRKWriteUARTN(arr, 0x40);
		return DS_NoError;
	} else {
		u8 arr[0x40];
		memset(arr, 0, 0x40);

		arr[4]     = 0x80;
		*(u32*)arr = 0x40;
		arr[8]     = 0x00;

		TRKWriteUARTN(arr, 0x40);
		return TRKTargetContinue();
	}
}
u32 TRKDoStep(TRKBuffer* buffer)
{
	MessageBuffer* b = (MessageBuffer*)buffer;
	TRKResult result;
	u8 options;
	u8 count;
	u32 rangeStart;
	u32 rangeEnd;
	u32 pc;
	TRKSetBufferPosition(buffer, 0);

	options    = *(u8*)&b->data[8];
	rangeStart = *(u32*)&b->data[16];
	rangeEnd   = *(u32*)&b->data[20];

	switch (options) {
	case DSSTEP_IntoCount:
	case DSSTEP_OverCount:
		count = b->data[12];
		if (count >= 1) {
			break;
		}
		return TRKStandardACK(buffer, DSMSG_ReplyACK, DSREPLY_ParameterError);
	case DSSTEP_IntoRange:
	case DSSTEP_OverRange:
		pc = TRKTargetGetPC();
		if (pc >= rangeStart && pc <= rangeEnd) {
			break;
		}
		return TRKStandardACK(buffer, DSMSG_ReplyACK, DSREPLY_ParameterError);
	default:
		return TRKStandardACK(buffer, DSMSG_ReplyACK, DSREPLY_UnsupportedOptionError);
	}

	if (!TRKTargetStopped()) {
		return TRKStandardACK(buffer, DSMSG_ReplyACK, DSREPLY_NotStopped);
	} else {
		result = TRKStandardACK(buffer, DSMSG_ReplyACK, DSREPLY_NoError);
		switch (options) {
		case DSSTEP_IntoCount:
		case DSSTEP_OverCount:
			result = TRKTargetSingleStep(count, (options == DSSTEP_OverCount));
			break;
		case DSSTEP_IntoRange:
		case DSSTEP_OverRange:
			result = TRKTargetStepOutOfRange(rangeStart, rangeEnd, (options == DSSTEP_OverRange));
			break;
		}

		return result;
	}
}
u32 TRKDoStop(TRKBuffer* buffer)
{
	MessageCommandID c;

	switch (TRKTargetStop()) {
	case DS_NoError:
		c = DSMSG_Ping;
		break;
	case DS_InvalidProcessID:
		c = '!';
		break;
	case DS_InvalidThreadID:
		c = '\"';
		break;
	case DS_OSError:
		c = ' ';
		break;
	default:
		c = DSMSG_Connect;
		break;
	}

	TRKStandardACK(buffer, DSMSG_ReplyACK, c);

	return DS_NoError;
}
u32 TRKDoSetOption(TRKBuffer* buffer)
{
	u8 enable = DATA(buffer)[0xc];

	if (DATA(buffer)[0x8] == '\1') {
		usr_puts_serial((char*)_s_optmsg);
		if (enable) {
			usr_puts_serial((char*)_s_enable);
		} else {
			usr_puts_serial((char*)_s_disable);
		}
		SetUseSerialIO(enable);
	}

	TRKStandardACK(buffer, DSMSG_ReplyACK, DS_NoError);

	return 0;
}
