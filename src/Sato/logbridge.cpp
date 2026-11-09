#include "Sato/logbridge.h"
#include "Sato/GeographyObjMgr.h"

#include <stdio.h>
#include <string.h>

#include "JSystem/J3D/J3DModelLoader.h"
#include "JSystem/J3D/J3DTransform.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "JSystem/JMath/JMath.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Kaneshige/Course/Course.h"
#include "Kaneshige/ExModel.h"
#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/SimpleDrawer.h"
#include "Kaneshige/TexLODControl.h"
#include "Osako/ResMgr.h"
#include "Sato/ObjUtility.h"
#include "Sato/stMath.h"
#include "dolphin/mtx.h"
#include "JSystem/JGeometry/Util.h"
#include "types.h"

namespace {
    f32 mAddVelocityX = 3.0f;
    f32 mAddVelocityY = 3.0f;
    s16 mMaxVelCntX = 500;
    s16 mMaxVelCntY = 500;
    f32 sKartWeightMagXZ = 10.0f;
    f32 sKartWeightMagY = 1.0f;
    u8 mVeladdNodeNumX = 2;
    u8 mVeladdNodeNumY = 2;
    f32 mMaxLengthRatio = 1.5f;
    f32 mTension = 0.9f;
    f32 mAirFric = 0.98f;
    f32 mDirAdjRatio = 0.5f;
    f32 mScaleX = 1.0f;
    f32 mScaleY = 1.0f;
    f32 mGravity = -0.3f;
    f32 sHeightCheckYMax = 100.0f;
    f32 sHeightCheckYMin = 1000.0f;
    f32 sBridgeVelYRatio = 0.5f;
    f32 sWallDepth = 5.0f;
    f32 mHungPow;
}

StringBridge::StringBridge(const CrsData::SObject &object) : ExGeographyObj(object) {
    _25c = 0.0f;
    _260 = 0.0f;
    _278 = 0;
    _27a = 0;
    _27e = 0;
    _28c = 0;
    _290 = 0;
    _294 = 0;
    _298 = 0;
    _29c = 0;
    _280 = (u16)object.mParam1;
    _284 = (u8)object.mParam2;

    if (object.mParam5 != 0) {
        _260 = 100.0f;
        _25c = 3.0f * object.mParam5;
    }

    if (_280 < 0xa) {
        _280 = 0xa;
    }

    if (object.mParam4 == 0) {
        setParam();
    } else {
        char name[0x40];
        snprintf(name, 0x40, "/Bridge/Bridge_%04d.lbd", (u16)object.mParam4);
        void *bridgeParam = ResMgr::getPtr(ResMgr::mcArcCourse, name);
        if (bridgeParam == 0) {
            JUTReport(100, 50, "BridgeParam Not Found. %s", name);
        }

        _2d4 = ((f32 *)bridgeParam)[0];
        _2d8 = ((f32 *)bridgeParam)[1];
        _2dc = ((f32 *)bridgeParam)[2];
        _2e0 = ((f32 *)bridgeParam)[3];
        _2e4 = ((f32 *)bridgeParam)[4];
        _2e8 = ((f32 *)bridgeParam)[5];
        _2ec = ((f32 *)bridgeParam)[6];
        _2f0 = ((f32 *)bridgeParam)[7];
        _2f4 = ((f32 *)bridgeParam)[8];
        _2f8[0] = ((u8 *)bridgeParam)[0x24];
        _2f8[1] = ((u8 *)bridgeParam)[0x25];
        _2f8[2] = ((u8 *)bridgeParam)[0x26];
        _2f8[3] = ((u8 *)bridgeParam)[0x27];
        _2fc = ((f32 *)bridgeParam)[0xa];
        _300 = ((f32 *)bridgeParam)[0xb];
        _304 = ((f32 *)bridgeParam)[0xc];
        _308 = ((f32 *)bridgeParam)[0xd];
        _2f8[0] = 0;
    }

    init();
    setObjFlagSimpleDraw();
}

void StringBridge::setParam() {
    _2d4 = mAirFric;
    _2e4 = mTension;
    _2d8 = mDirAdjRatio;
    _30c = mMaxLengthRatio;
    _2dc = mScaleX;
    _2e0 = mScaleY;
    _2e4 = mTension;
    _2e8 = mGravity;
    _2f0 = sKartWeightMagXZ;
    _2f4 = sKartWeightMagY;
    _2ec = mHungPow;
    _2f8[0] = 0;
    _2f8[1] = 0;
    _2f8[2] = mVeladdNodeNumX;
    _2f8[2] = mVeladdNodeNumY;
    _2fc = mAddVelocityX;
    _300 = mAddVelocityY;
    _304 = mMaxVelCntX;
    _308 = mMaxVelCntY;
}

