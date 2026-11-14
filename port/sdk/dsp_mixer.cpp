// High-level emulation of the JAudio2 DSP microcode (the jdsp[] image the game
// uploads in libs/JSystem/JAudio/Task/dsptask.c), replacing the DSP task glue.
//
// The microcode is the "Zelda" DAC ucode family (CRC 0x2FCDF1EC, shared with
// Wind Waker's successors and Pikmin 2). The rendering below follows Dolphin's
// HLE of it (Source/Core/Core/HW/DSPHLE/UCodes/Zelda.cpp), which matches
// Dolphin's DSP LLE on this game: TChannel is the 0xC0-word voice parameter
// block (VPB), Fxline the reverb parameter block.
//
// Contract with the CPU side (JASDSPInterface.cpp, JASDriver, JASAramStream):
//  - DsetupTable hands over the 64 TChannel blocks, the table holding the
//    4-tap/64-phase resampling filter, the oscillator patterns and the Dolby
//    sine table, the AFC coefficient table and the 4 FX (reverb) lines.
//  - DsyncFrame2(subframes, L, R) starts a frame of subframes x 80 samples; the
//    DSP waits for DSPReleaseHalt2 (one per 16-channel group, the last is 3),
//    renders one subframe and mails 0xF355FF00. The CPU updates the channels
//    again for the next subframe; the last mail of the frame lets the CPU sync
//    the next frame (JASAudioThread::DSPCallback, JASDriver::finishDSPFrame).
//  - Voice samples are mixed into 80-sample buffers named by their DSP
//    data-memory address. Only the front L/R buffers (0x0D00/0x0D60) reach the
//    output, scaled by the 4.12 mixer level (DsetMixerLevel); the reverb lines
//    feed back through their own buffers (0x0DC0..0x0EE0) into the front ones.
//    The back/surround buffers are kept but, as in Dolphin's HLE, not output.
#include "port_sdk.h"
#include "JSystem/JAudio/System/JASDsp.h"
#include "JSystem/JAudio/dsp.h"

#define CHECK_OFFSET(f, off) typedef char check_##f[(__builtin_offsetof(JASDsp::TChannel, f) == (off)) ? 1 : -1]
CHECK_OFFSET(mPitch, 0x04);
CHECK_OFFSET(_10, 0x10);
CHECK_OFFSET(_50, 0x50);
CHECK_OFFSET(_60, 0x60);
CHECK_OFFSET(_68, 0x68);
CHECK_OFFSET(_70, 0x70);
CHECK_OFFSET(_78, 0x78);
CHECK_OFFSET(_80, 0x80);
CHECK_OFFSET(_A8, 0xA8);
CHECK_OFFSET(_B0, 0xB0);
CHECK_OFFSET(_D0, 0xD0);
CHECK_OFFSET(_100, 0x100);
CHECK_OFFSET(_118, 0x118);
CHECK_OFFSET(mFir8FilterParam, 0x120);
CHECK_OFFSET(mIirFilterParam, 0x148);
typedef char check_size[sizeof(JASDsp::TChannel) == 0x180 ? 1 : -1];
typedef char check_fx[sizeof(JASDsp::Fxline) == 0x20 ? 1 : -1];

extern "C" u8 *port_aram(void);

extern "C" void port_dsp_dbg_dump(const s16 *l, const s16 *r, int n);

