#include "Inagaki/GameAudioAnimator.h"
#include "Inagaki/GameAudioCamera.h"
#include "Inagaki/GameAudioCommon.h"
#include "Inagaki/GameAudioMain.h"
#include "Inagaki/GameSoundMgr.h"
#include "Inagaki/GameSoundTable.h"
#include "JSystem/JAudio/Interface/JAISound.h"
#include "JSystem/JAudio/Interface/JAISoundHandles.h"
#include "JSystem/JAudio/JAUSoundObject.h"
#include "JSystem/JAudio/System/JASTrack.h"
#include "JSystem/JGeometry/Vec.h"
#include "JSystem/JUtility/JUTAssert.h"
#include "kartEnums.h"
#include "types.h"

namespace GameAudio {

u32 BoundSe[0x31];
u32 WheelSpinSe[0x19];
u32 SpinSe[0x19];
u32 SpinTurnSe[0x19];

static const bool sSeTableInit = (
    BoundSe[0] = 0x10046, BoundSe[1] = 0x10045, BoundSe[2] = 0x10049, BoundSe[3] = 0x10048,
    BoundSe[4] = 0x10045, BoundSe[5] = 0x10045, BoundSe[6] = 0x10045, BoundSe[7] = 0x10045,
    BoundSe[8] = 0x10045, BoundSe[9] = 0x10045, BoundSe[10] = 0x10045, BoundSe[11] = 0x1004a,
    BoundSe[12] = 0x10047, BoundSe[13] = 0x10045, BoundSe[14] = 0x10045, BoundSe[15] = 0x1004a,
    BoundSe[16] = 0x10045, BoundSe[17] = 0x10045, BoundSe[18] = 0x10045, BoundSe[19] = 0x10045,
    BoundSe[20] = 0x10045, BoundSe[21] = 0x10045, BoundSe[22] = 0x10045, BoundSe[23] = 0x10045,

    WheelSpinSe[0] = 0x1006f, WheelSpinSe[1] = 0x1006e, WheelSpinSe[2] = 0x1006e, WheelSpinSe[3] = 0x10072,
    WheelSpinSe[4] = 0x10071, WheelSpinSe[5] = 0x1006e, WheelSpinSe[6] = 0x1006e, WheelSpinSe[7] = 0x1006e,
    WheelSpinSe[8] = 0x1006e, WheelSpinSe[9] = 0x1006e, WheelSpinSe[10] = 0x10074, WheelSpinSe[11] = 0x1006e,
    WheelSpinSe[12] = 0x1006e, WheelSpinSe[13] = 0x1006e, WheelSpinSe[14] = 0x10074, WheelSpinSe[15] = 0x1006e,
    WheelSpinSe[16] = 0x10075, WheelSpinSe[17] = 0x1006e, WheelSpinSe[18] = 0x1006e, WheelSpinSe[19] = 0x10070,
    WheelSpinSe[20] = 0x10070, WheelSpinSe[21] = 0x10073, WheelSpinSe[22] = 0x10076, WheelSpinSe[23] = 0x1006e,
    WheelSpinSe[24] = 0x1006e,

    SpinSe[0] = 0x1009e, SpinSe[1] = 0x1009d, SpinSe[2] = 0x100a6, SpinSe[3] = 0x100a1,
    SpinSe[4] = 0x100a0, SpinSe[5] = 0x100b0, SpinSe[6] = 0x100af, SpinSe[7] = 0x100af,
    SpinSe[8] = 0x100af, SpinSe[9] = 0x100af, SpinSe[10] = 0x100a7, SpinSe[11] = 0x100a9,
    SpinSe[12] = 0x100ad, SpinSe[13] = 0x100ab, SpinSe[14] = 0x100aa, SpinSe[15] = 0x100a3,
    SpinSe[16] = 0x100ac, SpinSe[17] = 0x100a4, SpinSe[18] = 0x1009e, SpinSe[19] = 0x100a8,
    SpinSe[20] = 0x1009f, SpinSe[21] = 0x1009f, SpinSe[22] = 0x100a2, SpinSe[23] = 0x100a5,
    SpinSe[24] = 0x1009d,

    SpinTurnSe[0] = 0x100b6, SpinTurnSe[1] = 0x100b5, SpinTurnSe[2] = 0x100be, SpinTurnSe[3] = 0x100b9,
    SpinTurnSe[4] = 0x100b8, SpinTurnSe[5] = 0x100c8, SpinTurnSe[6] = 0x100c7, SpinTurnSe[7] = 0x100c7,
    SpinTurnSe[8] = 0x100c7, SpinTurnSe[9] = 0x100c7, SpinTurnSe[10] = 0x100bf, SpinTurnSe[11] = 0x100c1,
    SpinTurnSe[12] = 0x100c5, SpinTurnSe[13] = 0x100c3, SpinTurnSe[14] = 0x100c2, SpinTurnSe[15] = 0x100bb,
    SpinTurnSe[16] = 0x100c4, SpinTurnSe[17] = 0x100bc, SpinTurnSe[18] = 0x100b6, SpinTurnSe[19] = 0x100c0,
    SpinTurnSe[20] = 0x100b7, SpinTurnSe[21] = 0x100b7, SpinTurnSe[22] = 0x100ba, SpinTurnSe[23] = 0x100bd,
    SpinTurnSe[24] = 0x100b5
);

f32 GA_ENEMY_VOLUME_DOWN_VALUE = 0.85f;

const f32 EngineKeisuuRaceUp[] = {
    0.006f, 0.006f, 0.005f, 0.003f,
    0.015f, 0.015f, 0.013f, 0.011f,
    0.02f,
};

const f32 EngineKeisuuRaceDown[] = {
    0.005f, 0.003f, 0.005f, 0.002f,
    0.012f, 0.012f, 0.012f, 0.009f,
    0.019f,
};

static const f32 DashEngineAdjustInitialValue[] = {
    0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f,
};

static const f32 DashEngineIncPerFrame[] = {
    0.025f, 0.025f, 0.025f, 0.095f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f,
};

static const f32 DashEngineDecPerFrame[] = {
    0.005f, 0.005f, 0.005f, 0.005f, 0.005f, 0.005f, 0.005f, 0.005f, 0.005f,
};

static const s16 SignalUpPitchWidth[] = { 0x64, 0x82, 0x78 };
static const s16 SignalUpPitchBase[] = { 0x32, 0x46, 0x50 };
static const s16 SignalUpIntervalWidth[] = { 0x46, 0x1e, 0xa };
static const s16 SignalUpIntervalBase[] = { 0x14, 0xa, 0x5 };
static const s16 SignalDownIntervalWidth[] = { 0x50, 0x32, 0x14 };
static const s16 SignalDownIntervalBase[] = { 0x14, 0xa, 0x5 };

static const u8 cKartRankClassTable0[7] = { 0, 0, 1, 1, 1, 2, 2 };
static const u8 cKartRankClassTable1[7] = { 0, 0, 1, 1, 2, 2, 2 };

u8 KartSoundMgr::smKartCount;
u8 KartSoundMgr::smEntryKartCount;
u8 KartSoundMgr::smGoalKartCount;

u8 KartSoundMgr::smKartRankClassMem[7] = {};

KartSoundMgr::KartSoundMgr(Vec *pos, JKRHeap *heap, u8 p3, u8 p4) : SoundMgr(pos, heap, 12) {
    _114 = 1.0f;
    _110 = 1.0f;
    _11c = 0;
    _124 = 1.0f;
    _120 = 1.0f;
    _12c = 0;
    _C = 0xff;
    _61 = p3;

    if (p4 == 0) {
        mKartCount = smKartCount++;
    }
    else {
        mKartCount = 4;
    }

    if (p4 != 2) {
        smEntryKartCount++;
    }

    _66 = p4;
    mArea = new CrsArea();

    u8 i = 0;
    u8 klass;
    if (_66 == 0) {
        _64 = 3;
    }
    else {
        klass = (u8)(Random::getSignalEngineRandomU32() % 7);
        while (smKartRankClassMem[klass] == 1 && i < 7) {
            klass++;
            i++;
            if (klass == 7) {
                klass = 0;
            }
        }
        smKartRankClassMem[klass] = 1;

        if (Parameters::getPlayerMode() != 1) {
            _64 = cKartRankClassTable0[klass];
        }
        else {
            _64 = cKartRankClassTable1[klass];
        }
    }

    init();
    _7c = 0;

    CameraMgr *camera = Main::getAudio()->getCamera();
    u32 sceneMax = camera->getSceneMax();
    if (sceneMax > 1 && sceneMax > mKartCount) {
        _7c = (1 << mKartCount) ^ 0xf;
    }
}

KartSoundMgr::~KartSoundMgr() {
    setChibiFlag(false, false);

    for (u8 i = 0; i < 7; i++) {
        smKartRankClassMem[i] = 0;
    }

    if (_66 == 0 && (int)mKartCount < 4) {
        CustomAudience<4>::smCameraVolume[(int)mKartCount] = 1.0f;
    }

    smKartCount = 0;
    smEntryKartCount = 0;
    smGoalKartCount = 0;
}

void KartSoundMgr::startSoundHandleNumber(u8 handleNum, u32 id, u32 fadeFrames) {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }

