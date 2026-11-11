#include "Shiraiwa/Objects/SMAButterfly.h"

#include "Sato/GeographyObjMgr.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

TSmallAnimal::TSmallAnimal(u32 id) : TMapObjHioNode(id)
{
    mOrigin = nullptr;
    clrAllCheckKartHitFlag();
    clrObjFlagCheckItemHitting();
}

TSmallAnimal::~TSmallAnimal() {}

void TSmallAnimal::reset()
{
    resetObject();
}

TSmallAnimalGen::TSmallAnimalGen(const CrsData::SObject &obj, u32 id) : TMapObjHioNode(obj)
{
    mObjects = nullptr;
    mCount = 0;
    mId = id;
    int num = obj.mParam1 & 0xffff;
    if (num == 0) {
        num = 1;
    }
    makeObjects(num);
}

TSmallAnimalGen::~TSmallAnimalGen()
{
    for (int i = 0; i < mCount; i++) {
        delete mObjects[i];
    }
    delete[] mObjects;
}

void TSmallAnimalGen::reset() {}

TSmallAnimal *TSmallAnimalGen::getNthObject(u16 n)
{
    JUT_MINMAX_ASSERT(0, n, mCount);
    return mObjects[n];
}

void TSmallAnimalGen::makeObjects(u16 num)
{
    mCount = num;
    mObjects = new TSmallAnimal *[mCount];
    for (int i = 0; i < mCount; i++) {
        mObjects[i] = (TSmallAnimal *)GeographyObjManager::gMgr->createSubObj(mId);
        mObjects[i]->setOrigin(&mPos);
    }
}
