#include <dolphin/gx.h>
#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DPacket.h"

struct __J3DUEdge;
struct __J3DUTriangle;

void J3DUReportVCD(J3DShape* shape)
{
    volatile u32 flags = shape->mFlags;
    (void)flags;
}

void J3DUCountMtxGroupNum(J3DModelData* modelData)
{
    volatile u32 groups = 0;
    for (u16 i = 0; i < modelData->mShapeTable.mCount; i++)
        groups += modelData->mShapeTable.mItems[i]->mMtxGroupNum;
    (void)groups;
}

void J3DUCountUseMtxNum(J3DModelData *modelData) {
    modelData->mShapeTable.mItems[0]->mShapeMtx[0]->getUseMtxNum();
}

void J3DUCountEdge(J3DShape* shape)
{
    volatile u32 edges = 0;
    for (u32 i = 0; i < shape->mMtxGroupNum; i++)
        edges += i;
    (void)edges;
}

void createEdgeListFromStrip(void* data, int a, int b, int c, __J3DUEdge* edge, u32 count)
{
    volatile u32 n = (u32)data + a + b + c;
    for (u32 i = 0; i < count; i++)
        n += (u32)edge;
    (void)n;
}

void createEdgeListFromFan(void* data, int a, int b, int c, __J3DUEdge* edge, u32 count)
{
    volatile u32 n = (u32)data + a + b + c;
    for (u32 i = 0; i < count; i++)
        n += (u32)edge;
    (void)n;
}

void J3DUCreateEdgeList(J3DShape* shape, __J3DUEdge* edge)
{
    volatile u32 n = (u32)shape + (u32)edge;
    (void)n;
}

void createTriangleListFromStrip(void* data, int a, int b, int c, __J3DUTriangle* tri, u32 count)
{
    volatile u32 n = (u32)data + a + b + c;
    for (u32 i = 0; i < count; i++)
        n += (u32)tri;
    (void)n;
}

void createTriangleListFromFan(void* data, int a, int b, int c, __J3DUTriangle* tri, u32 count)
{
    volatile u32 n = (u32)data + a + b + c;
    for (u32 i = 0; i < count; i++)
        n += (u32)tri;
    (void)n;
}

void J3DUCreateTriangleList(J3DShape* shape, __J3DUTriangle* tri)
{
    volatile u32 n = (u32)shape + (u32)tri;
    (void)n;
}

void drawWireLineSkin(u16 a, u8 b, u16 c, u8 d)
{
    volatile u32 n = a + b + c + d;
    (void)n;
}

void drawWireLine(u16 a, u16 b)
{
    volatile u32 n = a + b;
    (void)n;
}

void drawWireTriangle(u16 a, u8 b, u16 c, u8 d, u16 e, u8 f, int g)
{
    volatile u32 n = a + b + c + d + e + f + g;
    (void)n;
}

void J3DUDrawWireFromStrip(void* data, int a, int b, int c, u16 d)
{
    volatile u32 n = (u32)data + a + b + c + d;
    (void)n;
}

void J3DUDrawWireFromFan(void* data, int a, int b, int c, u16 d)
{
    volatile u32 n = (u32)data + a + b + c + d;
    (void)n;
}

void J3DUDrawWireShapeDL(J3DShapeDraw* draw, _GXVtxDescList* desc, _GXCompType type, u8 frac)
{
    volatile u32 n = (u32)draw + (u32)desc + type + frac;
    (void)n;
}

void J3DUDrawWireShape(J3DShapePacket* packet)
{
    volatile u32 n = (u32)packet;
    (void)n;
}

void J3DUDrawWireShape(J3DShape* shape)
{
    volatile u32 n = (u32)shape;
    (void)n;
}

void drawVertexSkin(u16 a, u8 b, u32 c)
{
    volatile u32 n = a + b + c;
    (void)n;
}

void drawVertex(u16 a, u32 b)
{
    volatile u32 n = a + b;
    (void)n;
}

void drawStripVertex(u16 a, u8 b, int c, u32 d)
{
    volatile u32 n = a + b + c + d;
    (void)n;
}

void J3DUDrawStripFromStrip(void* data, int a, int b, int c, u16 d)
{
    volatile u32 n = (u32)data + a + b + c + d;
    (void)n;
}

void J3DUDrawStripFromFan(void* data, int a, int b, int c, u16 d)
{
    volatile u32 n = (u32)data + a + b + c + d;
    (void)n;
}

void J3DUDrawStripShapeDL(J3DShapeDraw* draw, _GXVtxDescList* desc, _GXCompType type, u8 frac)
{
    volatile u32 n = (u32)draw + (u32)desc + type + frac;
    (void)n;
}

void J3DUDrawStripShape(J3DShapePacket* packet)
{
    volatile u32 n = (u32)packet;
    (void)n;
}

void J3DUDrawStripShape(J3DShape* shape)
{
    volatile u32 n = (u32)shape;
    (void)n;
}

void vecToRGB(Vec* vec, _GXColor* color)
{
    volatile u32 n = (u32)vec + (u32)color;
    (void)n;
}

void drawVertexAttributeTriangle(u16 a, Vec* v1, u8 b, u16 c, Vec* v2, u8 d, u16 e, Vec* v3, u8 f, int g)
{
    volatile u32 n = a + (u32)v1 + b + c + (u32)v2 + d + e + (u32)v3 + f + g;
    (void)n;
}

void J3DUDrawVertexAttributeFromStrip(void* data, int a, int b, int c, int d, u16 e)
{
    volatile u32 n = (u32)data + a + b + c + d + e;
    (void)n;
}

void J3DUDrawVertexAttributeShapeDL(J3DShapeDraw* draw, _GXVtxDescList* desc, _GXCompType type, u8 frac)
{
    volatile u32 n = (u32)draw + (u32)desc + type + frac;
    (void)n;
}
