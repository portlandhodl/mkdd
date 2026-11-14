// End-to-end race test: the lobby server plus two headless Chrome instances that each boot the game
// into the same online race (NETCODE.md phase 1) and drive their own kart.
//
//   MKDD_ISO=/path/GM4E01.iso node tools/web_wasm/net/test/e2e-race.mjs
//   NET_LAG=50 NET_JITTER=10 NET_LOSS=0.05 MKDD_ISO=... node tools/web_wasm/net/test/e2e-race.mjs
//
// Serves the web build (MKDD_WWW, default build/web_wasm/web-netweb) from the lobby server, opens
// Alice (host) and Bob, creates and joins a room through the panel, picks the race (Luigi Circuit,
// 150cc, 3 laps) and Bob's drivers and kart, starts the session; both pages boot the game
// (MKDD_NET_ONLINE=1) straight into the race. A small in-page bot then drives each player's own kart
// with KeyboardEvents (X = accelerate, arrows = steer) toward the course ahead
// (Module._port_net_debug_kart): for the first second after GO the two karts steer toward each other
// so they bump, Bob weaves a little. Canvas screenshots are taken from both browsers at RACE_SHOTS
// seconds after GO, and the replica error that each game measures (Module.netStatsLog: the remote
// kart's replica at the sender's shared-clock frame vs the sender's own state) is summarised.
//
// Env: RACE_SECONDS (default 60), RACE_SHOTS (default "3,10,20,35,50"), NET_LAG / NET_JITTER (ms) and
// NET_LOSS (0..1) for the simulated network, MKDD_NET_OUT (default build/web_wasm/net-race), ports
// MKDD_NET_PORT (18610) and Chrome debug 19610/19611. Headless Chrome renders with SwiftShader:
// expect a low frame rate, worse with both instances on one loaded machine.
import { spawn } from 'node:child_process';
import { mkdir, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createServer, loadConfig } from '../server/server.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../../../..');
const www = path.resolve(process.env.MKDD_WWW || path.join(root, 'build/web_wasm/web-netweb'));
const out = path.resolve(process.env.MKDD_NET_OUT || path.join(root, 'build/web_wasm/net-race'));
const iso = process.env.MKDD_ISO;
const port = Number(process.env.MKDD_NET_PORT || 18610);
const raceSeconds = Number(process.env.RACE_SECONDS || 60);
const shots = (process.env.RACE_SHOTS || '3,10,20,35,50').split(',').filter(Boolean).map(Number).sort((a, b) => a - b);
if (!iso) { console.error('set MKDD_ISO to your disc image'); process.exit(2); }
await mkdir(out, { recursive: true });

const srv = createServer(loadConfig({ PORT: String(port), HOST: '127.0.0.1', STATIC_DIR: www, STUN_URLS: '' }));
await srv.listen();
const delay = (ms) => new Promise((r) => setTimeout(r, ms));
const browsers = [];
process.on('exit', () => { for (const b of browsers) b.kill(); });

async function waitFor(check, what, timeout = 30_000) {
  const t0 = Date.now();
  let last;
  while (Date.now() - t0 < timeout) {
    try { const v = await check(); if (v) return v; } catch (e) { last = e; }
    await delay(200);
  }
  throw new Error(`timed out waiting for ${what}${last ? ' (' + last.message + ')' : ''}`);
}

