#ifndef _JSYSTEM_J3D_J3DIND_H
#define _JSYSTEM_J3D_J3DIND_H

#include <dolphin/gx.h>
#include "JSystem/J3D/J3DTypes.h"
#include "types.h"

struct J3DIndTexCoordScaleInfo {
    J3DIndTexCoordScaleInfo& operator=(const J3DIndTexCoordScaleInfo& other)
    {
        *(u32*)this = *(u32*)&other;
        return *this;
    }

    u8 mScaleS; // _00
    u8 mScaleT; // _01
    u8 _02;     // _02
    u8 _03;     // _03
};

extern const J3DIndTexCoordScaleInfo j3dDefaultIndTexCoordScaleInfo;

struct J3DIndTexCoordScale : public J3DIndTexCoordScaleInfo {
    J3DIndTexCoordScale() { *(u32*)this = *(u32*)&j3dDefaultIndTexCoordScaleInfo; }
    J3DIndTexCoordScale(const J3DIndTexCoordScaleInfo& info) { *(u32*)this = *(u32*)&info; }
    ~J3DIndTexCoordScale() { }

    inline J3DIndTexCoordScale& operator=(const J3DIndTexCoordScale& other)
    {
        *(u32*)this = *(u32*)&other;
        return *this;
    }

    // NOTE: names fabricated
    u8 getScaleS() { return _00; }
    u8 getScaleT() { return _01; }

    u8 _00;    // _00
    u8 _01;    // _01
    u8 _02[2]; // _02
};

struct J3DIndTexMtxInfo {
    void operator=(const J3DIndTexMtxInfo& other);

    f32 _00;
    f32 _04;
    f32 _08;
    f32 _0C;
    f32 _10;
    f32 _14;
    char _18;
};

extern const J3DIndTexMtxInfo j3dDefaultIndTexMtxInfo;

/**
 * @size{0x1C}
 */
struct J3DIndTexMtx : public J3DIndTexMtxInfo {
    J3DIndTexMtx() { *(J3DIndTexMtxInfo*)this = j3dDefaultIndTexMtxInfo; }

    /** @fabricated */
    J3DIndTexMtx(const J3DIndTexMtxInfo& info) { this->J3DIndTexMtxInfo::operator=(info); }

    // NOTE: return types below are fabricated
    f32 (*getOffsetMtx())[3] { return (f32(*)[3])&_00; }
    char getScaleExp() { return _18; }
    void load(u32 id) { J3DGDSetIndTexMtx(GXIndTexMtxID(id + GX_ITM_0), getOffsetMtx(), getScaleExp()); }

    ~J3DIndTexMtx() { }
};

/** @fabricated */
struct J3DIndTexOrderInfo {
    inline J3DIndTexOrderInfo& operator=(const J3DIndTexOrderInfo& other)
    {
        *(u32*)this = *(u32*)&other;
        return *this;
    }

    u8 mTexCoordID; // _00
    u8 mTexMapID;   // _01
    u8 _02;         // _02
    u8 _03;         // _03
};

extern const J3DIndTexOrderInfo j3dDefaultIndTexOrderNull;
// extern const struct J3DIndTexOrder j3dDefaultIndTexOrderNull;

/**
 * @size{0x4}
 */
struct J3DIndTexOrder {
    J3DIndTexOrder() { *(u32*)this = *(u32*)&j3dDefaultIndTexOrderNull; }

    /** @fabricated */
    J3DIndTexOrder(const J3DIndTexOrderInfo& info) { *(u32*)this = *(u32*)&info; }

    // /** @fabricated */
    // inline J3DIndTexOrder(_GXTexCoordID coordID, GXTexMapID mapID)
    //     : mTexCoordID(coordID)
    //     , mTexMapID(mapID)
    // {
    // }

    // /** @fabricated */
    // inline J3DIndTexOrder(u8 coordID, u8 mapID, u8 p3, u8 p4)
    //     : mTexCoordID(coordID)
    //     , mTexMapID(mapID)
    //     , _02(p3)
    //     , _03(p4)
    // {
    // }

    inline J3DIndTexOrder& operator=(const J3DIndTexOrder& other)
    {
        *(u32*)this = *(u32*)&other;
        return *this;
    }

    u8 mTexCoordID; // _00
    u8 mTexMapID;   // _01
    u8 _02;         // _02
    u8 _03;         // _03
};

