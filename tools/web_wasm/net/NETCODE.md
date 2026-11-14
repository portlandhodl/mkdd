# Online multiplayer netcode: design

Status: phase 1 implemented (branch `netcode-p1`; see §15): two to eight browsers
race each other, each driving its own kart, items off. Scope: the WebAssembly port
(`tools/web_wasm`). Decomp files stay pristine. Game-side hooks are patches in
`tools/web_wasm/patches/`, and port-side code lives in `tools/web_wasm/net/`.

This design is the alternative to lockstep input netplay, which is being
built separately. Lockstep feeds identical pads to identical simulations and
needs input delay plus perfect determinism. In this design:

- Every browser runs its own player's kart locally with **zero input delay**
  and is the authority for that kart.
- Each browser broadcasts kart state at 30 Hz.
- Remote karts are reconstructed locally.
- Items and hits travel as events.
- Race flow (start, laps, ranking, results) is host-authoritative.

The two efforts share the transport. This design reuses the lobby/signalling
server and WebRTC data channels being built in `tools/web_wasm/net/` and
`web/net.js`. It adds no sockets of its own.

All code references are to the decomp tree (`src/`, `include/`) unless
prefixed with `tools/web_wasm/`.

---

## 1. Summary of the recommendation

| Area | Decision |
| --- | --- |
| Remote karts | **Hybrid** (§4). The remote kart stays a full `KartBody`. It runs the game's own physics every frame on the remote player's latest inputs (input prediction = hold last input). After integration, its pose and velocity are pulled toward the newest received state, extrapolated to "now", with an exponential blend (time constant about 6 frames, or 100 ms). Errors above a threshold snap. Wheels, suspension, driver animation, sounds, effects, drift sparks and kart-to-kart bumps all fall out of the game's own code. |
| Kart identity | Remote karts are created exactly as the game creates LAN karts. They get a `KartGamePad` of type `NETWORK` with port `PORT_NET` (player kind 3, `mGameStatus |= 0x802`, `src/Yamamoto/kartBody.cpp:2012`). The port writes that pad from network input. No CPU logic (`mGameStatus & 8`) runs for them. |
| Local kart | Unchanged: `NORMAL` pad on port 1 and full local simulation. Sampled after pose integration and sent at 30 Hz (60 Hz optional on LAN). |
| Items | Events: `ITEM_USE` from the owner, `ITEM_HIT` from the victim, `ITEM_DESTROY`, plus host-owned item boxes and roulette. The owner simulates its projectiles authoritatively and streams red shell, spiny and bomb state at 10–15 Hz. Every peer runs the projectile locally for visuals and local hit tests. **The victim decides hits on its own kart** (victim authority, the "favour the target" rule that suits a kart racer). |
| Roulette | Host-authoritative. The local roulette runs for presentation. The result comes from the host in an `ITEM_GRANT` and overrides `ItemShuffleMgr::SlotItem`. Fallback (phase 3a): each peer rolls its own item with its own RNG stream and announces it. |
| Hazards | The course objects that matter run on per-object timers that start at `GeographyObjManager::reset()`. A shared race clock (start frame plus a frame counter) is enough for the timer-driven ones. Kart-reactive ones (Piranha, Chain Chomp, Cataquack, Geyser in proximity mode, Dino feet) are simulated locally per peer against the local kart only. Traffic cars are timer/path based and stay in step as long as no item hits them. |
| Race flow | The host owns `GO` time, lap and finish decisions (each kart reports its own lap/finish; the host validates and orders them), the final ranking and results. Live ranking for the HUD comes from each peer's own `KartChecker` progression for all karts, so remote karts get `mRaceProgression` from their reconstructed position, corrected by a host `RANK` message at 2 Hz. |
| Session | A custom session start, not the menus. The host sends a `RACE_SETUP` (course, mode, laps, CC, mirror, seed, and per-slot character/kart/peer). Every peer fills `gRaceInfo` directly, the way `LANEntry::setRaceInfo()` (`src/Osako/LANEntry.cpp:207`) does, and enters `RaceApp`. |
| Clock | Race frames at 60 Hz. NTP-style offset and RTT estimation over the data channel. The host broadcasts `GO` as a race frame, and each peer maps it to its local frame. |
| Wire | Binary. One 56-byte kart state per peer per tick (spike format, §10). That is about 2.6 KB/s per remote kart at 30 Hz with overhead. An 8-player full mesh is about 18 KB/s up and 18 KB/s down per peer. |

---

## 2. How the game is put together (what matters for netcode)

### 2.1 Frame order

`RaceApp::calc` (`src/Osako/RaceApp.cpp:259`) calls these in order:

