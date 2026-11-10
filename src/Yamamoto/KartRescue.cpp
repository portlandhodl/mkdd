#include "Yamamoto/KartRescue.h"
#include "Kameda/WipeManager.h"
#include "Kaneshige/Course/Course.h"
#include "Kaneshige/RaceMgr.h"
#include "Yamamoto/KartCtrl.h"
#include "Yamamoto/kartBody.h"
#include "Yamamoto/kartParams.h"
#include "Kaneshige/KartChecker.h"
#include "Sato/GeographyObjMgr.h"
#include "Shiraiwa/JugemMain.h"
#include "Kaneshige/KartDrawer.h"
#include "Sato/J3DEffectMgr.h"
#include "Sato/JPEffectPerformer.h"
#include "Sato/stEffectMgr.h"
#include "Yamamoto/KartAnime.h"
#include "Yamamoto/KartCrash.h"
#include "Yamamoto/KartDamage.h"
#include "Yamamoto/KartGame.h"
#include "Yamamoto/KartItem.h"
#include "Yamamoto/KartSound.h"
#include "Yamamoto/KartThunder.h"
#include "Yamamoto/kartCamera.h"
#include "Yamamoto/KartDemoCam.h"
#include "Yamamoto/KartMat.h"
#include "Yamamoto/KartTarget.h"
#include "Yamamoto/KartCheck.h"
#include "Kaneshige/ViewCtrlModel.h"
#include "Kaneshige/CenterLine.h"
#include "Shiraiwa/Objects/MapObjAntLion.h"
#include "Sato/RivalKart.h"
#include "Sato/RivalBodyCtrl.h"
#include "JSystem/JUtility/TColor.h"

#include "JSystem/JAudio/JASFakeMatch2.h"

// comments inside functions are inline functions being called in that function

void KartRescue::Init(int idx) {
    mFlags = 0;
    _76 = 0;
    _77 = 0;
    _78 = 1;
    _80 = 0.0f;
    _84 = 0.0f;
    _8c.zero();
    _98.zero();
    _a4.zero();
    _b0.zero();
    _bc.zero();
    _c8.zero();
    mBody = GetKartCtrl()->getKartBody(idx);
    _8 = 0;
}

void KartRescue::DoKeep() {
    KartBody *body = mBody;
    f32 zero = 0.0f;
    body->_2cc.x = zero;
    body->_2cc.y = zero;
    body->_2cc.z = zero;
    body->mVel.x = zero;
    body->mVel.y = zero;
    body->mVel.z = zero;
    body->mWg.zero();
    body->_4d8 = zero;
    body->_4d4 = zero;
    body->_4c4 = zero;
    body->_4c0 = zero;
    body->_3cc = zero;
    body->mTireAngle = zero;
    body->_394 = zero;
    body->mLiftframe = zero;
    body->getDossin()->mVelocity.zero();
    body->_2b4.zero();
    body->_29c.zero();
    body->_284.zero();
}

void KartRescue::GetJugemuStatus() {
    KartBody *body = mBody;
    if (_75 == 4) {
        Course *course = RaceMgr::getManager()->getCourse();
        mJugemPoint = course->searchJugemPoint(mObj->getObjData()->mParam2);
    } else {
        mJugemPoint = body->mBodyGround.getJugemPoint();
    }
    mGround = body->mBodyGround;
}

