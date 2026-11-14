// Netcode spike: drive one kart from a (simulated) network stream of kart state.
// See NETCODE.md. Called from KartCtrl::DynamicsStage1/2 (patch 0020-netcode-remote-kart).
//
//   MKDD_NET=rec    MKDD_NET_FILE=f   record every kart's wire state each frame
//   MKDD_NET=play   MKDD_NET_FILE=f   kart MKDD_NET_KART is remote, fed from kart MKDD_NET_KART's recording
//   MKDD_NET=mirror                   kart MKDD_NET_KART is remote, fed from live kart MKDD_NET_SRC
//                                     delayed by MKDD_NET_LAG frames (a trailing "copy")
//   MKDD_NET_MODE  0 interpolate in the past and snap (puppet)
//                  1 local physics on the remote's inputs + extrapolated, blended correction (default)
//                  2 remote inputs only, no correction (shows divergence)
//   MKDD_NET_LAT / _JITTER (frames), _LOSS (percent), _RATE (send every N frames): simulated link
//
// Remote state only ever passes through the 56-byte wire format below, so quantisation is part of the test.
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "Kaneshige/RaceMgr.h"
#include "Yamamoto/kartCtrl.h"

extern "C" int port_env(const char *name);
extern "C" char *gpu_js_env_str(const char *name);
extern "C" void free(void *);

// The decomp's MSL headers shadow the host libc here, so math is builtins and file I/O goes through node's fs (netkart_io.c).
#define sqrtf __builtin_sqrtf
#define fabsf __builtin_fabsf
#define lrintf __builtin_lrintf

extern "C" int net_js_open(const char *path, int write);
extern "C" void net_js_write(const void *p, int n);
extern "C" int net_js_read(void *p, int n);
extern "C" void net_js_close(void);

