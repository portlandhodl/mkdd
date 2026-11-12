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
#include "mathHelper.h"

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
    if ((status & 0x400000ULL) == 0) {
        if (ground.getAttribute() == 2 || ground.getAttribute() == 0x12) {
            f32 scale = ground.getWallNormal(&normal, nullptr);
            mCam->mCameraPos.x += normal.x * scale * 2.0f;
            mCam->mCameraPos.y += normal.y * scale * 2.0f;
            mCam->mCameraPos.z += normal.z * scale * 2.0f;
        } else {
            f32 height = ground.getHeight();
            f32 diff = mCam->mCameraPos.y - 30.0f - height;
            if (diff < 0.0f) {
                mCam->mCameraPos.y -= diff;
            }
        }
    }
    _80.set(mCam->mCameraPos);
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
    mCam->mCameraLookPos.set(mBody->mPlayerPosMtx[0][3],
                             mBody->mPlayerPosMtx[1][3],
                             mBody->mPlayerPosMtx[2][3]);
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
    tmp.set(mBody->mPlayerPosMtx[0][3],
            mBody->mPlayerPosMtx[1][3],
            mBody->mPlayerPosMtx[2][3]);
    dir.sub(tmp, mCam->mCameraLookPos);
    f32 len = dir.length();
    dir.normalize();
    len *= 0.3f;
    dir.scale(len);
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
    tmp.set(mBody->mPlayerPosMtx[0][3],
            mBody->mPlayerPosMtx[1][3],
            mBody->mPlayerPosMtx[2][3]);
    mCamKartDiff.sub(mCam->mCameraPos, tmp);
    mCamKartDiffLength = mCamKartDiff.length();
    mCamKartDiff.normalize();
    mCam->mCameraPos.x = tmp.x + mCamKartDiff.x * mCamKartDiffLength;
    mCam->mCameraPos.y = tmp.y + mCamKartDiff.y * mCamKartDiffLength;
    mCam->mCameraPos.z = tmp.z + mCamKartDiff.z * mCamKartDiffLength;
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
    tmp.set(mBody->mPlayerPosMtx[0][3],
            mBody->mPlayerPosMtx[1][3],
            mBody->mPlayerPosMtx[2][3]);
    mCam->mCameraPos.x = tmp.x + mCamKartDiff.x * mCamKartDiffLength;
    mCam->mCameraPos.y = tmp.y + mCamKartDiff.y * mCamKartDiffLength;
    mCam->mCameraPos.z = tmp.z + mCamKartDiff.z * mCamKartDiffLength;
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
    KartBody *body = mBody;
    mCam->mCameraLookPos.set(body->mPlayerPosMtx[0][3],
                             body->mPlayerPosMtx[1][3] + 30.0f,
                             body->mPlayerPosMtx[2][3]);
    camera->getPosition(&mCam->mCameraPos);
    mCamKartDiff.sub(mCam->mCameraPos, mCam->mCameraLookPos);
    mCamKartDiffLength = mCamKartDiff.length();
    mCamKartDiff.normalize();
    mCam->mCameraPos.x = mCam->mCameraLookPos.x + mCamKartDiff.x * mCamKartDiffLength;
    mCam->mCameraPos.y = mCam->mCameraLookPos.y + mCamKartDiff.y * mCamKartDiffLength;
    mCam->mCameraPos.z = mCam->mCameraLookPos.z + mCamKartDiff.z * mCamKartDiffLength;
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
    tmp.set(mBody->mPlayerPosMtx[0][3],
            mBody->mPlayerPosMtx[1][3] + 30.0f,
            mBody->mPlayerPosMtx[2][3]);
    dir.sub(tmp, mCam->mCameraLookPos);
    f32 len = dir.length();
    dir.normalize();
    len *= 0.6f;
    dir.scale(len);
    mCam->mCameraLookPos.add(dir);
    mCam->mCameraPos.x = mCam->mCameraLookPos.x + mCamKartDiff.x * mCamKartDiffLength;
    mCam->mCameraPos.y = mCam->mCameraLookPos.y + mCamKartDiff.y * mCamKartDiffLength;
    mCam->mCameraPos.z = mCam->mCameraLookPos.z + mCamKartDiff.z * mCamKartDiffLength;
    DoCol();
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoChaseCam", 38);
}

void KartDemoCam::InitFixPathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    if (camera->getPathID() < 0) {
        mCam->InitOutView();
        return;
    }
    crsData->getPathData(camera->getPathID());
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 0);
    mCam->mCameraPos.set(point->pos);
    up.set(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f dir;
    camera->getFrontDirection(&dir);
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraPos, (VecPtr)&mCam->mCameraLookPos, 50.0f, 0.0f, std::atan2f(dir.x, dir.z));
    mCam->SetFovy(camera->getFovy());
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitFixPathCam", 39);
}

void KartDemoCam::DoFixPathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f dir;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 1);
    up.set(0.0f, 1.0f, 0.0f);
    if (camera->getPathID() < 0) {
        mCam->OutView();
        return;
    }
    tmp.set(point->pos);
    dir.sub(tmp, mCam->mCameraPos);
    int speed = camera->getPathSpeed();
    if (speed == 0) {
        speed = 1;
    }
    if (speed > 100) {
        speed = 100;
    }
    f32 scale = speed * 0.0005f * dir.length();
    dir.normalize();
    dir.scale(scale);
    mCam->mCameraPos.add(dir);
    ChaseFovy(camera);
    JGeometry::TVec3f front;
    camera->getFrontDirection(&front);
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraPos, (VecPtr)&mCam->mCameraLookPos, 50.0f, 0.0f, std::atan2f(front.x, front.z));
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoFixPathCam", 40);
}

void KartDemoCam::InitChasePathCam() {
    JGeometry::TVec3f up;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    if (camera->getPathID() < 0) {
        mCam->InitOutView();
        return;
    }
    crsData->getPathData(camera->getPathID());
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 0);
    mCam->mCameraPos.set(point->pos);
    up.set(0.0f, 1.0f, 0.0f);
    mCam->mCameraLookPos.set(mBody->mPlayerPosMtx[0][3],
                             mBody->mPlayerPosMtx[1][3] + 30.0f,
                             mBody->mPlayerPosMtx[2][3]);
    mCam->SetFovy(camera->getFovy());
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitChasePathCam", 41);
}

