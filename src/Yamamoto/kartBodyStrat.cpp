#include "Yamamoto/KartStrat.h"
#include "Yamamoto/kartBody.h"

#include "JSystem/JAudio/JASFakeMatch2.h"
#include "Sato/JPEffectPerformer.h"
#include "Yamamoto/kartCamera.h"
#include "Yamamoto/kartCtrl.h"

#include <std/math.h>

// comments inside functions are inline functions being called in that function

void KartStrat::Init(int idx) {
    mBody = GetKartCtrl()->getKartBody(idx);
}

void KartStrat::GetBodySpeed() {
    KartBody *body = mBody;
    body->_454 = 2.16f * body->_448;
    if (body->mGameStatus & 0x200) {
        body->_458 = 0.0f;
    } else {
        body->_458 = body->_454;
    }
}

void KartStrat::GetBodyRoll() {
    KartBody *body = mBody;
    f32 len = GetKartCtrl()->SpeedySqrtf(body->_2f0.x * body->_2f0.x + body->_2f0.z * body->_2f0.z);
    if (len == 0.0f) {
        body->_460 = 0.0f;
    } else {
        body->_460 = std::atanf(body->_2f0.y / len);
    }
    f32 len2 = GetKartCtrl()->SpeedySqrtf(body->_308.x * body->_308.x + body->_308.z * body->_308.z);
    if (len2 == 0.0f) {
        body->_464 = 0.0f;
    } else {
        body->_464 = std::atanf(body->_308.y / len2);
    }
    body->mShadowModel->setRoll(body->_460);
    body->_29c.zero();
    body->_284.zero();
    body->_290.zero();
    JGeometry::TVec3f vec;
    vec.set(body->mVel);
    vec.normalize();
    body->_510 = vec.angle(body->_308);
}

void KartStrat::GetRoadBodyRoll() {
    KartBody *body = mBody;
    JGeometry::TVec3f vec;
    vec.set(body->_1a0[0][3], body->_1a0[1][3], body->_1a0[2][3]);
    body->mBodyGround.search(vec, body->_254);
    if (body->mBodyGround.isObject()) {
        body->_58c = body->mBodyGround.getObject()->getKind();
    } else {
        body->_58c = 0;
    }
    if (body->mBodyGround.getAttribute() == 10 || body->mBodyGround.getAttribute() == 2) {
        body->_32c.set(0.0f, 1.0f, 0.0f);
    } else {
        body->mBodyGround.getNormal(&body->_32c);
    }
    JGeometry::TVec3f vec1;
    JGeometry::TVec3f vec2;
    JGeometry::TVec3f vec3;
    vec1.cross(body->_32c, body->_308);
    vec1.normalize();
    vec2.cross(vec1, body->_32c);
    vec2.normalize();
    vec3.cross(vec2, vec1);
    vec3.normalize();
    body->_170[0][0] = vec1.x;
    body->_170[1][0] = vec1.y;
    body->_170[2][0] = vec1.z;
    body->_170[0][1] = vec3.x;
    body->_170[1][1] = vec3.y;
    body->_170[2][1] = vec3.z;
    body->_170[0][2] = vec2.x;
    body->_170[1][2] = vec2.y;
    body->_170[2][2] = vec2.z;
    f32 len = GetKartCtrl()->SpeedySqrtf(vec1.x * vec1.x + vec1.z * vec1.z);
    if (len == 0.0f) {
        body->_468 = body->_460;
    } else {
        body->_468 = std::atanf(vec1.y / len);
        body->_468 = body->_460 - body->_468;
    }
    len = GetKartCtrl()->SpeedySqrtf(vec2.x * vec2.x + vec2.z * vec2.z);
    if (len != 0.0f) { // NON_MATCHING: branch layout
        body->_46c = std::atanf(vec2.y / len);
        body->_46c = body->_464 - body->_46c;
    }
}

void KartStrat::GetBodyNorm() {
    KartBody *body = mBody;
    body->_368.setNormal(body->getSus(0)->_284, body->getSus(1)->_284, body->getSus(2)->_284);
    body->_368.scale(-1.0f);
    body->_368.normalize();
    if (body->_368.y < 0.0f) {
        body->_368.y = -body->_368.y;
    }
}

void KartStrat::DoEnemyMaxmZCrl(f32 p1) {
    KartBody *body = mBody;
    JGeometry::TVec3f vec;
    vec.set(body->_308);
    f32 scale;
    if (GetKartCtrl()->GetCarSpeed(body->mMynum) < 20.0f) {
        scale = body->mSpeedScale * (0.2f * body->mVel.length()) + 1.0f;
    } else {
        scale = vec.length();
    }
    vec.scale(scale);
    f32 diff = p1 - vec.length();
    if (diff <= 0.0f)
        return;
    vec.y /= diff;
    vec.y = 0.0f;
    vec.x /= diff;
    vec.z /= diff;
    vec.scale(diff);
    body->mVel.add(vec);
}

void KartStrat::DoMaxLevelZXVelCrl(f32 p1, f32 p2) {
    KartBody *body = mBody;
    JGeometry::TVec3f vec;
    vec.set(body->mVel.x, 0.0f, body->mVel.z);
    f32 diff = p1 - vec.length();
    JGeometry::TVec3f vec2;
    if (diff <= 0.0f)
        return;
    vec2.x = body->mVel.x / diff;
    vec2.y = body->mVel.y / diff;
    vec2.z = body->mVel.z / diff;
    vec2.y *= p2;
    vec2.scale(diff);
    body->mVel.add(vec2);
}

void KartStrat::DoVelCrl(f32 max) {
    KartBody *body = mBody;
    f32 len = body->mVel.length();
    if (len > max && len > 0.0f) {
        body->mVel.scale(max / len);
    }
}

void KartStrat::DoWgCrl(f32 max) {
    KartBody *body = mBody;
    f32 len = body->mWg.length();
    if (len > max && len > 0.0f) {
        body->mWg.scale(max / len);
    }
}

void KartStrat::DoRotPose(JGeometry::TVec3f dir, f32 scale) {
    KartBody *body = mBody;
    JGeometry::TVec3f dvec;
    JGeometry::TVec3f ex;
    JGeometry::TVec3f ey;
    JGeometry::TVec3f ez;
    ex.set(body->_110[0][0], body->_110[1][0], body->_110[2][0]);
    ey.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
    ez.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
    dvec.set(dir);
    dvec.normalize();
    ez.add(dvec * scale * body->mSpeedScale);
    ez.normalize();
    body->_110[0][2] = ez.x;
    body->_110[1][2] = ez.y;
    body->_110[2][2] = ez.z;
    ez.normalize();
    dvec.cross(ey, ez);
    dvec.normalize();
    body->_110[0][0] = dvec.x;
    body->_110[1][0] = dvec.y;
    body->_110[2][0] = dvec.z;
    ey.cross(ez, dvec);
    ey.normalize();
    body->_110[0][1] = ey.x;
    body->_110[1][1] = ey.y;
    body->_110[2][1] = ey.z;
    PSMTXCopy(body->_110, body->mPlayerPosMtx);
    f32 offset = GetKartCtrl()->GetKartBodyOffset(body->mMynum);
    body->mPlayerPosMtx[0][3] += body->mPlayerPosMtx[0][1] * offset;
    body->mPlayerPosMtx[1][3] += body->mPlayerPosMtx[1][1] * offset;
    body->mPlayerPosMtx[2][3] += body->mPlayerPosMtx[2][1] * offset;
    body->_2f0.set(body->_110[0][0], body->_110[1][0], body->_110[2][0]);
    body->_2fc.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
    body->_308.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
}

