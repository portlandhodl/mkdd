#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DSkinDeform.h"

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

void J3DModel::calcNrmMtx()
{
    mMtxBuffer->calcNrmMtx();
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
