#include <dolphin/types.h>
#include <string.h>

typedef struct IFRingBlock {
    u32 offset;
    u32 size;
} IFRingBlock;

static u32 MargeBlock(IFRingBlock* blocks, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5);

asm u32 IFRingIn(u8* base, u32 offset, u32 len, u32 unused, u8* src, u32 size)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-56(r1)
    stmw r27,36(r1)
    addi r28,r5,0
    addi r27,r3,0
    add r0,r28,r6
    add r5,r27,r4
    cmplw r5,r0
    addi r29,r7,0
    addi r30,r8,0
    mr r3,r0
    bgt L_in_after
    subf r3,r4,r3
L_in_after:
    cmplw r28,r3
    bgt L_in_single
    subf r0,r3,r5
    cmpw r30,r0
    mr r31,r0
    bgt L_in_split
    addi r4,r29,0
    addi r5,r30,0
    bl memmove
    mr r3,r28
    b L_in_end
L_in_split:
    addi r4,r29,0
    addi r5,r31,0
    bl memmove
    add r29,r29,r31
    subf r30,r31,r30
    addi r3,r27,0
    addi r4,r29,0
    addi r5,r30,0
    bl memmove
    mr r3,r28
    b L_in_end
L_in_single:
    addi r4,r29,0
    addi r5,r30,0
    bl memmove
    mr r3,r28
L_in_end:
    lmw r27,36(r1)
    lwz r0,60(r1)
    addi r1,r1,56
    mtlr r0
    blr
}
asm u32 IFRingOut(u8* base, u32 offset, u32 pos, u32 unused, u8* dst, u32 len)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-56(r1)
    stmw r27,36(r1)
    addi r27,r3,0
    addi r28,r5,0
    addi r30,r8,0
    add r3,r27,r4
    add r0,r28,r30
    cmplw r0,r3
    addi r29,r7,0
    bge L_out_1
    addi r3,r29,0
    addi r4,r28,0
    addi r5,r30,0
    bl memmove
    add r28,r28,r30
    b L_out_2
L_out_1:
    subf r31,r28,r3
    addi r3,r29,0
    addi r4,r28,0
    addi r5,r31,0
    bl memmove
    addi r28,r27,0
    add r29,r29,r31
    subf r30,r31,r30
    addi r3,r29,0
    addi r4,r28,0
    addi r5,r30,0
    bl memmove
    add r28,r28,r30
L_out_2:
    mr r3,r28
    lmw r27,36(r1)
    lwz r0,60(r1)
    addi r1,r1,56
    mtlr r0
    blr
}

asm u32 IFRingGet(u8* base, u32 offset, u32 limit, u32 unused, u32* info, u32 len)
{
    nofralloc
    add r6,r3,r4
    cmplw r6,r5
    bgt L_get_1
    subf r5,r4,r5
L_get_1:
    add r0,r5,r8
    cmplw r0,r6
    bgt L_get_2
    stw r5,0(r7)
    li r3,1
    stw r8,4(r7)
    blr
L_get_2:
    stw r5,0(r7)
    subf r4,r5,r6
    subf r0,r4,r8
    stw r4,4(r7)
    stw r3,8(r7)
    li r3,2
    stw r0,12(r7)
    blr
}

u32 IFRingPut(u8* base, u32 offset, u32 len, u32 unused, u32 size)
{
    __asm {
        add r4,r3,r4
        add r0,r5,r7
        cmplw r0,r4
        bge L_put_else
        mr r3,r0
        blr
    L_put_else:
        subf r4,r5,r4
        add r0,r3,r7
        subf r3,r4,r0
    }
}

