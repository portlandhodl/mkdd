#include <dolphin/types.h>
#include <dolphin/os.h>
#include <dolphin/eth/eth.h>
#include <string.h>

typedef int (*ETHRecvCallback0)(u16 type, s32 length, u8 flags);
typedef void (*ETHRecvCallback1)(u8* buf, s32 length);
typedef void (*IFCallback)(void* arg, int unused);

typedef struct IFQueue {
    void* head;
    void* tail;
} IFQueue;

typedef struct IFPacket {
    void* ifp;
    IFQueue* queue;
    struct IFPacket* next;
    struct IFPacket* prev;
    u16 type;
    u8 dst[6];
    u8 src[6];
    u8 data[0x3C];
} IFPacket;

extern void IGMPOnReset(BOOL);
extern void IGMPInit(void*);
extern void UDPOnReset(BOOL);
extern void TCPOnReset(BOOL);
extern void ARPInit(void);
extern void IPInitRoute(u8*, u8*, u8*);
extern void IFFifoInit(void*, void*, u32);
extern void IFFifoFree(void*, void*, u32);
extern void* IFFifoAlloc(void*);
extern void DHCPReboot(void);
extern void IPAutoConfig(void);
extern void IPSetConfigError(void*, s32);
extern void IPIn(void*, u8*, u32, int);
extern void PPPoEIn(void*, u8*, u32);
extern void ARPHold(void*, void*);
extern void ARPIn(void*, u8*, u32);
extern u32 ARPLookup(void*, u8*, u8*);
extern u32 IPAddrAny;
extern u32 IPLimited;

static BOOL OnReset(BOOL);
static void LinkCheckHandler(void);
static u32 Go(void);
static void GoCallback(u32);
static void* Callback0(u16, s32, u8);
static void Callback1(u8*, s32);
static u32 Cancel(void*, void*);
static void* Alloc(u32);
static void Free(void*);
u32 IFMute(u32);
u32 IFInit(s32);
void ETHOut(void*, void*);
void ETHIn(void*, u8*, u32);
static u32 NullFilter(void);

const char *__IPVersion = "<< Dolphin SDK - IP\trelease build: Sep 29 2003 18:25:54 (0x2301) >>";

static OSResetFunctionInfo ResetFunctionInfo = {OnReset, 0x6F};

static u16 Protocols[4] = {0x0800, 0x0806, 0x8863, 0x8864};
static u8 HwBroadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static u32 Mute;

static u8 RecvBuf[0x5EE] __attribute__((aligned(32)));
static u8 SendBuf[0x5EE] __attribute__((aligned(32)));
static u8 SendFifo[0x10];
static u8 SendHeap[0x4000];
static u8 LinkAlarm[0x28] __attribute__((aligned(8)));
static u8 Current[0x58];
u8 __IFDefault[0xA8] __attribute__((aligned(8)));

static u32 Loopback;
static u32 Sending;
static void* SendQueue[2];
static u32 LinkState;
static u32 Reset;
static u32 initialized;

static asm u32 NullFilter(void)
{
    nofralloc
    li r3,1
    blr
}

