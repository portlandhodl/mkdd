#include <dolphin/gd.h>
#include <dolphin/os.h>

GDCurrentDL *__GDCurrentDL = NULL;
GDOverflowCallback overflowcb = NULL;

void GDInitGDLObj(GDCurrentDL *obj, u8 *start, s32 len) {
    obj->begin = start;
    obj->pDisplayListData = start;
    obj->end = start + len;
    obj->length = len;
}

void GDFlushCurrToMem() {
    DCFlushRange(__GDCurrentDL->begin, __GDCurrentDL->length);
}

void GDPadCurr32() {
    u32 i = (u32)__GDCurrentDL->pDisplayListData & 31;
    if (i) {
        for (; i < 32; i++) {
            __GDWrite(0);
        }
    }
}

void GDOverflowed(void) {
    if (overflowcb) {
        (*overflowcb)();
    }
}

void GDSetOverflowCallback(GDOverflowCallback cb) { overflowcb = cb; }

GDOverflowCallback GDGetOverflowCallback(void) { return overflowcb; }