async function launch(name, debugPort) {
  const b = spawn(process.env.CHROME_BIN || 'google-chrome', [
    '--headless=new', '--no-sandbox', '--disable-dev-shm-usage', '--enable-unsafe-swiftshader', '--use-angle=swiftshader',
    '--disable-background-timer-throttling', '--disable-renderer-backgrounding', '--disable-backgrounding-occluded-windows',
    '--disable-features=WebRtcHideLocalIpsWithMdns', '--autoplay-policy=no-user-gesture-required',
    `--remote-debugging-port=${debugPort}`, `--user-data-dir=${path.join(out, 'profile-' + name)}`, '--window-size=860,900', 'about:blank',
  ], { stdio: 'ignore' });
  browsers.push(b);
  const target = await waitFor(async () => {
    const list = await (await fetch(`http://127.0.0.1:${debugPort}/json/list`)).json();
    return list.find((t) => t.type === 'page');
  }, 'chrome ' + name);
  const ws = new WebSocket(target.webSocketDebuggerUrl);
  await new Promise((r, j) => { ws.onopen = r; ws.onerror = j; });
  let nextId = 1;
  const pending = new Map(), errors = [];
  ws.onmessage = (ev) => {
    const m = JSON.parse(String(ev.data));
    if (m.id && pending.has(m.id)) {
      const p = pending.get(m.id); pending.delete(m.id);
      m.error ? p.reject(new Error(m.error.message)) : p.resolve(m.result);
    } else if (m.method === 'Runtime.exceptionThrown') {
      errors.push('EXCEPTION ' + (m.params.exceptionDetails.exception?.description || m.params.exceptionDetails.text));
    }
  };
  const send = (method, params = {}) => new Promise((resolve, reject) => {
    const id = nextId++; pending.set(id, { resolve, reject });
    ws.send(JSON.stringify({ id, method, params }));
  });
  const evaluate = async (expr) => {
    const r = await send('Runtime.evaluate', { expression: expr, awaitPromise: true, returnByValue: true });
    if (r.exceptionDetails) throw new Error(r.exceptionDetails.exception?.description || r.exceptionDetails.text);
    return r.result.value;
  };
  await send('Page.enable'); await send('Runtime.enable'); await send('DOM.enable');
  return { name, send, evaluate, errors };
}

const q = new URLSearchParams({ netserver: `ws://127.0.0.1:${port}/ws` });
for (const e of ['MKDD_GFX_SCALE=1', 'MKDD_GFX_MSAA=0', 'MKDD_GFX_ANISO=1', `MKDD_GFX_SHADER_WARM=${process.env.RACE_WARM || 0}`]) q.append('env', e);
for (const [k, e] of [['netlag', 'NET_LAG'], ['netjitter', 'NET_JITTER'], ['netloss', 'NET_LOSS']]) if (process.env[e]) q.set(k, process.env[e]);
async function open(peer, extra) {
  await peer.send('Page.navigate', { url: `http://127.0.0.1:${port}/?${q}&${new URLSearchParams(extra)}` });
  await waitFor(() => peer.evaluate("document.readyState === 'complete' && !!window.mkddNetUi"), peer.name + ' page');
  const doc = await peer.send('DOM.getDocument');
  const { nodeId } = await peer.send('DOM.querySelector', { nodeId: doc.root.nodeId, selector: '#pick' });
  await peer.send('DOM.setFileInputFiles', { nodeId, files: [path.resolve(iso)] });
}

// In-page driver: KeyboardEvents for the page's own input layer (input.js), decided every animation
// frame from the local kart's pose. opts: { weave: amplitude 0..1, rival: steer toward the other kart
// for the first `rival` race frames }.
const BOT = (opts) => `(() => {
  const o = ${JSON.stringify(opts)};
  const buf = Module._malloc(48), held = {};
  const key = (code, down) => { if (!!held[code] === down) return; held[code] = down;
    dispatchEvent(new KeyboardEvent(down ? 'keydown' : 'keyup', { code })); };
  window.__bot = { frames: 0, steer: 0 };
  function tick() {
    requestAnimationFrame(tick);
    const f = Module._port_net_debug_frame();
    if (f < 0 || !Module._port_net_debug_kart(buf)) { key('KeyX', false); key('ArrowLeft', false); key('ArrowRight', false); return; }
    const v = new Float32Array(HEAPU8.buffer, buf, 12);
    let tx = v[6] - v[0], tz = v[8] - v[2];
    if (f < o.rival && Module.netStats) { // toward the other kart: a bump at the start
      const other = Module.netStats.karts.find((k) => !k.local);
      if (other) { tx = other.pos[0] - v[0]; tz = other.pos[2] - v[2]; }
    }
    const len = Math.hypot(tx, tz) || 1;
    let side = (tx * v[9] + tz * v[11]) / len; // > 0: target to the left (X axis = the kart's left)
    if (o.weave) side += o.weave * Math.sin(f / 50);
    key('KeyX', true);
    key('ArrowLeft', side > 0.08);
    key('ArrowRight', side < -0.08);
    window.__bot.frames = f; window.__bot.steer = side;
  }
  requestAnimationFrame(tick);
  return true;
})()`;

