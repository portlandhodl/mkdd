#include "JSystem/J3D/J3DGD.h"
#include <dolphin/os.h>

static u8 J3DGDTexImage0Ids[8] = { 0x88, 0x89, 0x8A, 0x8B, 0xA8, 0xA9, 0xAA, 0xAB };
static u8 J3DGDTexImage3Ids[8] = { 0x94, 0x95, 0x96, 0x97, 0xB4, 0xB5, 0xB6, 0xB7 };
static u8 J3DGDTexTlutIds[8] = { 0x98, 0x99, 0x9A, 0x9B, 0xB8, 0xB9, 0xBA, 0xBB };
static u8 J3DTexImage1Ids[8] = { 0x8C, 0x8D, 0x8E, 0x8F, 0xAC, 0xAD, 0xAE, 0xAF };
static u8 J3DTexImage2Ids[8] = { 0x90, 0x91, 0x92, 0x93, 0xB0, 0xB1, 0xB2, 0xB3 };

void J3DGDSetTexImgAttr(GXTexMapID id, u16 w, u16 h, GXTexFmt fmt)
{
    J3DGDWriteBPCmd((w - 1) | ((h - 1) << 10) | (fmt << 20) | (J3DGDTexImage0Ids[id] << 24));
}

void J3DGDSetTexImgPtr(GXTexMapID id, void* tex)
{
    J3DGDWriteBPCmd((OSCachedToPhysical(tex) >> 5) | (J3DGDTexImage3Ids[id] << 24));
}

void J3DGDSetTexImgPtrRaw(GXTexMapID id, u32 addr)
{
    GDOverflowCheck(5);
    J3DGDWriteBPCmd(addr | (J3DGDTexImage3Ids[id] << 24));
}

void J3DGDSetTexTlut(GXTexMapID id, u32 addr, GXTlutFmt fmt)
{
    J3DGDWriteBPCmd(((addr - 0x80000) >> 9) | fmt << 10 | J3DGDTexTlutIds[id] << 24);
}

void J3DFifoLoadPosMtxImm(Mtx mtx, u32 idx)
{
    GX_WRITE_U8(GX_FIFO_CMD_LOAD_XF_REG);
    GX_WRITE_U16(11);
    GX_WRITE_U16((u16)idx * 4);
    GX_WRITE_U32(*(u32*)&mtx[0][0]);
    GX_WRITE_U32(*(u32*)&mtx[0][1]);
    GX_WRITE_U32(*(u32*)&mtx[0][2]);
    GX_WRITE_U32(*(u32*)&mtx[0][3]);
    GX_WRITE_U32(*(u32*)&mtx[1][0]);
    GX_WRITE_U32(*(u32*)&mtx[1][1]);
    GX_WRITE_U32(*(u32*)&mtx[1][2]);
    GX_WRITE_U32(*(u32*)&mtx[1][3]);
    GX_WRITE_U32(*(u32*)&mtx[2][0]);
    GX_WRITE_U32(*(u32*)&mtx[2][1]);
    GX_WRITE_U32(*(u32*)&mtx[2][2]);
    GX_WRITE_U32(*(u32*)&mtx[2][3]);
}

void J3DFifoLoadNrmMtxImm(Mtx mtx, u32 idx)
{
    GX_WRITE_U8(GX_FIFO_CMD_LOAD_XF_REG);
    GX_WRITE_U16(8);
    GX_WRITE_U16(idx * 3 + 0x400);
    GX_WRITE_U32(*(u32*)&mtx[0][0]);
    GX_WRITE_U32(*(u32*)&mtx[0][1]);
    GX_WRITE_U32(*(u32*)&mtx[0][2]);
    GX_WRITE_U32(*(u32*)&mtx[1][0]);
    GX_WRITE_U32(*(u32*)&mtx[1][1]);
    GX_WRITE_U32(*(u32*)&mtx[1][2]);
    GX_WRITE_U32(*(u32*)&mtx[2][0]);
    GX_WRITE_U32(*(u32*)&mtx[2][1]);
    GX_WRITE_U32(*(u32*)&mtx[2][2]);
}

