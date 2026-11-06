#include <dolphin/types.h>
#include <string.h>

extern u32 IPLimited;
extern u32 IPAddrAny;
extern u16 PPPDeleteOpt(u8*, u8*);
extern void IPInitRoute(u8*, u8*, u8*);
extern s32 SOGetResolver(u32*, u32*);
extern void SOSetResolver(u32, u32);
extern void OSCreateAlarm(void*);

static asm s32 ReceiveConfigureRequest(u8* ppp, u8* ip);
static asm s32 ReceiveConfigureAck(u8* ppp, u8* ip);
static asm s32 ReceiveConfigureNak(u8* ppp, u8* ip);
static asm s32 ReceiveConfigureReject(u8* ppp, u8* ip);
static asm s32 Up(u8* ppp);
static asm s32 Down(u8* ppp);
static asm s32 Started(u8* ppp);
static asm s32 Finished(u8* ppp);

static asm s32 ReceiveConfigureRequest(u8* ppp, u8* ip)
{
    nofralloc
    mflr r0
    li r7,0
    stw r0,4(r1)
    stwu r1,-40(r1)
    stw r31,36(r1)
    mr r31,r4
    addi r0,r31,4
    stw r30,32(r1)
    mr r6,r0
    stw r29,28(r1)
    mr r29,r0
    stw r28,24(r1)
    li r28,0
    lhz r4,2(r4)
    addi r4,r4,-4
    clrlwi r4,r4,16
    add r5,r29,r4
    b L_r_cond
L_r_body:
    lbz r0,0(r6)
    cmpwi r0,3
    beq L_r_1
    b L_r_bad
L_r_1:
    lbz r0,1(r6)
    cmplwi r0,6
    beq L_r_2
    li r3,-3
    b L_r_end
L_r_2:
    lwz r0,2(r6)
    cmplwi r0,0
    mr r28,r0
    bne L_r_next
    li r7,1
    b L_r_next
L_r_bad:
    li r7,1
L_r_next:
    lbz r0,1(r6)
    add r6,r6,r0
L_r_cond:
    cmplw r6,r5
    bge L_r_3
    cmpwi r7,0
    beq L_r_body
L_r_3:
    cmpwi r7,0
    beq L_r_ok
    mr r30,r29
    b L_r_4
L_r_5:
    lbz r0,0(r30)
    cmpwi r0,3
    beq L_r_6
    b L_r_7
L_r_6:
    cmplwi r28,0
    beq L_r_7
    addi r3,r29,0
    addi r5,r30,0
    bl PPPDeleteOpt
    mr r4,r3
    b L_r_4
L_r_7:
    lbz r0,1(r30)
    add r30,r30,r0
L_r_4:
    clrlwi r0,r4,16
    add r0,r29,r0
    cmplw r30,r0
    blt L_r_5
    addi r0,r4,4
    sth r0,2(r31)
    li r3,-1
    b L_r_end
L_r_ok:
    stw r28,152(r3)
    li r3,0
L_r_end:
    lwz r0,44(r1)
    lwz r31,36(r1)
    lwz r30,32(r1)
    lwz r29,28(r1)
    lwz r28,24(r1)
    addi r1,r1,40
    mtlr r0
    blr
}

static asm s32 ReceiveConfigureAck(u8* ppp, u8* ip)
{
    nofralloc
    mflr r0
    mr r6,r3
    stw r0,4(r1)
    stwu r1,-8(r1)
    lhz r5,48(r6)
    lhz r3,2(r4)
    addi r0,r5,4
    cmpw r3,r0
    bne L_a_bad
    addi r3,r4,4
    addi r4,r6,28
    bl memcmp
    cmpwi r3,0
    beq L_a_ok
L_a_bad:
    li r3,0
    b L_a_end
L_a_ok:
    li r3,1
L_a_end:
    lwz r0,12(r1)
    addi r1,r1,8
    mtlr r0
    blr
}

static asm s32 ReceiveConfigureNak(u8* ppp, u8* ip)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-40(r1)
    stw r31,36(r1)
    addi r31,r3,0
    stw r30,32(r1)
    stw r29,28(r1)
    lhz r5,2(r4)
    addi r4,r4,4
    addi r29,r4,0
    addi r0,r5,-4
    clrlwi r0,r0,16
    add r30,r4,r0
    b L_n_cond
