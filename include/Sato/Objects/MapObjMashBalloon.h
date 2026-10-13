#ifndef MAPOBJMASHBALLOON_H
#define MAPOBJMASHBALLOON_H

#include "Sato/GeographyObj.h"

class TMapObjSkyShip : public GeographyObj
{
public:
    TMapObjSkyShip(const CrsData::SObject &);
    virtual ~TMapObjSkyShip();
    virtual void calc() {}

private:
    u8 _stub[0x14]; // sizeof 0x160
};

class TMapObjMashBalloon : public TMapObjSkyShip
{
public:
    TMapObjMashBalloon(const CrsData::SObject &obj) : TMapObjSkyShip(obj) {}
    virtual ~TMapObjMashBalloon() {}

private:
    u8 _stub[0x190 - 0x160];
};

class TMapObjPathYoshiHeli : public TMapObjSkyShip
{
public:
    TMapObjPathYoshiHeli(const CrsData::SObject &obj) : TMapObjSkyShip(obj) {}
    virtual ~TMapObjPathYoshiHeli() {}

private:
    u8 _stub[0x180 - 0x160];
};

#endif
