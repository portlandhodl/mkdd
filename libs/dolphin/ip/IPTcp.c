#include <dolphin/types.h>
#include <dolphin/ip/ip.h>
#include <dolphin/tcp/tcp.h>
#include <dolphin/os.h>

#include <string.h>

typedef struct IPInterface IPInterface;
typedef struct TCPInfoList {
    TCPSocket* head;
    TCPSocket* tail;
} TCPInfoList;

struct IPInterface {
    u8 unk00[0x40];
    s32 mtu;
    u8 unk44[0x24];
    void* (*alloc)(IPInterface* interface, u32 size);
    void (*free)(IPInterface* interface, void* packet, u32 size);
};

typedef struct TCPPacket {
    u32 unk00;
    u32 unk04;
    u8 unk08[29];
    u8 unk25;
    u8 unk26[2];
    u32 unk28;
    u32 unk2c;
    u32 unk30;
    u8* data;
    u32 len;
    u8 buf[];
} TCPPacket;

typedef struct TCPStat {
    u32 unk00;
    u32 rxPackets;
    u32 unk08;
    u32 unk0C;
    u32 unk10;
} TCPStats;

typedef struct TCPInfoAddr {
    u8 unk00[2];
    u16 port;
    u8 addr[4];
} TCPInfoAddr;

TCPStats TCPStat;
extern TCPInfoList TCPInfoQueue;

extern TCPSocket* TCPLookupInfo(IPHeader* ip, TCPHeader* tcp);
extern BOOL TCPTestTimeWait(IPInterface* interface, IPHeader* ip, TCPHeader* tcp);
extern void TCPStartTimeWait(IPInterface* interface, TCPSocket* so);
extern s32 TCPAbort(TCPSocket* so);
extern void TCPUpdateRtt(TCPSocket* so, s64 rtt);
extern s32 TCPSendIn(TCPSocket* so, s32 reset);
extern s32 TCPPeekOut(TCPSocket* so, u8* dst, s32 len, s32 peek);
extern s32 TCPRecvOut(TCPSocket* so);
extern void TCPDeleteSackHoles(TCPSocket* so, TCPHeader* tcp);
extern int TCPUpdateScoreboard(TCPSocket* so, TCPHeader* tcp, u8* opt, s32 len);
extern void TCPInitRtt(TCPSocket* so);
extern void TCPStopRxmitTimer(TCPSocket* so, TCPHeader* tcp);
extern s32 TCPMakeOption(TCPHeader* tcp, TCPSocket* so, u16 flag);
extern void TCPOutput(TCPSocket* so, u16 flag);
extern void TCPStartRxmitTimer(TCPSocket* so);

extern s32 IPIsBroadcastAddr(IPInterface* interface, const u8* addr);
extern s32 IPLookupInfo(TCPSocket** queue, const u8* dstAddr,
                        const u8* srcAddr, u16 dstPort, u16 srcPort, u32 flag);
extern s32 IPOut(void* packet);
extern void __IPWakeupPollingThreads(void);
extern s64 OSGetTime(void);
extern s64 __div2i(s64, s64);
extern u32 __IPRandom AT_ADDRESS(OS_BASE_CACHED | 0x30DC);
extern void OSCancelAlarm(OSAlarm* alarm);

extern s32 IFRingIn(u8* base, u32 offset, u32 len, u32 unused, u8* src, u32 size);
extern s32 IFRingInEx(u8* base, u32 offset, u32 len, u32 size, u32 limit,
                      u8* src, s32* outLen, void* sack, s32 arg9, s32 arg10);
extern s32 IFRingOut(u8* base, u32 offset, u32 pos, u32 unused, u8* dst, u32 len);
extern s32 IFRingPut(u8* base, u32 offset, u32 len, u32 unused, u32 size);

static s32 DoOption(TCPSocket* so, IPHeader* ip, TCPHeader* tcp, s32 unused);
static BOOL TCPTrimSegment(TCPSocket* so, TCPHeader* tcp, u16* flag);
static TCPSocket* DoListen(IPInterface* interface, TCPSocket* listener,
                           IPHeader* ip, TCPHeader* tcp);
static TCPSocket* DoSynSent(IPInterface* interface, TCPSocket* so,
                            IPHeader* ip, TCPHeader* tcp);
static void DoUrg(TCPSocket* so, TCPHeader* tcp);
static void DoData(IPInterface* interface, TCPSocket* so, IPHeader* ip,
                   TCPHeader* tcp, u16 flag);
static asm void Rotate(void)
{
    nofralloc
    nop
    blr
}

static asm void TCPIsn(void)
{
    nofralloc
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    blr
}

asm u16 TCPCheckSum(TCPSegment* segment, s32 count)
{
    nofralloc
LTCPCheckSum_0:
    lwz     r9,0(r3)
LTCPCheckSum_4:
    lwz     r0,4(r3)
LTCPCheckSum_8:
    lhz     r7,12(r9)
LTCPCheckSum_c:
    lhz     r6,14(r9)
LTCPCheckSum_10:
    lhz     r5,16(r9)
LTCPCheckSum_14:
    add     r10,r7,r6
LTCPCheckSum_18:
    lbz     r8,0(r9)
LTCPCheckSum_1c:
    add     r10,r10,r5
LTCPCheckSum_20:
    lhz     r6,18(r9)
LTCPCheckSum_24:
    lhz     r5,2(r9)
LTCPCheckSum_28:
    rlwinm  r7,r8,2,26,29
LTCPCheckSum_2c:
    add     r10,r10,r6
LTCPCheckSum_30:
    subf    r5,r7,r5
LTCPCheckSum_34:
    addi    r10,r10,6
LTCPCheckSum_38:
    add     r6,r9,r7
LTCPCheckSum_3c:
    add     r10,r10,r5
LTCPCheckSum_40:
    subf    r7,r7,r0
LTCPCheckSum_44:
    b LTCPCheckSum_58
LTCPCheckSum_48:
    lhz     r0,0(r6)
LTCPCheckSum_4c:
    addi    r6,r6,2
LTCPCheckSum_50:
    addi    r7,r7,-2
LTCPCheckSum_54:
    add     r10,r10,r0
LTCPCheckSum_58:
    cmpwi   r7,1
LTCPCheckSum_5c:
    bgt LTCPCheckSum_48
LTCPCheckSum_60:
    bne LTCPCheckSum_70
LTCPCheckSum_64:
    lbz     r0,0(r6)
LTCPCheckSum_68:
    slwi    r8,r0,8
LTCPCheckSum_6c:
    b LTCPCheckSum_74
LTCPCheckSum_70:
    li      r8,0
LTCPCheckSum_74:
    addic.  r4,r4,-1
LTCPCheckSum_78:
    bgt LTCPCheckSum_84
LTCPCheckSum_7c:
    add     r10,r10,r8
LTCPCheckSum_80:
    b LTCPCheckSum_c0
LTCPCheckSum_84:
    lwz     r5,12(r3)
LTCPCheckSum_88:
    addi    r3,r3,8
LTCPCheckSum_8c:
    cmpwi   r5,0
LTCPCheckSum_90:
    beq LTCPCheckSum_74
LTCPCheckSum_94:
    cmpwi   r7,1
LTCPCheckSum_98:
    lwz     r6,0(r3)
LTCPCheckSum_9c:
    bne LTCPCheckSum_b8
LTCPCheckSum_a0:
    lbz     r0,0(r6)
LTCPCheckSum_a4:
    addi    r7,r5,-1
LTCPCheckSum_a8:
    addi    r6,r6,1
LTCPCheckSum_ac:
    or      r8,r8,r0
LTCPCheckSum_b0:
    add     r10,r10,r8
LTCPCheckSum_b4:
    b LTCPCheckSum_58
LTCPCheckSum_b8:
    mr      r7,r5
LTCPCheckSum_bc:
    b LTCPCheckSum_58
LTCPCheckSum_c0:
    clrlwi  r3,r10,16
LTCPCheckSum_c4:
    srwi    r0,r10,16
LTCPCheckSum_c8:
    add     r0,r3,r0
LTCPCheckSum_cc:
    clrlwi  r3,r0,16
LTCPCheckSum_d0:
    srwi    r0,r0,16
LTCPCheckSum_d4:
    add     r0,r3,r0
LTCPCheckSum_d8:
    xori    r3,r0,65535
LTCPCheckSum_dc:
    blr
}

static asm void TCPDumpHeader(void)
{
    nofralloc
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    blr
}

static asm s32 DoOption(TCPSocket* so, IPHeader* ip, TCPHeader* tcp, s32 unused)
{
    nofralloc
LDoOption_e0:
    mflr    r0
LDoOption_e4:
    lis     r4,1
LDoOption_e8:
    stw     r0,4(r1)
LDoOption_ec:
    stwu    r1,-48(r1)
LDoOption_f0:
    stmw    r26,24(r1)
LDoOption_f4:
    mr      r29,r5
LDoOption_f8:
    addi    r28,r3,0
LDoOption_fc:
    addi    r30,r29,20
LDoOption_100:
    addi    r27,r4,-1
LDoOption_104:
    lhz     r0,12(r5)
LDoOption_108:
    rlwinm  r3,r0,22,26,29
LDoOption_10c:
    addi    r31,r3,-20
LDoOption_110:
    b LDoOption_1d8
LDoOption_114:
    cmplwi  r0,1
LDoOption_118:
    bne LDoOption_124
LDoOption_11c:
    li      r26,1
LDoOption_120:
    b LDoOption_134
LDoOption_124:
    lbz     r3,1(r30)
LDoOption_128:
    cmpwi   r3,2
LDoOption_12c:
    addi    r26,r3,0
LDoOption_130:
    blt LDoOption_1ec
LDoOption_134:
    cmpwi   r0,4
LDoOption_138:
    beq LDoOption_198
LDoOption_13c:
    bge LDoOption_14c
LDoOption_140:
    cmpwi   r0,2
LDoOption_144:
    beq LDoOption_158
LDoOption_148:
    b LDoOption_1d0
LDoOption_14c:
    cmpwi   r0,6
LDoOption_150:
    bge LDoOption_1d0
LDoOption_154:
    b LDoOption_1bc
LDoOption_158:
    cmpwi   r26,4
LDoOption_15c:
    bne LDoOption_1d0
LDoOption_160:
    lhz     r0,12(r29)
LDoOption_164:
    rlwinm. r0,r0,0,30,30
LDoOption_168:
    beq LDoOption_1d0
LDoOption_16c:
    lhz     r0,2(r30)
LDoOption_170:
    lwz     r3,244(r28)
LDoOption_174:
    cmpw    r0,r3
LDoOption_178:
    bge LDoOption_180
LDoOption_17c:
    b LDoOption_184
LDoOption_180:
    mr      r0,r3
LDoOption_184:
    stw     r0,244(r28)
LDoOption_188:
    lwz     r0,244(r28)
LDoOption_18c:
    stw     r0,640(r28)
LDoOption_190:
    stw     r27,644(r28)
LDoOption_194:
    b LDoOption_1d0
LDoOption_198:
    cmpwi   r26,2
LDoOption_19c:
    bne LDoOption_1d0
LDoOption_1a0:
    lhz     r0,12(r29)
LDoOption_1a4:
    rlwinm. r0,r0,0,30,30
LDoOption_1a8:
    beq LDoOption_1d0
LDoOption_1ac:
    lwz     r0,232(r28)
LDoOption_1b0:
    ori     r0,r0,8192
LDoOption_1b4:
    stw     r0,232(r28)
LDoOption_1b8:
    b LDoOption_1d0
LDoOption_1bc:
    addi    r3,r28,0
LDoOption_1c0:
    addi    r4,r29,0
LDoOption_1c4:
    addi    r5,r30,0
LDoOption_1c8:
    addi    r6,r26,0
LDoOption_1cc:
    bl TCPUpdateScoreboard
LDoOption_1d0:
    add     r30,r30,r26
LDoOption_1d4:
    subf    r31,r26,r31
LDoOption_1d8:
    cmpwi   r31,0
LDoOption_1dc:
    ble LDoOption_1ec
LDoOption_1e0:
    lbz     r0,0(r30)
LDoOption_1e4:
    cmplwi  r0,0
LDoOption_1e8:
    bne LDoOption_114
LDoOption_1ec:
    lmw     r26,24(r1)
LDoOption_1f0:
    li      r3,1
LDoOption_1f4:
    lwz     r0,52(r1)
LDoOption_1f8:
    addi    r1,r1,48
LDoOption_1fc:
    mtlr    r0
LDoOption_200:
    blr
}

static asm void TCPGetSegmentLength(void)
{
    nofralloc
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    blr
}

asm void TCPRespond(IPInterface* interface, TCPSocket* info, u8* dstAddr,
                u16 dstPort, u8* srcAddr, u16 srcPort, s32 seq, s32 ack,
                u16 flag, u16 win)
{
    nofralloc
LTCPRespond_204:
    mflr    r0
LTCPRespond_208:
    stw     r0,4(r1)
LTCPRespond_20c:
    stwu    r1,-96(r1)
LTCPRespond_210:
    stmw    r18,40(r1)
LTCPRespond_214:
    mr.     r19,r4
LTCPRespond_218:
    lhz     r25,106(r1)
LTCPRespond_21c:
    addi    r18,r3,0
LTCPRespond_220:
    lhz     r26,110(r1)
LTCPRespond_224:
    addi    r20,r5,0
LTCPRespond_228:
    addi    r21,r6,0
LTCPRespond_22c:
    addi    r31,r7,0
LTCPRespond_230:
    addi    r22,r8,0
LTCPRespond_234:
    addi    r23,r9,0
LTCPRespond_238:
    addi    r24,r10,0
LTCPRespond_23c:
    li      r27,0
LTCPRespond_240:
    beq LTCPRespond_2a4
LTCPRespond_244:
    lwz     r0,232(r19)
LTCPRespond_248:
    rlwinm. r0,r0,0,18,18
LTCPRespond_24c:
    beq LTCPRespond_2a4
LTCPRespond_250:
    lwz     r0,108(r19)
LTCPRespond_254:
    cmplwi  r0,0
LTCPRespond_258:
    beq LTCPRespond_2a4
LTCPRespond_25c:
    addi    r4,r19,132
LTCPRespond_260:
    addi    r0,r19,108
LTCPRespond_264:
    b LTCPRespond_278
LTCPRespond_268:
    lwz     r3,0(r4)
LTCPRespond_26c:
    cmplwi  r3,0
LTCPRespond_270:
    bne LTCPRespond_280
LTCPRespond_274:
    addi    r4,r4,-8
LTCPRespond_278:
    cmplw   r0,r4
LTCPRespond_27c:
    ble LTCPRespond_268
LTCPRespond_280:
    addi    r3,r19,108
LTCPRespond_284:
    addi    r0,r4,8
LTCPRespond_288:
    subf    r0,r3,r0
LTCPRespond_28c:
    srawi   r0,r0,3
LTCPRespond_290:
    addze   r0,r0
LTCPRespond_294:
    slwi    r3,r0,3
LTCPRespond_298:
    addi    r0,r3,5
LTCPRespond_29c:
    clrrwi  r27,r0,2
LTCPRespond_2a0:
    b LTCPRespond_2a8
LTCPRespond_2a4:
    li      r19,0
LTCPRespond_2a8:
    lwz     r12,104(r18)
LTCPRespond_2ac:
    addi    r28,r27,100
LTCPRespond_2b0:
    addi    r3,r18,0
LTCPRespond_2b4:
    mtlr    r12
LTCPRespond_2b8:
    addi    r4,r28,0
LTCPRespond_2bc:
    blrl
LTCPRespond_2c0:
    mr.     r29,r3
LTCPRespond_2c4:
    beq LTCPRespond_3b0
LTCPRespond_2c8:
    addi    r30,r29,60
LTCPRespond_2cc:
    addi    r4,r20,0
LTCPRespond_2d0:
    addi    r3,r30,16
LTCPRespond_2d4:
    li      r5,4
LTCPRespond_2d8:
    bl memmove
LTCPRespond_2dc:
    addi    r4,r31,0
LTCPRespond_2e0:
    addi    r3,r30,12
LTCPRespond_2e4:
    li      r5,4
LTCPRespond_2e8:
    bl memmove
LTCPRespond_2ec:
    li      r0,69
LTCPRespond_2f0:
    stb     r0,0(r30)
LTCPRespond_2f4:
    li      r31,0
LTCPRespond_2f8:
    li      r6,255
LTCPRespond_2fc:
    stb     r31,1(r30)
LTCPRespond_300:
    li      r3,6
LTCPRespond_304:
    clrlwi  r0,r25,26
LTCPRespond_308:
    lbz     r7,0(r30)
LTCPRespond_30c:
    addi    r4,r19,0
LTCPRespond_310:
    addi    r5,r25,0
LTCPRespond_314:
    rlwinm  r7,r7,2,26,29
LTCPRespond_318:
    add     r7,r7,r27
LTCPRespond_31c:
    addi    r7,r7,20
LTCPRespond_320:
    sth     r7,2(r30)
LTCPRespond_324:
    stb     r6,8(r30)
LTCPRespond_328:
    stb     r3,9(r30)
LTCPRespond_32c:
    sth     r31,6(r30)
LTCPRespond_330:
    lbz     r3,0(r30)
LTCPRespond_334:
    rlwinm  r3,r3,2,26,29
LTCPRespond_338:
    add     r19,r30,r3
LTCPRespond_33c:
    sth     r0,12(r19)
LTCPRespond_340:
    mr      r3,r19
LTCPRespond_344:
    bl TCPMakeOption
LTCPRespond_348:
    sth     r22,0(r19)
LTCPRespond_34c:
    li      r0,1
LTCPRespond_350:
    addi    r3,r29,0
LTCPRespond_354:
    sth     r21,2(r19)
LTCPRespond_358:
    stw     r23,4(r19)
LTCPRespond_35c:
    stw     r24,8(r19)
LTCPRespond_360:
    sth     r26,14(r19)
LTCPRespond_364:
    sth     r31,16(r19)
LTCPRespond_368:
    sth     r31,18(r19)
LTCPRespond_36c:
    stw     r0,48(r29)
LTCPRespond_370:
    stw     r30,52(r29)
LTCPRespond_374:
    lhz     r0,2(r30)
LTCPRespond_378:
    stw     r0,56(r29)
LTCPRespond_37c:
    stw     r31,40(r29)
LTCPRespond_380:
    stw     r31,44(r29)
LTCPRespond_384:
    stw     r31,4(r29)
LTCPRespond_388:
    stb     r31,37(r29)
LTCPRespond_38c:
    bl IPOut
LTCPRespond_390:
    cmpwi   r3,0
LTCPRespond_394:
    bge LTCPRespond_3b0
LTCPRespond_398:
    lwz     r12,108(r18)
LTCPRespond_39c:
    addi    r3,r18,0
LTCPRespond_3a0:
    addi    r4,r29,0
LTCPRespond_3a4:
    mtlr    r12
LTCPRespond_3a8:
    addi    r5,r28,0
LTCPRespond_3ac:
    blrl
LTCPRespond_3b0:
    lmw     r18,40(r1)
LTCPRespond_3b4:
    lwz     r0,100(r1)
LTCPRespond_3b8:
    addi    r1,r1,96
LTCPRespond_3bc:
    mtlr    r0
LTCPRespond_3c0:
    blr
}

