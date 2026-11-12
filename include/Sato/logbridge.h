#ifndef LOGBRIDGE_H
#define LOGBRIDGE_H

// NOTE: logbridge.cpp's original translation unit was compiled against a version of
// Sato/ObjCollision.h whose ObjColBlock had an inline constructor passing CKind 5
// (see StringBridge::createColModel). The repo's ObjCollision.h is read-only, so a
// verbatim copy lives below (with that constructor restored) and the real header's
// include guard is pre-defined so GeographyObj.h's include is a no-op.

#include "JSystem/J3D/J3DModel.h"
#include "JSystem/JGeometry/Vec.h"
#include "Kaneshige/Course/CrsGround.h"
#include "Sato/stMath.h"

class ObjColCylinder;

class ObjColBase
{
public:
    enum CKind
    {
        SPHERE,
        CYLINDER,
        CUBE,
        KIND_3, // used for jump1
    };

    void initialize();
    ObjColBase(J3DModelData *, CKind);
    ObjColBase(J3DModelData *, f32, CKind);
    ObjColBase(J3DModelData *, JGeometry::TVec3f, CKind);
    ObjColBase(f32, f32, CKind);
    void initRadius(J3DModelData *);
    void setScale(const JGeometry::TVec3f &);

    // Vtable
    virtual bool IsHitSphere(JGeometry::TVec3f, JGeometry::TVec3f, f32) = 0;
    virtual bool IsHitCylinder(JGeometry::TVec3f, JGeometry::TVec3f, const ObjColCylinder &) = 0;

    u8 IsHitBoundPos(JGeometry::TVec3f, JGeometry::TVec3f);

    void setRadius(const f32 rad) { mRadius = rad; }
    void scaleRadius(const f32 scale) { mRadius = scale * mRadius; }
    void setScale(f32 scale) { mScale = scale; }

    f32 getRadius() const { return mRadius; }
    CKind getKind() const { return mKind; }
    f32 getScale() const { return mScale; }
    f32 getBoundDepth() const { return mBoundDepth; }
    const JGeometry::TVec3f &getPos() const { return mPos; }

protected:
    // VTBL _00
    f32 mRadius;            // _04
    CKind mKind;            // _08
    f32 mScale;             // _0C
    f32 mBoundDepth;        // _10
    JGeometry::TVec3f mPos; // _14
    u32 _20;                // _20, unused?
};

class ObjColSphere : public ObjColBase
{
public:
    ObjColSphere(J3DModelData *modelData, JGeometry::TVec3f scale) : ObjColBase(modelData, scale, SPHERE) {}
    ObjColSphere(f32 radius, f32 scale) : ObjColBase(radius, scale, SPHERE) {}

    bool checkRadialCollisionXYZ(f32 otherRadius, JGeometry::TVec3f &deltaPos, f32 &minSeparationSq,
            f32 &distanceSq) const
    {
        f32 minSeparation = mRadius * mScale + otherRadius;

        minSeparationSq = minSeparation * minSeparation;
        distanceSq = deltaPos.squared();

        return minSeparationSq - distanceSq > 0.0f;
    }

    virtual bool IsHitSphere(JGeometry::TVec3f, JGeometry::TVec3f, f32);
    virtual bool IsHitCylinder(JGeometry::TVec3f, JGeometry::TVec3f, const ObjColCylinder &);
};

class ObjColCylinder : public ObjColBase
{
public:
    ObjColCylinder(J3DModelData *modelData, JGeometry::TVec3f scale) : ObjColBase(modelData, scale, CYLINDER), mCylinderRadius(0.0f), mCylinderHeight(0.0f) {}
    ObjColCylinder(f32 radius, f32 scale) : ObjColBase(radius, scale, CYLINDER), mCylinderRadius(0.0f), mCylinderHeight(0.0f) {}

    bool checkRadialCollisionXZ(f32 otherRadius, JGeometry::TVec3f const &posThis,
            JGeometry::TVec3f const &posOther, f32 &minSeparationSq, f32 &distanceSq, f32 &deltaX, f32 &deltaZ) const
    {
        // I hate this, but the register allocation is wrong if I write it the natural way
        f32 minSeparation = mCylinderRadius;
        minSeparation = (minSeparation * mScale) + otherRadius;

        deltaX = posOther.x - posThis.x;
        deltaZ = posOther.z - posThis.z;

        minSeparationSq = minSeparation * minSeparation;
        distanceSq = (deltaX * deltaX) + (deltaZ * deltaZ);

        return distanceSq < minSeparationSq;
    }