void KartStrat::DoPoseCrl() {
    KartBody *body = mBody;
    JGeometry::TVec3f v50;
    JGeometry::TVec3f v44;
    JGeometry::TVec3f v38;
    JGeometry::TVec3f v2c;
    JGeometry::TVec3f v20;
    v38.set(body->_110[0][0], body->_110[1][0], body->_110[2][0]);
    v2c.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
    v20.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
    if (body->_584 == 13 || (body->_584 >= 2 && body->_584 < 7)) {
        if (body->_588 != 1) {
            return;
        }
        f32 dy = body->_110[1][3] - body->mBodyGround.getHeight();
        if (dy > 350.0f) {
            return;
        }
        body->mVel.x *= 0.3f;
        body->mVel.z *= 0.3f;
        v50.set(body->_32c);
        f32 d1 = v2c.dot(v50);
        if (v2c.y < 0.0f) {
            v2c = v50;
        } else {
            v2c.add(v50 * 0.6f * body->mSpeedScale);
        }
        v2c.normalize();
        v44.set(body->_338);
        v44.scale(body->mSpeedScale);
        v20.add(v44);
        v20.normalize();
        v38.cross(v2c, v20);
        v38.normalize();
        body->_110[0][0] = v38.x;
        body->_110[1][0] = v38.y;
        body->_110[2][0] = v38.z;
        v2c.cross(v20, v38);
        v2c.normalize();
        body->_110[0][1] = v2c.x;
        body->_110[1][1] = v2c.y;
        body->_110[2][1] = v2c.z;
        v20.cross(v38, v2c);
        v20.normalize();
        body->_110[0][2] = v20.x;
        body->_110[1][2] = v20.y;
        body->_110[2][2] = v20.z;
        v44.set(body->_338);
        v50.set(body->_32c);
        v38.cross(v50, v44);
        v38.normalize();
        v2c.cross(v44, v38);
        v2c.normalize();
        v44.cross(v38, v2c);
        v44.normalize();
        f32 f31 = v20.dot(v44);
        if (body->getTouchNum() >= 1 && f31 >= 0.8f) {
            RollCrashClear();
            return;
        }
        if (d1 >= 0.5f && f31 >= 0.8f) {
            RollCrashClear();
            return;
        }
        if (d1 >= 0.8f && f31 >= 0.8f) {
            RollCrashClear();
            return;
        }
        return;
    }
    u8 flag = 0;
    if ((body->getRescue()->mFlags & 0x40) != 0) {
        return;
    }
    f32 f1;
    f32 f0;
    f32 f30;
    if (body->getTouchNum() == 0) {
        if (body->_4c0 > 0.1f || body->_4c0 < -0.1f || (body->mCarStatus & 1) != 0) {
            f0 = 0.9f;
            flag = 1;
            f30 = 0.05f;
        } else {
            f0 = 1.0f;
            f30 = 0.03f;
        }
        if ((body->mCarStatus & 0x1000) != 0) {
            f30 = 0.33f;
        }
    } else {
        if (v2c.y < 0.0f) {
            f0 = 1.0f;
            f30 = 0.4f;
        } else {
            f1 = v2c.dot(body->_32c);
            f0 = 0.348888f;
            f30 = 0.0f;
        }
    }
    if (f1 >= f0) {
        return;
    }
    if (flag != 0 || f1 < 0.1f) {
        v50.set(0.0f, 1.0f, 0.0f);
    } else {
        v50.set(body->_32c);
    }
    v50.scale(f30 * body->mSpeedScale);
    v2c.add(v50);
    v2c.normalize();
    body->_110[0][1] = v2c.x;
    body->_110[1][1] = v2c.y;
    body->_110[2][1] = v2c.z;
    v50.cross(v20, v2c);
    v50.normalize();
    body->_110[0][0] = v50.x;
    body->_110[1][0] = v50.y;
    body->_110[2][0] = v50.z;
    v20.cross(v2c, v50);
    body->_110[0][2] = v20.x;
    body->_110[1][2] = v20.y;
    body->_110[2][2] = v20.z;
    body->mWg.x = 0.0f;
    body->mWg.z = 0.0f;
}

void KartStrat::MovingSpinClear() {
    KartBody *body = mBody;
    body->mCarStatus &= ~0x180000;
    body->_584 = 0;
    body->_588 = 0;
    body->getDamage()->SetDamager();
    AllGravyClear();
    body->getCrash()->DoDecBalloon();
}

void KartStrat::MovingHalfSpinClear() {
    KartBody *body = mBody;
    body->mCarStatus &= ~0x1100000;
    body->_584 = 0;
    body->_588 = 0;
    body->getDamage()->SetDamager();
    AllGravyClear();
    body->getCrash()->DoDecBalloon();
}

void KartStrat::MovingTornadeClear() {
    KartBody *body = mBody;
    body->mCarStatus &= ~0x80000100000ull;
    body->_584 = 0;
    body->_588 = 0;
    body->getDamage()->SetDamager();
    AllGravyClear();
}

void KartStrat::FreezeClear() {
    KartBody *body = mBody;
    body->mCarStatus &= ~0x2000080100000ull;
    body->_584 = 0;
    body->_588 = 0;
    body->getDamage()->SetDamager();
    AllGravyClear();
}

void KartStrat::RollCrashClear() {
    KartBody *body = mBody;
    if ((body->mCarStatus & 0x800800000ull) != 0) {
        body->mVel.x = 0.0f;
        body->mVel.z = 0.0f;
    }
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashEndSound();
    body->mCarStatus &= ~0x200840B02000ull;
    body->_584 = 0;
    body->_588 = 0;
    body->getDamage()->SetDamager();
    AllGravyClear();
    body->getCrash()->DoDecBalloon();
}

void KartStrat::AllGravyClear() {
    KartBody *body = mBody;
    body->mLiftframe = 0.0f;
    body->_398 = 0.0f;
    GravyClear();
}

void KartStrat::GravyClear() {
    KartBody *body = mBody;
    body->_4c0 = 0.0f;
    body->_4c4 = 0.0f;
    body->_4d4 = 0.0f;
    body->_4d8 = 0.0f;
    body->_35c.zero();
}

void KartStrat::LiftClear() {
    mBody->mCarStatus &= ~0x1800000000003ull;
}

void KartStrat::PitchClear() {
    mBody->mCarStatus &= ~0x300;
}

void KartStrat::DashClear() {
    KartBody *body = mBody;
    if ((body->mCarStatus & 0x10000) != 0) {
        body->_5b5 = 0xf;
    }
    body->mCarStatus &= ~0x4002003C000ull;
    body->mBoostTimer = 0;
    body->_52c = 0.0f;
    body->mSpeedScale = 0.313f;
}