static asm void DoReset(void)
{
    nofralloc
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    blr
}

static asm void DoAck(void)
{
    nofralloc
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    blr
}

static asm BOOL TCPTrimSegment(TCPSocket* so, TCPHeader* tcp, u16* flag)
{
    nofralloc
Ltrim_3c4:
    lwz     r0,100(r3)
Ltrim_3c8:
    cmpwi   r0,0
Ltrim_3cc:
    ble Ltrim_3d8
Ltrim_3d0:
    li      r0,16
Ltrim_3d4:
    b Ltrim_3dc
Ltrim_3d8:
    li      r0,0
Ltrim_3dc:
    sth     r0,0(r5)
Ltrim_3e0:
    lwz     r6,4(r4)
Ltrim_3e4:
    lwz     r0,84(r3)
Ltrim_3e8:
    subf.   r0,r6,r0
Ltrim_3ec:
    mr      r7,r0
Ltrim_3f0:
    ble Ltrim_4d4
Ltrim_3f4:
    lhz     r0,0(r5)
Ltrim_3f8:
    ori     r0,r0,16
Ltrim_3fc:
    sth     r0,0(r5)
Ltrim_400:
    lhz     r6,12(r4)
Ltrim_404:
    rlwinm. r0,r6,0,30,30
Ltrim_408:
    beq Ltrim_454
Ltrim_40c:
    rlwinm  r0,r6,0,31,29
Ltrim_410:
    sth     r0,12(r4)
Ltrim_414:
    lwz     r6,4(r4)
Ltrim_418:
    addi    r0,r6,1
Ltrim_41c:
    stw     r0,4(r4)
Ltrim_420:
    lwz     r6,100(r3)
Ltrim_424:
    addi    r0,r6,-1
Ltrim_428:
    stw     r0,100(r3)
Ltrim_42c:
    lhz     r6,18(r4)
Ltrim_430:
    cmplwi  r6,1
Ltrim_434:
    ble Ltrim_444
Ltrim_438:
    addi    r0,r6,-1
Ltrim_43c:
    sth     r0,18(r4)
Ltrim_440:
    b Ltrim_450
Ltrim_444:
    lhz     r0,12(r4)
Ltrim_448:
    rlwinm  r0,r0,0,27,25
Ltrim_44c:
    sth     r0,12(r4)
Ltrim_450:
    addi    r7,r7,-1
Ltrim_454:
    lwz     r0,100(r3)
Ltrim_458:
    cmpw    r0,r7
Ltrim_45c:
    bgt Ltrim_478
Ltrim_460:
    lhz     r6,12(r4)
Ltrim_464:
    clrlwi. r0,r6,31
Ltrim_468:
    beq Ltrim_474
Ltrim_46c:
    clrrwi  r0,r6,1
Ltrim_470:
    sth     r0,12(r4)
Ltrim_474:
    lwz     r7,100(r3)
Ltrim_478:
    cmpwi   r7,0
Ltrim_47c:
    ble Ltrim_4d4
Ltrim_480:
    lwz     r0,4(r4)
Ltrim_484:
    add     r0,r0,r7
Ltrim_488:
    stw     r0,4(r4)
Ltrim_48c:
    lwz     r0,100(r3)
Ltrim_490:
    subf    r0,r7,r0
Ltrim_494:
    stw     r0,100(r3)
Ltrim_498:
    lwz     r0,104(r3)
Ltrim_49c:
    add     r0,r0,r7
Ltrim_4a0:
    stw     r0,104(r3)
Ltrim_4a4:
    lhz     r6,18(r4)
Ltrim_4a8:
    cmpw    r7,r6
Ltrim_4ac:
    bge Ltrim_4c0
Ltrim_4b0:
    clrlwi  r0,r7,16
Ltrim_4b4:
    subf    r0,r0,r6
Ltrim_4b8:
    sth     r0,18(r4)
Ltrim_4bc:
    b Ltrim_4d4
Ltrim_4c0:
    lhz     r6,12(r4)
Ltrim_4c4:
    li      r0,0
Ltrim_4c8:
    rlwinm  r6,r6,0,27,25
Ltrim_4cc:
    sth     r6,12(r4)
Ltrim_4d0:
    sth     r0,18(r4)
Ltrim_4d4:
    lwz     r8,84(r3)
Ltrim_4d8:
    lwz     r7,88(r3)
Ltrim_4dc:
    lwz     r6,4(r4)
Ltrim_4e0:
    lwz     r0,100(r3)
Ltrim_4e4:
    add     r7,r8,r7
Ltrim_4e8:
    add     r0,r6,r0
Ltrim_4ec:
    subf.   r6,r7,r0
Ltrim_4f0:
    ble Ltrim_540
Ltrim_4f4:
    lhz     r0,0(r5)
Ltrim_4f8:
    ori     r0,r0,16
Ltrim_4fc:
    sth     r0,0(r5)
Ltrim_500:
    lhz     r5,12(r4)
Ltrim_504:
    clrlwi. r0,r5,31
Ltrim_508:
    beq Ltrim_524
Ltrim_50c:
    clrrwi  r0,r5,1
Ltrim_510:
    sth     r0,12(r4)
Ltrim_514:
    addi    r6,r6,-1
Ltrim_518:
    lwz     r5,100(r3)
Ltrim_51c:
    addi    r0,r5,-1
Ltrim_520:
    stw     r0,100(r3)
Ltrim_524:
    lwz     r0,100(r3)
Ltrim_528:
    cmpw    r0,r6
Ltrim_52c:
    bge Ltrim_534
Ltrim_530:
    mr      r6,r0
Ltrim_534:
    lwz     r0,100(r3)
Ltrim_538:
    subf    r0,r6,r0
Ltrim_53c:
    stw     r0,100(r3)
Ltrim_540:
    lwz     r5,88(r3)
Ltrim_544:
    cmpwi   r5,0
Ltrim_548:
    bne Ltrim_578
Ltrim_54c:
    lwz     r4,4(r4)
Ltrim_550:
    li      r5,0
Ltrim_554:
    lwz     r0,84(r3)
Ltrim_558:
    cmpw    r4,r0
Ltrim_55c:
    bne Ltrim_570
Ltrim_560:
    lwz     r0,100(r3)
Ltrim_564:
    cmpwi   r0,0
Ltrim_568:
    bne Ltrim_570
Ltrim_56c:
    li      r5,1
Ltrim_570:
    mr      r3,r5
Ltrim_574:
    blr
Ltrim_578:
    lwz     r7,84(r3)
Ltrim_57c:
    li      r6,0
Ltrim_580:
    lwz     r8,4(r4)
Ltrim_584:
    subf.   r0,r7,r8
Ltrim_588:
    blt Ltrim_59c
Ltrim_58c:
    add     r0,r7,r5
Ltrim_590:
    subf.   r0,r8,r0
Ltrim_594:
    ble Ltrim_59c
Ltrim_598:
    li      r6,1
Ltrim_59c:
    lwz     r4,100(r3)
Ltrim_5a0:
    cmpwi   r4,0
Ltrim_5a4:
    bne Ltrim_5b0
Ltrim_5a8:
    mr      r3,r6
Ltrim_5ac:
    blr
Ltrim_5b0:
    addi    r5,r4,-1
Ltrim_5b4:
    add     r5,r8,r5
Ltrim_5b8:
    subf.   r0,r7,r5
Ltrim_5bc:
    li      r4,0
Ltrim_5c0:
    blt Ltrim_5d8
Ltrim_5c4:
    lwz     r0,88(r3)
Ltrim_5c8:
    add     r0,r7,r0
Ltrim_5cc:
    subf.   r0,r5,r0
Ltrim_5d0:
    ble Ltrim_5d8
Ltrim_5d4:
    li      r4,1
Ltrim_5d8:
    mr      r3,r4
Ltrim_5dc:
    blr
}

s32 IFRingGet(s32 arg0, s32 arg1, void* ringData, s32 size,
              TCPSegment* fragments, s32 len);

static asm void TCPOpenWindow(void)
{
    nofralloc
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    blr
}

