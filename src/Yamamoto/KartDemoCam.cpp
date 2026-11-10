#include "Yamamoto/KartDemoCam.h"
#include "Yamamoto/kartCamera.h"
#include "Yamamoto/KartMath.h"

#include "Kameda/J2DManager.h"
#include "Kaneshige/RaceMgr.h"

#include "Osako/kartPad.h"
#include "JSystem/JUtility/JUTGamePad.h"
#include "JSystem/JUtility/JUTDbPrint.h"
#include "dolphin/mtx.h"
#include "Sato/GeographyObjMgr.h"
#include "Shiraiwa/JugemMain.h"

#include <std/math.h>

#include "JSystem/JAudio/JASFakeMatch2.h"

// comments inside functions are inline functions being called in that function

void KartDemoCam::Init(KartCam *cam) {
    mCam = cam;
    mBody = mCam->getBody();
    _38 = 0;
    _3b = 0;
    mStartId = -1;
    _50.x = 0.0f;
    _50.z = 0.0f;
    _50.y = 0.0f;
    _4c = 1000.0f;
}

void KartDemoCam::ChaseFovy(const CrsData::Camera *camera) {
    f32 fovy = mCam->GetFovy();
    f32 fovy2 = camera->getFovy2();
    if (fovy2 == 0.0f) {
        mCam->SetFovy(camera->getFovy());
    } else {
        GetKartCtrl()->ChaseFnumber(&fovy, fovy2, 0.01f);
        mCam->SetFovy(fovy);
    }
}

void KartDemoCam::DoInitCol() {
    vec._80.set(mCam->mCameraPos);
}

void KartDemoCam::DoCol() {
    CrsGround ground;
    JGeometry::TVec3f normal;
    ground.search(mCam->mCameraPos, vec._80);
    u64 status = mBody->mCarStatus;
    if ((status & 0x0040000000000000ULL) == 0) {
        if (ground.getAttribute() == 2 || ground.getAttribute() == 0x12) {
            f32 scale = ground.getWallNormal(&normal, nullptr);
            mCam->mCameraPos.x += normal.x * scale * 2.0f;
            mCam->mCameraPos.y += normal.y * scale * 2.0f;
            mCam->mCameraPos.z += normal.z * scale * 2.0f;
        } else {
            f32 height = ground.getHeight();
            f32 diff = mCam->mCameraPos.y - 2.0f - height;
            if (diff < 0.0f) {
                mCam->mCameraPos.y -= diff;
            }
        }
    }
    vec._80.set(mCam->mCameraPos);
}

void KartDemoCam::InitFixCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f pos;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    up.set(0.0f, 1.0f, 0.0f);
    camera->getPosition(&pos);
    mCam->mCameraPos.set(pos);
    mCam->mCameraLookPos.set(mCam->mCameraPos);
    JGeometry::TVec3f dir;
    camera->getFrontDirection(&dir);
    mCam->mCameraLookPos.x += dir.x * 100.0f;
    mCam->mCameraLookPos.y += dir.y * 100.0f;
    mCam->mCameraLookPos.z += dir.z * 100.0f;
    mCam->SetFovy(camera->getFovy());
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitFixCam", 31);
}

void KartDemoCam::DoFixCam() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    JGeometry::TVec3f up;
    ChaseFovy(camera);
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoFixCam", 32);
}

void KartDemoCam::InitFixSearchCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f pos;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    up.set(0.0f, 1.0f, 0.0f);
    camera->getPosition(&pos);
    mCam->mCameraPos.set(pos);
    mCam->mCameraLookPos.set(mCam->mTargetBody->mPlayerPosMtx[0][3],
                             mCam->mTargetBody->mPlayerPosMtx[1][3],
                             mCam->mTargetBody->mPlayerPosMtx[2][3]);
    mCam->SetFovy(camera->getFovy());
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitFixSearchCam", 33);
}

void KartDemoCam::DoFixSearchCam() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    JGeometry::TVec3f up;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f tmp;
    up.set(0.0f, 1.0f, 0.0f);
    tmp.set(mCam->mTargetBody->mPlayerPosMtx[0][3],
            mCam->mTargetBody->mPlayerPosMtx[1][3],
            mCam->mTargetBody->mPlayerPosMtx[2][3]);
    dir.sub(tmp, mCam->mCameraLookPos);
    f32 len = dir.length();
    dir.normalize();
    dir.scale(len * 0.3f);
    mCam->mCameraLookPos.add(dir);
    ChaseFovy(camera);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoFixSearchCam", 34);
}

