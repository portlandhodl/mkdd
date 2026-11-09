#include "JSystem/J3D/J3DDrawBuffer.h"
#include "JSystem/J3D/J3DMaterial.h"
#include "JSystem/J3D/J3DPacket.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/J3D/J3DTexture.h"
#include "JSystem/JKernel/JKRHeap.h"

void J3DDrawBuffer::initialize()
{
    mDrawMode = 0;
    mSortType = J3DSORT_Mat;
    mZNear = 1.0f;
    mZFar = 10000.0f;
    mZMtx = nullptr;
    mCallBackPacket = nullptr;
    mBufferSize = 0x20;
    calcZRatio();
}

J3DErrType J3DDrawBuffer::allocBuffer(u32 size)
{
    mBuffer = new (0x20) J3DPacket*[size];
    mBufferSize = size;
    frameInit();
    calcZRatio();
    return (J3DErrType)0;
}

J3DDrawBuffer::~J3DDrawBuffer()
{
    frameInit();
    delete[] mBuffer;
    mBuffer = nullptr;
}

void J3DDrawBuffer::frameInit()
{
    u32 bufSize = mBufferSize;
    for (u32 i = 0; i < bufSize; i++)
        mBuffer[i] = nullptr;

    mCallBackPacket = nullptr;
}

int J3DDrawBuffer::entryMatSort(J3DMatPacket *pMatPacket)
{
    u32 hash;

    pMatPacket->drawClear();
    pMatPacket->getShapePacket()->drawClear();

    if (pMatPacket->isChanged()) {
        pMatPacket->setNextPacket(mBuffer[0]);
        mBuffer[0] = pMatPacket;
        return 1;
    }

    J3DTexture *pTexture = j3dSys.getTexture();
    u32 texNo = pMatPacket->getMaterial()->getTexNo(0);

    if (texNo == 0xFFFF) {
        hash = 0;
    } else {
        hash = (reinterpret_cast< u32 >(pTexture->getResTIMG(texNo)) + pTexture->getResTIMG(texNo)->mImageDataOffset) >> 5;
    }
    u32 slot = hash & (mBufferSize - 1);

    if (mBuffer[slot] == nullptr) {
        mBuffer[slot] = pMatPacket;
        return 1;
    }

    J3DMatPacket *packet;
    for (packet = static_cast< J3DMatPacket * >(mBuffer[slot]); packet != nullptr; packet = static_cast< J3DMatPacket * >(packet->getNextPacket())) {
        if (packet->isSame(pMatPacket)) {
            packet->addShapePacket(pMatPacket->getShapePacket());
            return 0;
        }
    }

    pMatPacket->setNextPacket(mBuffer[slot]);
    mBuffer[slot] = pMatPacket;
    return 1;
}

int J3DDrawBuffer::entryMatAnmSort(J3DMatPacket *pMatPacket)
{
    J3DMaterialAnm *pMaterialAnm = pMatPacket->mMaterialAnm;
    u32 slot = reinterpret_cast< u32 >(pMaterialAnm) & (mBufferSize - 1);

    if (pMaterialAnm == nullptr) {
        return entryMatSort(pMatPacket);
    }

    pMatPacket->drawClear();
    pMatPacket->getShapePacket()->drawClear();

    if (mBuffer[slot] == nullptr) {
        mBuffer[slot] = pMatPacket;
        return 1;
    }

    J3DMatPacket *packet;
    for (packet = static_cast< J3DMatPacket * >(mBuffer[slot]); packet != nullptr; packet = static_cast< J3DMatPacket * >(packet->getNextPacket())) {
        if (packet->mMaterialAnm == pMaterialAnm) {
            packet->addShapePacket(pMatPacket->getShapePacket());
            return 0;
        }
    }

    pMatPacket->setNextPacket(mBuffer[slot]);
    mBuffer[slot] = pMatPacket;
    return 1;
}

int J3DDrawBuffer::entryZSort(J3DMatPacket *pMatPacket)
{
    pMatPacket->drawClear();
    pMatPacket->getShapePacket()->drawClear();

    Vec tmp;
    tmp.x = mZMtx[0][3];
    tmp.y = mZMtx[1][3];
    tmp.z = mZMtx[2][3];

    f32 value = -J3DCalcZValue(j3dSys.getViewMtx(), tmp);

    u32 index;
    if (mZNear + mZRatio < value) {
        if (mZFar - mZRatio > value) {
            index = value / mZRatio;
        } else {
            index = mBufferSize - 1;
        }
    } else {
        index = 0;
    }

    index = (mBufferSize - 1) - index;
    pMatPacket->setNextPacket(mBuffer[index]);
    mBuffer[index] = pMatPacket;
    return 1;
}

int J3DDrawBuffer::entryModelSort(J3DMatPacket *pMatPacket)
{
    pMatPacket->drawClear();
    pMatPacket->getShapePacket()->drawClear();

    if (mCallBackPacket != nullptr) {
        mCallBackPacket->addChildPacket(pMatPacket);
        return 1;
    }

    return 0;
}

int J3DDrawBuffer::entryInvalidSort(J3DMatPacket *pMatPacket)
{
    pMatPacket->drawClear();
    pMatPacket->getShapePacket()->drawClear();

    if (mCallBackPacket != nullptr) {
        mCallBackPacket->addChildPacket(pMatPacket->getShapePacket());
        return 1;
    }

    return 0;
}

int J3DDrawBuffer::entryNonSort(J3DMatPacket *pMatPacket)
{
    pMatPacket->drawClear();
    pMatPacket->getShapePacket()->drawClear();

    pMatPacket->setNextPacket(mBuffer[0]);
    mBuffer[0] = pMatPacket;
    return 1;
}

int J3DDrawBuffer::entryImm(J3DPacket *pPacket, u16 index)
{
    pPacket->setNextPacket(mBuffer[index]);
    mBuffer[index] = pPacket;
    return 1;
}

void J3DDrawBuffer::draw() const
{
    drawFunc func = drawFuncTable[mDrawMode];
    (this->*func)();
}

void J3DDrawBuffer::drawHead() const
{
    u32 size = mBufferSize;
    J3DPacket **buf = mBuffer;

    for (u32 i = 0; i < size; i++) {
        for (J3DPacket *packet = buf[i]; packet != nullptr; packet = packet->getNextPacket()) {
            packet->draw();
        }
    }
}

void J3DDrawBuffer::drawTail() const
{
    for (int i = mBufferSize - 1; i >= 0; i--) {
        for (J3DPacket *packet = mBuffer[i]; packet != nullptr; packet = packet->getNextPacket()) {
            packet->draw();
        }
    }
}

void J3DDrawBuffer::setCallBackPacket(J3DCallBackPacket *packet)
{
    mCallBackPacket = packet;
}

J3DDrawBuffer::sortFunc J3DDrawBuffer::sortFuncTable[6] = {
    &J3DDrawBuffer::entryMatSort,   &J3DDrawBuffer::entryMatAnmSort,  &J3DDrawBuffer::entryZSort,
    &J3DDrawBuffer::entryModelSort, &J3DDrawBuffer::entryInvalidSort, &J3DDrawBuffer::entryNonSort,
};

J3DDrawBuffer::drawFunc J3DDrawBuffer::drawFuncTable[2] = {
    &J3DDrawBuffer::drawHead,
    &J3DDrawBuffer::drawTail,
};

int J3DDrawBuffer::entryNum;
