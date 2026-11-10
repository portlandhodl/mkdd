#ifndef _JSYSTEM_J3D_J3DPE_H
#define _JSYSTEM_J3D_J3DPE_H

#include "JSystem/J3D/J3DTypes.h"
#include "types.h"

struct J3DFogInfo {
    J3DFogInfo& operator=(const J3DFogInfo& other);
    J3DFogInfo& operator=(const J3DFogInfo* other)
    {
        *this = *other;
        return *this;
    }

    u8 mType;             // _00
    u8 mAdjEnable;        // _01
    u16 mCenter;          // _02
    f32 mStartZ;          // _04
    f32 mEndZ;            // _08
    f32 mNearZ;           // _0C
    f32 mFarZ;            // _10
    GXColor mColor;       // _14
    u16 mFogAdjTable[10]; // _18
};

extern const J3DFogInfo j3dDefaultFogInfo;

struct J3DFog : J3DFogInfo {
    J3DFog() { *(J3DFogInfo*)this = j3dDefaultFogInfo; }
    J3DFog(const J3DFog& other)
        : J3DFogInfo(other)
    {
    }
    ~J3DFog();

    void setFogInfo(J3DFogInfo info) { *(J3DFogInfo*)this = info; }
    void setFogInfo(const J3DFogInfo* info) { *(J3DFogInfo*)this = *info; }

    J3DFogInfo* getFogInfo() { return (J3DFogInfo*)this; }

    void load() const
    {
        J3DGDSetFog(GXFogType(mType), mStartZ, mEndZ, mNearZ, mFarZ, mColor);
        J3DGDSetFogRangeAdj(mAdjEnable, mCenter, (GXFogAdjTable*)mFogAdjTable);
    }

    u8 getType() const { return mType; }
    void setType(u8 type) { mType = type; }
    void setStartZ(f32 startZ) { mStartZ = startZ; }
    void setEndZ(f32 endZ) { mEndZ = endZ; }
    void setNearZ(f32 nearZ) { mNearZ = nearZ; }
    void setFarZ(f32 farZ) { mFarZ = farZ; }

    void setFogColor(GXColor& color) {
        mColor.r = color.r;
        mColor.g = color.g;
        mColor.b = color.b;
        mColor.a = color.a;
    }
};

/**
 * @size{0x4}
 */
struct J3DPEBlock {
    virtual void reset(J3DPEBlock*);                // _08 (weak)
    virtual void load() = 0;                        // _0C
    virtual void patch();                           // _10 (weak)
    virtual void diff(unsigned long);               // _14 (weak)
    virtual void diffFog();                         // _18 (weak)
    virtual void diffBlend();                       // _1C (weak)
    virtual u32 countDLSize();                      // _20 (weak)
    virtual JBlockType getType() = 0;               // _24
    virtual void setFog(J3DFog);                    // _28 (weak)
    virtual void setFog(J3DFog*);                   // _2C (weak)
    virtual J3DFog* getFog();                       // _30 (weak)
    virtual void setAlphaComp(const J3DAlphaComp*); // _34 (weak)
    virtual void setAlphaComp(const J3DAlphaComp&); // _38 (weak)
    virtual J3DAlphaComp* getAlphaComp();           // _3C (weak)
    virtual void setBlend(const J3DBlend*);         // _40 (weak)
    virtual void setBlend(const J3DBlend&);         // _44 (weak)
    virtual J3DBlend* getBlend();                   // _48 (weak)
    virtual void setZMode(const J3DZMode*);         // _4C (weak)
    virtual void setZMode(J3DZMode);                // _50 (weak)
    virtual J3DZMode* getZMode();                   // _54 (weak)
    virtual void setZCompLoc(const unsigned char*); // _58 (weak)
    virtual void setZCompLoc(unsigned char);        // _5C (weak)
    virtual u8 getZCompLoc() const;                 // _60 (weak)
    virtual void setDither(const unsigned char*);   // _64 (weak)
    virtual void setDither(unsigned char);          // _68 (weak)
    virtual u8 getDither() const;                   // _6C (weak)
    virtual u32 getFogOffset() const;               // _70 (weak)
    virtual void setFogOffset(unsigned long);       // _74 (weak)
    virtual ~J3DPEBlock() { }                       // _78 (weak)
};

