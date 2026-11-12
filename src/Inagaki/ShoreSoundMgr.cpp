
#include "Inagaki/GameAudioCamera.h"
#include "Inagaki/GameSoundMgr.h"
#include "JSystem/JGeometry/Vec.h"
#include "JSystem/JAudio/Interface/JAISe.h"


namespace GameAudio {

JGeometry::TVec3f sShoreDummyVec(0, 0, 0);

GameAudio::ShoreSoundMgr::ShoreSoundMgr(JKRHeap *heap) : SoundMgr<ShoreSoundMgr>(&sShoreDummyVec, heap, 4) {
    _5c = 0;
    _64 = new JGeometry::TVec3f[4];
    _68 = new PosMove[4];
    _6c = new PosMove[4];

    for (u8 i = 0; i < 4; i++) {
        _64[i].set(0.0f, 0.0f, 0.0f);
    }
    _5d = 0;
    _60 = 0.0f;
}

GameAudio::ShoreSoundMgr::~ShoreSoundMgr() {}

void GameAudio::ShoreSoundMgr::loop() {
#line 79
    JUT_ASSERT_MSG(mSoundPos, "ShoreSoundMgr::loop　disable中です！！！！！\n");
    if (mKillSw)
        return;

    const u32 sceneMax = Main::getAudio()->getCamera()->getSceneMax();
    for (s32 index = 0; index < (u32)sceneMax; index++) {
        if ((*this)[index] == nullptr) {
            continue;
        }

        const JGeometry::TVec3f &playPos = getPlayPos(index);
        JAIAudible *audible = (*this)[index]->getAudible();
        if (audible) {
            audible->setPos(playPos);
        }
    }
    _5c = 0;
}
void GameAudio::ShoreSoundMgr::frameWork() {
    if (mKillSw)
        return;
    _5c = 1;
}

static inline bool isNearXY(const JGeometry::TVec3f &a, const JGeometry::TVec3f &b, f32 eps) {
    bool equal = false;
    if (JGeometry::TUtilf::epsilonEquals(a.x, b.x, eps) && JGeometry::TUtilf::epsilonEquals(a.y, b.y, eps)) {
        equal = true;
    }
    return equal;
}

static inline bool isNear(const JGeometry::TVec3f &a, const JGeometry::TVec3f &b) {
    f32 eps = JGeometry::TUtilf::epsilon();
    bool equal = false;
    if (isNearXY(a, b, eps) && JGeometry::TUtilf::epsilonEquals(a.z, b.z, eps)) {
        equal = true;
    }
    return equal;
}

void GameAudio::ShoreSoundMgr::setTide(u8 tide, JGeometry::TVec3f *p1, JGeometry::TVec3f *p2) {
#line 108
    JUT_ASSERT_MSG(tide < 4, "ShoreSoundMgr::setSe camnum over");
    if (mKillSw)
        return;

    PosMove *move;
    JGeometry::TVec3f *pos;
    for (u8 i = 0; i < 2; i++) {
        if (i == 0) {
            move = &_68[tide];
            pos = p1;
        }
        else {
            move = &_6c[tide];
            pos = p2;
        }

        if (pos == nullptr) {
            move->_0 = 0;
        }
        else if (move->_0 == 0) {
            move->_4 = *pos;
            move->_0 = 1;
            move->_28 = 0;
        }
        else if (!isNear(move->_4, *pos)) {
            move->_10 = (*pos - move->_4) * (1.0f / 30.0f);
            move->_1c = *pos;
            if (_5d == 1) {
                move->_28 = 180;
            }
            else {
                move->_28 = 30;
            }
        }

        if (move->_28) {
            move->_28--;
            if (move->_28 == 0) {
                move->_4 = move->_1c;
            }
            else {
                move->_4 += move->_10;
            }
        }
    }

    if (tide >= Main::getAudio()->getCamera()->getSceneMax())
        return;

    PosMove *a = &_68[tide];
    PosMove *b = &_6c[tide];
    if (!a->_0 && !b->_0)
        return;

    if (!a->_0) {
        _64[tide] = b->_4;
    }
    else if (!b->_0) {
        _64[tide] = a->_4;
    }
    else if (!isNear(a->_4, b->_4)) {
        CameraInfo *camInfo = Main::getAudio()->getCamera()->getCameraInfo(tide);
        Vec ab = a->_4 - b->_4;
        Vec ac = a->_4 - camInfo->_0;
        f32 dx = a->_4.x - b->_4.x;
        f32 dy = a->_4.y - b->_4.y;
        f32 dz = a->_4.z - b->_4.z;
        f32 ratio = (ab.x * ac.x + ab.y * ac.y + ab.z * ac.z) / (dx * dx + dy * dy + dz * dz);
        if (ratio <= 0.0f) {
            _64[tide] = a->_4;
        }
        else if (ratio >= 1.0f) {
            _64[tide] = b->_4;
        }
        else {
            Vec newPos = a->_4 + (b->_4 - a->_4) * ratio;
            _64[tide] = (JGeometry::TVec3f &)newPos;
        }
    }
    else {
        _64[tide] = a->_4;
    }

    u32 seID = _5d ? 0x40022 : 0x40023;
    JAISoundHandle &handle = (*this)[tide];
    JAISoundStarter *starter = JAISoundStarter::getInstance();
    if (seID == 0)
        return;

    starter->startSound(seID, &handle, nullptr);
    if (handle->getAudible() != nullptr)
        return;

    u32 mask = (1 << tide) ^ 0xf;
    handle.getSound()->asSe()->audience_ = nullptr;
    handle->newAudible(JGeometry::TVec3f(_64[tide]), &_64[tide], mask, smAudience);
}


void GameAudio::ShoreSoundMgr::setStatus(bool p1, f32 p2) {
    _5d = p1;
    _60 = p2;
}
    
}
#include "JSystem/JAudio/JASFakeMatch.h"

#ifdef MATCHING // TODO

template <>
JASMemPool< JAUDopplerAudible<4> > JASPoolAllocObject< JAUDopplerAudible<4> >::memPool_;
template <>
JASMemPool<JAISe> JASPoolAllocObject<JAISe>::memPool_;

#endif
