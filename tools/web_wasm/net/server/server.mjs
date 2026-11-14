// Lobby and WebRTC signalling server for the MKDD web port's online play.
//
//   node tools/web_wasm/net/server/server.mjs
//
// Private rooms with 6-character join codes (MAX_MEMBERS, default 8), member list
// with names / slots / ready flags / a small per-member profile (character and kart
// choice), host game settings, a version check (disc game ID + build fingerprint
// must match the room's), relaying of WebRTC SDP/ICE between members, heartbeats,
// cleanup and basic rate/size limits. Game traffic goes peer-to-peer over WebRTC
// data channels; this server only sees lobby messages. Optionally serves a static directory
// (STATIC_DIR) so one process hosts the page as well. See ../README.md for the env.
import http from 'node:http';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { WebSocketServer } from 'ws';

export const PROTO = 1;
const CODE_ALPHABET = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789'; // no 0/O, 1/I
const MAX_JSON = 2048; // host game settings / member profile, serialised

function envInt(env, name, def) {
  const v = parseInt(env[name] ?? '', 10);
  return Number.isFinite(v) ? v : def;
}
const list = (s) => (s || '').split(',').map((x) => x.trim()).filter(Boolean);

export function loadConfig(env = process.env) {
  return {
    port: envInt(env, 'PORT', 8095),
    maxMembers: Math.max(2, Math.min(8, envInt(env, 'MAX_MEMBERS', 8))),
    host: env.HOST || '127.0.0.1',
    staticDir: env.STATIC_DIR ? path.resolve(env.STATIC_DIR) : null,
    stun: env.STUN_URLS !== undefined ? list(env.STUN_URLS) : ['stun:stun.l.google.com:19302', 'stun:stun.cloudflare.com:3478'],
    turn: list(env.TURN_URLS),
    turnUser: env.TURN_USERNAME || '',
    turnCredential: env.TURN_CREDENTIAL || '',
    turnSecret: env.TURN_SECRET || '', // coturn use-auth-secret: per-client time-limited credentials
    turnTtl: envInt(env, 'TURN_TTL', 6 * 3600),
    maxRooms: envInt(env, 'MAX_ROOMS', 500),
    maxConnPerIp: envInt(env, 'MAX_CONN_PER_IP', 16),
    maxMsgBytes: envInt(env, 'MAX_MSG_BYTES', 64 * 1024),
    msgRate: envInt(env, 'MSG_RATE', 60), // messages per second, sustained
    msgBurst: envInt(env, 'MSG_BURST', 200),
    roomsPerMinutePerIp: envInt(env, 'ROOMS_PER_MIN', 10),
    heartbeatMs: envInt(env, 'HEARTBEAT_MS', 10_000),
    roomIdleMs: envInt(env, 'ROOM_IDLE_MS', 6 * 3600_000),
    trustProxy: env.TRUST_PROXY === '1',
  };
}

function iceServers(cfg, id) {
  const servers = cfg.stun.length ? [{ urls: cfg.stun }] : [];
  if (cfg.turn.length) {
    if (cfg.turnSecret) {
      const username = `${Math.floor(Date.now() / 1000) + cfg.turnTtl}:${id}`;
      const credential = crypto.createHmac('sha1', cfg.turnSecret).update(username).digest('base64');
      servers.push({ urls: cfg.turn, username, credential });
    } else {
      servers.push({ urls: cfg.turn, username: cfg.turnUser, credential: cfg.turnCredential });
    }
  }
  return servers;
}

const MIME = { '.html': 'text/html; charset=utf-8', '.js': 'text/javascript', '.mjs': 'text/javascript', '.wasm': 'application/wasm',
               '.css': 'text/css', '.json': 'application/json', '.png': 'image/png', '.ico': 'image/x-icon', '.map': 'application/json',
               '.svg': 'image/svg+xml', '.woff2': 'font/woff2', '.txt': 'text/plain; charset=utf-8' };

function serveStatic(cfg, req, res) {
  if (!cfg.staticDir || (req.method !== 'GET' && req.method !== 'HEAD')) {
    res.writeHead(404).end();
    return;
  }
  let rel;
  try { rel = decodeURIComponent(new URL(req.url, 'http://x').pathname); } catch { res.writeHead(400).end(); return; }
  if (rel.endsWith('/')) rel += 'index.html';
  const file = path.join(cfg.staticDir, path.normalize(rel));
  if (!file.startsWith(cfg.staticDir + path.sep)) { res.writeHead(403).end(); return; }
  fs.stat(file, (err, st) => {
    if (err || !st.isFile()) { res.writeHead(404).end(); return; }
    res.writeHead(200, { 'Content-Type': MIME[path.extname(file)] || 'application/octet-stream', 'Content-Length': st.size,
                         'Cache-Control': 'no-cache' });
    if (req.method === 'HEAD') res.end();
    else fs.createReadStream(file).pipe(res);
  });
}

