#include "JSystem/J3D/J3DMaterial.h"
#include "JSystem/J3D/J3DColorBlock.h"
#include "JSystem/J3D/J3DDisplayListObj.h"
#include "JSystem/J3D/J3DGD.h"
#include "JSystem/J3D/J3DInd.h"
#include "JSystem/J3D/J3DPacket.h"
#include "JSystem/J3D/J3DPE.h"
#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DTexGenBlock.h"
#include "macros.h"

J3DColorBlock* J3DMaterial::createColorBlock(u32 createFlag)
{
    J3DColorBlock* result = nullptr;
    switch (createFlag) {
    case 0:
        result = new J3DColorBlockLightOff;
        break;
    case J3DMLF_Material_Color_LightOn:
        result = new J3DColorBlockLightOn;
        break;
    case J3DMLF_Material_Color_AmbientOn:
        result = new J3DColorBlockAmbientOn;
        break;
    }
    return result;
}

J3DTexGenBlock* J3DMaterial::createTexGenBlock(u32 createFlag)
{
    J3DTexGenBlock* result = nullptr;
    switch (createFlag) {
    case J3DMLF_Material_TexGen_Block4:
        return new J3DTexGenBlock4();
    case 0:
    default:
        return new J3DTexGenBlockBasic();
    }
}

J3DTevBlock* J3DMaterial::createTevBlock(int stageCount)
{
    J3DTevBlock* block = nullptr;
    if (stageCount <= 1) {
        block = new J3DTevBlock1();
    } else if (stageCount == 2) {
        block = new J3DTevBlock2();
    } else if (stageCount <= 4) {
        block = new J3DTevBlock4();
    } else if (stageCount <= 16) {
        block = new J3DTevBlock16();
    }
    return block;
}

J3DIndBlock* J3DMaterial::createIndBlock(int stageCount)
{
    if (stageCount != 0) {
        return new J3DIndBlockFull();
    }
    return new J3DIndBlockNull();
}

J3DPEBlock* J3DMaterial::createPEBlock(u32 createFlag, u32 materialFlag)
{
    J3DPEBlock* rv = nullptr;
    if (createFlag == 0) {
        if (materialFlag & 1) {
            return new J3DPEBlockOpa();
        } else if (materialFlag & 2) {
            return new J3DPEBlockTexEdge();
        } else if (materialFlag & 4) {
            return new J3DPEBlockXlu();
        }
    }

    if (createFlag == J3DMLF_Material_PE_Full) {
        rv = new J3DPEBlockFull();
    } else if (createFlag == J3DMLF_Material_PE_FogOff) {
        rv = new J3DPEBlockFogOff();
    }
    return rv;
}

u32 J3DMaterial::calcSizeColorBlock(u32 createFlag)
{
    u32 size = 0;
    switch (createFlag) {
    case 0x0:
        size = sizeof(J3DColorBlockLightOff);
        break;
    case J3DMLF_Material_Color_LightOn:
        size = sizeof(J3DColorBlockLightOn);
        break;
    case J3DMLF_Material_Color_AmbientOn:
        size = sizeof(J3DColorBlockAmbientOn);
        break;
    }
    return size;
}

u32 J3DMaterial::calcSizeTexGenBlock(u32 createFlag)
{
    switch (createFlag) {
    case J3DMLF_Material_TexGen_Block4:
        return sizeof(J3DTexGenBlock4);
    case 0:
    default:
        return sizeof(J3DTexGenBlockBasic);
    }
}

u32 J3DMaterial::calcSizeTevBlock(int count)
{
    u32 result = 0;
    if (count <= 1) {
        result = sizeof(J3DTevBlock1);
    } else if (count == 2) {
        result = sizeof(J3DTevBlock2);
    } else if (count <= 4) {
        result = sizeof(J3DTevBlock4);
    } else if (count <= 16) {
        result = sizeof(J3DTevBlock16);
    }
    return result;
}

u32 J3DMaterial::calcSizeIndBlock(int count)
{
    if (count != 0) {
        return sizeof(J3DIndBlockFull);
    } else {
        return sizeof(J3DIndBlockNull);
    }
}

