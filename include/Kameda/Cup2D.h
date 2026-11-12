#ifndef CUP2D_H
#define CUP2D_H

#include "kartEnums.h"

#include "JSystem/JKernel/JKRArchive.h"

class Cup2D
{
public:
    Cup2D(JKRArchive *archive);  // 0x80197f10
    ~Cup2D();                    // 0x80197fc0

    void *getCupTexture(ERaceGpCup cup, int rank);  // 0x80198014

    static Cup2D *create(JKRArchive *archive, JKRHeap *heap)
    {
        if (mCup2D == nullptr)
            mCup2D = new (heap, 0) Cup2D(archive);
        return mCup2D;
    }

    static Cup2D *ptr() { return mCup2D; }

private:
    static Cup2D *mCup2D;  // 0x80416388

    void *mTextures[CUP_MAX][3];
};

#endif  // CUP2D_H
