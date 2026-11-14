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
  const setStatus = (t) => { ui.status.textContent = t; };

  const fingerprint = Net.buildFingerprint().then((b) => { build = b; return b; }).catch((e) => { log('[net] cannot fingerprint mkdd.wasm: ' + e); return ''; });

  async function connect() {
    if (connecting) return;
    if (lobby) { disconnect(); return; }
    connecting = true;
    persist();
    setStatus('Connecting to ' + ui.server.value + ' ...');
    try {
      await fingerprint;
      if (!page.getDisc()) throw new Error('choose your disc image first');
      gameId = await Net.gameIdOf(page.getDisc());
      lobby = new Net.Lobby();
      lobby.on('close', () => {
        log('[net] lobby connection closed');
        if (!(mesh && mesh.session)) { if (mesh) mesh.close(); mesh = null; lobby = null; }
        render();
      });
      lobby.on('error', (e) => { setStatus('Lobby: ' + e.msg); log('[net] lobby: ' + e.msg); });
      lobby.on('room', render);
      lobby.on('peer-left', (m) => log(`[net] ${nameOf(m.id)} ${m.reason === 'kicked' ? 'was removed' : 'left'}`));
      await lobby.connect(ui.server.value, { name: ui.name.value || 'Player', gameId, build });
      lobby.setProfile(profile);
      newMesh();
      if (sim.lag || sim.loss || sim.jitter) log(`[net] simulated network: ${sim.lag} ms lag, ${sim.jitter} ms jitter, ${sim.loss * 100}% loss`);
      setStatus(`Connected as ${ui.name.value || 'Player'} (${gameId}). Create a room or join one.`);
      if (params.has('nethost')) lobby.create();
      else if (params.get('netroom')) lobby.join(params.get('netroom'));
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
    mesh.on('end', () => { showMsg('The host ended the session.', [['Reload', () => location.reload()]]); render(); });
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
    ui.connect.textContent = lobby ? 'Disconnect' : 'Connect';
    ui.connect.disabled = running;
    ui.server.disabled = ui.name.disabled = !!lobby;
    ui.lobbyBar.hidden = !lobby;
    ui.create.hidden = ui.join.hidden = ui.code.parentNode.hidden = !!room;
    ui.leave.hidden = !room || running;
    ui.room.hidden = !room;
    page.setSoloStart(!room);
    ui.summary.textContent = room ? `— room ${room.code}, ${room.members.length} player${room.members.length > 1 ? 's' : ''}` + (running ? ', in session' : '') : lobby ? '— connected' : '';
    if (!room) return;
    const host = lobby.isHost;
    ui.roomCode.textContent = room.code;
    ui.roomInfo.textContent = `${room.gameId} · ${host ? 'you are the host' : 'host: ' + nameOf(room.host)}`;
    ui.members.textContent = '';
    for (const m of room.members) {
      const tr = ui.members.insertRow();
      const p = mesh && mesh.peers.get(m.id);
      // styling hooks (theme.css)
      tr.dataset.slot = String(m.slot);
      tr.dataset.ready = m.id === room.host ? 'host' : m.ready ? '1' : '0';
      tr.dataset.link = m.id === lobby.id || !p ? '' : p.left ? 'left' : p.lost ? 'lost' : p.open ? 'direct' : 'connecting';
      tr.classList.toggle('host', m.id === room.host);
      tr.classList.toggle('me', m.id === lobby.id);
      const slotCell = tr.insertCell();
      if (host && !running) {
        const sel = document.createElement('select');
        sel.add(new Option('watch', '-1'));
        for (let i = 0; i < (room.maxMembers || 8); i++) sel.add(new Option(String(i + 1), String(i)));
        sel.value = String(m.slot);
        sel.onchange = () => lobby.setSettings({ slots: { [m.id]: +sel.value } });
        slotCell.append(sel);
      } else slotCell.textContent = m.slot >= 0 ? String(m.slot + 1) : 'watch';
      tr.insertCell().textContent = m.name + (m.id === room.host ? ' (host)' : '') + (m.id === lobby.id ? ' (you)' : '');
      tr.insertCell().textContent = describe(m.profile);
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
    for (const sel of ui.panel.querySelectorAll('select[data-p]')) { sel.value = String(profile[sel.dataset.p]); sel.disabled = running; }
    for (const sel of ui.game.querySelectorAll('select[data-g]')) {
      const v = room.settings && room.settings[sel.dataset.g];
      if (v !== undefined) sel.value = String(v);
      sel.disabled = !host || running;
    }
    const me = lobby.me();
    ui.ready.checked = !!(me && me.ready);
    ui.ready.disabled = running;
    ui.ready.parentNode.hidden = host;
    ui.start.hidden = !host;
    const allReady = room.members.filter((m) => m.id !== lobby.id).every((m) => m.ready);
    const allOpen = mesh && mesh.allConnected();
    ui.start.disabled = running || !allReady || !allOpen;
    if (!running) {
      if (!allOpen) setStatus('Connecting to the other players (WebRTC) ...');
      else if (host) setStatus(allReady ? 'Everyone is ready: start the session.' : 'Waiting for everyone to be ready.');
      else setStatus(me && me.ready ? 'Ready: waiting for the host to start.' : 'Pick your drivers and kart, then tick Ready.');
    }
  }

  // -- session start: the game boots on every page, with the transport attached --
  function onStart(session) {
    hideMsg();
    setStatus(`In session ${session.code}: ${session.members.length} players.`);
    const attach = (Module) => { if (!bridged) { bridged = true; mesh.attachModule(Module); } };
    if (window.Module && Module.portReady) attach(Module);
    if (!params.has('netnoboot') && page.getDisc() && !page.started())
      page.startGame({ env: { MKDD_NET_ONLINE: '1' }, attach });
    ui.hud.hidden = false;
    render();
  }

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
  function hud() {
    requestAnimationFrame(hud);
    if (!mesh || !mesh.session) return;
    const now = performance.now();
    if (now - lastHud < 250) return;
    lastHud = now;
    const peers = [...mesh.peers.values()];
    ui.hud.textContent = peers.map((p) => `${p.name} ${p.left ? 'left' : p.lost ? 'LOST' : Math.round(p.rtt) + ' ms'}`).join('  ') +
                         (mesh.isHost ? '' : `\nclock ${mesh.clockSynced() ? (mesh.clockOffset >= 0 ? '+' : '') + Math.round(mesh.clockOffset) + ' ms' : 'syncing'}`);
    const lost = peers.filter((p) => p.lost && !p.left);
    const key = lost.map((p) => p.id).join(',');
    if (key !== (ui.msgKey || '')) {
      ui.msgKey = key;
      if (key) showMsg(`Connection to ${lost.map((p) => p.name).join(', ')} interrupted...`); else ui.msg.hidden = true;
    }
  }
  requestAnimationFrame(hud);

  ui.connect.onclick = connect;
  ui.create.onclick = () => lobby && lobby.create();
  ui.join.onclick = () => lobby && ui.code.value.trim() && lobby.join(ui.code.value.trim());
  ui.code.addEventListener('keydown', (e) => { if (e.key === 'Enter') ui.join.click(); });
  ui.leave.onclick = () => { lobby.leave(); newMesh(); render(); };
  ui.copy.onclick = () => navigator.clipboard && navigator.clipboard.writeText(ui.roomCode.textContent).catch(() => {});
  ui.ready.onchange = () => lobby && lobby.setReady(ui.ready.checked);
  ui.start.onclick = () => lobby && lobby.start();
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
  setInterval(() => { if (mesh && !mesh.session) render(); }, 1000);
  // keys typed into the panel's fields must not drive the game
  for (const el of ui.panel.querySelectorAll('input')) el.addEventListener('keydown', (e) => e.stopPropagation());
  if (params.has('netserver') && (params.has('nethost') || params.has('netroom'))) {
    // automation: connect as soon as the disc is known
    const t = setInterval(() => { if (page.getDisc() && build) { clearInterval(t); connect(); } }, 100);
  }
  window.mkddNetUi = { get lobby() { return lobby; }, get mesh() { return mesh; }, connect, render };
})();
