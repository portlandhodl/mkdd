// Online play, phase 1 of NETCODE.md: every browser drives its own kart and sees the others' karts.
//
// Session (no menus): the page boots the game with MKDD_NET_ONLINE=1 when the lobby session starts.
// LogoApp skips the logos and calls port_net_boot_race(), which fills gRaceInfo from the session
// (port_net_session_json: course/cc/laps/mirror from the host, drivers/kart per member, lobby slots) as
// LANEntry::setRaceInfo does, loads the course and enters RaceApp: a VS race, one kart per member with a
// lobby slot, no CPUs, items off (item boxes need host arbitration, a later phase). The local kart is the
// human on pad port 1 with the full-screen view; the others get KartGamePad(PORT_NET, NETWORK) pads
// (player kind 3, NET).
//
// Start: the Lakitu countdown waits at the gate before the lights (port_net_countdown_hold, patched into
// TJugem); each peer reports READY{attempt} to the host, the host broadcasts GO{attempt, T_go} on its
// clock (port_net_host_time) and every peer starts its 240-frame countdown at T_go - 4004 ms, so the
// GO frames line up (port_net_go_hold keeps a fast peer from going early).
//
// Karts: the local kart's 56-byte state (netkart.h) goes to everyone at 30 Hz on the unreliable channel.
// Its frame field is the shared clock frame, round(host time / 16.683 ms) (low 16 bits), so a receiver
// knows each sample's age on the same clock. Remote karts use the hybrid model (spike mode 1): the game's
// own physics on the newest input, then pose and velocity are pulled toward the newest state extrapolated
// to now (time constant ~6 frames at 60 Hz, scaled by the local frame time), snapping above 1500 units.
// Kart-kart bumps use the game's physics on both sides; the remote kart is pulled back to its owner's truth.
// A remote kart with no state for 0.5 s is frozen; a lost peer's kart is frozen and stops colliding, and
// a peer that left is also hidden.
//
// Measured: on every received state (truth of the sender's kart at shared frame f), the replica's
// position at the same shared frame (local history, interpolated) gives the true replica error. Once a
// second the page log gets "[net] stats" lines and Module.netStats / Module.netStatsLog are updated
// (netonline_js.c) for the overlay and tests.
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "Kaneshige/RaceMgr.h"
#include "Kaneshige/RaceInfo.h"
#include "Kaneshige/KartDrawer.h"
#include "Yamamoto/kartCtrl.h"
#include "Osako/kartPad.h"
#include "Osako/ResMgr.h"
#include "Osako/AppMgr.h"
#include "Osako/RaceApp.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "net_bridge.h"
#include "netkart.h"
#include "jsonlite.h"

extern "C" void netonline_js_stats(const char *json);
extern "C" void port_netsync_message(int from, int kind, const u8 *m, int n);
extern "C" void netonline_js_phase(const char *phase);
extern "C" u32 port_vi_retrace_count(void);

using namespace netkart;
using namespace jsonlite;

