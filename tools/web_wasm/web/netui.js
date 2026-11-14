// Multiplayer panel (index.html #mp) and in-game overlay (#netHud, #netMsg).
// Transport: net.js. URL parameters: ?netserver=ws://host:port/ws, ?netname=Name,
// ?nethost=1 (create a room on connect) / ?netroom=CODE (join), ?netnoboot=1 (do not
// boot the game at session start; tests), simulated network ?netlag=ms&netjitter=ms&netloss=0..1.
(() => {
  'use strict';
  const page = window.mkddPage, Net = window.MkddNet;
  if (!page || !Net) return;
  const $ = (id) => document.getElementById(id);
  const params = page.params;
  const ui = {
    server: $('mpServer'), name: $('mpName'), connect: $('mpConnect'), lobbyBar: $('mpLobbyBar'), create: $('mpCreate'),
    code: $('mpCode'), join: $('mpJoin'), leave: $('mpLeave'), room: $('mpRoom'), roomCode: $('mpRoomCode'), copy: $('mpCopy'),
    roomInfo: $('mpRoomInfo'), members: $('mpMembers').tBodies[0], ready: $('mpReady'), start: $('mpStart'),
    status: $('mpStatus'), summary: $('mpSummary'), panel: $('mp'), game: $('mpGame'),
    hud: $('netHud'), msg: $('netMsg'), msgText: $('netMsgText'), msgButtons: $('netMsgButtons'),
    alert: $('mpAlert'), alertText: $('mpAlertText'), alertButtons: $('mpAlertButtons'),
  };
  // include/kartEnums.h ECharID / EKartID / ECourseID
  const CHARS = ['', 'Baby Mario', 'Baby Luigi', 'Paratroopa', 'Koopa', 'Peach', 'Daisy', 'Mario', 'Luigi', 'Wario', 'Waluigi',
                 'Yoshi', 'Birdo', 'Donkey Kong', 'Diddy Kong', 'Bowser', 'Bowser Jr.', 'Toad', 'Toadette', 'King Boo', 'Petey Piranha'];
  const KARTS = ['Red Fire', 'DK Jumbo', 'Turbo Yoshi', 'Koopa Dasher', 'Heart Coach', 'Goo-Goo Buggy', 'Wario Car', 'Koopa King',
                 'Green Fire', 'Barrel Train', 'Turbo Birdo', 'Para-Wing', 'Bloom Coach', 'Rattle Buggy', 'Waluigi Racer',
                 'Bullet Blaster', 'Toad Kart', 'Toadette Kart', 'Boo Pipes', 'Piranha Pipes', 'Parade Kart'];
  const COURSES = [[0x24, 'Luigi Circuit'], [0x22, 'Peach Beach'], [0x21, 'Baby Park'], [0x32, 'Dry Dry Desert'],
                   [0x28, 'Mushroom Bridge'], [0x25, 'Mario Circuit'], [0x23, 'Daisy Cruiser'], [0x2a, 'Waluigi Stadium'],
                   [0x33, 'Sherbet Land'], [0x29, 'Mushroom City'], [0x26, 'Yoshi Circuit'], [0x2d, 'DK Mountain'],
                   [0x2b, 'Wario Colosseum'], [0x2c, 'Dino Dino Jungle'], [0x2f, 'Bowser\'s Castle'], [0x31, 'Rainbow Road']];
  const fill = (sel, items) => { for (const [v, t] of items) sel.add(new Option(t, String(v))); };
  fill($('mpChar1'), CHARS.map((n, i) => [i, n]).slice(1));
  fill($('mpChar2'), CHARS.map((n, i) => [i, n]).slice(1));
  fill($('mpKart'), KARTS.map((n, i) => [i, n]));
  fill($('mpCourse'), COURSES);

  const KEY = 'mkdd-net-v1';
  let saved = {};
  try { saved = JSON.parse(localStorage.getItem(KEY) || '{}'); } catch (e) {}
  const profile = { char1: 7, char2: 8, kart: 0, ...(saved.profile || {}) };
  const persist = () => {
    try { localStorage.setItem(KEY, JSON.stringify({ server: ui.server.value, name: ui.name.value, profile })); } catch (e) {}
  };
  const defaultServer = `${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.hostname || '127.0.0.1'}:8095/ws`;
  ui.server.value = params.get('netserver') || saved.server || defaultServer;
  ui.name.value = params.get('netname') || saved.name || '';
  // a page served by the lobby server itself finds the lobby on the same origin
  if (!params.has('netserver') && !saved.server)
    fetch('net-config.json').then((r) => (r.ok ? r.json() : null)).then((c) => {
      if (c && c.ws) ui.server.value = new URL(c.ws, location.href.replace(/^http/, 'ws')).href;
    }).catch(() => {});
  if (params.has('netserver')) ui.panel.open = true;

  const sim = { lag: +params.get('netlag') || 0, jitter: +params.get('netjitter') || 0, loss: +params.get('netloss') || 0 };
  const log = (t) => page.log(t);
  let lobby = null, mesh = null, gameId = '', build = '', connecting = false, bridged = false;
  // this window's part of a session: '' (none yet), 'booting', 'failed' (bootWhy says why)
  let bootState = '', bootWhy = '';
  let autoRoom = '', autoDone = false; // ?netroom: the room to (re)join once connected
  const setStatus = (t) => { ui.status.textContent = t; };

  // Why this window cannot race right now ('' = it can): it needs a disc image and a game that has
  // not been started solo (a running game cannot switch to the online race; only a reload can).
  function cannotRace() {
    if (!page.getDisc()) return 'no disc image chosen';
    if (page.started() && !(mesh && mesh.session)) return 'game already running; reload the page to play online';
    return '';
  }
  // the same, for another member, from the status that member reports to the room
  function memberProblem(m) {
    const st = m && m.status;
    if (!st) return '';
    if (st.disc === false) return 'no disc image chosen';
    if (st.idle === false && !(lobby && lobby.room && lobby.room.state === 'playing')) return 'game already running, needs a page reload';
    return '';
  }
  const BOOT = { booting: 'starting the game', loading: 'loading the course', waiting: 'at the start line', countdown: 'countdown',
                 race: 'racing', finished: 'finished', none: 'not started' };
  const bootText = (st) => !st || !st.boot ? '' : st.boot === 'failed' ? 'could not start: ' + (st.why || '?') : BOOT[st.boot] || st.boot;
  // tell the room whether this window can race (lobby) and how far its game got (session)
  let sentStatus = '';
  function syncStatus() {
    if (!lobby || !lobby.id) return;
    const st = { disc: !!page.getDisc(), idle: !page.started() };
    if (mesh && mesh.session) {
      const M = window.Module || {};
      st.boot = bootState === 'failed' ? 'failed' : M.netPhase || (page.started() ? 'booting' : params.has('netnoboot') ? 'none' : 'booting');
      if (bootState === 'failed') st.why = bootWhy.slice(0, 120);
    }
    const key = JSON.stringify(st);
    if (key === sentStatus) return;
    sentStatus = key;
    lobby.setStatus(st);
  }

  // Reload into the lobby: same server and name, rejoining `room` (or creating a room with `host`).
  function reloadUrl({ room, host } = {}) {
    const u = new URL(location.href);
    u.searchParams.delete('nethost'); u.searchParams.delete('netroom');
    u.searchParams.set('netserver', ui.server.value);
    if (ui.name.value) u.searchParams.set('netname', ui.name.value);
    if (room) u.searchParams.set('netroom', room);
    else if (host) u.searchParams.set('nethost', '1');
    return u.href;
  }
  const reloadTo = (o) => () => { persist(); location.href = reloadUrl(o); };

  // A notice at the top of the Multiplayer drawer and, with overlay, over the game picture.
  // sticky: stays until replaced (the "connection interrupted" overlay does not cover it).
  let noticeTimer = 0, stickyMsg = false;
  function notice(text, buttons = [], { overlay = true, sticky = false, timeout = 0 } = {}) {
    clearTimeout(noticeTimer);
    ui.alertText.textContent = text;
    ui.alertButtons.textContent = '';
    for (const [label, fn] of buttons) {
      const b = document.createElement('button');
      b.textContent = label; b.onclick = fn;
      ui.alertButtons.append(b);
    }
    ui.alert.hidden = false;
    if (overlay) { showMsg(text, buttons); stickyMsg = sticky; }
    if (timeout) noticeTimer = setTimeout(clearNotice, timeout);
  }
  function clearNotice() {
    clearTimeout(noticeTimer);
    ui.alert.hidden = true;
    if (stickyMsg || !ui.msgKey) { stickyMsg = false; hideMsg(); }
  }
  // the game runs solo in this window: it cannot create or join a room until a reload
  function refuseRunning(o) {
    const text = 'Your game is already running in this window, so it cannot go online. Reload the page to play online' +
                 (o.room ? ` (you rejoin room ${o.room})` : o.host ? ' (a new room is created)' : '') + '.';
    log('[net] ' + text);
    setStatus(text);
    notice(text, [['Reload to play online', reloadTo(o)], ['Not now', clearNotice]]);
  }
  function createRoom() {
    if (!lobby) return;
    if (page.started()) return refuseRunning({ host: true });
    lobby.create();
  }
  let joinRetry = 0;
  function joinRoom(code) {
    clearTimeout(joinRetry);
    code = String(code || '').trim().toUpperCase();
    if (!lobby || !code) return;
    if (page.started()) return refuseRunning({ room: code });
    lobby.join(code);
  }

  const fingerprint = Net.buildFingerprint().then((b) => { build = b; return b; }).catch((e) => { log('[net] cannot fingerprint mkdd.wasm: ' + e); return ''; });

  async function connect() {
    if (connecting) return;
    if (lobby) { disconnect(); return; }
    if (page.started()) {
      const room = params.get('netroom');
      refuseRunning(room ? { room } : params.has('nethost') ? { host: true } : {});
      return;
    }
    if (!page.getDisc()) {
      setStatus('Choose your disc image first: every player needs their own disc image to race online.');
      notice('Choose your disc image first (the "Choose disc image" button or drag and drop it on the page): every player needs their own disc to race online.',
             [], { overlay: false });
      return;
    }
    connecting = true;
    persist();
    setStatus('Connecting to ' + ui.server.value + ' ...');
    try {
      await fingerprint;
      gameId = await Net.gameIdOf(page.getDisc());
      lobby = new Net.Lobby();
      lobby.on('close', () => {
        log('[net] lobby connection closed');
        clearTimeout(joinRetry);
        if (!(mesh && mesh.session)) { if (mesh) mesh.close(); mesh = null; lobby = null; }
        else notice('Lost the connection to the lobby server. The race goes on between the players.', [], { overlay: false, timeout: 10000 });
        render();
      });
      lobby.on('error', (e) => {
        log('[net] lobby: ' + e.msg);
        // an automatic rejoin (reload after a session) waits for the running session to end
        if (e.re === 'join' && e.code === 'started' && autoRoom) {
          setStatus(`Room ${autoRoom} is still in a session: you join as soon as the host ends it.`);
          notice(`Room ${autoRoom} is still racing: you join as soon as the host ends the session.`, [['Stop waiting', () => { autoRoom = ''; clearTimeout(joinRetry); clearNotice(); }]], { overlay: false });
          joinRetry = setTimeout(() => { if (lobby && !lobby.room && autoRoom) joinRoom(autoRoom); }, 3000);
          return;
        }
        setStatus('Lobby: ' + e.msg);
        if (['create', 'join', 'start', 'ready', 'settings'].includes(e.re) || e.code === 'kicked' || e.code === 'proto')
          notice(e.re === 'start' ? 'Cannot start: ' + e.msg : e.msg.replace(/^./, (c) => c.toUpperCase()), [], { overlay: false, timeout: 12000 });
      });
      lobby.on('room', (room) => {
        if (autoRoom && room.code === autoRoom) { autoRoom = ''; clearTimeout(joinRetry); clearNotice(); }
        render();
      });
      lobby.on('peer-left', (m) => log(`[net] ${nameOf(m.id)} ${m.reason === 'kicked' ? 'was removed' : 'left'}`));
      await lobby.connect(ui.server.value, { name: ui.name.value || 'Player', gameId, build });
      lobby.setProfile(profile);
      clearNotice();
      sentStatus = '';
      syncStatus();
      newMesh();
      if (sim.lag || sim.loss || sim.jitter) log(`[net] simulated network: ${sim.lag} ms lag, ${sim.jitter} ms jitter, ${sim.loss * 100}% loss`);
      setStatus(`Connected as ${ui.name.value || 'Player'} (${gameId}). Create a room or join one.`);
      // ?nethost / ?netroom act once; a reload after that should not repeat them
      if (!autoDone) {
        autoDone = true;
        if (params.has('nethost')) createRoom();
        else if (params.get('netroom')) { autoRoom = params.get('netroom').toUpperCase(); joinRoom(autoRoom); }
        if (params.has('nethost') || params.has('netroom')) {
          const u = new URL(location.href);
          u.searchParams.delete('nethost'); u.searchParams.delete('netroom');
          try { history.replaceState(history.state, '', u.href); } catch (e) {}
        }
      }
    } catch (e) {
      setStatus('Could not connect: ' + e.message);
      if (lobby) lobby.close();
      lobby = null;
    }
    connecting = false;
    render();
  }
  function newMesh() {
    if (mesh) mesh.close();
    mesh = new Net.Mesh(lobby, { sim, log });
    mesh.on('peers', render);
    mesh.on('start', onStart);
    mesh.on('end', () => {
      const code = lobby && lobby.room && lobby.room.code;
      notice('The host ended the session. Reload the page to race again' + (code ? ` (you rejoin room ${code}).` : '.'),
             [['Reload', reloadTo({ room: code })]], { sticky: true });
      render();
    });
    mesh.on('host', (id) => log(`[net] ${nameOf(id)} is the host now`));
  }
  function disconnect() {
    if (mesh) mesh.close();
    if (lobby) lobby.close();
    mesh = null; lobby = null;
    render();
  }
  const nameOf = (id) => { const m = lobby && lobby.room && lobby.room.members.find((x) => x.id === id); return m ? m.name : '#' + id; };
  const describe = (p) => p && p.char1 ? `${CHARS[p.char1] || '?'} + ${CHARS[p.char2] || '?'}, ${KARTS[p.kart] ?? '?'}` : '';

  function render() {
    const room = lobby && lobby.room;
    const running = !!(mesh && mesh.session);
    syncStatus();
    ui.connect.textContent = lobby ? 'Disconnect' : 'Connect';
    ui.connect.disabled = running;
    ui.server.disabled = ui.name.disabled = !!lobby;
    ui.lobbyBar.hidden = !lobby;
    ui.create.hidden = ui.join.hidden = ui.code.parentNode.hidden = !!room;
    ui.leave.hidden = !room || running;
    ui.room.hidden = !room;
    page.setSoloStart(!room, room && lobby.isHost ? 'You are in a multiplayer room: start the race with GO! in the Multiplayer panel.'
      : 'You are in a multiplayer room: the host starts the race.');
    ui.summary.textContent = room ? `— room ${room.code}, ${room.members.length} player${room.members.length > 1 ? 's' : ''}` + (running ? ', in session' : '') : lobby ? '— connected' : '';
    if (!room) return;
    const host = lobby.isHost;
    ui.roomCode.textContent = room.code;
    ui.roomInfo.textContent = `${room.gameId} · ${host ? 'you are the host' : 'host: ' + nameOf(room.host)}`;
    // (not rebuilt while the host has a slot list open: it would close under the pointer)
    if (!ui.members.contains(document.activeElement)) {
      ui.members.textContent = '';
      for (const m of room.members) {
        const tr = ui.members.insertRow();
        const p = mesh && mesh.peers.get(m.id);
        const problem = m.id === lobby.id ? cannotRace() : memberProblem(m);
        // styling hooks (theme.css)
        tr.dataset.slot = String(m.slot);
        tr.dataset.ready = m.id === room.host ? 'host' : m.ready ? '1' : '0';
        tr.dataset.link = m.id === lobby.id || !p ? '' : p.left ? 'left' : p.lost ? 'lost' : p.open ? 'direct' : 'connecting';
        tr.classList.toggle('host', m.id === room.host);
        tr.classList.toggle('me', m.id === lobby.id);
        tr.classList.toggle('blocked', !!problem || !!(m.status && m.status.boot === 'failed'));
        const slotCell = tr.insertCell();
        if (host && !running) {
          const sel = document.createElement('select');
          sel.add(new Option('watch', '-1'));
          for (let i = 0; i < (room.maxMembers || 8); i++) sel.add(new Option(String(i + 1), String(i)));
          sel.value = String(m.slot);
          sel.onchange = () => { lobby.setSettings({ slots: { [m.id]: +sel.value } }); sel.blur(); };
          slotCell.append(sel);
        } else slotCell.textContent = m.slot >= 0 ? String(m.slot + 1) : 'watch';
        tr.insertCell().textContent = m.name + (m.id === room.host ? ' (host)' : '') + (m.id === lobby.id ? ' (you)' : '');
        const state = problem ? 'cannot race: ' + problem : running ? bootText(m.status) : '';
        tr.insertCell().textContent = describe(m.profile) + (state ? (problem ? '\n⚠ ' : ' · ') + state : '');
        tr.insertCell().textContent = p && p.rtt ? Math.round(p.rtt) + ' ms' : m.id === lobby.id ? '' : '…';
        tr.insertCell().textContent = m.id === lobby.id ? '' : !p ? '' : p.left ? 'left' : p.lost ? 'lost' : p.open ? 'direct' : p.pc.connectionState;
        tr.insertCell().textContent = m.id === room.host ? '' : m.ready ? 'yes' : 'no';
        const act = tr.insertCell();
        if (host && m.id !== lobby.id && !running) {
          const b = document.createElement('button');
          b.className = 'ghost'; b.textContent = 'Remove';
          b.onclick = () => lobby.kick(m.id);
          act.append(b);
        }
      }
    }
    for (const sel of ui.panel.querySelectorAll('select[data-p]')) { sel.value = String(profile[sel.dataset.p]); sel.disabled = running; }
    for (const sel of ui.game.querySelectorAll('select[data-g]')) {
      const v = room.settings && room.settings[sel.dataset.g];
      if (v !== undefined) sel.value = String(v);
      sel.disabled = !host || running;
    }
    const me = lobby.me();
    const mine = cannotRace();
    // Ready only with a disc and a game that has not been started solo
    if (mine && me && me.ready && !running) lobby.setReady(false);
    ui.ready.checked = !!(me && me.ready) && !mine;
    ui.ready.disabled = running || !!mine;
    ui.ready.parentNode.title = mine ? 'You cannot race: ' + mine : '';
    ui.ready.parentNode.hidden = host;
    ui.start.hidden = !host;
    const others = room.members.filter((m) => m.id !== lobby.id);
    const notReady = others.filter((m) => !m.ready);
    const blocked = others.filter((m) => memberProblem(m)).map((m) => `${m.name} (${memberProblem(m)})`);
    if (mine) blocked.unshift(`you (${mine})`);
    const allOpen = mesh && mesh.allConnected();
    const why = blocked.length ? 'Cannot start: ' + blocked.join(', ') + '.'
      : !allOpen ? 'Connecting to the other players (WebRTC) ...'
      : notReady.length ? `Waiting for ${notReady.map((m) => m.name).join(', ')} to be ready.` : '';
    ui.start.disabled = running || !!why;
    ui.start.title = why || 'Start the race on every player\'s screen';
    if (!running) {
      if (mine) setStatus(`You cannot race: ${mine}.`);
      else if (host) setStatus(why || 'Everyone is ready: press GO! to start the race.');
      else if (blocked.length || !allOpen) setStatus(why);
      else setStatus(me && me.ready ? 'Ready: waiting for the host to start.' : 'Pick your drivers and kart, then tick Ready.');
    } else showFailures(room);
  }

  // Another player could not boot into the race: everyone sees who and why; the host can end the session.
  let failKey = '';
  function showFailures(room) {
    const failed = room.members.filter((m) => m.id !== lobby.id && m.status && m.status.boot === 'failed');
    const key = failed.map((m) => m.id).join(',');
    if (!key || key === failKey || bootState === 'failed') return;
    failKey = key;
    const text = `${failed.map((m) => `${m.name} cannot join the race: ${m.status.why || 'their game did not start'}`).join('. ')}. ` +
                 (lobby.isHost ? 'End the session, then everyone reloads and you start again.' : 'The host has to end the session and start again.');
    log('[net] ' + text);
    notice(text, lobby.isHost ? [['End session', () => lobby.end()], ['Keep racing', clearNotice]] : [['OK', clearNotice]], { sticky: true });
  }

  // -- session start: the game boots on every page, with the transport attached --
  function failBoot(why) {
    if (bootState === 'failed') return;
    bootState = 'failed'; bootWhy = why;
    const code = mesh && mesh.session && mesh.session.code;
    const text = `The race started, but this window cannot join it: ${why}. ` +
                 `Reload the page to play online${code ? ` (you rejoin room ${code} once the host ends this session)` : ''}.`;
    log('[net] ' + text);
    setStatus(text);
    notice(text, [['Reload to play online', reloadTo({ room: code })]], { sticky: true });
    syncStatus();
  }
  function onStart(session) {
    hideMsg();
    setStatus(`In session ${session.code}: ${session.members.length} players.`);
    const attach = (Module) => { if (!bridged) { bridged = true; mesh.attachModule(Module); } };
    const why = !page.getDisc() ? 'no disc image is chosen in this window'
      : page.started() ? 'the game was already running in this window before the session started' : '';
    ui.hud.dataset.boot = why && !params.has('netnoboot') ? 'fail' : 'ok';
    if (params.has('netnoboot')) { if (window.Module && Module.portReady) attach(Module); }
    else if (why) failBoot(why);
    else {
      bootState = 'booting';
      if (window.Module && Module.portReady) attach(Module);
      Promise.resolve(page.startGame({ env: { MKDD_NET_ONLINE: '1' }, attach }))
        .catch((e) => failBoot('the game did not start (' + (e && e.message || e) + ')'));
    }
    ui.hud.hidden = false;
    render();
  }
  if (page.onAbort) page.onAbort((what) => { if (mesh && mesh.session && bootState === 'booting') failBoot(`the game stopped (${what}); see the pit radio log`); });

  function hostSettings() {
    const g = {};
    for (const sel of ui.game.querySelectorAll('select[data-g]')) g[sel.dataset.g] = +sel.value;
    return g;
  }

  // -- overlay --
  function showMsg(text, buttons = []) {
    ui.msgText.textContent = text;
    ui.msgButtons.textContent = '';
    for (const [label, fn] of buttons) {
      const b = document.createElement('button');
      b.textContent = label; b.onclick = fn;
      ui.msgButtons.append(b);
    }
    ui.msg.hidden = false;
  }
  function hideMsg() { ui.msg.hidden = true; ui.msgKey = ''; }
  let lastHud = 0;
  const fmtTime = (ms) => `${Math.floor(ms / 60000)}:${String(Math.floor(ms / 1000) % 60).padStart(2, '0')}.${String(ms % 1000).padStart(3, '0')}`;
  function hud() {
    requestAnimationFrame(hud);
    if (!mesh || !mesh.session) return;
    const now = performance.now();
    if (now - lastHud < 250) return;
    lastHud = now;
    syncStatus();
    const peers = [...mesh.peers.values()];
    // the game's side (tools/web_wasm/net/netonline.cpp): race phase and per-kart replica state
    const M = window.Module || {}, st = M.netStats;
    const phase = { loading: 'loading the course', waiting: 'waiting for the other players at the start', countdown: 'countdown' }[M.netPhase];
    const karts = st && st.phase !== 'loading' ? st.karts.filter((k) => !k.local).map((k) =>
      `${k.name}: ${k.status}${k.status === 'live' ? `, ${Math.round(k.err)} u off, ${k.age.toFixed(1)} f old` : ''}` +
      (k.goal >= 0 ? `, finished ${fmtTime(k.goal)}` : '')) : [];
    // until the race runs: how far the others got, from what they report to the room
    const room = lobby && lobby.room;
    const boots = room && (!M.netPhase || M.netPhase === 'loading' || M.netPhase === 'waiting')
      ? room.members.filter((m) => m.id !== lobby.id && m.status && m.status.boot).map((m) => `${m.name}: ${bootText(m.status)}`) : [];
    ui.hud.textContent = peers.map((p) => `${p.name} ${p.left ? 'left' : p.lost ? 'LOST' : Math.round(p.rtt) + ' ms'}`).join('  ') +
                         (mesh.isHost ? '' : `\nclock ${mesh.clockSynced() ? (mesh.clockOffset >= 0 ? '+' : '') + Math.round(mesh.clockOffset) + ' ms' : 'syncing'}`) +
                         (phase ? '\n' + phase : '') + (karts.length ? '\n' + karts.join('\n') : '') + (boots.length ? '\n' + boots.join('\n') : '');
    const lost = peers.filter((p) => p.lost && !p.left);
    const key = lost.map((p) => p.id).join(',');
    if (!stickyMsg && key !== (ui.msgKey || '')) {
      ui.msgKey = key;
      if (key) showMsg(`Connection to ${lost.map((p) => p.name).join(', ')} interrupted...`); else ui.msg.hidden = true;
    }
  }
  requestAnimationFrame(hud);

  ui.connect.onclick = connect;
  ui.create.onclick = createRoom;
  ui.join.onclick = () => joinRoom(ui.code.value);
  ui.code.addEventListener('keydown', (e) => { if (e.key === 'Enter') ui.join.click(); });
  ui.leave.onclick = () => { lobby.leave(); newMesh(); render(); };
  ui.copy.onclick = () => navigator.clipboard && navigator.clipboard.writeText(ui.roomCode.textContent).catch(() => {});
  ui.ready.onchange = () => {
    if (!lobby) return;
    const why = cannotRace();
    if (why && ui.ready.checked) { ui.ready.checked = false; setStatus(`You cannot race: ${why}.`); return; }
    lobby.setReady(ui.ready.checked);
  };
  ui.start.onclick = () => {
    if (!lobby || ui.start.disabled) return;
    setStatus('Starting the session ...');
    lobby.start();
  };
  for (const sel of ui.panel.querySelectorAll('select[data-p]'))
    sel.addEventListener('change', () => { profile[sel.dataset.p] = +sel.value; persist(); if (lobby) lobby.setProfile(profile); });
  for (const sel of ui.game.querySelectorAll('select[data-g]'))
    sel.addEventListener('change', () => { if (lobby && lobby.isHost) lobby.setSettings({ game: hostSettings() }); });
  lobbyHostDefaults();
  function lobbyHostDefaults() {
    // a new room starts with the host's current race form
    const t = setInterval(() => {
      if (!lobby || !lobby.room) return;
      clearInterval(t);
      if (lobby.isHost && !Object.keys(lobby.room.settings || {}).length) lobby.setSettings({ game: hostSettings() });
    }, 200);
  }
  ui.name.onchange = persist;
  ui.server.onchange = persist;
  setInterval(() => { if (mesh && !mesh.session) render(); else syncStatus(); }, 1000);
  // keys typed into the panel's fields must not drive the game
  for (const el of ui.panel.querySelectorAll('input')) el.addEventListener('keydown', (e) => e.stopPropagation());
  if (params.has('netserver') && (params.has('nethost') || params.has('netroom'))) {
    // automation (and "Reload to play online"): connect as soon as the disc is known
    let ticks = 0;
    const t = setInterval(() => {
      if (page.getDisc() && build) { clearInterval(t); connect(); return; }
      if (++ticks === 30 && !page.getDisc())
        notice('Choose your disc image to go online' + (params.get('netroom') ? `: you then join room ${params.get('netroom').toUpperCase()}.` : '.'), [], { overlay: false });
    }, 100);
  }
  window.mkddNetUi = { get lobby() { return lobby; }, get mesh() { return mesh; }, connect, render };
})();
