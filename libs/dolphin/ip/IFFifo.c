#include <dolphin/types.h>

typedef struct IFFifo {
    u8* buffer;
    s32 size;
    u8* head;
    s32 used;
} IFFifo;

static asm u8* FifoAlloc(IFFifo* fifo, s32 size);

void IFFifoInit(IFFifo* fifo, void* buffer, s32 size)
{
    fifo->size = size;
    fifo->buffer = buffer;
    fifo->head = buffer;
    fifo->used = 0;
}

static asm u8* FifoAlloc(IFFifo* fifo, s32 size)
{
    nofralloc
    mr r9,r3
    lwz r5,4(r3)
    lwz r3,12(r3)
    lwz r7,0(r9)
    subf r0,r3,r5
    cmpw r0,r4
    add r8,r7,r5
    bge L_alloc_space
    li r3,0
    blr
L_alloc_space:
    lwz r6,8(r9)
    add r0,r6,r3
    cmplw r8,r0
    mr r3,r0
    bgt L_alloc_tail_ready
    subf r3,r5,r3
L_alloc_tail_ready:
    cmplw r6,r3
    bne L_alloc_nonempty
    stw r7,8(r9)
    stw r4,12(r9)
    lwz r3,0(r9)
    blr
L_alloc_nonempty:
    bge L_alloc_append
    subf r5,r3,r8
    cmpw r4,r5
    bgt L_alloc_wrap
    lwz r0,12(r9)
    add r0,r0,r4
    stw r0,12(r9)
    blr
L_alloc_wrap:
    subf r0,r7,r6
    cmpw r4,r0
    bgt L_alloc_fail
    lwz r0,12(r9)
    add r0,r4,r0
    add r0,r5,r0
    stw r0,12(r9)
    lwz r3,0(r9)
    blr
L_alloc_append:
    lwz r0,12(r9)
    add r0,r0,r4
    stw r0,12(r9)
    blr
L_alloc_fail:
    li r3,0
    blr
}

u8* IFFifoAlloc(IFFifo* fifo, s32 size)
{
    return FifoAlloc(fifo, size);
}

asm BOOL IFFifoFree(IFFifo* fifo, u8* ptr, s32 size)
{
    nofralloc
    cmpwi r5,0
    lwz r0,0(r3)
    lwz r7,4(r3)
    add r9,r0,r7
    ble L_free_invalid
    lwz r6,12(r3)
    cmpw r6,r5
    blt L_free_invalid
    cmplwi r4,0
    beq L_free_invalid
    cmplw r4,r0
    blt L_free_invalid
    cmplw r9,r4
    bgt L_free_range
L_free_invalid:
    li r3,0
    blr
L_free_range:
    lwz r8,8(r3)
    add r6,r8,r6
    cmplw r9,r6
    addi r10,r6,0
    bgt L_free_tail_ready
    subf r10,r7,r10
L_free_tail_ready:
    add r6,r4,r5
    cmplw r9,r6
    bgt L_free_next_ready
    subf r6,r7,r6
L_free_next_ready:
    cmplw r6,r0
    blt L_free_next_invalid
    cmplw r9,r6
    bgt L_free_cases
L_free_next_invalid:
    li r3,0
    blr
L_free_cases:
    cmplw r8,r10
    bne L_free_not_full
    cmplw r8,r6
    bge L_free_full_lower
    cmplw r6,r4
    ble L_free_full_invalid
    cmplw r4,r8
    bge L_free_common
L_free_full_invalid:
    li r3,0
    blr
L_free_full_lower:
    cmplw r6,r4
    bgt L_free_full_check
    cmplw r4,r8
    bge L_free_full_check
    li r3,0
    blr
L_free_full_check:
    cmplw r6,r10
    bne L_free_common
    lwz r0,12(r3)
    subf r0,r5,r0
    stw r0,12(r3)
    li r3,1
    blr
L_free_not_full:
    bge L_free_head_greater
    cmplw r6,r4
    ble L_free_contiguous_invalid
    cmplw r4,r8
    blt L_free_contiguous_invalid
    cmplw r10,r6
    bge L_free_contiguous_check
L_free_contiguous_invalid:
    li r3,0
    blr
L_free_contiguous_check:
    cmplw r6,r10
    bne L_free_common
    lwz r0,12(r3)
    subf r0,r5,r0
    stw r0,12(r3)
    li r3,1
    blr
L_free_head_greater:
    cmplw r8,r6
    bge L_free_wrapped_lower
    cmplw r6,r4
    ble L_free_wrapped_upper_invalid
    cmplw r4,r8
    bge L_free_common
L_free_wrapped_upper_invalid:
    li r3,0
    blr
L_free_wrapped_lower:
    cmplw r10,r6
    bge L_free_wrapped_in_range
    li r3,0
    blr
L_free_wrapped_in_range:
    cmplw r6,r4
    bgt L_free_wrapped_check
    cmplw r4,r8
    bge L_free_wrapped_check
    li r3,0
    blr
L_free_wrapped_check:
    cmplw r6,r10
    bne L_free_common
    lwz r0,12(r3)
    subf r0,r5,r0
    stw r0,12(r3)
    li r3,1
    blr
L_free_common:
    cmplw r6,r10
    bgt L_free_common_wrapped
    subf r0,r6,r10
    stw r0,12(r3)
    b L_free_finish
L_free_common_wrapped:
    subf r4,r6,r9
    subf r0,r0,r10
    add r0,r4,r0
    stw r0,12(r3)
L_free_finish:
    stw r6,8(r3)
    li r3,1
    blr
}
