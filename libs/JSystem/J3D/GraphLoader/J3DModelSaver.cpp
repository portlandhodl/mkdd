#include "JSystem/J3D/J3DShape.h"

static void fakeModelSaverFunc(u16 idx, J3DShapeMtx *shapeMtx)
{
    shapeMtx->getUseMtxIndex(idx);
}
