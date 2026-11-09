#include "JSystem/J3D/J3DSys.h"

J3DSys j3dSys;

void J3DSys::loadPosMtxIndx(int addr, u16 indx) const
{
    GXWGFifo.u8 = 0x20;
    GXWGFifo.u16 = indx;
    GXWGFifo.u16 = (u16)(addr * 0xC) | 0xB000;
}

void J3DSys::loadNrmMtxIndx(int addr, u16 indx) const
{
    GXWGFifo.u8 = 0x28;
    GXWGFifo.u16 = indx;
    GXWGFifo.u16 = (u16)(addr * 9 + 0x400) | 0x8000;
}

void J3DSys::reinitGX()
{
    reinitGenMode();
    reinitLighting();
    reinitTransform();
    reinitTexture();
    reinitTevStages();
    reinitIndStages();
    reinitPixelProc();
}

void J3DSys::reinitGenMode()
{
    GXSetNumChans(0);
    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    GXSetNumIndStages(0);
    GXSetCullMode(GX_CULL_BACK);
    GXSetCoPlanar(GX_FALSE);
}
