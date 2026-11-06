#include "JSystem/JGeometry.h"
#include "JSystem/JGeometry/Matrix.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "dolphin/mtx.h"

class LumbarCtrl
{
public:
    u8 _0[0xe4];
    JGeometry::TVec3f mVec;
};

class IKCtrl
{
public:
    IKCtrl(int, u8);
    virtual ~IKCtrl() {}
    static void calcJointScaleMtx(JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > *, float (*)[4]);
};

class ArmCtrl : public IKCtrl
{
public:
    ArmCtrl(int, u8);
    virtual ~ArmCtrl() {}
    void reset();
    void update_HandR(const LumbarCtrl &, float (*)[4]);

    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > mMtx; // 0x04
    u8 _34[0xc]; // 0x34
};

ArmCtrl::ArmCtrl(int a, u8 b) : IKCtrl(a, b)
{
    PSMTXIdentity((Mtx &)mMtx);
    reset();
}

void ArmCtrl::reset()
{
}

extern "C" static void ArmCtrl_calcJointScaleMtx(void *mtx, float (*p)[4])
{
    IKCtrl::calcJointScaleMtx((JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > *)mtx, p);
}

void ArmCtrl::update_HandR(const LumbarCtrl &lumbar, float (*mtx)[4])
{
    Mtx tmp;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f t;

    __asm__ volatile {
        psq_l f1,228(r4),0,0
        lfs f0,236(r4)
        mr r31,r5
        mr r30,r3
        la r3,tmp(r1)
        mr r4,r31
        psq_st f1,20(r1),0,0
        stfs f0,28(r1)
        bl ArmCtrl_calcJointScaleMtx
        la r4,pos(r1)
        la r3,tmp(r1)
        mr r5,r4
        bl PSMTXMultVec
        lfs f2,44(r31)
        lfs f1,28(r31)
        lfs f0,12(r31)
        psq_l f4,20(r1),0,0
        psq_l f3,28(r1),1,0
        psq_l f9,0(r31),0,0
        psq_l f8,8(r31),0,0
        stfs f2,t+8
        psq_l f7,16(r31),0,0
        psq_l f2,16(r1),1,0
        psq_l f6,24(r31),0,0
        ps_add f2,f2,f3
        psq_l f5,32(r31),0,0
        psq_l f3,40(r31),0,0
        stfs f0,t
        psq_st f9,4(r30),0,0
        stfs f1,t+4
        psq_st f8,12(r30),0,0
        psq_l f0,8(r1),0,0
        psq_st f7,20(r30),0,0
        ps_add f0,f0,f4
        psq_st f6,28(r30),0,0
        psq_st f0,8(r1),0,0
        psq_st f5,36(r30),0,0
        lfs f0,t
        psq_st f3,44(r30),0,0
        lfs f1,12(r1)
        psq_st f2,16(r1),1,0
        stfs f0,16(r30)
        lfs f0,t+8
        stfs f1,32(r30)
        stfs f0,48(r30)
    }
}

