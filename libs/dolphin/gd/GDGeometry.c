#include "dolphin/gd.h"
#include "dolphin/os.h"

void GDSetVtxDescv(GXVtxDescList* lists)
{
    u32 v1 = 0;
    u32 v2 = 0;
    u32 v3 = 0;
    u32 v4 = 0;
    u32 v5 = 0;
    u32 v6 = 1;
    u32 v7 = 0;
    u32 v8 = 0;
    u32 v9 = 0;
    u32 v10 = 0;
    u32 v11 = 0;
    u32 v12 = 0;
    u32 v13 = 0;
    u32 v14 = 0;
    u32 v15 = 0;
    u32 v16 = 0;
    u32 v17 = 0;

    while (lists->attr != GX_VA_NULL) {
        switch (lists->attr) {
        case GX_VA_PNMTXIDX:
            v4 = lists->type;
            break;
        case GX_VA_TEX0MTXIDX:
            v5 = v5 & ~0x1 | lists->type;
            break;
        case GX_VA_TEX1MTXIDX:
            v5 = v5 & ~0x2 | lists->type << 1;
            break;
        case GX_VA_TEX2MTXIDX:
            v5 = v5 & ~0x4 | lists->type << 2;
            break;
        case GX_VA_TEX3MTXIDX:
            v5 = v5 & ~0x8 | lists->type << 3;
            break;
        case GX_VA_TEX4MTXIDX:
            v5 = v5 & ~0x10 | lists->type << 4;
            break;
        case GX_VA_TEX5MTXIDX:
            v5 = v5 & ~0x20 | lists->type << 5;
            break;
        case GX_VA_TEX6MTXIDX:
            v5 = v5 & ~0x40 | lists->type << 6;
            break;
        case GX_VA_TEX7MTXIDX:
            v5 = v5 & ~0x80 | lists->type << 7;
            break;
        case GX_VA_POS:
            v6 = lists->type;
            break;
        case GX_VA_NRM:
            if (lists->type != GX_NONE) {
                v1 = 1;
                v7 = lists->type;
            }
            break;
        case GX_VA_NBT:
            if (lists->type != GX_NONE) {
                v1 = 2;
                v7 = lists->type;
            }
            break;
        case GX_VA_CLR0:
            v8 = lists->type;
            v2 += (v8 != 0);
            break;
        case GX_VA_CLR1:
            v9 = lists->type;
            v2 += (v9 != 0);
            break;
        case GX_VA_TEX0:
            v10 = lists->type;
            v3 += (v10 != 0);
            break;
        case GX_VA_TEX1:
            v11 = lists->type;
            v3 += (v11 != 0);
            break;
        case GX_VA_TEX2:
            v12 = lists->type;
            v3 += (v12 != 0);
            break;
        case GX_VA_TEX3:
            v13 = lists->type;
            v3 += (v13 != 0);
            break;
        case GX_VA_TEX4:
            v14 = lists->type;
            v3 += (v14 != 0);
            break;
        case GX_VA_TEX5:
            v15 = lists->type;
            v3 += (v15 != 0);
            break;
        case GX_VA_TEX6:
            v16 = lists->type;
            v3 += (v16 != 0);
            break;
        case GX_VA_TEX7:
            v17 = lists->type;
            v3 += (v17 != 0);
            break;
        case GX_POS_MTX_ARRAY:
        case GX_NRM_MTX_ARRAY:
        case GX_TEX_MTX_ARRAY:
        case GX_LIGHT_ARRAY:
            break;
        }
        lists++;
    }

    __GDCheckOverflowed(1);
    __GDWriteU8(8);
    __GDCheckOverflowed(1);
    __GDWriteU8(80);
    __GDCheckOverflowed(4);
    __GDWriteU32(v4 | v5 << 1 | v6 << 9 | v7 << 11 | v8 << 13 | v9 << 15);

    __GDCheckOverflowed(1);
    __GDWriteU8(8);
    __GDCheckOverflowed(1);
    __GDWriteU8(96);
    __GDCheckOverflowed(4);
    __GDWriteU32(v10 | v11 << 2 | v12 << 4 | v13 << 6 | v14 << 8 | v15 << 10 | v16 << 12 | v17 << 14);

    __GDCheckOverflowed(1);
    __GDWriteU8(16);
    __GDCheckOverflowed(2);
    __GDWriteU16(0);
    __GDCheckOverflowed(2);
    __GDWriteU16(4104);
    __GDCheckOverflowed(4);
    __GDWriteU32(v2 | v1 << 2 | v3 << 4);
}

void GDSetArray(GXAttr attr, const void* data, u8 stride)
{
    u32 v1;
    if (attr == GX_VA_NBT) {
        v1 = 1;
    } else {
        v1 = attr - 9;
    }

    __GDCheckOverflowed(1);
    __GDWriteU8(8);
    __GDCheckOverflowed(1);
    __GDWriteU8(v1 + 160);
    __GDCheckOverflowed(4);
    __GDWriteU32(OS_BASE_CACHED + (u32)data);

    __GDCheckOverflowed(1);
    __GDWriteU8(8);
    __GDCheckOverflowed(1);
    __GDWriteU8(v1 + 176);
    __GDCheckOverflowed(4);
    __GDWriteU32(stride);
}

void GDSetArrayRaw(GXAttr attr, u32 data, u8 stride)
{
    u32 v1;
    if (attr == GX_VA_NBT) {
        v1 = 1;
    } else {
        v1 = attr - 9;
    }

    __GDCheckOverflowed(1);
    __GDWriteU8(8);
    __GDCheckOverflowed(1);
    __GDWriteU8(v1 + 160);
    __GDCheckOverflowed(4);
    __GDWriteU32(data);

    __GDCheckOverflowed(1);
    __GDWriteU8(8);
    __GDCheckOverflowed(1);
    __GDWriteU8(v1 + 176);
    __GDCheckOverflowed(4);
    __GDWriteU32(stride);
}
