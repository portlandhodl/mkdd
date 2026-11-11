#ifndef GIRECORD_H
#define GIRECORD_H

#include "Osako/TARecord.h"

class GIRecord {
public:
    GIRecord() {}
    GIRecord(TARecord &record);
    bool isValid();

    TARecord &getRecord() { return mRecord; } // fabricated

private:
    TARecord mRecord;
    u32 mTick;
    char mSignature[3]; // GID
    u8 mChecksum;
};

#endif