asm void TCPIn(IPInterface* interface, IPHeader* ip)
{
    nofralloc
LTCPIn_5e0:
    mflr    r0
LTCPIn_5e4:
    stw     r0,4(r1)
LTCPIn_5e8:
    li      r0,0
LTCPIn_5ec:
    stwu    r1,-96(r1)
LTCPIn_5f0:
    stmw    r25,68(r1)
LTCPIn_5f4:
    addi    r30,r4,0
LTCPIn_5f8:
    addi    r28,r3,0
LTCPIn_5fc:
    sth     r0,48(r1)
LTCPIn_600:
    lbz     r0,0(r4)
LTCPIn_604:
    rlwinm  r4,r0,2,26,29
LTCPIn_608:
    add     r3,r30,r4
LTCPIn_60c:
    lhz     r0,12(r3)
LTCPIn_610:
    addi    r31,r3,0
LTCPIn_614:
    rlwinm  r3,r0,0,16,19
LTCPIn_618:
    rlwinm  r0,r0,22,26,29
LTCPIn_61c:
    cmpwi   r0,20
LTCPIn_620:
    blt LTCPIn_1178
LTCPIn_624:
    srawi   r0,r3,10
LTCPIn_628:
    lhz     r3,2(r30)
LTCPIn_62c:
    add     r0,r4,r0
LTCPIn_630:
    cmpw    r3,r0
LTCPIn_634:
    blt LTCPIn_1178
LTCPIn_638:
    stw     r30,52(r1)
LTCPIn_63c:
    addi    r3,r1,52
LTCPIn_640:
    li      r4,1
LTCPIn_644:
    lhz     r0,2(r30)
LTCPIn_648:
    stw     r0,56(r1)
LTCPIn_64c:
    bl TCPCheckSum
LTCPIn_650:
    clrlwi. r0,r3,16
LTCPIn_654:
    bne LTCPIn_1178
LTCPIn_658:
    lis     r3,TCPStat@ha
LTCPIn_65c:
    addi    r6,r3,TCPStat@l
LTCPIn_660:
    lwz     r5,4(r6)
LTCPIn_664:
    addi    r3,r30,0
LTCPIn_668:
    addi    r4,r31,0
LTCPIn_66c:
    addi    r0,r5,1
LTCPIn_670:
    stw     r0,4(r6)
LTCPIn_674:
    bl TCPLookupInfo
LTCPIn_678:
    mr.     r29,r3
LTCPIn_67c:
    beq LTCPIn_68c
LTCPIn_680:
    lwz     r0,228(r29)
LTCPIn_684:
    cmpwi   r0,4
LTCPIn_688:
    bge LTCPIn_6a4
LTCPIn_68c:
    addi    r3,r28,0
LTCPIn_690:
    addi    r4,r30,0
LTCPIn_694:
    addi    r5,r31,0
LTCPIn_698:
    bl TCPTestTimeWait
LTCPIn_69c:
    cmpwi   r3,0
LTCPIn_6a0:
    bne LTCPIn_1178
LTCPIn_6a4:
    cmplwi  r29,0
LTCPIn_6a8:
    bne LTCPIn_73c
LTCPIn_6ac:
    lhz     r6,12(r31)
LTCPIn_6b0:
    rlwinm. r0,r6,0,29,29
LTCPIn_6b4:
    bne LTCPIn_1178
LTCPIn_6b8:
    rlwinm. r0,r6,0,27,27
LTCPIn_6bc:
    bne LTCPIn_704
LTCPIn_6c0:
    lbz     r4,0(r30)
LTCPIn_6c4:
    rlwinm. r0,r6,0,30,30
LTCPIn_6c8:
    lhz     r3,2(r30)
LTCPIn_6cc:
    rlwinm  r5,r6,22,26,29
LTCPIn_6d0:
    rlwinm  r4,r4,2,26,29
LTCPIn_6d4:
    subf    r0,r4,r3
LTCPIn_6d8:
    subf    r3,r5,r0
LTCPIn_6dc:
    li      r9,0
LTCPIn_6e0:
    beq LTCPIn_6e8
LTCPIn_6e4:
    addi    r3,r3,1
LTCPIn_6e8:
    clrlwi. r0,r6,31
LTCPIn_6ec:
    beq LTCPIn_6f4
LTCPIn_6f0:
    addi    r3,r3,1
LTCPIn_6f4:
    lwz     r0,4(r31)
LTCPIn_6f8:
    li      r4,20
LTCPIn_6fc:
    add     r10,r0,r3
LTCPIn_700:
    b LTCPIn_710
LTCPIn_704:
    lwz     r9,8(r31)
LTCPIn_708:
    li      r10,0
LTCPIn_70c:
    li      r4,4
LTCPIn_710:
    stw     r4,8(r1)
LTCPIn_714:
    li      r0,0
LTCPIn_718:
    addi    r3,r28,0
LTCPIn_71c:
    stw     r0,12(r1)
LTCPIn_720:
    addi    r5,r30,12
LTCPIn_724:
    addi    r7,r30,16
LTCPIn_728:
    lhz     r6,0(r31)
LTCPIn_72c:
    li      r4,0
LTCPIn_730:
    lhz     r8,2(r31)
LTCPIn_734:
    bl TCPRespond
LTCPIn_738:
    b LTCPIn_1178
LTCPIn_73c:
    stw     r28,40(r29)
LTCPIn_740:
    lwz     r0,228(r29)
LTCPIn_744:
    cmpwi   r0,0
LTCPIn_748:
    beq LTCPIn_1178
LTCPIn_74c:
    lhz     r6,12(r31)
LTCPIn_750:
    lbz     r4,0(r30)
LTCPIn_754:
    rlwinm. r0,r6,0,30,30
LTCPIn_758:
    lhz     r3,2(r30)
LTCPIn_75c:
    rlwinm  r4,r4,2,26,29
LTCPIn_760:
    rlwinm  r5,r6,22,26,29
LTCPIn_764:
    subf    r0,r4,r3
LTCPIn_768:
    subf    r3,r5,r0
LTCPIn_76c:
    beq LTCPIn_774
LTCPIn_770:
    addi    r3,r3,1
LTCPIn_774:
    clrlwi. r0,r6,31
LTCPIn_778:
    beq LTCPIn_780
LTCPIn_77c:
    addi    r3,r3,1
LTCPIn_780:
    stw     r3,100(r29)
LTCPIn_784:
    lhz     r0,12(r31)
LTCPIn_788:
    rlwinm  r0,r0,22,26,29
LTCPIn_78c:
    add     r0,r31,r0
LTCPIn_790:
    stw     r0,104(r29)
LTCPIn_794:
    lwz     r5,228(r29)
LTCPIn_798:
    cmpwi   r5,2
LTCPIn_79c:
    beq LTCPIn_824
LTCPIn_7a0:
    bge LTCPIn_7b0
LTCPIn_7a4:
    cmpwi   r5,1
LTCPIn_7a8:
    bge LTCPIn_7bc
LTCPIn_7ac:
    b LTCPIn_9d4
LTCPIn_7b0:
    cmpwi   r5,4
LTCPIn_7b4:
    bge LTCPIn_9d4
LTCPIn_7b8:
    b LTCPIn_88c
LTCPIn_7bc:
    addi    r3,r28,0
LTCPIn_7c0:
    addi    r4,r29,0
LTCPIn_7c4:
    addi    r5,r30,0
LTCPIn_7c8:
    addi    r6,r31,0
LTCPIn_7cc:
    bl DoListen
LTCPIn_7d0:
    mr.     r25,r3
LTCPIn_7d4:
    beq LTCPIn_1178
LTCPIn_7d8:
    addi    r3,r25,0
LTCPIn_7dc:
    addi    r4,r31,0
LTCPIn_7e0:
    addi    r5,r1,48
LTCPIn_7e4:
    bl TCPTrimSegment
LTCPIn_7e8:
    lhz     r3,12(r31)
LTCPIn_7ec:
    clrlwi. r0,r3,31
LTCPIn_7f0:
    beq LTCPIn_808
LTCPIn_7f4:
    clrrwi  r0,r3,1
LTCPIn_7f8:
    sth     r0,12(r31)
LTCPIn_7fc:
    lwz     r3,100(r25)
LTCPIn_800:
    addi    r0,r3,-1
LTCPIn_804:
    stw     r0,100(r25)
LTCPIn_808:
    addi    r3,r28,0
LTCPIn_80c:
    addi    r4,r25,0
LTCPIn_810:
    addi    r5,r30,0
LTCPIn_814:
    addi    r6,r31,0
LTCPIn_818:
    li      r7,0
LTCPIn_81c:
    bl DoData
LTCPIn_820:
    b LTCPIn_1178
LTCPIn_824:
    addi    r3,r28,0
LTCPIn_828:
    addi    r4,r29,0
LTCPIn_82c:
    addi    r5,r30,0
LTCPIn_830:
    addi    r6,r31,0
LTCPIn_834:
    bl DoSynSent
LTCPIn_838:
    cmplwi  r3,0
LTCPIn_83c:
    beq LTCPIn_1178
LTCPIn_840:
    addi    r3,r29,0
LTCPIn_844:
    addi    r4,r31,0
LTCPIn_848:
    addi    r5,r1,48
LTCPIn_84c:
    bl TCPTrimSegment
LTCPIn_850:
    lhz     r3,12(r31)
LTCPIn_854:
    clrlwi. r0,r3,31
LTCPIn_858:
    beq LTCPIn_870
LTCPIn_85c:
    clrrwi  r0,r3,1
LTCPIn_860:
    sth     r0,12(r31)
LTCPIn_864:
    lwz     r3,100(r29)
LTCPIn_868:
    addi    r0,r3,-1
LTCPIn_86c:
    stw     r0,100(r29)
LTCPIn_870:
    addi    r3,r28,0
LTCPIn_874:
    addi    r4,r29,0
LTCPIn_878:
    addi    r5,r30,0
LTCPIn_87c:
    addi    r6,r31,0
LTCPIn_880:
    li      r7,0
LTCPIn_884:
    bl DoData
LTCPIn_888:
    b LTCPIn_1178
LTCPIn_88c:
    lhz     r0,12(r31)
LTCPIn_890:
    rlwinm. r3,r0,0,27,27
LTCPIn_894:
    beq LTCPIn_93c
LTCPIn_898:
    lwz     r9,8(r31)
LTCPIn_89c:
    lwz     r3,48(r29)
LTCPIn_8a0:
    subf.   r3,r9,r3
LTCPIn_8a4:
    bge LTCPIn_8b4
LTCPIn_8a8:
    lwz     r3,80(r29)
LTCPIn_8ac:
    subf.   r3,r3,r9
LTCPIn_8b0:
    ble LTCPIn_93c
LTCPIn_8b4:
    rlwinm. r3,r0,0,29,29
LTCPIn_8b8:
    bne LTCPIn_1178
LTCPIn_8bc:
    rlwinm. r3,r0,0,27,27
LTCPIn_8c0:
    bne LTCPIn_908
LTCPIn_8c4:
    lbz     r5,0(r30)
LTCPIn_8c8:
    rlwinm. r3,r0,0,30,30
LTCPIn_8cc:
    lhz     r4,2(r30)
LTCPIn_8d0:
    rlwinm  r6,r0,22,26,29
LTCPIn_8d4:
    rlwinm  r5,r5,2,26,29
LTCPIn_8d8:
    subf    r3,r5,r4
LTCPIn_8dc:
    subf    r4,r6,r3
LTCPIn_8e0:
    li      r9,0
LTCPIn_8e4:
    beq LTCPIn_8ec
LTCPIn_8e8:
    addi    r4,r4,1
LTCPIn_8ec:
    clrlwi. r0,r0,31
LTCPIn_8f0:
    beq LTCPIn_8f8
LTCPIn_8f4:
    addi    r4,r4,1
LTCPIn_8f8:
    lwz     r0,4(r31)
LTCPIn_8fc:
    li      r3,20
LTCPIn_900:
    add     r10,r0,r4
LTCPIn_904:
    b LTCPIn_910
LTCPIn_908:
    li      r10,0
LTCPIn_90c:
    li      r3,4
LTCPIn_910:
    stw     r3,8(r1)
LTCPIn_914:
    li      r0,0
LTCPIn_918:
    addi    r3,r28,0
LTCPIn_91c:
    stw     r0,12(r1)
LTCPIn_920:
    addi    r5,r30,12
LTCPIn_924:
    addi    r7,r30,16
LTCPIn_928:
    lhz     r6,0(r31)
LTCPIn_92c:
    li      r4,0
LTCPIn_930:
    lhz     r8,2(r31)
LTCPIn_934:
    bl TCPRespond
LTCPIn_938:
    b LTCPIn_1178
LTCPIn_93c:
    lwz     r7,4(r31)
LTCPIn_940:
    lwz     r3,96(r29)
LTCPIn_944:
    subf.   r3,r7,r3
LTCPIn_948:
    ble LTCPIn_9d4
LTCPIn_94c:
    rlwinm. r3,r0,0,29,29
LTCPIn_950:
    bne LTCPIn_1178
LTCPIn_954:
    rlwinm. r3,r0,0,27,27
LTCPIn_958:
    bne LTCPIn_99c
LTCPIn_95c:
    lbz     r5,0(r30)
LTCPIn_960:
    rlwinm. r3,r0,0,30,30
LTCPIn_964:
    lhz     r4,2(r30)
LTCPIn_968:
    rlwinm  r6,r0,22,26,29
LTCPIn_96c:
    rlwinm  r5,r5,2,26,29
LTCPIn_970:
    subf    r3,r5,r4
LTCPIn_974:
    subf    r3,r6,r3
LTCPIn_978:
    li      r9,0
LTCPIn_97c:
    beq LTCPIn_984
LTCPIn_980:
    addi    r3,r3,1
LTCPIn_984:
    clrlwi. r0,r0,31
LTCPIn_988:
    beq LTCPIn_990
LTCPIn_98c:
    addi    r3,r3,1
LTCPIn_990:
    add     r10,r7,r3
LTCPIn_994:
    li      r0,20
LTCPIn_998:
    b LTCPIn_9a8
LTCPIn_99c:
    lwz     r9,8(r31)
LTCPIn_9a0:
    li      r10,0
LTCPIn_9a4:
    li      r0,4
LTCPIn_9a8:
    stw     r0,8(r1)
LTCPIn_9ac:
    li      r0,0
LTCPIn_9b0:
    addi    r3,r28,0
LTCPIn_9b4:
    stw     r0,12(r1)
LTCPIn_9b8:
    addi    r5,r30,12
LTCPIn_9bc:
    addi    r7,r30,16
LTCPIn_9c0:
    lhz     r6,0(r31)
LTCPIn_9c4:
    li      r4,0
LTCPIn_9c8:
    lhz     r8,2(r31)
LTCPIn_9cc:
    bl TCPRespond
LTCPIn_9d0:
    b LTCPIn_1178
LTCPIn_9d4:
    lhz     r0,12(r31)
LTCPIn_9d8:
    rlwinm. r0,r0,0,29,29
LTCPIn_9dc:
    beq LTCPIn_a64
LTCPIn_9e0:
    lwz     r3,84(r29)
LTCPIn_9e4:
    lwz     r4,4(r31)
LTCPIn_9e8:
    subf.   r0,r3,r4
LTCPIn_9ec:
    blt LTCPIn_1178
LTCPIn_9f0:
    lwz     r0,88(r29)
LTCPIn_9f4:
    add     r0,r3,r0
LTCPIn_9f8:
    subf.   r0,r4,r0
LTCPIn_9fc:
    ble LTCPIn_1178
LTCPIn_a00:
    cmpwi   r5,8
LTCPIn_a04:
    bge LTCPIn_a18
LTCPIn_a08:
    cmpwi   r5,3
LTCPIn_a0c:
    beq LTCPIn_a28
LTCPIn_a10:
    bge LTCPIn_a3c
LTCPIn_a14:
    b LTCPIn_a64
LTCPIn_a18:
    cmpwi   r5,10
LTCPIn_a1c:
    beq LTCPIn_1178
LTCPIn_a20:
    bge LTCPIn_a64
LTCPIn_a24:
    b LTCPIn_a50
LTCPIn_a28:
    li      r0,-11
LTCPIn_a2c:
    stw     r0,44(r29)
LTCPIn_a30:
    mr      r3,r29
LTCPIn_a34:
    bl TCPAbort
LTCPIn_a38:
    b LTCPIn_1178
LTCPIn_a3c:
    li      r0,-3
LTCPIn_a40:
    stw     r0,44(r29)
LTCPIn_a44:
    mr      r3,r29
LTCPIn_a48:
    bl TCPAbort
LTCPIn_a4c:
    b LTCPIn_1178
LTCPIn_a50:
    mr      r3,r29
LTCPIn_a54:
    bl TCPAbort
LTCPIn_a58:
    b LTCPIn_1178
LTCPIn_a5c:
    b LTCPIn_1178
LTCPIn_a60:
    b LTCPIn_1178
LTCPIn_a64:
    addi    r3,r29,0
LTCPIn_a68:
    addi    r4,r31,0
LTCPIn_a6c:
    addi    r5,r1,48
LTCPIn_a70:
    bl TCPTrimSegment
LTCPIn_a74:
    cmpwi   r3,0
LTCPIn_a78:
    bne LTCPIn_ac4
LTCPIn_a7c:
    lhz     r0,12(r31)
LTCPIn_a80:
    rlwinm. r0,r0,0,29,29
LTCPIn_a84:
    bne LTCPIn_1178
LTCPIn_a88:
    li      r0,16
LTCPIn_a8c:
    stw     r0,8(r1)
LTCPIn_a90:
    addi    r3,r28,0
LTCPIn_a94:
    addi    r4,r29,0
LTCPIn_a98:
    lwz     r0,88(r29)
LTCPIn_a9c:
    addi    r5,r30,12
LTCPIn_aa0:
    addi    r7,r30,16
LTCPIn_aa4:
    clrlwi  r0,r0,16
LTCPIn_aa8:
    stw     r0,12(r1)
LTCPIn_aac:
    lhz     r6,0(r31)
LTCPIn_ab0:
    lhz     r8,2(r31)
LTCPIn_ab4:
    lwz     r9,48(r29)
LTCPIn_ab8:
    lwz     r10,84(r29)
LTCPIn_abc:
    bl TCPRespond
LTCPIn_ac0:
    b LTCPIn_1178
LTCPIn_ac4:
    lhz     r3,12(r31)
LTCPIn_ac8:
    rlwinm. r0,r3,0,30,30
LTCPIn_acc:
    beq LTCPIn_b70
LTCPIn_ad0:
    li      r0,-3
LTCPIn_ad4:
    stw     r0,44(r29)
LTCPIn_ad8:
    lhz     r6,12(r31)
LTCPIn_adc:
    rlwinm. r0,r6,0,29,29
LTCPIn_ae0:
    bne LTCPIn_b64
LTCPIn_ae4:
    rlwinm. r0,r6,0,27,27
LTCPIn_ae8:
    bne LTCPIn_b30
LTCPIn_aec:
    lbz     r4,0(r30)
LTCPIn_af0:
    rlwinm. r0,r6,0,30,30
LTCPIn_af4:
    lhz     r3,2(r30)
LTCPIn_af8:
    rlwinm  r5,r6,22,26,29
LTCPIn_afc:
    rlwinm  r4,r4,2,26,29
LTCPIn_b00:
    subf    r0,r4,r3
LTCPIn_b04:
    subf    r3,r5,r0
LTCPIn_b08:
    li      r9,0
LTCPIn_b0c:
    beq LTCPIn_b14
LTCPIn_b10:
    addi    r3,r3,1
LTCPIn_b14:
    clrlwi. r0,r6,31
LTCPIn_b18:
    beq LTCPIn_b20
LTCPIn_b1c:
    addi    r3,r3,1
LTCPIn_b20:
    lwz     r0,4(r31)
LTCPIn_b24:
    li      r4,20
LTCPIn_b28:
    add     r10,r0,r3
LTCPIn_b2c:
    b LTCPIn_b3c
LTCPIn_b30:
    lwz     r9,8(r31)
LTCPIn_b34:
    li      r10,0
LTCPIn_b38:
    li      r4,4
LTCPIn_b3c:
    stw     r4,8(r1)
LTCPIn_b40:
    li      r0,0
LTCPIn_b44:
    addi    r3,r28,0
LTCPIn_b48:
    stw     r0,12(r1)
LTCPIn_b4c:
    addi    r5,r30,12
LTCPIn_b50:
    addi    r7,r30,16
LTCPIn_b54:
    lhz     r6,0(r31)
LTCPIn_b58:
    li      r4,0
LTCPIn_b5c:
    lhz     r8,2(r31)
LTCPIn_b60:
    bl TCPRespond
LTCPIn_b64:
    mr      r3,r29
LTCPIn_b68:
    bl TCPAbort
LTCPIn_b6c:
    b LTCPIn_1178
LTCPIn_b70:
    rlwinm. r0,r3,0,27,27
LTCPIn_b74:
    beq LTCPIn_1178
LTCPIn_b78:
    lwz     r0,232(r29)
LTCPIn_b7c:
    rlwinm. r0,r0,0,18,18
LTCPIn_b80:
    beq LTCPIn_b90
LTCPIn_b84:
    addi    r3,r29,0
LTCPIn_b88:
    addi    r4,r31,0
LTCPIn_b8c:
    bl TCPDeleteSackHoles
LTCPIn_b90:
    addi    r3,r29,0
LTCPIn_b94:
    addi    r4,r30,0
LTCPIn_b98:
    addi    r5,r31,0
LTCPIn_b9c:
    bl DoOption
LTCPIn_ba0:
    lwz     r0,228(r29)
LTCPIn_ba4:
    lwz     r4,48(r29)
LTCPIn_ba8:
    lwz     r3,8(r31)
LTCPIn_bac:
    cmpwi   r0,3
LTCPIn_bb0:
    subf    r27,r4,r3
LTCPIn_bb4:
    beq LTCPIn_bc8
LTCPIn_bb8:
    blt LTCPIn_10a0
LTCPIn_bbc:
    cmpwi   r0,10
LTCPIn_bc0:
    bge LTCPIn_10a0
LTCPIn_bc4:
    b LTCPIn_c40
LTCPIn_bc8:
    lwz     r0,72(r29)
LTCPIn_bcc:
    cmpw    r4,r0
LTCPIn_bd0:
    bne LTCPIn_bd8
LTCPIn_bd4:
    addi    r27,r27,-1
LTCPIn_bd8:
    lhz     r3,14(r31)
LTCPIn_bdc:
    li      r0,4
LTCPIn_be0:
    stw     r3,56(r29)
LTCPIn_be4:
    lwz     r3,4(r31)
LTCPIn_be8:
    stw     r3,64(r29)
LTCPIn_bec:
    lwz     r3,8(r31)
LTCPIn_bf0:
    stw     r3,68(r29)
LTCPIn_bf4:
    lwz     r3,56(r29)
LTCPIn_bf8:
    stw     r3,76(r29)
LTCPIn_bfc:
    stw     r0,228(r29)
LTCPIn_c00:
    lwz     r0,724(r29)
LTCPIn_c04:
    cmplwi  r0,0
LTCPIn_c08:
    beq LTCPIn_c40
LTCPIn_c0c:
    lwz     r3,728(r29)
LTCPIn_c10:
    cmplwi  r3,0
LTCPIn_c14:
    beq LTCPIn_c24
LTCPIn_c18:
    li      r0,0
LTCPIn_c1c:
    stw     r0,0(r3)
LTCPIn_c20:
    stw     r0,728(r29)
LTCPIn_c24:
    lwz     r12,724(r29)
LTCPIn_c28:
    li      r0,0
LTCPIn_c2c:
    addi    r3,r29,0
LTCPIn_c30:
    stw     r0,724(r29)
LTCPIn_c34:
    mtlr    r12
LTCPIn_c38:
    li      r4,0
LTCPIn_c3c:
    blrl
LTCPIn_c40:
    lwz     r3,80(r29)
LTCPIn_c44:
    lwz     r5,8(r31)
LTCPIn_c48:
    subf.   r0,r3,r5
LTCPIn_c4c:
    ble LTCPIn_c98
LTCPIn_c50:
    lhz     r0,12(r31)
LTCPIn_c54:
    rlwinm. r0,r0,0,29,29
LTCPIn_c58:
    bne LTCPIn_1178
LTCPIn_c5c:
    li      r0,16
LTCPIn_c60:
    stw     r0,8(r1)
LTCPIn_c64:
    addi    r3,r28,0
LTCPIn_c68:
    addi    r4,r29,0
LTCPIn_c6c:
    lwz     r0,88(r29)
LTCPIn_c70:
    addi    r5,r30,12
LTCPIn_c74:
    addi    r7,r30,16
LTCPIn_c78:
    clrlwi  r0,r0,16
LTCPIn_c7c:
    stw     r0,12(r1)
LTCPIn_c80:
    lhz     r6,0(r31)
LTCPIn_c84:
    lhz     r8,2(r31)
LTCPIn_c88:
    lwz     r9,48(r29)
LTCPIn_c8c:
    lwz     r10,84(r29)
LTCPIn_c90:
    bl TCPRespond
LTCPIn_c94:
    b LTCPIn_1178
LTCPIn_c98:
    lwz     r6,48(r29)
LTCPIn_c9c:
    subf.   r0,r5,r6
LTCPIn_ca0:
    blt LTCPIn_db4
LTCPIn_ca4:
    cmpw    r6,r3
LTCPIn_ca8:
    bne LTCPIn_cb8
LTCPIn_cac:
    li      r0,0
LTCPIn_cb0:
    stw     r0,524(r29)
LTCPIn_cb4:
    b LTCPIn_10a0
LTCPIn_cb8:
    lwz     r0,232(r29)
LTCPIn_cbc:
    rlwinm. r0,r0,0,18,18
LTCPIn_cc0:
    beq LTCPIn_10a0
LTCPIn_cc4:
    lwz     r4,524(r29)
LTCPIn_cc8:
    cmpwi   r4,3
LTCPIn_ccc:
    bge LTCPIn_10a0
LTCPIn_cd0:
    cmpw    r6,r5
LTCPIn_cd4:
    bne LTCPIn_ce8
LTCPIn_cd8:
    lhz     r3,14(r31)
LTCPIn_cdc:
    lwz     r0,56(r29)
LTCPIn_ce0:
    cmpw    r3,r0
LTCPIn_ce4:
    bge LTCPIn_cf4
LTCPIn_ce8:
    li      r0,0
LTCPIn_cec:
    stw     r0,524(r29)
LTCPIn_cf0:
    b LTCPIn_10a0
LTCPIn_cf4:
    lwz     r0,220(r29)
LTCPIn_cf8:
    subf.   r0,r5,r0
LTCPIn_cfc:
    blt LTCPIn_d0c
LTCPIn_d00:
    li      r0,0
LTCPIn_d04:
    stw     r0,524(r29)
LTCPIn_d08:
    b LTCPIn_10a0
LTCPIn_d0c:
    addi    r0,r4,1
LTCPIn_d10:
    cmpwi   r0,3
LTCPIn_d14:
    stw     r0,524(r29)
LTCPIn_d18:
    beq LTCPIn_d38
LTCPIn_d1c:
    lwz     r4,244(r29)
LTCPIn_d20:
    lwz     r3,48(r29)
LTCPIn_d24:
    lwz     r0,208(r29)
LTCPIn_d28:
    mulli   r4,r4,3
LTCPIn_d2c:
    subf    r0,r3,r0
LTCPIn_d30:
    cmpw    r4,r0
LTCPIn_d34:
    bge LTCPIn_10a0
LTCPIn_d38:
    lwz     r0,80(r29)
LTCPIn_d3c:
    stw     r0,220(r29)
LTCPIn_d40:
    lwz     r0,640(r29)
LTCPIn_d44:
    lwz     r3,56(r29)
LTCPIn_d48:
    cmpw    r0,r3
LTCPIn_d4c:
    bge LTCPIn_d54
LTCPIn_d50:
    b LTCPIn_d58
LTCPIn_d54:
    mr      r0,r3
LTCPIn_d58:
    srawi   r0,r0,1
LTCPIn_d5c:
    addze   r0,r0
LTCPIn_d60:
    stw     r0,644(r29)
LTCPIn_d64:
    lwz     r0,244(r29)
LTCPIn_d68:
    lwz     r3,644(r29)
LTCPIn_d6c:
    slwi    r0,r0,1
LTCPIn_d70:
    cmpw    r3,r0
LTCPIn_d74:
    ble LTCPIn_d7c
LTCPIn_d78:
    b LTCPIn_d80
LTCPIn_d7c:
    mr      r3,r0
LTCPIn_d80:
    stw     r3,644(r29)
LTCPIn_d84:
    li      r0,0
LTCPIn_d88:
    addi    r3,r29,600
LTCPIn_d8c:
    stw     r0,648(r29)
LTCPIn_d90:
    bl OSCancelAlarm
LTCPIn_d94:
    li      r0,3
LTCPIn_d98:
    stw     r0,524(r29)
LTCPIn_d9c:
    lwz     r0,232(r29)
LTCPIn_da0:
    ori     r0,r0,4096
LTCPIn_da4:
    stw     r0,232(r29)
LTCPIn_da8:
    lwz     r0,644(r29)
LTCPIn_dac:
    stw     r0,640(r29)
LTCPIn_db0:
    b LTCPIn_10a0
LTCPIn_db4:
    lwz     r0,232(r29)
LTCPIn_db8:
    rlwinm. r0,r0,0,18,18
LTCPIn_dbc:
    beq LTCPIn_e4c
LTCPIn_dc0:
    lwz     r3,524(r29)
LTCPIn_dc4:
    cmpwi   r3,3
LTCPIn_dc8:
    blt LTCPIn_e4c
LTCPIn_dcc:
    lwz     r0,220(r29)
LTCPIn_dd0:
    subf.   r0,r5,r0
LTCPIn_dd4:
    ble LTCPIn_dfc
LTCPIn_dd8:
    addi    r0,r3,1
LTCPIn_ddc:
    cmpwi   r3,3
LTCPIn_de0:
    stw     r0,524(r29)
LTCPIn_de4:
    bne LTCPIn_e54
LTCPIn_de8:
    addi    r3,r29,600
LTCPIn_dec:
    bl OSCancelAlarm
LTCPIn_df0:
    li      r0,0
LTCPIn_df4:
    stw     r0,648(r29)
LTCPIn_df8:
    b LTCPIn_e54
LTCPIn_dfc:
    lwz     r3,212(r29)
LTCPIn_e00:
    lwz     r0,244(r29)
LTCPIn_e04:
    lwz     r4,644(r29)
LTCPIn_e08:
    add     r0,r3,r0
LTCPIn_e0c:
    cmpw    r4,r0
LTCPIn_e10:
    bge LTCPIn_e18
LTCPIn_e14:
    b LTCPIn_e1c
LTCPIn_e18:
    mr      r4,r0
LTCPIn_e1c:
    stw     r4,640(r29)
LTCPIn_e20:
    lwz     r0,244(r29)
LTCPIn_e24:
    lwz     r3,640(r29)
LTCPIn_e28:
    cmpw    r0,r3
LTCPIn_e2c:
    ble LTCPIn_e34
LTCPIn_e30:
    b LTCPIn_e38
LTCPIn_e34:
    mr      r0,r3
LTCPIn_e38:
    stw     r0,640(r29)
LTCPIn_e3c:
    li      r0,0
LTCPIn_e40:
    stw     r0,524(r29)
LTCPIn_e44:
    stw     r0,648(r29)
LTCPIn_e48:
    b LTCPIn_e54
LTCPIn_e4c:
    addi    r3,r29,600
LTCPIn_e50:
    bl OSCancelAlarm
LTCPIn_e54:
    lwz     r0,524(r29)
LTCPIn_e58:
    cmpwi   r0,3
LTCPIn_e5c:
    bge LTCPIn_eac
LTCPIn_e60:
    lwz     r3,640(r29)
LTCPIn_e64:
    lwz     r0,644(r29)
LTCPIn_e68:
    cmpw    r3,r0
LTCPIn_e6c:
    bge LTCPIn_e80
LTCPIn_e70:
    lwz     r0,244(r29)
LTCPIn_e74:
    add     r0,r3,r0
LTCPIn_e78:
    stw     r0,640(r29)
LTCPIn_e7c:
    b LTCPIn_e94
LTCPIn_e80:
    lwz     r0,244(r29)
LTCPIn_e84:
    mullw   r0,r0,r0
LTCPIn_e88:
    divw    r0,r0,r3
LTCPIn_e8c:
    add     r0,r3,r0
LTCPIn_e90:
    stw     r0,640(r29)
LTCPIn_e94:
    lis     r3,1
LTCPIn_e98:
    lwz     r0,640(r29)
LTCPIn_e9c:
    addi    r3,r3,-1
LTCPIn_ea0:
    cmpw    r3,r0
LTCPIn_ea4:
    bge LTCPIn_eac
LTCPIn_ea8:
    stw     r3,640(r29)
LTCPIn_eac:
    lwz     r0,8(r31)
LTCPIn_eb0:
    stw     r0,48(r29)
LTCPIn_eb4:
    lwz     r0,52(r29)
LTCPIn_eb8:
    lwz     r3,48(r29)
LTCPIn_ebc:
    subf.   r0,r0,r3
LTCPIn_ec0:
    ble LTCPIn_ec8
LTCPIn_ec4:
    stw     r3,52(r29)
LTCPIn_ec8:
    lwz     r0,208(r29)
LTCPIn_ecc:
    lwz     r3,48(r29)
LTCPIn_ed0:
    subf.   r0,r0,r3
LTCPIn_ed4:
    ble LTCPIn_ef4
LTCPIn_ed8:
    stw     r3,208(r29)
LTCPIn_edc:
    lwz     r3,208(r29)
LTCPIn_ee0:
    lwz     r0,52(r29)
LTCPIn_ee4:
    lwz     r4,216(r29)
LTCPIn_ee8:
    subf    r0,r3,r0
LTCPIn_eec:
    add     r0,r4,r0
LTCPIn_ef0:
    stw     r0,212(r29)
LTCPIn_ef4:
    lwz     r0,472(r29)
LTCPIn_ef8:
    lwz     r6,384(r29)
LTCPIn_efc:
    cmplwi  r0,0
LTCPIn_f00:
    addi    r3,r6,0
LTCPIn_f04:
    beq LTCPIn_f18
LTCPIn_f08:
    lwz     r0,488(r29)
LTCPIn_f0c:
    cmpwi   r0,0
LTCPIn_f10:
    ble LTCPIn_f18
LTCPIn_f14:
    add     r3,r3,r0
LTCPIn_f18:
    cmpw    r3,r27
LTCPIn_f1c:
    bge LTCPIn_f2c
LTCPIn_f20:
    li      r26,1
LTCPIn_f24:
    addi    r27,r27,-1
LTCPIn_f28:
    b LTCPIn_f30
LTCPIn_f2c:
    li      r26,0
LTCPIn_f30:
    cmpwi   r27,0
LTCPIn_f34:
    ble LTCPIn_f7c
LTCPIn_f38:
    cmpwi   r6,0
LTCPIn_f3c:
    ble LTCPIn_f7c
LTCPIn_f40:
    cmpw    r27,r6
LTCPIn_f44:
    bge LTCPIn_f50
LTCPIn_f48:
    mr      r25,r27
LTCPIn_f4c:
    b LTCPIn_f54
LTCPIn_f50:
    mr      r25,r6
LTCPIn_f54:
    lwz     r3,372(r29)
LTCPIn_f58:
    mr      r7,r25
LTCPIn_f5c:
    lwz     r4,376(r29)
LTCPIn_f60:
    lwz     r5,380(r29)
LTCPIn_f64:
    bl IFRingPut
LTCPIn_f68:
    stw     r3,380(r29)
LTCPIn_f6c:
    subf    r27,r25,r27
LTCPIn_f70:
    lwz     r0,384(r29)
LTCPIn_f74:
    subf    r0,r25,r0
LTCPIn_f78:
    stw     r0,384(r29)
LTCPIn_f7c:
    cmpwi   r27,0
LTCPIn_f80:
    ble LTCPIn_fc0
LTCPIn_f84:
    lwz     r0,472(r29)
LTCPIn_f88:
    cmplwi  r0,0
LTCPIn_f8c:
    beq LTCPIn_fc0
LTCPIn_f90:
    lwz     r0,488(r29)
LTCPIn_f94:
    cmpwi   r0,0
LTCPIn_f98:
    ble LTCPIn_fc0
LTCPIn_f9c:
    lwz     r0,484(r29)
LTCPIn_fa0:
    add     r0,r0,r27
LTCPIn_fa4:
    stw     r0,484(r29)
LTCPIn_fa8:
    lwz     r0,488(r29)
LTCPIn_fac:
    subf    r0,r27,r0
LTCPIn_fb0:
    stw     r0,488(r29)
LTCPIn_fb4:
    lwz     r0,480(r29)
LTCPIn_fb8:
    add     r0,r0,r27
LTCPIn_fbc:
    stw     r0,480(r29)
LTCPIn_fc0:
    addi    r3,r29,0
LTCPIn_fc4:
    li      r4,0
LTCPIn_fc8:
    bl TCPSendIn
LTCPIn_fcc:
    lha     r0,4(r29)
LTCPIn_fd0:
    cmpwi   r0,0
LTCPIn_fd4:
    ble LTCPIn_fdc
LTCPIn_fd8:
    bl __IPWakeupPollingThreads
LTCPIn_fdc:
    lwz     r12,472(r29)
LTCPIn_fe0:
    cmplwi  r12,0
LTCPIn_fe4:
    beq LTCPIn_1028
LTCPIn_fe8:
    lwz     r0,488(r29)
LTCPIn_fec:
    cmpwi   r0,0
LTCPIn_ff0:
    bgt LTCPIn_1028
LTCPIn_ff4:
    lwz     r4,476(r29)
LTCPIn_ff8:
    cmplwi  r4,0
LTCPIn_ffc:
    beq LTCPIn_1010
LTCPIn_1000:
    lwz     r3,480(r29)
LTCPIn_1004:
    li      r0,0
LTCPIn_1008:
    stw     r3,0(r4)
LTCPIn_100c:
    stw     r0,476(r29)
LTCPIn_1010:
    li      r0,0
LTCPIn_1014:
    mtlr    r12
LTCPIn_1018:
    stw     r0,472(r29)
LTCPIn_101c:
    addi    r3,r29,0
LTCPIn_1020:
    lwz     r4,480(r29)
LTCPIn_1024:
    blrl
LTCPIn_1028:
    lwz     r0,228(r29)
LTCPIn_102c:
    cmpwi   r0,8
LTCPIn_1030:
    beq LTCPIn_1064
LTCPIn_1034:
    bge LTCPIn_1044
LTCPIn_1038:
    cmpwi   r0,5
LTCPIn_103c:
    beq LTCPIn_1050
LTCPIn_1040:
    b LTCPIn_10a0
LTCPIn_1044:
    cmpwi   r0,10
LTCPIn_1048:
    bge LTCPIn_10a0
LTCPIn_104c:
    b LTCPIn_1084
LTCPIn_1050:
    cmpwi   r26,0
LTCPIn_1054:
    beq LTCPIn_10a0
LTCPIn_1058:
    li      r0,6
LTCPIn_105c:
    stw     r0,228(r29)
LTCPIn_1060:
    b LTCPIn_10a0
LTCPIn_1064:
    cmpwi   r26,0
LTCPIn_1068:
    beq LTCPIn_10a0
LTCPIn_106c:
    li      r0,0
LTCPIn_1070:
    stw     r0,44(r29)
LTCPIn_1074:
    addi    r3,r28,0
LTCPIn_1078:
    addi    r4,r29,0
LTCPIn_107c:
    bl TCPStartTimeWait
LTCPIn_1080:
    b LTCPIn_1178
LTCPIn_1084:
    cmpwi   r26,0
LTCPIn_1088:
    beq LTCPIn_10a0
LTCPIn_108c:
    li      r0,0
LTCPIn_1090:
    stw     r0,44(r29)
LTCPIn_1094:
    mr      r3,r29
LTCPIn_1098:
    bl TCPAbort
LTCPIn_109c:
    b LTCPIn_1178
LTCPIn_10a0:
    lwz     r0,648(r29)
LTCPIn_10a4:
    cmpwi   r0,0
LTCPIn_10a8:
    beq LTCPIn_10e0
LTCPIn_10ac:
    lwz     r3,652(r29)
LTCPIn_10b0:
    lwz     r0,8(r31)
LTCPIn_10b4:
    subf.   r0,r3,r0
LTCPIn_10b8:
    ble LTCPIn_10e0
LTCPIn_10bc:
    li      r0,0
LTCPIn_10c0:
    stw     r0,648(r29)
LTCPIn_10c4:
    bl OSGetTime
LTCPIn_10c8:
    lwz     r5,660(r29)
LTCPIn_10cc:
    lwz     r0,656(r29)
LTCPIn_10d0:
    subfc   r6,r5,r4
LTCPIn_10d4:
    subfe   r5,r0,r3
LTCPIn_10d8:
    addi    r3,r29,0
LTCPIn_10dc:
    bl TCPUpdateRtt
LTCPIn_10e0:
    lwz     r4,64(r29)
LTCPIn_10e4:
    lwz     r3,4(r31)
LTCPIn_10e8:
    subf.   r0,r4,r3
LTCPIn_10ec:
    bgt LTCPIn_1128
LTCPIn_10f0:
    cmpw    r4,r3
LTCPIn_10f4:
    bne LTCPIn_1108
LTCPIn_10f8:
    lwz     r3,68(r29)
LTCPIn_10fc:
    lwz     r0,8(r31)
LTCPIn_1100:
    subf.   r0,r3,r0
LTCPIn_1104:
    bge LTCPIn_1128
LTCPIn_1108:
    lwz     r3,68(r29)
LTCPIn_110c:
    lwz     r0,8(r31)
LTCPIn_1110:
    cmpw    r3,r0
LTCPIn_1114:
    bne LTCPIn_1154
LTCPIn_1118:
    lwz     r3,56(r29)
LTCPIn_111c:
    lhz     r0,14(r31)
LTCPIn_1120:
    cmpw    r3,r0
LTCPIn_1124:
    bge LTCPIn_1154
LTCPIn_1128:
    lhz     r0,14(r31)
LTCPIn_112c:
    stw     r0,56(r29)
LTCPIn_1130:
    lwz     r0,4(r31)
LTCPIn_1134:
    stw     r0,64(r29)
LTCPIn_1138:
    lwz     r0,8(r31)
LTCPIn_113c:
    stw     r0,68(r29)
LTCPIn_1140:
    lwz     r0,76(r29)
LTCPIn_1144:
    lwz     r3,56(r29)
LTCPIn_1148:
    cmpw    r0,r3
LTCPIn_114c:
    bge LTCPIn_1154
LTCPIn_1150:
    stw     r3,76(r29)
LTCPIn_1154:
    addi    r3,r29,0
LTCPIn_1158:
    addi    r4,r31,0
LTCPIn_115c:
    bl TCPStopRxmitTimer
LTCPIn_1160:
    lhz     r7,48(r1)
LTCPIn_1164:
    addi    r3,r28,0
LTCPIn_1168:
    addi    r4,r29,0
LTCPIn_116c:
    addi    r5,r30,0
LTCPIn_1170:
    addi    r6,r31,0
LTCPIn_1174:
    bl DoData
LTCPIn_1178:
    lmw     r25,68(r1)
LTCPIn_117c:
    lwz     r0,100(r1)
LTCPIn_1180:
    addi    r1,r1,96
LTCPIn_1184:
    mtlr    r0
LTCPIn_1188:
    blr
}

