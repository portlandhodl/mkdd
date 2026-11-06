#ifndef MAPOBJPEACHTREE_H
#define MAPOBJPEACHTREE_H

#include "Sato/Objects/GeoTree.h"

class TMapObjPeachTree : public GeoMarioTree1 {
public:
    TMapObjPeachTree(const CrsData::SObject &);
    virtual ~TMapObjPeachTree();

    virtual void createColModel(J3DModelData *);
    virtual const char *getBmdFileName();
    virtual GeoAnmTableEntry *getAnmTbl();
    virtual u16 getSizeAnmTbl();

    static GeoAnmTableEntry sBckAnmTable[1];
};

class TMapObjPeachTree2 : public GeoMarioTree1 {
public:
    TMapObjPeachTree2(const CrsData::SObject &);
    virtual ~TMapObjPeachTree2();

    virtual void createColModel(J3DModelData *);
    virtual const char *getBmdFileName();
    virtual GeoAnmTableEntry *getAnmTbl();
    virtual u16 getSizeAnmTbl();

    static GeoAnmTableEntry sBckAnmTable[1];
};

#endif
