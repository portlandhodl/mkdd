#include "types.h"
#include "PowerPC_EABI_Support/MetroTRK/trk.h"

typedef struct StopInfo_PPC {
	u32 PC;
	u32 PCInstruction;
	u16 exceptionID;
} StopInfo_PPC;

typedef struct TRKExceptionStatus {
	StopInfo_PPC exceptionInfo;
	u8 inTRK;
	u8 exceptionDetected;
} TRKExceptionStatus;

typedef struct TRKStepStatus {
	BOOL active;
	int type;
	u32 count;
	u32 rangeStart;
	u32 rangeEnd;
} TRKStepStatus;

typedef struct Default_PPC {
	u32 GPR[32];
	u32 PC;
	u32 LR;
	u32 CR;
	u32 CTR;
	u32 XER;
} Default_PPC;

typedef struct Float_PPC {
	u64 FPR[32];
	u64 FPSCR;
	u64 FPECR;
} Float_PPC;

typedef struct Extended1_PPC {
	u32 SR[16];
	u32 TBL;
	u32 TBU;
	u32 HID0_;
	u32 HID1;
	u32 MSR;
	u32 PVR;
	u32 IBAT0U;
	u32 IBAT0L;
	u32 IBAT1U;
	u32 IBAT1L;
	u32 IBAT2U;
	u32 IBAT2L;
	u32 IBAT3U;
	u32 IBAT3L;
	u32 DBAT0U;
	u32 DBAT0L;
	u32 DBAT1U;
	u32 DBAT1L;
	u32 DBAT2U;
	u32 DBAT2L;
	u32 DBAT3U_;
	u32 DBAT3L_;
	u32 DMISS;
	u32 DCMP;
	u32 HASH1;
	u32 HASH2;
	u32 IMISS;
	u32 ICMP;
	u32 RPA;
	u32 SDR1;
	u32 DAR;
	u32 DSISR;
	u32 SPRG0;
	u32 SPRG1;
	u32 SPRG2;
	u32 SPRG3;
	u32 DEC;
	u32 IABR;
	u32 EAR;
	u32 DABR;
	u32 PMC1;
	u32 PMC2;
	u32 PMC3;
	u32 PMC4;
	u32 SIA;
	u32 MMCR0;
	u32 MMCR1;
	u32 THRM1;
	u32 THRM2;
	u32 THRM3;
	u32 ICTC;
	u32 L2CR;
	u32 UMMCR2;
	u32 UBAMR;
	u32 UMMCR0;
	u32 UPMC1;
	u32 UPMC2;
	u32 USIA;
	u32 UMMCR1;
	u32 UPMC3;
	u32 UPMC4;
	u32 USDA;
	u32 MMCR2;
	u32 BAMR;
	u32 SDA;
	u32 MSSCR0;
	u32 MSSCR1;
	u32 PIR;
	u32 exceptionID;
	u32 GQR[8];
	u32 HID_G;
	u32 WPAR;
	u32 DMA_U_;
	u32 DMA_L_;
} Extended1_PPC;

typedef struct Extended2_PPC {
	u32 PSR[32][2];
} Extended2_PPC;

typedef struct ProcessorState_PPC {
	Default_PPC Default;
	Float_PPC Float;
	Extended1_PPC Extended1;
	Extended2_PPC Extended2;
	u32 transport_handler_saved_ra;
} ProcessorState_PPC;

typedef struct ProcessorRestoreFlags_PPC {
	u8 TBR;
	u8 DEC;
	u8 padding[7];
} ProcessorRestoreFlags_PPC;

typedef struct memRange {
	u8* start;
	u8* end;
	BOOL readable;
	BOOL writeable;
} memRange;

typedef struct CommandReply {
	u32 _00;
	u8 commandID;
	u8 _05[3];
	u32 replyError;
	u32 _0C;
	u8 _10[0x30];
} CommandReply;

typedef struct { u32 words[4]; } u128;
typedef struct DSVersions {
	u8 kernelMajor;
	u8 kernelMinor;
	u8 protocolMajor;
	u8 protocolMinor;
} DSVersions;

typedef struct DSCPUType {
	u8 cpuMajor;
	u8 cpuMinor;
	u8 bigEndian;
	u8 defaultTypeSize;
	u8 fpTypeSize;
	u8 extended1TypeSize;
	u8 extended2TypeSize;
} DSCPUType;

extern BOOL gTRKBigEndian;
extern u8 TRKTargetCPUMinorType(void);
typedef void (*RegAccessFunc)(void* srcDestPtr, u128 val);
typedef int DSIOResult;

#define SPR_HID2   920
#define SPR_GQR0   912
#define SPR_FPECR  1022

#define DS_NoError          0
#define DS_InvalidMemory    0x700
#define DS_InvalidRegister  0x701
#define DS_CWDSException    0x702
#define DS_UnsupportedError 0x703
#define DS_IONoError        0
#define DS_IOError          1

#define VALIDMEM_Readable  0
#define VALIDMEM_Writeable 1

#define DSSTEP_IntoCount  0x0
#define DSSTEP_IntoRange  0x1
#define DSSTEP_OverCount  0x10

#define NUBEVENT_Breakpoint 3
#define NUBEVENT_Exception  4
#define NUBEVENT_Support    5

#define DSMSG_WriteFile    0xD0
#define DSMSG_ReadFile     0xD1
#define DSMSG_OpenFile     0xD2
#define DSMSG_CloseFile    0xD3
#define DSMSG_PositionFile 0xD4
#define DSMSG_NotifyStopped 0x90