void StringBridge::reset() {
    resetObject();
    _58 = 1;
    init();
}

void StringBridge::init() {
    JGeometry::TVec3f pos;
    JGeometry::TVec3f xdir;
    JGeometry::TVec3f ydir;
    JGeometry::TVec3f zdir;
    JGeometry::TVec3f v14;
    JGeometry::TVec3f v20;
    JGeometry::TVec3f v2c;
    JGeometry::TVec3f v38;
    JGeometry::TVec3f v44;
    JGeometry::TVec3f v50;
    JGeometry::TVec3f v5c;
    JGeometry::TVec3f v68;
    JGeometry::TVec3f v74;
    JGeometry::TVec3f v80;
    JGeometry::TVec3f up(0.0f, 1.0f, 0.0f);

    _278 = 0;
    _27a = 0;

    pos = mPos;
    getAssignedModelXYZdir(&xdir, &ydir, &zdir);

    v14 = ydir;
    v14.scale(100.0f * mScale.x);
    v20 = zdir;
    v20.scale(100.0f * mScale.z);
    PSVECAdd(&pos, &v14, &v14);
    PSVECAdd(&v14, &v20, &v14);
    v44 = xdir;
    v44.scale(100.0f * mScale.z);
    v50 = xdir;
    v50.scale(-100.0f * mScale.x);
    v2c = v44;
    v2c.scale(-1.0f);

    mBridgeNodeMgr[0].setPosition(v14, v50, _280);
    mBridgeNodeMgr[1].setPosition(v68, v5c, _280);

    mBridgeNodeMgr[0]._1c = mBridgeNodeMgr[0]._18 * _2e4;
    mBridgeNodeMgr[1]._1c = mBridgeNodeMgr[1]._18 * _2e4;

    _1e8[0] = v80;
    _1e8[1] = v74;
    _1e8[2] = v5c;
    _1e8[3] = v68;

    _264 = 0.0f;
    for (u32 i = 0; i < 4; i++) {
        if (_264 < _1e8[i].y) {
            _264 = _1e8[i].y;
        }
    }

    _2c4.normalize(v50);

    for (u32 i = 0; i < 4; i++) {
        PSVECCrossProduct(&up, &_1b8[i], &_1b8[i]);
        if (0.0f < PSVECMag(&_1b8[i])) {
            PSVECNormalize(&_1b8[i], &_1b8[i]);
        }
        stMakePlaneParam(_218[i], _1b8[i], _1e8[i]);
    }

    _1b4 = PSVECMag(&v50);
    _1b0 = 0.5f * _1b4;

    if (_284 != 0) {
        _288 = _280 / _284 - 1;

        for (u32 i = 0; i < _2f8[2]; i++) {
            mBridgeNodeMgr[i]._14 |= 1 << (i + 1);
            mBridgeNodeMgr[i]._14 |= 1 << (i + 1);
        }
    }

    _2d0 = mBridgeNodeMgr[0]._18 * _30c;

    BridgeNode **nodes0 = mBridgeNodeMgr[0].mNodes;
    for (JSUPtrLink *link = mBridgeNodeMgr[0].getFirstLink(); link != mBridgeNodeMgr[0].mTail; link = link->getNext()) {
        BridgeNode *nodeA = (BridgeNode *)link->mData;
        BridgeNode *nodeB = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(0)->mData;
        nodeA->mVel.zero();
        PSVECAdd(&nodeB->mPos, &nodeA->mVel, &nodeA->mPos);
        nodeA->_18 = nodeA->mPos;
        nodes0++;
    }
}

void StringBridge::loadmodel(J3DModelData *modelData) {
    loadmodel_base(modelData);
}