    JAISoundStarter *starter = JAISoundStarter::getInstance();
    JAISoundHandles *handles = this;
    JAISoundHandle *handle = &(*handles)[handleNum];
    starter->startSound(id, handle, nullptr);

    if (*handle) {
        if (!(*handle)->getAudible()) {
            u32 mask = 0;
            CameraMgr *camera = Main::getAudio()->getCamera();
            u32 sceneMax = camera->getSceneMax();
            if (sceneMax > 1 && sceneMax > mKartCount) {
                mask = (1 << mKartCount) ^ 0xf;
            }
            (*handle)->newAudible(JGeometry::TVec3f(*mSoundPos), &_18, mask, nullptr);
        }

        JAISound *sound = handle->operator->();
        sound->fader_.mIntensity = 0.0f;
        if (fadeFrames != 0) {
            sound->fader_.mTransition.set(1.0f, sound->fader_.mIntensity, fadeFrames);
        }
        else {
            sound->fader_.forceIn();
        }
    }

    setEcho(handle, _6c);
}

void KartSoundMgr::dispose() {
    JAUSoundObject::dispose();
    clearInvincibleBgm(3);
}

void KartSoundMgr::init() {
    _60 = 0;
    _5d = 0;
    _68 = 0.0f;
    _63 = 0;
    _5c = 0;
    _6c = 0.0f;
    _70 = 0.0f;
    _80 = 0;
    _84 = 0.0f;
    _88 = 0.0f;
    _8c = 0;
    _8d = 0;
    _8e = 0;
    _90 = 0;
    _92 = 0;
    _94 = 0;
    _96 = 0;
    _98 = 0.0f;
    _9c = 0;
    _a0 = 0.0f;

    for (u8 i = 0; i < 4; i++) {
        _a4[i] = 0.0f;
        _b4[i] = 0.0f;
        _c4[i] = 0.0f;
        _e4[i] = 0;
        mWaterDepth[i] = 0.0f;
        _e8[i] = 0;
    }

    _ec = 0.0f;
    _f0 = 0.0f;
    _f4 = 0;
    _f8 = 0;
    _100 = 0;
    _101 = 0;
    _102 = 0;
    _103 = 0;
    _104 = 0;
    _105 = 1;
    _fc = 0.0f;
    _5f = 0;
    _114 = 0.0f;
    _110 = 0.0f;
    _11c = 0;
    _124 = 1.0f;
    _120 = 1.0f;
    _12c = 0;
    _78 = 0;
    _5e = 0;
    _74 = 0;
    _65 = 0xff;

    if (_66 == 0) {
        f32 v = _124;
        if ((int)mKartCount < 4) {
            CustomAudience<4>::smCameraVolume[(int)mKartCount] = v;
        }
    }

    CustomSoundTable *soundTable = Main::getAudio()->getSoundTable();
    for (int i = 0; i < _10; i++) {
        JAISoundHandles *handles = this;
        if ((*handles)[i] && (soundTable->getSwBit((*handles)[i]->getID()) & 0x800000)) {
            (*handles)[i]->stop();
        }
    }

    setChibiFlag(false, false);
    clearInvincibleBgm(3);
    smGoalKartCount = 0;
}

void KartSoundMgr::changeDriver(bool) {}

void KartSoundMgr::frameWork(u8 rank) {
    if (_66 == 0) {
        mArea->search(6, *mSoundPos);
        if (mArea->isInside()) {
            switch (Parameters::getRaceCourse()) {
            case WALUIGI_STADIUM:
                if ((u8)(rank) <= 3 && !(*this)[7]) {
                    startSoundHandleNumber(7, 0x40024, 0);
                }
                break;
            case MUSHROOM_CITY:
                _5c = 1;
                _6c = 0.5f;
                break;
            }
        }
        else {
            switch (Parameters::getRaceCourse()) {
            case MUSHROOM_CITY:
                _5c = 0;
                break;
            }
        }
    }

    checkEcho();

    if (mKillSw != 0 || _66 == 2) {
        return;
    }

    crushRenzokuTaisaku();
    slipParamSet();
    if (_63 != 3) {
        _65 = rank;
    }
    checkAfterGoalVolume();
}

void KartSoundMgr::checkAfterGoalVolume() {
    if (_66 != 0) {
        return;
    }
    if (_5e != 1) {
        return;
    }
    if (_78 == 0) {
        JAISoundHandles *handles = this;
        JAISoundHandle *handle = &(*handles)[3];
        if (*handle) {
            (*handle)->stop();
        }
        if (Main::getAudio()->getCamera()->getSceneMax() == 2 && (u8)Parameters::getRaceMode() == TIME_ATTACK) {
            _7c = 12;
            _80 = 120;
        }
    }
    else if (_78 == 60) {
        if (_12c == 0 && 0.35f == _124) {
        }
        else if (_12c != 0 && 0.35f == _120) {
        }
        else {
            _120 = 0.35f;
            _128 = (_124 - _120) / 61.0f;
            _12c = 61;
        }
    }
    else if (_78 == 360) {
        if (_12c == 0 && 0.0f == _124) {
        }
        else if (_12c != 0 && 0.0f == _120) {
        }
        else {
            _120 = 0.0f;
            _128 = (_124 - _120) / 301.0f;
            _12c = 301;
        }
    }

    _78++;

    u8 changed;
    if (_12c == 0) {
        changed = 0;
    }
    else if (--_12c == 0) {
        _124 = _120;
        changed = 1;
    }
    else {
        _124 = _124 - _128;
        changed = 1;
    }

    if (changed && (int)mKartCount < 4) {
        CustomAudience<4>::smCameraVolume[(int)mKartCount] = _124;
    }
}

void KartSoundMgr::setWaterDepth(u8 idx, f32 depth) { mWaterDepth[idx] = depth; }

