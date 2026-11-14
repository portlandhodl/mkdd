// The MULTIPLAYER scene: title menu entry 4 (the slot of the console's LAN mode, patch 0022) opens it in
// place of SceneLanEntry, on the LAN entry archive's animated background, with the game's message font
// and menu sounds.
//
//   offline   Connect (lobby server and name come from the page's Multiplayer panel) / Back
//   lobby     Create room / Join room / Disconnect / Back
//   join      on-screen code picker: stick or arrows choose, A types, B deletes, START or OK joins
//   room      the code, the members (host, you, ready, link); host: Start race, others: Ready;
//             Leave room (asks first). B asks to leave.
//
// Every lobby action goes to the page as a "lobby" bus event (netbus.c -> web/netbus.js -> the lobby
// client of net.js), the lobby state comes back as JSON (port_net_lobby_json) a few times a second.
// Start race (host, everyone else ready) sends START to the room (netsync.cpp) and every member's game
// goes to the menus (SCENE_MENU) with the players' controllers in lockstep. Races of the online menus
// come back here.
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "types.h"
#include "Kameda/Scene.h"
#include "Kameda/SequenceInfo.h"
#include "Kaneshige/RaceInfo.h"
#include "Inagaki/GameAudioMain.h"
#include "JSystem/J2D/J2DGrafContext.h"
#include "JSystem/J2D/J2DPrint.h"
#include "JSystem/JUtility/JUTFader.h"
#include "JSystem/JFramework/JFWDisplay.h"
#include "Osako/LANBackground.h"
#include "Osako/FontMgr.h"
#include "Osako/SequenceApp.h"
#include "Osako/kartPad.h"
#include "Osako/system.h"
#include "net_bridge.h"
#include "jsonlite.h"

using namespace jsonlite;

extern "C" int port_netsync_host_start(void);
extern "C" int port_netsync_take_start(void);
extern "C" void port_netsync_leave(void);
extern "C" void port_netsync_pump(void);
extern "C" void port_netsync_scene(int scene);

namespace {

const char CODE_CHARS[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // the lobby server's room code alphabet
enum { CODE_COLS = 8, CODE_ROWS = 4, CODE_LEN = 6, MAX_MEMBERS = 8 };

enum Mode { M_OFFLINE, M_LOBBY, M_JOIN, M_ROOM, M_CONFIRM, M_LEAVING };
enum Item {
    I_CONNECT, I_BACK, I_CREATE, I_JOIN, I_DISCONNECT, I_START, I_READY, I_LEAVE, I_NO, I_YES,
};

struct MemberView {
    int id;
    char name[24];
    bool ready, host, self;
    char link[12];
    int rtt;
};

struct LobbyView {
    char state[16];
    char server[96];
    char name[24];
    char error[96];
    char code[8];
    int self, host;
    int n;
    MemberView m[MAX_MEMBERS];
};

void readLobby(LobbyView &v)
{
    static char buf[8192];
    memset(&v, 0, sizeof v);
    if (port_net_lobby_json(buf, sizeof buf) <= 0) {
        strcpy(v.state, "offline");
        return;
    }
    jcopy(jfield(buf, "state"), v.state, sizeof v.state);
    jcopy(jfield(buf, "server"), v.server, sizeof v.server);
    jcopy(jfield(buf, "name"), v.name, sizeof v.name);
    jcopy(jfield(buf, "error"), v.error, sizeof v.error);
    jcopy(jfield(buf, "code"), v.code, sizeof v.code);
    v.self = jint(jfield(buf, "self"), 0);
    v.host = jint(jfield(buf, "host"), 0);
    const char *ms = jfield(buf, "members");
    for (int i = 0; v.n < MAX_MEMBERS; i++) {
        const char *m = jelem(ms, i);
        if (!m)
            break;
        MemberView &mv = v.m[v.n++];
        mv.id = jint(jfield(m, "id"), 0);
        jcopy(jfield(m, "name"), mv.name, sizeof mv.name);
        if (!mv.name[0])
            snprintf(mv.name, sizeof mv.name, "Player %d", v.n);
        mv.ready = jint(jfield(m, "ready"), 0) != 0;
        jcopy(jfield(m, "link"), mv.link, sizeof mv.link);
        mv.rtt = jint(jfield(m, "rtt"), 0);
        mv.host = mv.id == v.host;
        mv.self = mv.id == v.self;
    }
}

void lobbyOp(const char *op, const char *extra = nullptr)
{
    char js[160];
    snprintf(js, sizeof js, "{\"op\":\"%s\"%s%s}", op, extra ? "," : "", extra ? extra : "");
    port_net_emit("lobby", js);
}

void se(u32 id) { GameAudio::Main::getAudio()->startSystemSe(id); }
const u32 SE_MOVE = 0x20000, SE_DECIDE = 0x20002, SE_CANCEL = 0x20004, SE_OK = 0x20003;

const JUtility::TColor WHITE(255, 255, 255, 255), YELLOW(255, 230, 40, 255), ORANGE(255, 150, 0, 255),
    GREY(170, 170, 190, 255), GREEN(110, 255, 110, 255), RED(255, 110, 90, 255), PANEL(0, 0, 40, 150),
    HILITE(255, 200, 0, 90);

} // namespace

class SceneOnline : public Scene {
public:
    SceneOnline(JKRArchive *archive, JKRHeap *heap) : Scene(archive, heap), mPrint(FontMgr::mspResFont, 0.0f)
    {
        mBackground.setup(archive);
    }
    ~SceneOnline() {}

