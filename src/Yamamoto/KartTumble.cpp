#include "Yamamoto/KartTumble.h"

#include "JSystem/J3D/J3DPacket.h"
#include "Kaneshige/RaceMgr.h"
#include "Sato/GeographyObjMgr.h"
#include "Sato/ItemObjMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Shiraiwa/TKartThrower.h"
#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartCtrl.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

#include "macros.h"

// comments inside functions are inline functions being called in that function

void KartTumble::Init(int idx) {
    this->mBody = GetKartCtrl()->getKartBody(idx);
    _4 = 0;
    _5[0] = 0;
    _14 = 0;
    _8.zero();
}

void KartTumble::MakeWanWanTumble(ItemObj *item) {
    KartBody *body = mBody;
    ItemObjMgr::KartHitList *hitList = GetItemObjMgr()->getKartHitList(body->mMynum);

    if ((_4 & 1) || body->getChecker()->CheckCrash()) {
        return;
    }

    JGeometry::TVec3f dir;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f itemPos;

    body->_584 = 4;
    body->_588 = 0;
    _5[0] = 0;
    hitList->mObjects[9]->getPos(&itemPos);
    body->getGame()->ItemWatchMan(item);
    itemPos.y = body->mPos.y;

    dir.sub(itemPos, body->mPos);
    dir.length();
    body->_308.angle(dir);
    vel.set(dir);
    vel.scale(-1);
    GetKartCtrl()->DevMatrixByVector(&_8, &vel, body->_110);
    _8.y = 0;
    _8.normalize();

    body->_594 = 0;
    body->_4b4 = 0;
    body->_4bc = 0.680333f;
    body->getCrash()->NonRescue();
    body->mCarStatus |= 0x102000;
    body->getCrash()->SaveDir();
    body->getGame()->MakeClear();
    body->getDamage()->SetBigDamageAnime();
    body->getItem()->FallItem();
    body->mVel.zero();
    body->_2cc.zero();
    body->mWg.zero();

    vel.normalize();
    dir.set(vel);
    dir.y = 0;
    dir.scale(80);
    dir.scale(body->_3a4);
    body->DoForce(&body->mPos, &dir);

    _4 |= 1;
    _14 = 60;
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashStartSound();
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashVoice();
    JPEffectPerformer::setEffect(JPEffectPerformer::Effect_Unknown0, body->mMynum, body->mPos, 1);
    body->getStrat()->DoMotor((MotorManager::MotorType)11);
    body->getCrash()->SetMatchlessTimer();
}

DECL_WEAK void ItemObj::getPos(JGeometry::TVec3f *pos) { *pos = mPos; }

void KartTumble::MakeKameTumble(ItemObj *item) {
    KartBody *body = mBody;
    ItemObjMgr::KartHitList *hitList = GetItemObjMgr()->getKartHitList(body->mMynum);

    if ((_4 & 1) || body->getChecker()->CheckCrash()) {
        return;
    }

    JGeometry::TVec3f dir;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f itemPos;

    body->_584 = 4;
    body->_588 = 0;
    _5[0] = 0;
    hitList->mObjects[11]->getPos(&itemPos);
    itemPos.y = body->mPos.y;

    dir.sub(itemPos, body->mPos);
    dir.length();
    body->_308.angle(dir);
    vel.set(dir);
    vel.scale(-1);
    GetKartCtrl()->DevMatrixByVector(&_8, &vel, body->_110);
    _8.y = 0;
    _8.normalize();

    body->_594 = 0;
    body->_4b4 = 0;
    body->_4bc = 0.680333f;
    body->mCarStatus |= 0x102000;
    body->getCrash()->SaveDir();
    body->getGame()->MakeClear();
    body->getDamage()->SetBigDamageAnime();
    body->getItem()->FallItem();
    body->mVel.zero();
    body->_2cc.zero();
    body->mWg.zero();

    vel.normalize();
    dir.set(vel);
    dir.y = 0;
    dir.scale(80);
    dir.scale(body->_3a4);
    body->DoForce(&body->mPos, &dir);

    _4 |= 1;
    _14 = 60;
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashStartSound();
    JPEffectPerformer::setEffect(JPEffectPerformer::Effect_Unknown0, body->mMynum, body->mPos, 1);
    body->getStrat()->DoMotor((MotorManager::MotorType)11);
    body->getGame()->ItemWatchMan(item);
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashVoice();
    body->getCrash()->SetMatchlessTimer();
}