void KartStrat::OtherClear() {
    KartBody *body = mBody;
    body->mCarStatus &= ~0x828000000Cull;
    body->mCarStatus &= ~0x800;
    body->mCarStatus &= ~0x20000000000ull;
    body->getDamage()->ClrAllDamage();
    GetKartCtrl()->getKartAnime(body->mMynum)->mFlags &= ~0x201000ull;
    body->getDossin()->_1c = 1.0f;
    body->getDossin()->_14 = 0;
    body->mDriftSterr = 0;
    body->mMTState = 0;
    body->mMTBoost = 0;
    body->_5b6 = 0;
    body->_4a4 = 0.0f;
    body->_5b4 = 0;
    body->_314.zero();
    body->_564 = 0.0f;
    body->_518 = 0.0f;
    body->getGame()->_0b = 0;
}

void KartStrat::ShakeGround() {}

void KartStrat::DoAdjustment() { 
    // void KartBody::getPipe() {}
}

void KartStrat::DoWheelSpinCrl() {
    KartBody *body = mBody;
    KartSus *sus0 = body->getSus(0);
    KartSus *sus1 = body->getSus(1);
    KartSus *sus2 = body->getSus(2);
    KartSus *sus3 = body->getSus(3);
    switch (body->_588) {
    case 0:
        body->mVel.x = 0.0f;
        body->mVel.z = 0.0f;
        body->_3c8 = 5.0f;
        body->mWg.zero();
        sus0->_10c += 0.0348888f;
        sus1->_10c += 0.0348888f;
        sus2->_10c += 0.0348888f;
        sus3->_10c += 0.0348888f;
        body->_594++;
        if (body->_594 == 0x3c) {
            body->_588 = 1;
            body->_594 = 0;
        }
        break;
    case 1:
        body->mVel.x = 0.0f;
        body->mVel.z = 0.0f;
        body->_3c8 = 5.0f;
        body->mWg.zero();
        body->_594++;
        if (body->_594 == 0x28) {
            body->_588 = 2;
            body->_594 = 0;
        }
        sus0->_10c += 0.0348888f;
        sus1->_10c += 0.0348888f;
        sus2->_10c += 0.0348888f;
        sus3->_10c += 0.0348888f;
        break;
    case 2:
        body->_3c8 = 0.0f;
        sus0->_10c = 0.0f;
        sus1->_10c = 0.0f;
        sus2->_10c = 0.0f;
        sus3->_10c = 0.0f;
        sus0->_110 = 0.0f;
        sus1->_110 = 0.0f;
        sus2->_110 = 0.0f;
        sus3->_110 = 0.0f;
        body->mCarStatus &= ~0x800;
        body->_584 = 0;
        body->_588 = 0;
        break;
    }
}

void KartStrat::DoWallCrl() {
    KartBody *body = mBody;
    if ((body->mGameStatus & 8) != 0)
        return;
    if ((body->mCarStatus & 0x420) != 0 || (body->mCarStatus & 0x8000000) != 0) {
        if (body->mTireAngle == 0.0f)
            return;
        if (body->getTouchNum() == 0)
            return;
        if (body->_3c8 == 0.0 && (body->mGameStatus & 0x200) != 0)
            return;
        if ((body->mCarStatus & 3) != 0) {
            body->mWg.y += 0.0697777f * body->mFrame;
        } else {
            body->mWg.y += 0.0697777f * body->mFrame;
        }
    }
    if ((body->mCarStatus & 0x20000000) == 0)
        return;
    if ((body->mCarStatus & 0x14000400) != 0)
        return;
    if (body->getTouchNum() < 1)
        return;
    body->mWg.y = 0.0f;
}

void KartStrat::DoYawLimit() {
    KartBody *body = mBody;
    f32 limit = 0.0784999f;
    if ((body->mCarStatus & 8) != 0) {
        limit = -0.174444f;
    } else if ((body->mCarStatus & 2) != 0 && body->mFrame >= 0.4f) {
        limit = 0.226777f;
    } else if ((body->mCarStatus & 2) != 0) {
        limit = 0.0174444f;
    } else if ((body->mCarStatus & 1) != 0 && body->mFrame <= -0.4f) {
        limit = 0.226777f;
    } else if ((body->mCarStatus & 1) != 0) {
        limit = 0.0174444f;
    } else if ((body->mCarStatus & 3) == 0 && body->mFrame < 0.1f && body->mFrame >= 0.0f) {
        limit = 0.000174444f;
    }
    if ((body->mCarStatus & 3) == 0 && body->mFrame > -0.1f && body->mFrame <= 0.0f) {
        limit = 0.000174444f;
    }
    if (body->mWg.y > limit) {
        body->mWg.y = limit;
    } else if (body->mWg.y < -limit) {
        body->mWg.y = -limit;
    }
    if (body->_458 <= 1.0f) {
        body->mWg.y = 0.0f;
    }
}

void KartStrat::DoRollLimit() {
    KartBody *body = mBody;
    f32 limit = 0.0697777f;
    if (body->mWg.z > limit) {
        body->mWg.z = limit;
    } else if (body->mWg.z < -limit) {
        body->mWg.z = -limit;
    }
}

void KartStrat::DoLiftCrl() {
    KartBody *body = mBody;
    if ((body->mCarStatus & 3) == 0) {
        if (body->_458 < 15.0f) {
            body->_4d8 = 0.0f;
        }
    }
    if ((body->mCarStatus & 0x300) != 0 && body->getTouchNum() != 0) {
        GetKartCtrl()->ChaseFnumber(&body->_4d8, 0.0f, 0.1f);
    } else if ((body->mCarStatus & 2) != 0) {
        if (body->getTouchNum() >= 3) {
            GetKartCtrl()->ChaseFnumber(&body->_4d8, body->_4c4 * body->_3bc, body->_3b8);
        } else {
            GetKartCtrl()->ChaseFnumber(&body->_4d8, body->_4c4 * body->_3c4, body->_3c0);
        }
    } else if ((body->mCarStatus & 1) != 0) {
        if (body->getTouchNum() >= 3) {
            GetKartCtrl()->ChaseFnumber(&body->_4d8, body->_4c4 * body->_3bc, body->_3b8);
        } else {
            GetKartCtrl()->ChaseFnumber(&body->_4d8, body->_4c4 * body->_3c4, body->_3c0);
        }
    } else {
        GetKartCtrl()->ChaseFnumber(&body->_4d8, body->_4d4, 0.75f);
    }
    DoLiftYawCrl();
}

void KartStrat::DoLiftYawCrl() {
    // void SysDebug::checkNaNVector(Vec *, char *) {}
}

