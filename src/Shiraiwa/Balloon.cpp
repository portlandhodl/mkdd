#include "Shiraiwa/Balloon.h"

#include "JSystem/J3D/J3DAnmBase.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "Sato/ObjUtility.h"

void TBalloon::reset() {
    mGeoObjFlag &= ~2;
    mCheckKartHitFlags = 0;
    mGeoObjFlag |= 0x20;
    mFlag = 0;
    mCount = 0;
}

void TBalloon::update() {
    mMaterial.anmFrameProc();
    setModelMatrixAndScale();
    mModel.update(0);
}



























































































































































































































































































































































































































































































void TBalloon::loadAnimation() {
    J3DModelData *modelData = mModel.getModelData();
    void *anm = ObjUtility::getPtrCourseArc("/Objects/MiniGame_Balloon.brk");
    JUT_ASSERT_MSG(anm != nullptr, "TBalloon: /Objects/MiniGame_Balloon.brk not found.");
    J3DAnmObjMaterial::setupTevRegAnmData(&sBalloonBrkAnm, modelData, anm);
}

void TBalloon::calc() {
    f32 scale = mScaleRatio;
    if (RaceMgr::sRaceManager->getRacePhase() == PHASE_CRS_DEMO) {
        scale = 0.0f;
    } else if (mFrame < scScaleFrame2) {
        f32 s = 0.0f;
        f32 frame = mFrame;
        if (frame > 0.0f) {
            if (frame < scScaleFrame1) {
                s = frame * scScale1 / scScaleFrame1;
            } else {
                if (frame - scScaleFrame1 <= 1.0f) {
                    mString->shake();
                }
                s = scScale1 + (scScale1 - 1.0f) * (scScaleFrame1 - mFrame) / (scScaleFrame2 - scScaleFrame1);
            }
        }
        scale = mScaleRatio * s;
        mFrame += 1.0f;
    }

    if (scale > 0.0f) {
        mScale.x = scale;
        mScale.y = scale;
        mScale.z = scale;
        if (mFlag != 0) {
            if (!mString->tstObjFlagHidding()) {
                mFlag = 0;
                if (mShadowMdl != nullptr) {
                    mShadowMdl->setVisibleAll();
                }
            }
        } else {
            if (mString->tstObjFlagHidding()) {
                mFlag = 1;
                if (mShadowMdl != nullptr) {
                    mShadowMdl->clrVisibleAll();
                }
            } else {
                JGeometry::TVec3f up;
                JGeometry::TVec3f dir;

                mString->getPoint(&mPos);
                JGeometry::TVec3f v1(0.0f, 0.0f, 1.0f);
                mString->getPointDir(&dir);
                up.cross(dir, v1);
                up.normalize();
                v1.cross(up, dir);
                v1.normalize();

                mRotMtx[0][0] = up.x;
                mRotMtx[1][0] = up.y;
                mRotMtx[2][0] = up.z;
                mRotMtx[0][1] = dir.x;
                mRotMtx[1][1] = dir.y;
                mRotMtx[2][1] = dir.z;
                mRotMtx[0][2] = v1.x;
                mRotMtx[1][2] = v1.y;
                mRotMtx[2][2] = v1.z;
            }
        }
    } else {
        f32 one = 1.0f;
        mScale.x = one;
        mScale.y = one;
        mScale.z = one;
        mFlag = 1;
        if (mShadowMdl != nullptr) {
            mShadowMdl->clrVisibleAll();
        }
    }
}

void TBalloon::viewCalc(u32 viewNo) {
    mModel.viewCalc(viewNo);
}

void TBalloon::createModel(JKRSolidHeap *heap, u32 viewNo) {
    mMaterial.setExModel(&mModel);
    mMaterial.setAnmBase(sBalloonBrkAnm);
    mMaterial.initFrameCtrl(mMaterial.mAnmBase);
}

void TBalloon::setCurrentViewNo(u32 viewNo) {
    mModel.setCurrentViewNo(viewNo);
}

TBalloonSupervisor::~TBalloonSupervisor() {}

void TBalloonSupervisor::drawString(u32) {}

void TBalloonSupervisor::drawBalloon(u32 viewNo) {
    TBalloonString *balloons[32];
    int count = 0;
    JSUPtrLink *link = mList.getFirstLink();
    while (link != nullptr) {
        if (count >= 32) {
            break;
        }
        balloons[count++] = (TBalloonString *)link->getObjectPtr();
        link = link->getNext();
    }
    for (int i = count; i < 32; i++) {
        balloons[i] = nullptr;
    }
    for (int i = 0; i < 32; i++) {
        if (balloons[i] != nullptr) {
            balloons[i]->drawBalloon(viewNo);
        }
    }
}

__declspec(weak) TBalloon::~TBalloon() {}
