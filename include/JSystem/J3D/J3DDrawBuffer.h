#ifndef _JSYSTEM_J3D_J3DDRAWBUFFER_H
#define _JSYSTEM_J3D_J3DDRAWBUFFER_H

#include "JSystem/J3D/J3DTypes.h"
#include "types.h"

struct J3DPacket;
struct J3DMatPacket;
struct J3DDrawBuffer;
struct J3DCallBackPacket;
struct J3DTexture;

#ifndef __MWERKS__
inline f32 J3DCalcZValue(MtxPtr m, Vec v)
{
    return m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3];
}
#else
inline f32 J3DCalcZValue(register MtxPtr m, register Vec v)
{
    register f32 temp_f4;
    register f32 out;
    register f32 temp_f0;
    register f32 temp_f2;
    register f32 temp_f1 = 1.0f;

    asm {
        psq_l temp_f0, 0(v), 0, 0
        lfs temp_f2, 8(v)
        psq_l temp_f4, 32(m), 0, 0
        psq_l out, 40(m), 0, 0
        ps_merge00 temp_f2, temp_f2, temp_f1
        ps_mul temp_f4, temp_f0, temp_f4
        ps_madd out, temp_f2, out, temp_f4
        ps_sum0 out, out, out, out
    }

    return out;
}
#endif

#ifdef __MWERKS__
inline u32 J3DCalcTexHash(register J3DTexture *texture)
{
    register u32 hash;

    asm {
        lwz r4, 4(texture)
        rlwinm r0, r3, 5, 11, 26
        add r3, r4, r0
        lwz r0, 28(r3)
        add hash, r3, r0
        srwi hash, hash, 5
    }

    return hash;
}
#endif

#pragma push
#pragma optimize_for_size off

/**
 * @size{0x24}
 */
struct J3DDrawBuffer
{
typedef int (J3DDrawBuffer::*sortFunc)(J3DMatPacket *);
    typedef void (J3DDrawBuffer::*drawFunc)() const;

    enum EDrawType
    {
        J3DDRAW_Head = 0,
        J3DDRAW_Tail = 1,
    };

    enum ESortType
    {
        J3DSORT_Mat = 0,
        J3DSORT_MatAnm = 1,
        J3DSORT_Z = 2,
        J3DSORT_Model = 3,
        J3DSORT_INVALID = 4,
        J3DSORT_NonSort = 5,
    };

    J3DDrawBuffer() { initialize();    }

    J3DDrawBuffer(u32 size)
    {
        initialize();
        allocBuffer(size);
    }

    ~J3DDrawBuffer();

    void initialize();
    J3DErrType allocBuffer(u32);
    void frameInit();
    int entryMatSort(J3DMatPacket *);
    int entryMatAnmSort(J3DMatPacket *);
    int entryZSort(J3DMatPacket *);
    int entryModelSort(J3DMatPacket *);
    int entryInvalidSort(J3DMatPacket *);
    int entryNonSort(J3DMatPacket *);
    int entryImm(J3DPacket *, u16);
    void draw() const;
    void drawHead() const;
    void drawTail() const;
    void setCallBackPacket(J3DCallBackPacket *);

    inline void calcZRatio() { mZRatio = (mZFar - mZNear) / static_cast< f32 >(mBufferSize); }
    void setNonSort() { mSortType = J3DSORT_NonSort; }
    void setZSort() { mSortType = J3DSORT_Z; }
    void setDrawMode(u32 mode) { mDrawMode = mode; }

    static sortFunc sortFuncTable[6];
    static drawFunc drawFuncTable[2];
    static int entryNum;

    J3DPacket **mBuffer;           // _00
    u32 mBufferSize;            // _04
    u32 mDrawMode;                // _08
    ESortType mSortType;        // _0C
    f32 mZNear;                    // _10
    f32 mZFar;                    // _14
    f32 mZRatio;                // _18
    MtxPtr mZMtx;                   // _1C
    J3DPacket *mCallBackPacket; // _20
};

#pragma pop

#endif