void KartDemoCam::DoChasePathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f dir2;
    JGeometry::TVec3f target;
    JGeometry::TVec3f dir;
    up.set(0.0f, 1.0f, 0.0f);
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 1);
    if (camera->getPathID() < 0) {
        mCam->OutView();
        return;
    }
    tmp.set(point->pos);
    dir.sub(tmp, mCam->mCameraPos);
    int speed = camera->getPathSpeed();
    if (speed == 0) {
        speed = 1;
    }
    if (speed > 100) {
        speed = 100;
    }
    f32 scale = speed * 0.0005f * dir.length();
    dir.normalize();
    dir.scale(scale);
    mCam->mCameraPos.add(dir);
    target.set(mBody->mPlayerPosMtx[0][3],
               mBody->mPlayerPosMtx[1][3] + 30.0f,
               mBody->mPlayerPosMtx[2][3]);
    dir2.sub(target, mCam->mCameraLookPos);
    f32 len2 = dir2.length();
    dir2.normalize();
    len2 *= 0.6f;
    dir2.scale(len2);
    mCam->mCameraLookPos.add(dir2);
    ChaseFovy(camera);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoChasePathCam", 42);
}

void KartDemoCam::InitFixSplCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    if (camera->getPathID() < 0) {
        mCam->InitOutView();
        return;
    }
    mCamKartDiffLength = 0.0f;
    _3e = 1;
    crsData->getPathData(camera->getPathID());
    mCam->_1b0 = 0.0f;
    int off = 0;
    for (int i = 0; i < 4; i++, off += 12) {
        mCam->mPathData[i].set(crsData->getPointData(camera->getPathID(), i)->pos);
    }
    mCam->mPathSpline->setAll(mCam->mPathData);
    mCam->mCameraPos.set(mCam->mPathData[0]);
    mCam->_1b0 = 0.0f;
    up.set(0.0f, 1.0f, 0.0f);
    JGeometry::TVec3f dir;
    camera->getFrontDirection(&dir);
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraPos, (VecPtr)&mCam->mCameraLookPos, 50.0f, 0.0f, std::atan2f(dir.x, dir.z));
    mCam->SetFovy(camera->getFovy());
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitFixSplCam", 43);
}

void KartDemoCam::DoFixSplCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f bezier;
    JGeometry::TVec3f unused2;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    crsData->getPointData(camera->getPathID(), 1);
    up.set(0.0f, 1.0f, 0.0f);
    if (camera->getPathID() < 0) {
        mCam->OutView();
        return;
    }
    mCam->mPathSpline->setAll(mCam->mPathData);
    mCam->mPathSpline->getBezierPoint(&bezier, mCam->_1b0);
    mCam->mCameraPos.set(bezier);
    u16 speed = camera->getPathSpeed();
    if (speed == 0) {
        speed = 1;
    }
    if (speed > 100) {
        speed = 100;
    }
    u16 div = 100;
    div /= speed;
    GetKartCtrl()->ChaseFcnvge4(&mCamKartDiffLength, &_3e, 1.0f, div);
    mCam->_1b0 = mCamKartDiffLength;
    ChaseFovy(camera);
    JGeometry::TVec3f dir;
    camera->getFrontDirection(&dir);
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraPos, (VecPtr)&mCam->mCameraLookPos, 50.0f, 0.0f, std::atan2f(dir.x, dir.z));
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoFixSplCam", 44);
}

void KartDemoCam::InitChaseSplCam() {
    JGeometry::TVec3f up;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    int i;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    if (camera->getPathID() < 0) {
        mCam->InitOutView();
        return;
    }
    mCamKartDiffLength = 0.0f;
    _3e = 1;
    crsData->getPathData(camera->getPathID());
    mCam->_1b0 = 0.0f;
    int off = 0;
    for (i = 0; i < 4; i++, off += 12) {
        mCam->mPathData[i].set(crsData->getPointData(camera->getPathID(), i)->pos);
    }
    mCam->mPathSpline->setAll(mCam->mPathData);
    mCam->mCameraPos.set(mCam->mPathData[0]);
    mCam->_1b0 = 0.0f;
    mCam->_e5 = 0;
    up.set(0.0f, 1.0f, 0.0f);
    mCam->mCameraLookPos.set(mBody->mPlayerPosMtx[0][3],
                             mBody->mPlayerPosMtx[1][3] + 30.0f,
                             mBody->mPlayerPosMtx[2][3]);
    mCam->SetFovy(camera->getFovy());
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitChaseSplCam", 45);
}

void KartDemoCam::DoChaseSplCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f dir2;
    JGeometry::TVec3f target;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f bezier;
    JGeometry::TVec3f unused2;
    up.set(0.0f, 1.0f, 0.0f);
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    crsData->getPointData(camera->getPathID(), 1);
    if (camera->getPathID() < 0) {
        mCam->OutView();
        return;
    }
    mCam->mPathSpline->setAll(mCam->mPathData);
    mCam->mPathSpline->getBezierPoint(&bezier, mCam->_1b0);
    mCam->mCameraPos.set(bezier);
    u16 speed = camera->getPathSpeed();
    if (speed == 0) {
        speed = 1;
    }
    if (speed > 100) {
        speed = 100;
    }
    u16 div = 100;
    div /= speed;
    GetKartCtrl()->ChaseFcnvge4(&mCamKartDiffLength, &_3e, 1.0f, div);
    mCam->_1b0 = mCamKartDiffLength;
    target.set(mBody->mPlayerPosMtx[0][3],
               mBody->mPlayerPosMtx[1][3] + 30.0f,
               mBody->mPlayerPosMtx[2][3]);
    dir2.sub(target, mCam->mCameraLookPos);
    f32 len2 = dir2.length();
    dir2.normalize();
    len2 *= 0.6f;
    dir2.scale(len2);
    mCam->mCameraLookPos.add(dir2);
    ChaseFovy(camera);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoChaseSplCam", 46);
}

