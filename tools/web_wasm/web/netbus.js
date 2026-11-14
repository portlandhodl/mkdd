// Game <-> page event bus (tools/web_wasm/net/netbus.c, net_bridge.h) for the in-game multiplayer
// menus: the game drives the lobby (connect/create/join/leave/ready) through the Multiplayer panel's
// Lobby (netui.js, window.mkddNetUi), reads the lobby state back, and reports its phase. Also keeps
// the wasm transport bridge (Mesh.attachModule) attached to the current mesh while in a room, so the
// game can talk to the other members before (and without) a lobby session.
//
// Module.netBus = { fromGame(type, data), toGame: [{type, data}], lobbyState(), post(type, data) }
// window events: 'mkdd-game' CustomEvent { detail: { type, data } } for everything the game emits.
(() => {
  'use strict';
  const M = window.Module;
  if (!M) return;
  const ui = () => window.mkddNetUi;
  const toGame = [];
  let error = '', phase = '', connectWanted = false;
  const post = (type, data = {}) => { toGame.push({ type, data }); if (toGame.length > 256) toGame.shift(); };

  function lobbyState() {
    const u = ui(), lobby = u && u.lobby, mesh = u && u.mesh, room = lobby && lobby.room;
    const server = document.getElementById('mpServer'), name = document.getElementById('mpName');
    const s = { state: 'offline', server: server ? server.value : '', name: (name && name.value) || '', error };
    if (!lobby) { if (connectWanted) s.state = 'connecting'; return s; }
    if (!lobby.id) { s.state = 'connecting'; return s; }
    s.state = room ? 'room' : 'connected';
    s.self = lobby.id;
    if (!room) return s;
    s.code = room.code; s.host = room.host; s.roomState = room.state; s.max = room.maxMembers || 8;
    s.members = room.members.map((m) => {
      const p = mesh && mesh.peers.get(m.id);
      const link = m.id === lobby.id ? 'self' : !p ? 'none' : p.left ? 'left' : p.lost ? 'lost' : p.open ? 'open' : 'connecting';
      return { id: m.id, name: m.name, ready: !!m.ready, slot: m.slot, link, rtt: p && p.rtt ? Math.round(p.rtt) : 0 };
    });
    return s;
  }

  const ops = {
    async connect(d) {
      const u = ui();
      if (!u || u.lobby) return;
      if (d.name) { const n = document.getElementById('mpName'); if (n && !n.value) n.value = d.name; }
      if (d.server) { const s = document.getElementById('mpServer'); if (s) s.value = d.server; }
      connectWanted = true; error = '';
      try { await u.connect(); } finally { connectWanted = false; }
      if (!u.lobby) { const st = document.getElementById('mpStatus'); error = st ? st.textContent : 'could not connect'; post('lobby-error', { msg: error }); }
    },
    disconnect() { const u = ui(); if (u && u.lobby) u.connect(); /* connect() toggles */ },
    create() { const l = ui() && ui().lobby; if (l) { error = ''; l.create(); } },
    join(d) { const l = ui() && ui().lobby; if (l && d.code) { error = ''; l.join(String(d.code)); } },
    leave() { const b = document.getElementById('mpLeave'); if (b && ui() && ui().lobby && ui().lobby.room) b.click(); },
    ready(d) { const u = ui(), l = u && u.lobby; if (l) { u.render(); /* status { inGame } first */ l.setReady(!!d.ready); } },
    name(d) { const l = ui() && ui().lobby; if (l && d.name) l.setName(String(d.name)); },
    profile(d) { const l = ui() && ui().lobby; if (l && d.profile) l.setProfile(d.profile); },
  };

  M.netBus = {
    toGame, post, lobbyState,
    get phase() { return phase; },
    inGame: false, // set once the game's MULTIPLAYER menu drives the lobby (netui.js: a running game may go online)
    fromGame(type, data) {
      if (type === 'lobby') M.netBus.inGame = true;
      if (type === 'lobby' && ops[data.op]) Promise.resolve(ops[data.op](data)).catch((e) => post('lobby-error', { msg: String(e.message || e) }));
      if (type === 'phase') phase = data.phase || '';
      if (type === 'log' && window.mkddPage) window.mkddPage.log('[game] ' + (data.msg || ''));
      dispatchEvent(new CustomEvent('mkdd-game', { detail: { type, data } }));
    },
  };

  // lobby errors and the bridge: follow netui's lobby/mesh as they are replaced
  let lobbySeen = null, meshSeen = null;
  setInterval(() => {
    const u = ui();
    if (!u) return;
    if (u.lobby && u.lobby !== lobbySeen) {
      lobbySeen = u.lobby;
      u.lobby.on('error', (e) => { error = e.msg || String(e.code); post('lobby-error', { msg: error, code: e.code, re: e.re }); });
      u.lobby.on('close', () => post('lobby-closed', {}));
    }
    if (u.mesh && u.mesh !== meshSeen && M.portReady) {
      meshSeen = u.mesh;
      u.mesh.attachModule(M);
    }
  }, 100);
})();
