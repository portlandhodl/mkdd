// Online menus: the game's own multiplayer menus (SceneMenu: players/mode/characters, SceneCourseSelect,
// SceneMapSelect) run in lockstep on every browser of a lobby room, as if all players sat at one
// console with one controller each.
//
// The host's multiplayer scene (sceneonline.cpp) sends START {seq, consoles, input delay, the host's
// unlock flags}; console i (the host is 0) is controller port i+1 on every machine. Each lockstep
// scene's init (patch 0022) arms a barrier: a SCENE {seq, epoch} message to everyone, and the next
// PADRead blocks until every console has reached the same scene (epoch). Then each game frame f of the
// epoch, PADRead sends the local controller as PAD {epoch, f + D} (reliable channel) and waits until the
// pads of frame f of every console are in; the four PADStatus it returns are the consoles' pads, so the
// menus see the same input on the same frame everywhere. Frames below D are neutral. A console that
// moved on to the next scene (SCENE carries the last frame it sent) or left counts as neutral after its
// last frame, so a console still fading out of the previous scene is not stuck. The menus' few
// wall-clock seeds (random driver, roulettes) use port_netsync_time() so they agree too.
//
// The race is not lockstep: at the end of course select (port_net_race_begin) the host describes the
// race (members with grid slot, drivers and kart; course, level, laps) as session JSON, sends it as RACE
// and every machine hands it to netonline.cpp (netonline_menu_race): one full-screen kart per browser,
// state sync, GO aligned on the host clock. After the race everyone returns to the multiplayer scene.
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "dolphin/pad.h"
#include "dolphin/vi.h"
#include "dolphin/os.h"
#include "JSystem/JUtility/JUTGamePad.h"
#include "Kaneshige/RaceInfo.h"
#include "Osako/SystemRecord.h"
#include "Osako/kartPad.h"
#include "Osako/ResMgr.h"
#include "Kameda/Scene.h"
#include "net_bridge.h"
#include "jsonlite.h"

using namespace jsonlite;

extern "C" int netonline_menu_race(const char *json);
extern "C" int port_env(const char *name);
extern "C" double emscripten_get_now(void);