void KartSoundMgr::setSlip(u8 wheelIdx, u8 p2, u8 p3, f32 depth) {
#line 907
    JUT_ASSERT_MSG(wheelIdx < 4, "KartSoundMgr::setSlip wheel ERROR!!!\n");

    _e8[wheelIdx] = p2;

    if (mKillSw != 0 || _66 == 2) {
        return;
    }
    if (_66 != 0) {
        return;
    }

    f32 f30 = 0.0f;
    f32 f31 = 1.0f;
    f32 f29;

    u8 sel;
    switch (p2) {
    case 0x11:
        if (mWaterDepth[wheelIdx] <= 0.0f) {
            goto water_slide;
        }
        switch (p3) {
        case 0xa:
            f29 = 0.1f;
            sel = 0x14;
            break;
        case 0x1c:
            f29 = 0.1f;
            sel = 0x15;
            break;
        default:
            f29 = 0.0f;
            sel = 0xff;
            break;
        }
        break;
    water_slide:
        f29 = f30;
        sel = 0xa;
        break;
    default:
        switch (p3) {
        case 1:
            if (p2 == 1) {
                f29 = 0.1f;
            }
            else {
                f29 = -0.1f;
            }
            sel = 0;
            break;
        case 0:
            f29 = 0.1f;
            if ((u8)Parameters::getCharacterType(_61) == 1) {
                sel = 9;
            }
            else {
                sel = 1;
            }
            break;
        case 7:
            if (p2 == 1) {
                f29 = -0.06f;
            }
            else {
                f29 = -0.1f;
            }
            sel = 2;
            break;
        case 6:
            if (p2 == 1) {
                f29 = 0.0f;
            }
            else {
                f29 = -0.1f;
            }
            sel = 3;
            break;
        case 2:
            f29 = 0.1f;
            sel = 4;
            break;
        case 8:
            f29 = 0.1f;
            sel = 0x13;
            break;
        case 3:
            f29 = -0.1f;
            sel = 5;
            break;
        case 5:
            f29 = -0.1f;
            sel = 6;
            break;
        case 0xe:
            f29 = 0.1f;
            sel = 0xd;
            break;
        case 0x14:
            f29 = 0.1f;
            sel = 0xf;
            break;
        case 0x16:
            f29 = -0.06f;
            sel = 0xc;
            break;
        case 0xd:
            f29 = -0.06f;
            sel = 0xe;
            break;
        case 0xa:
            f29 = 0.1f;
            sel = 0x14;
            break;
        case 0x1c:
            f29 = 0.1f;
            sel = 0x15;
            break;
        case 0xc:
            f29 = 0.1f;
            sel = 0xb;
            break;
        case 0x15:
            f29 = 0.1f;
            sel = 0x10;
            break;
        case 0xb:
            f29 = 0.1f;
            sel = 0x11;
            break;
        case 0x17:
            f29 = 0.1f;
            sel = 0x16;
            break;
        case 0x18:
            f29 = 0.1f;
            sel = 0x17;
            break;
        default:
            f29 = 0.0f;
            sel = 0xff;
            break;
        }
        break;
    }

    _105 = sel;

    if (sel == 0xff) {
        return;
    }
    if (_8d == 0) {
        return;
    }
    if (_84 < 5.0f) {
        return;
    }

    switch (p2) {
    case 0:
    case 1:
    case 0xc:
        switch (p3) {
        default:
        case 0:
        case 1:
        case 4:
        case 6:
        case 7:
        case 8:
        case 9:
        case 0xf:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x15:
        case 0x17:
        case 0x18:
        case 0x19:
        case 0x1a:
        case 0x1b:
            f31 = 0.7f + _84 * 0.0026666666f;
            /* fallthrough */
        case 0xb:
        case 0xc:
        case 0x14:
        case 0x1c:
            f30 = 0.2f + (depth - f29);
            break;
        case 3:
            f31 = 0.7f + _84 * 0.0026666666f;
            /* fallthrough */
        case 2:
        case 5:
        case 0xa:
        case 0xd:
        case 0xe:
        case 0x16: {
            f32 val = _84 / 70.0f;
            if (val > 1.0f) {
                val = 1.0f;
            }
            f30 = val * (0.4f + (depth - f29));
            break;
        }
        }
        break;
    case 0x11:
        switch (sel) {
        case 0x14:
        case 0x15:
            f31 = 0.7f + _84 * 0.0026666666f;
            f30 = 0.2f + (depth - f29);
            break;
        case 0xa: {
            f32 bound = mWaterDepth[wheelIdx];
            if (bound > 40.0f) {
                bound = 40.0f;
            }
            f32 frac = (40.0f - bound) / 40.0f;
            if (_84 < 10.0f) {
                f30 = 0.0f;
            } else {
                f32 val = _84 / 70.0f;
                if (val > 1.0f) {
                    val = 1.0f;
                }
                f30 = 1.2f * val + 0.2f;
                f31 = frac * (0.53f * val) + 0.79f;
            }
            break;
        }
        }
        break;
    default: {
        f32 val = _84 / 70.0f;
        if (val > 1.0f) {
            val = 1.0f;
        }
        f30 = val * (0.4f + (depth - f29));
        break;
    }
    }

    if (f30 <= 0.1f) {
        return;
    }

    if (f30 > 1.5f) {
        f30 = 1.5f;
    }

    _e4[wheelIdx] = 1;
    _b4[wheelIdx] = f30;

    if (wheelIdx & 1) {
        _a4[wheelIdx] = 0.47f;
    } else {
        _a4[wheelIdx] = 0.53f;
    }

    _c4[wheelIdx] = f31;

    switch (wheelIdx) {
    case 0:
        _104 = sel;
        break;
    case 1:
    case 2:
    case 3:
        if (_104 != 6 && _104 != 5 && _104 != 0xa) {
            _104 = sel;
        }
        break;
    default:
        break;
    }
}
void KartSoundMgr::setConductStatus(f32 p1, f32 p2, bool p3, bool p4, bool p5, u8 currentLap, CrsArea *area) {
    if (area != nullptr) {
        if (_5c == 0) {
            f32 rate = area->getRate();
            if (rate != 0.0f) {
                _6c = 0.8f * area->getEchoRate();
                if (_6c > 0.8f) {
                    _6c = 0.8f;
                }
            }
            else {
                _6c = rate;
            }
        }
    }

    if (mKillSw != 0 || _66 == 2) {
        return;
    }

    _88 = p1;
    if (p5 != 0) {
        if (_84 < 1.0f) {
            p5 = false;
        }
    }
    if (p5 != 0) {
        _8c = p4;
    }
    else {
        _8c = p3;
    }
    _8d = currentLap;

    if ((u8)Parameters::getCharacterType(_61) != 9) {
        switch (_8d) {
        case 0:
            setConductSignal();
            setWaterCutoffPort(0);
            break;
        case 1:
            setConductRace(p5);
            break;
        case 2:
            setConductOutOfCourse(currentLap);
            setWaterCutoffPort(0);
            if (_66 == 0) {
                if (_63 != currentLap) {
                    if (Parameters::getRaceCourse() == 0x2f) {
                        startSoundHandleNumber(7, 0x40074, 0);
                    }
                }
            }
            break;
        case 3:
            countGoalKart();
            _8c = 1;
            setConductAfterGoal(p5);
            _5e = 1;
            break;
        case 4:
            setConductTrouble(p1, currentLap);
            setWaterCutoffPort(0);
            if (_66 == 0) {
                if (_63 != currentLap) {
                    if (Parameters::getRaceCourse() == 0x2a) {
                        startSoundHandleNumber(7, 0x40057, 0);
                    }
                    else if (Parameters::getRaceCourse() == 0x2f) {
                        startSoundHandleNumber(7, 0x40074, 0);
                    }
                }
            }
            break;
        case 5:
            if (_66 == 0) {
                if (_63 != currentLap) {
                    if (Parameters::getRaceCourse() == 0x2f) {
                        startSoundHandleNumber(7, 0x40074, 0);
                    }
                }
            }
            setConductPressed();
            break;
        }
    }
    else {
        switch (_8d) {
        case 0:
        case 2:
        case 4:
            if (_63 != currentLap) {
                _8e = 0;
                _92 = 0;
            }
            setConductLocomotiveAccel();
            if (_66 == 0) {
                if (_63 != currentLap) {
                    if (Parameters::getRaceCourse() == 0x2a) {
                        startSoundHandleNumber(7, 0x40057, 0);
                    }
                    else if (Parameters::getRaceCourse() == 0x2f) {
                        startSoundHandleNumber(7, 0x40074, 0);
                    }
                }
            }
            break;
        case 1: {
            if (_63 != currentLap) {
                _8e = 0;
                _92 = 0;
            }
            setConductLocomotiveSpeed(true);
            break;
        }
        case 3: {
            countGoalKart();
            _8c = 1;
            _5e = 1;
            if (_63 != currentLap) {
                _8e = 0;
                _92 = 0;
            }
            setConductLocomotiveSpeed(p5);
            break;
        }
        case 5:
            if (_66 == 0) {
                if (_63 != currentLap) {
                    if (Parameters::getRaceCourse() == 0x2f) {
                        startSoundHandleNumber(7, 0x40074, 0);
                    }
                }
            }
            setConductPressed();
            break;
        }
    }

    _68 = p1;
    _63 = currentLap;
    _98 = _84;
}

