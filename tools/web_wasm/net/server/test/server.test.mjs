import { test, before, after } from 'node:test';
import assert from 'node:assert/strict';
import { WebSocket } from 'ws';
import { createServer, loadConfig, PROTO } from '../server.mjs';

let srv, url;
const events = [];
before(async () => {
  srv = createServer(loadConfig({ PORT: '0', HOST: '127.0.0.1', STUN_URLS: 'stun:stun.example:3478', TURN_URLS: 'turn:turn.example:3478',
                                  TURN_SECRET: 's3cret', ROOMS_PER_MIN: '50', MAX_MSG_BYTES: '4096', MAX_MEMBERS: '4' }),
                     { log: (line) => events.push(line) });
  const a = await srv.listen();
  url = `ws://127.0.0.1:${a.port}/ws`;
});
after(() => srv.close());

class Client {
  constructor() {
    this.msgs = [];
    this.waiters = [];
    this.ws = new WebSocket(url);
    this.ws.on('message', (d) => {
      const m = JSON.parse(String(d));
      this.msgs.push(m);
      this.waiters = this.waiters.filter((w) => !(w.pred(m) && (w.resolve(m), true)));
    });
    this.opened = new Promise((r) => this.ws.on('open', r));
    this.closed = new Promise((r) => this.ws.on('close', (code) => r(code)));
  }
  send(m) { this.ws.send(JSON.stringify(m)); }
  next(pred, what = 'message') {
    const found = this.msgs.find(pred);
    if (found) { this.msgs.splice(this.msgs.indexOf(found), 1); return Promise.resolve(found); }
    return new Promise((resolve, reject) => {
      const t = setTimeout(() => reject(new Error('timeout waiting for ' + what)), 2000);
      this.waiters.push({ pred, resolve: (m) => { clearTimeout(t); this.msgs.splice(this.msgs.indexOf(m), 1); resolve(m); } });
    });
  }
  type(t, extra = () => true) { return this.next((m) => m.t === t && extra(m), t); }
  async hello(name, gameId = 'GM4E01', build = 'build-a') {
    await this.opened;
    this.send({ t: 'hello', name, gameId, build, proto: PROTO });
    this.welcome = await this.type('welcome');
    return this.welcome;
  }
  close() { this.ws.close(); }
}

test('welcome carries the ICE configuration with TURN REST credentials', async () => {
  const c = new Client();
  const w = await c.hello('A');
  assert.equal(w.proto, PROTO);
  assert.deepEqual(w.iceServers[0].urls, ['stun:stun.example:3478']);
  assert.deepEqual(w.iceServers[1].urls, ['turn:turn.example:3478']);
  assert.match(w.iceServers[1].username, new RegExp(`^\\d+:${w.id}$`));
  assert.ok(w.iceServers[1].credential.length > 10);
  c.close();
});

test('create, join, slots, ready, settings, profiles, start', async () => {
  const a = new Client(), b = new Client();
  await a.hello('Alice');
  await b.hello('Bob');
  a.send({ t: 'create' });
  const r1 = await a.type('room');
  assert.match(r1.code, /^[A-HJ-NP-Z2-9]{6}$/);
  assert.equal(r1.host, a.welcome.id);
  b.send({ t: 'join', code: r1.code.toLowerCase() });
  const r2 = await b.type('room', (m) => m.members.length === 2);
  assert.deepEqual(r2.members.map((m) => [m.name, m.slot]), [['Alice', 0], ['Bob', 1]]);
  await a.type('room', (m) => m.members.length === 2);

  // only the host changes settings
  b.send({ t: 'settings', game: { course: 0x21 } });
  assert.equal((await b.type('error')).code, 'host');
  a.send({ t: 'settings', game: { course: 0x21, laps: 3 }, slots: { [a.welcome.id]: 2, [b.welcome.id]: 0 } });
  const r3 = await b.type('room', (m) => m.settings.laps === 3);
  assert.deepEqual(r3.settings, { course: 0x21, laps: 3 });
  assert.deepEqual(r3.members.map((m) => m.slot), [2, 0]);
  a.send({ t: 'settings', slots: { [b.welcome.id]: 2 } });
  assert.equal((await a.type('error')).code, 'slots');
  a.send({ t: 'settings', game: { big: 'x'.repeat(3000) } });
  assert.equal((await a.type('error')).code, 'settings');
  b.send({ t: 'profile', profile: { char1: 1, char2: 2, kart: 5 } });
  const r4 = await a.type('room', (m) => m.members[1].profile.kart === 5);
  assert.deepEqual(r4.members[1].profile, { char1: 1, char2: 2, kart: 5 });

  a.send({ t: 'start' });
  assert.equal((await a.type('error')).code, 'start'); // Bob not ready
  b.send({ t: 'ready', ready: true });
  await a.type('room', (m) => m.members[1].ready);
  a.send({ t: 'start' });
  assert.equal((await b.type('room', (m) => m.state === 'playing')).state, 'playing');

  // a locked room refuses joins
  const c = new Client();
  await c.hello('Carol');
  c.send({ t: 'join', code: r1.code });
  assert.equal((await c.type('error')).code, 'started');
  a.send({ t: 'end' });
  await b.type('ended');
  a.close(); b.close(); c.close();
});

test('version check refuses a different disc or build', async () => {
  const a = new Client(), b = new Client(), c = new Client();
  await a.hello('A', 'GM4E01', 'build-x');
  await b.hello('B', 'GM4P01', 'build-x');
  await c.hello('C', 'GM4E01', 'build-y');
  a.send({ t: 'create' });
  const { code } = await a.type('room');
  b.send({ t: 'join', code });
  const eb = await b.type('error');
  assert.equal(eb.code, 'version');
  assert.match(eb.msg, /GM4E01/);
  c.send({ t: 'join', code });
  assert.match((await c.type('error')).msg, /build mismatch/);
  a.close(); b.close(); c.close();
});