void KartRescue::SetPose(u8 kind) {
    KartBody *body = mBody;
    JGeometry::TVec3f pos;
    Mtx rot = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    JGeometry::TVec3f dir;
    if (kind == 0) {
        dir.set(body->_308);
        pos.set(body->mPos);
        if (_75 == 6) {
            RaceMgr::getManager()->getStartPoint(&_bc, &_c8, body->mMynum);
            _bc.y += 1800.0f;
        } else if (mJugemPoint) {
            mJugemPoint->getPosition(&_bc);
            mJugemPoint->getFrDirection(&_c8);
            _bc.y += 1800.0f;
        }
    } else {
        if (mJugemPoint || _75 == 6) {
            pos.set(_bc);
            dir.set(_c8);
        } else {
            dir.set(body->_308);
            pos.set(body->mPos);
            pos.y = 1800.0f + body->mBodyGround.getHeight();
        }
        GetKartCtrl()->SetPosePosMatrix(body->_110, rot, &pos);
        GetKartCtrl()->SetPosePosMatrix(body->mPlayerPosMtx, rot, &pos);
        JGeometry::TVec3f a;
        JGeometry::TVec3f b;
        JGeometry::TVec3f c;
        a.set(0.0f, 1.0f, 0.0f);
        b.cross(a, dir);
        b.normalize();
        c.cross(b, a);
        c.normalize();
        a.cross(c, b);
        a.normalize();
        body->_110[0][0] = b.x;
        body->_110[1][0] = b.y;
        body->_110[2][0] = b.z;
        body->_110[0][1] = a.x;
        body->_110[1][1] = a.y;
        body->_110[2][1] = a.z;
        body->_110[0][2] = c.x;
        body->_110[1][2] = c.y;
        body->_110[2][2] = c.z;
        body->_110[0][3] = pos.x;
        body->_110[1][3] = pos.y;
        body->_110[2][3] = pos.z;
        if (mJugemPoint) {
            JGeometry::TVec3f offs;
            JGeometry::TVec3f res;
            int num = body->mMynum;
            if (body->mGameStatus & 0x10) {
                num = 0;
            }
            int half = num / 2;
            f32 z = -250.0f * (f32)half;
            f32 x = (num % 2) ? 100.0f : -100.0f;
            offs.set(x, 0.0f, z);
            PSMTXMultVecSR(body->_110, &offs, &res);
            pos.add(res);
            body->_110[0][3] = pos.x;
            body->_110[1][3] = pos.y;
            body->_110[2][3] = pos.z;
        }
    }
    body->mPos.set(pos);
    PSMTXCopy(body->_110, body->mPlayerPosMtx);
    f32 offset = GetKartCtrl()->GetKartBodyOffset(body->mMynum);
    body->mPlayerPosMtx[0][3] += body->mPlayerPosMtx[0][1] * offset;
    body->mPlayerPosMtx[1][3] += body->mPlayerPosMtx[1][1] * offset;
    body->mPlayerPosMtx[2][3] += body->mPlayerPosMtx[2][1] * offset;
    body->_2f0.set(body->_110[0][0], body->_110[1][0], body->_110[2][0]);
    body->_2fc.set(body->_110[0][1], body->_110[1][1], body->_110[2][1]);
    body->_308.set(body->_110[0][2], body->_110[1][2], body->_110[2][2]);
}

bool KartRescue::SetJugemuPose(int idx, KartBody *body) {
    TJugem *jugem = GetGeoObjMgr()->getJugem(idx);
    if (!jugem) {
        return false;
    }
    JGeometry::TVec3f pos;
    JGeometry::TVec3f offs;
    offs.set(0.0f, _98.y, _98.z);
    PSMTXMultVec(body->mPlayerPosMtx, &offs, &pos);
    JGeometry::TVec3f local;
    local.set(pos);
    _d4.mMtx[0][3] = local.x;
    _d4.mMtx[1][3] = local.y;
    _d4.mMtx[2][3] = local.z;
    jugem->setPosition(local);
    jugem->setRMtx(_d4);
    return true;
}

void KartRescue::DoMove(KartBody *body) {
    JGeometry::TVec3f offs;
    JGeometry::TVec3f pos;
    JGeometry::TVec3f accel;
    body->_2cc.zero();
    body->mVel.zero();
    offs.set(0.0f, _98.y, _98.z);
    PSMTXMultVec(body->mPlayerPosMtx, &offs, &pos);
    accel.zero();
    accel.y = _80 * body->_3a4;
    body->DoForce(&pos, &accel);
    body->_110[0][3] = _a4.x;
    body->_110[1][3] = _a4.y;
    body->_110[2][3] = _a4.z;
    body->mPlayerPosMtx[0][3] = _a4.x;
    body->mPlayerPosMtx[1][3] = _a4.y;
    body->mPlayerPosMtx[2][3] = _a4.z;
    body->mPos.set(_a4);
    f32 offset = GetKartCtrl()->GetKartBodyOffset(body->mMynum);
    body->mPlayerPosMtx[0][3] += body->mPlayerPosMtx[0][1] * offset;
    body->mPlayerPosMtx[1][3] += body->mPlayerPosMtx[1][1] * offset;
    body->mPlayerPosMtx[2][3] += body->mPlayerPosMtx[2][1] * offset;
    body->_2d8.scale(-250.0f);
    body->_2cc.zero();
}

