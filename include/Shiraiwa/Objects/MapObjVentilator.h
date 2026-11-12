#ifndef MAPOBJVENTILATOR_H
#define MAPOBJVENTILATOR_H

#include <JSystem/J3D/J3DModel.h>
#include <JSystem/JGeometry.h>

#include "Kaneshige/Course/CrsData.h"
#include "Sato/StateObserver.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TMapObjVentilator : public TMapObjHioNode, public StateObserver
{
public:
    TMapObjVentilator(const CrsData::SObject &);
    virtual ~TMapObjVentilator();

    virtual void reset();
    virtual void calc();
    virtual const char *getBmdFileName();

    void MoveExec();

    void InitExec();

    void initFunc_Slow();
    void doFunc_Slow();
    void doRotate(const JGeometry::TQuat4f &);
    void initFunc_Slow2Fast();
    void doFunc_Slow2Fast();
    void initFunc_Fast();
    void doFunc_Fast();
    void initFunc_Fast2Slow();
    void doFunc_Fast2Slow();

private:
    static StateFuncSet<TMapObjVentilator> sTable[4];

    JGeometry::TQuat4f mCurQuat;  // 158
    JGeometry::TQuat4f mFastQuat; // 168
    JGeometry::TQuat4f mSlowQuat; // 178
    f32 mSpeed;                   // 188
};

#endif // MAPOBJVENTILATOR_H
