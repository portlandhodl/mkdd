#include "Kawano/driver.h"

#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/JGeometry.h"
#include "JSystem/JGeometry/Matrix.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "Kaneshige/RaceMgr.h"
#include "Kawano/driverMgr.h"
#include "Sato/NeckCtrl.h"
#include "Sato/stMath.h"
#include "dolphin/mtx.h"

typedef JGeometry::TPosition3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > > TPos;

class LumbarCtrl : public IKCtrl
{
public:
    void calcScaleMtx(float (*)[4]);
    void calcString(const JGeometry::TVec3f &);
    void update(float (*)[4], DriverModel *);
    void update_damage(float (*)[4], DriverModel *);

    u8 _40[0x70 - 0x40];
    u8 m_70; // 0x70
    u8 _71[0xa8 - 0x71];
    f32 mScale; // 0xa8
};

class FootCtrl : public IKCtrl
{
public:
    void reset();
    void update_damage(float (*)[4], const LumbarCtrl &, int);
};

class ArmCtrl : public IKCtrl
{
public:
    void reset();
    void update_HandR(const LumbarCtrl &, float (*)[4]);
};

static const f32 sZeroQuatset[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

DriverModel *DriverModel::sCurDriver;

J3DJointCallBack DriverModel::mscaCallBackTbl[27] = {
    koshiCallBack,
    spinCallBack,
    footL1CallBack,
    footL2CallBack,
    footL3CallBack,
    footLPosCallBack,
    footR1CallBack,
    footR2CallBack,
    footR3CallBack,
    footRPosCallBack,
    armL1CallBack,
    armL2CallBack,
    armL3CallBack,
    nullptr,
    armR1CallBack,
    armR2CallBack,
    armR3CallBack,
    handRCallBack,
    faceCallBack,
    mouthCallBack,
    skirtCallBack,
    skirtPosCallBack,
    dHair1CallBack,
    dHair2CallBack,
    pHair1CallBack,
    pHair2CallBack,
    tie1CallBack,
};

void CheckMaxNeckAngle(float (*base)[4], float (*neck)[4], char axis, float maxCos)
{
    JGeometry::TVec3f baseDir(base[0][axis], base[1][axis], base[2][axis]);
    JGeometry::TVec3f neckDir(neck[0][axis], neck[1][axis], neck[2][axis]);
    if (PSVECMag(&baseDir) == 0.0f)
        return;
    if (neckDir.squared() <= JGeometry::TUtil<f32>::epsilon())
        return;

    stVecNormalize(baseDir);
    stVecNormalize(neckDir);
    if (baseDir.dot(neckDir) >= maxCos)
        return;

    JGeometry::TVec3f halfDir;
    halfDir.add(baseDir, neckDir);
    halfDir.scale(0.5f);
    stVecNormalize(halfDir);
    neckDir.scaleAdd(maxCos - 1.0f, halfDir, baseDir);
    stVecNormalize(neckDir);

    neck[0][axis] = neckDir.x;
    neck[1][axis] = neckDir.y;
    neck[2][axis] = neckDir.z;
}

static inline f32 getBoneLength(DriverLoader *loader, LumbarCtrl *lumbar, u16 boneNo)
{
    return loader->getBoneLength(boneNo) * lumbar->mScale;
}

static inline bool epsilonEqualsF(f32 a, f32 b, f32 eps)
{
    bool equal = false;
    f32 d = a - b;
    if (-eps <= d && d <= eps)
        equal = true;
    return equal;
}

static inline bool epsilonEquals(const JGeometry::TVec3f &a, const JGeometry::TVec3f &b, f32 eps)
{
    bool ret = false;
    bool xy = false;
    if (epsilonEqualsF(a.x, b.x, eps) && epsilonEqualsF(a.y, b.y, eps))
        xy = true;
    if (xy && epsilonEqualsF(a.z, b.z, eps))
        ret = true;
    return ret;
}

void DriverModel::setDirMtx(JGeometry::TVec3f &pos, JGeometry::TVec3f &target, JGeometry::TVec3f &up, TPos *m)
{
    if (PSVECMag(&up) == 0.0f)
        up.z = -1.0f;

    JGeometry::TVec3f dir;
    dir.sub(target, pos);
    if (PSVECMag(&dir) == 0.0f)
        dir.y = 1.0f;

    JGeometry::TVec3f x, y, z;
    PSVECNormalize(&dir, &x);
    PSVECCrossProduct(&up, &x, &z);
    PSVECNormalize(&z, &z);
    PSVECCrossProduct(&z, &x, &y);

    m->mMtx[0][0] = x.x;
    m->mMtx[1][0] = x.y;
    m->mMtx[2][0] = x.z;
    m->mMtx[0][1] = y.x;
    m->mMtx[1][1] = y.y;
    m->mMtx[2][1] = y.z;
    m->mMtx[0][2] = z.x;
    m->mMtx[1][2] = z.y;
    m->mMtx[2][2] = z.z;
    m->mMtx[0][3] = pos.x;
    m->mMtx[1][3] = pos.y;
    m->mMtx[2][3] = pos.z;
}

void DriverModel::calcMPos(JGeometry::TVec3f &pos, JGeometry::TVec3f &target, JGeometry::TVec3f &up, float len1, float len2, JGeometry::TVec3f *out)
{
    if (epsilonEquals(pos, target, JGeometry::TUtil<f32>::epsilon())) {
        *out = pos;
        return;
    }

    TPos m;
    setDirMtx(pos, target, up, &m);
    TPos inv;
    PSMTXInverse((Mtx &)m, (Mtx &)inv);
    JGeometry::TVec3f local;
    PSMTXMultVec((Mtx &)inv, &target, &local);

    JGeometry::TVec3f res;
    if (local.x < len1 + len2) {
        f32 len1Sq = len1 * len1;
        res.x = (local.x * local.x + (len1Sq - len2 * len2)) / (2.0f * local.x);
        f32 h = len1Sq - res.x * res.x;
        res.y = h < 0.0f ? 0.0f : JMAFastSqrt(h);
        res.z = 0.0f;
    } else {
        res.set(len1, 0.0f, 0.0f);
    }
    PSMTXMultVec((Mtx &)m, &res, out);
}

void DriverModel::calc1stJnt(J3DModel *model, u16 jnt1, u16 jnt2, u16 no, u16 bone1, u16 bone2)
{
    TPos m1, m2;
    PSMTXCopy(model->mMtxBuffer->mWorldMatrices[mDriverLoader->getJntBindNo(jnt1)], m1);
    PSMTXCopy(model->mMtxBuffer->mWorldMatrices[mDriverLoader->getJntBindNo(jnt2)], m2);

    m_6c4[no].set(m1.mMtx[0][3], m1.mMtx[1][3], m1.mMtx[2][3]);
    m_724[no].set(m1.mMtx[0][1], m1.mMtx[1][1], m1.mMtx[2][1]);

    TPos scale;
    IKCtrl::calcJointScaleMtx(&scale, m1);

    JGeometry::TVec3f target;
    if (mLumbarCtrl->m_70 & 4) {
        JGeometry::TVec3f dir, p, side, z;
        m2.getTrans(p);
        dir.sub(m_6c4[no], p);
        stVecNormalize(dir);
        m1.getZDir(z);
        PSVECCrossProduct(&z, &dir, &side);
        stVecNormalize(side);
        m_724[no] = side;
    } else {
        stVecNormalize(m_724[no]);
        m_724[no].negate();
    }

    m2.getTrans(target);
    f32 len1 = getBoneLength(mDriverLoader, mLumbarCtrl, bone1);
    f32 len2 = getBoneLength(mDriverLoader, mLumbarCtrl, bone2);
    calcMPos(m_6c4[no], target, m_724[no], len1, len2, &m_6f4[no]);
    setDirMtx(m_6c4[no], m_6f4[no], m_724[no], &m1);
    PSMTXConcat(m1, scale, m1);

    MtxPtr src = m1;
    PSMTXCopy(src, model->mMtxBuffer->mWorldMatrices[mDriverLoader->getJntBindNo(jnt1)]);
    PSMTXCopy(m1, j3dSys.mCurrentMtx);
}

void DriverModel::calc2ndJnt(J3DModel *model, u16 jnt1, u16 no, u16 jnt2)
{
    TPos dirMtx;
    Mtx m1, m2;
    PSMTXCopy(model->mMtxBuffer->mWorldMatrices[mDriverLoader->getJntBindNo(jnt1)], m1);
    PSMTXCopy(model->mMtxBuffer->mWorldMatrices[mDriverLoader->getJntBindNo(jnt2)], m2);

    TPos scale;
    IKCtrl::calcJointScaleMtx(&scale, m1);

    JGeometry::TVec3f target(m2[0][3], m2[1][3], m2[2][3]);
    m_724[no].negate();
    m_724[no] += m_6f4[no] - m_6c4[no];
    PSVECNormalize(&m_724[no], &m_724[no]);
    setDirMtx(m_6f4[no], target, m_724[no], &dirMtx);
    PSMTXConcat(dirMtx, scale, dirMtx);

    MtxPtr src = dirMtx;
    PSMTXCopy(src, model->mMtxBuffer->mWorldMatrices[mDriverLoader->getJntBindNo(jnt1)]);
    PSMTXCopy(dirMtx, j3dSys.mCurrentMtx);
}

void DriverModel::calc3rdJnt(J3DModel *model, u16 jnt1, u16 jnt2, bool useJnt1)
{
    TPos tmp2;
    TPos tmp;
    Mtx m2, m1;
    PSMTXCopy(model->mMtxBuffer->mWorldMatrices[mDriverLoader->getJntBindNo(jnt1)], m1);
    PSMTXCopy(model->mMtxBuffer->mWorldMatrices[mDriverLoader->getJntBindNo(jnt2)], m2);

    IKCtrl::calcJointScaleMtx(&tmp, m1);
    PSMTXInverse(tmp, tmp);
    PSMTXConcat(m1, tmp, m1);

    IKCtrl::calcJointScaleMtx(&tmp2, m2);
    PSMTXConcat(m1, tmp2, m1);

    u16 jnt = jnt2;
    if (useJnt1)
        jnt = jnt1;

    MtxPtr src = m1;
    PSMTXCopy(src, model->mMtxBuffer->mWorldMatrices[mDriverLoader->getJntBindNo(jnt)]);
    PSMTXCopy(m1, j3dSys.mCurrentMtx);
}

bool DriverModel::CheckNoIK()
{
    return DriverData::mspDriverData->m_18 == 0;
}

bool DriverModel::koshiCallBack(J3DJoint *, int joint)
{
    if (sCurDriver != nullptr) {
        Mtx *wm = j3dSys.getModel()->mMtxBuffer->mWorldMatrices;
        sCurDriver->mLumbarCtrl->calcScaleMtx(wm[sCurDriver->mDriverLoader->getJntBindNo(0)]);
    }

    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    Mtx *mtx = &wm[d->mDriverLoader->getJntBindNo(0)];

    if (d->mLumbarCtrl->m_70 & 4)
        d->mLumbarCtrl->update_damage(*mtx, d);
    else
        d->mLumbarCtrl->update(*mtx, d);

    PSMTXCopy((Mtx &)d->mLumbarCtrl->mMtx, *mtx);
    PSMTXCopy(*mtx, j3dSys.mCurrentMtx);
    }
    return true;
}