void KartRescue::SetHookPos(KartBody *body) {
    JGeometry::TVec3f pos;
    pos.set(_98);
    PSMTXMultVec(body->mPlayerPosMtx, &pos, &_a4);
}

void KartRescue::DoCatch() {
    KartBody *body = mBody;
    _80 = 1.0f;
    _84 = 0.0f;
    mFlags = 0;
    SetPose(0);
    _98.set(Jdata[body->mIdx * 3], Jdata[body->mIdx * 3 + 1], Jdata[body->mIdx * 3 + 2]);
    SetHookPos(body);
    if (mJugemPoint != 0) {
        KartChecker *checker = RCMGetKartChecker(body->mMynum);
        checker->beginWarp(mJugemPoint);
    }
}

void KartRescue::DoTop() {
    KartBody *body = mBody;
    mFlags |= 0x20;
    SetPose(1);
    _80 = 1.0f;
    body->getThunder()->ClearThunder();
    _a4.set(body->mPos);
    if (GetKartCtrl()->CheckCamera(body->mMynum)) {
        int cam = GetKartCtrl()->GetCameraNum(body->mMynum);
        _d4.mMtx[0][0] = body->_110[0][0];
        _d4.mMtx[1][0] = body->_110[1][0];
        _d4.mMtx[2][0] = body->_110[2][0];
        _d4.mMtx[0][1] = body->_110[0][1];
        _d4.mMtx[1][1] = body->_110[1][1];
        _d4.mMtx[2][1] = body->_110[2][1];
        _d4.mMtx[0][2] = body->_110[0][2];
        _d4.mMtx[1][2] = body->_110[1][2];
        _d4.mMtx[2][2] = body->_110[2][2];
        _d4.mMtx[0][3] = 0.0f;
        _d4.mMtx[1][3] = 0.0f;
        _d4.mMtx[2][3] = 0.0f;
        Mtx rot;
        GetKartCtrl()->RotYMatrix(rot, 3.1415927f);
        PSMTXConcat(_d4.mMtx, rot, _d4.mMtx);
        SetJugemuPose(cam, body);
        if (GetKartCtrl()->getKartCam(cam)->GetCameraMode() == 2) {
            GetKartCtrl()->getKartCam(cam)->GetDemoCam()->InitID();
        } else {
            GetKartCtrl()->getKartCam(cam)->InitDropRescueView();
            GetKartCtrl()->getKartCam(cam)->SetFlag(0x10);
        }
    }
    GetKartCtrl()->getKartAnime(body->mMynum)->mFlags |= 0x20000;
}

bool KartRescue::DoDrop() {
    KartBody *body = mBody;
    TJugem *jugem = nullptr;
    if (GetKartCtrl()->CheckCamera(body->mMynum)) {
        int cam = GetKartCtrl()->GetCameraNum(body->mMynum);
        if (GetKartCtrl()->getKartCam(cam)->GetCameraMode() != 2) {
            GetKartCtrl()->getKartCam(cam)->SetFlag(0x10);
        }
        if (SetJugemuPose(cam, body)) {
            jugem = GetGeoObjMgr()->getJugem(cam);
        }
    }
    _a4.y -= 12.0f;
    DoMove(body);
    f32 dist = body->mPlayerPosMtx[1][3] - body->mBodyGround.getHeight();
    if (dist < 600.0f && _77 >= 5) {
        if (jugem) {
            jugem->cutFishLine();
        }
        return true;
    }
    return false;
}

