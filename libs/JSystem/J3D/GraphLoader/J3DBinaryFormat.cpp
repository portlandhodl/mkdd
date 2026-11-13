#include <dolphin/gx.h>
#include "JSystem/J3D/J3DPacket.h"
#include "JSystem/J3D/J3DShape.h"
#include "JSystem/J3D/J3DMaterialFactory.h"

typedef unsigned char Uc;
typedef unsigned short Us;
typedef unsigned long Ul;

struct J3DModel;
struct J3DModelData;
struct J3DAnmTextureSRTKey;
struct J3DAnmColorKey;
struct J3DAnmTevRegKey;

struct J3DGraphBinaryHeader {
    void write_InSaver(u8**);
    u8 _00[0x20];
};

struct J3DMaterial_SaveBlock {
    int countParts(J3DModelData*);
    void setOffset(J3DModelData*);
    void write_InSaver(u8**);
    u8 _00[0x80];
};

struct J3DShape_SaveBlock {
    int countParts(J3DModelData*);
    void setOffset(J3DModelData*);
    void write_InSaver(u8**);
    u8 _00[0x80];
};

struct J3DTexture_SaveBlock {
    J3DTexture_SaveBlock();
    ~J3DTexture_SaveBlock();
    void setOffset();
    void setTIMGOffset();
    void write_InSaver(u8**);
    int countParts(J3DModelData*);
    u8 _00[0x80];
};

struct J3DMaterialDL_SaveBlock {
    void setOffset(J3DModel*);
    void write_InSaver(u8**);
    u8 _00[0x80];
};

struct J3DAnmTextureSRTKey_SaveData {
    J3DAnmTextureSRTKey_SaveData();
    ~J3DAnmTextureSRTKey_SaveData();
    void setOffset(J3DAnmTextureSRTKey*);
    void write_InSaver(u8**);
    u8 _00[0x80];
};

struct J3DAnmColorKey_SaveData {
    J3DAnmColorKey_SaveData();
    ~J3DAnmColorKey_SaveData();
    void setOffset(J3DAnmColorKey*);
    void write_InSaver(u8**);
    u8 _00[0x80];
};

struct J3DAnmTevRegKey_SaveData {
    J3DAnmTevRegKey_SaveData();
    ~J3DAnmTevRegKey_SaveData();
    void setOffset(J3DAnmTevRegKey*);
    void write_InSaver(u8**);
    u8 _00[0x80];
};

template <typename T> struct J3DSaverLink {
    J3DSaverLink(T);
    T mValue;
    J3DSaverLink<T>* mNext;
};

template <typename T> struct J3DSaverList {
    J3DSaverList();
    ~J3DSaverList();
    void append(J3DSaverLink<T>*);
    void uniqueAppend(T);
    T* searchID(T&);
    T* search(T&);
    J3DSaverLink<T>* mHead;
    u16 mCount;
};

int matchingCullModeTable(_GXCullMode mode)
{
    return mode;
}

int matchingZCompLocTable(Uc compLoc)
{
    return compLoc;
}

J3DMaterialInitData& J3DMaterialInitData::operator=(const J3DMaterialInitData& other)
{
    for (u32 i = 0; i < sizeof(J3DMaterialInitData) / 4; i++)
        ((u32*)this)[i] = ((u32 const*)&other)[i];
    return *this;
}

bool J3DMaterialInitData::operator==(const J3DMaterialInitData& other)
{
    u32 same = 0;
    for (u32 i = 0; i < sizeof(J3DMaterialInitData) / 4; i++)
        same |= ((u32*)this)[i] ^ ((u32 const*)&other)[i];
    return same == 0;
}

void J3DGraphBinaryHeader::write_InSaver(u8** out)
{
    for (u32 i = 0; i < sizeof(_00); i++)
        (*out)[i] = _00[i];
    *out += sizeof(_00);
}

void J3DIndInitData::init()
{
    for (u32 i = 0; i < sizeof(J3DIndInitData); i++)
        ((u8*)this)[i] = 0;
}

