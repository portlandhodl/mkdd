#include "JSystem/J3D/J3DTexture.h"

void J3DTexture::loadGX(u16 idx, GXTexMapID texMapID) const
{
    ResTIMG* timg = getResTIMG(idx);
    GXTexObj texObj;
    GXTlutObj tlutObj;

    if (!timg->mPaletteFormat) {
        GXInitTexObj(&texObj, (u8*)timg + timg->mImageDataOffset, timg->mSizeX, timg->mSizeY,
                     (GXTexFmt)timg->mTextureFormat, (GXTexWrapMode)timg->mWrapS,
                     (GXTexWrapMode)timg->mWrapT, timg->mIsMIPmapEnabled);
    } else {
        GXInitTexObjCI(&texObj, (u8*)timg + timg->mImageDataOffset, timg->mSizeX, timg->mSizeY,
                       (GXCITexFmt)timg->mTextureFormat, (GXTexWrapMode)timg->mWrapS,
                       (GXTexWrapMode)timg->mWrapT, timg->mIsMIPmapEnabled, (u32)texMapID);
        GXInitTlutObj(&tlutObj, (u8*)timg + timg->mPaletteOffset, (GXTlutFmt)timg->mLutFormat,
                      timg->mPaletteEntryCount);
        GXLoadTlut(&tlutObj, texMapID);
    }

    const f32 kLODClampScale = 1.0f / 8.0f;
    const f32 kLODBiasScale = 1.0f / 100.0f;
    GXInitTexObjLOD(&texObj, (GXTexFilter)timg->mMinFilterType, (GXTexFilter)timg->mMagFilterType,
                    timg->mMinLOD * kLODClampScale, timg->mMaxLOD * kLODClampScale,
                    timg->mLODBias * kLODBiasScale, timg->mIsBiasClamp, timg->mDoEdgeLOD,
                    (GXAnisotropy)timg->mIsMaxAnisotropy);
    GXLoadTexObj(&texObj, texMapID);
}
