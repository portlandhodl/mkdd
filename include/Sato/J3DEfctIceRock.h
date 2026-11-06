#ifndef J3DEFCTICEROCK_H
#define J3DEFCTICEROCK_H

#include <JSystem/J3D/J3DAnmTransform.h>
#include <JSystem/J3D/J3DMtxCalc.h>
#include <JSystem/J3D/J3DAnmTextureSRTKey.h>

#include "Sato/AnmController.h"
#include "Sato/J3DEffectMgr.h"

class J3DEfctIceRock : public J3DEffectObj
{
public:
    J3DEfctIceRock();
    void loadAnmData();
    static const char *getBmdFileName();
    static const char *getBckFileName(u8);
    static const char *getBtkFileName();
    void onlyinit();
    void init();
    void calc();
    void setTrigger();
    void update();
    void setModelMtx();
    virtual ~J3DEfctIceRock();

    static J3DAnmTransform *sBckAnmTrans[1];
    static J3DMtxCalc *sBckMtxCalc[1];
    static J3DAnmTextureSRTKey *sTexSRTKey;

    int mKartNo;            // d8
    AnmController mAnmCtrl; // dc
    u8 _f0;                 // f0
    u8 _f1;                 // f1
    u16 _f2;                // f2
};

#endif // J3DEFCTICEROCK_H