static asm TCPSocket* DoListen(IPInterface* interface, TCPSocket* listener, IPHeader* ip,
                    TCPHeader* tcp)
{
    nofralloc
LDoListen_118c:
    mflr    r0
LDoListen_1190:
    stw     r0,4(r1)
LDoListen_1194:
    stwu    r1,-120(r1)
LDoListen_1198:
    stmw    r26,96(r1)
LDoListen_119c:
    addi    r26,r3,0
LDoListen_11a0:
    addi    r28,r5,0
LDoListen_11a4:
    addi    r27,r4,0
LDoListen_11a8:
    addi    r29,r6,0
LDoListen_11ac:
    lwz     r3,40(r4)
LDoListen_11b0:
    addi    r4,r28,12
LDoListen_11b4:
    bl IPIsBroadcastAddr
LDoListen_11b8:
    cmpwi   r3,0
LDoListen_11bc:
    bne LDoListen_11d0
LDoListen_11c0:
    lbz     r0,12(r28)
LDoListen_11c4:
    rlwinm  r0,r0,0,24,27
LDoListen_11c8:
    cmpwi   r0,224
LDoListen_11cc:
    bne LDoListen_11d8
LDoListen_11d0:
    li      r3,0
LDoListen_11d4:
    b LDoListen_15ec
LDoListen_11d8:
    lhz     r7,12(r29)
LDoListen_11dc:
    rlwinm. r3,r7,0,29,29
LDoListen_11e0:
    beq LDoListen_11ec
LDoListen_11e4:
    li      r3,0
LDoListen_11e8:
    b LDoListen_15ec
LDoListen_11ec:
    rlwinm. r0,r7,0,27,27
LDoListen_11f0:
    beq LDoListen_1284
LDoListen_11f4:
    cmpwi   r3,0
LDoListen_11f8:
    lwz     r3,40(r27)
LDoListen_11fc:
    bne LDoListen_127c
LDoListen_1200:
    cmpwi   r0,0
LDoListen_1204:
    bne LDoListen_124c
LDoListen_1208:
    lbz     r5,0(r28)
LDoListen_120c:
    rlwinm. r0,r7,0,30,30
LDoListen_1210:
    lhz     r4,2(r28)
LDoListen_1214:
    rlwinm  r6,r7,22,26,29
LDoListen_1218:
    rlwinm  r5,r5,2,26,29
LDoListen_121c:
    subf    r0,r5,r4
LDoListen_1220:
    subf    r5,r6,r0
LDoListen_1224:
    li      r9,0
LDoListen_1228:
    beq LDoListen_1230
LDoListen_122c:
    addi    r5,r5,1
LDoListen_1230:
    clrlwi. r0,r7,31
LDoListen_1234:
    beq LDoListen_123c
LDoListen_1238:
    addi    r5,r5,1
LDoListen_123c:
    lwz     r0,4(r29)
LDoListen_1240:
    li      r4,20
LDoListen_1244:
    add     r10,r0,r5
LDoListen_1248:
    b LDoListen_1258
LDoListen_124c:
    lwz     r9,8(r29)
LDoListen_1250:
    li      r10,0
LDoListen_1254:
    li      r4,4
LDoListen_1258:
    stw     r4,8(r1)
LDoListen_125c:
    li      r0,0
LDoListen_1260:
    addi    r5,r28,12
LDoListen_1264:
    stw     r0,12(r1)
LDoListen_1268:
    addi    r7,r28,16
LDoListen_126c:
    li      r4,0
LDoListen_1270:
    lhz     r6,0(r29)
LDoListen_1274:
    lhz     r8,2(r29)
LDoListen_1278:
    bl TCPRespond
LDoListen_127c:
    li      r3,0
LDoListen_1280:
    b LDoListen_15ec
LDoListen_1284:
    rlwinm. r0,r7,0,30,30
LDoListen_1288:
    bne LDoListen_1294
LDoListen_128c:
    li      r3,0
LDoListen_1290:
    b LDoListen_15ec
LDoListen_1294:
    lwz     r3,700(r27)
LDoListen_1298:
    cmplwi  r3,0
LDoListen_129c:
    beq LDoListen_12bc
LDoListen_12a0:
    addi    r3,r3,4
LDoListen_12a4:
    addi    r4,r28,16
LDoListen_12a8:
    li      r5,4
LDoListen_12ac:
    bl memmove
LDoListen_12b0:
    lhz     r0,2(r29)
LDoListen_12b4:
    lwz     r3,700(r27)
LDoListen_12b8:
    sth     r0,2(r3)
LDoListen_12bc:
    lwz     r3,704(r27)
LDoListen_12c0:
    cmplwi  r3,0
LDoListen_12c4:
    beq LDoListen_12e4
LDoListen_12c8:
    addi    r3,r3,4
LDoListen_12cc:
    addi    r4,r28,12
LDoListen_12d0:
    li      r5,4
LDoListen_12d4:
    bl memmove
LDoListen_12d8:
    lhz     r0,0(r29)
LDoListen_12dc:
    lwz     r3,704(r27)
LDoListen_12e0:
    sth     r0,2(r3)
LDoListen_12e4:
    lwz     r0,724(r27)
LDoListen_12e8:
    cmplwi  r0,0
LDoListen_12ec:
    beq LDoListen_1328
LDoListen_12f0:
    lwz     r3,728(r27)
LDoListen_12f4:
    cmplwi  r3,0
LDoListen_12f8:
    beq LDoListen_1304
LDoListen_12fc:
    li      r0,0
LDoListen_1300:
    stw     r0,0(r3)
LDoListen_1304:
    lwz     r12,724(r27)
LDoListen_1308:
    addi    r3,r27,0
LDoListen_130c:
    li      r4,0
LDoListen_1310:
    mtlr    r12
LDoListen_1314:
    blrl
LDoListen_1318:
    cmpwi   r3,0
LDoListen_131c:
    bne LDoListen_1328
LDoListen_1320:
    li      r3,0
LDoListen_1324:
    b LDoListen_15ec
LDoListen_1328:
    lwz     r3,708(r27)
LDoListen_132c:
    cmplwi  r3,0
LDoListen_1330:
    bne LDoListen_133c
LDoListen_1334:
    li      r3,0
LDoListen_1338:
    b LDoListen_15ec
LDoListen_133c:
    lwz     r4,716(r3)
LDoListen_1340:
    addi    r30,r3,0
LDoListen_1344:
    cmplwi  r4,0
LDoListen_1348:
    bne LDoListen_1358
LDoListen_134c:
    li      r0,0
LDoListen_1350:
    stw     r0,712(r27)
LDoListen_1354:
    b LDoListen_1360
LDoListen_1358:
    li      r0,0
LDoListen_135c:
    stw     r0,720(r4)
LDoListen_1360:
    stw     r4,708(r27)
LDoListen_1364:
    li      r31,0
LDoListen_1368:
    addi    r3,r30,0
LDoListen_136c:
    stw     r31,696(r30)
LDoListen_1370:
    stw     r26,40(r30)
LDoListen_1374:
    stw     r31,44(r30)
LDoListen_1378:
    bl TCPInitRtt
LDoListen_137c:
    lis     r3,-3
LDoListen_1380:
    lwz     r4,232(r30)
LDoListen_1384:
    addi    r0,r3,-131
LDoListen_1388:
    and     r0,r4,r0
LDoListen_138c:
    stw     r0,232(r30)
LDoListen_1390:
    lis     r3,3
LDoListen_1394:
    addi    r0,r3,130
LDoListen_1398:
    lwz     r5,232(r27)
LDoListen_139c:
    addi    r3,r30,20
LDoListen_13a0:
    lwz     r6,232(r30)
LDoListen_13a4:
    addi    r4,r28,12
LDoListen_13a8:
    and     r0,r5,r0
LDoListen_13ac:
    or      r0,r6,r0
LDoListen_13b0:
    stw     r0,232(r30)
LDoListen_13b4:
    li      r5,4
LDoListen_13b8:
    lwz     r0,232(r30)
LDoListen_13bc:
    ori     r0,r0,256
LDoListen_13c0:
    stw     r0,232(r30)
LDoListen_13c4:
    lwz     r0,732(r27)
LDoListen_13c8:
    stw     r0,732(r30)
LDoListen_13cc:
    lwz     r0,372(r30)
LDoListen_13d0:
    stw     r0,380(r30)
LDoListen_13d4:
    stw     r31,384(r30)
LDoListen_13d8:
    lwz     r0,504(r30)
LDoListen_13dc:
    stw     r0,516(r30)
LDoListen_13e0:
    lwz     r0,508(r30)
LDoListen_13e4:
    stw     r0,88(r30)
LDoListen_13e8:
    stw     r31,512(r30)
LDoListen_13ec:
    stw     r31,484(r30)
LDoListen_13f0:
    stw     r31,488(r30)
LDoListen_13f4:
    stw     r31,536(r30)
LDoListen_13f8:
    stw     r31,540(r30)
LDoListen_13fc:
    stw     r31,544(r30)
LDoListen_1400:
    bl memmove
LDoListen_1404:
    addi    r3,r30,12
LDoListen_1408:
    addi    r4,r28,16
LDoListen_140c:
    li      r5,4
LDoListen_1410:
    bl memmove
LDoListen_1414:
    lhz     r0,0(r29)
LDoListen_1418:
    addi    r31,r30,252
LDoListen_141c:
    addi    r3,r31,16
LDoListen_1420:
    sth     r0,18(r30)
LDoListen_1424:
    addi    r4,r28,12
LDoListen_1428:
    li      r5,4
LDoListen_142c:
    lhz     r0,2(r29)
LDoListen_1430:
    sth     r0,10(r30)
LDoListen_1434:
    bl memmove
LDoListen_1438:
    addi    r3,r31,12
LDoListen_143c:
    addi    r4,r28,16
LDoListen_1440:
    li      r5,4
LDoListen_1444:
    bl memmove
LDoListen_1448:
    lwz     r4,64(r26)
LDoListen_144c:
    lis     r3,1
LDoListen_1450:
    addi    r0,r3,-1
LDoListen_1454:
    addi    r3,r4,-40
LDoListen_1458:
    stw     r3,244(r30)
LDoListen_145c:
    lis     r3,-32768
LDoListen_1460:
    cmplwi  r30,0
LDoListen_1464:
    lwz     r4,244(r30)
LDoListen_1468:
    stw     r4,640(r30)
LDoListen_146c:
    stw     r0,644(r30)
LDoListen_1470:
    lwz     r4,12508(r3)
LDoListen_1474:
    clrlwi  r3,r4,27
LDoListen_1478:
    rotlw   r31,r4,r3
LDoListen_147c:
    beq LDoListen_14f0
LDoListen_1480:
    lwz     r0,12(r30)
LDoListen_1484:
    lhz     r9,18(r30)
LDoListen_1488:
    xor     r31,r31,r0
LDoListen_148c:
    lhz     r8,10(r30)
LDoListen_1490:
    clrlwi  r5,r31,27
LDoListen_1494:
    lwz     r0,20(r30)
LDoListen_1498:
    subfic  r4,r5,32
LDoListen_149c:
    clrlwi  r7,r9,27
LDoListen_14a0:
    subfic  r3,r7,32
LDoListen_14a4:
    slw     r6,r31,r5
LDoListen_14a8:
    srw     r5,r31,r4
LDoListen_14ac:
    slw     r4,r8,r7
LDoListen_14b0:
    srw     r3,r8,r3
LDoListen_14b4:
    clrlwi  r7,r8,27
LDoListen_14b8:
    or      r6,r6,r5
LDoListen_14bc:
    or      r3,r4,r3
LDoListen_14c0:
    xor     r6,r6,r3
LDoListen_14c4:
    xor     r6,r6,r0
LDoListen_14c8:
    clrlwi  r4,r6,27
LDoListen_14cc:
    subfic  r3,r4,32
LDoListen_14d0:
    subfic  r0,r7,32
LDoListen_14d4:
    slw     r5,r6,r4
LDoListen_14d8:
    srw     r4,r6,r3
LDoListen_14dc:
    slw     r3,r9,r7
LDoListen_14e0:
    srw     r0,r9,r0
LDoListen_14e4:
    or      r31,r5,r4
LDoListen_14e8:
    or      r0,r3,r0
LDoListen_14ec:
    xor     r31,r31,r0
LDoListen_14f0:
    lis     r3,-32768
LDoListen_14f4:
    lwz     r0,248(r3)
LDoListen_14f8:
    lis     r3,17180
LDoListen_14fc:
    addi    r3,r3,-8573
LDoListen_1500:
    srwi    r0,r0,2
LDoListen_1504:
    mulhwu  r0,r3,r0
LDoListen_1508:
    srwi    r26,r0,16
LDoListen_150c:
    bl OSGetTime
LDoListen_1510:
    addi    r6,r26,0
LDoListen_1514:
    li      r5,0
LDoListen_1518:
    bl __div2i
LDoListen_151c:
    add     r31,r31,r4
LDoListen_1520:
    stw     r31,72(r30)
LDoListen_1524:
    li      r6,0
LDoListen_1528:
    addi    r3,r30,0
LDoListen_152c:
    lwz     r0,4(r29)
LDoListen_1530:
    addi    r4,r28,0
LDoListen_1534:
    addi    r5,r29,0
LDoListen_1538:
    stw     r0,96(r30)
LDoListen_153c:
    lwz     r0,96(r30)
LDoListen_1540:
    stw     r0,520(r30)
LDoListen_1544:
    lwz     r7,4(r29)
LDoListen_1548:
    addi    r0,r7,1
LDoListen_154c:
    stw     r0,84(r30)
LDoListen_1550:
    lwz     r0,84(r27)
LDoListen_1554:
    stw     r0,92(r30)
LDoListen_1558:
    lwz     r0,72(r30)
LDoListen_155c:
    stw     r0,48(r30)
LDoListen_1560:
    lwz     r0,72(r30)
LDoListen_1564:
    stw     r0,52(r30)
LDoListen_1568:
    lwz     r0,72(r30)
LDoListen_156c:
    stw     r0,80(r30)
LDoListen_1570:
    lwz     r0,72(r30)
LDoListen_1574:
    stw     r0,60(r30)
LDoListen_1578:
    lwz     r0,100(r27)
LDoListen_157c:
    stw     r0,100(r30)
LDoListen_1580:
    lwz     r0,104(r27)
LDoListen_1584:
    stw     r0,104(r30)
LDoListen_1588:
    stw     r6,100(r27)
LDoListen_158c:
    stw     r6,104(r27)
LDoListen_1590:
    lwz     r0,48(r30)
LDoListen_1594:
    stw     r0,220(r30)
LDoListen_1598:
    lwz     r0,48(r30)
LDoListen_159c:
    stw     r0,208(r30)
LDoListen_15a0:
    lwz     r0,48(r30)
LDoListen_15a4:
    stw     r0,224(r30)
LDoListen_15a8:
    stw     r6,216(r30)
LDoListen_15ac:
    stw     r6,212(r30)
LDoListen_15b0:
    bl DoOption
LDoListen_15b4:
    li      r0,3
LDoListen_15b8:
    stw     r0,228(r30)
LDoListen_15bc:
    li      r4, TCPInfoQueue
LDoListen_15c0:
    lwzu    r3,4(r4)
LDoListen_15c4:
    cmplwi  r3,0
LDoListen_15c8:
    bne LDoListen_15d4
LDoListen_15cc:
    stw     r30, TCPInfoQueue(r0)
LDoListen_15d0:
    b LDoListen_15d8
LDoListen_15d4:
    stw     r30,24(r3)
LDoListen_15d8:
    stw     r3,28(r30)
LDoListen_15dc:
    li      r0,0
LDoListen_15e0:
    addi    r3,r30,0
LDoListen_15e4:
    stw     r0,24(r30)
LDoListen_15e8:
    stw     r30,0(r4)
LDoListen_15ec:
    lmw     r26,96(r1)
LDoListen_15f0:
    lwz     r0,124(r1)
LDoListen_15f4:
    addi    r1,r1,120
LDoListen_15f8:
    mtlr    r0
LDoListen_15fc:
    blr
}

