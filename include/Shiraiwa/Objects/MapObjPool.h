#ifndef MAPOBJPOOL_H
#define MAPOBJPOOL_H

#include "JSystem/JKernel/JKRDisposer.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "JSystem/JSupport/JSUList.h"
#include "Kaneshige/Course/CrsData.h"

#include "Sato/GeographyObj.h"
#include "Sato/J3DAnmObject.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "types.h"

class TMapObjPool;

class TPoolSupervisor : public GeoObjSupervisor {
public:
    TPoolSupervisor();
    ~TPoolSupervisor();
    void calc();
    void entry(TMapObjPool *);
    f32 getHeight(f32 x, f32 y);
    // Inline
    void reset() {} // 0x802c2afc
private:
    JSUPtrList mEntryList; // 0x24
};

class TMapObjPool : public TMapObjHioNode, JKRDisposer {
public:
    TMapObjPool(const CrsData::SObject &); // 0x802c204c
    virtual void loadAnimation(); // 0x802c2234
    virtual void createModel(JKRSolidHeap *, u32); // 0x802c2298
    virtual void update(); // 0x802c22e0
    virtual void reset();  // 0x802c2330
    virtual void calc();   // 0x802c2418

    static TPoolSupervisor*sSupervisor; // 0x80417088
    static J3DAnmTextureSRTKey *sPoolBtkAnm; // 0x8041708c
    static u8 sUseBtk; // 0x80417090
    // Inline/Unused
    void getUpDir(JGeometry::TVec3f *);
    f32 getHeight(f32, f32);
    // Inline
    virtual ~TMapObjPool(); // 0x802c2134
    virtual const char *getBmdFileName(); // 0x802c2b00
    virtual TPoolSupervisor *getSupervisor() { return sSupervisor; }

    static TPoolSupervisor *getSupervisor() { return sSupervisor; }
private: // TODO
    friend class TPoolSupervisor;

    JSUPtrLink mEntryLink;   // 0x164
    J3DAnmObjMaterial mBtk;  // 0x174
    f32 mBlend;              // 0x194
}; // class MapObjPool

#endif // MAPOBJPOOL_H