namespace {

using JASDsp::TChannel;

enum { SUBFRAME = 80, NUM_CH_MAX = 64, ARAM_SIZE = 0x01000000 };

// Mixing buffers, by DSP data-memory address.
enum {
    BUF_FRONT_L,    // 0x0D00
    BUF_FRONT_R,    // 0x0D60
    BUF_BACK_L,     // 0x0F40
    BUF_BACK_R,     // 0x0CA0
    BUF_FRONT_L_FX, // 0x0E80 (reverb line 2)
    BUF_FRONT_R_FX, // 0x0EE0 (reverb line 3)
    BUF_BACK_L_FX,  // 0x0C00
    BUF_BACK_R_FX,  // 0x0C50
    BUF_FX0,        // 0x0DC0 (reverb line 0)
    BUF_FX1,        // 0x0E20 (reverb line 1)
    BUF_09A0,
    BUF_0FA0,
    BUF_0B00,
    NUM_BUFS
};

int bufferForId(u16 id)
{
    switch (id) {
    case 0x0D00: return BUF_FRONT_L;
    case 0x0D60: return BUF_FRONT_R;
    case 0x0F40: return BUF_BACK_L;
    case 0x0CA0: return BUF_BACK_R;
    case 0x0E80: return BUF_FRONT_L_FX;
    case 0x0EE0: return BUF_FRONT_R_FX;
    case 0x0C00: return BUF_BACK_L_FX;
    case 0x0C50: return BUF_BACK_R_FX;
    case 0x0DC0: return BUF_FX0;
    case 0x0E20: return BUF_FX1;
    case 0x09A0: return BUF_09A0;
    case 0x0FA0: return BUF_0FA0;
    case 0x0B00: return BUF_0B00;
    default: return -1;
    }
}

// VPB fields the TChannel declaration leaves unnamed (byte offsets)
enum {
    VPB_END_REACHED = 0x0A,    // _0A
    VPB_CONST_SAMPLE = 0x66,   // _66: last output sample, played while paused
    VPB_AFC_LEFT = 0x64,       // _64: decoded AFC samples not yet consumed
    VPB_RESAMPLE_HIST = 0x78,  // _78[4]
    VPB_FIR_HIST = 0x80,       // _80[20]
    VPB_BIQUAD_HIST = 0xA8,    // _A8[4]: x1, x2, y1, y2
    VPB_AFC_SAMPLES = 0xB0,    // _B0[16]: last decoded AFC block
    VPB_LOWPASS_HIST = 0xD0,   // y1, x1
    VPB_FIR_COEFS = 0x120,     // 20 taps
    VPB_BIQUAD_COEFS = 0x148,  // b1, b2, a1, a2
    VPB_LOWPASS_COEF = 0x150,  // setDistFilter
};

inline s16 *vpb16(TChannel *ch, int off) { return (s16 *)((u8 *)ch + off); }

// sources (TChannel::_100)
enum {
    SRC_SQUARE = 0,
    SRC_SAW = 1,
    SRC_SQUARE_25 = 3,
    SRC_PATTERN_1 = 4,
    SRC_AFC_LQ = 5,
    SRC_PATTERN_0 = 7,
    SRC_PCM8 = 8,
    SRC_AFC_HQ = 9,
    SRC_PATTERN_0_VAR = 10,
    SRC_PATTERN_2 = 11,
    SRC_PATTERN_3 = 12,
    SRC_PCM16 = 16,
};

typedef s16 MixBuf[SUBFRAME];

TChannel *sChannels;
int sNumChannels;
const u32 *sResTable;  // s16 pairs packed in host-order u32: 0x100 resampling taps, 4x0x40 patterns, 0x80 sine
const u8 *sAdpcmTable; // 16 coefficient pairs, big-endian s16
JASDsp::Fxline *sFx;
u16 sMixerLevel = 0x4000; // 4.12
int sDebug; // MKDD_DSP_DEBUG: 1 voice starts, 2 bus/feature stats, 3 active voices, 4 voice ends + MKDD_DSP_DUMP

MixBuf sBuf[NUM_BUFS];
s16 sPatterns[4][0x40];
bool sPatternsLoaded;
u16 sFxBlock[4];
s16 sFxLast8[4][8];

// current frame
s16 *sOutL, *sOutR;
u32 sSubframes, sSubframe;
bool sFrameActive;
u32 sSamples; // output samples rendered (debug timestamps)

DSPCallback sCallback;
u32 sMail[32];
int sMailHead, sMailTail;

struct Stats {
    u32 subframes, voices, paused, dolby, fir, biquad, lowpass, osc, forced;
    double energy[NUM_BUFS];
} sStats;

inline s16 clamp16(s32 v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : (s16)v; }
inline s16 be16(const u8 *p) { return (s16)(p[0] << 8 | p[1]); }

s16 table16(u32 i)
{
    u32 w = sResTable[i >> 1];
    return (s16)((i & 1) ? (w & 0xFFFF) : (w >> 16));
}

const u8 *aram(u32 addr, u32 len)
{
    if (addr >= ARAM_SIZE || len > ARAM_SIZE - addr)
        return NULL;
    return port_aram() + addr;
}

// ---- mixing helpers (Zelda ucode arithmetic) ----

// dst += src * vol, vol a 1.15 volume ramping by step per sample (both in 16.16)
s32 addWithVolumeRamp(s16 *dst, const s16 *src, s32 vol, s32 step)
{
    if (!vol && !step)
        return vol;
    for (int i = 0; i < SUBFRAME; i++) {
        dst[i] = clamp16(dst[i] + (((vol >> 16) * src[i]) >> 16));
        vol += step;
    }
    return vol;
}

void addWithVolume(s16 *dst, const s16 *src, int count, s16 vol)
{
    for (int i = 0; i < count; i++)
        dst[i] = clamp16(dst[i] + clamp16((src[i] * vol) >> 15));
}

void applyVolume(s16 *buf, u16 vol, int fracBits)
{
    for (int i = 0; i < SUBFRAME; i++)
        buf[i] = clamp16((buf[i] * (s32)vol) >> fracBits);
}

// ---- reverb (FX lines) ----

// Before the voices: each enabled line reads its oldest block, filters it with
// its 8 taps and sends it to its two destinations; the line's own buffer starts
// from that (feedback), the voices add to it and it is written back afterwards.
void reverbPre()
{
    static const int lineBuf[4] = {BUF_FX0, BUF_FX1, BUF_FRONT_L_FX, BUF_FRONT_R_FX};
    for (int i = 0; i < 4; i++) {
        s16 *out = sBuf[lineBuf[i]];
        JASDsp::Fxline *fx = sFx ? &sFx[i] : NULL;
        if (!fx || !fx->_00 || !fx->_04 || !fx->_02) {
            __builtin_memset(out, 0, sizeof(MixBuf));
            continue;
        }
        if (sFxBlock[i] >= fx->_02)
            sFxBlock[i] = 0;
        const s16 *line = fx->_04 + sFxBlock[i] * SUBFRAME;
        s16 buf[8 + SUBFRAME];
        __builtin_memcpy(buf, sFxLast8[i], sizeof sFxLast8[i]);
        __builtin_memcpy(buf + 8, line, SUBFRAME * sizeof(s16));
        __builtin_memcpy(sFxLast8[i], buf + SUBFRAME, sizeof sFxLast8[i]);
        auto filter = [&] {
            for (int n = 0; n < SUBFRAME; n++) {
                s64 acc = 0;
                for (int k = 0; k < 8; k++)
                    acc += buf[n + k] * fx->mFilterTable[k];
                buf[n] = clamp16((s32)(acc >> 15));
            }
        };
        if (fx->_00 & 1)
            filter();
        const u16 dst[2][2] = {{fx->_08, fx->_0A}, {fx->_0C, fx->_0E}};
        for (int d = 0; d < 2; d++) {
            int b = dst[d][0] ? bufferForId(dst[d][0]) : -1;
            if (b >= 0)
                addWithVolume(sBuf[b], buf, SUBFRAME, (s16)dst[d][1]);
        }
        if (fx->_00 & 2)
            filter();
        __builtin_memcpy(out, buf, sizeof(MixBuf));
    }
}

void reverbPost()
{
    static const int lineBuf[4] = {BUF_FX0, BUF_FX1, BUF_FRONT_L_FX, BUF_FRONT_R_FX};
    for (int i = 0; i < 4; i++) {
        JASDsp::Fxline *fx = sFx ? &sFx[i] : NULL;
        if (!fx || !fx->_00 || !fx->_04 || !fx->_02)
            continue;
        __builtin_memcpy(fx->_04 + sFxBlock[i] * SUBFRAME, sBuf[lineBuf[i]], sizeof(MixBuf));
        sFxBlock[i] = (u16)((sFxBlock[i] + 1) % fx->_02);
    }
}

void prepareFrame()
{
    __builtin_memset(sBuf[BUF_FRONT_L], 0, sizeof(MixBuf));
    __builtin_memset(sBuf[BUF_FRONT_R], 0, sizeof(MixBuf));
    applyVolume(sBuf[BUF_BACK_L], 0x6784, 15);
    applyVolume(sBuf[BUF_BACK_R], 0x6784, 15);

    reverbPre();
    // Dolby reverb sends of the previous subframe, folded into the front reverb lines
    addWithVolume(sBuf[BUF_FRONT_L_FX], sBuf[BUF_BACK_L_FX], SUBFRAME, 0x7FFF);
    addWithVolume(sBuf[BUF_FRONT_R_FX], sBuf[BUF_BACK_L_FX], SUBFRAME, (s16)0xB820);
    addWithVolume(sBuf[BUF_FRONT_L_FX], sBuf[BUF_BACK_R_FX] + 0x28, 0x28, (s16)0xB820);
    addWithVolume(sBuf[BUF_FRONT_R_FX], sBuf[BUF_BACK_R_FX] + 0x28, 0x28, 0x7FFF);
    __builtin_memset(sBuf[BUF_BACK_L_FX], 0, sizeof(MixBuf));
    __builtin_memset(sBuf[BUF_BACK_R_FX], 0, sizeof(MixBuf));
    // never output; keep them from saturating
    __builtin_memset(sBuf[BUF_09A0], 0, sizeof(MixBuf));
    __builtin_memset(sBuf[BUF_0FA0], 0, sizeof(MixBuf));
    __builtin_memset(sBuf[BUF_0B00], 0, sizeof(MixBuf));

    // oscillator patterns 2 and 3 evolve every subframe
    if (!sPatternsLoaded && sResTable) {
        for (int p = 0; p < 4; p++)
            for (int i = 0; i < 0x40; i++)
                sPatterns[p][i] = table16(0x100 + p * 0x40 + i);
        sPatternsLoaded = true;
    }
    s16 *pattern2 = sPatterns[2];
    s32 yn2 = pattern2[0x40 - 2], yn1 = pattern2[0x40 - 1], v;
    for (int i = 0; i < 0x40; i += 2) {
        v = yn2 * yn1 - (pattern2[i] << 16);
        yn2 = yn1;
        yn1 = pattern2[i];
        pattern2[i] = (s16)(v >> 16);
        v = 2 * (yn2 * yn1 + (pattern2[i + 1] << 16));
        yn2 = yn1;
        yn1 = pattern2[i + 1];
        pattern2[i + 1] = (s16)(v >> 16);
    }
    s16 *pattern3 = sPatterns[3];
    yn2 = pattern3[0x40 - 2];
    yn1 = pattern3[0x40 - 1];
    s16 acc = (s16)yn1;
    s16 step = (s16)(pattern3[0] + ((yn1 * yn2 + ((yn2 << 16) + yn1)) >> 16));
    step = (s16)((step & 0x1FF) | 0x2000);
    for (s32 i = 0; i < 0x40; ++i)
        pattern3[i] = (s16)(acc + (i + 1) * step);
}

// ---- sample sources ----

template <typename T> void downloadPcm(TChannel *ch, s16 *dst, u32 count)
{
    if (ch->_08) {
        ch->_74 = ch->_114 - ch->_68;
        ch->_70 = ch->_118 + ch->_68 * sizeof(T);
    }
    u16 &endReached = *(u16 *)vpb16(ch, VPB_END_REACHED);
    endReached = 0;
    while (count) {
        if (endReached) {
            endReached = 0;
            if (!ch->_102 || ch->_110 >= ch->_114) {
                __builtin_memset(dst, 0, count * sizeof(s16));
                ch->mIsFinished = 1;
                break;
            }
            ch->_68 = ch->_110;
            ch->_74 = ch->_114 - ch->_68;
            ch->_70 = ch->_118 + ch->_68 * sizeof(T);
        }
        u32 n = (u32)ch->_74 < count ? (u32)ch->_74 : count;
        const u8 *p = aram(ch->_70, n * sizeof(T));
        for (u32 i = 0; i < n; i++)
            *dst++ = !p ? 0 : sizeof(T) == 1 ? (s16)((s8)p[i] << 8) : be16(p + i * 2);
        ch->_74 -= n;
        ch->_70 += n * sizeof(T);
        count -= n;
        if (!ch->_74)
            endReached = 1;
    }
}

void decodeAfc(TChannel *ch, s16 *dst, u32 blocks)
{
    u32 bytes = ch->_100;
    const u8 *src = aram(ch->_70, blocks * bytes);
    ch->_70 += blocks * bytes;
    s16 *hist = vpb16(ch, VPB_AFC_SAMPLES);
    s32 yn1 = hist[15], yn2 = hist[14];
    for (u32 b = 0; b < blocks; b++) {
        if (!src) {
            __builtin_memset(dst, 0, 16 * sizeof(s16));
            dst += 16;
            continue;
        }
        s16 nibbles[16];
        s32 delta = 1 << ((*src >> 4) & 0xF);
        int idx = *src & 0xF;
        src++;
        if (bytes == SRC_AFC_HQ) {
            for (int i = 0; i < 16; i += 2) {
                nibbles[i] = (s16)((s16)((*src >> 4) << 12) >> 1);
                nibbles[i + 1] = (s16)((s16)((*src & 0xF) << 12) >> 1);
                src++;
            }
        } else {
            for (int i = 0; i < 16; i += 4) {
                for (int k = 0; k < 4; k++)
                    nibbles[i + k] = (s16)((s16)(((*src >> (6 - 2 * k)) & 3) << 14) >> 1);
                src++;
            }
        }
        s32 c1 = sAdpcmTable ? be16(sAdpcmTable + idx * 4) : 0;
        s32 c2 = sAdpcmTable ? be16(sAdpcmTable + idx * 4 + 2) : 0;
        for (int i = 0; i < 16; i++) {
            s32 s = clamp16((delta * nibbles[i] + yn1 * c1 + yn2 * c2) >> 11);
            *dst++ = (s16)s;
            yn2 = yn1;
            yn1 = s;
        }
    }
    hist[14] = (s16)yn2;
    hist[15] = (s16)yn1;
}

void downloadAfc(TChannel *ch, s16 *dst, u32 count)
{
    s16 *cache = vpb16(ch, VPB_AFC_SAMPLES);
    u16 &left = ch->_64;
    if (ch->_08) {
        cache[15] = cache[14] = 0;
        left = 0;
        ch->_74 = ch->_114;
        ch->_70 = ch->_118;
    }
    if (left > 16)
        left = 16;
    for (int guard = 0; guard < 64; guard++) {
        u32 out = left < count ? left : count;
        const s16 *base = cache + 16 - left;
        for (u32 i = 0; i < out; i++)
            *dst++ = base[i];
        left -= out;
        count -= out;
        if (!count)
            return;
        u32 remain = (u32)ch->_74;
        if (count <= remain) {
            u32 blocks = (count + 15) >> 4;
            u32 decoded = blocks << 4;
            if (decoded < remain) {
                left = (u16)(decoded - count);
                ch->_74 = remain - decoded;
            } else {
                left = (u16)(remain - count);
                ch->_74 = 0;
            }
            decodeAfc(ch, dst, blocks);
            if (left) {
                for (int i = 0; i < 16; i++)
                    cache[i] = dst[decoded - 16 + i];
                if (!ch->_74 && ch->_114) {
                    // the end is not block aligned: keep the samples before it
                    const s16 *b = cache + ((ch->_114 + 15) & 15);
                    for (u32 i = 0; i < left; i++)
                        cache[15 - i] = *b--;
                }
            }
            return;
        }
        // more samples needed than left before the end: finish, or loop
        if (remain) {
            count -= remain;
            decodeAfc(ch, dst, (remain + 15) >> 4);
            dst += remain;
        }
        if (!ch->_102 || ch->_110 >= ch->_114) {
            ch->mIsFinished = 1;
            __builtin_memset(dst, 0, count * sizeof(s16));
            return;
        }
        ch->_70 = ch->_118 + (ch->_110 >> 4) * ch->_100;
        cache[14] = ch->_106;
        cache[15] = ch->_104;
        decodeAfc(ch, cache, 1);
        left = (u16)(16 - (ch->_110 & 15));
        ch->_74 = ch->_114 - left - ch->_110;
    }
    __builtin_memset(dst, 0, count * sizeof(s16));
}

void resample(TChannel *ch, const s16 *src, s16 *dst)
{
    u32 ratio = (u16)ch->mPitch;
    u32 pos = ch->_60;
    if ((ratio >> 12) >= 4) {
        for (int n = 0; n < SUBFRAME; n++) {
            pos += ratio;
            dst[n] = src[pos >> 12];
        }
    } else {
        for (int n = 0; n < SUBFRAME; n++) {
            u32 c = ((pos & 0xFFF) >> 6) * 4;
            const s16 *in = src + (pos >> 12);
            s64 acc = 0; // 40-bit accumulator on the DSP: full-scale input overflows 32 bits
            if (sResTable) {
                for (int k = 0; k < 4; k++)
                    acc += 2 * (s64)table16(c + k) * in[k];
                acc >>= 16;
            } else {
                acc = in[1];
            }
            dst[n] = clamp16((s32)acc);
            pos += ratio;
        }
    }
    s16 *hist = vpb16(ch, VPB_RESAMPLE_HIST);
    for (int i = 0; i < 4; i++)
        hist[i] = src[(pos >> 12) + i];
    *vpb16(ch, VPB_CONST_SAMPLE) = dst[SUBFRAME - 1];
    ch->_60 = (u16)(pos & 0xFFF);
}

void loadInputSamples(TChannel *ch, s16 *buf)
{
    if (ch->mPauseFlag) {
        s16 c = *vpb16(ch, VPB_CONST_SAMPLE);
        for (int n = 0; n < SUBFRAME; n++)
            buf[n] = c;
        return;
    }
    u16 type = ch->_100;
    switch (type) {
    case SRC_SQUARE:
    case SRC_SQUARE_25: {
        u32 shift = type == SRC_SQUARE ? 1 : 2;
        u32 mask = (1u << shift) - 1;
        u32 ratio = (u32)(u16)ch->mPitch << (shift - 1);
        u32 pos = (u32)ch->_60 << shift;
        for (int n = 0; n < SUBFRAME; n++) {
            buf[n] = ((pos >> 16) & mask) ? (s16)0xC000 : 0x4000;
            pos += ratio;
        }
        ch->_60 = (u16)((pos >> shift) & 0xFFFF);
        sStats.osc++;
        return;
    }
    case SRC_SAW: {
        u32 pos = ch->_60;
        for (int n = 0; n < SUBFRAME; n++) {
            buf[n] = (s16)(pos & 0xFFFF);
            pos += (u16)ch->mPitch >> 1;
        }
        ch->_60 = (u16)pos;
        sStats.osc++;
        return;
    }
    case SRC_PATTERN_0:
    case SRC_PATTERN_0_VAR:
    case SRC_PATTERN_1:
    case SRC_PATTERN_2:
    case SRC_PATTERN_3: {
        int idx = type == SRC_PATTERN_1 ? 1 : type == SRC_PATTERN_2 ? 2 : type == SRC_PATTERN_3 ? 3 : 0;
        const s16 *pattern = sPatterns[idx];
        u32 pos = (u32)ch->_60 << 6;
        u32 step = (u32)(u16)ch->mPitch << 5;
        for (int n = 0; n < SUBFRAME; n++) {
            buf[n] = pattern[pos >> 16];
            pos = (pos + step) % (0x40u << 16);
            if (type == SRC_PATTERN_0_VAR)
                pos = ((pos << 10) + sBuf[BUF_BACK_R][n] * (s32)(u16)ch->mPitch) >> 10;
        }
        ch->_60 = (u16)(pos >> 6);
        sStats.osc++;
        return;
    }
    case SRC_PCM8:
    case SRC_PCM16:
    case SRC_AFC_LQ:
    case SRC_AFC_HQ: {
        s16 raw[4 + 0x500 + 0x10];
        __builtin_memcpy(raw, vpb16(ch, VPB_RESAMPLE_HIST), 4 * sizeof(s16));
        u32 needed = ((u32)ch->_60 + SUBFRAME * (u32)(u16)ch->mPitch) >> 12;
        if (type == SRC_PCM8)
            downloadPcm<s8>(ch, raw + 4, needed);
        else if (type == SRC_PCM16)
            downloadPcm<s16>(ch, raw + 4, needed);
        else
            downloadAfc(ch, raw + 4, needed);
        resample(ch, raw, buf);
        return;
    }
    default:
        __builtin_memset(buf, 0, sizeof(MixBuf));
        return;
    }
}

// ---- per-voice filters ----

void lowPass(TChannel *ch, s16 *buf)
{
    s16 *h = vpb16(ch, VPB_LOWPASS_HIST);
    s32 yn1 = ch->_08 ? 0 : h[0], xn1 = ch->_08 ? 0 : h[1];
    s32 coef = (u16)*vpb16(ch, VPB_LOWPASS_COEF);
    for (int n = 0; n < SUBFRAME; n++) {
        s32 xn0 = buf[n];
        s64 t = ((s64)(xn0 - xn1) * coef >> 7) + yn1;
        s16 yn0 = (s16)(t > 32767 ? 32767 : t < -32768 ? -32768 : t);
        buf[n] = yn0;
        yn1 = yn0;
        xn1 = xn0;
    }
    h[0] = (s16)yn1;
    h[1] = (s16)xn1;
}

// The variable-length FIR (TChannel::setFIR8FilterParam, up to 20 taps); its
// exact arithmetic is not documented, tap 0 weights the newest sample (the
// neutral filter the game sets is {0x7FFF, 0, ...}).
void fir(TChannel *ch, s16 *buf, int taps)
{
    if (taps > 20)
        taps = 20;
    s16 *h = vpb16(ch, VPB_FIR_HIST);
    const s16 *c = vpb16(ch, VPB_FIR_COEFS);
    for (int n = 0; n < SUBFRAME; n++) {
        for (int k = 19; k > 0; k--)
            h[k] = h[k - 1];
        h[0] = buf[n];
        s64 acc = 0;
        for (int k = 0; k < taps; k++)
            acc += c[k] * h[k];
        buf[n] = clamp16((s32)(acc >> 15));
    }
}

void biquad(TChannel *ch, s16 *buf)
{
    s16 *h = vpb16(ch, VPB_BIQUAD_HIST);
    const s16 *c = vpb16(ch, VPB_BIQUAD_COEFS);
    s32 xn1 = h[0], xn2 = h[1], yn1 = h[2], yn2 = h[3];
    for (int n = 0; n < SUBFRAME; n++) {
        s32 xn0 = buf[n];
        s64 t = (s64)c[0] * xn1 + (s64)c[1] * xn2 + (s64)c[2] * yn1 + (s64)c[3] * yn2;
        t >>= 15;
        s16 yn0 = (s16)(t > 32767 ? 32767 : t < -32768 ? -32768 : t);
        buf[n] = yn0;
        xn2 = xn1;
        xn1 = xn0;
        yn2 = yn1;
        yn1 = yn0;
    }
    h[0] = (s16)xn1;
    h[1] = (s16)xn2;
    h[2] = (s16)yn1;
    h[3] = (s16)yn2;
}

// ---- voices ----

void addVoice(int idx, TChannel *ch)
{
    if (!ch->mIsActive || ch->mIsFinished)
        return;
    if (ch->_08 && sDebug)
        fprintf(stderr, "[dsp] @%u ch%d start: src %u, aram %x, end %u, loop %u@%u, pitch %x, filter %x, %s\n", sSamples, idx, ch->_100,
                ch->_118, ch->_114, ch->_102, ch->_110, (u16)ch->mPitch, ch->mFilterMode,
                ch->_58 ? "auto mixer" : "mixers");
    sStats.voices++;
    if (ch->mPauseFlag)
        sStats.paused++;
    if (ch->mForcedStop)
        sStats.forced++;

    MixBuf in;
    loadInputSamples(ch, in);
    if (*vpb16(ch, VPB_LOWPASS_COEF)) {
        lowPass(ch, in);
        sStats.lowpass++;
    }
    if (ch->mFilterMode & 0x1F) {
        fir(ch, in, ch->mFilterMode & 0x1F);
        sStats.fir++;
    }
    const s16 *bq = vpb16(ch, VPB_BIQUAD_COEFS);
    if ((ch->mFilterMode & 0x20) && (bq[0] != 0x7FFF || bq[1] || bq[2] || bq[3])) {
        biquad(ch, in);
        sStats.biquad++;
    }

    if (ch->_58) {
        // auto mixer: Dolby positional volume _54 -> _56, position _50 (0XXXXXXX 0YYYYYYY), reverb _52
        sStats.dolby++;
        if (ch->mForcedStop) {
            ch->_56 = (u16)((s16)ch->_54 / 2);
            if (!ch->_56)
                ch->mIsFinished = 1;
        }
        u32 x = (ch->_50 >> 8) & 0x7F, y = ch->_50 & 0x7F;
        s32 right = sResTable ? table16(0x200 + x) : 0x5A82, back = sResTable ? table16(0x200 + y) : 0;
        s32 left = sResTable ? table16(0x200 + (x ^ 0x7F)) : 0x5A82, front = sResTable ? table16(0x200 + (y ^ 0x7F)) : 0x7FFF;
        s16 quad[4] = {(s16)((left * front) >> 15), (s16)((left * back) >> 15), (s16)((right * front) >> 15),
                       (s16)((right * back) >> 15)};
        s16 delta = (s16)((s16)ch->_56 - (s16)ch->_54);
        s16 qdelta[4], rev[4], revDelta[4];
        for (int i = 0; i < 4; i++)
            qdelta[i] = (s16)(((u16)quad[i] * delta) >> 15);
        for (int i = 0; i < 4; i++)
            quad[i] = (s16)((quad[i] * (s16)ch->_54) >> 15);
        for (int i = 0; i < 4; i++) {
            rev[i] = (s16)((quad[i] * (s16)ch->_52) >> 15);
            revDelta[i] = (s16)((qdelta[i] * (s16)ch->_52) >> 15);
        }
        static const int dest[8] = {BUF_FRONT_L,    BUF_BACK_L,    BUF_FRONT_R,    BUF_BACK_R,
                                    BUF_FRONT_L_FX, BUF_BACK_L_FX, BUF_FRONT_R_FX, BUF_BACK_R_FX};
        for (int i = 0; i < 8; i++) {
            s16 v = i < 4 ? quad[i] : rev[i - 4], d = i < 4 ? qdelta[i] : revDelta[i - 4];
            addWithVolumeRamp(sBuf[dest[i]], in, (s32)v << 16, ((s32)d << 16) / SUBFRAME);
        }
        ch->_54 = ch->_56;
    } else {
        TChannel::Mixer *mix = ch->_10; // 6 slots: _10[4] runs on into _30
        if (ch->mForcedStop) {
            bool mute = true;
            for (int i = 0; i < 6; i++) {
                mix[i]._02 = (s16)(mix[i]._04 / 2);
                mute &= mix[i]._02 == 0;
            }
            if (mute)
                ch->mIsFinished = 1;
        }
        for (int i = 0; i < 6; i++) {
            if (!mix[i].busConnect)
                continue;
            s16 delta = (s16)(mix[i]._02 - mix[i]._04);
            s32 step = ((s32)delta << 16) / SUBFRAME;
            if (!mix[i]._04 && !step)
                continue;
            int b = bufferForId(mix[i].busConnect);
            if (b < 0)
                continue;
            s32 v = addWithVolumeRamp(sBuf[b], in, (s32)mix[i]._04 << 16, step);
            mix[i]._04 = (s16)(v >> 16);
        }
    }

    if (sDebug >= 4 && ch->mIsFinished)
        fprintf(stderr, "[dsp] @%u ch%d finished (%s)\n", sSamples, idx, ch->mForcedStop ? "stopped" : "ended");
    if (!ch->mPauseFlag)
        ch->_08 = 0;
}

void postMail(u32 mail);

void renderSubframe()
{
    prepareFrame();
    for (int i = 0; i < sNumChannels; i++)
        addVoice(i, &sChannels[i]);

    s16 *l = sOutL + sSubframe * SUBFRAME, *r = sOutR + sSubframe * SUBFRAME;
    applyVolume(sBuf[BUF_FRONT_L], sMixerLevel, 12);
    applyVolume(sBuf[BUF_FRONT_R], sMixerLevel, 12);
    __builtin_memcpy(l, sBuf[BUF_FRONT_L], sizeof(MixBuf));
    __builtin_memcpy(r, sBuf[BUF_FRONT_R], sizeof(MixBuf));
    reverbPost();
    if (sDebug >= 4)
        port_dsp_dbg_dump(l, r, SUBFRAME);
    sSamples += SUBFRAME;

    if (sDebug >= 2) {
        for (int b = 0; b < NUM_BUFS; b++)
            for (int n = 0; n < SUBFRAME; n++)
                sStats.energy[b] += (double)sBuf[b][n] * sBuf[b][n];
        if (++sStats.subframes == 2000) {
            static const char *names[NUM_BUFS] = {"0D00", "0D60", "0F40", "0CA0", "0E80", "0EE0", "0C00",
                                                  "0C50", "0DC0", "0E20", "09A0", "0FA0", "0B00"};
            fprintf(stderr, "[dsp] %u voice-subframes: paused %u dolby %u fir %u biquad %u lowpass %u osc %u forced %u; rms:",
                    sStats.voices, sStats.paused, sStats.dolby, sStats.fir, sStats.biquad, sStats.lowpass, sStats.osc,
                    sStats.forced);
            for (int b = 0; b < NUM_BUFS; b++)
                fprintf(stderr, " %s=%.0f", names[b], __builtin_sqrt(sStats.energy[b] / (2000.0 * SUBFRAME)));
            fprintf(stderr, "\n");
            __builtin_memset(&sStats, 0, sizeof sStats);
            for (int i = 0; sDebug >= 3 && i < sNumChannels; i++) {
                TChannel *ch = &sChannels[i];
                if (!ch->mIsActive || ch->mIsFinished)
                    continue;
                fprintf(stderr, "[dsp]   ch%d src %u aram %x left %d loop %u pitch %x pause %u stop %u ", i, ch->_100,
                        ch->_118, ch->_74, ch->_102, (u16)ch->mPitch, ch->mPauseFlag, ch->mForcedStop);
                if (ch->_58)
                    fprintf(stderr, "auto vol %d->%d pos %04x fx %04x\n", (s16)ch->_54, (s16)ch->_56, ch->_50, ch->_52);
                else
                    for (int m = 0; m < 6; m++)
                        fprintf(stderr, "%04x:%d%s", ch->_10[m].busConnect, ch->_10[m]._04, m == 5 ? "\n" : " ");
            }
        }
    }
}

void deliverMail(void *, void *)
{
    if (sCallback)
        sCallback(NULL);
}

void postMail(u32 mail)
{
    int next = (sMailTail + 1) % 32;
    if (next == sMailHead)
        return;
    sMail[sMailTail] = mail;
    sMailTail = next;
    port_defer(deliverMail, NULL, NULL);
}

} // namespace

