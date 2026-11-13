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

    JSUPtrLink *head = getFirstLink();
    ((BridgeNode *)head->mData)->mPos = pos;
    JSUPtrLink *prev = getFirstLink();
    JSUPtrLink *link = head->getNext();
    for (; link != 0; link = link->getNext(), prev = prev->getNext()) {
        BridgeNode *n = (BridgeNode *)link->mData;
        BridgeNode *p = (BridgeNode *)prev->mData;
        n->mVel.zero();
        PSVECAdd(&p->mPos, &v, &n->mPos);
        n->_18 = n->mPos;
    }
}

void BridgeNodeManager::calcMove(f32 ratio) {
    u32 j;
    u32 n = getNumLinks() - 3;
    for (j = 2; j <= n; j++) {
        JSUPtrLink *link = getNthLink(j);
        JSUPtrLink *prev = getNthLink(j - 1);
        calcbetweenVel((BridgeNode *)link->getObjectPtr(), (BridgeNode *)prev->getObjectPtr(), ratio);
    }
    for (; n >= 2; n--) {
        JSUPtrLink *link = getNthLink(n);
        JSUPtrLink *next = getNthLink(n + 1);
        calcbetweenVel((BridgeNode *)link->getObjectPtr(), (BridgeNode *)next->getObjectPtr(), ratio);
    }
}

void BridgeNodeManager::doMove() {
    u32 n = getNumLinks() - 2;
    for (u32 j = 2; j < n; j++) {
        BridgeNode *node = (BridgeNode *)getNthLink(j)->getObjectPtr();
        PSVECAdd(&node->mPos, &node->mVel, &node->mPos);
    }
}

void BridgeNodeManager::doCtrlPos(f32 maxLen) {
    JGeometry::TVec3f diff;
    for (JSUPtrLink *link = getFirstLink()->getNext(); link != getLastLink(); link = link->getNext()) {
        diff.sub(((BridgeNode *)link->getObjectPtr())->mPos, ((BridgeNode *)link->getObjectPtr())->_18);
        if (PSVECMag(&diff) > maxLen) {
            diff.normalize();
            diff.scale(maxLen);
            ((BridgeNode *)link->getObjectPtr())->mPos.add(((BridgeNode *)link->getObjectPtr())->_18, diff);
        }
    }
}

void BridgeNodeManager::doAirFricG(f32 fric, f32 gravity) {
    u32 n = getNumLinks() - 2;
    for (u32 j = 2; j < n; j++) {
        JSUPtrLink *link = getNthLink(j);
        PSVECScale(&((BridgeNode *)link->getObjectPtr())->mVel, &((BridgeNode *)link->getObjectPtr())->mVel, fric);
        ((BridgeNode *)link->getObjectPtr())->mVel.y += gravity;
    }
}

void BridgeNodeManager::doHung(u32 last, f32 hung, u8 step) {
    u32 bit = 0;
    for (u32 j = step; j < last - step; j += step, bit++) {
        if (_14 & (1 << bit)) {
            ((BridgeNode *)getNthLink(j)->getObjectPtr())->mVel.y += hung;
        }
    }
}

void BridgeNodeManager::calcbetweenVel(BridgeNode *node, BridgeNode *other, f32 ratio) {
    JGeometry::TVec3f diff;
    PSVECSubtract(&other->mPos, &node->mPos, &diff);
    f32 len = PSVECMag(&diff);
    if (len > _1c) {
        JGeometry::TVec3f dir;
        PSVECNormalize(&diff, &dir);
        JGeometry::TVec3f vel(dir);
        PSVECScale(&vel, &vel, ratio * (len - _1c));
        PSVECAdd(&vel, &node->mVel, &node->mVel);
    }
}

void BridgeNodeManager::calcCenter() {
    JSUPtrLink *prev = getFirstLink();
    JSUPtrLink *link = prev->getNext();
    JGeometry::TVec3f *np = mNodePos;
    for (; link != nullptr; link = link->getNext(), prev = prev->getNext(), np++) {
        np->add(((BridgeNode *)prev->getObjectPtr())->mPos, ((BridgeNode *)link->getObjectPtr())->mPos);
        np->scale(0.5f);
    }
}