bool DriverModel::spinCallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;

    }
    return true;
}

bool DriverModel::footL1CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (!IsFootLIKOn(d))
        return true;

    d->calc1stJnt(model, 2, 5, 0, 2, 3);
    }
    return true;
}

bool DriverModel::footL2CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (!IsFootLIKOn(d))
        return true;

    d->calc2ndJnt(model, 3, 0, 5);
    }
    return true;
}

bool DriverModel::footL3CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (!IsFootLIKOn(d))
        return true;

    if (!(d->mLumbarCtrl->m_70 & 4)) {
        TPos tmp2;
        TPos tmp;
        Mtx m2, m1;
        PSMTXCopy(model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(5)], m1);
        PSMTXCopy(model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(4)], m2);

        IKCtrl::calcJointScaleMtx(&tmp, m1);
        PSMTXInverse(tmp, tmp);
        PSMTXConcat(m1, tmp, m1);

        IKCtrl::calcJointScaleMtx(&tmp2, m2);
        PSMTXConcat(m1, tmp2, m1);

        u16 idx = 4;
        if (d->IsDash2())
            idx = 5;

        MtxPtr src = m1;
        PSMTXCopy(src, model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(idx)]);
        PSMTXCopy(m1, j3dSys.mCurrentMtx);
    }
    }
    return true;
}