void KartDemoCam::InitFixChaseCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f tmp;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    up.set(0.0f, 1.0f, 0.0f);
    camera->getPosition(&mCam->mCameraPos);
    tmp.set(mCam->mTargetBody->mPlayerPosMtx[0][3],
            mCam->mTargetBody->mPlayerPosMtx[1][3],
            mCam->mTargetBody->mPlayerPosMtx[2][3]);
    vec.mCamKartDiff.sub(mCam->mCameraPos, tmp);
    mCamKartDiffLength = vec.mCamKartDiff.length();
    vec.mCamKartDiff.normalize();
    mCam->mCameraPos.x = tmp.x + vec.mCamKartDiff.x * mCamKartDiffLength;
    mCam->mCameraPos.y = tmp.y + vec.mCamKartDiff.y * mCamKartDiffLength;
    mCam->mCameraPos.z = tmp.z + vec.mCamKartDiff.z * mCamKartDiffLength;
    mCam->mCameraLookPos.set(mCam->mCameraPos);
    JGeometry::TVec3f dir;
    camera->getFrontDirection(&dir);
    mCam->mCameraLookPos.x += dir.x * 100.0f;
    mCam->mCameraLookPos.y += dir.y * 100.0f;
    mCam->mCameraLookPos.z += dir.z * 100.0f;
    mCam->SetFovy(camera->getFovy());
    DoInitCol();
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitFixChaseCam", 35);
}

void KartDemoCam::DoFixChaseCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f tmp;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    ChaseFovy(camera);
    up.set(0.0f, 1.0f, 0.0f);
    tmp.set(mCam->mTargetBody->mPlayerPosMtx[0][3],
            mCam->mTargetBody->mPlayerPosMtx[1][3],
            mCam->mTargetBody->mPlayerPosMtx[2][3]);
    mCam->mCameraPos.x = tmp.x + vec.mCamKartDiff.x * mCamKartDiffLength;
    mCam->mCameraPos.y = tmp.y + vec.mCamKartDiff.y * mCamKartDiffLength;
    mCam->mCameraPos.z = tmp.z + vec.mCamKartDiff.z * mCamKartDiffLength;
    mCam->mCameraLookPos.set(mCam->mCameraPos);
    JGeometry::TVec3f dir;
    camera->getFrontDirection(&dir);
    mCam->mCameraLookPos.x += dir.x * 100.0f;
    mCam->mCameraLookPos.y += dir.y * 100.0f;
    mCam->mCameraLookPos.z += dir.z * 100.0f;
    DoCol();
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoFixChaseCam", 36);
}

void KartDemoCam::InitChaseCam() {
    JGeometry::TVec3f up;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    up.set(0.0f, 1.0f, 0.0f);
    KartBody *body = mCam->mTargetBody;
    mCam->mCameraLookPos.set(body->mPlayerPosMtx[0][3],
                             body->mPlayerPosMtx[1][3] + 30.0f,
                             body->mPlayerPosMtx[2][3]);
    camera->getPosition(&mCam->mCameraPos);
    vec.mCamKartDiff.sub(mCam->mCameraPos, mCam->mCameraLookPos);
    mCamKartDiffLength = vec.mCamKartDiff.length();
    vec.mCamKartDiff.normalize();
    mCam->mCameraPos.x = mCam->mCameraLookPos.x + vec.mCamKartDiff.x * mCamKartDiffLength;
    mCam->mCameraPos.y = mCam->mCameraLookPos.y + vec.mCamKartDiff.y * mCamKartDiffLength;
    mCam->mCameraPos.z = mCam->mCameraLookPos.z + vec.mCamKartDiff.z * mCamKartDiffLength;
    mCam->SetFovy(camera->getFovy());
    DoInitCol();
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitChaseCam", 37);
}