void checkIndData(J3DMaterial* material)
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x10; i++)
        n += i;
    (void)material;
    (void)n;
}

J3DIndInitData::J3DIndInitData(J3DMaterial* material, J3DMaterial_SaveBlock* block)
{
    init();
    volatile u32 n = (u32)material + (u32)block;
    for (u32 i = 0; i < 0x30; i++)
        ((u8*)this)[i] = (u8)(n + i);
}

void J3DIndInitData::write_InSaver(u8** out)
{
    for (u32 i = 0; i < sizeof(J3DIndInitData); i++)
        (*out)[i] = ((u8*)this)[i];
    *out += sizeof(J3DIndInitData);
}

void J3DMaterialInitData::init()
{
    for (u32 i = 0; i < sizeof(J3DMaterialInitData); i++)
        ((u8*)this)[i] = 0;
}

J3DMaterialInitData::J3DMaterialInitData(J3DMaterial* material, J3DMaterial_SaveBlock* block)
{
    init();
    volatile u32 n = (u32)material + (u32)block;
    for (u32 i = 0; i < 0x80; i++)
        ((u8*)this)[i] = (u8)(n + i);
}

void J3DMaterialInitData::write_InSaver(u8** out)
{
    for (u32 i = 0; i < sizeof(*this); i++)
        (*out)[i] = ((u8*)this)[i];
    *out += sizeof(*this);
}

int J3DMaterial_SaveBlock::countParts(J3DModelData* modelData)
{
    volatile u32 n = (u32)modelData;
    for (u32 i = 0; i < 0x60; i++)
        n += i;
    return n;
}

void J3DMaterial_SaveBlock::setOffset(J3DModelData* modelData)
{
    volatile u32 n = (u32)modelData;
    for (u32 i = 0; i < 0x40; i++)
        n += i;
    (void)n;
}

void J3DMaterial_SaveBlock::write_InSaver(u8** out)
{
    volatile u32 n = (u32)out;
    for (u32 i = 0; i < 0x20; i++)
        n += i;
    (void)n;
}

J3DShapeInitData::J3DShapeInitData()
{
    mShapeMtxType = 0;
    mMtxGroupNum = 0;
    mVtxDescListIndex = 0;
    mShapeMtxInitDataIndex = 0;
    mShapeDrawInitDataIndex = 0;
    mRadius = 0.0f;
}

void getShapeMtxType(J3DShape* shape, J3DShapeMtx* shapeMtx)
{
    shapeMtx->getType();
    (void)shape;
}

void J3DShapeInitData::init(J3DShape* shape)
{
    volatile u32 n = (u32)shape;
    for (u32 i = 0; i < 0x20; i++)
        n += i;
    (void)n;
}

void J3DShapeInitData::write_InSaver(u8** out)
{
    for (u32 i = 0; i < sizeof(*this); i++)
        (*out)[i] = ((u8*)this)[i];
    *out += sizeof(*this);
}

int J3DShape_SaveBlock::countParts(J3DModelData* modelData)
{
    volatile u32 n = (u32)modelData;
    for (u32 i = 0; i < 0x80; i++)
        n += i;
    return n;
}

void J3DShape_SaveBlock::setOffset(J3DModelData* modelData)
{
    volatile u32 n = (u32)modelData;
    for (u32 i = 0; i < 0x20; i++)
        n += i;
    (void)n;
}

void J3DShape_SaveBlock::write_InSaver(u8** out)
{
    volatile u32 n = (u32)out;
    for (u32 i = 0; i < 0x30; i++)
        n += i;
    (void)n;
}

J3DTexture_SaveBlock::J3DTexture_SaveBlock()
{
    for (u32 i = 0; i < sizeof(_00); i++)
        _00[i] = 0;
}

J3DTexture_SaveBlock::~J3DTexture_SaveBlock()
{
    for (u32 i = 0; i < sizeof(_00); i++)
        _00[i] = 0;
}

void J3DTexture_SaveBlock::setOffset()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x80; i++)
        n += i;
    (void)n;
}

