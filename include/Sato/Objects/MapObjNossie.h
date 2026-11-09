#ifndef MAPOBJNOSSIE_H
#define MAPOBJNOSSIE_H

#include "Shiraiwa/AnmPlayer.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/J3DAnmObject.h"

class TPathUtil;

class TMapObjNossieColBase : public TMapObjStamper
{
public:
    TMapObjNossieColBase(u32 id) : TMapObjStamper(id) {}

    virtual ~TMapObjNossieColBase() {}
    virtual void reset();
    virtual void calc();
    virtual u32 getReaction();
};

class TMapObjNossieColFoot : public TMapObjNossieColBase
{
public:
    TMapObjNossieColFoot(u32 id) : TMapObjNossieColBase(id)
    {
        createSoundMgr();
    }

    virtual ~TMapObjNossieColFoot() {}
    virtual void reset();
    virtual void calc();
    virtual void createColModel(J3DModelData *);
    virtual u32 getReleaseFrame();
    virtual u32 getReaction() { return 0xa; }

    void moveShadowModel();
    bool isStepped(int);
    void getHitFlag(u8 *);

    JGeometry::TVec3f mShadowPos; // 0x174
    f32 mShake;                   // 0x180
    u8 mPrevHit;                  // 0x184
    u8 mHit;                      // 0x185
    u8 _186[6];                   // 0x186
};

class TMapObjNossieColFootFront : public TMapObjNossieColFoot
{
public:
    TMapObjNossieColFootFront(u32 id = 0x11a3) : TMapObjNossieColFoot(id) {}
    virtual ~TMapObjNossieColFootFront() {}
    virtual const char *getShadowBmdFileName() { return "/Objects/NossieFrontShadow.bmd"; }

private:
    u8 _extra[0x0];
};

class TMapObjNossieColFootRear : public TMapObjNossieColFoot
{
public:
    TMapObjNossieColFootRear(u32 id = 0x11a7) : TMapObjNossieColFoot(id) {}
    virtual ~TMapObjNossieColFootRear() {}
    virtual const char *getShadowBmdFileName() { return "/Objects/NossieBackShadow.bmd"; }

private:
    u8 _extra[0x0];
};

class TMapObjNossieColHead : public TMapObjNossieColBase
{
public:
    TMapObjNossieColHead(int id = 0x11a2) : TMapObjNossieColBase(id) { createSoundMgr(); }

    virtual ~TMapObjNossieColHead() {}
    virtual void calc();
    virtual void createColModel(J3DModelData *);

    static f32 sShakeRadius;
};

class TMapObjNossieColBody : public TMapObjNossieColBase
{
public:
    TMapObjNossieColBody(int id = 0x11a8) : TMapObjNossieColBase(id) {}

    virtual ~TMapObjNossieColBody() {}
    virtual void createColModel(J3DModelData *);
    virtual u32 getReaction() { return 1; }
};

class TMapObjNossie : public TMapObjHioNode
{
public:
    TMapObjNossie(const CrsData::SObject &);
    virtual ~TMapObjNossie();

    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual const char *getBmdFileName();

    void reverse();
    bool isTimeAttack();
    void getJointPosition(JGeometry::TVec3f *, long);
    void makeStepEmitter(u32, const JGeometry::TVec3f &);

    static TAnmInfo sAnmInfos[16];
    static TAnmInfo sAnmInfosTA[1];
    static const char *sNossieLegJointName[4];
    static const char *sHeadJointName;
    static int sNossieLegJointNo[4];
    static int sHeadJointNo;
    static JGeometry::TVec3f sOffsetY[4];
    static JGeometry::TVec3f sHeadOffset;
    static JGeometry::TVec3f sBodyOffset;

    TAnmPlayer mAnmPlayer;      // 0x14c
    u8 _164[0x30];              // 0x164
    TMapObjNossieColHead *mHead;   // 0x194
    TMapObjNossieColFoot *mFoot[4]; // 0x198
    TMapObjNossieColBody *mBody;   // 0x1a8
}; // Size: 0x1ac

class TMapObjSwimNossie : public TMapObjHioNode
{
public:
    TMapObjSwimNossie(const CrsData::SObject &);
    virtual ~TMapObjSwimNossie();

    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual void update();
    virtual const char *getBmdFileName();

    float moveToNextNode();

    static f32 sMoveMaxSpeed;
    static f32 sMoveAccel;
    static f32 sReachLength;

    J3DAnmObjMaterial mMaterial; // 0x14c
    TPathUtil *mPath;            // 0x16c
}; // Size: 0x170

class TMapObjPtera : public TMapObjHioNode
{
public:
    TMapObjPtera(const CrsData::SObject &);
    virtual ~TMapObjPtera();

    virtual void loadAnimation();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual void update();
    virtual const char *getBmdFileName();

    float moveToNextNode();

    static f32 sMoveMaxSpeed;
    static f32 sMoveAccel;
    static f32 sReachLength;
    static TAnmInfo sAnmInfos[2];

    TPathUtil *mPath;            // 0x14c
    J3DAnmObjMaterial mMaterial; // 0x150
    TAnmPlayer mAnmPlayer;       // 0x170
    u8 _188[0];                  // 0x188
}; // Size: 0x188

#endif
