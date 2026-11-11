#define JGEOM_NO_TVEC3_SET
#include <JSystem/JGeometry/Vec.h>

namespace JGeometry {
template <> template <> inline void TVec3f::set(f32 X, f32 Y, f32 Z) { x = X; y = Y; z = Z; }
}

#undef JGEOM_NO_TVEC3_SET

#include "Yamamoto/kartCamera.h"

#include "dolphin/gx.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "JSystem/JMath/JMath.h"
#include "Kaneshige/KartMtx.h"
#include "Kaneshige/RaceMgr.h"
#include "Yamamoto/kartParams.h"
#include "Osako/system.h"

#include "dolphin/mtx.h"
#include <std/math.h>


// Unused function calls set from TVec3f
// comments inside functions are inline functions being called in that function

void KartCam::SetMTXLookAt(JGeometry::TVec3f *pos, JGeometry::TVec3f *up, JGeometry::TVec3f *look) {
    C_MTXLookAt(mMtx, (VecPtr)pos, (VecPtr)up, (VecPtr)look);
    SetCameraPos(pos);
    SetCameraLookPos(look);
}

void KartCam::SetCameraLookPos(JGeometry::TVec3f *pos) {
    mCameraLookPos.set(pos->x, pos->y, pos->z);
}

void KartCam::SetCameraPos(JGeometry::TVec3f *pos) {
    mCameraPos.set(pos->x, pos->y, pos->z);
}

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
    int viewIdx;
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    int camNum = mgr->getCameraNumber();
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
    ECharID driver0 = mgr->getKartInfo(mCamNo)->getDriverCharID(0);
    ECharID driver1 = mgr->getKartInfo(mCamNo)->getDriverCharID(1);
    if (driver0 == PETEY || driver1 == PETEY) {
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

void KartCam::First4ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 3.75f, 3.75f);
        if (mWidth == (f32)System::get3DScisW() && mHeight == (f32)System::get3DScisH()) {
            GetKartCtrl()->mBitfield |= 4;
        }
    }
    if (idx == 2) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 0.01f, 3.75f, 3.75f);
        f32 h = (f32)System::get3DScisDiv4H(0) - mHeight;
        f32 vh = (f32)System::get3DVpDiv4H(0) - mVpHeight;
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) + h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) + vh);
    }
    if (idx == 1) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 3.75f, 3.75f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 0.01f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 3.75f, 3.75f);
        f32 w = (f32)System::get3DScisDiv4W(0) - mWidth;
        f32 vw = (f32)System::get3DVpDiv4W(0) - mVpWidth;
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) + w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) + vw);
    }
    if (idx == 3) {
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 0.01f, 3.75f, 3.75f);
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 0.01f, 5.0f, 5.0f);
        f32 h = (f32)System::get3DScisDiv4H(0) - mHeight;
        f32 vh = (f32)System::get3DVpDiv4H(0) - mVpHeight;
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) + h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) + vh);
        f32 w = (f32)System::get3DScisDiv4W(0) - mWidth;
        f32 vw = (f32)System::get3DVpDiv4W(0) - mVpWidth;
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) + w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) + vw);
    }
    if (mHeight == 0.0f || mWidth == 0.0f) {
        mgr->hideConsole(idx);
    }
    if (mWidth != 0.0f && mHeight != 0.0f) {
        mAspect = mWidth / mHeight;
    }
    if (mAspect > 3.0f) {
        mAspect = 3.0f;
    }
    if (mAspect < 0.1f) {
        mAspect = 0.1f;
    }
}

void KartCam::Second4ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 0.01f, 3.75f, 3.75f);
    }
    if (idx == 2) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 3.75f, 3.75f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 3.75f, 3.75f);
        f32 h = mHeight - (f32)System::get3DScisDiv4H(0);
        f32 vh = mVpHeight - (f32)System::get3DVpDiv4H(0);
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) - h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) - vh);
        if (mWidth == (f32)System::get3DScisW() && mHeight == (f32)System::get3DScisH()) {
            GetKartCtrl()->mBitfield |= 4;
        }
    }
    if (idx == 1) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 0.01f, 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 0.01f, 3.75f, 3.75f);
        f32 w = (f32)System::get3DScisDiv4W(0) - mWidth;
        f32 vw = (f32)System::get3DVpDiv4W(0) - mVpWidth;
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) + w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) + vw);
    }
    if (idx == 3) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 0.01f, 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 3.75f, 3.75f);
        f32 w = (f32)System::get3DScisDiv4W(0) - mWidth;
        f32 vw = (f32)System::get3DVpDiv4W(0) - mVpWidth;
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) + w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) + vw);
        f32 h = mHeight - (f32)System::get3DScisDiv4H(0);
        f32 vh = mVpHeight - (f32)System::get3DVpDiv4H(0);
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) - h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) - vh);
    }
    if (mHeight == 0.0f || mWidth == 0.0f) {
        mgr->hideConsole(idx);
    }
    if (mWidth != 0.0f && mHeight != 0.0f) {
        mAspect = mWidth / mHeight;
    }
    if (mAspect > 3.0f) {
        mAspect = 3.0f;
    }
    if (mAspect < 0.1f) {
        mAspect = 0.1f;
    }
}

