#include "JSystem/J3D/J3DSkinDeform.h"
#include "JSystem/J3D/J3DAnmCluster.h"
#include "JSystem/JMath/JMath.h"
#include <dolphin/os.h>
#include <dolphin/base/PPCArch.h>

inline f32 JMath::TAsinAcosTable<1024, f32>::acos_(f32 x) const
{
    if (x >= 1.0f) {
        return 0.0f;
    } else if (x <= -1.0f) {
        return TAngleConstant_<f32>::RADIAN_DEG180();
    } else if (x < 0.0f) {
        return mTable[(u32)(-x * 1023.5f)] + TAngleConstant_<f32>::RADIAN_DEG090();
    } else {
        return TAngleConstant_<f32>::RADIAN_DEG090() - mTable[(u32)(x * 1023.5f)];
    }
}

namespace JMath {
inline f32 acosDegree(f32 value)
{
    return asinAcosTable_.acos_(value) * TAngleConstant_<f32>::RADIAN_TO_DEGREE_FACTOR();
}
}

J3DDeformData::J3DDeformData()
{
    mClusterNum = 0;
    mClusterKeyNum = 0;
    mClusterVertexNum = 0;
    mClusterPointer = NULL;
    mClusterKeyPointer = NULL;
    mClusterVertex = NULL;
    mVtxPosNum = 0;
    mVtxNrmNum = 0;
    mVtxPos = NULL;
    mVtxNrm = NULL;
    mClusterName = NULL;
    mClusterKeyName = NULL;
}

void J3DDeformData::offAllFlag(u32 flag)
{
    for (u16 i = 0; i < mClusterNum; i++) {
        mClusterPointer[i].mDeformer->mFlags &= ~flag;
    }
}

void J3DDeformData::deform(J3DModel* model)
{
    deform(model->getVertexBuffer());
}

void J3DDeformData::deform(J3DVertexBuffer* vtxBuffer)
{
    void* prev = vtxBuffer->mVtxPos[0];
    vtxBuffer->mVtxPos[0] = vtxBuffer->mVtxPos[1];
    vtxBuffer->mVtxPos[1] = prev;

    prev = vtxBuffer->mVtxNorm[0];
    vtxBuffer->mVtxNorm[0] = vtxBuffer->mVtxNorm[1];
    vtxBuffer->mVtxNorm[1] = prev;

    for (u16 i = 0; i < mClusterNum; i++) {
        mClusterPointer[i].mDeformer->deform(vtxBuffer, i);
    }

    DCStoreRangeNoSync(vtxBuffer->mVtxPos[0], vtxBuffer->mVtxData->mVtxNum * 12);
    DCStoreRangeNoSync(vtxBuffer->mVtxNorm[0], vtxBuffer->mVtxData->mNormNum * 12);
    PPCSync();

    vtxBuffer->setCurrentVtxPos(vtxBuffer->mVtxPos[0]);
    vtxBuffer->setCurrentVtxNrm(vtxBuffer->mVtxNorm[0]);
}

void J3DDeformData::setAnm(J3DAnmCluster* anm)
{
    for (u16 i = 0; i < mClusterNum; i++) {
        mClusterPointer[i].mDeformer->mAnmCluster = anm;
    }
}

J3DDeformer::J3DDeformer(J3DDeformData* deformData)
{
    mDeformData = deformData;
    mAnmCluster = NULL;
    _08 = 0;
    _0C = 0;
    mFlags = 3;
}

void J3DDeformer::deform(J3DVertexBuffer* vtxBuffer, u16 index)
{
    u16 keyIndex = 0;
    if (mAnmCluster != NULL) {
        for (u16 i = 0; i < index; i++) {
            keyIndex += mDeformData->getClusterPointer(i)->_10;
        }

        u16 keyNum = mDeformData->getClusterPointer(index)->_10;
        for (u16 i = 0; i < keyNum; i++) {
            ((f32*)_08)[i] = mAnmCluster->getWeight(keyIndex++);
        }
        deform(vtxBuffer, index, (f32*)_08);
    }
}

void J3DDeformer::deform_VtxPosF32(J3DVertexBuffer* vtxBuffer, J3DCluster* cluster,
                                   J3DClusterKey* keys, f32* weights)
{
    J3DClusterKey* key;
    int posNum = cluster->_12;
    int keyNum = cluster->_10;
    f32* vtxPosArray = (f32*)vtxBuffer->mVtxPos[0];
    f32* deformVtxPos = mDeformData->getVtxPos();
    u16* dstIndices = *(u16**)cluster->_18;

    for (int i = 0; i < posNum; i++) {
        int index = dstIndices[i] * 3;
        vtxPosArray[index] = 0.0f;
        vtxPosArray[index + 1] = 0.0f;
        vtxPosArray[index + 2] = 0.0f;
    }

    f32 modifiers[2] = { 1.0f, -1.0f };

    for (u16 i = 0; i < posNum; i++) {
        int index = (*(u16**)cluster->_18)[i] * 3;
        for (u16 j = 0; j < keyNum; j++) {
            int flags;
            int srcIndex;
            key = &keys[j];
            flags = srcIndex = key->_04[i];
            srcIndex &= ~0xE000;
            srcIndex *= 3;
            f32 deform0 = deformVtxPos[srcIndex];
            f32 deform1 = deformVtxPos[srcIndex + 1];
            f32 deform2 = deformVtxPos[srcIndex + 2];
            deform0 *= modifiers[(flags & 0x8000) >> 15];
            deform1 *= modifiers[(flags & 0x4000) >> 14];
            deform2 *= modifiers[(flags & 0x2000) >> 13];
            vtxPosArray[index] += deform0 * weights[j];
            vtxPosArray[index + 1] += deform1 * weights[j];
            vtxPosArray[index + 2] += deform2 * weights[j];
        }
    }
}