struct J3DPEBlockFogOff : public J3DPEBlock {
    J3DPEBlockFogOff() { initialize(); }

    virtual void reset(J3DPEBlock*); // _08
    virtual void load();             // _0C
    virtual void diff(unsigned long flag) // _14 (weak)
    {
        if (flag & J3DMDF_DiffBlend) {
            diffBlend();
        }
    }
    virtual void diffBlend();                                                                          // _1C
    virtual u32 countDLSize();                                                                         // _20
    virtual JBlockType getType() { return JBT_PEFogOff; }                                              // _24 (weak)
    virtual void setAlphaComp(const J3DAlphaComp* alphaComp) { mAlphaComp = *alphaComp; }              // _34 (weak)
    virtual void setAlphaComp(const J3DAlphaComp& alphaComp) { mAlphaComp = alphaComp; }               // _38 (weak)
    virtual J3DAlphaComp* getAlphaComp() { return &mAlphaComp; }                                       // _3C (weak)
    virtual void setBlend(const J3DBlend* blend) { mBlend = *blend; }                                  // _40 (weak)
    virtual void setBlend(const J3DBlend& blend) { mBlend = blend; }                                   // _44 (weak)
    virtual J3DBlend* getBlend() { return &mBlend; }                                                   // _48 (weak)
    virtual void setZMode(const J3DZMode* zMode) { mZMode = *zMode; }                                  // _4C (weak)
    virtual void setZMode(J3DZMode zMode) { mZMode = zMode; }                                          // _50 (weak)
    virtual J3DZMode* getZMode() { return &mZMode; }                                                   // _54 (weak)
    virtual void setZCompLoc(const unsigned char* compLoc) { mZCompLoc = *compLoc; }                   // _58 (weak)
    virtual void setZCompLoc(unsigned char compLoc) { mZCompLoc = compLoc; }                           // _5C (weak)
    virtual u8 getZCompLoc() const { return mZCompLoc; }                                               // _60 (weak)
    virtual void setDither(const unsigned char* dither) { mDither = *dither; }                         // _64 (weak)
    virtual void setDither(unsigned char dither) { mDither = dither; }                                 // _68 (weak)
    virtual u8 getDither() const { return mDither; }                                                   // _6C (weak)
    virtual ~J3DPEBlockFogOff() { }                                                                    // _78 (weak)

    void initialize();

    J3DAlphaComp mAlphaComp; // _04
    J3DBlend mBlend;         // _08
    J3DZMode mZMode;         // _0C
    u8 mZCompLoc;            // _0E
    u8 mDither;              // _0F
};

struct J3DPEBlockFull : public J3DPEBlock {
    J3DPEBlockFull() { initialize(); }

