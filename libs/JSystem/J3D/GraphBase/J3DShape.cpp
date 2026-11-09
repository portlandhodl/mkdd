#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DGD.h"
#include "JSystem/J3D/J3DPacket.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DVertexData.h"
#include "JSystem/JKernel/JKRHeap.h"
#include <dolphin/gd.h>
#include <dolphin/os.h>
#include <string.h>

static inline void J3DFifoLoadCPCmd(u8 reg, u32 val) {
    GXWGFifo.u8 = 8;
    GXWGFifo.u8 = reg;
    GXWGFifo.u32 = val;
}

static inline void J3DFifoLoadXFCmdHdr(u16 cmd, u16 len) {
    GXWGFifo.u8 = 16;
    GXWGFifo.u16 = len - 1;
    GXWGFifo.u16 = cmd;
}

void J3DShape::initialize() {
    mMaterial = NULL;
    mId = -1;
    mMtxGroupNum = 0;
    mFlags = 0;
    mRadius = 0.0f;
    mMin.x = 0.0f;
    mMin.y = 0.0f;
    mMin.z = 0.0f;
    mMax.x = 0.0f;
    mMax.y = 0.0f;
    mMax.z = 0.0f;
    mVtxDesc = NULL;
    mShapeMtx = NULL;
    mShapeDraw = NULL;
    mVtxData = NULL;
    mDrawMtxData = NULL;
    mFlagList = NULL;
    mTree1 = NULL;
    mTree2 = NULL;
    mCurrentViewNumber = (u32*)&j3dDefaultViewNo;
    mMode = false;
    _48 = false;
}

J3DShape::~J3DShape() {
}

void J3DShape::addTexMtxIndexInDL(GXAttr attr, u32 valueBase) {
    u32 kSize[] = {0, 1, 1, 2};

    s32 pnmtxidxOffs = -1;
    s32 attrOffs = -1;
    s32 stride = 0;
    bool found = false;

    for (GXVtxDescList* vtxDesc = mVtxDesc; vtxDesc->attr != GX_VA_NULL; vtxDesc++) {
        if (vtxDesc->attr == GX_VA_PNMTXIDX)
            pnmtxidxOffs = stride;

        if (attr < vtxDesc->attr && !found) {
            attrOffs = stride;
            found = true;
        }

        stride = stride + kSize[vtxDesc->type];
    }

    if (pnmtxidxOffs == -1)
        return;

    for (u16 i = 0; i < (u16)getMtxGroupNum(); i++)
        getShapeDraw(i)->addTexMtxIndexInDL(stride, attrOffs, (s32)valueBase);
}

void J3DShape::addTexMtxIndexInVcd(GXAttr attr) {
    s32 attrIdx = -1;
    s32 attrOffs = -1;
    s32 stride = 0;

    GXVtxDescList* vtxDesc = mVtxDesc;
    s32 attrCount = 0;

    for (; vtxDesc->attr != GX_VA_NULL; attrCount++, vtxDesc++) {
        if (vtxDesc->attr == GX_VA_PNMTXIDX)
            attrIdx = stride;
    }

    if (attrIdx == -1)
        return;

    GXVtxDescList* newVtxDesc = new GXVtxDescList[attrCount + 2];
    bool inserted = false;

    vtxDesc = mVtxDesc;
    GXVtxDescList* dst = newVtxDesc;
    for (; vtxDesc->attr != GX_VA_NULL; dst++, vtxDesc++) {
        if ((attr < vtxDesc->attr) && !inserted) {
            dst->attr = attr;
            dst->type = GX_DIRECT;
            attrOffs = stride;
            dst++;

            inserted = true;
        }

        dst->attr = vtxDesc->attr;
        dst->type = vtxDesc->type;
        stride += vtxDesc->type == GX_INDEX16 ? sizeof(u16) : vtxDesc->type != GX_NONE;
    }

    dst->attr = GX_VA_NULL;
    dst->type = GX_NONE;
    mVtxDesc = newVtxDesc;
    makeVcdVatCmd();
}