static asm TCPSocket* DoSynSent(IPInterface* interface, TCPSocket* so, IPHeader* ip,
                     TCPHeader* tcp)
{
    nofralloc
LDoSynSent_1600:
    mflr    r0
LDoSynSent_1604:
    stw     r0,4(r1)
LDoSynSent_1608:
    stwu    r1,-48(r1)
LDoSynSent_160c:
    stw     r31,44(r1)
LDoSynSent_1610:
    mr      r31,r6
LDoSynSent_1614:
    stw     r30,40(r1)
LDoSynSent_1618:
    addi    r30,r4,0
LDoSynSent_161c:
    addi    r4,r5,0
LDoSynSent_1620:
    lhz     r8,12(r6)
LDoSynSent_1624:
    rlwinm. r3,r8,0,27,27
LDoSynSent_1628:
    beq LDoSynSent_16d4
LDoSynSent_162c:
    lwz     r9,8(r31)
LDoSynSent_1630:
    lwz     r0,72(r30)
LDoSynSent_1634:
    subf.   r0,r9,r0
LDoSynSent_1638:
    bge LDoSynSent_1648
LDoSynSent_163c:
    lwz     r0,80(r30)
LDoSynSent_1640:
    subf.   r0,r0,r9
LDoSynSent_1644:
    ble LDoSynSent_16d4
LDoSynSent_1648:
    rlwinm. r0,r8,0,29,29
LDoSynSent_164c:
    lwz     r3,40(r30)
LDoSynSent_1650:
    bne LDoSynSent_16cc
LDoSynSent_1654:
    rlwinm. r0,r8,0,27,27
LDoSynSent_1658:
    bne LDoSynSent_16a0
LDoSynSent_165c:
    lbz     r6,0(r4)
LDoSynSent_1660:
    rlwinm. r0,r8,0,30,30
LDoSynSent_1664:
    lhz     r5,2(r4)
LDoSynSent_1668:
    rlwinm  r7,r8,22,26,29
LDoSynSent_166c:
    rlwinm  r6,r6,2,26,29
LDoSynSent_1670:
    subf    r0,r6,r5
LDoSynSent_1674:
    subf    r6,r7,r0
LDoSynSent_1678:
    li      r9,0
LDoSynSent_167c:
    beq LDoSynSent_1684
LDoSynSent_1680:
    addi    r6,r6,1
LDoSynSent_1684:
    clrlwi. r0,r8,31
LDoSynSent_1688:
    beq LDoSynSent_1690
LDoSynSent_168c:
    addi    r6,r6,1
LDoSynSent_1690:
    lwz     r0,4(r31)
LDoSynSent_1694:
    li      r5,20
LDoSynSent_1698:
    add     r10,r0,r6
LDoSynSent_169c:
    b LDoSynSent_16a8
LDoSynSent_16a0:
    li      r10,0
LDoSynSent_16a4:
    li      r5,4
LDoSynSent_16a8:
    stw     r5,8(r1)
LDoSynSent_16ac:
    li      r0,0
LDoSynSent_16b0:
    addi    r5,r4,12
LDoSynSent_16b4:
    stw     r0,12(r1)
LDoSynSent_16b8:
    addi    r7,r4,16
LDoSynSent_16bc:
    li      r4,0
LDoSynSent_16c0:
    lhz     r6,0(r31)
LDoSynSent_16c4:
    lhz     r8,2(r31)
LDoSynSent_16c8:
    bl TCPRespond
LDoSynSent_16cc:
    li      r3,0
LDoSynSent_16d0:
    b LDoSynSent_183c
LDoSynSent_16d4:
    rlwinm. r0,r8,0,29,29
LDoSynSent_16d8:
    beq LDoSynSent_16fc
LDoSynSent_16dc:
    cmpwi   r3,0
LDoSynSent_16e0:
    beq LDoSynSent_16f4
LDoSynSent_16e4:
    li      r0,-3
LDoSynSent_16e8:
    stw     r0,44(r30)
LDoSynSent_16ec:
    mr      r3,r30
LDoSynSent_16f0:
    bl TCPAbort
LDoSynSent_16f4:
    li      r3,0
LDoSynSent_16f8:
    b LDoSynSent_183c
LDoSynSent_16fc:
    rlwinm. r0,r8,0,30,30
LDoSynSent_1700:
    bne LDoSynSent_170c
LDoSynSent_1704:
    li      r3,0
LDoSynSent_1708:
    b LDoSynSent_183c
LDoSynSent_170c:
    lwz     r5,4(r31)
LDoSynSent_1710:
    addi    r3,r30,0
LDoSynSent_1714:
    addi    r0,r5,1
LDoSynSent_1718:
    stw     r0,84(r30)
LDoSynSent_171c:
    mr      r5,r31
LDoSynSent_1720:
    lwz     r0,84(r30)
LDoSynSent_1724:
    stw     r0,92(r30)
LDoSynSent_1728:
    lwz     r0,4(r31)
LDoSynSent_172c:
    stw     r0,96(r30)
LDoSynSent_1730:
    lwz     r0,96(r30)
LDoSynSent_1734:
    stw     r0,520(r30)
LDoSynSent_1738:
    bl DoOption
LDoSynSent_173c:
    lhz     r0,12(r31)
LDoSynSent_1740:
    rlwinm. r0,r0,0,27,27
LDoSynSent_1744:
    beq LDoSynSent_1830
LDoSynSent_1748:
    lwz     r0,648(r30)
LDoSynSent_174c:
    cmpwi   r0,0
LDoSynSent_1750:
    beq LDoSynSent_1788
LDoSynSent_1754:
    lwz     r3,652(r30)
LDoSynSent_1758:
    lwz     r0,8(r31)
LDoSynSent_175c:
    subf.   r0,r3,r0
LDoSynSent_1760:
    ble LDoSynSent_1788
LDoSynSent_1764:
    li      r0,0
LDoSynSent_1768:
    stw     r0,648(r30)
LDoSynSent_176c:
    bl OSGetTime
LDoSynSent_1770:
    lwz     r5,660(r30)
LDoSynSent_1774:
    lwz     r0,656(r30)
LDoSynSent_1778:
    subfc   r6,r5,r4
LDoSynSent_177c:
    subfe   r5,r0,r3
LDoSynSent_1780:
    addi    r3,r30,0
LDoSynSent_1784:
    bl TCPUpdateRtt
LDoSynSent_1788:
    lhz     r0,14(r31)
LDoSynSent_178c:
    stw     r0,56(r30)
LDoSynSent_1790:
    lwz     r0,4(r31)
LDoSynSent_1794:
    stw     r0,64(r30)
LDoSynSent_1798:
    lwz     r0,8(r31)
LDoSynSent_179c:
    stw     r0,68(r30)
LDoSynSent_17a0:
    lwz     r0,56(r30)
LDoSynSent_17a4:
    stw     r0,76(r30)
LDoSynSent_17a8:
    lwz     r3,48(r30)
LDoSynSent_17ac:
    lwz     r0,72(r30)
LDoSynSent_17b0:
    cmpw    r3,r0
LDoSynSent_17b4:
    bne LDoSynSent_17d4
LDoSynSent_17b8:
    addi    r0,r3,1
LDoSynSent_17bc:
    stw     r0,48(r30)
LDoSynSent_17c0:
    lwz     r0,52(r30)
LDoSynSent_17c4:
    lwz     r3,48(r30)
LDoSynSent_17c8:
    subf.   r0,r0,r3
LDoSynSent_17cc:
    ble LDoSynSent_17d4
LDoSynSent_17d0:
    stw     r3,52(r30)
LDoSynSent_17d4:
    addi    r3,r30,0
LDoSynSent_17d8:
    addi    r4,r31,0
LDoSynSent_17dc:
    bl TCPStopRxmitTimer
LDoSynSent_17e0:
    li      r0,4
LDoSynSent_17e4:
    stw     r0,228(r30)
LDoSynSent_17e8:
    lwz     r0,724(r30)
LDoSynSent_17ec:
    cmplwi  r0,0
LDoSynSent_17f0:
    beq LDoSynSent_1828
LDoSynSent_17f4:
    lwz     r3,728(r30)
LDoSynSent_17f8:
    cmplwi  r3,0
LDoSynSent_17fc:
    beq LDoSynSent_180c
LDoSynSent_1800:
    li      r0,0
LDoSynSent_1804:
    stw     r0,0(r3)
LDoSynSent_1808:
    stw     r0,728(r30)
LDoSynSent_180c:
    lwz     r12,724(r30)
LDoSynSent_1810:
    li      r0,0
LDoSynSent_1814:
    addi    r3,r30,0
LDoSynSent_1818:
    stw     r0,724(r30)
LDoSynSent_181c:
    mtlr    r12
LDoSynSent_1820:
    li      r4,0
LDoSynSent_1824:
    blrl
LDoSynSent_1828:
    mr      r3,r30
LDoSynSent_182c:
    b LDoSynSent_183c
LDoSynSent_1830:
    li      r0,3
LDoSynSent_1834:
    stw     r0,228(r30)
LDoSynSent_1838:
    mr      r3,r30
LDoSynSent_183c:
    lwz     r0,52(r1)
LDoSynSent_1840:
    lwz     r31,44(r1)
LDoSynSent_1844:
    lwz     r30,40(r1)
LDoSynSent_1848:
    addi    r1,r1,48
LDoSynSent_184c:
    mtlr    r0
LDoSynSent_1850:
    blr
}

