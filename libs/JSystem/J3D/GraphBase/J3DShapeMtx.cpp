#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DGD.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DMtxBuffer.h"
#include "JSystem/J3D/J3DPacket.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DTexMtx.h"
#include "JSystem/J3D/J3DTransform.h"

J3DShapeMtx::LoadMtxIndxFunction J3DShapeMtx::sMtxLoadPipeline[4] = {
    &J3DShapeMtx::loadMtxIndx_PNGP,
    &J3DShapeMtx::loadMtxIndx_PCPU,
    &J3DShapeMtx::loadMtxIndx_NCPU,
    &J3DShapeMtx::loadMtxIndx_PNCPU,
};

J3DShapeMtxConcatView::LoadMtxConcatViewFunction J3DShapeMtxConcatView::sMtxLoadPipeline[4] = {
    &J3DShapeMtxConcatView::loadMtxConcatView_PNGP,
    &J3DShapeMtxConcatView::loadMtxConcatView_PCPU,
    &J3DShapeMtxConcatView::loadMtxConcatView_NCPU,
    &J3DShapeMtxConcatView::loadMtxConcatView_PNCPU,
};

J3DShapeMtxConcatView::LoadMtxConcatViewFunction J3DShapeMtxConcatView::sMtxLoadLODPipeline[4] = {
    &J3DShapeMtxConcatView::loadMtxConcatView_PNGP_LOD,
    &J3DShapeMtxConcatView::loadMtxConcatView_PCPU,
    &J3DShapeMtxConcatView::loadMtxConcatView_NCPU,
    &J3DShapeMtxConcatView::loadMtxConcatView_PNCPU,
};

u16 J3DShapeMtx::sMtxLoadCache[10];
u32 J3DShapeMtx::sCurrentPipeline;
u8* J3DShapeMtx::sCurrentScaleFlag;
bool J3DShapeMtx::sNBTFlag;
bool J3DShapeMtx::sLODFlag;
u32 J3DShapeMtx::sTexMtxLoadType;
Mtx* J3DShapeMtxConcatView::sMtxPtrTbl[2];
J3DTexGenBlock* J3DDifferedTexMtx::sTexGenBlock;
J3DTexMtxObj* J3DDifferedTexMtx::sTexMtxObj;

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