bool KartRescue::DoCheckEnd() {
    KartBody *body = mBody;
    if (body->getTouchNum() <= 2) {
        return false;
    }
    body->getCrash()->DoDecBalloon();
    if (mEffect) {
        mEffect->setTrigger();
        RaceMgr::getManager()->getKartDrawer(body->mMynum)->stopTevAnm();
        GetKartCtrl()->getKartSound(body->mMynum)->DoKartsetSeSound(0x1004c);
        int num = body->mMynum;
        GetKartCtrl()->getKartAnime(num)->DoReStopAnime(num);
        GetStEfctMgr()->setExhaustStopFlg(body->mMynum, false);
    }
    JPEffectPerformer::deleteKartEfctWet(body->mMynum);
    body->mCarStatus &= (u64)(s32)0xffffbfff;
    body->_584 = 0;
    _76 = 0;
    if (GetKartCtrl()->CheckCamera(body->mMynum)) {
        int cam = GetKartCtrl()->GetCameraNum(body->mMynum);
        if (GetKartCtrl()->getKartCam(cam)->GetCameraMode() != 2) {
            GetKartCtrl()->getKartCam(cam)->ClrFlag(0x18);
        }
    }
    body->getDamage()->SetDriverEndDamageAnime();
    GetKartCtrl()->getKartAnime(body->mMynum)->mFlags |= 0x40000;
    mFlags = 0;
    JGeometry::TVec3f vel;
    vel.set(0.0f, 0.0f, 5.0f);
    if (body->getGame()->_08 & 2) {
        if (body->_3c8 != 0.0f) {
            if (body->_3c8 <= 0.3f * body->_3d0) {
                body->getGame()->MakeCrashDash();
                vel.set(0.0f, -60.0f, 30.0f);
            }
        }
    }
    body->getGame()->_08 &= ~2;
    PSMTXMultVecSR(body->_110, &vel, &body->mVel);
    return true;
}

void KartRescue::MakeRescue(GeographyObj *obj, u8 kind) {
    u8 num = mBody->mMynum;
    KartBody *body = mBody;
    _75 = kind;
    if (body->mCarStatus & 0x40000) {
        body->getStar()->StopStar();
    }
    body->getItem()->KillWanWan();
    body->mCarStatus &= 0xfffe7ffffffffffcULL;
    body->getGame()->MakeClear();
    body->mCarStatus = 0;
    body->_584 = 9;
    _76 = 0;
    _77 = 0;
    mFlags = 0;
    _88 = GetKartCtrl()->GetCarSpeed(num);
    body->mCarStatus |= 0x400000;
    body->mCarStatus &= 0xfffffbf7de451fffULL;
    body->getDamage()->ClrAllDamage();
    if (GetKartCtrl()->getKartAnime(num)->IsDrive(num)) {
        GetKartCtrl()->getKartAnime(num)->mDamageFlags |= 0x40000;
    }
    mObj = (TMapObjAntLion *)obj;
    Init(num);
    GetJugemuStatus();
    body->getItem()->FallAllItem();
    body->getGame()->DoItmCancel();
    body->getCrash()->DoRabbitMark();
    body->_5c3 &= ~2;
    if (_75 == 3) {
        JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)39, body->mMynum, JGeometry::TVec3f(0.0f, 0.0f, 0.0f), 0);
        GetKartCtrl()->getKartSound(body->mMynum)->DoKartsetSeSound(0x10081);
        GetKartCtrl()->getKartSound(body->mMynum)->DoAllVoice(4);
    }
    if (_75 == 3 || _75 == 4) {
        if (!GetKartCtrl()->getKartAnime(num)->IsDangerPlusAnime(num)) {
            if (!GetKartCtrl()->getKartAnime(num)->IsDamage(num)) {
                if (body->_460 >= 0.0f) {
                    GetKartCtrl()->getKartAnime(num)->mFlags |= 0x4000000;
                } else {
                    GetKartCtrl()->getKartAnime(num)->mFlags |= 0x2000000;
                }
                GetKartCtrl()->getKartSound(num)->DoStaggerVoice();
            }
        }
    }
    if (_75 == 5) {
        GetKartCtrl()->getKartSound(body->mMynum)->DoAllVoice(4);
    }
}