static asm void DoUrg(TCPSocket* so, TCPHeader* tcp)
{
    nofralloc
    lhz r0, 12(r4)
    rlwinm. r0, r0, 0, 26, 26
    beq Lurg_none
    lhz r5, 18(r4)
    cmplwi r5, 0
    beq Lurg_none
    lwz r0, 228(r3)
    cmpwi r0, 7
    bgelr
    cmpwi r0, 4
    bge Lurg_run
    blr
    blr
Lurg_run:
    lwz r0, 4(r4)
    lwz r6, 92(r3)
    add r5, r0, r5
    subf. r0, r6, r5
    addi r0, r5, 0
    ble Lurg_set
    mr r6, r0
Lurg_set:
    stw r6, 92(r3)
    lwz r0, 232(r3)
    rlwinm r0, r0, 0, 27, 24
    stw r0, 232(r3)
    lhz r4, 18(r4)
    lwz r0, 100(r3)
    cmpw r4, r0
    bgt Lurg_afterbyte
    lwz r0, 104(r3)
    add r4, r0, r4
    lbz r0, -1(r4)
    stb r0, 548(r3)
    lwz r0, 232(r3)
    ori r0, r0, 32
    stw r0, 232(r3)
Lurg_afterbyte:
    lwz r4, 84(r3)
    lwz r0, 92(r3)
    lwz r5, 512(r3)
    subf r0, r4, r0
    lwz r4, 544(r3)
    add r0, r5, r0
    add r0, r4, r0
    stw r0, 552(r3)
    blr
Lurg_none:
    lwz r0, 92(r3)
    lwz r4, 84(r3)
    subf. r0, r0, r4
    blelr
    stw r4, 92(r3)
    blr
}

static asm void DoData(IPInterface* interface, TCPSocket* so, IPHeader* ip,
                   TCPHeader* tcp, u16 flag)
{
    nofralloc
LDoData_191c:
    mflr    r0
LDoData_1920:
    stw     r0,4(r1)
LDoData_1924:
    li      r0,0
LDoData_1928:
    stwu    r1,-72(r1)
LDoData_192c:
    stmw    r27,52(r1)
LDoData_1930:
    addi    r28,r4,0
LDoData_1934:
    addi    r27,r3,0
LDoData_1938:
    addi    r30,r6,0
LDoData_193c:
    addi    r29,r5,0
LDoData_1940:
    addi    r31,r7,0
LDoData_1944:
    addi    r3,r28,0
LDoData_1948:
    stw     r0,44(r4)
LDoData_194c:
    addi    r4,r30,0
LDoData_1950:
    bl DoUrg
LDoData_1954:
    lwz     r0,228(r28)
LDoData_1958:
    cmpwi   r0,7
LDoData_195c:
    bge LDoData_1b74
LDoData_1960:
    cmpwi   r0,5
LDoData_1964:
    bge LDoData_1998
LDoData_1968:
    cmpwi   r0,3
LDoData_196c:
    bge LDoData_1978
LDoData_1970:
    b LDoData_1b74
LDoData_1974:
    b LDoData_1b74
LDoData_1978:
    lwz     r0,236(r28)
LDoData_197c:
    cmplwi  r0,0
LDoData_1980:
    bne LDoData_1990
LDoData_1984:
    lwz     r0,232(r28)
LDoData_1988:
    rlwinm. r0,r0,0,28,28
LDoData_198c:
    beq LDoData_1998
LDoData_1990:
    li      r0,5
LDoData_1994:
    stw     r0,228(r28)
LDoData_1998:
    lwz     r0,100(r28)
LDoData_199c:
    stw     r0,40(r1)
LDoData_19a0:
    lhz     r0,12(r30)
LDoData_19a4:
    clrlwi. r0,r0,31
LDoData_19a8:
    beq LDoData_19b8
LDoData_19ac:
    lwz     r3,40(r1)
LDoData_19b0:
    addi    r0,r3,-1
LDoData_19b4:
    stw     r0,40(r1)
LDoData_19b8:
    lwz     r0,232(r28)
LDoData_19bc:
    andi.   r0,r0,17
LDoData_19c0:
    beq LDoData_1a70
LDoData_19c4:
    lwz     r0,40(r1)
LDoData_19c8:
    cmpwi   r0,0
LDoData_19cc:
    ble LDoData_1a70
LDoData_19d0:
    li      r0,-8
LDoData_19d4:
    stw     r0,44(r28)
LDoData_19d8:
    lhz     r6,12(r30)
LDoData_19dc:
    rlwinm. r0,r6,0,29,29
LDoData_19e0:
    bne LDoData_1a64
LDoData_19e4:
    rlwinm. r0,r6,0,27,27
LDoData_19e8:
    bne LDoData_1a30
LDoData_19ec:
    lbz     r4,0(r29)
LDoData_19f0:
    rlwinm. r0,r6,0,30,30
LDoData_19f4:
    lhz     r3,2(r29)
LDoData_19f8:
    rlwinm  r5,r6,22,26,29
LDoData_19fc:
    rlwinm  r4,r4,2,26,29
LDoData_1a00:
    subf    r0,r4,r3
LDoData_1a04:
    subf    r3,r5,r0
LDoData_1a08:
    li      r9,0
LDoData_1a0c:
    beq LDoData_1a14
LDoData_1a10:
    addi    r3,r3,1
LDoData_1a14:
    clrlwi. r0,r6,31
LDoData_1a18:
    beq LDoData_1a20
LDoData_1a1c:
    addi    r3,r3,1
LDoData_1a20:
    lwz     r0,4(r30)
LDoData_1a24:
    li      r4,20
LDoData_1a28:
    add     r10,r0,r3
LDoData_1a2c:
    b LDoData_1a3c
LDoData_1a30:
    lwz     r9,8(r30)
LDoData_1a34:
    li      r10,0
LDoData_1a38:
    li      r4,4
LDoData_1a3c:
    stw     r4,8(r1)
LDoData_1a40:
    li      r0,0
LDoData_1a44:
    addi    r3,r27,0
LDoData_1a48:
    stw     r0,12(r1)
LDoData_1a4c:
    addi    r5,r29,12
LDoData_1a50:
    addi    r7,r29,16
LDoData_1a54:
    lhz     r6,0(r30)
LDoData_1a58:
    li      r4,0
LDoData_1a5c:
    lhz     r8,2(r30)
LDoData_1a60:
    bl TCPRespond
LDoData_1a64:
    mr      r3,r28
LDoData_1a68:
    bl TCPAbort
LDoData_1a6c:
    b LDoData_1d0c
LDoData_1a70:
    lwz     r3,84(r28)
LDoData_1a74:
    lwz     r4,4(r30)
LDoData_1a78:
    cmpw    r3,r4
LDoData_1a7c:
    bne LDoData_1b14
LDoData_1a80:
    lwz     r0,40(r1)
LDoData_1a84:
    cmpwi   r0,0
LDoData_1a88:
    ble LDoData_1b74
LDoData_1a8c:
    li      r0,4
LDoData_1a90:
    stw     r0,8(r1)
LDoData_1a94:
    ori     r31,r31,16
LDoData_1a98:
    addi    r9,r1,40
LDoData_1a9c:
    lwz     r3,504(r28)
LDoData_1aa0:
    addi    r10,r28,108
LDoData_1aa4:
    lwz     r4,508(r28)
LDoData_1aa8:
    li      r7,0
LDoData_1aac:
    lwz     r5,516(r28)
LDoData_1ab0:
    lwz     r6,512(r28)
LDoData_1ab4:
    lwz     r8,104(r28)
LDoData_1ab8:
    bl IFRingInEx
LDoData_1abc:
    stw     r3,516(r28)
LDoData_1ac0:
    lwz     r3,512(r28)
LDoData_1ac4:
    lwz     r0,40(r1)
LDoData_1ac8:
    add     r0,r3,r0
LDoData_1acc:
    stw     r0,512(r28)
LDoData_1ad0:
    lwz     r3,104(r28)
LDoData_1ad4:
    lwz     r0,40(r1)
LDoData_1ad8:
    add     r0,r3,r0
LDoData_1adc:
    stw     r0,104(r28)
LDoData_1ae0:
    lwz     r3,84(r28)
LDoData_1ae4:
    lwz     r0,40(r1)
LDoData_1ae8:
    add     r0,r3,r0
LDoData_1aec:
    stw     r0,84(r28)
LDoData_1af0:
    lwz     r3,40(r1)
LDoData_1af4:
    lwz     r0,88(r28)
LDoData_1af8:
    subf    r0,r3,r0
LDoData_1afc:
    stw     r0,88(r28)
LDoData_1b00:
    lha     r0,4(r28)
LDoData_1b04:
    cmpwi   r0,0
LDoData_1b08:
    ble LDoData_1b74
LDoData_1b0c:
    bl __IPWakeupPollingThreads
LDoData_1b10:
    b LDoData_1b74
LDoData_1b14:
    lwz     r0,40(r1)
LDoData_1b18:
    cmpwi   r0,0
LDoData_1b1c:
    ble LDoData_1b24
LDoData_1b20:
    ori     r31,r31,16
LDoData_1b24:
    li      r0,4
LDoData_1b28:
    stw     r0,8(r1)
LDoData_1b2c:
    subf    r7,r3,r4
LDoData_1b30:
    addi    r9,r1,40
LDoData_1b34:
    lwz     r3,504(r28)
LDoData_1b38:
    addi    r10,r28,108
LDoData_1b3c:
    lwz     r4,508(r28)
LDoData_1b40:
    lwz     r5,516(r28)
LDoData_1b44:
    lwz     r6,512(r28)
LDoData_1b48:
    lwz     r8,104(r28)
LDoData_1b4c:
    bl IFRingInEx
LDoData_1b50:
    stw     r3,516(r28)
LDoData_1b54:
    lhz     r3,12(r30)
LDoData_1b58:
    clrlwi. r0,r3,31
LDoData_1b5c:
    beq LDoData_1b74
LDoData_1b60:
    clrrwi  r0,r3,1
LDoData_1b64:
    sth     r0,12(r30)
LDoData_1b68:
    lwz     r3,100(r28)
LDoData_1b6c:
    addi    r0,r3,-1
LDoData_1b70:
    stw     r0,100(r28)
LDoData_1b74:
    lhz     r0,12(r30)
LDoData_1b78:
    clrlwi. r0,r0,31
LDoData_1b7c:
    beq LDoData_1bd4
LDoData_1b80:
    lwz     r3,84(r28)
LDoData_1b84:
    addi    r0,r3,1
LDoData_1b88:
    stw     r0,84(r28)
LDoData_1b8c:
    lwz     r0,228(r28)
LDoData_1b90:
    cmpwi   r0,5
LDoData_1b94:
    beq LDoData_1bc0
LDoData_1b98:
    bge LDoData_1ba8
LDoData_1b9c:
    cmpwi   r0,3
LDoData_1ba0:
    bge LDoData_1bb4
LDoData_1ba4:
    b LDoData_1bd4
LDoData_1ba8:
    cmpwi   r0,7
LDoData_1bac:
    bge LDoData_1bd4
LDoData_1bb0:
    b LDoData_1bcc
LDoData_1bb4:
    li      r0,7
LDoData_1bb8:
    stw     r0,228(r28)
LDoData_1bbc:
    b LDoData_1bd4
LDoData_1bc0:
    li      r0,8
LDoData_1bc4:
    stw     r0,228(r28)
LDoData_1bc8:
    b LDoData_1bd4
LDoData_1bcc:
    li      r0,10
LDoData_1bd0:
    stw     r0,228(r28)
LDoData_1bd4:
    lwz     r0,528(r28)
LDoData_1bd8:
    cmplwi  r0,0
LDoData_1bdc:
    beq LDoData_1c34
LDoData_1be0:
    mr      r3,r28
LDoData_1be4:
    bl TCPRecvOut
LDoData_1be8:
    mr.     r4,r3
LDoData_1bec:
    bgt LDoData_1bfc
LDoData_1bf0:
    lhz     r0,12(r30)
LDoData_1bf4:
    clrlwi. r0,r0,31
LDoData_1bf8:
    beq LDoData_1c34
LDoData_1bfc:
    lwz     r3,532(r28)
LDoData_1c00:
    lwz     r12,528(r28)
LDoData_1c04:
    cmplwi  r3,0
LDoData_1c08:
    beq LDoData_1c18
LDoData_1c0c:
    stw     r4,0(r3)
LDoData_1c10:
    li      r0,0
LDoData_1c14:
    stw     r0,532(r28)
LDoData_1c18:
    li      r0,0
LDoData_1c1c:
    mtlr    r12
LDoData_1c20:
    stw     r0,528(r28)
LDoData_1c24:
    addi    r3,r28,0
LDoData_1c28:
    stw     r0,536(r28)
LDoData_1c2c:
    stw     r0,544(r28)
LDoData_1c30:
    blrl
LDoData_1c34:
    lwz     r0,552(r28)
LDoData_1c38:
    cmpwi   r0,0
LDoData_1c3c:
    ble LDoData_1cd8
LDoData_1c40:
    bgt LDoData_1c50
LDoData_1c44:
    lwz     r0,232(r28)
LDoData_1c48:
    rlwinm. r0,r0,0,26,26
LDoData_1c4c:
    beq LDoData_1cd8
LDoData_1c50:
    lwz     r0,556(r28)
LDoData_1c54:
    cmplwi  r0,0
LDoData_1c58:
    beq LDoData_1cd8
LDoData_1c5c:
    lwz     r3,232(r28)
LDoData_1c60:
    rlwinm. r0,r3,0,25,25
LDoData_1c64:
    bne LDoData_1cd8
LDoData_1c68:
    rlwinm. r0,r3,0,26,26
LDoData_1c6c:
    beq LDoData_1c88
LDoData_1c70:
    rlwinm. r0,r3,0,20,20
LDoData_1c74:
    li      r4,1
LDoData_1c78:
    bne LDoData_1c8c
LDoData_1c7c:
    xori    r0,r3,96
LDoData_1c80:
    stw     r0,232(r28)
LDoData_1c84:
    b LDoData_1c8c
LDoData_1c88:
    li      r4,0
LDoData_1c8c:
    lwz     r3,560(r28)
LDoData_1c90:
    cmplwi  r3,0
LDoData_1c94:
    beq LDoData_1ca4
LDoData_1c98:
    stw     r4,0(r3)
LDoData_1c9c:
    li      r0,0
LDoData_1ca0:
    stw     r0,560(r28)
LDoData_1ca4:
    lwz     r5,564(r28)
LDoData_1ca8:
    cmplwi  r5,0
LDoData_1cac:
    beq LDoData_1cc0
LDoData_1cb0:
    lbz     r3,548(r28)
LDoData_1cb4:
    li      r0,0
LDoData_1cb8:
    stb     r3,0(r5)
LDoData_1cbc:
    stw     r0,564(r28)
LDoData_1cc0:
    lwz     r12,556(r28)
LDoData_1cc4:
    li      r0,0
LDoData_1cc8:
    addi    r3,r28,0
LDoData_1ccc:
    stw     r0,556(r28)
LDoData_1cd0:
    mtlr    r12
LDoData_1cd4:
    blrl
LDoData_1cd8:
    lhz     r0,12(r30)
LDoData_1cdc:
    clrlwi. r0,r0,31
LDoData_1ce0:
    beq LDoData_1d00
LDoData_1ce4:
    lwz     r0,228(r28)
LDoData_1ce8:
    cmpwi   r0,10
LDoData_1cec:
    bne LDoData_1d00
LDoData_1cf0:
    lwz     r3,40(r28)
LDoData_1cf4:
    mr      r4,r28
LDoData_1cf8:
    bl TCPStartTimeWait
LDoData_1cfc:
    b LDoData_1d0c
LDoData_1d00:
    addi    r3,r28,0
LDoData_1d04:
    addi    r4,r31,0
LDoData_1d08:
    bl TCPOutput
LDoData_1d0c:
    lmw     r27,52(r1)
LDoData_1d10:
    lwz     r0,76(r1)
LDoData_1d14:
    addi    r1,r1,72
LDoData_1d18:
    mtlr    r0
LDoData_1d1c:
    blr
}

asm s32 TCPSendIn(TCPSocket* so, s32 reset)
{
    nofralloc
    mflr r0
    stw r0, 4(r1)
    stwu r1, -32(r1)
    stw r31, 28(r1)
    addi r31, r4, 0
    stw r30, 24(r1)
    mr r30, r3
    stw r29, 20(r1)
    lwz r0, 472(r3)
    cmplwi r0, 0
    beq Lsend_fail
    lwz r3, 488(r30)
    cmpwi r3, 0
    bgt Lsend_cont
Lsend_fail:
    li r3, 0
    b Lsend_end
Lsend_cont:
    cmpwi r31, 0
    bne Lsend_calc
    lwz r0, 384(r30)
    cmpwi r0, 0
    bne Lsend_calc
    lwz r0, 376(r30)
    cmpw r0, r3
    bge Lsend_calc
    li r29, 0
    b Lsend_go
Lsend_calc:
    lwz r3, 384(r30)
    lwz r0, 376(r30)
    lwz r4, 488(r30)
    subf r0, r3, r0
    cmpw r0, r4
    bge Lsend_min
    mr r4, r0
Lsend_min:
    mr r29, r4
Lsend_go:
    cmpwi r29, 0
    ble Lsend_reset
    lwz r3, 372(r30)
    mr r8, r29
    lwz r4, 376(r30)
    lwz r5, 380(r30)
    lwz r6, 384(r30)
    lwz r7, 484(r30)
    bl IFRingIn
    stw r3, 380(r30)
    lwz r0, 384(r30)
    add r0, r0, r29
    stw r0, 384(r30)
    lwz r0, 484(r30)
    add r0, r0, r29
    stw r0, 484(r30)
    lwz r0, 488(r30)
    subf r0, r29, r0
    stw r0, 488(r30)
    lwz r0, 480(r30)
    add r0, r0, r29
    stw r0, 480(r30)
Lsend_reset:
    cmpwi r31, 0
    beq Lsend_ret
    li r0, 0
    stw r0, 484(r30)
    stw r0, 488(r30)
Lsend_ret:
    mr r3, r29
Lsend_end:
    lwz r0, 36(r1)
    lwz r31, 28(r1)
    lwz r30, 24(r1)
    lwz r29, 20(r1)
    addi r1, r1, 32
    mtlr r0
    blr
}

