#define JGEOM_NO_TVEC3_SET
#include <JSystem/JGeometry/Vec.h>

namespace JGeometry {
template <> template <> inline void TVec3f::set(f32 X, f32 Y, f32 Z) { x = X; y = Y; z = Z; }
}

#undef JGEOM_NO_TVEC3_SET

void kartCamStratEmitSet() {
    JGeometry::TVec3f v;
    v.set(0.0f, 0.0f, 0.0f);
}
#include "Yamamoto/kartCamera.h"

#include "dolphin/gx.h"
#include "JSystem/JAudio/JASFakeMatch2.h"
#include "JSystem/JMath/JMath.h"
#include "Kaneshige/KartMtx.h"
#include "Kaneshige/RaceMgr.h"
#include "Osako/system.h"

#include "dolphin/mtx.h"
#include <std/math.h>


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

void KartCam::First4ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 5.0f, 5.0f);
        if (mWidth == (f32)System::get3DScisW() && mHeight == (f32)System::get3DScisH()) {
            GetKartCtrl()->mBitfield |= 4;
        }
    }
    if (idx == 2) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 3.75f, 5.0f, 5.0f);
        f32 h = (f32)System::get3DScisDiv4H(0) - mHeight;
        f32 vh = (f32)System::get3DVpDiv4H(0) - mVpHeight;
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) + h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) + vh);
    }
    if (idx == 1) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 3.75f, 0.3f, 0.3f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 5.0f, 5.0f);
        f32 w = (f32)System::get3DScisDiv4W(0) - mWidth;
        f32 vw = (f32)System::get3DVpDiv4W(0) - mVpWidth;
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) + w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) + vw);
    }
    if (idx == 3) {
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 3.75f, 5.0f, 5.0f);
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 3.75f, 0.3f, 0.3f);
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
    if (mAspect > 0.01f) {
        mAspect = 0.01f;
    }
    if (mAspect < 3.0f) {
        mAspect = 3.0f;
    }
}

void KartCam::Second4ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 3.75f, 5.0f, 5.0f);
    }
    if (idx == 2) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 0.3f, 0.3f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 5.0f, 5.0f);
        f32 h = mHeight - (f32)System::get3DScisDiv4H(0);
        f32 vh = mVpHeight - (f32)System::get3DVpDiv4H(0);
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) - h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) - vh);
        if (mWidth == (f32)System::get3DScisW() && mHeight == (f32)System::get3DScisH()) {
            GetKartCtrl()->mBitfield |= 4;
        }
    }
    if (idx == 1) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 3.75f, 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 3.75f, 5.0f, 5.0f);
        f32 w = (f32)System::get3DScisDiv4W(0) - mWidth;
        f32 vw = (f32)System::get3DVpDiv4W(0) - mVpWidth;
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) + w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) + vw);
    }
    if (idx == 3) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 3.75f, 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 5.0f, 5.0f);
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
    if (mAspect > 0.01f) {
        mAspect = 0.01f;
    }
    if (mAspect < 3.0f) {
        mAspect = 3.0f;
    }
}

void KartCam::Third4ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 3.75f, 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 5.0f, 5.0f);
    }
    if (idx == 2) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 3.75f, 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 3.75f, 5.0f, 5.0f);
        f32 h = (f32)System::get3DScisDiv4H(0) - mHeight;
        f32 vh = (f32)System::get3DVpDiv4H(0) - mVpHeight;
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) + h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) + vh);
    }
    if (idx == 1) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 5.0f, 5.0f);
        f32 w = mWidth - (f32)System::get3DScisDiv4W(0);
        f32 vw = mVpWidth - (f32)System::get3DVpDiv4W(0);
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) - w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) - vw);
        if (mWidth == (f32)System::get3DScisW() && mHeight == (f32)System::get3DScisH()) {
            GetKartCtrl()->mBitfield |= 4;
        }
    }
    if (idx == 3) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 3.75f, 5.0f, 5.0f);
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
    if (mAspect > 0.01f) {
        mAspect = 0.01f;
    }
    if (mAspect < 3.0f) {
        mAspect = 3.0f;
    }
}

void KartCam::Fourth4ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 3.75f, 0.3f, 0.3f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 3.75f, 5.0f, 5.0f);
    }
    if (idx == 2) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, 0.0f, 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, 0.0f, 0.3f, 0.3f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 5.0f, 5.0f);
        f32 h = mHeight - (f32)System::get3DScisDiv4H(0);
        f32 vh = mVpHeight - (f32)System::get3DVpDiv4H(0);
        mPosH = (int)((f32)System::get3DScisDiv4Y(idx) - h);
        mVpPosH = (int)((f32)System::get3DVpDiv4Y(idx) - vh);
    }
    if (idx == 1) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 0.3f, 0.3f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 3.75f, 5.0f, 5.0f);
        f32 w = mWidth - (f32)System::get3DScisDiv4W(0);
        f32 vw = mVpWidth - (f32)System::get3DVpDiv4W(0);
        mPosV = (int)((f32)System::get3DScisDiv4X(idx) - w);
        mVpPosV = (int)((f32)System::get3DVpDiv4X(idx) - vw);
    }
    if (idx == 3) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 5.0f, 5.0f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 0.3f, 0.3f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 5.0f, 5.0f);
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
    if (mAspect > 0.01f) {
        mAspect = 0.01f;
    }
    if (mAspect < 3.0f) {
        mAspect = 3.0f;
    }
}