    virtual void reset(J3DPEBlock*);  // _08
    virtual void load();              // _0C
    virtual void patch();             // _10
    virtual void diff(unsigned long); // _14
    virtual void diffFog();           // _18
    virtual void diffBlend();         // _1C
    virtual u32 countDLSize();        // _20
    /**
     * @reifiedAddress{800816D0}
     * @reifiedFile{JSystem/J3D/J3DMatBlock.cpp}
     */
    virtual JBlockType getType() // _24 (weak)
    {
        return JBT_PEFull;
    }
    virtual void setFog(J3DFog fog) { mFog = fog; }                                                // _28 (weak)
    virtual void setFog(J3DFog* fog) { mFog = *fog; }                                              // _2C (weak)
    virtual J3DFog* getFog() { return &mFog; }                                                     // _30 (weak)
    virtual void setAlphaComp(const J3DAlphaComp* alphaComp) { mAlphaComp = *alphaComp; }          // _34 (weak)
    virtual void setAlphaComp(const J3DAlphaComp& alphaComp) { mAlphaComp = alphaComp; }           // _38 (weak)
    virtual J3DAlphaComp* getAlphaComp() { return &mAlphaComp; }                                   // _3C (weak)
    virtual void setBlend(const J3DBlend* blend) { mBlend = *blend; }                              // _40 (weak)
    virtual void setBlend(const J3DBlend& blend) { mBlend = blend; }                               // _44 (weak)
    virtual J3DBlend* getBlend() { return &mBlend; }                                               // _48 (weak)
    virtual void setZMode(const J3DZMode* zMode) { mZMode = *zMode; }                              // _4C (weak)
    virtual void setZMode(J3DZMode zMode) { mZMode = zMode; }                                      // _50 (weak)
    virtual J3DZMode* getZMode() { return &mZMode; }                                               // _54 (weak)
    virtual void setZCompLoc(const unsigned char* compLoc) { mZCompLoc = *compLoc; }               // _58 (weak)
    virtual void setZCompLoc(unsigned char compLoc) { mZCompLoc = compLoc; }                       // _5C (weak)
    virtual u8 getZCompLoc() const { return mZCompLoc; }                                           // _60 (weak)
    /**
     * @reifiedAddress{80081910}
     * @reifiedFile{JSystem/J3D/J3DMatBlock.cpp}
     */
    virtual void setDither(const unsigned char* dither) // _64 (weak)
    {
        mDither = *dither;
    }
    /**
     * @reifiedAddress{80081908}
     * @reifiedFile{JSystem/J3D/J3DMatBlock.cpp}
     */
    virtual void setDither(unsigned char dither) // _68 (weak)
    {
        mDither = dither;
    }
    /**
     * @reifiedAddress{8008191C}
     * @reifiedFile{JSystem/J3D/J3DMatBlock.cpp}
     */
    virtual u8 getDither() const // _6C (weak)
    {
        return mDither;
    }
    /**
     * @reifiedAddress{80081924}
     * @reifiedFile{JSystem/J3D/J3DMatBlock.cpp}
     */
    virtual u32 getFogOffset() const // _70 (weak)
    {
        return mFogOffset;
    }
    /**
     * @reifiedAddress{8008192C}
     * @reifiedFile{JSystem/J3D/J3DMatBlock.cpp}
     */
    virtual void setFogOffset(unsigned long fogOffset) // _74 (weak)
    {
        mFogOffset = fogOffset;
    }
    /**
     * @reifiedAddress{80081934}
     * @reifiedFile{JSystem/J3D/J3DMatBlock.cpp}
     */
    virtual ~J3DPEBlockFull() {}; // _78 (weak)

    void initialize();

    J3DFog mFog;             // _04
    J3DAlphaComp mAlphaComp; // _30
    J3DBlend mBlend;         // _34
    J3DZMode mZMode;         // _38
    u8 mZCompLoc;            // _3A
    u8 mDither;              // _3B
    u32 mFogOffset;          // _3C
};

/**
 * @size{0x4}
 */
struct J3DPEBlockNull : public J3DPEBlock {
    virtual void load() { } // _0C (weak)
    /**
     * @reifiedAddress{8006F448}
     * @reifiedFile{JSystem/J3D/J3DMaterialFactory.cpp}
     */
    virtual JBlockType getType() // _24 (weak)
    {
        return JBT_PENull;
    }
    virtual ~J3DPEBlockNull() { } // _78 (weak)
};

/**
 * @size{0x4}
 */
struct J3DPEBlockOpa : public J3DPEBlock {
    virtual void load();                               // _0C
    virtual u32 countDLSize();                         // _20
    virtual JBlockType getType() { return JBT_PEOpa; } // _24 (weak)
    virtual ~J3DPEBlockOpa() { }                       // _78 (weak)
};

/**
 * @size{0x4}
 */
struct J3DPEBlockTexEdge : public J3DPEBlock {
    virtual void load();                                   // _0C
    virtual u32 countDLSize();                             // _20
    virtual JBlockType getType() { return JBT_PETexEdge; } // _24 (weak)
    virtual ~J3DPEBlockTexEdge() { }                       // _78 (weak)
};

/**
 * @size{0x4}
 */
struct J3DPEBlockXlu : public J3DPEBlock {
    virtual void load();                               // _0C
    virtual u32 countDLSize();                         // _20
    virtual JBlockType getType() { return JBT_PEXlu; } // _24 (weak)
    virtual ~J3DPEBlockXlu() { }                       // _78 (weak)
};

#endif
