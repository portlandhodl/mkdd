#ifndef ITEMSUCCESSION_H
#define ITEMSUCCESSION_H

#include "Sato/ItemObj.h"

// TODO: virtual and static functions

class ItemMultiBase
{
public:
    ItemMultiBase(ItemObj *link) { mObj = link; }
    ~ItemMultiBase() {}

    // Inline/Unused
    void getOffsetScale();
    void setDivestedStartPos(const JGeometry::TVec3f &);

    // Vtable 0
    virtual void ParamInit(const JGeometry::TVec3<s16> &) = 0;
    virtual void DoEquipScaleUp(f32) = 0;
    virtual void MoveEquip() = 0;
    virtual void InitMoveDivested() = 0;
    virtual void MoveDivested() = 0;

    ItemObj *mObj; // 4
};

class ItemJuggle : public ItemMultiBase
{
public:
    ItemJuggle(ItemObj *link) : ItemMultiBase(link) {}

    void doHandJuggleEquip(const JGeometry::TVec3f &, Mtx);
    void doMoveJuggleEquip(const JGeometry::TVec3f &, Mtx, JGeometry::TVec3f, f32);

    // Vtable 0
    virtual void ParamInit(const JGeometry::TVec3<s16> &); // 8
    virtual void DoEquipScaleUp(f32);                      // C
    virtual void MoveDivested();                           // 10
    virtual void InitMoveDivested();                       // 14
    virtual void MoveEquip();                              // 18

    static f32 sJuggleRotStartOffset;
    static f32 sJuggleRotRatio;
    static f32 sJuggleRotRatioSub;

    int _8;              // 8
    u8 _c;               // c
    u8 _d;               // d
    int _10;             // 10
    u8 _14;              // 14
    f32 _18;             // 18
    int _1c;             // 1c
    JGeometry::TVec3f _20; // 20
    JGeometry::TVec3f _2c; // 2c
    int _38;             // 38
    JGeometry::TVec3f _3c; // 3c
    JGeometry::TVec3f _48; // 48
    JGeometry::TVec3f _54; // 54
};

class ItemBuild : public ItemMultiBase
{
public:
    ItemBuild(ItemObj *link) : ItemMultiBase(link) {}

    void setFirstPos(JGeometry::TVec3f *, f32);
    void setVel(f32);
    void setPos(JGeometry::TVec3f *, f32);
    void setHeight(f32);
    void setRotate(Mtx);
    void setFirstRotate(Mtx);

    // Vtable
    virtual void ParamInit(const JGeometry::TVec3<s16> &); // 8
    virtual void DoEquipScaleUp(f32);                      // C
    virtual void MoveDivested();                           // 10
    virtual void InitMoveDivested();                       // 14
    virtual void MoveEquip();                              // 18

    JGeometry::TVec3f mOffset; // 8
    f32 mHeight;               // 14
    JGeometry::TVec3f _18;     // 18
    s16 mAngle;                // 24
    f32 mRadAngle;             // 28
    u8 mCount;                 // 2c
    u8 _2d;                    // 2d
    u8 mFirstPos;              // 2e
    ItemBuild *mPrev;          // 30
    u8 _34;                    // 34
    u8 _35;                    // 35
    u8 _36;                    // 36
    Mtx _38;                   // 38
};

class ItemObjSuc : public ItemObj
{
public:
    ItemObjSuc(u32 p1) : ItemObj(p1) {
        setSucMultiType(cMultiType_0);
    }

    void initJuggleEquip();
    void doJuggleEquip();
    void setInitBuildAppend();
    void initBuildEquip();
    void doBuildEquip();
    JSUListIterator<ItemObj> selectJuggleSucChild();
    void setSucMultiType(cMultiType);
    static f32 getRotAxisRatio();
    JSUListIterator<ItemObj> selectBuildSucChild();
    u8 IsAvailableDivested() const { return mJuggleFirst; }

    virtual void reset();
    virtual void initEquip();
    virtual void doEquip();
    virtual JSUListIterator<ItemObj> selectSucChild();    
    virtual void initDivested();
    virtual void doDivested();    
    virtual void initForceDisappear();
    virtual void doForceDisappear();

    static f32 sJuggleXLength;
    static f32 sJuggleRndAngle;
    static f32 sJuggleRndAngle2;
    static f32 sJuggleRotAxisRatio;
    static f32 sJuggleGravity;
    static f32 sJuggleGravityUpRatio;
    static f32 sJuggleAnmOverFrame;

    typedef void (ItemObjSuc::*EquipFunc)();
    typedef JSUListIterator<ItemObj> (ItemObjSuc::*SelectFunc)();

    EquipFunc mEquipInitFunc;  // 2b0
    EquipFunc mEquipDoFunc;    // 2bc
    SelectFunc mSelectSucFunc; // 2c8
    cMultiType mSucMultiType;  // 2d4
    int mPrevSucNum;           // 2d8
    f32 mJuggleXLength;        // 2dc
    f32 mJuggleGravity;        // 2e0
    int mJuggleIndex;          // 2e4
    int mJuggleFrameStep;      // 2e8
    bool mJuggleFinished;      // 2ec
    f32 mJuggleRot;            // 2f0
    u8 mJuggleFirst;           // 2f4
    f32 mJuggleGravityUp;      // 2f8
    u8 mUseHandAnm;            // 2fc
};

#endif // ITEMSUCCESSION_H
