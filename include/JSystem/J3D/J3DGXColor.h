#ifndef _JSYSTEM_J3D_J3DGXCOLOR_H
#define _JSYSTEM_J3D_J3DGXCOLOR_H

#include "types.h"
#include <dolphin/gx.h>
#include <dolphin/gd.h>

#pragma push
#pragma optimize_for_size off

struct J3DGXColor : public GXColor {
    J3DGXColor() { }

    J3DGXColor(u8 _r, u8 _g, u8 _b, u8 _a)
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }

    J3DGXColor(const J3DGXColor& other) { *(u32*)this = *(const u32*)&other; }

    J3DGXColor(const GXColor& other) { *(u32*)this = *(const u32*)&other; }

    J3DGXColor(const u32& other) { *(u32*)this = other; }

    J3DGXColor& operator=(const GXColor& color)
    {
        *(u32*)this = *(const u32*)&color;
        return *this;
    }

    /** @fabricated */
    const u8* asBytes() const { return (u8*)this; }
};

struct J3DGXColorS10 : public GXColorS10 {
    J3DGXColorS10() { }

    J3DGXColorS10(u16 _r, u16 _g, u16 _b, u16 _a)
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }

    J3DGXColorS10(f32 _r, f32 _g, f32 _b, u16 _a)
    {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }

    J3DGXColorS10(const J3DGXColorS10& other)
    {
        ((u32*)this)[0] = ((const u32*)&other)[0];
        ((u32*)this)[1] = ((const u32*)&other)[1];
    }

    J3DGXColorS10(const GXColorS10& other) : GXColorS10(other) { }

    J3DGXColorS10(const u64& other)
    {
        GXColorS10* otherBytes = (GXColorS10*)&other;
        r                      = otherBytes->r;
        g                      = otherBytes->g;
        b                      = otherBytes->b;
        a                      = otherBytes->a;
    }

    J3DGXColorS10& operator=(const GXColorS10& color)
    {
        *(u32*)this = *(u32*)&color;
        ((u32*)this)[1] = ((u32*)&color)[1];
        return *this;
    }
};

#pragma pop

#endif
