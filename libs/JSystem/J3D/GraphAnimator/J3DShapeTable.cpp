#include "JSystem/J3D/J3DShape.h"
#include "types.h"

void J3DShapeTable::initShapeNodes(J3DDrawMtxData *pMtxData, J3DVertexData *pVtxData)
{
    u16 shapeNum = mCount;
    for (u16 i = 0; i < shapeNum; i++) {
        J3DShape *shapeNode = mItems[i];

        shapeNode->setDrawMtxDataPointer(pMtxData);
        shapeNode->setVertexDataPointer(pVtxData);
        shapeNode->makeVcdVatCmd();
    }
}

void J3DShapeTable::sortVcdVatCmd()
{
    u16 shapeNum = mCount;
    for (u16 next = 0; next < shapeNum; next++) {
        for (u16 prev = 0; prev < next; prev++) {
            if (mItems[next]->isSameVcdVatCmd(mItems[prev])) {
                mItems[next]->setVcdVatCmd(mItems[prev]->getVcdVatCmd());
            }
        }
    }
}