void KartTumble::MakeStarTumble() {
    KartBody *body = mBody;

    if ((_4 & 1) || body->getChecker()->CheckCrash()) {
        return;
    }

    JGeometry::TVec3f dir;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f pos;

    body->_584 = 4;
    body->_588 = 0;
    _5[0] = 0;
    { KartBody *other = (KartBody *)body->mUnkSub10c; pos.set(other->mPos); }
    pos.y = body->mPos.y;

    dir.sub(pos, body->mPos);
    vel.set(dir);
    vel.scale(-1);
    GetKartCtrl()->DevMatrixByVector(&_8, &vel, body->_110);
    _8.y = 0;
    _8.normalize();

    body->_594 = 0;
    body->_4b4 = 0;
    body->_4bc = 0.680333f;
    body->getCrash()->NonRescue();
    body->mCarStatus |= 0x102000;
    body->getCrash()->SaveDir();
    body->getGame()->MakeClear();
    body->getDamage()->SetBigDamageAnime();
    body->getItem()->ReleseWanWan();
    body->mVel.zero();
    body->_2cc.zero();
    body->mWg.zero();

    vel.normalize();
    dir.set(vel);
    dir.y = 0;
    dir.scale(80);
    dir.scale(body->_3a4);
    body->DoForce(&body->mPos, &dir);

    _4 |= 1;
    _14 = 60;
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashStartSound();
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashVoice();
    JPEffectPerformer::setEffect(JPEffectPerformer::Effect_Unknown0, body->mMynum, body->mPos, 1);
    body->getStrat()->DoMotor((MotorManager::MotorType)11);
    body->getCrash()->SetMatchlessTimer();
}

void KartTumble::MakeDashTumble() {
    KartBody *body = mBody;

    JGeometry::TVec3f dir;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f pos;

    pos.set(((KartBody *)body->mUnkSub10c)->mPrevPos);
    dir.sub(pos, body->mPrevPos);
    vel.sub(body->_308, ((KartBody *)body->mUnkSub10c)->_308);

    f32 angle = vel.angle(dir);
    angle = angle * 180 / 3.141f;
    if (angle < 90) {
        return;
    }

    dir.normalize();
    dir.y = 0;
    dir.scale(500);
    dir.scale(body->_3a4);
    body->DoForce(&body->mPos, &dir);
}

void KartTumble::DoTumble() {
    KartBody *body = mBody;
    GeographyObj *obj = GetGeoObjMgr()->getKartReactHitObjectList(body->mMynum)[11];

    if (_4 & 1) {
        return;
    }
    if (obj == NULL) {
        return;
    }

    if (!body->getChecker()->CheckCrash())
        goto body;
    else
        goto end;

body:

    JGeometry::TVec3f dir;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f normal;
    JGeometry::TVec3f pos;

    body->_584 = 4;
    body->_588 = 0;
    _5[0] = 0;
    obj->getPosition(&pos);
    obj->getColScaleRadius();
    GetGeoObjMgr()->getKartHitDepthNormalObj(body->mMynum);
    normal.set(*GetGeoObjMgr()->getKartHitRefVecNormalObj(body->mMynum));
    obj->getVelocity(&vel);

    vel.y = 0;
    pos.y = body->mPos.y;
    dir.sub(pos, body->mPos);
    vel.set(dir);
    vel.scale(-1);
    GetKartCtrl()->DevMatrixByVector(&_8, &vel, body->_110);
    _8.y = 0;
    _8.normalize();

    body->_594 = 0;
    body->_4b4 = 0;
    body->_4bc = 0.680333f;
    body->mCarStatus |= 0x102000;
    body->getCrash()->SaveDir();
    body->getGame()->MakeClear();
    body->getDamage()->SetBigDamageAnime();
    body->getItem()->FallItem();
    body->ObjectReflection(&normal);
    body->mVel.zero();
    body->_2cc.zero();
    body->mWg.zero();

    vel.normalize();
    dir.set(vel);
    dir.y = 0;
    dir.scale(80);
    dir.scale(body->_3a4);
    body->DoForce(&body->mPos, &dir);

    _4 |= 1;
    _14 = 300;
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashStartSound();
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashVoice();
    JPEffectPerformer::setEffect(JPEffectPerformer::Effect_Unknown0, body->mMynum, body->mPos, 1);
    body->getStrat()->DoMotor((MotorManager::MotorType)11);
    body->getCrash()->SetMatchlessTimer();

end:;
}

