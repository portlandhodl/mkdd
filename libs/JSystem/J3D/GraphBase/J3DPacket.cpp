#include "JSystem/J3D/J3DPacket.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DMtxBuffer.h"
#include "JSystem/J3D/J3DDrawBuffer.h"
#include "JSystem/J3D/J3DMaterial.h"
#include "JSystem/J3D/J3DTexGenBlock.h"
#include "JSystem/J3D/J3DTevBlock.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DTexMtx.h"
#include "macros.h"
#include "JSystem/JKernel/JKRHeap.h"
#include <dolphin/os.h>
#include <string.h>

J3DErrType J3DDisplayListObj::newDisplayList(u32 maxSize) {
    mCapacity = ALIGN_NEXT(maxSize, 0x20);
    mDisplayList[0] = new (0x20) u8[mCapacity];
    mDisplayList[1] = new (0x20) u8[mCapacity];
    mSize = 0;

    return JET_Success;
}

J3DErrType J3DDisplayListObj::newSingleDisplayList(u32 maxSize) {
    mCapacity = ALIGN_NEXT(maxSize, 0x20);
    mDisplayList[0] = new (0x20) u8[mCapacity];
    mDisplayList[1] = mDisplayList[0];
    mSize = 0;

    return JET_Success;
}

J3DErrType J3DDisplayListObj::single_To_Double() {
    if (mDisplayList[0] == mDisplayList[1]) {
        mDisplayList[1] = new (0x20) u8[mCapacity];

        memcpy(mDisplayList[1], mDisplayList[0], mCapacity);
        DCStoreRange(mDisplayList[1], mCapacity);
    }

    return JET_Success;
}

void J3DDisplayListObj::setDisplayList(void* pDLData, u32 size) {
    mCapacity = ALIGN_NEXT(size, 0x20);
    mDisplayList[0] = (u8*)pDLData;
    mDisplayList[1] = new (0x20) u8[mCapacity];
    memcpy(mDisplayList[1], pDLData, size);
    DCStoreRange(mDisplayList[1], size);
    mSize = size;
}

void J3DDisplayListObj::setSingleDisplayList(void* pDLData, u32 size) {
    mCapacity = ALIGN_NEXT(size, 0x20);
    mDisplayList[0] = (u8*)pDLData;
    mDisplayList[1] = mDisplayList[0];
    mSize = size;
}

void J3DDisplayListObj::swapBuffer() {
    u8* tmp = mDisplayList[0];
    mDisplayList[0] = mDisplayList[1];
    mDisplayList[1] = tmp;
}

void J3DDisplayListObj::callDL() const {
    GXCallDisplayList(mDisplayList[0], mSize);
}

void J3DDisplayListObj::fifoWrite() const {
    GXCallDisplayList(mDisplayList[0], mSize);
}

void J3DDisplayListObj::beginDL() {
    u8* tmp = mDisplayList[0];
    mDisplayList[0] = mDisplayList[1];
    mDisplayList[1] = tmp;
    sInterruptFlag = OSDisableInterrupts();
    GDInitGDLObj(&sGDLObj, mDisplayList[0], mCapacity);
    __GDSetCurrent(&sGDLObj);
}

u32 J3DDisplayListObj::endDL() {
    GDPadCurr32();
    OSRestoreInterrupts(sInterruptFlag);
    mSize = sGDLObj.pDisplayListData - sGDLObj.begin;
    GDFlushCurrToMem();
    __GDSetCurrent(NULL);
    return mSize;
}

void J3DDisplayListObj::beginPatch() {
    beginDL();
}

u32 J3DDisplayListObj::endPatch() {
    OSRestoreInterrupts(sInterruptFlag);
    __GDSetCurrent(NULL);
    return mSize;
}

bool J3DPacket::isSame(J3DMatPacket*) const {
    return false;
}

int J3DPacket::entry(J3DDrawBuffer*) {
    return 1;
}