void J3DDeformer::deform_VtxNrmF32(J3DVertexBuffer* vtxBuffer, J3DCluster* cluster,
                                   J3DClusterKey* key, f32* weights)
{
    Vec* vtxNorms;
    u16 i;
    u16 clusterSize;
    f32* vtxNormBuffer;
    f32* vecBuffer;
    int clusterUnk;
    u32 idx2;
    f32* currVec;

    vtxNormBuffer = (f32*)vtxBuffer->mVtxNorm[0];
    vtxNorms = (Vec*)mDeformData->getVtxNrm();
    vecBuffer = (f32*)_0C;
    clusterSize = cluster->_10;
    clusterUnk = cluster->_16;

    for (i = 0; i < cluster->_14; i++) {
        f32* sumVec = &vecBuffer[(int)(i * 3)];
        sumVec[0] = 0.0f;
        sumVec[1] = 0.0f;
        sumVec[2] = 0.0f;
        for (u16 j = 0; j < clusterSize; j++) {
            u16 flag = key[j]._08[i];
            f32 xWeight;
            if (flag & 0x8000) {
                xWeight = -vtxNorms[flag & ~0xE000].x;
            } else {
                xWeight = vtxNorms[flag & ~0xE000].x;
            }
            f32 yWeight;
            if (flag & 0x4000) {
                yWeight = -vtxNorms[flag & ~0xE000].y;
            } else {
                yWeight = vtxNorms[flag & ~0xE000].y;
            }
            f32 zWeight;
            if (flag & 0x2000) {
                zWeight = -vtxNorms[flag & ~0xE000].z;
            } else {
                zWeight = vtxNorms[flag & ~0xE000].z;
            }

            sumVec[0] += xWeight * weights[j];
            sumVec[1] += yWeight * weights[j];
            sumVec[2] += zWeight * weights[j];
        }
        PSVECNormalize((Vec*)sumVec, (Vec*)sumVec);
    }

    for (u16 i = 0; i < clusterUnk; i++) {
        J3DClusterVertex* vertex = &((J3DClusterVertex*)cluster->_1C)[i];
        Vec vec;
        vec.x = 0.0f;
        vec.y = 0.0f;
        vec.z = 0.0f;
        f32 val = 1.0f / f32(vertex->_00);
        for (u16 j = 0; j < vertex->_00; j++) {
            f32* currVec = &vecBuffer[(int)(vertex->_04[j] * 3)];
            vec.x += val * currVec[0];
            vec.y += val * currVec[1];
            vec.z += val * currVec[2];
        }

        PSVECNormalize(&vec, &vec);

        for (u16 j = 0; j < vertex->_00; j++) {
            u16 idx = vertex->_08[j];
            if (idx == 0xFFFF) {
                continue;
            }

            idx2 = vertex->_04[j];
            currVec = &vecBuffer[(int)(idx2 * 3)];
            int idx3 = idx * 3;
            f32 dotProd = vec.x * currVec[0] + vec.y * currVec[1] + vec.z * currVec[2];
            if (dotProd >= 1.0f) {
                dotProd = 0.0f;
            } else if (dotProd > -1.0f) {
                dotProd = JMath::acosDegree(dotProd);
            } else {
                dotProd = 180.0f;
            }

            if (dotProd <= cluster->_04) {
                f32* norm = &vtxNormBuffer[idx3];
                norm[0] = vec.x;
                norm[1] = vec.y;
                norm[2] = vec.z;
                continue;
            }

            if (dotProd > cluster->_00) {
                f32* nextNorm = &vecBuffer[idx2 * 3];
                f32* norm = &vtxNormBuffer[idx3];
                norm[0] = nextNorm[0];
                norm[1] = nextNorm[1];
                norm[2] = nextNorm[2];
                continue;
            }

            f32 factor = (dotProd - cluster->_04) / (cluster->_00 - cluster->_04);
            f32* norm = &vtxNormBuffer[idx3];

            f32 inv = 1.0f - factor;
            norm[0] = factor * currVec[0] + inv * vec.x;
            norm[1] = factor * currVec[1] + inv * vec.y;
            norm[2] = factor * currVec[2] + inv * vec.z;
        }
    }
}

void J3DDeformer::deform(J3DVertexBuffer* vtxBuffer, u16 index, f32* weights)
{
    if ((mFlags & 2) && vtxBuffer->mVtxData->mVtxPosType == GX_F32) {
        J3DCluster* cluster = mDeformData->getClusterPointer(index);

        u16 keyStart = 0;
        for (u16 i = 0; i < index; i++) {
            keyStart += mDeformData->getClusterPointer(i)->_10 + 1;
        }

        J3DClusterKey* key = mDeformData->getClusterKeyPointer(keyStart);
        normalizeWeight(cluster->_10, weights);
        deform_VtxPosF32(vtxBuffer, cluster, key, weights);

        if ((mFlags & 1) && cluster->_0C && vtxBuffer->mVtxData->mVtxNrmType == GX_F32) {
            deform_VtxNrmF32(vtxBuffer, cluster, key, weights);
        }
    }
}

void J3DDeformer::normalizeWeight(int count, f32* weights)
{
    f32 total = 0.0f;
    for (u16 i = 0; i < count; i++) {
        total += weights[i];
    }
    total = 1.0f / total;

    for (u16 i = 0; i < count; i++) {
        weights[i] *= total;
    }
}