void KartDemoCam::InitFixPalaCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f tmp2;
    JGeometry::TVec3f targetPos;
    Mtx rotY;
    Mtx rotX;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    if (mCam->mTargetBody->mCarStatus & 0x00100000ULL) {
        mMat[0][0] = 1.0f;
        mMat[1][0] = 0.0f;
        mMat[2][0] = 0.0f;
        mMat[0][1] = 0.0f;
        mMat[1][1] = 1.0f;
        mMat[2][1] = 0.0f;
        mMat[0][2] = 0.0f;
        mMat[1][2] = 0.0f;
        mMat[2][2] = 1.0f;
        mMat[0][3] = mCam->mTargetBody->_110[0][3];
        mMat[1][3] = mCam->mTargetBody->_110[1][3];
        mMat[2][3] = mCam->mTargetBody->_110[2][3];
    } else {
        PSMTXCopy(mCam->mTargetBody->_110, mMat);
    }
    up.set(0.0f, 1.0f, 0.0f);
    mCam->SetFovy(camera->getFovy());
    camera->getTargetPos(&targetPos);
    mCam->mCameraLookPos.set(mBody->mPos);
    mCam->mCameraLookPos.y += 30.0f;
    if (mCam->mTargetBody->mIdx == 16) {
        mCam->mCameraLookPos.y += 40.0f;
    } else if (mCam->mTargetBody->mIdx == 17) {
        mCam->mCameraLookPos.y += 40.0f;
    } else if (mCam->mTargetBody->mIdx == 13) {
        mCam->mCameraLookPos.y += 40.0f;
    } else if (mCam->mTargetBody->mIdx == 5) {
        mCam->mCameraLookPos.y += 40.0f;
    }
    GetKartCtrl()->RotYMatrix(rotY, (3.141f * targetPos.x) / 180.0f);
    GetKartCtrl()->RotXMatrix(rotX, (3.141f * targetPos.y) / 180.0f);
    PSMTXConcat(rotY, rotX, rotY);
    PSMTXConcat(mCam->mTargetBody->_110, rotY, rotY);
    mCam->mCameraPos.x = mCam->mCameraLookPos.x + rotY[0][2] * targetPos.z;
    mCam->mCameraPos.y = mCam->mCameraLookPos.y + rotY[1][2] * targetPos.z;
    mCam->mCameraPos.z = mCam->mCameraLookPos.z + rotY[2][2] * targetPos.z;
    camera->getTargetPos2(&targetPos);
    GetKartCtrl()->RotYMatrix(rotY, (3.141f * targetPos.x) / 180.0f);
    GetKartCtrl()->RotXMatrix(rotX, (3.141f * targetPos.y) / 180.0f);
    PSMTXConcat(rotY, rotX, rotY);
    PSMTXConcat(mCam->mTargetBody->_110, rotY, rotY);
    targetPos.z = 100.0f;
    mCam->mCameraLookPos.x = rotY[0][2] * targetPos.z + mCam->mCameraPos.x;
    mCam->mCameraLookPos.y = rotY[1][2] * targetPos.z + mCam->mCameraPos.y;
    mCam->mCameraLookPos.z = rotY[2][2] * targetPos.z + mCam->mCameraPos.z;
    DoInitCol();
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitFixPalaCam", 47);
}

void KartDemoCam::DoFixPalaCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f tmp2;
    JGeometry::TVec3f targetPos;
    Mtx base;
    Mtx rotY;
    Mtx rotX;
    if (mCam->mTargetBody->mCarStatus & 0x00100000ULL) {
        PSMTXCopy(mMat, base);
    } else {
        PSMTXCopy(mBody->_110, mMat);
        PSMTXCopy(mBody->_110, base);
    }
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    ChaseFovy(camera);
    up.set(0.0f, 1.0f, 0.0f);
    camera->getTargetPos(&targetPos);
    mCam->mCameraLookPos.set(mBody->mPos);
    mCam->mCameraLookPos.y += 30.0f;
    if (mCam->mTargetBody->mIdx == 16) {
        mCam->mCameraLookPos.y += 40.0f;
    } else if (mCam->mTargetBody->mIdx == 17) {
        mCam->mCameraLookPos.y += 40.0f;
    } else if (mCam->mTargetBody->mIdx == 13) {
        mCam->mCameraLookPos.y += 40.0f;
    } else if (mCam->mTargetBody->mIdx == 5) {
        mCam->mCameraLookPos.y += 40.0f;
    }
    GetKartCtrl()->RotYMatrix(rotY, (3.141f * targetPos.x) / 180.0f);
    GetKartCtrl()->RotXMatrix(rotX, (3.141f * targetPos.y) / 180.0f);
    PSMTXConcat(rotY, rotX, rotY);
    PSMTXConcat(base, rotY, rotY);
    mCam->mCameraPos.x = mCam->mCameraLookPos.x + rotY[0][2] * targetPos.z;
    mCam->mCameraPos.y = mCam->mCameraLookPos.y + rotY[1][2] * targetPos.z;
    mCam->mCameraPos.z = mCam->mCameraLookPos.z + rotY[2][2] * targetPos.z;
    camera->getTargetPos2(&targetPos);
    GetKartCtrl()->RotYMatrix(rotY, (3.141f * targetPos.x) / 180.0f);
    GetKartCtrl()->RotXMatrix(rotX, (3.141f * targetPos.y) / 180.0f);
    PSMTXConcat(rotY, rotX, rotY);
    PSMTXConcat(base, rotY, rotY);
    targetPos.z = 100.0f;
    mCam->mCameraLookPos.x = rotY[0][2] * targetPos.z + mCam->mCameraPos.x;
    mCam->mCameraLookPos.y = rotY[1][2] * targetPos.z + mCam->mCameraPos.y;
    mCam->mCameraLookPos.z = rotY[2][2] * targetPos.z + mCam->mCameraPos.z;
    DoCol();
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoFixPalaCam", 48);
}

void KartDemoCam::InitDemoPathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = crsData->getCameraData(mCam->mCurCamIdx);
    _3c = 0;
    crsData->getPathData(camera->getPathID());
    mCam->mCameraPos.set(crsData->getPointData(camera->getPathID(), 0)->pos);
    up.set(0.0f, 1.0f, 0.0f);
    mCam->SetFovy(camera->getFovy());
    camera->getTargetPos(&mCam->mCameraLookPos);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitDemoPathCam", 49);
}

