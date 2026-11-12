#ifndef LENSFLARE_H
#define LENSFLARE_H

#include "JSystem/JGeometry/Box.h"
#include "JSystem/JKernel/JKRDisposer.h"
#include "JSystem/JSupport/JSUList.h"
#include "JSystem/JUtility/JUTTexture.h"
#include "JSystem/JUtility/TColor.h"
#include "types.h"

#include "Shiraiwa/Objects/MapObjHioNode.h"

class TLensFlare;

struct TFlareInfo {
    int type;
    f32 scale;
    s16 r;
    s16 g;
    s16 b;
    s16 _e;
};

class TLensFlareSupervisor : public GeoObjSupervisor {
public:
    TLensFlareSupervisor();
    ~TLensFlareSupervisor();
    void entry(TLensFlare *);
    void getBufferZ(u32);
    void draw(u32);
    void setupGX();
    void load();

    // Inline
    void reset() { }
    void calc() { }

    static const char *fileName[4];

    JSUList<TLensFlare> mList;  // 0x24
    ResTIMG **mTexInfo;         // 0x30
};

class TLensFlareScrn {
public:
    TLensFlareScrn();
    ~TLensFlareScrn();
    void calc(bool, f32);
    void frameIn();
    void frameOut();

    // Inline/Unused
    void init(u32, const JUTColor &, u8, u8);
    bool draw();
    void setupGX();

    static u8 sAlphaAccel;
    static u8 sAlphaDecel;
    static u8 sAlphaMax;
    static u8 sAlphaMin;

protected:
    u32 mConsole;                  // 0x00
    JGeometry::TBox2f mViewport;   // 0x04
    JUTColor mColor;               // 0x14
    u8 mAlphaMax;                  // 0x18
    u8 mAlphaMin;                  // 0x19
    u8 _1a[2];                     // 0x1a
    u32 mState;                    // 0x1c
    int mAlpha;                    // 0x20
    u8 mPrevInside;                // 0x24
    u8 _25[3];                     // 0x25
}; // Size: 0x28

class TFlareLine : public TLensFlareScrn {
public:
    friend class TLensFlare;
    class TFlare {
    public:
        TFlare() {}
        ~TFlare() {}

        JUTTexture *mTexture;       // 0x00
        JGeometry::TVec2f mCenter;  // 0x04
        JGeometry::TVec2f mRadius;  // 0x0c
        JUTColor mColor;            // 0x14
        u8 mAdditive;               // 0x18
        u8 _19[3];                  // 0x19
    }; // Size: 0x1c

    TFlareLine(u32 = 0);
    ~TFlareLine();
    void reset(u8, u8, u8, u8, u8);
    void resetFlares();
    void calc(const JGeometry::TVec3f &);
    bool isInside(const JGeometry::TVec2f &);
    void setPosition(const JGeometry::TVec2f &, const JGeometry::TVec2f &);
    u8 draw();
    void drawFixCenter(TFlare &);

    // Inline/Unused
    void setTexture(ResTIMG **, int);
    void getBufferZ();
    f32 getAspectRate();

    static f32 sScale;
    static f32 sFlareInterval;
    static f32 sOutRadius;
    static u8 sAlpha;
    static u8 sAlphaAccel;
    static u8 sAlphaDecel;
    static f32 sScrnScaleRange;
    static TFlareInfo sFlareInfo[21];
    static JUTColor sColor;

protected:
    JGeometry::TBox2f mScrnBounds; // 0x28
    bool mInside;                  // 0x38
    u8 _39[3];                     // 0x39
    int mCameraNo;                 // 0x3c
    f32 mAngle;                    // 0x40
    s16 mAlpha2;                   // 0x44
    u16 _46;                       // 0x46
    f32 mCenterX;                  // 0x48
    f32 mCenterY;                  // 0x4c
    u32 mBufferZ;                  // 0x50
    f32 _54;                       // 0x54
    f32 mScrnScale;                // 0x58
    u8 mAlphaClamp;                // 0x5c
    u8 _5d[3];                     // 0x5d
    TFlare **mFlares;              // 0x60
}; // Size: 0x64

class TLensFlare : public TMapObjHioNode, public JKRDisposer {
public:
    friend class TLensFlareSupervisor;
    TLensFlare(const CrsData::SObject &); // 0x802b0558
    virtual ~TLensFlare();
    void reset();                         // 0x802b0758
    void calc();                          // 0x802b086c
    u8 draw(u32);                         // 0x802b08c8

    // Inline/Unused
    void getBufferZ(u32);

    // Inline
    virtual TLensFlareSupervisor *getSupervisor() { return sSupervisor; }
    static TLensFlareSupervisor *getSupervisor() { return sSupervisor; }

private:
    static TLensFlareSupervisor *sSupervisor; // 0x80416f48
    JSULink<TLensFlare> mLink;                 // 0x164
    TFlareLine *mFlares;                       // 0x174
}; // class LensFlare

#endif // LENSFLARE_H
