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

void ArmCtrl::update_HandR(const LumbarCtrl &lumbar, float (*mtx)[4])
{
    JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > tmp;
    JGeometry::TVec3f pos(lumbar.mVec);

    IKCtrl::calcJointScaleMtx(&tmp, mtx);
    PSMTXMultVec(tmp, (Vec *)&pos, (Vec *)&pos);

    JGeometry::TVec3f t(mtx[0][3], mtx[1][3], mtx[2][3]);
    t.add(pos);
    mMtx.set(mtx);
    mMtx.setTrans(t);
}

