#include <dolphin/os.h>
#include "JSystem/J3D/J3DDisplayListObj.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DSys.h"

void J3DModelData::clear()
{
    mBmd = nullptr;
    mModelLoaderFlags = 0;
    mBumpFlag = 0;
    mBillboardFlag = 0;
}

J3DModelData::J3DModelData()
{
    clear();
}

s32 J3DModelData::newSharedDisplayList(u32 mdlFlags)
{
    u16 matNum = getMaterialNum();

    for (u16 i = 0; i < matNum; i++)
    {
        s32 ret;
        if (mdlFlags & J3DMLF_UseSingleSharedDL)
        {
            ret = getMaterialNodePointer(i)->newSingleSharedDisplayList(getMaterialNodePointer(i)->countDLSize());
            if (ret != JET_Success)
                return ret;
        }
        else
        {
            ret = getMaterialNodePointer(i)->newSharedDisplayList(getMaterialNodePointer(i)->countDLSize());
            if (ret != JET_Success)
                return ret;
        }
    }

    return JET_Success;
}

void J3DModelData::indexToPtr()
{
    j3dSys._58 = getTexture();

    static BOOL sInterruptFlag = OSDisableInterrupts();
    OSDisableScheduler();

    u32 matNum = getMaterialNum();
    GDCurrentDL gdlObj;
    for (u16 i = 0; i < matNum; i++)
    {
        J3DMaterial* matNode = getMaterialNodePointer(i);
        J3DDisplayListObj* dlObj = matNode->getSharedDisplayListObj();

        GDInitGDLObj(&gdlObj, dlObj->getDisplayList(0), dlObj->getDisplayListSize());
        __GDSetCurrent(&gdlObj);
        matNode->getTevBlock()->indexToPtr();
    }

    __GDSetCurrent(nullptr);
    OSEnableScheduler();
    OSRestoreInterrupts(sInterruptFlag);
}

void J3DModelData::makeSharedDL()
{
    j3dSys._58 = mMaterialTable.mTextures;
    u16 count = mMaterialTable.mMaterialNum;
    for (u16 i = 0; i < count; i++)
    {
        J3DMaterial* material = mMaterialTable.mMaterials[i];
        material->makeSharedDisplayList();
    }
}

void J3DModelData::simpleCalcMaterial(u16 jointIndex, Mtx mtx)
{
    syncJ3DSysFlags();
    for (J3DMaterial* material = mJointTree.getJointNodePointer(jointIndex)->mMaterial; material != nullptr;
         material = material->getNext())
    {
        if (material->getMaterialAnm())
        {
            material->getMaterialAnm()->calc(material);
        }
        material->calc(mtx);
    }
}

void J3DModelData::syncJ3DSysPointers() const
{
    j3dSys._58 = getTexture();
    j3dSys.setVtxPos(getVtxPosArray());
    j3dSys.setVtxNrm(getVtxNrmArray());
    j3dSys.setVtxCol(getVtxColorArray(0));
}

void J3DModelData::syncJ3DSysFlags() const
{
    if (mModelLoaderFlags & J3DMLF_UsePostTexMtx)
    {
        j3dSys._34 |= 0x40000000;
    }
    else
    {
        j3dSys._34 &= ~0x40000000;
    }
}