    void setCylinderRadius(f32 radius) { mCylinderRadius = radius; }
    void setCylinderHeight(f32 height) { mCylinderHeight = height; }

    f32 getCylinderRadius() const { return mCylinderRadius; }
    f32 getCylinderHeight() const { return mCylinderHeight; }

    f32 getScaledHeight() const { return mScale * mCylinderHeight; }

    virtual bool IsHitSphere(JGeometry::TVec3f, JGeometry::TVec3f, f32);
    virtual bool IsHitCylinder(JGeometry::TVec3f, JGeometry::TVec3f, const ObjColCylinder &);

private:
    f32 mCylinderRadius; // _24
    f32 mCylinderHeight; // _28
};

class ObjColCube : public ObjColBase
{
public:
    ObjColCube(J3DModelData *modelData, JGeometry::TVec3f scale) : ObjColBase(modelData, scale, CUBE) {}

    void makeParameter(J3DModelData *, Mtx m);
    void updateParameter(Mtx m);
    bool chkIsHitQuad(const JGeometry::TVec3f &, const f32 &);
    virtual bool IsHitSphere(JGeometry::TVec3f, JGeometry::TVec3f, f32);
    virtual bool IsHitCylinder(JGeometry::TVec3f, JGeometry::TVec3f, const ObjColCylinder &);

private:
    f32 mDimensions[3];             // _24 - edge lengths
    stPlaneParam mSidePlanes[4];    // _30
    stPlaneParam mTopPlane;         // _70
    JGeometry::TVec3f mVertices[5]; // _80
}; // size: 0xbc

class ExObjColBase : public ObjColBase
{
public:
    ExObjColBase(J3DModelData *mdlData, JGeometry::TVec3f scale, CKind kind) : ObjColBase(mdlData, scale, kind) {
        mAttr = 1;
        mMaterial = 0;
        mAttrIndex = 0;
        mVelocity.zero();
        mIsDashPlane = 0;
    }

    void setBottom(f32);
    void setWallParam(int);

    // Vtable
    virtual void makeParameter(J3DModelData *, Mtx m) = 0;             // 10
    virtual bool IsHitBound(JGeometry::TVec3f, JGeometry::TVec3f);     // 14
    virtual u8 IsHitBoundRadius(JGeometry::TVec3f, f32);               // 18
    virtual void Search(JGeometry::TVec3f, JGeometry::TVec3f) = 0;     // 1C
    virtual u8 IsOnDashPlane(JGeometry::TVec3f)                        // 20
    {
        return mIsDashPlane;
    }

    JGeometry::TVec3f getNormal() const {
        return mNormal;
    }
    JGeometry::TVec3f getWallNormal() const {
        return mWallNormal;
    }
    JGeometry::TVec3f getVelocity() {
        return mVelocity;
    }
    f32 getHeight() const {
        return mHeight;
    }
    u32 getAttr() const {
        return mAttr;
    }
    u32 getMaterial() const {
        return mMaterial;
    }
    f32 getColDepth() const {
        return mColDepth;
    }
    u32 getAttrIndex() const {
        return mAttrIndex;
    }

    void setAttr(u32 attr) {
        mAttr = attr;
    }
    void setMaterial(u32 material) {
        mMaterial = material;
    }
    void zeroVelocity() {
        mVelocity.zero();
    }
    void setWallNormal(JGeometry::TVec3f normal) {
        mWallNormal = normal;
    }
    void setNormal(JGeometry::TVec3f normal) {
        mNormal = normal;
    }
    void setVelocity(JGeometry::TVec3f vel) {
        mVelocity = vel;
    }
    void setAttrIndex(u32 idx) {
        mAttrIndex = idx;
    }
    void setHeight(f32 height) {
        mHeight = height;
    }
    void setColDepth(f32 depth) {
        mColDepth = depth;
    }

protected:
    stPlaneParam mSidePlanes[4];   // _24
    stPlaneParam mTopPlane;        // _64
    stPlaneParam mBottomPlane;     // _74
    JGeometry::TVec3f mNormal;     // _84
    JGeometry::TVec3f mWallNormal; // _90
    JGeometry::TVec3f mDirection;  // _9c
    JGeometry::TVec3f mVelocity;   // _a8
    f32 mHeight;                   // _b4
    f32 mBottom;                   // _b8
    u32 mAttr;                     // _bc
    u32 mMaterial;                 // _c0
    s32 mHitNum;                   // _c4
    f32 mColDepth;                 // _c8
    u8 mIsDashPlane;               // _cc, name is just a guess
    u32 mAttrIndex;                // _d0
};

