#include "Shiraiwa/Objects/MapObjFireBar.h"

#include <math.h>

#include "JSystem/JMath/JMath.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/ObjUtility.h"
#include "Shiraiwa/Objects/MapObjFireBall.h"

TMapObjFireBar::TMapObjFireBar(const CrsData::SObject &obj, u32 id) : TMapObjFireObj(obj, id) {
    mAngle = 0;
    int sep = mObjData->mParam4;
    if (sep == 0) {
        sep = 2;
    }
    for (int i = 0; i < mFireNum; i++) {
        if (i + sep >= mFireNum) {
            mFire[i]->createSoundMgr();
        }
    }
}

TMapObjFireBar::~TMapObjFireBar() {}

void TMapObjFireBar::reset() {
    resetObject();
    mGeoObjFlag |= 1;
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    _58 = 1;
    ResetState();
    mSpeed = mObjData->mParam2 * 0.001f;
    if (mSpeed == 0.0f) {
        mSpeed = 0.03f;
    }
    mAngle = mObjData->mParam3;
    if (mAngle == 0) {
        mAngle = 200;
    }
    mSep = mObjData->mParam4;
    if (mSep == 0) {
        mSep = 2;
    }
    mFireRotRate = 0.0f;
    resetFire();
}

void TMapObjFireBar::resetFire() {
    JGeometry::TVec3f v;
    int group = 0;
    for (int i = 0; i < mFireNum; i++) {
        if (i % mSep == 0) {
            group++;
        }
        s16 ang = (i % mSep) * 0x10000 / mSep;
        v.set(mAngle * group * JMACosShort(ang), mAngle * group * JMASinShort(ang), 0.0f);
        mFire[i]->_178.set(v);
        mFire[i]->setRMtx(mRotMtx);
        mFire[i]->_184.set(mPos);
        mFire[i]->mScale.set(mScale);
        mFire[i]->mIndex = i + mSep >= mFireNum;
        if (mObjData->mParam5 == 0 && mFireNum - i > mSep) {
            TMapObjFireBallBase *fire = mFire[i];
            fire->mCheckKartHitFlags = 0;
            fire->mGeoObjFlag &= ~2;
        }
    }
    if (mSpeed != 0.0f) {
        v.set(0.0f, 0.0f, 1.0f);
        ((JGeometry::TQuat4f &)mFireRotQ[0]).setRotate(v, mSpeed);
        ((JGeometry::TQuat4f &)mFireRotQ[1]).setRotate(v, 3.0f * mSpeed);
    } else {
        mFireRotQ[0].set(0.0f, 0.0f, 0.0f, 1.0f);
        mFireRotQ[1].set(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

void TMapObjFireBar::createColModel(J3DModelData *modelData) {
    createBoundsSphere(modelData);
}

const char *TMapObjFireBar::getBmdFileName() {
    static const char *cBmdName = "/Objects/FireBar1.bmd";
    return cBmdName;
}

const char *TMapObjFireBar::getShadowBmdFileName() {
    return nullptr;
}

void TMapObjFireBar::calc() {
    TMapObjFireObj::calc();
}

TMapObjKpFireBar::TMapObjKpFireBar(const CrsData::SObject &obj) : TMapObjFireBar(obj, 0x1263) {
    if (mAnmCtrl == nullptr) {
        mAnmCtrl = new AnmController();
    }
}

void TMapObjKpFireBar::reset() {
    resetObject();
    mGeoObjFlag |= 1;
    mGeoObjFlag |= 2;
    mCheckKartHitFlags = -1;
    _58 = 1;
    ResetState();
    mSpeed = mObjData->mParam2 * 0.001f;
    if (mSpeed == 0.0f) {
        mSpeed = 0.03f;
    }
    mAngle = mObjData->mParam3;
    if (mAngle == 0) {
        mAngle = 200;
    }
    mSep = mObjData->mParam4;
    if (mSep == 0) {
        mSep = 2;
    }
    mFireRotRate = 0.0f;
    resetFire();
    mScale.x = 1.0f;
    mScale.y = 1.0f;
    mScale.z = 1.0f;
    if (sUseBck) {
        mAnmCtrl->Reset();
    }
    f32 s = sin(-1.5707963705062866);
    f32 c = cos(-1.5707963705062866);
    Mtx m;
    m[2][1] = s;
    m[0][0] = 1.0f;
    m[1][1] = c;
    m[1][2] = -s;
    m[2][2] = c;
    m[2][0] = 0.0f;
    m[0][2] = 0.0f;
    m[1][0] = 0.0f;
    m[0][1] = 0.0f;
    mRotMtx[0][3] = 0.0f;
    mRotMtx[1][3] = 0.0f;
    mRotMtx[2][3] = 0.0f;
    PSMTXConcat(mRotMtx, m, mRotMtx);
    mCheckKartHitFlags = 0;
    mGeoObjFlag &= ~2;
}

void TMapObjKpFireBar::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *anm = ObjUtility::getPtrCourseArc("/Objects/KpFireBar1.bck");
    if (anm != nullptr) {
        sUseBck = 1;
        J3DAnmObjTrans::setupTransAnmData(&sKpFireBarBckAnmTrans, &sKpFireBarBckMtxCalc, modelData, anm);
    } else {
        sUseBck = 0;
    }
}

void TMapObjKpFireBar::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mModel.createModel(heap, viewNo, 0);
    if (sUseBck) {
        AnmController *ctrl = mAnmCtrl;
        ctrl->mTrans = new AnmControlTrans();
        ctrl->mTrans->initAnm(1, &mModel);
        mAnmCtrl->mTrans->registration(0, sKpFireBarBckAnmTrans, sKpFireBarBckMtxCalc);
    }
}

void TMapObjKpFireBar::createColModel(J3DModelData *) {}

J3DAnmTransform *TMapObjKpFireBar::sKpFireBarBckAnmTrans;
J3DMtxCalc *TMapObjKpFireBar::sKpFireBarBckMtxCalc;
u8 TMapObjKpFireBar::sUseBck;

TMapObjFireCircle::TMapObjFireCircle(const CrsData::SObject &obj) : TMapObjFireObj(obj, 0x106a) {
    mRadius = 0.0f;
    createSoundMgr();
}

TMapObjFireCircle::~TMapObjFireCircle() {}

void TMapObjFireCircle::reset() {
    resetObject();
    ResetState();
    mGeoObjFlag &= ~1;
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    _58 = 0;
    mSpeed = mObjData->mParam2 * 0.001f;
    if (mSpeed == 0.0f) {
        mSpeed = 0.001f;
    }
    if (mObjData->mParam3 == 10 || mObjData->mParam3 == 0) {
        mRotSpeed = 1.0f;
    } else {
        mRotSpeed = mObjData->mParam3 * 0.1f;
    }
    mRadius = (int)mObjData->mParam4;
    if (mRadius == 0.0f) {
        mRadius = 400.0f;
    }
    mAngle = 0;
    mAngleSpeed = 273;
    mFireRotRate = 0.0f;
    resetFire();
}

void TMapObjFireCircle::resetFire() {
    s16 step = 0x10000 / mFireNum;
    s16 ang = 0;
    for (s16 i = 0; i < mFireNum; i++) {
        mFire[i]->_178.set(mRadius * JMACosShort(ang), mRadius * JMASinShort(ang), 0.0f);
        mFire[i]->_184.set(mPos);
        mFire[i]->setRMtx(mRotMtx);
        mFire[i]->mScale.set(mScale);
        TMapObjFireBallBase *fire = mFire[i];
        fire->mCheckKartHitFlags = 0;
        fire->mGeoObjFlag &= ~2;
        ang += step;
    }
    if (mSpeed != 0.0f) {
        static JGeometry::TVec3f zAxis(0.0f, 0.0f, 1.0f);
        ((JGeometry::TQuat4f &)mFireRotQ[0]).setRotate(zAxis, mSpeed);
        ((JGeometry::TQuat4f &)mFireRotQ[1]).setRotate(zAxis, 3.0f * mSpeed);
    } else {
        mFireRotQ[0].set(0.0f, 0.0f, 0.0f, 1.0f);
        mFireRotQ[1].set(0.0f, 0.0f, 0.0f, 1.0f);
    }
}

void TMapObjFireCircle::calc() {
    f32 radius = mRadius * mRotSpeed + 1000.0f;
    f32 sq = radius * radius;
    u8 mask = 0;
    for (int i = 0; i < RaceMgr::getManager()->getKartNumber(); i++) {
        JGeometry::TVec3f pos;
        ObjUtility::getKartPos(i, &pos);
        pos.sub(mPos);
        if (pos.squared() < sq) {
            mask |= 1 << i;
        }
    }
    for (int j = 0; j < mFireNum; j++) {
        for (int k = 0; k < RaceMgr::getManager()->getKartNumber(); k++) {
            int bit = 1 << k;
            if (mask & bit) {
                mFire[j]->mCheckKartHitFlags |= bit;
            } else {
                mFire[j]->mCheckKartHitFlags &= ~bit;
            }
        }
    }
    TMapObjFireObj::calc();
    if (mRotSpeed != 1.0f) {
        f32 v = 0.5f * (mRotSpeed + (1.0f + (mRotSpeed - 1.0f) * JMACosShort(mAngle)));
        for (int i = 0; i < mFireNum; i++) {
            mFire[i]->_174 = v;
        }
        mAngle += mAngleSpeed;
        mSoundMgr->setSe(0x40032);
    }
}

#include "JSystem/JAudio/JASFakeMatch2.h"