void StringBridge::loadmodel_base(J3DModelData *modelData) {
    u32 nodeNum = _280 - 1;

    if (_28c == 0) {
        _28c = new ExModel[nodeNum];
    }
    if (_290 == 0 && _294 == 0) {
        _290 = new ExModel[nodeNum - 1];
        _294 = new ExModel[nodeNum - 1];
        _2a0 = new JGeometry::TVec3f[nodeNum];
        _2a4 = new JGeometry::TVec3f[nodeNum];
    }
    if (_284 != 0) {
        if (_298 == 0 && _29c == 0) {
            _298 = new ExModel[_288];
            _29c = new ExModel[_288];
        }
    }

    ExModel *model = _28c;
    for (u32 i = 0; i < nodeNum; i++, model++) {
        model->setModelData(modelData);
        RCMGetCourse()->setFogInfo(modelData);
        ExModel::setLODBias(modelData, TexLODControl::getManager()->getLODBias(TexLODControl::cLODBiasID_Geography));
    }

    JGeometry::TVec3f *mdlData0 = *(JGeometry::TVec3f **)(*(u32 *)((u32)modelData + 0x28));
    JGeometry::TVec3f v = *(JGeometry::TVec3f *)((u32)mdlData0 + 0x48);
    _2b0 = v.y;
    _2ac = v.z;
    createColModel(modelData);
    _2b8 = 1.0f;
    _2bc = 1.0f;
    _2c0 = 1.0f;
    _2c0 = _1b0 / _2ac;

    if (getLopeBmdFileName() != 0) {
        _27e = 1;
        J3DModelData *model2 = (J3DModelData *)J3DModelLoaderDataBase::load(
            ResMgr::getPtr(ResMgr::mcArcCourse, getLopeBmdFileName()), 0x18210010);
        ExModel::patchModelData(model2);
        GeographyObjManager::gMgr->_20 = (u32)model2;

        ExModel *model3 = _290;
        ExModel *model4 = _294;
        for (u32 i = 0; i < nodeNum - 1; i++, model3++, model4++) {
            model3->setModelData(model2);
            RCMGetCourse()->setFogInfo(model2);
            ExModel::setLODBias(model2, TexLODControl::getManager()->getLODBias(TexLODControl::cLODBiasID_Geography));
            model4->setModelData(model2);
            RCMGetCourse()->setFogInfo(model2);
            ExModel::setLODBias(model2, TexLODControl::getManager()->getLODBias(TexLODControl::cLODBiasID_Geography));
        }

        if (_284 != 0) {
            ExModel *model5 = _298;
            ExModel *model6 = _29c;
            for (u32 i = 0; i < _288; i++, model5++, model6++) {
                model5->setModelData(model2);
                RCMGetCourse()->setFogInfo(model2);
                ExModel::setLODBias(model2, TexLODControl::getManager()->getLODBias(TexLODControl::cLODBiasID_Geography));
                model6->setModelData(model2);
                RCMGetCourse()->setFogInfo(model2);
                ExModel::setLODBias(model2, TexLODControl::getManager()->getLODBias(TexLODControl::cLODBiasID_Geography));
            }
        }

        JGeometry::TVec3f v2 = *(JGeometry::TVec3f *)((u32)*(u32 **)(*(u32 *)((u32)model2 + 0x28)) + 0x48);
        _2b4 = v2.z;
    }
}

void StringBridge::createColModel(J3DModelData *modelData) {
    mExBounds = new ObjColBlock(modelData, mScale);
}

void StringBridge::createModel(JKRSolidHeap *heap, u32 p2) {
    ExModel *model = _28c;

    for (u32 i = 0; i < _280 - 1; i++, model++) {
        model->createModel(heap, p2, 0);
    }

    if (_290 != 0 && _294 != 0) {
        ExModel *model2 = _290;
        ExModel *model3 = _294;

        for (u32 i = 0; i < _280 - 2; i++, model2++, model3++) {
            model2->createModel(heap, p2, 0);
            model3->createModel(heap, p2, 0);
        }
    }

    if (_288 != 0) {
        if (_298 != 0 && _29c != 0) {
            ExModel *model4 = _298;
            ExModel *model5 = _29c;

            for (u32 i = 0; i < _288; i++, model4++, model5++) {
                model4->createModel(heap, p2, 0);
                model5->createModel(heap, p2, 0);
            }
        }
    }
}

void StringBridge::makeSharedDL() {
    _28c->makeSharedDL();

    J3DModelData *modelData = _290->getModelData();
    if (_27e != 0) {
        modelData->newSharedDisplayList(0x40000);
        modelData->simpleCalcMaterial(0, *(Mtx*)&j3dDefaultMtx);
        modelData->makeSharedDL();
    }
}