void J3DFifoLoadNrmMtxImm3x3(Mtx33 mtx, u32 idx)
{
    GX_WRITE_U8(GX_FIFO_CMD_LOAD_XF_REG);
    GX_WRITE_U16(8);
    GX_WRITE_U16(idx * 3 + 0x400);
    GX_WRITE_U32(*(u32*)&mtx[0][0]);
    GX_WRITE_U32(*(u32*)&mtx[0][1]);
    GX_WRITE_U32(*(u32*)&mtx[0][2]);
    GX_WRITE_U32(*(u32*)&mtx[1][0]);
    GX_WRITE_U32(*(u32*)&mtx[1][1]);
    GX_WRITE_U32(*(u32*)&mtx[1][2]);
    GX_WRITE_U32(*(u32*)&mtx[2][0]);
    GX_WRITE_U32(*(u32*)&mtx[2][1]);
    GX_WRITE_U32(*(u32*)&mtx[2][2]);
}

#undef GX_WRITE_U32
#define GX_WRITE_U32(v) ((volatile PPCWGPipe*)GXFIFO_ADDR)->u32 = (u32)(v)

void J3DFifoLoadNrmMtxToTexMtx(Mtx mtx, u32 idx)
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    GX_WRITE_U8(GX_FIFO_CMD_LOAD_XF_REG);
    GX_WRITE_U16(11);
    GX_WRITE_U16((u16)idx * 4);

    GX_WRITE_U32(*(u32*)&mtx[0][0]);
    GX_WRITE_U32(*(u32*)&mtx[0][1]);
    GX_WRITE_U32(*(u32*)&mtx[0][2]);
    GX_WRITE_U32(*(volatile u32*)&x);

    GX_WRITE_U32(*(u32*)&mtx[1][0]);
    GX_WRITE_U32(*(u32*)&mtx[1][1]);
    GX_WRITE_U32(*(u32*)&mtx[1][2]);
    GX_WRITE_U32(*(volatile u32*)&y);

    GX_WRITE_U32(*(u32*)&mtx[2][0]);
    GX_WRITE_U32(*(u32*)&mtx[2][1]);
    GX_WRITE_U32(*(u32*)&mtx[2][2]);
    GX_WRITE_U32(*(volatile u32*)&z);
}

void J3DFifoLoadNrmMtxToTexMtx3x3(Mtx33 mtx, u32 idx)
{
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    GX_WRITE_U8(GX_FIFO_CMD_LOAD_XF_REG);
    GX_WRITE_U16(11);
    GX_WRITE_U16((u16)idx * 4);

    GX_WRITE_U32(*(u32*)&mtx[0][0]);
    GX_WRITE_U32(*(u32*)&mtx[0][1]);
    GX_WRITE_U32(*(u32*)&mtx[0][2]);
    GX_WRITE_U32(*(volatile u32*)&x);

    GX_WRITE_U32(*(u32*)&mtx[1][0]);
    GX_WRITE_U32(*(u32*)&mtx[1][1]);
    GX_WRITE_U32(*(u32*)&mtx[1][2]);
    GX_WRITE_U32(*(volatile u32*)&y);

    GX_WRITE_U32(*(u32*)&mtx[2][0]);
    GX_WRITE_U32(*(u32*)&mtx[2][1]);
    GX_WRITE_U32(*(u32*)&mtx[2][2]);
    GX_WRITE_U32(*(volatile u32*)&z);
}

#undef GX_WRITE_U32
#define GX_WRITE_U32(v) GXWGFifo.u32 = (u32)v

void J3DFifoLoadTexCached(GXTexMapID id, u32 tmemEven, GXTexCacheSize sizeEven, u32 tmemOdd, GXTexCacheSize sizeOdd)
{
    GX_WRITE_U8(GX_FIFO_CMD_LOAD_BP_REG);
    u32 sizeEven3 = sizeEven + 3;
    GX_WRITE_U32(J3DTexImage1Ids[id] << 24 | (sizeEven3 << 18 | (tmemEven >> 5 | sizeEven3 << 15)));

    if (sizeOdd != GX_TEXCACHE_NONE && tmemOdd < 0x100000) {
        GX_WRITE_U8(GX_FIFO_CMD_LOAD_BP_REG);
        u32 sizeOdd3 = sizeOdd + 3;
        GX_WRITE_U32(J3DTexImage2Ids[id] << 24 | (sizeOdd3 << 18 | (tmemOdd >> 5 | sizeOdd3 << 15)));
    }
}