asm void ETHIn(void* ifp, u8* frame, u32 len)
{
    nofralloc
    mflr r0
    lis r6,RecvBuf@h
    stw r0,4(r1)
    stwu r1,-40(r1)
    stw r31,36(r1)
    addi r31,r5,0
    stw r30,32(r1)
    mr r30,r3
    stw r29,28(r1)
    addi r29,r6,RecvBuf@l
    stw r28,24(r1)
    addi r28,r4,0
    lwz r12,112(r3)
    mtlr r12
    blrl
    cmpwi r3,0
    beq L_ein_end
    addi r3,r28,0
    li r4,HwBroadcastAddr
    li r5,6
    bl memcmp
    cmpwi r3,0
    bne L_ein_1
    lwz r3,19716(r29)
    li r6,1
    addi r0,r3,1
    stw r0,19716(r29)
    b L_ein_2
L_ein_1:
    lbz r0,0(r28)
    cmplwi r0,1
    bne L_ein_3
    lwz r3,19716(r29)
    li r6,2
    addi r0,r3,1
    stw r0,19716(r29)
    b L_ein_2
L_ein_3:
    lwz r3,19712(r29)
    li r6,0
    addi r0,r3,1
    stw r0,19712(r29)
L_ein_2:
    lhz r4,12(r28)
    cmpwi r4,2054
    beq L_ein_4
    bge L_ein_5
    cmpwi r4,2048
    beq L_ein_6
    b L_ein_end
L_ein_5:
    lis r3,1
    addi r0,r3,-30619
    cmpw r4,r0
    bge L_ein_end
    addi r0,r3,-30621
    cmpw r4,r0
    bge L_ein_7
    b L_ein_end
L_ein_4:
    addi r3,r30,0
    addi r4,r28,0
    addi r5,r31,0
    bl ARPIn
    b L_ein_end
L_ein_6:
    addi r3,r28,14
    lbz r0,26(r28)
    addi r4,r3,0
    cmplwi r0,127
    beq L_ein_end
    lbz r0,16(r4)
    cmplwi r0,127
    beq L_ein_end
    lwz r5,12(r4)
    lwz r7,16(r4)
    cmplw r5,r7
    beq L_ein_end
    lwz r0,68(r30)
    li r10,0
    lwz r8,IPAddrAny
    li r9,0
    cmplw r0,r8
    beq L_ein_8
    subf r3,r0,r5
    subf r0,r0,r7
    cntlzw r3,r3
    cntlzw r0,r0
    srwi r3,r3,5
    srwi r0,r0,5
    addi r9,r3,0
    mr r10,r0
L_ein_8:
    lwz r0,84(r30)
    cmplw r0,r8
    beq L_ein_9
    subf r3,r0,r5
    subf r0,r0,r7
    cntlzw r3,r3
    cntlzw r0,r0
    srwi r3,r3,5
    srwi r0,r0,5
    or r9,r9,r3
    or r10,r10,r0
L_ein_9:
    cmpwi r9,0
    beq L_ein_10
    cmpwi r10,0
    bne L_ein_end
L_ein_10:
    addi r3,r30,0
    addi r5,r31,-14
    bl IPIn
    b L_ein_end
L_ein_7:
    addi r3,r30,0
    addi r4,r28,0
    addi r5,r31,0
    bl PPPoEIn
L_ein_end:
    lwz r0,44(r1)
    lwz r31,36(r1)
    lwz r30,32(r1)
    lwz r29,28(r1)
    lwz r28,24(r1)
    addi r1,r1,40
    mtlr r0
    blr
}

static asm void* Callback0(u16 type, s32 length, u8 flags)
{
    nofralloc
    clrlwi r4,r3,16
    cmpwi r4,2054
    beq L_cb0_1
    bge L_cb0_2
    cmpwi r4,2048
    beq L_cb0_1
    b L_cb0_no
L_cb0_2:
    lis r3,1
    addi r0,r3,-30619
    cmpw r4,r0
    bge L_cb0_no
    addi r0,r3,-30621
    cmpw r4,r0
    bge L_cb0_1
    b L_cb0_no
L_cb0_1:
    lwz r0,Reset
    cmpwi r0,0
    bne L_cb0_no
    lis r3,RecvBuf@h
    addi r3,r3,RecvBuf@l
    blr
L_cb0_no:
    li r3,0
    blr
}

static asm void Callback1(u8* buf, s32 len)
{
    nofralloc
    mflr r0
    addi r5,r4,0
    stw r0,4(r1)
    cmpwi r5,18
    addi r4,r3,0
    stwu r1,-8(r1)
    blt L_cb1_end
    lis r3,__IFDefault@h
    addi r3,r3,__IFDefault@l
    addi r5,r5,-4
    bl ETHIn
L_cb1_end:
    lwz r0,12(r1)
    addi r1,r1,8
    mtlr r0
    blr
}

