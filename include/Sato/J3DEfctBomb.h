#ifndef J3DEFCTBOMB_H
#define J3DEFCTBOMB_H

#include "Sato/J3DEffectMgr.h"

class J3DEfctBomb : public J3DEffectObj
{
public:
    J3DEfctBomb();
    static void loadAnmData();
    static const char *getBmdFileName();
    void onlyinit();
    void init();
    void calc();
    void update();
    virtual ~J3DEfctBomb();

    u8 _d8[0xf4 - 0xd8];
};

#endif // J3DEFCTBOMB_H
