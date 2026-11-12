#include <dolphin/ip/ip.h>

#include <string.h>
extern void OSCancelAlarm(void*);
extern BOOL OSDisableInterrupts(void);
extern void OSRestoreInterrupts(BOOL);
extern void OSCreateAlarm(void*);
extern u64 OSGetTime(void);
extern void OSSetPeriodicAlarm(void*, u64, u64, void*);
extern u16 IPCheckSum(void*);

static u8 Control[0x40];

static asm u32 SaveFirstFragment(u8* frag, u8* ip);
static asm u8* Reassemble(u8* frag, u8* ip, u32 size);

static asm void TimeoutHandler(void)
{
    nofralloc
    mflr r0
    lis r3, Control@ha
    stw r0,4(r1)
    stwu r1,-40(r1)
    stw r31,36(r1)
    addi r31,r3,Control@l
    stw r30,32(r1)
    addi r30,r31,4
    stw r29,28(r1)
    lwz r0,4(r31)
    cmplwi r0,0
    beq L_th_end
    mr r29,r0
    b L_th_cond
L_th_body:
    lbz r0,76(r29)
    addi r5,r29,76
    cmplwi r0,0
    beq L_th_next
    lbz r3,8(r5)
    cmplwi r3,1
    bgt L_th_dec
    li r4,0
    stb r4,0(r5)
    addi r5,r29,8
    lbz r0,8(r29)
    cmplwi r0,0
    beq L_th_next
    li r0,11
    stb r0,16(r1)
    li r0,1
    addi r3,r1,16
    stb r0,17(r1)
    stw r4,20(r1)
    lwz r4,0(r29)
    lwz r6,4(r29)
    bl ICMPSendError
    b L_th_next
L_th_dec:
    addi r0,r3,-1
    stb r0,8(r5)
L_th_next:
    lwz r0,16(r31)
    add r29,r29,r0
L_th_cond:
    lwz r3,0(r30)
    lwz r0,8(r31)
    add r0,r3,r0
    cmplw r29,r0
    blt L_th_body
L_th_end:
    lwz r0,44(r1)
    lwz r31,36(r1)
    lwz r30,32(r1)
    lwz r29,28(r1)
    addi r1,r1,40
    mtlr r0
    blr
}

asm u32 IPSetReassemblyBuffer(u8* buffer, u32 size, u32 mtu)
{
    nofralloc
    mflr r0
    cmpwi r5,576
    stw r0,4(r1)
    lis r6, Control@ha
    stwu r1,-48(r1)
    stmw r27,28(r1)
    addi r29,r3,0
    addi r31,r6,Control@l
    ble L_srb_1
    b L_srb_2
L_srb_1:
    li r5,576
L_srb_2:
    lis r3,1
    addi r30,r3,-1
    cmpw r5,r30
    bge L_srb_3
    mr r30,r5
L_srb_3:
    cmplwi r29,0
    addi r0,r30,115
    clrrwi r28,r0,5
    beq L_srb_4
    cmpw r4,r28
    bge L_srb_5
L_srb_4:
    lwz r0,0(r31)
    clrlwi. r0,r0,31
    beq L_srb_6
    addi r3,r31,24
    bl OSCancelAlarm
    lwz r0,0(r31)
    clrrwi r0,r0,1
    stw r0,0(r31)
L_srb_6:
    li r3,-12
    b L_srb_end
L_srb_5:
    divw r0,r4,r28
    mullw r0,r0,r28
    subf r0,r0,r4
    subf r27,r0,r4
    addi r3,r29,0
    addi r5,r27,0
    li r4,0
    bl memset
    bl OSDisableInterrupts
    stw r29,4(r31)
    mr r29,r3
    stw r27,8(r31)
    stw r30,12(r31)
    stw r28,16(r31)
    lwz r3,0(r31)
    clrlwi. r0,r3,31
    bne L_srb_7
    ori r0,r3,1
    addi r30,r31,24
    stw r0,0(r31)
    mr r3,r30
    bl OSCreateAlarm
    bl OSGetTime
    lis r5,-32768
    lwz r0,248(r5)
    addi r5,r3,0
    lis r3, TimeoutHandler@ha
    addi r9,r3,TimeoutHandler@l
    srwi r8,r0,2
    addi r3,r30,0
    addi r6,r4,0
    li r7,0
    bl OSSetPeriodicAlarm
L_srb_7:
    mr r3,r29
    bl OSRestoreInterrupts
    li r3,0
L_srb_end:
    lmw r27,28(r1)
    lwz r0,52(r1)
    addi r1,r1,48
    mtlr r0
    blr
}