void KartSoundMgr::setWaterCutoffPort(u16 port) {
    JAISoundHandles *handles = this;
    JAISoundHandle *handle = &(*handles)[3];
    if (*handle) {
        (*handle)->getTrack()->writePort(10, port);
    }
}

void KartSoundMgr::countGoalKart() {
    if (_5e == 0 && _63 != _8d) {
        smGoalKartCount++;
    }
}

void KartSoundMgr::setConductLocomotiveAccel() {
    u8 flag = 0;
    f32 vol;

    if (_8c != 0) {
        if (_8e == 0) {
            if (_92 == 0) {
                _92 = 9;
            }
            else if (_92 > 4) {
                _92--;
            }
            _8e = _92;
            flag = 1;
            vol = 0.6f;
        }
    }
    else if (_8e == 0) {
        if (_92 == 0) {
            _92 = 9;
        }

        if (_92 < 9) {
            _92++;
            _8e = _92;
        }
        else {
            _8e = _92;
        }
        flag = 1;
        vol = 0.4f;
    }

    if (flag != 0) {
        JAISoundHandles *handles = this;
        JAISoundStarter *starter = JAISoundStarter::getInstance();

        u32 id = -(_96++ & 1) + 18;

        JAISoundHandle *handle = &(*handles)[3];

        id += (_66 != 0) ? 0x14 : 0;
        starter->startSound(id, handle, nullptr);

        if (*handle) {
            if (!(*handle)->getAudible()) {
                u32 mask = _7c;
                (*handle)->newAudible(JGeometry::TVec3f(*mSoundPos), &_18, mask, nullptr);
                if (_80 != 0) {
                    JAISound *sound = handle->operator->();
                    sound->fader_.mIntensity = 0.0f;
                    if (_80 != 0) {
                        sound->fader_.mTransition.set(1.0f, sound->fader_.mIntensity, _80);
                    }
                    else {
                        sound->fader_.forceIn();
                    }
                    _80 = 0;
                }
            }
        }

        setEcho(handle, _6c);

        JAISoundHandle *handle2 = &(*handles)[3];
        if (*handle2) {
            (*handle2)->getAuxiliary().moveVolume(vol, 0);
            f32 pitch = 0.6f + 0.4f * (f32)(9 - _92) / 5.0f;
            (*handle2)->getAuxiliary().movePitch(pitch, 0);
        }
    }
    else {
        if (_8e != 0) {
            _8e--;
        }
    }
}

void KartSoundMgr::setConductLocomotiveSpeed(bool) {
    if (_8e != 0) {
        _8e--;
        return;
    }

    f32 engineRot = _84;
    if (engineRot < 1.0f) {
        engineRot = 1.0f;
    }
    if (engineRot > 130.0f) {
        engineRot = 130.0f;
    }

    _8e = (s16)((130.0f - engineRot) / 20.0f + 3.0f);

    JAISoundHandles *handles = this;
    JAISoundStarter *starter = JAISoundStarter::getInstance();

    u32 id = -(_96++ & 1) + 18;

    JAISoundHandle *handle = &(*handles)[3];

    id += (_66 != 0) ? 0x14 : 0;
    starter->startSound(id, handle, nullptr);

    if (*handle) {
        if (!(*handle)->getAudible()) {
            u32 mask = _7c;
            (*handle)->newAudible(JGeometry::TVec3f(*mSoundPos), &_18, mask, nullptr);
            if (_80 != 0) {
                JAISound *sound = handle->operator->();
                sound->fader_.mIntensity = 0.0f;
                if (_80 != 0) {
                    sound->fader_.mTransition.set(1.0f, sound->fader_.mIntensity, _80);
                }
                else {
                    sound->fader_.forceIn();
                }
                _80 = 0;
            }
        }
    }

    setEcho(handle, _6c);

    JAISoundHandle *handle2 = &(*handles)[3];
    if (*handle2) {
        (*handle2)->getAuxiliary().moveVolume(0.7f * (engineRot / 130.0f) + 0.5f, 0);

        f32 f30 = 1.0f;
        if (engineRot < 100.0f) {
            f30 = engineRot / 100.0f;
        }
        if (_5d != 0) {
            f30 *= Parameters::getChibiPitch(id);
        }

        (*handle2)->getAuxiliary().movePitch(0.4f * f30 + 0.6f, 0);
    }
}

void KartSoundMgr::setConductPressed() {
    JAISoundHandles *handles = this;
    JAISoundStarter *starter = JAISoundStarter::getInstance();
    JAISoundHandle *handle = &(*handles)[3];
    u32 id = 0x13;
    id += ((_66 != 0) ? -1 : 0) & 0x14;
    starter->startSound(id, handle, nullptr);

    if (*handle) {
        if (!(*handle)->getAudible()) {
            u32 mask = _7c;
            (*handle)->newAudible(JGeometry::TVec3f(*mSoundPos), &_18, mask, nullptr);
            if (_80 != 0) {
                JAISound *sound = handle->operator->();
                sound->fader_.mIntensity = 0.0f;
                if (_80 != 0) {
                    sound->fader_.mTransition.set(1.0f, sound->fader_.mIntensity, _80);
                }
                else {
                    sound->fader_.forceIn();
                }
                _80 = 0;
            }
        }
    }

    setEcho(handle, _6c);

    f32 vol = _84;
    if (vol > 100.0f) {
        vol = 100.0f;
    }
    vol = 1.0f + vol / 100.0f;

    JAISoundHandle *handle2 = &(*handles)[3];
    if (*handle2) {
        (*handle2)->getAuxiliary().movePitch(vol, 0);
    }
}

