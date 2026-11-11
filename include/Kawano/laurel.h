#ifndef LAUREL_H
#define LAUREL_H

#include "JSystem/JKernel/JKRDisposer.h"
#include "Kaneshige/ExModel.h"
#include "Kawano/driverObj.h"
#include "types.h"

class DriverModel;
class JKRSolidHeap;
class J3DModelData;

class Laurel : public JKRDisposer, public ExModel {
public:
    Laurel(DriverModel *);
    virtual ~Laurel() {}

    void updateOffset();
    virtual void update(u16);
    virtual bool createModel(JKRSolidHeap *, u32, u32);
    virtual void setCurrentViewNo(u32);

    DriverModel *mDriver; // 0xa4
}; // Size: 0xa8

class LaurelMgr : public JKRDisposer, public DriverObj {
public:
    LaurelMgr();
    virtual ~LaurelMgr();

    static LaurelMgr *ptr();
    void init(DriverModel *);
    void reset();
    void update(DriverModel *);
    void createModel(DriverModel *, JKRSolidHeap *, u32, u32);
    void setCurrentViewNo(DriverModel *, u32);

    static const char *mscaLaurelBmdFileName;
    static LaurelMgr *mspLaurelMgr;

    J3DModelData *mModelData; // 0x30
    s32 mCount;               // 0x34
    Laurel *mLaurels[16];     // 0x38
}; // Size: 0x78

#endif // LAUREL_H
