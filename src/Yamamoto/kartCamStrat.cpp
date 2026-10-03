#include "Yamamoto/kartCamera.h"

#include "dolphin/gx.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "JSystem/JMath/JMath.h"
#include "Kaneshige/KartMtx.h"
#include "Kaneshige/RaceMgr.h"

// Unused function calls set from TVec3f
// comments inside functions are inline functions being called in that function

void KartCam::SetTargetNum(unsigned char num) {
    mTargetIdx = num;
    SetTarget();
}

void KartCam::SetClipper() {
    mClipper.setAspect(mAspect);
    mClipper.setNear(100.0f);
    mClipper.setFar(mFar);
    mClipper.setFovy(mFovY);
    mClipper.calcViewFrustum();
    _16c = mAspect;
    _170 = 100.0f;
    _174 = mFar;
    _178 = mFovY;
}

void KartCam::CheckClipper() {
    if (_16c == mAspect && _174 == mFar && _178 == mFovY) {
        return;
    }
    SetClipper();
}

GrafPort::GrafPort(int vpX, int vpY, int vpW, int vpH, f32 fovY, f32 near, f32 far)
    : J2DPerspGraph(vpX, vpY, vpW, vpH, fovY, near, far) {
    mCamAspect = 1.0f;
    mCamNear = 0.0f;
    mCamFar = 0.0f;
}

void GrafPort::setPort() {
    J2DGrafContext::setPort();
    KartMtxPerspective(mMtx44, mFovY, mCamAspect, mCamNear, mCamFar, RaceMgr::getCurrentManager()->isMirror());
    GXSetProjection(mMtx44, GX_PERSPECTIVE);
}

void GrafPort::SetParam(f32 aspect, f32 near, f32 far) {
    mCamAspect = aspect;
    mCamNear = near;
    mCamFar = far;
}

void KartCam::MakeShaker(f32 shake) {
    if (shake == 0.0f) {
        return;
    }
    if (_194 >= 25.0f * shake) {
        return;
    }
    mFlags |= 1;
    _190 += 1.046666f;
    _194 = 25.0f * shake;
}

void KartCam::DoShaker(JGeometry::TVec3f *pos, JGeometry::TVec3f dir) {
    if ((mFlags & 1) == 0) {
        return;
    }
    _190 += 1.046666f;
    if (_190 >= 100.0f) {
        _190 = 0.0f;
    }
    GetKartCtrl()->ChaseFnumber(&_194, 0.0f, 0.04f);
    if (_194 == 0.0f) {
        mFlags &= ~1;
    }
    f32 amplitude = _194 * JMASSin((s16)(16384.0f * (180.0f * _190 / 3.141f) / 90.0f));
    pos->y = pos->y + amplitude;
    pos->x = pos->x + 0.3f * (amplitude * dir.x);
    pos->z = pos->z + 0.3f * (amplitude * dir.z);
}

void KartCam::SetPtr() {
    if (RaceMgr::getCurrentManager()->isCrsDemoMode()) {
        mPtr = 0;
        return;
    }
    if (RaceMgr::getCurrentManager()->isAwardDemoMode()) {
        mPtr = 0;
        return;
    }
    switch (RaceMgr::getCurrentManager()->getCameraNumber()) {
    case 1:
        mPtr = 0;
        break;
    case 2:
        if (RaceMgr::getCurrentManager()->isSubScrExist()) {
            switch (mCamNo) {
            case 0:
                mPtr = 0;
                break;
            case 1:
                mPtr = 7;
                break;
            }
        } else {
            switch (mCamNo) {
            case 0:
                mPtr = 1;
                break;
            case 1:
                mPtr = 2;
                break;
            }
        }
        break;
    case 3:
    case 4:
        switch (mCamNo) {
        case 0:
            mPtr = 3;
            break;
        case 1:
            mPtr = 4;
            break;
        case 2:
            mPtr = 5;
            break;
        case 3:
            mPtr = 6;
            break;
        }
        break;
    default:
        mPtr = 0;
        break;
    }
}

JGeometry::TVec3f *KartCam::GetCameraPos() { return &mCameraPos; }

