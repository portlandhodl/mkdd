#include "Yamamoto/kartSus.h"
#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCtrl.h"
#include "Yamamoto/kartLine.h"
#include "Yamamoto/kartParams.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

// comments inside functions are inline functions being called in that function

void KartSus::InitTestTirePose(int idx) {
    // UNUSED, fabricated (needed for symbol order)
    JGeometry::TVec3f pos;

    pos.set(_128[0][3] + _104, _128[1][3] - mTireRadius - _b0, _128[2][3]);
    PSMTXMultVec(getKartBody()->_110, &pos, &_c0);
    _cc.set(_c0);
    _d8.set(_c0);
    mCrsGnd.reset();
    mCrsGnd.search(_c0, _c0);
    _120 = mCrsGnd.getHeight();
    mCrsGnd.getNormal(&_e4);
    if (mCrsGnd.isObject()) {
        _2c8 = mCrsGnd.getObject()->getKind();
    } else {
        _2c8 = 0;
    }

    pos.set(_128[0][3] + _104, 0.0f, _128[2][3]);
    PSMTXMultVec(getKartBody()->_110, &pos, &_c0);
    _c0.y = _120;

    int kind = getKartBody()->mIdx;
    if (idx == 1 || idx == 0) {
        _c0.x += _e4.x * (mTireRadius - _b0 * BodyOpData[kind]->_8);
        _c0.y += _e4.y * (mTireRadius - _b0 * BodyOpData[kind]->_8);
        _c0.z += _e4.z * (mTireRadius - _b0 * BodyOpData[kind]->_8);
    } else {
        _c0.x += _e4.x * (mTireRadius - _b0 * BodyOpData[kind]->_c);
        _c0.y += _e4.y * (mTireRadius - _b0 * BodyOpData[kind]->_c);
        _c0.z += _e4.z * (mTireRadius - _b0 * BodyOpData[kind]->_c);
    }

    _284.set(0.0f, 1.0f, 0.0f);
    _78[0].set(_c0);
    _78[1].set(_c0);
    _78[2].set(_c0);
    _78[3].set(_c0);
}

void KartSus::InitTirePose(int idx) {
    JGeometry::TVec3f pos;

    pos.set(_128[0][3] + _104, _128[1][3] - mTireRadius - _b0, _128[2][3]);
    PSMTXMultVec(getKartBody()->_110, &pos, &_c0);
    _cc.set(_c0);
    mCrsGnd.reset();
    mCrsGnd.search(_c0, _c0);
    _120 = mCrsGnd.getHeight();
    mCrsGnd.getNormal(&_e4);
    if (mCrsGnd.isObject()) {
        _2c8 = mCrsGnd.getObject()->getKind();
    } else {
        _2c8 = 0;
    }

    pos.set(_128[0][3] + _104, 0.0f, _128[2][3]);
    PSMTXMultVec(getKartBody()->_110, &pos, &_c0);
    _c0.y = _120;

    int kind = getKartBody()->mIdx;
    if (idx == 1 || idx == 0) {
        _c0.x += _e4.x * (mTireRadius - _b0 * BodyOpData[kind]->_8);
        _c0.y += _e4.y * (mTireRadius - _b0 * BodyOpData[kind]->_8);
        _c0.z += _e4.z * (mTireRadius - _b0 * BodyOpData[kind]->_8);
    } else {
        _c0.x += _e4.x * (mTireRadius - _b0 * BodyOpData[kind]->_c);
        _c0.y += _e4.y * (mTireRadius - _b0 * BodyOpData[kind]->_c);
        _c0.z += _e4.z * (mTireRadius - _b0 * BodyOpData[kind]->_c);
    }

    _284.set(0.0f, 1.0f, 0.0f);
    _78[0].set(_c0);
    _78[1].set(_c0);
    _78[2].set(_c0);
    _78[3].set(_c0);
}

void KartSus::InitSettingParam(int idx) {
    getKartBody();
    getKartBody();
    if (idx == 0 || idx == 1) {
        _294 = 26.0f;
        _b0 = SusParamsData[getKartBody()->mIdx]->_30;
        _2c0 = SusParamsData[getKartBody()->mIdx]->_34;
    } else {
        _294 = 26.0f;
        _b0 = SusParamsData[getKartBody()->mIdx]->_38;
        _2bc = SusParamsData[getKartBody()->mIdx]->_3c;
    }
    _114 = 0.97f;
}

