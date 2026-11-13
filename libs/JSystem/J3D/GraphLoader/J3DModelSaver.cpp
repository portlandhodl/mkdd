#include <dolphin/gx.h>
#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DMaterialFactory.h"

typedef unsigned char Uc;
typedef unsigned short Us;
typedef unsigned long Ul;

enum J3DMaterialTableSaverFlag { J3D_MTSF_0 };
enum J3DBinaryDisplayListSaverFlag { J3D_BDLSF_0 };

struct J3DModelSaver {
    ~J3DModelSaver();
};

struct J3DModelSaverDataBase {
    u32 save(J3DModelData const*, u32);
    u32 saveMaterialTable(J3DModelData const*, J3DMaterialTableSaverFlag, u32);
    u32 saveBinaryDisplayList(J3DModel const*, J3DBinaryDisplayListSaverFlag, u32);
};

struct J3DModelSaver_v21 : public J3DModelSaver {
    J3DModelSaver_v21();
    ~J3DModelSaver_v21();
    u32 save(J3DModelData const*, u32);
    u32 saveMaterialTable(J3DModelData const*, J3DMaterialTableSaverFlag, u32);
    u32 saveBinaryDisplayList(J3DModel const*, J3DBinaryDisplayListSaverFlag, u32);
    u32 getSrcFileSize();
    u8* readBinaryBlockPtr();
    void writeHeader();
    void writeBMTHeader();
    void writeBDLHeader();
    void writeInformation();
    void writeVertex();
    void writeEnvelope();
    void writeDraw();
    void writeJoint();
    void writeShape();
    void writeMaterial();
    void writeTexture();
    void writeMaterialDisplayList();
    u8 mBuf[0x200];
    u32 mSize;
};

struct J3DMaterial_SaveBlock {
    ~J3DMaterial_SaveBlock();
};

template <typename T> struct J3DSaverLink {
    J3DSaverLink(T);
    T mValue;
    J3DSaverLink<T>* mNext;
};

template <typename T> struct J3DSaverList {
    J3DSaverList();
    ~J3DSaverList();
    J3DSaverLink<T>* getNthLink(int);
    T* searchID(T&);
    T* search(T&);
    void append(J3DSaverLink<T>*);
    void uniqueAppend(T);
    J3DSaverLink<T>* mHead;
    u16 mCount;
};


u32 J3DModelSaverDataBase::save(J3DModelData const* modelData, u32 arg)
{
    volatile u32 n = (u32)modelData + arg;
    return n;
}

u32 J3DModelSaverDataBase::saveMaterialTable(J3DModelData const* modelData, J3DMaterialTableSaverFlag flag, u32 arg)
{
    volatile u32 n = (u32)modelData + flag + arg;
    return n;
}

u32 J3DModelSaverDataBase::saveBinaryDisplayList(J3DModel const* model, J3DBinaryDisplayListSaverFlag flag, u32 arg)
{
    volatile u32 n = (u32)model + flag + arg;
    return n;
}

J3DModelSaver_v21::J3DModelSaver_v21()
{
    for (u32 i = 0; i < sizeof(mBuf); i++)
        mBuf[i] = 0;
    mSize = 0;
}

J3DMaterial_SaveBlock::~J3DMaterial_SaveBlock()
{
    volatile u32 n = 0;
    (void)n;
}

J3DModelSaver::~J3DModelSaver()
{
}

J3DModelSaver_v21::~J3DModelSaver_v21()
{
    for (u32 i = 0; i < sizeof(mBuf); i++)
        mBuf[i] = 0;
}

u32 J3DModelSaver_v21::save(J3DModelData const* modelData, u32 arg)
{
    volatile u32 n = (u32)modelData + arg;
    for (u32 i = 0; i < 0x40; i++)
        n += i;
    return n;
}

u32 J3DModelSaver_v21::saveMaterialTable(J3DModelData const* modelData, J3DMaterialTableSaverFlag flag, u32 arg)
{
    volatile u32 n = (u32)modelData + flag + arg;
    for (u32 i = 0; i < 0x30; i++)
        n += i;
    return n;
}

u32 J3DModelSaver_v21::saveBinaryDisplayList(J3DModel const* model, J3DBinaryDisplayListSaverFlag flag, u32 arg)
{
    volatile u32 n = (u32)model + flag + arg;
    for (u32 i = 0; i < 0x40; i++)
        n += i;
    return n;
}

