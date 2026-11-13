#ifndef MAPOBJAWARDITEMBOX_H
#define MAPOBJAWARDITEMBOX_H

#include <JSystem/JKernel/JKRDisposer.h>
#include <JSystem/JSupport/JSUList.h>

#include "Sato/GeographyObj.h"
#include "Sato/Objects/GeoItemBox.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjAwardItemBox;
class TMapObjAwardCup;

class TAwardItemBoxSupervisor : public GeoObjSupervisor {
public:
    TAwardItemBoxSupervisor();
    void entry(TMapObjAwardItemBox *);
    void skip();
    bool isRollState();

    virtual ~TAwardItemBoxSupervisor() {} // 0x802cd57c
    void reset() {} // 0x802cd61c
    void calc() {} // 0x802cd620

private:
    JSUList<TMapObjAwardItemBox> mList; // 0x24
}; // Size: 0x30

class TMapObjAwardItemBox : public TMapObjHioNode, public StateObserver, public JKRDisposer {
public:
    TMapObjAwardItemBox(const CrsData::SObject &); // 0x802cb76c
    void createColModel(J3DModelData *); // 0x802cba78
    void reset(); // 0x802cbaa0
    void skip(); // 0x802cbc34
    void doKartColCallBack(int); // 0x802cbd10
    void calc(); // 0x802cbf40
    void initFunc_Appear(); // 0x802cbf64
    void doFunc_Appear(); // 0x802cbfa4
    void initFunc_Roll(); // 0x802cc050
    void doFunc_Roll(); // 0x802cc0f4
    void rotAnimation(); // 0x802cc114
    void initFunc_Disappear(); // 0x802cc354
    void doFunc_Disappear(); // 0x802cc370
    void initFunc_Hide(); // 0x802cc428
    void doFunc_Hide(); // 0x802cc450

    static const int scIDTable[18]; // 0x80385450
    static StateFuncSet<TMapObjAwardItemBox> sTable[4]; // 0x803a9e10
    static f32 sRollSpeed; // 0x80415480
    static s16 sAppearFrame; // 0x80415484
    static s16 sDisappearFrame; // 0x80415486
    static TAwardItemBoxSupervisor *sSupervisor; // 0x804170f8

    // Inline
    virtual GeoObjSupervisor *getSupervisor() { return sSupervisor; } // 0x802cb998
#ifndef __MWERKS__
    virtual GeoObjSupervisor *getSupervisorVirtual() { return sSupervisor; }
#endif
    virtual ~TMapObjAwardItemBox(); // 0x802cb9a0
    void MoveExec(); // 0x802cbed4
    void InitExec(); // 0x802cbe68
    virtual const char *getBmdFileName() { return GeoItemBox::getBmdName(); } // 0x802cd6cc
    virtual const char *getShadowBmdFileName() { return GeoItemBox::getShadowBmdName(); } // 0x802cd6ec

    friend class TAwardItemBoxSupervisor;
    JSULink<TMapObjAwardItemBox> mLink; // 0x170
private:
    JPABaseEmitter *mEmitter[2]; // 0x180
    TMapObjAwardCup *mCup;       // 0x188
    JGeometry::TVec3f mModelZDir; // 0x18c
    JGeometry::TVec3f mModelYDir; // 0x198
    JGeometry::TVec3f mModelXDir; // 0x1a4
    f32 mRotAngle0;              // 0x1b0
    f32 mRotAngle1;              // 0x1b4
    s16 mRandDir;                // 0x1b8
}; // Size: 0x1bc

#endif // MAPOBJAWARDITEMBOX_H