namespace {

const double FRAME_MS = 1001.0 / 60.0;       // NTSC field
const double COUNTDOWN_MS = 240 * FRAME_MS;  // TJugemRodSignal: countdown start -> green light
const double GO_LEAD_MS = 1500;              // host: time for GO to reach everyone
const double READY_TIMEOUT_MS = 30000;       // host: go without peers that never reach the gate
const double NO_HOST_TIMEOUT_MS = 45000;     // peer: no GO from any host -> start alone
const double STALE_MS = 500;                 // no state for this long: freeze the remote kart
enum { MAX_MEMBERS = 16, RB = 16, HB = 128, TB = 32 };
enum { MSG_FINISH = 0x31, MSG_READY = 0x41, MSG_GO = 0x42 };
enum { ST_LIVE, ST_LOST, ST_LEFT };

struct Member {
    int id, slot, kart; // kart index, -1 for a spectator
    int status;
    int ready;          // host: highest READY attempt
    double frameMs;     // host: its frame time at the start gate
    char name[24];
    ECharID c1, c2;
    EKartID kartId;
};

struct Remote {
    State recv[RB];     // newest last
    int recvLen;
    u16 prevButtons;
    bool active, ghost, hidden;
    double lastRx;      // host ms
    // replica position history on the shared clock, for the error measure
    double histT[HB];
    f32 histPos[HB][3];
    int histHead, histLen;
    // per-second window and totals
    double errSum, errMax, ageSum;
    int errN, ageN, rx, snaps;
    double errSumAll, errMaxAll;
    int errNAll, rxAll, snapsAll;
    int finishMs;
    // received truths waiting for the replica history to reach their frame
    double truthF[TB];
    f32 truthPos[TB][3];
    int truthLen;
};

int sOnline = -1;
bool sMenuRace;          // a race started from the online menus (netsync.cpp), not the boot session
const char *sMenuJson;   // its session JSON
int sRaceHost;           // the member that sends GO ("host" of the session JSON)
Member sMembers[MAX_MEMBERS];
int sNumMembers;
int sKartMember[8]; // kart index -> member index
int sGrid[8];       // kart index -> start position (the order of the lobby slots)
int sNumKarts;
int sSelf, sSelfKart = -1, sSelfSlot = -1;
int sCourse = LUIGI_CIRCUIT, sLaps = 3;
ERaceLevel sLevel = LVL_150CC;
ERaceMode sMode = VERSUS_RACE; // battles come from the online menus (no items yet)
u32 sSeed;
char sCode[16];
KartGamePad *sNetPad[8];
u8 sNetPadMem[8][sizeof(KartGamePad)] __attribute__((aligned(16)));
Remote sRemote[8];

int sBoot;
// countdown gate
int sAttempt;
bool sWaiting, sStarted, sGoLogged;
double sGateSince, sGoFor[16], sFirstReady[16], sAllReadySince, sLastReadySent;
const char *sPhase = "";
// sending / stats
s32 sLastSentF = -1000;
double sLastNow, sLastStats;
int sFrames, sSent, sGoFrame;
bool sFinished;
int sGoalTime;

double hostNow() { return port_net_host_time(); }
double clockFrame() { return hostNow() / FRAME_MS; }
// without a page (node runs with MKDD_NET_SESSION) this peer is the host and nobody else is connected
bool isHost() { return sRaceHost ? sRaceHost == sSelf : port_net_host() == sSelf || !port_net_active(); }
bool clockSynced() { return port_net_clock_synced() || (!port_net_active() && !sMenuRace); }

Member *memberById(int id)
{
    for (int i = 0; i < sNumMembers; i++)
        if (sMembers[i].id == id)
            return &sMembers[i];
    return nullptr;
}

bool loadSession()
{
    static char buf[16384];
    int n = sMenuJson ? 0 : port_net_session_json(buf, sizeof buf);
    if (sMenuJson) {
        strncpy(buf, sMenuJson, sizeof buf - 1);
        buf[sizeof buf - 1] = 0;
    } else if (n <= 0) {
        // node / tests without a page: the session JSON from the environment
        char *e = gpu_js_env_str("MKDD_NET_SESSION");
        if (!e)
            return false;
        strncpy(buf, e, sizeof buf - 1);
        buf[sizeof buf - 1] = 0;
        free(e);
    }
    const char *settings = jfield(buf, "settings");
    sSelf = sMenuJson ? port_net_self() : jint(jfield(buf, "self"), 0);
    sRaceHost = sMenuJson ? jint(jfield(buf, "host"), 0) : 0;
    jcopy(jfield(buf, "code"), sCode, sizeof sCode);
    int course = jint(jfield(settings, "course"), LUIGI_CIRCUIT);
    sCourse = (course >= BABY_PARK && course <= SHERBET_LAND) ? course : LUIGI_CIRCUIT;
    int cc = jint(jfield(settings, "cc"), 150);
    sLevel = cc <= 50 ? LVL_50CC : cc <= 100 ? LVL_100CC : LVL_150CC;
    if (jint(jfield(settings, "mirror"), 0))
        sLevel = LVL_MIRROR;
    int mode = jint(jfield(settings, "mode"), VERSUS_RACE);
    sMode = mode == BALLOON_BATTLE || mode == ESCAPE_BATTLE || mode == BOMB_BATTLE ? (ERaceMode)mode : VERSUS_RACE;
    int level = jint(jfield(settings, "level"), -1); // the menus' ERaceLevel
    if (level >= LVL_50CC && level <= LVL_MIRROR)
        sLevel = (ERaceLevel)level;
    sLaps = jint(jfield(settings, "laps"), 3);
    if (sLaps < 1 || sLaps > 9)
        sLaps = 3;
    // every peer derives the same seed from the room code
    sSeed = 0x811c9dc5u;
    for (const char *c = sCode; *c; c++)
        sSeed = (sSeed ^ (u8)*c) * 0x01000193u;

    const char *members = jfield(buf, "members");
    sNumMembers = 0;
    for (int i = 0; sNumMembers < MAX_MEMBERS; i++) {
        const char *m = jelem(members, i);
        if (!m)
            break;
        Member &mb = sMembers[sNumMembers++];
        memset(&mb, 0, sizeof mb);
        mb.id = jint(jfield(m, "id"), 0);
        mb.slot = jint(jfield(m, "slot"), -1);
        mb.kart = -1;
        jcopy(jfield(m, "name"), mb.name, sizeof mb.name);
        const char *prof = jfield(m, "profile");
        int c1 = jint(jfield(prof, "char1"), MARIO), c2 = jint(jfield(prof, "char2"), LUIGI);
        int k = jint(jfield(prof, "kart"), RED_FIRE);
        mb.c1 = (ECharID)(c1 >= BABY_MARIO && c1 <= PETEY ? c1 : MARIO);
        mb.c2 = (ECharID)(c2 >= BABY_MARIO && c2 <= PETEY ? c2 : LUIGI);
        mb.kartId = (EKartID)(k >= RED_FIRE && k <= PARADE_KART ? k : RED_FIRE);
    }
    // grid positions in slot order; the local kart is kart 0, the others follow in slot order (the
    // game takes its local players to be the first karts: RaceApp's pad recorder and the status
    // views index karts by local player number)
    int bySlot[8], nSlot = 0;
    for (int slot = 0; slot < 8; slot++)
        for (int i = 0; i < sNumMembers; i++)
            if (sMembers[i].slot == slot && nSlot < 8)
                bySlot[nSlot++] = i;
    sNumKarts = 0;
    for (int pass = 0; pass < 2; pass++)
        for (int g = 0; g < nSlot; g++) {
            int i = bySlot[g];
            if ((sMembers[i].id == sSelf) != (pass == 0))
                continue;
            sMembers[i].kart = sNumKarts;
            sGrid[sNumKarts] = g;
            sKartMember[sNumKarts++] = i;
        }
    Member *me = memberById(sSelf);
    sSelfKart = me ? me->kart : -1;
    sSelfSlot = me ? me->slot : -1;
    fprintf(stderr, "[net] online session %s: course 0x%x, level %d, %d laps, %d karts, self %d (kart %d)\n", sCode,
            sCourse, (int)sLevel, sLaps, sNumKarts, sSelf, sSelfKart);
    for (int k = 0; k < sNumKarts; k++) {
        Member &m = sMembers[sKartMember[k]];
        fprintf(stderr, "[net]   kart %d: slot %d %s (member %d) chars %d+%d kart %d%s\n", k, m.slot, m.name, m.id, m.c1,
                m.c2, m.kartId, k == sSelfKart ? " (you)" : "");
    }
    if (!port_net_active() && !sMenuRace)
        for (int i = 0; i < sNumMembers; i++)
            if (sMembers[i].id != sSelf)
                sMembers[i].status = ST_LOST;
    return sNumKarts > 0;
}

KartGamePad *netPad(int k)
{
    if (!sNetPad[k])
        sNetPad[k] = new (sNetPadMem[k])
            KartGamePad(JUTGamePad::Port_unknown, KartGamePad::PORT_NET, KartGamePad::NETWORK, KartGamePad::STATE_0);
    return sNetPad[k];
}

// LANEntry::setRaceInfo for one console: VS, n human karts, no CPUs
void setupRaceInfo()
{
    gRaceInfo.setRace(sMode, sNumKarts, sNumKarts, 1, 1);
    gRaceInfo.setRaceLevel(sLevel);
    gRaceInfo.setRandomSeed(sSeed);
    gRaceInfo.setItemSlotType(1); // VS "no items": no item boxes (NETCODE.md phase 1)
    gRaceInfo.mVsLapNum = (s16)sLaps;
    for (int k = 0; k < sNumKarts; k++) {
        Member &m = sMembers[sKartMember[k]];
        gRaceInfo.setKart(k, m.kartId, m.c1, k == sSelfKart ? gpaKartPad[0] : netPad(k), m.c2, nullptr);
        gRaceInfo.setStartPosIdx(k, sGrid[k]);
    }
    // a spectator (no slot) watches kart 0
    gRaceInfo.setConsoleTarget(0, sSelfKart >= 0 ? sSelfKart : 0, false);
}

void setPhase(const char *p)
{
    if (p == sPhase)
        return;
    sPhase = p;
    netonline_js_phase(p);
}

// ---- messages ----
void put32(u8 *p, u32 v) { memcpy(p, &v, 4); }
u32 get32(const u8 *p) { u32 v; memcpy(&v, p, 4); return v; }

// this machine's frame time at the start gate (EMA), sent with READY: the countdown is 240 game frames
double sGateFrameMs;
double gateFrameMs() { return sGateFrameMs > FRAME_MS ? sGateFrameMs : FRAME_MS; }

// READY: u8 type, u8 attempt, u16 0, u32 frame time in 1/100 ms
void sendReady(int to)
{
    u8 m[8] = {MSG_READY, (u8)sAttempt, 0, 0};
    put32(m + 4, (u32)(gateFrameMs() * 100));
    port_net_send(to, PORT_NET_RELIABLE, m, sizeof m);
}

void hostGo(int attempt, double t)
{
    u8 m[16] = {MSG_GO, (u8)attempt};
    memcpy(m + 8, &t, 8);
    sGoFor[attempt & 15] = t;
    port_net_send(PORT_NET_ALL, PORT_NET_RELIABLE, m, sizeof m);
    fprintf(stderr, "[net] host: GO %d at host time %.0f (in %.0f ms)\n", attempt, t, t - hostNow());
}

// host, while waiting at its own gate: GO once every live member has reached the gate
void hostCheckGo()
{
    int a = sAttempt;
    if (sGoFor[a & 15] > 0)
        return;
    int waiting = 0;
    for (int i = 0; i < sNumMembers; i++) {
        Member &m = sMembers[i];
        if (m.id != sSelf && m.status == ST_LIVE && m.ready < a)
            waiting++;
    }
    double now = hostNow();
    if (sFirstReady[a & 15] == 0)
        sFirstReady[a & 15] = now;
    if (waiting && now - sFirstReady[a & 15] < READY_TIMEOUT_MS) {
        sAllReadySince = 0;
        return;
    }
    // everyone is there: give the frame time estimates (READY repeats every second) a moment
    if (!sAllReadySince)
        sAllReadySince = now;
    if (!waiting && now - sAllReadySince < 1500)
        return;
    if (waiting)
        fprintf(stderr, "[net] host: %d player(s) not at the start after %.0f s, starting without them\n", waiting,
                READY_TIMEOUT_MS / 1000);
    double lead = GO_LEAD_MS, slowest = gateFrameMs();
    for (int i = 0; i < sNumMembers; i++) {
        Member &m = sMembers[i];
        if (m.id == sSelf || m.status != ST_LIVE)
            continue;
        if (port_net_rtt(m.id) > 0 && 1000 + 2 * port_net_rtt(m.id) > lead)
            lead = 1000 + 2 * port_net_rtt(m.id);
        if (m.frameMs > slowest)
            slowest = m.frameMs;
    }
    if (slowest > 250)
        slowest = 250;
    sAllReadySince = 0;
    hostGo(a, now + lead + 240 * slowest);
}

void onState(int from, const u8 *p)
{
    Member *m = memberById(from);
    if (!m || m->kart < 0 || m->kart == sSelfKart || p[1] != (u8)m->slot)
        return;
    Remote &r = sRemote[m->kart];
    State s;
    u16 f16 = decode(p, s);
    double nowF = clockFrame();
    s32 nowI = (s32)nowF;
    s.frame = nowI + (s16)(u16)(f16 - (u16)nowI);
    r.lastRx = hostNow();
    r.rx++;
    r.rxAll++;
    if (m->status == ST_LOST)
        m->status = ST_LIVE;
    // true replica error: measured once the replica history covers the sender's frame (recordReplica)
    if (r.truthLen < TB) {
        r.truthF[r.truthLen] = s.frame;
        memcpy(r.truthPos[r.truthLen], s.pos, sizeof s.pos);
        r.truthLen++;
    }
    r.ageSum += nowF - s.frame;
    r.ageN++;
    // receive buffer: sorted by frame, no duplicates or stale samples
    if (r.recvLen && s.frame <= r.recv[r.recvLen - 1].frame) {
        for (int j = 0; j < r.recvLen; j++)
            if (r.recv[j].frame == s.frame)
                return;
        if (s.frame < r.recv[0].frame)
            return;
    }
    if (r.recvLen == RB) {
        memmove(r.recv, r.recv + 1, sizeof(State) * (RB - 1));
        r.recvLen--;
    }
    int at = r.recvLen;
    while (at > 0 && r.recv[at - 1].frame > s.frame)
        at--;
    memmove(r.recv + at + 1, r.recv + at, sizeof(State) * (r.recvLen - at));
    r.recv[at] = s;
    r.recvLen++;
}

void onEvent(int code, int id)
{
    Member *m = memberById(id);
    const char *name = m ? m->name : "?";
    switch (code) {
    case PORT_NET_EV_LEAVE:
        if (m)
            m->status = ST_LEFT;
        fprintf(stderr, "[net] %s left: kart removed\n", name);
        break;
    case PORT_NET_EV_LOST:
        if (m && m->status == ST_LIVE)
            m->status = ST_LOST;
        fprintf(stderr, "[net] connection to %s lost: kart frozen\n", name);
        break;
    case PORT_NET_EV_BACK:
        if (m && m->status == ST_LOST)
            m->status = ST_LIVE;
        fprintf(stderr, "[net] %s is back\n", name);
        break;
    case PORT_NET_EV_HOST:
        fprintf(stderr, "[net] %s is the host now\n", name);
        // a new host collects the READYs again
        if (sWaiting && sGoFor[sAttempt & 15] == 0) {
            sFirstReady[sAttempt & 15] = 0;
            if (id != sSelf)
                sendReady(id);
        }
        break;
    }
}

void pump()
{
    static u8 buf[2048];
    int from, kind, n;
    while ((n = port_net_recv(&from, &kind, buf, sizeof buf)) != 0) {
        if (n < 0) { // too big for us: drop it
            static u8 *big;
            static int bigCap;
            if (-n > bigCap) {
                bigCap = -n;
                big = (u8 *)JKRAllocFromSysHeap(bigCap, 4);
            }
            if (!big || port_net_recv(&from, &kind, big, bigCap) <= 0)
                break;
            continue;
        }
        if (kind == PORT_NET_EVENT) {
            if (n >= 8)
                onEvent(buf[0], (int)get32(buf + 4));
            port_netsync_message(from, kind, buf, n);
            continue;
        }
        switch (buf[0]) {
        case MSG_KART_STATE:
            if (n == WIRE && kind == PORT_NET_UNRELIABLE)
                onState(from, buf);
            break;
        case MSG_READY:
            if (n >= 8) {
                Member *m = memberById(from);
                if (m && buf[1] > m->ready)
                    m->ready = buf[1];
                if (m)
                    m->frameMs = get32(buf + 4) / 100.0;
                // a GO this peer may have missed (it arrived late or the host changed)
                if (m && isHost() && sGoFor[buf[1] & 15] > 0) {
                    u8 g[16] = {MSG_GO, buf[1]};
                    memcpy(g + 8, &sGoFor[buf[1] & 15], 8);
                    port_net_send(from, PORT_NET_RELIABLE, g, sizeof g);
                }
            }
            break;
        case MSG_GO:
            if (n >= 16) {
                double t;
                memcpy(&t, buf + 8, 8);
                if (t > 0 && sGoFor[buf[1] & 15] == 0) {
                    sGoFor[buf[1] & 15] = t;
                    fprintf(stderr, "[net] GO %d at host time %.0f (in %.0f ms)\n", buf[1], t, t - hostNow());
                }
            }
            break;
        case MSG_FINISH:
            if (n >= 8) {
                Member *m = memberById(from);
                if (m && m->kart >= 0 && m->kart != sSelfKart) {
                    int ms = (int)get32(buf + 4);
                    sRemote[m->kart].finishMs = ms;
                    fprintf(stderr, "[net] %s finished: %d:%02d.%03d\n", m->name, ms / 60000, ms / 1000 % 60, ms % 1000);
                }
            }
            break;
        default: // the online menus' (a START for the next round can come during a race)
            port_netsync_message(from, kind, buf, n);
            break;
        }
    }
}

// ---- remote karts ----
void freeze(KartBody *b, Remote &r, bool ghost)
{
    State s = r.recv[r.recvLen - 1];
    for (int j = 0; j < 3; j++)
        s.vel[j] = s.effVel[j] = s.wg[j] = 0;
    snapTo(b, s);
    if (ghost != r.ghost) {
        if (ghost)
            b->mGameStatus |= 0x10; // as a ghost: no kart-kart collisions
        else
            b->mGameStatus &= ~0x10u;
        r.ghost = ghost;
    }
}

void correctRemote(int k, double nowF, double dtF, f32 kDt)
{
    KartBody *b = KartCtrl::getKartCtrl()->getKartBody(k);
    Remote &r = sRemote[k];
    Member &m = sMembers[sKartMember[k]];
    if (!r.recvLen)
        return;
    const State &last = r.recv[r.recvLen - 1];
    if (m.status == ST_LEFT) {
        if (!r.hidden) {
            RaceMgr::getManager()->getKartDrawer(k)->hide();
            r.hidden = true;
        }
        freeze(b, r, true);
        return;
    }
    if (m.status == ST_LOST || hostNow() - r.lastRx > STALE_MS) {
        freeze(b, r, m.status == ST_LOST);
        return;
    }
    if (r.ghost) {
        b->mGameStatus &= ~0x10u;
        r.ghost = false;
    }
    double age = nowF - last.frame;
    if (age < 0)
        age = 0;
    if (age > 15)
        age = 15;
    // displacement per shared-clock frame: mEffctVel is per game frame of the sender, which differs when
    // the sender runs below 60 fps, so measure it from the last two samples when they are close
    f32 vel[3];
    for (int j = 0; j < 3; j++)
        vel[j] = last.effVel[j];
    if (r.recvLen >= 2) {
        const State &prev = r.recv[r.recvLen - 2];
        s32 df = last.frame - prev.frame;
        if (df > 0 && df <= 12) {
            f32 move[3], m2 = 0, e2v = 0;
            for (int j = 0; j < 3; j++) {
                move[j] = (last.pos[j] - prev.pos[j]) / (f32)df;
                m2 += move[j] * move[j];
                e2v += last.effVel[j] * last.effVel[j];
            }
            // trust the measured rate unless it is far above the kart's own speed (a snap or teleport)
            if (m2 <= e2v * 1.5f + 1.0f)
                for (int j = 0; j < 3; j++)
                    vel[j] = move[j];
        }
    }
    // (carrying the replica forward by the shared frames a slow local frame skips was tried: in the
    // two-Chrome test at ~9 fps it raised the average error, since the measured rate overshoots in turns
    // and wall contacts)
    f32 target[3], err[3], here[3], e2 = 0;
    for (int j = 0; j < 3; j++) {
        target[j] = last.pos[j] + vel[j] * (f32)age;
        here[j] = (&b->mPos.x)[j];
        err[j] = target[j] - here[j];
        e2 += err[j] * err[j];
    }
    if (!r.active || e2 > 1500.0f * 1500.0f) {
        State s = last;
        memcpy(s.pos, target, sizeof target);
        snapTo(b, s);
        r.snaps++;
        r.snapsAll++;
        r.active = true;
        return;
    }
    f32 pos[3], rot[3][3], cur[3][3];
    for (int j = 0; j < 3; j++)
        pos[j] = here[j] + err[j] * kDt;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            cur[i][j] = b->_110[i][j];
    nlerpRot(cur, last.rot, kDt, rot);
    setPose(b, pos, rot);
    b->mVel.x += (last.vel[0] - b->mVel.x) * kDt;
    b->mVel.y += (last.vel[1] - b->mVel.y) * kDt;
    b->mVel.z += (last.vel[2] - b->mVel.z) * kDt;
}

void recordReplica(int k, double nowF)
{
    KartBody *b = KartCtrl::getKartCtrl()->getKartBody(k);
    Remote &r = sRemote[k];
    r.histT[r.histHead] = nowF;
    r.histPos[r.histHead][0] = b->mPos.x;
    r.histPos[r.histHead][1] = b->mPos.y;
    r.histPos[r.histHead][2] = b->mPos.z;
    r.histHead = (r.histHead + 1) % HB;
    if (r.histLen < HB)
        r.histLen++;
    // received truths (sender's own state at shared frame f) vs the replica at f, interpolated between
    // the local frames around it
    int keep = 0;
    for (int t = 0; t < r.truthLen; t++) {
        double f = r.truthF[t];
        if (f > nowF) { // not reached yet here
            r.truthF[keep] = r.truthF[t];
            memcpy(r.truthPos[keep++], r.truthPos[t], sizeof r.truthPos[t]);
            continue;
        }
        int i2 = -1, i1 = -1;
        for (int n = 0, i = r.histHead; n < r.histLen; n++) {
            i = (i + HB - 1) % HB;
            if (r.histT[i] <= f) {
                i1 = i;
                break;
            }
            i2 = i;
        }
        if (i1 < 0 || i2 < 0 || r.histT[i2] - r.histT[i1] > 30)
            continue;
        f32 w = (f32)((f - r.histT[i1]) / (r.histT[i2] - r.histT[i1]));
        f32 at[3];
        for (int j = 0; j < 3; j++)
            at[j] = r.histPos[i1][j] + (r.histPos[i2][j] - r.histPos[i1][j]) * w;
        double e = dist(at, r.truthPos[t]);
        r.errSum += e;
        r.errN++;
        if (e > r.errMax)
            r.errMax = e;
        r.errSumAll += e;
        r.errNAll++;
        if (e > r.errMaxAll)
            r.errMaxAll = e;
    }
    r.truthLen = keep;
}

void stats(double now)
{
    static char js[4096];
    static double lastFrames, lastT;
    KartCtrl *ctrl = KartCtrl::getKartCtrl();
    double fps = lastT > 0 ? (sFrames - lastFrames) * 1000.0 / (now - lastT) : 0;
    lastFrames = sFrames;
    lastT = now;
    int n = snprintf(js, sizeof js, "{\"t\":%.0f,\"self\":%d,\"phase\":\"%s\",\"fps\":%.1f,\"go\":%.0f,\"sent\":%d,\"karts\":[",
                     now, sSelf, sPhase, fps, sGoFor[sAttempt & 15], sSent);
    for (int k = 0; k < sNumKarts && n < (int)sizeof js - 400; k++) {
        Member &m = sMembers[sKartMember[k]];
        KartBody *b = ctrl->getKartBody(k);
        n += snprintf(js + n, sizeof js - n, "%s{\"kart\":%d,\"slot\":%d,\"id\":%d,\"name\":\"%s\",\"pos\":[%.0f,%.0f,%.0f]",
                      k ? "," : "", k, m.slot, m.id, m.name, b->mPos.x, b->mPos.y, b->mPos.z);
        if (k == sSelfKart) {
            n += snprintf(js + n, sizeof js - n, ",\"local\":true,\"lap\":%d,\"goal\":%d}",
                          RaceMgr::getManager()->getKartChecker(k)->getLap(), sFinished ? sGoalTime : -1);
            continue;
        }
        Remote &r = sRemote[k];
        const char *st = m.status == ST_LEFT ? "left" : m.status == ST_LOST ? "lost"
                         : !r.recvLen ? "waiting" : now - r.lastRx > STALE_MS ? "stale" : "live";
        n += snprintf(js + n, sizeof js - n,
                      ",\"status\":\"%s\",\"err\":%.1f,\"errMax\":%.1f,\"errN\":%d,\"errAll\":%.1f,\"errMaxAll\":%.1f,"
                      "\"age\":%.1f,\"rx\":%d,\"snaps\":%d,\"goal\":%d}",
                      st, r.errN ? r.errSum / r.errN : 0.0, r.errMax, r.errN, r.errNAll ? r.errSumAll / r.errNAll : 0.0,
                      r.errMaxAll, r.ageN ? r.ageSum / r.ageN : 0.0, r.rx, r.snaps, r.finishMs ? r.finishMs : -1);
        if (r.errN || r.rx)
            fprintf(stderr, "[net] stats %s: err avg %.1f max %.1f (n %d) | age %.1f f | rx %d/s | snaps %d | %s | fps %.0f\n",
                    m.name, r.errN ? r.errSum / r.errN : 0.0, r.errMax, r.errN, r.ageN ? r.ageSum / r.ageN : 0.0, r.rx,
                    r.snaps, st, fps);
        r.errSum = r.errMax = r.ageSum = 0;
        r.errN = r.ageN = r.rx = r.snaps = 0;
    }
    if (n < (int)sizeof js - 3)
        snprintf(js + n, sizeof js - n, "]}");
    netonline_js_stats(js);
}

} // namespace