void KartCam::Third4ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 0.01f, 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 3.75f, 3.75f);
    }
    if (idx == 2) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 0.01f, 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 0.01f, 3.75f, 3.75f);
        f32 h = (f32)System::get3DScisDiv4H(0) - mHeight;
        f32 vh = (f32)System::get3DVpDiv4H(0) - mVpHeight;
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) + h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) + vh);
    }
    if (idx == 1) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 3.75f, 3.75f);
        f32 w = mWidth - (f32)System::get3DScisDiv4W(0);
        f32 vw = mVpWidth - (f32)System::get3DVpDiv4W(0);
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) - w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) - vw);
        if (mWidth == (f32)System::get3DScisW() && mHeight == (f32)System::get3DScisH()) {
            GetKartCtrl()->mBitfield |= 4;
        }
    }
    if (idx == 3) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 0.01f, 3.75f, 3.75f);
        f32 h = (f32)System::get3DScisDiv4H(0) - mHeight;
        f32 vh = (f32)System::get3DVpDiv4H(0) - mVpHeight;
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) + h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) + vh);
        f32 w = mWidth - (f32)System::get3DScisDiv4W(0);
        f32 vw = mVpWidth - (f32)System::get3DVpDiv4W(0);
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) - w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) - vw);
    }
    if (mHeight == 0.0f || mWidth == 0.0f) {
        mgr->hideConsole(idx);
    }
    if (mWidth != 0.0f && mHeight != 0.0f) {
        mAspect = mWidth / mHeight;
    }
    if (mAspect > 3.0f) {
        mAspect = 3.0f;
    }
    if (mAspect < 0.1f) {
        mAspect = 0.1f;
    }
}

void KartCam::Fourth4ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 3.75f, 3.75f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 0.01f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 0.01f, 3.75f, 3.75f);
    }
    if (idx == 2) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 3.75f, 3.75f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 0.0f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 3.75f, 3.75f);
        f32 h = mHeight - (f32)System::get3DScisDiv4H(0);
        f32 vh = mVpHeight - (f32)System::get3DVpDiv4H(0);
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) - h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) - vh);
    }
    if (idx == 1) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 3.75f, 3.75f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 0.01f, 3.75f, 3.75f);
        f32 w = mWidth - (f32)System::get3DScisDiv4W(0);
        f32 vw = mVpWidth - (f32)System::get3DVpDiv4W(0);
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) - w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) - vw);
    }
    if (idx == 3) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 3.75f, 3.75f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 3.75f, 3.75f);
        f32 h = mHeight - (f32)System::get3DScisDiv4H(0);
        f32 vh = mVpHeight - (f32)System::get3DVpDiv4H(0);
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) - h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) - vh);
        f32 w = mWidth - (f32)System::get3DScisDiv4W(0);
        f32 vw = mVpWidth - (f32)System::get3DVpDiv4W(0);
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) - w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) - vw);
        if (mWidth == (f32)System::get3DScisW() && mHeight == (f32)System::get3DScisH()) {
            GetKartCtrl()->mBitfield |= 4;
        }
    }
    if (mHeight == 0.0f || mWidth == 0.0f) {
        mgr->hideConsole(idx);
    }
    if (mWidth != 0.0f && mHeight != 0.0f) {
        mAspect = mWidth / mHeight;
    }
    if (mAspect > 3.0f) {
        mAspect = 3.0f;
    }
    if (mAspect < 0.1f) {
        mAspect = 0.1f;
    }
}

void KartCam::First2ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 5.0f, 5.0f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 3.75f, 3.75f);
        if (mWidth == (f32)System::get3DScisW() && mHeight == (f32)System::get3DScisH()) {
            GetKartCtrl()->mBitfield |= 4;
        }
    }
    if (idx == 1) {
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 0.01f, 3.75f, 3.75f);
        f32 h = (f32)System::get3DScisDiv2H(0) - mHeight;
        f32 vh = (f32)System::get3DVpDiv2H(0) - mVpHeight;
        mPosH = (int)((f32)System::get3DScisDiv2Y(idx) + h);
        mVpPosH = (int)((f32)System::get3DVpDiv2Y(idx) + vh);
    }
    if (mHeight == 0.0f || mWidth == 0.0f) {
        mgr->hideConsole(idx);
    }
    if (mWidth != 0.0f && mHeight != 0.0f) {
        mAspect = mWidth / mHeight;
    }
    if (mAspect > 3.0f) {
        mAspect = 3.0f;
    }
    if (mAspect < 0.1f) {
        mAspect = 0.1f;
    }
}

void KartCam::Second2ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 0.01f, 3.75f, 3.75f);
    }
    if (idx == 1) {
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 3.75f, 3.75f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 3.75f, 3.75f);
        f32 h = mHeight - (f32)System::get3DScisDiv4H(0);
        f32 vh = mVpHeight - (f32)System::get3DVpDiv4H(0);
        mPosH = (int)((f32)System::get3DScisDiv2Y(idx) - h);
        mVpPosH = (int)((f32)System::get3DVpDiv2Y(idx) - vh);
        if (mWidth == (f32)System::get3DScisW() && mHeight == (f32)System::get3DScisH()) {
            GetKartCtrl()->mBitfield |= 4;
        }
    }
    if (mHeight == 0.0f || mWidth == 0.0f) {
        mgr->hideConsole(idx);
    }
    if (mWidth != 0.0f && mHeight != 0.0f) {
        mAspect = mWidth / mHeight;
    }
    if (mAspect > 3.0f) {
        mAspect = 3.0f;
    }
    if (mAspect < 0.1f) {
        mAspect = 0.1f;
    }
}