void J3DShape::calcNBTScale(const Vec& param_0, Mtx33* param_1, Mtx33* param_2) {
    for (u16 i = 0; i < mMtxGroupNum; i++)
        mShapeMtx[i]->calcNBTScale(param_0, param_1, param_2);
}

int J3DShape::countBumpMtxNum() const {
    u32 num = 0;
    for (u16 i = 0; i < mMtxGroupNum; i++)
        num += mShapeMtx[i]->getUseMtxNum();

    return num;
}

void J3DLoadCPCmd(u8 addr, u32 val) {
    GXWGFifo.u8 = 8;
    GXWGFifo.u8 = addr;
    GXWGFifo.u32 = val;
}

static void J3DLoadArrayBasePtr(GXAttr attr, void* data) {
    u32 idx = (attr == GX_VA_NBT) ? 1 : (attr - GX_VA_POS);
    J3DLoadCPCmd(0xA0 + idx, ((u32)data & 0x7FFFFFFF));
}

void J3DShape::loadVtxArray() const {
    J3DLoadArrayBasePtr(GX_VA_POS, j3dSys.getVtxPos());

    if (!mMode) {
        J3DLoadArrayBasePtr(GX_VA_NRM, j3dSys.getVtxNrm());
    }

    J3DLoadArrayBasePtr(GX_VA_CLR0, j3dSys.getVtxCol());
}

bool J3DShape::isSameVcdVatCmd(J3DShape* other) {
    u8* a = (u8*)other->getVcdVatCmd();
    u8* b = mVcdVatCmd;
    for (u32 i = 0; i < kVcdVatDLSize; i++)
        if (a[i] != b[i])
            return false;

    return true;
}

void J3DShape::makeVtxArrayCmd() {
    GXVtxAttrFmtList* vtxAttr = mVtxData->getVtxAttrFmtList();

    u8 stride[12];
    void* array[12];
    for (u32 i = 0; i < 12; i++) {
        stride[i] = 0;
        array[i] = 0;
    }

    for (; vtxAttr->attr != GX_VA_NULL; vtxAttr++) {
        switch (vtxAttr->attr) {
        case GX_VA_POS: {
            if (vtxAttr->type == GX_F32)
                stride[vtxAttr->attr - GX_VA_POS] = 12;
            else
                stride[vtxAttr->attr - GX_VA_POS] = 6;

            array[vtxAttr->attr - GX_VA_POS] = mVtxData->getVtxPosArray();
            mVtxData->setVtxPosFrac((u8)vtxAttr->frac);
            mVtxData->setVtxPosType((GXCompType)vtxAttr->type);
        } break;
        case GX_VA_NRM: {
            if (vtxAttr->type == GX_F32)
                stride[vtxAttr->attr - GX_VA_POS] = 12;
            else
                stride[vtxAttr->attr - GX_VA_POS] = 6;

            array[vtxAttr->attr - GX_VA_POS] = mVtxData->getVtxNrmArray();
            mVtxData->setVtxNrmFrac((u8)vtxAttr->frac);
            mVtxData->setVtxNrmType((GXCompType)vtxAttr->type);
        } break;
        case GX_VA_CLR0:
        case GX_VA_CLR1: {
            stride[vtxAttr->attr - GX_VA_POS] = 4;
            array[vtxAttr->attr - GX_VA_POS] = mVtxData->getVtxColorArray(vtxAttr->attr - GX_VA_CLR0);
        } break;
        case GX_VA_TEX0:
        case GX_VA_TEX1:
        case GX_VA_TEX2:
        case GX_VA_TEX3:
        case GX_VA_TEX4:
        case GX_VA_TEX5:
        case GX_VA_TEX6:
        case GX_VA_TEX7: {
            if (vtxAttr->type == GX_F32)
                stride[vtxAttr->attr - GX_VA_POS] = 8;
            else
                stride[vtxAttr->attr - GX_VA_POS] = 4;

            array[vtxAttr->attr - GX_VA_POS] = mVtxData->getVtxTexCoordArray(vtxAttr->attr - GX_VA_TEX0);
        } break;
        default:
            break;
        }
    }

    GXVtxDescList* vtxDesc = mVtxDesc;
    _48 = false;
    for (; vtxDesc->attr != GX_VA_NULL; vtxDesc++) {
        if (vtxDesc->attr == GX_VA_NBT && vtxDesc->type != GX_NONE) {
            mMode = true;
            stride[GX_VA_NRM - GX_VA_POS] *= 3;
            array[GX_VA_NRM - GX_VA_POS] = mVtxData->getVtxNBTArray();
        } else if (vtxDesc->attr == GX_VA_PNMTXIDX && vtxDesc->type != GX_NONE) {
            _48 = true;
        }
    }

    for (u32 i = 0; i < 12; i++) {
        if (array[i] != 0)
            GDSetArray((GXAttr)(i + GX_VA_POS), array[i], stride[i]);
        else
            GDSetArrayRaw((GXAttr)(i + GX_VA_POS), NULL, stride[i]);
    }
}