#define INSTR_NOP  0x60000000
#define INSTR_BLR  0x4E800020
#define INSTR_PSQ_ST(psr, offset, rDest, w, gqr) (0xF0000000 | (psr << 21) | (rDest << 16) | (w << 15) | (gqr << 12) | offset)
#define INSTR_PSQ_L(psr, offset, rSrc, w, gqr)   (0xE0000000 | (psr << 21) | (rSrc << 16) | (w << 15) | (gqr << 12) | offset)
#define INSTR_STW(rSrc, offset, rDest)           (0x90000000 | (rSrc << 21) | (rDest << 16) | offset)
#define INSTR_LWZ(rDest, offset, rSrc)           (0x80000000 | (rDest << 21) | (rSrc << 16) | offset)
#define INSTR_STFD(fprSrc, offset, rDest)        (0xD8000000 | (fprSrc << 21) | (rDest << 16) | offset)
#define INSTR_LFD(fprDest, offset, rSrc)         (0xC8000000 | (fprDest << 21) | (rSrc << 16) | offset)
#define INSTR_MFSPR(rDest, spr)                  (0x7C000000 | (rDest << 21) | ((spr & 0xFE0) << 6) | ((spr & 0x1F) << 16) | 0x2A6)
#define INSTR_MTSPR(spr, rSrc)                   (0x7C000000 | (rSrc << 21) | ((spr & 0xFE0) << 6) | ((spr & 0x1F) << 16) | 0x3A6)

#define DSFetch_u32(_p_) (*((u32*)_p_))
#define DSFetch_u64(_p_) (*((u64*)_p_))

extern void* memset(void* dst, int val, size_t n);
extern void TRK_flush_cache(u32 addr, int len);
extern TRKResult TRKAppendBuffer_ui32(TRKBuffer*, const u32*, int);
extern TRKResult TRKReadBuffer_ui32(TRKBuffer*, u32*, int);
extern TRKResult TRKAppendBuffer1_ui64(TRKBuffer*, const u64);
extern TRKResult TRKReadBuffer1_ui64(TRKBuffer*, u64*);
extern TRKResult TRKAppendBuffer_ui8(TRKBuffer*, u8*, int);
extern void TRKConstructEvent(TRKEvent*, int);
extern TRKResult TRKPostEvent(TRKEvent*);
extern TRKResult TRKDoNotifyStopped(int cmd);
extern u32 TRKTargetTranslate(u32 addr);
extern TRKResult TRKSuppAccessFile(u32 file_handle, u8* data, size_t* count, DSIOResult* io_result, BOOL need_reply, BOOL read);
extern TRKResult HandleOpenFileSupportRequest(const char* path, u8 replyError, u32* param_3, DSIOResult* ioResult);
extern TRKResult HandleCloseFileSupportRequest(u32 handle, DSIOResult* ioResult);
extern TRKResult HandlePositionFileSupportRequest(u32 handle, u32* position, u8 whence, DSIOResult* ioResult);
extern int TRK__read_aram(u32, u32, u32*);
extern int TRK__write_aram(u32, u32, u32*);
extern void TRKSaveExtended1Block(void);
extern void TRKRestoreExtended1Block(void);
extern void TRKUARTInterruptHandler(void);
extern void TRKInterruptHandlerEnableInterrupts(void);
extern void TRKExceptionHandler(u16);
extern void TRKPostInterruptEvent(void);

extern void __TRK_set_MSR(u32 msr);
extern u32 __TRK_get_MSR(void);

ProcessorRestoreFlags_PPC gTRKRestoreFlags = { FALSE, FALSE };
static TRKExceptionStatus gTRKExceptionStatus = { { 0, 0, 0 }, TRUE, 0 };
static TRKStepStatus gTRKStepStatus = { FALSE, DSSTEP_IntoCount, 0, 0, 0 };

static __declspec(section ".data") u16 TRK_saved_exceptionID = 0;
TRKState gTRKState;
ProcessorState_PPC gTRKCPUState;
Default_PPC gTRKSaveState;
u128 TRKvalue128_temp;

const memRange gTRKMemMap[1] = { { (u8*)0, (u8*)-1, TRUE, TRUE } };

void __TRK_get_PVR(void);
void __TRK_get_IBAT0U(void);
void __TRK_get_IBAT0L(void);
void __TRK_get_IBAT1U(void);
void __TRK_get_IBAT1L(void);
void __TRK_get_IBAT2U(void);
void __TRK_get_IBAT2L(void);
void __TRK_get_IBAT3U(void);
void __TRK_get_IBAT3L(void);
void __TRK_get_DBAT0U(void);
void __TRK_get_DBAT0L(void);
void __TRK_get_DBAT1U(void);
void __TRK_get_DBAT1L(void);
void __TRK_get_DBAT2U(void);
void __TRK_get_DBAT2L(void);
void __TRK_get_DBAT3U(void);
void __TRK_get_DBAT3L(void);

static void TRK_ppc_memcpy(register void* dest, register const void* src, register int n, register u32 param_4, register u32 param_5);

void TRKExceptionHandler(u16);
static void GetThreadInfo(int*, int*);
static TRKResult TRKPPCAccessSPR(void* srcDestPtr, u32 spr, BOOL read);
static TRKResult TRKPPCAccessPairedSingleRegister(void* srcDestPtr, u32 psr, BOOL read);
static TRKResult TRKPPCAccessSpecialReg(void* srcDestPtr, u32* instructionData, BOOL read);
static TRKResult TRKPPCAccessFPRegister(void* srcDestPtr, u32 fpr, BOOL read);
void WriteFPSCR(f64*);
void ReadFPSCR(f64*);