namespace netonline {

bool enabled()
{
    if (sOnline < 0) {
        sOnline = 0;
        if (port_env("MKDD_NET_ONLINE") && loadSession())
            sOnline = 1;
    }
    return sOnline == 1 || sMenuRace;
}

// the running race is the session's (not one started from the menus after it)
bool sessionRace()
{
    RaceMgr *rcm = RaceMgr::getManager();
    KartCtrl *ctrl = KartCtrl::getKartCtrl();
    return rcm && ctrl && rcm->getKartNumber() == sNumKarts && ctrl->GetKartNumber() == sNumKarts &&
           rcm->getRaceMode() == sMode &&
           (sSelfKart < 0 || rcm->getKartInfo(sSelfKart)->getPad(0) == gpaKartPad[0]);
}

void preDynamics()
{
    KartCtrl *ctrl = KartCtrl::getKartCtrl();
    if (!sessionRace())
        return;
    sFrames++;
    pump();
    double now = hostNow();
    for (int k = 0; k < sNumKarts && k < ctrl->GetKartNumber(); k++) {
        if (k == sSelfKart)
            continue;
        Remote &r = sRemote[k];
        makeRemote(ctrl->getKartBody(k));
        if (!r.recvLen)
            continue;
        State s = r.recv[r.recvLen - 1];
        if (now - r.lastRx > STALE_MS || sMembers[sKartMember[k]].status != ST_LIVE) {
            s.sx = s.sy = 0;
            s.buttons = 0;
            s.aL = s.aR = 0;
        }
        // input prediction: hold the newest input (PadMgr does not touch PORT_NET pads)
        feedPad(k, s, r.prevButtons);
    }
}

void postStage1()
{
    KartCtrl *ctrl = KartCtrl::getKartCtrl();
    RaceMgr *rcm = RaceMgr::getManager();
    if (!sessionRace())
        return;
    double now = hostNow(), nowF = now / FRAME_MS;
    // blend factor for this frame: 0.15 per 60 Hz frame, compounded over the frames this one took
    double dtF = sLastNow > 0 ? (now - sLastNow) / FRAME_MS : 1;
    sLastNow = now;
    if (dtF < 1)
        dtF = 1;
    if (dtF > 8)
        dtF = 8;
    f32 keep = 1;
    for (int i = 0; i < (int)dtF; i++)
        keep *= 0.85f;
    keep *= 1 - 0.15f * (f32)(dtF - (int)dtF);
    f32 kDt = 1 - keep;

    int n = ctrl->GetKartNumber();
    for (int k = 0; k < sNumKarts && k < n; k++) {
        if (k == sSelfKart)
            continue;
        correctRemote(k, nowF, dtF, kDt);
        Remote &r = sRemote[k];
        if (r.recvLen) {
            // keep the input on the pad for DoAccel (stage 2) and remember it for trigger edges
            State s = r.recv[r.recvLen - 1];
            if (now - r.lastRx > STALE_MS || sMembers[sKartMember[k]].status != ST_LIVE) {
                s.sx = s.sy = 0;
                s.buttons = 0;
                s.aL = s.aR = 0;
            }
            feedPad(k, s, r.prevButtons);
            r.prevButtons = s.buttons;
        }
        recordReplica(k, nowF);
    }

    if (sSelfKart >= 0 && sSelfKart < n) {
        s32 f = (s32)(nowF + 0.5);
        if (f - sLastSentF >= 2) { // 30 Hz
            State s;
            u8 msg[WIRE];
            capture(sSelfKart, f, s);
            encode(s, sSelfSlot, msg);
            port_net_send(PORT_NET_ALL, PORT_NET_UNRELIABLE, msg, WIRE);
            sLastSentF = f;
            sSent++;
        }
        static int trace = -1;
        if (trace < 0)
            trace = envInt("MKDD_NET_TRACE", 0);
        if (trace > 0 && sFrames % trace == 0) {
            KartBody *b = ctrl->getKartBody(sSelfKart);
            fprintf(stderr, "[net] trace %d %.0f %.0f %.0f\n", sFrames, b->mPos.x, b->mPos.y, b->mPos.z);
        }
        KartChecker *kc = rcm->getKartChecker(sSelfKart);
        if (kc->isGoal() && !sFinished) {
            sFinished = true;
            sGoalTime = kc->getTotalTime().get();
            u8 m[8] = {MSG_FINISH, (u8)sSelfSlot};
            put32(m + 4, (u32)sGoalTime);
            port_net_send(PORT_NET_ALL, PORT_NET_RELIABLE, m, sizeof m);
            fprintf(stderr, "[net] you finished: %d:%02d.%03d\n", sGoalTime / 60000, sGoalTime / 1000 % 60, sGoalTime % 1000);
            setPhase("finished");
        }
    }
    if (now - sLastStats >= 1000) {
        sLastStats = now;
        stats(now);
    }
}

} // namespace netonline