void J3DDifferedTexMtx::loadExecute(const Mtx srcMtx)
{
    static Mtx qMtx  = { { 0.5f, 0.0f, 0.5f, 0.0f }, { 0.0f, -0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f } };
    static Mtx qMtx2 = { { 0.5f, 0.0f, 0.0f, 0.5f }, { 0.0f, -0.5f, 0.0f, 0.5f }, { 0.0f, 0.0f, 1.0f, 0.0f } };

    Mtx tmpMtx;
    Mtx44 invMtx;
    Mtx44 texMtx;
    J3DTexGenBlock* block      = sTexGenBlock;
    J3DTexMtxObj* mtxObj        = sTexMtxObj;
    u16 texMtxNum              = mtxObj->mTexMtxNum;

    if (j3dSys.checkFlag(0x40000000)) {
        for (u16 i = 0; i < texMtxNum; i++) {
            int type = block->getTexCoord(i)->getTexGenType();
            if (type == 1 || type == 0) {
                J3DTexMtx* tex = block->getTexMtx(i);
                u8 info        = tex->getTexMtxInfo().mInfo;
                Mtx* mtx;
                u32 maya;
                switch (info & 0x3F) {
                case 3:
                case 9:
                    mtx = &mtxObj->getMtx(i);
                    break;
                case 1:
                case 6:
                case 7:
                    mtx = &mtxObj->getMtx(i);
                    break;
                case 2:
                case 8:
                    PSMTXInverse(j3dSys.mViewMtx, tmpMtx);
                    PSMTXConcat(mtxObj->getMtx(i), tmpMtx, tmpMtx);
                    mtx = &tmpMtx;
                    break;
                case 5:
                    maya = (info & 0x80) >> 7;
                    if (maya == 0) {
                        J3DGetTextureMtxOld(tex->getTexMtxInfo().mSRT, tex->getTexMtxInfo().mCenter, texMtx);
                    } else if (maya == 1) {
                        J3DGetTextureMtxMayaOld(tex->getTexMtxInfo().mSRT, texMtx);
                    }
                    J3DMtxProjConcat(texMtx, mtxObj->getEffectMtx(i), tmpMtx);
                    PSMTXInverse(j3dSys.mViewMtx, invMtx);
                    PSMTXConcat(tmpMtx, invMtx, tmpMtx);
                    tmpMtx[2][3] = 0.0f;
                    tmpMtx[1][3] = 0.0f;
                    tmpMtx[0][3] = 0.0f;
                    mtx          = &tmpMtx;
                    break;
                case 11:
                    maya = (info & 0x80) >> 7;
                    if (maya == 0) {
                        J3DGetTextureMtx(tex->getTexMtxInfo().mSRT, tex->getTexMtxInfo().mCenter, texMtx);
                    } else if (maya == 1) {
                        J3DGetTextureMtxMaya(tex->getTexMtxInfo().mSRT, texMtx);
                    }
                    PSMTXConcat(texMtx, qMtx, texMtx);
                    J3DMtxProjConcat(texMtx, mtxObj->getEffectMtx(i), tmpMtx);
                    PSMTXInverse(j3dSys.mViewMtx, invMtx);
                    PSMTXConcat(tmpMtx, invMtx, tmpMtx);
                    tmpMtx[2][3] = 0.0f;
                    tmpMtx[1][3] = 0.0f;
                    tmpMtx[0][3] = 0.0f;
                    mtx          = &tmpMtx;
                    break;
                case 10:
                    maya = (info & 0x80) >> 7;
                    if (maya == 0) {
                        J3DGetTextureMtx(tex->getTexMtxInfo().mSRT, tex->getTexMtxInfo().mCenter, texMtx);
                    } else if (maya == 1) {
                        J3DGetTextureMtxMaya(tex->getTexMtxInfo().mSRT, texMtx);
                    }
                    PSMTXConcat(texMtx, qMtx2, texMtx);
                    J3DMtxProjConcat(texMtx, mtxObj->getEffectMtx(i), tmpMtx);
                    PSMTXInverse(j3dSys.mViewMtx, invMtx);
                    PSMTXConcat(tmpMtx, invMtx, tmpMtx);
                    tmpMtx[2][3] = 0.0f;
                    tmpMtx[1][3] = 0.0f;
                    tmpMtx[0][3] = 0.0f;
                    mtx          = &tmpMtx;
                    break;
                case 0:
                case 4:
                default:
                    mtx = &mtxObj->getMtx(i);
                    break;
                }
                GXLoadTexMtxImm(*mtx, i * 3 + 0x40, GX_MTX3x4);
            }
        }
    } else {
        for (u16 i = 0; i < texMtxNum; i++) {
            int type = block->getTexCoord(i)->getTexGenType();
            if (type == 1 || type == 0) {
                J3DTexMtx* tex = block->getTexMtx(i);
                u8 info        = tex->getTexMtxInfo().mInfo;
                Mtx* mtx;
                u32 maya;
                switch (info & 0x3F) {
                case 3:
                case 9:
                    PSMTXConcat(mtxObj->getMtx(i), srcMtx, tmpMtx);
                    mtx = &tmpMtx;
                    break;
                case 1:
                case 6:
                case 7:
                    PSMTXCopy(srcMtx, invMtx);
                    invMtx[2][3] = 0.0f;
                    invMtx[1][3] = 0.0f;
                    invMtx[0][3] = 0.0f;
                    PSMTXConcat(mtxObj->getMtx(i), invMtx, tmpMtx);
                    mtx = &tmpMtx;
                    break;
                case 2:
                case 8:
                    mtx = &mtxObj->getMtx(i);
                    break;
                case 5:
                    maya = (info & 0x80) >> 7;
                    if (maya == 0) {
                        J3DGetTextureMtxOld(tex->getTexMtxInfo().mSRT, tex->getTexMtxInfo().mCenter, texMtx);
                    } else if (maya == 1) {
                        J3DGetTextureMtxMayaOld(tex->getTexMtxInfo().mSRT, texMtx);
                    }
                    J3DMtxProjConcat(texMtx, mtxObj->getEffectMtx(i), tmpMtx);
                    PSMTXInverse(j3dSys.mViewMtx, invMtx);
                    PSMTXConcat(tmpMtx, invMtx, tmpMtx);
                    PSMTXConcat(tmpMtx, srcMtx, tmpMtx);
                    tmpMtx[2][3] = 0.0f;
                    tmpMtx[1][3] = 0.0f;
                    tmpMtx[0][3] = 0.0f;
                    mtx          = &tmpMtx;
                    break;
                case 11:
                    maya = (info & 0x80) >> 7;
                    if (maya == 0) {
                        J3DGetTextureMtx(tex->getTexMtxInfo().mSRT, tex->getTexMtxInfo().mCenter, texMtx);
                    } else if (maya == 1) {
                        J3DGetTextureMtxMaya(tex->getTexMtxInfo().mSRT, texMtx);
                    }
                    PSMTXConcat(texMtx, qMtx, texMtx);
                    J3DMtxProjConcat(texMtx, mtxObj->getEffectMtx(i), tmpMtx);
                    PSMTXInverse(j3dSys.mViewMtx, invMtx);
                    PSMTXConcat(tmpMtx, invMtx, tmpMtx);
                    PSMTXConcat(tmpMtx, srcMtx, tmpMtx);
                    tmpMtx[2][3] = 0.0f;
                    tmpMtx[1][3] = 0.0f;
                    tmpMtx[0][3] = 0.0f;
                    mtx          = &tmpMtx;
                    break;
                case 10:
                    maya = (info & 0x80) >> 7;
                    if (maya == 0) {
                        J3DGetTextureMtx(tex->getTexMtxInfo().mSRT, tex->getTexMtxInfo().mCenter, texMtx);
                    } else if (maya == 1) {
                        J3DGetTextureMtxMaya(tex->getTexMtxInfo().mSRT, texMtx);
                    }
                    PSMTXConcat(texMtx, qMtx2, texMtx);
                    J3DMtxProjConcat(texMtx, mtxObj->getEffectMtx(i), tmpMtx);
                    PSMTXInverse(j3dSys.mViewMtx, invMtx);
                    PSMTXConcat(tmpMtx, invMtx, tmpMtx);
                    PSMTXConcat(tmpMtx, srcMtx, tmpMtx);
                    tmpMtx[2][3] = 0.0f;
                    tmpMtx[1][3] = 0.0f;
                    tmpMtx[0][3] = 0.0f;
                    mtx          = &tmpMtx;
                    break;
                case 0:
                case 4:
                default:
                    mtx = &mtxObj->getMtx(i);
                    break;
                }
                GXLoadTexMtxImm(*mtx, i * 3 + 30, (GXTexMtxType)tex->getTexMtxInfo().mProjection);
            }
        }
    }
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

void J3DShapeMtxConcatView::loadMtxConcatView_PCPU(int slot, u16 drawIdx) const
{
    Mtx mtx;
    PSMTXConcat(*j3dSys.mShapePacket->mBaseMtxPtr, j3dSys._104[drawIdx], mtx);
    if (J3DDifferedTexMtx::sTexGenBlock != NULL) {
        J3DDifferedTexMtx::loadExecute(mtx);
    }
    J3DFifoLoadPosMtxImm(*j3dSys.mShapePacket->mBaseMtxPtr, slot * 3);
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

void J3DShapeMtxConcatView::loadMtxConcatView_PNCPU(int slot, u16 drawIdx) const
{
    if (J3DDifferedTexMtx::sTexGenBlock != NULL) {
        Mtx mtx;
        PSMTXConcat(*j3dSys.mShapePacket->mBaseMtxPtr, j3dSys._104[drawIdx], mtx);
        J3DDifferedTexMtx::loadExecute(mtx);
    }
    J3DFifoLoadPosMtxImm(*j3dSys.mShapePacket->mBaseMtxPtr, slot * 3);
    J3DFifoLoadNrmMtxImm(*j3dSys.mShapePacket->mBaseMtxPtr, slot * 3);
    if (sTexMtxLoadType == 0x2000) {
        J3DFifoLoadNrmMtxToTexMtx(*j3dSys.mShapePacket->mBaseMtxPtr, slot * 3 + 0x1E);
    }
}

void J3DShapeMtxConcatView::loadMtxConcatView_PNGP_LOD(int slot, u16 drawIdx) const
{
    Mtx mtx;
    PSMTXConcat(*j3dSys.mShapePacket->mBaseMtxPtr, j3dSys._104[drawIdx], mtx);
    PSMTXConcat(mtx, j3dSys.getModel()->getModelData()->getInvJointMtx(drawIdx), mtx);
    if (J3DDifferedTexMtx::sTexGenBlock != NULL) {
        J3DDifferedTexMtx::loadExecute(mtx);
    }
    J3DFifoLoadPosMtxImm(mtx, slot * 3);
    loadNrmMtx(slot, drawIdx, mtx);
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

void J3DShapeMtxConcatView::load() const
{
    sMtxPtrTbl[0] = j3dSys.getModel()->mMtxBuffer->mUserAnmMtx;
    sMtxPtrTbl[1] = j3dSys.getModel()->mMtxBuffer->mWeightEnvelopeMatrices;
    LoadMtxConcatViewFunction function = sMtxLoadPipeline[sCurrentPipeline];
    j3dSys.setModelDrawMtx(sMtxPtrTbl[j3dSys.getModel()->getModelData()->getJointTree().getDrawMtxFlag(mUseMtxIndex)]);
    (this->*function)(0, j3dSys.getModel()->getModelData()->getJointTree().mMtxData.mDrawMtxIdx[mUseMtxIndex]);
}

void J3DShapeMtxConcatView::loadNrmMtx(int slot, u16 drawIdx, Mtx srcMtx) const
{
    if (sCurrentScaleFlag[drawIdx] == 1) {
        if (sTexMtxLoadType == 0x2000) {
            J3DFifoLoadNrmMtxToTexMtx(srcMtx, 0x1E);
        }
        if (!sNBTFlag) {
            J3DFifoLoadNrmMtxImm(srcMtx, 0);
        } else {
            Mtx33 mtx;
            J3DPSMtx33CopyFrom34(srcMtx, mtx);
            J3DScaleNrmMtx33(mtx, *j3dSys._118);
            J3DFifoLoadNrmMtxImm3x3(mtx, 0);
        }
    } else {
        Mtx33 mtx;
        J3DPSCalcInverseTranspose(srcMtx, mtx);
        if (sTexMtxLoadType == 0x2000) {
            J3DFifoLoadNrmMtxToTexMtx3x3(mtx, 0x1E);
        }
        if (!sNBTFlag) {
            J3DFifoLoadNrmMtxImm3x3(mtx, 0);
        } else {
            J3DScaleNrmMtx33(mtx, *j3dSys._118);
            J3DFifoLoadNrmMtxImm3x3(mtx, 0);
        }
    }
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

void J3DShapeMtxMulti::calcNBTScale(const Vec& scale, Mtx33* srcArray, Mtx33* dstArray)
{
    int count = mUseMtxNum;
    for (int i = 0; i < count; i++) {
        if (_0C[i] != 0xFFFF) {
            J3DPSMtx33Copy(srcArray[_0C[i]], dstArray[_0C[i]]);
            J3DScaleNrmMtx33(dstArray[_0C[i]], scale);
        }
    }
}

void J3DShapeMtxMultiConcatView::load() const
{
    static u8 mtxCache[20] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };

    sMtxPtrTbl[0] = j3dSys.getModel()->mMtxBuffer->mUserAnmMtx;
    sMtxPtrTbl[1] = j3dSys.getModel()->mMtxBuffer->mWeightEnvelopeMatrices;
    if (!sLODFlag) {
        LoadMtxConcatViewFunction function = sMtxLoadPipeline[sCurrentPipeline];
        int count = mUseMtxNum;
        for (int i = 0; i < count; i++) {
            u16 useMtxIndex = _0C[i];
            if (useMtxIndex != 0xFFFF) {
                J3DJointTree& tree = j3dSys.getModel()->getModelData()->getJointTree();
                u16 drawMtxIndex = tree.getDrawMtxIndex(useMtxIndex);
                j3dSys.setModelDrawMtx(sMtxPtrTbl[tree.getDrawMtxFlag(useMtxIndex)]);
                (this->*function)(i, drawMtxIndex);
            }
        }
    } else {
        u16* importantMtxIndex = j3dSys.getModel()->getModelData()->getJointTree().getWEvlpImportantMtxIndex();
        j3dSys.setModelDrawMtx(sMtxPtrTbl[0]);
        int count = mUseMtxNum;
        for (int i = 0; i < count; i++) {
            u32 pipeline = sCurrentPipeline;
            LoadMtxConcatViewFunction function = sMtxLoadLODPipeline[pipeline];
            u16 useMtxIndex = _0C[i];
            if (useMtxIndex != 0xFFFF) {
                u16 index = importantMtxIndex[useMtxIndex];
                if (index != sMtxLoadCache[i]) {
                    if (j3dSys.getModel()->getModelData()->getJointTree().getDrawMtxFlag(useMtxIndex) == 0) {
                        function = sMtxLoadPipeline[pipeline];
                    }
                    (this->*function)(i, index);
                    sMtxLoadCache[i] = index;
                }
            }
        }
    }
}

void J3DShapeMtxMultiConcatView::loadNrmMtx(int slot, u16 drawIdx, Mtx srcMtx) const
{
    if (sCurrentScaleFlag[drawIdx] == 1) {
        if (sTexMtxLoadType == 0x2000) {
            J3DFifoLoadNrmMtxToTexMtx(srcMtx, slot * 3 + 0x1E);
        }
        if (!sNBTFlag) {
            J3DFifoLoadNrmMtxImm(srcMtx, slot * 3);
        } else {
            Mtx33 mtx;
            J3DPSMtx33CopyFrom34(srcMtx, mtx);
            J3DScaleNrmMtx33(mtx, *j3dSys._118);
            J3DFifoLoadNrmMtxImm3x3(mtx, 0);
        }
    } else {
        Mtx33 mtx;
        J3DPSCalcInverseTranspose(srcMtx, mtx);
        if (sTexMtxLoadType == 0x2000) {
            J3DFifoLoadNrmMtxToTexMtx3x3(mtx, slot * 3 + 0x1E);
        }
        if (!sNBTFlag) {
            J3DFifoLoadNrmMtxImm3x3(mtx, slot * 3);
        } else {
            J3DScaleNrmMtx33(mtx, *j3dSys._118);
            J3DFifoLoadNrmMtxImm3x3(mtx, slot * 3);
        }
    }
}

void J3DShapeMtxBBoardConcatView::load() const
{
    Mtx mtx;
    J3DModel* model = j3dSys.getModel();
    u16 drawMtxIndex = model->getModelData()->getJointTree().getDrawMtxIndex(mUseMtxIndex);
    if (model->getModelData()->getJointTree().getDrawMtxFlag(mUseMtxIndex) == 0) {
        PSMTXConcat(j3dSys.mViewMtx, model->mMtxBuffer->mUserAnmMtx[drawMtxIndex], mtx);
    } else {
        PSMTXConcat(j3dSys.mViewMtx, model->mMtxBuffer->mWeightEnvelopeMatrices[drawMtxIndex], mtx);
    }
    J3DCalcBBoardMtx(mtx);
    J3DFifoLoadPosMtxImm(mtx, 0);
    mtx[0][0] = 1.0f / mtx[0][0];
    mtx[1][1] = 1.0f / mtx[1][1];
    mtx[2][2] = 1.0f / mtx[2][2];
    mtx[0][3] = 0.0f;
    mtx[1][3] = 0.0f;
    mtx[2][3] = 0.0f;
    if (!sNBTFlag) {
        J3DFifoLoadNrmMtxImm(mtx, 0);
    } else {
        J3DScaleNrmMtx(mtx, *j3dSys._118);
        J3DFifoLoadNrmMtxImm(mtx, 0);
    }
}

void J3DShapeMtxYBBoardConcatView::load() const
{
    Mtx mtx;
    J3DModel* model = j3dSys.getModel();
    u16 drawMtxIndex = model->getModelData()->getJointTree().getDrawMtxIndex(mUseMtxIndex);
    if (model->getModelData()->getJointTree().getDrawMtxFlag(mUseMtxIndex) == 0) {
        PSMTXConcat(j3dSys.mViewMtx, model->mMtxBuffer->mUserAnmMtx[drawMtxIndex], mtx);
    } else {
        PSMTXConcat(j3dSys.mViewMtx, model->mMtxBuffer->mWeightEnvelopeMatrices[drawMtxIndex], mtx);
    }
    J3DCalcYBBoardMtx(mtx);
    J3DFifoLoadPosMtxImm(mtx, 0);
    if (sCurrentScaleFlag[mUseMtxIndex] == 1) {
        if (!sNBTFlag) {
            J3DFifoLoadNrmMtxImm(mtx, 0);
        } else {
            J3DScaleNrmMtx(mtx, *j3dSys._118);
            J3DFifoLoadNrmMtxImm(mtx, 0);
        }
    } else {
        Mtx33 nrmMtx;
        J3DPSCalcInverseTranspose(mtx, nrmMtx);
        if (!sNBTFlag) {
            J3DFifoLoadNrmMtxImm3x3(nrmMtx, 0);
        } else {
            J3DScaleNrmMtx33(nrmMtx, *j3dSys._118);
            J3DFifoLoadNrmMtxImm3x3(nrmMtx, 0);
        }
    }
}