void BridgeNodeManager::setbwLength(f32 ratio) {
    _1c = _18 * ratio;
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
        _260 = 1.0f;
        _25c = 100.0f * object.mParam5;
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
        _30c = ((f32 *)bridgeParam)[0xe];
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
    JGeometry::TVec3f dirs[4];
    JGeometry::TVec3f pos;
    JGeometry::TVec3f corner1;
    JGeometry::TVec3f corner2;
    JGeometry::TVec3f corner3;
    JGeometry::TVec3f xdir;
    JGeometry::TVec3f zdir;
    JGeometry::TVec3f ydir;
    JGeometry::TVec3f halfX;
    JGeometry::TVec3f halfZ;
    JGeometry::TVec3f scale;
    JGeometry::TVec3f up;

    _278 = 0;
    _27a = 0;

    pos = mPos;
    getAssignedModelXYZdir(&xdir, &ydir, &zdir);

    scale = mScale;
    scale.scale(100.0f);
    xdir.scale(-scale.x);
    zdir.scale(-scale.z);

    halfX = xdir;
    halfX.scale(0.5f);
    halfZ = zdir;
    halfZ.scale(0.5f);
    pos.sub(halfX);
    pos.sub(halfZ);

    corner1 = pos;
    corner1.add(zdir);
    corner2 = pos;
    corner2.add(xdir);
    corner3 = corner2;
    corner3.add(zdir);

    mBridgeNodeMgr[0].setPosition(pos, corner1, _280);
    mBridgeNodeMgr[1].setPosition(corner2, corner3, _280);

    mBridgeNodeMgr[0]._1c = mBridgeNodeMgr[0]._18 * _2e4;
    mBridgeNodeMgr[1]._1c = mBridgeNodeMgr[1]._18 * _2e4;

    _1e8[0] = pos;
    _1e8[1] = corner1;
    _1e8[2] = corner3;
    _1e8[3] = corner2;

    dirs[0] = zdir;
    dirs[1] = xdir;
    dirs[2] = zdir;
    dirs[2].scale(-1.0f);
    dirs[3] = xdir;
    dirs[3].scale(-1.0f);

    _264 = 0.0f;
    for (u8 i = 0; i < 4; i++) {
        if (_264 < _1e8[i].y) {
            _264 = _1e8[i].y;
        }
    }

    up.set(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f *normal = _1b8;
    JGeometry::TVec3f *dir = dirs;
    for (u8 i = 0; i < 4; i++, normal++, dir++) {
        PSVECCrossProduct(&up, dir, normal);
        if (0.0f < PSVECMag(normal)) {
            PSVECNormalize(normal, normal);
        }
        stMakePlaneParam(_218[i], *normal, _1e8[i]);
    }

    _1b4 = PSVECMag(&xdir);
    _1b0 = 0.5f * _1b4;
    _2c4.normalize(xdir);
    _258 = scale.y;

    if (_284 != 0) {
        _288 = (_280 - 2) / _284 - 1;
    } else {
        _288 = 0;
    }

    _2d0 = mBridgeNodeMgr[0]._18 * _30c;

    for (u32 i = 0; i < _288; i++) {
        mBridgeNodeMgr[0]._14 |= 1 << i;
        mBridgeNodeMgr[1]._14 |= 1 << i;
    }

    mBridgeNodeMgr[0].calcCenter();
    mBridgeNodeMgr[1].calcCenter();
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
            u32 i;
            ExModel *model5 = _298;
            ExModel *model6 = _29c;
            for (i = 0; i < _288; i++, model5++, model6++) {
                model5->setModelData(model2);
                RCMGetCourse()->setFogInfo(model2);
                ExModel::setLODBias(model2, TexLODControl::getManager()->getLODBias(TexLODControl::cLODBiasID_Geography));
                model6->setModelData(model2);
                RCMGetCourse()->setFogInfo(model2);
                ExModel::setLODBias(model2, TexLODControl::getManager()->getLODBias(TexLODControl::cLODBiasID_Geography));
            }
        }

        v = *(JGeometry::TVec3f *)((u32)*(u32 **)(*(u32 *)((u32)model2 + 0x28)) + 0x48);
        _2b4 = v.z;
    }
}


void StringBridge::loadmodel(J3DModelData *modelData) {
    loadmodel_base(modelData);
}