class ObjColJump1 : public ExObjColBase
{
public:
    ObjColJump1(J3DModelData *mdlData, JGeometry::TVec3f scale) : ExObjColBase(mdlData, scale, ObjColBase::KIND_3) {}

    virtual void makeParameter(J3DModelData *, Mtx m);
    f32 SearchWall(JGeometry::TVec3f, JGeometry::TVec3f);
    virtual void Search(JGeometry::TVec3f, JGeometry::TVec3f);
    virtual bool IsHitSphere(JGeometry::TVec3f, JGeometry::TVec3f, f32);
    virtual bool IsHitCylinder(JGeometry::TVec3f, JGeometry::TVec3f, const ObjColCylinder &);
};

class ObjColJump2 : public ExObjColBase // UNUSED
{
public:
    virtual bool IsHitBound(JGeometry::TVec3f, JGeometry::TVec3f);
    virtual void makeParameter(J3DModelData *, Mtx m);
    virtual void Search(JGeometry::TVec3f, JGeometry::TVec3f);
    virtual void makeColParam(JGeometry::TVec3f, stPlaneParam, CrsGround::EAttr);
    virtual u8 IsOnDashPlane(JGeometry::TVec3f);
    virtual bool IsHitSphere(JGeometry::TVec3f, JGeometry::TVec3f, f32);
    virtual bool IsHitCylinder(JGeometry::TVec3f, JGeometry::TVec3f, const ObjColCylinder &);
};

class ObjColBlock : public ExObjColBase
{
public:
    ObjColBlock(J3DModelData *mdlData, JGeometry::TVec3f scale) : ExObjColBase(mdlData, scale, (ObjColBase::CKind)5) {}

    virtual void makeParameter(J3DModelData *, Mtx m);
    virtual f32 SearchWall(JGeometry::TVec3f, JGeometry::TVec3f);
    virtual void Search(JGeometry::TVec3f, JGeometry::TVec3f);
    virtual bool IsHitSphere(JGeometry::TVec3f, JGeometry::TVec3f, f32);
    virtual bool IsHitCylinder(JGeometry::TVec3f, JGeometry::TVec3f, const ObjColCylinder &);
};

class ObjColBoard : public ExObjColBase // UNUSED
{
public:
    f32 SearchWall(JGeometry::TVec3f, JGeometry::TVec3f);
    virtual void Search(JGeometry::TVec3f, JGeometry::TVec3f);
};

#define OBJCOLLISION_H
// --- end inline copy of Sato/ObjCollision.h ---

#include "Sato/GeographyObj.h"

class BridgeNode
{
public:
    BridgeNode() : mLink(this) {
        mVel.zero();
        mPos.zero();
    }
    ~BridgeNode() {}

    JGeometry::TVec3f mVel;   // 0x00
    JGeometry::TVec3f mPos;   // 0x0C
    JGeometry::TVec3f _18;    // 0x18
    JSULink<BridgeNode> mLink; // 0x24
};                            // 0x34

class BridgeNodeManager : public JSUList<BridgeNode>
{
public:
    BridgeNodeManager() : JSUList<BridgeNode>(), mNodes(0), mNodePos(0) {}
    ~BridgeNodeManager() {
        delete[] mNodes;
        delete[] mNodePos;
    }

    void setPosition(JGeometry::TVec3f &, JGeometry::TVec3f &, u32);

    BridgeNode *mNodes;       // 0x0C
    JGeometry::TVec3f *mNodePos; // 0x10
    u32 _14;                  // 0x14
    f32 _18;                  // 0x18
    f32 _1c;                  // 0x1C
};                            // 0x20

class StringBridge : public ExGeographyObj
{
public:
    StringBridge(const CrsData::SObject &);

    virtual ~StringBridge() {
        delete[] mBridgeNodeMgr[0].mNodes;
        delete[] mBridgeNodeMgr[0].mNodePos;
        delete[] mBridgeNodeMgr[1].mNodes;
        delete[] mBridgeNodeMgr[1].mNodePos;
        delete[] _28c;
        delete[] _290;
        delete[] _294;
        delete[] _298;
        delete[] _29c;
    }