void KartSoundMgr::setConductSignal() {
    static const f32 EngineKarabukashiLength[] = {
        90.0f, 40.0f, 90.0f, 30.0f, 40.0f, 35.0f, 30.0f, 95.0f, 90.0f,
    };
    static const f32 EnginePitchKeisuuSignal[] = {
        0.02f, 0.05f, 0.02f, 0.09f, 0.05f, 0.06f, 0.06f, 0.02f, 0.05f,
    };
    static const f32 EnginePitchDownSignal[] = {
        0.9f, 0.9f, 0.6f, 0.9f, 0.9f, 0.7f, 0.7f, 0.6f, 0.8f,
    };

    if (_63 != _8d) {
        _fc = 0.0f;
    }

    f32 pitch;
    f32 volume;
    if (_66 == 0 || (u8)Parameters::getNetworkCubes() != 1) {
        if (_8c == 1) {
            volume = 1.4f;
            if (_fc < EngineKarabukashiLength[(u8)Parameters::getEngineType(_61)]) {
                _fc = _fc + 1.0f;
            }
            pitch = _fc * EnginePitchKeisuuSignal[(u8)Parameters::getEngineType(_61)] + 0.7f;
        }
        else {
            volume = 0.7f;
            if (_fc > 0.0f) {
                _fc = _fc - EnginePitchDownSignal[(u8)Parameters::getEngineType(_61)];
            }
            else {
                _fc = 0.0f;
            }
            pitch = _fc * EnginePitchKeisuuSignal[(u8)Parameters::getEngineType(_61)] + 0.7f;
        }

        if (_fc < 20.0f) {
            volume = volume + -0.015000001f * _fc + 0.3f;
        }
    }
    else {
        if (_11c == 0) {
            _5f ^= 1;
            if (_5f != 0) {
                u32 interval = SignalUpIntervalBase[_64] + Random::getSignalEngineRandomU32() % SignalUpIntervalWidth[_64];
                f32 newPitch = (f32)(SignalUpPitchBase[_64] + Random::getSignalEngineRandomU32() % SignalUpPitchWidth[_64]) / 100.0f;
                if (_11c == 0 && _114 == newPitch) {
                    goto signal_noop;
                }
                if (_11c != 0 && _110 == newPitch) {
                    goto signal_noop;
                }
                _110 = newPitch;
                if (interval == 0) {
                    _114 = newPitch;
                    goto signal_noop;
                }
                interval++;
                _118 = (_114 - _110) / (f32)interval;
                _11c = interval;
            signal_noop:;
            }
            else {
                u32 interval = SignalDownIntervalBase[_64] + Random::getSignalEngineRandomU32() % SignalDownIntervalWidth[_64];
                if (_11c == 0 && _114 == 0.7f) {
                    goto signal_noop2;
                }
                if (_11c != 0 && _110 == 0.7f) {
                    goto signal_noop2;
                }
                _110 = 0.7f;
                if (interval == 0) {
                    _114 = 0.7f;
                    goto signal_noop2;
                }
                interval++;
                _118 = (_114 - _110) / (f32)interval;
                _11c = interval;
            signal_noop2:;
            }
        }

        if (_11c != 0) {
            if (--_11c != 0) {
                _114 = _114 - _118;
            }
            else {
                _114 = _110;
            }
        }

        if (_5f != 0) {
            volume = 1.4f;
        }
        else {
            volume = 0.7f;
        }
        volume = volume * GA_ENEMY_VOLUME_DOWN_VALUE;
        pitch = _114;
    }

    int engineType = Parameters::getEngineType(_61);
    u32 id = engineType + 8;
    id += (_66 != 0) * 0x14;

    JAISoundStarter *starter = JAISoundStarter::getInstance();
    JAISoundHandles *handles = this;
    JAISoundHandle *handle = &(*handles)[3];
    starter->startSound(id, handle, nullptr);

    if (*handle) {
        if (!(*handle)->getAudible()) {
            u32 mask = _7c;
            (*handle)->newAudible(JGeometry::TVec3f(*mSoundPos), &_18, mask, nullptr);
            if (_80 != 0) {
                JAISound *sound = handle->operator->();
                sound->fader_.mIntensity = 0.0f;
                if (_80 != 0) {
                    sound->fader_.mTransition.set(1.0f, sound->fader_.mIntensity, _80);
                }
                else {
                    sound->fader_.forceIn();
                }
                _80 = 0;
            }
        }
    }

    setEcho(handle, _6c);

    JAISoundHandle *handle2 = &(*handles)[3];
    if (*handle2) {
        (*handle2)->getAuxiliary().moveVolume(volume, 0);
        (*handle2)->getAuxiliary().movePitch(pitch, 0);
    }
}

void KartSoundMgr::setConductOutOfCourse(u8 lap) {
    static const f32 EnginePitchKeisuuOutOfCourse[] = {
        0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f, 0.01f,
    };

    if (_63 != lap) {
        _94 = 0;
    }

    f32 vol;
    if (_8c == 1) {
        if (_94 < 100) {
            _94++;
            _94++;
        }
        vol = 1.2f;
    }
    else {
        if (_94 != 0) {
            _94--;
        }
        vol = 0.55f;
    }

    f32 pitch = _94 * EnginePitchKeisuuOutOfCourse[(u8)Parameters::getEngineType(_61)] + 0.6f;
    if (_66 != 0) {
        vol *= GA_ENEMY_VOLUME_DOWN_VALUE;
    }

    int engineType = Parameters::getEngineType(_61);
    u32 id = engineType + 8;
    id += (_66 != 0) * 0x14;

    JAISoundStarter *starter = JAISoundStarter::getInstance();
    JAISoundHandles *handles = this;
    JAISoundHandle *handle = &(*handles)[3];
    starter->startSound(id, handle, nullptr);

    if (*handle) {
        if (!(*handle)->getAudible()) {
            u32 mask = _7c;
            (*handle)->newAudible(JGeometry::TVec3f(*mSoundPos), &_18, mask, nullptr);
            if (_80 != 0) {
                JAISound *sound = handle->operator->();
                sound->fader_.mIntensity = 0.0f;
                if (_80 != 0) {
                    sound->fader_.mTransition.set(1.0f, sound->fader_.mIntensity, _80);
                }
                else {
                    sound->fader_.forceIn();
                }
                _80 = 0;
            }
        }
    }

    setEcho(handle, _6c);

    JAISoundHandle *handle2 = &(*handles)[3];
    if (*handle2) {
        if (_5d != 0) {
            pitch *= Parameters::getChibiPitch((*handle2)->getID());
            vol *= 0.65f;
        }
        (*handle2)->getAuxiliary().moveVolume(vol, 0);
        (*handle2)->getAuxiliary().movePitch(pitch, 0);
    }
}

void KartSoundMgr::setConductTrouble(f32 distance, u8 lap) {
    static const f32 EnginePitchKeisuuTrouble[] = {
        0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f, 0.025f,
    };

    if (_63 != lap) {
        _92 = (s16)(38.0f * distance);
    }

    f32 vol;
    if (_8c == 1) {
        if (_92 < 38) {
            _92++;
            _92++;
        }
        vol = 1.2f;
    }
    else {
        if (_92 != 0) {
            _92--;
        }
        vol = 0.55f;
    }

    f32 pitch = (f32)_92 * EnginePitchKeisuuTrouble[(u8)Parameters::getEngineType(_61)] + 0.5f;
    if (_66 != 0) {
        vol *= GA_ENEMY_VOLUME_DOWN_VALUE;
    }

    int engineType = Parameters::getEngineType(_61);
    u32 id = engineType + 8;
    id += (_66 != 0) * 0x14;

    JAISoundStarter *starter = JAISoundStarter::getInstance();
    JAISoundHandles *handles = this;
    JAISoundHandle *handle = &(*handles)[3];
    starter->startSound(id, handle, nullptr);

    if (*handle) {
        if (!(*handle)->getAudible()) {
            u32 mask = _7c;
            (*handle)->newAudible(JGeometry::TVec3f(*mSoundPos), &_18, mask, nullptr);
            if (_80 != 0) {
                JAISound *sound = handle->operator->();
                sound->fader_.mIntensity = 0.0f;
                if (_80 != 0) {
                    sound->fader_.mTransition.set(1.0f, sound->fader_.mIntensity, _80);
                }
                else {
                    sound->fader_.forceIn();
                }
                _80 = 0;
            }
        }
    }

    setEcho(handle, _6c);

    JAISoundHandle *handle2 = &(*handles)[3];
    if (*handle2) {
        if (_5d != 0) {
            pitch *= Parameters::getChibiPitch((*handle2)->getID());
            vol *= 0.65f;
        }
        (*handle2)->getAuxiliary().moveVolume(vol, 0);
        (*handle2)->getAuxiliary().movePitch(pitch, 0);
    }
}

