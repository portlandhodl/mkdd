#ifndef TTESTSTRING_H
#define TTESTSTRING_H

#include "Sato/GeographyObj.h"
#include "Shiraiwa/Objects/MapObjHioNode.h"

class TTestStrBase;

class TTestStrSupervisor
{
public:
    void drawAll(u32);
};

class TTestStrBarrel : public GeographyObj
{
public:
    TTestStrBarrel(u32 id = 0xe0d) : GeographyObj(id) {}
    virtual ~TTestStrBarrel() {}

    void calc() {}

private:
    u32 _14c;
};

class TTestStrItem : public GeographyObj
{
public:
    TTestStrItem(u32 id = 0xe0b) : GeographyObj(id) {}
    virtual ~TTestStrItem() {}

    void calc() {}

private:
    u32 _14c;
    u32 _150;
    u32 _154;
    u32 _158;
};

class TTestString : public GeographyObj
{
public:
    TTestString(const CrsData::SObject &);
    virtual ~TTestString() {}

    void calc() {}

    static TTestStrSupervisor *sSupervisor;

private:
    u8 _stub[0x64]; // 0x1b0
};

#endif