TRKResult TRKValidMemory32(const void* addr, size_t length, int readWriteable)
{
	int err = DS_InvalidMemory;

	const u8* start;
	const u8* end;

	s32 i;

	start = (const u8*)addr;
	end   = ((const u8*)addr + (length - 1));

	if (end < start)
		return DS_InvalidMemory;

	for (i = 0; (i < (s32)(sizeof(gTRKMemMap) / sizeof(memRange))); i++) {
		if ((start <= (const u8*)gTRKMemMap[i].end) && (end >= (const u8*)gTRKMemMap[i].start)) {
			if (((readWriteable == VALIDMEM_Readable) && !gTRKMemMap[i].readable)
			    || ((readWriteable == VALIDMEM_Writeable) && !gTRKMemMap[i].writeable)) {
				err = DS_InvalidMemory;
			} else {
				err = DS_NoError;

				if (start < (const u8*)gTRKMemMap[i].start)
					err = TRKValidMemory32(start, (u32)((const u8*)gTRKMemMap[i].start - start), readWriteable);

				if ((err == DS_NoError) && (end > (const u8*)gTRKMemMap[i].end))
					err = TRKValidMemory32((const u8*)gTRKMemMap[i].end, (u32)(end - (const u8*)gTRKMemMap[i].end), readWriteable);
			}

			break;
		}
	}

	return err;
}

TRKResult TRKTargetAccessMemory(void* data, u32 start, size_t* length, int accessOptions, BOOL read)
{
	TRKResult error;
	u32 uVar5;
	void* addr;
	u32 param4;
	TRKExceptionStatus tempExceptionStatus = gTRKExceptionStatus;
	gTRKExceptionStatus.exceptionDetected       = FALSE;

	addr  = (void*)TRKTargetTranslate(start);
	error = TRKValidMemory32(addr, *length, read == FALSE);

	if (error != DS_NoError) {
		*length = 0;
	} else {
		uVar5  = __TRK_get_MSR();
		param4 = uVar5 | (gTRKCPUState.Extended1.MSR & 0x10);

		if (read) {
			TRK_ppc_memcpy(data, addr, *length, uVar5, param4);
		} else {
			TRK_ppc_memcpy(addr, data, *length, param4, uVar5);
			TRK_flush_cache((u32)addr, *length);
			if ((void*)start != addr) {
				TRK_flush_cache(start, *length);
			}
		}
	}

	if (gTRKExceptionStatus.exceptionDetected) {
		*length = 0;
		error   = DS_CWDSException;
	}

	gTRKExceptionStatus = tempExceptionStatus;
	return error;
}

TRKResult TRKTargetReadInstruction(void* data, u32 start)
{
	TRKResult error;
	size_t registersLength = 4;

	error = TRKTargetAccessMemory(data, start, &registersLength, 0, TRUE);

	if (error == DS_NoError && registersLength != 4) {
		error = DS_InvalidMemory;
	}

	return error;
}

TRKResult TRKTargetAccessDefault(u32 firstRegister, u32 lastRegister, TRKBuffer* b, size_t* registersLengthPtr, BOOL read)
{
	TRKResult error;
	u32 count;
	u32* data;
	TRKExceptionStatus tempExceptionStatus;

	if (lastRegister > 0x24) {
		return DS_InvalidRegister;
	}

	tempExceptionStatus                   = gTRKExceptionStatus;
	gTRKExceptionStatus.exceptionDetected = FALSE;

	data                = gTRKCPUState.Default.GPR + firstRegister;
	count               = (lastRegister - firstRegister) + 1;
	*registersLengthPtr = count * sizeof(u32);

	if (read) {
		error = TRKAppendBuffer_ui32(b, data, count);
	} else {
		error = TRKReadBuffer_ui32(b, data, count);
	}

	if (gTRKExceptionStatus.exceptionDetected) {
		*registersLengthPtr = 0;
		error               = DS_CWDSException;
	}

	gTRKExceptionStatus = tempExceptionStatus;
	return error;
}

TRKResult TRKTargetAccessFP(u32 firstRegister, u32 lastRegister, TRKBuffer* b, size_t* registersLengthPtr, BOOL read)
{
	u64 temp;
	TRKResult error;
	TRKExceptionStatus tempExceptionStatus;
	u32 current;

	if (lastRegister > 0x21) {
		return DS_InvalidRegister;
	}

	tempExceptionStatus                   = gTRKExceptionStatus;
	gTRKExceptionStatus.exceptionDetected = FALSE;

	__TRK_set_MSR(__TRK_get_MSR() | 0x2000);

	*registersLengthPtr = 0;
	error               = DS_NoError;

	for (current = firstRegister; (current <= lastRegister) && (error == DS_NoError); current++, *registersLengthPtr += sizeof(f64)) {
		if (read) {
			TRKPPCAccessFPRegister(&temp, current, read);
			error = TRKAppendBuffer1_ui64(b, temp);
		} else {
			TRKReadBuffer1_ui64(b, &temp);
			error = TRKPPCAccessFPRegister(&temp, current, read);
		}
	}

	if (gTRKExceptionStatus.exceptionDetected) {
		*registersLengthPtr = 0;
		error               = DS_CWDSException;
	}

	gTRKExceptionStatus = tempExceptionStatus;
	return error;
}