void KartDemoCam::DoChaseCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f tmp;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    ChaseFovy(camera);
    up.set(0.0f, 1.0f, 0.0f);
    tmp.set(mCam->mTargetBody->mPlayerPosMtx[0][3],
            mCam->mTargetBody->mPlayerPosMtx[1][3] + 30.0f,
            mCam->mTargetBody->mPlayerPosMtx[2][3]);
    dir.sub(tmp, mCam->mCameraLookPos);
    f32 len = dir.length();
    dir.normalize();
    dir.scale(len * 0.6f);
    mCam->mCameraLookPos.add(dir);
    mCam->mCameraPos.x = mCam->mCameraLookPos.x + vec.mCamKartDiff.x * mCamKartDiffLength;
    mCam->mCameraPos.y = mCam->mCameraLookPos.y + vec.mCamKartDiff.y * mCamKartDiffLength;
    mCam->mCameraPos.z = mCam->mCameraLookPos.z + vec.mCamKartDiff.z * mCamKartDiffLength;
    DoCol();
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoChaseCam", 38);
}

void KartDemoCam::InitFixPathCam() {
    // void CrsData::Camera::getPathID() const {}
}

void KartDemoCam::DoFixPathCam() {
    // void CrsData::Camera::getPathSpeed() const {}
}

void KartDemoCam::InitChasePathCam() {}

void KartDemoCam::DoChasePathCam() {}

void KartDemoCam::InitFixSplCam() {}

void KartDemoCam::DoFixSplCam() {}

void KartDemoCam::InitChaseSplCam() {}

void KartDemoCam::DoChaseSplCam() {}

void KartDemoCam::InitFixPalaCam() {
    // void CrsData::Camera::getTargetPos2(JGeometry::TVec3<float> *) const {}
    // void CrsData::Camera::getTargetPos(JGeometry::TVec3<float> *) const {}
}

void KartDemoCam::DoFixPalaCam() {}

void KartDemoCam::InitDemoPathCam() {}

void KartDemoCam::DoDemoPathCam() {}

void KartDemoCam::InitDemoParallelView() {}

void KartDemoCam::DemoParallelView() {}

void KartDemoCam::InitDemoGoalView() {}

void KartDemoCam::DemoGoalView() {}

void KartDemoCam::MapView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f pos;
    JUTGamePad *pad = gpaGamePad[1];
    up.set(0.0f, 1.0f, 0.0f);
    f32 dist = 0.0f;
    if (pad->testButton(JUTGamePad::DPAD_UP)) {
        dist = 50.0f;
    } else if (pad->testButton(JUTGamePad::DPAD_DOWN)) {
        dist = -50.0f;
    }
    if (pad->testButton(JUTGamePad::L)) {
        _50.y += 0.05235988f;
    } else if (pad->testButton(JUTGamePad::R)) {
        _50.y -= 0.05235988f;
    }
    f32 azim = _50.y;
    _50.z += pad->getMainStickY() * 0.08726646f;
    if (_50.z > 0.0f) {
        _50.z = 0.0f;
    }
    if (_50.z < -1.3962634f) {
        _50.z = -1.3962634f;
    }
    if (pad->testButton(JUTGamePad::A)) {
        mCam->mFovY += 1.0f;
    } else if (pad->testButton(JUTGamePad::B)) {
        mCam->mFovY -= 1.0f;
    }
    if (mCam->mFovY > 180.0f) {
        mCam->mFovY = 180.0f;
    }
    if (mCam->mFovY < 1.0f) {
        mCam->mFovY = 1.0f;
    }
    if (pad->testButton(JUTGamePad::DPAD_RIGHT)) {
        azim -= 1.5707964f;
        dist = 50.0f;
    } else if (pad->testButton(JUTGamePad::DPAD_LEFT)) {
        azim += 1.5707964f;
        dist = 50.0f;
    }
    if (pad->testButton(JUTGamePad::Y)) {
        mCam->mCameraPos.y += 50.0f;
    } else if (pad->testButton(JUTGamePad::X)) {
        mCam->mCameraPos.y -= 50.0f;
    }
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraPos, (VecPtr)&pos, dist, 0.0f, azim);
    GetKartCtrl()->PolarToCross((VecPtr)&pos, (VecPtr)&mCam->mCameraLookPos, 1000.0f, _50.z, _50.y);
    mCam->mCameraPos.set(pos);
    if (pad->testButton(JUTGamePad::Z)) {
        mCam->mCameraPos.set(9000.0f, 3650.0f, 17000.0f);
        mCam->mCameraLookPos.set(9728.0f, 9176.0f, 16340.0f);
        mCam->mFovY = 40.0f;
        _50.z = 0.017453292f;
        _50.y = 2.3561945f;
    }
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    if (_3b == 0) {
        JUTReport(120, 80, "CAMERA");
        JUTReport(120, 100, "X:%7.1f", mCam->mCameraPos.x);
        JUTReport(120, 120, "Y:%7.1f", mCam->mCameraPos.y);
        JUTReport(120, 140, "Z:%7.1f", mCam->mCameraPos.z);
        JUTReport(290, 80, "LOOK");
        JUTReport(290, 100, "X:%7.1f", mCam->mCameraLookPos.x);
        JUTReport(290, 120, "Y:%7.1f", mCam->mCameraLookPos.y);
        JUTReport(290, 140, "Z:%7.1f", mCam->mCameraLookPos.z);
        f32 deg = azim * 180.0f / 3.1415927f;
        f32 mod = deg - (f32)((int)deg / 360) * 360.0f;
        JUTReport(120, 160, "Y_DIR%7.0f", mod);
        JUTReport(120, 180, "X_DIR%7.0f", _50.z * 180.0f / 3.1415927f);
        JUTReport(120, 200, "FOVY%7.1f", mCam->mFovY);
    }
}