void KartStrat::DoRollLim(f32 roll, f32 lim) {
    KartBody *body = mBody;
    JGeometry::TVec3f a;
    JGeometry::TVec3f ex;
    JGeometry::TVec3f ey;
    JGeometry::TVec3f ez;
    ex.set(body->_110[0][0], body->_110[1][0], body->_110[2][0]);
    ey.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
    ez.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
    Mtx mtx;
    if (roll < 0.0f) {
        GetKartCtrl()->RotZMatrix(mtx, lim);
    } else {
        GetKartCtrl()->RotZMatrix(mtx, -lim);
    }
    GetKartCtrl()->MulMatrix(mtx, mtx, body->_170);
    GetKartCtrl()->AddMatrix(body->_170, mtx);
    a.set(body->_170[0][0], body->_170[1][0], body->_170[2][0]);
    a.scale(0.05f * body->mSpeedScale);
    ex.add(a);
    ex.normalize();
    ez.cross(ex, ey);
    ez.normalize();
    ey.cross(ez, ex);
    ey.normalize();
    body->_110[0][0] = ex.x;
    body->_110[1][0] = ex.y;
    body->_110[2][0] = ex.z;
    body->_110[0][1] = ey.x;
    body->_110[1][1] = ey.y;
    body->_110[2][1] = ey.z;
    body->_110[0][2] = ez.x;
    body->_110[1][2] = ez.y;
    body->_110[2][2] = ez.z;
    PSMTXCopy(body->_110, body->mPlayerPosMtx);
    body->_2f0.set(body->_110[0][0], body->_110[1][0], body->_110[2][0]);
    body->_2fc.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
    body->_308.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
    body->mWg.z = 0.0f;
}

void KartStrat::DoPitchLim() {
    f32 pitch;
    f32 limit;
    KartBody *body = mBody;
    if (body->getTouchNum() == 0) {
        limit = 0.523333f;
        pitch = body->_464;
    } else {
        pitch = body->_46c;
        limit = 0.523333f;
    }
    if (pitch <= -limit || pitch >= limit) {
        JGeometry::TVec3f v50;
        JGeometry::TVec3f v44;
        JGeometry::TVec3f v38;
        v50.cross(body->_32c, body->_308);
        v50.normalize();
        v44.cross(v50, body->_32c);
        v44.normalize();
        v38.cross(v44, v50);
        v38.normalize();
        body->_170[0][0] = v50.x;
        body->_170[1][0] = v50.y;
        body->_170[2][0] = v50.z;
        body->_170[0][1] = v38.x;
        body->_170[1][1] = v38.y;
        body->_170[2][1] = v38.z;
        body->_170[0][2] = v44.x;
        body->_170[1][2] = v44.y;
        body->_170[2][2] = v44.z;
        JGeometry::TVec3f v2c;
        JGeometry::TVec3f v20;
        JGeometry::TVec3f v14;
        JGeometry::TVec3f v8;
        v20.set(body->_110[0][0], body->_110[1][0], body->_110[2][0]);
        v14.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
        v8.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
        Mtx mtx;
        if (pitch < 0.0f) {
            GetKartCtrl()->RotXMatrix(mtx, -limit);
        } else {
            GetKartCtrl()->RotXMatrix(mtx, limit);
        }
        GetKartCtrl()->MulMatrix(mtx, mtx, body->_170);
        GetKartCtrl()->AddMatrix(body->_170, mtx);
        v2c.set(body->_170[0][1], body->_170[1][1], body->_170[2][1]);
        v2c.scale(0.05f * body->mSpeedScale);
        v14.add(v2c);
        v14.normalize();
        v20.cross(v8, v14);
        v20.normalize();
        v8.cross(v14, v20);
        v8.normalize();
        body->_110[0][0] = v20.x;
        body->_110[1][0] = v20.y;
        body->_110[2][0] = v20.z;
        body->_110[0][1] = v14.x;
        body->_110[1][1] = v14.y;
        body->_110[2][1] = v14.z;
        body->_110[0][2] = v8.x;
        body->_110[1][2] = v8.y;
        body->_110[2][2] = v8.z;
        PSMTXCopy(body->_110, body->mPlayerPosMtx);
        body->_2f0.set(body->_110[0][0], body->_110[1][0], body->_110[2][0]);
        body->_2fc.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
        body->_308.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
        body->mWg.x = 0.0f;
    }
}

void KartStrat::DoRollCrash() {
    KartBody *body = mBody;
    f32 v31 = body->_4d8;
    if ((body->mCarStatus & 3) == 0) {
        if ((body->_468 < 0.0f && body->mWg.z < 0.0f) || (body->_468 > 0.0f && body->mWg.z > 0.0f)) {
            body->mWg.z *= 0.585f;
        }
    }
    if (body->mWg.z > 0.0348888f) {
        body->mWg.z = 0.0348888f;
    }
    if (body->mWg.z < -0.0348888f) {
        body->mWg.z = -0.0348888f;
    }
    f32 pitch;
    f32 lim;
    if (body->getTouchNum() == 0) {
        lim = 0.523333f;
        pitch = body->_460;
    } else {
        pitch = body->_468;
        lim = 0.523333f;
    }
    if ((body->mCarStatus & 3) != 0 || body->_4d8 != 0.0f || body->mBodyGround.getAttribute() == 2 ||
        body->getTouchNum() <= 2) {
        if (pitch <= -lim || pitch >= lim) {
            DoRollLim(pitch, lim);
        } else {
            body->mWg.z += (3.141f * v31) / 180.0f;
        }
    }
    DoRollLimit();
}

void KartStrat::DoTestPitchCrl() {
    KartBody *body = mBody;
    DoYawLimit();
    if ((body->mCarStatus & 0x300) != 0) {
        body->mWg.x *= 0.6f;
        if (body->getTouchNum() == 0) {
            if ((body->mCarStatus & 0x200) != 0) {
                body->mWg.x = 0.0f;
                f32 lim = 0.348888f;
                if (body->_464 <= -lim)
                    return;
                body->mWg.x += (3.141f * body->_4c0) / 180.0f;
            } else {
                body->mWg.x = 0.0f;
                if (body->_464 >= 0.261666f)
                    return;
                body->mWg.x += (3.141f * body->_4c0) / 180.0f;
            }
        } else {
            body->_4c0 = 0.0f;
        }
    } else {
        body->_4c0 = 0.0f;
    }
    if ((body->mCarStatus & 0x100000) == 0) {
        if (body->mWg.x > 0.226777f) {
            body->mWg.x = 0.226777f;
        } else if (body->mWg.x < 0.174444f) {
            body->mWg.x = 0.174444f;
        }
    }
}

f32 KartStrat::DoDashCrl(f32 speed) {
    KartBody *body = mBody;
    if ((body->getThunder()->mFlags & 1) != 0) {
        speed *= 0.7f;
    }
    if ((body->mCarStatus & 8) != 0) {
        return 20.0f;
    }
    if ((body->mCarStatus & 0x9000) == 0) {
        if ((body->mCarStatus & 0x20000000000ull) != 0) {
            speed *= 1.3f;
        }
        return speed;
    }
    body->mMTBoost = 0;
    body->mCarStatus &= ~0x20000000000ull;
    if ((body->mCarStatus & 0x1000) != 0) {
        speed = 200.0f;
    } else if ((body->mCarStatus & 0x20000000) != 0) {
        if (body->mClass == 0) {
            speed = 1.55f * body->_3f0;
        } else {
            speed = 1.45f * body->_3f0;
        }
    } else if ((body->mCarStatus & 0x20004000) != 0) {
        if (body->mClass == 0) {
            speed = 1.2f * body->_3f0;
        } else {
            speed = 1.3f * body->_3f0;
        }
    } else if ((body->mCarStatus & 0x40000000000ull) != 0) {
        speed = 1.9f * body->_3f0;
    } else if ((body->mCarStatus & 0) != 0) {
        speed = body->_3f0;
    }
    return speed;
}

