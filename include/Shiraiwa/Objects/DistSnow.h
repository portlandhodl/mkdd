#ifndef DISTSNOW_H
#define DISTSNOW_H

#include <JSystem/JGeometry.h>
#include <JSystem/JKernel/JKRDisposer.h>
#include <JSystem/JUtility/JUTTexture.h>

#include "Shiraiwa/Objects/MapObjHioNode.h"

class TDistSnow;

class TDistSnowSupervisor : public GeoObjSupervisor
{
public:
    TDistSnowSupervisor() { mDistSnow = nullptr; }
    virtual ~TDistSnowSupervisor() {}
    virtual void reset() {}
    virtual void calc() {}
    virtual void load();

    void draw(u32);

    TDistSnow *mDistSnow; // 0x24
}; // Size: 0x28

class TSnowPanel
{
public:
    TSnowPanel();
    ~TSnowPanel();
    void reset();
    void calc();
    void draw();
    void setSize(f32 width, f32 height)
    {
        mWidth = width;
        mHeight = height;
    }

    JGeometry::TVec2f mPos; // 0x00
    f32 mVel;               // 0x08
    f32 mWidth;             // 0x0c
    f32 mHeight;            // 0x10
    s16 mAlpha;             // 0x14
    u16 mLife;              // 0x16
    u16 mMaxLife;           // 0x18
}; // Size: 0x1c

class TDistSnow : public TMapObjHioNode, public JKRDisposer
{
public:
    TDistSnow(const CrsData::SObject &);
    virtual ~TDistSnow();
    virtual void reset();
    virtual void calc();
    virtual GeoObjSupervisor *getSupervisor() { return sSupervisor; }

    void draw(u32);
    void setupGX();
    void load();

    static TDistSnowSupervisor *sSupervisor;

    JUTTexture *mTexture;      // 0x164
    TSnowPanel *mPanels;       // 0x168
    JGeometry::TVec3f mOffset; // 0x16c
}; // Size: 0x178

#endif
