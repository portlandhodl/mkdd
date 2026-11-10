#include "Yamamoto/KartDemoCam.h"
#include "Yamamoto/kartCamera.h"
#include "Yamamoto/KartMath.h"

#include "Kameda/J2DManager.h"
#include "Kaneshige/RaceMgr.h"

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
    _80.set(mCam->mCameraPos);
}

void KartDemoCam::DoCol() {
    CrsGround ground;
    JGeometry::TVec3f normal;
    ground.search(mCam->mCameraPos, _80);
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
    _80.set(mCam->mCameraPos);
}

void KartDemoCam::InitFixCam() {
    // void CrsData::Camera::getPosition(JGeometry::TVec3<float> *) const {}
}

void KartDemoCam::DoFixCam() {}

void KartDemoCam::InitFixSearchCam() {}

void KartDemoCam::DoFixSearchCam() {}

void KartDemoCam::InitFixChaseCam() {}

void KartDemoCam::DoFixChaseCam() {}

void KartDemoCam::InitChaseCam() {}

void KartDemoCam::DoChaseCam() {}

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

void KartDemoCam::MapView() {}

void KartDemoCam::ObjectView() {}

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

void KartDemoCam::InitVsLastCam() {}

void KartDemoCam::VsLastCam() {}

void KartDemoCam::InitLastCam() {
    // void TJugem::getPosition() const {}
}
void KartDemoCam::LastCam() {}

void KartDemoCam::InitStartDemoCam() {}

void KartDemoCam::StartDemoCam() {}

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

void KartDemoCam::StartMapCam() {}

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