void KartDemoCam::DoDemoPathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f target;
    JGeometry::TVec3f dir;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = crsData->getCameraData(mCam->mCurCamIdx);
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 1);
    up.set(0.0f, 1.0f, 0.0f);
    target.set(point->pos);
    dir.sub(target, mCam->mCameraPos);
    f32 scale = 0.0005f * camera->getPathSpeed();
    scale *= dir.length();
    dir.normalize();
    dir.scale(scale);
    mCam->mCameraPos.add(dir);
    camera->getTargetPos2(&dir);
    dir.sub(dir, mCam->mCameraLookPos);
    scale = 0.01f * dir.length();
    dir.normalize();
    dir.scale(scale);
    mCam->mCameraLookPos.add(dir);
    ChaseFovy(camera);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DoDemoPathCam", 50);
}

void KartDemoCam::InitDemoParallelView() {
    JGeometry::TVec3f look;
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    mCam->SetFovy(viewdata[mCam->mPtr].fovY);
    look.set(mBody->mPlayerPosMtx[0][3], mBody->mPlayerPosMtx[1][3], mBody->mPlayerPosMtx[2][3]);
    mCam->mCameraLookPos.sub(look, mBody->_350);
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraLookPos, (VecPtr)&mCam->mCameraPos, 500.0f, 0.348888f, 0.0f);
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitDemoParallelView", 51);
}

void KartDemoCam::DemoParallelView() {
    JGeometry::TVec3f look;
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    mCam->SetFovy(viewdata[mCam->mPtr].fovY);
    look.set(mBody->mPlayerPosMtx[0][3], mBody->mPlayerPosMtx[1][3], mBody->mPlayerPosMtx[2][3]);
    mCam->mCameraLookPos.sub(look, mBody->_350);
    GetKartCtrl()->PolarToCross((VecPtr)&mCam->mCameraLookPos, (VecPtr)&mCam->mCameraPos, 500.0f, 0.348888f, 0.0f);
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DemoParallelView", 52);
}

void KartDemoCam::InitDemoGoalView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f tmp2;
    JGeometry::TVec3f targetPos;
    Mtx rotY;
    Mtx rotX;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    if (crsData->searchCameraID('para') == -1) {
        InitDemoParallelView();
        return;
    }
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(crsData->searchCameraID('para'));
    if (mCam->mTargetBody->mCarStatus & 0x00100000ULL) {
        mMat[0][0] = 1.0f;
        mMat[1][0] = 0.0f;
        mMat[2][0] = 0.0f;
        mMat[0][1] = 0.0f;
        mMat[1][1] = 1.0f;
        mMat[2][1] = 0.0f;
        mMat[0][2] = 0.0f;
        mMat[1][2] = 0.0f;
        mMat[2][2] = 1.0f;
        mMat[0][3] = mCam->mTargetBody->_110[0][3];
        mMat[1][3] = mCam->mTargetBody->_110[1][3];
        mMat[2][3] = mCam->mTargetBody->_110[2][3];
    } else {
        PSMTXCopy(mCam->mTargetBody->_110, mMat);
    }
    up.set(0.0f, 1.0f, 0.0f);
    mCam->SetFovy(camera->getFovy());
    camera->getTargetPos(&targetPos);
    mCam->mCameraLookPos.set(mBody->mPos);
    mCam->mCameraLookPos.y += 30.0f;
    if (mCam->mTargetBody->getThunder()->mFlags & 1) {
        if (mCam->mTargetBody->mIdx == 16) {
            mCam->mCameraLookPos.y += 60.0f;
        } else if (mCam->mTargetBody->mIdx == 17) {
            mCam->mCameraLookPos.y += 60.0f;
        } else if (mCam->mTargetBody->mIdx == 13) {
            mCam->mCameraLookPos.y += 40.0f;
        } else if (mCam->mTargetBody->mIdx == 5) {
            mCam->mCameraLookPos.y += 40.0f;
        }
    }
    GetKartCtrl()->RotYMatrix(rotY, (3.141f * targetPos.x) / 180.0f);
    GetKartCtrl()->RotXMatrix(rotX, (3.141f * targetPos.y) / 180.0f);
    PSMTXConcat(rotY, rotX, rotY);
    PSMTXConcat(mMat, rotY, rotY);
    mCam->mCameraPos.x = mCam->mCameraLookPos.x + rotY[0][2] * targetPos.z;
    mCam->mCameraPos.y = mCam->mCameraLookPos.y + rotY[1][2] * targetPos.z;
    mCam->mCameraPos.z = mCam->mCameraLookPos.z + rotY[2][2] * targetPos.z;
    camera->getTargetPos2(&targetPos);
    GetKartCtrl()->RotYMatrix(rotY, (3.141f * targetPos.x) / 180.0f);
    GetKartCtrl()->RotXMatrix(rotX, (3.141f * targetPos.y) / 180.0f);
    PSMTXConcat(rotY, rotX, rotY);
    PSMTXConcat(mMat, rotY, rotY);
    targetPos.z = 100.0f;
    mCam->mCameraLookPos.x = rotY[0][2] * targetPos.z + mCam->mCameraPos.x;
    mCam->mCameraLookPos.y = rotY[1][2] * targetPos.z + mCam->mCameraPos.y;
    mCam->mCameraLookPos.z = rotY[2][2] * targetPos.z + mCam->mCameraPos.z;
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitDemoGoalView", 53);
}

