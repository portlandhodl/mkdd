#include <dolphin/gx.h>
#include <dolphin/gx/GXPriv.h>

void __GXSetDirtyState(void)
{
  u32 dirtyFlags = gx->dirtyState;

  if (dirtyFlags & 1) {
    __GXSetSUTexRegs();
  }

  if (dirtyFlags & 2) {
    __GXUpdateBPMask();
  }

  if (dirtyFlags & 4) {
    __GXSetGenMode();
  }

  if (dirtyFlags & 8) {
    __GXSetVCD();
  }

  if (dirtyFlags & 0x10) {
    __GXSetVAT();
  }

  if (dirtyFlags & 0x18) {
    __GXCalculateVLim();
  }

  gx->dirtyState = 0;
}

void GXBegin(GXPrimitive type, GXVtxFmt fmt, u16 vert_num)
{
  if (gx->dirtyState) {
    __GXSetDirtyState();
  }

  if (GX_CHECK_FLUSH()) {
    __GXSendFlushPrim();
  }

  GX_WRITE_U8(fmt | type);
  GX_WRITE_U16(vert_num);
}

void __GXSendFlushPrim(void)
{
  u32 i;
  u16 vNum = gx->vNum;
  u32 sz = (vNum * gx->vLim);

  DECOMP_HOST((*(volatile u8*)GXFIFO_ADDR) = 0x98, port_gx_w8(0x98));
  DECOMP_HOST((*(volatile u16*)GXFIFO_ADDR) = vNum, port_gx_w16(vNum));

  for (i = 0; i < sz; i += 4) {
    DECOMP_HOST((*(volatile u32*)GXFIFO_ADDR) = 0, port_gx_w32(0));
  }

  gx->bpSentNot = GX_TRUE;
}

void GXSetLineWidth(u8 width, GXTexOffset offsets)
{
  GX_SET_REG(gx->lpSize, width, 24, 31);
  GX_SET_REG(gx->lpSize, offsets, 13, 15);

  GX_BP_LOAD_REG(gx->lpSize);

  gx->bpSentNot = GX_FALSE;
}

void GXGetLineWidth(u8* width, GXTexOffset* offsets)
{
  *width = GX_BITGET(gx->lpSize, 24, 8);
  *offsets = GX_BITGET(gx->lpSize, 13, 3);
}

void GXSetPointSize(u8 size, GXTexOffset offsets)
{
  GX_SET_REG(gx->lpSize, size, 16, 23);
  GX_SET_REG(gx->lpSize, offsets, 10, 12);

  GX_BP_LOAD_REG(gx->lpSize);

  gx->bpSentNot = GX_FALSE;
}

void GXGetPointSize(u8* size, GXTexOffset* offsets)
{
  *size = GX_BITGET(gx->lpSize, 16, 8);
  *offsets = GX_BITGET(gx->lpSize, 10, 3);
}

void GXEnableTexOffsets(GXTexCoordID coord, GXBool line, GXBool point)
{
  GX_SET_REG(gx->suTs0[coord], line, 13, 13);
  GX_SET_REG(gx->suTs0[coord], point, 12, 12);

  GX_BP_LOAD_REG(gx->suTs0[coord]);

  gx->bpSentNot = GX_FALSE;
}

void GXSetCullMode(GXCullMode mode)
{
  switch (mode) {
  case GX_CULL_FRONT:
    mode = GX_CULL_BACK;
    break;
  case GX_CULL_BACK:
    mode = GX_CULL_FRONT;
    break;
  }

  GX_SET_REG(gx->genMode, mode, 16, 17);
  gx->dirtyState |= 4;
}

void GXGetCullMode(GXCullMode* mode)
{
  *mode = GX_BITGET(gx->genMode, 16, 2);

  switch (*mode) {
  case GX_CULL_FRONT:
    *mode = GX_CULL_BACK;
    break;
  case GX_CULL_BACK:
    *mode = GX_CULL_FRONT;
    break;
  }
}

void GXSetCoPlanar(GXBool enable)
{
  GX_SET_REG(gx->genMode, enable, 12, 12);
  GX_BP_LOAD_REG(0xFE080000);
  GX_BP_LOAD_REG(gx->genMode);
}

void __GXSetGenMode(void)
{
  GX_BP_LOAD_REG(gx->genMode);
  gx->bpSentNot = GX_FALSE;
}
