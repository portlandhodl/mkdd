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

void BridgeNodeManager::setPosition(JGeometry::TVec3f &pos, JGeometry::TVec3f &dir, u32 num) {
    u32 count = num;
    if (mNodes == 0) {
        mNodes = new BridgeNode[count];
    }
    if (mNodePos == 0) {
        mNodePos = new JGeometry::TVec3f[count - 1];
    }

    u32 i;
    BridgeNode *node = mNodes;
    for (i = 0; i < num; i++, node++) {
        append(&node->mLink);
    }

    _14 = 0;
    JGeometry::TVec3f n;
    JGeometry::TVec3f v;
    PSVECSubtract(&dir, &pos, &v);
    if (0.0f < PSVECMag(&v)) {
        PSVECScale(&v, &v, 1.0f / (num - 1));
        PSVECNormalize(&v, &n);
    }
    _18 = PSVECMag(&v);

    BridgeNode *first = (BridgeNode *)getFirstLink()->mData;
    first->mPos = pos;
    JSUPtrLink *prev = getFirstLink();

    for (JSUPtrLink *link = getFirstLink()->getNext(); link != 0; link = link->getNext(), prev = prev->getNext()) {
        BridgeNode *n = (BridgeNode *)link->mData;
        BridgeNode *p = (BridgeNode *)prev->mData;
        n->mVel.zero();
        PSVECAdd(&p->mPos, &v, &n->mPos);
        n->_18 = n->mPos;
    }
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
    _284 = object.mParam2;

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
        snprintf(name, 0x40, "/Bridge/Bridge_%04d.lbd", object.mParam4);
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
        _30c = *(f32 *)&object.mParam5;
        _2f8[0] = 0;
    }

    init();
    setObjFlagSimpleDraw();
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

    BridgeNode *nodes0 = mBridgeNodeMgr[0].mNodes;
    for (JSUPtrLink *link = mBridgeNodeMgr[0].getFirstLink(); link != mBridgeNodeMgr[0].mTail; link = link->getNext()) {
        BridgeNode *nodeA = (BridgeNode *)link->mData;
        BridgeNode *nodeB = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(0)->mData;
        nodeA->mVel.zero();
        PSVECAdd(&nodeB->mPos, &nodeA->mVel, &nodeA->mPos);
        nodeA->_18 = nodeA->mPos;
        nodes0++;
    }
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

    const char *lopeName = getLopeBmdFileName();
    if (lopeName != 0) {
        _27e = 1;
        J3DModelData *model2 = (J3DModelData *)J3DModelLoaderDataBase::load(
            ResMgr::getPtr(ResMgr::mcArcCourse, lopeName), 0x18210010);
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


void StringBridge::loadmodel(J3DModelData *modelData) {
    loadmodel_base(modelData);
}

void StringBridge::createModel(JKRSolidHeap *heap, u32 p2) {
    ExModel *model = _28c;
    u32 num = _280 - 1;

    for (u32 i = 0; i < num; i++, model++) {
        model->createModel(heap, p2, 0);
    }

    ExModel *model2 = _290;
    ExModel *model3 = _294;

    for (u32 i = 0; i < num - 1; i++, model2++, model3++) {
        model2->createModel(heap, p2, 0);
        model3->createModel(heap, p2, 0);
    }

    if (_288 != 0) {
        ExModel *model4 = _298;
        ExModel *model5 = _29c;

        for (u32 i = 0; i < _288; i++, model4++, model5++) {
            model4->createModel(heap, p2, 0);
            model5->createModel(heap, p2, 0);
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
    ExModel *model = _28c;
    u32 num = _280 - 1;

    for (u32 i = 0; i < num; i++, model++) {
        model->getModel()->lock();
    }

    ExModel *model2 = _290;
    ExModel *model3 = _294;

    for (u32 i = 0; i < num - 1; i++, model2++, model3++) {
        model2->getModel()->lock();
        model3->getModel()->lock();
    }

    if (_288 != 0) {
        ExModel *model4 = _298;
        ExModel *model5 = _29c;

        for (u32 i = 0; i < _288; i++, model4++, model5++) {
            if (mBridgeNodeMgr[0]._14 & (1 << i)) {
                model4->getModel()->lock();
            }
            if (mBridgeNodeMgr[1]._14 & (1 << i)) {
                model5->getModel()->lock();
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

    BridgeNode *nodes0 = mBridgeNodeMgr[0].mNodes;
    BridgeNode *nodes1 = mBridgeNodeMgr[1].mNodes;
    for (u32 k = 0; k < mBridgeNodeMgr[0].mLinkCount; k++) {
        BridgeNode *nodeA = &nodes0[k];
        BridgeNode *nodeB = &nodes1[k];
        PSVECAdd(&nodeA->mPos, &nodeB->mPos, mBridgeNodeMgr[0].mNodePos + k);
        PSVECScale(mBridgeNodeMgr[0].mNodePos + k, mBridgeNodeMgr[0].mNodePos + k, 0.5f);
    }
}

void StringBridge::update() {
    JSUPtrLink *link;
    ExModel *model = _28c;
    link = mBridgeNodeMgr[0].getFirstLink();

    for (; link != mBridgeNodeMgr[0].mTail; link = link->getNext(), model++) {
        model->update(0);
    }

    if (_27e != 0) {
        updateString();
    }
}

void StringBridge::simpleDraw(u32 p2) {
    _2b8 = _2dc;
    _2bc = _2e0;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f dir2;
    JGeometry::TVec3f pos;
    Mtx m;

    JSUPtrLink *l0 = mBridgeNodeMgr[0].getFirstLink();
    JSUPtrLink *l1 = l0->getNext();
    JSUPtrLink *l2 = mBridgeNodeMgr[1].getFirstLink();
    JSUPtrLink *l3 = l2->getNext();
    u32 k = 0;
    for (; l0 != mBridgeNodeMgr[0].mTail;
         l0 = l0->getNext(), l1 = l1->getNext(), l2 = l2->getNext(), l3 = l3->getNext(), k++) {
        BridgeNode *n0 = (BridgeNode *)l0->mData;
        BridgeNode *n1 = (BridgeNode *)l1->mData;
        BridgeNode *n2 = (BridgeNode *)l2->mData;
        BridgeNode *n3 = (BridgeNode *)l3->mData;

        PSVECSubtract(&n1->mPos, &n0->mPos, &dir);
        PSVECSubtract(&n3->mPos, &n2->mPos, &dir2);

        dir += dir2;

        if (dir.length() > 0.0f && dir.squared() > 3.0f * FLT_EPSILON) {
            dir.normalize();
        }
        pos = dir;

        calcMtx(m, pos, mBridgeNodeMgr[0].mNodePos[k], mBridgeNodeMgr[1].mNodePos[k]);
        _28c[k].setBaseTRMtx(m);
        _28c[k].setBaseScale(*(JGeometry::TVec3f *)&_2b8);

        f32 half = 0.5f * (_2ac * _2c0);
        JGeometry::TVec3f normal(m[0][2], m[1][2], m[2][2]);
        JGeometry::TVec3f trans(m[0][3], m[1][3], m[2][3]);
        _2a0[k].set(trans.x + normal.x * half, trans.y + normal.y * half, trans.z + normal.z * half);
        _2a4[k].set(trans.x - normal.x * half, trans.y - normal.y * half, trans.z - normal.z * half);
    }

    SimpleDrawer drawer;
    drawer.drawInit(_28c->getModelData());
    while (drawer.loadPreDrawSetting()) {
        for (u32 i = 0; i < _280 - 1; i++) {
            _28c[i].simpleDraw(p2, nullptr, 1);
        }
    }

    if (_27e != 0) {
        simpleDrawString(p2);
    }
}

void StringBridge::simpleDrawString(u32 p2) {
    Mtx m;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f v0;
    JGeometry::TVec3f v1;
    JGeometry::TVec3f w0;
    for (u32 i = 0; i < _280 - 2; i++) {
        v0 = _2a0[i];
        v1 = _2a0[i + 1];

        calcStringMtx(m, pos, v0, v1);
        _290[i].setBaseTRMtx(m);
        _290[i].setBaseScale(pos);

        w0 = _2a4[i];
        v1 = _2a4[i + 1];
        calcStringMtx(m, pos, w0, v1);
        _294[i].setBaseTRMtx(m);
        _294[i].setBaseScale(pos);
    }

    SimpleDrawer drawer;
    drawer.drawInit(_290->getModelData());
    while (drawer.loadPreDrawSetting()) {
        for (u32 i = 0; i < _280 - 2; i++) {
            _290[i].simpleDraw(p2, nullptr, 1);
            _294[i].simpleDraw(p2, nullptr, 1);
        }
    }

    if (_284 == 0) {
        return;
    }

    ExModel *modelA = _298;
    ExModel *modelB = _29c;
    u32 idx = _284;
    u32 bit = 0;
    for (; idx < _280 - _284 - 1; idx += _284, bit++) {
        if (mBridgeNodeMgr[0]._14 & (1 << bit)) {
            modelA->show();
        } else {
            modelA->hide();
        }

        calcVertiMtx(m, pos, (BridgeNode *)mBridgeNodeMgr[0].getNthLink(idx)->mData,
                     (BridgeNode *)mBridgeNodeMgr[0].getNthLink(idx + 1)->mData);
        modelA->setBaseTRMtx(m);
        modelA->setBaseScale(pos);

        if (mBridgeNodeMgr[0]._14 & (1 << bit)) {
            modelB->show();
        } else {
            modelB->hide();
        }

        calcVertiMtx(m, pos, (BridgeNode *)mBridgeNodeMgr[1].getNthLink(idx)->mData,
                     (BridgeNode *)mBridgeNodeMgr[1].getNthLink(idx + 1)->mData);
        modelB->setBaseTRMtx(m);
        modelB->setBaseScale(pos);

        modelA++;
        modelB++;
    }

    drawer.drawInit(_290->getModelData());
    while (drawer.loadPreDrawSetting()) {
        ExModel *da = _298;
        ExModel *db = _29c;
        for (u32 i = _284; i < _280 - _284 - 1; i += _284) {
            da->simpleDraw(p2, nullptr, 1);
            db->simpleDraw(p2, nullptr, 1);
            da++;
            db++;
        }
    }
}

void StringBridge::updateString() {
    ExModel *modelA;
    ExModel *modelB;
    u32 num;
    modelA = _290;
    modelB = _294;
    num = _280;
    for (u32 i = 0; i < num - 2; i++, modelA++, modelB++) {
        modelA->update(0);
        modelB->update(0);
    }

    if (_284 == 0) {
        return;
    }

    ExModel *modelC = _298;
    ExModel *modelD = _29c;
    for (u32 i = _284; i < _280 - _284 - 1; i += _284, modelC++, modelD++) {
        modelC->update(0);
        modelD->update(0);
    }
}

void StringBridge::calcMtx(Mtx m, JGeometry::TVec3f pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    calcBridgeModelMtx(m, pos, vec1, vec2);
}

void StringBridge::calcBridgeModelMtx(Mtx m, JGeometry::TVec3f pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    JGeometry::TVec3f dir(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f normal(0.0f, 0.0f, 1.0f);
    JGeometry::TVec3f vec(0.0f, 0.0f, 0.0f);

    vec = vec1;
    vec += vec2;
    PSVECScale(&vec, &vec, 0.5f);
    dir = vec2;
    dir -= vec1;
    if (0.0f < PSVECMag(&dir)) {
        PSVECNormalize(&dir, &dir);
    }
    PSVECCrossProduct(&dir, &pos, &normal);
    if (0.0f < PSVECMag(&normal)) {
        PSVECNormalize(&normal, &normal);
    }
    PSVECCrossProduct(&normal, &dir, &pos);

    m[0][0] = pos.x;
    m[0][1] = pos.y;
    m[0][2] = pos.z;
    m[1][0] = normal.x;
    m[1][1] = normal.y;
    m[1][2] = normal.z;
    m[2][0] = dir.x;
    m[2][1] = dir.y;
    m[2][2] = dir.z;
    m[3][0] = vec.x;
    m[3][1] = vec.y;
    m[3][2] = vec.z;
}

void StringBridge::calcStringMtx(Mtx m, JGeometry::TVec3f &pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    JGeometry::TVec3f sum;
    JGeometry::TVec3f vec;
    JGeometry::TVec3f forward;
    JGeometry::TVec3f right;

    forward.set(1.0f, 0.0f, 0.0f);
    right.set(0.0f, 1.0f, 0.0f);
    sum = vec1;
    sum += vec2;
    vec = sum;

    f32 mag = PSVECMag(&vec);
    vec.normalize();
    f32 scale = 0.5f * mag / _2b4;

    PSVECCrossProduct(&right, &vec, &forward);
    PSVECCrossProduct(&vec, &forward, &right);

    m[0][0] = forward.x;
    m[0][1] = right.x;
    m[0][2] = vec.x;
    m[0][3] = sum.x;
    m[1][0] = forward.y;
    m[1][1] = right.y;
    m[1][2] = vec.y;
    m[1][3] = sum.y;
    m[2][0] = forward.z;
    m[2][1] = right.z;
    m[2][2] = vec.z;
    m[2][3] = sum.z;

    pos.set(0.0f, 0.0f, scale);
}

void StringBridge::calcVertiMtx(Mtx m, JGeometry::TVec3f &pos, BridgeNode *nodeA, BridgeNode *nodeB) {
    JGeometry::TVec3f trans;
    JGeometry::TVec3f normal;
    JGeometry::TVec3f forward;
    JGeometry::TVec3f right;

    trans = nodeA->mPos;
    trans += nodeB->mPos;
    trans *= 0.5f;

    normal = nodeB->mPos;
    normal -= nodeA->mPos;
    normal.normalize();

    forward.set(1.0f, 0.0f, 0.0f);
    f32 scale = 1000.0f / _2b4;

    PSVECCrossProduct(&normal, &forward, &right);
    PSVECCrossProduct(&right, &normal, &forward);

    m[0][0] = forward.x;
    m[0][1] = right.x;
    m[0][2] = normal.x;
    m[0][3] = trans.x;
    m[1][0] = forward.y;
    m[1][1] = right.y;
    m[1][2] = normal.y;
    m[1][3] = trans.y;
    m[2][0] = forward.z;
    m[2][1] = right.z;
    m[2][2] = normal.z;
    m[2][3] = trans.z;

    pos.set(0.0f, 0.0f, scale);
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

u32 StringBridge::findNthCenter(const JGeometry::TVec3f &pos) {
    JGeometry::TVec3f posCopy = pos;
    u32 low = 0;
    u32 high = mBridgeNodeMgr[0].mLinkCount - 2;
    JGeometry::TVec3f lo[2] = { mBridgeNodeMgr[0].mNodePos[0], mBridgeNodeMgr[1].mNodePos[0] };
    JGeometry::TVec3f hi[2] = { mBridgeNodeMgr[0].mNodePos[high], mBridgeNodeMgr[1].mNodePos[high] };

    while (high - low > 1) {
        f32 d0 = calcXZLength(posCopy, lo[0], lo[1]);
        f32 d1 = calcXZLength(posCopy, hi[0], hi[1]);
        u32 delta = (u32)(0.5f * (f64)high);
        if (delta == 0) {
            delta = 1;
        }
        if (d0 < d1) {
            high -= delta;
            for (u32 i = 0; i < 2; i++) {
                hi[i] = mBridgeNodeMgr[i].mNodePos[high];
            }
        } else {
            low += delta;
            for (u32 i = 0; i < 2; i++) {
                lo[i] = mBridgeNodeMgr[i].mNodePos[low];
            }
        }
    }

    f32 d0 = calcXZLength(posCopy, lo[0], lo[1]);
    f32 d1 = calcXZLength(posCopy, hi[0], hi[1]);
    return (d0 < d1) ? low : high;
}

JGeometry::TVec3f StringBridge::getcollideNormalandDir(u32 idx, JGeometry::TVec3f &out1, JGeometry::TVec3f &out2) {
    out2 = mBridgeNodeMgr[0].mNodePos[idx];
    out2 -= mBridgeNodeMgr[1].mNodePos[idx];
    if (out2.length() > 0.0f) {
        out2.normalize();
    }

    BridgeNode *a0 = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(idx)->mData;
    BridgeNode *b0 = (BridgeNode *)mBridgeNodeMgr[1].getNthLink(idx)->mData;
    BridgeNode *a1 = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(idx + 1)->mData;
    BridgeNode *b1 = (BridgeNode *)mBridgeNodeMgr[1].getNthLink(idx + 1)->mData;

    JGeometry::TVec3f v0 = a0->mPos;
    v0 -= b0->mPos;
    JGeometry::TVec3f v1 = a1->mPos;
    v1 -= b1->mPos;
    out1 = v0;
    out1 += v1;
    if (out1.length() > 0.0f) {
        out1.normalize();
    }

    JGeometry::TVec3f tmp;
    PSVECCrossProduct(&out2, &out1, &tmp);
    PSVECCrossProduct(&tmp, &out2, &out1);
    return tmp;
}

void StringBridge::AddVel(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &vel) {
    if (RaceMgr::sRaceManager->mRaceInfo->getRaceMode() == 1) {
        return;
    }

    f32 ratio = getRightnodeVelRatio(pos);
    f32 oneMinus = 1.0f - ratio;
    JGeometry::TVec3f vec;
    JGeometry::TVec3f vec2;

    vec = vel;
    vec2 = vel;
    vec *= oneMinus;
    vec2 *= ratio;
    vec2.y *= _2f4;

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
    JGeometry::TVec3f vec;
    if (_2f8[1] != 0) {
        _27a = (u16)(_27a + _308 + 1);
        vec.set(0.0f, _300 * JMASinShort(_27a), 0.0f);

        if (_2f8[1] == 1) {
            u32 waveNode = getWaveNodeNum(_2f8[3]);
            if (waveNode >= 2 && waveNode <= (u32)(mBridgeNodeMgr[0].mLinkCount - 3)) {
                BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(waveNode)->mData;
                PSVECAdd(&vec, &node->mVel, &node->mVel);
            }
            if (waveNode >= 2 && waveNode <= (u32)(mBridgeNodeMgr[1].mLinkCount - 3)) {
                BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[1].getNthLink(waveNode)->mData;
                PSVECAdd(&vec, &node->mVel, &node->mVel);
            }
        } else {
            vec.set(0.0f, _2fc * JMASinShort(_278), _2c4.x);
            u32 waveNode = getWaveNodeNum(_2f8[3]);
            if (waveNode >= 2 && waveNode <= (u32)(mBridgeNodeMgr[0].mLinkCount - 3)) {
                BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(waveNode)->mData;
                PSVECAdd(&vec, &node->mVel, &node->mVel);
            }
            if (waveNode >= 2 && waveNode <= (u32)(mBridgeNodeMgr[1].mLinkCount - 3)) {
                BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[1].getNthLink(waveNode)->mData;
                PSVECAdd(&vec, &node->mVel, &node->mVel);
            }
        }
    }

    if (_2f8[0] != 0) {
        _278 = (u16)(_278 + _304 + 1);
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
}

u32 StringBridge::getWaveNodeNum(u8 param) {
    switch (param) {
    case 0:
        return 2;
    case 1:
        return (u32)(0.5f * _280);
    case 2:
        return _280 - 3;
    default:
        return 0;
    }
}

u8 StringBridge::Search_Bound(const JGeometry::TVec3f &pos) {
    u32 ret = 0;
    u8 hit = 0;

    if (_260) {
        if (pos.y > _1e8[0].y + _25c) {
            return false;
        }
    }
    if (pos.y < _1e8[0].y - _258) {
        return false;
    }

    for (u8 i = 0; i < 4; i++) {
        hit += stSearchInSurface(pos, _1b8[i], _1e8[i]);
    }

    if (hit == 4) {
        ret = 1;
    }
    return (u8)ret;
}

u8 StringBridge::Search_BoundRadius(const JGeometry::TVec3f &pos, f32 radius) {
    u32 ret = 0;
    u8 hit = 0;

    if (_260) {
        if (pos.y > _1e8[0].y + _25c) {
            return false;
        }
    }
    if (pos.y < _1e8[0].y - _258) {
        return false;
    }

    f32 depth = 0.0f;
    for (u8 i = 0; i < 4; i++) {
        hit += stCollideSurfaceAndSphere(pos, radius, _218[i], depth);
    }

    if (hit == 4) {
        ret = 1;
    }
    return (u8)ret;
}

void StringBridge::Search(const JGeometry::TVec3f &dir, const JGeometry::TVec3f &pos) {
    _268 = findNthCenter(dir);
    _26c = dir;

    JGeometry::TVec3f out1;
    JGeometry::TVec3f out2;
    JGeometry::TVec3f normal = getcollideNormalandDir(_268, out1, out2);

    JGeometry::TVec3f point = mBridgeNodeMgr[0].mNodePos[_268];
    point += normal;
    stPlaneParam plane;
    stMakePlaneParam(plane, point, out1);

    f32 height = getplaneHeight(plane, pos) - pos.y;
    f32 depth = 0.0f;
    if (height > 0.0f) {
        CrsGround ground(RCMGetCourse());
        ground.search(this, dir, pos);
        _258 = 0.0f;
        _25c = 0.0f;
        _260 = 0.0f;
        ground.getNormal(&normal);
        _2b0 = ground.getHeight();
        _2d4 = 0.0f;
        _2d8 = (f32)_25c;
        _2dc = (f32)_25c;
        _2e0 = (f32)_25c;
    } else {
        depth = stCollideLineToPlaneIn(pos, dir, plane);
        if (0.0f > depth && depth > -1.0f) {
            depth = stGetCollideDepthFromT(pos, dir, depth);
        }
        f32 maxDepth = _2b0 * _2e0;
        _258 = depth;
    }
}

void StringBridge::Search_Wall(const JGeometry::TVec3f &pos, f32 radius) {
    _268 = findNthCenter(pos);
    JGeometry::TVec3f out1;
    JGeometry::TVec3f out2;
    JGeometry::TVec3f normal = getcollideNormalandDir(_268, out1, out2);

    JGeometry::TVec3f point;
    point = mBridgeNodeMgr[0].mNodePos[_268];
    point += normal;

    stPlaneParam plane;
    stMakePlaneParam(plane, point, out1);

    f32 height = getplaneHeight(plane, pos) - pos.y;
    if (height <= -sHeightCheckYMin || height > sHeightCheckYMax) {
        return;
    }

    f32 depth = 0.0f;
    if (stCollideSurfaceAndSphere(pos, radius, plane, depth) && depth < 0.25f * radius) {
        CrsGround ground(RCMGetCourse());
        ground.search(this, pos, pos);
        _258 = ground.getHeight();
        _25c = depth;
    } else {
        _25c = 0.0f;
    }
}

f32 StringBridge::calcXZLength(JGeometry::TVec3f &pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    f32 x = 0.5f * (vec1.x + vec2.x);
    f32 z = 0.5f * (vec1.z + vec2.z);
    return stLength2(x - pos.x, z - pos.z);
}

stPlaneParam StringBridge::getSidePlaneParam(const JGeometry::TVec3f &pos, u32 idx) {
    JGeometry::TVec3f p0 = mBridgeNodeMgr[0].mNodePos[idx];
    JGeometry::TVec3f p1 = mBridgeNodeMgr[1].mNodePos[idx];
    JGeometry::TVec3f center = p0;
    center += p1;
    center *= 0.5f;
    center.normalize();

    f32 d = PSVECDotProduct(&center, &pos);
    JGeometry::TVec3f normal;
    f32 width;
    if (d > 0.0f) {
        normal = _1b8[2];
        width = _1b0;
    } else {
        normal = _1b8[0];
        width = -_1b0;
    }

    JGeometry::TVec3f point;
    point = center;
    point.scale(width);
    point += pos;

    stPlaneParam plane;
    stMakePlaneParam(plane, point, normal);
    return plane;
}

f32 StringBridge::getplaneHeight(stPlaneParam &plane, const JGeometry::TVec3f &pos) {
    f32 h = plane.x * pos.x + plane.z * pos.z + plane.d;
    return -(h / plane.y) + _2b0 * _2e0;
}

f32 StringBridge::getplaneHeight(stPlaneParam &plane, JGeometry::TVec3f vec, u32 num, const JGeometry::TVec3f &pos, f32 &depth) {
    JGeometry::TVec3f v14;
    JGeometry::TVec3f v18;
    f32 f2;

    f2 = -((plane.z * pos.z + plane.x * pos.x + plane.d) / plane.y) + _2b0 * _2e0;

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

    BridgeNode *node1 = (BridgeNode *)mBridgeNodeMgr[1].getNthLink(_268)->mData;
    BridgeNode *node0 = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(_268)->mData;
    v8.zero();
    PSVECSubtract(&node1->mVel, &node0->mVel, &v8);

    if (v8.length() != 0.0f && v8.squared() > 32.0f * FLT_EPSILON) {
        v8.normalize();
        v14.set(v8.x, v8.y, v8.z);
        JGeometry::TVec3f p = pos;
        BridgeNode *node = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(_268)->mData;
        PSVECSubtract(&p, &node->mPos, &v20);
        return PSVECDotProduct(&v20, &v14) / _1b4;
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





