extern "C" int port_net_online(void) { return netonline::enabled(); }

// LogoApp, once the logos are done: set up the race from the session, load the course, enter RaceApp.
// Returns 1 while it owns the boot (online), 0 offline.
extern "C" int port_net_boot_race(void)
{
    if (!netonline::enabled())
        return 0;
    switch (sBoot) {
    case 0:
        setupRaceInfo();
        ResMgr::loadCourseData((ECourseID)sCourse, ResMgr::mcOrder_2);
        setPhase("loading");
        sBoot = 1;
        break;
    case 1:
        if (ResMgr::isFinishLoadingArc(ResMgr::mcArcCourse) && ResMgr::isFinishLoadingArc(ResMgr::mcArcMRAM) &&
            ResMgr::isFinishLoadingArc(ResMgr::mcArcMRAMLoc) && ResMgr::isFinishLoadingArc(ResMgr::mcArcRace2D) &&
            ResMgr::isFinishLoadingArc(ResMgr::mcArcARAM)) {
            // the keep data (MRAM, ARAM, Race2D) loads in the background since the logos; the race needs all of it
            AppMgr::deleteCurrentApp();
            RaceApp::call();
            sBoot = 2;
        }
        break;
    }
    return 1;
}

// TJugem, before the countdown lights: hold until the shared start time minus the countdown length.
extern "C" int port_net_countdown_hold(void)
{
    if (!netonline::enabled())
        return 0;
    pump();
    double now = hostNow();
    if (!sWaiting) {
        sWaiting = true;
        sStarted = false;
        sGoLogged = false;
        sAttempt++;
        sGateSince = now;
        sLastReadySent = 0;
        fprintf(stderr, "[net] at the start (attempt %d), waiting for the others\n", sAttempt);
        setPhase("waiting");
    }
    // this machine's frame time while waiting: the countdown is 240 game frames, which take longer than
    // 4 s below 60 fps, so a slow peer starts it earlier (and the host plans GO for the slowest)
    static double lastCall;
    if (lastCall > 0 && now - lastCall < 500)
        sGateFrameMs = sGateFrameMs > 0 ? sGateFrameMs * 0.9 + (now - lastCall) * 0.1 : now - lastCall;
    lastCall = now;
    if (!isHost() && sGoFor[sAttempt & 15] == 0 && now - sLastReadySent >= 1000) {
        sendReady(port_net_host());
        sLastReadySent = now;
    }
    if (isHost())
        hostCheckGo();
    double go = sGoFor[sAttempt & 15];
    if (go == 0) {
        if (now - sGateSince < NO_HOST_TIMEOUT_MS)
            return 1;
        fprintf(stderr, "[net] no GO from the host, starting alone\n");
        go = sGoFor[sAttempt & 15] = now + COUNTDOWN_MS;
    }
    double countdown = 240 * gateFrameMs();
    if (countdown > 60000)
        countdown = 60000;
    if (!clockSynced() || now < go - countdown)
        return 1;
    sWaiting = false;
    sStarted = true;
    lastCall = 0;
    setPhase("countdown");
    fprintf(stderr, "[net] countdown %d: starts %.0f ms before GO (%.1f ms/frame here), GO at host time %.0f\n", sAttempt,
            go - now, gateFrameMs(), go);
    return 0;
}

