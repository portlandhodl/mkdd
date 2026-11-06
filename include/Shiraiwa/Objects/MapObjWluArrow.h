#ifndef MAPOBJWLUARROW_H
#define MAPOBJWLUARROW_H

#include "Shiraiwa/Objects/MapObjHioNode.h"
#include "Sato/J3DAnmObject.h"

class TMapObjWlArrow : public TMapObjHioNode
{
public:
    TMapObjWlArrow(const CrsData::SObject &); // 0x802b65e0
    virtual ~TMapObjWlArrow();                // 0x802b6694

    virtual const char *getBmdFileName();          // 0x802b6764
    virtual void reset();                          // 0x802b678c
    virtual void createModel(JKRSolidHeap *, u32); // 0x802b6810
    virtual void loadAnimation();                  // 0x802b6968
    virtual void calc();                           // 0x802b69d0
    virtual void update();                         // 0x802b69f4

    static J3DAnmTextureSRTKey *sWlArrowBtkAnm; // 0x80416fa0
    static J3DAnmTransform *sWlArrowBckAnmTrans; // 0x80416fa4
    static J3DMtxCalc *sWlArrowBckMtxCalc;       // 0x80416fa8

private:
    J3DAnmObjMaterial mAnmObj; // 0x14c
}; // Size: 0x16c

class TMapObjWluSearchLight : public TMapObjHioNode
{
public:
    TMapObjWluSearchLight(const CrsData::SObject &); // 0x802b6a38
    virtual ~TMapObjWluSearchLight();                // 0x802b6ab0

    virtual const char *getBmdFileName();          // 0x802b6b40
    virtual void reset();                          // 0x802b6b68
    virtual void createModel(JKRSolidHeap *, u32); // 0x802b6bbc
    virtual void loadAnimation();                  // 0x802b6cf4
    virtual void calc() {}                         // 0x802b6d40

    static J3DAnmTransform *sWlScLightBckAnmTrans; // 0x80416fb4
    static J3DMtxCalc *sWlScLightBckMtxCalc;       // 0x80416fb8
}; // Size: 0x14c

#endif // MAPOBJWLUARROW_H