bool DriverModel::footLPosCallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (!IsFootLIKOn(d))
        return true;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    Mtx *mtx = &wm[d->mDriverLoader->getJntBindNo(5)];
    if (d->mLumbarCtrl->m_70 & 4) {
        Mtx *a = &wm[d->mDriverLoader->getJntBindNo(13)];
        Mtx *b = &wm[d->mDriverLoader->getJntBindNo(17)];
        JGeometry::TVec3f pos;
        JGeometry::TVec3f va((*a)[0][3], (*a)[1][3], (*a)[2][3]);
        JGeometry::TVec3f vb((*b)[0][3], (*b)[1][3], (*b)[2][3]);
        pos.add(va, vb);
        pos.scale(0.5f);

        d->mLumbarCtrl->calcString(pos);
        d->mFootCtrlL->update_damage(*mtx, *d->mLumbarCtrl, 0);
        PSMTXCopy((Mtx &)d->mFootCtrlL->mMtx, *mtx);
        PSMTXCopy(*mtx, j3dSys.mCurrentMtx);
    }
    }
    return true;
}

bool DriverModel::footR1CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (!IsFootRIKOn(d))
        return true;

    d->calc1stJnt(model, 6, 9, 1, 2, 3);
    }
    return true;
}

bool DriverModel::footR2CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (!IsFootRIKOn(d))
        return true;

    d->calc2ndJnt(model, 7, 1, 9);
    }
    return true;
}