f32 KartStrat::DoStarCrl(f32) {
    f32 ret;
    if ((mBody->mCarStatus & 0x40000) == 0)
        return ret;
    if ((mBody->mCarStatus & 0x8000) != 0)
        return ret;
    ret = mBody->_3f0 * 1.2f;
    return ret;
}

void KartStrat::DoAirCrl() {
    KartBody *body = mBody;
    if (body->getTouchNum() != 0) {
        body->_560 = 0.02f;
    }
    if ((body->mCarStatus & 0x500000) == 0 && (body->mCarStatus & 0x40) != 0 && body->mVel.y < 0.0f &&
        (body->_464 >= 0.0872222f || body->_464 <= -0.0872222f)) {
        f32 power = body->_464 / 0.348888f;
        if (power > 1.0f) {
            power = 1.0f;
        }
        if (power < -1.0f) {
            power = -1.0f;
        }
        body->_560 = GetKartCtrl()->fcnvge(body->_560, 0.0f, 0.001f, 0.001f);
        power = power * -body->_560;
        body->mVel.y = body->mVel.y * (1.0f + power);
    }
    if (body->getTouchNum() != 0 && (body->getGame()->_08 & 1) != 0) {
        body->getGame()->_08 &= ~1;
        if (GetKartCtrl()->getKartAnime(body->mMynum)->IsFloatAnime(body->mMynum)) {
            GetKartCtrl()->getKartAnime(body->mMynum)->mFlags |= 0x4000000000ull;
        }
    }
    JGeometry::TVec3f pos;
    pos.set(body->mPlayerPosMtx[0][3], body->mPlayerPosMtx[1][3], body->mPlayerPosMtx[2][3]);
    if ((body->mCarStatus & 0x40) != 0) {
        if (body->mBodyGround.getAttribute() == 17) {
            f32 dy = body->mPlayerPosMtx[1][3] - body->mBodyGround.getWaterHeight();
            if (dy < -40.0f && (body->mCarStatus & 0x400000) == 0) {
                Mtx splashMtx;
                splashMtx[0][3] = body->mPlayerPosMtx[0][3];
                splashMtx[1][3] = body->mBodyGround.getWaterHeight();
                splashMtx[2][3] = body->mPlayerPosMtx[2][3];
                GetJ3DEfctMgr()->setEffectKart(body->mMynum, 1, splashMtx);
                GetKartCtrl()->getKartSound(body->mMynum)->DoKartDiveSound();
                body->getStrat()->DoMotor(MotorManager::MotorType_15);
            } else {
                if (body->getTouchNum() >= 2) {
                    if (dy >= 30.0f) {
                        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)3, body->mMynum, pos, 2);
                        body->getGame()->_08 &= ~1;
                    }
                } else {
                    pos.y = body->mBodyGround.getWaterHeight();
                    JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)7, body->mMynum, pos, 2);
                    body->getGame()->_08 &= ~1;
                }
            }
            if ((body->mCarStatus & 0x400000) != 0) {
                body->mCarStatus &= ~8ull;
                body->mCarStatus &= ~0x40ull;
                body->getGame()->_08 &= ~1;
            }
        } else if (body->mBodyGround.getSplashHeight() > 0.0f) {
            f32 dy = body->mPlayerPosMtx[1][3] - body->mBodyGround.getSplashHeight();
            int splashID = body->mBodyGround.getSplashID();
            if (dy < -100.0f && (body->mCarStatus & 0x400000) == 0) {
                Mtx splashMtx;
                splashMtx[0][3] = body->mPlayerPosMtx[0][3];
                splashMtx[1][3] = body->mBodyGround.getSplashHeight();
                splashMtx[2][3] = body->mPlayerPosMtx[2][3];
                if (splashID == 5) {
                    JPEffectPerformer::setEffectSplash(&body->mBodyGround, body->mMynum, pos);
                    GetKartCtrl()->getKartSound(body->mMynum)->DoKartsetSeSound(0x1005e);
                } else {
                    GetKartCtrl()->getKartSound(body->mMynum)->DoKartDiveSound();
                    GetJ3DEfctMgr()->setEffectSplash(&body->mBodyGround, body->mMynum, splashMtx);
                }
                body->getStrat()->DoMotor(MotorManager::MotorType_15);
            } else {
                if (splashID != 5 && body->getTouchNum() >= 2) {
                    JPEffectPerformer::setEffectSplash(&body->mBodyGround, body->mMynum, pos);
                }
            }
            if ((body->mCarStatus & 0x400000) != 0) {
                body->mCarStatus &= ~8ull;
                body->mCarStatus &= ~0x40ull;
                body->getGame()->_08 &= ~1;
            }
        }
    }
    if (body->getTouchNum() >= 2 && (body->mCarStatus & 0x400000000000ull) != 0) {
        DoMotor(MotorManager::MotorType_19);
        body->mCarStatus &= ~0x400000000000ull;
    }
    if (body->getTouchNum() != 0 && (body->mCarStatus & 0x40) != 0) {
        body->getGame()->_08 &= ~1;
        body->mCarStatus &= ~0x40ull;
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)3, body->mMynum, pos, 2);
        if (body->mVel.y < 0.0f) {
            body->getStrat()->DoPowerMotor(1.0f, 10, 0);
            GetKartCtrl()->getKartSound(body->mMynum)->DoLandingSound(1.0f);
        } else {
            f32 power = body->mVel.y;
            if (power < 0.0f) {
                power = -power;
            }
            power = power / 150.0f;
            f32 power2 = power;
            power = power * 10.0f;
            power += 5.0f;
            if (power > 10.0f) {
                power = 10.0f;
            }
            body->getStrat()->DoPowerMotor(1.0f, (u8)power, 0);
            power2 = power2 * 3.8f;
            if (power2 > 1.0f) {
                power2 = 1.0f;
            }
            GetKartCtrl()->getKartSound(body->mMynum)->DoLandingSound(power2);
        }
    }
}
void KartStrat::DoYawCrl() {
    KartBody *body = mBody;
    if (body->getChecker()->CheckCrash() == 1)
        return;
    KartSus *sus0 = body->getSus(0);
    KartSus *sus1 = body->getSus(1);
    KartSus *sus2 = body->getSus(2);
    KartSus *sus3 = body->getSus(3);
    if (((sus2->_124 & 1) == 0 && (sus3->_124 & 1) == 0) ||
        (sus0->_124 & 1) != 0 || (sus1->_124 & 1) != 0) {
        if (body->_2c0.y >= 0.226777f) {
            body->_2c0.y = 0.226777f;
        }
        if (body->_2c0.y <= 0.174444f) {
            body->_2c0.y = 0.174444f;
        }
    } else {
        if (body->_2c0.y >= -0.174444f) {
            body->_2c0.y = -0.174444f;
        }
        if (body->_2c0.y <= -0.226777f) {
            body->_2c0.y = -0.226777f;
        }
    }
}

void KartStrat::DoSignalCrl() {
    KartBody *body = mBody;
    body->mWg.y = 0.0f;
    body->mVel.x = 0.0f;
    body->mVel.y = 0.0f;
    body->mVel.z = 0.0f;
    body->_2cc.x = 0.0f;
    body->_2cc.y = 0.0f;
    body->_2cc.z = 0.0f;
}

