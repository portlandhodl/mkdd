#include "JSystem/J3D/J3DVertexBuffer.h"
#include "JSystem/J3D/J3DVertexData.h"
#include "JSystem/J3D/J3DDrawMtxData.h"
#include "JSystem/J3D/J3DPacket.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/JKernel/JKRHeap.h"
#include <dolphin/os.h>
#include <string.h>

J3DVertexData::J3DVertexData() {
    mVtxNum = 0;
    mNormNum = 0;
    mColorNum = 0;
    mTexCoordNum = 0;
    mPacketNum = 0;
    mVtxAttrFmtList = nullptr;
    mVtxPos = nullptr;
    mVtxNorm = nullptr;
    mVtxNBT = nullptr;
    for (int i = 0; i < 2; ++i) {
        mVtxColor[i] = nullptr;
    }
    for (int i = 0; i < 8; ++i) {
        mVtxTexCoord[i] = nullptr;
    }
    mVtxPosFrac = 0;
    mVtxPosType = GX_F32;
    mVtxNrmFrac = 0;
    mVtxNrmType = GX_F32;
}

bool J3DVertexData::isDeformablePositionFormat() const {
    return mVtxPosType == GX_F32 || (mVtxPosType == GX_S16 && mVtxPosFrac == 0) || (mVtxPosType == GX_S8 && mVtxPosFrac == 0)
        || mVtxPosType == GX_U8;
}

bool J3DVertexData::isDeformableNormalFormat() const {
    return mVtxNrmType == GX_F32 || (mVtxNrmType == GX_S16 && mVtxNrmFrac == 0) || (mVtxNrmType == GX_S8 && mVtxNrmFrac == 0);
}

bool J3DVertexData::isCpuSkinningPositionFormat() const {
    return true;
}

bool J3DVertexData::isCpuSkinningNormalFormat() const {
    return mVtxNrmType == GX_F32 && mVtxNrmFrac == 0 && mVtxNrmFrac == 0;
}

void J3DVertexBuffer::setVertexData(J3DVertexData* data) {
    mVtxData = data;
    mVtxPos[0] = data->mVtxPos;
    mVtxNorm[0] = data->mVtxNorm;
    mVtxColor[0] = data->mVtxColor[0];
    mVtxPos[1] = nullptr;
    mVtxNorm[1] = nullptr;
    mVtxColor[1] = nullptr;
    mTransformedVtxPos[0] = data->mVtxPos;
    mTransformedVtxNorm[0] = data->mVtxNorm;
    mTransformedVtxPos[1] = nullptr;
    mTransformedVtxNorm[1] = nullptr;
    frameInit();
}

void J3DVertexBuffer::init() {
    mVtxData = nullptr;
    mVtxPos[0] = mVtxPos[1] = nullptr;
    mVtxNorm[0] = mVtxNorm[1] = nullptr;
    mVtxColor[0] = mVtxColor[1] = nullptr;
    mTransformedVtxPos[0] = mTransformedVtxPos[1] = nullptr;
    mTransformedVtxNorm[0] = mTransformedVtxNorm[1] = nullptr;
    mCurrentVtxPos = nullptr;
    mCurrentVtxNorm = nullptr;
    mCurrentVtxColor = nullptr;
    frameInit();
}

J3DVertexBuffer::~J3DVertexBuffer() { }

void J3DVertexBuffer::setArray() const {
    j3dSys.setVtxPos(mCurrentVtxPos);
    j3dSys.setVtxNrm(mCurrentVtxNorm);
    j3dSys.setVtxCol(mCurrentVtxColor);
}

void J3DVertexBuffer::copyLocalVtxPosArray(u32 flag) {
    if (flag & 1) {
        for (int i = 0; i < 2; i++) {
            mVtxPos[i] = new (0x20) u8[mVtxData->mVtxNum * 12];
            memcpy(mVtxPos[i], mVtxData->mVtxPos, mVtxData->mVtxNum * 12);
            DCStoreRange(mVtxPos[i], mVtxData->mVtxNum * 12);
        }
    } else {
        mVtxPos[0] = mVtxData->mVtxPos;
        if (mVtxPos[1] == nullptr) {
            mVtxPos[1] = new (0x20) u8[mVtxData->mVtxNum * 12];
        }
        memcpy(mVtxPos[1], mVtxData->mVtxPos, mVtxData->mVtxNum * 12);
        DCStoreRange(mVtxPos[1], mVtxData->mVtxNum * 12);
    }
}

void J3DVertexBuffer::copyLocalVtxNrmArray(u32 flag) {
    if (flag & 1) {
        for (int i = 0; i < 2; i++) {
            mVtxNorm[i] = new (0x20) u8[mVtxData->mNormNum * 12];
            memcpy(mVtxNorm[i], mVtxData->mVtxNorm, mVtxData->mNormNum * 12);
            DCStoreRange(mVtxNorm[i], mVtxData->mNormNum * 12);
        }
    } else {
        mVtxNorm[0] = mVtxData->mVtxNorm;
        if (mVtxNorm[1] == nullptr) {
            mVtxNorm[1] = new (0x20) u8[mVtxData->mNormNum * 12];
        }
        memcpy(mVtxNorm[1], mVtxData->mVtxNorm, mVtxData->mNormNum * 12);
        DCStoreRange(mVtxNorm[1], mVtxData->mNormNum * 12);
    }
}