// TJugem, at the green light: hold a peer that got there early.
extern "C" int port_net_go_hold(void)
{
    if (!netonline::enabled() || !sStarted)
        return 0;
    double go = sGoFor[sAttempt & 15], now = hostNow();
    if (now < go)
        return 1;
    if (!sGoLogged) {
        sGoLogged = true;
        sGoFrame = sFrames;
        fprintf(stderr, "[net] GO %d: %.0f ms after the shared GO time (retrace %u)\n", sAttempt, now - go,
                port_vi_retrace_count());
        setPhase("race");
    }
    return 0;
}

// debug (tests): game frames since the local GO, -1 before it
extern "C" int netonline_race_frames(void) { return sGoLogged ? sFrames - sGoFrame : -1; }

// debug (tests): the local kart's position and forward axis (out[0..5]), a point on the course ahead
// (out[6..8]: the middle of the next sector's front checkpoint) and the kart's X axis (out[9..11], its
// left); 0 without a race
extern "C" int netonline_local_kart(f32 *out)
{
    if (!netonline::enabled() || sSelfKart < 0 || !netonline::sessionRace())
        return 0;
    KartBody *b = KartCtrl::getKartCtrl()->getKartBody(sSelfKart);
    out[0] = b->mPos.x;
    out[1] = b->mPos.y;
    out[2] = b->mPos.z;
    out[3] = b->_110[0][2];
    out[4] = b->_110[1][2];
    out[5] = b->_110[2][2];
    out[6] = out[7] = out[8] = 0;
    out[9] = b->_110[0][0];
    out[10] = b->_110[1][0];
    out[11] = b->_110[2][0];
    Course::Sector *s = RaceMgr::getManager()->getKartChecker(sSelfKart)->getCheckSector();
    if (s && s->getNextSector(0))
        s = s->getNextSector(0);
    if (s && s->mFPoint) {
        out[6] = (s->mFPoint->cpStart.x + s->mFPoint->cpEnd.x) * 0.5f;
        out[7] = (s->mFPoint->cpStart.y + s->mFPoint->cpEnd.y) * 0.5f;
        out[8] = (s->mFPoint->cpStart.z + s->mFPoint->cpEnd.z) * 0.5f;
    }
    return 1;
}

// netsync.cpp, at the end of the online menus: this race, described as session JSON by the host
extern "C" int netonline_menu_race(const char *json)
{
    sMenuRace = true;
    sMenuJson = json;
    // a fresh start gate, kart streams and stats for every race
    sAttempt = 0;
    sWaiting = sStarted = sGoLogged = sFinished = false;
    memset(sGoFor, 0, sizeof sGoFor);
    memset(sFirstReady, 0, sizeof sFirstReady);
    sAllReadySince = sLastReadySent = sGateSince = 0;
    memset(sRemote, 0, sizeof sRemote);
    sLastSentF = -1000;
    sLastNow = sLastStats = 0;
    sFrames = sSent = sGoFrame = 0;
    sGateFrameMs = 0;
    bool ok = loadSession();
    sMenuJson = nullptr;
    if (!ok) {
        sMenuRace = false;
        return 0;
    }
    setupRaceInfo();
    setPhase("loading");
    return 1;
}

// RaceApp, leaving a race of the online menus (patch 0022): back to the multiplayer scene
extern "C" int port_net_menu_race(void) { return sMenuRace ? 1 : 0; }
extern "C" void port_net_menu_race_end(void)
{
    if (sMenuRace)
        setPhase("");
    sMenuRace = false;
}
