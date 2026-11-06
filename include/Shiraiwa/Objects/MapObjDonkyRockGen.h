#ifndef MAPOBJDONKYROCKGEN_H
#define MAPOBJDONKYROCKGEN_H

#include "Sato/GeographyObj.h"

class TMapObjDonkyRock : public GeographyObj {
public:
    TMapObjDonkyRock();
    virtual ~TMapObjDonkyRock();

    void start();
    void setObjData(const CrsData::SObject *);

    u8 _pad[6];
    u16 _152;
};

class TMapObjDonkyRockGen : public GeographyObj {
public:
    TMapObjDonkyRockGen(const CrsData::SObject &);
    virtual ~TMapObjDonkyRockGen();

    virtual void reset();
    virtual void calc();
    virtual void createColModel(J3DModelData *) {}

    bool appearRock();
    void makeRockObject(int);

private:
    int mInterval;             // 14c
    int mCount;                // 150
    unsigned int mTimer;       // 154
    TMapObjDonkyRock **mRocks; // 158
};

#endif