namespace {

enum { MAX_KART = 8, HIST = 1024, WIRE = 56 };

struct State {
    u32 frame;
    f32 pos[3];
    f32 rot[3][3]; // KartBody::_110 rotation part
    f32 vel[3], effVel[3], wg[3];
    s8 sx, sy;     // stick, -127..127
    u16 buttons;   // JUTGamePad bits
    u8 aL, aR;
    u64 carStatus;
    u16 boost, mtBoost;
    u8 mtState, driver;
    bool valid;
};

// ---- wire format (little endian on every host this runs on) ----
void put16(u8 *p, u16 v) { memcpy(p, &v, 2); }
void put32(u8 *p, u32 v) { memcpy(p, &v, 4); }
u16 get16(const u8 *p) { u16 v; memcpy(&v, p, 2); return v; }
u32 get32(const u8 *p) { u32 v; memcpy(&v, p, 4); return v; }
s16 qs16(f32 v, f32 scale)
{
    f32 q = v * scale;
    if (q > 32767.0f) q = 32767.0f;
    if (q < -32767.0f) q = -32767.0f;
    return (s16)lrintf(q);
}

void mtxToQuat(const f32 m[3][3], f32 q[4]) // q = x y z w
{
    f32 tr = m[0][0] + m[1][1] + m[2][2];
    if (tr > 0.0f) {
        f32 s = sqrtf(tr + 1.0f) * 2.0f;
        q[3] = 0.25f * s;
        q[0] = (m[2][1] - m[1][2]) / s;
        q[1] = (m[0][2] - m[2][0]) / s;
        q[2] = (m[1][0] - m[0][1]) / s;
    } else if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
        f32 s = sqrtf(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
        q[3] = (m[2][1] - m[1][2]) / s;
        q[0] = 0.25f * s;
        q[1] = (m[0][1] + m[1][0]) / s;
        q[2] = (m[0][2] + m[2][0]) / s;
    } else if (m[1][1] > m[2][2]) {
        f32 s = sqrtf(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
        q[3] = (m[0][2] - m[2][0]) / s;
        q[0] = (m[0][1] + m[1][0]) / s;
        q[1] = 0.25f * s;
        q[2] = (m[1][2] + m[2][1]) / s;
    } else {
        f32 s = sqrtf(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
        q[3] = (m[1][0] - m[0][1]) / s;
        q[0] = (m[0][2] + m[2][0]) / s;
        q[1] = (m[1][2] + m[2][1]) / s;
        q[2] = 0.25f * s;
    }
}

void quatToMtx(const f32 q[4], f32 m[3][3])
{
    f32 x = q[0], y = q[1], z = q[2], w = q[3];
    m[0][0] = 1 - 2 * (y * y + z * z); m[0][1] = 2 * (x * y - z * w);     m[0][2] = 2 * (x * z + y * w);
    m[1][0] = 2 * (x * y + z * w);     m[1][1] = 1 - 2 * (x * x + z * z); m[1][2] = 2 * (y * z - x * w);
    m[2][0] = 2 * (x * z - y * w);     m[2][1] = 2 * (y * z + x * w);     m[2][2] = 1 - 2 * (x * x + y * y);
}

void quatNorm(f32 q[4])
{
    f32 l = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    for (int i = 0; i < 4; i++)
        q[i] /= l;
}

// smallest-three: 2-bit index of the dropped (largest) component, 3 x 10 bits
u32 quatPack(const f32 qin[4])
{
    f32 q[4] = {qin[0], qin[1], qin[2], qin[3]};
    quatNorm(q);
    int big = 0;
    for (int i = 1; i < 4; i++)
        if (fabsf(q[i]) > fabsf(q[big]))
            big = i;
    if (q[big] < 0)
        for (int i = 0; i < 4; i++)
            q[i] = -q[i];
    u32 out = (u32)big << 30;
    int shift = 20;
    for (int i = 0; i < 4; i++) {
        if (i == big)
            continue;
        f32 v = (q[i] / 0.70710678f + 1.0f) * 0.5f; // [-1/sqrt2, 1/sqrt2] -> 0..1
        int iv = (int)lrintf(v * 1023.0f);
        if (iv < 0) iv = 0;
        if (iv > 1023) iv = 1023;
        out |= (u32)iv << shift;
        shift -= 10;
    }
    return out;
}

void quatUnpack(u32 in, f32 q[4])
{
    int big = in >> 30, shift = 20;
    f32 sum = 0;
    for (int i = 0; i < 4; i++) {
        if (i == big)
            continue;
        f32 v = (f32)((in >> shift) & 1023) / 1023.0f;
        q[i] = (v * 2.0f - 1.0f) * 0.70710678f;
        sum += q[i] * q[i];
        shift -= 10;
    }
    q[big] = sqrtf(sum < 1.0f ? 1.0f - sum : 0.0f);
}

void encode(const State &s, int kart, u8 *p)
{
    f32 q[4];
    memset(p, 0, WIRE);
    p[0] = 1; // KART_STATE
    p[1] = (u8)kart;
    put16(p + 2, (u16)s.frame);
    for (int i = 0; i < 3; i++)
        memcpy(p + 4 + i * 4, &s.pos[i], 4);
    mtxToQuat(s.rot, q);
    put32(p + 16, quatPack(q));
    for (int i = 0; i < 3; i++) {
        put16(p + 20 + i * 2, (u16)qs16(s.vel[i], 64.0f));
        put16(p + 26 + i * 2, (u16)qs16(s.effVel[i], 64.0f));
        put16(p + 32 + i * 2, (u16)qs16(s.wg[i], 4096.0f));
    }
    p[38] = (u8)s.sx;
    p[39] = (u8)s.sy;
    put16(p + 40, s.buttons);
    p[42] = s.aL;
    p[43] = s.aR;
    put32(p + 44, (u32)s.carStatus);
    put16(p + 48, (u16)(s.carStatus >> 32));
    put16(p + 50, s.boost);
    put16(p + 52, s.mtBoost);
    p[54] = s.mtState;
    p[55] = s.driver;
}

void decode(const u8 *p, u32 nowFrame, State &s)
{
    f32 q[4];
    u16 f16 = get16(p + 2);
    // full frame = the latest frame <= now with these low 16 bits (clock is shared in the spike)
    s.frame = (nowFrame & ~0xffffu) | f16;
    if (s.frame > nowFrame)
        s.frame -= 0x10000;
    for (int i = 0; i < 3; i++)
        memcpy(&s.pos[i], p + 4 + i * 4, 4);
    quatUnpack(get32(p + 16), q);
    quatToMtx(q, s.rot);
    for (int i = 0; i < 3; i++) {
        s.vel[i] = (s16)get16(p + 20 + i * 2) / 64.0f;
        s.effVel[i] = (s16)get16(p + 26 + i * 2) / 64.0f;
        s.wg[i] = (s16)get16(p + 32 + i * 2) / 4096.0f;
    }
    s.sx = (s8)p[38];
    s.sy = (s8)p[39];
    s.buttons = get16(p + 40);
    s.aL = p[42];
    s.aR = p[43];
    s.carStatus = get32(p + 44) | ((u64)get16(p + 48) << 32);
    s.boost = get16(p + 50);
    s.mtBoost = get16(p + 52);
    s.mtState = p[54];
    s.driver = p[55];
    s.valid = true;
}

// ---- capture / apply on KartBody ----
s8 stick8(f32 v)
{
    int i = (int)lrintf(v * 127.0f);
    return (s8)(i > 127 ? 127 : i < -127 ? -127 : i);
}

void capture(int k, u32 frame, State &s)
{
    KartCtrl *ctrl = KartCtrl::getKartCtrl();
    KartBody *b = ctrl->getKartBody(k);
    KartGamePad *pad = ctrl->GetDriveCont(k);
    s.frame = frame;
    s.pos[0] = b->mPos.x; s.pos[1] = b->mPos.y; s.pos[2] = b->mPos.z;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            s.rot[i][j] = b->_110[i][j];
    s.vel[0] = b->mVel.x; s.vel[1] = b->mVel.y; s.vel[2] = b->mVel.z;
    s.effVel[0] = b->mEffctVel.x; s.effVel[1] = b->mEffctVel.y; s.effVel[2] = b->mEffctVel.z;
    s.wg[0] = b->mWg.x; s.wg[1] = b->mWg.y; s.wg[2] = b->mWg.z;
    s.sx = stick8(pad->mMainStick.mX);
    s.sy = stick8(pad->mMainStick.mY);
    s.buttons = (u16)pad->mButtons.mButton;
    s.aL = pad->mButtons.mAnalogL;
    s.aR = pad->mButtons.mAnalogR;
    s.carStatus = b->mCarStatus;
    s.boost = b->mBoostTimer;
    s.mtBoost = b->mMTBoost;
    s.mtState = b->mMTState;
    s.driver = b->mDriver;
    s.valid = true;
}

// Same fields RivalBodyCtrl::doRotPose / KartBody::DoPose keep consistent.
void setPose(KartBody *b, const f32 pos[3], const f32 rot[3][3])
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            b->_110[i][j] = rot[i][j];
    b->_110[0][3] = pos[0];
    b->_110[1][3] = pos[1];
    b->_110[2][3] = pos[2];
    b->mPos.set(pos[0], pos[1], pos[2]);
    PSMTXCopy(b->_110, b->mPlayerPosMtx);
    f32 off = KartCtrl::getKartCtrl()->GetKartBodyOffset(b->mMynum);
    b->mPlayerPosMtx[0][3] += b->mPlayerPosMtx[0][1] * off;
    b->mPlayerPosMtx[1][3] += b->mPlayerPosMtx[1][1] * off;
    b->mPlayerPosMtx[2][3] += b->mPlayerPosMtx[2][1] * off;
    b->_2f0.set(b->_110[0][0], b->_110[1][0], b->_110[2][0]);
    b->_2fc.set(b->_110[0][1], b->_110[1][1], b->_110[2][1]);
    b->_308.set(b->_110[0][2], b->_110[1][2], b->_110[2][2]);
}

void nlerpRot(const f32 a[3][3], const f32 b[3][3], f32 t, f32 out[3][3])
{
    f32 qa[4], qb[4], q[4];
    mtxToQuat(a, qa);
    mtxToQuat(b, qb);
    f32 d = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3];
    f32 sgn = d < 0 ? -1.0f : 1.0f;
    for (int i = 0; i < 4; i++)
        q[i] = qa[i] + (qb[i] * sgn - qa[i]) * t;
    quatNorm(q);
    quatToMtx(q, out);
}

// ---- configuration and simulated link ----
struct Packet {
    u32 arrive;
    u8 data[WIRE];
};

int sInit, sMode, sRemote, sSrc, sLag, sLat, sJit, sLoss, sRate, sRec;
bool sMirror, sPlay;
bool sFile;
enum { TAPE = 16384 };
State sHist[HIST];             // mirror: live source history by frame % HIST
State sTape[TAPE];             // play: recorded frames of the remote kart
u32 sTapeLen;
Packet sQueue[256];
int sQueueLen;
State sRecv[64];               // receive buffer, newest last
int sRecvLen;
bool sActive;
u32 sRng = 0x2545F491;         // own LCG: never touch the game's stRandom streams
double sErrSum, sErrMax;
int sErrN, sSent, sDropped, sSnaps;
u16 sPrevButtons;
u32 sLastNow;

// port_env() maps "0" to 1 (it is a flag reader); numeric settings need the real value
int envInt(const char *name, int def)
{
    char *v = gpu_js_env_str(name);
    if (!v)
        return def;
    int sign = 1, n = 0;
    const char *p = v;
    if (*p == '-') {
        sign = -1;
        p++;
    }
    while (*p >= '0' && *p <= '9')
        n = n * 10 + (*p++ - '0');
    free(v);
    return sign * n;
}

u32 rnd()
{
    sRng = sRng * 1664525u + 1013904223u;
    return sRng >> 8;
}

void init()
{
    sInit = 1;
    char *m = gpu_js_env_str("MKDD_NET");
    if (!m)
        return;
    sRec = !strcmp(m, "rec");
    sMirror = !strcmp(m, "mirror");
    sPlay = !strcmp(m, "play");
    free(m);
    sMode = envInt("MKDD_NET_MODE", 1);
    sRemote = envInt("MKDD_NET_KART", 0);
    sSrc = envInt("MKDD_NET_SRC", sRemote == 0 ? 1 : 0);
    sLag = envInt("MKDD_NET_LAG", 90);
    sLat = envInt("MKDD_NET_LAT", 6);
    sJit = envInt("MKDD_NET_JITTER", 2);
    sLoss = envInt("MKDD_NET_LOSS", 0);
    sRate = envInt("MKDD_NET_RATE", 2);
    if (sRate <= 0)
        sRate = 2;
    char *path = gpu_js_env_str("MKDD_NET_FILE");
    if (sRec || sPlay) {
        if (!path) {
            fprintf(stderr, "[net] MKDD_NET_FILE missing\n");
            sRec = 0;
            sPlay = false;
            return;
        }
        sFile = net_js_open(path, sRec);
        if (!sFile) {
            fprintf(stderr, "[net] cannot open %s\n", path);
            sRec = 0;
            sPlay = false;
        }
    }
    if (sPlay && sFile) {
        // records: u32 frame, u8 wire[WIRE]; keep the remote kart's
        u8 rec[4 + WIRE];
        while (net_js_read(rec, sizeof rec) == sizeof rec) {
            u32 f = get32(rec);
            if (rec[4 + 1] != sRemote || f >= TAPE)
                continue;
            decode(rec + 4, f, sTape[f]);
            sTape[f].frame = f;
            if (f + 1 > sTapeLen)
                sTapeLen = f + 1;
        }
        net_js_close();
        sFile = false;
    }
    free(path);
    fprintf(stderr, "[net] %s remote=%d src=%d mode=%d lag=%d lat=%d jitter=%d loss=%d%% rate=%d tape=%u\n",
            sRec ? "rec" : sMirror ? "mirror" : sPlay ? "play" : "off", sRemote, sSrc, sMode, sLag, sLat, sJit, sLoss,
            sRate, sTapeLen);
}

// Ground truth of what the remote peer's kart is doing at remote frame f (unquantised).
const State *truth(u32 f)
{
    if (sMirror) {
        if (f < (u32)sLag)
            return nullptr;
        const State &s = sHist[(f - sLag) % HIST];
        return (s.valid && s.frame == f - sLag) ? &s : nullptr;
    }
    if (sPlay)
        return (f < sTapeLen && sTape[f].valid) ? &sTape[f] : nullptr;
    return nullptr;
}

f32 speedOf(const State &s)
{
    return sqrtf(s.effVel[0] * s.effVel[0] + s.effVel[1] * s.effVel[1] + s.effVel[2] * s.effVel[2]);
}

void netTick(u32 now)
{
    // sender: the "remote peer" emits its state every sRate frames
    const State *t = truth(now);
    if (t && now % sRate == 0) {
        sSent++;
        if ((int)(rnd() % 100) < sLoss) {
            sDropped++;
        } else if (sQueueLen < 256) {
            State s = *t;
            s.frame = now; // stamp with the remote's clock
            Packet &p = sQueue[sQueueLen++];
            p.arrive = now + sLat + (sJit ? rnd() % (sJit + 1) : 0);
            encode(s, sRemote, p.data);
        }
    }
    // receiver: deliver arrived packets (possibly out of order), keep buffer sorted by frame
    for (int i = 0; i < sQueueLen;) {
        if (sQueue[i].arrive > now) {
            i++;
            continue;
        }
        State s;
        decode(sQueue[i].data, now, s);
        sQueue[i] = sQueue[--sQueueLen];
        if (sRecvLen && s.frame <= sRecv[sRecvLen - 1].frame) {
            bool dup = false;
            for (int j = 0; j < sRecvLen; j++)
                dup |= sRecv[j].frame == s.frame;
            if (dup || s.frame < sRecv[0].frame)
                continue;
        }
        if (sRecvLen == 64) {
            memmove(sRecv, sRecv + 1, sizeof(State) * 63);
            sRecvLen--;
        }
        int at = sRecvLen;
        while (at > 0 && sRecv[at - 1].frame > s.frame)
            at--;
        memmove(sRecv + at + 1, sRecv + at, sizeof(State) * (sRecvLen - at));
        sRecv[at] = s;
        sRecvLen++;
    }
}

void makeRemote(KartBody *b)
{
    b->mGameStatus &= ~8u;     // no CPU driver
    b->mGameStatus |= 0x802;   // what KartBody::Init gives a PORT_NET pad (player kind 3)
    b->mGameStatus &= ~0x100u; // not a local human
}

void feedPad(int k, const State &s)
{
    KartGamePad *pad = KartCtrl::getKartCtrl()->GetDriveCont(k);
    // item use arrives as events (NETCODE.md 5.3), never from the predicted pad
    u16 itemMask = (u16)KartCtrl::getKartCtrl()->getKartPad(k)->mItemMask;
    u16 buttons = s.buttons & ~itemMask, prev = sPrevButtons & ~itemMask;
    pad->mMainStick.mX = s.sx / 127.0f;
    pad->mMainStick.mY = s.sy / 127.0f;
    pad->mButtons.mButton = buttons;
    pad->mButtons.mTrigger = buttons & ~prev;
    pad->mButtons.mRelease = prev & ~buttons;
    pad->mButtons.mAnalogL = s.aL;
    pad->mButtons.mAnalogR = s.aR;
}

void snapTo(KartBody *b, const State &s)
{
    setPose(b, s.pos, s.rot);
    b->mPrevPos.set(s.pos[0], s.pos[1], s.pos[2]);
    b->mVel.set(s.vel[0], s.vel[1], s.vel[2]);
    b->mEffctVel.set(s.effVel[0], s.effVel[1], s.effVel[2]);
    b->mWg.set(s.wg[0], s.wg[1], s.wg[2]);
}

void correct(u32 now)
{
    KartCtrl *ctrl = KartCtrl::getKartCtrl();
    KartBody *b = ctrl->getKartBody(sRemote);
    if (!sRecvLen)
        return;
    const State &last = sRecv[sRecvLen - 1];
    if (!sActive) {
        // mirror: wait until the source has left the grid so the copy does not land on it
        if (sMirror && speedOf(last) < 1.0f)
            return;
        sActive = true;
        makeRemote(b);
        snapTo(b, last);
        sSnaps++;
        fprintf(stderr, "[net] frame %u: kart %d is remote (state of frame %u)\n", now, sRemote, last.frame);
    }
    makeRemote(b);

    if (sMode == 0) {
        // interpolation buffer: render the remote sRate frames behind the newest data
        u32 T = now - sLat - sJit - sRate;
        int i = sRecvLen - 1;
        while (i > 0 && sRecv[i - 1].frame > T)
            i--;
        State s;
        if (i > 0 && sRecv[i - 1].frame <= T && sRecv[i].frame >= T) {
            const State &a = sRecv[i - 1], &c = sRecv[i];
            f32 t = c.frame == a.frame ? 0.0f : (f32)(T - a.frame) / (f32)(c.frame - a.frame);
            s = c;
            for (int j = 0; j < 3; j++) {
                s.pos[j] = a.pos[j] + (c.pos[j] - a.pos[j]) * t;
                s.vel[j] = a.vel[j] + (c.vel[j] - a.vel[j]) * t;
                s.effVel[j] = a.effVel[j] + (c.effVel[j] - a.effVel[j]) * t;
            }
            nlerpRot(a.rot, c.rot, t, s.rot);
        } else {
            // starved: extrapolate the newest sample, at most 10 frames
            s = last;
            int dt = (int)(T - last.frame);
            if (dt > 10)
                dt = 10;
            for (int j = 0; j < 3; j++)
                s.pos[j] += s.effVel[j] * dt;
        }
        snapTo(b, s);
    } else if (sMode == 1) {
        // the kart already ran this frame's physics on the remote's (predicted) inputs; pull it toward
        // the newest state extrapolated to now
        int dt = (int)(now - last.frame);
        if (dt > 15)
            dt = 15;
        f32 target[3], err[3], e2 = 0;
        for (int j = 0; j < 3; j++) {
            target[j] = last.pos[j] + last.effVel[j] * dt;
            err[j] = target[j] - (&b->mPos.x)[j];
            e2 += err[j] * err[j];
        }
        if (e2 > 1500.0f * 1500.0f) {
            State s = last;
            memcpy(s.pos, target, sizeof target);
            snapTo(b, s);
            sSnaps++;
        } else {
            const f32 k = 0.15f; // ~ 6-frame time constant
            f32 pos[3], rot[3][3], cur[3][3];
            for (int j = 0; j < 3; j++)
                pos[j] = (&b->mPos.x)[j] + err[j] * k;
            for (int i = 0; i < 3; i++)
                for (int j = 0; j < 3; j++)
                    cur[i][j] = b->_110[i][j];
            nlerpRot(cur, last.rot, k, rot);
            setPose(b, pos, rot);
            b->mVel.x += (last.vel[0] - b->mVel.x) * k;
            b->mVel.y += (last.vel[1] - b->mVel.y) * k;
            b->mVel.z += (last.vel[2] - b->mVel.z) * k;
        }
    }

    const State *t = truth(now);
    if (t) {
        f32 e2 = 0;
        for (int j = 0; j < 3; j++) {
            f32 d = t->pos[j] - (&b->mPos.x)[j];
            e2 += d * d;
        }
        f32 e = sqrtf(e2);
        sErrSum += e;
        sErrN++;
        if (e > sErrMax)
            sErrMax = e;
    }
    if (now % 120 == 0 && sErrN) {
        fprintf(stderr, "[net] frame %u: err avg %.1f max %.1f (truth speed %.1f/frame) sent %d dropped %d snaps %d buf %d\n",
                now, sErrSum / sErrN, sErrMax, t ? speedOf(*t) : 0.0f, sSent, sDropped, sSnaps, sRecvLen);
        sErrSum = sErrMax = 0;
        sErrN = 0;
    }
}

} // namespace

