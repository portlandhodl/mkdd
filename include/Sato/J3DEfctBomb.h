#ifndef J3DEFCTBOMB_H
#define J3DEFCTBOMB_H

#include <JSystem/J3D/J3DAnmTransform.h>
#include <JSystem/J3D/J3DAnmTevRegKey.h>
#include <JSystem/J3D/J3DMtxCalc.h>
#include <JSystem/J3D/J3DAnmTextureSRTKey.h>

#include "Sato/AnmController.h"
#include "Sato/J3DEffectMgr.h"

class J3DEfctBomb : public J3DEffectObj
{
public:
    J3DEfctBomb();
    static void loadAnmData();
    static const char *getBmdFileName();
    static const char *getBckFileName();
    static const char *getBtkFileName();
    static const char *getBrkFileName(u8);
    void onlyinit();
    void init();
    void calc();
    void update();
    void changeColor(u8);
    virtual ~J3DEfctBomb();

    static J3DAnmTransform *sBckAnmTrans[1];
    static J3DMtxCalc *sBckMtxCalc[1];
    static J3DAnmTextureSRTKey *sTexSRTKey;
    static J3DAnmTevRegKey *sTevRegKey[4];
    static u8 sBrkAnmNo;

    u8 _dc;                 // dc
    u8 _dd;                 // dd
    u8 _de;                 // de
    u8 _df;                 // df
    AnmController mAnmCtrl; // e0
}; // 0xf4

#endif // J3DEFCTBOMB_H
