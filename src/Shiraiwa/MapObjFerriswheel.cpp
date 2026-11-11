#include "Shiraiwa/Objects/MapObjFerriswheel.h"

#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DSys.h"
#include "JSystem/JMath/JMath.h"

int TMapObjFerriswheel::sRotFrame = 9000;
const char *TMapObjFerriswheel::scRotJointName = "kanran_skl_root";
const char *TMapObjFerriswheel::scGondolaJointName[16] = {
    "arm01", "arm02", "arm03", "arm04", "arm05", "arm06", "arm07", "arm08",
    "arm09", "arm10", "arm11", "arm12", "arm13", "arm14", "arm15", "arm16",
};
int TMapObjFerriswheel::sRotJointNo;

#include "JSystem/JAudio/JASFakeMatch2.h"

TMapObjFerriswheel::TMapObjFerriswheel(const CrsData::SObject &obj) : TMapObjHioNode(obj)
{
    PSMTXIdentity(mRotMtx);
    PSMTXIdentity(mRotStepMtx);
    PSMTXIdentity(mGondolaMtx);
}

TMapObjFerriswheel::~TMapObjFerriswheel() {}

const char *TMapObjFerriswheel::getBmdFileName()
{
    static const char *cBmdName = "/Objects/BabyKanran.bmd";
    return cBmdName;
}

void TMapObjFerriswheel::createModel(JKRSolidHeap *heap, u32 viewNo)
{
    mModel.createModel(heap, viewNo, 0);
    setCallBack(scRotJointName, centerCallBack);
    for (int i = 0; i < 16; i++) {
        setCallBack(scGondolaJointName[i], gondolaCallBack);
    }
}

void TMapObjFerriswheel::setCallBack(const char *name, int (*func)(J3DJoint *, int))
{
    int idx = mModel.getModelData()->getJointName()->getIndex(name);
    mModel.getModelData()->getJointNodePointer(idx)->mFunction = (J3DJointCallBack)func;
    mModel._14 = (u32 *)this;
}

void TMapObjFerriswheel::reset()
{
    resetObject();
    clrObjFlagHidding();
    clrObjFlagCheckItemHitting();
    clrAllCheckKartHitFlag();
    PSMTXIdentity(mRotMtx);
    PSMTXIdentity(mRotStepMtx);
    f32 s;
    f32 c;
    f32 angle = -6.2831855f / sRotFrame;
    s = sin(angle);
    c = cos(angle);
    mRotStepMtx[0][0] = c;
    mRotStepMtx[0][1] = -s;
    mRotStepMtx[1][0] = s;
    mRotStepMtx[1][1] = c;
    mRotStepMtx[2][2] = 1.0f;
    mRotStepMtx[0][2] = mRotStepMtx[2][0] = mRotStepMtx[1][2] = mRotStepMtx[2][1] = 0.0f;
    PSMTXIdentity(mGondolaMtx);

    JGeometry::TVec3f yDir;
    JGeometry::TVec3f xDir;
    JGeometry::TVec3f zDir;
    zDir.set(mObjData->zNorm);
    zDir.normalize();
    yDir.set(mObjData->yNorm);
    yDir.negate();
    yDir.normalize();
    xDir.cross(zDir, yDir);
    xDir.normalize();
    mGondolaMtx.setXYZDir(yDir, zDir, xDir);
}

void TMapObjFerriswheel::calc() {}

void TMapObjFerriswheel::rotate(Mtx mtx)
{
    PSMTXConcat(mRotStepMtx, mRotMtx, mRotMtx);
    PSMTXConcat(mtx, mRotMtx, mtx);
    PSMTXCopy(mtx, mModel.getModel()->getAnmMtx(sRotJointNo));
}

void TMapObjFerriswheel::setGondolaMtx(int jointIndex, Mtx mtx)
{
    JGeometry::TPos3f copy;
    PSMTXCopy(mGondolaMtx, copy);
    copy[0][3] = mtx[0][3];
    copy[1][3] = mtx[1][3];
    copy[2][3] = mtx[2][3];
    PSMTXCopy(copy, mModel.getModel()->getAnmMtx(jointIndex));
    PSMTXCopy(copy, mtx);
}

int TMapObjFerriswheel::centerCallBack(J3DJoint *joint, int param)
{
    if (param == 0) {
        u16 jointIndex = joint->mJointIdx;
        J3DModel *model = j3dSys._38;
        TMapObjFerriswheel *obj = (TMapObjFerriswheel *)((ExModel *)model->getUserArea())->_14;
        if (obj != nullptr) {
            Mtx &mtx = model->getAnmMtx(jointIndex);
            obj->rotate(mtx);
            PSMTXCopy(mtx, j3dSys.mCurrentMtx);
        }
    }
    return true;
}

int TMapObjFerriswheel::gondolaCallBack(J3DJoint *joint, int param)
{
    if (param == 0) {
        u16 jointIndex = joint->mJointIdx;
        J3DModel *model = j3dSys._38;
        TMapObjFerriswheel *obj = (TMapObjFerriswheel *)((ExModel *)model->getUserArea())->_14;
        if (obj != nullptr) {
            Mtx &mtx = model->getAnmMtx(jointIndex);
            obj->setGondolaMtx(jointIndex, mtx);
            PSMTXCopy(mtx, j3dSys.mCurrentMtx);
        }
    }
    return true;
}