static asm u8* Reassemble(u8* frag, u8* ip, u32 size)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-48(r1)
    stmw r26,24(r1)
    mr r26,r3
    mr r27,r4
    lbz r0,0(r4)
    lhz r3,2(r4)
    lhz r7,6(r4)
    rlwinm r4,r0,2,26,29
    subf r3,r4,r3
    lbz r0,0(r26)
    rlwinm r30,r7,3,16,28
    addi r4,r3,-1
    add r4,r30,r4
    rlwinm r0,r0,2,26,29
    addi r6,r4,1
    add r6,r0,r6
    addi r29,r4,0
    cmpw r5,r6
    addi r4,r6,0
    blt L_ra_1
    cmpwi r3,0
    ble L_ra_1
    rlwinm. r0,r7,0,18,18
    beq L_ra_2
    srawi r0,r3,3
    addze r0,r0
    slwi r0,r0,3
    subfc. r0,r0,r3
    beq L_ra_2
L_ra_1:
    li r0,0
    stb r0,0(r26)
    li r3,0
    b L_ra_end
L_ra_2:
    lhz r0,2(r26)
    cmpw r0,r4
    ble L_ra_3
    b L_ra_4
L_ra_3:
    mr r0,r4
L_ra_4:
    sth r0,2(r26)
    addi r28,r26,10
    addi r31,r30,-1
    b L_ra_5
L_ra_6:
    lbz r0,0(r26)
    rlwinm r0,r0,2,26,29
    add r3,r0,r3
    add r3,r26,r3
    lhz r4,2(r3)
    cmpw r4,r30
    blt L_ra_7
    lhz r5,0(r3)
    cmpw r29,r5
    bge L_ra_8
L_ra_7:
    addi r28,r3,4
    b L_ra_5
L_ra_8:
    lhz r0,4(r3)
    cmpw r5,r30
    sth r0,0(r28)
    bge L_ra_9
    sth r31,2(r3)
    li r6,0
    lhz r0,0(r3)
    sth r0,0(r28)
    addi r28,r3,4
    b L_ra_10
L_ra_9:
    subf r6,r30,r5
L_ra_10:
    cmpw r29,r4
    bge L_ra_11
    lhz r0,6(r27)
    rlwinm. r0,r0,0,18,18
    beq L_ra_12
    lbz r3,0(r26)
    addi r0,r29,1
    rlwinm r3,r3,2,26,29
    add r3,r3,r29
    addi r3,r3,1
    add r3,r26,r3
    sth r0,0(r3)
    sth r4,2(r3)
    lhz r0,0(r28)
    sth r0,4(r3)
    lhz r0,0(r3)
    sth r0,0(r28)
    addi r28,r3,4
    b L_ra_13
L_ra_12:
    li r0,1
    sth r0,0(r28)
    b L_ra_13
L_ra_11:
    mr r29,r4
L_ra_13:
    lbz r4,0(r26)
    add r0,r30,r6
    lbz r3,0(r27)
    subf r5,r0,r29
    rlwinm r0,r4,2,26,29
    add r4,r0,r30
    rlwinm r0,r3,2,26,29
    add r3,r4,r6
    add r4,r0,r6
    add r3,r26,r3
    add r4,r27,r4
    addi r5,r5,1
    bl memmove
L_ra_5:
    lhz r3,0(r28)
    cmplwi r3,1
    bne L_ra_6
    lhz r0,10(r26)
    cmplwi r0,1
    bne L_ra_14
    mr r3,r26
    b L_ra_end
L_ra_14:
    li r3,0
L_ra_end:
    lmw r26,24(r1)
    lwz r0,52(r1)
    addi r1,r1,48
    mtlr r0
    blr
}

static asm u32 SaveFirstFragment(u8* frag, u8* ip)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-32(r1)
    stw r31,28(r1)
    mr r31,r4
    stw r30,24(r1)
    stw r29,20(r1)
    stw r28,16(r1)
    addi r28,r3,0
    lhz r0,6(r4)
    rlwinm. r0,r0,3,16,28
    beq L_sf_1
    li r3,1
    b L_sf_end
L_sf_1:
    lbz r0,0(r31)
    lhz r3,2(r31)
    rlwinm r4,r0,2,26,29
    addi r5,r4,8
    cmpw r3,r5
    bge L_sf_2
    li r3,0
    b L_sf_end
L_sf_2:
    lbz r0,8(r28)
    cmplwi r0,0
    beq L_sf_3
    rlwinm r0,r0,2,26,29
    cmpw r4,r0
    bne L_sf_4
    li r3,1
    b L_sf_end
L_sf_4:
    li r3,0
    b L_sf_end
L_sf_3:
    addi r4,r31,0
    addi r3,r28,8
    bl memmove
    lbz r3,76(r28)
    addi r29,r28,76
    lbz r0,0(r31)
    rlwinm r3,r3,2,26,29
    rlwinm r5,r0,2,26,29
    cmpw r3,r5
    bne L_sf_5
    li r3,1
    b L_sf_end
L_sf_5:
    lis r3, Control@ha
    lhz r6,2(r29)
    addi r3,r3,Control@l
    addi r4,r5,-20
    lwz r3,12(r3)
    add r0,r6,r4
    cmpw r3,r0
    addi r28,r4,0
    bge L_sf_6
    li r3,0
    b L_sf_end