void KartCam::SetVictoryScreenPort(unsigned char idx) {
    s16 zoom;
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    int camNum = mgr->getCameraNumber();
    if (camNum == 1 || !mgr->isZoomWinConsole()) {
        return;
    }
    zoom = mgr->getZoomWinConsoleNo();
    int mode = mgr->getRaceMode();
    if (mode >= 8 || mode < 2) {
        return;
    }
    if (camNum == 2) {
        if (zoom == 0) {
            First2ScreenPort(idx);
        }
        if (zoom == 1) {
            Second2ScreenPort(idx);
        }
    } else {
        if (zoom == 0) {
            First4ScreenPort(idx);
        }
        if (zoom == 2) {
            Second4ScreenPort(idx);
        }
        if (zoom == 1) {
            Third4ScreenPort(idx);
        }
        if (zoom == 3) {
            Fourth4ScreenPort(idx);
        }
    }
}

inline s16 RaceMgr::getZoomWinConsoleNo() const { return mRaceDirector->getZoomWinConsoleNo(); }

inline s16 RaceDirector::getZoomWinConsoleNo() const { return mZoomWinCnsNo; }

inline bool RaceMgr::isZoomWinConsole() const { return mRaceDirector->isZoomWinConsole(); }

void KartCam::DoMoveCamera(JGeometry::TVec3<float> *target, JGeometry::TVec3<float> *pos) {
    JGeometry::TVec3f d;
    d.sub(*target, *pos);
    f32 len = d.length();
    KartBody *body = mTargetBody;
    f32 move = 0.0f;
    if (body->mCarStatus & 0x500800ull) {
        _164 = 0.0f;
    } else if (mPtr == 7) {
        GetKartCtrl()->ChaseFnumber(&_164, 1.0f, 0.6f);
        move = len * _164;
    } else if (body->mCarStatus & 3ull) {
        if (body->mSterrNorm > 0.0174444f) {
            GetKartCtrl()->ChaseFnumber(&_164, 0.15f, 0.05f);
        } else {
            GetKartCtrl()->ChaseFnumber(&_164, 0.3f, 0.02f);
        }
        move = len * _164;
    } else if (body->mCarStatus & 4ull) {
        GetKartCtrl()->ChaseFnumber(&_164, 0.65f, 0.3f);
        move = len * _164;
    } else if (len != move) {
        if (body->_458 <= 30.0f) {
            GetKartCtrl()->ChaseFnumber(&_164, 0.1f, 0.05f);
            move = len * _164;
        } else if (len > 130.0f) {
            GetKartCtrl()->ChaseFnumber(&_164, body->_4ec, 0.0052f);
            move = len * _164;
        } else {
            GetKartCtrl()->ChaseFnumber(&_164, 0.0f, 0.0052f);
            move = len * _164;
        }
    }
    if (move) {
        d.normalize();
        d.scale(move);
        pos->add(d);
    }
}

void KartCam::DoRoof(JGeometry::TVec3<float> *pos, CrsArea *area) {
    JGeometry::TVec3f a;
    JGeometry::TVec3f b;
    if (area->isInside() == 1) {
        area->getRoofPosition(&a);
        f32 d = pos->y - (a.y - 80.0f);
        if (d > 0.0f) {
            mFlags |= 4;
            d = 0.3f * d;
            b.set(0.0f, -1.0f, 0.0f);
            b.scale(d);
            pos->add(b);
        }
    }
}

void KartCam::DoColCamera() {
    JGeometry::TVec3f a, b, f, e, d, c, g, h;
    f32 len = _1c4 + (_1a4 + (_1ac + _184));
    if (len >= 1.55305f) {
        len = 1.55305f;
    }
    if (len < 0.0) {
        len = 0.0f;
    }
    GetKartCtrl()->PolarToCross((VecPtr)&mCameraLookPos, (VecPtr)&e, _18c, len, _15c);
    DoMoveCamera(&e, &_20c);
    f.sub(mCameraLookPos, _20c);
    f.normalize();
    d.set(f.x * _18c, f.y * _18c, f.z * _18c);
    _20c.sub(mCameraLookPos, d);
    _200.set(_20c);
    mCameraPos.set(_200.x, _200.y, _200.z);
}