u32 J3DMaterial::calcSizePEBlock(u32 createFlag, u32 materialFlag)
{
    u32 size = 0;
    if (createFlag == 0) {
        if (materialFlag & 1) {
            size = sizeof(J3DPEBlockOpa);
        } else if (materialFlag & 2) {
            size = sizeof(J3DPEBlockTexEdge);
        } else if (materialFlag & 4) {
            size = sizeof(J3DPEBlockXlu);
        }
    } else if (createFlag == J3DMLF_Material_PE_Full) {
        size = sizeof(J3DPEBlockFull);
    } else if (createFlag == J3DMLF_Material_PE_FogOff) {
        size = sizeof(J3DPEBlockFogOff);
    }
    return size;
}

void J3DMaterial::initialize()
{
    mShape        = nullptr;
    mNext         = nullptr;
    mJoint        = nullptr;
    mDrawMode     = 1;
    mIndex        = -1;
    mInvalid      = 0;
    mMaterialID   = 0;
    mColorBlock   = nullptr;
    mTexGenBlock  = nullptr;
    mTevBlock     = nullptr;
    mIndBlock     = nullptr;
    mPeBlock      = nullptr;
    mOrigMaterial = nullptr;
    mAnm          = nullptr;
    mSharedDLObj  = nullptr;
}

u32 J3DMaterial::countDLSize()
{
    return ALIGN_NEXT(mColorBlock->countDLSize() + mTexGenBlock->countDLSize() + mTevBlock->countDLSize() + mIndBlock->countDLSize()
                          + mPeBlock->countDLSize(),
                      0x20);
}

DECL_WEAK u32 J3DTexGenBlock::countDLSize() { return 0; }

DECL_WEAK u32 J3DColorBlock::countDLSize() { return 0; }

DECL_WEAK u32 J3DTevBlock::countDLSize() { return 0; }

DECL_WEAK u32 J3DIndBlock::countDLSize() { return 0; }

DECL_WEAK u32 J3DPEBlock::countDLSize() { return 0; }

DECL_WEAK void J3DColorBlock::load() { }

DECL_WEAK u8 J3DColorBlock::getCullMode() const { return 2; }

DECL_WEAK u8 J3DIndBlock::getIndTexStageNum() const { return 0; }

DECL_WEAK u8 J3DColorBlock::getColorChanNum() const { return 0; }

DECL_WEAK void J3DTevBlock::load() { }

void J3DMaterial::makeDisplayList_private(J3DDisplayListObj* obj)
{
    obj->beginDL();
    mTevBlock->load();
    mIndBlock->load();
    mPeBlock->load();
    J3DGDSetGenMode(mTexGenBlock->getTexGenNum(), mColorBlock->getColorChanNum(), mTevBlock->getTevStageNum(),
                    mIndBlock->getIndTexStageNum(), (GXCullMode)mColorBlock->getCullMode());
    mTexGenBlock->load();
    mColorBlock->load();
    J3DGDSetNumChans(mColorBlock->getColorChanNum());
    J3DGDSetNumTexGens(mTexGenBlock->getTexGenNum());
    obj->endDL();
}

void J3DMaterial::makeDisplayList()
{
    if (!j3dSys.mMatPacket->checkFlag(J3DDrawPacket::J3DDP_IsLocked)) {
        j3dSys.mMatPacket->mDiffFlag = mMaterialID;
        makeDisplayList_private(j3dSys.mMatPacket->mDisplayList);
    }
}

void J3DMaterial::makeSharedDisplayList()
{
    makeDisplayList_private(mSharedDLObj);
}

void J3DMaterial::load()
{
    j3dSys._54 = mDrawMode;
    if (!j3dSys.checkFlag(2)) {
        loadNBTScale(*mTexGenBlock->getNBTScale());
    }
}

DECL_WEAK J3DNBTScale* J3DTexGenBlock::getNBTScale() { return nullptr; }

void J3DMaterial::loadSharedDL()
{
    j3dSys._54 = mDrawMode;
    if (!j3dSys.checkFlag(2)) {
        mSharedDLObj->callDL();
        loadNBTScale(*mTexGenBlock->getNBTScale());
    }
}

void J3DMaterial::patch()
{
    j3dSys.mMatPacket->mDiffFlag = mMaterialID;
    j3dSys.mMatPacket->mDisplayList->beginPatch();
    mTevBlock->patch();
    mColorBlock->patch();
    mTexGenBlock->patch();
    j3dSys.mMatPacket->mDisplayList->endPatch();
}

DECL_WEAK void J3DColorBlock::patch() { }