static asm u32 Go(void)
{
    nofralloc
    mflr r0
    lis r3,RecvBuf@h
    stw r0,4(r1)
    stwu r1,-48(r1)
    stmw r22,8(r1)
    addi r31,r3,RecvBuf@l
    lwz r0,19584(r31)
    cmpwi r0,5
    bge L_go_1
    cmpwi r0,0
    bge L_go_2
    b L_go_1
L_go_2:
    li r25,1
    b L_go_3
L_go_1:
    li r25,0
L_go_3:
    bl OSDisableInterrupts
    lwz r0,Loopback
    addi r27,r3,0
    cmpwi r0,0
    bne L_go_4
    lwz r0,Sending
    cmplwi r0,0
    bne L_go_4
    lwz r3,SendQueue
    cmplwi r3,0
    bne L_go_5
L_go_4:
    mr r3,r27
    bl OSRestoreInterrupts
    li r3,0
    b L_go_end
L_go_5:
    lwz r4,8(r3)
    addi r26,r3,0
    cmplwi r4,0
    bne L_go_6
    li r0,0
    li r3,SendQueue
    stw r0,4(r3)
    b L_go_7
L_go_6:
    li r0,0
    stw r0,12(r4)
L_go_7:
    stw r4,SendQueue
    li r3,0
    stw r3,4(r26)
    stw r26,Sending
    lwz r4,48(r26)
    cmpwi r4,1
    ble L_go_8
    addi r0,r4,-1
    slwi r3,r0,3
L_go_8:
    addi r28,r3,60
    addi r30,r31,19500
    addi r3,r30,0
    addi r4,r26,0
    addi r5,r28,0
    bl memcpy
    addi r4,r26,0
    addi r5,r28,0
    addi r3,r31,3056
    bl IFFifoFree
    lhz r0,16(r30)
    addi r26,r31,1536
    addi r3,r26,0
    sth r0,1548(r31)
    addi r4,r30,18
    li r5,6
    bl memmove
    addi r29,r31,19640
    addi r4,r29,0
    addi r3,r26,6
    li r5,6
    bl memmove
    lhz r0,16(r30)
    addi r24,r31,1550
    li r28,14
    cmplwi r0,34916
    bne L_go_9
    lbz r5,36(r30)
    cmplwi r5,0
    beq L_go_9
    addi r3,r24,0
    addi r4,r30,28
    bl memmove
    lbz r3,36(r30)
    addi r28,r3,14
    add r24,r24,r3
L_go_9:
    lwz r0,48(r30)
    addi r23,r30,52
    slwi r3,r0,3
    addi r22,r3,52
    add r22,r30,r22
    b L_go_11
L_go_10:
    lwz r4,0(r23)
    mr r3,r24
    lwz r5,4(r23)
    bl memmove
    lwz r5,4(r23)
    addi r3,r31,3056
    lwz r4,0(r23)
    add r28,r28,r5
    add r24,r24,r5
    bl IFFifoFree
    addi r23,r23,8
L_go_11:
    cmplw r23,r22
    blt L_go_10
    cmpwi r28,60
    bge L_go_12
    addi r3,r24,0
    subfic r5,r28,60
    li r4,0
    bl memset
L_go_12:
    lbz r0,37(r30)
    addi r5,r28,0
    addi r3,r31,19584
    clrrwi r0,r0,1
    stb r0,37(r30)
    addi r4,r31,1536
    lwz r12,19700(r31)
    mtlr r12
    blrl
    cmpwi r3,0
    bne L_go_13
    lbz r0,37(r30)
    ori r0,r0,1
    stb r0,37(r30)
L_go_13:
    lbz r0,37(r30)
    clrlwi. r0,r0,31
    beq L_go_14
    li r22,1
    b L_go_19
L_go_14:
    cmpwi r25,0
    beq L_go_18
    addi r4,r29,0
    addi r3,r30,18
    li r5,6
    bl memcmp
    cmpwi r3,0
    beq L_go_18
    stw r28,19496(r31)
    li r22,0
    lbz r0,0(r26)
    clrlwi. r0,r0,31
    beq L_go_15
    lwz r3,19732(r31)
    addi r0,r3,1
    stw r0,19732(r31)
    b L_go_16
L_go_15:
    lwz r3,19728(r31)
    addi r0,r3,1
    stw r0,19728(r31)
L_go_16:
    lis r3,GoCallback@h
    addi r5,r3,GoCallback@l
    addi r4,r28,0
    addi r3,r31,1536
    bl ETHSendAsync
    b L_go_19
L_go_18:
    li r22,1
L_go_19:
    mr r3,r27
    bl OSRestoreInterrupts
    mr r3,r22
L_go_end:
    lmw r22,8(r1)
    lwz r0,52(r1)
    addi r1,r1,48
    mtlr r0
    blr
}

