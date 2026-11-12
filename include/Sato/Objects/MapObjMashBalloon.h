#ifndef MAPOBJMASHBALLOON_H
#define MAPOBJMASHBALLOON_H

#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/J3DAnmObject.h"
#include "Shiraiwa/Coord3D.h"

class TMapObjSkyShip : public TMapObjHioNode
{
public:
    TMapObjSkyShip(const CrsData::SObject &); // 0x80298b48
    virtual ~TMapObjSkyShip();                // 0x80298c30

    virtual void reset();                                                                       // 0x80298d04
    virtual const char *getBmdFileName();                                                       // 0x80298f10
    virtual const char *getShadowBmdFileName();                                                 // 0x80298f38
    virtual void calc();                                                                        // 0x80298f60
    virtual void createColModel(J3DModelData *) {}                                              // 0x8029a3d8
    virtual ShadowModel::ShadowKind getShadowKind() const { return ShadowModel::cShadowKind_SkyShip; } // 0x8029a3dc
    virtual f32 getAmplitude() { return sAmplitude; }                                           // 0x80299260
    virtual s16 getAmpTimer() { return sAmpTimer; }                                             // 0x80299258

    void moveShadowRot();                           // 0x802990bc
    void getNodeDir(u16, JGeometry::TVec3f *);      // 0x80299170
    void aimFront();                                // UNUSED
    void localMove();                               // 0x802991cc

    static f32 sAmplitude; // 0x80415198
    static s16 sAmpTimer;  // 0x8041519c

protected:
    u16 mAmpAngle;            // 0x14c
    f32 mBaseY;               // 0x150
    f32 mPathY;               // 0x154
    TPathMove *mPathMove;     // 0x158
    TFreeRotate *mFreeRotate; // 0x15c
}; // Size: 0x160

class TMapObjMashBalloon : public TMapObjSkyShip
{
public:
    TMapObjMashBalloon(const CrsData::SObject &obj) : TMapObjSkyShip(obj) {}
    virtual ~TMapObjMashBalloon() {}

    virtual void reset();                                                                   // 0x80299a48
    virtual const char *getBmdFileName() { return "/Objects/mash_balloon.bmd"; }            // 0x8029a3b0
    virtual const char *getShadowBmdFileName() { return "/Objects/mash_balloon_shadow.bmd"; } // 0x8029a3bc
    virtual f32 getAmplitude() { return sAmplitude; }                                       // 0x8029a3c8
    virtual s16 getAmpTimer() { return sAmpTimer; }                                         // 0x8029a3d0

    static f32 sAmplitude; // 0x804151ac
    static s16 sAmpTimer;  // 0x804151b0

private:
    JGeometry::TPos3f mRotBase; // 0x160
}; // Size: 0x190

class TMapObjPathYoshiHeli : public TMapObjSkyShip
{
public:
    TMapObjPathYoshiHeli(const CrsData::SObject &obj) : TMapObjSkyShip(obj) { createSoundMgr(); }
    virtual ~TMapObjPathYoshiHeli() {}

    virtual void loadAnimation();                                               // 0x802999c4
    virtual void createModel(JKRSolidHeap *, u32);                              // 0x80299a0c
    virtual void reset();                                                       // 0x80299268
    virtual void calc();                                                        // 0x802994b0
    virtual void update();                                                      // 0x80299980
    virtual const char *getBmdFileName() { return "/Objects/YoshiHeliB.bmd"; } // 0x8029a508
    virtual const char *getShadowBmdFileName() { return nullptr; }              // 0x8029a514
    virtual f32 getAmplitude() { return sAmplitude; }                           // 0x8029a51c
    virtual s16 getAmpTimer() { return sAmpTimer; }                             // 0x8029a524

    void aimVelDir(); // 0x8029964c

    static f32 sAmplitude;                           // 0x804151a0
    static s16 sAmpTimer;                            // 0x804151a4
    static f32 sReachDistance;                       // 0x804151a8
    static J3DAnmTextureSRTKey *sPathYoshiHeliBtkAnm; // 0x80416e30

private:
    J3DAnmObjMaterial mAnmObj; // 0x160
}; // Size: 0x180

#endif