extern "C" u32 DSPCheckMailFromDSP(void) { return sMailHead != sMailTail ? 0x80000000u : 0; }

extern "C" u32 DSPReadMailFromDSP(void)
{
    u32 mail;
    if (sMailHead == sMailTail)
        return 0;
    mail = sMail[sMailHead];
    sMailHead = (sMailHead + 1) % 32;
    return mail;
}

void DspBoot(DSPCallback callback)
{
    sCallback = callback;
    sDebug = port_env("MKDD_DSP_DEBUG");
}

void DspFinishWork(u16) {}

void DsetupTable(u32 numChannels, u32 channels, u32 resTable, u32 adpcmTable, u32 fxLines)
{
    sNumChannels = numChannels > NUM_CH_MAX ? NUM_CH_MAX : (int)numChannels;
    sChannels = (JASDsp::TChannel *)channels;
    sResTable = (const u32 *)resTable;
    sAdpcmTable = (const u8 *)adpcmTable;
    sFx = (JASDsp::Fxline *)fxLines;
}

void DsetMixerLevel(f32 level) { sMixerLevel = (u16)(4096.0f * level); }

void DsyncFrame2(u32 subframes, u32 left, u32 right)
{
    sSubframes = subframes;
    sSubframe = 0;
    sOutL = (s16 *)left;
    sOutR = (s16 *)right;
    sFrameActive = sChannels && sOutL && sOutR && subframes;
}