JGeometry::TVec3f *KartCam::GetCameraLookPos() { return &mCameraLookPos; }

void KartCam::SetFovyData() {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    int camNum = mgr->getCameraNumber();
    int viewIdx;
    if (camNum == 1) {
        viewIdx = 0;
    } else if (camNum == 2) {
        viewIdx = 1;
    } else if (camNum == 3 || camNum == 4) {
        viewIdx = 2;
    } else {
        viewIdx = 0;
    }
    u8 pakkun = 0;
    if (mgr->getKartInfo(mCamNo)->getDriverCharID(0) == PETEY ||
        mgr->getKartInfo(mCamNo)->getDriverCharID(1) == PETEY) {
        pakkun = 1;
    }
    switch (mgr->getRaceMode()) {
    case BALLOON_BATTLE:
        mFovY = balloonMinidata[viewIdx].fovY;
        if (pakkun != 0) {
            mFovY = balloonPakkunMinidata[viewIdx].fovY;
        }
        break;
    case ROBBERY_BATTLE:
        mFovY = turtleMinidata[viewIdx].fovY;
        if (pakkun != 0) {
            mFovY = balloonPakkunMinidata[viewIdx].fovY;
        }
        break;
    case BOMB_BATTLE:
        mFovY = bombMinidata[viewIdx].fovY;
        if (pakkun != 0) {
            mFovY = bombPakkunMinidata[viewIdx].fovY;
        }
        break;
    case ESCAPE_BATTLE:
        mFovY = devilMinidata[viewIdx].fovY;
        if (pakkun != 0) {
            mFovY = devilPakkunMinidata[viewIdx].fovY;
        }
        break;
    default:
        if (viewIdx == 2) {
            viewIdx = 3;
        }
        mFovY = viewdata[viewIdx].fovY;
        break;
    }
    _1bc = mFovY;
    _14c = 0.0f;
}

void KartCam::First4ScreenPort(unsigned char) {}

void KartCam::Second4ScreenPort(unsigned char) {}

void KartCam::Third4ScreenPort(unsigned char) {}

void KartCam::Fourth4ScreenPort(unsigned char) {}

void KartCam::First2ScreenPort(unsigned char) {}

void KartCam::Second2ScreenPort(unsigned char) {}

void KartCam::SetVictoryScreenPort(unsigned char) {
    // void RaceMgr::getZoomWinConsoleNo() const {}
    // void RaceDirector::getZoomWinConsoleNo() const {}
    // void RaceMgr::isZoomWinConsole() const {}
}

void KartCam::DoMoveCamera(JGeometry::TVec3<float> *, JGeometry::TVec3<float> *) {}

void KartCam::DoRoof(JGeometry::TVec3<float> *, CrsArea *) {}

void KartCam::DoColCamera() {}

void KartCam::DoSea(JGeometry::TVec3<float> *, CrsGround *) {}

void KartCam::DoGround(JGeometry::TVec3<float> *, CrsGround *) {}

void KartCam::GroundCheck(JGeometry::TVec3<float> *, JGeometry::TVec3<float> *, CrsArea *) {}

void KartCam::OutViewCalc() {}

void KartCam::InitOutView() {}

void KartCam::DoChangFovy() {}

void KartCam::DoLookChase() {}

void KartCam::DoYRotation() {}

void KartCam::DoXRotation() {}

void KartCam::DoDist() {}

void KartCam::DoCamPos(float, JGeometry::TVec3<float> *) {}

void KartCam::OutView() {}

void KartCam::InitRaceBackView() {}

void KartCam::ParallelView() {}

void KartCam::InitBackView() {
    _184 = 0.0f;
    _1a4 = 0.0f;
    _164 = 0.0f;
    _1b8 = 0.0f;
    OutViewCalc();
}

void KartCam::BackView() {
    _1b8 = 0.0f;
    OutView();
}

void KartCam::HangRescueView() {}

void KartCam::InitDropRescueView() {}

void KartCam::DropRescueView() {}

void KartCam::LaunchView() {}

void KartCam::LandView() {}

void KartCam::InitPipeView() {}

void KartCam::PipeView() {}
