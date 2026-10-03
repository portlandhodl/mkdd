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


void J3DAnmObjBase::loadJ3DModelData(void *param_1, u32 param_2) {
    // FIX: Identify what the bitmask here is really used for.
    J3DModelLoaderDataBase().load(this, (u32)param_1 | 0x19200010);
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

void J3DAnmObjMaterial::setupColorAnmData(J3DAnmColor **, J3DModelData *, void *) {}

void J3DAnmObjMaterial::setupTexSRTAnmData(J3DAnmTextureSRTKey **, J3DModelData *, void *) {}

void J3DAnmObjMaterial::setupTexPatternAnmData(J3DAnmTexPattern **, J3DModelData *, void *) {}

void J3DAnmObjMaterial::setupTevRegAnmData(J3DAnmTevRegKey **param_1, J3DModelData *param_2, void *param_3) {}

template<> void J3DAnmObjMaterial::setMaterialAnmTev<J3DAnmTevRegKey>(J3DAnmTevRegKey **param_1, J3DModelData *param_2) {
    
}

void J3DAnmObjCluster::loadClusterData(J3DDeformData **, void *) {}

void J3DAnmObjCluster::loadClusterAnmData(J3DAnmCluster **anmCluster, void *) {}

void J3DAnmObjCluster::setDeformData(ExModel *param_1, J3DDeformData *param_2, bool param_3) {}

void J3DAnmObjCluster::attach(J3DAnmCluster *anmCluster) {
    mAnmBase = anmCluster;
    J3DAnmBase *anmBase = mAnmBase;
    mFrameCtrl.mAttr = anmBase->getAttribute();
    mFrameCtrl.mEndFrame = anmBase->getFrameMax();
}
