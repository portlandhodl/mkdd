#include "PowerPC_EABI_Support/MetroTRK/trk.h"
#include "dolphin/ar/ar.h"
#include "dolphin/os.h"
#include "macros.h"

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

typedef struct Extended1_PPC_6xx_7xx {
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
} Extended1_PPC_6xx_7xx;

typedef struct Extended2_PPC_6xx_7xx {
	u32 PSR[32][2];
} Extended2_PPC_6xx_7xx;

typedef struct ProcessorState_PPC_6xx_7xx {
	Default_PPC Default;
	Float_PPC Float;
	Extended1_PPC_6xx_7xx Extended1;
	Extended2_PPC_6xx_7xx Extended2;
	u32 transport_handler_saved_ra;
} ProcessorState_PPC_6xx_7xx;

typedef ProcessorState_PPC_6xx_7xx ProcessorState_PPC;

extern ProcessorState_PPC gTRKCPUState;
extern TRKState gTRKState;
extern void EnableEXI2Interrupts(void);
extern int InitMetroTRKCommTable(int hwId);
extern void TRK_main(void);
extern u8 gTRKInterruptVectorTable[];
extern u32 __TRK_get_MSR(void);
extern void TRK_flush_cache(void* addr, u32 size);
extern void TRKSaveExtended1Block(void);

#define PPC_SystemReset              0x100
#define PPC_MachineCheck             0x200
#define PPC_DataStorage              0x300
#define PPC_InstructionStorage       0x400
#define PPC_ExternalInterrupt        0x500
#define PPC_Alignment                0x600
#define PPC_Program                  0x700
#define PPC_FloatingPointUnavailable 0x800
#define PPC_Decrementer              0x900
#define PPC_SystemCall          0xC00
#define PPC_Trace               0xD00
#define PPC_PerformanceMonitor  0xF00
#define PPC_InstructionAddressBreakpoint 0x1300
#define PPC_SystemManagementInterrupt 0x1400
#define PPC_ThermalManagementInterrupt 0x1700

extern u8 _db_stack_addr[];
#define EXCEPTIONMASK_ADDR 0x80000044

static u32 lc_base;

static u32 TRK_ISR_OFFSETS[15] = { PPC_SystemReset,
                                   PPC_MachineCheck,
                                   PPC_DataStorage,
                                   PPC_InstructionStorage,
                                   PPC_ExternalInterrupt,
                                   PPC_Alignment,
                                   PPC_Program,
                                   PPC_FloatingPointUnavailable,
                                   PPC_Decrementer,
                                   PPC_SystemCall,
                                   PPC_Trace,
                                   PPC_PerformanceMonitor,
                                   PPC_InstructionAddressBreakpoint,
                                   PPC_SystemManagementInterrupt,
                                   PPC_ThermalManagementInterrupt };

static __declspec(section ".data") u32 lc_base = 0;

DECL_SECTION(".init") void __TRK_reset()
{
	OSResetSystem(FALSE, 0, FALSE);
}

/**
 * @note Address: 0x800C03EC
 * @note Size: 0x20
 */
void EnableMetroTRKInterrupts(void)
{
	EnableEXI2Interrupts();
}

/**
 * @note Address: 0x800C0394
 * @note Size: 0x58
 */
u32 TRKTargetTranslate(u32 val)
{
	if (val >= lc_base && val < lc_base + 0x4000) {
		if (((u32*)(&gTRKCPUState.Extended1))[36] & 3)
			return val;
	}

	if (val >= 0x7E000000 && val <= 0x80000000) {
		return val;
	}

	return (val & 0x3FFFFFFF) | 0x80000000;
}

/**
 * @note Address: N/A
 * @note Size: 0xB0
 */
void TRK_copy_vector(u32 offset)
{
	void* destPtr = (void*)TRKTargetTranslate(offset);
	TRK_memcpy(destPtr, (void*)(gTRKInterruptVectorTable + offset), 0x100);
	TRK_flush_cache(destPtr, 0x100);
}

/**
 * @note Address: 0x800C0268
 * @note Size: 0x12C
 */
void __TRK_copy_vectors(void)
{
	u32 r3 = lc_base;
	u32* isrOffsetPtr;
	int i;
	u32 r29;

	if (r3 <= 0x44 && r3 + 0x4000 > 0x44 && gTRKCPUState.Extended1.DBAT3U_ & 3) {
		r3 = 0x44;
	} else {
		r3 = EXCEPTIONMASK_ADDR;
	}

	i            = 0;
	r29          = *(u32*)r3;
	isrOffsetPtr = TRK_ISR_OFFSETS;

	do {
		if ((r29 & (1 << i)) && i != 4) {
			TRK_copy_vector(isrOffsetPtr[i]);
		}

		i++;
	} while (i <= 14);
}