void KartSus::InitParam(int idx) {
    JGeometry::TVec3f pos;
    Mtx m2 = { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f } };

    KartCtrl *ctrl = GetKartCtrl();
    int kind = getKartBody()->mIdx;

    _108 = 0.0f;
    _10c = 0.0f;
    _110 = 0.0f;
    _124 = 0;
    _280 = 1.0f;
    _2a0 = 0.0f;
    _278 = 0.0f;

    mWheel = mLoader->getExModelWheel(idx);
    mArm = mLoader->getExModelArm(idx);
    mShock = mLoader->getExModelShock(idx);

    InitSettingParam(idx);

    if (idx == 0 || idx == 1) {
        _a8 = SusParamsData[kind]->_0;
        _ac = SusParamsData[kind]->_4;
        pos.set(TireOpData[kind][idx]._10, _2c0, TireOpData[kind][idx]._14);
    } else {
        _a8 = SusParamsData[kind]->_8;
        _ac = SusParamsData[kind]->_c;
        pos.set(TireOpData[kind][idx]._10, _2bc, TireOpData[kind][idx]._14);
    }

    _298 = TireParamsData[kind]->_1c;
    _29c = TireParamsData[kind]->_20;
    mTireRadius = TireOpData[kind][idx]._8;
    _104 = TireOpData[kind][idx]._c;
    _100 = 1.2f * TireOpData[kind][2]._8;
    _b4 = _b0;
    _b8 = _b4;
    _118 = SusParamsData[kind]->_10;
    _11c = SusParamsData[kind]->_14;
    _2a8 = SusParamsData[kind]->_1c;
    _2ac = SusParamsData[kind]->_20;
    _2b0 = SusParamsData[kind]->_24;
    _2b8 = SusParamsData[kind]->_2c;
    _2b4 = SusParamsData[kind]->_28;

    Mtx m3 = { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f } };
    if (idx == 1 || idx == 3) {
        GetKartCtrl()->RotZMatrix(m3, 0.174444f);
    } else {
        GetKartCtrl()->RotZMatrix(m3, -0.174444f);
    }
    GetKartCtrl()->SetPosePosMatrix(_128, m3, &pos);
    GetKartCtrl()->SetPosePosMatrix(_188, m2, &pos);

    _27c = TireParamsData[kind]->_18;
    _290 = SusParamsData[kind]->_18;
    pos.set(ArmOpData[kind][idx]._0, ArmOpData[kind][idx]._4, ArmOpData[kind][idx]._8);
    GetKartCtrl()->SetPosePosMatrix(_1b8, m2, &pos);
    pos.set(DumpOpData[kind][idx]._0, DumpOpData[kind][idx]._4, DumpOpData[kind][idx]._8);
    GetKartCtrl()->SetPosePosMatrix(_218, m2, &pos);
}

void KartSus::Init(int idx) {
    Mtx m = { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f } };

    _a8 = 0.0f;
    _ac = 0.0f;
    _b0 = 0.0f;
    _b4 = 0.0f;
    _bc = 0.0f;
    mTireRadius = 0.0f;
    _100 = 0.0f;
    _104 = 0.0f;
    _108 = 0.0f;
    _10c = 0.0f;
    _110 = 0.0f;
    _114 = 0.0f;
    _278 = 0.0f;
    _27c = 0.0f;
    _280 = 0.0f;
    _118 = 0.0f;
    _11c = 0.0f;
    _290 = 0.0f;
    _294 = 0.0f;
    _298 = 0.0f;
    _29c = 0.0f;
    _120 = 0.0f;
    _2a8 = 0.0f;
    _2ac = 0.0f;
    _2b0 = 0.0f;
    _2b8 = 0.0f;
    _2b4 = 0.0f;
    _2c4 = 0.0f;
    _124 = 0;
    _2c0 = 0.0f;
    _2bc = 0.0f;
    _284.zero();
    _c0.zero();
    _cc.zero();
    _d8.zero();
    _e4.zero();
    _f0.zero();
    _2a4 = 0xff;

    GetKartCtrl()->SetPosePosMatrix(_128, m, &_c0);
    GetKartCtrl()->SetPosePosMatrix(_158, m, &_c0);
    GetKartCtrl()->SetPosePosMatrix(_188, m, &_c0);

    InitParam(idx);
    InitTirePose(idx);
}

void KartSus::NormalInit(int idx) {
    mSpline = new Spline();
    Init(idx);
}

void KartSus::ResetInit(int idx) { Init(idx); }

void KartSus::WallReflection(JGeometry::TVec3f *normal, JGeometry::TVec3f *vel, f32 bound, f32 fric) {
    // UNUSED, fabricated (needed for symbol order)
    JGeometry::TVec3f force;
    JGeometry::TVec3f tangent;

    f32 d = vel->dot(*normal);
    if (d >= 0.0f)
        return;

    force.scale(-d * (1.5f + bound), *normal);
    tangent.scale(d, *normal);
    tangent.sub(*vel, tangent);
    tangent.scale(-fric * 0.8f);
    force.add(tangent);
    force.scale(0.6f * getKartBody()->_3a4);
    getKartBody()->DoForce(&_c0, &force);
}

