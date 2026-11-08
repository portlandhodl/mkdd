#include "JSystem/J3D/J3DJointTree.h"
#include "JSystem/J3D/J3DModel.h"
#include "JSystem/J3D/J3DMtxCalc.h"

J3DJointTree::J3DJointTree()
    : mHierarchy(nullptr)
    , mFlags(0)
    , mModelDataType(0)
    , mRootNode(nullptr)
    , mTransformCalc(nullptr)
    , mJoints(nullptr)
    , mJointCnt(0)
    , mEnvelopeCnt(0)
    , mEnvelopeMixCnt(nullptr)
    , mEnvelopeMixIdx(nullptr)
    , mEnvelopeMixWeight(nullptr)
    , mInvJointMtx(nullptr)
    , mEnvelopeImptIdx(nullptr)
    , mMtxData()
    , _40(0)
    , mNametab(nullptr)
{
}

void J3DJointTree::clear()
{
}

void J3DJointTree::makeHierarchy(J3DJoint* joint, const J3DModelHierarchy** hierarchies, J3DMaterialTable* matTable,
                                 J3DShapeTable* shapeTable)
{
    J3DJoint* curJoint = joint;

    while (true) {
        J3DJoint* newJoint = nullptr;
        J3DMaterial* newMaterial = nullptr;
        J3DShape* newShape = nullptr;
        const J3DModelHierarchy* inf = *hierarchies;
        u16 val;

        switch (inf->_00) {
        case 1:
            *hierarchies = inf + 1;
            makeHierarchy(curJoint, hierarchies, matTable, shapeTable);
            break;
        case 2:
            *hierarchies = inf + 1;
            return;
        case 0:
            return;
        case 0x10: {
            J3DJoint** jointNodePointer = mJoints;
            *hierarchies = inf + 1;
            newJoint = jointNodePointer[inf->_02];
        } break;
        case 0x11:
            *hierarchies = inf + 1;
            val = inf->_02;
            newMaterial = matTable->getMaterialNodePointer(val);
            break;
        case 0x12:
            *hierarchies = inf + 1;
            val = inf->_02;
            newShape = shapeTable->getShapeNodePointer(val);
            break;
        }

        if (newJoint) {
            curJoint = newJoint;
            if (!joint) {
                mRootNode = newJoint;
            } else {
                joint->appendChild(newJoint);
            }
        } else if (newMaterial) {
            if (joint->getMesh()) {
                newMaterial->mNext = joint->getMesh();
            }
            joint->mMaterial = newMaterial;
            newMaterial->mJoint = joint;
        } else if (newShape) {
            newMaterial = joint->getMesh();
            newMaterial->mShape = newShape;
            newShape->mMaterial = newMaterial;
        }
    }
}

void J3DJointTree::findImportantMtxIndex()
{
    const s32 wEvlpMtxNum = getWEvlpMtxNum();
    u32 tableIdx = 0;
    const u32 drawFullWgtMtxNum = getDrawFullWgtMtxNum();
    u16* wEvlpMixIndex = getWEvlpMixIndex();
    const f32* wEvlpMixWeight = getWEvlpMixWeight();
    u16* wEvlpImportantMtxIdx = getWEvlpImportantMtxIndex();

    for (u16 i = 0; i < (u16)drawFullWgtMtxNum; i++) {
        wEvlpImportantMtxIdx[i] = mMtxData.mDrawMtxIdx[i];
    }

    for (s32 i = 0; i < wEvlpMtxNum; i++) {
        s32 mixNum = getWEvlpMixMtxNum(i);
        u16 bestIdx = 0;
        f32 bestWeight = -0.1f;

        for (s32 j = 0; j < mixNum; j++, tableIdx++) {
            if (bestWeight < wEvlpMixWeight[tableIdx]) {
                bestWeight = wEvlpMixWeight[tableIdx];
                bestIdx = wEvlpMixIndex[tableIdx];
            }
        }

        wEvlpImportantMtxIdx[i + getDrawFullWgtMtxNum()] = bestIdx;
    }
}

void J3DJointTree::calc(J3DMtxBuffer* buffer, const Vec& vec, const Mtx& mtx)
{
    mTransformCalc->init(vec, mtx);
    void (*const setMtxBuffer)(J3DMtxBuffer*) = J3DMtxCalc::setMtxBuffer;
    setMtxBuffer(buffer);
    J3DJoint* root = mRootNode;
    if (root) {
        J3DJoint::mCurrentMtxCalc = mTransformCalc;
        root->recursiveCalc();
    }
}