void StringBridge::lockDisplayList() {
    for (u32 i = 0; i < _280 - 1; i++) {
        _28c[i].getModel()->lock();
    }

    if (_290 != 0 && _294 != 0) {
        for (u32 i = 0; i < _280 - 2; i++) {
            _290[i].getModel()->lock();
            _294[i].getModel()->lock();
        }
    }

    if (_288 != 0) {
        for (u32 i = 0; i < _288; i++) {
            if (mBridgeNodeMgr[0]._14 & (1 << i)) {
                _298[i].getModel()->lock();
            }
            if (mBridgeNodeMgr[1]._14 & (1 << i)) {
                _29c[i].getModel()->lock();
            }
        }
    }
}

void StringBridge::calc() {
    for (u32 j = 2; j < (u32)(mBridgeNodeMgr[0].mLinkCount - 2); j++) {
        calcparallel((BridgeNode *)mBridgeNodeMgr[0].getNthLink(j)->mData,
                     (BridgeNode *)mBridgeNodeMgr[1].getNthLink(j)->mData);
    }

    for (u32 i = 0; i < 2; i++) {
        for (u32 j = 2; j < (u32)(mBridgeNodeMgr[i].mLinkCount - 2); j++) {
            BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[i].getNthLink(j)->mData;
            PSVECScale(&node->mVel, &node->mVel, _2d4);
            node->mVel.y += _2e8;
        }
    }

    if (_284 != 0) {
        for (u32 k = 0; k < 2; k++) {
            u32 flag = mBridgeNodeMgr[k]._14;
            for (u32 j = _284, bit = 0; j < _280 - 1 - _284; j += _284, bit++) {
                if (flag & (1 << bit)) {
                    BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[k].getNthLink(j)->mData;
                    node->mVel.y += _2ec;
                }
            }
        }
    }

    for (u32 i = 0; i < 2; i++) {
        JGeometry::TVec3f vec;
        JGeometry::TVec3f vec2;

        mBridgeNodeMgr[i]._1c = mBridgeNodeMgr[i]._18 * _2e4;

        for (u32 j = 2; j <= (u32)(mBridgeNodeMgr[i].mLinkCount - 3); j++) {
            BridgeNode *nodeB = (BridgeNode *)mBridgeNodeMgr[i].getNthLink(j)->mData;
            BridgeNode *nodeA = (BridgeNode *)mBridgeNodeMgr[i].getNthLink(j - 1)->mData;
            PSVECSubtract(&nodeA->mPos, &nodeB->mPos, &vec);
            f32 mag = PSVECMag(&vec);
            if (mag > mBridgeNodeMgr[i]._1c) {
                PSVECNormalize(&vec, &vec2);
                PSVECScale(&vec2, &vec2, _2d8 * (mag - mBridgeNodeMgr[i]._1c));
                PSVECAdd(&vec2, &nodeB->mVel, &nodeB->mVel);
            }
        }

        for (u32 j = mBridgeNodeMgr[i].mLinkCount - 3; j >= 2; j--) {
            BridgeNode *nodeB = (BridgeNode *)mBridgeNodeMgr[i].getNthLink(j)->mData;
            BridgeNode *nodeA = (BridgeNode *)mBridgeNodeMgr[i].getNthLink(j + 1)->mData;
            PSVECSubtract(&nodeA->mPos, &nodeB->mPos, &vec);
            f32 mag = PSVECMag(&vec);
            if (mag > mBridgeNodeMgr[i]._1c) {
                PSVECNormalize(&vec, &vec2);
                PSVECScale(&vec2, &vec2, _2d8 * (mag - mBridgeNodeMgr[i]._1c));
                PSVECAdd(&vec2, &nodeB->mVel, &nodeB->mVel);
            }
        }
    }

    addVelNode();

    BridgeNode **nodes0 = mBridgeNodeMgr[0].mNodes;
    BridgeNode **nodes1 = mBridgeNodeMgr[1].mNodes;
    for (u32 k = 0; k < mBridgeNodeMgr[0].mLinkCount; k++) {
        BridgeNode *nodeA = nodes0[k];
        BridgeNode *nodeB = nodes1[k];
        PSVECAdd(&nodeA->mPos, &nodeB->mPos, mBridgeNodeMgr[0].mNodePos + k);
        PSVECScale(mBridgeNodeMgr[0].mNodePos + k, mBridgeNodeMgr[0].mNodePos + k, 0.5f);
    }
}