void KartSus::CircleFriction(JGeometry::TVec3f *vec, f32 max) {
    f32 len = vec->length();
    if (0.0f != len && len > max) {
        vec->scale(max / len);
    }
}

f32 KartSus::GetCornerForce() {
    if (getKartBody()->mCarStatus & 3)
        return 250.0f;
    return 280.0f;
}

f32 KartSus::GetCircleFric() {
    f32 fric;
    switch (getKartBody()->mBodyGround.getAttribute()) {
    case 1:
        fric = getKartBody()->_530;
        break;
    case 0:
    case 3:
    case 11:
    case 12:
    case 17:
        fric = getKartBody()->_534;
        break;
    case 4:
        fric = getKartBody()->_538;
        break;
    default:
        fric = getKartBody()->_530;
        break;
    }

    if (_2a4 == 6 || getKartBody()->_58c == 3) {
        fric = 0.0f;
    } else if (getKartBody()->mCarStatus & 0x40020074000ull) {
        fric = getKartBody()->_530;
    }

    if (fric > 1.0f)
        fric = getKartBody()->_530;
    return fric;
}

void KartSus::DoSusAction(int idx) {
    JGeometry::TVec3f pos;
    JGeometry::TVec3f force;
    JGeometry::TVec3f diff;
    JGeometry::TVec3f tirePos;

    if (getKartBody()->getThunder()->mFlags & 1) {
        pos.set((_128[0][3] + _104) * getKartBody()->getThunder()->getScale(), _128[1][3],
                _128[2][3] * getKartBody()->getThunder()->getScale());
    } else {
        pos.set(_128[0][3] + _104, _128[1][3], _128[2][3]);
    }
    PSMTXMultVec(getKartBody()->_110, &pos, &_c0);
    _bc = 0.0f;
    _124 &= ~4;

    f32 radius = mTireRadius;
    if (getKartBody()->getThunder()->mFlags & 1) {
        pos.set((_128[0][3] + _104) * getKartBody()->getThunder()->getScale(), _128[1][3] - radius - _b4,
                _128[2][3] * getKartBody()->getThunder()->getScale());
    } else {
        pos.set(_128[0][3] + _104, _128[1][3] - radius - _b4, _128[2][3]);
    }
    PSMTXMultVec(getKartBody()->_110, &pos, &tirePos);
    mCrsGnd.search(tirePos, _cc);

    JGeometry::TVec3f bezier;
    bool noGround = false;
    switch (mCrsGnd.getAttribute()) {
    case 2:
    case 18:
        noGround = true;
        break;
    default:
        _120 = mCrsGnd.getHeight();
        mCrsGnd.getNormal(&_e4);
        _cc.set(tirePos);
        _78[3].set(tirePos.x, _120, tirePos.z);
        mSpline->setAll(_78);
        mSpline->getBezierPoint(&bezier, 0.95f);
        _120 = bezier.y;
        if (_120 > _c0.y)
            _c0.y = _120;
        break;
    }

    _284.set(tirePos.x, _120, tirePos.z);
    _78[0].set(_78[1]);
    _78[1].set(_78[2]);
    _78[2].set(_78[3]);

    if (mCrsGnd.isObject()) {
        _2c8 = mCrsGnd.getObject()->getKind();
    } else {
        _2c8 = 0;
    }

    if (noGround || mCrsGnd.getAttribute() == 10 || getKartBody()->_2fc.y < 0.0f ||
        (getKartBody()->_2fc.y < 0.1f && !(getKartBody()->mCarStatus & 0x100000ull)) ||
        getKartBody()->getRescue()->mFlags & 0x20 || getKartBody()->getCannon()->mFlags & 0x20) {
        _b4 = _b0;
        _124 &= ~1;
        return;
    }

    pos.set(tirePos.x, _120, tirePos.z);
    diff.sub(pos, _c0);
    f32 len = diff.length();
    f32 stroke = len - radius;
    if (stroke > _b0) {
        stroke = _b4;
        GetKartCtrl()->ChaseFnumber(&stroke, _b0, 0.35f);
        if (stroke > _b0)
            stroke = _b0;
        _124 &= ~1;
    } else {
        _124 |= 1;
        if (stroke < 0.0f)
            stroke = 0.0f;

        f32 compress = stroke - _b0;
        f32 damper;
        if (stroke == _b0) {
            damper = 0.0f;
        } else {
            f32 vel = stroke - _b4;
            if (vel >= 0.0f) {
                f32 rate = 1.0f - vel / _2b8;
                if (rate <= 0.1f)
                    rate = 0.1f;
                damper = vel * (_11c * rate);
            } else {
                f32 rate = 1.0f - vel / -_2b8;
                if (rate <= 0.1f)
                    rate = 0.1f;
                damper = vel * (_118 * rate);
            }
        }

        f32 spring = _a8;
        f32 rate = 1.0f - (stroke / (_b0 * _2a8) - 1.0f);
        if (rate > 1.0f)
            rate *= 0.98f;
        if (rate > _2ac)
            rate = _2ac;
        if (rate < _2b0)
            rate = _2b0;
        spring *= rate;

        f32 power = -spring * compress * (2e-5f * getKartBody()->_3a4) - _ac * damper * (2.5e-5f * getKartBody()->_3a4);
        power *= _294 * SusPowerData[getKartBody()->mIdx];
        if (power >= 95.0f * getKartBody()->_3a4)
            power = 95.0f * getKartBody()->_3a4;
        if (!(getKartBody()->mGameStatus & 4))
            power = getKartBody()->_3a4;

        if (getKartBody()->mCarStatus & 3) {
            force.set(0.2f * (power * getKartBody()->_2fc.x), power * getKartBody()->_2fc.y,
                      0.2f * (power * getKartBody()->_2fc.z));
        } else {
            force.set(0.73f * (power * getKartBody()->_2fc.x), power * getKartBody()->_2fc.y,
                      0.73f * (power * getKartBody()->_2fc.z));
        }
        _d8.set(_c0.x - len * getKartBody()->_2fc.x, _c0.y - len * getKartBody()->_2fc.y,
                _c0.z - len * getKartBody()->_2fc.z);
        getKartBody()->DoForce(&_d8, &force);
        _bc = power;
    }

    _b4 = stroke;
    force.set(0.0f, _290 * getKartBody()->_3ac, 0.0f);
    getKartBody()->DoForce(&_c0, &force);
    _2a4 = 0xff;
}