asm s32 TCPPeekOut(TCPSocket* so, u8* dst, s32 len, s32 peek)
{
    nofralloc
    mflr r0
    stw r0, 4(r1)
    stwu r1, -48(r1)
    stw r31, 44(r1)
    mr r31, r3
    stw r30, 40(r1)
    addi r30, r5, 0
    stw r29, 36(r1)
    addi r29, r6, 0
    stw r28, 32(r1)
    addi r28, r4, 0
    lwz r7, 512(r3)
    cmpw r30, r7
    bge Lpk_clamp
    b Lpk_afterclamp
Lpk_clamp:
    mr r30, r7
Lpk_afterclamp:
    cmpwi r30, 0
    bgt Lpk_urg
    li r3, 0
    b Lpk_end
Lpk_urg:
    lwz r3, 552(r31)
    cmpwi r3, 0
    ble Lpk_send
    cmpwi r3, 1
    bne Lpk_urgmany
    lwz r0, 232(r31)
    rlwinm. r0, r0, 0, 24, 24
    beq Lpk_one
    lwz r3, 504(r31)
    mr r6, r7
    lwz r4, 508(r31)
    addi r7, r1, 24
    lwz r5, 516(r31)
    li r8, 1
    bl IFRingOut
    stw r3, 516(r31)
    lwz r3, 512(r31)
    addi r0, r3, -1
    stw r0, 512(r31)
    lwz r3, 552(r31)
    addi r0, r3, -1
    stw r0, 552(r31)
    lwz r0, 512(r31)
    cmpw r30, r0
    bge Lpk_setclamp
    b Lpk_send
Lpk_setclamp:
    mr r30, r0
    b Lpk_send
Lpk_one:
    li r30, 1
    b Lpk_send
Lpk_urgmany:
    cmpw r3, r30
    bgt Lpk_send
    addi r30, r3, -1
Lpk_send:
    cmpwi r30, 0
    ble Lpk_ret
    lwz r3, 504(r31)
    mr r7, r28
    lwz r4, 508(r31)
    mr r8, r30
    lwz r5, 516(r31)
    lwz r6, 512(r31)
    bl IFRingOut
    cmpwi r29, 0
    bne Lpk_ret
    stw r3, 516(r31)
    lwz r0, 512(r31)
    subf r0, r30, r0
    stw r0, 512(r31)
    lwz r0, 552(r31)
    cmpwi r0, 0
    ble Lpk_ret
    subf r0, r30, r0
    stw r0, 552(r31)
Lpk_ret:
    mr r3, r30
Lpk_end:
    lwz r0, 52(r1)
    lwz r31, 44(r1)
    lwz r30, 40(r1)
    lwz r29, 36(r1)
    lwz r28, 32(r1)
    addi r1, r1, 48
    mtlr r0
    blr
}

asm s32 TCPRecvOut(TCPSocket* so)
{
    nofralloc
    mflr r0
    stw r0, 4(r1)
    stwu r1, -24(r1)
    stw r31, 20(r1)
    stw r30, 16(r1)
    mr r30, r3
    lwz r0, 232(r3)
    rlwinm. r0, r0, 0, 21, 21
    beq Lrecv_zero
    li r31, 1
    b Lrecv_call
Lrecv_zero:
    li r31, 0
Lrecv_call:
    lwz r4, 536(r30)
    mr r6, r31
    lwz r5, 544(r30)
    mr r3, r30
    lwz r0, 540(r30)
    add r4, r4, r5
    subf r5, r5, r0
    bl TCPPeekOut
    cmpwi r31, 0
    beq Lrecv_add
    b Lrecv_end
Lrecv_add:
    lwz r0, 544(r30)
    add r3, r0, r3
    stw r3, 544(r30)
Lrecv_end:
    lwz r0, 28(r1)
    lwz r31, 20(r1)
    lwz r30, 16(r1)
    addi r1, r1, 24
    mtlr r0
    blr
}

asm void TCPNotify(IPHeader* ip, s32 unused, s32 arg)
{
    nofralloc
    mflr r0
    addi r4, r3, 16
    stw r0, 4(r1)
    li r8, 0
    stwu r1, -32(r1)
    stw r31, 28(r1)
    addi r31, r5, 0
    addi r5, r3, 12
    lbz r0, 0(r3)
    rlwinm r0, r0, 2, 26, 29
    add r7, r3, r0
    lhz r6, 2(r7)
    li r3, TCPInfoQueue
    lhz r7, 0(r7)
    bl IPLookupInfo
    cmplwi r3, 0
    beq L_notify_end
    stw r31, 44(r3)
L_notify_end:
    lwz r0, 36(r1)
    lwz r31, 28(r1)
    addi r1, r1, 32
    mtlr r0
    blr
}

asm void TCPSourceQuench(IPHeader* ip)
{
    nofralloc
    mflr r0
    addi r4, r3, 16
    stw r0, 4(r1)
    addi r5, r3, 12
    li r8, 0
    stwu r1, -8(r1)
    lbz r0, 0(r3)
    rlwinm r0, r0, 2, 26, 29
    add r7, r3, r0
    lhz r6, 2(r7)
    li r3, TCPInfoQueue
    lhz r7, 0(r7)
    bl IPLookupInfo
    cmplwi r3, 0
    beq L_sq_end
    lwz r0, 244(r3)
    stw r0, 640(r3)
L_sq_end:
    lwz r0, 12(r1)
    addi r1, r1, 8
    mtlr r0
    blr
}

asm void TCPDeleteSackHoles(TCPSocket* so, TCPHeader* tcp)
{
    nofralloc
LTCPDeleteSackHoles_20ac:
    mflr    r0
LTCPDeleteSackHoles_20b0:
    stw     r0,4(r1)
LTCPDeleteSackHoles_20b4:
    stwu    r1,-32(r1)
LTCPDeleteSackHoles_20b8:
    stw     r31,28(r1)
LTCPDeleteSackHoles_20bc:
    mr      r31,r3
LTCPDeleteSackHoles_20c0:
    stw     r30,24(r1)
LTCPDeleteSackHoles_20c4:
    lwz     r0,232(r3)
LTCPDeleteSackHoles_20c8:
    rlwinm. r0,r0,0,18,18
LTCPDeleteSackHoles_20cc:
    beq LTCPDeleteSackHoles_2198
LTCPDeleteSackHoles_20d0:
    lwz     r0,228(r31)
LTCPDeleteSackHoles_20d4:
    cmpwi   r0,1
LTCPDeleteSackHoles_20d8:
    beq LTCPDeleteSackHoles_2198
LTCPDeleteSackHoles_20dc:
    lwz     r0,80(r31)
LTCPDeleteSackHoles_20e0:
    lwz     r30,8(r4)
LTCPDeleteSackHoles_20e4:
    subf.   r0,r0,r30
LTCPDeleteSackHoles_20e8:
    ble LTCPDeleteSackHoles_20f0
LTCPDeleteSackHoles_20ec:
    b LTCPDeleteSackHoles_2198
LTCPDeleteSackHoles_20f0:
    lwz     r3,48(r31)
LTCPDeleteSackHoles_20f4:
    subf.   r0,r3,r30
LTCPDeleteSackHoles_20f8:
    ble LTCPDeleteSackHoles_2100
LTCPDeleteSackHoles_20fc:
    b LTCPDeleteSackHoles_2104
LTCPDeleteSackHoles_2100:
    mr      r30,r3
LTCPDeleteSackHoles_2104:
    lwz     r0,204(r31)
LTCPDeleteSackHoles_2108:
    addi    r4,r31,140
LTCPDeleteSackHoles_210c:
    slwi    r3,r0,4
LTCPDeleteSackHoles_2110:
    addi    r5,r3,140
LTCPDeleteSackHoles_2114:
    add     r5,r31,r5
LTCPDeleteSackHoles_2118:
    addi    r0,r5,15
LTCPDeleteSackHoles_211c:
    subf    r0,r4,r0
LTCPDeleteSackHoles_2120:
    srwi    r0,r0,4
LTCPDeleteSackHoles_2124:
    cmplw   r4,r5
LTCPDeleteSackHoles_2128:
    mtctr   r0
LTCPDeleteSackHoles_212c:
    bge LTCPDeleteSackHoles_2144
LTCPDeleteSackHoles_2130:
    lwz     r0,4(r4)
LTCPDeleteSackHoles_2134:
    subf.   r0,r30,r0
LTCPDeleteSackHoles_2138:
    bgt LTCPDeleteSackHoles_2144
LTCPDeleteSackHoles_213c:
    addi    r4,r4,16
LTCPDeleteSackHoles_2140:
    bdnz LTCPDeleteSackHoles_2130
LTCPDeleteSackHoles_2144:
    addi    r3,r31,140
LTCPDeleteSackHoles_2148:
    cmplw   r3,r4
LTCPDeleteSackHoles_214c:
    bge LTCPDeleteSackHoles_2164
LTCPDeleteSackHoles_2150:
    subf    r5,r4,r5
LTCPDeleteSackHoles_2154:
    srawi   r0,r5,4
LTCPDeleteSackHoles_2158:
    addze   r0,r0
LTCPDeleteSackHoles_215c:
    stw     r0,204(r31)
LTCPDeleteSackHoles_2160:
    bl memmove
LTCPDeleteSackHoles_2164:
    lwz     r0,204(r31)
LTCPDeleteSackHoles_2168:
    cmpwi   r0,0
LTCPDeleteSackHoles_216c:
    ble LTCPDeleteSackHoles_2198
LTCPDeleteSackHoles_2170:
    lwz     r0,140(r31)
LTCPDeleteSackHoles_2174:
    addi    r4,r31,140
LTCPDeleteSackHoles_2178:
    subf.   r0,r0,r30
LTCPDeleteSackHoles_217c:
    ble LTCPDeleteSackHoles_2198
LTCPDeleteSackHoles_2180:
    stw     r30,0(r4)
LTCPDeleteSackHoles_2184:
    lwz     r0,12(r4)
LTCPDeleteSackHoles_2188:
    lwz     r3,0(r4)
LTCPDeleteSackHoles_218c:
    subf.   r0,r0,r3
LTCPDeleteSackHoles_2190:
    ble LTCPDeleteSackHoles_2198
LTCPDeleteSackHoles_2194:
    stw     r3,12(r4)
LTCPDeleteSackHoles_2198:
    lwz     r0,36(r1)
LTCPDeleteSackHoles_219c:
    lwz     r31,28(r1)
LTCPDeleteSackHoles_21a0:
    lwz     r30,24(r1)
LTCPDeleteSackHoles_21a4:
    addi    r1,r1,32
LTCPDeleteSackHoles_21a8:
    mtlr    r0
LTCPDeleteSackHoles_21ac:
    blr
}