void KartSoundMgr::setConductRace(bool flag) {
    static const f32 EngineAddKeisuuRaceUp[] = {
        0.0006000000f, 0.0006000000f, 0.0006000000f, 0.0005000000f,
        0.0012000000f, 0.0012000000f, 0.0012000000f, 0.0012000000f,
        0.0012000000f,
    };
    static const f32 UpEngineLoopStart[] = {
        100639.0f, 133568.0f, 121677.0f, 89762.0f,
    };
    static const f32 UpEngineLoopEnd[] = {
        113023.0f, 167266.0f, 138431.0f, 100738.0f,
    };
    static const f32 EngineAddKeisuuRaceDown[] = {
        0.00015f, 0.00015f, 0.00015f, 0.00015f,
        0.0006f, 0.0006f, 0.0006f, 0.0006f, 0.0006f,
    };
    static const f32 DownEngineLoopStart[] = {
        42032.0f, 57452.0f, 54457.0f, 33775.0f,
    };
    static const f32 DownEngineLoopEnd[] = {
        93600.0f, 104350.0f, 79743.0f, 63943.0f,
    };

    u32 loopCount = 0;
    u8 engineType = Parameters::getEngineType(_61);

    if (_e8[0] == 0x11 || _e8[1] == 0x11 || _e8[2] == 0x11 || _e8[3] == 0x11) {
        f32 wDepth = 215.0f;
        for (u8 i = 0; i < 4; i++) {
            if (wDepth > mWaterDepth[i]) {
                wDepth = mWaterDepth[i];
            }
        }
        if (wDepth < 15.0f) {
            wDepth = 0.0f;
        }
        else {
            if (wDepth > 215.0f) {
                wDepth = 215.0f;
            }
            wDepth = (wDepth - 15.0f) / 200.0f;
        }

        if (wDepth == 0.0f) {
            loopCount = 0;
        } else {
            loopCount = (u32)(s32)(127.0f - 126.0f * wDepth);
        }
    }

    if (1.0f + _84 > _98) {
        if (_84 < 1.0f) {
            _100++;
            if (_100 == 20) {
                _8c = 0;
                _100 = 0;
            }
        }
        if (_84 > 50.0f) {
            if (_96 < 0x320) {
                _96++;
            }
        }
    } else {
        if (_84 < 10.0f) {
            _8c = 0;
        }
        if (_96 >= 40) {
            _96 -= 40;
        }
    }

    if (_8c == 1) {
        if (_fc > 0.0f) {
            _fc = _fc - 1.0f;
        }
        _90 = 0;
        if (_8e < 9999) {
            _8e++;
        }

        f32 releaseDensity = -0.0016666667f * _84 + 0.3f;
        if (_a0 != 0.0f) {
            if (_96 > -200) {
                _96 -= (s16)(80.0f * _a0);
            }
        }
        adjustEngine();
        if (_f0 > 0.0f) {
            _fc = 0.0f;
        }

        f32 mediaPitch = EngineKeisuuRaceUp[engineType] * _84 + EngineKeisuuRaceUp[engineType] * _88;
        mediaPitch += (f32)(s16)_96 * EngineAddKeisuuRaceUp[engineType];
        mediaPitch += 0.5f;
        mediaPitch += releaseDensity;
        mediaPitch += _f0;
        mediaPitch += 0.02f * _fc;

        f32 vol = 1.4f;
        if (_84 < 40.0f) {
            vol = 1.4f + -0.0075000003f * _84;
            vol = 0.3f + vol;
        }

        u32 num;
        if (engineType < 4 && flag == 0) {
            num = 0;
        }
        else {
            num = 8;
        }
        u32 id = engineType + num;
        if (_66 != 0) {
            id += 0x14;
        }
        if (loopCount != 0) {
            vol *= (0.5f + 0.5f * loopCount / 127.0f);
        }

        JAISoundStarter *starter = JAISoundStarter::getInstance();
        JAISoundHandles *handles = this;
        JAISoundHandle *handle = &(*handles)[3];
        starter->startSound(id, handle, nullptr);

        if (*handle) {
            if (!(*handle)->getAudible()) {
                u32 mask = _7c;
                (*handle)->newAudible(JGeometry::TVec3f(*mSoundPos), &_18, mask, nullptr);
                if (_80 != 0) {
                    JAISound *sound = handle->operator->();
                    sound->fader_.mIntensity = 0.0f;
                    if (_80 != 0) {
                        sound->fader_.mTransition.set(1.0f, sound->fader_.mIntensity, _80);
                    }
                    else {
                        sound->fader_.forceIn();
                    }
                    _80 = 0;
                }
            }
        }

        setEcho(handle, _6c);

        JAISoundHandle *handle2 = &(*handles)[3];
        if (*handle2) {
            if (_5d != 0) {
                mediaPitch *= Parameters::getChibiPitch((*handle2)->getID());
                vol *= 0.65f;
            }
            (*handle2)->getAuxiliary().moveVolume(vol, 0);
            (*handle2)->getAuxiliary().movePitch(mediaPitch, 0);
            (*handle2)->getTrack()->writePort(10, loopCount);
        }

        // cross fade to loop for water? no

        JAISoundHandle *handle3 = &(*handles)[3];
        if (*handle3) {
            if (engineType < 4) {
                u32 loopPoint = (u32)(_84 * (UpEngineLoopStart[engineType] / 150.0f) - 10000.0f);
                f32 loopPointF = loopPoint;
                if (loopPointF < 0.0f) {
                    loopPointF = 0.0f;
                }
                if (loopPointF > UpEngineLoopEnd[engineType]) {
                    loopPointF = UpEngineLoopEnd[engineType];
                }
                (*handle3)->getTrack()->mSkipSample = (u32)loopPointF;
            }
        }
    }
    else {
        _96 = 0;
        _8e = 0;
        if (_90 < 9999) {
            _90++;
        }
        adjustEngine();

        f32 mediaPitch = EngineKeisuuRaceDown[engineType] * _84 + EngineKeisuuRaceDown[engineType] * _88;
        mediaPitch += (f32)(s16)_96 * EngineAddKeisuuRaceDown[engineType];
        mediaPitch += 0.7f;
        mediaPitch += _f0;

        f32 vol;
        if (engineType < 4) {
            vol = 1.1f;
        }
        else {
            vol = 0.8f;
        }
        if (_84 < 40.0f) {
            vol = -0.01f * _84 + vol;
            vol = 0.4f + vol;
        }

        u32 num;
        if (engineType < 4 && flag == 0) {
            num = 4;
        }
        else {
            num = 8;
        }
        u32 id = engineType + num;
        if (_66 != 0) {
            id += 0x14;
        }
        if (_66 != 0) {
            vol *= GA_ENEMY_VOLUME_DOWN_VALUE;
        }

        JAISoundStarter *starter = JAISoundStarter::getInstance();
        JAISoundHandles *handles = this;
        JAISoundHandle *handle = &(*handles)[3];
        starter->startSound(id, handle, nullptr);

        if (*handle) {
            if (!(*handle)->getAudible()) {
                u32 mask = _7c;
                (*handle)->newAudible(JGeometry::TVec3f(*mSoundPos), &_18, mask, nullptr);
                if (_80 != 0) {
                    JAISound *sound = handle->operator->();
                    sound->fader_.mIntensity = 0.0f;
                    if (_80 != 0) {
                        sound->fader_.mTransition.set(1.0f, sound->fader_.mIntensity, _80);
                    }
                    else {
                        sound->fader_.forceIn();
                    }
                    _80 = 0;
                }
            }
        }

        setEcho(handle, _6c);

        JAISoundHandle *handle2 = &(*handles)[3];
        if (*handle2) {
            if (_5d != 0) {
                mediaPitch *= Parameters::getChibiPitch((*handle2)->getID());
                vol *= 0.65f;
            }
            (*handle2)->getAuxiliary().moveVolume(vol, 0);
            (*handle2)->getAuxiliary().movePitch(mediaPitch, 0);
            (*handle2)->getTrack()->writePort(10, loopCount);
        }

        JAISoundHandle *handle3 = &(*handles)[3];
        if (*handle3) {
            if (engineType < 4) {
                u32 loopPoint = (u32)(_84 * -(DownEngineLoopStart[engineType] / 150.0f) + DownEngineLoopStart[engineType] - 5000.0f);
                f32 loopPointF = loopPoint;
                if (loopPointF < 0.0f) {
                    loopPointF = 0.0f;
                }
                if (loopPointF > DownEngineLoopEnd[engineType]) {
                    loopPointF = DownEngineLoopEnd[engineType];
                }
                (*handle3)->getTrack()->mSkipSample = (u32)loopPointF;
            }
        }
    }
}

void KartSoundMgr::setConductAfterGoal(bool flag) { setConductRace(flag); }