L_n_body:
    lbz r4,0(r29)
    cmpwi r4,129
    beq L_n_1
    bge L_n_2
    cmpwi r4,3
    beq L_n_1
    b L_n_next
L_n_2:
    cmpwi r4,131
    beq L_n_1
    b L_n_next
L_n_1:
    lbz r0,1(r29)
    cmplwi r0,6
    beq L_n_3
    li r3,0
    b L_n_end
L_n_3:
    lhz r3,48(r31)
    addi r5,r31,28
    addi r3,r3,28
    add r3,r31,r3
    b L_n_optcond
L_n_opt:
    lbz r0,0(r5)
    cmplw r4,r0
    bne L_n_optnext
    cmpwi r0,129
    beq L_n_set
    bge L_n_opt2
    cmpwi r0,3
    beq L_n_set
    b L_n_after
L_n_opt2:
    cmpwi r0,131
    beq L_n_set
    b L_n_after
L_n_set:
    lwz r0,2(r29)
    stw r0,2(r5)
    b L_n_after
L_n_optnext:
    lbz r0,1(r5)
    add r5,r5,r0
L_n_optcond:
    cmplw r5,r3
    blt L_n_opt
L_n_after:
    cmplw r3,r5
    bgt L_n_next
    lbz r5,1(r29)
    mr r4,r29
    bl memmove
    lhz r3,48(r31)
    lbz r0,1(r29)
    add r0,r3,r0
    sth r0,48(r31)
L_n_next:
    lbz r0,1(r29)
    add r29,r29,r0
L_n_cond:
    cmplw r29,r30
    blt L_n_body
    li r3,1
L_n_end:
    lwz r0,44(r1)
    lwz r31,36(r1)
    lwz r30,32(r1)
    lwz r29,28(r1)
    addi r1,r1,40
    mtlr r0
    blr
}

static asm s32 ReceiveConfigureReject(u8* ppp, u8* ip)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-40(r1)
    stw r31,36(r1)
    stw r30,32(r1)
    stw r29,28(r1)
    stw r28,24(r1)
    addi r28,r3,0
    addi r3,r4,4
    lhz r5,2(r4)
    addi r29,r3,0
    addi r30,r28,28
    addi r0,r5,-4
    clrlwi r0,r0,16
    add r31,r3,r0
    b L_j_cond
L_j_body:
    lhz r4,48(r28)
    lbz r3,0(r29)
    addi r5,r4,28
    add r5,r28,r5
    b L_j_optcond
L_j_opt:
    cmplw r5,r30
    bgt L_j_optnext
    li r3,0
    b L_j_end
L_j_optnext:
    lbz r0,1(r30)
    add r30,r30,r0
L_j_optcond:
    lbz r0,0(r30)
    cmplw r3,r0
    bne L_j_opt
    addi r5,r30,0
    addi r3,r28,28
    bl PPPDeleteOpt
    sth r3,48(r28)
    lbz r0,1(r29)
    add r29,r29,r0
L_j_cond:
    cmplw r29,r31
    blt L_j_body
    li r3,1
L_j_end:
    lwz r0,44(r1)
    lwz r31,36(r1)
    lwz r30,32(r1)
    lwz r29,28(r1)
    lwz r28,24(r1)
    addi r1,r1,40
    mtlr r0
    blr
}

