// High-level emulation of the JAudio2 DSP microcode (the jdsp[] image the game
// uploads in libs/JSystem/JAudio/Task/dsptask.c), replacing the DSP task glue.
//
// Contract with the CPU side (JASDSPInterface.cpp, JASDriver, JASAramStream):
//  - DsetupTable hands over the 64 JASDsp::TChannel blocks, the 4-tap
//    resampling table, the ADPCM coefficient table and the 4 FX lines.
//  - DsyncFrame2(subframes, L, R) starts a frame of subframes x 80 samples; the
//    DSP waits for DSPReleaseHalt2 (one per 16-channel group, the last is 3),
//    renders one subframe and mails 0xF355FF00. The CPU updates the channels
//    again for the next subframe; the last mail of the frame lets the CPU sync
//    the next frame (JASAudioThread::DSPCallback, JASDriver::finishDSPFrame).
//  - Per channel the CPU writes the wave (ARAM address _118, format via block
//    bytes _100 / block samples _64, loop flag _102, loop start _110, end/loop
//    end _114, loop predictor _104/_106, skip _68), pitch (Q12, 0x1000 = one
//    source sample per 32 kHz output sample), 6 mixer slots (bus address,
//    target and current volume), or the auto mixer (_50.._58, Dolby mode).
//    The DSP clears _08 once the voice started, keeps the fetch address in _70
//    and the samples left before the end/loop point in _74 (JASAramStream reads
//    both and adjusts _74/_110/_114/_102 while a stream plays), and raises
//    mIsFinished when a one-shot wave ends or a forced stop faded out.
//
// Where the microcode's exact arithmetic is unknown the choices here are
// plausible stand-ins: linear volume ramps across a subframe, the game's own
// 4-tap/64-phase table for resampling, the 8-tap FIR as written, a one-pole
// low-pass for the IIR mode, and FX lines as feedback delays through their FIR.
#include "port_sdk.h"
#include "JSystem/JAudio/System/JASDsp.h"
#include "JSystem/JAudio/dsp.h"

#define CHECK_OFFSET(f, off) typedef char check_##f[(__builtin_offsetof(JASDsp::TChannel, f) == (off)) ? 1 : -1]
CHECK_OFFSET(mPitch, 0x04);
CHECK_OFFSET(_10, 0x10);
CHECK_OFFSET(_50, 0x50);
CHECK_OFFSET(_70, 0x70);
CHECK_OFFSET(_100, 0x100);
CHECK_OFFSET(_118, 0x118);
CHECK_OFFSET(mFir8FilterParam, 0x120);
CHECK_OFFSET(mIirFilterParam, 0x148);
typedef char check_size[sizeof(JASDsp::TChannel) == 0x180 ? 1 : -1];
typedef char check_fx[sizeof(JASDsp::Fxline) == 0x20 ? 1 : -1];

extern "C" u8 *port_aram(void);