TRKResult TRKTargetAccessExtended1(u32 firstRegister, u32 lastRegister, TRKBuffer* b, size_t* registersLengthPtr, BOOL read)
{
	TRKExceptionStatus tempExceptionStatus;
	int error;
	u32* data;
	int count;

	if (lastRegister > 0x60) {
		return DS_InvalidRegister;
	}

	tempExceptionStatus                   = gTRKExceptionStatus;
	gTRKExceptionStatus.exceptionDetected = FALSE;

	*registersLengthPtr = 0;

	if (firstRegister <= lastRegister) {
		data  = (u32*)&gTRKCPUState.Extended1 + firstRegister;
		count = lastRegister - firstRegister + 1;
		*registersLengthPtr += count * sizeof(u32);

		if (read) {
			error = TRKAppendBuffer_ui32(b, data, count);
		} else {
			if (data <= &gTRKCPUState.Extended1.TBU && (data + count - 1) >= &gTRKCPUState.Extended1.TBL) {
				gTRKRestoreFlags.TBR = 1;
			}

			if (data <= &gTRKCPUState.Extended1.DEC && (data + count - 1) >= &gTRKCPUState.Extended1.DEC) {
				gTRKRestoreFlags.DEC = 1;
			}
			error = TRKReadBuffer_ui32(b, data, count);
		}
	}
	if (gTRKExceptionStatus.exceptionDetected) {
		*registersLengthPtr = 0;
		error               = DS_CWDSException;
	}

	gTRKExceptionStatus = tempExceptionStatus;
	return error;
}

TRKResult TRKTargetAccessExtended2(u32 firstRegister, u32 lastRegister, TRKBuffer* b, size_t* registerStorageSize, BOOL read)
{
	TRKExceptionStatus savedException;
	u32 i;
	u32 value_buf0[1];
	u32 value_buf[2];
	TRKResult err;

	if (lastRegister > 0x1f)
		return DS_InvalidRegister;

	savedException                        = gTRKExceptionStatus;
	gTRKExceptionStatus.exceptionDetected = FALSE;

	TRKPPCAccessSPR(value_buf0, SPR_HID2, TRUE);

	value_buf0[0] |= 0xA0000000;
	TRKPPCAccessSPR(value_buf0, SPR_HID2, FALSE);

	value_buf0[0] = 0;
	TRKPPCAccessSPR(value_buf0, SPR_GQR0, FALSE);

	*registerStorageSize = 0;
	err                  = DS_NoError;

	for (i = firstRegister; (i <= lastRegister) && (err == DS_NoError); i++) {
		if (read) {
			err = TRKPPCAccessPairedSingleRegister((u64*)value_buf, i, read);
			err = TRKAppendBuffer1_ui64(b, *(u64*)value_buf);
		} else {
			err = TRKReadBuffer1_ui64(b, (u64*)value_buf);
			err = TRKPPCAccessPairedSingleRegister((u64*)value_buf, i, read);
		}

		*registerStorageSize += sizeof(u64);
	}

	if (gTRKExceptionStatus.exceptionDetected) {
		*registerStorageSize = 0;
		err                  = DS_CWDSException;
	}

	gTRKExceptionStatus = savedException;

	return err;
}

TRKResult TRKTargetVersions(DSVersions* versions)
{
	versions->kernelMajor   = 0;
	versions->kernelMinor   = 8;
	versions->protocolMajor = 1;
	versions->protocolMinor = 10;
	return DS_NoError;
}

TRKResult TRKTargetSupportMask(u8 mask[32])
{
	mask[0]    = 0x7a;
	mask[1]    = 0;
	mask[2]    = 0x4f;
	mask[3]    = 7;
	mask[4]    = 0;
	mask[5]    = 0;
	mask[6]    = 0;
	mask[7]    = 0;
	mask[8]    = 0;
	mask[9]    = 0;
	mask[10]   = 0;
	mask[0xb]  = 0;
	mask[0xc]  = 0;
	mask[0xd]  = 0;
	mask[0xe]  = 0;
	mask[0xf]  = 0;
	mask[0x10] = 1;
	mask[0x11] = 0;
	mask[0x12] = 3;
	mask[0x13] = 0;
	mask[0x14] = 0;
	mask[0x15] = 0;
	mask[0x16] = 0;
	mask[0x17] = 0;
	mask[0x18] = 0;
	mask[0x19] = 0;
	mask[0x1a] = 3;
	mask[0x1b] = 0;
	mask[0x1c] = 0;
	mask[0x1d] = 0;
	mask[0x1e] = 0;
	mask[0x1f] = 0x80;
	return DS_NoError;
}

TRKResult TRKTargetCPUType(DSCPUType* cpuType)
{
	cpuType->cpuMajor          = 0;
	cpuType->cpuMinor          = TRKTargetCPUMinorType();
	cpuType->bigEndian         = (u8)gTRKBigEndian;
	cpuType->defaultTypeSize   = 4;
	cpuType->fpTypeSize        = 8;
	cpuType->extended1TypeSize = 4;
	cpuType->extended2TypeSize = 8;
	cpuType->defaultTypeSize   = 4;
	return DS_NoError;
}

void TRKTargetCheckException(void)
{
	if (gTRKExceptionStatus.exceptionDetected) {
		gTRKExceptionStatus.exceptionDetected           = FALSE;
		gTRKExceptionStatus.exceptionInfo.exceptionID   = 0;
		gTRKExceptionStatus.exceptionInfo.PCInstruction = 0;
	}
}

void TRKPostInterruptEvent(void)
{
	int eventType;
	u32 local_14;
	size_t registerSize;
	TRKEvent event;

	if (gTRKState._9C) {
		gTRKState._9C = FALSE;
	} else {
		switch (gTRKCPUState.Extended1.exceptionID & 0xFFFF) {
		case 0xd00:
		case 0x700:
			registerSize = 4;
			TRKTargetReadInstruction(&local_14, gTRKCPUState.Default.PC);

			if (local_14 == 0xfe00000) {
				eventType = NUBEVENT_Support;
			} else {
				eventType = NUBEVENT_Breakpoint;
			}
			break;
		default:
			eventType = NUBEVENT_Exception;
			break;
		}

		TRKConstructEvent(&event, eventType);
		TRKPostEvent(&event);
	}
}