void KartCam::DoSea(JGeometry::TVec3<float> *pos, CrsGround *ground) {
    JGeometry::TVec3f normal;
    normal.set(0.0f, 1.0f, 0.0f);
    f32 height = ground->getWaterHeight();
    f32 y = pos->y - 80.0f;
    JGeometry::TVec3f top;
    JGeometry::TVec3f d;
    top.set(pos->x, 300.0f + height, pos->z);
    d.sub(top, mCameraLookPos);
    f32 diff = d.length() - _18c;
    if (pos->y - 200.0f <= height || (mFlags & 2)) {
        if (diff < 0.0f) {
            GetKartCtrl()->ChaseFnumber(&_1c0, 0.0f, 0.01f);
        } else {
            GetKartCtrl()->ChaseFnumber(&_1c0, diff, 0.3f);
        }
    } else {
        GetKartCtrl()->ChaseFnumber(&_1c0, 0.0f, 0.01f);
    }
    if (y <= height) {
        JGeometry::TVec3f v;
        f32 dist;
        f32 angleY;
        f32 angleX;
        normal.scale(3.0f * (-y + height));
        v.sub(*pos, normal);
        GetKartCtrl()->CrossToPolar(&mCameraLookPos, &v, &dist, &angleX, &angleY);
        GetKartCtrl()->ChaseFnumber(&_1a4, angleX, 0.01f);
        if (_1a4 < 0.0f) {
            _1a4 = 0.0f;
        }
        mFlags |= 2;
        DoColCamera();
    } else if (mFlags & 2) {
        if (pos->y - 180.0f > height) {
            mFlags &= ~2;
        } else {
            DoColCamera();
        }
    } else {
        mFlags &= ~2;
        GetKartCtrl()->ChaseFnumber(&_1a4, 0.0f, 0.01f);
    }
}

void KartCam::DoGround(JGeometry::TVec3<float> *pos, CrsGround *ground) {
    JGeometry::TVec3f normal;
    JGeometry::TVec3f unused;
    _1c0 = 0.0f;
    ground->getNormal(&normal);
    f32 height = ground->getHeight();
    f32 y = pos->y - 80.0f;
    if (y <= height) {
        JGeometry::TVec3f v;
        f32 dist;
        f32 angleY;
        f32 angleX;
        if (mTargetBody->mCarStatus & 0x80000000000ull) {
            normal.scale(9.0f * (-y + height));
            v.add(*pos, normal);
            _200.y = v.y;
            _1dc.y = _200.y;
            mCameraPos.set(_200);
        } else {
            normal.scale(9.0f * (-y + height));
            v.sub(*pos, normal);
            GetKartCtrl()->CrossToPolar(&mCameraLookPos, &v, &dist, &angleX, &angleY);
            GetKartCtrl()->ChaseFnumber(&_1a4, angleX, 0.1f);
            if (_1a4 < 0.0f) {
                _1a4 = 0.0f;
            }
            mFlags |= 2;
            DoColCamera();
        }
    } else if ((mFlags & 2) && !(mTargetBody->mCarStatus & 0x80000000000ull)) {
        if (pos->y - 230.0f > height) {
            mFlags &= ~2;
        } else {
            DoColCamera();
        }
    } else {
        mFlags &= ~2;
        GetKartCtrl()->ChaseFnumber(&_1a4, 0.0f, 0.08f);
    }
}

void KartCam::GroundCheck(JGeometry::TVec3<float> *pos, JGeometry::TVec3<float> *prev, CrsArea *area) {
    CrsGround ground;
    JGeometry::TVec3f p;
    if (mTargetBody->mBodyGround.getAttribute() == 15 && mTargetBody->mBodyGround.getSplashHeight() > 0.0f &&
        mTargetBody->mPlayerPosMtx[1][3] - mTargetBody->mBodyGround.getSplashHeight() < -165.0f) {
        return;
    }
    if ((mTargetBody->mCarStatus & 0x400000) && mTargetBody->getRescue()->_75 == 5) {
        return;
    }
    if (!(mTargetBody->mCarStatus & 0x8100000000ull)) {
        p.set(*pos);
        mFlags &= ~4;
        ground.search(*pos, *prev);
        if (ground.getAttribute() == 10) {
            return;
        }
        if (ground.getCameraCode() == 4) {
            mFlags &= ~2;
            GetKartCtrl()->ChaseFnumber(&_1a4, 0.0f, 0.1f);
            return;
        }
        if (ground.getAttribute() != 2 && !ground.isObject() && ground.getAttribute() != 18) {
            if (ground.getAttribute() == 17) {
                DoSea(&p, &ground);
            } else {
                DoGround(&p, &ground);
            }
        }
    }
    DoRoof(pos, area);
}

void KartCam::OutViewCalc() {
    JGeometry::TVec3f look;
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f pos;
    SetFovyData();
    _1c0 = 0.0f;
    _e6 = 0;
    _1c4 = 0.0f;
    _1c8 = 0.0f;
    _1a0 = 0.0f;
    pos.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    mCameraLookPos.sub(look, pos);
    _1ac = 3.141f * mTargetBody->_4e8 / 180.0f;
    _1ac = _1ac + 3.141f * _180 / 180.0f;
    _18c = mTargetBody->_4e4;
    f32 yaw = std::atan2f(mTargetBody->_308.x, mTargetBody->_308.z) + _1b8;
    GetKartCtrl()->PolarToCross((VecPtr)&mCameraLookPos, (VecPtr)&_200, mTargetBody->_4e4, _1ac, yaw);
    up.set(0.0f, 1.0f, 0.0f);
    mCameraPos.set(_200);
    _20c.set(_200);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
}

void KartCam::InitOutView() {
    _184 = 0.0f;
    _1a4 = 0.0f;
    _164 = 0.0f;
    _1b8 = 3.14f;
    OutViewCalc();
    InitRaceBackView();
    mFlags &= ~0x20;
}