    void reset();
    void init();
    void setParam();
    void loadmodel_base(J3DModelData *);
    virtual void createColModel(J3DModelData *modelData) { mExBounds = new ObjColBlock(modelData, mScale); }
    virtual void loadmodel(J3DModelData *modelData);
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void makeSharedDL();
    virtual void lockDisplayList();
    virtual void calc();
    virtual void update();
    virtual void simpleDraw(u32);
    void calcStringPos(Mtx, u32);
    void simpleDrawString(u32);
    void updateString();
    virtual const char *getLopeBmdFileName();
    virtual void calcMtx(Mtx, JGeometry::TVec3f, const JGeometry::TVec3f &, const JGeometry::TVec3f &);
    void calcBridgeModelMtx(Mtx, JGeometry::TVec3f, const JGeometry::TVec3f &, const JGeometry::TVec3f &);
    void calcStringMtx(Mtx, JGeometry::TVec3f &, const JGeometry::TVec3f &, const JGeometry::TVec3f &);
    void calcVertiMtx(Mtx, JGeometry::TVec3f &, BridgeNode *, BridgeNode *);
    void calcparallel(BridgeNode *, BridgeNode *);
    u32 findNthCenter(const JGeometry::TVec3f &);
    JGeometry::TVec3f getcollideNormalandDir(u32, JGeometry::TVec3f &, JGeometry::TVec3f &);
    virtual void AddVel(const JGeometry::TVec3f &, const JGeometry::TVec3f &);
    void addVelNode();
    u32 getWaveNodeNum(u8);
    virtual u8 Search_Bound(const JGeometry::TVec3f &);
    virtual u8 Search_BoundRadius(const JGeometry::TVec3f &, f32);
    virtual void Search(const JGeometry::TVec3f &, const JGeometry::TVec3f &);
    virtual void Search_Wall(const JGeometry::TVec3f &, f32);
    f32 calcXZLength(JGeometry::TVec3f &, const JGeometry::TVec3f &, const JGeometry::TVec3f &);
    stPlaneParam getSidePlaneParam(const JGeometry::TVec3f &, u32);
    f32 getplaneHeight(stPlaneParam &, const JGeometry::TVec3f &);
    f32 getplaneHeight(stPlaneParam &, JGeometry::TVec3f, u32, const JGeometry::TVec3f &, f32 &);
    f32 getRightnodeVelRatio(const JGeometry::TVec3f &);
    virtual const char *getBmdFileName();
    virtual f32 getMaxHeight() const { return _264; }

    BridgeNodeManager mBridgeNodeMgr[2]; // 0x170
    f32 _1b0;                             // 0x1B0
    f32 _1b4;                             // 0x1B4
    JGeometry::TVec3f _1b8[4];            // 0x1B8
    JGeometry::TVec3f _1e8[4];            // 0x1E8
    stPlaneParam _218[4];                 // 0x218
    f32 _258;                             // 0x258
    f32 _25c;                             // 0x25C
    f32 _260;                             // 0x260
    f32 _264;                             // 0x264
    u32 _268;                             // 0x268
    JGeometry::TVec3f _26c;               // 0x26C
    s16 _278;                             // 0x278
    s16 _27a;                             // 0x27A
    u16 _27c;                             // 0x27C
    u8 _27e;                              // 0x27E
    u32 _280;                             // 0x280
    u8 _284;                              // 0x284
    u32 _288;                             // 0x288
    ExModel *_28c;                        // 0x28C
    ExModel *_290;                        // 0x290
    ExModel *_294;                        // 0x294
    ExModel *_298;                        // 0x298
    ExModel *_29c;                        // 0x29C
    JGeometry::TVec3f *_2a0;              // 0x2A0
    JGeometry::TVec3f *_2a4;              // 0x2A4
    u32 _2a8;                              // 0x2A8
    f32 _2ac;                             // 0x2AC
    f32 _2b0;                             // 0x2B0
    f32 _2b4;                             // 0x2B4
    f32 _2b8;                             // 0x2B8
    f32 _2bc;                             // 0x2BC
    f32 _2c0;                             // 0x2C0
    JGeometry::TVec3f _2c4;               // 0x2C4
    f32 _2d0;                             // 0x2D0
    f32 _2d4;                             // 0x2D4
    f32 _2d8;                             // 0x2D8
    f32 _2dc;                             // 0x2DC
    f32 _2e0;                             // 0x2E0
    f32 _2e4;                             // 0x2E4
    f32 _2e8;                             // 0x2E8
    f32 _2ec;                             // 0x2EC
    f32 _2f0;                             // 0x2F0
    f32 _2f4;                             // 0x2F4
    u8 _2f8[4];                           // 0x2F8
    f32 _2fc;                             // 0x2FC
    f32 _300;                             // 0x300
    f32 _304;                             // 0x304
    f32 _308;                             // 0x308
    f32 _30c;                             // 0x30C
};                                        // 0x310

#endif
