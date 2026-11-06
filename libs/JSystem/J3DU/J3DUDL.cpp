#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DModel.h"

void J3DUCountUseMtxNum(J3DModelData *modelData) {
    modelData->mShapeTable.mItems[0]->mShapeMtx[0]->getUseMtxNum();
}