void KartDemoCam::ObjectView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f pos;
    JUTGamePad *pad = gpaGamePad[1];
    up.set(0.0f, 1.0f, 0.0f);
    if (pad->testButton(JUTGamePad::L)) {
        _4c += 20.0f;
    }
    if (pad->testButton(JUTGamePad::R)) {
        _4c -= 20.0f;
    }
    if (_4c > 10000.0f) {
        _4c = 10000.0f;
    }
    if (_4c < 50.0f) {
        _4c = 50.0f;
    }
    _50.y += pad->getMainStickX() * 0.034906585f;
    f32 azim = _50.y;
    _50.z += pad->getMainStickY() * 0.034906585f;
    if (_50.z > 1.3962634f) {
        _50.z = 1.3962634f;
    }
    if (_50.z < -1.3962634f) {
        _50.z = -1.3962634f;
    }
    GetKartCtrl()->PolarToCross((VecPtr)&vec._8c, (VecPtr)&mCam->mCameraPos, _4c, _50.z, _50.y);
    mCam->mCameraLookPos.set(vec._8c);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    if (_3b == 0) {
        JUTReport(120, 80, "CAMERA");
        JUTReport(120, 100, "X:%7.1f", mCam->mCameraPos.x);
        JUTReport(120, 120, "Y:%7.1f", mCam->mCameraPos.y);
        JUTReport(120, 140, "Z:%7.1f", mCam->mCameraPos.z);
        JUTReport(290, 80, "LOOK");
        JUTReport(290, 100, "X:%7.1f", mCam->mCameraLookPos.x);
        JUTReport(290, 120, "Y:%7.1f", mCam->mCameraLookPos.y);
        JUTReport(290, 140, "Z:%7.1f", mCam->mCameraLookPos.z);
        f32 deg = azim * 180.0f / 3.1415927f;
        f32 mod = deg - (f32)((int)deg / 360) * 360.0f;
        JUTReport(120, 160, "Y_DIR%7.0f", mod);
        JUTReport(120, 180, "X_DIR%7.0f", _50.z * 180.0f / 3.1415927f);
    }
}

void KartDemoCam::DemoInitID() {
    // void CrsData::Camera::getKind() const {}
}

void KartDemoCam::InitID() {}

void KartDemoCam::DoDemoView() {}

void KartDemoCam::LiveViewChange() {}

void KartDemoCam::DemoView() {
    // void RaceMgr::requestDemoCameraID() const {}
}

void KartDemoCam::DoPathEnd(JGeometry::TVec3<float> *, float *) {}

void KartDemoCam::InitStartFixPathCam() {}

void KartDemoCam::DoStartFixPathCam() {}

void KartDemoCam::InitStartPathCam() {}

void KartDemoCam::DoStartPathCam() {}

void KartDemoCam::InitStartLookPathCam() {}

void KartDemoCam::DoStartLookPathCam() {}