bool DriverModel::footR3CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (!IsFootRIKOn(d))
        return true;

    if (!(d->mLumbarCtrl->m_70 & 4)) {
        TPos tmp2;
        TPos tmp;
        Mtx m2, m1;
        PSMTXCopy(model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(9)], m1);
        PSMTXCopy(model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(8)], m2);

        IKCtrl::calcJointScaleMtx(&tmp, m1);
        PSMTXInverse(tmp, tmp);
        PSMTXConcat(m1, tmp, m1);

        IKCtrl::calcJointScaleMtx(&tmp2, m2);
        PSMTXConcat(m1, tmp2, m1);

        u16 idx = 8;
        if (d->IsDash2())
            idx = 9;

        MtxPtr src = m1;
        PSMTXCopy(src, model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(idx)]);
        PSMTXCopy(m1, j3dSys.mCurrentMtx);
    }
    }
    return true;
}

bool DriverModel::footRPosCallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (!IsFootRIKOn(d))
        return true;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    Mtx *mtx = &wm[d->mDriverLoader->getJntBindNo(9)];
    if (d->mLumbarCtrl->m_70 & 4) {
        d->mFootCtrlR->update_damage(*mtx, *d->mLumbarCtrl, 1);
        PSMTXCopy((Mtx &)d->mFootCtrlR->mMtx, *mtx);
        PSMTXCopy(*mtx, j3dSys.mCurrentMtx);
    }
    }
    return true;
}

bool DriverModel::armL1CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (d->IsGoal())
        return true;
    if (d->IsHappy())
        return true;
    if (d->IsArmLIKOn(d))
        d->calc1stJnt(model, 10, 13, 2, 0, 1);
    }
    return true;
}

bool DriverModel::armL2CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (d->IsGoal())
        return true;
    if (d->IsHappy())
        return true;
    if (d->IsArmLIKOn(d))
        d->calc2ndJnt(model, 11, 2, 13);
    }
    return true;
}