void KartStrat::DoSpeedCrl() {
    KartBody *body = mBody;
    if ((body->mGameStatus & 8) != 0) {
        DoComSpeedCrl();
    } else {
        DoDash();
    }
    f32 f30 = 0.0f;
    if (body->_284.x != 0.0f || body->_284.y != 0.0f || body->_284.z != 0.0f) {
        f30 = 1.0f;
    }
    if (body->_29c.x != 0.0f || body->_29c.y != 0.0f || body->_29c.z != 0.0f) {
        f30 = 1.0f;
    }
    switch (body->_584) {
    case 1:
    case 7:
    case 18:
        DoVelCrl(body->_3f0 / 2.16f / body->mSpeedScale);
        body->_3ec = body->_454;
        break;
    case 2:
    case 3:
        DoVelCrl(body->_3f0 / 2.16f / body->mSpeedScale);
        body->_3ec = body->_454;
        break;
    case 4:
    case 5:
    case 6:
        DoVelCrl(body->_3f0 / 2.16f / body->mSpeedScale);
        body->_3ec = body->_454;
        break;
    case 16:
        DoVelCrl(body->getPipe()->mSpeed / 2.16f / body->mSpeedScale);
        body->_3ec = body->_454;
        break;
    case 17:
        DoVelCrl(87.96296f / body->mSpeedScale);
        body->_3ec = body->_454;
        break;
    case 13:
    case 15:
        DoVelCrl(92.59259f / body->mSpeedScale);
        body->_3ec = body->_454;
        break;
    case 8:
        DoVelCrl(27.777777f / body->mSpeedScale);
        body->_3ec = body->_454;
        break;
    case 9:
        DoVelCrl(body->getRescue()->_88 / 2.16f / body->mSpeedScale);
        body->_3ec = body->_454;
        break;
    case 12:
        DoVelCrl(body->getCannon()->_14 / 2.16f / body->mSpeedScale);
        body->_3ec = body->_454;
        break;
    case 14:
        DoVelCrl(body->getDossin()->_20 / 2.16f / body->mSpeedScale);
        body->_3ec = body->_454;
        break;
    default: {
        f32 f29 = GetKartCtrl()->GetMaxSpeed(body->mMynum);
        f32 f31 = 0.2f * f29 * (body->_3cc / body->_3d8);
        if (f29 > f31 && body->getTouchNum() != 0) {
            f29 -= f31;
        }
        f31 = DoDashCrl(DoStarCrl(f29));
        if (GetKartCtrl()->GetCarSpeed(body->mMynum) >= 40.0f && body->_510 > 2.44222f) {
            f31 = 40.0f;
        }
        KartGamePad *cont = GetKartCtrl()->GetDriveCont(body->mMynum);
        if ((body->mCarStatus & 0x8040) == 0 && f30 == 0.0f && !body->mBodyGround.isShaking()) {
            if (cont->testButton(GetKartCtrl()->getKartPad(body->mMynum)->mTrigZ | GetKartCtrl()->getKartPad(body->mMynum)->mBtnY)) {
                if (GetKartCtrl()->GetCarSpeed(body->mMynum) < 100.0f && body->_3c8 == 0.0f && body->_3cc == 0.0f) {
                    f31 = 0.0f;
                }
            } else {
                if (GetKartCtrl()->GetCarSpeed(body->mMynum) < 40.0f && body->_3c8 == 0.0f && body->_3cc == 0.0f) {
                    f31 = 0.0f;
                }
            }
        }
        f29 = GetKartCtrl()->GetDownSlopeSpeed(body->mMynum);
        if ((body->mCarStatus & 0x1000) != 0 || body->mBodyGround.getAttribute() == 6) {
            GetKartCtrl()->ChaseFnumber(&body->_3ec, f31, 1.0f);
        } else if (GetKartCtrl()->GetCarSpeed(body->mMynum) >= 20.0f && (body->getTouchNum() == 0 || (body->mCarStatus & 0x40) != 0)) {
            GetKartCtrl()->ChaseFnumber(&body->_3ec, 135.0f, 0.1f);
        } else if ((body->mCarStatus & 0x20000048004ull) != 0) {
            GetKartCtrl()->ChaseFnumber(&body->_3ec, f31, 0.2f);
        } else if (body->getTouchNum() == 0 || (body->mCarStatus & 0x40) != 0) {
            GetKartCtrl()->ChaseFnumber(&body->_3ec, 135.0f, 0.26f);
        } else if (f30 != 0.0f) {
            GetKartCtrl()->ChaseFnumber(&body->_3ec, f31, body->_524);
        } else {
            f30 = f31 + f29;
            if (f30 < body->_454) {
                GetKartCtrl()->ChaseFnumber(&body->_3ec, f30, 0.07f);
            } else if (body->_454 < f31 * 0.5f) {
                f30 = GetKartCtrl()->GetDownSlopeAcc(body->mMynum);
                if ((body->mCarStatus & 3) != 0 && (body->mFrame > 0.2f || body->mFrame < -0.2f)) {
                    GetKartCtrl()->ChaseFnumber(&body->_3ec, 0.55f * f31, body->_520 * f30);
                } else if (body->_3ec > body->_454) {
                    body->_3ec = body->_454 + 0.1f;
                } else {
                    GetKartCtrl()->ChaseFnumber(&body->_3ec, 0.55f * f31, body->_520 * f30);
                }
            } else {
                f30 = GetKartCtrl()->GetDownSlopeAcc(body->mMynum);
                if ((body->mCarStatus & 3) != 0) {
                    GetKartCtrl()->ChaseFnumber(&body->_3ec, 3.0f + f31, body->_524 * f30);
                } else if (body->_3ec > body->_454) {
                    body->_3ec = body->_454 + f29 + 0.1f;
                } else {
                    GetKartCtrl()->ChaseFnumber(&body->_3ec, 2.0f + f31 + f29, body->_51c * f30);
                }
            }
        }
        if (body->_3ec > 200.0f) {
            body->_3ec = 200.0f;
        }
        DoVelCrl(body->_3ec / 2.16f / body->mSpeedScale);
        if (body->getTouchNum() == 0) {
            if (body->mVel.y > 300.0f) {
                body->mVel.y = 300.0f;
            } else if (body->mVel.y < -200.0f) {
                body->mVel.y = -200.0f;
            }
        } else {
            if (body->mVel.y > 200.0f) {
                body->mVel.y = 200.0f;
            }
            if (body->mVel.y < -200.0f) {
                body->mVel.y = -200.0f;
            }
        }
        break;
    }
    }
}

void KartStrat::DoComSpeedCrl() {}

void KartStrat::DoCutSlide() {
    KartBody *body = mBody;
    if ((body->mCarStatus & 0x100500007ull) != 0)
        return;
    f32 scale = 0.9f;
    if ((body->mGameStatus & 8) != 0) {
        scale = 0.01f;
    } else {
        scale *= 0.0001f;
    }
    JGeometry::TVec3f vec;
    GetKartCtrl()->DevMatrixByVector(&vec, &body->mVel, body->_110);
    vec.x = vec.x * (1.0f - scale);
    PSMTXMultVecSR(body->_110, &vec, &body->mVel);
}