void J3DShape::makeVcdVatCmd() {
    static BOOL sInterruptFlag = OSDisableInterrupts();
    OSDisableScheduler();

    GDCurrentDL gdl_obj;
    GDInitGDLObj(&gdl_obj, mVcdVatCmd, kVcdVatDLSize);
    __GDSetCurrent(&gdl_obj);
    GDSetVtxDescv(mVtxDesc);
    makeVtxArrayCmd();
    J3DGDSetVtxAttrFmtv(GX_VTXFMT0, mVtxData->getVtxAttrFmtList(), mMode);
    GDPadCurr32();
    GDFlushCurrToMem();
    __GDSetCurrent(NULL);
    OSEnableScheduler();
    OSRestoreInterrupts(sInterruptFlag);
}

void J3DShape::loadCurrentMtx() const {
    J3DFifoLoadCPCmd(0x30, *(volatile u32*)&mCurrentMtx.mMtxIdxRegA);
    J3DFifoLoadCPCmd(0x40, *(volatile u32*)&mCurrentMtx.mMtxIdxRegB);
    J3DFifoLoadXFCmdHdr(0x1018, 2);
    GXWGFifo.u32 = *(volatile u32*)&mCurrentMtx.mMtxIdxRegA;
    GXWGFifo.u32 = *(volatile u32*)&mCurrentMtx.mMtxIdxRegB;
}

void J3DShape::loadPreDrawSetting() const {
    if (sOldVcdVatCmd != mVcdVatCmd) {
        GXCallDisplayList(mVcdVatCmd, kVcdVatDLSize);
        sOldVcdVatCmd = mVcdVatCmd;
    }

    J3DFifoLoadCPCmd(0x30, *(volatile u32*)&mCurrentMtx.mMtxIdxRegA);
    J3DFifoLoadCPCmd(0x40, *(volatile u32*)&mCurrentMtx.mMtxIdxRegB);
    J3DFifoLoadXFCmdHdr(0x1018, 2);
    GXWGFifo.u32 = *(volatile u32*)&mCurrentMtx.mMtxIdxRegA;
    GXWGFifo.u32 = *(volatile u32*)&mCurrentMtx.mMtxIdxRegB;
}

void J3DShape::setArrayAndBindPipeline() const {
    J3DShapeMtx::sCurrentPipeline = (mFlags & 0x1C) >> 2;
    loadVtxArray();
    j3dSys.setModelDrawMtx(((Mtx**)mTree1)[*mCurrentViewNumber]);
    j3dSys.setModelNrmMtx(((Mtx33**)mTree2)[*mCurrentViewNumber]);
    J3DShapeMtx::sCurrentScaleFlag = mFlagList;
    J3DShapeMtx::sNBTFlag = mMode;
    sEnvelopeFlag = _48;
    J3DShapeMtx::sTexMtxLoadType = getTexMtxLoadType();
}