extern "C" void port_net_pre_dynamics(void)
{
    if (!sInit)
        init();
    if (!sActive || sMode == 0 || !sRecvLen)
        return;
    // input prediction: hold the newest received input (PadMgr already overwrote a human pad this frame)
    feedPad(sRemote, sRecv[sRecvLen - 1]);
}

extern "C" void port_net_post_stage1(void)
{
    if (!sInit)
        init();
    if (!sRec && !sMirror && !sPlay)
        return;
    RaceMgr *rcm = RaceMgr::getManager();
    KartCtrl *ctrl = KartCtrl::getKartCtrl();
    if (!rcm || !ctrl)
        return;
    u32 now = rcm->getRaceFrame();
    int n = ctrl->GetKartNumber();
    if (now < sLastNow) {
        // RaceMgr restarted its frame counter (course demo -> race, retry): drop the session state
        sQueueLen = sRecvLen = 0;
        sActive = false;
        sErrSum = sErrMax = 0;
        sErrN = 0;
    }
    sLastNow = now;

    if (sRec && sFile) {
        for (int k = 0; k < n; k++) {
            State s;
            u8 rec[4 + WIRE];
            capture(k, now, s);
            put32(rec, now);
            encode(s, k, rec + 4);
            net_js_write(rec, sizeof rec);
        }
        return;
    }
    if (sRemote >= n || (sMirror && sSrc >= n))
        return;
    if (sMirror)
        capture(sSrc, now, sHist[now % HIST]);
    netTick(now);
    correct(now);
    if (sActive && sMode != 0 && sRecvLen) {
        // keep the input on the pad for DoAccel (stage 2) and remember it for trigger edges
        feedPad(sRemote, sRecv[sRecvLen - 1]);
        sPrevButtons = sRecv[sRecvLen - 1].buttons;
    }
}
