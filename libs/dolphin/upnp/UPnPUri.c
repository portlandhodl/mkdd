#include <dolphin/types.h>
#include <string.h>
#include <stdio.h>
#include <PowerPC_EABI_Support/MSL_C/MSL_Common/ctype_api.h>

static asm char* GoUpper(char* str, int sep);

int UPnPReplaceEscaped(char* str)
{
    char* dst = str;
    char* start = str;
    int val;

    while (*str) {
        if (*str == '%' && (__ctype_map[(u8)str[1]] & __hex_digit) && (__ctype_map[(u8)str[2]] & __hex_digit)) {
            if (sscanf(str + 1, "%2x", &val) == 1) {
                str += 3;
                *dst++ = (char)val;
            }
        } else {
            *dst++ = *str++;
        }
    }
    *dst = 0;
    return dst - start;
}

asm int UPnPRemoveSemicolon(char* str)
{
    nofralloc
    addi r5,r3,0
    addi r6,r3,0
    b L_s_cond
L_s_body:
    extsb r0,r4
    cmpwi r0,59
    bne L_s_copy
    b L_s_skipcond
L_s_skipchar:
    lbz r4,0(r3)
    cmpwi r4,47
    bne L_s_skipcond
    stb r4,0(r5)
    addi r3,r3,1
    addi r5,r5,1
    b L_s_cond
L_s_skipcond:
    lbzu r0,1(r3)
    extsb. r0,r0
    bne L_s_skipchar
    b L_s_cond
L_s_copy:
    lbz r0,0(r3)
    addi r3,r3,1
    stb r0,0(r5)
    addi r5,r5,1
L_s_cond:
    lbz r4,0(r3)
    extsb. r0,r4
    bne L_s_body
    li r0,0
    stb r0,0(r5)
    subf r3,r6,r5
    blr
}

static asm char* GoUpper(char* str, int sep)
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
    bl strrchr
    cmplwi r3,0
    bne L_g_cmp
    li r3,0
    b L_g_end
L_g_cmp:
    cmplw r3,r29
    bne L_g_else
    li r0,0
    stbu r0,1(r3)
    b L_g_end
L_g_else:
    li r31,0
    stb r31,0(r3)
    addi r3,r29,0
    addi r4,r30,0
    bl strrchr
    cmplwi r3,0
    bne L_g_found
    stb r31,0(r29)
    mr r3,r29
    b L_g_end
L_g_found:
    stbu r31,1(r3)
L_g_end:
    lwz r0,36(r1)
    lwz r31,28(r1)
    lwz r30,24(r1)
    lwz r29,20(r1)
    addi r1,r1,32
    mtlr r0
    blr
}

asm int UPnPRemoveDots(char* str)
{
    nofralloc
    mflr r0
    li r6,0
    stw r0,4(r1)
    stwu r1,-24(r1)
    stw r31,20(r1)
    stw r30,16(r1)
    addi r30,r3,0
    addi r31,r30,0
    b L_d_cond
L_d_body:
    extsb r0,r4
    cmpwi r0,47
    bne L_d_notslash
L_d_slash:
    lbzu r0,1(r30)
    cmpwi r0,47
    beq L_d_slash
    cmpwi r6,0
    bne L_d_slashset
    li r0,47
    stb r0,0(r3)
    addi r3,r3,1
L_d_slashset:
    li r6,1
    b L_d_cond
L_d_notslash:
    cmpwi r6,0
    bne L_d_check
    cmplw r31,r30
    bne L_d_copy
L_d_check:
    extsb r0,r4
    cmpwi r0,46
    bne L_d_copy
    lbz r5,1(r30)
    cmpwi r5,46
    bne L_d_dot1
    lbz r4,2(r30)
    cmpwi r4,47
    beq L_d_dotdot
    extsb. r0,r4
    bne L_d_dot1
L_d_dotdot:
    li r0,0
    stb r0,0(r3)
    addi r3,r31,0
    li r4,47
    addi r30,r30,2
    bl GoUpper
    cmplwi r3,0
    bne L_d_dotdotok
    li r3,-1
    b L_d_ret
L_d_dotdotok:
    li r6,1
    b L_d_cond
L_d_dot1:
    extsb r0,r5
    cmpwi r0,47
    beq L_d_dotskip
    extsb. r0,r5
    bne L_d_copy2
L_d_dotskip:
    li r6,1
    addi r30,r30,1
    b L_d_cond
L_d_copy:
    li r6,0
L_d_copy2:
    lbz r0,0(r30)
    addi r30,r30,1
    stb r0,0(r3)
    addi r3,r3,1
L_d_cond:
    lbz r4,0(r30)
    extsb. r0,r4
    beq L_d_end
    extsb r0,r4
    cmpwi r0,63
    beq L_d_end
    cmpwi r0,35
    bne L_d_body
L_d_end:
    li r0,0
    stb r0,0(r3)
    subf r3,r31,r3
L_d_ret:
    lwz r0,28(r1)
    lwz r31,20(r1)
    lwz r30,16(r1)
    addi r1,r1,24
    mtlr r0
    blr
}