void KartDemoCam::DemoGoalView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f tmp2;
    JGeometry::TVec3f targetPos;
    Mtx base;
    Mtx rotY;
    Mtx rotX;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    if (crsData->searchCameraID('para') == -1) {
        DemoParallelView();
        return;
    }
    if (mCam->mTargetBody->mCarStatus & 0x00100000ULL) {
        PSMTXCopy(mMat, base);
    } else {
        PSMTXCopy(mBody->_110, mMat);
        PSMTXCopy(mBody->_110, base);
    }
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(crsData->searchCameraID('para'));
    ChaseFovy(camera);
    up.set(0.0f, 1.0f, 0.0f);
    camera->getTargetPos(&targetPos);
    mCam->mCameraLookPos.set(mBody->mPos);
    mCam->mCameraLookPos.y += 30.0f;
    if (mCam->mTargetBody->getThunder()->mFlags & 1) {
        if (mCam->mTargetBody->mIdx == 16) {
            mCam->mCameraLookPos.y += 60.0f;
        } else if (mCam->mTargetBody->mIdx == 17) {
            mCam->mCameraLookPos.y += 60.0f;
        } else if (mCam->mTargetBody->mIdx == 13) {
            mCam->mCameraLookPos.y += 40.0f;
        } else if (mCam->mTargetBody->mIdx == 5) {
            mCam->mCameraLookPos.y += 40.0f;
        }
    }
    GetKartCtrl()->RotYMatrix(rotY, (3.141f * targetPos.x) / 180.0f);
    GetKartCtrl()->RotXMatrix(rotX, (3.141f * targetPos.y) / 180.0f);
    PSMTXConcat(rotY, rotX, rotY);
    PSMTXConcat(base, rotY, rotY);
    mCam->mCameraPos.x = mCam->mCameraLookPos.x + rotY[0][2] * targetPos.z;
    mCam->mCameraPos.y = mCam->mCameraLookPos.y + rotY[1][2] * targetPos.z;
    mCam->mCameraPos.z = mCam->mCameraLookPos.z + rotY[2][2] * targetPos.z;
    camera->getTargetPos2(&targetPos);
    GetKartCtrl()->RotYMatrix(rotY, (3.141f * targetPos.x) / 180.0f);
    GetKartCtrl()->RotXMatrix(rotX, (3.141f * targetPos.y) / 180.0f);
    PSMTXConcat(rotY, rotX, rotY);
    PSMTXConcat(base, rotY, rotY);
    targetPos.z = 100.0f;
    mCam->mCameraLookPos.x = rotY[0][2] * targetPos.z + mCam->mCameraPos.x;
    mCam->mCameraLookPos.y = rotY[1][2] * targetPos.z + mCam->mCameraPos.y;
    mCam->mCameraLookPos.z = rotY[2][2] * targetPos.z + mCam->mCameraPos.z;
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("DemoGoalView", 54);
}

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
        _50.y += 0.0523333f;
    } else if (pad->testButton(JUTGamePad::R)) {
        _50.y -= 0.0523333f;
    }
    f32 azim = _50.y;
    _50.z += pad->getMainStickY() * 0.0872222f;
    if (_50.z > 1.395555f) {
        _50.z = 1.395555f;
    }
    if (_50.z < -1.395555f) {
        _50.z = -1.395555f;
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
        azim -= 1.57f;
        dist = 50.0f;
    } else if (pad->testButton(JUTGamePad::DPAD_LEFT)) {
        azim += 1.57f;
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
        mCam->mCameraLookPos.set(9728.0f, 3670.0f, 16340.0f);
        mCam->mFovY = 40.0f;
        _50.z = 0.0174444f;
        _50.y = 2.3549922f;
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
        f32 deg = azim * 180.0f / 3.141f;
        f32 mod = deg - (f32)((int)deg / 360) * 360.0f;
        JUTReport(120, 160, "Y_DIR%7.0f", mod);
        JUTReport(120, 180, "X_DIR%7.0f", _50.z * 180.0f / 3.141f);
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
    _50.y += pad->getMainStickX() * 0.0348888f;
    f32 azim = _50.y;
    _50.z += pad->getMainStickY() * 0.0348888f;
    if (_50.z > 1.395555f) {
        _50.z = 1.395555f;
    }
    if (_50.z < -1.395555f) {
        _50.z = -1.395555f;
    }
    GetKartCtrl()->PolarToCross((VecPtr)&_8c, (VecPtr)&mCam->mCameraPos, _4c, _50.z, _50.y);
    mCam->mCameraLookPos.set(_8c);
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
        f32 deg = azim * 180.0f / 3.141f;
        f32 mod = deg - (f32)((int)deg / 360) * 360.0f;
        JUTReport(120, 160, "Y_DIR%7.0f", mod);
        JUTReport(120, 180, "X_DIR%7.0f", _50.z * 180.0f / 3.141f);
    }
}

void KartDemoCam::GetPerView() {
    KartPerCam *per = mBody->getPerCam();
    JUTReport(120, 80, "CAMERA");
    JUTReport(120, 100, "X:%7.1f", per->mPos.x);
    JUTReport(120, 120, "Y:%7.1f", per->mPos.y);
    JUTReport(120, 140, "Z:%7.1f", per->mPos.z);
    JUTReport(120, 160, "ID:%7d", per->mCamId);
}

void KartDemoCam::DemoInitID() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    if (camera->getKind() == 0) {
        InitFixCam();
    } else if (camera->getKind() == 0x100) {
        InitFixSearchCam();
    } else if (camera->getKind() == 1) {
        InitFixPathCam();
    } else if (camera->getKind() == 0x101) {
        InitChasePathCam();
    } else if (camera->getKind() == 2) {
        InitFixChaseCam();
    } else if (camera->getKind() == 0x102) {
        InitChaseCam();
    } else if (camera->getKind() == 7) {
        InitFixPalaCam();
    } else if (camera->getKind() == 3) {
        InitFixSplCam();
    } else if (camera->getKind() == 0x103) {
        InitChaseSplCam();
    } else if (camera->getKind() == 5) {
        InitDemoPathCam();
    } else {
        mCam->InitOutView();
    }
}

void KartDemoCam::InitID() {
    if (mCam->mFlags & 0x200) {
        mCam->mCurCamIdx = RaceMgr::getManager()->getCourse()->getCrsData()->searchCameraID('mini');
        _38 |= 0x80;
    } else if (!(mBody->mGameStatus & 0x40) && !(_38 & 0x40)) {
        mCam->mCurCamIdx = mCam->mArea.getCameraID();
    }
    if (mCam->mFlags & 0x100) {
        InitDemoParallelView();
        _38 |= 0x20;
    } else if (mBody->mGameStatus & 0x40) {
        InitDemoGoalView();
    } else if (mCam->mCurCamIdx < 0 || (mBody->mCarStatus & 0x400000)) {
        InitDemoParallelView();
        _38 |= 0x10;
    } else {
        if (RaceMgr::getCurrentManager()->isAwardDemoMode() && RaceMgr::getCurrentManager()->getAwardKartNo() == -1) {
            mCam->mCurCamIdx = RaceMgr::getManager()->getCourse()->getCrsData()->searchCameraID('next');
        }
        RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
        DemoInitID();
    }
}