void J3DMaterial::diff(u32 p1)
{
    if (j3dSys.mMatPacket->mInitShapePacket->mDisplayList != nullptr) {
        j3dSys.mMatPacket->beginDiff();
        mTevBlock->diff(p1);
        mIndBlock->diff(p1);
        mPeBlock->diff(p1);
        if (p1 & J3DMDF_DiffKonstColor) {
            J3DGDSetGenMode_3Param(mTexGenBlock->getTexGenNum(), mTevBlock->getTevStageNum(), mIndBlock->getIndTexStageNum());
            J3DGDSetNumTexGens(mTexGenBlock->getTexGenNum());
        }
        mTexGenBlock->diff(p1);
        mColorBlock->diff(p1);
        j3dSys.mMatPacket->endDiff();
    }
}

DECL_WEAK void J3DColorBlock::diff(u32) { }

DECL_WEAK void J3DPEBlock::diff(u32) { }

void J3DMaterial::calc(const Mtx mtx)
{
    if (j3dSys.checkFlag(0x40000000)) {
        mTexGenBlock->calcPostTexMtx(mtx);
    } else {
        mTexGenBlock->calc(mtx);
    }

    calcCurrentMtx();
    setCurrentMtx();
}

void J3DMaterial::calcDiffTexMtx(const Mtx mtx)
{
    if (j3dSys.checkFlag(0x40000000)) {
        mTexGenBlock->calcPostTexMtxWithoutViewMtx(mtx);
    } else {
        mTexGenBlock->calcWithoutViewMtx(mtx);
    }
}

void J3DMaterial::setCurrentMtx()
{
    mShape->mCurrentMtx = mCurrentMtx;
}

void J3DMaterial::calcCurrentMtx()
{
    J3DCurrentMtx* mtx = &mCurrentMtx;
    if (!j3dSys.checkFlag(0x40000000)) {
        mtx->setCurrentTexMtx(mTexGenBlock->getTexCoord(0)->getTexGenMtx(), mTexGenBlock->getTexCoord(1)->getTexGenMtx(),
                                     mTexGenBlock->getTexCoord(2)->getTexGenMtx(), mTexGenBlock->getTexCoord(3)->getTexGenMtx(),
                                     mTexGenBlock->getTexCoord(4)->getTexGenMtx(), mTexGenBlock->getTexCoord(5)->getTexGenMtx(),
                                     mTexGenBlock->getTexCoord(6)->getTexGenMtx(), mTexGenBlock->getTexCoord(7)->getTexGenMtx());
    } else {
        mtx->setCurrentTexMtx(mTexGenBlock->getTexCoord(0)->getTexMtxReg(), mTexGenBlock->getTexCoord(1)->getTexMtxReg(),
                                     mTexGenBlock->getTexCoord(2)->getTexMtxReg(), mTexGenBlock->getTexCoord(3)->getTexMtxReg(),
                                     mTexGenBlock->getTexCoord(4)->getTexMtxReg(), mTexGenBlock->getTexCoord(5)->getTexMtxReg(),
                                     mTexGenBlock->getTexCoord(6)->getTexMtxReg(), mTexGenBlock->getTexCoord(7)->getTexMtxReg());
    }
}

DECL_WEAK J3DTexCoord* J3DTexGenBlock::getTexCoord(u32) { return nullptr; }

DECL_WEAK void J3DPEBlock::reset(J3DPEBlock*) { }

DECL_WEAK void J3DIndBlock::reset(J3DIndBlock*) { }

DECL_WEAK void J3DTevBlock::reset(J3DTevBlock*) { }

DECL_WEAK void J3DTexGenBlock::reset(J3DTexGenBlock*) { }

DECL_WEAK void J3DColorBlock::reset(J3DColorBlock*) { }

void J3DMaterial::copy(J3DMaterial* other)
{
    mColorBlock->reset(other->mColorBlock);
    mTexGenBlock->reset(other->mTexGenBlock);
    mTevBlock->reset(other->mTevBlock);
    mIndBlock->reset(other->mIndBlock);
    mPeBlock->reset(other->mPeBlock);
}

void J3DMaterial::reset()
{
    if ((~mMaterialID & 0x80000000) == 0) {
        mMaterialID &= ~0x80000000;
        mDrawMode = mOrigMaterial->mDrawMode;
        mInvalid  = mOrigMaterial->mInvalid;
        mAnm      = nullptr;
        J3DMaterial* orig = mOrigMaterial;
        mColorBlock->reset(orig->mColorBlock);
        mTexGenBlock->reset(orig->mTexGenBlock);
        mTevBlock->reset(orig->mTevBlock);
        mIndBlock->reset(orig->mIndBlock);
        mPeBlock->reset(orig->mPeBlock);
    }
}