TRKResult TRKTargetInterrupt(TRKEvent* event)
{
	TRKResult error = DS_NoError;
	switch (event->mEventType) {
	case NUBEVENT_Breakpoint:
	case NUBEVENT_Exception:
		if (TRKTargetCheckStep() == FALSE) {
			TRKTargetSetStopped(TRUE);
			error = TRKDoNotifyStopped(DSMSG_NotifyStopped);
		}
		break;
	default:
		break;
	}

	return error;
}

static inline u32* ConvertAddress(u32 addr)
{
	return (u32*)(addr | 0x80000000);
}

TRKResult TRKTargetAddStopInfo(TRKBuffer* b)
{
	TRKResult error;
	CommandReply reply;
	int t;

	memset(&reply, 0, 0x40);
	reply._00          = 0x40;
	reply.commandID    = 0x90;
	reply.replyError   = gTRKCPUState.Default.PC;
	TRKTargetReadInstruction(&t, gTRKCPUState.Default.PC);

	reply._0C      = t;
	*(u32*)reply._10 = gTRKCPUState.Extended1.exceptionID & 0xFFFF;
	error            = TRKAppendBuffer_ui8(b, (u8*)&reply, 0x40);
	return error;
}

void TRKTargetAddExceptionInfo(TRKBuffer* b)
{
	u32 local_54;
	CommandReply reply;

	memset(&reply, 0, 0x40);

	reply._00        = 0x40;
	reply.commandID  = 0x91;
	reply.replyError = gTRKExceptionStatus.exceptionInfo.PC;

	TRKTargetReadInstruction(&local_54, gTRKExceptionStatus.exceptionInfo.PC);

	reply._0C        = local_54;
	*(u32*)reply._10 = gTRKExceptionStatus.exceptionInfo.exceptionID;

	TRKAppendBuffer_ui8(b, (u8*)&reply, 0x40);
}

static TRKResult TRKTargetEnableTrace(BOOL val)
{
	if (val) {
		gTRKCPUState.Extended1.MSR = (gTRKCPUState.Extended1.MSR | 0x400);
	} else {
		gTRKCPUState.Extended1.MSR = (gTRKCPUState.Extended1.MSR & ~0x400);
	}
	return DS_NoError;
}

static BOOL TRKTargetStepDone(void)
{
	BOOL result = TRUE;

	if (gTRKStepStatus.active && ((u16)gTRKCPUState.Extended1.exceptionID) == 0xD00) {
		switch (gTRKStepStatus.type) {
		case DSSTEP_IntoCount:
			if (gTRKStepStatus.count > 0) {
				result = FALSE;
			}
			break;
		case DSSTEP_IntoRange:
			if (gTRKCPUState.Default.PC >= gTRKStepStatus.rangeStart && gTRKCPUState.Default.PC <= gTRKStepStatus.rangeEnd) {
				result = FALSE;
			}
			break;
		default:
			break;
		}
	}

	return result;
}

static TRKResult TRKTargetDoStep(void)
{
	gTRKStepStatus.active = TRUE;
	MWTRACE(1, "TargetDoStep()\n");
	TRKTargetEnableTrace(TRUE);

	if (gTRKStepStatus.type == DSSTEP_IntoCount || gTRKStepStatus.type == DSSTEP_OverCount) {
		gTRKStepStatus.count--;
	}

	TRKTargetSetStopped(FALSE);
	return DS_NoError;
}

static BOOL TRKTargetCheckStep(void)
{
	if (gTRKStepStatus.active) {
		TRKTargetEnableTrace(FALSE);

		if (TRKTargetStepDone()) {
			gTRKStepStatus.active = FALSE;
		} else {
			TRKTargetDoStep();
		}
	}

	return gTRKStepStatus.active;
}

TRKResult TRKTargetSingleStep(u32 count, BOOL stepOver)
{
	TRKResult error = DS_NoError;

	if (stepOver) {
		error = DS_UnsupportedError;
	} else {
		gTRKStepStatus.count = count;
		gTRKStepStatus.type  = DSSTEP_IntoCount;
		error                = TRKTargetDoStep();
	}

	return error;
}

TRKResult TRKTargetStepOutOfRange(u32 rangeStart, u32 rangeEnd, BOOL stepOver)
{
	TRKResult error = DS_NoError;

	if (stepOver) {
		error = DS_UnsupportedError;
	} else {
		gTRKStepStatus.type       = DSSTEP_IntoRange;
		gTRKStepStatus.rangeStart = rangeStart;
		gTRKStepStatus.rangeEnd   = rangeEnd;
		error                     = TRKTargetDoStep();
	}

	return error;
}

u32 TRKTargetGetPC(void)
{
	return gTRKCPUState.Default.PC;
}