void J3DPacket::addChildPacket(J3DPacket* pPacket) {
    if (mChildPacket == NULL) {
        mChildPacket = pPacket;
    } else {
        pPacket->setNextPacket(mChildPacket);
        mChildPacket = pPacket;
    }
}

DECL_WEAK void J3DPacket::draw() {
}

J3DTexMtxObj::J3DTexMtxObj(u16 num) {
    mTexMtx = new Mtx[num];
    mEffMtx = new Mtx44[num];
    mTexMtxNum = num;
}

J3DTexMtxObj::~J3DTexMtxObj() {
    delete[] mTexMtx;
    delete[] mEffMtx;
}

J3DDrawPacket::J3DDrawPacket() {
    mFlags = 0;
    mDisplayList = NULL;
    mTexMtxObj = NULL;
}

DECL_WEAK J3DPacket::~J3DPacket() {
}

J3DDrawPacket::~J3DDrawPacket() {
}

J3DErrType J3DDrawPacket::newDisplayList(u32 size) {
    mDisplayList = new J3DDisplayListObj();

    if (mDisplayList == NULL)
        return JET_OutOfMemory;

    J3DErrType ret = mDisplayList->newDisplayList(size);
    if (ret != JET_Success)
        return ret;

    return JET_Success;
}

J3DErrType J3DDrawPacket::newSingleDisplayList(u32 size) {
    mDisplayList = new J3DDisplayListObj();

    if (mDisplayList == NULL)
        return JET_OutOfMemory;

    J3DErrType ret = mDisplayList->newSingleDisplayList(size);
    if (ret != JET_Success)
        return ret;

    return JET_Success;
}

void J3DDrawPacket::draw() {
    mDisplayList->callDL();
}

J3DMatPacket::J3DMatPacket() {
    mInitShapePacket = NULL;
    mMaterial = NULL;
    mDiffFlag = 0xFFFFFFFF;
    mTexture = NULL;
    mMaterialAnm = NULL;
}

J3DMatPacket::~J3DMatPacket() {
}

void J3DMatPacket::addShapePacket(J3DShapePacket* pShape) {
    if (mShapePacket == NULL) {
        mShapePacket = pShape;
    } else {
        pShape->setNextPacket(mShapePacket);
        mShapePacket = pShape;
    }
}

void J3DMatPacket::beginDiff() {
    mInitShapePacket->beginDL();
}

u32 J3DMatPacket::endDiff() {
    return mInitShapePacket->endDL();
}

bool J3DMatPacket::isSame(J3DMatPacket* pOther) const {
    return mDiffFlag == pOther->mDiffFlag && (mDiffFlag >> 31) == 0;
}

void J3DMatPacket::draw() {
    mMaterial->load();
    callDL();

    J3DShapePacket* packet = getShapePacket();
    packet->getShape()->loadPreDrawSetting();

    while (packet != NULL) {
        if (packet->mDisplayList != NULL)
            packet->mDisplayList->callDL();

        packet->drawFast();
        packet = (J3DShapePacket*)packet->getNextPacket();
    }

    J3DShape::resetVcdVatCache();
}

static u32 sDifferedRegister[7] = {
    J3DMDF_DiffMatColor, J3DMDF_DiffLight,          J3DMDF_DiffColorReg,
    J3DMDF_DiffFog,      J3DMDF_DiffBlend,          J3DMDF_DiffKonstColor, J3DMDF_DiffTevStageIndirect,
};

static s32 sSizeOfDiffered[7] = {
    13, 21, 120, 55, 15, 19, 45,
};

J3DShapePacket::J3DShapePacket() {
    mShape = NULL;
    mMtxBuffer = NULL;
    mBaseMtxPtr = NULL;
    mDiffFlag = 0;
    mModel = NULL;
}

J3DShapePacket::~J3DShapePacket() {
}

