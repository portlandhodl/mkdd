
#include "Inagaki/GameAudioCamera.h"
#include "Inagaki/GameSoundMgr.h"
#include "JSystem/JGeometry/Vec.h"


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

void GameAudio::ShoreSoundMgr::setTide(u8 tide, JGeometry::TVec3f *p1, JGeometry::TVec3f *p2) {
#line 108
    JUT_ASSERT_MSG(tide < 4, "ShoreSoundMgr::setSe camnum over");
    if (mKillSw)
        return;

    for (u8 i = 0; i < 2; i++) {
        PosMove *move = (i == 0) ? &_68[tide] : &_6c[tide];
        JGeometry::TVec3f *pos = (i == 0) ? p1 : p2;

        if (!pos) {
            move->_0 = 0;
        }
        else if (!move->_0) {
            move->_4.set(*pos);
            move->_0 = 1;
            move->_28 = 0;
        }
        else {
            f32 eps = JGeometry::TUtilf::epsilon();
            if (-eps <= move->_4.x - pos->x && move->_4.x - pos->x <= eps
             && -eps <= move->_4.y - pos->y && move->_4.y - pos->y <= eps
             && -eps <= move->_4.z - pos->z && move->_4.z - pos->z <= eps) {
            }
            else {
                move->_10.set(*pos);
                move->_1c.set(*pos);
                move->_28 = (_5d == 1) ? 180 : 30;
            }
        }

        if (move->_28) {
            move->_28--;
            if (!move->_28) {
                move->_4.set(move->_1c);
            }
            else {
                move->_4.x += (move->_10.x - move->_4.x) / 30.0f;
                move->_4.y += (move->_10.y - move->_4.y) / 30.0f;
                move->_4.z += (move->_10.z - move->_4.z) / 30.0f;
            }
        }
    }

    u32 num = Main::getAudio()->getCamera()->getSceneMax();
    if (tide >= num)
        return;

    PosMove *a = &_68[tide];
    PosMove *b = &_6c[tide];
    if (!a->_0 && !b->_0)
        return;

    JGeometry::TVec3f &playPos = getPlayPos(tide);
    if (!a->_0) {
        playPos.set(b->_4);
    }
    else if (!b->_0) {
        playPos.set(a->_4);
    }
    else {
        f32 dx = b->_4.x - a->_4.x;
        f32 dy = b->_4.y - a->_4.y;
        f32 dz = b->_4.z - a->_4.z;
        f32 lenSq = dx * dx + dy * dy + dz * dz;
        f32 ratio = 0.0f;
        if (lenSq != 0.0f) {
            ratio = ((playPos.x - a->_4.x) * dx + (playPos.y - a->_4.y) * dy
                   + (playPos.z - a->_4.z) * dz) / lenSq;
        }

        if (ratio <= 0.0f) {
            playPos.set(a->_4);
        }
        else if (ratio >= 1.0f) {
            playPos.set(b->_4);
        }
        else {
            playPos.x = a->_4.x + dx * ratio;
            playPos.y = a->_4.y + dy * ratio;
            playPos.z = a->_4.z + dz * ratio;
        }
    }

    u32 seID = 0x40024 - _5d;
    JAISoundHandle &handle = (*this)[tide];
    JAISoundStarter::getInstance()->startSound(seID, &handle, nullptr);

    if (!handle) {
        handle.getSound();
    }
    if (handle->getAudible() != nullptr)
        return;

    u32 mask = (1 << tide) ^ 0xf;
    JAISound *sound = handle.getSound();
    *(u32 *)((u8 *)sound + 0x1c) = 0;

    JGeometry::TVec3f tmp = playPos;
    handle->newAudible(tmp, &playPos, mask, smAudience);
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