void KartDemoCam::DoDemoView() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    if (camera->getKind() == 0) {
        DoFixCam();
    } else if (camera->getKind() == 0x100) {
        DoFixSearchCam();
    } else if (camera->getKind() == 1) {
        DoFixPathCam();
    } else if (camera->getKind() == 0x101) {
        DoChasePathCam();
    } else if (camera->getKind() == 2) {
        DoFixChaseCam();
    } else if (camera->getKind() == 0x102) {
        DoChaseCam();
    } else if (camera->getKind() == 7) {
        DoFixPalaCam();
    } else if (camera->getKind() == 3) {
        DoFixSplCam();
    } else if (camera->getKind() == 0x103) {
        DoChaseSplCam();
    } else if (camera->getKind() == 5) {
        DoDemoPathCam();
    } else {
        mCam->ParallelView();
    }
}

void KartDemoCam::LiveViewChange() {
    if (mCam->mFlags & 0x100) {
        int kartNum = GetKartCtrl()->GetKartNumber();
        int i;
        for (i = 0; i < kartNum; i++) {
            if (RCMGetKartChecker(i)->getRank() == 1) {
                break;
            }
        }
        if (i < kartNum && i != mCam->GetTargetNum()) {
            GETJ2DManager()->setStatus2Kart(mCam->mCamNo, i);
            mCam->SetTargetNum(i);
            mCam->SetTarget();
            mBody = mCam->getBody();
            InitDemoParallelView();
            _38 |= 0x20;
        }
    }
}

void KartDemoCam::DemoView() {
    if (_38 & 0x80) {
        mCam->mCurCamIdx = RaceMgr::getManager()->getCourse()->getCrsData()->searchCameraID('mini');
    } else {
        if (!(_38 & 0x40)) {
            if (RaceMgr::getCurrentManager()->requestDemoCameraID() >= 0) {
                mCam->mCurCamIdx = RaceMgr::getManager()->requestDemoCameraID();
                _38 |= 0x40;
                InitID();
                return;
            }
        }
        if (!(mCam->mFlags & 0x100)) {
            if (mBody->mGameStatus & 0x40) {
                DemoGoalView();
                return;
            }
        }
        if (!(_38 & 0x40)) {
            mCam->mArea.searchCamera(mBody->mPos);
            LiveViewChange();
            if (mCam->mArea.getCameraID() != mCam->mCurCamIdx) {
                _38 &= ~0x10;
                InitID();
                return;
            }
        }
        if (RaceMgr::getCurrentManager()->isAwardDemoMode()) {
            if (RaceMgr::getCurrentManager()->getAwardKartNo() == -1) {
                mCam->mCurCamIdx = RaceMgr::getManager()->getCourse()->getCrsData()->searchCameraID('next');
            }
        }
    }
    RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mCam->mCurCamIdx);
    if (mCam->mCurCamIdx < 0 || (_38 & 0x30)) {
        DemoParallelView();
    } else {
        DoDemoView();
    }
}

bool KartDemoCam::DoPathEnd(JGeometry::TVec3<float> *dir, float *out) {
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = crsData->getCameraData(mStartId);
    f32 speed = camera->getPathSpeed();
    f32 dist = 0.0005f * speed;
    dist = dist * dir->length();
    if (camera->getTime() != 0) {
        if (_3c < 0xffff) {
            _3c++;
        }
        if (_3c < camera->getTime()) {
            goto store;
        }
        if (_38 & 0x100) {
            SetMapNextID();
        } else if (_38 & 0x200) {
            if (!SetStaffNextID()) {
                return false;
            }
        } else {
            SetNextID();
        }
        return true;
    }
    if (dist < 5.0f) {
        if (_38 & 0x100) {
            SetMapNextID();
        } else if (_38 & 0x200) {
            if (!SetStaffNextID()) {
                return false;
            }
        } else {
            SetNextID();
        }
        return true;
    }
store:
    *out = dist;
    return false;
}

void KartDemoCam::InitStartFixPathCam() {
    JGeometry::TVec3f up;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = crsData->getCameraData(mStartId);
    crsData->getPathData(camera->getPathID());
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 0);
    mCam->mCameraPos.set(point->pos);
    up.set(0.0f, 1.0f, 0.0f);
    mCam->SetFovy(camera->getFovy());
    camera->getTargetPos(&mCam->mCameraLookPos);
    _3c = 0;
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
}

void KartDemoCam::DoStartFixPathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f dir;
    f32 scale;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = crsData->getCameraData(mStartId);
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 1);
    up.set(0.0f, 1.0f, 0.0f);
    tmp.set(point->pos);
    dir.sub(tmp, mCam->mCameraPos);
    if (!DoPathEnd(&dir, &scale)) {
        dir.normalize();
        dir.scale(scale);
        mCam->mCameraPos.add(dir);
        ChaseFovy(camera);
        C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    }
}

void KartDemoCam::InitStartPathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = crsData->getCameraData(mStartId);
    _3c = 0;
    crsData->getPathData(camera->getPathID());
    mCam->mCameraPos.set(crsData->getPointData(camera->getPathID(), 0)->pos);
    up.set(0.0f, 1.0f, 0.0f);
    mCam->SetFovy(camera->getFovy());
    camera->getTargetPos(&mCam->mCameraLookPos);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
}

void KartDemoCam::DoStartPathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f dir;
    f32 scale;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = crsData->getCameraData(mStartId);
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 1);
    up.set(0.0f, 1.0f, 0.0f);
    tmp.set(point->pos);
    dir.sub(tmp, mCam->mCameraPos);
    if (!DoPathEnd(&dir, &scale)) {
        dir.normalize();
        dir.scale(scale);
        mCam->mCameraPos.add(dir);
        camera->getTargetPos2(&dir);
        dir.sub(dir, mCam->mCameraLookPos);
        scale = 0.01f * dir.length();
        dir.normalize();
        dir.scale(scale);
        mCam->mCameraLookPos.add(dir);
        ChaseFovy(camera);
        C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    }
}

void KartDemoCam::InitStartLookPathCam() {
    JGeometry::TVec3f up;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    _3c = 0;
    camera->getPosition(&mCam->mCameraPos);
    up.set(0.0f, 1.0f, 0.0f);
    mCam->SetFovy(camera->getFovy());
    camera->getTargetPos(&mCam->mCameraLookPos);
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
}

