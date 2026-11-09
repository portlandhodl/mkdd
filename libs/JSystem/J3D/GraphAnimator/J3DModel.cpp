#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DSkinDeform.h"
#include "JSystem/J3D/J3DTexMtx.h"
#include "JSystem/J3D/J3DTransform.h"
#include "JSystem/J3D/J3DVtxColorCalc.h"

struct J3DUnkCalc1 {
    virtual void calc(J3DModel *);
};

struct J3DUnkCalc2 {
    virtual void unk();
    virtual void calc(J3DModelData *);
};

void J3DCalcViewBaseMtx(Mtx, const Vec &, const Mtx &, Mtx);

void J3DModel::initialize()
{
    mModelData = NULL;
    mFlags = 0;
    mDiffFlag = 0;
    mCalcCallBack = NULL;
    mUserArea = 0;
    mModelScale.x = 1.0f;
    mModelScale.y = 1.0f;
    mModelScale.z = 1.0f;
    PSMTXIdentity(mPosMtx);
    PSMTXIdentity(mInternalView);
    mMtxBuffer = NULL;
    mMatPackets = NULL;
    mShapePackets = NULL;
    mDeformData = NULL;
    mSkinDeform = NULL;
    mVtxColorCalc = NULL;
    _D4 = 0;
    _D8 = NULL;
}

int J3DModel::entryModelData(J3DModelData *data, u32 matFlags, u32 viewNum)
{
    mModelData = data;
    mMtxBuffer = new J3DMtxBuffer;
    int result = mMtxBuffer->create(data, viewNum);
    if (result) {
        return result;
    }
    result = createShapePacket(data);
    if (result) {
        return result;
    }
    result = createMatPacket(data, matFlags);
    if (result) {
        return result;
    }
    mVertexBuffer.setVertexData(&data->mVertexData);
    prepareShapePackets();
    if (matFlags & J3DMODEL_Unk1) {
        onFlag(J3DMODEL_Unk1);
    }
    if (matFlags & J3DMODEL_UseDefaultJ3D) {
        onFlag(J3DMODEL_UseDefaultJ3D);
    }
    if (matFlags & J3DMODEL_LevelOfDetail) {
        onFlag(J3DMODEL_LevelOfDetail);
    }
    if (matFlags & J3DMODEL_CreateNewDL) {
        onFlag(J3DMODEL_CreateNewDL);
    }
    if (matFlags & J3DMODEL_UseSingleSharedDL) {
        onFlag(J3DMODEL_UseSingleSharedDL);
    }
    if (matFlags & J3DMODEL_ShareDL) {
        onFlag(J3DMODEL_ShareDL);
    }
    if (matFlags & J3DMODEL_SkinPosCpu) {
        onFlag(J3DMODEL_SkinPosCpu);
    }
    if (matFlags & J3DMODEL_SkinNrmCpu) {
        onFlag(J3DMODEL_SkinNrmCpu);
    }
    return 0;
}

int J3DModel::createShapePacket(J3DModelData *data)
{
    if (data->mShapeTable.mCount) {
        mShapePackets = new J3DShapePacket[data->mShapeTable.mCount];
        for (int i = 0; i < data->mShapeTable.mCount; i++) {
            mShapePackets[i].setShape(data->getShapeNodePointer(i));
            mShapePackets[i].setModel(this);
        }
    }
    return 0;
}

