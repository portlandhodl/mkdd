#include "Shiraiwa/Objects/MapObjDonkyRockGen.h"

#include "JSystem/JAudio/JASFakeMatch2.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Sato/GeographyObjMgr.h"















TMapObjDonkyRockGen::TMapObjDonkyRockGen(const CrsData::SObject &obj) : GeographyObj(obj) {
    mInterval = 0;
    mTimer = 0;
    mRocks = nullptr;

    JUT_ASSERT_MSG(mObjData->pathID != 0xffff, "TMapObjDonkyRockGen: Illegal Path ID");

    int count = obj.mParam1;
    if (count < 1) {
        count = 1;
    }
    makeRockObject(count);
}

TMapObjDonkyRockGen::~TMapObjDonkyRockGen() {
    delete[] mRocks;
}

void TMapObjDonkyRockGen::reset() {
    GeographyObj::resetObject();
    clrObjFlagCheckGeoHitting();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();

    mInterval = mObjData->mParam2;
    if (mInterval < 1) {
        mInterval = 60;
    }
    mTimer = 0;
}

void TMapObjDonkyRockGen::calc() {
    if (mTimer > mInterval) {
        if (appearRock()) {
            mTimer = 0;
        }
    } else {
        mTimer++;
    }
}

bool TMapObjDonkyRockGen::appearRock() {
    bool appeared = false;
    for (int i = 0; i < mCount; i++) {
        if (mRocks[i] != nullptr && mRocks[i]->_152 == 0) {
            mRocks[i]->start();
            appeared = true;
            break;
        }
    }
    return appeared;
}

void TMapObjDonkyRockGen::makeRockObject(int count) {
    mCount = count;
    mRocks = new TMapObjDonkyRock *[mCount];

    for (int i = 0; i < mCount; i++) {
        mRocks[i] = (TMapObjDonkyRock *)GetGeoObjMgr()->createSubObj(0x1197);
        mRocks[i]->setObjData(mObjData);
    }
}