void KartDemoCam::InitStartOnlyPathCam() {}

void KartDemoCam::DoStartOnlyPathCam() {}

void KartDemoCam::InitStartFixCam() {}

void KartDemoCam::DoStartFixCam() {}

void KartDemoCam::InitStartChasePathCam() {}

void KartDemoCam::DoStartChasePathCam() {}

void KartDemoCam::InitVsLastCam() {
    JGeometry::TVec3f up;
    _3c = 0;
    _6c = 0.0f;
    _40 = 1;
    mCamKartDiffLength = 0.0f;
    _3e = 1;
    up.set(0.0f, 1.0f, 0.0f);
    mCam->InitOutView();
    mCam->_18c *= 3.5f;
    f32 yaw = std::atan2f(mCam->mTargetBody->_308.x, mCam->mTargetBody->_308.z) + 3.1415927f;
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraLookPos, (VecPtr)&mCam->mCameraPos, mCam->_18c, mCam->_1ac, yaw);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitVsLastCam", 30);
}

void KartDemoCam::VsLastCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f out;
    up.set(0.0f, 1.0f, 0.0f);
    switch (_3a) {
    case 1:
        _3c++;
        if (GETJ2DManager()->getPreRaceWipeType() == 0) {
            if (_3c >= 10) {
                _3a = 2;
            }
        } else {
            if (_3c >= 3) {
                _3a = 2;
            }
        }
        break;
    case 2:
        mCam->mPathData[0].set(mCam->mCameraPos);
        _3c++;
        if (_3c >= 5) {
            _3a = 4;
        }
        break;
    case 5:
    case 4: {
        GetKartCtrl()->ChaseFnumber(&mCam->_18c, mCam->mTargetBody->_4e4, 0.03f);
        f32 yaw = std::atan2f(mCam->mTargetBody->_308.x, mCam->mTargetBody->_308.z) + 3.1415927f;
        GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraLookPos, (VecPtr)&mCam->mCameraPos, mCam->_18c, mCam->_1ac, yaw);
        if (RaceMgr::getCurrentManager()->isAbleStart()
            || (mCam->mTargetBody->mCarStatus & 0x0000000000008000ULL) != 0
            || !(GetKartCtrl()->GetCarSpeed(mCam->mTargetBody->mMynum) >= 5.0f)) {
            _38 = 0;
            mCam->mMode = 0;
            mCam->OutView();
        }
        break;
    }
    }
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("VsLastCam", 29);
}