namespace {

enum { SUBFRAME = 80, NUM_CH_MAX = 64, ARAM_SIZE = 0x01000000 };

// mix buses, named by their DSP data-memory addresses (JASDsp connect/send tables)
enum { BUS_L, BUS_R, BUS_FX0, BUS_FX1, BUS_FX2, BUS_FX3, BUS_0CA0, BUS_0F40, BUS_S, BUS_0B00, BUS_C, NUM_BUSES };

int busIndex(u16 addr)
{
    switch (addr) {
    case 0x0D00: return BUS_L;
    case 0x0D60: return BUS_R;
    case 0x0DC0: return BUS_FX0;
    case 0x0E20: return BUS_FX1;
    case 0x0E80: return BUS_FX2;
    case 0x0EE0: return BUS_FX3;
    case 0x0CA0: return BUS_0CA0;
    case 0x0F40: return BUS_0F40;
    case 0x0FA0: return BUS_S;
    case 0x0B00: return BUS_0B00;
    case 0x09A0: return BUS_C;
    default: return -1;
    }
}

struct Voice {
    s16 win[4];      // resampler window: x[n-1], x[n], x[n+1], x[n+2]
    u32 frac;        // Q16 position between win[1] and win[2]
    s16 blk[16];     // decoded block
    int blkPos, blkLen;
    s32 hist1, hist2; // ADPCM predictor history
    s32 fir[8];      // FIR input history
    s32 iir;         // IIR output history
    u32 oscPhase;
    bool ended;
};

struct FxState {
    u32 block;
    s32 hist[8];
};

JASDsp::TChannel *sChannels;
int sNumChannels;
const u32 *sResTable;  // 64 phases x 4 taps (s16 pairs packed in host-order u32)
const u8 *sAdpcmTable; // 16 coefficient pairs, big-endian s16
JASDsp::Fxline *sFx;
u16 sMixerLevel = 0x4000;
int sDebug; // MKDD_DSP_DEBUG: log voice starts

Voice sVoices[NUM_CH_MAX];
FxState sFxState[4];

// current frame
s16 *sOutL, *sOutR;
u32 sSubframes, sSubframe;
bool sFrameActive;

DSPCallback sCallback;
u32 sMail[32];
int sMailHead, sMailTail;

s32 sBus[NUM_BUSES][SUBFRAME];

inline s16 clamp16(s32 v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : (s16)v; }
inline s16 be16(const u8 *p) { return (s16)(p[0] << 8 | p[1]); }

const u8 *aram(u32 addr, u32 len)
{
    if (addr >= ARAM_SIZE || len > ARAM_SIZE - addr)
        return NULL;
    return port_aram() + addr;
}

int samplesPerBlock(const JASDsp::TChannel *ch) { return (ch->_100 == 9 || ch->_100 == 5) ? 16 : 1; }

u32 byteOffset(const JASDsp::TChannel *ch, u32 sample)
{
    switch (ch->_100) {
    case 9: return sample / 16 * 9;
    case 5: return sample / 16 * 5;
    case 8: return sample;
    default: return sample * 2;
    }
}

void decodeAdpcm(Voice *v, const u8 *p, int bits)
{
    int scale = 1 << (p[0] >> 4);
    int idx = p[0] & 15;
    s32 c1 = sAdpcmTable ? be16(sAdpcmTable + idx * 4) : 0;
    s32 c2 = sAdpcmTable ? be16(sAdpcmTable + idx * 4 + 2) : 0;
    for (int i = 0; i < 16; i++) {
        s32 d;
        if (bits == 4) {
            d = (p[1 + i / 2] >> ((i & 1) ? 0 : 4)) & 15;
            if (d >= 8)
                d -= 16;
            d = (d * scale) << 11;
        } else {
            d = (p[1 + i / 4] >> (6 - 2 * (i & 3))) & 3;
            if (d >= 2)
                d -= 4;
            d = (d * scale) << 13;
        }
        s32 s = clamp16((d + c1 * v->hist1 + c2 * v->hist2) >> 11);
        v->hist2 = v->hist1;
        v->hist1 = s;
        v->blk[i] = (s16)s;
    }
}

// Fetch the next block at _70. _74 counts the samples left before the end
// (or loop end); at zero the voice loops to _110 or ends.
bool fetch(JASDsp::TChannel *ch, Voice *v)
{
    int spb = samplesPerBlock(ch);
    int skip = 0;
    if (ch->_74 == 0) {
        if (!ch->_102)
            return false;
        u32 loop = ch->_110;
        u32 start = loop - loop % spb;
        if (start >= ch->_114)
            return false;
        ch->_70 = ch->_118 + byteOffset(ch, start);
        ch->_74 = ch->_114 - start;
        skip = loop - start;
        v->hist1 = ch->_104;
        v->hist2 = ch->_106;
    }
    const u8 *p;
    switch (ch->_100) {
    case 9:
        if (!(p = aram(ch->_70, 9)))
            return false;
        decodeAdpcm(v, p, 4);
        ch->_70 += 9;
        break;
    case 5:
        if (!(p = aram(ch->_70, 5)))
            return false;
        decodeAdpcm(v, p, 2);
        ch->_70 += 5;
        break;
    case 8:
        if (!(p = aram(ch->_70, 1)))
            return false;
        v->blk[0] = (s16)((s8)p[0] << 8);
        ch->_70 += 1;
        break;
    default:
        if (!(p = aram(ch->_70, 2)))
            return false;
        v->blk[0] = be16(p);
        ch->_70 += 2;
        break;
    }
    u32 n = ch->_74 < (u32)spb ? ch->_74 : (u32)spb;
    ch->_74 -= n;
    v->blkLen = (int)n;
    v->blkPos = skip < (int)n ? skip : (int)n;
    return true;
}

s16 nextSample(JASDsp::TChannel *ch, Voice *v)
{
    while (v->blkPos >= v->blkLen) {
        if (v->ended || !fetch(ch, v)) {
            v->ended = true;
            return 0;
        }
    }
    return v->blk[v->blkPos++];
}

// Basic oscillator voices (setOscInfo): one cycle of a table waveform per 64
// steps. The DSP resampling table also holds a sine and a triangle cycle.
s16 oscSample(JASDsp::TChannel *ch, Voice *v)
{
    u32 i = (v->oscPhase >> 16) & 63;
    s16 s;
    if (!sResTable) {
        s = 0;
    } else {
        const u32 *wave = sResTable + (ch->_100 == 1 ? 160 : 128); // triangle : sine
        u32 w = wave[i / 2];
        s = (s16)((i & 1) ? (w & 0xFFFF) : (w >> 16));
        if (ch->_100 == 2)
            s = s >= 0 ? 0x7FFF : -0x7FFF; // square
    }
    v->oscPhase += (u32)(u16)ch->mPitch << 4;
    return s;
}

void startVoice(JASDsp::TChannel *ch, Voice *v)
{
    __builtin_memset(v, 0, sizeof *v);
    if (ch->_100 >= 4) {
        u32 skip = 0;
        if (ch->_100 == 8 || ch->_100 == 16)
            skip = ch->_68 < ch->_114 ? ch->_68 : 0; // ADPCM skips were folded into _118/_114 by setWaveInfo
        ch->_70 = ch->_118 + byteOffset(ch, skip);
        ch->_74 = ch->_114 - skip;
    }
}

void renderVoice(int idx, JASDsp::TChannel *ch)
{
    Voice *v = &sVoices[idx];
    s32 buf[SUBFRAME];

    if (ch->_08) {
        if (sDebug)
            fprintf(stderr, "[dsp] ch%d start: block bytes %u, aram %x, end %u, loop %u@%u, pitch %x, %s\n", idx, ch->_100,
                    ch->_118, ch->_114, ch->_102, ch->_110, (u16)ch->mPitch, ch->_58 ? "auto mixer" : "mixers");
        startVoice(ch, v);
        ch->_08 = 0;
    }

    bool isWave = ch->_100 >= 4;
    u32 step = (u32)(u16)ch->mPitch << 4; // Q12 -> Q16
    for (int n = 0; n < SUBFRAME; n++) {
        u32 phase = v->frac >> 10;
        s32 t0 = 0x7FFF, t1 = 0, t2 = 0, t3 = 0;
        if (sResTable) {
            u32 a = sResTable[phase * 2], b = sResTable[phase * 2 + 1];
            t0 = (s16)(a >> 16), t1 = (s16)a, t2 = (s16)(b >> 16), t3 = (s16)b;
        }
        buf[n] = (t0 * v->win[0] + t1 * v->win[1] + t2 * v->win[2] + t3 * v->win[3]) >> 15;
        v->frac += step;
        while (v->frac >= 0x10000) {
            v->frac -= 0x10000;
            v->win[0] = v->win[1];
            v->win[1] = v->win[2];
            v->win[2] = v->win[3];
            v->win[3] = isWave ? nextSample(ch, v) : oscSample(ch, v);
        }
    }

    // filters (JASTrack::setFIR/setIIR): low 5 bits = FIR taps, 0x20 = IIR
    int taps = ch->mFilterMode & 0x1F;
    if (taps) {
        if (taps > 8)
            taps = 8;
        for (int n = 0; n < SUBFRAME; n++) {
            for (int k = 7; k > 0; k--)
                v->fir[k] = v->fir[k - 1];
            v->fir[0] = buf[n];
            s32 acc = 0;
            for (int k = 0; k < taps; k++)
                acc += ch->mFir8FilterParam[k] * v->fir[k];
            buf[n] = clamp16(acc >> 15);
        }
    }
    if (ch->mFilterMode & 0x20) {
        s32 b = ch->mIirFilterParam[0];
        if (b < 0)
            b = 0;
        for (int n = 0; n < SUBFRAME; n++) {
            v->iir = (b * buf[n] + (0x7FFF - b) * v->iir) >> 15;
            buf[n] = v->iir;
        }
    }

    bool stopping = ch->mForcedStop != 0;
    if (ch->_58) {
        // auto mixer (Dolby mode): volume _56 ramped from _54, pan/dolby in _50, fx in _52
        s32 v0 = ch->_54, v1 = stopping ? 0 : ch->_56;
        f32 pan = (ch->_50 >> 8) / 127.0f, dolby = (ch->_50 & 0xFF) / 127.0f, fx = (ch->_52 >> 8) / 127.0f;
        if (pan > 1.0f) pan = 1.0f;
        if (dolby > 1.0f) dolby = 1.0f;
        if (fx > 1.0f) fx = 1.0f;
        const f32 hpi = 1.5707963f;
        f32 front = __builtin_cosf(dolby * hpi), rear = __builtin_sinf(dolby * hpi) * 0.70710678f;
        f32 gl = __builtin_sinf((1.0f - pan) * hpi), gr = __builtin_sinf(pan * hpi);
        s32 kl = (s32)(gl * front * 32768.0f), kr = (s32)(gr * front * 32768.0f), ks = (s32)(rear * 32768.0f);
        s32 kfl = (s32)(gl * fx * 32768.0f), kfr = (s32)(gr * fx * 32768.0f);
        for (int n = 0; n < SUBFRAME; n++) {
            s32 vol = v0 + (v1 - v0) * (n + 1) / SUBFRAME;
            s32 s = (buf[n] * vol) >> 15;
            sBus[BUS_L][n] += (s * kl) >> 15;
            sBus[BUS_R][n] += (s * kr) >> 15;
            sBus[BUS_S][n] += (s * ks) >> 15;
            sBus[BUS_FX0][n] += (s * kfl) >> 15;
            sBus[BUS_FX1][n] += (s * kfr) >> 15;
        }
        ch->_54 = (u16)v1;
    } else {
        JASDsp::TChannel::Mixer *mix = ch->_10; // 6 slots: _10[4] runs on into _30
        for (int i = 0; i < 6; i++) {
            int bus = busIndex(mix[i].busConnect);
            s32 v0 = mix[i]._04, v1 = stopping ? 0 : mix[i]._02;
            if (bus >= 0 && (v0 || v1)) {
                for (int n = 0; n < SUBFRAME; n++) {
                    s32 vol = v0 + (v1 - v0) * (n + 1) / SUBFRAME;
                    sBus[bus][n] += (buf[n] * vol) >> 15;
                }
            }
            mix[i]._04 = (s16)v1;
        }
    }

    if (stopping || (isWave && v->ended))
        ch->mIsFinished = 1;
}

void runFxLines()
{
    if (!sFx)
        return;
    for (int i = 0; i < 4; i++) {
        JASDsp::Fxline *fx = &sFx[i];
        FxState *st = &sFxState[i];
        if (!fx->_00 || !fx->_04 || !fx->_02)
            continue;
        s16 *line = fx->_04 + (st->block % fx->_02) * SUBFRAME;
        int out1 = busIndex(fx->_08), out2 = busIndex(fx->_0C);
        s32 g1 = (s16)fx->_0A, g2 = (s16)fx->_0E;
        for (int n = 0; n < SUBFRAME; n++) {
            for (int k = 7; k > 0; k--)
                st->hist[k] = st->hist[k - 1];
            st->hist[0] = line[n];
            s32 acc = 0;
            for (int k = 0; k < 8; k++)
                acc += fx->mFilterTable[k] * st->hist[k];
            s32 y = acc >> 15;
            if (out1 >= 0)
                sBus[out1][n] += (y * g1) >> 15;
            if (out2 >= 0)
                sBus[out2][n] += (y * g2) >> 15;
            line[n] = clamp16(sBus[BUS_FX0 + i][n] + y);
        }
        st->block++;
    }
}

void postMail(u32 mail);

void renderSubframe()
{
    __builtin_memset(sBus, 0, sizeof sBus);
    for (int i = 0; i < sNumChannels; i++) {
        JASDsp::TChannel *ch = &sChannels[i];
        if (!ch->mIsActive || ch->mIsFinished || ch->mPauseFlag)
            continue;
        renderVoice(i, ch);
    }
    runFxLines();

    s16 *l = sOutL + sSubframe * SUBFRAME, *r = sOutR + sSubframe * SUBFRAME;
    for (int n = 0; n < SUBFRAME; n++) {
        s32 c = sBus[BUS_C][n] + sBus[BUS_0F40][n] + sBus[BUS_0CA0][n] + sBus[BUS_0B00][n];
        s32 s = sBus[BUS_S][n];
        s32 cl = (c * 0x5A82) >> 15, sl = (s * 0x5A82) >> 15;
        s32 L = sBus[BUS_L][n] + cl + sl;
        s32 R = sBus[BUS_R][n] + cl - sl;
        l[n] = clamp16((L * sMixerLevel) >> 14);
        r[n] = clamp16((R * sMixerLevel) >> 14);
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