static asm void GoCallback(u32 flags)
{
    nofralloc
    mflr r0
    lis r4,RecvBuf@h
    stw r0,4(r1)
    stwu r1,-56(r1)
    stmw r22,16(r1)
    addi r31,r4,RecvBuf@l
    addi r0,r31,1550
    mr r23,r0
    clrlwi r30,r3,28
    rlwinm r29,r3,0,24,24
    addi r28,r31,19500
    addi r27,r31,19640
    addi r26,r31,19456
    addi r22,r31,3072
L_gcb_loop:
    lwz r0,19584(r31)
    cmpwi r0,5
    bge L_gcb_1
    cmpwi r0,0
    bge L_gcb_2
    b L_gcb_1
L_gcb_2:
    li r0,1
    b L_gcb_3
L_gcb_1:
    li r0,0
L_gcb_3:
    cmpwi r0,0
    beq L_gcb_4
    li r3,LinkState
    bl ETHGetLinkStateAsync
    cmpwi r30,0
    beq L_gcb_5
    lwz r3,19744(r31)
    addi r0,r3,1
    stw r0,19744(r31)
L_gcb_5:
    cmpwi r29,0
    beq L_gcb_4
    lwz r3,19740(r31)
    addi r0,r3,1
    stw r0,19740(r31)
L_gcb_4:
    lwz r3,Sending
    addis r0,r3,1
    cmplwi r0,65535
    beq L_gcb_20
    lbz r0,37(r28)
    lwz r24,40(r28)
    clrlwi. r0,r0,31
    lwz r25,44(r28)
    beq L_gcb_6
    li r0,0
    stw r0,Loopback
    b L_gcb_10
L_gcb_6:
    addi r4,r27,0
    addi r3,r28,18
    li r5,6
    bl memcmp
    cmpwi r3,0
    bne L_gcb_7
    li r0,1
    stw r0,Loopback
    b L_gcb_10
L_gcb_7:
    lhz r0,16(r28)
    cmplwi r0,2048
    bne L_gcb_10
    addi r3,r28,18
    li r4,HwBroadcastAddr
    li r5,6
    bl memcmp
    cmpwi r3,0
    bne L_gcb_8
    li r0,2
    stw r0,Loopback
    b L_gcb_10
L_gcb_8:
    lbz r0,18(r28)
    cmplwi r0,1
    bne L_gcb_10
    li r0,3
    stw r0,Loopback
L_gcb_10:
    lwz r3,Sending
    cmplw r3,r22
    blt L_gcb_11
    cmplw r26,r3
    bgt L_gcb_12
L_gcb_11:
    li r0,0
    stw r0,0(r3)
L_gcb_12:
    cmplwi r24,0
    beq L_gcb_13
    addi r12,r24,0
    mtlr r12
    addi r3,r25,0
    li r4,0
    blrl
L_gcb_13:
    lwz r0,Loopback
    cmpwi r0,2
    beq L_gcb_14
    bge L_gcb_15
    cmpwi r0,1
    bge L_gcb_16
    b L_gcb_19
L_gcb_15:
    cmpwi r0,4
    bge L_gcb_19
    b L_gcb_17
L_gcb_16:
    lhz r5,2(r23)
    addi r4,r23,0
    addi r3,r31,19584
    li r6,4
    bl IPIn
    b L_gcb_19
L_gcb_14:
    lhz r5,2(r23)
    addi r4,r23,0
    addi r3,r31,19584
    li r6,5
    bl IPIn
    b L_gcb_19
L_gcb_17:
    lhz r5,2(r23)
    addi r4,r23,0
    addi r3,r31,19584
    li r6,6
    bl IPIn
L_gcb_19:
    li r0,0
    stw r0,Loopback
L_gcb_20:
    li r0,0
    stw r0,Sending
    bl Go
    cmpwi r3,0
    bne L_gcb_loop
    lmw r22,16(r1)
    lwz r0,60(r1)
    addi r1,r1,56
    mtlr r0
    blr
}