void StringBridge::createModel(JKRSolidHeap *heap, u32 p2) {
    ExModel *model = _28c;
    u32 i;
    u32 num = _280 - 1;

    for (i = 0; i < num; i++, model++) {
        model->createModel(heap, p2, 0);
    }

    ExModel *model2 = _290;
    ExModel *model3 = _294;

    for (i = 0; i < num - 1; i++, model2++, model3++) {
        model2->createModel(heap, p2, 0);
        model3->createModel(heap, p2, 0);
    }

    if (_288 != 0) {
        ExModel *model4 = _298;
        ExModel *model5 = _29c;

        for (i = 0; i < _288; i++, model4++, model5++) {
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
    u32 i;
    ExModel *model = _28c;
    u32 num = _280 - 1;

    for (i = 0; i < num; i++, model++) {
        model->getModel()->lock();
    }

    ExModel *model2 = _290;
    ExModel *model3 = _294;

    for (i = 0; i < num - 1; i++, model2++, model3++) {
        model2->getModel()->lock();
        model3->getModel()->lock();
    }

    if (_288 != 0) {
        ExModel *model5 = _29c;
        ExModel *model4 = _298;

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
    u32 last = _280 - 1;
    u32 n = mBridgeNodeMgr[0].getNumLinks() - 2;
    u32 j;

    for (j = 2; j < n; j++) {
        JSUPtrLink *link0 = mBridgeNodeMgr[0].getNthLink(j);
        JSUPtrLink *link1 = mBridgeNodeMgr[1].getNthLink(j);
        calcparallel((BridgeNode *)link0->getObjectPtr(), (BridgeNode *)link1->getObjectPtr());
    }

    for (u8 i = 0; i < 2; i++) {
        mBridgeNodeMgr[i].doAirFricG(_2d4, _2e8);
    }

    if (_284 != 0) {
        for (u8 i = 0; i < 2; i++) {
            mBridgeNodeMgr[i].doHung(last, _2ec, _284);
        }
    }

    for (u8 i = 0; i < 2; i++) {
        mBridgeNodeMgr[i].setbwLength(_2e4);
        mBridgeNodeMgr[i].calcMove(_2d8);
    }

    for (u8 i = 0; i < 2; i++) {
        mBridgeNodeMgr[i].doMove();
        mBridgeNodeMgr[i].doCtrlPos(_2d0);
    }

    addVelNode();

    mBridgeNodeMgr[0].calcCenter();
    mBridgeNodeMgr[1].calcCenter();
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

void StringBridge::calcStringPos(Mtx m, u32 k) {
    JGeometry::TVec3f normal;
    JGeometry::TVec3f trans(m[0][3], m[1][3], m[2][3]);
    normal.set(m[0][2], m[1][2], m[2][2]);
    normal.scale(0.8f * (_2ac * _2c0));
    _2a0[k].add(normal, trans);
    normal.scale(-1.0f);
    _2a4[k].add(normal, trans);
}

void StringBridge::simpleDraw(u32 p2) {
    Mtx m;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f dir2;

    JSUPtrLink *l1, *l0, *l3, *l2;
    l0 = mBridgeNodeMgr[0].mHead;
    l1 = l0->mNext;
    l2 = mBridgeNodeMgr[1].mHead;
    l3 = l2->mNext;
    ExModel *model = _28c;
    u32 k = 0;
    _2b8 = _2dc;
    _2bc = _2e0;
    dir.zero();
    dir2.zero();
    for (; l0 != mBridgeNodeMgr[0].mTail;
         k++, l0 = l0->getNext(), l2 = l2->getNext(), l1 = l1->getNext(), l3 = l3->getNext()) {
        BridgeNode *n0 = (BridgeNode *)l0->mData;
        BridgeNode *n1 = (BridgeNode *)l1->mData;
        BridgeNode *n2 = (BridgeNode *)l2->mData;
        BridgeNode *n3 = (BridgeNode *)l3->mData;
        PSVECSubtract(&n1->mPos, &n0->mPos, &dir);
        PSVECSubtract(&n3->mPos, &n2->mPos, &dir2);
        dir.add(dir2);
        if (PSVECMag(&dir) > 0.0f) {
            dir.normalize();
        }

        calcMtx(m, dir, mBridgeNodeMgr[0].mNodePos[k], mBridgeNodeMgr[1].mNodePos[k]);
        model->setBaseTRMtx(m);
        model->setBaseScale(*(JGeometry::TVec3f *)&_2b8);
        model++;

        calcStringPos(m, k);
    }

    SimpleDrawer drawer;
    drawer.drawInit(_28c->getModelData());
    u32 num = _280 - 1;
    while (drawer.loadPreDrawSetting()) {
        ExModel *dm = _28c;
        for (u32 i = 0; i < num; dm++, i++) {
            dm->simpleDraw(p2, nullptr, 1);
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

    u32 i;
    ExModel *modelA = _290;
    ExModel *modelB = _294;
    u32 num = _280;
    JGeometry::TVec3f *p = _2a0;
    JGeometry::TVec3f *q = _2a4;
    for (i = 0; i < num - 2; i++, modelA++, modelB++, p++, q++) {
        v0 = p[0];
        v1 = p[1];
        calcStringMtx(m, pos, v0, v1);
        modelA->setBaseTRMtx(m);
        modelA->setBaseScale(pos);

        v0 = q[0];
        v1 = q[1];
        calcStringMtx(m, pos, v0, v1);
        modelB->setBaseTRMtx(m);
        modelB->setBaseScale(pos);
    }

    SimpleDrawer drawer;
    drawer.drawInit(_290->getModelData());
    while (drawer.loadPreDrawSetting()) {
        modelA = _290;
        i = 0;
        modelB = _294;
        for (; i < num - 2; i++) {
            modelA->simpleDraw(p2, nullptr, 1);
            modelB->simpleDraw(p2, nullptr, 1);
            modelA++;
            modelB++;
        }
    }

    if (_284 == 0) {
        return;
    }

    ExModel *modelC = _298;
    u32 idx = _284;
    ExModel *modelD = _29c;
    u32 bit = 0;
    for (; idx < _280 - _284 - 1; idx += _284, bit++, modelC++, modelD++) {
        if (mBridgeNodeMgr[0]._14 & (1 << bit)) {
            modelC->show();
        } else {
            modelC->hide();
        }
        BridgeNode *next0 = (BridgeNode *)mBridgeNodeMgr[0].getNthLink(idx + 1)->mData;
        calcVertiMtx(m, pos, (BridgeNode *)mBridgeNodeMgr[0].getNthLink(idx)->mData, next0);
        modelC->setBaseTRMtx(m);
        modelC->setBaseScale(pos);

        if (mBridgeNodeMgr[0]._14 & (1 << bit)) {
            modelC->show();
        } else {
            modelD->hide();
        }
        BridgeNode *next1 = (BridgeNode *)mBridgeNodeMgr[1].getNthLink(idx + 1)->mData;
        calcVertiMtx(m, pos, (BridgeNode *)mBridgeNodeMgr[1].getNthLink(idx)->mData, next1);
        modelD->setBaseTRMtx(m);
        modelD->setBaseScale(pos);
    }

    drawer.drawInit(_290->getModelData());
    while (drawer.loadPreDrawSetting()) {
        ExModel *da = _298;
        ExModel *db = _29c;
        for (u32 j = _284; j < _280 - _284 - 1; j += _284) {
            da->simpleDraw(p2, nullptr, 1);
            db->simpleDraw(p2, nullptr, 1);
            da++;
            db++;
        }
    }
}

void StringBridge::updateString() {
    ExModel *modelA = _290;
    ExModel *modelB = _294;
    u32 num = _280 - 1;
    for (u32 i = 0; i < num - 1; i++, modelA++, modelB++) {
        modelA->update(0);
        modelB->update(0);
    }

    if (_284 != 0) {
        u32 j = _284;
        modelA = _298;
        modelB = _29c;
        for (; j < _280 - _284 - 1; j += _284, modelA++, modelB++) {
            modelA->update(0);
            modelB->update(0);
        }
    }
}

void StringBridge::calcMtx(Mtx m, JGeometry::TVec3f pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    calcBridgeModelMtx(m, pos, vec1, vec2);
}

void StringBridge::calcBridgeModelMtx(Mtx m, JGeometry::TVec3f pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    JGeometry::TVec3f normal(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f dir(0.0f, 0.0f, 1.0f);
    JGeometry::TVec3f vec(0.0f, 0.0f, 0.0f);

    vec.add(vec1, vec2);
    PSVECScale(&vec, &vec, 0.5f);
    dir.sub(vec2, vec1);
    if (0.0f < PSVECMag(&dir)) {
        PSVECNormalize(&dir, &dir);
    }
    PSVECCrossProduct(&dir, &pos, &normal);
    if (0.0f < PSVECMag(&normal)) {
        PSVECNormalize(&normal, &normal);
    }
    PSVECCrossProduct(&normal, &dir, &pos);

    m[0][0] = pos.x;
    m[1][0] = pos.y;
    m[2][0] = pos.z;
    m[0][1] = normal.x;
    m[1][1] = normal.y;
    m[2][1] = normal.z;
    m[0][2] = dir.x;
    m[1][2] = dir.y;
    m[2][2] = dir.z;
    m[0][3] = vec.x;
    m[1][3] = vec.y;
    m[2][3] = vec.z;
}

void StringBridge::calcStringMtx(Mtx m, JGeometry::TVec3f &pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    JGeometry::TVec3f mid;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f forward;
    JGeometry::TVec3f up;

    mid.add(vec1, vec2);
    mid.scale(0.5f);
    dir.sub(vec2, vec1);

    f32 mag = PSVECMag(&dir);
    dir.normalize();
    f32 scale = 0.5f * mag / _2b4;

    forward.set(1.0f, 0.0f, 0.0f);
    up.set(0.0f, 1.0f, 0.0f);
    PSVECCrossProduct(&up, &dir, &forward);
    PSVECCrossProduct(&dir, &forward, &up);

    m[0][0] = forward.x;
    m[1][0] = forward.y;
    m[2][0] = forward.z;
    m[0][1] = up.x;
    m[1][1] = up.y;
    m[2][1] = up.z;
    m[0][2] = dir.x;
    m[1][2] = dir.y;
    m[2][2] = dir.z;
    m[0][3] = mid.x;
    m[1][3] = mid.y;
    m[2][3] = mid.z;

    pos.set(1.0f, 1.0f, scale);
}

void StringBridge::calcVertiMtx(Mtx m, JGeometry::TVec3f &pos, BridgeNode *nodeA, BridgeNode *nodeB) {
    JGeometry::TVec3f top;
    JGeometry::TVec3f mid;
    JGeometry::TVec3f up;
    JGeometry::TVec3f forward;
    JGeometry::TVec3f right;

    mid.add(nodeA->mPos, nodeB->mPos);
    mid.scale(0.5f);
    top = mid;
    top.y += 2000.0f;
    mid.y += 1000.0f;
    up.set(0.0f, 1.0f, 0.0f);
    up.normalize();

    f32 scale = 1000.0f / _2b4;
    forward.set(1.0f, 0.0f, 0.0f);
    PSVECCrossProduct(&up, &forward, &right);
    PSVECCrossProduct(&right, &up, &forward);

    m[0][0] = forward.x;
    m[1][0] = forward.y;
    m[2][0] = forward.z;
    m[0][1] = right.x;
    m[1][1] = right.y;
    m[2][1] = right.z;
    m[0][2] = up.x;
    m[1][2] = up.y;
    m[2][2] = up.z;
    m[0][3] = mid.x;
    m[1][3] = mid.y;
    m[2][3] = mid.z;

    pos.set(1.0f, 1.0f, scale);
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
    JGeometry::TVec3f lo[2];
    JGeometry::TVec3f hi[2];
    JGeometry::TVec3f posCopy;

    posCopy = pos;
    u32 low = 0;
    u32 high = mBridgeNodeMgr[0].mLinkCount - 2;
    u32 delta = high;
    lo[0] = mBridgeNodeMgr[0].mNodePos[0];
    lo[1] = mBridgeNodeMgr[1].mNodePos[0];
    hi[0] = mBridgeNodeMgr[0].mNodePos[high];
    hi[1] = mBridgeNodeMgr[1].mNodePos[high];

    while (high - low > 1) {
        f32 d0 = calcXZLength(posCopy, lo[0], lo[1]);
        f32 d1 = calcXZLength(posCopy, hi[0], hi[1]);
        u32 t = 0.5f * delta;
        delta = (t == 0) ? 1 : t;
        if (d0 < d1) {
            high -= delta;
            for (u8 i = 0; i < 2; i++) {
                hi[i] = mBridgeNodeMgr[i].mNodePos[high];
            }
        } else {
            low += delta;
            for (u8 i = 0; i < 2; i++) {
                lo[i] = mBridgeNodeMgr[i].mNodePos[low];
            }
        }
    }

    f32 d0 = calcXZLength(posCopy, lo[0], lo[1]);
    f32 d1 = calcXZLength(posCopy, hi[0], hi[1]);
    return (d0 < d1) ? low : high;
}

#pragma auto_inline off
JGeometry::TVec3f StringBridge::getcollideNormalandDir(u32 idx, JGeometry::TVec3f &out1, JGeometry::TVec3f &out2) {
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f v0;
    JGeometry::TVec3f v1;

    out2.sub(mBridgeNodeMgr[1].mNodePos[idx], mBridgeNodeMgr[0].mNodePos[idx]);
    if (PSVECMag(&out2) > 0.0f) {
        out2.normalize();
    }

    JSUPtrLink *a0 = mBridgeNodeMgr[0].getNthLink(idx);
    JSUPtrLink *b0 = mBridgeNodeMgr[1].getNthLink(idx);
    JSUPtrLink *a1 = mBridgeNodeMgr[0].getNthLink(idx + 1);
    JSUPtrLink *b1 = mBridgeNodeMgr[1].getNthLink(idx + 1);

    v0.sub(((BridgeNode *)a1->mData)->mPos, ((BridgeNode *)a0->mData)->mPos);
    v1.sub(((BridgeNode *)b1->mData)->mPos, ((BridgeNode *)b0->mData)->mPos);
    out1.add(v0, v1);
    if (PSVECMag(&out1) > 0.0f) {
        out1.normalize();
    }

    PSVECCrossProduct(&out2, &out1, &tmp);
    PSVECCrossProduct(&tmp, &out2, &out1);
    return tmp;
}
#pragma auto_inline reset

static inline void addNodeVel(BridgeNodeManager *mgr, u32 n, u32 cnt, const JGeometry::TVec3f &v) {
    if (n >= 2 && n <= cnt - 3) {
        BridgeNode *node = (BridgeNode *)mgr->getNthLink(n)->mData;
        PSVECAdd(&node->mVel, &v, &node->mVel);
    }
}
void StringBridge::AddVel(const JGeometry::TVec3f &pos, const JGeometry::TVec3f &vel) {
    if (RaceMgr::sRaceManager->mRaceInfo->getRaceMode() == 1) {
        return;
    }

    f32 ratio = getRightnodeVelRatio(pos);
    f32 oneMinus = 1.0f - ratio;
    JGeometry::TVec3f left;
    JGeometry::TVec3f right;
    JGeometry::TVec3f v(vel);
    v.y *= _2f4;
    left = right = v;
    left.scale(oneMinus);
    right.scale(ratio);

    addNodeVel(&mBridgeNodeMgr[0], _268, mBridgeNodeMgr[0].mLinkCount, left);
    addNodeVel(&mBridgeNodeMgr[1], _268, mBridgeNodeMgr[1].mLinkCount, right);
}

void StringBridge::addVelNode() {
    if (_2f8[1] != 0) {
        _27a = _27a + _308;
        JGeometry::TVec3f vec(0.0f, _300 * JMASinShort(_27a), 0.0f);
        if (_2f8[1] == 1) {
            u32 waveNode = getWaveNodeNum(_2f8[3]);
            addNodeVel(&mBridgeNodeMgr[0], waveNode, mBridgeNodeMgr[0].mLinkCount, vec);
            vec.scale(-1.0f);
            addNodeVel(&mBridgeNodeMgr[1], waveNode, mBridgeNodeMgr[1].mLinkCount, vec);
        } else {
            u32 waveNode = getWaveNodeNum(_2f8[3]);
            addNodeVel(&mBridgeNodeMgr[0], waveNode, mBridgeNodeMgr[0].mLinkCount, vec);
            addNodeVel(&mBridgeNodeMgr[1], waveNode, mBridgeNodeMgr[1].mLinkCount, vec);
        }
    }

    if (_2f8[0] != 0) {
        _278 = _278 + _304;
        f32 s = _2fc * JMASinShort(_278);
        JGeometry::TVec3f vec(_2c4);
        vec.scale(s);
        u32 waveNode = getWaveNodeNum(_2f8[2]);
        addNodeVel(&mBridgeNodeMgr[0], waveNode, mBridgeNodeMgr[0].mLinkCount, vec);
        addNodeVel(&mBridgeNodeMgr[1], waveNode, mBridgeNodeMgr[1].mLinkCount, vec);
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

static inline JGeometry::TVec3f calcNodeVel(JSUPtrLink *a0, JSUPtrLink *b0, JSUPtrLink *a1, JSUPtrLink *b1) {
    JGeometry::TVec3f vel(0.0f, 0.0f, 0.0f);
    JGeometry::TVec3f sum1;
    vel.add(((BridgeNode *)a0->mData)->mVel, ((BridgeNode *)b0->mData)->mVel);
    sum1.add(((BridgeNode *)a1->mData)->mVel, ((BridgeNode *)b1->mData)->mVel);
    vel.add(sum1);
    vel.scale(0.25f);
    vel.y *= sBridgeVelYRatio;
    return vel;
}

void StringBridge::Search(const JGeometry::TVec3f &prevPos, const JGeometry::TVec3f &pos) {
    JGeometry::TVec3f posCopy(pos);
    JGeometry::TVec3f out1;
    JGeometry::TVec3f out2;
    JGeometry::TVec3f normal;
    stPlaneParam plane;
    JGeometry::TVec3f point;
    JGeometry::TVec3f groundNormal;
    f32 depth;

    _268 = findNthCenter(posCopy);
    _26c = pos;
    normal = getcollideNormalandDir(_268, out1, out2);
    ExObjColBase *col = mExBounds;
    col->setNormal(normal);
    point = mBridgeNodeMgr[0].mNodePos[_268];
    stMakePlaneParam(plane, normal, point);

    depth = 0.0f;
    f32 height = getplaneHeight(plane, out1, _268, pos, depth);
    col->setHeight(height);
    col->setAttr(1);
    col->setMaterial(5);

    if (height - pos.y > sHeightCheckYMax) {
        CrsGround ground(RCMGetCourse());
        ground.search(this, pos, prevPos);
        col->zeroVelocity();
        ground.getNormal(&groundNormal);
        col->setNormal(groundNormal);
        col->setHeight(ground.getHeight());
        col->setColDepth(-1.0f);
        col->setAttr(ground.getAttribute());
        col->setMaterial(ground.getMaterial());
        col->setAttrIndex(ground.getAttrIndex());
    } else {
        f32 t = stCollideLineToPlaneIn(prevPos, pos, plane);
        f32 colDepth = 0.0f;
        if (0.0f > t && 1.0f < t) {
            colDepth = stGetCollideDepthFromT(prevPos, pos, t);
        }
        colDepth = (colDepth > _2b0 * _2e0) ? _2b0 * _2e0 : colDepth;
        col->setColDepth(colDepth + depth);

        u32 n = _268;
        JSUPtrLink *a0 = mBridgeNodeMgr[0].getNthLink(n);
        JSUPtrLink *b0 = mBridgeNodeMgr[1].getNthLink(n);
        JSUPtrLink *a1 = mBridgeNodeMgr[0].getNthLink(n + 1);
        JSUPtrLink *b1 = mBridgeNodeMgr[1].getNthLink(n + 1);
        col->setVelocity(calcNodeVel(a0, b0, a1, b1));
    }
}

void StringBridge::Search_Wall(const JGeometry::TVec3f &pos, f32 radius) {
    JGeometry::TVec3f posCopy(pos);
    JGeometry::TVec3f out1;
    JGeometry::TVec3f out2;
    JGeometry::TVec3f normal;
    stPlaneParam plane;
    JGeometry::TVec3f point;
    f32 depth = -1.0f;

    _268 = findNthCenter(posCopy);
    normal = getcollideNormalandDir(_268, out1, out2);
    point = mBridgeNodeMgr[0].mNodePos[_268];
    stMakePlaneParam(plane, normal, point);

    f32 height = getplaneHeight(plane, pos) - pos.y;
    ExObjColBase *col = mExBounds;
    if (height > -sHeightCheckYMin && height <= sHeightCheckYMax) {
        stPlaneParam sidePlane = getSidePlaneParam(pos, _268);
        if (stCollideSurfaceAndSphere(pos, radius, sidePlane, depth) && depth < 2.0f * radius) {
            CrsGround ground(RCMGetCourse());
            ground.search(this, pos, pos);
            f32 groundHeight = ground.getHeight();
            col->zeroVelocity();
            JGeometry::TVec3f wallNormal(sidePlane.x, sidePlane.y, sidePlane.z);
            stVecNormalize(wallNormal);
            col->setWallNormal(wallNormal);
            col->setHeight(groundHeight);
            col->setAttr(2);
            col->setMaterial(5);
            if (depth > sWallDepth) {
                depth = sWallDepth;
            }
        } else {
            depth = -1.0f;
        }
    }
    col->setColDepth(depth);
}

f32 StringBridge::calcXZLength(JGeometry::TVec3f &pos, const JGeometry::TVec3f &vec1, const JGeometry::TVec3f &vec2) {
    f32 x = 0.5f * (vec1.x + vec2.x);
    f32 z = 0.5f * (vec1.z + vec2.z);
    return stLength2(x - pos.x, z - pos.z);
}

stPlaneParam StringBridge::getSidePlaneParam(const JGeometry::TVec3f &pos, u32 idx) {
    JGeometry::TVec3f center;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f rel;
    JGeometry::TVec3f point;
    JGeometry::TVec3f normal;

    center.add(mBridgeNodeMgr[0].mNodePos[idx], mBridgeNodeMgr[1].mNodePos[idx]);
    center.scale(0.5f);
    dir.sub(mBridgeNodeMgr[1].mNodePos[idx], mBridgeNodeMgr[0].mNodePos[idx]);
    stVecNormalize(dir);
    rel.sub(pos, center);

    if (rel.dot(dir) > 0.0f) {
        JGeometry::TVec3f t(dir);
        t.scale(_1b0);
        point.add(center, t);
        normal = _1b8[2];
    } else {
        JGeometry::TVec3f t(dir);
        t.scale(-_1b0);
        point.add(center, t);
        normal = _1b8[0];
    }

    stPlaneParam plane;
    stMakePlaneParam(plane, normal, point);
    return plane;
}

f32 StringBridge::getplaneHeight(stPlaneParam &plane, const JGeometry::TVec3f &pos) {
    f32 h = -(plane.d + (plane.x * pos.x + plane.z * pos.z)) / plane.y;
    h += _2b0 * _2e0;
    return h;
}

f32 StringBridge::getplaneHeight(stPlaneParam &plane, JGeometry::TVec3f vec, u32 num, const JGeometry::TVec3f &pos, f32 &depth) {
    JGeometry::TVec3f mid(0.0f, 0.0f, 0.0f);
    JGeometry::TVec3f diff;

    mid.add(mBridgeNodeMgr[0].mNodePos[num], mBridgeNodeMgr[1].mNodePos[num]);
    mid.scale(0.5f);
    diff.sub(mid, pos);

    f32 h = -(plane.d + (plane.x * pos.x + plane.z * pos.z)) / plane.y;
    f32 dot = vec.dot(diff);
    f32 r = _2b0 * _2e0;
    f32 rr = r * r;
    f32 dd = dot * dot;
    depth = rr - dd;
    if (depth < 0.0f) {
        depth = 0.0f;
    } else {
        depth = JMAFastSqrt(depth);
    }

    h += depth;
    return h;
}

f32 StringBridge::getRightnodeVelRatio(const JGeometry::TVec3f &pos) {
    JGeometry::TVec3f d2;
    JGeometry::TVec3f diff;
    JGeometry::TVec3f p;
    JGeometry::TVec3f base;
    JGeometry::TVec3f d;
    JGeometry::TVec3f dir;

    u32 n = _268;
    JSUPtrLink *link0 = mBridgeNodeMgr[0].getNthLink(n);
    JSUPtrLink *link1 = mBridgeNodeMgr[1].getNthLink(n);
    dir.set(0.0f, 0.0f, 0.0f);
    PSVECSubtract(&((BridgeNode *)link1->mData)->mPos, &((BridgeNode *)link0->mData)->mPos, &dir);
    if (PSVECMag(&dir)) {
        dir.normalize();
    }
    d2 = d = dir;
    p = pos;
    base = ((BridgeNode *)mBridgeNodeMgr[0].getNthLink(_268)->mData)->mPos;
    PSVECSubtract(&p, &base, &diff);
    return PSVECDotProduct(&d2, &diff) / _1b4;
}

const char *StringBridge::getBmdFileName() {
    static const char *cBmdName = "/Objects/HBMaruta.bmd";
    return cBmdName;
}

const char *StringBridge::getLopeBmdFileName() {
    static const char *cMLopeName = "/Objects/HBMarutaLope.bmd";
    return cMLopeName;
}





