void J3DVertexBuffer::copyLocalVtxArray(u32 flag) {
    if (~flag & 2) {
        copyLocalVtxPosArray(flag);
    } else {
        mVtxPos[0] = mVtxPos[1] = mVtxData->mVtxPos;
    }

    if (~flag & 4) {
        copyLocalVtxNrmArray(flag);
    } else {
        mVtxNorm[0] = mVtxNorm[1] = mVtxData->mVtxNorm;
    }
}

s32 J3DVertexBuffer::allocTransformedVtxPosArray() {
    if (mTransformedVtxPos[0] != nullptr && mTransformedVtxPos[1] != nullptr) {
        return 0;
    }

    for (int i = 0; i < 2; i++) {
        if (i == 0 || mTransformedVtxPos[i] == nullptr) {
            mTransformedVtxPos[i] = new (0x20) u8[mVtxData->mVtxNum * 12];
            if (mTransformedVtxPos[i] == nullptr) {
                return 1;
            }
        }
    }
    return 0;
}

s32 J3DVertexBuffer::allocTransformedVtxNrmArray() {
    if (mTransformedVtxNorm[0] != nullptr && mTransformedVtxNorm[1] != nullptr) {
        return 0;
    }

    for (int i = 0; i < 2; i++) {
        if (i == 0 || mTransformedVtxNorm[i] == nullptr) {
            mTransformedVtxNorm[i] = new (0x20) u8[mVtxData->mNormNum * 12];
            if (mTransformedVtxNorm[i] == nullptr) {
                return 1;
            }
        }
    }
    return 0;
}

s32 J3DVertexBuffer::allocTransformedVtxArray(u32 flag) {
    if (~flag & 2) {
        s32 ret = allocTransformedVtxPosArray();
        if (ret != 0) {
            return ret;
        }
    } else {
        mTransformedVtxPos[0] = mTransformedVtxPos[1] = mVtxData->mVtxPos;
    }

    if (~flag & 4) {
        s32 ret = allocTransformedVtxNrmArray();
        if (ret != 0) {
            return ret;
        }
    } else {
        mTransformedVtxNorm[0] = mTransformedVtxNorm[1] = mVtxData->mVtxNorm;
    }
    return 0;
}

s32 J3DVertexBuffer::copyTransformedVtxArray() {
    if (!mTransformedVtxPos[0] || !mTransformedVtxPos[1] || !mTransformedVtxNorm[0] || !mTransformedVtxNorm[1]) {
        for (int i = 0; i < 2; ++i) {
            if (i == 0 || !mTransformedVtxPos[i]) {
                mTransformedVtxPos[i] = new (0x20) u8[mVtxData->mVtxNum * 12];
                if (!mTransformedVtxPos[i]) return 1;
            }
            memcpy(mTransformedVtxPos[i], mVtxData->mVtxPos, mVtxData->mVtxNum * 12);
            DCStoreRange(mTransformedVtxPos[i], mVtxData->mVtxNum * 12);
        }
        for (int i = 0; i < 2; ++i) {
            if (i == 0 || !mTransformedVtxNorm[i]) {
                mTransformedVtxNorm[i] = new (0x20) u8[mVtxData->mNormNum * 12];
                if (!mTransformedVtxNorm[i]) return 1;
            }
            memcpy(mTransformedVtxNorm[i], mVtxData->mVtxNorm, mVtxData->mNormNum * 12);
            DCStoreRange(mTransformedVtxNorm[i], mVtxData->mNormNum * 12);
        }
    }
    return mVtxData->mVtxNum;
}

void J3DVertexBuffer::copyVtxColorArray(J3DDeformAttachFlag flag) {
    if (!mVtxColor[0] || !mVtxColor[1]) {
        if (flag & 1) {
            for (int i = 0; i < 2; i++) {
                if (i == 0 || !mVtxColor[i]) {
                    mVtxColor[i] = new (0x20) GXColor[mVtxData->mColorNum];
                    memcpy(mVtxColor[i], mVtxData->mVtxColor[0], mVtxData->mColorNum << 2);
                    DCStoreRange(mVtxColor[i], mVtxData->mColorNum << 2);
                }
            }
        } else {
            mVtxColor[0] = mVtxData->mVtxColor[0];
            if (!mVtxColor[1]) {
                mVtxColor[1] = new (0x20) GXColor[mVtxData->mColorNum];
            }
            memcpy(mVtxColor[1], mVtxData->mVtxColor[0], mVtxData->mColorNum << 2);
            DCStoreRange(mVtxColor[1], mVtxData->mColorNum << 2);
        }
    }
}

void J3DVertexBuffer::resetVtxColorArray() {
    if (mVtxColor[0] && mVtxColor[0] != mVtxData->mVtxColor[0]) {
        delete[] mVtxColor[0];
    }
    if (mVtxColor[1] && mVtxColor[1] != mVtxData->mVtxColor[0]) {
        delete[] mVtxColor[1];
    }
    mVtxColor[0] = mVtxData->mVtxColor[0];
    mVtxColor[1] = mVtxColor[0];
    mCurrentVtxColor = mVtxColor[0];
}

J3DDrawMtxData::J3DDrawMtxData() {
    mCount = 0;
    mDrawMtxFlag = nullptr;
    mDrawMtxIdx = nullptr;
}

J3DDrawMtxData::~J3DDrawMtxData() { }