void StringBridge::update() {
    ExModel *model = _28c;

    for (JSUPtrLink *link = mBridgeNodeMgr[0].getFirstLink(); link != mBridgeNodeMgr[0].mTail; link = link->getNext(), model++) {
        model->update(0);
    }

    if (_27e != 0) {
        updateString();
    }
}

void StringBridge::updateString() {
    for (u32 i = 0; i < _280 - 2; i++) {
        _290[i].update(0);
        _294[i].update(0);
    }

    if (_284 == 0) {
        return;
    }

    for (u32 i = _284; i < _280 - _284 - 1; i += _284) {
        _298[i].update(0);
        _29c[i].update(0);
    }
}

void StringBridge::calcMtx(Mtx m, JGeometry::TVec3f pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    JGeometry::TVec3f posCopy = pos;
    calcBridgeModelMtx(m, posCopy, vec1, vec2);
}

void StringBridge::calcBridgeModelMtx(Mtx m, JGeometry::TVec3f pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    JGeometry::TVec3f vec;
    JGeometry::TVec3f normal;
    JGeometry::TVec3f dir;

    PSVECAdd(&pos, &vec1, &vec);
    PSVECScale(&vec, &vec, 0.5f);
    PSVECSubtract(&vec1, &pos, &vec);
    if (0.0f < PSVECMag(&vec)) {
        PSVECNormalize(&vec, &vec);
    }
    PSVECCrossProduct(&vec, &normal, &dir);
    if (0.0f < PSVECMag(&dir)) {
        PSVECNormalize(&dir, &dir);
    }
    PSVECCrossProduct(&dir, &vec, &normal);

    m[0][0] = vec.x;
    m[0][1] = normal.x;
    m[0][2] = dir.x;
}

void StringBridge::calcparallel(BridgeNode *nodeA, BridgeNode *nodeB) {
    JGeometry::TVec3f v44;
    JGeometry::TVec3f v38;
    JGeometry::TVec3f v2c;
    JGeometry::TVec3f v20;
    JGeometry::TVec3f v14;
    JGeometry::TVec3f v8;

    PSVECAdd(&nodeA->mPos, &nodeB->mPos, &v44);
    PSVECScale(&v44, &v44, 0.5f);
    PSVECSubtract(&v44, &nodeA->mPos, &v38);
    if (0.0f < PSVECMag(&v38)) {
        return;
    }
    PSVECNormalize(&v38, &v38);
    PSVECScale(&v38, &v2c, -_1b0);
    PSVECScale(&v38, &v20, _1b0);
    PSVECAdd(&v44, &v2c, &v14);
    PSVECAdd(&v44, &v20, &v8);
    nodeA->mPos = v14;
    nodeB->mPos = v8;
}

u32 StringBridge::getWaveNodeNum(u8 param) {
    switch (param) {
    case 0:
        return 2;
    case 1:
        return (u32)(0.5f * (f64)_280);
    case 2:
        return _280 - 3;
    default:
        return 0;
    }
}

void StringBridge::AddVel(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &vel) {
    if (RaceMgr::sRaceManager->mRaceInfo->getRaceMode() == 1) {
        return;
    }

    f32 ratio = getRightnodeVelRatio(vel);
    f32 oneMinus = 1.0f - ratio;
    JGeometry::TVec3f vec;
    JGeometry::TVec3f vec2;

    vec = vel;
    PSVECScale(&vel, &vec2, ratio);
    PSVECScale(&vel, &vec, oneMinus);

    if (_268 >= 2 && _268 <= (u32)(mBridgeNodeMgr[0].mLinkCount - 3)) {
        BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(_268)->mData;
        PSVECAdd(&vec2, &node->mVel, &node->mVel);
    }
    if (_268 >= 2 && _268 <= (u32)(mBridgeNodeMgr[1].mLinkCount - 3)) {
        BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[1].getNthLink(_268)->mData;
        PSVECAdd(&vec, &node->mVel, &node->mVel);
    }
}

