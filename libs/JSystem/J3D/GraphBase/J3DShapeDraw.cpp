#include "JSystem/J3D/J3DShape.h"
#include "JSystem/JKernel/JKRHeap.h"
#include <dolphin/gx.h>
#include <string.h>

u32 J3DShapeDraw::countVertex(u32 stride) {
    u32 count = 0;
    u8 *dlStart = (u8 *)mDisplayList;

    for (u8 *dl = dlStart; (dl - dlStart) < mDlSize;) {
        u8 cmd = *dl;
        dl++;
        if (cmd != GX_TRIANGLEFAN && cmd != GX_TRIANGLESTRIP)
            break;
        int vtxNum = *(u16 *)dl;
        dl += 2;
        count += vtxNum;
        dl += stride * vtxNum;
    }

    return count;
}

void J3DShapeDraw::addTexMtxIndexInDL(u32 stride, u32 attrOffs, u32 valueBase) {
    u32 newSize = ALIGN_NEXT(countVertex(stride) + mDlSize, 0x20);
    u8 *newDLStart = new (0x20) u8[newSize];
    u8 *oldDLStart = (u8 *)mDisplayList;
    u8 *oldDL = oldDLStart;
    u8 *newDL = newDLStart;

    for (; (oldDL - oldDLStart) < mDlSize;) {
        u8 cmd = *oldDL;
        oldDL++;
        *newDL++ = cmd;

        if (cmd != GX_TRIANGLEFAN && cmd != GX_TRIANGLESTRIP)
            break;

        int vtxNum = *(u16 *)oldDL;
        oldDL += 2;
        *(u16 *)newDL = vtxNum;
        newDL += 2;

        for (int i = 0; i < vtxNum; i++) {
            u8 *oldDLVtx = &oldDL[stride * i];
            u8 pnmtxidx = *oldDLVtx;
            memcpy(newDL, oldDLVtx, attrOffs);
            newDL += attrOffs;
            *newDL++ = valueBase + pnmtxidx;
            memcpy(newDL, oldDLVtx + attrOffs, stride - attrOffs);
            newDL += stride - attrOffs;
        }

        oldDL += stride * vtxNum;
    }

    u32 realSize = ALIGN_NEXT((u32)newDL - (u32)newDLStart, 0x20);
    for (; (newDL - newDLStart) < newSize; newDL++)
        *newDL = 0;

    mDlSize = realSize;
    mDisplayList = newDLStart;
    DCStoreRange(newDLStart, mDlSize);
}

J3DShapeDraw::J3DShapeDraw(const u8 *dl, u32 size) {
    mDisplayList = dl;
    mDlSize = size;
}

void J3DShapeDraw::draw() const {
    GXCallDisplayList((void *)mDisplayList, mDlSize);
}

inline J3DShapeDraw::~J3DShapeDraw() {
}
