#ifndef MAPOBJFERRISWHEEL_H
#define MAPOBJFERRISWHEEL_H

#include <JSystem/J3D/J3DJoint.h>
#include <JSystem/JGeometry.h>

#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjFerriswheel : public TMapObjHioNode
{
public:
    TMapObjFerriswheel(const CrsData::SObject &);
    virtual ~TMapObjFerriswheel();
    virtual void createModel(JKRSolidHeap *, u32);
    virtual void reset();
    virtual void calc();
    virtual const char *getBmdFileName();

    void setCallBack(const char *, int (*)(J3DJoint *, int));
    void rotate(Mtx);
    void setGondolaMtx(int, Mtx);
    static int centerCallBack(J3DJoint *, int);
    static int gondolaCallBack(J3DJoint *, int);

    static int sRotFrame;
    static const char *scRotJointName;
    static const char *scGondolaJointName[16];
    static int sRotJointNo;

    JGeometry::TPos3f mRotMtx;     // 0x14c
    JGeometry::TPos3f mRotStepMtx; // 0x17c
    JGeometry::TPos3f mGondolaMtx; // 0x1ac
}; // Size: 0x1dc

#endif