asm void ETHOut(void* ifp, void* p)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-32(r1)
    stw r31,28(r1)
    stw r30,24(r1)
    addi r30,r4,0
    stw r29,20(r1)
    addi r29,r3,0
    bl OSDisableInterrupts
    li r0,0
    stw r0,4(r30)
    mr r31,r3
    stw r29,0(r30)
    lhz r4,16(r30)
    cmpwi r4,2054
    beq L_eo_1
    bge L_eo_2
    cmpwi r4,2048
    beq L_eo_3
    b L_eo_end
L_eo_2:
    lis r3,1
    addi r0,r3,-30619
    cmpw r4,r0
    bge L_eo_end
    addi r0,r3,-30621
    cmpw r4,r0
    bge L_eo_1
    b L_eo_end
L_eo_1:
    li r3,SendQueue
    stw r3,4(r30)
    addi r4,r3,4
    lwz r3,4(r3)
    cmplwi r3,0
    bne L_eo_4
    stw r30,SendQueue
    b L_eo_5
L_eo_4:
    stw r30,8(r3)
L_eo_5:
    stw r3,12(r30)
    li r0,0
    stw r0,8(r30)
    stw r30,0(r4)
    b L_eo_end
L_eo_3:
    addi r3,r29,0
    addi r4,r30,24
    addi r5,r30,18
    bl ARPLookup
    cmpwi r3,-1
    beq L_eo_6
    blt L_eo_7
    b L_eo_7
L_eo_6:
    addi r3,r29,0
    addi r4,r30,0
    bl ARPHold
    b L_eo_end
L_eo_7:
    li r3,SendQueue
    stw r3,4(r30)
    addi r4,r3,4
    lwz r3,4(r3)
    cmplwi r3,0
    bne L_eo_8
    stw r30,SendQueue
    b L_eo_9
L_eo_8:
    stw r30,8(r3)
L_eo_9:
    stw r3,12(r30)
    li r0,0
    stw r0,8(r30)
    stw r30,0(r4)
L_eo_end:
    mr r3,r31
    bl OSRestoreInterrupts
    bl Go
    cmpwi r3,0
    beq L_eo_ret
    li r3,0
    bl GoCallback
L_eo_ret:
    lwz r0,36(r1)
    lwz r31,28(r1)
    lwz r30,24(r1)
    lwz r29,20(r1)
    addi r1,r1,32
    mtlr r0
    blr
}

static asm u32 Cancel(void* ifp, void* p)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-24(r1)
    stw r31,20(r1)
    mr r31,r4
    bl OSDisableInterrupts
    lwz r0,Sending
    cmplw r0,r31
    bne L_cancel_1
    li r0,-1
    stw r0,Sending
    li r0,0
    stw r0,0(r31)
    b L_cancel_4
L_cancel_1:
    lwz r4,4(r31)
    cmplwi r4,0
    beq L_cancel_4
    lwz r5,8(r31)
    lwz r6,12(r31)
    cmplwi r5,0
    bne L_cancel_2
    stw r6,4(r4)
    b L_cancel_3
L_cancel_2:
    stw r6,12(r5)
L_cancel_3:
    cmplwi r6,0
    bne L_cancel_5
    lwz r4,4(r31)
    stw r5,0(r4)
    b L_cancel_6
L_cancel_5:
    stw r5,8(r6)
L_cancel_6:
    li r0,0
    stw r0,4(r31)
    stw r0,0(r31)
L_cancel_4:
    bl OSRestoreInterrupts
    lwz r0,28(r1)
    lwz r31,20(r1)
    addi r1,r1,24
    mtlr r0
    blr
}

static asm void* Alloc(u32 size)
{
    nofralloc
    mflr r0
    lis r3,SendFifo@h
    stw r0,4(r1)
    addi r3,r3,SendFifo@l
    stwu r1,-8(r1)
    bl IFFifoAlloc
    lwz r0,12(r1)
    addi r1,r1,8
    mtlr r0
    blr
}

static asm void Free(void* p)
{
    nofralloc
    mflr r0
    lis r3,SendFifo@h
    stw r0,4(r1)
    addi r3,r3,SendFifo@l
    stwu r1,-8(r1)
    bl IFFifoFree
    lwz r0,12(r1)
    addi r1,r1,8
    mtlr r0
    blr
}