    void init()
    {
        mTimer = 0;
        mCursor = 0;
        mCodeLen = 0;
        mCodeCur = 0;
        mCode[0] = 0;
        mNext = Scene::SCENE_MAX;
        mMsg[0] = 0;
        mMsgTimer = 0;
        mLeaving = false;
        port_netsync_scene(Scene::SCENE_LAN_ENTRY); // out of any lockstep
        readLobby(mLobby);
        mMode = modeFor(mLobby);
        GameAudio::Main::getAudio()->startSequenceBgm(0x1000001);
        System::getDisplay()->startFadeIn(15);
        port_net_emit("phase", "{\"phase\":\"multiplayer\"}");
    }

    void calc()
    {
        mTimer++;
        if (mTimer % 6 == 1 || mMode == M_LEAVING)
            readLobby(mLobby);
        events();
        if (mMode == M_LEAVING) {
            leaving();
        } else {
            // the host started: everyone in the room goes to the menus
            if (mLobby.state[0] == 'r' && port_netsync_take_start()) {
                se(SE_OK);
                toMenus();
            } else {
                Mode want = modeFor(mLobby);
                if (mMode != M_JOIN && mMode != M_CONFIRM && want != mMode) {
                    mMode = want;
                    mCursor = 0;
                } else if (mMode == M_JOIN && want == M_ROOM) {
                    mMode = M_ROOM;
                    mCursor = 0;
                } else if (mMode == M_CONFIRM && want != M_ROOM) {
                    mMode = want;
                    mCursor = 0;
                }
                input();
            }
        }
        if (mMsgTimer > 0)
            mMsgTimer--;
        mBackground.calc();
    }

    void draw()
    {
        J2DOrthoGraph *o = System::getJ2DOrtho();
        o->setPort();
        mBackground.mScreen.draw(0.0f, 0.0f, o);
        o->setPort();
        mPrint.initiate();
        text(60, 52, 30, YELLOW, ORANGE, "MULTIPLAYER");
        switch (mMode) {
        case M_OFFLINE:
            drawOffline();
            break;
        case M_LOBBY:
            drawLobby();
            break;
        case M_JOIN:
            drawJoin();
            break;
        case M_ROOM:
        case M_CONFIRM:
        case M_LEAVING:
            drawRoom();
            if (mMode == M_CONFIRM)
                drawConfirm();
            break;
        }
        const char *msg = mMsgTimer > 0 ? mMsg : mLobby.error;
        if (msg[0])
            text(60, 440, 15, RED, RED, "%s", msg);
    }

private:
    Mode modeFor(const LobbyView &v)
    {
        if (!strcmp(v.state, "room"))
            return M_ROOM;
        if (!strcmp(v.state, "connected"))
            return M_LOBBY;
        return M_OFFLINE;
    }