void KartDemoCam::DoStartLookPathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f dir;
    f32 scale;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    up.set(0.0f, 1.0f, 0.0f);
    camera->getTargetPos2(&dir);
    dir.sub(dir, mCam->mCameraLookPos);
    if (!DoPathEnd(&dir, &scale)) {
        dir.normalize();
        dir.scale(scale);
        mCam->mCameraLookPos.add(dir);
        ChaseFovy(camera);
        C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    }
}

void KartDemoCam::InitStartOnlyPathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f front;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = crsData->getCameraData(mStartId);
    _3c = 0;
    crsData->getPathData(camera->getPathID());
    mCam->mCameraPos.set(crsData->getPointData(camera->getPathID(), 0)->pos);
    up.set(0.0f, 1.0f, 0.0f);
    mCam->SetFovy(camera->getFovy());
    camera->getFrontDirection(&front);
    mCam->mCameraLookPos.x = mCam->mCameraPos.x + 10.0f * front.x;
    mCam->mCameraLookPos.y = mCam->mCameraPos.y + 10.0f * front.y;
    mCam->mCameraLookPos.z = mCam->mCameraPos.z + 10.0f * front.z;
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
}

void KartDemoCam::DoStartOnlyPathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f front;
    f32 scale;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = crsData->getCameraData(mStartId);
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 1);
    up.set(0.0f, 1.0f, 0.0f);
    tmp.set(point->pos);
    dir.sub(tmp, mCam->mCameraPos);
    if (!DoPathEnd(&dir, &scale)) {
        dir.normalize();
        dir.scale(scale);
        mCam->mCameraPos.add(dir);
        camera->getFrontDirection(&front);
        mCam->mCameraLookPos.x = mCam->mCameraPos.x + 10.0f * front.x;
        mCam->mCameraLookPos.y = mCam->mCameraPos.y + 10.0f * front.y;
        mCam->mCameraLookPos.z = mCam->mCameraPos.z + 10.0f * front.z;
        ChaseFovy(camera);
        C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    }
}

void KartDemoCam::InitStartFixCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f pos;
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    _3c = 0;
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

void KartDemoCam::DoStartFixCam() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    JGeometry::TVec3f dir;
    f32 scale;
    dir.zero();
    scale = 0.0f;
    if (!DoPathEnd(&dir, &scale)) {
        JGeometry::TVec3f up;
        ChaseFovy(camera);
        up.set(0.0f, 1.0f, 0.0f);
        C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
        mCam->DegubCamera("DoFixCam", 32);
    }
}

void KartDemoCam::InitStartChasePathCam() {
    JGeometry::TVec3f up;
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    crsData->getPathData(camera->getPathID());
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 0);
    mCam->mCameraPos.set(point->pos);
    up.set(0.0f, 1.0f, 0.0f);
    mCam->mCameraLookPos.set(mBody->mPlayerPosMtx[0][3],
                             mBody->mPlayerPosMtx[1][3] + 30.0f,
                             mBody->mPlayerPosMtx[2][3]);
    mCam->SetFovy(camera->getFovy());
    C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
    mCam->DegubCamera("InitChasePathCam", 41);
}