static asm void LinkCheckHandler(void)
{
    nofralloc
    mflr r0
    lis r3,RecvBuf@h
    stw r0,4(r1)
    stwu r1,-32(r1)
    stw r31,28(r1)
    addi r31,r3,RecvBuf@l
    li r3,LinkState
    stw r30,24(r1)
    addi r30,r31,19588
    stw r29,20(r1)
    lwz r29,19588(r31)
    bl ETHGetLinkStateAsync
    lwz r0,LinkState
    cmpwi r29,0
    stw r0,0(r30)
    bne L_lch_1
    lwz r0,LinkState
    cmpwi r0,1
    bne L_lch_1
    bl DHCPReboot
    lwz r3,19668(r31)
    lwz r0,IPAddrAny
    cmplw r3,r0
    beq L_lch_1
    bl IPAutoConfig
L_lch_1:
    cmpwi r29,0
    beq L_lch_end
    lwz r0,LinkState
    cmpwi r0,0
    bne L_lch_end
    addi r3,r31,19584
    li r4,-112
    bl IPSetConfigError
L_lch_end:
    lwz r0,36(r1)
    lwz r31,28(r1)
    lwz r30,24(r1)
    lwz r29,20(r1)
    addi r1,r1,32
    mtlr r0
    blr
}

asm u32 IFInit(s32 mode)
{
    nofralloc
    mflr r0
    lis r4,RecvBuf@h
    stw r0,4(r1)
    stwu r1,-32(r1)
    stw r31,28(r1)
    addi r31,r4,RecvBuf@l
    stw r30,24(r1)
    stw r29,20(r1)
    addi r29,r3,0
    lwz r0,initialized
    cmpwi r0,0
    beq L_ifi_1
    li r3,0
    bl IFMute
    li r3,1
    b L_ifi_end
L_ifi_1:
    li r0,1
    lwz r3,__IPVersion
    stw r0,initialized
    bl OSRegisterVersion
    cmpwi r29,2
    beq L_ifi_2
    bge L_ifi_3
    cmpwi r29,0
    beq L_ifi_0
    bge L_ifi_4
    b L_ifi_5
L_ifi_3:
    cmpwi r29,4
    beq L_ifi_6
    bge L_ifi_5
    b L_ifi_7
L_ifi_0:
    li r29,0
    b L_ifi_8
L_ifi_4:
    li r29,1
    b L_ifi_8
L_ifi_2:
    li r29,2
    b L_ifi_8
L_ifi_7:
    li r29,3
    b L_ifi_8
L_ifi_6:
    li r29,4
    b L_ifi_8
L_ifi_5:
    li r29,5
L_ifi_8:
    addi r3,r31,19600
    bl OSCreateAlarm
    li r30,0
    li r3,SendQueue
    stw r30,4(r3)
    addi r3,r31,3056
    addi r4,r31,3072
    stw r30,SendQueue
    li r5,16384
    bl IFFifoInit
    li r3,0
    li r4,0
    li r5,0
    bl IPInitRoute
    bl ARPInit
    li r0,1500
    stw r0,19648(r31)
    addi r3,r31,19660
    li r4,IPLimited
    li r5,4
    bl memcpy
    lis r3,ETHOut@h
    addi r0,r3,ETHOut@l
    lis r3,Cancel@h
    stw r0,19680(r31)
    addi r0,r3,Cancel@l
    lis r3,Alloc@h
    stw r0,19684(r31)
    addi r0,r3,Alloc@l
    lis r3,Free@h
    stw r0,19688(r31)
    addi r0,r3,Free@l
    lis r3,NullFilter@h
    stw r0,19692(r31)
    addi r0,r3,NullFilter@l
    stw r0,19700(r31)
    mr r3,r29
    stw r0,19696(r31)
    stw r30,19708(r31)
    stw r30,19704(r31)
    stw r30,Reset
    bl ETHInit
    cmpwi r3,1
    bne L_ifi_9
    stw r29,19584(r31)
    addi r3,r31,19640
    stw r30,LinkState
    stw r30,19588(r31)
    bl ETHGetMACAddr
    li r3,Protocols
    li r4,4
    bl ETHSetProtoType
    li r3,0
    bl IFMute
    addi r3,r31,19456
    bl OSCreateAlarm
    bl OSGetTime
    lis r5,0x8000
    lwz r0,248(r5)
    lis r5,0x1062
    addi r5,r5,0x4DD3
    srwi r0,r0,2
    mulhwu r0,r5,r0
    srwi r0,r0,6
    mulli r8,r0,250
    lis r5,LinkCheckHandler@h
    addi r9,r5,LinkCheckHandler@l
    addi r5,r3,0
    addi r6,r4,0
    addi r3,r31,19456
    li r7,0
    bl OSSetPeriodicAlarm
    addi r3,r31,19584
    bl IGMPInit
    b L_ifi_10
L_ifi_9:
    li r0,5
    stw r0,19584(r31)
    stw r30,LinkState
    stw r30,19588(r31)
L_ifi_10:
    lis r3,ResetFunctionInfo@h
    addi r3,r3,ResetFunctionInfo@l
    bl OSRegisterResetFunction
    li r3,1
L_ifi_end:
    lwz r0,36(r1)
    lwz r31,28(r1)
    lwz r30,24(r1)
    lwz r29,20(r1)
    addi r1,r1,32
    mtlr r0
    blr
}