bool DriverModel::armL3CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (d->IsGoal())
        return true;
    if (d->IsHappy())
        return true;
    if (d->IsArmLIKOn(d)) {
        TPos tmp2;
        TPos tmp;
        Mtx m2, m1;
        PSMTXCopy(model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(13)], m1);
        PSMTXCopy(model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(12)], m2);

        IKCtrl::calcJointScaleMtx(&tmp, m1);
        PSMTXInverse(tmp, tmp);
        PSMTXConcat(m1, tmp, m1);

        IKCtrl::calcJointScaleMtx(&tmp2, m2);
        PSMTXConcat(m1, tmp2, m1);

        PSMTXCopy(m1, model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(12)]);
        PSMTXCopy(m1, j3dSys.mCurrentMtx);
    }
    }
    return true;
}

bool DriverModel::armR1CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (d->IsThrow())
        return true;
    if (d->IsTire())
        return true;
    if (d->IsJuggle())
        return true;
    if (IsAllIKOff(d))
        return true;
    if (d->IsGoal())
        return true;
    if (d->IsHappy())
        return true;
    if (d->IsAppeal())
        return true;
    if (d->IsArmRIKOn(d))
        d->calc1stJnt(model, 14, 17, 3, 0, 1);
    }
    return true;
}

bool DriverModel::armR2CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (d->IsThrow())
        return true;
    if (d->IsTire())
        return true;
    if (d->IsJuggle())
        return true;
    if (IsAllIKOff(d))
        return true;
    if (d->IsGoal())
        return true;
    if (d->IsHappy())
        return true;
    if (d->IsAppeal())
        return true;
    if (d->IsArmRIKOn(d))
        d->calc2ndJnt(model, 15, 3, 17);
    }
    return true;
}

bool DriverModel::armR3CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (d->IsThrow())
        return true;
    if (d->IsTire())
        return true;
    if (d->IsJuggle())
        return true;
    if (IsAllIKOff(d))
        return true;
    if (d->IsGoal())
        return true;
    if (d->IsHappy())
        return true;
    if (d->IsAppeal())
        return true;
    if (d->IsArmRIKOn(d)) {
        TPos tmp2;
        TPos tmp;
        Mtx m2, m1;
        PSMTXCopy(model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(17)], m1);
        PSMTXCopy(model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(16)], m2);

        IKCtrl::calcJointScaleMtx(&tmp, m1);
        PSMTXInverse(tmp, tmp);
        PSMTXConcat(m1, tmp, m1);

        IKCtrl::calcJointScaleMtx(&tmp2, m2);
        PSMTXConcat(m1, tmp2, m1);

        PSMTXCopy(m1, model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(16)]);
        PSMTXCopy(m1, j3dSys.mCurrentMtx);
    }
    }
    return true;
}

bool DriverModel::faceCallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    Mtx *a, *b;
    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (d->m_e4 != 0)
        return true;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    a = &wm[d->mDriverLoader->getJntBindNo(18)];
    b = &wm[d->mDriverLoader->getJntBindNo(0)];

    int flag = 0;
    if (d->IsDemo() || d->IsAttack() || d->IsChange() || d->IsDanger() ||
        d->IsFire() || d->IsDash() || d->IsTurn() || d->IsThrow() ||
        d->IsDamage() || d->IsStart() || d->IsBeforeStart() ||
        d->IsAppeal() || d->IsGoal())
        flag = 1;

    if (d->IsBeforeStart() || (IsAllIKOff(d) && !IsAllIKOffWithoutFace(d)))
        flag = 2;

    switch (d->mState) {
    case 118:
    case 119:
        flag = 2;
        break;
    }

    d->mNeckCtrl->update(*a, *b, d->mLumbarCtrl, flag);
    PSMTXCopy((Mtx &)d->mNeckCtrl->mMtx, *a);
    PSMTXCopy(*a, j3dSys.mCurrentMtx);
    }
    return true;
}