test('rooms hold at most four', async () => {
  const cs = [new Client(), new Client(), new Client(), new Client(), new Client()];
  for (const [i, c] of cs.entries()) await c.hello('P' + i);
  cs[0].send({ t: 'create' });
  const { code } = await cs[0].type('room');
  for (const c of cs.slice(1, 4)) { c.send({ t: 'join', code }); await c.type('room'); }
  cs[4].send({ t: 'join', code });
  assert.equal((await cs[4].type('error')).code, 'full');
  const r = await cs[3].type('room', (m) => m.members.length === 4).catch(() => null);
  void r;
  assert.deepEqual(srv.rooms.get(code).members.map((m) => m.slot), [0, 1, 2, 3]);
  for (const c of cs) c.close();
});

test('signals are relayed within a room only; host leaves -> next member hosts; empty room is removed', async () => {
  const a = new Client(), b = new Client(), x = new Client();
  await a.hello('A'); await b.hello('B'); await x.hello('X');
  a.send({ t: 'create' });
  const { code } = await a.type('room');
  b.send({ t: 'join', code });
  await b.type('room');
  a.send({ t: 'signal', to: b.welcome.id, data: { sdp: 'offer' } });
  const s = await b.type('signal');
  assert.equal(s.from, a.welcome.id);
  assert.deepEqual(s.data, { sdp: 'offer' });
  x.send({ t: 'signal', to: b.welcome.id, data: 'nope' }); // not in the room
  a.close();
  await b.type('peer-left');
  assert.equal((await b.type('host-changed')).host, b.welcome.id);
  const r = await b.type('room', (m) => m.members.length === 1);
  assert.equal(r.host, b.welcome.id);
  b.close();
  await b.closed;
  await new Promise((res) => setTimeout(res, 50));
  assert.equal(srv.rooms.has(code), false);
  assert.equal(b.msgs.filter((m) => m.t === 'signal').length, 0);
  x.close();
});

test('rate and size limits', async () => {
  const big = new Client();
  await big.hello('Big');
  big.send({ t: 'name', name: 'x'.repeat(8000) });
  assert.equal(await big.closed, 1009); // message too big
  const spam = new Client();
  await spam.hello('Spam');
  for (let i = 0; i < 400; i++) spam.send({ t: 'ping', ts: i });
  assert.equal(await spam.closed, 1008);
});

test('names are sanitised and messages need hello', async () => {
  const c = new Client();
  await c.opened;
  c.send({ t: 'create' });
  assert.equal((await c.type('error')).code, 'bad');
  await c.hello('<b>Evil\u0007</b>' + 'y'.repeat(50));
  c.send({ t: 'create' });
  const r = await c.type('room');
  assert.equal(r.members[0].name, 'bEvil/b' + 'y'.repeat(17));
  c.close();
});

test('member status: a member whose game runs or who has no disc cannot be ready, the start is refused; room events are logged', async () => {
  const a = new Client(), b = new Client();
  await a.hello('Ann'); await b.hello('Ben');
  a.send({ t: 'create' });
  const { code } = await a.type('room');
  b.send({ t: 'join', code });
  await a.type('room', (m) => m.members.length === 2);
  b.send({ t: 'status', status: { disc: true, idle: true } });
  const r1 = await a.type('room', (m) => m.members[1].status && m.members[1].status.idle === true);
  assert.deepEqual(r1.members[1].status, { disc: true, idle: true });
  b.send({ t: 'ready', ready: true });
  await a.type('room', (m) => m.members[1].ready);
  // Ben's game starts solo: he is no longer ready and cannot become ready again
  b.send({ t: 'status', status: { disc: true, idle: false, junk: 'x'.repeat(50) } });
  const r2 = await a.type('room', (m) => m.members[1].status.idle === false);
  assert.equal(r2.members[1].ready, false);
  assert.deepEqual(r2.members[1].status, { disc: true, idle: false });
  b.send({ t: 'ready', ready: true });
  assert.equal((await b.type('error')).code, 'ready');
  a.send({ t: 'start' });
  const e = await a.type('error', (m) => m.re === 'start');
  assert.match(e.msg, /Ben \(game already running/);
  b.send({ t: 'status', status: { disc: false, idle: true } });
  await a.type('room', (m) => m.members[1].status.disc === false);
  a.send({ t: 'start' });
  assert.match((await a.type('error', (m) => m.re === 'start')).msg, /Ben \(no disc image chosen\)/);
  assert.equal(srv.rooms.get(code).state, 'lobby');
  // fixed: the start goes through; boot progress is passed on during the session
  b.send({ t: 'status', status: { disc: true, idle: true } });
  b.send({ t: 'ready', ready: true });
  await a.type('room', (m) => m.members[1].ready);
  a.send({ t: 'start' });
  await b.type('room', (m) => m.state === 'playing');
  b.send({ t: 'status', status: { disc: true, idle: false, boot: 'failed', why: 'no <disc>' } });
  const r3 = await a.type('room', (m) => m.members[1].status.boot === 'failed');
  assert.equal(r3.members[1].status.why, 'no disc');
  a.close(); b.close();
  await b.closed;
  await new Promise((res) => setTimeout(res, 50));
  const mine = events.filter((l) => l.includes(code));
  for (const re of [/created by #\d+ "Ann"/, /#\d+ "Ben" joined, 2 members/, /start refused: cannot race: Ben \(game already running/,
                    /session started by #\d+ "Ann": .*"Ben" slot 1/, /"Ben" could not start the game: no disc/, /room closed/])
    assert.ok(mine.some((l) => re.test(l)), `${re} in\n${mine.join('\n')}`);
});