void StringBridge::addVelNode() {
    if (_2f8[1] == 0) {
        return;
    }

    if (_2f8[1] == 1) {
        _27a = (u16)(_27a + _308 + 1);
        JGeometry::TVec3f vec;
        vec.set(0.0f, _300 * JMASinShort(_27a), 0.0f);

        u32 waveNode = getWaveNodeNum(_2f8[3]);
        if (waveNode >= 2 && waveNode <= (u32)(mBridgeNodeMgr[0].mLinkCount - 3)) {
            BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(waveNode)->mData;
            PSVECAdd(&vec, &node->mVel, &node->mVel);
        }
        if (waveNode >= 2 && waveNode <= (u32)(mBridgeNodeMgr[1].mLinkCount - 3)) {
            BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[1].getNthLink(waveNode)->mData;
            PSVECAdd(&vec, &node->mVel, &node->mVel);
        }
        return;
    }

    JGeometry::TVec3f vec;
    vec.set(0.0f, _2fc * JMASinShort(_278), _2c4.x);
    u32 waveNode = getWaveNodeNum(_2f8[2]);
    if (waveNode >= 2 && waveNode <= (u32)(mBridgeNodeMgr[0].mLinkCount - 3)) {
        BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(waveNode)->mData;
        PSVECAdd(&vec, &node->mVel, &node->mVel);
    }
    if (waveNode >= 2 && waveNode <= (u32)(mBridgeNodeMgr[1].mLinkCount - 3)) {
        BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[1].getNthLink(waveNode)->mData;
        PSVECAdd(&vec, &node->mVel, &node->mVel);
    }
}

f32 StringBridge::calcXZLength(JGeometry::TVec3f &pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    f32 x = 0.5f * (vec1.x + vec2.x) - pos.x;
    f32 z = 0.5f * (vec1.z + vec2.z) - pos.z;
    return stLength2(x, z);
}

f32 StringBridge::getplaneHeight(stPlaneParam &plane, const JGeometry::TVec3f &pos) {
    return -((plane.d + plane.x * pos.x + plane.y * pos.y + plane.z * pos.z) / plane.y) + _2b0 * _2e0;
}

f32 StringBridge::getplaneHeight(stPlaneParam &plane, JGeometry::TVec3f vec, u32 num, const JGeometry::TVec3f &pos, f32 &depth) {
    JGeometry::TVec3f v14;
    JGeometry::TVec3f v18;
    f32 f2;

    f2 = -((plane.d + plane.x * pos.x + plane.y * pos.y + plane.z * pos.z) / plane.y) + _2b0 * _2e0;

    v18 = vec;
    v18.sub(*mBridgeNodeMgr[0].mNodePos, v18);
    v14.set(0.5f * (mBridgeNodeMgr[0].mNodePos[num].x + mBridgeNodeMgr[1].mNodePos[num].x) - v18.x,
            0.5f * (mBridgeNodeMgr[0].mNodePos[num].y + mBridgeNodeMgr[1].mNodePos[num].y) - v18.y,
            0.5f * (mBridgeNodeMgr[0].mNodePos[num].z + mBridgeNodeMgr[1].mNodePos[num].z) - v18.z);

    f32 dist = _2b0 * _2e0 * _2b0 * _2e0 - v14.squared();

    if (dist < 0.0f) {
        depth = 0.0f;
    } else if (dist > 0.0f) {
        depth = dist * _2b0 * _2e0;
    }

    return f2 + depth;
}

f32 StringBridge::getRightnodeVelRatio(const JGeometry::TVec3f &pos) {
    JGeometry::TVec3f v8;
    JGeometry::TVec3f v14;
    JGeometry::TVec3f v20;

    v8.set(mBridgeNodeMgr[0].mNodePos[_268].x - mBridgeNodeMgr[1].mNodePos[_268].x,
           mBridgeNodeMgr[0].mNodePos[_268].y - mBridgeNodeMgr[1].mNodePos[_268].y,
           mBridgeNodeMgr[0].mNodePos[_268].z - mBridgeNodeMgr[1].mNodePos[_268].z);

    if (v8.length() != 0.0f && v8.squared() > 32.0f * FLT_EPSILON) {
        v8.normalize();
        v20.set(v8.x, v8.y, v8.z);
        PSVECSubtract(&pos, &mBridgeNodeMgr[0].mNodePos[_268], &v20);
        return PSVECDotProduct(&v20, &v8) / _1b4;
    }

    return 0.0f;
}

const char *StringBridge::getBmdFileName() {
    static const char *cBmdName = "/Objects/HBMaruta.bmd";
    return cBmdName;
}

const char *StringBridge::getLopeBmdFileName() {
    static const char *cMLopeName = "/Objects/HBMarutaLope.bmd";
    return cMLopeName;
}

StringBridge::~StringBridge() {}

f32 StringBridge::getMaxHeight() const {
    return _264;
}