int J3DShapePacket::calcDifferedBufferSize(u32 diffFlags) {
    u32 bufferSize = 0;

    for (u32 i = 0; i < 7; i++) {
        if ((diffFlags & sDifferedRegister[i]) != 0) {
            bufferSize += sSizeOfDiffered[i];
        }
    }

    u32 lightObjNum = getDiffFlag_LightObjNum(diffFlags);
    bufferSize += lightObjNum * 0x48;

    u32 texGenNum = getDiffFlag_TexGenNum(diffFlags);
    if (texGenNum != 0) {
        u32 mat_texGenNum = mShape->getMaterial()->getTexGenBlock()->getTexGenNum();
        u32 sp30 = texGenNum > mat_texGenNum ? texGenNum : mat_texGenNum;

        if (diffFlags & J3DMDF_DiffTexGen) {
            bufferSize += sp30 * 0x3d + 10;
        } else {
            bufferSize += calcDifferedBufferSize_TexMtxSize(sp30);
        }
    }

    u32 texNoNum = getDiffFlag_TexNoNum(diffFlags);
    if (texNoNum != 0) {
        u8 sp9;
        if (mShape->getMaterial()->getTevBlock()->getTevStageNum() > 8) {
            sp9 = 8;
        } else {
            sp9 = mShape->getMaterial()->getTevBlock()->getTevStageNum();
        }

        u32 mat_texNoNum = sp9;
        u32 sp24 = texNoNum > mat_texNoNum ? texNoNum : mat_texNoNum;

        if (diffFlags & J3DMDF_DiffTexCoordScale) {
            bufferSize += calcDifferedBufferSize_TexNoAndTexCoordScaleSize(sp24);
        } else {
            bufferSize += calcDifferedBufferSize_TexNoSize(sp24);
        }
    }

    u32 tevStageNum = getDiffFlag_TevStageNum(diffFlags);
    if (tevStageNum != 0) {
        u8 sp8;
        if (mShape->getMaterial()->getTevBlock()->getTevStageNum() > 8) {
            sp8 = 8;
        } else {
            sp8 = mShape->getMaterial()->getTevBlock()->getTevStageNum();
        }

        u32 mat_tevStageNum = sp8;
        u32 sp18 = tevStageNum > mat_tevStageNum ? tevStageNum : mat_tevStageNum;

        bufferSize += calcDifferedBufferSize_TevStageSize(sp18);
        if (diffFlags & J3DMDF_DiffTevStageIndirect) {
            bufferSize += calcDifferedBufferSize_TevStageDirectSize(sp18);
        }
    }

    return OSRoundUp32B(bufferSize);
}

J3DErrType J3DShapePacket::newDifferedDisplayList(u32 diffFlags) {
    mDiffFlag = diffFlags;

    u32 bufSize = calcDifferedBufferSize(diffFlags);
    J3DErrType ret = newDisplayList(bufSize);
    if (ret != JET_Success) {
        return ret;
    }

    return JET_Success;
}

J3DErrType J3DShapePacket::newDifferedTexMtx(J3DTexDiffFlag flag) {
    switch (flag) {
    case TEXDIFF_Material: {
        u32 texGenNum = mShape->getMaterial()->getTexGenBlock()->getTexGenNum();
        mTexMtxObj = new J3DTexMtxObj(texGenNum);
        if (mTexMtxObj == NULL)
            return JET_OutOfMemory;
        break;
    }
    case TEXDIFF_Default:
        mTexMtxObj = new J3DTexMtxObj(8);
        if (mTexMtxObj == NULL)
            return JET_OutOfMemory;
        break;
    default:
        return JET_InvalidArg;
    }

    return JET_Success;
}