void KartRescue::DoRescueCrl() {
    KartBody *body = mBody;
    for (u32 i = 0; (int)i < 4; i++) {
        KartSus *sus = GetKartCtrl()->getKartSus(body->mMynum * 4 + i);
        GetKartCtrl()->SetTireDispRound(body, sus, 0.0f);
    }
    switch (_76) {
    case 0:
        DoCatch();
        _77 = 0;
        _76 = 1;
        if (_75 == 4 || _75 == 3) {
            break;
        }
        if (GetKartCtrl()->CheckCamera(body->mMynum) == 1) {
            if (_75 == 5) {
                GETWipeManager()->setWipe(body->mMynum, 16, 0, 30, 70, JUTColor(0, 0, 0, 255));
            } else if (_75 == 1) {
                GETWipeManager()->setWipe(body->mMynum, 18, 0, 35, 65, JUTColor(0, 0, 0, 255));
            } else if (body->mBodyGround.getSplashHeight() > 0.0f &&
                       body->mPlayerPosMtx[1][3] - body->mBodyGround.getSplashHeight() < 0.0f) {
                GETWipeManager()->setWipe(body->mMynum, 16, 0, 30, 70, JUTColor(0, 0, 0, 255));
            } else {
                GETWipeManager()->setWipe(body->mMynum, 14, 0, 30, 70, JUTColor(0, 0, 0, 255));
            }
        }
        if (_75 == 4 || _75 == 3) {
            _76 = 1;
        } else {
            _76 = 2;
        }
        break;
    case 1:
        if (_75 == 4) {
            body->mShadowModel->clrVisibleAll();
            _88 = GetKartCtrl()->fcnvge(_88, 1.0f, 100.0f, 100.0f);
            if (_88 >= 30.0f) {
                _88 = 30.0f;
            }
            _77++;
            if (_77 == 25) {
                mObj->eat();
            }
            if (_77 == 75) {
                if (GetKartCtrl()->CheckCamera(body->mMynum) == 1) {
                    GETWipeManager()->setWipe(body->mMynum, 10, 0, 30, 70, JUTColor(0, 0, 0, 255));
                }
                _77 = 0;
                _76 = 2;
            }
        } else {
            _88 = GetKartCtrl()->fcnvge(_88, 0.0f, 3.0f, 3.0f);
            _77++;
            if (_77 == 100) {
                if (GetKartCtrl()->CheckCamera(body->mMynum) == 1) {
                    GETWipeManager()->setWipe(body->mMynum, 12, 0, 30, 70, JUTColor(0, 0, 0, 255));
                }
                _77 = 0;
                _76 = 2;
            }
        }
        break;
    case 2:
        _77++;
        if (_75 == 0) {
            _88 = GetKartCtrl()->fcnvge(_88, 0.0f, 50.0f, 50.0f);
        } else if (_75 == 5) {
            _88 = GetKartCtrl()->fcnvge(_88, 0.0f, 1.0f, 1.0f);
        } else if (_75 == 3) {
            _88 = GetKartCtrl()->fcnvge(_88, 0.0f, 5.0f, 5.0f);
        } else if (_75 == 2) {
            _88 = 0.0f;
        } else if (_75 == 4) {
            _88 = 0.0f;
        } else if (_75 == 6) {
            _88 = 0.0f;
        }
        if (_88 > 100.0f) {
            _88 = GetKartCtrl()->fcnvge(_88, 0.0f, 60.0f, 60.0f);
        } else if (_88 > 50.0f) {
            _88 = GetKartCtrl()->fcnvge(_88, 0.0f, 30.0f, 30.0f);
        }
        if (_77 == 100) {
            if (GetKartCtrl()->CheckCamera(body->mMynum)) {
                if (_75 == 4) {
                    GETWipeManager()->setWipe(body->mMynum, 13, 2, 28, 0, JUTColor(0, 0, 0, 255));
                } else if (_75 == 3) {
                    GETWipeManager()->setWipe(body->mMynum, 11, 2, 28, 0, JUTColor(0, 0, 0, 255));
                } else if (_75 == 5) {
                    GETWipeManager()->setWipe(body->mMynum, 15, 2, 28, 0, JUTColor(0, 0, 0, 255));
                } else if (_75 == 1) {
                    GETWipeManager()->setWipe(body->mMynum, 17, 2, 28, 0, JUTColor(0, 0, 0, 255));
                } else if (body->mBodyGround.getSplashHeight() > 0.0f &&
                           body->mPlayerPosMtx[1][3] - body->mBodyGround.getSplashHeight() < 0.0f) {
                    GETWipeManager()->setWipe(body->mMynum, 15, 2, 28, 0, JUTColor(0, 0, 0, 255));
                } else {
                    GETWipeManager()->setWipe(body->mMynum, 13, 2, 28, 0, JUTColor(0, 0, 0, 255));
                }
            }
            _7c = 0;
            mEffect = 0;
            if (body->getChecker()->CheckPlayer() && RaceMgr::getManager()->getCourse()->isFreeze()) {
                mEffect = GetJ3DEfctMgr()->setEffectKart(body->mMynum, 3, body->_110);
                RaceMgr::getManager()->getKartDrawer(body->mMynum)->playTevAnm(1);
                u8 num = body->mMynum;
                GetKartCtrl()->getKartAnime(num)->DoStopAnime(num);
                GetStEfctMgr()->setExhaustStopFlg(body->mMynum, true);
            } else {
                if (_75 == 5) {
                    _7c = 2;
                } else if (_75 == 3) {
                    JPEffectPerformer::deleteKartEfctTireSand(body->mMynum);
                } else if (_75 == 4) {
                    body->mShadowModel->setVisibleAll();
                } else if (body->mBodyGround.getSplashHeight() > 0.0f &&
                           body->mPlayerPosMtx[1][3] - body->mBodyGround.getSplashHeight() < 0.0f) {
                    _7c = body->mBodyGround.getSplashID();
                }
            }
            _88 = 100.0f;
            if (mJugemPoint) {
                CLPoint *cl = mJugemPoint->getCLPoint();
                GetKartCtrl()->getKartTarget(body->mMynum)->DoSearchRescueTarget(cl);
                if (body->mGameStatus & 8) {
                    GetKartCtrl()->getKartEnemy(body->mMynum)->getBodyCtrl()->comebackRescure(cl, false);
                }
            } else if (!GetKartCtrl()->IsMiniGame() && _75 == 6) {
                CLPoint *cl = RaceMgr::getCurrentManager()->getManager()->getCourse()->getFirstCLPoint(0);
                GetKartCtrl()->getKartTarget(body->mMynum)->DoSearchRescueTarget(cl);
                if (body->mGameStatus & 8) {
                    GetKartCtrl()->getKartEnemy(body->mMynum)->getBodyCtrl()->comebackRescure(cl, false);
                }
            }
            body->mBodyGround.reset();
            DoTop();
            u8 num = body->mMynum;
            GetKartCtrl()->getKartTarget(num)->DoSearchTarget(num);
            _76 = 3;
            if (mJugemPoint) {
                RCMGetKartChecker(body->mMynum)->endWarp();
            }
            body->getCrash()->DoDecPointBomb();
            _77 = 0;
        }
        break;
    case 3:
        if (_77 == 0) {
            switch (_7c) {
            case 1:
            case 2:
            case 6:
                JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)14, body->mMynum, body->mPos, 0);
                break;
            case 3:
            case 4:
                JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)14, body->mMynum, body->mPos, 1);
                break;
            case 5:
                JPEffectPerformer::setEffect((JPEffectPerformer::EffectType)15, body->mMynum, body->mPos, 1);
                break;
            }
        }
        if (_77 <= 60) {
            _77++;
        }
        if (_77 <= 10) {
            DoKeep();
        } else if (DoDrop() == 1) {
            _76 = 4;
            _77 = 0;
            mFlags &= ~0x40;
            body->mCarStatus &= 0xffffffff8000ffffULL;
            mFlags &= ~0x20;
        }
        break;
    case 4:
        if (_77 <= 100) {
            _77++;
        }
        DoCheckEnd();
        break;
    }
}