async function shot(peer, label) {
  const clip = await peer.evaluate("(() => { const r = document.getElementById('screen').getBoundingClientRect(); return { x: r.left + scrollX, y: r.top + scrollY, width: r.width, height: r.height, scale: 1 }; })()");
  const img = await peer.send('Page.captureScreenshot', { format: 'png', clip });
  const file = path.join(out, `${peer.name}-${label}.png`);
  await writeFile(file, Buffer.from(img.data, 'base64'));
  return file;
}

const results = {};
const check = (name, ok, detail = '') => { results[name] = ok; console.log(`${ok ? 'ok  ' : 'FAIL'} ${name}${detail ? ': ' + detail : ''}`); };
const t0 = Date.now();
const A = await launch('alice', 19610), B = await launch('bob', 19611);
const summary = { sim: Object.fromEntries([...q].filter(([k]) => k.startsWith('net') && k !== 'netserver')), shots: [] };
try {
  await open(A, { netname: 'Alice', nethost: '1' });
  const code = await waitFor(() => A.evaluate('mkddNetUi.lobby && mkddNetUi.lobby.room && mkddNetUi.lobby.room.code'), 'room code');
  await open(B, { netname: 'Bob', netroom: code });
  await waitFor(() => A.evaluate('mkddNetUi.lobby.room.members.length === 2 && mkddNetUi.mesh.allConnected()'), 'WebRTC between the peers');
  check('room + WebRTC', true, `room ${code}`);

  const pick = (peer, id, v) => peer.evaluate(`(() => { const s = document.getElementById('${id}'); s.value = '${v}'; s.dispatchEvent(new Event('change')); return true; })()`);
  await pick(B, 'mpChar1', 9); await pick(B, 'mpChar2', 10); await pick(B, 'mpKart', 6);
  await pick(A, 'mpCourse', 0x24); await pick(A, 'mpCc', 150); await pick(A, 'mpLaps', 3);
  await waitFor(() => B.evaluate('mkddNetUi.lobby.room.settings.course === 0x24 && mkddNetUi.lobby.room.settings.cc === 150'), 'race settings on Bob');
  await B.evaluate("document.getElementById('mpReady').click(); true");
  await waitFor(() => A.evaluate("!document.getElementById('mpStart').disabled"), 'start enabled');
  await delay(1500); // host clock samples
  await A.evaluate("document.getElementById('mpStart').click(); true");
  const tStart = Date.now();

  // both games boot straight into the race
  for (const p of [A, B])
    await waitFor(() => p.evaluate("typeof Module !== 'undefined' && ['waiting', 'countdown', 'race'].includes(Module.netPhase)"), p.name + ' at the start', 240_000);
  check('both games at the start line', true, `${((Date.now() - tStart) / 1000).toFixed(0)} s after Start session`);
  await shot(A, 'start'); await shot(B, 'start');
  await A.evaluate(BOT({ weave: 0, rival: 60 }));
  await B.evaluate(BOT({ weave: 0.25, rival: 60 }));
  for (const p of [A, B]) await waitFor(() => p.evaluate("Module.netPhase === 'race' || Module.netPhase === 'finished'"), p.name + ' GO', 120_000);
  const tGo = Date.now();
  check('GO on both', true);

  const pending = [...shots];
  while (Date.now() - tGo < raceSeconds * 1000) {
    await delay(250);
    while (pending.length && Date.now() - tGo >= pending[0] * 1000) {
      const s = pending.shift();
      const [fa, fb, sa, sb] = await Promise.all([shot(A, `t${s}`), shot(B, `t${s}`), A.evaluate('Module.netStats'), B.evaluate('Module.netStats')]);
      summary.shots.push({ t: s, files: [fa, fb], alice: sa, bob: sb });
      const desc = (st) => st ? st.karts.map((k) => `${k.name}${k.local ? '*' : ''} (${k.pos.join(', ')})${k.local ? '' : ` ${k.status} err ${k.err.toFixed(0)}`}`).join(' | ') : '-';
      console.log(`  t+${s}s alice: ${desc(sa)}\n         bob:   ${desc(sb)}`);
    }
  }

  // replica error measured in each game (truth on the owner vs replica on the other peer, same shared frame)
  for (const [p, other] of [[A, 'Bob'], [B, 'Alice']]) {
    const log = await p.evaluate('Module.netStatsLog || []');
    const rows = log.filter((s) => s.phase === 'race' || s.phase === 'finished').map((s) => s.karts.find((k) => k.name === other)).filter((k) => k && k.errN);
    const n = rows.reduce((a, k) => a + k.errN, 0);
    const avg = n ? rows.reduce((a, k) => a + k.err * k.errN, 0) / n : NaN;
    const max = Math.max(0, ...rows.map((k) => k.errMax));
    const perSec = rows.map((k) => k.err).sort((x, y) => x - y);
    const p95 = perSec.length ? perSec[Math.min(perSec.length - 1, Math.floor(perSec.length * 0.95))] : NaN;
    const fps = log.filter((s) => s.phase === 'race').map((s) => s.fps);
    const last = log[log.length - 1] || {};
    const rx = rows.length ? rows.reduce((a, k) => a + k.rx, 0) / rows.length : 0;
    const snaps = rows.reduce((a, k) => a + k.snaps, 0);
    const age = rows.length ? rows.reduce((a, k) => a + k.age, 0) / rows.length : 0;
    const stat = { replicaOf: other, samples: n, errAvg: +avg.toFixed(1), errMax: +max.toFixed(1), errP95PerSecond: +(+p95).toFixed(1),
                   rxPerSecond: +rx.toFixed(1), ageFrames: +age.toFixed(1), snaps, fpsAvg: +(fps.reduce((a, x) => a + x, 0) / (fps.length || 1)).toFixed(1),
                   last: last.karts };
    summary[p.name] = stat;
    check(`${p.name}: replica of ${other} tracked`, n > 20 && avg < 400,
          `err avg ${stat.errAvg} max ${stat.errMax} p95(1 s) ${stat.errP95PerSecond} units over ${n} samples, ${stat.rxPerSecond} states/s, age ${stat.ageFrames} frames, ${snaps} snaps, ${stat.fpsAvg} fps`);
  }
  // GO alignment from the logs
  for (const p of [A, B]) {
    const log = await p.evaluate("document.getElementById('log').textContent");
    const m = log.match(/GO 1: (-?\d+) ms after the shared GO time/);
    summary[p.name + 'Go'] = m ? +m[1] : null;
  }
  check('GO alignment', summary.aliceGo !== null && summary.bobGo !== null, `alice ${summary.aliceGo} ms, bob ${summary.bobGo} ms after the shared GO time`);
} catch (e) {
  check('run', false, e.message);
}
for (const p of [A, B]) {
  const log = await p.evaluate("document.getElementById('log').textContent").catch(() => '');
  await writeFile(path.join(out, p.name + '.log'), log + '\n' + p.errors.join('\n'));
  if (p.errors.length) console.log(p.name, p.errors.slice(0, 5).join('\n'));
}
await writeFile(path.join(out, 'summary.json'), JSON.stringify({ results, ...summary }, null, 1));
const ok = Object.values(results).every(Boolean);
console.log(`\n${ok ? 'PASS' : 'FAIL'} (${((Date.now() - t0) / 1000).toFixed(1)} s); wrote ${out}`);
for (const b of browsers) b.kill();
await srv.close();
process.exit(ok ? 0 : 1);