asm u32 IFMute(u32 mute)
{
    nofralloc
    mflr r0
    lis r4,__IFDefault@h
    stw r0,4(r1)
    stwu r1,-24(r1)
    stw r31,20(r1)
    lwz r31,Mute
    stw r3,Mute
    lwz r0,__IFDefault@l(r4)
    cmpwi r0,5
    bge L_mute_end
    cmpwi r0,0
    bge L_mute_1
    b L_mute_end
L_mute_1:
    cmpwi r3,0
    beq L_mute_2
    cmpwi r31,0
    bne L_mute_2
    li r3,0
    li r4,0
    bl ETHSetRecvCallback
    bl ETHClearMulticastAddresses
    b L_mute_end
L_mute_2:
    cmpwi r3,0
    bne L_mute_end
    cmpwi r31,0
    beq L_mute_end
    lis r3,Callback0@h
    lis r4,Callback1@h
    addi r3,r3,Callback0@l
    addi r4,r4,Callback1@l
    bl ETHSetRecvCallback
L_mute_end:
    mr r3,r31
    lwz r0,28(r1)
    lwz r31,20(r1)
    addi r1,r1,24
    mtlr r0
    blr
}

static asm BOOL OnReset(BOOL final)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-24(r1)
    stw r31,20(r1)
    li r31,1
    stw r30,16(r1)
    addi r30,r3,0
    stw r31,Reset
    bl IGMPOnReset
    cmpwi r30,0
    beq L_or_1
    mr r3,r30
    bl UDPOnReset
    mr r3,r30
    bl TCPOnReset
    li r3,1
    b L_or_end
L_or_1:
    mr r3,r30
    bl UDPOnReset
    cmpwi r3,0
    beq L_or_zero
    mr r3,r30
    bl TCPOnReset
    cmpwi r3,0
    beq L_or_zero
    lwz r0,Sending
    cmplwi r0,0
    bne L_or_zero2
    lwz r0,SendQueue
    cmplwi r0,0
    beq L_or_2
L_or_zero2:
    li r3,0
    b L_or_end
L_or_2:
    lwz r4,Mute
    lis r3,__IFDefault@h
    stw r31,Mute
    lwz r0,__IFDefault@l(r3)
    cmpwi r0,5
    bge L_or_3
    cmpwi r0,0
    bge L_or_4
    b L_or_3
L_or_4:
    cmpwi r4,0
    bne L_or_3
    li r3,0
    li r4,0
    bl ETHSetRecvCallback
    bl ETHClearMulticastAddresses
L_or_3:
    li r3,1
    b L_or_end
L_or_zero:
    li r3,0
L_or_end:
    lwz r0,28(r1)
    lwz r31,20(r1)
    lwz r30,16(r1)
    addi r1,r1,24
    mtlr r0
    blr
}