namespace {

enum { MSG_START = 0x60, MSG_SCENE = 0x61, MSG_PAD = 0x62, MSG_RACE = 0x63, MSG_END = 0x64 };
enum { MAXC = 4, PB = 1024 };

struct PadIn {
    u16 button;
    s8 sx, sy, cx, cy;
    u8 tl, tr;
};

struct Console {
    int id;
    int sceneEpoch;   // highest epoch it reached
    int lastFrame[8]; // [epoch & 7]: last frame it sent in that epoch, -1 while it is still in it
    bool gone;
    char name[24];
};

struct Slot {
    int epoch, frame;
    PadIn p;
};

Console sCon[MAXC];
int sN, sSelfIdx, sDelay = 4;
int sSeq;            // START sequence of the current configuration
bool sConfigured;    // lockstep scenes are synchronised (between START and the race / leaving)
int sEpoch;          // epoch of the current lockstep scene
bool sArmed, sRunning;
int sFrame, sSentUpTo = -1;
double sWaitSince;
Slot sPads[MAXC][PB];
bool sStartPending;
u8 sStartMsg[64];
int sStartLen, sStartFrom;
char sRaceJson[8192];
bool sRaceReady;
bool sSavedRec;
u16 sOwnGameFlag, sOwnSecretKart;
int sOwnOptions;

void put32(u8 *p, u32 v) { memcpy(p, &v, 4); }
u32 get32(const u8 *p) { u32 v; memcpy(&v, p, 4); return v; }

void logf(const char *fmt, int a = 0, int b = 0, int c = 0, int d = 0)
{
    char msg[160], js[200];
    snprintf(msg, sizeof msg, fmt, a, b, c, d);
    fprintf(stderr, "[netsync] %s\n", msg);
    snprintf(js, sizeof js, "{\"msg\":\"netsync: %s\"}", msg);
    port_net_emit("log", js);
}

int consoleOf(int id)
{
    for (int i = 0; i < sN; i++)
        if (sCon[i].id == id)
            return i;
    return -1;
}

void onStart(int from, const u8 *m, int n)
{
    // START: u8 type, u8 seq, u8 consoles, u8 delay, u32 gameFlag, u32 secretKart, u32 options, u32 ids[]
    if (n < 16 + 4 * m[2] || m[2] < 1 || m[2] > MAXC)
        return;
    memcpy(sStartMsg, m, n);
    sStartLen = n;
    sStartFrom = from;
    sStartPending = true;
}

void apply(const u8 *m)
{
    int self = port_net_self();
    sSeq = m[1];
    sN = m[2];
    sDelay = m[3];
    sSelfIdx = 0;
    for (int i = 0; i < sN; i++) {
        memset(&sCon[i], 0, sizeof sCon[i]);
        sCon[i].id = (int)get32(m + 16 + 4 * i);
        for (int e = 0; e < 8; e++)
            sCon[i].lastFrame[e] = -1;
        if (sCon[i].id == self)
            sSelfIdx = i;
    }
    for (int i = 0; i < MAXC; i++)
        for (int f = 0; f < PB; f++)
            sPads[i][f].epoch = -1;
    // the menus offer what the host has unlocked (the same choices everywhere)
    if (!sSavedRec) {
        sSavedRec = true;
        sOwnGameFlag = gSystemRecord.mGameFlag;
        sOwnSecretKart = gSystemRecord.mSecretKart;
        sOwnOptions = gSystemRecord.mOptions;
    }
    gSystemRecord.mGameFlag = (u16)get32(m + 4);
    gSystemRecord.mSecretKart = (u16)get32(m + 8);
    gSystemRecord.mOptions = (int)get32(m + 12);
    sConfigured = true;
    sEpoch = 0;
    sArmed = sRunning = false;
    sRaceReady = false;
    logf("start %d: %d consoles, you are player %d, input delay %d frames", sSeq, sN, sSelfIdx + 1, sDelay);
}

void onMessage(int from, int kind, const u8 *m, int n)
{
    if (kind == PORT_NET_EVENT) {
        if (n >= 8 && m[0] == PORT_NET_EV_LEAVE) {
            int c = consoleOf((int)get32(m + 4));
            if (c >= 0 && sConfigured && c != sSelfIdx) {
                sCon[c].gone = true;
                logf("player %d left", c + 1);
            }
        }
        return;
    }
    int c = consoleOf(from);
    switch (m[0]) {
    case MSG_START:
        onStart(from, m, n);
        break;
    case MSG_SCENE: // u8 type, u8 seq, u16 0, u32 epoch, i32 last frame of the previous epoch
        if (n >= 12 && c >= 0 && m[1] == sSeq) {
            int e = (int)get32(m + 4);
            if (e > sCon[c].sceneEpoch) {
                sCon[c].sceneEpoch = e;
                sCon[c].lastFrame[(e - 1) & 7] = (int)get32(m + 8);
            }
        }
        break;
    case MSG_PAD: // u8 type, u8 seq, u16 0, u32 epoch, u32 frame, PadIn
        if (n >= 12 + (int)sizeof(PadIn) && c >= 0 && m[1] == sSeq) {
            int f = (int)get32(m + 8);
            Slot &s = sPads[c][f & (PB - 1)];
            s.epoch = (int)get32(m + 4);
            s.frame = f;
            memcpy(&s.p, m + 12, sizeof(PadIn));
        }
        break;
    case MSG_RACE:
        if (n > 1 && n - 1 < (int)sizeof sRaceJson) {
            memcpy(sRaceJson, m + 1, n - 1);
            sRaceJson[n - 1] = 0;
            sRaceReady = true;
            if (c >= 0)
                sCon[c].gone = true; // the host left the menus
        }
        break;
    case MSG_END:
        if (c >= 0 && sConfigured)
            sCon[c].gone = true;
        break;
    }
}

void pump()
{
    static u8 buf[9000];
    int from, kind, n;
    while ((n = port_net_recv(&from, &kind, buf, sizeof buf)) != 0) {
        if (n < 0) { // not ours: too big
            static u8 big[65536];
            if (port_net_recv(&from, &kind, big, sizeof big) <= 0)
                break;
            continue;
        }
        onMessage(from, kind, buf, n);
    }
}

void sendAll(const u8 *m, int n)
{
    for (int i = 0; i < sN; i++)
        if (i != sSelfIdx && !sCon[i].gone)
            port_net_send(sCon[i].id, PORT_NET_RELIABLE, m, n);
}

// a console's pad for frame f of the current epoch; false while it is still to come
bool padOf(int c, int f, PadIn &out)
{
    memset(&out, 0, sizeof out);
    if (f < sDelay || sCon[c].gone)
        return true;
    const Slot &s = sPads[c][f & (PB - 1)];
    if (s.epoch == sEpoch && s.frame == f) {
        out = s.p;
        return true;
    }
    if (sCon[c].sceneEpoch > sEpoch && f > sCon[c].lastFrame[sEpoch & 7])
        return true; // it moved on to the next scene before this frame
    return false;
}

// neutral controllers at a scene change: the first frame of a scene must not see a button that one
// machine still had down from the previous scene
void clearPads()
{
    for (int i = 0; i < 4; i++) {
        JUTGamePad::CButton &b = JUTGamePad::mPadButton[i];
        b.mButton = b.mTrigger = b.mRelease = b.mRepeat = b.mRepeatTimer = b.mRepeatLastButton = 0;
        b.mAnalogA = b.mAnalogB = b.mAnalogL = b.mAnalogR = 0;
        b.mAnalogLf = b.mAnalogRf = 0;
        JUTGamePad::mPadMStick[i].clear();
        JUTGamePad::mPadSStick[i].clear();
    }
    for (JSUListIterator<JUTGamePad> it = JUTGamePad::mPadList.getFirst(); it != JUTGamePad::mPadList.getEnd(); ++it)
        it->update();
}

bool waiting(const char *what, double limitMs)
{
    double now = emscripten_get_now();
    if (sWaitSince == 0)
        sWaitSince = now;
    if (now - sWaitSince > limitMs) {
        for (int i = 0; i < sN; i++)
            if (i != sSelfIdx && !sCon[i].gone) {
                bool missing = false;
                if (sArmed)
                    missing = sCon[i].sceneEpoch < sEpoch;
                else {
                    PadIn p;
                    missing = !padOf(i, sFrame, p);
                }
                if (missing) {
                    sCon[i].gone = true;
                    logf("player %d did not answer: playing on without them", i + 1);
                }
            }
        sWaitSince = 0;
        return false;
    }
    (void)what;
    return true;
}

} // namespace

