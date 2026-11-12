#ifndef J3DEFCTSPLASHDOWN_H
#define J3DEFCTSPLASHDOWN_H

#include <JSystem/J3D/J3DAnmTransform.h>
#include <JSystem/J3D/J3DAnmTevRegKey.h>
#include <JSystem/J3D/J3DMtxCalc.h>
#include <JSystem/J3D/J3DAnmTextureSRTKey.h>

#include "Sato/AnmController.h"
#include "Sato/J3DEffectMgr.h"

class J3DEfctSplashDown : public J3DEffectObj
{
public:
    J3DEfctSplashDown();
    static void loadAnmData();
    static const char *getBmdFileName();
    static const char *getBckFileName();
    static const char *getBtkFileName();
    static const char *getBrkFileName();
    virtual void onlyinit();
    virtual void init();
    virtual void calc();
    virtual void update();
    virtual const char *getJPAfilename();
    virtual ~J3DEfctSplashDown();

    int getKartIndex() const { return mKartNo; }

    static J3DAnmTransform *sBckAnmTrans[1];
    static J3DMtxCalc *sBckMtxCalc[1];
    static J3DAnmTextureSRTKey *sTexSRTKey;
    static J3DAnmTevRegKey *sTevRegKey;

    JGeometry::TPos3f *getRotMtx() { return (JGeometry::TPos3f *)((u8 *)this + 0x94); }
    int &getLife() { return *(int *)((u8 *)this + 0x90); }

    AnmController mAnmCtrl;    // dc
    u8 _f0;                    // f0
};

class J3DEfctMudSplashDown : public J3DEfctSplashDown
{
public:
    J3DEfctMudSplashDown();
    static void loadAnmData();
    static const char *getBmdFileName();
    static const char *getBckFileName();
    static const char *getBtkFileName();
    static const char *getBrkFileName();
    virtual void onlyinit();
    virtual const char *getJPAfilename();
    virtual ~J3DEfctMudSplashDown();

    static J3DAnmTransform *sBckAnmTrans[1];
    static J3DMtxCalc *sBckMtxCalc[1];
    static J3DAnmTextureSRTKey *sTexSRTKey;
    static J3DAnmTevRegKey *sTevRegKey;
};

#endif // J3DEFCTSPLASHDOWN_H