void J3DMaterial::change()
{
    if ((mMaterialID & 0xc0000000) == 0) {
        mMaterialID |= 0x80000000;
        mAnm = nullptr;
    }
}

J3DErrType J3DMaterial::newSharedDisplayList(u32 p1)
{
    if (mSharedDLObj == nullptr) {
        mSharedDLObj = new J3DDisplayListObj;
        if (mSharedDLObj == nullptr) {
            return JET_OutOfMemory;
        }
        J3DErrType res = mSharedDLObj->newDisplayList(p1);
        switch (res) {
        case JET_Success:
            break;
        default:
            return res;
        }
    }
    return JET_Success;
}

J3DErrType J3DMaterial::newSingleSharedDisplayList(u32 p1)
{
    if (mSharedDLObj == nullptr) {
        mSharedDLObj = new J3DDisplayListObj;
        if (mSharedDLObj == nullptr) {
            return JET_OutOfMemory;
        }
        J3DErrType res = mSharedDLObj->newSingleDisplayList(p1);
        switch (res) {
        case JET_Success:
            break;
        default:
            return res;
        }
    }
    return JET_Success;
}

void J3DPatchedMaterial::initialize()
{
    J3DMaterial::initialize();
}

void J3DPatchedMaterial::makeDisplayList() { }

void J3DPatchedMaterial::makeSharedDisplayList() { }

void J3DPatchedMaterial::load()
{
    j3dSys._54 = mDrawMode;
    if (j3dSys.checkFlag(2)) {
        return;
    }
}

void J3DPatchedMaterial::loadSharedDL()
{
    j3dSys._54 = mDrawMode;
    if (!j3dSys.checkFlag(2)) {
        mSharedDLObj->callDL();
    }
}

void J3DPatchedMaterial::reset() { }

void J3DPatchedMaterial::change() { }

void J3DLockedMaterial::initialize()
{
    J3DMaterial::initialize();
}

void J3DLockedMaterial::makeDisplayList() { }

void J3DLockedMaterial::makeSharedDisplayList() { }

void J3DLockedMaterial::load()
{
    j3dSys._54 = mDrawMode;
    if (j3dSys.checkFlag(2)) {
        return;
    }
}

void J3DLockedMaterial::loadSharedDL()
{
    j3dSys._54 = mDrawMode;
    if (!j3dSys.checkFlag(2)) {
        mSharedDLObj->callDL();
    }
}

void J3DLockedMaterial::patch() { }

void J3DLockedMaterial::diff(u32) { }

void J3DLockedMaterial::calc(const Mtx) { }

void J3DLockedMaterial::reset() { }

void J3DLockedMaterial::change() { }


DECL_WEAK void J3DPEBlock::patch() { }

DECL_WEAK void J3DPEBlock::diffFog() { }

DECL_WEAK void J3DPEBlock::diffBlend() { }

DECL_WEAK void J3DPEBlock::setFog(J3DFog*) { }

DECL_WEAK void J3DPEBlock::setFog(J3DFog) { }

DECL_WEAK J3DFog* J3DPEBlock::getFog() { return nullptr; }

DECL_WEAK void J3DPEBlock::setAlphaComp(const J3DAlphaComp&) { }

DECL_WEAK void J3DPEBlock::setAlphaComp(const J3DAlphaComp*) { }

DECL_WEAK J3DAlphaComp* J3DPEBlock::getAlphaComp() { return nullptr; }

DECL_WEAK void J3DPEBlock::setBlend(const J3DBlend&) { }

DECL_WEAK void J3DPEBlock::setBlend(const J3DBlend*) { }

DECL_WEAK J3DBlend* J3DPEBlock::getBlend() { return nullptr; }

DECL_WEAK void J3DPEBlock::setZMode(J3DZMode) { }

DECL_WEAK void J3DPEBlock::setZMode(const J3DZMode*) { }

DECL_WEAK J3DZMode* J3DPEBlock::getZMode() { return nullptr; }

DECL_WEAK void J3DPEBlock::setZCompLoc(u8) { }

DECL_WEAK void J3DPEBlock::setZCompLoc(const u8*) { }

DECL_WEAK u8 J3DPEBlock::getZCompLoc() const { return 0; }

DECL_WEAK void J3DPEBlock::setDither(u8) { }