// ---- hooks ----

// sdk/vi_pad.c PADRead, after the local controller is in status[0]. Returns 1 when it filled status.
extern "C" int port_netsync_pad(PADStatus *status)
{
    if (!sConfigured || !(sArmed || sRunning))
        return 0;
    PadIn local = {status[0].button, status[0].stickX, status[0].stickY, status[0].substickX, status[0].substickY,
                   status[0].triggerLeft, status[0].triggerRight};
    if (sArmed) {
        bool shown = false;
        for (;;) {
            pump();
            bool all = true;
            for (int i = 0; i < sN; i++)
                if (i != sSelfIdx && !sCon[i].gone && sCon[i].sceneEpoch < sEpoch)
                    all = false;
            if (all || !waiting("scene", 60000))
                break;
            if (!shown) {
                shown = true;
                port_net_emit("sync", "{\"wait\":\"players\"}");
            }
            VIWaitForRetrace();
        }
        if (shown)
            port_net_emit("sync", "{\"wait\":\"\"}");
        sWaitSince = 0;
        sArmed = false;
        sRunning = true;
        sFrame = 0;
        sSentUpTo = -1;
    }
    // this frame's input goes out for frame + D
    int t = sFrame + sDelay;
    u8 m[12 + sizeof(PadIn)] = {MSG_PAD, (u8)sSeq};
    put32(m + 4, (u32)sEpoch);
    put32(m + 8, (u32)t);
    memcpy(m + 12, &local, sizeof local);
    sendAll(m, sizeof m);
    Slot &own = sPads[sSelfIdx][t & (PB - 1)];
    own.epoch = sEpoch;
    own.frame = t;
    own.p = local;
    sSentUpTo = t;

    PadIn pads[MAXC];
    for (;;) {
        pump();
        bool all = true;
        for (int i = 0; i < sN; i++)
            if (!padOf(i, sFrame, pads[i]))
                all = false;
        if (all || !waiting("pads", 20000))
            break;
        VIWaitForRetrace();
    }
    sWaitSince = 0;
    for (int i = 0; i < 4; i++) {
        PADStatus &s = status[i];
        memset(&s, 0, sizeof s);
        if (i < sN) {
            s.button = pads[i].button;
            s.stickX = pads[i].sx;
            s.stickY = pads[i].sy;
            s.substickX = pads[i].cx;
            s.substickY = pads[i].cy;
            s.triggerLeft = pads[i].tl;
            s.triggerRight = pads[i].tr;
            s.err = PAD_ERR_NONE;
        } else
            s.err = PAD_ERR_NO_CONTROLLER;
    }
    sFrame++;
    return 1;
}