bool DriverModel::handRCallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    Mtx *mtx;
    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (d->IsThrow())
        return true;
    if (d->IsTire())
        return true;
    if (d->m_b8 == 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (d->IsGoal())
        return true;
    if (d->IsHappy())
        return true;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    ArmCtrl *arm = d->mArmCtrl;
    mtx = &wm[d->mDriverLoader->getJntBindNo(17)];
    arm->update_HandR(*d->mLumbarCtrl, *mtx);
    PSMTXCopy((Mtx &)arm->mMtx, *mtx);
    PSMTXCopy(*mtx, j3dSys.mCurrentMtx);
    }
    return true;
}

bool DriverModel::mouthCallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->mCharID != PETEY)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    }
    return true;
}

bool DriverModel::skirtCallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (d->IsDash1())
        return false;

    TPos tmp2;
    TPos tmp;
    Mtx m2, m1;
    PSMTXCopy(model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(21)], m1);
    PSMTXCopy(model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(20)], m2);

    IKCtrl::calcJointScaleMtx(&tmp, m1);
    PSMTXInverse((Mtx &)tmp, (Mtx &)tmp);
    PSMTXConcat(m1, (Mtx &)tmp, m1);

    IKCtrl::calcJointScaleMtx(&tmp2, m2);
    PSMTXConcat(m1, (Mtx &)tmp2, m1);

    MtxPtr src = m1;
    PSMTXCopy(src, model->mMtxBuffer->mWorldMatrices[d->mDriverLoader->getJntBindNo(20)]);
    PSMTXCopy(m1, j3dSys.mCurrentMtx);
    }
    return true;
}

bool DriverModel::skirtPosCallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (d->IsDash1())
        return false;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    Mtx *mtx = &wm[d->mDriverLoader->getJntBindNo(21)];
    if (d->mLumbarCtrl->m_70 & 4) {
        d->mFootCtrlR->update_damage(*mtx, *d->mLumbarCtrl, 2);
        PSMTXCopy((Mtx &)d->mFootCtrlR->mMtx, *mtx);
        PSMTXCopy(*mtx, j3dSys.mCurrentMtx);
    }
    }
    return true;
}

bool DriverModel::dHair1CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->mCharID != DAISY)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    Mtx *b = &wm[d->mDriverLoader->getJntBindNo(22)];
    ((HairCtrl *)d->mHairCtrl)->update_hair1(wm[d->mDriverLoader->getJntBindNo(18)], *b, d);
    PSMTXCopy((Mtx &)((HairCtrl *)d->mHairCtrl)->mMtx, *b);
    PSMTXCopy(*b, j3dSys.mCurrentMtx);
    }
    return true;
}

bool DriverModel::dHair2CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->mCharID != DAISY)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    Mtx *mtx = &wm[d->mDriverLoader->getJntBindNo(23)];
    ((HairCtrl *)d->mHairCtrl)->update_hair2(*mtx, d);
    PSMTXCopy((Mtx &)((HairCtrl *)d->mHairCtrl)->mMtx, *mtx);
    PSMTXCopy(*mtx, j3dSys.mCurrentMtx);
    }
    return true;
}

bool DriverModel::pHair1CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->mCharID != PEACH)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    Mtx *b = &wm[d->mDriverLoader->getJntBindNo(24)];
    ((HairCtrl *)d->mHairCtrl)->update_hair1(wm[d->mDriverLoader->getJntBindNo(18)], *b, d);
    PSMTXCopy((Mtx &)((HairCtrl *)d->mHairCtrl)->mMtx, *b);
    PSMTXCopy(*b, j3dSys.mCurrentMtx);
    }
    return true;
}

bool DriverModel::pHair2CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->mCharID != PEACH)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    Mtx *mtx = &wm[d->mDriverLoader->getJntBindNo(25)];
    ((HairCtrl *)d->mHairCtrl)->update_hair2(*mtx, d);
    PSMTXCopy((Mtx &)((HairCtrl *)d->mHairCtrl)->mMtx, *mtx);
    PSMTXCopy(*mtx, j3dSys.mCurrentMtx);
    }
    return true;
}