L_sf_6:
    addi r30,r29,20
    addi r4,r30,0
    add r3,r29,r5
    addi r5,r6,-12
    bl memmove
    addi r3,r30,0
    addi r5,r28,0
    addi r4,r31,20
    bl memmove
    lbz r0,0(r31)
    li r3,1
    stb r0,0(r29)
    lhz r0,2(r29)
    add r0,r0,r28
    sth r0,2(r29)
L_sf_end:
    lwz r0,36(r1)
    lwz r31,28(r1)
    lwz r30,24(r1)
    lwz r29,20(r1)
    lwz r28,16(r1)
    addi r1,r1,32
    mtlr r0
    blr
}

asm u8* IPReassemble(u8* ip, u32 id, u32 src, u32 dst)
{
    nofralloc
    mflr r0
    lis r6, Control@ha
    stw r0,4(r1)
    stwu r1,-48(r1)
    stmw r26,24(r1)
    addi r31,r6,Control@l
    addi r27,r4,0
    lwz r7,4(r31)
    cmplwi r7,0
    bne L_ip_1
    li r3,0
    b L_ip_end
L_ip_1:
    lwz r0,8(r31)
    mr r29,r7
    lwz r6,16(r31)
    li r30,0
    add r7,r7,r0
    b L_ip_loop
L_ip_body:
    lbz r0,76(r29)
    addi r28,r29,76
    cmplwi r0,0
    bne L_ip_2
    cmplwi r30,0
    bne L_ip_next
    mr r30,r29
    b L_ip_next
L_ip_2:
    lhz r4,4(r28)
    lhz r0,4(r27)
    cmplw r4,r0
    bne L_ip_next
    lbz r0,9(r28)
    lbz r8,9(r27)
    cmplw r0,r8
    bne L_ip_next
    lwz r4,12(r28)
    lwz r0,12(r27)
    cmplw r4,r0
    bne L_ip_next
    lwz r4,16(r28)
    lwz r0,16(r27)
    cmplw r4,r0
    bne L_ip_next
    lwz r0,0(r29)
    cmplw r0,r3
    bne L_ip_3
    lwz r0,4(r29)
    cmplw r0,r5
    beq L_ip_4
L_ip_3:
    li r0,0
    stb r0,0(r28)
    li r3,0
    b L_ip_end
L_ip_4:
    cmplwi r8,6
    bne L_ip_5
    lhz r0,6(r27)
    rlwinm r0,r0,3,16,28
    cmpwi r0,8
    bne L_ip_5
    li r0,0
    stb r0,0(r28)
    li r3,0
    b L_ip_end
L_ip_5:
    addi r3,r29,0
    addi r4,r27,0
    bl SaveFirstFragment
    cmpwi r3,0
    bne L_ip_6
    li r0,0
    stb r0,0(r28)
    li r3,0
    b L_ip_end
L_ip_6:
    lwz r5,12(r31)
    addi r3,r28,0
    addi r4,r27,0
    bl Reassemble
    mr. r26,r3
    beq L_ip_7
    lhz r4,6(r26)
    li r0,0
    addi r3,r26,0
    clrrwi r4,r4,14
    sth r4,6(r26)
    lbz r4,9(r29)
    stb r4,1(r26)
    lbz r4,16(r29)
    stb r4,8(r26)
    sth r0,10(r26)
    bl IPCheckSum
    sth r3,10(r26)
L_ip_7:
    mr r3,r26
    b L_ip_end
L_ip_next:
    add r29,r29,r6
L_ip_loop:
    cmplw r29,r7
    blt L_ip_body
    lbz r0,9(r27)
    cmplwi r0,6
    bne L_ip_8
    lhz r0,6(r27)
    rlwinm r0,r0,3,16,28
    cmpwi r0,8
    bne L_ip_8
    li r3,0
    b L_ip_end
L_ip_8:
    cmplwi r30,0
    beq L_ip_9
    stw r3,0(r30)
    addi r26,r30,76
    li r28,0
    stw r5,4(r30)
    addi r3,r26,0
    addi r4,r27,0
    stb r28,8(r30)
    li r5,20
    bl memmove
    li r0,69
    stb r0,0(r26)
    li r0,60
    li r5,20
    stb r0,8(r26)
    li r0,1
    addi r29,r31,12
    sth r28,10(r26)
    addi r3,r30,0
    addi r4,r27,0
    sth r5,2(r26)
    lbz r5,0(r26)
    rlwinm r5,r5,2,26,29
    add r7,r26,r5
    sth r28,0(r7)
    lbz r6,0(r26)
    lwz r5,12(r31)
    rlwinm r6,r6,2,26,29
    subf r5,r6,r5
    sth r5,2(r7)
    sth r0,4(r7)
    bl SaveFirstFragment
    cmpwi r3,0
    bne L_ip_11
    stb r28,0(r26)
    li r3,0
    b L_ip_end
L_ip_11:
    lwz r5,0(r29)
    addi r3,r26,0
    addi r4,r27,0
    bl Reassemble
L_ip_9:
    li r3,0
L_ip_end:
    lmw r26,24(r1)
    lwz r0,52(r1)
    addi r1,r1,48
    mtlr r0
    blr
}