// sdk/vi_pad.c SIProbe: the number of controllers the menus see; 0 offline (port 1 only)
extern "C" int port_netsync_ports(void) { return sConfigured && (sArmed || sRunning) ? sN : 0; }

// wall-clock seeds in the menus (SequenceInfo::setRandomSeed, roulettes): the same value everywhere
extern "C" u32 port_netsync_time(u32 fallback)
{
    if (!sConfigured || !sRunning)
        return fallback;
    return (u32)sSeq * 7919u + (u32)sEpoch * 104729u + (u32)sFrame * 2654435761u;
}

extern "C" int port_netsync_online(void) { return sConfigured ? 1 : 0; }
extern "C" int port_netsync_player_count(void) { return sConfigured ? sN : 1; }
extern "C" int port_netsync_is_host(void) { return sConfigured && sSelfIdx == 0; }

// leave the lockstep (back in the multiplayer scene, at the title, or the race starts)
extern "C" void port_netsync_stop(void)
{
    if (!sConfigured)
        return;
    u8 m[4] = {MSG_END, (u8)sSeq};
    sendAll(m, sizeof m);
    sConfigured = sArmed = sRunning = false;
}

// a scene's init (patch 0022): menu scenes are lockstep while configured, anything else ends it
extern "C" void port_netsync_scene(int scene)
{
    if (!sConfigured)
        return;
    switch (scene) {
    case Scene::SCENE_MENU:
    case Scene::SCENE_COURSE:
    case Scene::SCENE_MAP_SELECT:
        break;
    default:
        port_netsync_stop();
        return;
    }
    int last = sRunning ? sSentUpTo : -1;
    sEpoch++;
    sArmed = true;
    sRunning = false;
    sWaitSince = 0;
    u8 m[12] = {MSG_SCENE, (u8)sSeq};
    put32(m + 4, (u32)sEpoch);
    put32(m + 8, (u32)last);
    sendAll(m, sizeof m);
    clearPads();
}

// ---- the multiplayer scene (sceneonline.cpp) ----

extern "C" void port_netsync_pump(void) { pump(); }

// host: everyone in the room (up to 4, the host first) goes to the menus. Returns the player count.
extern "C" int port_netsync_host_start(void)
{
    int ids[16], n = port_net_members(ids, 16), self = port_net_self();
    if (n < 1)
        return 0;
    u8 m[16 + 4 * MAXC] = {MSG_START, (u8)(sSeq + 1), 0, 0};
    int k = 0;
    put32(m + 16 + 4 * k++, (u32)self);
    int maxRtt = 0;
    for (int i = 0; i < n && k < MAXC; i++)
        if (ids[i] != self) {
            put32(m + 16 + 4 * k++, (u32)ids[i]);
            if (port_net_rtt(ids[i]) > maxRtt)
                maxRtt = port_net_rtt(ids[i]);
        }
    m[2] = (u8)k;
    // input delay: one way + margin, in 60 Hz frames (the frame counter, not wall time, is what waits)
    int d = (int)((maxRtt / 2.0 + 30) / 16.7) + 1;
    m[3] = (u8)(d < 3 ? 3 : d > 12 ? 12 : d);
    put32(m + 4, gSystemRecord.mGameFlag);
    put32(m + 8, gSystemRecord.mSecretKart);
    put32(m + 12, (u32)gSystemRecord.mOptions);
    int len = 16 + 4 * k;
    for (int i = 1; i < k; i++)
        port_net_send((int)get32(m + 16 + 4 * i), PORT_NET_RELIABLE, m, len);
    apply(m);
    return k;
}