/**
 * @note Address: 0x800C021C
 * @note Size: 0x4C
 */
TRKResult TRKInitializeTarget()
{
	gTRKState.mIsStopped = TRUE;
	gTRKState._8C       = __TRK_get_MSR();
	lc_base             = 0xE0000000;
	return TRKSuccess;
}

#define __dcbi(a, b)    asm { dcbi a, b }
#define __dcbfASM(a, b) asm { dcbf a, b }

/**
 * @note Address: 0x800C00E8
 * @note Size: 0x134
 */
void TRK__read_aram(register int c, register u32 p2, void* p3)
{
	u32 err;
	int i;
	register int counter;
	u16 r;
	u32 g;
	u32 x;
	u32 size;

	if ((size_t)p2 < 0x4000 || p2 + *(u32*)p3 > 0x8000000) {
		return;
	}

	x       = p2 & ~0x1F;
	size    = *(u32*)p3 + (p2 & 0x1F);
	size    = OSRoundUp32B(size);
	counter = 0;

	for (i = 0; i < size; i += 0x20) {
		__dcbi(counter, c);
		counter += 0x20;
	}

	do {
		err = ARGetDMAStatus();
	} while (err);

	r = __ARGetInterruptStatus();
	g = 0x8000000;
	__ARClearInterrupt();

	ARStartDMA(1, c, x, size);

	while (!__ARGetInterruptStatus()) { }

	if (!r) {
		__ARClearInterrupt();
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x5C
 */
void __read_aram_1block(void)
{
	// UNUSED FUNCTION
}

/**
 * @note Address: 0x800BFEFC
 * @note Size: 0x1EC
 */
void TRK__write_aram(register int c, register u32 p2, void* p3)
{
	u8 buff[32] ATTRIBUTE_ALIGN(32);
	u32 err;
	register int count = c;
	register u32 bf;
	u32 uVar1;
	u32 size;
	u16 r;
	register u32 g;
	register int counter;
	u32 i;

	if ((size_t)p2 < 0x4000 || p2 + *(u32*)p3 > 0x8000000) {
		return;
	}

	uVar1   = p2 & ~0x1f;
	counter = 0;
	size    = *(u32*)p3 + (p2 & 0x1f);
	size    = OSRoundUp32B(size);

	for (i = 0; i < size; i += 0x20) {
		__dcbf((void*)counter, count);
		counter += 0x20;
	}

	do {
		err = ARGetDMAStatus();
	} while (err);

	r = __ARGetInterruptStatus();
	g = 0x8000000;

	counter = p2 & 0x1f;
	if (counter) {
		g  = uVar1;
		bf = (u32)buff;
		__dcbi(r0, bf);
		__ARClearInterrupt();

		ARStartDMA(1, bf, uVar1, 0x20);

		while (!__ARGetInterruptStatus()) { }

		TRK_memcpy((void*)c, buff, counter);
		__dcbfASM(r0, c);
	}

	p2 += *(u32*)p3;
	counter = p2 & 0x1f;
	if (counter) {
		u32 val = p2 & ~0x1F;
		if (val != g) {
			bf = (u32)buff;
			__dcbi(r0, bf);
			__ARClearInterrupt();
			ARStartDMA(1, bf, val, 0x20);

			while (!__ARGetInterruptStatus()) { }
		}
		g = c + p2;
		TRK_memcpy((void*)g, (void*)(buff + counter), 0x20 - counter);

		__dcbfASM(r0, g);
	}
	__sync();
	__ARClearInterrupt();
	ARStartDMA(0, c, uVar1, size);
	if (!r) {
		while (!__ARGetInterruptStatus()) { }

		__ARClearInterrupt();
	}
}

/**
 * @note Address: 0x800BFDD0
 * @note Size: 0x98
 */
ASM void InitMetroTRK(void)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	addi r1, r1, -4
	stw r3, 0(r1)
	lis r3, gTRKCPUState@h
	ori r3, r3, gTRKCPUState@l
	stmw r0, ProcessorState_PPC.Default.GPR(r3) //Save the gprs
	lwz r4, 0(r1)
	addi r1, r1, 4
	stw r1, ProcessorState_PPC.Default.GPR[1](r3)
	stw r4, ProcessorState_PPC.Default.GPR[3](r3)
	mflr r4
	stw r4, ProcessorState_PPC.Default.LR(r3)
	stw r4, ProcessorState_PPC.Default.PC(r3)
	mfcr r4
	stw r4, ProcessorState_PPC.Default.CR(r3)
	//???
	mfmsr r4
	ori r3, r4, (1 << (31 - 16))
	xori r3, r3, (1 << (31 - 16))
	mtmsr r3
	mtsrr1 r4 //Copy msr to srr1
	//Save misc registers to gTRKCPUState
	bl TRKSaveExtended1Block
	lis r3, gTRKCPUState@h
	ori r3, r3, gTRKCPUState@l
	lmw r0, ProcessorState_PPC.Default.GPR(r3) //Restore the gprs
	//Reset IABR and DABR
	li r0, 0
	mtspr  0x3f2, r0
	mtspr  0x3f5, r0
	//Restore stack pointer
	lis r1, _db_stack_addr@h
	ori r1, r1, _db_stack_addr@l
	mr r3, r5
	bl InitMetroTRKCommTable //Initialize comm table
	/*
	If InitMetroTRKCommTable returned 1 (failure), an invalid hardware
	id or the id for GDEV was somehow passed. Since only BBA or NDEV
	are supported, we return early. Otherwise, we proceed with
	starting up TRK.
	*/
	cmpwi r3, 1
	bne initCommTableSuccess
	/*
	BUG: The code probably originally reloaded gTRKCPUState here, but
	as is it will read the returned value of InitMetroTRKCommTable
	as a TRKCPUState struct pointer, causing the CPU to return to
	a garbage code address.
	*/
	lwz r4, ProcessorState_PPC.Default.LR(r3)
	mtlr r4
	lmw r0, ProcessorState_PPC.Default.GPR(r3) //Restore the gprs
	blr
initCommTableSuccess:
	b TRK_main //Jump to TRK_main
	blr
#endif // clang-format on
}

/**
 * @note Address: 0x800BFE68
 * @note Size: 0x94
 */
ASM void InitMetroTRK_BBA(void)
{
#ifdef __MWERKS__ // clang-format off
	nofralloc
	addi r1, r1, -4
	stw r3, 0(r1)
	lis r3, gTRKCPUState@h
	ori r3, r3, gTRKCPUState@l
	stmw r0, ProcessorState_PPC.Default.GPR(r3) //Save the gprs
	lwz r4, 0(r1)
	addi r1, r1, 4
	stw r1, ProcessorState_PPC.Default.GPR[1](r3)
	stw r4, ProcessorState_PPC.Default.GPR[3](r3)
	mflr r4
	stw r4, ProcessorState_PPC.Default.LR(r3)
	stw r4, ProcessorState_PPC.Default.PC(r3)
	mfcr r4
	stw r4, ProcessorState_PPC.Default.CR(r3)
	//Turn on external interrupts
	mfmsr r4
	ori r3, r4, (1 << (31 - 16))
	mtmsr r3
	mtsrr1 r4 //Copy original msr to srr1
	//Save misc registers to gTRKCPUState
	bl TRKSaveExtended1Block
	lis r3, gTRKCPUState@h
	ori r3, r3, gTRKCPUState@l
	lmw r0, ProcessorState_PPC.Default.GPR(r3) //Restore the gprs
	//Reset IABR and DABR
	li r0, 0
	mtspr  0x3f2, r0
	mtspr 0x3f5, r0
	//Restore the stack pointer
	lis r1, _db_stack_addr@h
	ori r1, r1, _db_stack_addr@l
	li r3, 2
	bl InitMetroTRKCommTable //Initialize comm table as BBA hardware
	/*
	If InitMetroTRKCommTable returned 1 (failure), something went wrong
	or whatever reason. If everything goes as expected, we proceed with
	starting up TRK.
	*/
	cmpwi r3, 1
	bne initCommTableSuccess
	/*
	BUG: The code probably originally reloaded gTRKCPUState here, but
	as is it will read the returned value of InitMetroTRKCommTable
	as a TRKCPUState struct pointer, causing the CPU to return to
	a garbage code address.
	*/
	lwz r4, ProcessorState_PPC.Default.LR(r3)
	mtlr r4
	lmw r0, ProcessorState_PPC.Default.GPR(r3)
	blr
initCommTableSuccess:
	b TRK_main //Jump to TRK_main
	blr
#endif // clang-format on
}