void KartCam::DoChangFovy() {
    if (mPtr == 7) {
        return;
    }
    if ((mTargetBody->mCarStatus & 0x100000000ull) && mTargetBody->getCannon()->mTimer != 0) {
        if (GetKartCtrl()->GetCarSpeed(mCamNo) >= 5.0f) {
            GetKartCtrl()->ChaseFnumber(&mFovY, 1.3f * _1bc, 0.3f);
        }
        _14c = 0.0f;
    } else if (mTargetBody->mCarStatus & 0x400210a4000ull) {
        GetKartCtrl()->ChaseFnumber(&mFovY, 1.2f * _1bc, 0.33f);
        _14c = 0.0f;
    } else if (mTargetBody->mCarStatus & 0x10000ull) {
        if (GetKartCtrl()->GetCarSpeed(mCamNo) >= 10.0f) {
            GetKartCtrl()->ChaseFnumber(&mFovY, 1.3f * _1bc, 0.4f);
        }
        _14c = 0.0f;
    } else {
        GetKartCtrl()->ChaseFnumber(&_14c, 0.05f, 0.05f);
        GetKartCtrl()->ChaseFnumber(&mFovY, _1bc, _14c);
    }
}

void KartCam::DoLookChase() {
    JGeometry::TVec3f c;
    JGeometry::TVec3f b;
    JGeometry::TVec3f a;
    a.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    c.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    b.sub(c, a);
    if (mTargetBody->getRescue()->_75 != 4 || (mTargetBody->mCarStatus & 0x400000) == 0) {
        GetKartCtrl()->ChaseFnumber(&mCameraLookPos.y, b.y, 0.89f);
    }
    mCameraLookPos.x = b.x;
    mCameraLookPos.z = b.z;
}

void KartCam::DoYRotation() {
    if (mTargetBody->getRescue()->_75 == 4 && (mTargetBody->mCarStatus & 0x400000)) {
        return;
    }
    if (!(mTargetBody->mCarStatus & 0x800800000ull) && !(mTargetBody->getDossin()->_14 & 3)) {
        if (!(mTargetBody->mCarStatus & 0x80000000000ull)) {
            _15c = _1b8 + std::atan2f(mTargetBody->_308.x, mTargetBody->_308.z);
            _15c = _15c + _1d0;
        }
        _e6 = 0;
        if (_e6 == 1) {
            _e6 = 2;
        }
    } else {
        _e6 = 1;
    }
    if (_e6 == 2) {
        JGeometry::TVec3f d;
        d.sub(mCameraLookPos, _200);
        _1c4 = 3.141f * std::atan2f(GetKartCtrl()->SpeedySqrtf(d.x * d.x + d.z * d.z), d.y) / 180.0f;
        _1c4 = _1c4 - (_1a4 + (_1ac + _184));
        _1c8 = d.length();
        _1c8 = _1c8 - (mTargetBody->_4e4 + _1c0);
        _e6 = 0;
    }
}

f32 KartCam::DoXRotation() {
    f32 target = 3.141f * mTargetBody->_4e8 / 180.0f;
    target += 3.141f * _180 / 180.0f;
    if (mTargetBody->getRescue()->_75 == 4 && (mTargetBody->mCarStatus & 0x400000)) {
        return _1c4 + (_1ac + _184);
    }
    if (mTargetBody->mCarStatus & 0x8100000000ull) {
        if (mTargetBody->mBodyGround.getCameraCode() == 3) {
            GetKartCtrl()->ChaseFnumber(&_1ac, -1.046666f, 0.02f);
        } else {
            GetKartCtrl()->ChaseFnumber(&_1ac, -0.1046666f, 0.02f);
        }
    } else {
        GetKartCtrl()->ChaseFnumber(&_1ac, target, 0.01f);
    }
    GetKartCtrl()->ChaseFnumber(&_1c4, 0.0f, 0.05f);
    if (((mTargetBody->mCarStatus & 0x400000) && mTargetBody->getTouchNum() == 0) ||
        ((mTargetBody->mCarStatus & 0x40) && !(mTargetBody->mCarStatus & 0x100000000ull))) {
        GetKartCtrl()->ChaseFnumber(&_184, 0.348888f + _1a4, 0.03f);
        _1a0 = 0.2f;
        _1a8 = 0.0f;
    } else if ((mTargetBody->mCarStatus & 0x100100000ull) || mTargetBody->mBodyGround.getCameraCode() == 0 ||
               mTargetBody->_32c.y > 0.99f || mTargetBody->mBodyGround.getCameraCode() == 4) {
        GetKartCtrl()->ChaseFnumber(&_1a0, 0.2f, 0.01f);
        if (mTargetBody->mCarStatus & 0x100000000ull) {
            GetKartCtrl()->ChaseFnumber(&_184, 0.0f, _1a0);
        } else {
            GetKartCtrl()->ChaseFnumber(&_184, _1a4, _1a0);
        }
        _1a8 = 0.0f;
    } else {
        f32 v;
        f32 rate;
        switch (mTargetBody->mBodyGround.getCameraCode()) {
        case 1:
            _1a0 = 0.0f;
            v = mTargetBody->_464;
            rate = v;
            if (v < 0.0f) {
                rate = -v;
            }
            rate = rate / 0.174444f;
            if (rate > 1.0f) {
                rate = 1.0f;
            }
            if (rate < 0.1f) {
                rate = 0.0f;
            }
            rate = 0.7f * rate;
            _1a8 = -v * rate;
            GetKartCtrl()->ChaseFnumber(&_184, _1a8 + _1a4, 0.025f);
            break;
        case 2:
            _1a0 = 0.0f;
            v = mTargetBody->_464;
            rate = v;
            if (v < 0.0f) {
                rate = -v;
            }
            rate = rate / 0.0872222f;
            if (rate > 1.0f) {
                rate = 1.0f;
            }
            if (rate < 0.1f) {
                rate = 0.0f;
            }
            _1a8 = -v * rate;
            GetKartCtrl()->ChaseFnumber(&_184, _1a8 + _1a4 - _1ac, 0.05f);
            break;
        case 5:
            _1a0 = 0.0f;
            v = mTargetBody->_464;
            if (v < 0.0f) {
                _1a8 = 0.680333f;
            } else {
                rate = v;
                if (v < 0.0f) {
                    rate = -v;
                }
                rate = rate / 0.174444f;
                if (rate > 1.0f) {
                    rate = 1.0f;
                }
                if (rate < 0.1f) {
                    rate = 0.0f;
                }
                rate = 0.7f * rate;
                _1a8 = -v * rate;
            }
            GetKartCtrl()->ChaseFnumber(&_184, _1a8 + _1a4, 0.025f);
            break;
        default:
            _1a0 = 0.0f;
            v = mTargetBody->_464;
            rate = v;
            if (v < 0.0f) {
                rate = -v;
            }
            rate = rate / 0.174444f;
            if (rate > 1.0f) {
                rate = 1.0f;
            }
            if (rate < 0.1f) {
                rate = 0.0f;
            }
            rate = 0.7f * rate;
            _1a8 = -v * rate;
            GetKartCtrl()->ChaseFnumber(&_184, _1a8 + _1a4, 0.1f);
            break;
        }
    }
    f32 rot = _1c4 + (_1ac + _184);
    if (rot >= 1.55305f) {
        rot = 1.55305f;
    }
    if (mTargetBody->mCarStatus & 0x8100000000ull) {
        if (rot < -1.395555f) {
            rot = -1.395555f;
        }
    } else if (rot < -0.174444f) {
        rot = -0.174444f;
    }
    return rot;
}