void KartSoundMgr::setCrushSe(CrsGround::EMat mat, f32 val) {
    switch (mat) {
    case (CrsGround::EMat)5:
    case (CrsGround::EMat)0x11:
        setCrushSe(0x1001d, val);
        break;
    case (CrsGround::EMat)8:
        setCrushSe(0x10015, val);
        break;
    case (CrsGround::EMat)7:
    case (CrsGround::EMat)0xf:
        setCrushSe(0x10021, val);
        break;
    case (CrsGround::EMat)0x10:
        setCrushSe(0x10023, val);
        break;
    case (CrsGround::EMat)0x12:
        setCrushSe(0x10053, val);
        break;
    case (CrsGround::EMat)0x13:
        setCrushSe(0x10056, val);
        break;
    case (CrsGround::EMat)9:
        setCrushSe(0x10019, val);
        break;
    case (CrsGround::EMat)0x19:
        setCrushSe(0x10067, val);
        break;
    case (CrsGround::EMat)0x1b:
        setCrushSe(0x1007d, val);
        break;
    case (CrsGround::EMat)0xff:
        break;
    default:
        setCrushSe(0x10015, val);
        break;
    }
}

void KartSoundMgr::setCrushSe(u32 id, f32 crushPower) {
    id = Common::changeRandomId(id, 0);
    u32 rnd = Random::getRandomU32();

    if (mKillSw != 0 || _66 == 2) {
        return;
    }
    if (_66 != 0) {
        return;
    }

    u32 frame = *(u32 *)((u8 *)Main::getAudio() + 0x80);
    if (frame - _74 < 60) {
        return;
    }
    _74 = frame;

    if (_9c != 0) {
        return;
    }

    startSoundHandleNumber(0, id, 0);

    JAISoundHandles *handles = this;
    JAISoundHandle *handle = &(*handles)[0];
    crushPower = 0.6f + crushPower;
    if (*handle) {
        f32 vol = 0.8f * crushPower;
        (*handle)->getAuxiliary().moveVolume(vol, 0);
        _a0 = crushPower;
    }

    _9c = (u8)(rnd & 7) + 4;
}

void KartSoundMgr::setBrakeSe(u32 id) {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }
    if (_66 != 0) {
        return;
    }

    startSoundHandleNumber(6, id, 0);

    JAISoundHandles *handles = this;
    JAISoundHandle *handle = &(*handles)[6];
    if (*handle) {
        f32 volume = 0.008f * _84 + 0.25f;
        f32 pitch = 0.003f * _84 + 0.6f;
        if (_5d != 0) {
            pitch = pitch * Parameters::getChibiPitch(id);
        }
        (*handle)->getAuxiliary().moveVolume(0.8f * volume, 0);
        (*handle)->getAuxiliary().movePitch(pitch, 0);
    }
    setChibiPitch(handle);
}

void KartSoundMgr::setDashSe(u32 id) {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }
    if (_66 == 1 && (u8)Parameters::getRaceMode() != GRAND_PRIX) {
        return;
    }

    setSe(id);

    _f0 = DashEngineAdjustInitialValue[(u8)Parameters::getEngineType(_61)];
    _f4 = 0x10014;
    _f8 = 100;
}

void KartSoundMgr::setMiniturboSe(u32 id) {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }
    if (_66 != 0) {
        return;
    }

    setSe(id);

    _f0 = DashEngineAdjustInitialValue[(u8)Parameters::getEngineType(_61)];
    _f4 = 0x10044;
    _f8 = 40;
}

void KartSoundMgr::setJumpUpSe(u32 id) {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }
    if (Parameters::getRaceCourse() != PEACH_BEACH) {
        return;
    }
    setSe(id);
}

void KartSoundMgr::setBoundSe(f32 bound) {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }
    if (_66 == 1 && (u8)Parameters::getRaceMode() != GRAND_PRIX) {
        return;
    }
    if (_105 >= 0x19) {
        return;
    }

    startSoundHandleNumber(4, BoundSe[_105], 0);

    JAISoundHandles *handles = this;
    JAISoundHandle *handle = &(*handles)[4];
    if (*handle) {
        (*handle)->getAuxiliary().movePitch(0.5f * bound + 0.5f, 0);
    }
    if (*handle) {
        (*handle)->getAuxiliary().moveVolume(bound * _124, 0);
    }

    setChibiPitch(handle);
}

void KartSoundMgr::setWheelSpinSe() {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }
    if (_66 != 0) {
        return;
    }

    if ((u8)Parameters::getCharacterType(_61) == 1) {
        startSoundHandleNumber(4, 0x10077, 0);
    }
    else if (_105 < 0x19) {
        startSoundHandleNumber(4, WheelSpinSe[_105], 0);
    }

    setChibiPitch(&(*this)[4]);
}

void KartSoundMgr::setSpinSe() {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }

    if ((u8)Parameters::getCharacterType(_61) == 1) {
        startSoundHandleNumber(1, 0x100AE, 0);
    }
    else if (_105 < 0x19) {
        startSoundHandleNumber(1, SpinSe[_105], 0);
    }

    setChibiPitch(&(*this)[1]);
}

void KartSoundMgr::setSpinTurnSe() {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }
    if (_66 != 0) {
        return;
    }

    if ((u8)Parameters::getCharacterType(_61) == 1) {
        startSoundHandleNumber(1, 0x100C6, 0);
    }
    else if (_105 < 0x19) {
        startSoundHandleNumber(1, SpinTurnSe[_105], 0);
    }

    setChibiPitch(&(*this)[1]);
}

void KartSoundMgr::setSe(u32 id) {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }

    CustomSoundTable *soundTable = Main::getAudio()->getSoundTable();
    if (_8d == 3 && soundTable->getSwBit(id) & 0x8000000) {
        return;
    }
    if (_66 != 0 && soundTable->getSwBit(id) & 0x80000000) {
        return;
    }
    if (id == 0x10041 && *(f32 *)((u8 *)Main::getAudio() + 0x9c) == 0.0f) {
        return;
    }

    u32 mask = 0;
    CameraMgr *camera = Main::getAudio()->getCamera();
    u32 sceneMax = camera->getSceneMax();
    if (sceneMax > 1 && sceneMax > mKartCount) {
        mask = (1 << mKartCount) ^ 0xf;
    }

    JAISoundHandle *handle = startSoundCustom(id, mask);

    if (_66 != 0 && handle && *handle && *handle) {
        (*handle)->getAuxiliary().moveVolume(0.8f * _124, 0);
    }

    if (id != 0x10038 && id != 0x10039) {
        setChibiPitch(handle);
    }
    setEcho(handle, _6c);
}

void KartSoundMgr::setChibiPitch(JAISoundHandle *handle) {
    if (_5d == 0) {
        return;
    }
    if (handle && *handle) {
        handle->operator->()->getAuxiliary().movePitch(Parameters::getChibiPitch(handle->operator->()->getID()), 0);
    }
}

void KartSoundMgr::adjustEngine() {
    int engineType = Parameters::getEngineType(_61);

    if (_f8 == 0) {
        return;
    }

    switch (_f4) {
    case 0x10014:
        if (_f8 > 120) {
            _f0 = _f0 + DashEngineIncPerFrame[(u8)engineType];
            _f8--;
        }
        else if (_f8 > 110) {
            _f8--;
        }
        else if (_f8 == 0 || _f0 < 0.0f) {
            _f8 = 0;
            _f0 = 0.0f;
            _f4 = 0;
        }
        else {
            _f0 = _f0 - DashEngineDecPerFrame[(u8)engineType];
            _f8--;
        }
        break;
    case 0x10044:
        if (_f8 > 48) {
            _f0 = _f0 + 0.05f;
            _f8--;
        }
        else if (_f8 > 44) {
            _f8--;
        }
        else if (_f8 == 0 || _f0 < 0.0f) {
            _f8 = 0;
            _f0 = 0.0f;
            _f4 = 0;
        }
        else {
            _f0 = _f0 - 0.02f;
            _f8--;
        }
        break;
    }
}

void KartSoundMgr::crushRenzokuTaisaku() {
    if (_9c != 0) {
        _9c--;
        if (_9c == 0) {
            _a0 = 0.0f;
        }
    }
}