int J3DModel::createMatPacket(J3DModelData *data, u32 flags)
{
    if (data->mMaterialTable.mMaterialNum != 0) {
        mMatPackets = new J3DMatPacket[data->mMaterialTable.mMaterialNum];
    }

    u32 count = data->getMaterialNum();
    for (u16 i = 0; i < count; i++) {
        J3DMaterial *material = data->getMaterialNodePointer(i);
        J3DMatPacket *matPacket = &mMatPackets[i];
        J3DShapePacket *shapePacket = getShapePacket(material->getShape()->getIndex());
        matPacket->mMaterial = material;
        matPacket->mInitShapePacket = shapePacket;
        matPacket->addShapePacket(shapePacket);
        matPacket->mTexture = data->getTexture();
        matPacket->mDiffFlag = material->mMaterialID;
        if (data->getJointTree().getModelDataType() == 1) {
            matPacket->lock();
        }

        if (flags & J3DMODEL_ShareDL) {
            matPacket->mDisplayList = material->getSharedDisplayListObj();
        } else if (data->getJointTree().getModelDataType() == 1) {
            if (flags & J3DMODEL_UseSingleSharedDL) {
                matPacket->mDisplayList = material->getSharedDisplayListObj();
            } else {
                J3DDisplayListObj *displayList = material->getSharedDisplayListObj();
                int result = displayList->single_To_Double();
                if (result) {
                    return result;
                }
                matPacket->mDisplayList = displayList;
            }
        } else if (flags & J3DMODEL_CreateNewDL) {
            if (flags & J3DMODEL_UseSingleSharedDL) {
                material->newSingleSharedDisplayList(material->countDLSize());
                matPacket->mDisplayList = material->getSharedDisplayListObj();
            } else {
                material->newSharedDisplayList(material->countDLSize());
                J3DDisplayListObj *displayList = material->getSharedDisplayListObj();
                displayList->single_To_Double();
                matPacket->mDisplayList = displayList;
            }
        } else {
            if (flags & J3DMODEL_UseSingleSharedDL) {
                matPacket->newSingleDisplayList(material->countDLSize());
            } else {
                matPacket->newDisplayList(material->countDLSize());
            }
        }
    }
    return 0;
}

int J3DModel::newDifferedDisplayList(u32 displayListFlag)
{
    mDiffFlag = displayListFlag;
    u16 count = mModelData->getShapeNum();
    for (u16 i = 0; i < count; i++) {
        int result = mShapePackets[i].newDifferedDisplayList(displayListFlag);
        if (result) {
            return result;
        }
    }
    return 0;
}

int J3DModel::newDifferedTexMtx(J3DTexDiffFlag texDiffFlag)
{
    u16 count = mModelData->getShapeNum();
    for (u16 i = 0; i < count; i++) {
        int result = mShapePackets[i].newDifferedTexMtx(texDiffFlag);
        if (result) {
            return result;
        }
    }
    return 0;
}

void J3DModel::lock()
{
    int matNum = mModelData->mMaterialTable.mMaterialNum;
    for (int i = 0; i < matNum; i++) {
        mMatPackets[i].lock();
    }
}

void J3DModel::makeDL()
{
    j3dSys._38 = this;
    j3dSys._58 = getModelData()->getTexture();
    u32 count = getModelData()->mMaterialTable.mMaterialNum;
    for (u16 i = 0; i < count; i++) {
        j3dSys.mMatPacket = getMatPacket(i);
        mModelData->getMaterialNodePointer(i)->makeDisplayList();
    }
}

void J3DModel::calcMaterial()
{
    j3dSys._38 = this;
    if (mFlags & J3DMODEL_SkinPosCpu) {
        j3dSys._34 |= 0x4;
    } else {
        j3dSys._34 &= ~0x4;
    }
    if (mFlags & J3DMODEL_SkinNrmCpu) {
        j3dSys._34 |= 0x8;
    } else {
        j3dSys._34 &= ~0x8;
    }
    getModelData()->syncJ3DSysFlags();
    j3dSys._58 = getModelData()->getTexture();
    u16 matNum = getModelData()->mMaterialTable.mMaterialNum;
    for (u16 i = 0; i < matNum; i++) {
        j3dSys.mMatPacket = getMatPacket(i);
        J3DMaterial *material = getModelData()->getMaterialNodePointer(i);
        if (material->getMaterialAnm() != NULL) {
            material->getMaterialAnm()->calc(material);
        }
        material->calc(getAnmMtx(material->getJoint()->getJntNo()));
    }
}

