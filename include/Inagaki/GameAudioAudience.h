#ifndef GAMEAUDIOAUDIENCE_H
#define GAMEAUDIOAUDIENCE_H


#include "JSystem/JAudio/JAUAudience.h"
namespace GameAudio {

template <int N>
class CustomAudience : public JAUDopplerAudience<N> {
public:
    typedef JAUDopplerAudible<N> Audible;
    typedef JAUDopplerAudibleChannel Channel;

    CustomAudience() {}
    ~CustomAudience() {} // 08, override
    void mixChannelOut(const JASSoundParams &params, JAIAudible *audible, int index); // 1C, override
    JAIAudible *newAudible(const JGeometry::TVec3f &pos, JAISoundID soundID, const JGeometry::TVec3f *p3, u32 channels); // 0C, override
    u32 calcPriority(JAIAudible *audible); // 18, override

    f32 calcVolume_(f32 dist, const f32 &maxDist, const f32 &slope) const {
        if (dist > maxDist) {
            return this->setting_.minVolume;
        }
        if (dist < this->setting_._08) {
            return 1.0f;
        }
        return slope * (dist - this->setting_._08) + 1.0f;
    }

    u32 calcDeltaPriority_(f32 dist, u16 distType, bool p2) const {
        if (dist > mDistanceThreshold[distType]) {
            if (p2) {
                return -1;
            }
            return this->setting_._38;
        }

        if (dist < this->setting_._08) {
            return 0;
        }

        return this->setting_._48 * (dist - this->setting_._08);
    }

    void init(int i) { // fabricated
        mEchoRate[i] = 0.0f;
        if (i < N) // ???
            smCameraVolume[i] = 1.0f;
    }

    void setEchoRate(int i, f32 rate) {
        mEchoRate[i] = rate;
    }

    void updateVolume() {
        smCameraVolume[0] = 1.0f;
        smCameraVolume[1] = 1.0f;
        smCameraVolume[2] = 1.0f;
        smCameraVolume[3] = 1.0f;
    }

    void initSlopesAndThresholds() { // fabricated
        const f32& minVolume = setting_.minVolume;
        const f32& refDistance = setting_._04;
        const f32& nearDistance = setting_._08;

        f32 fullVolume = 1.0f;
        f32 volume2x = 2.0f;
        f32 volume3x = 3.0f;
        f32 volume4x = 4.0f;
        f32 volumeHalf = 0.5f;
        f32 volumeQuarter = 0.25f;
        f32 volumeSixth = 0.16666667f;
        f32 volumeEighth = 0.125f;

        mVolumeSlope[0] = (minVolume - fullVolume) / (( refDistance * volume2x) - nearDistance);
        mVolumeSlope[1] = (minVolume - fullVolume) / (( refDistance * volume3x) - nearDistance);
        mVolumeSlope[2] = (minVolume - fullVolume) / (( refDistance * volume4x) - nearDistance);
        mVolumeSlope[3] = (minVolume - fullVolume) / (( refDistance * volumeHalf) - nearDistance);
        mVolumeSlope[4] = (minVolume - fullVolume) / (( refDistance * volumeQuarter) - nearDistance);
        mVolumeSlope[5] = (minVolume - fullVolume) / (( refDistance * volumeSixth) - nearDistance);
        mVolumeSlope[6] = (minVolume - fullVolume) / (( refDistance * volumeEighth) - nearDistance);

        mDistanceThreshold[0] = volume2x * refDistance;
        mDistanceThreshold[1] = volume3x * refDistance;
        mDistanceThreshold[2] = volume4x * refDistance;
        mDistanceThreshold[3] = volumeHalf * refDistance;
        mDistanceThreshold[4] = volumeQuarter * refDistance;
        mDistanceThreshold[5] = volumeSixth * refDistance;
        mDistanceThreshold[6] = volumeEighth * refDistance;
    }

    static f32 smCameraVolume[N];