TRKResult TRKTargetSupportRequest(void)
{
	DSIOResult ioResult;
	size_t* length;
	int commandId;
	TRKResult error;
	u32 local_28;
	u32 g4;
	u32 g5;
	u32 g6;
	TRKEvent event;

	commandId = gTRKCPUState.Default.GPR[3];
	if (commandId != DSMSG_ReadFile && commandId != DSMSG_WriteFile && commandId != DSMSG_OpenFile && commandId != DSMSG_CloseFile
	    && commandId != DSMSG_PositionFile) {
		TRKConstructEvent(&event, 4);
		TRKPostEvent(&event);
		return DS_NoError;
	} else if (commandId == DSMSG_OpenFile) {
		error = HandleOpenFileSupportRequest((const char*)gTRKCPUState.Default.GPR[4], gTRKCPUState.Default.GPR[5], (u32*)gTRKCPUState.Default.GPR[6], &ioResult);

		if (ioResult == DS_IONoError && error != DS_NoError) {
			ioResult = DS_IOError;
		}

		gTRKCPUState.Default.GPR[3] = ioResult;
	} else if (commandId == DSMSG_CloseFile) {
		error = HandleCloseFileSupportRequest(gTRKCPUState.Default.GPR[4], &ioResult);

		if (ioResult == DS_IONoError && error != DS_NoError) {
			ioResult = DS_IOError;
		}

		gTRKCPUState.Default.GPR[3] = ioResult;
	} else if (commandId == DSMSG_PositionFile) {
		local_28 = *(u32*)gTRKCPUState.Default.GPR[5];
		error    = HandlePositionFileSupportRequest(gTRKCPUState.Default.GPR[4], &local_28, (u8)gTRKCPUState.Default.GPR[6], &ioResult);

		if (ioResult == DS_IONoError && error != DS_NoError) {
			ioResult = DS_IOError;
		}

		gTRKCPUState.Default.GPR[3]        = ioResult;
		*(u32*)gTRKCPUState.Default.GPR[5] = local_28;
	} else {
		length = (size_t*)gTRKCPUState.Default.GPR[5];
		error  = TRKSuppAccessFile(gTRKCPUState.Default.GPR[4], (u8*)gTRKCPUState.Default.GPR[6], length, &ioResult, TRUE,
		                           commandId == DSMSG_ReadFile);

		if (ioResult == DS_IONoError && error != DS_NoError) {
			ioResult = DS_IOError;
		}

		gTRKCPUState.Default.GPR[3] = ioResult;

		if (commandId == DSMSG_ReadFile) {
			TRK_flush_cache(gTRKCPUState.Default.GPR[6], *length);
		}
	}

	gTRKCPUState.Default.PC += 4;
	return error;
}

TRKResult TRKTargetFlushCache(u8 unused, void* start, void* end)
{
	if (start < end) {
		TRK_flush_cache((u32)start, (u8*)end - (u8*)start);
		return DS_NoError;
	}

	return DS_InvalidMemory;
}

BOOL TRKTargetStopped(void)
{
	return gTRKState.mIsStopped;
}

void TRKTargetSetStopped(uint stopped)
{
	gTRKState.mIsStopped = stopped;
}

BOOL TRKTargetStop(void)
{
	gTRKState.mIsStopped = 1;
	return FALSE;
}

static TRKResult TRKPPCAccessSPR(void* value, u32 spr_register_num, BOOL read)
{
	u32 access_func[10] = { INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP };

	if (read) {
		access_func[0] = INSTR_MFSPR(4, spr_register_num);
		access_func[1] = (u32)INSTR_STW(4, 0, 3);
	} else {
		access_func[0] = (u32)INSTR_LWZ(4, 0, 3);
		access_func[1] = INSTR_MTSPR(spr_register_num, 4);
	}

	return TRKPPCAccessSpecialReg(value, access_func, read);
}

static TRKResult TRKPPCAccessPairedSingleRegister(void* srcDestPtr, u32 psr, BOOL read)
{
	u32 instructionData[] = { INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP };

	if (read) {
		instructionData[0] = INSTR_PSQ_ST(psr, 0, 3, 0, 0);
	} else {
		instructionData[0] = INSTR_PSQ_L(psr, 0, 3, 0, 0);
	}

	return TRKPPCAccessSpecialReg(srcDestPtr, instructionData, read);
}

static TRKResult TRKPPCAccessFPRegister(void* srcDestPtr, u32 fpr, BOOL read)
{
	TRKResult error = DS_NoError;
	u32 instructionData1[] = { INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP, INSTR_NOP };

	if (fpr < 0x20) {
		if (read) {
			instructionData1[0] = INSTR_STFD(fpr, 0, 3);
		} else {
			instructionData1[0] = INSTR_LFD(fpr, 0, 3);
		}

		error = TRKPPCAccessSpecialReg(srcDestPtr, instructionData1, read);
	} else if (fpr == 0x20) {
		if (read) {
			ReadFPSCR(srcDestPtr);
		} else {
			WriteFPSCR(srcDestPtr);
		}

		*(u64*)srcDestPtr &= 0xFFFFFFFF;
	} else if (fpr == 0x21) {
		if (!read) {
			*(u32*)srcDestPtr = *((u32*)(srcDestPtr) + 1);
		}

		error = TRKPPCAccessSPR(srcDestPtr, SPR_FPECR, read);
		if (read) {
			DSFetch_u64(srcDestPtr) = DSFetch_u32(srcDestPtr) & 0xffffffffLL;
		}
	}

	return error;
}

static TRKResult TRKPPCAccessSpecialReg(void* value, u32* access_func, BOOL read)
{
	typedef void (*asm_access_type)(void*, void*);

	asm_access_type asm_access;

	access_func[9] = INSTR_BLR;

	asm_access = (asm_access_type)access_func;

	TRK_flush_cache((u32)access_func, (sizeof(access_func[0]) * 10));
	(*asm_access)((u32*)value, (void*)&TRKvalue128_temp);

	return DS_NoError;
}

void TRKTargetSetInputPendingPtr(void* ptr)
{
	gTRKState._A0 = ptr;
}

void SetInputState(BOOL state)
{
	gTRKState._9C = state;
}

BOOL TRKGetInTRKFlag(void)
{
	return gTRKExceptionStatus.inTRK;
}

TRKResult TargetAccessARAM(u32 p1, u32 p2, u32* p3, BOOL read)
{
	TRKResult err;
	TRKExceptionStatus status;

	err    = DS_NoError;
	status = gTRKExceptionStatus;

	gTRKExceptionStatus.exceptionDetected = 0;

	if (read) {
		TRK__read_aram(p1, p2, p3);
	} else {
		TRK__write_aram(p1, p2, p3);
	}

	if (gTRKExceptionStatus.exceptionDetected) {
		*p3 = 0;
		err = 0x702;
	}
	gTRKExceptionStatus = status;
	return err;
}