void KartStrat::DoCalcSpeed() {
    KartBody *body = mBody;
    f32 velX = body->mSpeedScale * body->mVel.x;
    f32 velY = body->mSpeedScale * body->mVel.y;
    f32 velZ = body->mSpeedScale * body->mVel.z;
    body->mSpeed = GetKartCtrl()->SpeedySqrtf(velX * velX + velZ * velZ);
    body->_448 = GetKartCtrl()->SpeedySqrtf(velY * velY + velX * velX + velZ * velZ);
}

void KartStrat::DoMotor(MotorManager::MotorType type) {
    KartBody *body = mBody;
    if (body->getChecker()->CheckPlayer() != 0 && (body->mGameStatus & 0x80) == 0) {
        if (body->getHandle()->DoMotor(type)) {
            // empty
        } else {
            MotorManager::setMotor(type, body->mMynum);
        }
    }
}

void KartStrat::DoPowerMotor(f32 power, u8 p2, u8 p3) {
    KartBody *body = mBody;
    if (body->getChecker()->CheckPlayer() != 0 && (body->mGameStatus & 0x80) == 0) {
        if (!body->getHandle()->DoPowerMotor(power, p2, p3)) {
            MotorManager::setPowerMotor(body->mMynum, power, p2, p3);
        }
    }
}

void KartStrat::DashSpeedCtrl(float scale) { DashSpSpeedCtrl(scale); }

void KartStrat::DashSpSpeedCtrl(f32 speed) {
    KartBody *body = mBody;
    if (body->getTouchNum() == 0) {
        if (body->mVel.y > 0.0f) {
            DoMaxLevelZXVelCrl(speed / 2.16f / body->mSpeedScale, 0.98f);
        } else {
            DoMaxLevelZXVelCrl(speed / 2.16f / body->mSpeedScale, 0.0f);
        }
    } else {
        DoMaxLevelZXVelCrl(speed / 2.16f / body->mSpeedScale, 0.999f);
    }
    if (body->getTouchNum() >= 2 && body->mWg.x > 0.0f) {
        body->mWg.x -= 0.0348888f;
    }
}

bool KartStrat::CompulsionDash(JGeometry::TVec3f *dir) {
    KartBody *body = mBody;
    if (body->getGame()->_04 == nullptr)
        return false;
    JGeometry::TVec3f aimVec;
    aimVec.sub(body->getGame()->_38, body->mPos);
    aimVec.normalize();
    if (aimVec.angle(body->_308) > 1.57f)
        return false;
    JGeometry::TVec3f vecA;
    JGeometry::TVec3f vecB;
    JGeometry::TVec3f vecC;
    JGeometry::TVec3f vecD;
    JGeometry::TVec3f vecE;
    vecA.set(0.0f, 1.0f, 0.0f);
    vecB.cross(vecA, body->_308);
    vecB.normalize();
    vecC.cross(vecB, vecA);
    vecC.normalize();
    vecA.cross(vecC, vecB);
    vecA.normalize();
    Mtx mtx;
    mtx[0][0] = vecB.x;
    mtx[1][0] = vecB.y;
    mtx[2][0] = vecB.z;
    mtx[0][1] = vecA.x;
    mtx[1][1] = vecA.y;
    mtx[2][1] = vecA.z;
    mtx[0][2] = vecC.x;
    mtx[1][2] = vecC.y;
    mtx[2][2] = vecC.z;
    mtx[0][3] = body->mPos.x;
    mtx[1][3] = body->mPos.y;
    mtx[2][3] = body->mPos.z;
    vecE.y = dir->y;
    vecE.z = dir->z;
    vecE.x = 0.0f;
    body->getGame()->_04->getPosition(&vecD);
    f32 dy = body->mPos.y - vecD.y;
    vecD.y = body->mPos.y;
    vecD -= body->mPos;
    if (dy < 0.0f) {
        vecE.y *= 1.1f;
    } else if (dy < 700.0f) {
        vecE.y *= 0.8f;
    } else if (dy < 900.0f) {
        vecE.y *= 0.95f;
    } else if (dy < 1200.0f) {
        vecE.y *= 1.1f;
    } else if (dy > 1600.0f) {
        vecE.y *= 0.9f;
    }
    PSMTXMultVecSR(mtx, &vecE, &body->mVel);
    return true;
}