    f32 mVolumeSlope[7];       // 1a4 (2x, 3x, 4x, 1/2, 1/4, 1/6, 1/8)
    f32 mDistanceThreshold[7]; // 1c0
    f32 mEchoRate[N]; // 1dc
}; // Size: 0x1ec(probably variable)

}

#include "Inagaki/GameAudioMain.h"
#include "Inagaki/GameAudioHostIO.h"

namespace GameAudio {

template <int N>
u32 CustomAudience<N>::calcPriority(JAIAudible *i_audible) {
    u32 deltaPriority[N];
    Audible *audible = (Audible *)i_audible;
    Channel *channel;
    if (!audible->getAudibleParam()._00.bytes.b0_4) {
        for (int i = 0; i < this->numPlayers_; i++) {
            channel = audible->getChannel(i);
            if (channel) {
                channel->_2C = this->state[i].convertAbsToRel(audible->mPos, &channel->_20, &channel->_14);
            }
        }
        return 0;
    }

    u32 rv = 0xffffffff;
    for (int i = 0; i < this->numPlayers_; i++) {
        channel = audible->getChannel(i);
        if (channel) {
            channel->_2C = this->state[i].convertAbsToRel(audible->mPos, &channel->_20, &channel->_14);
            u16 sw = audible->getAudibleParam()._00.half.f1;
            u16 distType = (sw >> 12) & 7;
            if (!(sw & 0x8000)) {
                if (distType != 0) {
                    if (channel->_2C > mDistanceThreshold[distType - 1]) {
                        deltaPriority[i] = 0xffffffff;
                    }
                    else {
                        deltaPriority[i] = calcDeltaPriority_(channel->_2C, (u16)(distType - 1), audible->getAudibleParam()._00.bytes.b0_7);
                    }
                }
                else {
                    deltaPriority[i] = JAUDopplerAudience<N>::calcDeltaPriority_(channel->_2C, audible->getAudibleParam()._00.bytes.b0_7);
                }
            }
            else if (distType == 0 || distType >= 4) {
                deltaPriority[i] = JAUDopplerAudience<N>::calcDeltaPriority_(channel->_2C, audible->getAudibleParam()._00.bytes.b0_7);
            }
            else {
                deltaPriority[i] = calcDeltaPriority_(channel->_2C, (u16)(distType - 1), audible->getAudibleParam()._00.bytes.b0_7);
            }

            if (deltaPriority[i] < rv) {
                rv = deltaPriority[i];
            }
        }
    }
    return rv;
}

template <int N>
JAIAudible *CustomAudience<N>::newAudible(const JGeometry::TVec3f &pos, JAISoundID soundID, const JGeometry::TVec3f *p3, u32 channels) {
    u32 channelNum = channels | ~((1 << this->numPlayers_) - 1);
    if (channelNum == 0xffffffff) {
#line 192
        JUT_WARNING_F2("%s", "You masked all audible channels !")
        return NULL;
    }

    Audible *audible = new Audible(pos, p3, channelNum);
    if (audible == NULL) {
        return NULL;
    }

    for (int i = 0; i < this->numPlayers_; i++) {
        Channel *channel = audible->getChannel(i);
        if (channel) {
            channel->_2C = this->state[i].convertAbsToRel(audible->mPos, &channel->_20, &channel->_14);
        }
    }

    CustomSoundTable *table = Main::getAudio()->getSoundTable();
    if (table) {
        JAUAudibleParam audibleParams;
        audibleParams._00.half.f0 = table->getAudibleSw(soundID);
        u32 swBit = table->getSwBit(soundID);
        audibleParams._00.half.f1 = ((swBit >> 4) & 0x7000) + (swBit & 0xfff);
        if ((u8)Parameters::getRaceMode() == 2) {
            audibleParams._00.half.f1 += 0x8000;
        }
        audible->setAudibleParam(audibleParams);
    }

    return audible;
}

template <int N>
void CustomAudience<N>::mixChannelOut(const JASSoundParams &params, JAIAudible *pAudible, int index) {
    Audible *audible = (Audible *)pAudible;
    Channel *channel = audible->getChannel(index);
    if (!channel) {
        return;
    }

    JASSoundParams newParams;
    if (audible->getAudibleParam()._00.bytes.b0_5) {
        f32 dist = channel->_2C;
        u16 sw = audible->getAudibleParam()._00.half.f1 & 0x7000;
        newParams.mVolume = !sw ? calcVolume_(dist, this->setting_._04, this->setting_._44)
            : calcVolume_(dist, mDistanceThreshold[(u16)((sw >> 12) - 1)], mVolumeSlope[(u16)((sw >> 12) - 1)]);

        if (audible->getAudibleParam()._00.bytes.b1_2_7 & 1) {
            JGeometry::TVec3f &camPos = this->state[index]._3c;
            f32 height = fabs(camPos.y - audible->mPos.y);
            if (height > 1500.0f) {
                height = 1500.0f;
            }
            if (height > 300.0f) {
                newParams.mVolume *= (1200.0f - (height - 300.0f)) / 1200.0f;
            }
        }

        newParams.mVolume *= smCameraVolume[index];
        if (this->numPlayers_ > 2) {
            int other = index ^ 2;
            if (audible->getChannel(other)) {
                f32 camDist = PSVECDistance(&this->state[index]._3c, &this->state[other]._3c);
                if (camDist < GA_MULTI_CAMERA_VOL_DOWN_DISTANCE) {
                    newParams.mVolume *= 1.0f - (GA_MULTI_CAMERA_VOL_DOWN_RATE * (GA_MULTI_CAMERA_VOL_DOWN_DISTANCE - camDist)) / GA_MULTI_CAMERA_VOL_DOWN_DISTANCE;
                }
            }
        }
    }
    else {
        newParams.mVolume = 1.0f;
    }

    if (audible->getAudibleParam()._00.bytes.b1_1 && this->numPlayers_ == 1) {
        f32 dolby;
        if (channel->_14.z > this->setting_._14) {
            dolby = this->setting_._18;
        }
        else if (channel->_14.z < this->setting_._10) {
            dolby = this->setting_._20;
        }
        else if (channel->_14.z < 0.0f) {
            dolby = this->setting_._3c * channel->_14.z + this->setting_._1c;
        }
        else {
            dolby = this->setting_._40 * channel->_14.z + this->setting_._1c;
        }
        newParams.mDolby = dolby;
    }
    else {
        newParams.mDolby = 0.0f;
    }

    if (audible->getAudibleParam()._00.bytes.b1_0) {
        if (this->numPlayers_ > 1) {
            newParams.mPan = (index & 1) ? 1.0f : 0.0f;
        }
        else {
            f32 pan = channel->_2C <= JGeometry::TUtilf::epsilon() ? 0.0f : channel->_14.x / channel->_2C;
            newParams.mPan = pan * this->setting_._2c + 0.5f;
            if (Parameters::getMirrorSwitch()) {
                newParams.mPan = 1.0f - newParams.mPan;
            }
        }
    }
    else {
        newParams.mPan = 0.5f;
    }

    if (Main::getAudio()->getPauseStatus() != Main::PAUSE_ALL) {
        newParams.mPitch = this->calcPitch_(channel, audible, index);
    }

    f32 fxMix;
    if (audible->getAudibleParam()._00.bytes.b0_6) {
        if (channel->_2C > this->setting_._04) {
            fxMix = this->setting_._28;
        }
        else if (channel->_2C < this->setting_._08) {
            fxMix = this->setting_._24;
        }
        else {
            fxMix = this->setting_._4c * (channel->_2C - this->setting_._08) + this->setting_._24;
        }
    }
    else {
        fxMix = 0.0f;
    }
    newParams.mFxMix = fxMix;
    newParams.mFxMix += mEchoRate[index];

    audible->getOuterParams(index)->combine(params, newParams);
}

}

#endif