void KartCam::DoDist() {
    f32 dist = _1c8 + (mTargetBody->_4e4 + _1c0);
    if (mTargetBody->getDossin()->_14 & 3) {
        GetKartCtrl()->ChaseFnumber(&_18c, 2.0f * mTargetBody->_4e4, 0.45f);
    } else if (mTargetBody->getRescue()->_75 == 4 && (mTargetBody->mCarStatus & 0x400000)) {
        GetKartCtrl()->ChaseFnumber(&_18c, 2.5f * mTargetBody->_4e4, 0.1f);
    } else if (mTargetBody->getRescue()->_75 == 1 && (mTargetBody->mCarStatus & 0x400000)) {
        GetKartCtrl()->ChaseFnumber(&_18c, 2.5f * mTargetBody->_4e4, 0.008f);
    } else if (mTargetBody->mCarStatus & 0x80000000000ull) {
        GetKartCtrl()->ChaseFnumber(&_18c, 2.5f * mTargetBody->_4e4, 0.45f);
    } else {
        GetKartCtrl()->ChaseFnumber(&_18c, dist, 0.1f);
    }
    GetKartCtrl()->ChaseFnumber(&_1c8, 0.0f, 0.05f);
}

void KartCam::DoCamPos(float rot, JGeometry::TVec3<float> *out) {
    JGeometry::TVec3f pos;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f d;
    if (mTargetBody->mBodyGround.getAttribute() == 15 && mTargetBody->mBodyGround.getSplashHeight() > 0.0f &&
        mTargetBody->mPlayerPosMtx[1][3] - mTargetBody->mBodyGround.getSplashHeight() < -165.0f) {
        return;
    }
    if ((mTargetBody->mCarStatus & 0x400000) && mTargetBody->getRescue()->_75 == 5) {
        return;
    }
    GetKartCtrl()->PolarToCross((VecPtr)&mCameraLookPos, (VecPtr)&pos, _18c, rot, _15c);
    if ((mTargetBody->mCarStatus & 0x80800800000ull) || (mTargetBody->getDossin()->_14 & 3)) {
        if (mTargetBody->mCarStatus & 0x800800000ull) {
            GetKartCtrl()->ChaseFnumber(&_1dc.y, pos.y, 0.02f);
        }
        pos.y = _1dc.y;
    } else {
        _1dc.set(_200);
        _200.y = pos.y;
    }
    DoMoveCamera(&pos, &_200);
    dir.sub(mCameraLookPos, _200);
    dir.normalize();
    d.set(dir.x * _18c, dir.y * _18c, dir.z * _18c);
    _200.sub(mCameraLookPos, d);
    if ((mTargetBody->mCarStatus & 0x80800800000ull) || (mTargetBody->getDossin()->_14 & 3)) {
        _200.set(pos);
    }
    mCameraPos.set(_200.x, _200.y, _200.z);
    out->set(dir);
}

void KartCam::OutView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f prev;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f shake;
    mArea.searchRoof(mCameraPos);
    up.set(0.0f, 1.0f, 0.0f);
    DoChangFovy();
    DoLookChase();
    DoYRotation();
    f32 rot = DoXRotation();
    DoDist();
    shake.zero();
    DoCamPos(rot, &shake);
    DoShaker(&mCameraLookPos, shake);
    prev.set(_20c);
    _20c.set(_200);
    GroundCheck(&_200, &prev, &mArea);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("OutView", 0);
}