function cleanName(s) {
  s = String(s ?? '').replace(/[\u0000-\u001f\u007f<>]/g, '').trim().slice(0, 24);
  return s || 'Player';
}
function smallJson(v) {
  if (!v || typeof v !== 'object' || Array.isArray(v)) return undefined;
  const text = JSON.stringify(v);
  return text.length <= MAX_JSON ? JSON.parse(text) : undefined;
}
const cleanTag = (s, n) => String(s ?? '').replace(/[^\x21-\x7e]/g, '').slice(0, n);

export function createServer(cfg = loadConfig()) {
  const rooms = new Map(); // code -> room
  const ipConns = new Map();
  const ipRooms = new Map(); // ip -> [timestamps]
  let nextId = 1;

  const httpServer = http.createServer((req, res) => {
    if (req.url === '/healthz') { res.writeHead(200, { 'Content-Type': 'text/plain' }).end('ok\n'); return; }
    // a page served from here finds its lobby on the same origin
    if (cfg.staticDir && req.url.split('?')[0].endsWith('/net-config.json')) {
      res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-cache' }).end(JSON.stringify({ ws: '/ws' }));
      return;
    }
    serveStatic(cfg, req, res);
  });
  const wss = new WebSocketServer({ noServer: true, maxPayload: cfg.maxMsgBytes });

  httpServer.on('upgrade', (req, socket, head) => {
    let pathname;
    try { pathname = new URL(req.url, 'http://x').pathname; } catch { socket.destroy(); return; }
    if (pathname !== '/ws') { socket.destroy(); return; }
    const ip = (cfg.trustProxy && String(req.headers['x-forwarded-for'] || '').split(',')[0].trim()) || req.socket.remoteAddress || '?';
    if ((ipConns.get(ip) || 0) >= cfg.maxConnPerIp) {
      socket.write('HTTP/1.1 429 Too Many Requests\r\n\r\n');
      socket.destroy();
      return;
    }
    wss.handleUpgrade(req, socket, head, (ws) => wss.emit('connection', ws, ip));
  });

  function code() {
    for (;;) {
      let c = '';
      for (let i = 0; i < 6; i++) c += CODE_ALPHABET[crypto.randomInt(CODE_ALPHABET.length)];
      if (!rooms.has(c)) return c;
    }
  }

  const send = (c, msg) => { if (c.ws.readyState === 1) c.ws.send(JSON.stringify(msg)); };
  const error = (c, codeStr, msg, re) => send(c, { t: 'error', code: codeStr, msg, re });

  function roomView(room) {
    return {
      t: 'room', code: room.code, host: room.host.id, state: room.state, settings: room.settings, maxMembers: cfg.maxMembers,
      gameId: room.gameId, build: room.build,
      members: room.members.map((m) => ({ id: m.id, name: m.name, slot: m.slot, ready: m.ready, profile: m.profile })),
    };
  }
  function broadcast(room, msg, except) {
    for (const m of room.members) if (m !== except) send(m, msg);
  }
  const touch = (room) => { room.lastActive = Date.now(); };

  function freePort(room) {
    for (let p = 0; p < cfg.maxMembers; p++) if (!room.members.some((m) => m.slot === p)) return p;
    return -1;
  }

  function leave(c, reason) {
    const room = c.room;
    if (!room) return;
    c.room = null;
    room.members = room.members.filter((m) => m !== c);
    c.ready = false;
    touch(room);
    if (!room.members.length) {
      rooms.delete(room.code);
      return;
    }
    broadcast(room, { t: 'peer-left', id: c.id, reason });
    if (room.host === c) {
      // the longest-standing member (lowest id) becomes host, also during a session
      room.host = room.members[0];
      broadcast(room, { t: 'host-changed', host: room.host.id, reason });
    }
    broadcast(room, roomView(room));
  }

  function versionOk(c, room) {
    return c.gameId === room.gameId && c.build === room.build;
  }

  const handlers = {
    hello(c, m) {
      c.name = cleanName(m.name);
      c.gameId = cleanTag(m.gameId, 6);
      c.build = cleanTag(m.build, 80);
      c.hello = true;
      if (m.proto !== PROTO) return error(c, 'proto', `protocol ${m.proto} is not supported (server speaks ${PROTO}); reload the page`);
      send(c, { t: 'welcome', id: c.id, proto: PROTO, iceServers: iceServers(cfg, c.id) });
    },
    name(c, m) {
      c.name = cleanName(m.name);
      if (c.room) broadcast(c.room, roomView(c.room));
    },
    create(c, m) {
      if (!c.gameId || !c.build) return error(c, 'version', 'choose your disc image first (game ID and build are needed)', 'create');
      if (rooms.size >= cfg.maxRooms) return error(c, 'full', 'server has too many rooms', 'create');
      const now = Date.now();
      const recent = (ipRooms.get(c.ip) || []).filter((t) => now - t < 60_000);
      if (recent.length >= cfg.roomsPerMinutePerIp) return error(c, 'rate', 'too many rooms created, wait a minute', 'create');
      recent.push(now);
      ipRooms.set(c.ip, recent);
      leave(c, 'left');
      const room = { code: code(), host: c, members: [c], state: 'lobby', gameId: c.gameId, build: c.build,
                     settings: {}, lastActive: now };
      rooms.set(room.code, room);
      c.room = room;
      c.slot = 0;
      c.ready = false;
      send(c, roomView(room));
    },
    join(c, m) {
      const room = rooms.get(String(m.code || '').toUpperCase().replace(/[^A-Z0-9]/g, ''));
      if (!room) return error(c, 'no-room', 'no room with that code', 'join');
      if (room.members.includes(c)) return send(c, roomView(room));
      if (room.state !== 'lobby') return error(c, 'started', 'that session has already started', 'join');
      if (room.members.length >= cfg.maxMembers) return error(c, 'full', `room is full (${cfg.maxMembers} players)`, 'join');
      if (c.gameId !== room.gameId)
        return error(c, 'version', `disc mismatch: room plays ${room.gameId}, you chose ${c.gameId || 'no disc'}`, 'join');
      if (c.build !== room.build)
        return error(c, 'version', `build mismatch: room runs ${room.build}, you run ${c.build}; everyone needs the same mkdd.wasm`, 'join');
      leave(c, 'left');
      c.room = room;
      c.slot = freePort(room);
      c.ready = false;
      room.members.push(c);
      touch(room);
      broadcast(room, roomView(room));
    },
    leave(c) { leave(c, 'left'); },
    ready(c, m) {
      if (!c.room || c.room.state !== 'lobby') return;
      c.ready = !!m.ready;
      broadcast(c.room, roomView(c.room));
    },
    settings(c, m) {
      const room = c.room;
      if (!room || room.host !== c) return error(c, 'host', 'only the host can change settings', 'settings');
      if (room.state !== 'lobby') return error(c, 'started', 'session already started', 'settings');
      // game: free-form host settings (course, cc, laps, ...), passed through to everyone
      if (m.game !== undefined) {
        const g = smallJson(m.game);
        if (g === undefined) return error(c, 'settings', 'game settings must be a JSON object under 2 KB', 'settings');
        if (JSON.stringify(g) !== JSON.stringify(room.settings))
          for (const mem of room.members) if (mem !== c) mem.ready = false; // they agreed to other settings
        room.settings = g;
      }
      if (m.slots && typeof m.slots === 'object') {
        const want = new Map();
        for (const mem of room.members) {
          const p = m.slots[mem.id];
          want.set(mem, p === undefined ? mem.slot : parseInt(p, 10));
        }
        const used = new Set();
        for (const [, p] of want) {
          if (!(p >= -1 && p < cfg.maxMembers)) return error(c, 'slots', `slots are 0-${cfg.maxMembers - 1} (or -1 to watch)`, 'settings');
          if (p >= 0 && used.has(p)) return error(c, 'slots', 'two players in one slot', 'settings');
          used.add(p);
        }
        for (const [mem, p] of want) {
          if (mem.slot !== p) mem.ready = false;
          mem.slot = p;
        }
      }
      touch(room);
      broadcast(room, roomView(room));
    },
    // per-member choice (characters, kart, ...), free-form
    profile(c, m) {
      const p = smallJson(m.profile);
      if (p === undefined) return error(c, 'profile', 'profile must be a JSON object under 2 KB', 'profile');
      c.profile = p;
      if (c.room) broadcast(c.room, roomView(c.room));
    },
    // relay WebRTC offers/answers/candidates between members of one room
    signal(c, m) {
      const room = c.room;
      if (!room) return;
      const to = room.members.find((x) => x.id === m.to);
      if (!to || to === c) return;
      send(to, { t: 'signal', from: c.id, data: m.data });
    },
    // the host starts the session: the room is locked (no joins, settings fixed)
    start(c) {
      const room = c.room;
      if (!room || room.host !== c) return error(c, 'host', 'only the host can start', 'start');
      if (room.state !== 'lobby') return;
      const players = room.members.filter((x) => x.slot >= 0);
      if (!players.length) return error(c, 'start', 'nobody is in a player slot', 'start');
      const notReady = room.members.filter((x) => x !== c && !x.ready);
      if (notReady.length) return error(c, 'start', `not ready: ${notReady.map((x) => x.name).join(', ')}`, 'start');
      room.state = 'playing';
      touch(room);
      broadcast(room, roomView(room));
    },
    // the host ends the session: back to the lobby (everyone reloads the game)
    end(c) {
      const room = c.room;
      if (!room || room.host !== c) return;
      room.state = 'lobby';
      for (const x of room.members) x.ready = false;
      broadcast(room, { t: 'ended' });
      broadcast(room, roomView(room));
    },
    kick(c, m) {
      const room = c.room;
      if (!room || room.host !== c) return;
      const who = room.members.find((x) => x.id === m.id && x !== c);
      if (!who) return;
      error(who, 'kicked', 'the host removed you from the room');
      leave(who, 'kicked');
    },
    ping(c, m) { send(c, { t: 'pong', ts: m.ts }); },
  };

  wss.on('connection', (ws, ip) => {
    ipConns.set(ip, (ipConns.get(ip) || 0) + 1);
    const c = { id: nextId++, ws, ip, name: 'Player', gameId: '', build: '', room: null, slot: -1, ready: false, profile: {},
                tokens: cfg.msgBurst, lastFill: Date.now() };
    const alive = () => { ws.mkddDead = false; };
    ws.on('pong', alive);
    ws.on('message', (data, isBinary) => {
      alive();
      const now = Date.now();
      c.tokens = Math.min(cfg.msgBurst, c.tokens + (now - c.lastFill) * cfg.msgRate / 1000);
      c.lastFill = now;
      if (c.tokens < 1) {
        error(c, 'rate', 'too many messages');
        ws.close(1008, 'rate limit');
        return;
      }
      c.tokens--;
      if (isBinary) return;
      let m;
      try { m = JSON.parse(String(data)); } catch { return error(c, 'bad', 'malformed message'); }
      if (!m || typeof m.t !== 'string' || !Object.hasOwn(handlers, m.t)) return error(c, 'bad', 'unknown message');
      if (!c.hello && m.t !== 'hello' && m.t !== 'ping') return error(c, 'bad', 'say hello first');
      handlers[m.t](c, m);
    });
    ws.on('close', () => {
      const n = (ipConns.get(ip) || 1) - 1;
      if (n) ipConns.set(ip, n); else ipConns.delete(ip);
      leave(c, 'disconnected');
    });
    ws.on('error', () => {});
  });

  const timer = setInterval(() => {
    for (const client of wss.clients) {
      if (client.mkddDead) { client.terminate(); continue; }
      client.mkddDead = true;
      client.ping();
    }
    const now = Date.now();
    for (const room of rooms.values()) {
      if (now - room.lastActive > cfg.roomIdleMs) {
        for (const m of [...room.members]) { error(m, 'expired', 'room expired'); leave(m, 'expired'); }
        rooms.delete(room.code);
      }
    }
    for (const [ip, ts] of ipRooms) if (!ts.some((t) => now - t < 60_000)) ipRooms.delete(ip);
  }, cfg.heartbeatMs);
  return {
    httpServer, wss, rooms, cfg,
    listen() {
      return new Promise((resolve) => httpServer.listen(cfg.port, cfg.host, () => resolve(httpServer.address())));
    },
    close() {
      clearInterval(timer);
      for (const ws of wss.clients) ws.terminate();
      return new Promise((resolve) => httpServer.close(() => resolve()));
    },
  };
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const srv = createServer();
  const addr = await srv.listen();
  const where = `${addr.address}:${addr.port}`;
  console.log(`mkdd netplay server on ws://${where}/ws` + (srv.cfg.staticDir ? `, serving ${srv.cfg.staticDir} at http://${where}/` : ''));
  console.log(`ICE: ${JSON.stringify(iceServers(srv.cfg, 0).map((s) => s.urls))}`);
  const stop = () => srv.close().then(() => process.exit(0));
  process.on('SIGINT', stop);
  process.on('SIGTERM', stop);
}