void J3DModel::calcDiffTexMtx()
{
    u16 i;
    j3dSys._38 = this;
    u32 count = getModelData()->getMaterialTable().getMaterialNum();
    for (i = 0; i < count; i++) {
        j3dSys.mMatPacket = getMatPacket(i);
        J3DMaterial *material = getModelData()->getMaterialNodePointer(i);
        material->calcDiffTexMtx(getAnmMtx(material->getJoint()->getJntNo()));
    }

    count = mModelData->getShapeNum();
    for (i = 0; i < count; i++) {
        J3DShapePacket *shapePacket = getShapePacket(i);
        J3DTexGenBlock *texGenBlock = mModelData->getShapeNodePointer(i)->getMaterial()->getTexGenBlock();
        for (u16 j = 0; (int)j < 8; j++) {
            J3DTexMtx *texMtxNode = texGenBlock->getTexMtx(j);
            J3DTexMtxObj *texMtxObj = shapePacket->getTexMtxObj();
            if (texMtxNode != NULL && texMtxObj != NULL) {
                PSMTXCopy(texMtxNode->getMtx(), texMtxObj->mTexMtx[j]);
            }
        }
    }
}

void J3DModel::diff()
{
    u16 count = getModelData()->getMaterialTable().mMaterialNum;
    for (u16 i = 0; i < count; i++) {
        j3dSys.mMatPacket = getMatPacket(i);
        mModelData->getMaterialNodePointer(i)->diff(mDiffFlag);
    }
}

void J3DModel::setDeformData(J3DDeformData *deformData, u32 flags)
{
    mDeformData = deformData;
    if (deformData != NULL) {
        mVertexBuffer.copyLocalVtxArray(flags);
        if (flags & 4) {
            deformData->offAllFlag(1);
        }
    }
}

void J3DModel::update()
{
    calc();
    entry();
}

void J3DModel::calc()
{
    j3dSys._38 = this;
    if (mFlags & J3DMODEL_SkinPosCpu) {
        j3dSys._34 |= 0x4;
    } else {
        j3dSys._34 &= ~0x4;
    }
    if (mFlags & J3DMODEL_SkinNrmCpu) {
        j3dSys._34 |= 0x8;
    } else {
        j3dSys._34 &= ~0x8;
    }

    mModelData->syncJ3DSysFlags();
    mVertexBuffer.frameInit();
    if (_D8 != NULL) {
        ((J3DUnkCalc2 *)_D8)->calc(mModelData);
    }
    if (mDeformData != NULL) {
        mDeformData->deform(this);
    }
    if (mVtxColorCalc != NULL) {
        mVtxColorCalc->calc(this);
    }
    if (_D4 != 0) {
        ((J3DUnkCalc1 *)_D4)->calc(this);
    }

    j3dSys._38 = this;
    if (checkFlag(J3DMODEL_UseDefaultJ3D)) {
        getModelData()->getJointTree().calc(mMtxBuffer, j3dDefaultScale, j3dDefaultMtx);
    } else {
        getModelData()->getJointTree().calc(mMtxBuffer, mModelScale, mPosMtx);
    }

    if (getModelData()->getWEvlpMtxNum() != 0 && !checkFlag(J3DMODEL_LevelOfDetail)) {
        if (!getModelData()->checkFlag(0x100)) {
            mMtxBuffer->calcWeightEnvelopeMtx();
        }
    }
    if (mSkinDeform != NULL) {
        mSkinDeform->deform(this);
    }
    if (mCalcCallBack != NULL) {
        mCalcCallBack(this, 0);
    }
}

void J3DModel::entry()
{
    j3dSys._38 = this;
    if (mFlags & J3DMODEL_SkinPosCpu) {
        j3dSys._34 |= 0x4;
    } else {
        j3dSys._34 &= ~0x4;
    }
    if (mFlags & J3DMODEL_SkinNrmCpu) {
        j3dSys._34 |= 0x8;
    } else {
        j3dSys._34 &= ~0x8;
    }
    mModelData->syncJ3DSysFlags();
    j3dSys._58 = mModelData->getMaterialTable().getTexture();
    for (u16 i = 0; i < mModelData->getJointNum(); i++) {
        J3DJoint *joint = mModelData->getJointNodePointer(i);
        if (joint->mMaterial) {
            joint->entryIn();
        }
    }
}