void KartDemoCam::DoStartChasePathCam() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f dir2;
    JGeometry::TVec3f target;
    JGeometry::TVec3f dir;
    f32 scale;
    up.set(0.0f, 1.0f, 0.0f);
    CrsData *crsData = RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    CrsData::PointData *point = crsData->getPointData(camera->getPathID(), 1);
    tmp.set(point->pos);
    dir.sub(tmp, mCam->mCameraPos);
    int speed = camera->getPathSpeed();
    if (speed == 0) {
        speed = 1;
    }
    if (speed > 100) {
        speed = 100;
    }
    scale = speed * 0.0005f * dir.length();
    dir.normalize();
    dir.scale(scale);
    if (!DoPathEnd(&dir, &scale)) {
        mCam->mCameraPos.add(dir);
        target.set(mBody->mPlayerPosMtx[0][3],
                   mBody->mPlayerPosMtx[1][3] + 30.0f,
                   mBody->mPlayerPosMtx[2][3]);
        dir2.sub(target, mCam->mCameraLookPos);
        scale = dir2.length();
        dir2.normalize();
        scale *= 0.6f;
        dir2.scale(scale);
        mCam->mCameraLookPos.add(dir2);
        ChaseFovy(camera);
        C_MTXLookAt(mCam->mMtx, (VecPtr)&mCam->mCameraPos, (VecPtr)&up, (VecPtr)&mCam->mCameraLookPos);
        mCam->DegubCamera("DoChasePathCam", 42);
    }
}

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
    KartCam *cam = mCam;
    f32 yaw = std::atan2f(cam->mTargetBody->_308.x, cam->mTargetBody->_308.z) + 3.14f;
    GetKartCtrl()->PolarToCross((VecPtr)&cam->mCameraLookPos, (VecPtr)&cam->mCameraPos, cam->_18c, cam->_1ac, yaw);
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
    case 4: {
        GetKartCtrl()->ChaseFnumber(&mCam->_18c, mBody->_4e4, 0.03f);
        KartCam *cam = mCam;
        f32 yaw = std::atan2f(cam->mTargetBody->_308.x, cam->mTargetBody->_308.z) + 3.14f;
        GetKartCtrl()->PolarToCross((VecPtr)&cam->mCameraLookPos, (VecPtr)&cam->mCameraPos, cam->_18c, cam->_1ac, yaw);
        if (RaceMgr::getCurrentManager()->isAbleStart()
            || (mBody->mCarStatus & 0x8000ULL) != 0
            || GetKartCtrl()->GetCarSpeed(mBody->mMynum) >= 5.0f) {
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
    KartCam *cam;
    JGeometry::TVec3f *look;
    TJugem *jugem = GetGeoObjMgr()->getJugem(mCam->mCamNo);
    mCam->_18c *= 0.95f;
    mCam->_1ac *= 0.15f;
    cam = mCam;
    f32 yaw = std::atan2f(cam->mTargetBody->_308.x, cam->mTargetBody->_308.z) + 3.14f;
    GetKartCtrl()->PolarToCross((VecPtr)&cam->mCameraLookPos, (VecPtr)&cam->mCameraPos, cam->_18c, cam->_1ac, yaw);
    mCamKartDiff.set(mCam->mCameraPos);
    _a4.set(mCam->mCameraLookPos);
    mCam->mCameraLookPos.set(jugem->getPosition());
    cam = mCam;
    look = &cam->mCameraLookPos;
    yaw = std::atan2f(cam->mTargetBody->_308.x, cam->mTargetBody->_308.z) + 3.14f;
    GetKartCtrl()->PolarToCross((VecPtr)look, (VecPtr)look, 80.0f, 0.0f, yaw);
    cam = mCam;
    GetKartCtrl()->PolarToCross((VecPtr)&cam->mCameraLookPos, (VecPtr)&cam->mCameraPos, 200.0f, -0.523333f, std::atan2f(cam->mTargetBody->_308.x, cam->mTargetBody->_308.z) + 2.09333f);
    mCam->mPathData[0].set(mCam->mCameraPos);
    cam = mCam;
    GetKartCtrl()->PolarToCross((VecPtr)&_a4, (VecPtr)&cam->mPathData[1], 1.3f * cam->mTargetBody->_4e4, 0.0f, std::atan2f(cam->mTargetBody->_308.x, cam->mTargetBody->_308.z) + 1.046666f);
    cam = mCam;
    yaw = std::atan2f(cam->mTargetBody->_308.x, cam->mTargetBody->_308.z) + 1.91888f;
    GetKartCtrl()->PolarToCross((VecPtr)&_a4, (VecPtr)&cam->mPathData[2], cam->mTargetBody->_4e4, 0.0f, yaw);
    mCam->mPathData[3].set(mCamKartDiff);
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
        diff.sub(_a4, mCam->mCameraLookPos);
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
        if ((mCam->mTargetBody->mGameStatus & 4) != 0 && mCam->_1b0 >= 0.9999f && len < 0.01f) {
            _3a = 5;
            _3c = 0;
        }
        break;
    }
    case 5: {
        mCam->DoLookChase();
        f32 angle = 3.141f * mBody->_4e8 / 180.0f;
        KartCam *cam = mCam;
        angle += 3.141f * cam->_180 / 180.0f;
        GetKartCtrl()->ChaseFnumber(&cam->_1ac, angle, 0.04f);
        GetKartCtrl()->ChaseFnumber(&mCam->_18c, mBody->_4e4, 0.02f);
        cam = mCam;
        f32 yaw = std::atan2f(cam->mTargetBody->_308.x, cam->mTargetBody->_308.z) + 3.14f;
        GetKartCtrl()->PolarToCross((VecPtr)&cam->mCameraLookPos, (VecPtr)&_74, cam->_18c, cam->_1ac, yaw);
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
        if ((mBody->mCarStatus & 0x8000ULL) != 0
            || mCam->_1ac >= angle - 0.01f
            || GetKartCtrl()->GetCarSpeed(mBody->mMynum) >= 5.0f) {
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

void KartDemoCam::InitStartDemoCam() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    camera->getKind();
    switch (camera->getKind()) {
    case 4:
        InitStartFixPathCam();
        mCam->DegubCamera("InitStartFixPathCam", 19);
        break;
    case 5:
        InitStartPathCam();
        mCam->DegubCamera("InitStartPathCam", 20);
        break;
    case 6:
        InitStartLookPathCam();
        mCam->DegubCamera("InitStartLookPathCam", 21);
        break;
    case 1:
        InitStartOnlyPathCam();
        mCam->DegubCamera("InitStartOnlyPathCam", 22);
        break;
    }
}

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

bool KartDemoCam::GetStartID() {
    mStartId = RaceMgr::getManager()->getCourse()->getCrsData()->getStartCameraID();
    if (mCam->mMode == 2) {
        return true;
    }
    return mStartId != -1;
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

void KartDemoCam::InitStartMapCam() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    camera->getKind();
    switch (camera->getKind()) {
    case 4:
        InitStartFixPathCam();
        mCam->DegubCamera("InitStartFixPathCam", 11);
        break;
    case 5:
        InitStartPathCam();
        mCam->DegubCamera("InitStartPathCam", 12);
        break;
    case 6:
        InitStartLookPathCam();
        mCam->DegubCamera("InitStartLookPathCam", 13);
        break;
    case 1:
        InitStartOnlyPathCam();
        mCam->DegubCamera("InitStartOnlyPathCam", 14);
        break;
    }
}

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

void KartDemoCam::InitStaffView() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    camera->getKind();
    switch (camera->getKind()) {
    case 0:
        InitStartFixCam();
        break;
    case 4:
        InitStartFixPathCam();
        mCam->DegubCamera("InitStartFixPathCam", 11);
        break;
    case 5:
        InitStartPathCam();
        mCam->DegubCamera("InitStartPathCam", 12);
        break;
    case 6:
        InitStartLookPathCam();
        mCam->DegubCamera("InitStartLookPathCam", 13);
        break;
    case 1:
        InitStartOnlyPathCam();
        mCam->DegubCamera("InitStartOnlyPathCam", 14);
        break;
    case 0x101:
        InitStartChasePathCam();
        break;
    }
}

void KartDemoCam::StaffView() {
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    if (!(_38 & 0x400) && RaceMgr::getCurrentManager()->requestDemoCameraID() >= 0) {
        mStartId = RaceMgr::getManager()->requestDemoCameraID();
        _38 |= 0x400;
        InitStaffView();
        return;
    }
    if (RaceMgr::getCurrentManager()->requestDemoCameraID() >= 0) {
        camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    }
    switch (camera->getKind()) {
    case 0:
        DoStartFixCam();
        break;
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
    case 0x101:
        DoStartChasePathCam();
        break;
    }
}

bool KartDemoCam::SetStaffNextID() {
    RaceMgr::getManager()->getCourse()->getCrsData();
    CrsData::Camera *camera = RaceMgr::getManager()->getCourse()->getCrsData()->getCameraData(mStartId);
    if (camera->getNextCameraID() < 0) {
        return false;
    }
    mStartId = camera->getNextCameraID();
    InitStaffView();
    return true;
}

void KartDemoCam::SetUpStaffID() {
    mStartId = RaceMgr::getManager()->getCourse()->getCrsData()->getStartCameraID();
    _38 |= 0x200;
}