DECL_WEAK void J3DPEBlock::setDither(const u8*) { }

DECL_WEAK u8 J3DPEBlock::getDither() const { return 0; }

DECL_WEAK u32 J3DPEBlock::getFogOffset() const { return 0; }

DECL_WEAK void J3DPEBlock::setFogOffset(u32) { }

DECL_WEAK void J3DIndBlock::setIndTexStageNum(u8) { }

DECL_WEAK void J3DIndBlock::setIndTexOrder(u32, const J3DIndTexOrder*) { }

DECL_WEAK void J3DIndBlock::setIndTexOrder(u32, J3DIndTexOrder) { }

DECL_WEAK J3DIndTexOrder* J3DIndBlock::getIndTexOrder(u32) { return nullptr; }

DECL_WEAK void J3DIndBlock::setIndTexMtx(u32, J3DIndTexMtx) { }

DECL_WEAK void J3DIndBlock::setIndTexMtx(u32, const J3DIndTexMtx*) { }

DECL_WEAK J3DIndTexMtx* J3DIndBlock::getIndTexMtx(u32) { return nullptr; }

DECL_WEAK void J3DIndBlock::setIndTexCoordScale(u32, J3DIndTexCoordScale) { }

DECL_WEAK void J3DIndBlock::setIndTexCoordScale(u32, const J3DIndTexCoordScale*) { }

DECL_WEAK J3DIndTexCoordScale* J3DIndBlock::getIndTexCoordScale(u32) { return nullptr; }

DECL_WEAK void J3DTexGenBlock::setTexGenNum(u32) { }

DECL_WEAK void J3DTexGenBlock::setTexGenNum(const u32*) { }

DECL_WEAK void J3DTexGenBlock::setTexCoord(u32, const J3DTexCoord*) { }

DECL_WEAK void J3DTexGenBlock::setTexMtx(u32, J3DTexMtx*) { }

DECL_WEAK J3DTexMtx* J3DTexGenBlock::getTexMtx(u32) { return nullptr; }

DECL_WEAK void J3DTexGenBlock::setNBTScale(J3DNBTScale) { }

DECL_WEAK void J3DTexGenBlock::setNBTScale(const J3DNBTScale*) { }

DECL_WEAK u32 J3DTexGenBlock::getTexMtxOffset() const { return 0; }

DECL_WEAK void J3DTexGenBlock::setTexMtxOffset(u32) { }

DECL_WEAK void J3DColorBlock::patchMatColor() { }

DECL_WEAK void J3DColorBlock::patchLight() { }

DECL_WEAK void J3DColorBlock::diffMatColor() { }

DECL_WEAK void J3DColorBlock::diffLight() { }

DECL_WEAK void J3DColorBlock::setMatColor(u32, J3DGXColor) { }

DECL_WEAK void J3DColorBlock::setMatColor(u32, const J3DGXColor*) { }

DECL_WEAK J3DGXColor* J3DColorBlock::getMatColor(u32) { return nullptr; }

DECL_WEAK void J3DColorBlock::setAmbColor(u32, J3DGXColor) { }

DECL_WEAK void J3DColorBlock::setAmbColor(u32, const J3DGXColor*) { }

DECL_WEAK J3DGXColor* J3DColorBlock::getAmbColor(u32) { return nullptr; }

DECL_WEAK void J3DColorBlock::setColorChanNum(const u8*) { }

DECL_WEAK void J3DColorBlock::setColorChanNum(u8) { }

DECL_WEAK void J3DColorBlock::setColorChan(u32, const J3DColorChan*) { }

DECL_WEAK void J3DColorBlock::setColorChan(u32, const J3DColorChan&) { }

DECL_WEAK J3DColorChan* J3DColorBlock::getColorChan(u32) { return nullptr; }

DECL_WEAK void J3DColorBlock::setLight(u32, J3DLightObj*) { }

DECL_WEAK J3DLightObj* J3DColorBlock::getLight(u32) { return nullptr; }

DECL_WEAK void J3DColorBlock::setCullMode(u8) { }

DECL_WEAK void J3DColorBlock::setCullMode(const u8*) { }

DECL_WEAK u32 J3DColorBlock::getMatColorOffset() const { return 0; }

DECL_WEAK u32 J3DColorBlock::getColorChanOffset() const { return 0; }

DECL_WEAK void J3DColorBlock::setMatColorOffset(u32) { }

DECL_WEAK void J3DColorBlock::setColorChanOffset(u32) { }