void KartRescue::DoAfterRescueCrl() {
    KartBody *body = mBody;
    switch (_76) {
    case 0:
        body->getItem()->FallAllItem();
        body->getGame()->DoItmCancel();
        if (_75 == 2 || _75 == 4) {
            DoKeep();
            body->_3c8 = 0.0f;
        }
        break;
    case 1:
        if (_75 == 2 || _75 == 4) {
            DoKeep();
            body->_3c8 = 0.0f;
        }
        if (_75 == 4) {
            JGeometry::TVec3f dir;
            mObj->getPosition(&dir);
            dir.sub(body->mPos);
            dir.normalize();
            f32 inv = 1.0f / body->mSpeedScale;
            body->mVel.y = inv;
            body->mVel.x = dir.x * (0.15f / body->mSpeedScale);
            body->mVel.z = dir.z * (0.15f / body->mSpeedScale);
        }
        body->mWg.y = 0.0f;
        body->getItem()->FallAllItem();
        body->getGame()->DoItmCancel();
        break;
    case 2:
        if (_75 == 2 || _75 == 4) {
            DoKeep();
            body->_3c8 = 0.0f;
        }
        body->mWg.y = 0.0f;
        body->getItem()->FallAllItem();
        body->getGame()->DoItmCancel();
        break;
    case 3:
        DoKeep();
        body->getItem()->FallAllItem();
        body->getGame()->DoItmCancel();
        break;
    }
}