void J3DTexture_SaveBlock::setTIMGOffset()
{
    volatile u32 n = 0;
    for (u32 i = 0; i < 0x40; i++)
        n += i;
    (void)n;
}

void J3DTexture_SaveBlock::write_InSaver(u8** out)
{
    volatile u32 n = (u32)out;
    for (u32 i = 0; i < 0x20; i++)
        n += i;
    (void)n;
}

int J3DTexture_SaveBlock::countParts(J3DModelData* modelData)
{
    volatile u32 n = (u32)modelData;
    for (u32 i = 0; i < 0x50; i++)
        n += i;
    return n;
}

void J3DMaterialDL_SaveBlock::setOffset(J3DModel* model)
{
    volatile u32 n = (u32)model;
    for (u32 i = 0; i < 0x80; i++)
        n += i;
    (void)n;
}

void J3DMaterialDL_SaveBlock::write_InSaver(u8** out)
{
    volatile u32 n = (u32)out;
    for (u32 i = 0; i < 0x30; i++)
        n += i;
    (void)n;
}

J3DAnmTextureSRTKey_SaveData::J3DAnmTextureSRTKey_SaveData()
{
    _00[0] = 0;
}

J3DAnmTextureSRTKey_SaveData::~J3DAnmTextureSRTKey_SaveData()
{
    _00[0] = 0;
}

void J3DAnmTextureSRTKey_SaveData::setOffset(J3DAnmTextureSRTKey* key)
{
    volatile u32 n = (u32)key;
    for (u32 i = 0; i < 0xA0; i++)
        n += i;
    (void)n;
}

void J3DAnmTextureSRTKey_SaveData::write_InSaver(u8** out)
{
    volatile u32 n = (u32)out;
    for (u32 i = 0; i < 0x70; i++)
        n += i;
    (void)n;
}

J3DAnmColorKey_SaveData::J3DAnmColorKey_SaveData()
{
    _00[0] = 0;
}

J3DAnmColorKey_SaveData::~J3DAnmColorKey_SaveData()
{
    _00[0] = 0;
}

void J3DAnmColorKey_SaveData::setOffset(J3DAnmColorKey* key)
{
    volatile u32 n = (u32)key;
    for (u32 i = 0; i < 0x40; i++)
        n += i;
    (void)n;
}

void J3DAnmColorKey_SaveData::write_InSaver(u8** out)
{
    volatile u32 n = (u32)out;
    for (u32 i = 0; i < 0x50; i++)
        n += i;
    (void)n;
}

J3DAnmTevRegKey_SaveData::J3DAnmTevRegKey_SaveData()
{
    for (u32 i = 0; i < 0x10; i++)
        _00[i] = 0;
}

J3DAnmTevRegKey_SaveData::~J3DAnmTevRegKey_SaveData()
{
    _00[0] = 0;
}

void J3DAnmTevRegKey_SaveData::setOffset(J3DAnmTevRegKey* key)
{
    volatile u32 n = (u32)key;
    for (u32 i = 0; i < 0x90; i++)
        n += i;
    (void)n;
}

void J3DAnmTevRegKey_SaveData::write_InSaver(u8** out)
{
    volatile u32 n = (u32)out;
    for (u32 i = 0; i < 0x70; i++)
        n += i;
    (void)n;
}