void J3DShape::drawFast() const {
    if (sOldVcdVatCmd != mVcdVatCmd) {
        GXCallDisplayList(mVcdVatCmd, kVcdVatDLSize);
        sOldVcdVatCmd = mVcdVatCmd;
    }

    if (sEnvelopeFlag != 0 && !_48) {
        J3DFifoLoadCPCmd(0x30, mCurrentMtx.mMtxIdxRegA);
        J3DFifoLoadCPCmd(0x40, mCurrentMtx.mMtxIdxRegB);
        J3DFifoLoadXFCmdHdr(0x1018, 2);
        GXWGFifo.u32 = *(volatile u32*)&mCurrentMtx.mMtxIdxRegA;
        GXWGFifo.u32 = *(volatile u32*)&mCurrentMtx.mMtxIdxRegB;
    }

    setArrayAndBindPipeline();
    if (!checkFlag(NoMtx)) {
        if (J3DShapeMtx::sLODFlag)
            J3DShapeMtx::resetMtxLoadCache();

        for (u16 n = mMtxGroupNum, i = 0; i < n; i++) {
            if (mShapeMtx[i] != NULL)
                mShapeMtx[i]->load();
            if (mShapeDraw[i] != NULL)
                mShapeDraw[i]->draw();
        }
    } else {
        J3DFifoLoadPosMtxImm(*j3dSys.mShapePacket->getBaseMtxPtr(), GX_PNMTX0);
        J3DFifoLoadNrmMtxImm(*j3dSys.mShapePacket->getBaseMtxPtr(), GX_PNMTX0);
        for (u16 n = mMtxGroupNum, i = 0; i < n; i++)
            if (mShapeDraw[i] != NULL)
                mShapeDraw[i]->draw();
    }
}

void J3DShape::draw() const {
    resetVcdVatCache();
    loadPreDrawSetting();
    drawFast();
}

void J3DShape::simpleDraw() const {
    resetVcdVatCache();
    if (sOldVcdVatCmd != mVcdVatCmd) {
        GXCallDisplayList(mVcdVatCmd, kVcdVatDLSize);
        sOldVcdVatCmd = mVcdVatCmd;
    }
    J3DFifoLoadCPCmd(0x30, mCurrentMtx.mMtxIdxRegA);
    J3DFifoLoadCPCmd(0x40, mCurrentMtx.mMtxIdxRegB);
    J3DFifoLoadXFCmdHdr(0x1018, 2);
    GXWGFifo.u32 = *(volatile u32*)&mCurrentMtx.mMtxIdxRegA;
    GXWGFifo.u32 = *(volatile u32*)&mCurrentMtx.mMtxIdxRegB;
    J3DShapeMtx::sCurrentPipeline = (mFlags & 0x1C) >> 2;
    loadVtxArray();
    for (u32 n = getMtxGroupNum(), i = 0; i < n; i++) {
        if (mShapeDraw[i] != NULL) {
            mShapeDraw[i]->draw();
        }
    }
}

void J3DShape::simpleDrawCache() const {
    if (sOldVcdVatCmd != mVcdVatCmd) {
        GXCallDisplayList(mVcdVatCmd, kVcdVatDLSize);
        sOldVcdVatCmd = mVcdVatCmd;
    }

    if (sEnvelopeFlag && !_48) {
        register u32 a = mCurrentMtx.mMtxIdxRegA;
        register u32 b = mCurrentMtx.mMtxIdxRegB;
        J3DFifoLoadCPCmd(0x30, a);
        J3DFifoLoadCPCmd(0x40, b);
        J3DFifoLoadXFCmdHdr(0x1018, 2);
        GXWGFifo.u32 = *(volatile u32*)&mCurrentMtx.mMtxIdxRegA;
        GXWGFifo.u32 = *(volatile u32*)&mCurrentMtx.mMtxIdxRegB;
    }

    loadVtxArray();
    for (u32 n = getMtxGroupNum(), i = 0; i < n; i++)
        if (mShapeDraw[i] != NULL)
            mShapeDraw[i]->draw();
}

u8* J3DShape::sOldVcdVatCmd;
u8 J3DShape::sEnvelopeFlag;