    bool isHost() const { return mLobby.self && mLobby.self == mLobby.host; }
    const MemberView *me() const
    {
        for (int i = 0; i < mLobby.n; i++)
            if (mLobby.m[i].self)
                return &mLobby.m[i];
        return nullptr;
    }
    bool canStart() const
    {
        for (int i = 0; i < mLobby.n; i++) {
            const MemberView &m = mLobby.m[i];
            if (!m.self && (!m.ready || strcmp(m.link, "open")))
                return false;
        }
        return mLobby.n >= 1;
    }

    int items(Item *out)
    {
        int n = 0;
        switch (mMode) {
        case M_OFFLINE:
            out[n++] = I_CONNECT;
            out[n++] = I_BACK;
            break;
        case M_LOBBY:
            out[n++] = I_CREATE;
            out[n++] = I_JOIN;
            out[n++] = I_DISCONNECT;
            out[n++] = I_BACK;
            break;
        case M_ROOM:
            out[n++] = isHost() ? I_START : I_READY;
            out[n++] = I_LEAVE;
            break;
        case M_CONFIRM:
            out[n++] = I_NO;
            out[n++] = I_YES;
            break;
        default:
            break;
        }
        return n;
    }

    const char *label(Item it)
    {
        static char buf[48];
        switch (it) {
        case I_CONNECT:
            return !strcmp(mLobby.state, "connecting") ? "Connecting..." : "Connect";
        case I_BACK:
            return "Back to title";
        case I_CREATE:
            return "Create room";
        case I_JOIN:
            return "Join room";
        case I_DISCONNECT:
            return "Disconnect";
        case I_START:
            return canStart() ? "Start" : "Start (waiting for everyone to be ready)";
        case I_READY: {
            const MemberView *m = me();
            snprintf(buf, sizeof buf, "Ready: %s", m && m->ready ? "YES" : "no");
            return buf;
        }
        case I_LEAVE:
            return "Leave room";
        case I_NO:
            return "No";
        case I_YES:
            return "Yes, leave";
        }
        return "";
    }

    void message(const char *m)
    {
        strncpy(mMsg, m, sizeof mMsg - 1);
        mMsg[sizeof mMsg - 1] = 0;
        mMsgTimer = 300;
    }

    void events()
    {
        char type[32], js[512];
        int n;
        while ((n = port_net_poll_event(type, sizeof type, js, sizeof js)) != 0) {
            if (n < 0) {
                static char big[8192];
                port_net_poll_event(type, sizeof type, big, sizeof big);
                continue;
            }
            if (!strcmp(type, "lobby-error")) {
                char msg[96];
                jcopy(jfield(js, "msg"), msg, sizeof msg);
                message(msg);
                se(SE_CANCEL);
            } else if (!strcmp(type, "lobby-closed"))
                message("Disconnected from the lobby server.");
        }
        port_netsync_pump();
    }

    void input()
    {
        KartGamePad *pad = gpaGamePad[0];
        if (mMode == M_JOIN) {
            codeInput(pad);
            return;
        }
        Item it[8];
        int n = items(it);
        if (n == 0)
            return;
        if (mCursor >= n)
            mCursor = n - 1;
        if (pad->testRepeat(JUTGamePad::MAINSTICK_UP | JUTGamePad::DPAD_UP)) {
            se(SE_MOVE);
            mCursor = (mCursor + n - 1) % n;
        } else if (pad->testRepeat(JUTGamePad::MAINSTICK_DOWN | JUTGamePad::DPAD_DOWN)) {
            se(SE_MOVE);
            mCursor = (mCursor + 1) % n;
        }
        if (pad->testTrigger(JUTGamePad::A | JUTGamePad::START)) {
            choose(it[mCursor]);
        } else if (pad->testTrigger(JUTGamePad::B)) {
            se(SE_CANCEL);
            switch (mMode) {
            case M_OFFLINE:
            case M_LOBBY:
                toTitle();
                break;
            case M_ROOM:
                mMode = M_CONFIRM;
                mCursor = 0;
                break;
            case M_CONFIRM:
                mMode = M_ROOM;
                mCursor = 0;
                break;
            default:
                break;
            }
        }
    }