void KartTumble::DoPakunTumble() {
    KartBody *body = mBody;
    GeographyObj *obj = GetGeoObjMgr()->getKartReactHitObjectList(body->mMynum)[18];

    if (_4 & 1) {
        return;
    }
    if (obj == NULL) {
        return;
    }

    if (!body->getChecker()->CheckCrash()) {

    JGeometry::TVec3f throwDir;
    JGeometry::TVec3f dir;
    JGeometry::TVec3f normal;
    JGeometry::TVec3f pos;

    body->_584 = 4;
    body->_588 = 0;
    _5[0] = 0;
    JPEffectPerformer::setEffect(JPEffectPerformer::Effect_Unknown6, body->mMynum, body->mPos, 0);
    obj->getPosition(&pos);
    obj->getColScaleRadius();
    GetGeoObjMgr()->getKartHitDepthNormalObj(body->mMynum);
    normal.set(*GetGeoObjMgr()->getKartHitRefVecNormalObj(body->mMynum));
    obj->getKartThrowDirPow(&throwDir, NULL, body->mMynum);

    dir.set(throwDir);
    GetKartCtrl()->DevMatrixByVector(&_8, &dir, body->_110);
    _8.y = 0;
    dir.normalize();

    body->_594 = 0;
    body->_4b4 = 0;
    body->_4bc = 0.680333f;
    body->mCarStatus |= 0x102000;
    body->getCrash()->SaveDir();
    body->getGame()->MakeClear();
    body->getDamage()->SetBigDamageAnime();
    body->getItem()->FallItem();
    body->mVel.zero();
    body->_2cc.zero();
    body->mWg.zero();

    throwDir.y = 0;
    throwDir.normalize();
    throwDir.scale(body->_3a4);
    body->DoForce(&body->mPos, &throwDir);

    _4 |= 1;
    _14 = 120;
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashStartSound();
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashVoice();
    JPEffectPerformer::setEffect(JPEffectPerformer::Effect_Unknown0, body->mMynum, body->mPos, 1);
    body->getStrat()->DoMotor((MotorManager::MotorType)11);
    body->getCrash()->SetMatchlessTimer();
    }
}

void KartTumble::DoHanaTumble() {
    KartBody *body = mBody;
    GeographyObj *obj = GetGeoObjMgr()->getKartReactHitObjectList(body->mMynum)[15];

    if (_4 & 1) {
        return;
    }
    if (obj == NULL) {
        return;
    }

    if (!body->getChecker()->CheckCrash()) {

    JGeometry::TVec3f dir;
    JGeometry::TVec3f vel;
    JGeometry::TVec3f normal;
    JGeometry::TVec3f pos;

    body->_584 = 4;
    body->_588 = 0;
    _5[0] = 0;
    JPEffectPerformer::setEffect(JPEffectPerformer::Effect_Unknown6, body->mMynum, body->mPos, 0);
    obj->getPosition(&pos);
    obj->getColScaleRadius();
    GetGeoObjMgr()->getKartHitDepthNormalObj(body->mMynum);
    normal.set(*GetGeoObjMgr()->getKartHitRefVecNormalObj(body->mMynum));
    obj->getVelocity(&vel);

    vel.y = 0;
    pos.y = body->mPos.y;
    dir.sub(pos, body->mPos);
    vel.set(dir);
    vel.scale(-1);
    GetKartCtrl()->DevMatrixByVector(&_8, &vel, body->_110);
    _8.y = 0;
    _8.normalize();

    body->_594 = 0;
    body->_4b4 = 0;
    body->_4bc = 0.680333f;
    body->mCarStatus |= 0x102000;
    body->getCrash()->SaveDir();
    body->getGame()->MakeClear();
    body->getDamage()->SetBigDamageAnime();
    body->getItem()->FallItem();
    body->ObjectReflection(&normal);
    body->mVel.zero();
    body->_2cc.zero();
    body->mWg.zero();

    vel.normalize();
    dir.set(vel);
    dir.y = 0;
    dir.scale(80);
    dir.scale(body->_3a4);
    body->DoForce(&body->mPos, &dir);

    _4 |= 1;
    _14 = 120;
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashStartSound();
    GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashVoice();
    JPEffectPerformer::setEffect(JPEffectPerformer::Effect_Unknown0, body->mMynum, body->mPos, 1);
    body->getStrat()->DoMotor((MotorManager::MotorType)11);
    body->getCrash()->SetMatchlessTimer();
        } else {
        return;
    }
}

