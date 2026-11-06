#ifndef _JSYSTEM_J3D_J3DCLUSTER_H
#define _JSYSTEM_J3D_J3DCLUSTER_H

#include "JSystem/J3D/J3DTypes.h"
#include "types.h"

struct J3DDeformer;

struct J3DCluster {
    f32 _00;                // _00
    f32 _04;                // _04
    int _08;                // _08
    u8 _0C;                 // _0C
    u8 _0D[3];              // _0D
    u16 _10;                // _10
    u16 _12;                // _12
    u16 _14;                // _14
    u16 _16;                // _16
    u16 _18[2];             // _18
    int _1C;                // _1C
    J3DDeformer* mDeformer; // _20
};

struct J3DClusterKey {
    u16 _00;
    u16 _02;
    u16* _04;
    u16* _08;
};

struct J3DClusterVertex {
    u16 _00;
    u16 _02;
    u16* _04;
    u16* _08;
};

#endif