void KartCam::First2ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mWidth = GetKartCtrl()->fcnvge(mWidth, (f32)System::get3DScisW(), 0.3f, 0.3f);
        mVpWidth = GetKartCtrl()->fcnvge(mVpWidth, System::get3DVpW(), 0.3f, 0.3f);
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 5.0f, 5.0f);
        if (mWidth == (f32)System::get3DScisW() && mHeight == (f32)System::get3DScisH()) {
            GetKartCtrl()->mBitfield |= 4;
        }
    }
    if (idx == 1) {
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 3.75f, 5.0f, 5.0f);
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
    if (mAspect > 0.01f) {
        mAspect = 0.01f;
    }
    if (mAspect < 3.0f) {
        mAspect = 3.0f;
    }
}

void KartCam::Second2ScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    if (idx == 0) {
        mHeight = GetKartCtrl()->fcnvge(mHeight, 0.0f, 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, 3.75f, 5.0f, 5.0f);
    }
    if (idx == 1) {
        mHeight = GetKartCtrl()->fcnvge(mHeight, (f32)System::get3DScisH(), 5.0f, 5.0f);
        mVpHeight = GetKartCtrl()->fcnvge(mVpHeight, System::get3DVpH(), 5.0f, 5.0f);
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
    if (mAspect > 0.01f) {
        mAspect = 0.01f;
    }
    if (mAspect < 3.0f) {
        mAspect = 3.0f;
    }
}

void KartCam::SetVictoryScreenPort(unsigned char idx) {
    RaceMgr *mgr = RaceMgr::getCurrentManager();
    int camNum = mgr->getCameraNumber();
    s16 zoom;
    int mode;
    if (camNum == 1) {
        return;
    }
    if (mgr->isZoomWinConsole()) {
        zoom = mgr->getZoomWinConsoleNo();
        mode = mgr->getRaceMode();
        if (mode < 8) {
            if (mode >= 2) {
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
        } else {
            goto end;
        }
    } else {
        goto end;
    }
end:
    return;
}

inline s16 RaceMgr::getZoomWinConsoleNo() const { return mRaceDirector->getZoomWinConsoleNo(); }

inline s16 RaceDirector::getZoomWinConsoleNo() const { return mZoomWinCnsNo; }

inline bool RaceMgr::isZoomWinConsole() const { return mRaceDirector->isZoomWinConsole(); }

void KartCam::DoMoveCamera(JGeometry::TVec3<float> *, JGeometry::TVec3<float> *) {}

void KartCam::DoRoof(JGeometry::TVec3<float> *pos, CrsArea *area) {
    JGeometry::TVec3f a;
    JGeometry::TVec3f b;
    if (area->isInside() == 1) {
        area->getRoofPosition(&a);
        f32 d = pos->y - (a.y - 80.0f);
        if (d > 0.0f) {
            mFlags |= 4;
            f32 scale = 0.3f * d;
            b.set(0.0f, -1.0f, 0.0f);
            b.scale(scale);
            pos->add(b);
        }
    }
}

void KartCam::DoColCamera() {
    JGeometry::TVec3f a, b, f, e, d, c, g, h;
    f32 len = _1c4 + (_1a4 + (_1ac + _184));
    if (len >= 1.553f) {
        len = 1.553f;
    }
    if (len < 0.0) {
        len = 0.0f;
    }
    GetKartCtrl()->PolarToCross((VecPtr)&mCameraLookPos, (VecPtr)&e, _18c, len, *(f32*)&_15c[0]);
    DoMoveCamera(&e, &_20c);
    f.sub(mCameraLookPos, _20c);
    f.normalize();
    d.set(f.x * _18c, f.y * _18c, f.z * _18c);
    _20c.sub(mCameraLookPos, d);
    _200.set(_20c);
    mCameraPos.set(_200.x, _200.y, _200.z);
}

void KartCam::DoSea(JGeometry::TVec3<float> *, CrsGround *) {}

void KartCam::DoGround(JGeometry::TVec3<float> *, CrsGround *) {}

void KartCam::GroundCheck(JGeometry::TVec3<float> *, JGeometry::TVec3<float> *, CrsArea *) {}

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
    _1ac = 3.140625f * mTargetBody->_4e8 / 180.0f;
    _1ac = _1ac + 3.140625f * _180 / 180.0f;
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

void KartCam::DoChangFovy() {}

void KartCam::DoLookChase() {
    JGeometry::TVec3f c;
    JGeometry::TVec3f b;
    JGeometry::TVec3f a;
    a.set(mTargetBody->_350.x, mTargetBody->_350.y + _17c, mTargetBody->_350.z);
    c.set(mTargetBody->mPlayerPosMtx[0][3], mTargetBody->mPlayerPosMtx[1][3], mTargetBody->mPlayerPosMtx[2][3]);
    b.sub(c, a);
    if (mTargetBody->getRescue()->_75 != 4 || !(mTargetBody->mCarStatus & 0x400000)) {
        GetKartCtrl()->ChaseFnumber(&mCameraLookPos.y, b.y, 0.89f);
    }
    mCameraLookPos.x = b.x;
    mCameraLookPos.z = b.z;
}

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