struct J3DIndBlock {
    virtual void reset(J3DIndBlock*);                                            // _08 (weak)
    virtual void diff(unsigned long) = 0;                                        // _0C
    virtual void load()              = 0;                                        // _10
    virtual u32 countDLSize();                                                   // _14 (weak)
    virtual JBlockType getType() = 0;                                            // _18
    virtual void setIndTexStageNum(unsigned char);                               // _1C (weak)
    virtual u8 getIndTexStageNum() const;                                        // _20 (weak)
    virtual void setIndTexOrder(unsigned long, J3DIndTexOrder);                  // _24 (weak)
    virtual void setIndTexOrder(unsigned long, const J3DIndTexOrder*);           // _28 (weak)
    virtual J3DIndTexOrder* getIndTexOrder(unsigned long);                       // _2C (weak)
    virtual void setIndTexMtx(unsigned long, const J3DIndTexMtx*);               // _30 (weak)
    virtual void setIndTexMtx(unsigned long, J3DIndTexMtx);                      // _34 (weak)
    virtual J3DIndTexMtx* getIndTexMtx(unsigned long);                           // _38 (weak)
    virtual void setIndTexCoordScale(unsigned long, const J3DIndTexCoordScale*); // _3C (weak)
    virtual void setIndTexCoordScale(unsigned long, J3DIndTexCoordScale);        // _40 (weak)
    virtual J3DIndTexCoordScale* getIndTexCoordScale(unsigned long);             // _44 (weak)
    virtual ~J3DIndBlock() { }                                                   // _48 (weak)
};

struct J3DIndBlockFull : public J3DIndBlock {
    inline J3DIndBlockFull() { initialize(); }

    virtual void reset(J3DIndBlock*);                                                                 // _08
    virtual void diff(unsigned long);                                                                 // _0C
    virtual void load();                                                                              // _10
    virtual u32 countDLSize();                                                                        // _14
    virtual JBlockType getType() { return JBT_IndFull; }                                              // _18 (weak)
    virtual void setIndTexStageNum(unsigned char stageNum) { mIndTexStageNum = stageNum; }            // _1C (weak)
    virtual u8 getIndTexStageNum() const { return mIndTexStageNum; }                                  // _20 (weak)
    virtual void setIndTexOrder(unsigned long index, const J3DIndTexOrder* order) { mOrders[index] = *order; }        // _28 (weak)
    virtual void setIndTexOrder(unsigned long index, J3DIndTexOrder order) { mOrders[index] = order; }                // _24 (weak)
    virtual J3DIndTexOrder* getIndTexOrder(unsigned long index) { return &mOrders[index]; }                           // _2C (weak)
    virtual void setIndTexMtx(unsigned long index, const J3DIndTexMtx* mtx) { mTexMtxs[index] = *mtx; }              // _30 (weak)
    virtual void setIndTexMtx(unsigned long index, J3DIndTexMtx mtx) { mTexMtxs[index] = mtx; }                      // _34 (weak)
    virtual J3DIndTexMtx* getIndTexMtx(unsigned long index) { return &mTexMtxs[index]; }                              // _38 (weak)
    virtual void setIndTexCoordScale(unsigned long index, const J3DIndTexCoordScale* scale) { mCoordScales[index] = *scale; } // _3C (weak)
    virtual void setIndTexCoordScale(unsigned long index, J3DIndTexCoordScale scale) { mCoordScales[index] = scale; }         // _40 (weak)
    virtual J3DIndTexCoordScale* getIndTexCoordScale(unsigned long index) { return &mCoordScales[index]; }            // _44 (weak)
    virtual ~J3DIndBlockFull() { }                                                                                    // _48 (weak)

    void initialize();

    u8 mIndTexStageNum;        // _04
    J3DIndTexOrder mOrders[4]; // _05
    u32 : 0;
    J3DIndTexMtx mTexMtxs[3];            // _18
    J3DIndTexCoordScale mCoordScales[4]; // _6C
};

struct J3DIndBlockNull : public J3DIndBlock {
    virtual void reset(J3DIndBlock*) { }                 // _08 (weak)
    virtual void diff(unsigned long) { }                 // _0C (weak)
    virtual void load() { }                              // _10 (weak)
    virtual JBlockType getType() { return JBT_IndNull; } // _18 (weak)
    virtual ~J3DIndBlockNull() { }                       // _48 (weak)
};

#endif