void J3DModel::viewCalc()
{
    mMtxBuffer->swapDrawMtx();
    mMtxBuffer->swapNrmMtx();

    if (getModelData()->checkFlag(0x10)) {
        if ((mFlags & (J3DMODEL_Unk1 | J3DMODEL_UseDefaultJ3D)) == 2) {
            J3DCalcViewBaseMtx(j3dSys.mViewMtx, mModelScale, mPosMtx, mInternalView);
        }
    } else if (isCpuSkinningOn()) {
        if ((mFlags & (J3DMODEL_Unk1 | J3DMODEL_UseDefaultJ3D)) == 2) {
            J3DCalcViewBaseMtx(j3dSys.mViewMtx, mModelScale, mPosMtx, mInternalView);
        }
    } else if (checkFlag(J3DMODEL_SkinPosCpu)) {
        mMtxBuffer->calcDrawMtx(mFlags & (J3DMODEL_Unk1 | J3DMODEL_UseDefaultJ3D), mModelScale, mPosMtx);
        calcNrmMtx();
        calcBumpMtx();
        DCStoreRangeNoSync(getDrawMtxPtr(), mModelData->getDrawMtxNum() * sizeof(Mtx));
        DCStoreRange(getNrmMtxPtr(), mModelData->getDrawMtxNum() * sizeof(Mtx33));
    } else if (checkFlag(J3DMODEL_SkinNrmCpu)) {
        mMtxBuffer->calcDrawMtx(mFlags & (J3DMODEL_Unk1 | J3DMODEL_UseDefaultJ3D), mModelScale, mPosMtx);
        calcBBoardMtx();
        DCStoreRange(getDrawMtxPtr(), mModelData->getDrawMtxNum() * sizeof(Mtx));
    } else {
        mMtxBuffer->calcDrawMtx(mFlags & (J3DMODEL_Unk1 | J3DMODEL_UseDefaultJ3D), mModelScale, mPosMtx);
        calcNrmMtx();
        calcBBoardMtx();
        calcBumpMtx();
        DCStoreRangeNoSync(getDrawMtxPtr(), mModelData->getDrawMtxNum() * sizeof(Mtx));
        DCStoreRange(getNrmMtxPtr(), mModelData->getDrawMtxNum() * sizeof(Mtx33));
    }

    prepareShapePackets();
}

void J3DModel::calcNrmMtx()
{
    mMtxBuffer->calcNrmMtx();
}

void J3DModel::calcBumpMtx()
{
    int bumpMtxIdx;
    u32 materialNum;
    if (getModelData()->checkBumpFlag() != 1) {
        return;
    }

    bumpMtxIdx = 0;
    materialNum = getModelData()->getMaterialNum();
    for (u16 i = 0; i < materialNum; i++) {
        J3DMaterial *material = getModelData()->getMaterialNodePointer(i);
        if (material->getNBTScale()->mHasScale == TRUE) {
            material->getShape()->calcNBTScale(*material->getNBTScale()->getScale(), getNrmMtxPtr(), getBumpMtxPtr(bumpMtxIdx));
            DCStoreRange(getBumpMtxPtr(bumpMtxIdx), mModelData->getDrawMtxNum() * sizeof(Mtx33));
            bumpMtxIdx++;
        }
    }
}

void J3DModel::calcBBoardMtx()
{
    if (mModelData->checkBBoardFlag()) {
        mMtxBuffer->calcBBoardMtx();
    }
}

void J3DModel::prepareShapePackets()
{
    u32 shapeNum = getModelData()->getShapeNum();
    for (u16 i = 0; i < shapeNum; i++) {
        J3DShape *shapeNode = mModelData->getShapeNodePointer(i);
        J3DShapePacket *shapePacket = getShapePacket(i);
        shapePacket->setMtxBuffer(mMtxBuffer);
        if ((mFlags & (J3DMODEL_Unk1 | J3DMODEL_UseDefaultJ3D)) == 2) {
            shapePacket->setBaseMtxPtr(&mInternalView);
        } else {
            shapePacket->setBaseMtxPtr(&j3dSys.mViewMtx);
        }
    }
}