void KartSoundMgr::slipParamSet() {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }
    if (_66 != 0) {
        return;
    }

    u8 mode = Parameters::getPlayerMode();

    if (_e4[0] == 1 || _e4[1] == 1 || _e4[2] == 1 || _e4[3] == 1) {
        f32 speed = (150.0f < _84) ? 150.0f : _84;

        switch (_104) {
        case 6:
            _101++;
            _102 = 30.0f + speed * -25.0f / 150.0f;
            if (_101 > _102) {
                _101 = 0;
                if (_103 == 0) {
                    startSoundHandleNumber(2, 0x10007, 0);
                    _103 = 1;
                } else {
                    startSoundHandleNumber(2, 0x10008, 0);
                    _103 = 0;
                }
            }
            break;
        case 0xd:
            _101++;
            _102 = 30.0f + speed * -25.0f / 150.0f;
            if (_101 > _102) {
                startSoundHandleNumber(2, 0x1005c, 0);
            }
            break;
        case 5:
            _101++;
            _102 = 15.0f + speed * -13.0f / 150.0f;
            if (_101 > _102) {
                _101 = 0;
                if (_103 == 0) {
                    startSoundHandleNumber(2, 0x10005, 0);
                    _103 = 1;
                } else {
                    startSoundHandleNumber(2, 0x10006, 0);
                    _103 = 0;
                }
            }
            break;
        case 0xa:
            startSoundHandleNumber(2, 0x10036, 0);
            break;
        case 0xb:
            startSoundHandleNumber(2, 0x1004e, 0);
            break;
        case 0xc:
            startSoundHandleNumber(2, 0x10059, 0);
            break;
        case 0xe:
            startSoundHandleNumber(2, 0x1004f, 0);
            break;
        case 0xf:
            startSoundHandleNumber(2, 0x10057, 0);
            break;
        case 0x10:
            startSoundHandleNumber(2, 0x10058, 0);
            break;
        case 0x11:
            startSoundHandleNumber(2, 0x1004d, 0);
            break;
        case 0x13:
            startSoundHandleNumber(2, 0x1003b, 0);
            break;
        case 0x14:
            startSoundHandleNumber(2, 0x1003a, 0);
            break;
        case 0x15:
            startSoundHandleNumber(2, 0x10082, 0);
            break;
        case 0x16:
            startSoundHandleNumber(2, 0x1005f, 0);
            break;
        case 0x17:
            startSoundHandleNumber(2, 0x10069, 0);
            break;
        default:
            if (_104 <= 9) {
                startSoundHandleNumber(2, _104 + 0x10000, 0);
            }
            break;
        }

        _104 = 0xff;

        f32 panSum = 0.0f;
        f32 volumeSum = 0.0f;
        f32 pitchSum = 0.0f;
        u8 changeCnt = 0;
        for (u8 i = 0; i < 4; i++) {
            if (_e4[i] == 1) {
                changeCnt++;
                if (mode == 0) {
                    panSum += _a4[i];
                }
                volumeSum += _b4[i];
                pitchSum += _c4[i];
                _e4[i] = 0;
            }
        }

        f32 pan;
        if (mode == 0) {
            panSum /= changeCnt;
            pan = Common::panDeform(panSum, 3.0f);
        } else {
            pan = Common::setMultiPlayModePan(mKartCount);
        }

        volumeSum /= changeCnt;
        pitchSum /= changeCnt;

        JAISoundHandles *handles = this;
        JAISoundHandle *handle = &(*handles)[2];
        if (*handle) {
            if (Parameters::getMirrorSwitch()) {
                pan = 1.0f - pan;
            }
            (*handle)->getAuxiliary().movePan(pan, 3);
            if (*handle) {
                (*handle)->getAuxiliary().moveVolume(volumeSum * _124, 0);
            }
            if (_5d != 0) {
                pitchSum *= Parameters::getChibiPitch((*handle)->getID());
            }
            (*handle)->getAuxiliary().movePitch(pitchSum, 0);
        }
    }
}

void KartSoundMgr::checkEcho() {
    if (_70 == _6c) {
        return;
    }

    JAISoundHandles *handles = this;
    for (int i = 0; i < _10; i++) {
        if ((*handles)[i]) {
            (*handles)[i]->getAuxiliary().moveFxMix(_6c, 0);
        }
    }
    _70 = _6c;
}

void KartSoundMgr::setInvincibleBgm(u8 flag) {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }

    if (_66 != 0) {
        if (*(f32 *)((u8 *)Main::getAudio() + 0x9c) == 0.0f) {
            return;
        }
        switch (flag) {
        case 1:
            _60 |= 1;
            startSoundHandleNumber(5, 0x1003f, 0);
            break;
        case 2:
            startSoundHandleNumber(5, 0x10040, 0);
            _60 |= 2;
            break;
        default:
#line 2907
            JUT_WARNING_F2("%s", "KartSoundMgr::startInvincibleBgm : 不明なBITです。\n");
            break;
        }
    }
    else {
        switch (flag) {
        case 1:
            Main::getAudio()->setInvincibleBgm(_61, flag);
            _60 |= 1;
            break;
        case 2:
            Main::getAudio()->setInvincibleBgm(_61, flag);
            _60 |= 2;
            break;
        default:
#line 2923
            JUT_WARNING_F2("%s", "KartSoundMgr::startInvincibleBgm : 不明なBITです。\n");
            break;
        }
    }
}

void KartSoundMgr::clearInvincibleBgm(u8 flag) {
    if (mKillSw != 0 || _66 == 2) {
        return;
    }

    if (_66 != 0) {
        if (*(f32 *)((u8 *)Main::getAudio() + 0x9c) == 0.0f) {
            return;
        }
        switch (flag) {
        case 1:
            _60 ^= 1;
            break;
        case 2:
            _60 ^= 2;
            break;
        case 3:
            _60 = 0;
            break;
        default:
#line 2951
            JUT_WARNING_F2("%s", "KartSoundMgr::startInvincibleBgm : 不明なBITです。\n");
            break;
        }

        JAISoundHandles *handles = this;
        if (_60 == 0) {
            JAISoundHandle *handle = &(*handles)[5];
            if (*handle) {
                (*handle)->stop();
            }
        }
        else {
            switch (_60) {
            case 1:
                startSoundHandleNumber(5, 0x1003f, 0);
                break;
            case 2:
                startSoundHandleNumber(5, 0x10040, 0);
                break;
            }
        }
    }
    else {
        switch (flag) {
        case 1:
            Main::getAudio()->clearInvincibleBgm(_61, flag);
            _60 ^= 1;
            break;
        case 2:
            Main::getAudio()->clearInvincibleBgm(_61, flag);
            _60 ^= 2;
            break;
        case 3:
            _60 = 0;
            break;
        default:
#line 2984
            JUT_WARNING_F2("%s", "KartSoundMgr::startInvincibleBgm : 不明なBITです。\n");
            break;
        }
    }
}

void KartSoundMgr::setChibiFlag(bool chibi, bool flag2) {
    Parameters::setChibiFlag(_61, chibi);
    if (chibi) {
        if (_5d != chibi && flag2) {
            setSe(0x10038);
        }
        Main::getAudio()->setThunderMode(_61);
    }
    else {
        if (_5d != chibi && flag2) {
            setSe(0x10039);
        }
        Main::getAudio()->clearThunderMode(_61);

        JAISoundHandles *handles = this;
        for (int i = 0; i < _10; i++) {
            if ((*handles)[i]) {
                (*handles)[i]->getAuxiliary().movePitch(1.0f, 0);
            }
        }
    }
    _5d = chibi;
}

}

#ifdef MATCHING
#include "JSystem/JAudio/JASFakeMatch.h"

typedef JAUDopplerAudible<4> TDopplerAudible4Kart;
JASMemPool<TDopplerAudible4Kart> JASPoolAllocObject<TDopplerAudible4Kart>::memPool_;
JASMemPool<JAISeq> JASPoolAllocObject<JAISeq>::memPool_;
JASMemPool<JAISe> JASPoolAllocObject<JAISe>::memPool_;
#endif
