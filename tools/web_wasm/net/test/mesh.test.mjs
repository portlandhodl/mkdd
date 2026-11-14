// Transport test without browsers: web/net.js Meshes over mock WebRTC data channels
// (lossy, delayed), driven by a mock lobby. Checks the message API on both channel
// kinds, reserved types, join/leave/start events, host clock estimation with skewed
// clocks, simulated loss, and the wasm bridge queue (Module.netBridge).
//
//   node --test tools/web_wasm/net/test/mesh.test.mjs
import { test } from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

globalThis.window = globalThis;
const here = path.dirname(fileURLToPath(import.meta.url));
await import(pathToFileURL(path.join(here, '../../web/net.js')).href);
const { Mesh, KIND, EV } = globalThis.MkddNet;

// ---- mock WebRTC: channels link when both sides have both descriptions ----
const link = { loss: 0, lag: 0 };
class MockChannel {
  constructor(label, opts) { this.label = label; this.opts = opts; this.readyState = 'connecting'; this.peer = null; }
  send(data) {
    if (this.readyState !== 'open' || !this.peer) return;
    const copy = data.slice(0);
    const unreliable = this.opts.maxRetransmits === 0;
    if (unreliable && Math.random() < link.loss) return;
    const target = this.peer;
    const deliver = () => { if (target.readyState === 'open' && target.onmessage) target.onmessage({ data: copy }); };
    const d = link.lag + (unreliable ? Math.random() * link.lag : 0);
    if (!unreliable) { this.nextAt = Math.max(Date.now() + d, this.nextAt || 0); setTimeout(deliver, this.nextAt - Date.now()); }
    else setTimeout(deliver, d);
  }
  close() { this.readyState = 'closed'; this.onclose && this.onclose(); }
}
let pcId = 0;
const pcs = new Map();
class MockPC {
  constructor() { this.id = ++pcId; this.channels = {}; this.connectionState = 'new'; this.remoteDescription = null; this.localDescription = null; }
  createDataChannel(label, opts) { return (this.channels[label] = new MockChannel(label, opts)); }
  async createOffer() { return { type: 'offer', sdp: 'pc' + this.id }; }
  async createAnswer() { return { type: 'answer', sdp: 'pc' + this.id }; }
  async setLocalDescription(d) { this.localDescription = { ...d, toJSON: () => ({ type: d.type, sdp: d.sdp }) }; pcs.set(d.sdp, this); this.link(); }
  async setRemoteDescription(d) { this.remoteDescription = d; this.link(); }
  async addIceCandidate() {}
  link() {
    const other = this.remoteDescription && pcs.get(this.remoteDescription.sdp);
    if (!this.localDescription || !other || !other.localDescription || !other.remoteDescription) return;
    for (const [pc, o] of [[this, other], [other, this]]) {
      pc.connectionState = 'connected';
      for (const [label, ch] of Object.entries(pc.channels)) { ch.peer = o.channels[label]; ch.readyState = 'open'; }
    }
    for (const pc of [this, other]) {
      for (const ch of Object.values(pc.channels)) ch.onopen && ch.onopen();
      pc.onconnectionstatechange && pc.onconnectionstatechange();
    }
  }
  close() {
    this.connectionState = 'closed';
    for (const ch of Object.values(this.channels)) { ch.readyState = 'closed'; if (ch.peer) ch.peer.close(); }
  }
}
globalThis.RTCPeerConnection = MockPC;

class MockLobby {
  constructor(hub, id) { this.hub = hub; this.id = id; this.handlers = {}; this.iceServers = []; this.room = null; }
  on(ev, fn) { (this.handlers[ev] ||= []).push(fn); }
  off(ev, fn) { this.handlers[ev] = (this.handlers[ev] || []).filter((f) => f !== fn); }
  emit(ev, m) { for (const fn of this.handlers[ev] || []) fn(m); }
  signal(to, data) { const t = this.hub.get(to); setTimeout(() => t.emit('signal', { from: this.id, data }), 1); }
  get isHost() { return this.room.host === this.id; }
}
const delay = (ms) => new Promise((r) => setTimeout(r, ms));
async function until(check, what, ms = 3000) {
  const t0 = Date.now();
  while (!check()) { if (Date.now() - t0 > ms) throw new Error('timed out: ' + what); await delay(5); }
}

async function room(n, { skews = [], sim } = {}) {
  const hub = new Map();
  const members = [...Array(n)].map((_, i) => ({ id: i + 1, name: 'P' + (i + 1), slot: i, ready: true, profile: { kart: i } }));
  const r = { code: 'TEST00', host: 1, state: 'lobby', settings: { course: 0x24 }, members };
  const nodes = members.map((m, i) => {
    const lobby = new MockLobby(hub, m.id);
    hub.set(m.id, lobby);
    lobby.room = r;
    const skew = skews[i] || 0;
    const mesh = new Mesh(lobby, { log: () => {}, sim, now: () => performance.now() + skew });
    const got = [];
    mesh.on('message', (p, kind, data) => got.push({ from: p.id, kind, data: [...data] }));
    return { lobby, mesh, got };
  });
  await until(() => nodes.every((x) => x.mesh.allConnected()), 'mesh');
  const setRoom = (patch) => { Object.assign(r, patch); for (const x of nodes) { x.lobby.room = { ...r }; x.lobby.emit('room', x.lobby.room); } };
  return { nodes, setRoom, close: () => nodes.forEach((x) => x.mesh.close()) };
}