void KartDemoCam::InitLastCam() {
    if (RaceMgr::getCurrentManager()->getCameraNumber() != 1) {
        InitVsLastCam();
        return;
    }
    JGeometry::TVec3f up;
    _3c = 0;
    _6c = 0.0f;
    _40 = 1;
    mCamKartDiffLength = 0.0f;
    _3e = 1;
    up.set(0.0f, 1.0f, 0.0f);
    mCam->InitOutView();
    TJugem *jugem = GetGeoObjMgr()->getJugem(mCam->mCamNo);
    mCam->_18c *= 0.95f;
    mCam->_1ac *= 0.15f;
    f32 yaw = std::atan2f(mCam->mTargetBody->_308.x, mCam->mTargetBody->_308.z) + 3.1415927f;
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraLookPos, (VecPtr)&mCam->mCameraPos, mCam->_18c, mCam->_1ac, yaw);
    vec.mCamKartDiff.set(mCam->mCameraPos);
    vec._a4.set(mCam->mCameraLookPos);
    mCam->mCameraLookPos.set(jugem->getPosition());
    yaw = std::atan2f(mCam->mTargetBody->_308.x, mCam->mTargetBody->_308.z) + 3.1415927f;
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraLookPos, (VecPtr)&mCam->mCameraLookPos, 80.0f, 0.0f, yaw);
    yaw = std::atan2f(mCam->mTargetBody->_308.x, mCam->mTargetBody->_308.z) + 2.0929f;
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraLookPos, (VecPtr)&mCam->mCameraPos, 200.0f, -0.5236f, yaw);
    mCam->mPathData[0].set(mCam->mCameraPos);
    yaw = std::atan2f(mCam->mTargetBody->_308.x, mCam->mTargetBody->_308.z) + 1.0466f;
    GetKartCtrl()->PolarToCross((VecPtr)&vec._a4, (VecPtr)&mCam->mPathData[1], 1.3f * mCam->mTargetBody->_4e4, 0.0f, yaw);
    yaw = std::atan2f(mCam->mTargetBody->_308.x, mCam->mTargetBody->_308.z) + 1.918f;
    GetKartCtrl()->PolarToCross((VecPtr)&vec._a4, (VecPtr)&mCam->mPathData[2], mCam->mTargetBody->_4e4, 0.0f, yaw);
    mCam->mPathData[3].set(vec.mCamKartDiff);
    mCam->_1b0 = 0.0f;
    mCam->_1b4 = 0.0f;
    mCam->mPathSpline->setAll(mCam->mPathData);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitLastCam", 28);
}
void KartDemoCam::LastCam() {
    if (RaceMgr::getCurrentManager()->getCameraNumber() != 1) {
        VsLastCam();
        return;
    }
    JGeometry::TVec3f up;
    JGeometry::TVec3f diff;
    JGeometry::TVec3f pt;
    up.set(0.0f, 1.0f, 0.0f);
    switch (_3a) {
    case 1:
        _3c++;
        if (GETJ2DManager()->getPreRaceWipeType() == 0) {
            if (_3c >= 10) {
                _3a = 2;
            }
        } else {
            if (_3c >= 3) {
                _3a = 2;
            }
        }
        break;
    case 2:
        mCam->mPathData[0].set(mCam->mCameraPos);
        _3c++;
        if (_3c >= 5) {
            _3a = 4;
        }
        break;
    case 4: {
        diff.sub(vec._a4, mCam->mCameraLookPos);
        f32 len = diff.length();
        GetKartCtrl()->ChaseFcnvge(&_6c, &_40, 0.08f, 3200);
        len *= _6c;
        diff.normalize();
        mCam->mCameraLookPos.x += diff.x * len;
        mCam->mCameraLookPos.y += diff.y * len;
        mCam->mCameraLookPos.z += diff.z * len;
        mCam->mPathSpline->setAll(mCam->mPathData);
        mCam->mPathSpline->getBezierPoint(&pt, mCam->_1b0);
        mCam->mCameraPos.set(pt);
        GetKartCtrl()->ChaseFcnvge(&mCamKartDiffLength, &_3e, 1.0f, 2000);
        mCam->_1b0 = mCamKartDiffLength;
        if ((mCam->mTargetBody->mGameStatus & 4) != 0) {
            if (!(mCam->_1b0 >= 0.9999f)) { if (len < 0.01f) {
                _3a = 5;
                _3c = 0;
            } }
        }
        break;
    }
    case 5: {
        mCam->DoLookChase();
        KartBody *body = mCam->mTargetBody;
        f32 a = body->_4e8 * 3.1415927f;
        f32 b = mCam->_180 * 3.1415927f;
        f32 angle = a / 180.0f + b / 180.0f;
        GetKartCtrl()->ChaseFnumber(&mCam->_1ac, angle, 0.04f);
        GetKartCtrl()->ChaseFnumber(&mCam->_18c, mCam->mTargetBody->_4e4, 0.02f);
        f32 yaw = std::atan2f(mCam->mTargetBody->_308.x, mCam->mTargetBody->_308.z) + 3.1415927f;
        GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraLookPos, (VecPtr)&_74, mCam->_18c, mCam->_1ac, yaw);
        diff.sub(_74, mCam->mCameraPos);
        f32 len = diff.length();
        diff.normalize();
        mCam->mCameraPos.x += diff.x * len;
        mCam->mCameraPos.y += diff.y * len;
        mCam->mCameraPos.z += diff.z * len;
        diff.sub(mCam->mCameraLookPos, mCam->mCameraPos);
        diff.normalize();
        diff.scale(mCam->_18c);
        mCam->mCameraPos.sub(mCam->mCameraLookPos, diff);
        mCam->_200.set(mCam->mCameraPos.x, mCam->mCameraPos.y, mCam->mCameraPos.z);
        if ((mCam->mTargetBody->mCarStatus & 0x0000000000008000ULL) != 0
            || mCam->_1ac >= angle - 0.01f
            || !(GetKartCtrl()->GetCarSpeed(mCam->mTargetBody->mMynum) >= 5.0f)) {
            _38 = 0;
            mCam->mMode = 0;
            mCam->OutView();
        }
        break;
    }
    }
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("LastCam", 27);
}