    void choose(Item it)
    {
        switch (it) {
        case I_CONNECT:
            if (strcmp(mLobby.state, "connecting")) {
                se(SE_DECIDE);
                lobbyOp("connect");
                strcpy(mLobby.state, "connecting");
            }
            break;
        case I_BACK:
            se(SE_CANCEL);
            toTitle();
            break;
        case I_CREATE:
            se(SE_DECIDE);
            lobbyOp("create");
            break;
        case I_JOIN:
            se(SE_DECIDE);
            mMode = M_JOIN;
            mCodeLen = 0;
            mCode[0] = 0;
            mCodeCur = 0;
            break;
        case I_DISCONNECT:
            se(SE_CANCEL);
            lobbyOp("disconnect");
            break;
        case I_START:
            if (!canStart()) {
                se(SE_CANCEL);
                message("Everyone else must be ready (and connected) first.");
                break;
            }
            se(SE_OK);
            if (port_netsync_host_start() > 0)
                toMenus();
            break;
        case I_READY: {
            const MemberView *m = me();
            se(SE_DECIDE);
            lobbyOp("ready", m && m->ready ? "\"ready\":false" : "\"ready\":true");
            break;
        }
        case I_LEAVE:
            se(SE_DECIDE);
            mMode = M_CONFIRM;
            mCursor = 0;
            break;
        case I_NO:
            se(SE_CANCEL);
            mMode = M_ROOM;
            mCursor = 0;
            break;
        case I_YES:
            se(SE_DECIDE);
            port_netsync_leave();
            lobbyOp("leave");
            mMode = M_LOBBY;
            mCursor = 0;
            break;
        }
    }

    // code picker: CODE_ROWS rows of CODE_COLS characters, then a row with DEL and OK
    void codeInput(KartGamePad *pad)
    {
        int total = CODE_COLS * CODE_ROWS; // cells; total = DEL, total + 1 = OK
        int row = mCodeCur < total ? mCodeCur / CODE_COLS : CODE_ROWS;
        int col = mCodeCur < total ? mCodeCur % CODE_COLS : (mCodeCur == total ? 0 : 4);
        bool moved = false;
        if (pad->testRepeat(JUTGamePad::MAINSTICK_UP | JUTGamePad::DPAD_UP)) {
            row = (row + CODE_ROWS) % (CODE_ROWS + 1);
            moved = true;
        } else if (pad->testRepeat(JUTGamePad::MAINSTICK_DOWN | JUTGamePad::DPAD_DOWN)) {
            row = (row + 1) % (CODE_ROWS + 1);
            moved = true;
        } else if (pad->testRepeat(JUTGamePad::MAINSTICK_LEFT | JUTGamePad::DPAD_LEFT)) {
            col = row == CODE_ROWS ? (col ? 0 : 4) : (col + CODE_COLS - 1) % CODE_COLS;
            moved = true;
        } else if (pad->testRepeat(JUTGamePad::MAINSTICK_RIGHT | JUTGamePad::DPAD_RIGHT)) {
            col = row == CODE_ROWS ? (col ? 0 : 4) : (col + 1) % CODE_COLS;
            moved = true;
        }
        if (moved) {
            se(SE_MOVE);
            mCodeCur = row < CODE_ROWS ? row * CODE_COLS + col : (col < 4 ? total : total + 1);
        }
        if (pad->testTrigger(JUTGamePad::A)) {
            if (mCodeCur < total) {
                if (mCodeLen < CODE_LEN) {
                    se(SE_DECIDE);
                    mCode[mCodeLen++] = CODE_CHARS[mCodeCur];
                    mCode[mCodeLen] = 0;
                    if (mCodeLen == CODE_LEN)
                        mCodeCur = total + 1; // on OK
                } else
                    se(SE_CANCEL);
            } else if (mCodeCur == total)
                del();
            else
                submit();
        } else if (pad->testTrigger(JUTGamePad::START)) {
            submit();
        } else if (pad->testTrigger(JUTGamePad::B)) {
            if (mCodeLen > 0)
                del();
            else {
                se(SE_CANCEL);
                mMode = M_LOBBY;
                mCursor = 1;
            }
        }
    }
    void del()
    {
        if (mCodeLen > 0) {
            se(SE_CANCEL);
            mCode[--mCodeLen] = 0;
        }
    }
    void submit()
    {
        if (mCodeLen != CODE_LEN) {
            se(SE_CANCEL);
            message("A room code has 6 characters.");
            return;
        }
        char js[32];
        snprintf(js, sizeof js, "\"code\":\"%s\"", mCode);
        se(SE_OK);
        lobbyOp("join", js);
    }

