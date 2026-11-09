#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DGD.h"
#include "JSystem/J3D/J3DPacket.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DTexMtx.h"
#include "JSystem/J3D/J3DTransform.h"

void J3DShapeMtx::resetMtxLoadCache()
{
    sMtxLoadCache[9] = 0xFFFF;
    sMtxLoadCache[8] = 0xFFFF;
    sMtxLoadCache[7] = 0xFFFF;
    sMtxLoadCache[6] = 0xFFFF;
    sMtxLoadCache[5] = 0xFFFF;
    sMtxLoadCache[4] = 0xFFFF;
    sMtxLoadCache[3] = 0xFFFF;
    sMtxLoadCache[2] = 0xFFFF;
    sMtxLoadCache[1] = 0xFFFF;
    sMtxLoadCache[0] = 0xFFFF;
}

void J3DShapeMtx::loadMtxIndx_PNGP(int slot, u16 indx) const
{
    GXWGFifo.u8  = 0x20;
    GXWGFifo.u16 = indx;
    GXWGFifo.u16 = 0xB000 | (u16)(slot * 0x0C);
    GXWGFifo.u8  = 0x28;
    GXWGFifo.u16 = indx;
    GXWGFifo.u16 = 0x8000 | (u16)((slot * 0x09) + 0x400);
}

void J3DShapeMtx::loadMtxIndx_PCPU(int slot, u16 indx) const
{
    J3DFifoLoadPosMtxImm(*j3dSys.mShapePacket->mBaseMtxPtr, slot * 3);
    j3dSys.loadNrmMtxIndx(slot, indx);
}

void J3DShapeMtx::loadMtxIndx_NCPU(int slot, u16 indx) const
{
    j3dSys.loadPosMtxIndx(slot, indx);
    J3DFifoLoadNrmMtxImm(*j3dSys.mShapePacket->mBaseMtxPtr, slot * 3);
}

void J3DShapeMtx::loadMtxIndx_PNCPU(int slot, u16 indx) const
{
    J3DFifoLoadPosMtxImm(*j3dSys.mShapePacket->mBaseMtxPtr, slot * 3);
    J3DFifoLoadNrmMtxImm(*j3dSys.mShapePacket->mBaseMtxPtr, slot * 3);
}

void J3DShapeMtxConcatView::loadMtxConcatView_PNGP(int slot, u16 drawIdx) const
{
    Mtx mtx;
    PSMTXConcat(*j3dSys.mShapePacket->mBaseMtxPtr, j3dSys._104[drawIdx], mtx);
    if (J3DDifferedTexMtx::sTexGenBlock != NULL) {
        J3DDifferedTexMtx::loadExecute(mtx);
    }
    J3DFifoLoadPosMtxImm(mtx, slot * 3);
    loadNrmMtx(slot, drawIdx, mtx);
}

void J3DShapeMtxConcatView::loadMtxConcatView_NCPU(int slot, u16 drawIdx) const
{
    Mtx mtx;
    PSMTXConcat(*j3dSys.mShapePacket->mBaseMtxPtr, j3dSys._104[drawIdx], mtx);
    if (J3DDifferedTexMtx::sTexGenBlock != NULL) {
        J3DDifferedTexMtx::loadExecute(mtx);
    }
    J3DFifoLoadPosMtxImm(mtx, slot * 3);
    J3DFifoLoadNrmMtxImm(*j3dSys.mShapePacket->mBaseMtxPtr, slot * 3);
    if (sTexMtxLoadType == 0x2000) {
        J3DFifoLoadNrmMtxToTexMtx(*j3dSys.mShapePacket->mBaseMtxPtr, slot * 3 + 0x1E);
    }
}

void J3DShapeMtx::load() const
{
    LoadMtxIndxFunction function = sMtxLoadPipeline[sCurrentPipeline];
    (this->*function)(0, mUseMtxIndex);
}

void J3DShapeMtx::calcNBTScale(const Vec& scale, Mtx33* srcArray, Mtx33* dstArray)
{
    J3DPSMtx33Copy(srcArray[mUseMtxIndex], dstArray[mUseMtxIndex]);
    J3DScaleNrmMtx33(dstArray[mUseMtxIndex], scale);
}

void J3DShapeMtxMulti::load() const
{
    LoadMtxIndxFunction function = sMtxLoadPipeline[sCurrentPipeline];
    int count = mUseMtxNum;
    for (int i = 0; i < count; i++) {
        if (_0C[i] != 0xFFFF) {
            (this->*function)(i, _0C[i]);
        }
    }
}