asm u32 __TRK_get_MSR(void)
{
	nofralloc
	mfmsr r3
	blr
}

asm void __TRK_set_MSR(register u32 v)
{
	nofralloc
	mtmsr v
	blr
}

asm void __TRK_get_PVR(void)
{
	nofralloc
	mfspr r3, 287
	blr
}

asm void __TRK_get_IBAT0U(void)
{
	nofralloc
	mfspr r3, 528
	blr
}

asm void __TRK_get_IBAT0L(void)
{
	nofralloc
	mfspr r3, 529
	blr
}

asm void __TRK_get_IBAT1U(void)
{
	nofralloc
	mfspr r3, 530
	blr
}

asm void __TRK_get_IBAT1L(void)
{
	nofralloc
	mfspr r3, 531
	blr
}

asm void __TRK_get_IBAT2U(void)
{
	nofralloc
	mfspr r3, 532
	blr
}

asm void __TRK_get_IBAT2L(void)
{
	nofralloc
	mfspr r3, 533
	blr
}

asm void __TRK_get_IBAT3U(void)
{
	nofralloc
	mfspr r3, 534
	blr
}

asm void __TRK_get_IBAT3L(void)
{
	nofralloc
	mfspr r3, 535
	blr
}

asm void __TRK_get_DBAT0U(void)
{
	nofralloc
	mfspr r3, 536
	blr
}

asm void __TRK_get_DBAT0L(void)
{
	nofralloc
	mfspr r3, 537
	blr
}

asm void __TRK_get_DBAT1U(void)
{
	nofralloc
	mfspr r3, 538
	blr
}

asm void __TRK_get_DBAT1L(void)
{
	nofralloc
	mfspr r3, 539
	blr
}

asm void __TRK_get_DBAT2U(void)
{
	nofralloc
	mfspr r3, 540
	blr
}

asm void __TRK_get_DBAT2L(void)
{
	nofralloc
	mfspr r3, 541
	blr
}

asm void __TRK_get_DBAT3U(void)
{
	nofralloc
	mfspr r3, 542
	blr
}

asm void __TRK_get_DBAT3L(void)
{
	nofralloc
	mfspr r3, 543
	blr
}

static asm void TRK_ppc_memcpy(register void* dest, register const void* src, register int n, register u32 param_4, register u32 param_5)
{
	#define msr   r8
	#define byte  r9
	#define count r10
	nofralloc

	mfmsr msr
	li count, 0

top_loop:
	cmpw count, n
	beq out_loop

	mtmsr param_5
	sync

	lbzx byte, count, src

	mtmsr param_4
	sync

	stbx byte, count, dest

	addi count, count, 1

	b top_loop
out_loop:
	mtmsr msr
	sync

	blr
	#undef count
	#undef byte
	#undef msr
}

asm void TRKInterruptHandler(void)
{
	nofralloc
	mtsrr0 r2
	mtsrr1 r4
	mfsprg r4, 3
	mfcr r2
	mtsprg 3, r2
	lis r2, gTRKState@h
	ori r2, r2, gTRKState@l
	lwz r2, 0x8C(r2)
	ori r2, r2, 0x8002
	xori r2, r2, 0x8002
	sync
	mtmsr r2
	sync
	lis r2, TRK_saved_exceptionID@h
	ori r2, r2, TRK_saved_exceptionID@l
	sth r3, 0(r2)
	cmpwi r3, 0x500
	bne L_802CF694
	lis r2, gTRKCPUState@h
	ori r2, r2, gTRKCPUState@l
	mflr r3
	stw r3, 0x42C(r2)
	bl TRKUARTInterruptHandler
	lis r2, gTRKCPUState@h
	ori r2, r2, gTRKCPUState@l
	lwz r3, 0x42C(r2)
	mtlr r3
	lis r2, gTRKState@h
	ori r2, r2, gTRKState@l
	lwz r2, 0xA0(r2)
	lbz r2, 0(r2)
	cmpwi r2, 0
	beq L_802CF678
	lis r2, gTRKExceptionStatus@h
	ori r2, r2, gTRKExceptionStatus@l
	lbz r2, 0xC(r2)
	cmpwi r2, 1
	beq L_802CF678
	lis r2, gTRKState@h
	ori r2, r2, gTRKState@l
	li r3, 1
	stb r3, 0x9C(r2)
	b L_802CF694
L_802CF678:
	lis r2, gTRKSaveState@h
	ori r2, r2, gTRKSaveState@l
	lwz r3, 0x88(r2)
	mtcrf 0xff, r3
	lwz r3, 0xC(r2)
	lwz r2, 0x8(r2)
	rfi
L_802CF694:
	lis r2, TRK_saved_exceptionID@h
	ori r2, r2, TRK_saved_exceptionID@l
	lhz r3, 0(r2)
	lis r2, gTRKExceptionStatus@h
	ori r2, r2, gTRKExceptionStatus@l
	lbz r2, 0xC(r2)
	cmpwi r2, 0
	bne TRKExceptionHandler
	lis r2, gTRKCPUState@h
	ori r2, r2, gTRKCPUState@l
	stw r0, 0(r2)
	stw r1, 4(r2)
	mfsprg r0, 1
	stw r0, 8(r2)
	sth r3, 0x2F8(r2)
	sth r3, 0x2FA(r2)
	mfsprg r0, 2
	stw r0, 0xC(r2)
	stmw r4, 0x10(r2)
	mfsrr0 r27
	mflr r28
	mfsprg r29, 3
	mfctr r30
	mfxer r31
	stmw r27, 0x80(r2)
	bl TRKSaveExtended1Block
	lis r2, gTRKExceptionStatus@h
	ori r2, r2, gTRKExceptionStatus@l
	li r3, 1
	stb r3, 0xC(r2)
	lis r2, gTRKState@h
	ori r2, r2, gTRKState@l
	lwz r0, 0x8C(r2)
	sync
	mtmsr r0
	sync
	lwz r0, 0x80(r2)
	mtlr r0
	lwz r0, 0x84(r2)
	mtctr r0
	lwz r0, 0x88(r2)
	mtxer r0
	lwz r0, 0x94(r2)
	mtdsisr r0
	lwz r0, 0x90(r2)
	mtdar r0
	lmw r3, 0xC(r2)
	lwz r0, 0(r2)
	lwz r1, 4(r2)
	lwz r2, 8(r2)
	b TRKPostInterruptEvent
}