// The microcode renders a subframe once the CPU released all four channel groups.
void DSPReleaseHalt2(u32 msg)
{
    if (msg != 3 || !sFrameActive)
        return;
    renderSubframe();
    if (++sSubframe >= sSubframes)
        sFrameActive = false;
    postMail(0xF355FF00);
}

// .ast stream headers are read raw from the disc (JASAramStream::headerLoad/load);
// put their fields in host order. The sample data stays big-endian, as the DSP reads it.
static u32 bswap32(u32 v) { return v >> 24 | (v >> 8 & 0xFF00) | (v << 8 & 0xFF0000) | v << 24; }
static u16 bswap16(u16 v) { return (u16)(v >> 8 | v << 8); }

void port_ast_header(void *p)
{
    u8 *h = (u8 *)p;
    static const u8 w32[] = {0x00, 0x10, 0x18, 0x1C, 0x20};
    static const u8 w16[] = {0x0A, 0x0C, 0x0E};
    for (unsigned i = 0; i < sizeof w32; i++)
        *(u32 *)(h + w32[i]) = bswap32(*(u32 *)(h + w32[i]));
    for (unsigned i = 0; i < sizeof w16; i++)
        *(u16 *)(h + w16[i]) = bswap16(*(u16 *)(h + w16[i]));
}

void port_ast_block(void *p)
{
    u8 *h = (u8 *)p;
    *(u32 *)h = bswap32(*(u32 *)h);
    *(u32 *)(h + 4) = bswap32(*(u32 *)(h + 4));
    for (int i = 0; i < 12; i++)
        *(u16 *)(h + 8 + i * 2) = bswap16(*(u16 *)(h + 8 + i * 2));
}