void KartDemoCam::InitStartDemoCam() {}

void KartDemoCam::StartDemoCam() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    if (_38 & 4) {
        LastCam();
        return;
    }
    switch (camera->getKind()) {
    case 4:
        DoStartFixPathCam();
        mCam->DegubCamera("DoStartFixPathCam", 23);
        break;
    case 5:
        DoStartPathCam();
        mCam->DegubCamera("DoStartPathCam", 24);
        break;
    case 6:
        DoStartLookPathCam();
        mCam->DegubCamera("DoStartLookPathCam", 25);
        break;
    case 1:
        DoStartOnlyPathCam();
        mCam->DegubCamera("DoStartOnlyPathCam", 26);
        break;
    }
}

void KartDemoCam::SetLastDemo() {
    _38 |= 4;
    _3c = 0;
    _3a = 1;
    if (mCam->mCamNo == 0) {
        mCam->mCtrl->mBitfield |= 2;
    }
    InitLastCam();
}

void KartDemoCam::SetRaceCam() {
    _38 = 0;
    mCam->mMode = 0;
    mCam->InitOutView();
    if (mCam->mCamNo == 0) {
        mCam->mCtrl->mBitfield |= 2;
    }
}

void KartDemoCam::SetNextID() {
    RaceMgr::getManager()->getCourse()->getCrsData();
    mStartId = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId)->getNextCameraID();
    if (mStartId >= 0) {
        InitStartDemoCam();
    } else {
        if (!(_38 & 0xc)) {
            GETJ2DManager()->startPreRace();
        }
        _38 |= 4;
        _3a = 0;
    }
}

void KartDemoCam::DoExecution() {
    if (_38 & 1) {
        if (_38 & 4) {
            SetRaceCam();
        } else {
            if (!(_38 & 8)) {
                GETJ2DManager()->startPreRace();
            }
            _38 |= 8;
        }
    }
}

int KartDemoCam::GetStartID() {
    mStartId = RaceMgr::getManager()->getCourse()->getCrsData()->getStartCameraID();
    if (mCam->mMode == 2) {
        return 1;
    }
    return (mStartId != -1) ? 1 : 0;
}

void KartDemoCam::InitStartID(bool isLast) {
    _38 = 1;
    if (isLast) {
        SetLastDemo();
    } else {
        InitStartDemoCam();
    }
}

void KartDemoCam::StartDemoView() { StartDemoCam(); }

void KartDemoCam::InitStartMapCam() {}

void KartDemoCam::StartMapCam() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    switch (camera->getKind()) {
    case 4:
        DoStartFixPathCam();
        mCam->DegubCamera("DoStartFixPathCam", 15);
        break;
    case 5:
        DoStartPathCam();
        mCam->DegubCamera("DoStartPathCam", 16);
        break;
    case 6:
        DoStartLookPathCam();
        mCam->DegubCamera("DoStartLookPathCam", 17);
        break;
    case 1:
        DoStartOnlyPathCam();
        mCam->DegubCamera("DoStartOnlyPathCam", 18);
        break;
    }
}

void KartDemoCam::SetMapNextID() {
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    mStartId = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId)->getNextCameraID();
    if (mStartId < 0) {
        mStartId = crsData->getStartLoserCameraID();
    }
    InitStartMapCam();
}

void KartDemoCam::SetUpMapID() {
    mStartId = RaceMgr::getManager()->getCourse()->getCrsData()->getStartLoserCameraID();
    if (mStartId >= 0) {
        InitStartMapCam();
        _38 |= 0x100;
    }
}

void KartDemoCam::InitStaffView() {}

void KartDemoCam::StaffView() {}

int KartDemoCam::SetStaffNextID() {
    RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    int result;
    if (camera->getNextCameraID() < 0) {
        result = 0;
    } else {
        mStartId = camera->getNextCameraID();
        InitStaffView();
        result = 1;
    }
    return result;
}

void KartDemoCam::SetUpStaffID() {
    mStartId = RaceMgr::getManager()->getCourse()->getCrsData()->getStartCameraID();
    _38 |= 0x200;
}