1. `RaceMgr::calcRace` (`src/Kaneshige/RaceMgr.cpp:1122`)
   - `RaceDirector::calc`
   - `J2DManager::calc` (HUD)
   - `ItemObjMgr::clrKartItemUseTriggerList`
   - `frameWork()` (only when not paused, `isFrameRenewal`)
     - `Course::calcShaking`
     - **`KartCtrl::DynamicsStage1`** (`src/Yamamoto/kartCtrl.cpp:218`)
       - `RivalKart::setRankAry`
       - `KartItem::CheckHitItem`: steals from body contacts in the previous frame
       - `KartStrat::GetBody*` and `KartGame::DoElementForce`
       - `KartGame::DoStatus`: start/goal chapters, `DoContCtl` (pad → `KartPad` copy), hit-list reactions `DoActionMgr`, items `KartItem::DoItem`
       - `DoGForce`, `DoSterr`
       - suspension and tyres: `DoSusAction`/`DoTireAction`
       - ground/object/roof reflections
       - **`DoBodyHit`**: kart-vs-kart
       - **`DoPose`**: integrate velocity and angular velocity into `_110`/`mPos`
       - CPU AI `RivalKart::ActiveCtrl` for `mGameStatus & 8` (it writes the CPU's pad) and `KartTarget::DoSearchTarget`
     - **`KartCtrl::DynamicsStage2`** (`kartCtrl.cpp:337`)
       - `KartStrat::DoAccel`, `DoContPaste`, sound
       - `DoLod`, `HaveRabbit`
       - cameras `KartCam::DoCamera`
       - display matrices `KartDisp::MakeKartDisp`
     - `Course::calc`
     - **`GeographyObjManager::calc`** (`src/Sato/GeographyObjMgr.cpp:2180`): hazards, item boxes, kart-object hits
     - `checkKart()`: `KartChecker` lap/sector per kart
     - BGM
   - `mFrame++` (`getRaceFrame()`)
   - `checkRank()` (`RaceMgr.cpp:792`)
2. `RaceMgr::updateRace` (`RaceMgr.cpp:1195`)
   - draw buffers
   - **`ItemObjMgr::calc`**: item movement, item-vs-kart hit tests, roulette
   - effects

Pads are read before all of this, in `PadMgr::framework` (`src/Osako/PadMgr.cpp:26`). `JUTGamePad::read` feeds the 16 virtual kart pads `gpaKartPad[]` from 2-byte `KartPadData` (`include/Osako/KartPadData.h`: 5-bit stick X, 3-bit stick Y, 8 buttons). Under LAN, the data comes from `NetGameMgr::getKartPad` instead.

**Hook points used by this design** (patch `0020-netcode-remote-kart`, `src/Yamamoto/kartCtrl.cpp`):

- `port_net_pre_dynamics()`: top of `DynamicsStage1`. It writes predicted remote inputs into remote karts' pads after `PadMgr` has run.
- `port_net_post_stage1()`: top of `DynamicsStage2`, after `DoPose` and before `DoAccel`/camera/display. It does three things:
  - corrects remote karts toward their network state
  - samples local karts for sending
  - (later) applies kart-scoped events

`RaceMgr.cpp` already belongs to patch `0013`, so race-level hooks (`GO`, results) belong in an existing patch or in files nobody owns yet: `RaceDirector.cpp`, `KartChecker.cpp`, `ItemObjMgr.cpp`.

### 2.2 Karts, pads and players

**`RaceInfo`** (`include/Kaneshige/RaceInfo.h`, global `gRaceInfo`):

- `setRace(mode, kartCount, playerCount, consoleCount, statusCount)`: console = screen, status = HUD player.
- `setKart(no, kartID, char1, pad1, char2, pad2)`
- `setConsoleTarget(cns, kart, demo)`
- `setStartPosIdx`, `setRandomSeed`, `mVsLapNum`, `mItemSlotType`, `mIsLanMode`

**`KartInfo::KartCharacter::convPlayerKind`** (`src/Kaneshige/KartInfo.cpp:584`) classifies a slot by its pad:

| Pad | Kind |
| --- | --- |
| null | 2, COM |
| `NORMAL` | 1, local human |
| `NETWORK` + `PORT_NET` | **3, NET** |
| `NETWORK` + local port | 1 |
| `RECORD` | 4, ghost |

`KartBody::Init` (`src/Yamamoto/kartBody.cpp:1983-2020`) turns the kind into `mGameStatus` bits:

| Bit | Meaning |
| --- | --- |
| `0x2` | human |
| `0x100` | local human |
| `0x8` | CPU, driven by `RivalKart` |
| `0x800` | net human |
| `0x10` | ghost, no collisions |
| `0x1` | tag mode, two humans on one kart (driver + co-driver pads) |
| `0x2000` | active ("indication") |
| `0x8028` | set at goal: the kart becomes CPU-driven after finishing (`kartChapter.cpp:296`) |

CPU karts get a private dummy `KartGamePad` (`KartChecker::createGamePad`, `src/Kaneshige/KartChecker.cpp:248`). `RivalKart` writes into it: stick X in `RivalBodyCtrl.cpp:825`, buttons in `RivalSpeedCtrl.cpp:421` and `RivalItemCtrl.cpp:623`. That is useful: **every kart is driven through a pad**, so "drive this kart from network input" means "write its pad".

`KartCtrl` (`include/Yamamoto/kartCtrl.h`) owns per-kart arrays:

- `KartBody`
- `KartSus[4]`
- `KartAppendix[2]`
- `KartPad`: per-frame copy of button state
- `KartAnime`, `KartSound`, `KartTarget`, `KartDisp`, `KartCam`
- `RivalKart`

`KartBody` (`include/Yamamoto/kartBody.h`) owns the strategy objects:

| Object | Role |
| --- | --- |
| `KartStrat` | driving |
| `KartGame` | status and chapters |
| `KartItem` | item handling |
| `KartCrash`, `KartTumble`, `KartDamage`, `KartStar`, `KartThunder` | hit reactions |
| `KartRescue` | Lakitu rescue |
| `KartCannon`, `KartPipe`, `KartDossin` | cannon, pipe, Thwomp flatten |
| `KartHandle` | steering wheel and collision force |
| `KartAnt` | anti-? |
| `KartCheck` | predicates |
| `KartPerCam` | camera |

### 2.3 Kart state: synced vs derived

The pose fields that must stay mutually consistent are the ones `RivalBodyCtrl::doRotPose` (`src/Sato/RivalBodyCtrl.cpp:1119`) and `KartBody::DoPose` (`kartBody.cpp:527`) write:

- `_110`: rigid-body matrix, rotation plus position
- `mPos`
- `mPlayerPosMtx`: `_110` with the body offset along up
- `_2f0`, `_2fc`, `_308`: axis copies

`_1a0` is the display matrix that `KartDisp::MakeDispBodyPose/Pos` builds in stage 2. `mPrevPos` and `_254` are previous-frame values.

| Group | Fields | Sync? |
| --- | --- | --- |
| Pose | `mPos`, `_110` rotation | **sent** (position f32×3, smallest-three quaternion 32 bit) |
| Motion | `mVel`, `mEffctVel` (per-frame displacement, used for extrapolation), `mWg` (body-space angular velocity) | **sent** (s16 fixed point) |
| Input | driver pad: stick X/Y, buttons, analog L/R. Tag mode also needs co-driver buttons | **sent**. This is what makes the local re-simulation of the remote kart look right: steering angle, drift, hop, wheel turn, driver lean |
| Status | `mCarStatus` (u64; known bits: `0x40000` star, `0x4000` dash, `0x1000` Chain Chomp, `0x80000`/`0x1000000000` crash/jump families), `mBoostTimer`, `mMTBoost`/`mMTState` (mini-turbo), `mDriftSterr`, `mDriver`/`WhichNowDriver` (driver swap) | **sent** for verification and for snapping on discrete changes. With the hybrid model most are re-derived locally from inputs. Discrete *events* (star, mushroom, thunder shrink, hit reactions, swap) are sent as events so they start exactly once |
| Derived | suspension `KartSus` (`_b0`, `_128`…), tyre spin, `mSpeed`, RPM, sound state, `KartAnime` state, effects (`JPEffectPerformer`, `stEffectMgr`), shadows, ground (`mBodyGround`), `mMyRank` | **derived** by running the normal kart code |
| Race | lap, sector, progression (`KartChecker`) | derived locally from the position, with host-authoritative lap/finish events |

### 2.4 Kart-to-kart contact

`KartCtrl::DoBodyHit` → `KartBody::CheckBodyHit` (`kartBody.cpp:1350`) → `RigidReflection` / `RigidReflectionPower` (`:743`/`:837`) applies symmetric impulses. It also handles:

- star tumble: `_590 |= 0x10` → `MakeStarTumble`
- dash tumble
- thunder-flatten
- item steal (`_5b8[victim]` → `KartItem::CheckHitItem` → `FallStealItem` → `ItemObjMgr::robRivalOfItem`)

Because remote karts are real `KartBody`s in the hybrid model:

- **The local kart is bumped by remote karts using the game's own response, with zero latency.** The remote kart is also pushed locally, but the correction pulls it back to its owner's truth within about 100 ms. Its owner sees the same contact from its side, so the visible result is roughly symmetric and consistent with the original.
- Status effects caused by contact (star tumble, steal) are applied **by the victim** only. Patch `CheckBodyHit` so that a remote kart is never the victim of a locally detected status effect, then send a `CONTACT` hint so the victim can confirm it.

---

## 3. Existing remote-control machinery

### 3.1 Ghosts (Time Trial)

Ghosts are pure input replay:

- Recording: `PadRecorder` (`include/Osako/PadRecorder.h`) records `KartPadData` per frame into a `KartPadRecord` (`GhostFile`).
- Playback: `RaceApp` (`src/Osako/RaceApp.cpp:159-180`) creates `RECORD` pads, kind 4, and fixes the seed `0x12D5ECB`.
- Ghosts have `mGameStatus & 0x10`, so they skip collisions.

This proves the kart physics replays deterministically **in isolation** from 2-byte pads. It also shows why input replay alone is not enough online: the moment a ghost could touch anything, its future would depend on unshared state.

### 3.2 NetGameMgr (the original LAN mode)

`src/Osako/NetGameMgr.cpp` and `include/Osako/NetGameMgr.h` implement lockstep over UDP port `0x6790` between up to 8 consoles:

- SSDP discovery.
- Ring topology (`makeLoop` :198; `sendNext` :53).
- The head originates a pad packet every frame. Each console appends its 4 pads and forwards it (`recvPad` :715, `appendPadData` :807).
- `adjustFrame` (:1307) blocks until all pads for the frame are in.
- Desync detection: the sum of all kart positions is carried in `NetPadPkt::mTime` and checked (:857).
- Sleep/awake around races.

The race side only sees `NETWORK`/`PORT_NET` pads. The item and kart code has **no** network logic. LAN play is 100% "same inputs, same seed, same simulation".

**What we reuse:**

| Item | Use |
| --- | --- |
| `KartGamePad::NETWORK` + `PORT_NET`, kind 3, `0x802` | identity of a remote kart, so HUD colours, names and BGM logic for LAN karts keep working |
| `LANEntry::setRaceInfo()` (`src/Osako/LANEntry.cpp:207-290`) | template for building `RaceInfo` for N human karts with no CPUs (`setRace(mode, n, n, displays, localNum)`) |
| `mIsLanMode` / `mLapNumLAN` and the LAN item-slot tables (`ItemObjMgr::loadSlotData` `setToListLAN`, ItemObjMgr.cpp:2023) | LAN versus rules (no CPUs, laps, item odds) |
| `RaceDirector` LAN ending branches (`RaceDirector.cpp:411-538`) and `LANResult2D` | a multi-console results flow already exists |

**What we do not reuse:** the `NetGameMgr` state machine, SSDP/UPnP and the ring. `PadMgr::msKartPadInput` stays `Input_Local`. If it were set to `Input_Net`, every `gpaKartPad` would become `NETWORK` and would expect lockstep data.

---

## 4. Driving a kart from network state

### Options

**(a) Inputs plus periodic state correction.** Feed the remote's inputs into its pad and let the full simulation run, then periodically snap or blend toward received state.

- Pros:
  - everything looks native: suspension, tyres, steering, drift sparks, driver animation, sounds, effects
  - kart bumps use the game's physics
- Cons:
  - the inputs arrive late, so between corrections the kart diverges, especially in contact or after hits
  - a snap is visible

**(b) Puppet: bypass physics and set transform and animation directly.**

- Pros: exactly follows the remote's path (in the past). Simple.
- Cons:
  - every derived system that the physics feeds goes stale or wrong unless it is re-derived: wheel contact, suspension compression, drift sparks, sounds keyed on `mCarStatus`, driver lean, tyre smoke, splashes, `KartChecker` sectors
  - kart-to-kart bumps against an infinitely heavy, teleporting object feel bad
  - rendering in the past (interpolation buffer) means the remote kart is drawn 100–150 ms behind where it is. For a racer that is directly visible as wrong position ordering and in near misses

**(c) Hybrid (recommended).** Run (a) every frame with predicted inputs (hold the last received input). After `DoPose`, blend pose and velocity toward the **newest state extrapolated to the present** (`pos + mEffctVel × age`), and snap only on large error.

- Discrete events (hit reaction, item use, star, mushroom, swap) are applied from events, not inferred from inputs, so they fire exactly once and at the right moment.
- A smoothing "visual offset" can be layered later if blend steps are visible. The physics state is corrected and the drawn model eases the offset out.
- This is what modern kart racers and car-soccer games without rollback do.

### Why hybrid

1. **Collisions.** Local-vs-remote bumps use `RigidReflection` with real masses on both sides and no latency for the local player. With a puppet, the remote kart has infinite mass and only the local kart reacts.
2. **Wheels and suspension.** `KartSus::DoSusAction`/`DoTireAction` run from the pose each frame. Because we correct the pose before stage 2 (display) and the next stage 1, suspension and wheels follow the corrected body. In the spike, no visual glitching was seen at 15% blend per frame.
3. **Sounds and effects.** `KartSound::DoInfo`, `JPEffectPerformer` and the drift/mini-turbo sparks all key on `mCarStatus` and pad bits. With real inputs they behave natively.
4. **Present-time position.** Racing rank, item targeting (red shells choose by position and rank, `ItemRTurtle::searchTarget`, `src/Sato/ItemGTurtle.cpp:854`) and kart contact all want "where is the kart now", not "where was it 100 ms ago".

**Snap threshold and blend.** The spike uses k = 0.15 per frame and snaps above 1500 units (a kart is about 200 units long). Production should:

- blend position, rotation and velocity separately
- scale k by speed
- always snap on respawn (`KartRescue`), cannon/pipe (`KartCannon`/`KartPipe`) and teleports, using an event flag in the state packet

**Input prediction details.** Hold the last stick and buttons. Generate `mTrigger`/`mRelease` edges only from received changes, not every frame, otherwise a held item button re-fires. Item and hop triggers must come from events, not from the predicted pad: mask the item button bits on remote pads (`KartPad::mItemMask`) and drive item use through §5.

**Remote CPU karts.** CPU karts (if any are allowed online) are simulated by the host and sent like any other kart. Peers treat them as remote karts. Phase 1 has no CPUs online.

---

## 5. Items

### 5.1 Lifecycle in the game

`ItemObjMgr` (`include/Sato/ItemObjMgr.h`, `src/Sato/ItemObjMgr.cpp`) pre-allocates pools per kind (`createAllObj` :328):

| Kind | Item |
| --- | --- |
| 0 | green shell |
| 1 | Bowser shell |
| 2 | red shell |
| 3 | banana |
| 4 | big banana |
| 5 | mushroom |
| 6 | star |
| 7 | Chain Chomp |
| 8 | bomb |
| 9 | fireball |
| 10 | thunder |
| 11 | Yoshi egg |
| 12 | golden mushroom |
| 13 | spiny |
| 14 | heart |
| 15 | fake box |
| 17–21 | triples/succession parents |

Per kart and driver, the manager tracks `mEquipItem[8][2]`, `mStockItem[8][2]` (roulette result), `mShuffMgr[8][2]` (roulette) and `mHitList[8]`.

`ItemObj` states (`src/Sato/ItemObj.cpp`):

| State | Setter |
| --- | --- |
| equip 1 | `setStateEquip` :235 |
| release 2 | `setStateRelease` :292 |
| sp-release 3 | `setStateSpRelease` :320 |
| fall 4 | `setStateFall` :340 |
| divested 5 | `setStateDivested` :390 |
| occur 6 | `setStateOccur` :375 |
| force-disappear 7 | `setStateForceDisappear` :438 |
| disappear 8 | `setStateDisappear` :420 |
| dead 10 | returned to the pool in `calc` :1278 |

The use path is `KartItem::DoItem` (`src/Yamamoto/KartItemStrat.cpp:815`):

- item button trigger, a 5-frame throw animation, then `DoTurtleReleaseDir`/`DoBananaReleaseDir`
- **stick Y picks direction**: forward vs back for shells, throw vs drop for bananas
- self-use items (mushroom, star, thunder, golden mushroom, Chomp) go straight to `KartGame::MakeMashDash`, `KartStar::MakeStar`, `KartThunder::MakeThunder` and so on

Throw velocity comes from the owner's pose and velocity (`ItemObj::initThrowVel` :1978).

### 5.2 Hits in the game

`ItemObjMgr::calc` (:1159-1200) tests every active kart against every item with `ItemObj::IsHitKart` (a sphere test, virtual). On a hit it calls `setKartHittingList` (:1324), which writes `mHitList[kart][reaction] = item`, and the item disappears.

On the next stage 1, `KartGame::DoActionMgr` (`src/Yamamoto/kartCtrlStrat.cpp:1361`) applies the reaction by priority:

| Reaction | Cause | Handler |
| --- | --- | --- |
| 9 | Chomp | `KartTumble::MakeWanWanTumble` |
| 4/3 | shell, fake box | `KartCrash::MakeKameCrash` |
| 8 | bomb, spiny | `MakeBombCrash` |
| 2 | big banana, blast | `MakeSpin` |
| 0 | banana | `MakeHalfSpin` |
| 11 | Bowser shell | `MakeKameTumble` |
| 10 | fireball | `MakeBurn` |
| 12 | thunder | |
| 7 | star | |
| 6 | mushroom | |

`DoActionMgr` is skipped while starred (`0x40000`) or invincible (`KartCrash::CheckMatchlessTimer`). Hits also drop held items (`KartItem::FallItem`/`FallAllItem`), which consumes item RNG.

### 5.3 Authority rules

| Thing | Authority | Mechanism |
| --- | --- | --- |
| Using an item | owner | Owner runs `DoItem` normally. When `setStateRelease`/`SpRelease`/self-use fires, it sends `ITEM_USE{owner, driver, kind, netId, frame, pos, vel, dir/stickY, target}`. Peers spawn with `equipItemToKart(kind, owner, drv)` → set `KartPad::mItemStickY` → `setStateRelease/SpRelease` → overwrite `mPos`/`mVel` from the event, advanced by the event age. For triples, use `setChildState*`. For self-use, call `MakeStar`, `MakeMashDash`, `MakeThunder`… on the remote kart |
| Held items (trailing shells and bananas, triples orbiting) | owner | Visible on peers because `ITEM_EQUIP{owner, driver, kind, netId}` puts the same kind in the remote kart's `mEquipItem`. Its position is derived from the remote kart pose by the game's own equip code |
| Projectile flight | owner | Every peer simulates the projectile locally from the spawn event. Green shells bounce deterministically off walls (`colItemReflect`), so drift is small. Red shells and spinies **home**, and the target choice depends on rank and positions, so the owner includes `target` in `ITEM_USE` and peers force it (`mTargetNum`). The owner sends `ITEM_STATE{netId, pos, vel}` at 10 Hz for homing items and bombs, blended like karts |
| Hit on kart K | **K's owner (victim)** | Only the victim's machine runs `IsHitKart` for its own local kart, against its local copy of the item. On a hit it applies the reaction immediately, then sends `ITEM_HIT{netId, victim, reaction, frame}`. Peers apply the reaction to the remote kart (inject `mHitList`, or call `MakeSpin`/`MakeKameCrash` directly; see the null-item caveats in §5.5) and destroy the item. Hit tests against **remote** karts are disabled locally (patch `ItemObjMgr::calc` to skip non-local karts, or override `IsHitKart`) |
| Item vs item | owner of the older item | `ITEM_DESTROY{netId, by}`. Initially, let every peer resolve item-vs-item locally and accept small inconsistencies: whoever sees a collision destroys both locally and sends `ITEM_DESTROY`, and the first destroy wins |
| Bananas and fakes on the track | dropper | They are static, so spawn and destroy events are enough |
| Item boxes | **host** | Each peer runs `GeoItemBox::doKartColCallBack` (`src/Sato/GeoItemBox.cpp:389`) for its own kart only. Locally the box breaks and the roulette starts, then `BOX_TAKE{boxId, kart}` goes out. The host confirms, or rejects if the box is already taken. On reject, the result is cancelled (`abortItemShuffle`). Box respawn time comes from the host. Remote karts hitting boxes locally break the box visually but start no roulette |
| Roulette result | **host** | The local roulette (`startItemShuffleSingle/Double` → `ItemShuffleMgr::doShuffle`, 130 frames, early stop on button) is presentation only. `SlotItem` (`ItemObjMgr.cpp:2203`) is patched to wait for or consume `ITEM_GRANT{kart, driver, kind}`. The host computes the item with the game's own `calcRank`/`calcSlot` against **its** rank table, using the kart's rank as the host sees it. This keeps odds fair and makes item-RNG divergence harmless |
| Thunder | user | `ITEM_USE{kind=10}`. Every peer shrinks its **own** kart (victim rule) and the remote ones as presentation |
| Steals (star or mushroom contact, `robRivalOfItem`) | victim | `ITEM_STEAL{from, to, kind}` |

**Item IDs.** `netId = owner(3 bits) << 13 | counter`. Peers keep a `netId → ItemObj*` map. Pools are finite (`sMaxItemNum`, the global moving cap of 16) and `deleteItemKeepMax` evicts old items, so evictions are also broadcast (`ITEM_DESTROY`) by the owner.

**Lag compensation.**

- A spawned projectile is fast-forwarded by the event's age, with a cap of 6 frames.
- The victim-authority rule means a player is never hit by something they did not see. The cost is that a shooter sometimes sees a near-miss register as a miss. This is the standard trade for racers.
- To reduce "I clearly dodged that" moments, victims accept a hit only if the item is within the hit sphere **on their own screen**. That is free with victim authority.

### 5.4 RNG

There are six `stRandom` streams (`include/Sato/stMath.h:81`), all seeded from `RaceInfo::mRandomSeed` in `RaceMgr::setRandomSeed` (:396):

| Stream | Used by |
| --- | --- |
| 0 | items: roulette, fall directions, egg contents, angles |
| 1 | course objects: `getGeoRnd`, item-box single/double |
| 2 | CPU AI |
| 3 | particles |
| 4 | HUD roulette |
| 5 | blink |

Stream 0 is consumed by every item in play, so it diverges immediately in this design. That is acceptable because:

- roulette results are host-granted
- item-internal randomness (banana spin direction, egg contents, Bowser shell wobble) is either cosmetic or carried in the event: egg contents and fall direction go in `ITEM_DESTROY`/`ITEM_USE`

Stream 1 must stay **in step** for course objects that use it. Share the seed (`RACE_SETUP.seed`) and keep stream-1 consumers identical. Kart-reactive objects (§7) are the exception and break this. Mitigation: give each kart-reactive object its own RNG (a patch to `getGeoRnd` for those classes) or accept divergence of cosmetic randomness.

### 5.5 Caveats found in the code

`KartCrash::DoPointBomb`, `MakeWanWanTumble`, `MakeKameTumble` and `MakeBurn` read `mHitList->mObjects[n]` directly. To apply a remote hit, either:

- inject a real (local copy) `ItemObj` into the hit list, or
- spawn a dummy of the right kind.

Calling the `Make*` functions with null crashes in Bomb Battle and for those three reactions.

---

## 6. Race flow

### 6.1 Start

The intro demo and the Lakitu countdown are camera/state-machine driven:

- `KartDemoCam` sets `KartCtrl::mBitfield |= 2` (`kartCtrlInfo.cpp:541`).
- `TJugem` (`src/Shiraiwa/JugemStart.cpp`) and `TJugemRodSignal` (`JugemRodSignal.cpp:120-150`, 60-frame lights) run the countdown.
- `RaceMgr::isJugemCountStart` and `isAbleStart` (`RaceMgr.cpp:1342`) gate the karts.
- `KartChecker::incTime` (`KartChecker.cpp:747`) runs only while `isAbleStart`.
- The intro can be skipped with A (`KartGame::DoChapterOne`, `kartChapter.cpp:65`).

Plan:

- Every peer loads the race and reports `READY` (course loaded, first frame).
- The host picks `T_go = hostFrame + lead`, about 4 s, and broadcasts it.
- Each peer converts it to local frames with the clock offset.
- A patch in `JugemStart`/`JugemRodSignal` (or `RaceDirector::doRunning`) holds the countdown start so that `isAbleStart` flips exactly at `T_go`. Intro skip is disabled online (or host-only).
- Rocket starts stay local, from inputs.

### 6.2 Laps, ranking, finish

- Every peer runs `KartChecker` for **all** karts. Remote karts progress from their reconstructed position, so the local HUD rank (`RaceMgr::checkRank` → `checkRankForRace` :1043, by `getTotalUnitDist`) is live and smooth.
- Lap and finish of kart K are **K-authoritative events**: `LAP{kart, lap, raceTimeFrames}` and `FINISH{kart, time}`. Each is emitted when K's own `KartChecker::checkLap` (:493) increments or sets goal. The host checks plausibility (sector pass bits) and rebroadcasts the official order.
- On peers, remote karts' `KartChecker` lap and goal are **overwritten** from events, not computed. This avoids a remote kart "finishing" locally from a corrected position glitch. Patch `KartChecker::checkLap` to skip goal/lap changes for net karts and add a setter.
- Final ranking is host-authoritative: `RESULTS{order[], times[]}`. `RaceMgr::setRaceResult` (:1109) runs with the host's order (write `KartChecker` ranks and goal times before it).
- End condition: `RaceDirector::checkRaceEndForRace` (:692) already ends when all humans finish, or when N-1 have finished if all karts are human. Keep that, plus a host timeout (for example 30 s after the first finisher), then force-goal the rest (`setForceGoal`, `KartChecker.cpp:579`).
- After goal the game turns a kart into CPU (`mGameStatus |= 0x8028`, `kartChapter.cpp:296`). For remote karts, keep them as net karts (their owner's CPU drives them and keeps sending state). Patch the chapter code to skip `|8` for kind-3 karts.

### 6.3 HUD

`Race2D` reads ranks from `KartChecker::getRank` for `J2DManager::getStatus2Kart(status)`. With 1 local player there is one status. The rank list, minimap icons and names of remote players come from the same per-kart data (the LAN mode already shows coloured player markers via `KartChecker::setPlayerKartColor` :218). No new HUD is needed for phase 1. A player-name overlay is optional DOM over the canvas.

### 6.4 Disconnects

| Who | Handling |
| --- | --- |
| Remote peer | Its kart keeps extrapolating for 0.5 s, then goes "ghost": set `mGameStatus |= 0x10` (no collisions), fade or hide via `KartDrawer::hide` (as `kartBody.cpp:2003` does for unavailable ghosts), and freeze it in the rank list as DNF. Its items in flight persist until they expire. Pending `BOX_TAKE`s are released by the host |
| Host | The lobby elects the next host (lowest peer id). The new host takes over boxes, grants and results. State is reconstructible because every peer has every kart's laps |
| Local peer | A local warning, with an option to continue offline vs CPU |

---

## 7. Course objects and hazards

Objects update in `GeographyObjManager::calc` (`src/Sato/GeographyObjMgr.cpp:2180`) after the karts. They do **not** use `RaceMgr::mFrame`. Each has a `StateObserver` counter or private timer starting at `GeographyObjManager::reset()`, which runs during the intro demo too. So "same frame count since reset" means "same phase".

Online, every peer starts the race at `T_go` via the shared clock. The countdown is fixed-length and the intro is driven by the host. Timer-driven objects are therefore in step as long as peers run the same number of `frameWork()` calls between reset and `T_go`. The port runs fixed 60 Hz frames. Under frame drops, catch up with extra `frameWork` calls, or re-phase object timers at `GO` (patch: a per-object `mFrame` offset set at `GO`).

| Object | Code | Driver | Online handling |
| --- | --- | --- | --- |
| Traffic cars, bus, truck (Mushroom City/Bridge), `GeoCar` family | `src/Kaneshige/GeoCar.cpp` | path physics + supervisor spacing, `getGeoRnd` on damage | in step via clock. Item and star hits on cars are local-only reactions: accept, or send `OBJ_HIT{objId}` |
| Bomb cars | `GeoBombCar.cpp:104` | path; explode on kart contact | the victim reports the explosion as an `OBJ_HIT`. Peers explode the car and run blast reactions only on their own kart |
| Thwomp (`TMapObjDossun`) | `src/Shiraiwa/MapObjDossun.cpp` | pure timer | clock only |
| Fire bars, fire circles, Bowser statues, DK rock generator | `MapObjFireBar.cpp`, `MapObjKpFire.cpp`, `MapObjDonkyRockGen.cpp` | timer/path | clock only |
| Piranha plant (`TMapObjPakkun`) | `MapObjPakkun.cpp:270` | **nearest kart** in a radius | local-only: each peer runs it against *its* kart positions. Different peers may see it bite different karts. Damage applies only to the local kart (victim rule). Accept the cosmetic divergence |
| Chain Chomp (`TMapObjWanwan`) | `MapObjWanwan.cpp:382` | nearest kart, plus `getGeoRnd` wander | same as Piranha. Its random wander makes it diverge. Phase 4: host-owned with 10 Hz state |
| Dino (`TMapObjNossie`) | `MapObjNossie.cpp:158` | animation; reverses on kart contact | local-only reaction |
| Geyser (`TMapObjGeyser`) | `MapObjGeyser.cpp:369` | timer, or proximity (param8) | timer: clock. Proximity: local |
| Cataquack (`TMapObjPoihana`) | `MapObjPoihana.cpp:343` | nearest-kart chase | local-only |
| Pokey (`TMapObjSanbo`) | `MapObjSanbo.cpp` | timer + `getGeoRnd` | clock + seed; may diverge after stream-1 drift |
| Shy Guys, Goombas | `MapObjHeyho.cpp`, `GeoKuribo.cpp` | path; react to hits | local reaction |
| Meteors (Rainbow Road) | `TMapObjMeteor` | `getGeoRnd` position | cosmetic |
| Item boxes | `GeoItemBox.cpp` | | host-arbitrated (§5) |

`ObjUtility::chkAvailableEnemyByDistande` (`src/Sato/ObjUtility.cpp:501`) makes object collisions depend on camera karts, but only when CPUs exist. Online phase 1 has no CPUs, so it is off.

**Rule of thumb:** objects only ever hurt the local kart on the local machine. Divergent kart-reactive objects are a visual inconsistency, never a gameplay inconsistency for the person affected.

---

## 8. Game modes and session flow

**Scope first:** VS race, 2–8 humans, one kart per browser (no tag-mode co-driver), chosen course, 3 laps, items on, no CPUs. Then 100/150cc, mirror, and Balloon Battle (balloon state = hit events, so it fits the same model). GP, CPUs online, and local split-screen plus online come later.

**Session start (custom, no menus):**

1. Players meet in the lobby (existing signalling and WebRTC work). Each chooses a character pair and kart in a lightweight lobby UI (DOM), or reuses the game's menu once offline.
2. The host sends `RACE_SETUP{course, mode=VS, cc, mirror, laps, seed, itemSlotType, slots[8]{peer, kartID, char1, char2, startPos}}`.
3. Each peer, from the title scene (via a port hook like `MKDD_COURSE`'s `port_course_override` path), fills `gRaceInfo` the way `LANEntry::setRaceInfo()` (`src/Osako/LANEntry.cpp:207`) does:
   - `setRace(VS, n, n, 1 console, 1 status)`
   - for each slot, `setKart(i, kart, c1, pad, c2, nullptr)`
     - the **local** slot gets `gpaKartPad[0]` (`NORMAL`, port 1)
     - each **remote** slot gets a fresh `KartGamePad(PORT_NET, NETWORK)`, which is kind 3
   - `setConsoleTarget(0, localSlot)`, `setStartPosIdx`, `setRandomSeed(seed)`, `mVsLapNum`
   - then `ResMgr::loadCourseData`, `RaceApp::call()`, as `SceneCourseSelect::nextRace()` (`src/Kameda/SceneCourseSelect.cpp:715`) does
   - **do not** set `mIsLanMode` unless we want the LAN rules and results. The LAN ending code calls into `NetGameMgr` (sleep/awake) and needs guarding
4. After results, the host sends the next `RACE_SETUP` or `END`. Peers return to the lobby or title.

Phase 1 can start even simpler: an env/URL parameter `?online=1&slot=N` plus the host's `RACE_SETUP` over the data channel.

---

## 9. Clock sync and buffering

- **Clock.** Race frame `F` at 60 Hz since `GO`, plus a pre-start lobby clock in ms.
  - Peers send `PING{t_local}`; the reply carries `t_host`.
  - Offset = median of the last 16 samples of `t_host + rtt/2 − t_local`.
  - Converted to frames for `T_go`.
  - Every state packet carries the sender's race frame (u16, unwrapped against the local frame, as the spike does).
- **Remote-kart target.** The hybrid model renders remote karts at present time. Each new packet (frame `f`, received at local frame `n`, with `n − f` ≈ one-way latency) is extrapolated `n − f` frames (capped at 15) with `mEffctVel`.
- **Jitter and reordering.** Keep a 64-entry receive buffer sorted by frame and drop duplicates and stale packets (the spike does this). The newest packet always wins.
- **Mode-0 interpolation buffer** (if ever wanted, for example for spectators): render at `n − latency − 2·sendInterval` and interpolate between neighbours. Measured in the spike to cost several kart-lengths of position error at speed (§12).
- **Pause.** No pause online. The pause menu is suppressed (patch `RaceDirector::checkPauseChoice`, :543) or local-only (game keeps running).

---

## 10. Message protocol

Transport: the separately built WebRTC data channels.

- **unreliable/unordered** (`ordered:false, maxRetransmits:0`): state streams
- **reliable/ordered**: events and control

Little-endian. Every message starts with `u8 type`.

### 10.1 Kart state (unreliable, 30 Hz, 56 bytes; implemented in `tools/web_wasm/net/netkart.cpp`)

| Offset | Type | Field |
| --- | --- | --- |
| 0 | u8 | type = 1 |
| 1 | u8 | kart slot |
| 2 | u16 | sender race frame (low 16 bits) |
| 4 | f32×3 | position `mPos` |
| 16 | u32 | orientation of `_110`, smallest-three quaternion (2-bit index + 3×10 bit) |
| 20 | s16×3 | `mVel` × 64 |
| 26 | s16×3 | `mEffctVel` × 64 |
| 32 | s16×3 | `mWg` × 4096 |
| 38 | s8×2 | stick X, Y |
| 40 | u16 | buttons (JUT bits) |
| 42 | u8×2 | analog L, R |
| 44 | u32 + u16 | `mCarStatus` (48 bits) |
| 50 | u16 | `mBoostTimer` |
| 52 | u16 | `mMTBoost` |
| 54 | u8 | `mMTState` |
| 55 | u8 | `mDriver` (driver/co-driver; bit 7 reserved for "teleport, snap") |

Notes:

- Tag mode adds 4 bytes (co-driver stick and buttons).
- A packet may bundle the last 2 states (112 bytes) to ride out single losses. That was not needed in the spike at 5% loss.

### 10.2 Events (reliable)

| Type | Message | Payload |
| --- | --- | --- |
| 0x10 | `ITEM_EQUIP` | slot, drv, kind, netId |
| 0x11 | `ITEM_USE` | slot, drv, kind, netId, frame, dir(u8), stickY(s8), pos f32×3, vel s16×3, target(u8), seed(u16) |
| 0x12 | `ITEM_STATE` | netId, frame, pos f32×3, vel s16×3. Homing items, 10 Hz, unreliable |
| 0x13 | `ITEM_HIT` | netId, victim, reaction, frame |
| 0x14 | `ITEM_DESTROY` | netId, reason, frame |
| 0x15 | `ITEM_STEAL` | from, to, kind |
| 0x18 | `BOX_TAKE` | boxId(u16), slot, frame |
| 0x19 | `BOX_RESULT` | boxId, slot, ok, respawnFrame. Host |
| 0x1A | `ITEM_GRANT` | slot, drv, kind. Host |
| 0x20 | `KART_EVENT` | slot, frame, code (star, mushroom, golden, thunder-shrink, swap, rescue-start, rescue-end, cannon, pipe, spin…), arg |
| 0x21 | `CONTACT` | a, b, frame, effect: star-tumble/steal hint for the victim |
| 0x22 | `OBJ_HIT` | objId, slot, frame: bomb car and so on |
| 0x30 | `LAP` | slot, lap, frames |
| 0x31 | `FINISH` | slot, frames |
| 0x32 | `RANK` | order[8]. Host, 2 Hz, unreliable |
| 0x33 | `RESULTS` | order[8], times[8]. Host |
| 0x40 | `RACE_SETUP` | see §8 |
| 0x41 | `READY` | |
| 0x42 | `GO` | hostFrame |
| 0x43 | `PING` / `PONG` | |
| 0x44 | `LEAVE` | |

### 10.3 Bandwidth (8 players, full mesh)

Per remote kart:

- State: 56 B + about 40 B DTLS/SCTP/UDP/IP overhead ≈ 96 B × 30 Hz ≈ **2.9 KB/s**.
- Events: under 0.5 KB/s average, with short bursts.

Per peer, for 7 others: about **20 KB/s up and 20 KB/s down** (160 kbit/s). At 60 Hz, about 40 KB/s. This is fine for home connections.

A star topology through the host halves upload for non-hosts, but adds latency and host upload of about 140 KB/s. **Mesh is preferred.**

---

## 11. Trust model and security

The model is friends-only rooms (lobby codes) with **no anti-cheat**. Each peer is authoritative for its own kart, so a modified client can teleport, refuse hits or always grant itself stars.

The host does cheap plausibility checks that also catch bugs:

- speed cap
- lap only after all sector-pass bits
- `ITEM_USE` only of a granted kind

The host's grants and box arbitration stop the easiest cheat (item choice). Nothing is trusted for anything outside the game: messages are binary with fixed sizes, and lengths and slot indices are validated before indexing the game's arrays.

The port **never** sends game data (disc assets): only state and events.

---

## 12. Spike (phase 2) results

### What was built

- **Patch `0020-netcode-remote-kart`** (`src/Yamamoto/kartCtrl.cpp`) adds two calls: `port_net_pre_dynamics()` at the top of `DynamicsStage1` and `port_net_post_stage1()` at the top of `DynamicsStage2`.
- **`tools/web_wasm/net/netkart.cpp`** and **`netkart_io.c`** (node file I/O) contain:
  - capture of a kart's state into the 56-byte wire format (§10.1) and decode back. Remote state only ever passes through the quantised wire format.
  - a simulated link: latency, jitter, loss and send interval, using its own LCG so the game's `stRandom` streams are never touched.
  - a sorted receive buffer.
  - three remote-kart models, selected with `MKDD_NET_MODE`:
    - `0`: interpolate 10 frames in the past and snap (puppet)
    - `1`: hybrid
    - `2`: inputs only
  - the remote kart is marked as a net kart: CPU bit `8` cleared, `0x802` set
  - the remote pad gets the newest input with item buttons masked
  - an error metric against unquantised ground truth
- **`tools/web_wasm/tools/build.py`** compiles `tools/web_wasm/net/*.c` and `*.cpp`.

Sources of the "network" stream:

| `MKDD_NET` | Meaning |
| --- | --- |
| `rec` | write every kart's wire state each frame |
| `play` | kart `MKDD_NET_KART` replays its own recording from a previous run |
| `mirror` | kart `MKDD_NET_KART` follows live kart `MKDD_NET_SRC`, delayed by `MKDD_NET_LAG` frames, as if that kart were a remote player |

Run with `MKDD_NET_KART=0`: the camera kart is the remote one, so the frame dumps show the reconstructed kart up close.

```sh
python tools/web_wasm/tools/build.py --target node --tag nc
COMMON="MKDD_DISC=... MKDD_FAST_MOVIE=1 MKDD_CARD=none MKDD_PAD=<1P GP script> MKDD_EXIT_AFTER=150"
# record (P1 on autopilot, or scripted human input)
env $COMMON MKDD_AUTOPILOT=1 MKDD_NET=rec MKDD_NET_FILE=/tmp/k.bin node build/web_wasm/node-nc/mkdd.js
# replay kart 0 from the recording over a 100 ms / ±17 ms / 5% loss / 30 Hz link
env $COMMON MKDD_NET=play MKDD_NET_FILE=/tmp/k.bin MKDD_NET_KART=0 MKDD_NET_MODE=1 \
    MKDD_NET_LAT=6 MKDD_NET_JITTER=2 MKDD_NET_LOSS=5 MKDD_NET_RATE=2 node build/web_wasm/node-nc/mkdd.js
# live mirror: kart 0 is a remote copy of CPU kart 1, 45 frames behind it
env $COMMON MKDD_NET=mirror MKDD_NET_KART=0 MKDD_NET_SRC=1 MKDD_NET_LAG=45 node build/web_wasm/node-nc/mkdd.js
```

Every 2 s the log prints `[net] frame F: err avg/max` (world units; a kart is about 200 units long) against the true state of the remote at the same frame.

### Measurements

The test case is the first race of the Mushroom Cup GP, Luigi Circuit. The link is 6 frames latency (100 ms), 0–2 frames jitter, 5% loss, 30 Hz.

| Stream | Model | Position error vs truth | Notes |
| --- | --- | --- | --- |
| scripted human input (hold A, weave) | 1 hybrid | **avg 1–50, max 150**, mostly under 15 | Remote kart's own physics on held input does most of the work; correction trims |
| same | 2 inputs only | 340 → **3 200** within 20 s, never recovers | Different world, same inputs: the run's CPUs and timing differ from the recording run (async disc loading shifts things by frames), so collisions and walls differ and the replica wanders off |
| same | 0 interpolation (10 frames in the past) | 130–530 at speed (≈ 10 frames × speed) | Exactly the expected "drawn in the past" lag |
| CPU kart (autopilot P1) | 1 hybrid | about 130 steady at 31 u/frame | CPU karts get speed from `RivalSpeedCtrl`, not only from the pad, so held pad input under-drives the replica and the blend carries a steady lag of about 4 frames. Remote CPU karts would need a stronger velocity blend (or send target speed) |
| same | 2 inputs only | grows to **30 000+** (lost on the course) | |
| same | 0 interpolation | 312 at 31 u/frame (= 10 frames) | |
| live mirror of CPU kart 1 | 1 hybrid | avg 30–150, peaks 330 during bumps | Remote kart is pushed by other karts and pulled back |

**Conclusion.** Input replay alone is useless online, even with the same inputs. Pure interpolation is stable but always one interpolation delay behind, about 2–5 kart lengths at speed. The hybrid keeps a human-driven remote kart within a few units most of the time, at present time, while all wheels, suspension, driver animation, drift smoke and sounds come from the game itself.

### Frames

Frames are kept in the scratch directory and not committed, because they are game imagery:

| Image | Shows |
| --- | --- |
| `.../netcode/M1_sheet.png` | live mirror: 8 frames, 2.3 s apart, of camera kart 0 being a remote replica of CPU kart 1. The replica drives the course, leans, the back driver turns around holding an item (native `KartAnime::DoBackAnime` triggered by the remote's stick Y), and the HUD, minimap and rank stay consistent |
| `.../netcode/H1_sheet.png` | hybrid replay of a human input stream |
| `.../netcode/A_mirror_m1/frame_005500.png` | star effect on the replica |

No visual popping was visible at k = 0.15. One snap happened per run, at activation.

**Found while doing the spike** (all handled in the design):

- **Remote karts pick up item boxes locally.** The replica got a mushroom from a box it hit locally. → host-arbitrated boxes, and no roulette for remote karts (§5.3).
- **CPU pads include item and aim inputs.** Item buttons are now masked on remote pads.
- **Race frame resets.** `RaceMgr::mFrame` restarts between the course demo and the race. The session must key on the race-start (`GO`) frame, not on raw `getRaceFrame()`.
- **Load timing.** Two runs with identical scripted input do not reach the race at identical frames (async loading). This is another reason lockstep needs explicit synchronisation and this design needs `GO` alignment.
- **`port_env()` maps `"0"` to 1** (it is a flag reader). Numeric settings use their own parser.

---

## 13. Phased implementation plan

Estimates are for one experienced engineer familiar with this repository.

| Phase | Content | Estimate |
| --- | --- | --- |
| 0 (done) | Spike: hooks patch `0020`, wire format, simulated link, remote-kart correction, record/replay harness | — |
| 1 | **Two-browser kart sync.** Integrate the WebRTC transport (`web/net.js`): JS↔wasm message queue (`ccall` push and pull ring buffers). `RACE_SETUP` → custom `RaceInfo` with `PORT_NET` pads, no CPUs, one console. Clock sync, `GO` alignment patch in the countdown, state send and receive, and hybrid correction (spike code generalised to N remote karts). Mask remote item buttons. Disable item use online (`itemSlotType` none) for this phase | 1.5–2 weeks |
| 2 | **Race flow.** `LAP`/`FINISH`/`RESULTS`, host results, `KartChecker` overrides for net karts, goal-to-CPU suppression, disconnect handling, end timeout, lobby return. Rank and minimap names | 1 week |
| 3a | **Items, simple.** Local roulette with a local result announced via `ITEM_USE`. Owner-simulated projectiles, victim-authoritative hits, item equip and destroy, self-use events, thunder, steals | 2 weeks |
| 3b | **Items, host-arbitrated.** `BOX_TAKE`/`BOX_RESULT`, `ITEM_GRANT` patch in `SlotItem`, homing `ITEM_STATE` | 1 week |
| 4 | **Hazards.** Stream-1 isolation for kart-reactive objects, `OBJ_HIT` for bomb cars, timer re-phase at `GO`, host-owned Chain Chomp | 1 week |
| 5 | **Polish.** Visual-offset smoothing, adaptive blend, packet bundling, jitter stats HUD, 60 Hz LAN option, Balloon Battle, mirror/CC, tag mode (two players on one kart across or within browsers) | 1–2 weeks |
| 6 | Local split-screen + online (2 local karts per browser: `consoleNumber` 2, two `NORMAL` pads), CPUs hosted by the host | 1–2 weeks |

**MVP (phases 1–3a): about 5 weeks. Full feature set: about 9–10 weeks.**

### Test strategy

- **Single-process harness (exists).**
  - `MKDD_NET=rec|play|mirror` with simulated latency, jitter and loss (`MKDD_NET_LAT/JITTER/LOSS/RATE`).
  - It reports position error against ground truth every 2 s.
  - Use it in CI-style headless node runs for regression of the correction model (error budgets per scenario).
- **Two headless Chromes on one machine.**
  - Extend `tools/web_wasm/web/smoke.mjs` (CDP) to launch two pages joined to a local lobby.
  - Scripted pads (`MKDD_PAD`) per page, or autopilot.
  - Artificial latency and loss with a shim in `web/net.js` (delay queue, drop %) and/or `tc netem` on loopback for the real-UDP path.
  - Capture frames from both pages at the same race frame and compare kart positions (exported via a debug hook) with a tolerance.
- **Event fuzz.** Random reorder, duplicate and loss on the reliable path, to exercise idempotency (`netId` maps, double hits).
- **Determinism checks.** Timer-driven objects: log object phases at `GO + k` on both peers and diff them.
- **Soak.** 8 pages (4 per Chrome instance), 10-minute races, watch memory (the item pools) and desync counters.

---

## 14. Risks and open questions

1. **Correction vs physics feel.** Blending the pose of a body that is also in contact can produce jitter in pile-ups. Mitigations: blend velocity rather than position while in contact (`mCarStatus & 0x10` contact flag), or use a render-only visual offset. Tune in phase 5.
2. **Unnamed state.** Many `KartBody` fields are still `_xxx`. Some hidden state (drift timers `_5b5/_5b6`, chapter state, `KartStrat` internals) only re-derives from inputs approximately. Events (§10.2 `KART_EVENT`) cover the gameplay-relevant transitions, but each new desync symptom will need a field identified.
3. **Item edge cases.** Succession items (triples orbiting), Chain Chomp pulling its owner (`mCarStatus 0x1000` and its pad clearing in `DoContCtl`), Bowser shell, fake boxes and heart (`executeColHitHeart` steals) each need specific handling. The null-item reaction caveat is in §5.5.
4. **Countdown and intro alignment** need a patch in Shiraiwa's Jugem code. That code is not yet looked at in detail.
5. **Stream-1 RNG drift** affects course-object randomness (Pokey, meteors, Chomp wander, and the single/double item-box choice). The box choice should become host-decided too (in `BOX_RESULT`).
6. **Frame pacing in the browser.** The game's sim is tied to presented frames. A tab in the background throttles `requestAnimationFrame`, so its kart stops being sent. The owner should then be shown as "paused/lagging" and the kart extrapolated, then frozen.
7. **Interaction with lockstep netplay.** The two modes share transport and lobby but not game hooks. Both may want `RaceInfo` setup code, which can be shared.
8. **Open question for the user:** is victim authority acceptable for item hits (shooter occasionally sees a hit not register), or should hits favour the shooter (requires rewinding the victim's history, which we do not have)?
9. **Open question:** allow CPU karts online in VS (host simulates them and sends them like players, about +20 KB/s for 4 CPUs), or humans only at first?
10. **Open question:** phase-1 session UX. Is the custom lobby UI plus `RACE_SETUP` enough, or should the in-game character/kart menus be used before going online?

---

## 15. Phase 1 (implemented)

Code: `tools/web_wasm/net/netonline.cpp` (+ `netonline_js.c` for page exports), the
shared wire format and correction helpers in `netkart.cpp/.h`, patch
`0021-netcode-session` (`LogoApp.cpp`, `JugemStart.cpp`, `PauseManager.cpp`) next to
`0020-netcode-remote-kart`. How to play: `README.md`.

| Piece | As built |
| --- | --- |
| Session | No `RACE_SETUP` message: the lobby's start event already carries everything, so every peer reads `port_net_session_json()` (host settings `course/cc/laps/mirror`, members with slot and `profile {char1, char2, kart}`). Karts are the members with a slot, in slot order (= grid position). Seed = FNV-1a of the room code. `gRaceInfo`: `setRace(VERSUS_RACE, n, n, 1, 1)`, level from cc (mirror = `LVL_MIRROR`), `mVsLapNum`, `setItemSlotType(1)` (VS "no items": no item boxes), local kart `gpaKartPad[0]`, others `KartGamePad(Port_unknown, PORT_NET, NETWORK)` (kind 3), console 0 on the local kart (a member without a slot watches kart 0). `mIsLanMode` stays off |
| Boot | With `MKDD_NET_ONLINE=1` `LogoApp` skips the logo waits and, instead of the opening movie, calls `port_net_boot_race()`: set up `gRaceInfo`, `ResMgr::loadCourseData`, wait for the course and the keep archives (MRAM, MRAMLoc, Race2D, **ARAM**: entering the race before ARAM.arc is mounted crashes in `readAnmResTask`), then `RaceApp::call()`. The course intro demo plays as usual (A skips it) |
| Start | `TJugem::doFunc_StartWaitBlend` holds before the lights while `port_net_countdown_hold()`. Each peer sends `READY 0x41 {attempt, frame time}` to the host every second while waiting; once all live members are there (or after 30 s) the host sends `GO 0x42 {attempt, f64 T_go}` with `T_go = now + max(1.5 s, 1 s + 2 RTT) + 240 × slowest frame time`. A peer starts its 240-frame countdown at `T_go − 240 × its own frame time`, and `doFunc_StartUp` holds `mIsAbleStart` until `T_go` (the state does not leave 0xF before). A late GO is re-sent on the next READY; without any GO a peer starts alone after 45 s. A retry from the results menu is a new attempt |
| Wire | Kart state as §10.1, 30 Hz (every second shared frame, or every game frame below 30 fps), **frame field = shared clock frame** `round(host ms / 16.683)` instead of a race frame: the age of a sample is then known on every peer without a race-frame mapping, also when the sender runs slow. `FINISH 0x31 {slot, u32 ms}` (reliable) when the local kart's `KartChecker` reaches the goal |
| Remote karts | Hybrid (mode 1 of the spike) for up to 7 remote karts: newest input held on the PORT_NET pad (item buttons masked), the game's physics, then pose/velocity pulled toward the newest state extrapolated to now. Extrapolation uses the displacement per shared frame measured from the last two samples (falls back to `mEffctVel`), because `mEffctVel` is per game frame of a sender that may run below 60 fps. Blend 0.15 per 60 Hz frame compounded over the local frame time; snap above 1500 units. No state for 0.5 s: frozen in place (zero velocity, inputs released). Peer LOST: frozen and `mGameStatus |= 0x10` (no collisions) until it is back; LEAVE: also hidden (`KartDrawer::hide`) |
| Collisions | Local vs remote bumps are the game's own `CheckBodyHit` on both machines; the remote kart is pulled back to its owner's state within ~100 ms. No status effects (no items in phase 1) |
| Pause, end | The Start-button pause is off online. Race end and results are local (VS with only human karts ends when all but one have finished, as on the console); remote karts' laps come from their replicated positions. Reload the page to race again |
| Busy waits | `port_poll()` returns to the browser event loop at least every 50 ms during a session (`yield_for_net`, `sdk/os_thread.c`), so a game loop that polls `VIGetRetraceCount`/`OSGetTime` cannot starve the data channels |
| Measurement | Each received state (the sender's truth at shared frame f) is compared with the replica's position at f, interpolated in a local history, once the local game has passed f. Once a second: `[net] stats <name>: err avg/max` in the page log, `Module.netStats`/`Module.netStatsLog` (also used by the overlay) |

### Two-Chrome test (`net/test/e2e-race.mjs`)

Both instances on one loaded 12-core machine with SwiftShader WebGL: **8–9 fps** per
game (the countdown and GO logic are frame-rate aware; the karts themselves are in
slow motion at that rate, which is what the error numbers mostly measure). Luigi
Circuit, 150cc, the in-page bot drives each kart toward the course ahead with
keyboard events; 60 s from GO.

| Network | Replica of | Error avg | p95 of 1 s means | max | States/s | Sample age | GO vs shared time |
| --- | --- | --- | --- | --- | --- | --- | --- |
| direct (loopback) | Bob on Alice's screen | 66 | 267 | 934 | 6.8 | 7.7 frames | +63 ms |
| direct (loopback) | Alice on Bob's screen | 165 | 455 | 963 | 6.5 | 6.3 frames | +47 ms |
| 50 ms ± 10 ms, 5 % loss | Bob on Alice's screen | 145 | 570 | 1147 | 6.5 | 21 frames | +48 ms |
| 50 ms ± 10 ms, 5 % loss | Alice on Bob's screen | 203 | 553 | 975 | 6.3 | 22 frames | +16 ms |

Units are world units (a kart is about 200 long); each row is about 340 samples. The
states/s are below 30 because a game at 9 fps sends once per game frame. Earlier
runs of the same build ranged 66–210 average. The GO frames of both games landed
within 70 ms of the shared time once the start accounts for the frame rate (before
that, a 240-frame countdown at 9 fps made GO 21 s late). Screenshots from both
browsers show the other player's kart where its owner has it, the same ranks on
both screens and the overlay with the live error; they are game imagery and stay
out of the repository.

At 60 fps the same extrapolation and blend gave 1–50 units in the spike (§12); the
headless numbers are dominated by 7 shared frames passing per local frame. Note that
the page's `?netloss=` is a fraction (`0.05` = 5 %).

### Not done in phase 1

Items (boxes are off), host-authoritative laps/results (`LAP`/`RESULTS`, `KartChecker`
overrides for net karts), CPU karts, rank corrections, return to the lobby after the
race, hazards re-phased at GO (object timers run from course load, so a peer that
waited longer at the start sees them out of phase), victim-side contact effects,
visual-offset smoothing. `MKDD_AUTOPILOT` cannot drive an online kart: VS without
CPUs has no rival data (`RivalBodyCtrl::getTopComKartIndex` asserts).
