#ifndef TESTSTRING_H
#define TESTSTRING_H

#include "JSystem/JGeometry/Vec.h"
#include "JSystem/JKernel/JKRDisposer.h"
#include "JSystem/JSupport/JSUList.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "Sato/GeographyObj.h"
#include "Sato/Objects/GeoItemBox.h"
#include "Sato/StringObj.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "types.h"

class TTestString;

class TTestStrSupervisor : public GeoObjSupervisor {
public:
    TTestStrSupervisor()
    {
        mList.initiate();
        mTexture = new JUTTexture();
    }
    virtual ~TTestStrSupervisor();

    virtual void reset() {}
    virtual void calc() {}
    virtual void load();
    virtual void createModel(JKRSolidHeap *, u32);
    void drawAll(u32);

    static f32 sStringOffset[18];
    static f32 sStringTexOffset[12];

    JSUList<TTestString> mList; // 0x24
    JUTTexture *mTexture;       // 0x30
}; // Size: 0x34

class TTestStrBarrel : public GeographyObj {
public:
    TTestStrBarrel(u32 id = 0xe0d) : GeographyObj(id) {}
    virtual ~TTestStrBarrel() {}

    virtual void reset();
    virtual void calc();
    virtual u32 getSoundID() const;
    virtual const char *getBmdFileName() { return "/Objects/TestHimo1Ukiwa.bmd"; }
    virtual const char *getShadowBmdFileName() { return "/Objects/TestHimo1Shadow.bmd"; }
    virtual void createColModel(J3DModelData *);

    void moveShadowModel();
    void checkItemHitting();
    void doKartColCallBack(int);
    void vibrate(int);

    static f32 sItemVelScale;
    static f32 sKartVelScale;

    TTestString *mString; // 0x14c
}; // Size: 0x150

class TTestStrItem : public GeographyObj {
public:
    TTestStrItem(u32 id = 0xe0b) : GeographyObj(id) {}
    virtual ~TTestStrItem() {}

    virtual void reset();
    virtual void calc();
    virtual const char *getBmdFileName() { return GeoItemBox::getBmdName(); }
    virtual const char *getShadowBmdFileName() { return GeoItemBox::getShadowBmdName(); }
    virtual void createColModel(J3DModelData *);
    virtual void doKartColCallBack(int);

    TTestString *mString;            // 0x14c
    u16 mTimer;                      // 0x150
    u8 _152[2];                      // 0x152
    JPABaseEmitter *mEmitters[2];    // 0x154
}; // Size: 0x15c

class TTestString : public GeographyObj, public JKRDisposer {
public:
    TTestString(const CrsData::SObject &);
    virtual ~TTestString();

    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual TTestStrSupervisor *getSupervisor() { return sSupervisor; }

    void load(J3DModelData *);
    void attack(const JGeometry::TVec3f &);
    void attack(const JGeometry::TVec3f &, const JGeometry::TVec3f &);
    void draw(u32);
    void drawNode(f32, JGeometry::TVec3f &, JGeometry::TVec3f &, u32);
    void drawCrevicePyramid(JGeometry::TVec3f &, JGeometry::TVec3f &, JGeometry::TVec3f &);
    void drawPrism(JGeometry::TVec3f &, JGeometry::TVec3f &, JGeometry::TVec3f &, JGeometry::TVec3f &, JGeometry::TVec3f &);
    void getUpDir(JGeometry::TVec3f *);
    void getPoint(JGeometry::TVec3f *);
    void getPointDir(JGeometry::TVec3f *);
    void setParameters();

    static TTestStrSupervisor *sSupervisor;
    static f32 sAdjustRatio;
    static f32 sAirFric;
    static f32 sGravity;
    static f32 sWidth;
    static f32 sDownVel;

private:
    JSULink<TTestString> mLink;       // 0x164
    ExStringNodeManager *mNodeMgr;    // 0x174
    u32 _178;                         // 0x178
    TTestStrBarrel *mBarrel;          // 0x17c
    u8 mNodeNum;                      // 0x180
    u8 _181[3];                       // 0x181
    f32 mLength;                      // 0x184
    f32 *mNodeLengths;                // 0x188
    JGeometry::TVec3f mPoints[3];     // 0x18c
}; // Size: 0x1b0

#endif
