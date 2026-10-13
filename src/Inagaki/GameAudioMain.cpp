#include "Inagaki/GameAudioMain.h"

#include "JSystem/JAudio/System/JASAudioThread.h"
#include "JSystem/JAudio/System/JASWaveArcLoader.h"
#include "JSystem/JAudio/System/JASWaveBank.h"
#include "JSystem/JAudio/System/JASDriver.h"
#include "JSystem/JAudio/System/JASAudioReseter.h"
#include "JSystem/JAudio/System/JASReport.h"
#include "JSystem/JAudio/System/JASSeqParser.h"
#include "JSystem/JAudio/JAUAudioArchive.h"
#include "JSystem/JAudio/JAUInitializer.h"
#include "JSystem/JAudio/Interface/JAISoundHandles.h"
#include "JSystem/JUtility/JUTAssert.h"

#include "Inagaki/GameAudioCamera.h"
#include "Inagaki/GameAudioCommon.h"
#include "Inagaki/GameAudioMgr.h"
#include "Inagaki/GameSoundMgr.h"
#include "Inagaki/GameMapSoundMgr.h"

namespace GameAudio {

Main *Main::msBasic;
static const void *mLastBaaPointer;
int GoalFanfareDelayFrame = 90;

Main::Main() {

}

void Main::init(JKRSolidHeap *heap, u32 audioMemSize, void *baaData, void *baaPath, u32 param5) {
    mSeHandles = new (heap, 0) JAISoundHandles(_84, 4);
    JASWaveArcLoader::setCurrentDir("AudioRes/Waves");
    JAU_JASInitializer jasInit;
    jasInit.mAudioMemSize = audioMemSize;
    jasInit.mDvdThreadPrio = 4;
    jasInit.mAudioThreadPrio = 3;
    jasInit.mTrackPoolCount = 0x60;
    jasInit.mChannelPoolCount = 0xb4;
    jasInit.mChannelMax = 2;
    jasInit.mDSPLevel = 1.4f;
    jasInit.initJASystem(heap);
    JASSeqParser::sCallBackFunc = processSeqCallback;
    JAU_JAIInitializer jaiInit;
    jaiInit.mSeqCount = 3;
    jaiInit.mSeCount = 0x64;
    jaiInit.mStreamCount = 3;
    jaiInit.mChildCount = jasInit.mChannelMax * 3 + 12;
    jaiInit.initJAInterface();
    mReseter = new (heap, 0) JASAudioReseter();
    mMgr = new (heap, 0) CustomMgr();
    mAudience = new (heap, 0) CustomAudience<4>();
    mAudience->setting_._04 = 1.4f;
    mAudience->setting_._08 = 19000.0f;
    mAudience->setting_.minVolume = 0.0f;
    mAudience->setting_._10 = -1500.0f;
    mAudience->setting_._14 = 2200.0f;
    mAudience->setting_._1c = 0.07874016f;
    mAudience->setting_._30 = 0.47f;
    mAudience->setting_._2c = 400.0f;
    mAudience->setting_._34 = 1.3f;
    mAudience->setting_._38 = 0;
    mMgr->getSeMgr().setAudience(mAudience);
    mAudience->initSlopesAndThresholds();
    ShoreSoundMgr::smAudience = new (heap, 0) CustomAudience<4>();
    ShoreSoundMgr::smAudience->initSlopesAndThresholds();
    ShoreSoundMgr::smAudience->setting_._04 = 1.4f;
    ShoreSoundMgr::smAudience->setting_._08 = 3000.0f;
    ShoreSoundMgr::smAudience->setting_.minVolume = 0.0f;
    ShoreSoundMgr::smAudience->setting_._10 = -1500.0f;
    ShoreSoundMgr::smAudience->setting_._14 = 2200.0f;
    ShoreSoundMgr::smAudience->setting_._1c = 0.07874016f;
    ShoreSoundMgr::smAudience->setting_._30 = 0.47f;
    ShoreSoundMgr::smAudience->setting_._2c = 400.0f;
    ShoreSoundMgr::smAudience->setting_._34 = 1.3f;
    ShoreSoundMgr::smAudience->setting_._38 = 0;
    // ... work in progress
}

void Main::bootDSP() {
    JASAudioThread::bootDSP();
}

bool Main::isActive() {
    return mState.pack & 0x10;
}

void Main::initRaceSound() {
    if (_70.isSoundAttached()) {
        _70->stop(0);
    }
    mState.bits.bgmRequest = false;
    mState.bits.goalProcess = false;
    SoundMgr<CharacterSoundMgr>::initAll();
    SoundMgr<KartSoundMgr>::initAll();
    SoundMgr<ObjectSoundMgr>::initAll();
    SoundMgr<RockSoundMgr>::initAll();
    SoundMgr<NpcCarSoundMgr>::initAll();
    SoundMgr<ShoreSoundMgr>::initAll();
    SoundMgr<CoasterSoundMgr>::initAll();
    SoundMgr<MapSoundObjectMgr>::initAll();
    SoundMgr<CircleSoundMgr>::initAll();
    switch (Parameters::getPlayerMode()) {
    case 0:
        setRaceSeVolume(GA_PLAY_SE_VOLUME_1P, 0);
        break;
    case 1:
        setRaceSeVolume(GA_PLAY_SE_VOLUME_2P, 0);
        break;
    case 2:
        setRaceSeVolume(GA_PLAY_SE_VOLUME_3P, 0);
        break;
    case 3:
        setRaceSeVolume(GA_PLAY_SE_VOLUME_4P, 0);
        break;
    }
    mStreamVolumeFactor = 1.0f;
    updateBgmVolume(0);
}

void Main::frameWork() {
    checkResetInner();
    checkBgm();
    SoundMgr<CharacterSoundMgr>::frameWorkAll();
    SoundMgr<KartSoundMgr>::frameWorkAll();
    SoundMgr<ObjectSoundMgr>::frameWorkAll();
    SoundMgr<RockSoundMgr>::frameWorkAll();
    SoundMgr<NpcCarSoundMgr>::frameWorkAll();
    SoundMgr<ShoreSoundMgr>::frameWorkAll();
    checkThunderMode();
    checkCourceOnRace();
    checkSection();
    JAUSoundMgr *soundMgr = mMgr;
    soundMgr->calc();
    soundMgr->mixOut();
    _80++;
}

void Main::changeSection(u32 sectionNo) {
    bool success;
    mCurrentBaa = NULL;
    const void *baa = mBaaList[sectionNo + 1];
    if (mLastBaaPointer == baa) {
        success = true;
    }
    else if (mSectionHeap->getNumSections() != 1 && !mSectionHeap->popSection()) {
        success = false;
    }
    else {
        JAUSection *section = mSectionHeap->pushNewSection();
        JAUAudioArcLoader loader(section);
        loader.load(baa);
        section->finishBuild();
        mLastBaaPointer = baa;
        success = true;
    }
    if (!success) {
        mCurrentBaa = mBaaList[sectionNo + 1];
    }
}

void Main::checkSection() {
    const void *baa = mCurrentBaa;
    if (baa == NULL) {
        return;
    }
    bool success;
    if (mLastBaaPointer == baa) {
        success = true;
    }
    else if (mSectionHeap->getNumSections() != 1 && !mSectionHeap->popSection()) {
        success = false;
    }
    else {
        JAUSection *section = mSectionHeap->pushNewSection();
        {
            JAUAudioArcLoader loader(section);
            loader.load(baa);
            section->finishBuild();
        }
        mLastBaaPointer = baa;
        success = true;
    }
    if (success) {
        mCurrentBaa = NULL;
    }
}

bool Main::isWaveLoaded(WS_ID id) {
    JAUSectionHeap* sectionHeap = JASGlobalInstance<JAUSectionHeap>::sInstance;
    if(sectionHeap != nullptr)
    {
        JASWaveBank* bank = sectionHeap->sectionHeapData_.waveBankTable.getWaveBank(id);

        if(bank != nullptr)
        {
            JASWaveArc* arc = bank->getWaveArc(0);
            if (arc->_4c == 2) {
                return true;
            }
        }
    }

    return false;
}

void Main::checkCourceOnRace() {}

void Main::setPause(PAUSE_STATUS status) {
    if (mPauseStatus == status) {
        return;
    }
    if (mPauseStatus != PAUSE_ALL && (status == PAUSE_NONE || status == PAUSE_SE)) {
        return;
    }
    mPauseStatus = status;
    switch (status) {
    case PAUSE_ALL:
        mStreamVolumeFactor = 0.30708662f;
        SoundMgr<KartSoundMgr>::pauseAll(true);
        SoundMgr<CharacterSoundMgr>::pauseAll(true);
        updateBgmVolume(30);
        break;
    case PAUSE_NONE:
        mStreamVolumeFactor = 1.0f;
        SoundMgr<KartSoundMgr>::pauseAll(false);
        SoundMgr<CharacterSoundMgr>::pauseAll(false);
        updateBgmVolume(30);
        break;
    case PAUSE_SE:
        SoundMgr<KartSoundMgr>::stopAll();
        SoundMgr<CharacterSoundMgr>::stopAll();
        break;
    }
}

void Main::startSequenceBgm(u32 id) {
    JAISoundHandle *handle;
    switch (mTable->getPTrack(id)) {
    case 1:
        handle = &_6c;
        break;
    case 2:
        if (Parameters::getDemoMode() != 0) {
            return;
        }
        if (_6c.isSoundAttached()) {
            _6c->getAuxiliary().moveVolume(0.0f, 5);
        }
        mState.bits.bgmRequest = true;
        handle = &_70;
        break;
    default:
#line 756
        JUT_ASSERT_MSG(false, "Main::startSequenceBgm Track Error!!\n");
        break;
    }
    ((JAUSoundMgr *)mMgr)->startSound(id, handle, NULL);
    if (!handle->isSoundAttached()) {
#line 763
        JUT_WARNING_F(false, "%s", "GameAudioMain : startSequenceBgm failed\n");
    }
    switch (id) {
    case 0x1000000:
        changeSection(0);
        if (_50 != 0) {
            _74 = 0;
            _78 = 0;
            _7c = 0;
            _49 = 0;
            _4a = 0;
            mPauseStatus = PAUSE_NONE;
            mStreamVolumeFactor = 1.0f;
            _4b = 0;
            _50 = 1;
            if (_70.isSoundAttached()) {
                _70->stop(0);
            }
            Parameters::setDemoMode(0);
        }
        updateBgmVolume(0);
        _50 = 1;
        break;
    case 0x1000001:
        changeSection(0);
        _50 = 2;
        mStreamVolumeFactor = 1.0f;
        updateBgmVolume(0);
        break;
    case 0x1000004:
        mStreamVolumeFactor = 1.0f;
        updateBgmVolume(0);
        break;
    }
    if (handle->isSoundAttached()) {
        switch (mTable->getPTrack(id)) {
        case 1:
            if (_70.isSoundAttached()) {
                _6c->getAuxiliary().moveVolume(0.0f, 0);
            }
            break;
        case 2:
            break;
        default:
#line 811
            JUT_ASSERT_MSG(false, "Main::startSequenceBgm Track Error!!\n");
            break;
        }
    }
}

void Main::stopSequenceBgm(u32 fadeCount) {
    if (_6c.isSoundAttached()) {
        _6c.operator->();
        _6c->stop(fadeCount);
    }
}

u32 Main::getPlayingSequenceID() {
    if (_6c.isSoundAttached()) {
        return _6c->getID();
    }
    return -1;
}

void Main::prepareStreamBgm(u32 id) {
    if (Parameters::getDemoMode() != 0) {
        return;
    }
    switch (id) {
    case 0x2000020:
    case 0x2000021:
    case 0x2000022:
    case 0x2000026:
    case 0x2000031:
        setGoalProcess();
        break;
    case 0x2000027:
        changeSection(2);
        break;
    case 0x200002f:
        if (KartSoundMgr::smEntryKartCount - KartSoundMgr::smGoalKartCount > 2) {
            id = 0x2000020;
        }
        setGoalProcess();
        break;
    case 0x2000030:
        if (KartSoundMgr::smEntryKartCount - KartSoundMgr::smGoalKartCount > 2) {
            id = 0x2000021;
        }
        setGoalProcess();
        break;
    }
    ((JAUSoundMgr *)mMgr)->startSound(id, &_68, NULL);
    if (_68.isSoundAttached()) {
        _68->status_.lockWhenPrepared();
    }
    else {
#line 877
        JUT_WARNING_F(false, "%s", "GameAudioMain : prepareStreamBgm failed\n");
    }
    _7c = id;
}

void Main::startStreamBgm(u32 id) {
    if (Parameters::getDemoMode() == 1) {
        return;
    }
    switch (id) {
    case 0x200002f:
        if (_7c == 0x2000020) {
            if (KartSoundMgr::smEntryKartCount == KartSoundMgr::smGoalKartCount) {
                id = 0x200002f;
            }
            else {
                id = 0x2000020;
            }
        }
        break;
    case 0x2000030:
        if (_7c == 0x2000021) {
            if (KartSoundMgr::smEntryKartCount == KartSoundMgr::smGoalKartCount) {
                id = 0x2000030;
            }
            else {
                id = 0x2000021;
            }
        }
        break;
    case 0x2000024:
        if (_68.isSoundAttached()) {
            if (_68->getID() == 0x2000023 && _7c == 0x2000024) {
                _7c = 0;
                return;
            }
        }
        break;
    }
    if (_50 == 4 && id != 0x200002c) {
        return;
    }
    if (_6c.isSoundAttached()) {
        if (_6c->getID() == 0x1000009 || _6c->getID() == 0x100000b) {
            if ((id >= 0x2000010 && id <= 0x200001f) || (id >= 0x2000023 && id <= 0x2000025)) {
                _78 = id;
                return;
            }
            if (id == 0x2000026) {
                _6c->getAuxiliary().moveVolume(0.0f, 0);
                _78 = 0;
            }
        }
    }
    if (!(_68.isSoundAttached() && _68->getID() == id)) {
        ((JAUSoundMgr *)mMgr)->startSound(id, &_68, NULL);
    }
    else {
        _68->status_.unlockIfLocked();
        _7c = 0;
    }
    if (!_68.isSoundAttached()) {
#line 971
        JUT_WARNING_F(false, "%s", "GameAudioMain : startStreamBgm failed\n");
    }
    else if (id != 0x2000020 && id != 0x2000021 && id != 0x2000022 && id != 0x2000026) {
        if (_4a != 0) {
            _68->getAuxiliary().moveVolume(0.45f, 0);
        }
    }
}

void Main::stopStreamBgm(u32 fadeCount) {
    if (_68.isSoundAttached()) {
        _68.operator->();
        _68->stop(fadeCount);
    }
}

u32 Main::getPlayingStreamID() {
    if (_68.isSoundAttached() && _68->getCount() != 0u) {
        return _68->getID();
    }
    return -1;
}

void Main::setInvincibleBgm(u8 kartNo, u8 type) {
    if (_50 == 4) {
        return;
    }
    if (_4b == 0) {
        if (_68.isSoundAttached()) {
            if (mState.bits.goalProcess) {
                _78 = 0;
            }
            else {
                _78 = _68->getID();
            }
        }
        else {
            _78 = 0;
        }
        if (!_70.isSoundAttached()) {
            if (_78 != 0) {
                if (Parameters::getDemoMode() == 0) {
                    prepareStreamBgm(_78);
                }
            }
        }
    }
    startInvincibleBgm(type, true);
    {
        u8 count = _4b;
        u8 i;
        for (i = 0; i < count; i++) {
            if (_54[i].kartNo == kartNo && _54[i].type == type) {
                for (; i < _4b - 1; i++) {
                    _54[i].kartNo = _54[i + 1].kartNo;
                    _54[i].type = _54[i + 1].type;
                }
                _4b--;
                goto doneRemove;
            }
        }
    doneRemove:
        ;
    }
    {
        u8 i2 = _4b;
        _54[i2].kartNo = kartNo;
        _54[i2].type = type;
        if (_4b < 7) {
            _4b++;
        }
        else {
#line 1080
            JUT_WARNING_F(false, "%s", "Main::setInvincibleBgm mInvincibleBgmCount Over1!!!\n");
        }
    }
}

void Main::clearInvincibleBgm(u8 kartNo, u8 type) {
    u8 count = _4b;
    if (count > 1) {
        int limit = count - 1;
        u8 i;
        for (i = 0; i < limit; i++) {
            if (_54[i].kartNo == kartNo && _54[i].type == type) {
                for (; i < _4b - 1; i++) {
                    _54[i].kartNo = _54[i + 1].kartNo;
                    _54[i].type = _54[i + 1].type;
                }
                _4b--;
                return;
            }
        }
    }
    if (_4b == 0) {
        return;
    }
    _4b--;
    if (_4b == 0) {
        if (_6c.isSoundAttached()) {
            _6c.operator->();
            _6c->stop(0);
        }
        if (!_70.isSoundAttached()) {
            if (_78 != 0) {
                if (_78 == 0x2000025) {
                    _78 = 0x2000024;
                }
                startStreamBgm(_78);
            }
        }
    }
    else {
        startInvincibleBgm(_54[_4b - 1].type, false);
    }
}

void Main::startInvincibleBgm(u8 type, bool arg2) {
    u32 seqId = 0;
    switch (type) {
    case 1:
        seqId = 0x1000009;
        break;
    case 2:
        seqId = 0x100000b;
        break;
    }
    if (seqId == 0) {
        return;
    }
    if (!arg2) {
        if (!_6c.isSoundAttached()) {
            return;
        }
        if (_6c->getID() == seqId) {
            return;
        }
    }
    startSequenceBgm(seqId);
    if (_6c.isSoundAttached()) {
        if (mState.bits.goalProcess) {
            _6c->getAuxiliary().moveVolume(0.0f, 0);
        }
    }
}

void Main::checkBgm() {
    if (!_68.isSoundAttached()) {
        if (_6c.isSoundAttached()) {
            _6c->getAuxiliary().moveVolume(1.0f, 30);
        }
    }
    if (mState.bits.bgmRequest) {
        if (!_70.isSoundAttached()) {
            if (_7c != 0) {
                startStreamBgm(_7c);
            }
            if (_6c.isSoundAttached()) {
                _6c->getAuxiliary().moveVolume(1.0f, 60);
            }
            mState.bits.bgmRequest = false;
        }
    }
}

void Main::startSystemSe(u32 seID) {
    JAISoundHandle *handle = mSeHandles->getHandleSoundID(seID);
    if (handle == NULL) {
        handle = mSeHandles->getFreeHandle();
    }
    ((JAUSoundMgr *)mMgr)->startSound(seID, handle, NULL);
}

void Main::stopSystemSe(u32 seID) {
    JAISoundHandle *handle = mSeHandles->getHandleSoundID(seID);
    if (handle != NULL && handle->isSoundAttached()) {
        (*handle)->stop();
    }
}

void Main::startRaceSystemSe(u32 seID) {
    if (Parameters::getDemoMode() != 0 || (u8)Parameters::getRaceMode() == 3) {
        return;
    }
    JAISoundHandle *handle = mSeHandles->getHandleSoundID(seID);
    if (handle == NULL) {
        handle = mSeHandles->getFreeHandle();
    }
    ((JAUSoundMgr *)mMgr)->startSound(seID, handle, NULL);
}

void Main::startKartSystemSe(u8 kartNo, u32 seID) {
    if (Parameters::getDemoMode() != 0) {
        return;
    }
    if (kartNo >= 4) {
        return;
    }
    ((JAUSoundMgr *)mMgr)->startSound(seID, &_84[kartNo], NULL);
    if (_84[kartNo].isSoundAttached()) {
        if (getCamera()->getSceneMax() != 1) {
            _84[kartNo]->getAuxiliary().movePan(Common::setMultiPlayModePan(kartNo), 0);
        }
    }
}

void Main::checkThunderMode() {
    if (_4a == 0) {
        return;
    }
    static JAISoundHandle seHandle;
    if (Parameters::getDemoMode() != 0 || mPauseStatus == PAUSE_ALL) {
        return;
    }
    ((JAUSoundMgr *)mMgr)->startSound(0x40006, &seHandle, NULL);
}

void Main::setThunderMode(u8 kartNo) {
    if (Parameters::getDemoMode() != 0) {
        return;
    }
    if (_4a == 0) {
        if (_68.isSoundAttached()) {
            u32 streamId = getPlayingStreamID();
            if (streamId != 0x2000020 && streamId != 0x2000021 && streamId != 0x2000022 && streamId != 0x2000026) {
                _68->getAuxiliary().moveVolume(0.45f, 10);
            }
        }
    }
    _4a |= 1 << kartNo;
}

void Main::clearThunderMode(u8 kartNo) {
    _4a &= (1 << kartNo) ^ 0xff;
    if (_4a == 0) {
        if (_68.isSoundAttached()) {
            _68->getAuxiliary().moveVolume(1.0f, 70);
        }
    }
}

void Main::startBombPointSe(u8 kartNo, u8 type) {
    if (getAudio()->getCamera()->getSceneMax() == 2) {
        startRaceSystemSe(0x20029);
        return;
    }
    u8 kartPar = kartNo & 1;
    u8 typePar = type & 1;
    if (kartPar == typePar) {
        if (kartPar == 0) {
            startRaceSystemSe(0x2002c);
        }
        else {
            startRaceSystemSe(0x2002d);
        }
    }
    else if ((kartNo & 1) < (type & 1)) {
        startRaceSystemSe(0x2002a);
    }
    else {
        startRaceSystemSe(0x2002b);
    }
}

void Main::startBombReachSe(u8 kartNo, u8 type) {
    if (getAudio()->getCamera()->getSceneMax() == 2) {
        startRaceSystemSe(0x2002e);
        return;
    }
    u8 kartParity = kartNo & 1;
    u8 typeParity = type & 1;
    if (kartParity == typeParity) {
        if (kartParity == 0) {
            startRaceSystemSe(0x20031);
        }
        else {
            startRaceSystemSe(0x20032);
        }
    }
    else if (kartParity < typeParity) {
        startRaceSystemSe(0x2002f);
    }
    else {
        startRaceSystemSe(0x20030);
    }
}

void Main::setMasterVolume(s8 volume) {
    if (volume < -10) {
        volume = -10;
    }
    else if (volume > 10) {
        volume = 10;
    }
    f32 converted = volume;
    f32 level;
    if (volume > 0) {
        level = 0.6f * converted / 10.0f + 1.4f;
    }
    else if (volume < 0) {
        level = 0.75f * converted / 10.0f + 1.4f;
    }
    else {
        level = 1.4f;
    }
    JASDriver::setDSPLevel(level);
}

f32 Main::getMasterVolumeValue() { return JASDriver::getDSPLevel(); }

void Main::setBgmVolume(f32 volume) {
    if (mBgmVolume != volume) {
        mBgmVolume = volume;
        updateBgmVolume(0);
        if (volume == 0.0f) {
            setOutputMode(2);
        }
    }
}

void Main::updateBgmVolume(u32 frames) {
    f32 volume = mBgmVolume * mStreamVolumeFactor;
    JAISoundParamsMove *streamParams = (JAISoundParamsMove *)((u8 *)mMgr + 0x7a0);
    streamParams->moveVolume(1.3f * (1.2f * volume), frames);
    JAISoundParamsMove *seqParams = (JAISoundParamsMove *)((u8 *)mMgr + 0x73c);
    seqParams->moveVolume(1.3f * (1.5f * volume), frames);
}

void Main::fadeOutAll(u32 frames) {
    JASReport("fadeOutAll %d", frames);
    CustomMgr *mgr = mMgr;
    JAISoundParamsMove *streamParams = (JAISoundParamsMove *)((u8 *)mgr + 0x7a0);
    if (streamParams->mTransition.mVolume.mCount != 0) {
        if (_68.isSoundAttached()) {
            _68->getAuxiliary().moveVolume(((JAISoundParamsMove *)((u8 *)mgr + 0x7a0))->mVolume * _68->getAuxiliary().mVolume, 0);
        }
    }
    JAISoundParamsMove *seqParams = (JAISoundParamsMove *)((u8 *)mgr + 0x73c);
    *streamParams = *seqParams;
    if (streamParams->mTransition.mVolume.mCount != 0) {
        if (_6c.isSoundAttached()) {
            _6c->getAuxiliary().moveVolume(streamParams->mVolume * _6c->getAuxiliary().mVolume, 0);
        }
    }
    updateBgmVolume(0);
    ((JAIStreamMgr *)((u8 *)mgr + 0x79c))->stop(frames);
    ((JAISeqMgr *)((u8 *)mgr + 0x728))->stop(frames);
    _78 = 0;
    _7c = 0;
    _4b = 0;
    if (_50 != 0) {
        _50 = 4;
        setRaceSeVolume(0.0f, frames);
    }
}

void Main::setRaceSeVolume(f32 volume, u32 frames) {
    mMgr->getSeMgr().getCategory(0)->mParams.moveVolume(1.2f * volume, frames);
    mMgr->getSeMgr().getCategory(1)->mParams.moveVolume(1.75f * volume, frames);
    mMgr->getSeMgr().getCategory(2)->mParams.moveVolume(1.8f, 0);
    mMgr->getSeMgr().getCategory(3)->mParams.moveVolume(1.6f * volume, frames);
    mMgr->getSeMgr().getCategory(4)->mParams.moveVolume(1.4f * volume, frames);
}

void Main::setOutputMode(u32 mode) {
#line 1462
    JUT_ASSERT_MSG(mode <= 2, "GameAudio::setSoundOutputMode : invalid output mode.");
    JASDriver::setOutputMode(mode);
    mOutputMode = mode;
}

void Main::resetAudio(u32 frames) {
    bool success;
    mCurrentBaa = NULL;
    const void *baa = mBaaList[1];
    if (mLastBaaPointer == baa) {
        success = true;
    }
    else if (mSectionHeap->getNumSections() != 1 && !mSectionHeap->popSection()) {
        success = false;
    }
    else {
        JAUSection *section = mSectionHeap->pushNewSection();
        {
            JAUAudioArcLoader loader(section);
            loader.load(baa);
            section->finishBuild();
        }
        mLastBaaPointer = baa;
        success = true;
    }
    if (!success) {
        mCurrentBaa = mBaaList[1];
    }
    mState.bits.resetting = true;
    mReseter->start(frames * 7, false);
}

void Main::resumeAudio() {
    mStreamVolumeFactor = 1.0f;
    fadeOutAll(0);
    mReseter->resume();
    mState.bits.resetting = false;
    if (_50 != 0) {
        _74 = 0;
        _78 = 0;
        _7c = 0;
        _49 = 0;
        _4a = 0;
        mPauseStatus = PAUSE_NONE;
        mStreamVolumeFactor = 1.0f;
        _4b = 0;
        _50 = 1;
        if (_70.isSoundAttached()) {
            _70->stop(0);
        }
        Parameters::setDemoMode(0);
    }
}

void Main::checkResetInner() {
    if (mState.bits.resetting) {
        if (mReseter->checkDone()) {
            mMgr->getSeMgr().stop();
            fadeOutAll(0);
        }
    }
}

void Main::setGoalProcess() {
    if (_6c.isSoundAttached()) {
        _6c->getAuxiliary().moveVolume(0.0f, 0);
    }
    _78 = 0;
    mState.bits.goalProcess = true;
}

u16 Main::processSeqCallback(JASTrack *track, u16 port) {
    switch ((int)port) {
    case 10:
        JAUSection *section = msBasic->mSectionHeap->getSection(1);
        if (section != NULL && section->data_.registeredBankTables.test(7)) {
            if (msBasic->isWaveLoaded(STREAM3)) {
                return 1;
            }
        }
        u16 value = track->readPort(15);
        if (value >= 0x180) {
            return 2;
        }
        track->writePort(15, value + 1);
        return 0;
    }
    return 0;
}

}
