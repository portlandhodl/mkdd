// Online-play transport for the web port: lobby client and WebRTC data-channel mesh.
//
// Layers:
//  - Lobby: rooms on the lobby server (tools/web_wasm/net/server) over a WebSocket.
//  - Mesh: one RTCPeerConnection per other room member, each with an unreliable +
//    unordered channel and a reliable + ordered channel; RTT per peer, a shared host
//    clock, liveness, peer join/leave events and a generic binary message API.
//  - Bridge (attachModule): the same API for the game's wasm side, see
//    tools/web_wasm/net/net_bridge.h.
// No game protocol lives here: the state-sync netcode (tools/web_wasm/net/NETCODE.md)
// defines its own messages, which this layer only carries. Only lobby messages go to
// the server; game messages go peer-to-peer. Each player reads their own disc.
//
// window.MkddNet = { Lobby, Mesh, KIND, gameIdOf, buildFingerprint, PROTO }
(() => {
  'use strict';
  const PROTO = 1;
  // channel kinds (also the C side's PORT_NET_UNRELIABLE / PORT_NET_RELIABLE / PORT_NET_EVENT)
  const KIND = { UNRELIABLE: 0, RELIABLE: 1, EVENT: 2 };
  // first bytes 0xF0-0xFF are the transport's own; application messages use 0x00-0xEF
  const T_PING = 0xf0, T_PONG = 0xf1, T_RESERVED = 0xf0;
  const LOST_MS = 5000; // no packet for this long: peer shown as lost
  const CLOCK_SAMPLES = 16;
  // bridge events (KIND.EVENT payload: u8 code, u8 0, u16 0, u32 member id)
  const EV = { JOIN: 1, LEAVE: 2, LOST: 3, BACK: 4, START: 5, END: 6, HOST: 7 };

  const hex32 = (v) => (v >>> 0).toString(16).padStart(8, '0');

  // Disc game ID from the header (e.g. GM4E01); only these 6 bytes are read.
  async function gameIdOf(file) {
    const b = new Uint8Array(await file.slice(0, 6).arrayBuffer());
    return String.fromCharCode(...b).replace(/[^\x21-\x7e]/g, '?');
  }

  // Build fingerprint: a 64-bit hash of mkdd.wasm (same on every machine for the same
  // build; crypto.subtle is not used because it needs a secure context).
  async function buildFingerprint(url = 'mkdd.wasm') {
    const buf = await (await fetch(url, { cache: 'no-cache' })).arrayBuffer();
    const n = buf.byteLength & ~3, w = new Uint32Array(buf, 0, n >> 2);
    let a = 0x811c9dc5, b = 0x9e3779b9;
    for (let i = 0; i < w.length; i++) {
      a = Math.imul(a ^ w[i], 0x01000193);
      b = Math.imul(b ^ w[i] ^ (b >>> 15), 0x85ebca6b);
    }
    const tail = new Uint8Array(buf, n);
    for (const x of tail) a = Math.imul(a ^ x, 0x01000193);
    return `wasm-${hex32(a)}${hex32(b)}-${buf.byteLength}`;
  }

  class Emitter {
    constructor() { this.handlers = {}; }
    on(ev, fn) { (this.handlers[ev] ||= []).push(fn); return this; }
    off(ev, fn) { this.handlers[ev] = (this.handlers[ev] || []).filter((f) => f !== fn); }
    emit(ev, ...a) { for (const fn of this.handlers[ev] || []) try { fn(...a); } catch (e) { console.error(e); } }
  }

  // ---- lobby (WebSocket to tools/web_wasm/net/server) ----

  class Lobby extends Emitter {
    constructor() {
      super();
      this.ws = null; this.id = 0; this.room = null; this.iceServers = []; this.rtt = 0;
    }
    connect(url, hello) {
      return new Promise((resolve, reject) => {
        let settled = false;
        const ws = new WebSocket(url);
        this.ws = ws;
        ws.onopen = () => ws.send(JSON.stringify({ t: 'hello', proto: PROTO, ...hello }));
        ws.onerror = () => { if (!settled) { settled = true; reject(new Error('cannot reach the lobby server at ' + url)); } };
        ws.onclose = () => {
          clearInterval(this.pingTimer);
          if (!settled) { settled = true; reject(new Error('lobby server closed the connection')); }
          this.emit('close');
        };
        ws.onmessage = (ev) => {
          let m;
          try { m = JSON.parse(ev.data); } catch { return; }
          if (m.t === 'welcome') {
            this.id = m.id; this.iceServers = m.iceServers || [];
            settled = true; resolve(m);
          } else if (m.t === 'room') {
            this.room = m;
          } else if (m.t === 'pong') {
            this.rtt = performance.now() - m.ts;
          } else if (m.t === 'error' && !settled) {
            settled = true; reject(new Error(m.msg));
          }
          this.emit(m.t, m);
          this.emit('message', m);
        };
        this.pingTimer = setInterval(() => this.send({ t: 'ping', ts: performance.now() }), 5000);
      });
    }
    send(m) { if (this.ws && this.ws.readyState === 1) this.ws.send(JSON.stringify(m)); }
    create() { this.send({ t: 'create' }); }
    join(code) { this.send({ t: 'join', code }); }
    leave() { this.send({ t: 'leave' }); this.room = null; }
    setReady(ready) { this.send({ t: 'ready', ready }); }
    setName(name) { this.send({ t: 'name', name }); }
    setProfile(profile) { this.send({ t: 'profile', profile }); }
    // own state for the room: { disc, idle } (can race), { boot, why } during a session
    setStatus(status) { this.send({ t: 'status', status }); }
    // host: { game: {...settings}, slots: {memberId: slot} }
    setSettings(s) { this.send({ t: 'settings', ...s }); }
    start() { this.send({ t: 'start' }); }
    end() { this.send({ t: 'end' }); }
    kick(id) { this.send({ t: 'kick', id }); }
    signal(to, data) { this.send({ t: 'signal', to, data }); }
    get isHost() { return !!this.room && this.room.host === this.id; }
    me() { return this.room && this.room.members.find((m) => m.id === this.id); }
    close() { if (this.ws) this.ws.close(); }
  }

  // ---- one WebRTC peer ----

  class Peer {
    constructor(mesh, member) {
      this.mesh = mesh;
      this.id = member.id;
      this.name = member.name;
      this.slot = member.slot;
      this.profile = member.profile || {};
      this.rtt = 0; this.rttMax = 0; this.rtts = [];
      this.lastRecv = performance.now();
      this.lost = false; this.left = false; this.joined = false;
      this.stats = { pktIn: 0, pktOut: 0, dropped: 0, bytesIn: 0, bytesOut: 0 };
      this.pingSeq = 0;
      this.pendingCandidates = [];
      const pc = new RTCPeerConnection({ iceServers: mesh.lobby.iceServers });
      this.pc = pc;
      // negotiated channels: both sides create them, no ondatachannel race
      this.rel = pc.createDataChannel('rel', { negotiated: true, id: 0, ordered: true });
      this.unrel = pc.createDataChannel('unrel', { negotiated: true, id: 1, ordered: false, maxRetransmits: 0 });
      for (const ch of [this.rel, this.unrel]) {
        ch.binaryType = 'arraybuffer';
        ch.onopen = () => mesh.peerChanged(this);
        ch.onclose = () => mesh.peerChanged(this);
      }
      this.rel.onmessage = (ev) => mesh.receive(this, KIND.RELIABLE, ev.data);
      this.unrel.onmessage = (ev) => mesh.receive(this, KIND.UNRELIABLE, ev.data);
      pc.onicecandidate = (ev) => { if (ev.candidate) mesh.lobby.signal(this.id, { ice: ev.candidate.toJSON() }); };
      pc.onconnectionstatechange = () => mesh.peerChanged(this);
      if (mesh.lobby.id < this.id) this.offer(); // lower id offers: no glare
    }
    async offer() {
      try {
        await this.pc.setLocalDescription(await this.pc.createOffer());
        this.mesh.lobby.signal(this.id, { sdp: this.pc.localDescription.toJSON() });
      } catch (e) { this.mesh.log(`webrtc ${this.name}: ${e.message}`); }
    }
    async onSignal(data) {
      try {
        if (data.sdp) {
          await this.pc.setRemoteDescription(data.sdp);
          for (const c of this.pendingCandidates.splice(0)) await this.pc.addIceCandidate(c).catch(() => {});
          if (data.sdp.type === 'offer') {
            await this.pc.setLocalDescription(await this.pc.createAnswer());
            this.mesh.lobby.signal(this.id, { sdp: this.pc.localDescription.toJSON() });
          }
        } else if (data.ice) {
          if (this.pc.remoteDescription) await this.pc.addIceCandidate(data.ice).catch(() => {});
          else this.pendingCandidates.push(data.ice);
        }
      } catch (e) {
        this.mesh.log(`webrtc ${this.name}: ${e.message}`);
      }
    }
    get open() { return this.rel.readyState === 'open' && this.unrel.readyState === 'open'; }
    close() { try { this.pc.close(); } catch {} }
    addRtt(ms) {
      this.rtts.push(ms);
      if (this.rtts.length > 20) this.rtts.shift();
      this.rtt = this.rtt ? this.rtt * 0.8 + ms * 0.2 : ms;
      this.rttMax = Math.max(...this.rtts);
    }
  }

  // ---- the mesh ----
  //
  // API:
  //   send(to, kind, data)      to: member id | Peer | 'all' (or -1); kind: KIND.UNRELIABLE /
  //                             KIND.RELIABLE; data: Uint8Array | ArrayBuffer, first byte
  //                             0x00-0xEF (the message type). Returns the number of peers.
  //   on('message', (peer, kind, Uint8Array) => ...)
  //   on('peer-join', peer)     both channels open (first time)
  //   on('peer-leave', peer, reason)   left the room / kicked / connection closed
  //   on('peer-lost', peer) / on('peer-back', peer)   silent for 5 s or ICE disconnected, and back
  //   on('start', session) / on('end')   the host started / ended the session (see sessionInfo)
  //   on('host', hostId)        a new host (the old one left)
  //   on('peers')               anything in the member list or a link changed (UI)
  //   hostTime()                the host's clock in ms (performance.now() on the host),
  //                             estimated from pings: offset = median of the last 16
  //                             samples of t_host + rtt/2 - t_local; clockSynced() says
  //                             whether there are samples yet (always true on the host)
  //   peers: Map(id -> Peer {id, name, slot, profile, rtt, rttMax, open, lost, left, stats})
  // Transport packets (first byte 0xF0-0xFF, unreliable channel):
  //   PING 0xF0, u8 0, u16 0, u32 seq, f64 t_sender
  //   PONG 0xF1, u8 0, u16 0, u32 seq, f64 t_sender (echoed), f64 t_responder
  // Simulated network for tests (opts.sim or ?netlag=&netjitter=&netloss=): latency
  // (+ jitter) on both channels, loss on the unreliable one; reliable order is kept.
  class Mesh extends Emitter {
    constructor(lobby, opts = {}) {
      super();
      this.lobby = lobby;
      this.peers = new Map();
      this.frozen = false; // a session runs: membership changes are leaves, not rebuilds
      this.session = null;
      this.sim = { lag: 0, jitter: 0, loss: 0, ...(opts.sim || {}) };
      this.logFn = opts.log || ((t) => console.log(t));
      this.now = opts.now || (() => performance.now()); // the clock shared through hostTime()
      this.clockSamples = [];
      this.clockOffset = 0;
      this.hostId = lobby.room ? lobby.room.host : 0;
      this.listeners = [];
      const listen = (ev, fn) => { lobby.on(ev, fn); this.listeners.push([ev, fn]); };
      listen('signal', (m) => { const p = this.peers.get(m.from); if (p) p.onSignal(m.data); });
      listen('room', (room) => this.syncRoom(room));
      listen('peer-left', (m) => {
        const p = this.peers.get(m.id);
        if (!p) return;
        if (this.frozen) { p.left = true; this.peerChanged(p); }
        else this.removePeer(p);
        this.emit('peer-leave', p, m.reason || 'left');
      });
      listen('ended', () => this.endSession());
      this.pingTimer = setInterval(() => this.pingAll(), 250);
      this.liveTimer = setInterval(() => this.checkAlive(), 250);
      if (lobby.room) this.syncRoom(lobby.room);
    }
    log(t) { this.logFn('[net] ' + t); }

    syncRoom(room) {
      if (!room || this.closed) return;
      if (room.host !== this.hostId) {
        const first = !this.hostId;
        this.hostId = room.host;
        this.clockSamples = [];
        this.clockOffset = 0;
        if (!first) this.emit('host', room.host);
      }
      const ids = new Set(room.members.map((m) => m.id));
      for (const m of room.members) {
        if (m.id === this.lobby.id) continue;
        let p = this.peers.get(m.id);
        if (!p && !this.frozen) {
          p = new Peer(this, m);
          this.peers.set(m.id, p);
        }
        if (p) { p.name = m.name; p.slot = m.slot; p.profile = m.profile || {}; }
      }
      if (!this.frozen)
        for (const [id, p] of this.peers) if (!ids.has(id)) this.removePeer(p);
      if (room.state === 'playing' && !this.session) this.startSession(room);
      this.emit('peers');
    }
    removePeer(p) {
      p.close();
      this.peers.delete(p.id);
      this.emit('peers');
    }

    startSession(room) {
      this.frozen = true;
      this.session = {
        code: room.code, host: room.host, self: this.lobby.id, settings: room.settings || {},
        members: room.members.map((m) => ({ id: m.id, name: m.name, slot: m.slot, profile: m.profile || {} })),
      };
      this.log(`session start: ${this.session.members.map((m) => `${m.name}${m.slot >= 0 ? ' (slot ' + m.slot + ')' : ''}`).join(', ')}`);
      this.emit('start', this.session);
    }
    endSession() {
      if (!this.session) return;
      this.session = null;
      this.frozen = false;
      for (const p of [...this.peers.values()]) if (p.left) this.removePeer(p);
      this.emit('end');
      // (the room view saying 'lobby' comes after 'ended': the last one still says 'playing',
      // which would start the session again)
      if (this.lobby.room) this.syncRoom({ ...this.lobby.room, state: 'lobby' });
    }

    peerChanged(p) {
      const wasLost = p.lost;
      const st = p.pc.connectionState;
      if (p.open && !p.joined) { p.joined = true; p.lastRecv = performance.now(); this.emit('peer-join', p); }
      p.lost = p.joined && (p.left || st === 'failed' || st === 'closed' || st === 'disconnected' || !p.open);
      if (p.lost !== wasLost) {
        if (p.lost) this.log(`connection to ${p.name} lost (${p.left ? 'left the room' : st})`);
        this.emit(p.lost ? 'peer-lost' : 'peer-back', p);
      }
      this.emit('peers');
    }
    checkAlive() {
      const now = performance.now();
      for (const p of this.peers.values())
        if (p.joined && !p.left && !p.lost && now - p.lastRecv > LOST_MS) {
          p.lost = true;
          this.log(`no packets from ${p.name} for ${LOST_MS / 1000}s`);
          this.emit('peer-lost', p);
          this.emit('peers');
        }
    }
    allConnected() { return [...this.peers.values()].every((p) => p.open); }

    // -- sending --
    send(to, kind, data) {
      const bytes = data instanceof Uint8Array ? data : new Uint8Array(data);
      if (!bytes.length || bytes[0] >= T_RESERVED) throw new Error('message type must be 0x00-0xEF');
      const buf = bytes.byteOffset === 0 && bytes.byteLength === bytes.buffer.byteLength ? bytes.buffer : bytes.slice().buffer;
      const targets = to === 'all' || to === -1 ? [...this.peers.values()] : [to instanceof Peer ? to : this.peers.get(to)];
      let n = 0;
      for (const p of targets) {
        if (!p || p.left) continue;
        const ch = kind === KIND.RELIABLE ? p.rel : p.unrel;
        if (ch.readyState !== 'open') continue;
        this.sendRaw(p, ch, buf, kind !== KIND.RELIABLE);
        n++;
      }
      return n;
    }
    sendRaw(p, ch, data, unreliable) {
      const { lag, jitter, loss } = this.sim;
      if (unreliable && loss > 0 && Math.random() < loss) { p.stats.dropped++; return; }
      const go = () => { try { if (ch.readyState === 'open') ch.send(data); } catch {} };
      const d = lag + (jitter ? (Math.random() * 2 - 1) * jitter : 0);
      if (!unreliable && (d > 0 || (p.relQueue && p.relQueue.length))) {
        // reliable traffic keeps its order under simulated lag: one queue per peer
        const q = (p.relQueue ||= []);
        q.push({ at: Math.max(performance.now() + d, q.length ? q[q.length - 1].at : 0), go });
        const pump = () => {
          while (q.length && q[0].at <= performance.now() + 0.5) q.shift().go();
          p.relTimer = q.length ? setTimeout(pump, Math.max(0, q[0].at - performance.now())) : 0;
        };
        if (!p.relTimer) p.relTimer = setTimeout(pump, Math.max(0, q[0].at - performance.now()));
      } else if (d > 0) setTimeout(go, d);
      else go();
      p.stats.pktOut++;
      p.stats.bytesOut += data.byteLength;
    }

    // -- receiving --
    receive(p, kind, data) {
      if (!(data instanceof ArrayBuffer) || !data.byteLength) return;
      p.lastRecv = performance.now();
      p.stats.pktIn++;
      p.stats.bytesIn += data.byteLength;
      if (p.lost && !p.left && p.open) this.peerChanged(p);
      const bytes = new Uint8Array(data);
      if (bytes[0] >= T_RESERVED) {
        if (kind === KIND.UNRELIABLE) this.transport(p, bytes);
        return;
      }
      this.emit('message', p, kind, bytes);
    }
    transport(p, b) {
      const v = new DataView(b.buffer, b.byteOffset, b.byteLength);
      if (b[0] === T_PING && b.length >= 16) {
        const r = new ArrayBuffer(24), w = new DataView(r);
        w.setUint8(0, T_PONG); w.setUint32(4, v.getUint32(4, true), true);
        w.setFloat64(8, v.getFloat64(8, true), true); w.setFloat64(16, this.now(), true);
        this.sendRaw(p, p.unrel, r, true);
      } else if (b[0] === T_PONG && b.length >= 24) {
        const now = this.now(), rtt = now - v.getFloat64(8, true);
        p.addRtt(rtt);
        if (p.id === this.hostId) {
          this.clockSamples.push(v.getFloat64(16, true) + rtt / 2 - now);
          if (this.clockSamples.length > CLOCK_SAMPLES) this.clockSamples.shift();
          const sorted = [...this.clockSamples].sort((x, y) => x - y);
          this.clockOffset = sorted[sorted.length >> 1];
        }
      }
    }
    pingAll() {
      for (const p of this.peers.values()) {
        if (p.unrel.readyState !== 'open' || p.left) continue;
        const b = new ArrayBuffer(16), v = new DataView(b);
        v.setUint8(0, T_PING); v.setUint32(4, ++p.pingSeq, true); v.setFloat64(8, this.now(), true);
        this.sendRaw(p, p.unrel, b, true);
      }
    }

    // -- clock --
    get isHost() { return this.hostId === this.lobby.id; }
    hostTime() { return this.now() + (this.isHost ? 0 : this.clockOffset); }
    clockSynced() { return this.isHost || this.clockSamples.length > 0; }
    maxRtt() { let m = 0; for (const p of this.peers.values()) m = Math.max(m, p.rttMax); return m; }

    stats() {
      return {
        self: this.lobby.id, host: this.hostId, session: !!this.session, clockOffset: this.clockOffset, clockSamples: this.clockSamples.length,
        peers: [...this.peers.values()].map((p) => ({ id: p.id, name: p.name, slot: p.slot, rtt: Math.round(p.rtt), open: p.open,
                                                       lost: p.lost, left: p.left, ...p.stats })),
      };
    }

    // -- wasm bridge (tools/web_wasm/net/net_bridge.c) --
    // Messages for the game are queued here and pulled by port_net_recv(); peer and
    // session changes are queued as KIND.EVENT messages. port_net_send() lands in send().
    attachModule(module) {
      const queue = [];
      const MAX_QUEUE = 4096;
      const push = (from, kind, bytes) => {
        if (queue.length >= MAX_QUEUE) { // drop the oldest unreliable message, else refuse
          const i = queue.findIndex((q) => q.kind === KIND.UNRELIABLE);
          if (i < 0) { this.log('bridge queue full: dropping a reliable message'); return; }
          queue.splice(i, 1);
        }
        queue.push({ from, kind, bytes });
      };
      const event = (code, id) => {
        const b = new Uint8Array(8);
        b[0] = code; new DataView(b.buffer).setUint32(4, id >>> 0, true);
        push(id, KIND.EVENT, b);
      };
      this.on('message', (p, kind, bytes) => push(p.id, kind, bytes.slice()));
      this.on('peer-join', (p) => event(EV.JOIN, p.id));
      this.on('peer-leave', (p) => event(EV.LEAVE, p.id));
      this.on('peer-lost', (p) => event(EV.LOST, p.id));
      this.on('peer-back', (p) => event(EV.BACK, p.id));
      this.on('start', () => event(EV.START, this.lobby.id));
      this.on('end', () => event(EV.END, this.lobby.id));
      this.on('host', (id) => event(EV.HOST, id));
      if (this.session) event(EV.START, this.lobby.id);
      module.netBridge = {
        mesh: this,
        queue,
        send: (to, kind, bytes) => { try { return this.send(to, kind, bytes); } catch (e) { return -1; } },
        recv: () => queue.shift(),
        peek: () => queue[0],
        sessionJson: () => JSON.stringify(this.session || null),
        member: (id) => (id === this.lobby.id ? { id, slot: this.session ? (this.session.members.find((m) => m.id === id) || {}).slot : -1, rtt: 0 } : this.peers.get(id)),
      };
      return module.netBridge;
    }

    close() {
      this.closed = true;
      clearInterval(this.pingTimer); clearInterval(this.liveTimer);
      for (const [ev, fn] of this.listeners) this.lobby.off(ev, fn);
      for (const p of this.peers.values()) p.close();
      this.peers.clear();
    }
  }

  window.MkddNet = { Lobby, Mesh, Peer, KIND, EV, gameIdOf, buildFingerprint, PROTO };
})();