void KartSus::DoTireAction() {
    JGeometry::TVec3f force;
    JGeometry::TVec3f side;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f roll;
    JGeometry::TVec3f tmp;
    JGeometry::TVec3f prevVel;
    Mtx m = { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f } };

    if (mCrsGnd.getAttribute() == 2)
        return;
    if (0.0f == _bc)
        return;

    if (getKartBody()->mCarStatus & 0x202000ull) {
        getKartBody()->GroundReflection(&_d8, &_e4, getKartBody()->_430, getKartBody()->_42c, 0.5f);
        return;
    }

    force.zero();
    GetKartCtrl()->MulMatrix(m, _128, getKartBody()->_110);
    tmp.set(m[0][0], m[1][0], m[2][0]);
    side.cross(tmp, _e4);
    side.normalize();
    roll.set(side.x * (3.0f * (-200.0f * _110)), side.y * (3.0f * (-200.0f * _110)), side.z * (3.0f * (-200.0f * _110)));
    tmp.set(_d8.x - getKartBody()->mPos.x, _d8.y - getKartBody()->mPos.y, _d8.z - getKartBody()->mPos.z);
    tmp.scale(5.0f * _27c);
    if (getKartBody()->_458 <= 30.0f && !(getKartBody()->mCarStatus & 3)) {
        GetKartCtrl()->ChaseFnumber(&_280, 0.4f, 0.2f);
        tmp.scale(_280);
    } else {
        _280 = 1.0f;
    }

    JGeometry::TVec3f wg;
    wg.set(getKartBody()->_2c0);
    if (getKartBody()->mBodyGround.getSpiralCode() == 1) {
        wg.scale(0.221f);
    } else {
        wg.scale(0.15f);
    }
    tmp.cross(wg, tmp);
    vel.add(getKartBody()->mVel, tmp);
    vel.add(roll);
    GetKartCtrl()->VectorElement(&tmp, &vel, &_e4);
    if (getKartBody()->mBodyGround.getSpiralCode() == 1) {
        tmp.scale(2.3f);
    } else {
        tmp.scale(2.15f);
    }
    vel.sub(tmp);
    prevVel.set(vel);

    f32 cornerForce = GetCornerForce();
    tmp.cross(vel, side);
    tmp.cross(tmp, side);
    tmp.scale(_bc * cornerForce);
    vel.scale(-_bc);
    force.add(vel, tmp);
    _2a4 = getKartBody()->mBodyGround.getAttribute();
    f32 fric = GetCircleFric();
    CircleFriction(&force, fric * _bc);
    getKartBody()->DoForce(&_d8, &force);
    f32 d = force.dot(side);
    _110 -= 4.0f * (d / getKartBody()->_3a4);
}