test('messages on both channels, to one peer and to all; reserved types refused', async () => {
  const { nodes, close } = await room(3);
  const [a, b, c] = nodes;
  assert.equal(a.mesh.send(2, KIND.RELIABLE, new Uint8Array([0x40, 1, 2, 3])), 1);
  assert.equal(a.mesh.send('all', KIND.UNRELIABLE, new Uint8Array([0x01, 9])), 2);
  assert.throws(() => a.mesh.send('all', KIND.RELIABLE, new Uint8Array([0xf0])));
  await until(() => b.got.length === 2 && c.got.length === 1, 'delivery');
  assert.deepEqual(b.got.find((m) => m.kind === KIND.RELIABLE), { from: 1, kind: KIND.RELIABLE, data: [0x40, 1, 2, 3] });
  assert.deepEqual(c.got[0], { from: 1, kind: KIND.UNRELIABLE, data: [0x01, 9] });
  close();
});

test('reliable order and unreliable loss under a simulated bad network', async () => {
  const { nodes, close } = await room(2, { sim: { lag: 20, jitter: 10, loss: 0.3 } });
  const [a, b] = nodes;
  for (let i = 0; i < 100; i++) { a.mesh.send(2, KIND.RELIABLE, new Uint8Array([0x10, i])); a.mesh.send(2, KIND.UNRELIABLE, new Uint8Array([0x01, i])); }
  await until(() => b.got.filter((m) => m.kind === KIND.RELIABLE).length === 100, 'reliable', 5000);
  await delay(100);
  assert.deepEqual(b.got.filter((m) => m.kind === KIND.RELIABLE).map((m) => m.data[1]), [...Array(100).keys()]);
  const unrel = b.got.filter((m) => m.kind === KIND.UNRELIABLE).length;
  assert.ok(unrel > 40 && unrel < 95, `unreliable delivered ${unrel}/100`);
  assert.ok(a.mesh.peers.get(2).stats.dropped > 5);
  close();
});

test('host clock: peers with skewed clocks agree on the host time', async () => {
  const { nodes, close } = await room(3, { skews: [0, 123456, -98765] });
  await until(() => nodes.every((x) => x.mesh.clockSynced() && x.mesh.clockSamples.length >= 4 || x.mesh.isHost), 'clock samples', 5000);
  const t = nodes.map((x) => x.mesh.hostTime());
  assert.ok(Math.abs(t[1] - t[0]) < 15 && Math.abs(t[2] - t[0]) < 15, `host times ${t}`);
  assert.ok(Math.abs(nodes[1].mesh.clockOffset + 123456) < 15);
  close();
});

test('session start, peer leave events and the wasm bridge queue', async () => {
  const { nodes, setRoom, close } = await room(3);
  const [a, b, c] = nodes;
  const module = {};
  const bridge = b.mesh.attachModule(module);
  const starts = [];
  for (const x of nodes) x.mesh.on('start', (s) => starts.push(s));
  setRoom({ state: 'playing' });
  assert.equal(starts.length, 3);
  assert.deepEqual(starts[1].members.map((m) => [m.id, m.slot, m.profile.kart]), [[1, 0, 0], [2, 1, 1], [3, 2, 2]]);
  assert.equal(starts[1].self, 2);
  assert.deepEqual(JSON.parse(bridge.sessionJson()).settings, { course: 0x24 });
  a.mesh.send(2, KIND.UNRELIABLE, new Uint8Array([0x01, 7, 7]));
  await until(() => bridge.queue.some((q) => q.kind === KIND.UNRELIABLE), 'bridged message');
  // C side sends through the bridge
  assert.equal(module.netBridge.send(-1, KIND.RELIABLE, new Uint8Array([0x41])), 2);
  await until(() => a.got.length && c.got.length, 'bridge send');
  // c leaves during the session
  b.lobby.emit('peer-left', { id: 3, reason: 'disconnected' });
  const q = [];
  for (let m; (m = bridge.recv());) q.push(m);
  const events = q.filter((m) => m.kind === KIND.EVENT).map((m) => [m.bytes[0], new DataView(m.bytes.buffer).getUint32(4, true)]);
  assert.deepEqual(events.find((e) => e[0] === EV.START), [EV.START, 2]);
  assert.deepEqual(events.find((e) => e[0] === EV.LEAVE), [EV.LEAVE, 3]);
  assert.deepEqual(q.find((m) => m.kind === KIND.UNRELIABLE), { from: 1, kind: KIND.UNRELIABLE, bytes: new Uint8Array([0x01, 7, 7]) });
  assert.equal(b.mesh.peers.get(3).left, true); // kept (frozen) during the session
  close();
});