static asm u32 MargeBlock(IFRingBlock* blocks, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-80(r1)
    stmw r23,44(r1)
    addi r26,r3,0
    addi r29,r8,0
    cmplw r29,r26
    addi r27,r4,0
    addi r23,r5,0
    addi r28,r7,0
    bgt L_marge_1
    subf r25,r29,r26
    b L_marge_2
L_marge_1:
    add r0,r26,r28
    subf r25,r29,r0
L_marge_2:
    cmplw r29,r26
    slwi r0,r6,3
    addi r24,r25,0
    add r30,r25,r27
    add r31,r23,r0
    bne L_marge_11
    mr r26,r23
    b L_marge_9
L_marge_3:
    cmplw r29,r0
    bgt L_marge_4
    subf r3,r29,r0
    b L_marge_5
L_marge_4:
    add r0,r0,r28
    subf r3,r29,r0
L_marge_5:
    cmpw r3,r30
    bgt L_marge_8
    lwz r0,4(r26)
    add r0,r3,r0
    cmpw r0,r30
    ble L_marge_6
    b L_marge_7
L_marge_6:
    mr r0,r30
L_marge_7:
    addi r4,r26,8
    mr r30,r0
    addi r3,r26,0
    subf r27,r25,r0
    subf r5,r4,r31
    bl memmove
    addi r3,r31,-8
    li r4,0
    li r5,8
    bl memset
    b L_marge_9
L_marge_8:
    addi r26,r26,8
L_marge_9:
    cmplw r26,r31
    bge L_marge_10
    lwz r0,0(r26)
    cmplwi r0,0
    bne L_marge_3
L_marge_10:
    mr r3,r27
    b L_marge_23
L_marge_11:
    mr r25,r23
    b L_marge_19
L_marge_12:
    cmplw r29,r4
    bgt L_marge_13
    subf r3,r29,r4
    b L_marge_14
L_marge_13:
    add r0,r4,r28
    subf r3,r29,r0
L_marge_14:
    lwz r0,4(r25)
    add r0,r3,r0
    cmpw r24,r0
    bgt L_marge_18
    cmpw r3,r30
    bgt L_marge_18
    cmpw r0,r30
    ble L_marge_15
    b L_marge_16
L_marge_15:
    mr r0,r30
L_marge_16:
    cmpw r3,r24
    mr r30,r0
    bge L_marge_17
    addi r24,r3,0
    addi r26,r4,0
L_marge_17:
    addi r4,r25,8
    addi r3,r25,0
    subf r27,r24,r0
    subf r5,r4,r31
    bl memmove
    addi r3,r31,-8
    li r4,0
    li r5,8
    bl memset
    b L_marge_19
L_marge_18:
    addi r25,r25,8
L_marge_19:
    cmplw r25,r31
    bge L_marge_20
    lwz r4,0(r25)
    cmplwi r4,0
    bne L_marge_12
L_marge_20:
    cmplw r25,r31
    bge L_marge_21
    stw r26,0(r25)
    stw r27,4(r25)
    b L_marge_22
L_marge_21:
    addi r4,r23,8
    addi r3,r23,0
    subf r5,r4,r31
    bl memmove
    stw r26,-8(r31)
    stw r27,-4(r31)
L_marge_22:
    li r3,0
L_marge_23:
    lmw r23,44(r1)
    lwz r0,84(r1)
    addi r1,r1,80
    mtlr r0
    blr
}

asm u32 IFRingInEx(u8* base, u32 offset, u32 len, u32 unused0, u8* src, u32 size, u32 arg7, u32 limit, u32* outOffset, u32 arg10)
{
    nofralloc
    mflr r0
    stw r0,4(r1)
    stwu r1,-88(r1)
    stmw r21,44(r1)
    mr r25,r9
    lwz r27,96(r1)
    addi r21,r3,0
    addi r22,r4,0
    addi r23,r5,0
    addi r24,r8,0
    addi r26,r10,0
    lwz r29,0(r9)
    cmpwi r29,0
    bne L_ie_1
    mr r3,r23
    b L_ie_end
L_ie_1:
    add r0,r23,r6
    add r3,r21,r22
    cmplw r3,r0
    mr r31,r0
    bgt L_ie_2
    subf r31,r22,r31
L_ie_2:
    add r0,r31,r7
    cmplw r3,r0
    mr r30,r0
    bgt L_ie_3
    subf r30,r22,r30
L_ie_3:
    cmplw r23,r30
    bgt L_ie_5
    subf r0,r30,r3
    cmpw r29,r0
    mr r28,r0
    bgt L_ie_4
    addi r3,r30,0
    addi r4,r24,0
    addi r5,r29,0
    bl memmove
    b L_ie_6
L_ie_4:
    addi r3,r30,0
    addi r4,r24,0
    addi r5,r28,0
    bl memmove
    addi r3,r21,0
    add r4,r24,r28
    subf r5,r28,r29
    bl memmove
    b L_ie_6
L_ie_5:
    addi r3,r30,0
    addi r4,r24,0
    addi r5,r29,0
    bl memmove
L_ie_6:
    addi r3,r30,0
    addi r4,r29,0
    addi r5,r26,0
    addi r6,r27,0
    addi r7,r22,0
    addi r8,r31,0
    bl MargeBlock
    stw r3,0(r25)
    mr r3,r23
L_ie_end:
    lmw r21,44(r1)
    lwz r0,92(r1)
    addi r1,r1,88
    mtlr r0
    blr
}