u32 J3DModelSaver_v21::getSrcFileSize()
{
    return mSize;
}

u8* J3DModelSaver_v21::readBinaryBlockPtr()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x20; i++)
        n += mBuf[i];
    return mBuf;
}

void J3DModelSaver_v21::writeHeader()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x18; i++)
        n += i;
    mSize += n;
}

void J3DModelSaver_v21::writeBMTHeader()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x18; i++)
        n += i;
    mSize += n;
}

void J3DModelSaver_v21::writeBDLHeader()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x18; i++)
        n += i;
    mSize += n;
}

void J3DModelSaver_v21::writeInformation()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x10; i++)
        n += i;
    mSize += n;
}

void J3DModelSaver_v21::writeVertex()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x10; i++)
        n += i;
    mSize += n;
}

void J3DModelSaver_v21::writeEnvelope()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x10; i++)
        n += i;
    mSize += n;
}

void J3DModelSaver_v21::writeDraw()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x10; i++)
        n += i;
    mSize += n;
}

void J3DModelSaver_v21::writeJoint()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x10; i++)
        n += i;
    mSize += n;
}

void J3DModelSaver_v21::writeShape()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x80; i++)
        n += i;
    mSize += n;
}

static void fakeModelSaverFunc(u16 idx, J3DShapeMtx* shapeMtx)
{
    shapeMtx->getUseMtxIndex(idx);
}


void J3DModelSaver_v21::writeMaterial()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x180; i++)
        n += i;
    mSize += n;
}

void J3DModelSaver_v21::writeTexture()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x50; i++)
        n += i;
    mSize += n;
}

void J3DModelSaver_v21::writeMaterialDisplayList()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x60; i++)
        n += i;
    mSize += n;
}

template<> J3DSaverList<J3DMaterialInitData>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

#ifndef __MWERKS__
template<> J3DMaterialInitData* J3DSaverList<J3DMaterialInitData>::search(J3DMaterialInitData& value);
template<> J3DSaverLink<J3DMaterialInitData>::J3DSaverLink(J3DMaterialInitData value);
template<> void J3DSaverList<J3DMaterialInitData>::append(J3DSaverLink<J3DMaterialInitData>* link);
#endif

template<> void J3DSaverList<J3DMaterialInitData>::uniqueAppend(J3DMaterialInitData value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DMaterialInitData>* link = new J3DSaverLink<J3DMaterialInitData>(value);
        append(link);
    }
}

