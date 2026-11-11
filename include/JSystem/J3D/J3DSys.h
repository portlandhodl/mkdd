#ifndef _JSYSTEM_J3D_J3DSYS_H
#define _JSYSTEM_J3D_J3DSYS_H

#include <dolphin/mtx.h>
#include "JSystem/J3D/J3DPacket.h"
#include "JSystem/J3D/J3DTypes.h"
#include "JSystem/JGeometry.h"
#include "types.h"

struct J3DDrawBuffer;
struct J3DMatPacket;
struct J3DModel;
struct J3DMtxCalc;
struct J3DShapePacket;
struct J3DTexture;

struct J3DSys
{
    J3DSys();
    void drawInit();
    void reinitGX();
    void reinitGenMode();
    void reinitIndStages();
    void reinitLighting();
    void reinitPixelProc();
    void reinitTexture();
    void reinitTevStages();
    void reinitTransform();
    void loadPosMtxIndx(int, u16) const;
    void loadNrmMtxIndx(int, u16) const;
    void ErrorReport(J3DErrType) const;
    void setTexCacheRegion(GXTexCacheSize);

    void setDrawBuffer(J3DDrawBuffer *drawBuffer, int idx) { mDrawBuffer[idx] = drawBuffer; }
    void setDrawModeOpaTexEdge() { mDrawMode = 3; }
    void setDrawModeXlu() { mDrawMode = 4; }
    void setViewMtx(const Mtx mtx) { PSMTXCopy(mtx, mViewMtx); }
    Mtx &getViewMtx() { return mViewMtx; }
    J3DDrawBuffer *getDrawBuffer(int idx) { return mDrawBuffer[idx]; }
    J3DTexture *getTexture() { return _58; }

    J3DModel *getModel() { return _38; }

    static void setCurrentMtx(const Mtx m) { PSMTXCopy(m, mCurrentMtx); }

    void setVtxPos(void *p) { _10C = (u32)p; }
    void setVtxNrm(void *p) { _110 = (u32)p; }
    void setVtxCol(void *p) { _114 = (u32)p; }
    void *getVtxPos() { return (void *)_10C; }
    void *getVtxNrm() { return (void *)_110; }
    void *getVtxCol() { return (void *)_114; }

    void setModel(J3DModel *pModel) { _38 = pModel; }
    void setShapePacket(J3DShapePacket *pPacket) { mShapePacket = pPacket; }
    J3DShapePacket *getShapePacket() { return mShapePacket; }
    void setModelDrawMtx(Mtx *pMtx) { _104 = pMtx; GXSetArray(GX_POS_MTX_ARRAY, _104, sizeof(*_104)); }
    void setModelNrmMtx(Mtx33 *pMtx) { _108 = (Mtx *)pMtx; GXSetArray(GX_NRM_MTX_ARRAY, _108, sizeof(Mtx33)); }

    bool checkFlag(u32 flag) const { return _34 & flag; }
    void setNBTScale(const Vec *scale) { _118 = (Vec *)scale; }

    Mtx mViewMtx;                  // _000
    J3DMtxCalc *mMtxCalc;          // _030
    u32 _34;                       // _034 /* bitfield */
    J3DModel *_38;                 // _038
    J3DMatPacket *mMatPacket;      // _03C
    J3DShapePacket *mShapePacket;  // _040
    u32 _44;                       // _044
    J3DDrawBuffer *mDrawBuffer[2]; // _048
    int mDrawMode;                 // _050, TODO: enum/defines
    u32 _54;                       // _054
    J3DTexture *_58;               // _058
    u8 _5C[4];                     // _05C
    u32 mTexCacheRegionNum;        // _060
    GXTexRegion mTexCacheRegion[8]; // _064
    u8 _E4[0x20];                  // _0E4
    Mtx *_104;                     // _104
    Mtx *_108;                     // _108
    /*
     * These three might be part of a J3DVertexBuffer.
     * See setArray__15J3DVertexBufferCFv.
     */
    u32 _10C; // _10C
    u32 _110; // _110
    u32 _114; // _114 /* bitfield */

    Vec *_118;

    static Mtx mCurrentMtx;
    static JGeometry::TVec3f mCurrentS;
    static JGeometry::TVec3f mParentS;
    static u16 sTexCoordScaleTable[8][4];
};

extern J3DSys j3dSys;

extern s32 j3dDefaultViewNo;

#endif