// any member: a START arrived (also while still racing); applies it and returns 1 once
extern "C" int port_netsync_take_start(void)
{
    pump();
    if (!sStartPending)
        return 0;
    sStartPending = false;
    int self = port_net_self();
    bool in = false;
    for (int i = 0; i < sStartMsg[2]; i++)
        if ((int)get32(sStartMsg + 16 + 4 * i) == self)
            in = true;
    if (!in)
        return 0;
    apply(sStartMsg);
    return 1;
}

// leaving multiplayer for the title: own unlock flags back
extern "C" void port_netsync_leave(void)
{
    port_netsync_stop();
    sStartPending = false;
    if (sSavedRec) {
        gSystemRecord.mGameFlag = sOwnGameFlag;
        gSystemRecord.mSecretKart = sOwnSecretKart;
        gSystemRecord.mOptions = sOwnOptions;
        sSavedRec = false;
    }
}

// netonline.cpp: messages that are not the race's
extern "C" void port_netsync_message(int from, int kind, const u8 *m, int n) { onMessage(from, kind, m, n); }

// SceneCourseSelect/SceneMapSelect::nextRace, before RaceApp::call (patch 0022). Returns 1 to go on
// (offline at once; online once the race is agreed), 0 to wait a frame.
extern "C" int port_net_race_begin(void)
{
    if (!sConfigured)
        return 1;
    if (sSelfIdx == 0) {
        static char lob[8192];
        if (port_net_lobby_json(lob, sizeof lob) <= 0)
            lob[0] = 0;
        const char *members = jfield(lob, "members");
        char *js = sRaceJson;
        int cap = sizeof sRaceJson, n = 0;
        int laps = gRaceInfo.mVsLapNum > 0 ? gRaceInfo.mVsLapNum : 3;
        if (port_env("MKDD_NET_LAPS") > 0) // tests: shorter races
            laps = port_env("MKDD_NET_LAPS");
        n += snprintf(js + n, cap - n,
                      "{\"code\":\"m%d\",\"host\":%d,\"self\":%d,\"settings\":{\"course\":%d,\"level\":%d,\"laps\":%d,"
                      "\"mode\":%d},\"members\":[",
                      sSeq, sCon[0].id, sCon[0].id, (int)ResMgr::msCourseID, (int)gRaceInfo.getRaceLevel(), laps,
                      (int)gRaceInfo.getRaceMode());
        bool first = true;
        for (int k = 0; k < gRaceInfo.getKartNumber() && k < 8; k++) {
            KartInfo *ki = gRaceInfo.getKartInfo(k);
            int c = -1;
            for (int i = 0; i < sN; i++)
                if (ki->getPad(0) == gpaKartPad[i])
                    c = i;
            if (c < 0)
                continue; // CPU karts are not online yet
            char name[24] = "";
            for (int i = 0; const char *mb = jelem(members, i); i++)
                if (jint(jfield(mb, "id"), 0) == sCon[c].id)
                    jcopy(jfield(mb, "name"), name, sizeof name);
            if (!name[0])
                snprintf(name, sizeof name, "Player %d", c + 1);
            n += snprintf(js + n, cap - n, "%s{\"id\":%d,\"name\":\"%s\",\"slot\":%d,\"profile\":{\"char1\":%d,\"char2\":%d,\"kart\":%d}}",
                          first ? "" : ",", sCon[c].id, name, (int)gRaceInfo.mStartPosIndex[k], (int)ki->getDriverCharID(0),
                          (int)ki->getDriverCharID(1), (int)ki->getKartID());
            first = false;
        }
        snprintf(js + n, cap - n, "]}");
        int len = (int)strlen(js);
        static u8 msg[8193];
        msg[0] = MSG_RACE;
        memcpy(msg + 1, js, len);
        sendAll(msg, len + 1);
        fprintf(stderr, "[netsync] race: %s\n", js);
    } else {
        pump();
        if (!sRaceReady)
            return 0;
    }
    sConfigured = sArmed = sRunning = false;
    sRaceReady = false;
    netonline_menu_race(sRaceJson);
    return 1;
}
