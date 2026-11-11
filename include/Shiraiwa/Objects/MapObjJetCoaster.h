#ifndef MAPOBJJETCOASTER_H
#define MAPOBJJETCOASTER_H

#include "Inagaki/GameMapSoundMgr.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjJetCoaster : public TMapObjHioNode
{
public:
    TMapObjJetCoaster(u32);                                                       // 0x802c1000
    TMapObjJetCoaster(const CrsData::SObject &);
    virtual ~TMapObjJetCoaster();                                                 // 0x802c1078
    virtual void reset();                                                         // 0x802c1108
    virtual void loadAnimation();                                                 // 0x802c1208
    virtual void createModel(JKRSolidHeap *, u32);                                // 0x802c1254
    virtual void createColModel(J3DModelData *);                                  // 0x802c13a4
    virtual void calc();                                                          // 0x802c13cc
    virtual const char *getBmdFileName() = 0;
    virtual int getJointNo();                                                     // 0x802c15b8
    virtual void setJointNo();                                                    // 0x802c13a0

    void setAnimationFrame(f32);
    void getColPos(JGeometry::TVec3f *, long);

    static f32 sFrameRate;                        // 0x804153d8
    static J3DAnmTransform *sJetCoasterBckAnmTrans; // 0x80417080
    static J3DMtxCalc *sJetCoasterBckMtxCalc;       // 0x80417084

protected:
    JGeometry::TVec3f mPrevPos; // 14c
    JGeometry::TVec3f mCurPos;  // 158
    u8 mIsFirst;                // 164
}; // Size: 0x168

class TMapObjJetCoasterBody : public TMapObjJetCoaster
{
public:
    TMapObjJetCoasterBody(u32 id = 0) : TMapObjJetCoaster(id) {}
    virtual ~TMapObjJetCoasterBody() {}
    virtual const char *getBmdFileName() { return "/Objects/BabyJet_body.bmd"; }
    virtual int getJointNo() { return sJointNo; }
    virtual void setJointNo(); // 0x802c1e40

    static int sJointNo; // 0x804153e0
}; // Size: 0x168

class TMapObjJetCoasterHead : public TMapObjJetCoaster
{
public:
    TMapObjJetCoasterHead(const CrsData::SObject &); // 0x802c15c0
    virtual ~TMapObjJetCoasterHead();                // 0x802c1d5c
    virtual void reset();                            // 0x802c1790
    virtual void calc();                             // 0x802c1acc
    virtual const char *getBmdFileName() { return "/Objects/BabyJet_head.bmd"; }
    virtual int getJointNo() { return sJointNo; }
    virtual void setJointNo(); // 0x802c1e08

    void createBody(u8); // 0x802c16c4

    static int sJointNo; // 0x804153dc
    static const u8 scBodyMax = 50;

private:
    u8 mBodyNum;                               // 168
    f32 mFrame;                                // 16c
    TMapObjJetCoaster **mBodies;               // 170
    GameAudio::CoasterSoundMgr *mCoasterSound; // 174
}; // Size: 0x178

#endif // MAPOBJJETCOASTER_H