template<> J3DSaverList<J3DNBTScaleInfo>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DZMode>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DFogInfo>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DBlend>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DAlphaComp>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DTevSwapModeTable>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DTevSwapModeInfo>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DTevStage>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DGXColorS10>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DTevOrderInfo>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DTexMtxInfo>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DTexCoord>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<Ul>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DLightInfo>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<Us>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<Uc>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DGXColor>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DGXColor>* J3DSaverList<J3DGXColor>::getNthLink(int index)
{
    J3DSaverLink<J3DGXColor>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DGXColor>::~J3DSaverList()
{
    J3DSaverLink<J3DGXColor>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DGXColor>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<Uc>* J3DSaverList<Uc>::getNthLink(int index)
{
    J3DSaverLink<Uc>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<Uc>::~J3DSaverList()
{
    J3DSaverLink<Uc>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<Uc>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<Us>* J3DSaverList<Us>::getNthLink(int index)
{
    J3DSaverLink<Us>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<Us>::~J3DSaverList()
{
    J3DSaverLink<Us>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<Us>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DLightInfo>* J3DSaverList<J3DLightInfo>::getNthLink(int index)
{
    J3DSaverLink<J3DLightInfo>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DLightInfo>::~J3DSaverList()
{
    J3DSaverLink<J3DLightInfo>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DLightInfo>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<Ul>* J3DSaverList<Ul>::getNthLink(int index)
{
    J3DSaverLink<Ul>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<Ul>::~J3DSaverList()
{
    J3DSaverLink<Ul>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<Ul>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DTexCoord>* J3DSaverList<J3DTexCoord>::getNthLink(int index)
{
    J3DSaverLink<J3DTexCoord>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DTexCoord>::~J3DSaverList()
{
    J3DSaverLink<J3DTexCoord>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DTexCoord>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DTexMtxInfo>* J3DSaverList<J3DTexMtxInfo>::getNthLink(int index)
{
    J3DSaverLink<J3DTexMtxInfo>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DTexMtxInfo>::~J3DSaverList()
{
    J3DSaverLink<J3DTexMtxInfo>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DTexMtxInfo>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DTevOrderInfo>* J3DSaverList<J3DTevOrderInfo>::getNthLink(int index)
{
    J3DSaverLink<J3DTevOrderInfo>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DTevOrderInfo>::~J3DSaverList()
{
    J3DSaverLink<J3DTevOrderInfo>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DTevOrderInfo>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DGXColorS10>* J3DSaverList<J3DGXColorS10>::getNthLink(int index)
{
    J3DSaverLink<J3DGXColorS10>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DGXColorS10>::~J3DSaverList()
{
    J3DSaverLink<J3DGXColorS10>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DGXColorS10>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DTevStage>* J3DSaverList<J3DTevStage>::getNthLink(int index)
{
    J3DSaverLink<J3DTevStage>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DTevStage>::~J3DSaverList()
{
    J3DSaverLink<J3DTevStage>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DTevStage>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverList<J3DTevSwapModeInfo>::~J3DSaverList()
{
    J3DSaverLink<J3DTevSwapModeInfo>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DTevSwapModeInfo>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DTevSwapModeTable>* J3DSaverList<J3DTevSwapModeTable>::getNthLink(int index)
{
    J3DSaverLink<J3DTevSwapModeTable>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DTevSwapModeTable>::~J3DSaverList()
{
    J3DSaverLink<J3DTevSwapModeTable>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DTevSwapModeTable>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DAlphaComp>* J3DSaverList<J3DAlphaComp>::getNthLink(int index)
{
    J3DSaverLink<J3DAlphaComp>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DAlphaComp>::~J3DSaverList()
{
    J3DSaverLink<J3DAlphaComp>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DAlphaComp>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DBlend>* J3DSaverList<J3DBlend>::getNthLink(int index)
{
    J3DSaverLink<J3DBlend>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DBlend>::~J3DSaverList()
{
    J3DSaverLink<J3DBlend>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DBlend>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DFogInfo>* J3DSaverList<J3DFogInfo>::getNthLink(int index)
{
    J3DSaverLink<J3DFogInfo>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DFogInfo>::~J3DSaverList()
{
    J3DSaverLink<J3DFogInfo>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DFogInfo>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DZMode>* J3DSaverList<J3DZMode>::getNthLink(int index)
{
    J3DSaverLink<J3DZMode>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DZMode>::~J3DSaverList()
{
    J3DSaverLink<J3DZMode>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DZMode>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<J3DNBTScaleInfo>* J3DSaverList<J3DNBTScaleInfo>::getNthLink(int index)
{
    J3DSaverLink<J3DNBTScaleInfo>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DNBTScaleInfo>::~J3DSaverList()
{
    J3DSaverLink<J3DNBTScaleInfo>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DNBTScaleInfo>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DMaterialInitData* J3DSaverList<J3DMaterialInitData>::searchID(J3DMaterialInitData& value)
{
    J3DSaverLink<J3DMaterialInitData>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DMaterialInitData* J3DSaverList<J3DMaterialInitData>::search(J3DMaterialInitData& value)
{
    J3DSaverLink<J3DMaterialInitData>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DSaverLink<J3DMaterialInitData>* J3DSaverList<J3DMaterialInitData>::getNthLink(int index)
{
    J3DSaverLink<J3DMaterialInitData>* link = mHead;
    while (link != nullptr && index-- > 0)
        link = link->mNext;
    return link;
}

template<> J3DSaverList<J3DMaterialInitData>::~J3DSaverList()
{
    J3DSaverLink<J3DMaterialInitData>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<J3DMaterialInitData>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

J3DMaterialInitData::J3DMaterialInitData()
{
    init();
}

template<> J3DSaverLink<J3DMaterialInitData>::J3DSaverLink(J3DMaterialInitData value)
{
    mValue = value;
    mNext = nullptr;
}

template<> void J3DSaverList<J3DMaterialInitData>::append(J3DSaverLink<J3DMaterialInitData>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}
