# Online play: lobby and transport

The lobby server, the WebRTC transport and the wasm bridge that the state-sync
netcode (`NETCODE.md`, on the `netcode` branch) runs on. This layer has no game
protocol: it carries the netcode's messages between players. Each player loads
their own disc image; only lobby messages go to the server and game messages go
peer-to-peer.

| Path | Contents |
| --- | --- |
| `server/` | lobby + WebRTC signalling server (Node, `ws`), optional static file server |
| `net_bridge.c/.h` | the game's side: `port_net_send` / `port_net_recv` / session info / host clock |
| `netonline.cpp`, `netonline_js.c` | online race (phase 1): session → `RaceInfo`, start sync, kart state stream, remote karts, stats |
| `netkart.cpp/.h` | kart state wire format, remote-kart correction, the single-process spike harness |
| `sceneonline.cpp` | the game's MULTIPLAYER scene (title menu entry 4): connect, create/join (code picker), room |
| `netsync.cpp` | online menus: START, lockstep controllers for the menu scenes, the agreed race for `netonline.cpp` |
| `netbus.c`, `jsonlite.h` | game ↔ page event bus (`port_net_emit` / `port_net_poll_event` / `port_net_lobby_json`), tiny JSON reader |
| `../web/netbus.js` | the bus's page side: lobby commands from the game, lobby state, bridge attachment, background connect |
| `../web/net.js` | `Lobby`, `Mesh` (peers, channels, RTT, host clock, events), wasm bridge |
| `../web/netui.js` | Multiplayer panel (`index.html` `#mp`) and in-game overlay |
| `test/` | server tests, transport tests on mock WebRTC, two-Chrome end-to-end test |

## Running the server

```sh
npm ci --prefix tools/web_wasm/net/server           # installs ws into ./node_modules only
node tools/web_wasm/net/server/server.mjs           # ws://127.0.0.1:8095/ws
# one process for page + lobby:
STATIC_DIR=build/web_wasm/web HOST=0.0.0.0 node tools/web_wasm/net/server/server.mjs
```

Or keep `tools/web_wasm/web/serve.sh` for the page and run the server next to it:
the page's default lobby URL is `ws://<page host>:8095/ws` (a page served by the
server itself uses its own origin). The URL is editable in the panel; `?netserver=`
sets it.

| Env | Default | |
| --- | --- | --- |
| `PORT`, `HOST` | `8095`, `127.0.0.1` | listen address (`0.0.0.0` for other machines) |
| `STATIC_DIR` | - | also serve this directory (the web build) over HTTP |
| `MAX_MEMBERS` | 8 | players per room (2-8) |
| `STUN_URLS` | Google + Cloudflare public STUN | comma list, empty for none |
| `TURN_URLS` | - | comma list, e.g. `turn:turn.example.org:3478?transport=udp` |
| `TURN_USERNAME`, `TURN_CREDENTIAL` | - | static TURN credentials, handed to clients |
| `TURN_SECRET`, `TURN_TTL` | -, 6 h | coturn `use-auth-secret`: per-client time-limited credentials instead |
| `MAX_ROOMS`, `MAX_CONN_PER_IP`, `ROOMS_PER_MIN` | 500, 16, 10 | limits |
| `MAX_MSG_BYTES`, `MSG_RATE`, `MSG_BURST` | 64 KiB, 60/s, 200 | per-connection message limits |
| `HEARTBEAT_MS`, `ROOM_IDLE_MS` | 10 s, 6 h | dead-socket check, idle room expiry |
| `TRUST_PROXY` | - | `1`: use `X-Forwarded-For` for the per-IP limits (behind a reverse proxy) |
| `QUIET` | - | `1`: do not log room events |

The server logs room events to stdout: connections (name, game ID), room created / joined /
left / host changes, starts (members, slots, settings), refused joins and starts with the
reason, a member that could not boot into the race, ended sessions. No addresses are logged.

## Playing online from the game's own menus