asm int TCPUpdateScoreboard(TCPSocket* so, TCPHeader* tcp, u8* opt, s32 len)
{
    nofralloc
LTCPUpdateScoreboard_21b0:
    mflr    r0
LTCPUpdateScoreboard_21b4:
    stw     r0,4(r1)
LTCPUpdateScoreboard_21b8:
    stwu    r1,-64(r1)
LTCPUpdateScoreboard_21bc:
    stmw    r22,24(r1)
LTCPUpdateScoreboard_21c0:
    mr      r22,r3
LTCPUpdateScoreboard_21c4:
    addi    r23,r4,0
LTCPUpdateScoreboard_21c8:
    lwz     r0,232(r3)
LTCPUpdateScoreboard_21cc:
    rlwinm. r0,r0,0,18,18
LTCPUpdateScoreboard_21d0:
    beq LTCPUpdateScoreboard_26f0
LTCPUpdateScoreboard_21d4:
    cmpwi   r6,2
LTCPUpdateScoreboard_21d8:
    ble LTCPUpdateScoreboard_26f0
LTCPUpdateScoreboard_21dc:
    addi    r3,r6,-2
LTCPUpdateScoreboard_21e0:
    clrlwi. r0,r3,29
LTCPUpdateScoreboard_21e4:
    beq LTCPUpdateScoreboard_21ec
LTCPUpdateScoreboard_21e8:
    b LTCPUpdateScoreboard_26f0
LTCPUpdateScoreboard_21ec:
    lwz     r0,80(r22)
LTCPUpdateScoreboard_21f0:
    lwz     r4,8(r23)
LTCPUpdateScoreboard_21f4:
    subf.   r0,r0,r4
LTCPUpdateScoreboard_21f8:
    bgt LTCPUpdateScoreboard_26f0
LTCPUpdateScoreboard_21fc:
    lwz     r0,48(r22)
LTCPUpdateScoreboard_2200:
    subf.   r0,r4,r0
LTCPUpdateScoreboard_2204:
    ble LTCPUpdateScoreboard_220c
LTCPUpdateScoreboard_2208:
    b LTCPUpdateScoreboard_26f0
LTCPUpdateScoreboard_220c:
    addi    r24,r3,0
LTCPUpdateScoreboard_2210:
    addi    r26,r5,2
LTCPUpdateScoreboard_2214:
    addi    r28,r22,140
LTCPUpdateScoreboard_2218:
    b LTCPUpdateScoreboard_25a0
LTCPUpdateScoreboard_221c:
    lwz     r25,0(r26)
LTCPUpdateScoreboard_2220:
    addi    r24,r24,-8
LTCPUpdateScoreboard_2224:
    lwz     r30,4(r26)
LTCPUpdateScoreboard_2228:
    addi    r26,r26,8
LTCPUpdateScoreboard_222c:
    subf.   r0,r30,r25
LTCPUpdateScoreboard_2230:
    bge LTCPUpdateScoreboard_25a0
LTCPUpdateScoreboard_2234:
    lwz     r0,48(r22)
LTCPUpdateScoreboard_2238:
    subf.   r0,r30,r0
LTCPUpdateScoreboard_223c:
    bge LTCPUpdateScoreboard_25a0
LTCPUpdateScoreboard_2240:
    lwz     r0,8(r23)
LTCPUpdateScoreboard_2244:
    subf.   r0,r25,r0
LTCPUpdateScoreboard_2248:
    bge LTCPUpdateScoreboard_25a0
LTCPUpdateScoreboard_224c:
    lwz     r0,80(r22)
LTCPUpdateScoreboard_2250:
    subf.   r0,r0,r30
LTCPUpdateScoreboard_2254:
    bgt LTCPUpdateScoreboard_25a0
LTCPUpdateScoreboard_2258:
    lwz     r0,208(r22)
LTCPUpdateScoreboard_225c:
    subf.   r0,r0,r30
LTCPUpdateScoreboard_2260:
    ble LTCPUpdateScoreboard_2268
LTCPUpdateScoreboard_2264:
    stw     r30,208(r22)
LTCPUpdateScoreboard_2268:
    lwz     r0,204(r22)
LTCPUpdateScoreboard_226c:
    cmpwi   r0,0
LTCPUpdateScoreboard_2270:
    bne LTCPUpdateScoreboard_22cc
LTCPUpdateScoreboard_2274:
    li      r0,1
LTCPUpdateScoreboard_2278:
    stw     r0,204(r22)
LTCPUpdateScoreboard_227c:
    lwz     r0,8(r23)
LTCPUpdateScoreboard_2280:
    stw     r0,0(r28)
LTCPUpdateScoreboard_2284:
    stw     r25,4(r28)
LTCPUpdateScoreboard_2288:
    lwz     r0,0(r28)
LTCPUpdateScoreboard_228c:
    stw     r0,12(r28)
LTCPUpdateScoreboard_2290:
    lwz     r3,4(r28)
LTCPUpdateScoreboard_2294:
    lwz     r0,244(r22)
LTCPUpdateScoreboard_2298:
    subf    r3,r3,r30
LTCPUpdateScoreboard_229c:
    divw    r0,r3,r0
LTCPUpdateScoreboard_22a0:
    cmpwi   r0,3
LTCPUpdateScoreboard_22a4:
    ble LTCPUpdateScoreboard_22ac
LTCPUpdateScoreboard_22a8:
    li      r0,3
LTCPUpdateScoreboard_22ac:
    stw     r0,8(r28)
LTCPUpdateScoreboard_22b0:
    lwz     r0,8(r28)
LTCPUpdateScoreboard_22b4:
    cmpwi   r0,1
LTCPUpdateScoreboard_22b8:
    bge LTCPUpdateScoreboard_22c4
LTCPUpdateScoreboard_22bc:
    li      r0,1
LTCPUpdateScoreboard_22c0:
    stw     r0,8(r28)
LTCPUpdateScoreboard_22c4:
    stw     r30,224(r22)
LTCPUpdateScoreboard_22c8:
    b LTCPUpdateScoreboard_25a0
LTCPUpdateScoreboard_22cc:
    slwi    r3,r0,4
LTCPUpdateScoreboard_22d0:
    addi    r27,r3,140
LTCPUpdateScoreboard_22d4:
    add     r27,r22,r27
LTCPUpdateScoreboard_22d8:
    addi    r29,r22,140
LTCPUpdateScoreboard_22dc:
    subf    r31,r25,r30
LTCPUpdateScoreboard_22e0:
    b LTCPUpdateScoreboard_2528
LTCPUpdateScoreboard_22e4:
    lwz     r3,0(r29)
LTCPUpdateScoreboard_22e8:
    subf.   r0,r30,r3
LTCPUpdateScoreboard_22ec:
    bge LTCPUpdateScoreboard_2530
LTCPUpdateScoreboard_22f0:
    lwz     r4,4(r29)
LTCPUpdateScoreboard_22f4:
    subf.   r0,r4,r25
LTCPUpdateScoreboard_22f8:
    blt LTCPUpdateScoreboard_232c
LTCPUpdateScoreboard_22fc:
    lwz     r3,8(r29)
LTCPUpdateScoreboard_2300:
    addi    r0,r3,1
LTCPUpdateScoreboard_2304:
    stw     r0,8(r29)
LTCPUpdateScoreboard_2308:
    lwz     r3,4(r29)
LTCPUpdateScoreboard_230c:
    lwz     r0,244(r22)
LTCPUpdateScoreboard_2310:
    subf    r3,r3,r30
LTCPUpdateScoreboard_2314:
    divw    r0,r3,r0
LTCPUpdateScoreboard_2318:
    cmpwi   r0,3
LTCPUpdateScoreboard_231c:
    blt LTCPUpdateScoreboard_2524
LTCPUpdateScoreboard_2320:
    li      r0,3
LTCPUpdateScoreboard_2324:
    stw     r0,8(r29)
LTCPUpdateScoreboard_2328:
    b LTCPUpdateScoreboard_2524
LTCPUpdateScoreboard_232c:
    subf.   r0,r25,r3
LTCPUpdateScoreboard_2330:
    blt LTCPUpdateScoreboard_23b0
LTCPUpdateScoreboard_2334:
    lwz     r4,12(r29)
LTCPUpdateScoreboard_2338:
    subf.   r0,r4,r30
LTCPUpdateScoreboard_233c:
    ble LTCPUpdateScoreboard_2344
LTCPUpdateScoreboard_2340:
    b LTCPUpdateScoreboard_2348
LTCPUpdateScoreboard_2344:
    mr      r4,r30
LTCPUpdateScoreboard_2348:
    lwz     r0,216(r22)
LTCPUpdateScoreboard_234c:
    subf    r3,r3,r4
LTCPUpdateScoreboard_2350:
    subf    r0,r3,r0
LTCPUpdateScoreboard_2354:
    stw     r0,216(r22)
LTCPUpdateScoreboard_2358:
    lwz     r0,4(r29)
LTCPUpdateScoreboard_235c:
    subf.   r0,r0,r30
LTCPUpdateScoreboard_2360:
    blt LTCPUpdateScoreboard_238c
LTCPUpdateScoreboard_2364:
    addi    r4,r29,16
LTCPUpdateScoreboard_2368:
    addi    r3,r29,0
LTCPUpdateScoreboard_236c:
    subf    r5,r4,r27
LTCPUpdateScoreboard_2370:
    bl memmove
LTCPUpdateScoreboard_2374:
    lwz     r3,204(r22)
LTCPUpdateScoreboard_2378:
    addi    r27,r27,-16
LTCPUpdateScoreboard_237c:
    addi    r29,r29,-16
LTCPUpdateScoreboard_2380:
    addi    r0,r3,-1
LTCPUpdateScoreboard_2384:
    stw     r0,204(r22)
LTCPUpdateScoreboard_2388:
    b LTCPUpdateScoreboard_2524
LTCPUpdateScoreboard_238c:
    stw     r30,0(r29)
LTCPUpdateScoreboard_2390:
    lwz     r4,12(r29)
LTCPUpdateScoreboard_2394:
    lwz     r3,0(r29)
LTCPUpdateScoreboard_2398:
    subf.   r0,r4,r3
LTCPUpdateScoreboard_239c:
    ble LTCPUpdateScoreboard_23a4
LTCPUpdateScoreboard_23a0:
    b LTCPUpdateScoreboard_23a8
LTCPUpdateScoreboard_23a4:
    mr      r3,r4
LTCPUpdateScoreboard_23a8:
    stw     r3,12(r29)
LTCPUpdateScoreboard_23ac:
    b LTCPUpdateScoreboard_2524
LTCPUpdateScoreboard_23b0:
    subf.   r0,r4,r30
LTCPUpdateScoreboard_23b4:
    blt LTCPUpdateScoreboard_2420
LTCPUpdateScoreboard_23b8:
    lwz     r0,12(r29)
LTCPUpdateScoreboard_23bc:
    subf.   r3,r25,r0
LTCPUpdateScoreboard_23c0:
    ble LTCPUpdateScoreboard_23d0
LTCPUpdateScoreboard_23c4:
    lwz     r0,216(r22)
LTCPUpdateScoreboard_23c8:
    subf    r0,r3,r0
LTCPUpdateScoreboard_23cc:
    stw     r0,216(r22)
LTCPUpdateScoreboard_23d0:
    stw     r25,4(r29)
LTCPUpdateScoreboard_23d4:
    lwz     r3,12(r29)
LTCPUpdateScoreboard_23d8:
    lwz     r4,4(r29)
LTCPUpdateScoreboard_23dc:
    subf.   r0,r3,r4
LTCPUpdateScoreboard_23e0:
    ble LTCPUpdateScoreboard_23e8
LTCPUpdateScoreboard_23e4:
    b LTCPUpdateScoreboard_23ec
LTCPUpdateScoreboard_23e8:
    mr      r3,r4
LTCPUpdateScoreboard_23ec:
    stw     r3,12(r29)
LTCPUpdateScoreboard_23f0:
    lwz     r3,8(r29)
LTCPUpdateScoreboard_23f4:
    addi    r0,r3,1
LTCPUpdateScoreboard_23f8:
    stw     r0,8(r29)
LTCPUpdateScoreboard_23fc:
    lwz     r3,4(r29)
LTCPUpdateScoreboard_2400:
    lwz     r0,244(r22)
LTCPUpdateScoreboard_2404:
    subf    r3,r3,r30
LTCPUpdateScoreboard_2408:
    divw    r0,r3,r0
LTCPUpdateScoreboard_240c:
    cmpwi   r0,3
LTCPUpdateScoreboard_2410:
    blt LTCPUpdateScoreboard_2524
LTCPUpdateScoreboard_2414:
    li      r0,3
LTCPUpdateScoreboard_2418:
    stw     r0,8(r29)
LTCPUpdateScoreboard_241c:
    b LTCPUpdateScoreboard_2524
LTCPUpdateScoreboard_2420:
    lwz     r0,204(r22)
LTCPUpdateScoreboard_2424:
    cmpwi   r0,4
LTCPUpdateScoreboard_2428:
    bge LTCPUpdateScoreboard_2450
LTCPUpdateScoreboard_242c:
    addi    r4,r29,0
LTCPUpdateScoreboard_2430:
    addi    r3,r29,16
LTCPUpdateScoreboard_2434:
    subf    r5,r29,r27
LTCPUpdateScoreboard_2438:
    bl memmove
LTCPUpdateScoreboard_243c:
    lwz     r3,204(r22)
LTCPUpdateScoreboard_2440:
    addi    r27,r27,16
LTCPUpdateScoreboard_2444:
    addi    r0,r3,1
LTCPUpdateScoreboard_2448:
    stw     r0,204(r22)
LTCPUpdateScoreboard_244c:
    b LTCPUpdateScoreboard_247c
LTCPUpdateScoreboard_2450:
    addi    r5,r27,-16
LTCPUpdateScoreboard_2454:
    cmplw   r29,r5
LTCPUpdateScoreboard_2458:
    bge LTCPUpdateScoreboard_2478
LTCPUpdateScoreboard_245c:
    lwz     r0,-16(r27)
LTCPUpdateScoreboard_2460:
    addi    r4,r29,0
LTCPUpdateScoreboard_2464:
    addi    r3,r29,16
LTCPUpdateScoreboard_2468:
    stw     r0,224(r22)
LTCPUpdateScoreboard_246c:
    subf    r5,r29,r5
LTCPUpdateScoreboard_2470:
    bl memmove
LTCPUpdateScoreboard_2474:
    b LTCPUpdateScoreboard_247c
LTCPUpdateScoreboard_2478:
    stw     r30,224(r22)
LTCPUpdateScoreboard_247c:
    lwz     r3,12(r29)
LTCPUpdateScoreboard_2480:
    subf.   r0,r30,r3
LTCPUpdateScoreboard_2484:
    ble LTCPUpdateScoreboard_2498
LTCPUpdateScoreboard_2488:
    lwz     r0,216(r22)
LTCPUpdateScoreboard_248c:
    subf    r0,r31,r0
LTCPUpdateScoreboard_2490:
    stw     r0,216(r22)
LTCPUpdateScoreboard_2494:
    b LTCPUpdateScoreboard_24ac
LTCPUpdateScoreboard_2498:
    subf.   r3,r25,r3
LTCPUpdateScoreboard_249c:
    ble LTCPUpdateScoreboard_24ac
LTCPUpdateScoreboard_24a0:
    lwz     r0,216(r22)
LTCPUpdateScoreboard_24a4:
    subf    r0,r3,r0
LTCPUpdateScoreboard_24a8:
    stw     r0,216(r22)
LTCPUpdateScoreboard_24ac:
    stw     r25,4(r29)
LTCPUpdateScoreboard_24b0:
    lwz     r3,12(r29)
LTCPUpdateScoreboard_24b4:
    lwz     r4,4(r29)
LTCPUpdateScoreboard_24b8:
    subf.   r0,r3,r4
LTCPUpdateScoreboard_24bc:
    ble LTCPUpdateScoreboard_24c4
LTCPUpdateScoreboard_24c0:
    b LTCPUpdateScoreboard_24c8
LTCPUpdateScoreboard_24c4:
    mr      r3,r4
LTCPUpdateScoreboard_24c8:
    stw     r3,12(r29)
LTCPUpdateScoreboard_24cc:
    lwz     r3,8(r29)
LTCPUpdateScoreboard_24d0:
    addi    r0,r3,1
LTCPUpdateScoreboard_24d4:
    stw     r0,8(r29)
LTCPUpdateScoreboard_24d8:
    lwz     r3,4(r29)
LTCPUpdateScoreboard_24dc:
    lwz     r0,244(r22)
LTCPUpdateScoreboard_24e0:
    subf    r3,r3,r30
LTCPUpdateScoreboard_24e4:
    divw    r0,r3,r0
LTCPUpdateScoreboard_24e8:
    cmpwi   r0,3
LTCPUpdateScoreboard_24ec:
    blt LTCPUpdateScoreboard_24f8
LTCPUpdateScoreboard_24f0:
    li      r0,3
LTCPUpdateScoreboard_24f4:
    stw     r0,8(r29)
LTCPUpdateScoreboard_24f8:
    addi    r29,r29,16
LTCPUpdateScoreboard_24fc:
    cmplw   r29,r27
LTCPUpdateScoreboard_2500:
    bge LTCPUpdateScoreboard_2524
LTCPUpdateScoreboard_2504:
    stw     r30,0(r29)
LTCPUpdateScoreboard_2508:
    lwz     r4,12(r29)
LTCPUpdateScoreboard_250c:
    lwz     r3,0(r29)
LTCPUpdateScoreboard_2510:
    subf.   r0,r4,r3
LTCPUpdateScoreboard_2514:
    ble LTCPUpdateScoreboard_251c
LTCPUpdateScoreboard_2518:
    b LTCPUpdateScoreboard_2520
LTCPUpdateScoreboard_251c:
    mr      r3,r4
LTCPUpdateScoreboard_2520:
    stw     r3,12(r29)
LTCPUpdateScoreboard_2524:
    addi    r29,r29,16
LTCPUpdateScoreboard_2528:
    cmplw   r29,r27
LTCPUpdateScoreboard_252c:
    blt LTCPUpdateScoreboard_22e4
LTCPUpdateScoreboard_2530:
    lwz     r0,224(r22)
LTCPUpdateScoreboard_2534:
    subf.   r0,r0,r25
LTCPUpdateScoreboard_2538:
    ble LTCPUpdateScoreboard_25a0
LTCPUpdateScoreboard_253c:
    lwz     r3,204(r22)
LTCPUpdateScoreboard_2540:
    cmpwi   r3,4
LTCPUpdateScoreboard_2544:
    bge LTCPUpdateScoreboard_25a0
LTCPUpdateScoreboard_2548:
    addi    r0,r3,1
LTCPUpdateScoreboard_254c:
    stw     r0,204(r22)
LTCPUpdateScoreboard_2550:
    slwi    r3,r3,4
LTCPUpdateScoreboard_2554:
    addi    r3,r3,140
LTCPUpdateScoreboard_2558:
    lwz     r0,224(r22)
LTCPUpdateScoreboard_255c:
    add     r3,r22,r3
LTCPUpdateScoreboard_2560:
    stw     r0,0(r3)
LTCPUpdateScoreboard_2564:
    stw     r25,4(r3)
LTCPUpdateScoreboard_2568:
    lwz     r0,244(r22)
LTCPUpdateScoreboard_256c:
    divw    r0,r31,r0
LTCPUpdateScoreboard_2570:
    cmpwi   r0,3
LTCPUpdateScoreboard_2574:
    ble LTCPUpdateScoreboard_257c
LTCPUpdateScoreboard_2578:
    li      r0,3
LTCPUpdateScoreboard_257c:
    stw     r0,8(r3)
LTCPUpdateScoreboard_2580:
    lwz     r0,8(r3)
LTCPUpdateScoreboard_2584:
    cmpwi   r0,1
LTCPUpdateScoreboard_2588:
    bge LTCPUpdateScoreboard_2594
LTCPUpdateScoreboard_258c:
    li      r0,1
LTCPUpdateScoreboard_2590:
    stw     r0,8(r3)
LTCPUpdateScoreboard_2594:
    lwz     r0,0(r3)
LTCPUpdateScoreboard_2598:
    stw     r0,12(r3)
LTCPUpdateScoreboard_259c:
    stw     r30,224(r22)
LTCPUpdateScoreboard_25a0:
    cmpwi   r24,0
LTCPUpdateScoreboard_25a4:
    bgt LTCPUpdateScoreboard_221c
LTCPUpdateScoreboard_25a8:
    li      r0,0
LTCPUpdateScoreboard_25ac:
    stw     r0,216(r22)
LTCPUpdateScoreboard_25b0:
    addi    r6,r22,140
LTCPUpdateScoreboard_25b4:
    lwz     r0,204(r22)
LTCPUpdateScoreboard_25b8:
    slwi    r3,r0,4
LTCPUpdateScoreboard_25bc:
    addi    r4,r3,140
LTCPUpdateScoreboard_25c0:
    add     r4,r22,r4
LTCPUpdateScoreboard_25c4:
    addi    r3,r4,15
LTCPUpdateScoreboard_25c8:
    subf    r3,r6,r3
LTCPUpdateScoreboard_25cc:
    cmplw   r6,r4
LTCPUpdateScoreboard_25d0:
    srwi    r3,r3,4
LTCPUpdateScoreboard_25d4:
    bge LTCPUpdateScoreboard_26d8
LTCPUpdateScoreboard_25d8:
    srwi.   r0,r3,3
LTCPUpdateScoreboard_25dc:
    mtctr   r0
LTCPUpdateScoreboard_25e0:
    beq LTCPUpdateScoreboard_26b4
LTCPUpdateScoreboard_25e4:
    lwz     r4,0(r6)
LTCPUpdateScoreboard_25e8:
    lwz     r0,12(r6)
LTCPUpdateScoreboard_25ec:
    lwz     r5,216(r22)
LTCPUpdateScoreboard_25f0:
    subf    r0,r4,r0
LTCPUpdateScoreboard_25f4:
    add     r0,r5,r0
LTCPUpdateScoreboard_25f8:
    stw     r0,216(r22)
LTCPUpdateScoreboard_25fc:
    lwz     r4,16(r6)
LTCPUpdateScoreboard_2600:
    lwz     r0,28(r6)
LTCPUpdateScoreboard_2604:
    lwz     r5,216(r22)
LTCPUpdateScoreboard_2608:
    subf    r0,r4,r0
LTCPUpdateScoreboard_260c:
    add     r0,r5,r0
LTCPUpdateScoreboard_2610:
    stw     r0,216(r22)
LTCPUpdateScoreboard_2614:
    lwz     r4,32(r6)
LTCPUpdateScoreboard_2618:
    lwz     r0,44(r6)
LTCPUpdateScoreboard_261c:
    lwz     r5,216(r22)
LTCPUpdateScoreboard_2620:
    subf    r0,r4,r0
LTCPUpdateScoreboard_2624:
    add     r0,r5,r0
LTCPUpdateScoreboard_2628:
    stw     r0,216(r22)
LTCPUpdateScoreboard_262c:
    lwz     r4,48(r6)
LTCPUpdateScoreboard_2630:
    lwz     r0,60(r6)
LTCPUpdateScoreboard_2634:
    lwz     r5,216(r22)
LTCPUpdateScoreboard_2638:
    subf    r0,r4,r0
LTCPUpdateScoreboard_263c:
    add     r0,r5,r0
LTCPUpdateScoreboard_2640:
    stw     r0,216(r22)
LTCPUpdateScoreboard_2644:
    lwz     r4,64(r6)
LTCPUpdateScoreboard_2648:
    lwz     r0,76(r6)
LTCPUpdateScoreboard_264c:
    lwz     r5,216(r22)
LTCPUpdateScoreboard_2650:
    subf    r0,r4,r0
LTCPUpdateScoreboard_2654:
    add     r0,r5,r0
LTCPUpdateScoreboard_2658:
    stw     r0,216(r22)
LTCPUpdateScoreboard_265c:
    lwz     r4,80(r6)
LTCPUpdateScoreboard_2660:
    lwz     r0,92(r6)
LTCPUpdateScoreboard_2664:
    lwz     r5,216(r22)
LTCPUpdateScoreboard_2668:
    subf    r0,r4,r0
LTCPUpdateScoreboard_266c:
    add     r0,r5,r0
LTCPUpdateScoreboard_2670:
    stw     r0,216(r22)
LTCPUpdateScoreboard_2674:
    lwz     r4,96(r6)
LTCPUpdateScoreboard_2678:
    lwz     r0,108(r6)
LTCPUpdateScoreboard_267c:
    lwz     r5,216(r22)
LTCPUpdateScoreboard_2680:
    subf    r0,r4,r0
LTCPUpdateScoreboard_2684:
    add     r0,r5,r0
LTCPUpdateScoreboard_2688:
    stw     r0,216(r22)
LTCPUpdateScoreboard_268c:
    lwz     r4,112(r6)
LTCPUpdateScoreboard_2690:
    lwz     r0,124(r6)
LTCPUpdateScoreboard_2694:
    addi    r6,r6,128
LTCPUpdateScoreboard_2698:
    lwz     r5,216(r22)
LTCPUpdateScoreboard_269c:
    subf    r0,r4,r0
LTCPUpdateScoreboard_26a0:
    add     r0,r5,r0
LTCPUpdateScoreboard_26a4:
    stw     r0,216(r22)
LTCPUpdateScoreboard_26a8:
    bdnz LTCPUpdateScoreboard_25e4
LTCPUpdateScoreboard_26ac:
    andi.   r3,r3,7
LTCPUpdateScoreboard_26b0:
    beq LTCPUpdateScoreboard_26d8
LTCPUpdateScoreboard_26b4:
    mtctr   r3
LTCPUpdateScoreboard_26b8:
    lwz     r4,0(r6)
LTCPUpdateScoreboard_26bc:
    lwz     r0,12(r6)
LTCPUpdateScoreboard_26c0:
    addi    r6,r6,16
LTCPUpdateScoreboard_26c4:
    lwz     r5,216(r22)
LTCPUpdateScoreboard_26c8:
    subf    r0,r4,r0
LTCPUpdateScoreboard_26cc:
    add     r0,r5,r0
LTCPUpdateScoreboard_26d0:
    stw     r0,216(r22)
LTCPUpdateScoreboard_26d4:
    bdnz LTCPUpdateScoreboard_26b8
LTCPUpdateScoreboard_26d8:
    lwz     r3,208(r22)
LTCPUpdateScoreboard_26dc:
    lwz     r0,52(r22)
LTCPUpdateScoreboard_26e0:
    lwz     r4,216(r22)
LTCPUpdateScoreboard_26e4:
    subf    r0,r3,r0
LTCPUpdateScoreboard_26e8:
    add     r0,r4,r0
LTCPUpdateScoreboard_26ec:
    stw     r0,212(r22)
LTCPUpdateScoreboard_26f0:
    lmw     r22,24(r1)
LTCPUpdateScoreboard_26f4:
    lwz     r0,68(r1)
LTCPUpdateScoreboard_26f8:
    addi    r1,r1,64
LTCPUpdateScoreboard_26fc:
    mtlr    r0
LTCPUpdateScoreboard_2700:
    blr
}

static asm void __TCPTrimSegment(void)
{
    nofralloc
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    blr
}