    void toTitle()
    {
        port_netsync_leave();
        mNext = Scene::SCENE_TITLE;
        mMode = M_LEAVING;
        mLeaving = false;
    }
    void toMenus()
    {
        // what the title does before the menus (SceneTitle::nextScene): same start state everywhere
        mNext = Scene::SCENE_MENU;
        mMode = M_LEAVING;
        mLeaving = false;
        port_net_emit("phase", "{\"phase\":\"menus\"}");
    }
    void leaving()
    {
        if (!mLeaving) {
            mLeaving = true;
            mLeaveTimer = 0;
            System::getDisplay()->getFader()->setColor(JUtility::TColor(0, 0, 0, 255));
            System::getDisplay()->startFadeOut(15);
            GameAudio::Main::getAudio()->fadeOutAll(15);
        }
        if (mLeaveTimer++ > 15 && SequenceApp::ptr()->ready(mNext)) {
            if (mNext == Scene::SCENE_MENU) {
                gSequenceInfo.init();
                gRaceInfo.reset();
            }
            SequenceApp::ptr()->setNextScene(mNext);
        }
    }

    // ---- drawing ----
    void text(f32 x, f32 y, f32 size, JUtility::TColor top, JUtility::TColor bottom, const char *fmt, ...)
    {
        char buf[160];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        mPrint.setFontSize(size, size);
        mPrint.setGradColor(top);
        mPrint.setCharColor(bottom);
        mPrint.initiate();
        mPrint.print(x, y, "%s", buf);
    }
    void panel(f32 x, f32 y, f32 w, f32 h, JUtility::TColor c)
    {
        J2DOrthoGraph *o = System::getJ2DOrtho();
        o->setPort();
        o->setColor(c);
        o->fillBox(x, y, w, h);
    }
    void menu(f32 x, f32 y)
    {
        Item it[8];
        int n = items(it);
        for (int i = 0; i < n; i++) {
            bool sel = i == mCursor;
            if (sel)
                panel(x - 12, y + i * 34 - 24, 420, 30, HILITE);
            text(x, y + i * 34, 22, sel ? YELLOW : WHITE, sel ? ORANGE : GREY, "%s%s", sel ? "> " : "  ", label(it[i]));
        }
    }
    void drawOffline()
    {
        panel(40, 80, 560, 320, PANEL);
        text(60, 112, 16, WHITE, GREY, "Lobby server: %s", mLobby.server[0] ? mLobby.server : "(set in the page)");
        text(60, 136, 16, WHITE, GREY, "Your name: %s", mLobby.name[0] ? mLobby.name : "Player");
        text(60, 172, 16, GREY, GREY, "Race friends in other browsers: connect,");
        text(60, 194, 16, GREY, GREY, "then create a room or join one with its code.");
        menu(70, 250);
    }
    void drawLobby()
    {
        panel(40, 80, 560, 320, PANEL);
        text(60, 112, 16, GREEN, GREEN, "Connected as %s", mLobby.name[0] ? mLobby.name : "Player");
        text(60, 136, 16, WHITE, GREY, "%s", mLobby.server);
        menu(70, 190);
    }
    void drawJoin()
    {
        panel(40, 80, 560, 340, PANEL);
        text(60, 110, 18, WHITE, GREY, "Enter the room code:");
        for (int i = 0; i < CODE_LEN; i++) {
            f32 x = 330 + i * 40;
            panel(x, 88, 34, 36, i == mCodeLen ? HILITE : JUtility::TColor(255, 255, 255, 50));
            if (i < mCodeLen)
                text(x + 7, 118, 26, YELLOW, ORANGE, "%c", mCode[i]);
        }
        int total = CODE_COLS * CODE_ROWS;
        for (int r = 0; r < CODE_ROWS; r++)
            for (int c = 0; c < CODE_COLS; c++) {
                int i = r * CODE_COLS + c;
                f32 x = 90 + c * 58, y = 150 + r * 50;
                bool sel = i == mCodeCur;
                panel(x, y, 48, 42, sel ? JUtility::TColor(255, 200, 0, 200) : JUtility::TColor(255, 255, 255, 40));
                text(x + 13, y + 32, 26, sel ? WHITE : WHITE, sel ? ORANGE : GREY, "%c", CODE_CHARS[i]);
            }
        f32 y = 150 + CODE_ROWS * 50;
        panel(90, y, 220, 40, mCodeCur == total ? JUtility::TColor(255, 200, 0, 200) : JUtility::TColor(255, 255, 255, 40));
        text(160, y + 30, 22, WHITE, mCodeCur == total ? ORANGE : GREY, "DELETE");
        panel(334, y, 220, 40, mCodeCur == total + 1 ? JUtility::TColor(255, 200, 0, 200) : JUtility::TColor(255, 255, 255, 40));
        text(390, y + 30, 22, WHITE, mCodeCur == total + 1 ? ORANGE : GREY, "JOIN");
        text(60, 412, 14, GREY, GREY, "A: type   B: delete / back   START: join");
    }
    void drawRoom()
    {
        panel(40, 80, 560, 340, PANEL);
        text(60, 118, 18, WHITE, GREY, "ROOM");
        text(140, 124, 36, YELLOW, ORANGE, "%s", mLobby.code);
        text(400, 118, 14, GREY, GREY, "%d player%s", mLobby.n, mLobby.n == 1 ? "" : "s");
        for (int i = 0; i < mLobby.n; i++) {
            const MemberView &m = mLobby.m[i];
            f32 y = 160 + i * 26;
            text(64, y, 18, m.self ? YELLOW : WHITE, m.self ? ORANGE : GREY, "%d. %s%s", i + 1, m.name, m.self ? " (you)" : "");
            if (m.host)
                text(330, y, 16, YELLOW, ORANGE, "HOST");
            else
                text(330, y, 16, m.ready ? GREEN : GREY, m.ready ? GREEN : GREY, m.ready ? "READY" : "not ready");
            if (!m.self) {
                if (!strcmp(m.link, "open"))
                    text(450, y, 14, GREY, GREY, "%d ms", m.rtt);
                else
                    text(450, y, 14, RED, RED, "%s", m.link);
            }
        }
        f32 y = 160 + (mLobby.n < 4 ? 4 : mLobby.n) * 26 + 20;
        if (mMode == M_ROOM)
            menu(70, y);
        if (mLobby.n > 4)
            text(60, 400, 13, GREY, GREY, "Up to 4 players race; the first 4 in the room take part.");
        else if (!isHost())
            text(60, 400, 13, GREY, GREY, "The host picks mode and course; everyone picks drivers and kart.");
    }
    void drawConfirm()
    {
        panel(140, 190, 360, 140, JUtility::TColor(20, 0, 0, 230));
        text(170, 230, 22, WHITE, GREY, "Leave the room?");
        menu(190, 270);
    }

    LANBackground mBackground;
    J2DPrint mPrint;
    LobbyView mLobby;
    Mode mMode;
    int mCursor, mTimer;
    char mCode[CODE_LEN + 1];
    int mCodeLen, mCodeCur;
    char mMsg[96];
    int mMsgTimer;
    Scene::SceneType mNext;
    bool mLeaving;
    int mLeaveTimer;
};

// SceneFactory, SCENE_LAN_ENTRY (patch 0022)
Scene *port_net_create_scene(JKRArchive *archive, JKRHeap *heap) { return new (heap, 0) SceneOnline(archive, heap); }