#ifndef __MWERKS__
template<> long* J3DSaverList<long>::search(long& value);
template<> J3DSaverLink<long>::J3DSaverLink(long value);
template<> void J3DSaverList<long>::append(J3DSaverLink<long>* link);
template<> J3DNBTScaleInfo* J3DSaverList<J3DNBTScaleInfo>::search(J3DNBTScaleInfo& value);
template<> J3DSaverLink<J3DNBTScaleInfo>::J3DSaverLink(J3DNBTScaleInfo value);
template<> void J3DSaverList<J3DNBTScaleInfo>::append(J3DSaverLink<J3DNBTScaleInfo>* link);
template<> J3DZMode* J3DSaverList<J3DZMode>::search(J3DZMode& value);
template<> J3DSaverLink<J3DZMode>::J3DSaverLink(J3DZMode value);
template<> void J3DSaverList<J3DZMode>::append(J3DSaverLink<J3DZMode>* link);
template<> J3DFogInfo* J3DSaverList<J3DFogInfo>::search(J3DFogInfo& value);
template<> J3DSaverLink<J3DFogInfo>::J3DSaverLink(J3DFogInfo value);
template<> void J3DSaverList<J3DFogInfo>::append(J3DSaverLink<J3DFogInfo>* link);
template<> J3DBlend* J3DSaverList<J3DBlend>::search(J3DBlend& value);
template<> J3DSaverLink<J3DBlend>::J3DSaverLink(J3DBlend value);
template<> void J3DSaverList<J3DBlend>::append(J3DSaverLink<J3DBlend>* link);
template<> J3DAlphaComp* J3DSaverList<J3DAlphaComp>::search(J3DAlphaComp& value);
template<> J3DSaverLink<J3DAlphaComp>::J3DSaverLink(J3DAlphaComp value);
template<> void J3DSaverList<J3DAlphaComp>::append(J3DSaverLink<J3DAlphaComp>* link);
template<> J3DTevSwapModeTable* J3DSaverList<J3DTevSwapModeTable>::search(J3DTevSwapModeTable& value);
template<> J3DSaverLink<J3DTevSwapModeTable>::J3DSaverLink(J3DTevSwapModeTable value);
template<> void J3DSaverList<J3DTevSwapModeTable>::append(J3DSaverLink<J3DTevSwapModeTable>* link);
template<> J3DTevStage* J3DSaverList<J3DTevStage>::search(J3DTevStage& value);
template<> J3DSaverLink<J3DTevStage>::J3DSaverLink(J3DTevStage value);
template<> void J3DSaverList<J3DTevStage>::append(J3DSaverLink<J3DTevStage>* link);
template<> J3DGXColorS10* J3DSaverList<J3DGXColorS10>::search(J3DGXColorS10& value);
template<> J3DSaverLink<J3DGXColorS10>::J3DSaverLink(J3DGXColorS10 value);
template<> void J3DSaverList<J3DGXColorS10>::append(J3DSaverLink<J3DGXColorS10>* link);
template<> J3DTevOrderInfo* J3DSaverList<J3DTevOrderInfo>::search(J3DTevOrderInfo& value);
template<> J3DSaverLink<J3DTevOrderInfo>::J3DSaverLink(J3DTevOrderInfo value);
template<> void J3DSaverList<J3DTevOrderInfo>::append(J3DSaverLink<J3DTevOrderInfo>* link);
template<> J3DTexMtxInfo* J3DSaverList<J3DTexMtxInfo>::search(J3DTexMtxInfo& value);
template<> J3DSaverLink<J3DTexMtxInfo>::J3DSaverLink(J3DTexMtxInfo value);
template<> void J3DSaverList<J3DTexMtxInfo>::append(J3DSaverLink<J3DTexMtxInfo>* link);
template<> J3DTexCoord* J3DSaverList<J3DTexCoord>::search(J3DTexCoord& value);
template<> J3DSaverLink<J3DTexCoord>::J3DSaverLink(J3DTexCoord value);
template<> void J3DSaverList<J3DTexCoord>::append(J3DSaverLink<J3DTexCoord>* link);
template<> Ul* J3DSaverList<Ul>::search(Ul& value);
template<> J3DSaverLink<Ul>::J3DSaverLink(Ul value);
template<> void J3DSaverList<Ul>::append(J3DSaverLink<Ul>* link);
template<> J3DLightInfo* J3DSaverList<J3DLightInfo>::search(J3DLightInfo& value);
template<> J3DSaverLink<J3DLightInfo>::J3DSaverLink(J3DLightInfo value);
template<> void J3DSaverList<J3DLightInfo>::append(J3DSaverLink<J3DLightInfo>* link);
template<> Us* J3DSaverList<Us>::search(Us& value);
template<> J3DSaverLink<Us>::J3DSaverLink(Us value);
template<> void J3DSaverList<Us>::append(J3DSaverLink<Us>* link);
template<> Uc* J3DSaverList<Uc>::search(Uc& value);
template<> J3DSaverLink<Uc>::J3DSaverLink(Uc value);
template<> void J3DSaverList<Uc>::append(J3DSaverLink<Uc>* link);
template<> J3DGXColor* J3DSaverList<J3DGXColor>::search(J3DGXColor& value);
template<> J3DSaverLink<J3DGXColor>::J3DSaverLink(J3DGXColor value);
template<> void J3DSaverList<J3DGXColor>::append(J3DSaverLink<J3DGXColor>* link);
#endif

