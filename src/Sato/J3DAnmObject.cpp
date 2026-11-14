#include "JSystem/J3D/J3DAnmBase.h"
#include "JSystem/J3D/J3DAnmCluster.h"
#include "JSystem/J3D/J3DAnmLoader.h"   // Required to compile.
#include "JSystem/J3D/J3DAnmTevRegKey.h"
#include "JSystem/J3D/J3DClusterLoader.h"
#include "JSystem/J3D/J3DFrameCtrl.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DModelLoader.h"
#include "types.h"
#include "Sato/J3DAnmObject.h"
#include "JSystem/JAudio/JASFakeMatch.h"
#include "JSystem/JAudio/JASFakeMatch2.h"


J3DModelData *J3DAnmObjBase::loadJ3DModelData(void *param_1, u32 param_2) {
    // FIX: Identify what the bitmask here is really used for.
    return (J3DModelData *)J3DModelLoaderDataBase().load(param_1, param_2 | 0x19200010);
}

void J3DAnmObjBase::initFrameCtrl(J3DAnmBase *initAnmBase) {
    mFrameCtrl.mAttr = initAnmBase->getAttribute();
    mFrameCtrl.mEndFrame = initAnmBase->getFrameMax();
}

void J3DAnmObjTrans::loadTransAnm(J3DAnmTransform **anmTransform, void *param_2) {
    *anmTransform = (J3DAnmTransform *)J3DAnmLoaderDataBase().load(param_2, J3DLOADER_UNK_FLAG0);
}

void J3DAnmObjTrans::setupTransAnmData(J3DAnmTransform **anmTransform,
                                       J3DMtxCalc **mtxCalc,
                                       J3DModelData *modelData,
                                       void *param_4
) {    
    *anmTransform = (J3DAnmTransform *)J3DAnmLoaderDataBase().load(param_4, J3DLOADER_UNK_FLAG0);
    *mtxCalc = J3DNewMtxCalcAnm(modelData->getMtxCalcType() & 0xf, *anmTransform);
    return;
}

void J3DAnmObjTrans::attach(J3DAnmTransform *anmBase, J3DMtxCalc *mtxCalc) {
    mAnmBase = anmBase;
    mCalc = mtxCalc;
    mFrameCtrl.mAttr = anmBase->getAttribute();
    mFrameCtrl.mEndFrame = anmBase->getFrameMax();
}

void J3DAnmObjTrans::attachBlend(J3DAnmTransform *anmBase, J3DMtxCalc *mtxCalc) {
    _28 |= 1;
    mAnmBase = anmBase;
    mCalc = mtxCalc;
    mFrameCtrl.mAttr = anmBase->getAttribute();
    mFrameCtrl.mEndFrame = anmBase->getFrameMax();
}

void J3DAnmObjTrans::anmFrameProc() {
    mAnmBase->mCurrentFrame = mFrameCtrl.getFrame();
}

void J3DAnmObjMaterial::setupColorAnmData(J3DAnmColor **anm, J3DModelData *modelData, void *data) {
    *anm = (J3DAnmColor *)J3DAnmLoaderDataBase::load(data, J3DLOADER_UNK_FLAG0);
    (*anm)->searchUpdateMaterialID(modelData);
    u16 num = (*anm)->getUpdateMaterialNum();
    for (u16 i = 0; i < num; i++) {
        setMaterialAnm(modelData, (*anm)->getUpdateMaterialID(i));
    }
    modelData->getMaterialTable().entryMatColorAnimator(*anm);
}

void J3DAnmObjMaterial::setupTexSRTAnmData(J3DAnmTextureSRTKey **anm, J3DModelData *modelData, void *data) {
    *anm = (J3DAnmTextureSRTKey *)J3DAnmLoaderDataBase::load(data, J3DLOADER_UNK_FLAG0);
    (*anm)->searchUpdateMaterialID(modelData);
    u16 num = (*anm)->getUpdateMaterialNum();
    for (u16 i = 0; i < num; i++) {
        setMaterialAnm(modelData, (*anm)->getUpdateMaterialID(i));
    }
    modelData->getMaterialTable().entryTexMtxAnimator(*anm);
}

void J3DAnmObjMaterial::setupTexPatternAnmData(J3DAnmTexPattern **anm, J3DModelData *modelData, void *data) {
    *anm = (J3DAnmTexPattern *)J3DAnmLoaderDataBase::load(data, J3DLOADER_UNK_FLAG0);
    (*anm)->searchUpdateMaterialID(modelData);
    u16 num = (*anm)->mUpdateMaterialNum;
    for (u16 i = 0; i < num; i++) {
        setMaterialAnm(modelData, (*anm)->mUpdateMaterialID[i]);
    }
    modelData->getMaterialTable().entryTexNoAnimator(*anm);
}

void J3DAnmObjMaterial::setupTevRegAnmData(J3DAnmTevRegKey **anm, J3DModelData *modelData, void *data) {
    *anm = (J3DAnmTevRegKey *)J3DAnmLoaderDataBase::load(data, J3DLOADER_UNK_FLAG0);
    (*anm)->searchUpdateMaterialID(modelData);
    setMaterialAnmTev(anm, modelData);
    modelData->getMaterialTable().entryTevRegAnimator(*anm);
}

template<> void J3DAnmObjMaterial::setMaterialAnmTev<J3DAnmTevRegKey>(J3DAnmTevRegKey **anm, J3DModelData *modelData) {
    u16 i;
    u16 num = (*anm)->getCRegUpdateMaterialNum();
    for (i = 0; i < num; i++) {
        setMaterialAnm(modelData, (*anm)->getCRegUpdateMaterialID(i));
    }
    num = (*anm)->getKRegUpdateMaterialNum();
    for (i = 0; i < num; i++) {
        setMaterialAnm(modelData, (*anm)->getKRegUpdateMaterialID(i));
    }
}

void J3DAnmObjCluster::loadClusterData(J3DDeformData **deformData, void *data) {
    *deformData = (J3DDeformData *)J3DClusterLoaderDataBase::load(data);
}

void J3DAnmObjCluster::loadClusterAnmData(J3DAnmCluster **anmCluster, void *data) {
    *anmCluster = (J3DAnmCluster *)J3DAnmLoaderDataBase::load(data, J3DLOADER_UNK_FLAG0);
}

void J3DAnmObjCluster::setDeformData(ExModel *model, J3DDeformData *deformData, bool p3) {
    u32 flag = 1;
    if (p3)
        flag = 0;
    model->getModel()->setDeformData(deformData, flag);
}

void J3DAnmObjCluster::attach(J3DAnmCluster *anmCluster) {
    mAnmBase = anmCluster;
    J3DAnmBase *anmBase = mAnmBase;
    mFrameCtrl.mAttr = anmBase->getAttribute();
    mFrameCtrl.mEndFrame = anmBase->getFrameMax();
}