bool DriverModel::tie1CallBack(J3DJoint *, int joint)
{
    if (__rlwinm(__cntlzw((u32)DriverData::mspDriverData->m_18), 27, 24, 31))
        return true;
    if (joint == 0) {

    J3DModel *model = j3dSys.getModel();
    DriverModel *d = sCurDriver;
    if (d == nullptr)
        return true;
    if (d->mCharID != DONKEY_KONG)
        return true;
    if (d->m_d8 != 0)
        return true;
    if (IsAllIKOff(d))
        return true;
    if (d->IsDamage())
        return true;
    if (d->IsGoal())
        return true;

    Mtx *wm = model->mMtxBuffer->mWorldMatrices;
    Mtx *b = &wm[d->mDriverLoader->getJntBindNo(26)];
    ((HairCtrl *)d->mHairCtrl)->update_hair1(wm[d->mDriverLoader->getJntBindNo(1)], *b, d);
    PSMTXCopy((Mtx &)((HairCtrl *)d->mHairCtrl)->mMtx, *b);
    PSMTXCopy(*b, j3dSys.mCurrentMtx);
    }
    return true;
}

int DriverModel::IsFootLIKOn(DriverModel *d)
{
    if (d->IsDash1())
        return false;
    if (d->IsDash2())
        return false;

    switch (d->mCharID) {
    case PARATROOPA:
    case KOOPATROOPA:
    case YOSHI:
    case BIRDO:
        if (d->IsDriftRight() && !d->IsBack())
            return false;
        break;
    }
    return true;
}

int DriverModel::IsFootRIKOn(DriverModel *d)
{
    if (d->IsDash1())
        return false;
    if (d->IsDash2())
        return false;

    switch (d->mCharID) {
    case PARATROOPA:
    case KOOPATROOPA:
    case YOSHI:
    case BIRDO:
        if (d->IsDriftLeft() && !d->IsBack())
            return false;
        break;
    }
    return true;
}

int DriverModel::IsArmLIKOn(DriverModel *d)
{
    if (d->IsAppeal()) {
        switch (d->mCharID) {
        case PARATROOPA:
        case KOOPATROOPA:
        case WARIO:
        case WALUIGI:
        case DONKEY_KONG:
        case DIDDY_KONG:
        case BOWSER:
        case BOWSER_JR:
        case KING_BOO:
        case PETEY:
            return false;
        }
    }

    switch (d->mCharID) {
    case KING_BOO:
        return false;
    case WARIO:
    case PETEY:
        if (d->IsDriftLeft() && !d->IsBack() && d->m_b8 == 0)
            return false;
        if (d->IsDriftRight() && d->IsBack())
            return false;
        break;
    case BOWSER:
        if ((d->IsDrift() || d->IsWait() || d->IsTandem()) && d->IsBack())
            return false;
        break;
    }
    return true;
}

int DriverModel::IsArmRIKOn(DriverModel *d)
{
    switch (d->mCharID) {
    case KING_BOO:
        return false;
    case WARIO:
    case PETEY:
        if (d->IsDriftRight() && !d->IsBack() && d->m_b8 == 0)
            return false;
        break;
    }
    return true;
}

int DriverModel::IsAllIKOff(DriverModel *d)
{
    if (RaceMgr::sRaceManager->mRaceInfo->mIsTinyProcess == 1)
        return true;
    if (d->m_e8 != 0)
        return true;
    if (d->m_e4 != 0)
        return true;
    if (d->IsGoal())
        return true;
    if ((int)IsAllIKOffWithoutFace(d))
        return true;

    switch (d->mCharID) {
    case WALUIGI:
        if (d->IsSit() || d->IsFire())
            return true;
        break;
    }
    return false;
}

int DriverModel::IsAllIKOffWithoutFace(DriverModel *d)
{
    return !!d->IsDrive() ? 1 : 0;
}

void DriverModel::SetOffset(J3DModel *model, DriverModel *, u16)
{
    PSMTXIdentity(model->mMtxBuffer->mWorldMatrices[0]);
}

void DriverModel::SetOffset2(DriverModel *d, u16)
{
    PSMTXIdentity(d->getModel()->mMtxBuffer->mWorldMatrices[0]);
}