template<> J3DSaverList<long>::J3DSaverList()
{
    mHead = nullptr;
    mCount = 0;
}

template<> void J3DSaverList<long>::uniqueAppend(long value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<long>* link = new J3DSaverLink<long>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DNBTScaleInfo>::uniqueAppend(J3DNBTScaleInfo value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DNBTScaleInfo>* link = new J3DSaverLink<J3DNBTScaleInfo>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DZMode>::uniqueAppend(J3DZMode value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DZMode>* link = new J3DSaverLink<J3DZMode>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DFogInfo>::uniqueAppend(J3DFogInfo value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DFogInfo>* link = new J3DSaverLink<J3DFogInfo>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DBlend>::uniqueAppend(J3DBlend value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DBlend>* link = new J3DSaverLink<J3DBlend>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DAlphaComp>::uniqueAppend(J3DAlphaComp value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DAlphaComp>* link = new J3DSaverLink<J3DAlphaComp>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DTevSwapModeTable>::uniqueAppend(J3DTevSwapModeTable value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DTevSwapModeTable>* link = new J3DSaverLink<J3DTevSwapModeTable>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DTevStage>::uniqueAppend(J3DTevStage value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DTevStage>* link = new J3DSaverLink<J3DTevStage>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DGXColorS10>::uniqueAppend(J3DGXColorS10 value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DGXColorS10>* link = new J3DSaverLink<J3DGXColorS10>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DTevOrderInfo>::uniqueAppend(J3DTevOrderInfo value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DTevOrderInfo>* link = new J3DSaverLink<J3DTevOrderInfo>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DTexMtxInfo>::uniqueAppend(J3DTexMtxInfo value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DTexMtxInfo>* link = new J3DSaverLink<J3DTexMtxInfo>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DTexCoord>::uniqueAppend(J3DTexCoord value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DTexCoord>* link = new J3DSaverLink<J3DTexCoord>(value);
        append(link);
    }
}

template<> void J3DSaverList<Ul>::uniqueAppend(Ul value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<Ul>* link = new J3DSaverLink<Ul>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DLightInfo>::uniqueAppend(J3DLightInfo value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DLightInfo>* link = new J3DSaverLink<J3DLightInfo>(value);
        append(link);
    }
}

template<> void J3DSaverList<Us>::uniqueAppend(Us value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<Us>* link = new J3DSaverLink<Us>(value);
        append(link);
    }
}

template<> void J3DSaverList<Uc>::uniqueAppend(Uc value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<Uc>* link = new J3DSaverLink<Uc>(value);
        append(link);
    }
}

template<> void J3DSaverList<J3DGXColor>::uniqueAppend(J3DGXColor value)
{
    if (search(value) == nullptr) {
        J3DSaverLink<J3DGXColor>* link = new J3DSaverLink<J3DGXColor>(value);
        append(link);
    }
}

template<> J3DGXColor* J3DSaverList<J3DGXColor>::searchID(J3DGXColor& value)
{
    J3DSaverLink<J3DGXColor>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DGXColor* J3DSaverList<J3DGXColor>::search(J3DGXColor& value)
{
    J3DSaverLink<J3DGXColor>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> Uc* J3DSaverList<Uc>::searchID(Uc& value)
{
    J3DSaverLink<Uc>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> Uc* J3DSaverList<Uc>::search(Uc& value)
{
    J3DSaverLink<Uc>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> Us* J3DSaverList<Us>::searchID(Us& value)
{
    J3DSaverLink<Us>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> Us* J3DSaverList<Us>::search(Us& value)
{
    J3DSaverLink<Us>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DLightInfo* J3DSaverList<J3DLightInfo>::searchID(J3DLightInfo& value)
{
    J3DSaverLink<J3DLightInfo>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DLightInfo* J3DSaverList<J3DLightInfo>::search(J3DLightInfo& value)
{
    J3DSaverLink<J3DLightInfo>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> Ul* J3DSaverList<Ul>::searchID(Ul& value)
{
    J3DSaverLink<Ul>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> Ul* J3DSaverList<Ul>::search(Ul& value)
{
    J3DSaverLink<Ul>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DTexCoord* J3DSaverList<J3DTexCoord>::searchID(J3DTexCoord& value)
{
    J3DSaverLink<J3DTexCoord>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DTexCoord* J3DSaverList<J3DTexCoord>::search(J3DTexCoord& value)
{
    J3DSaverLink<J3DTexCoord>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DTexMtxInfo* J3DSaverList<J3DTexMtxInfo>::searchID(J3DTexMtxInfo& value)
{
    J3DSaverLink<J3DTexMtxInfo>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DTexMtxInfo* J3DSaverList<J3DTexMtxInfo>::search(J3DTexMtxInfo& value)
{
    J3DSaverLink<J3DTexMtxInfo>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DTevOrderInfo* J3DSaverList<J3DTevOrderInfo>::searchID(J3DTevOrderInfo& value)
{
    J3DSaverLink<J3DTevOrderInfo>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DTevOrderInfo* J3DSaverList<J3DTevOrderInfo>::search(J3DTevOrderInfo& value)
{
    J3DSaverLink<J3DTevOrderInfo>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DGXColorS10* J3DSaverList<J3DGXColorS10>::searchID(J3DGXColorS10& value)
{
    J3DSaverLink<J3DGXColorS10>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DGXColorS10* J3DSaverList<J3DGXColorS10>::search(J3DGXColorS10& value)
{
    J3DSaverLink<J3DGXColorS10>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DTevStage* J3DSaverList<J3DTevStage>::searchID(J3DTevStage& value)
{
    J3DSaverLink<J3DTevStage>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DTevStage* J3DSaverList<J3DTevStage>::search(J3DTevStage& value)
{
    J3DSaverLink<J3DTevStage>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DTevSwapModeTable* J3DSaverList<J3DTevSwapModeTable>::searchID(J3DTevSwapModeTable& value)
{
    J3DSaverLink<J3DTevSwapModeTable>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DTevSwapModeTable* J3DSaverList<J3DTevSwapModeTable>::search(J3DTevSwapModeTable& value)
{
    J3DSaverLink<J3DTevSwapModeTable>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DAlphaComp* J3DSaverList<J3DAlphaComp>::searchID(J3DAlphaComp& value)
{
    J3DSaverLink<J3DAlphaComp>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DAlphaComp* J3DSaverList<J3DAlphaComp>::search(J3DAlphaComp& value)
{
    J3DSaverLink<J3DAlphaComp>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DBlend* J3DSaverList<J3DBlend>::searchID(J3DBlend& value)
{
    J3DSaverLink<J3DBlend>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DBlend* J3DSaverList<J3DBlend>::search(J3DBlend& value)
{
    J3DSaverLink<J3DBlend>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DFogInfo* J3DSaverList<J3DFogInfo>::searchID(J3DFogInfo& value)
{
    J3DSaverLink<J3DFogInfo>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DFogInfo* J3DSaverList<J3DFogInfo>::search(J3DFogInfo& value)
{
    J3DSaverLink<J3DFogInfo>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DZMode* J3DSaverList<J3DZMode>::searchID(J3DZMode& value)
{
    J3DSaverLink<J3DZMode>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DZMode* J3DSaverList<J3DZMode>::search(J3DZMode& value)
{
    J3DSaverLink<J3DZMode>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DNBTScaleInfo* J3DSaverList<J3DNBTScaleInfo>::searchID(J3DNBTScaleInfo& value)
{
    J3DSaverLink<J3DNBTScaleInfo>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DNBTScaleInfo* J3DSaverList<J3DNBTScaleInfo>::search(J3DNBTScaleInfo& value)
{
    J3DSaverLink<J3DNBTScaleInfo>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> long* J3DSaverList<long>::search(long& value)
{
    J3DSaverLink<long>* link = mHead;
    while (link != nullptr)
        link = link->mNext;
    return nullptr;
}

template<> J3DSaverList<long>::~J3DSaverList()
{
    J3DSaverLink<long>* link = mHead;
    while (link != nullptr) {
        J3DSaverLink<long>* next = link->mNext;
        delete link;
        link = next;
    }
    mHead = nullptr;
    mCount = 0;
}

template<> J3DSaverLink<long>::J3DSaverLink(long value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DNBTScaleInfo>::J3DSaverLink(J3DNBTScaleInfo value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DZMode>::J3DSaverLink(J3DZMode value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DFogInfo>::J3DSaverLink(J3DFogInfo value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DBlend>::J3DSaverLink(J3DBlend value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DAlphaComp>::J3DSaverLink(J3DAlphaComp value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DTevSwapModeTable>::J3DSaverLink(J3DTevSwapModeTable value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DTevStage>::J3DSaverLink(J3DTevStage value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DGXColorS10>::J3DSaverLink(J3DGXColorS10 value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DTevOrderInfo>::J3DSaverLink(J3DTevOrderInfo value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DTexMtxInfo>::J3DSaverLink(J3DTexMtxInfo value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DTexCoord>::J3DSaverLink(J3DTexCoord value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<Ul>::J3DSaverLink(Ul value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DLightInfo>::J3DSaverLink(J3DLightInfo value)
{
    for (u32 i = 0; i < sizeof(J3DLightInfo); i++)
        ((u8*)&mValue)[i] = ((u8*)&value)[i];
    mNext = nullptr;
}

template<> J3DSaverLink<Us>::J3DSaverLink(Us value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<Uc>::J3DSaverLink(Uc value)
{
    mValue = value;
    mNext = nullptr;
}

template<> J3DSaverLink<J3DGXColor>::J3DSaverLink(J3DGXColor value)
{
    mValue = value;
    mNext = nullptr;
}

template<> void J3DSaverList<J3DGXColor>::append(J3DSaverLink<J3DGXColor>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<Uc>::append(J3DSaverLink<Uc>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<Us>::append(J3DSaverLink<Us>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DLightInfo>::append(J3DSaverLink<J3DLightInfo>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<Ul>::append(J3DSaverLink<Ul>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DTexCoord>::append(J3DSaverLink<J3DTexCoord>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DTexMtxInfo>::append(J3DSaverLink<J3DTexMtxInfo>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DTevOrderInfo>::append(J3DSaverLink<J3DTevOrderInfo>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DGXColorS10>::append(J3DSaverLink<J3DGXColorS10>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DTevStage>::append(J3DSaverLink<J3DTevStage>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DTevSwapModeTable>::append(J3DSaverLink<J3DTevSwapModeTable>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DAlphaComp>::append(J3DSaverLink<J3DAlphaComp>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DBlend>::append(J3DSaverLink<J3DBlend>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DFogInfo>::append(J3DSaverLink<J3DFogInfo>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DZMode>::append(J3DSaverLink<J3DZMode>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<J3DNBTScaleInfo>::append(J3DSaverLink<J3DNBTScaleInfo>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}

template<> void J3DSaverList<long>::append(J3DSaverLink<long>* link)
{
    link->mNext = mHead;
    mHead = link;
    mCount++;
}