static asm s32 Up(u8* ppp)
{
    nofralloc
    mflr r0
    addi r5,r3,28
    stw r0,4(r1)
    li r6,0
    stwu r1,-32(r1)
    stw r31,28(r1)
    li r31,0
    stw r30,24(r1)
    li r30,0
    lhz r4,48(r3)
    addi r4,r4,28
    add r4,r3,r4
    b L_u_cond
L_u_body:
    lbz r0,0(r5)
    cmpwi r0,129
    beq L_u_1
    bge L_u_2
    cmpwi r0,3
    beq L_u_3
    b L_u_next
L_u_2:
    cmpwi r0,131
    beq L_u_4
    b L_u_next
L_u_3:
    addi r6,r5,2
    b L_u_next
L_u_1:
    addi r31,r5,2
    b L_u_next
L_u_4:
    addi r30,r5,2
L_u_next:
    lbz r0,1(r5)
    add r5,r5,r0
L_u_cond:
    cmplw r5,r4
    blt L_u_body
    lwz r0,152(r3)
    cmplwi r0,0
    beq L_u_5
    addi r5,r3,152
    b L_u_6
L_u_5:
    li r5,IPLimited
L_u_6:
    addi r3,r6,0
    li r4,IPLimited
    bl IPInitRoute
    addi r3,r1,16
    addi r4,r1,12
    bl SOGetResolver
    cmpwi r3,0
    bne L_u_9
    lwz r0,16(r1)
    lwz r3,IPAddrAny
    cmplw r0,r3
    bne L_u_7
    lwz r0,12(r1)
    cmplw r0,r3
    bne L_u_7
    addi r3,r31,0
    addi r4,r30,0
    bl SOSetResolver
    b L_u_9
L_u_7:
    addi r3,r1,16
    addi r4,r1,12
    bl SOSetResolver
L_u_9:
    lwz r0,36(r1)
    li r3,1
    lwz r31,28(r1)
    lwz r30,24(r1)
    addi r1,r1,32
    mtlr r0
    blr
}

static asm s32 Down(u8* ppp)
{
    nofralloc
    mflr r0
    li r3,0
    stw r0,4(r1)
    li r4,0
    li r5,0
    stwu r1,-8(r1)
    bl IPInitRoute
    li r3,1
    lwz r0,12(r1)
    addi r1,r1,8
    mtlr r0
    blr
}

static asm s32 Started(u8* ppp)
{
    nofralloc
    li r3,1
    blr
}

static asm s32 Finished(u8* ppp)
{
    nofralloc
    li r3,1
    blr
}

asm void PPPInitIPCP(u8* ppp, u8* ip)
{
    nofralloc
    mflr r0
    li r5,160
    stw r0,4(r1)
    stwu r1,-24(r1)
    stw r31,20(r1)
    addi r31,r3,0
    stw r30,16(r1)
    addi r30,r4,0
    li r4,0
    bl memset
    lis r3,1
    addi r0,r3,-32735
    sth r0,0(r31)
    addi r3,r31,56
    bl OSCreateAlarm
    lis r3, ReceiveConfigureRequest@h
    stw r30,96(r31)
    addi r0,r3,ReceiveConfigureRequest@l
    lis r3, ReceiveConfigureAck@h
    stw r0,100(r31)
    addi r0,r3,ReceiveConfigureAck@l
    lis r3, ReceiveConfigureNak@h
    stw r0,104(r31)
    addi r0,r3,ReceiveConfigureNak@l
    lis r3, ReceiveConfigureReject@h
    stw r0,108(r31)
    addi r0,r3,ReceiveConfigureReject@l
    lis r3, Up@h
    stw r0,112(r31)
    addi r0,r3,Up@l
    lis r3, Down@h
    stw r0,116(r31)
    addi r0,r3,Down@l
    lis r3, Started@h
    stw r0,120(r31)
    addi r0,r3,Started@l
    lis r3, Finished@h
    stw r0,124(r31)
    addi r0,r3,Finished@l
    stw r0,128(r31)
    li r0,3
    li r6,6
    lhz r4,48(r31)
    li r5,0
    li r3,129
    addi r4,r4,28
    add r4,r31,r4
    stb r0,0(r4)
    li r0,131
    stb r6,1(r4)
    stw r5,2(r4)
    lhz r4,48(r31)
    addi r4,r4,6
    sth r4,48(r31)
    lhz r4,48(r31)
    addi r4,r4,28
    add r4,r31,r4
    stb r3,0(r4)
    stb r6,1(r4)
    stw r5,2(r4)
    lhz r3,48(r31)
    addi r3,r3,6
    sth r3,48(r31)
    lhz r3,48(r31)
    addi r3,r3,28
    add r3,r31,r3
    stb r0,0(r3)
    stb r6,1(r3)
    stw r5,2(r3)
    lhz r3,48(r31)
    addi r0,r3,6
    sth r0,48(r31)
    lwz r0,28(r1)
    lwz r31,20(r1)
    lwz r30,16(r1)
    addi r1,r1,24
    mtlr r0
    blr
}
