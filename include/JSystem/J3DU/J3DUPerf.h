#ifndef _JSYSTEM_J3D_J3DUPERF_H
#define _JSYSTEM_J3D_J3DUPERF_H

#include "types.h"

struct J3DModelData;
struct J3DShape;
class JUTDbPrint;

struct J3DUPerf
{
    J3DUPerf(J3DModelData *);
    ~J3DUPerf();

    void init();
    void count(J3DModelData *);
    void report();
    void debugPrint(JUTDbPrint *);
    void reportVtxFmt();
    void checkVertexFormat(J3DModelData *);

    J3DModelData *mModelData;
    u32 mVertexCount;
    u32 mPolygonCount;
    u32 mStripCount;
    u32 mVertexFormat;
    u32 mFlags;
};

void J3DUCountVertex(J3DShape *);
void J3DUCountVertex(J3DModelData *);
void J3DUCountPolygon(J3DShape *);
void J3DUCountPolygon(J3DModelData *);
void J3DUCheckPrimitiveStrip(J3DShape *);
void J3DUCheckPrimitiveStrip(J3DModelData *);
void J3DUCheckVertexFormat(J3DModelData *, _GXAttr);

#endif