The page is only settings: player name and lobby server (Multiplayer drawer, kept in
localStorage; the server defaults to the page's own host). Once a disc is chosen it connects
to the lobby in the background. Press Play; everything else happens in the game and needs no
reload:

1. Title screen: **MULTIPLAYER** (the fourth entry, where the console offered LAN MODE).
2. The multiplayer scene shows the server and your name; Connect if the page did not already.
   **Create room** shows the 6-character code big; **Join room** opens an on-screen picker
   (stick/arrows choose, A types, B deletes, START joins). The room lists the members with
   host / READY / ping. Others tick **Ready**; the host presses **Start** (up to 4 players).
3. Every member's game goes to the normal menus, which run in lockstep: the room's players are
   controller ports 1-4 (the host is port 1 and drives the mode, class and course screens as
   player 1 does on one console), the player count is fixed to the room's, VS only (no co-op).
   Each player picks their own drivers and kart in the character select and sees the others'
   picks live.
4. At the end of course select the host describes the race (karts in grid order with drivers
   and kart, course, class, laps) and every game starts the state-sync race of `NETCODE.md`
   (`netonline.cpp`) with one full-screen kart per browser and a shared GO.
5. After the race, any result-menu choice returns to the room screen; the host can start the
   next round.

How it works: `NETCODE.md` §16. Limits for now: VS races of up to 4 players, items off (as in
phase 1), no CPU karts (a Grand Prix runs as a VS race of the players), battles have no items.

## How to play with a friend (page-started session, phase 1)

The original phase 1 flow (the panel's Ready / GO! boots a fresh game straight into a race)
is still there for tests and debugging behind `?netpageboot=1`.

What you get: each browser drives its own kart full-screen and sees the other
players' karts (up to 8) move over the network (`NETCODE.md`). VS race, humans only
(no CPU karts), items off, no pause. After the race each player sees their own
results; reload the page to race again.

Build once (both players must use the same build, the room checks the `mkdd.wasm`
fingerprint):

```sh
source ~/emsdk/emsdk_env.sh
python tools/web_wasm/tools/build.py --target web            # build/web_wasm/web
npm ci --prefix tools/web_wasm/net/server
```

**Same LAN.** On one machine (say `192.168.1.20`) serve the page and the lobby from one
process:

```sh
STATIC_DIR=build/web_wasm/web HOST=0.0.0.0 PORT=8095 node tools/web_wasm/net/server/server.mjs
```

Both players open `http://192.168.1.20:8095/` (the page finds the lobby on its own
origin), choose their own disc image, open Multiplayer, type a name and Connect. Do not press
Play first: a game that already runs solo cannot switch to the online race, so the panel
refuses to create or join a room until the page is reloaded ("Reload to play online" does
that and rejoins the room). In a room the solo Play / Start buttons are gone; only the
host's GO starts the game, on every window at once.
One presses Create room and reads out the 6-letter code; the other types it and
Joins. Everyone picks Driver, Partner and Kart; the host picks Course, Class, Laps
and Mirror (and slots = grid positions). Non-hosts tick Ready, the host presses
Start (GO stays disabled, with the reason as its tooltip and below it, until every member is
connected, ready, has a disc and an idle game). The drawer closes on every window, the boot
lights show, and both games boot straight into the race (about 10-20 s to load the course),
wait for each other at the Lakitu, and the countdown ends on the same host-clock
moment everywhere. Keys: X accelerate, arrows steer, Z brake, Q/W drift; a gamepad
works too. The overlay (top left) shows ping, the race phase and, per other player,
`live` / `stale` / `lost` / `left` and how far off their kart is (units; a kart is
about 200 long).

**Over the internet.** Same as below: host page and lobby where both can reach them
(port forward TCP 8095, or a TLS reverse proxy and `wss://`), then as on a LAN with
`http://<your public IP or host name>:8095/`. Game traffic is peer-to-peer over
WebRTC; behind symmetric NATs set up TURN (see below).

Notes: a browser tab in the background is throttled and its kart stops being sent
(others see it frozen, then `lost` after 5 s); keep the game tab visible. A player
whose machine cannot hold 60 fps is in slow motion for themselves and starts late.

## Playing with a friend over the internet

1. Host the page and the lobby where both can reach them, e.g. on your machine with
   `STATIC_DIR=build/web_wasm/web HOST=0.0.0.0 PORT=8095`, forward TCP 8095 on your
   router and give your friend `http://<your public IP>:8095/`. Behind a TLS reverse
   proxy use `wss://` (an `https` page picks `wss`).
2. Both choose their own disc image (same game ID, e.g. GM4E01) and use the same
   build (the room refuses a different `mkdd.wasm`).
3. One creates a room, the other joins with the 6-character code, picks drivers and
   kart and ticks Ready; the host picks slots and the race and presses Start session.

Game traffic is peer-to-peer (WebRTC data channels over UDP). STUN is enough for
most home NATs; two symmetric NATs (some mobile/CGNAT networks) need a TURN relay:
run coturn and set `TURN_URLS` with credentials (`TURN_SECRET` for coturn's
`use-auth-secret`). Without TURN such peers stay at "connecting".

## Lobby protocol (JSON over WebSocket, `/ws`)

Client to server: `hello {name, gameId, build, proto}`, `create`, `join {code}`,
`leave`, `name {name}`, `ready {ready}`, `profile {profile}` (own choice, any JSON
object up to 2 KB, e.g. `{char1, char2, kart}`), `status {status}` (own state: `{disc,
idle}` = a disc image is chosen / the game was not started solo; during a session `{boot,
why}` = `booting`, the game's `netPhase`, or `failed` with the reason), `settings {game, slots}` (host:
`game` is any JSON object up to 2 KB, e.g. `{course, cc, laps, mirror}`; `slots`
maps member id to slot 0..7 or -1), `start` (host, everyone else ready, nobody's status has `disc` or `idle` false), `end`
(host), `kick {id}` (host), `signal {to, data}` (SDP/ICE relay), `ping {ts}`.

Server to client: `welcome {id, proto, iceServers}`, `room {code, host, state
(lobby|playing), settings, maxMembers, gameId, build, members[{id, name, slot,
ready, profile, status}]}`, `peer-left {id, reason}`, `host-changed {host, reason}` (the
longest-standing member takes over, also during a session), `ended`, `signal
{from, data}`, `pong`, `error {code, msg, re}`. Every joiner's game ID and build
fingerprint (64-bit hash of `mkdd.wasm`) must equal the room's. `ready` is refused (error,
member stays not ready) while the member's status says it cannot race.

A member whose page cannot boot at the session start (older page, game already running)
reports `status {boot: 'failed', why}`: it shows why with a "Reload to play online" button,
the others are told who could not join, and the host can end the session (`end`); everyone
then reloads into the same room (`?netroom=`, retried while the room is still in a session).

## Transport (net.js `Mesh`)

A full mesh of RTCPeerConnections (the lower member id offers) with two negotiated
data channels per peer: `unrel` (unordered, `maxRetransmits: 0`) and `rel`
(ordered, reliable).

```js
const mesh = new MkddNet.Mesh(lobby, { sim: { lag, jitter, loss }, log });
mesh.send(to /* member id | peer | 'all' */, MkddNet.KIND.UNRELIABLE | KIND.RELIABLE, Uint8Array);
mesh.on('message', (peer, kind, bytes) => ...);
mesh.on('peer-join' | 'peer-leave' | 'peer-lost' | 'peer-back', (peer) => ...);
mesh.on('start', (session) => ...);   // { code, host, self, settings, members[{id, name, slot, profile}] }
mesh.on('end' | 'host' | 'peers', ...);
mesh.hostTime();                      // the host's clock in ms; mesh.clockSynced()
mesh.peers.get(id).rtt;               // smoothed RTT (ms), also rttMax
```

- Message bytes start with the message type, `0x00-0xEF`; `0xF0-0xFF` are the
  transport's: `PING 0xF0 (u32 seq, f64 t)` and `PONG 0xF1 (u32 seq, f64 t echoed,
  f64 t_responder)` every 250 ms on the unreliable channel.
- Host clock: offset = median of the last 16 samples of `t_host + rtt/2 - t_local`
  from pongs of the host (`NETCODE.md` §9). Reset when the host changes.
- Liveness: a joined peer silent for 5 s or with a disconnected ICE link is `lost`
  until a packet arrives again; leaving the room is `peer-leave`.
- Session start: when the host starts, the room state becomes `playing` and every
  page emits `start` with the member list (slots, profiles) and the host's game
  settings; membership is then frozen (leaves are marked, nobody new joins).
- Simulated network (tests): `?netlag=ms&netjitter=ms&netloss=0..1` on the page,
  latency on both channels (reliable order kept), loss on the unreliable one.

## Event bus (`net_bridge.h`, `netbus.c`, `../web/netbus.js`)

Two-way JSON events between the game and the page, beside the binary transport:

```c
port_net_emit("lobby", "{\"op\":\"join\",\"code\":\"ABC234\"}"); /* commands for the page */
port_net_emit("phase", "{\"phase\":\"menus\"}");                    /* anything: window 'mkdd-game' event */
while ((n = port_net_poll_event(type, sizeof type, json, sizeof json)) != 0) ...  /* e.g. lobby-error */
port_net_lobby_json(buf, cap);  /* {state, server, name, self, code, host, members[{id,name,ready,link,rtt}]} */
```

Lobby ops: `connect`, `disconnect`, `create`, `join {code}`, `leave`, `ready {ready}`, `name {name}`,
`profile {profile}`, `hello` (the game is in its multiplayer menu). A game that drove the lobby
reports status `{inGame: true}`, which the server accepts as able to race although the game is
running (`idle` stays the rule for page-started sessions). `netbus.js` also attaches the wasm
bridge (`Mesh.attachModule`) to the current mesh whenever the game runs in a room, so the game can
talk to the other members without a lobby session (`port_net_live()`).

## Game side (`net_bridge.h`)

At session start the page boots the game with `MKDD_NET_ONLINE=1` and attaches the
mesh (`Mesh.attachModule(Module)` -> `Module.netBridge`). C/C++ (e.g. `netkart.cpp`):

```c
#include "net_bridge.h"
port_net_active();  port_net_self();  port_net_host();
port_net_members(ids, max);  port_net_slot(id);  port_net_rtt(id);
port_net_host_time();  port_net_clock_synced();
port_net_session_json(buf, cap);         /* the start event's JSON */
port_net_send(PORT_NET_ALL or id, PORT_NET_UNRELIABLE or PORT_NET_RELIABLE, data, len);
while ((n = port_net_recv(&from, &kind, buf, sizeof buf)) > 0) { ... }  /* once per frame */
```

Incoming messages queue in JS (up to 4096; when full the oldest unreliable message
is dropped). Peer and session changes arrive in the same queue as `PORT_NET_EVENT`
messages: `u8 code (PORT_NET_EV_JOIN/LEAVE/LOST/BACK/START/END/HOST), 3 x 0, u32
member id`. Without a page or session every call reports "nothing" (node runs work).

## Testing

```sh
npm test --prefix tools/web_wasm/net/server                 # lobby server (node:test)
node --test tools/web_wasm/net/test/mesh.test.mjs           # Mesh on mock WebRTC
python tools/web_wasm/tools/build.py --target web --tag netweb
MKDD_ISO=/path/GM4E01.iso node tools/web_wasm/net/test/e2e.mjs
NET_LAG=50 NET_JITTER=10 NET_LOSS=0.1 MKDD_ISO=... node tools/web_wasm/net/test/e2e.mjs
```

`e2e.mjs` serves `build/web_wasm/web-netweb` from the lobby server on port 18610,
starts two headless Chrome instances (debug ports 19610/19611), creates and joins a
room through the panel, sets a profile and the race, starts the session without
booting the game (`?netnoboot=1`; only the disc header is read), then checks
messages both ways on both channels, the wasm bridge (`port_net_send` on one page,
`port_net_recv` on the other), the session JSON, host-clock agreement and a stream
of 600 56-byte unreliable packets.

```sh
MKDD_ISO=/path/GM4E01.iso node tools/web_wasm/net/test/e2e-race.mjs
NET_LAG=50 NET_JITTER=10 NET_LOSS=0.05 MKDD_ISO=... node tools/web_wasm/net/test/e2e-race.mjs
```

`e2e-race.mjs` (same server and Chrome ports as `e2e.mjs`, output in `build/web_wasm/net-race`)
does the same lobby steps without `netnoboot`: both pages boot the game into one
online race on Luigi Circuit. An in-page bot drives each player's own kart with
KeyboardEvents toward the course ahead (the karts steer into each other for the
first second after GO), canvas screenshots are taken from both browsers
(`RACE_SHOTS` seconds after GO), and the replica error each game measured is
summarised in `summary.json`. Debug exports used by it: `Module.netStats`,
`Module.netStatsLog`, `Module.netPhase`, `Module._port_net_debug_frame()`,
`Module._port_net_debug_kart(ptr)` (`net/netonline_js.c`). Node runs can boot the
online path without a page: `MKDD_NET_ONLINE=1 MKDD_NET_SESSION='<session JSON>'`
(the other members count as lost; `MKDD_NET_TRACE=N` prints the local kart's
position every N frames).

```sh
MKDD_ISO=/path/GM4E01.iso node tools/web_wasm/net/test/e2e-start.mjs
```

`e2e-start.mjs` (server 18630, Chrome 19630/19631, output and screenshots in
`build/web_wasm/net-start`) starts a session the way a player does, with mouse clicks and
typing: connecting without a disc, pressing Play before joining (refused, "Reload to play
online" rejoins), GO with a guest not ready (disabled with the reason, a forced start is
refused and explained), a member whose game runs (blocks GO, cannot be Ready), a member that
cannot boot at the start (both sides told, the host ends the session, both reload into the
room), then the real start (drawer closes, countdown and boot lights, keyboard focus on the
game, both games load the race) and the server's room log.

All three e2e tests take `MKDD_WWW`, `MKDD_NET_PORT`, `MKDD_NET_DEBUG_PORT` (Chrome debug
port, and +1) and `MKDD_NET_OUT`.
