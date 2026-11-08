#include "JSystem/J3D/J3DAnmLoader.h"

inline J3DMtxCalcAnmBase::~J3DMtxCalcAnmBase() { }

J3DMtxCalcAnmBase* J3DNewMtxCalcAnm(u32 type, J3DAnmTransform* animation)
{
    switch (type) {
    case 0:
        return new J3DMtxCalcAnimation<J3DMtxCalcAnimationAdaptorDefault<J3DMtxCalcCalcTransformBasic>, J3DMtxCalcJ3DSysInitBasic>(animation);
    case 1:
        return new J3DMtxCalcAnimation<J3DMtxCalcAnimationAdaptorDefault<J3DMtxCalcCalcTransformSoftimage>, J3DMtxCalcJ3DSysInitSoftimage>(animation);
    case 2:
        return new J3DMtxCalcAnimation<J3DMtxCalcAnimationAdaptorDefault<J3DMtxCalcCalcTransformMaya>, J3DMtxCalcJ3DSysInitMaya>(animation);
    }
    return nullptr;
}

void J3DJoint::appendChild(J3DJoint* newChild)
{
    if (mChild == nullptr) {
        mChild = newChild;
    } else {
        J3DJoint* curChild = mChild;
        while (curChild->getYounger() != nullptr) {
            curChild = curChild->getYounger();
        }
        curChild->setYounger(newChild);
    }
}