asm void TRKExceptionHandler(u16 r3)
{
	nofralloc
	lis r2, gTRKExceptionStatus@h
	ori r2, r2, gTRKExceptionStatus@l
	sth r3, 8(r2)
	mfsrr0 r3
	stw r3, 0(r2)
	lhz r3, 8(r2)
	cmpwi r3, 0x200
	beq LAB_00010ba4
	cmpwi r3, 0x300
	beq LAB_00010ba4
	cmpwi r3, 0x400
	beq LAB_00010ba4
	cmpwi r3, 0x600
	beq LAB_00010ba4
	cmpwi r3, 0x700
	beq LAB_00010ba4
	cmpwi r3, 0x800
	beq LAB_00010ba4
	cmpwi r3, 0x1000
	beq LAB_00010ba4
	cmpwi r3, 0x1100
	beq LAB_00010ba4
	cmpwi r3, 0x1200
	beq LAB_00010ba4
	cmpwi r3, 0x1300
	beq LAB_00010ba4
	b LAB_00010bb0
LAB_00010ba4:
	mfsrr0 r3
	addi r3, r3, 0x4
	mtsrr0 r3
LAB_00010bb0:
	lis r2, gTRKExceptionStatus@h
	ori r2, r2, gTRKExceptionStatus@l
	li r3, 0x1
	stb r3, 0xD(r2)
	mfsprg r3, 3
	mtcrf 0xff, r3
	mfsprg r2, 1
	mfsprg r3, 2
	rfi
}

asm void TRKSwapAndGo(void)
{
	nofralloc
	lis r3, gTRKState@h
	ori r3, r3, gTRKState@l
	stmw r0, 0(r3)
	mfmsr r0
	stw r0, 0x8C(r3)
	mflr r0
	stw r0, 0x80(r3)
	mfctr r0
	stw r0, 0x84(r3)
	mfxer r0
	stw r0, 0x88(r3)
	mfdsisr r0
	stw r0, 0x94(r3)
	mfdar r0
	stw r0, 0x90(r3)
	li r1, -0x7ffe
	nor r1, r1, r1
	mfmsr r3
	and r3, r3, r1
	mtmsr r3
	lis r2, gTRKState@h
	ori r2, r2, gTRKState@l
	lwz r2, 0xA0(r2)
	lbz r2, 0(r2)
	cmpwi r2, 0
	beq L_802CF930
	lis r2, gTRKState@h
	ori r2, r2, gTRKState@l
	li r3, 1
	stb r3, 0x9C(r2)
	b TRKInterruptHandlerEnableInterrupts
L_802CF930:
	lis r2, gTRKExceptionStatus@h
	ori r2, r2, gTRKExceptionStatus@l
	li r3, 0
	stb r3, 0xC(r2)
	bl TRKRestoreExtended1Block
	lis r2, gTRKCPUState@h
	ori r2, r2, gTRKCPUState@l
	lmw r27, 0x80(r2)
	mtsrr0 r27
	mtlr r28
	mtcrf 0xff, r29
	mtctr r30
	mtxer r31
	lmw r3, 0xC(r2)
	lwz r0, 0(r2)
	lwz r1, 4(r2)
	lwz r2, 8(r2)
	rfi
}

asm void TRKInterruptHandlerEnableInterrupts(void)
{
	nofralloc
	lis r2, gTRKState@h
	ori r2, r2, gTRKState@l
	lwz r0, 0x8C(r2)
	sync
	mtmsr r0
	sync
	lwz r0, 0x80(r2)
	mtlr r0
	lwz r0, 0x84(r2)
	mtctr r0
	lwz r0, 0x88(r2)
	mtxer r0
	lwz r0, 0x94(r2)
	mtdsisr r0
	lwz r0, 0x90(r2)
	mtdar r0
	lmw r3, 0xC(r2)
	lwz r0, 0(r2)
	lwz r1, 4(r2)
	lwz r2, 8(r2)
	b TRKPostInterruptEvent
}

asm void ReadFPSCR(register f64*)
{
	nofralloc
	stwu r1, -0x40(r1)
	stfd f31, 0x10(r1)
	psq_st f31, 0x20(r1), 0, 0
	mffs f31
	stfd f31, 0x0(r3)
	psq_l f31, 0x20(r1), 0, 0
	lfd f31, 0x10(r1)
	addi r1, r1, 0x40
	blr
}

asm void WriteFPSCR(register f64*)
{
	nofralloc
	stwu r1, -0x40(r1)
	stfd f31, 0x10(r1)
	psq_st f31, 0x20(r1), 0, 0
	lfd f31, 0(r3)
	mtfsf 0xff, f31
	psq_l f31, 0x20(r1), 0, 0
	lfd f31, 0x10(r1)
	addi r1, r1, 0x40
	blr
}