void KartCam::InitRaceBackView() {
    JGeometry::TVec3f a;
    JGeometry::TVec3f b;
    _1cc = CameraOpData[mTargetBody->mIdx]->_8;
    a.set(0.0f, _17c + CameraOpData[mTargetBody->mIdx]->_0, 0.0f);
    b.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    _260.sub(b, a);
    f32 yaw = std::atan2f(mTargetBody->_308.x, mTargetBody->_308.z);
    GetKartCtrl()->PolarToCross((VecPtr)&_260, (VecPtr)&_254, CameraOpData[mTargetBody->mIdx]->_4, _1cc, yaw);
}

void KartCam::RaceBackView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f look;
    JGeometry::TVec3f base;
    base.set(0.0f, _17c + CameraOpData[mTargetBody->mIdx]->_0, 0.0f);
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    _260.sub(look, base);
    f32 yaw = std::atan2f(mTargetBody->_308.x, mTargetBody->_308.z);
    GetKartCtrl()->PolarToCross((VecPtr)&_260, (VecPtr)&_254, CameraOpData[mTargetBody->mIdx]->_4, _1cc, yaw);
    mCameraPos.set(_254);
    mCameraLookPos.set(_260);
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
}

void KartCam::InitParallelView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f look;
    JGeometry::TVec3f base;
    SetFovyData();
    _158 = 0.0f;
    base.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    mCameraLookPos.sub(look, base);
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("InitParallelView", 1);
}

void KartCam::ParallelView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f look;
    JGeometry::TVec3f off;
    JGeometry::TVec3f base;
    base.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    f32 angle = mTargetBody->_4e8 + _180;
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    mCameraLookPos.sub(look, base);
    GetKartCtrl()->PolarToCross((VecPtr)&mCameraLookPos, (VecPtr)&look, mTargetBody->_4e4, 3.141f * angle / 180.0f,
                                3.141f * _158 / 180.0f);
    up.set(0.0f, 1.0f, 0.0f);
    dir.sub(mCameraLookPos, look);
    f32 len = dir.length();
    f32 dist = mTargetBody->_4e4;
    off.set(dir.x / len, dir.y / len, dir.z / len);
    off.set(off.x * dist, off.y * dist, off.z * dist);
    mCameraPos.sub(mCameraLookPos, off);
    GetDemoCam()->DoCol();
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("ParallelView", 2);
}

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

void KartCam::InitHangRescueView() {
    _184 = 0.0f;
    _1a4 = 0.0f;
    _164 = 0.0f;
    _1c0 = 0.0f;
    _e6 = 0;
    SetFovyData();
    _218.z = mTargetBody->_4e4;
}

void KartCam::HangRescueView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f look;
    JGeometry::TVec3f target;
    JGeometry::TVec3f dir;
    CrsGround ground;
    up.set(0.0f, 1.0f, 0.0f);
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    target.sub(look, mTargetBody->_350);
    dir.sub(target, mCameraLookPos);
    f32 len = dir.length();
    dir.normalize();
    f32 move = 0.93f * len;
    mCameraLookPos.x += dir.x * move;
    mCameraLookPos.y += dir.y * move;
    mCameraLookPos.z += dir.z * move;
    GetKartCtrl()->ChaseFnumber(&_18c, 1.5f * _218.z, 0.01f);
    GetKartCtrl()->PolarToCross((VecPtr)&_200, (VecPtr)&mCameraPos, _18c, _1ac, _15c);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
}

void KartCam::InitDropRescueView() {
    JGeometry::TVec3f unused1;
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused2;
    _184 = 0.0f;
    _1a4 = 0.0f;
    _164 = 0.0f;
    _1c0 = 0.0f;
    _e6 = 0;
    SetFovyData();
    mCameraLookPos.set(mTargetBody->getRescue()->_a4);
    mCameraLookPos.x += 20.0f * mTargetBody->getRescue()->_d4.mMtx[0][2];
    mCameraLookPos.y += 20.0f * mTargetBody->getRescue()->_d4.mMtx[1][2];
    mCameraLookPos.z += 20.0f * mTargetBody->getRescue()->_d4.mMtx[2][2];
    mCameraLookPos.y += 100.0f;
    _218.y = -1.74444f;
    _218.x = 0.174444f;
    _218.z = 380.0f;
    f32 yaw = _218.y + std::atan2f(mTargetBody->getRescue()->_d4.mMtx[0][2], mTargetBody->getRescue()->_d4.mMtx[2][2]);
    GetKartCtrl()->PolarToCross((VecPtr)&mCameraLookPos, (VecPtr)&_200, _218.z, _218.x, yaw);
    _200.y -= 700.0f;
    mCameraPos.y = _200.y;
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("InitDropRescueView", 3);
}