void J3DShapePacket::prepareDraw() const {
    mModel->getVertexBuffer()->setArray();
    j3dSys.setModel(mModel);
    j3dSys.setShapePacket((J3DShapePacket*)this);
    J3DShapeMtx::sLODFlag = mModel->checkFlag(J3DMODEL_LevelOfDetail) != 0;

    if (mModel->checkFlag(J3DMODEL_SkinPosCpu)) {
        mShape->onFlag(J3DShape::SkinPos);
    } else {
        mShape->offFlag(J3DShape::SkinPos);
    }

    if (mModel->checkFlag(J3DMODEL_SkinNrmCpu) && mShape->checkFlag(J3DShape::EnableLOD) == false) {
        mShape->onFlag(J3DShape::SkinNorm);
    } else {
        mShape->offFlag(J3DShape::SkinNorm);
    }

    mShape->setCurrentViewNoPtr(mMtxBuffer->getCurrentViewNoPtr());
    mShape->setScaleFlagArray(mMtxBuffer->getScaleFlagArray());
    mShape->setDrawMtx(mMtxBuffer->getDrawMtxPtrPtr());

    if (!mShape->getNBTFlag()) {
        mShape->setNrmMtx(mMtxBuffer->getNrmMtxPtrPtr());
    } else {
        mShape->setNrmMtx(mMtxBuffer->mBumpMatrices[1][mShape->getBumpMtxOffset()]);
    }

    mModel->getModelData()->syncJ3DSysFlags();
}

#define J3D_PREPARE_DRAW_BODY() \
    mModel->getVertexBuffer()->setArray(); \
    j3dSys.setModel(mModel); \
    j3dSys.setShapePacket((J3DShapePacket*)this); \
    J3DShapeMtx::sLODFlag = mModel->checkFlag(J3DMODEL_LevelOfDetail) != 0; \
     \
    if (mModel->checkFlag(J3DMODEL_SkinPosCpu)) { \
    mShape->onFlag(J3DShape::SkinPos); \
    } else { \
    mShape->offFlag(J3DShape::SkinPos); \
    } \
     \
    if (mModel->checkFlag(J3DMODEL_SkinNrmCpu) && mShape->checkFlag(J3DShape::EnableLOD) == false) { \
    mShape->onFlag(J3DShape::SkinNorm); \
    } else { \
    mShape->offFlag(J3DShape::SkinNorm); \
    } \
     \
    mShape->setCurrentViewNoPtr(mMtxBuffer->getCurrentViewNoPtr()); \
    mShape->setScaleFlagArray(mMtxBuffer->getScaleFlagArray()); \
    mShape->setDrawMtx(mMtxBuffer->getDrawMtxPtrPtr()); \
     \
    if (!mShape->getNBTFlag()) { \
    mShape->setNrmMtx(mMtxBuffer->getNrmMtxPtrPtr()); \
    } else { \
    mShape->setNrmMtx(mMtxBuffer->mBumpMatrices[1][mShape->getBumpMtxOffset()]); \
    } \
     \
    mModel->getModelData()->syncJ3DSysFlags();

void J3DShapePacket::draw() {
    if (!checkFlag(0x10) && mShape != NULL) {
        J3D_PREPARE_DRAW_BODY();

        if (mTexMtxObj != NULL) {
            J3DDifferedTexMtx::sTexGenBlock = mShape->getMaterial()->getTexGenBlock();
            J3DDifferedTexMtx::sTexMtxObj = (J3DShapePacket_0x24*)getTexMtxObj();
        } else {
            J3DDifferedTexMtx::sTexGenBlock = NULL;
        }

        if (mDisplayList != NULL)
            mDisplayList->callDL();

        mShape->draw();
    }
}

void J3DShapePacket::drawFast() {
    if (!checkFlag(0x10) && mShape != NULL) {
        J3D_PREPARE_DRAW_BODY();

        if (mTexMtxObj != NULL) {
            J3DDifferedTexMtx::sTexGenBlock = mShape->getMaterial()->getTexGenBlock();
            J3DDifferedTexMtx::sTexMtxObj = (J3DShapePacket_0x24*)getTexMtxObj();
        } else {
            J3DDifferedTexMtx::sTexGenBlock = NULL;
        }

        mShape->drawFast();
    }
}

int J3DMatPacket::entry(J3DDrawBuffer* pBuffer) {
    J3DDrawBuffer::sortFunc func = J3DDrawBuffer::sortFuncTable[pBuffer->mSortType];
    return (pBuffer->*func)(this);
}

GDCurrentDL J3DDisplayListObj::sGDLObj;
int J3DDisplayListObj::sInterruptFlag;

