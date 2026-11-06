#ifndef MAPOBJDONKYTREE_H
#define MAPOBJDONKYTREE_H

#include "Sato/Objects/GeoTree.h"

class TMapObjDonkyTree : public GeoMarioTree1
{
public:
    TMapObjDonkyTree(const CrsData::SObject &);
    virtual ~TMapObjDonkyTree();

    virtual const char *getBmdFileName();
    virtual GeoAnmTableEntry *getAnmTbl();
    virtual u16 getSizeAnmTbl();

    static GeoAnmTableEntry sBckAnmTable[1];
};

class TMapObjDinoTree : public GeoMarioTree1
{
public:
    TMapObjDinoTree(const CrsData::SObject &);
    virtual ~TMapObjDinoTree();

    virtual void reset();
    virtual const char *getBmdFileName();
    virtual GeoAnmTableEntry *getAnmTbl();
    virtual u16 getSizeAnmTbl();

    static GeoAnmTableEntry sBckAnmTable[1];
};

class TMapObjDesertTree : public GeoMarioTree1
{
public:
    TMapObjDesertTree(const CrsData::SObject &);
    virtual ~TMapObjDesertTree();

    virtual void reset();
    virtual const char *getBmdFileName();
    virtual GeoAnmTableEntry *getAnmTbl();
    virtual u16 getSizeAnmTbl();
    virtual void createColModel(J3DModelData *);
};

#endif // MAPOBJDONKYTREE_H