void KartTumble::MakePoiHanaTumble() {
    KartBody *body = mBody;
    GeographyObj *obj = GetGeoObjMgr()->getKartReactHitObjectList(body->mMynum)[14];

    if (_4 & 1) {
        return;
    }
    if (obj == NULL) {
        return;
    }

    if (!body->getChecker()->CheckCrash()) {
        JGeometry::TVec3f throwDir;
        JGeometry::TVec3f dir;
        JGeometry::TVec3f normal;
        JGeometry::TVec3f pos;

        body->_584 = 13;
        body->_588 = 0;
        _5[0] = 0;
        obj->getPosition(&pos);
        obj->getColScaleRadius();
        GetGeoObjMgr()->getKartHitDepthNormalObj(body->mMynum);
        normal.set(*GetGeoObjMgr()->getKartHitRefVecNormalObj(body->mMynum));
        obj->getVelocity(&dir);

        dir.y = 0;
        pos.y = body->mPos.y;
        throwDir.sub(pos, body->mPos);
        throwDir.length();
        body->_308.angle(throwDir);
        dir.set(throwDir);
        dir.scale(-1);
        GetKartCtrl()->DevMatrixByVector(&_8, &dir, body->_110);
        _8.y = 0;
        _8.normalize();

        body->_594 = 0;
        body->_4b4 = 0;
        body->_4bc = 0.191888f;
        body->mCarStatus |= 0x800100000ULL;
        body->getCrash()->SaveDir();
        body->getGame()->MakeClear();
        body->getDamage()->SetBigDamageAnime();
        body->getItem()->FallItem();
        body->mVel.zero();
        body->_2cc.zero();
        body->mWg.zero();

        JGeometry::TVec3f tmp;
        f32 rate = ((TKartThrower *)obj)->getThrowPow();
        rate *= 5.0f;
        rate *= body->_3a4;
        ((TKartThrower *)obj)->getThrowDir(&tmp, body->mMynum);
        body->_2cc.x += rate * tmp.x;
        body->_2cc.y += rate * tmp.y;
        body->_2cc.z += rate * tmp.z;

        _4 |= 1;
        _14 = 120;
        if (obj->getKind() == 0xd4a) {
            GetKartCtrl()->getKartSound(body->mMynum)->DoShootSound();
        }
        GetKartCtrl()->getKartSound(body->mMynum)->DoShootVoice();
        JPEffectPerformer::setEffect(JPEffectPerformer::Effect_Unknown0, body->mMynum, body->mPos, 1);
        body->getStrat()->DoMotor((MotorManager::MotorType)12);
        body->getCrash()->SetMatchlessTimer();
    }
}

void KartTumble::DoShootCrashCrl() {
    KartBody *body = mBody;

    if ((body->mCarStatus & 0x800000000ULL) == 0) {
        GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashSound();
    }

    switch (body->_588) {
    case 0:
        body->_594++;
        if (body->_594 < 80 && body->_594 > 20 && body->getTouchNum() >= 3) {
            body->_588 = 1;
        } else if (body->_594 >= 80) {
            body->_588 = 1;
        }

        GetKartCtrl()->ChaseFnumber(&body->_4b4, body->_4bc, 0.8f);

        if ((_4 & 0x400ULL) == 0) {
            if (body->_594 < 10) {
                body->mVel.y += 10;
            }
            if (body->_594 <= 20) {
                body->mWg.scale(0.4f);
                body->mWg.x += body->_4b4 * _8.z;
                body->mWg.z += body->_4b4 * -_8.x;
            }
        }
            break;
    case 1:
        break;
    }
}

void KartTumble::DoTumbleCrl() {
    if (_14 != 0) {
        _14--;
    } else {
        _4 &= 0xfe;
    }
}

void KartTumble::DoAfterTumbleCrl() {
    KartBody *body = mBody;

    if ((body->mCarStatus & 0x800000000ULL) == 0) {
        GetKartCtrl()->getKartSound(body->mMynum)->DoRollCrashSound();
    }

    switch (body->_588) {
    case 0:
        body->_594++;
        if (body->_594 < 80 && body->_594 > 20 && body->getTouchNum() >= 3) {
            body->_588 = 1;
        } else if (body->_594 >= 80) {
            body->_588 = 1;
        }

        GetKartCtrl()->ChaseFnumber(&body->_4b4, body->_4bc, 0.8f);

        if ((_4 & 0x400ULL) == 0) {
            if (body->_594 <= 20) {
                body->mWg.scale(0.4f);
                body->mWg.x += body->_4b4 * _8.z;
                body->mWg.z += body->_4b4 * -_8.x;
            }
        }
        break;
    case 1:
        break;
    }
}
