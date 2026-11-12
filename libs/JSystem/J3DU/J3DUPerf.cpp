#include <dolphin/gx.h>
#include <dolphin/os.h>
#include <stdio.h>
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3DU/J3DUPerf.h"
#include "JSystem/J3DU/J3DU.h"
#include "JSystem/JUtility/JUTDbPrint.h"

#pragma push
#pragma force_active on
u32 J3DUMemRequest::sBusTransactionMax = OS_BUS_CLOCK / 120;
#pragma pop

void J3DUPerf::init()
{
    mModelData = nullptr;
    mVertexCount = 0;
    mPolygonCount = 0;
    mStripCount = 0;
    mVertexFormat = 0;
    mFlags = 0;
}

J3DUPerf::J3DUPerf(J3DModelData *modelData)
{
    init();
    mModelData = modelData;
    count(modelData);
}

void J3DUPerf::count(J3DModelData *modelData)
{
    for (u16 i = 0; i < modelData->getShapeNum(); i++) {
        J3DShape *shape = modelData->getShapeNodePointer(i);
        J3DUCountVertex(shape);
        J3DUCountPolygon(shape);
        J3DUCheckPrimitiveStrip(shape);
        checkVertexFormat(modelData);
    }
}

void J3DUPerf::report() {}

void J3DUPerf::debugPrint(JUTDbPrint *print)
{
    char buf[128];
    sprintf(buf, "vertex: %u\n", mVertexCount);
    sprintf(buf, "polygon: %u\n", mPolygonCount);
    sprintf(buf, "strip: %u\n", mStripCount);
    sprintf(buf, "vtxfmt: %x\n", mVertexFormat);
    sprintf(buf, "flags: %x\n", mFlags);
    (void)print;
    (void)buf;
}

void J3DUPerf::reportVtxFmt()
{
    volatile u32 fmt = mVertexFormat;
    (void)fmt;
}

void J3DUPerf::checkVertexFormat(J3DModelData *modelData)
{
    J3DUCheckVertexFormat(modelData, GX_VA_POS);
}

void J3DUCountVertex(J3DShape *shape)
{
    volatile u32 count = 0;
    for (u32 i = 0; i < shape->mMtxGroupNum; i++)
        count += shape->mShapeDraw[i]->mDlSize;
    (void)count;
}

void J3DUCountVertex(J3DModelData *modelData)
{
    volatile u32 count = 0;
    for (u16 i = 0; i < modelData->getShapeNum(); i++)
        count += modelData->getShapeNodePointer(i)->mMtxGroupNum;
    (void)count;
}

void J3DUCountPolygon(J3DShape *shape)
{
    volatile u32 count = 0;
    for (u32 i = 0; i < shape->mMtxGroupNum; i++)
        count += (u32)shape->mShapeDraw[i]->mDisplayList + shape->mShapeDraw[i]->mDlSize;
    (void)count;
}

void J3DUCountPolygon(J3DModelData *modelData)
{
    volatile u32 count = 0;
    for (u16 i = 0; i < modelData->getShapeNum(); i++)
        count += modelData->getShapeNodePointer(i)->mMtxGroupNum;
    (void)count;
}

void J3DUCheckPrimitiveStrip(J3DShape *shape)
{
    volatile u32 mode = shape->mMode;
    (void)mode;
}

void J3DUCheckPrimitiveStrip(J3DModelData *modelData)
{
    volatile u32 count = 0;
    for (u16 i = 0; i < modelData->getShapeNum(); i++)
        count += modelData->getShapeNodePointer(i)->mMode;
    (void)count;
}

void J3DUCheckVertexFormat(J3DModelData *modelData, _GXAttr attr)
{
    volatile u32 fmt = (u32)modelData->getVertexData() + attr;
    (void)fmt;
}

J3DUPerf::~J3DUPerf()
{
    mModelData = nullptr;
}

#pragma force_active on