void KartStrat::DoDash() {
    KartBody *body = mBody;
    u8 num = body->mMynum;
    JGeometry::TVec3f vec;
    f32 vlen = body->_284.length();
    u8 flag = 0;
    f32 maxSpeed;
    if (GetKartCtrl()->GetCarSpeed(body->mMynum) <= 40.0f) {
        if (body->_510 > 2.44222f) {
            flag = 1;
        }
    }
    if ((body->mCarStatus & 0x20000000000ull) != 0) {
        maxSpeed = 1.3f * body->_3f0;
        body->mMTBoost--;
        if (GetKartCtrl()->GetCarSpeed(num) > 50.0f && (body->mCarStatus & 0x8000) == 0 && vlen == 0.0f &&
            (body->mCarStatus & 0x14000400) == 0 && body->getTouchNum() >= 1) {
            if (flag != 0) {
                vec.set(0.0f, 0.0f, 10.0f);
                PSMTXMultVecSR(body->_110, &vec, &body->mVel);
            }
            DashSpeedCtrl(maxSpeed);
        }
        if (body->mMTBoost == 0) {
            body->mCarStatus &= ~0x20000000000ull;
        }
    }
    if (body->_5b5 != 0) {
        body->_5b5--;
    }
    if ((body->mCarStatus & 0x400000000ull) != 0 && body->_5b5 == 0 &&
        GetKartCtrl()->getKartAnime(num)->IsFloatAnime(num) == 1) {
        body->mCarStatus &= ~0x400000000ull;
        GetKartCtrl()->getKartAnime(num)->mFlags |= 0x1000000ull;
    }
    if ((body->mCarStatus & 0x8000) != 0) {
        if ((body->mCarStatus & 0x20004000) != 0) {
            if (body->mClass == 0) {
                maxSpeed = 1.2f * body->_3f0;
            } else {
                maxSpeed = 1.3f * body->_3f0;
            }
        } else {
            if (body->mClass == 0) {
                maxSpeed = 1.55f * body->_3f0;
            } else {
                maxSpeed = 1.45f * body->_3f0;
            }
        }
        if ((body->mCarStatus & 0x10000) != 0) {
            body->mBoostTimer--;
            body->mSpeedScale = body->_52c;
            body->_3c8 = body->_3d0;
            if (vlen == 0.0f && (body->mCarStatus & 0x4000400) == 0 && body->mBoostTimer >= body->_598 - 10 &&
                body->getTouchNum() >= 1) {
                if (flag != 0 || body->mBoostTimer >= body->_598 - 10) {
                    vec.set(0.0f, 1.0f, 10.0f);
                    PSMTXMultVecSR(body->_110, &vec, &body->mVel);
                }
                DashSpeedCtrl(maxSpeed);
            }
        }
        if (body->mBoostTimer == 0) {
            body->mCarStatus |= 0x400000000ull;
            DashClear();
        }
    }
    if ((body->mCarStatus & 0x4000) != 0) {
        body->mBoostTimer--;
        body->mSpeedScale = body->_52c;
        body->_3c8 = body->_3d0;
        if (vlen == 0.0f && (body->mCarStatus & 0x14000400) == 0 && body->mBoostTimer >= 0x46 &&
            (body->mCarStatus & 0x20) == 0 && body->getTouchNum() >= 1) {
            if (flag != 0) {
                vec.set(0.0f, 0.0f, 10.0f);
                PSMTXMultVecSR(body->_110, &vec, &body->mVel);
            }
            DashSpeedCtrl(maxSpeed);
        }
        if (body->mBoostTimer == 0) {
            DashClear();
        }
    }
    if ((body->mCarStatus & 0x20000) != 0) {
        body->mBoostTimer--;
        body->mSpeedScale = body->_52c;
        body->_3c8 = body->_3d0;
        if (vlen == 0.0f && (body->mCarStatus & 0x14000400) == 0 && body->getTouchNum() >= 1) {
            if (flag != 0) {
                vec.set(0.0f, 0.0f, 10.0f);
                PSMTXMultVecSR(body->_110, &vec, &body->mVel);
            }
            DashSpeedCtrl(maxSpeed);
        }
        if (body->mBoostTimer == 0) {
            DashClear();
        }
    }
    if ((body->mCarStatus & 0x20000000) != 0) {
        body->mBoostTimer--;
        body->mSpeedScale = body->_52c;
        body->_3c8 = body->_3d0;
        if (vlen == 0.0f && (body->mCarStatus & 0x14000400) == 0) {
            if (flag != 0) {
                vec.set(0.0f, 0.0f, 10.0f);
                PSMTXMultVecSR(body->_110, &vec, &body->mVel);
            }
            if ((body->mCarStatus & 0x40) != 0) {
                vec.set(0.0f, 49.0f, 65.0f);
                if (CompulsionDash(&vec)) {
                    DashSpeedCtrl(maxSpeed);
                }
            } else {
                DashSpeedCtrl(maxSpeed);
            }
        }
        if (body->mBoostTimer == 0) {
            DashClear();
        }
    }
    if ((body->mCarStatus & 0x400) != 0) {
        body->mBoostTimer--;
        body->mSpeedScale = body->_52c;
        body->_3c8 = body->_3d0;
        if (vlen == 0.0f && (body->mCarStatus & 0x4000400) == 0) {
            if (flag != 0) {
                vec.set(0.0f, 0.0f, 10.0f);
                PSMTXMultVecSR(body->_110, &vec, &body->mVel);
            }
            if ((body->mCarStatus & 0x40) != 0) {
                vec.set(0.0f, 55.0f, 70.0f);
                if (CompulsionDash(&vec)) {
                    DashSpeedCtrl(200.0f);
                }
            } else {
                DashSpSpeedCtrl(200.0f);
            }
        }
        if (body->mBoostTimer == 0) {
            DashClear();
        }
    }
    if (GetKartCtrl()->CheckCamera(num) != 0) {
        int camNum = GetKartCtrl()->GetCameraNum(num);
        if (GetKartCtrl()->getKartCam(camNum)->GetCameraMode() != 0) {
            JPEffectPerformer::deleteKartEfctKoukasen(num, (u8)camNum);
        }
    }
}


int KartStrat::DoStatusCrl() {
    KartBody *kartBody = mBody;
    
    if ((kartBody->mGameStatus & 4) == 0) {
        DoSignalCrl();
        return 2;
    }

    kartBody->mGameStatus = kartBody->mGameStatus & 0xfffffdff;

    switch(kartBody->_584) {
        case 1:
            kartBody->getCrash()->DoSpinCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 2:
        case 3:
            kartBody->getCrash()->DoFreezeCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 4:
            kartBody->getCrash()->DoHalfSpinCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 5:
            kartBody->getCrash()->DotornadeCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 6:
            kartBody->getCrash()->DoRollCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 7:
            kartBody->getCrash()->DoPitchCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 8:
            kartBody->getTumble()->DoAfterTumbleCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 9:
            kartBody->getCrash()->DoBombCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 0xc:
            kartBody->getTumble()->DoShootCrashCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            break;
        case 0xd:
            DoWheelSpinCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            if ((kartBody->mCarStatus & 0x10) == 0) {
                return 3;
            }
            break;
        case 0xe:
            kartBody->getRescue()->DoAfterRescueCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            return 0;
        case 0xf:
            kartBody->getCannon()->DoAfterCannonCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            return 0;
        case 0x10:
            kartBody->getPipe()->DoAfterPipeCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            return 0;
        case 0x11:
            kartBody->getAnt()->DoAfterAntCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            return 0;
        case 0x12:
            kartBody->getDossin()->DoAfterDossinCrl();
            DoAirCrl();
            DoSpeedCrl();
            DoCalcSpeed();
            return 0;
        default:
            DoWallCrl();
            kartBody->DegubBody(0x22);
            if (kartBody->getChecker()->CheckCrash() == 0) {
                DoLiftCrl();
                kartBody->DegubBody(0x23);      // MJB - are these constants part of an enum?
                DoRollCrash();
                kartBody->DegubBody(0x24);
                DoTestPitchCrl();
                DoPitchLim();
                kartBody->DegubBody(0x25);
            }
            DoSpeedCrl();
            kartBody->DegubBody(0x26);
            DoAirCrl();
            kartBody->DegubBody(0x27);
            DoCutSlide();
            kartBody->DegubBody(0x28);
            DoYawCrl();
            kartBody->DegubBody(0x29);
            DoCalcSpeed();
            kartBody->DegubBody(0x2a);
            if ((kartBody->mGameStatus & 0x400) != 0) {
                kartBody->mGameStatus |= 0x200;
                kartBody->mEffctVel.zero();
                return 2;
            }
            if ((kartBody->_29c.x != 0.0f) || (kartBody->_29c.y != 0.0f) || (kartBody->_29c.z != 0.0f)) {
                return 0;
            }
            if ((kartBody->_284.x != 0.0f) || (kartBody->_284.y != 0.0f) || (kartBody->_284.z != 0.0f)) {
                return 0;
            }

            if (((kartBody->getTouchNum() < 3) || (kartBody->_3c8 > 0.0f)) || ((kartBody->_4d4 != 0.0f || ((kartBody->mCarStatus & 0x2100010) != 0)))) {
                return 0;
            }

            if (kartBody->mWg.length() > 0.0872222f) {
                return 0;
            }
            if ((kartBody->mBodyGround.isShaking() != 0) || (kartBody->_58c == 7)) {
                return 0;
            }
            kartBody->DegubBody(0x2b);
            if (GetKartCtrl()->GetCarSpeed(kartBody->mMynum) < 0.052333299f) {
                if (kartBody->getChecker()->CheckCrash() == 0) {
                    if (kartBody->mWg.y < 0.052333299f && kartBody->mWg.y > -0.052333299f) {
                        kartBody->mWg.y = 0.0f;
                    }
                    if (kartBody->mVel.y > 1.0f) {
                        kartBody->mGameStatus = kartBody->mGameStatus | 0x200;
                        return 3;
                    }
                    kartBody->mGameStatus = kartBody->mGameStatus | 0x200;
                    kartBody->mEffctVel.zero();
                    return 2;
                }
            }
            break;
    }
    
    kartBody->DegubBody(0x2c);
    return 0;
}