void KartCam::DropRescueView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused1;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f look;
    JGeometry::TVec3f unused2;
    JGeometry::TVec3f pos;
    if (mTargetBody->getRescue()->_76 == 4) {
        pos.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
        look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
        dir.sub(look, pos);
        mCameraLookPos.y = mCameraLookPos.y + (dir.y - mCameraLookPos.y);
        _218.y = GetKartCtrl()->fcnvge(_218.y, 0.0f, 0.01395552f, 0.01395552f);
        GetKartCtrl()->ChaseFnumber(&_218.z, mTargetBody->_4e4, 0.7f);
        GetKartCtrl()->ChaseFnumber(&_218.x, _1ac, 0.5f);
        f32 yaw = _218.y + std::atan2f(mTargetBody->getRescue()->_d4.mMtx[0][2], mTargetBody->getRescue()->_d4.mMtx[2][2]);
        GetKartCtrl()->PolarToCross((VecPtr)&mCameraLookPos, (VecPtr)&mCameraPos, _218.z, _218.x, yaw);
        _200.y += 0.2f * (mCameraPos.y - _200.y);
        mCameraPos.y = _200.y;
        _200.x = mCameraPos.x;
        _200.z = mCameraPos.z;
        dir.sub(mCameraLookPos, _200);
        _18c = dir.length();
        _164 = 0.0f;
    } else {
        mCameraLookPos.set(mTargetBody->getRescue()->_a4);
        mCameraLookPos.x += 20.0f * mTargetBody->getRescue()->_d4.mMtx[0][2];
        mCameraLookPos.y += 20.0f * mTargetBody->getRescue()->_d4.mMtx[1][2];
        mCameraLookPos.z += 20.0f * mTargetBody->getRescue()->_d4.mMtx[2][2];
        mCameraLookPos.y += 100.0f;
        _218.y = GetKartCtrl()->fcnvge(_218.y, 0.0f, 0.01395552f, 0.01395552f);
        f32 yaw = _218.y + std::atan2f(mTargetBody->getRescue()->_d4.mMtx[0][2], mTargetBody->getRescue()->_d4.mMtx[2][2]);
        GetKartCtrl()->PolarToCross((VecPtr)&mCameraLookPos, (VecPtr)&mCameraPos, _218.z, _218.x, yaw);
        mCameraPos.y = _200.y;
    }
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("DropRescueView", 4);
}

void KartCam::InitLaunchView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f look;
    JGeometry::TVec3f base;
    SetFovyData();
    base.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    mCameraLookPos.sub(look, base);
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("InitLaunchView", 5);
}

void KartCam::LaunchView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f look;
    JGeometry::TVec3f target;
    JGeometry::TVec3f base;
    base.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    target.sub(look, base);
    dir.sub(target, mCameraLookPos);
    f32 len = dir.length();
    dir.normalize();
    dir.scale(0.8f * len);
    mCameraLookPos.add(dir);
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("LaunchView", 6);
}

void KartCam::InitLandView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f look;
    JGeometry::TVec3f base;
    SetFovyData();
    base.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    mCameraLookPos.sub(look, base);
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("InitLandView", 7);
}

void KartCam::LandView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f look;
    JGeometry::TVec3f target;
    JGeometry::TVec3f base;
    base.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    target.sub(look, base);
    dir.sub(target, mCameraLookPos);
    f32 len = dir.length();
    dir.normalize();
    dir.scale(0.8f * len);
    mCameraLookPos.add(dir);
    up.set(0.0f, 1.0f, 0.0f);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("LandView", 8);
}

void KartCam::InitPipeView() {
    JGeometry::TVec3f look;
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused;
    JGeometry::TVec3f pos;
    _184 = 0.0f;
    _1a4 = 0.0f;
    _164 = 0.0f;
    _1b8 = 3.14f;
    SetFovyData();
    _1c0 = 0.0f;
    _e6 = 0;
    _1c4 = 0.0f;
    _1c8 = 0.0f;
    _1a0 = 0.0f;
    pos.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    mCameraLookPos.sub(look, pos);
    _1ac = 0.0f;
    _1ac = _1ac + 3.141f * _180 / 180.0f;
    _18c = mTargetBody->_4e4;
    f32 yaw = _1b8 + std::atan2f(mTargetBody->_308.x, mTargetBody->_308.z);
    GetKartCtrl()->PolarToCross((VecPtr)&mCameraLookPos, (VecPtr)&_200, mTargetBody->_4e4, _1ac, yaw);
    up.set(0.0f, 1.0f, 0.0f);
    mCameraPos.set(_200);
    _20c.set(_200);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("InitPipeView", 9);
}

void KartCam::PipeView() {
    JGeometry::TVec3f up;
    JGeometry::TVec3f unused1;
    JGeometry::TVec3f unused2;
    JGeometry::TVec3f look;
    JGeometry::TVec3f unused3;
    JGeometry::TVec3f pos;
    pos.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    look.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    mCameraLookPos.sub(look, pos);
    _1ac = 0.0f;
    _1ac = _1ac + 3.141f * _180 / 180.0f;
    _18c = mTargetBody->_4e4;
    f32 yaw = _1b8 + std::atan2f(mTargetBody->_308.x, mTargetBody->_308.z);
    GetKartCtrl()->PolarToCross((VecPtr)&mCameraLookPos, (VecPtr)&_200, mTargetBody->_4e4, _1ac, yaw);
    up.set(0.0f, 1.0f, 0.0f);
    mCameraPos.set(_200);
    _20c.set(_200);
    C_MTXLookAt(mMtx, (VecPtr)&mCameraPos, (VecPtr)&up, (VecPtr)&mCameraLookPos);
    DegubCamera("PipeView", 10);
}

