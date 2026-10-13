#ifndef ACCESSORY_H
#define ACCESSORY_H

#include "types.h"

class DriverModel; // declared for the sake of the AccessoryMgr API
class JKRSolidHeap;

class AccessoryMgr {
public:
    void init(DriverModel *);
    void update();
    void createModel(DriverModel *, JKRSolidHeap *, u32, u32);
    void setCurrentViewNo(u32 viewNo);
    void simpleDraw(u32 viewNo);

    static AccessoryMgr *ptr();
    static AccessoryMgr *mspAccessoryMgr;
};

#endif
