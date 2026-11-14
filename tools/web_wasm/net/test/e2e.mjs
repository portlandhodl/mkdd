// End-to-end transport test: the lobby server plus two headless Chrome instances.
//
//   MKDD_ISO=/path/GM4E01.iso node tools/web_wasm/net/test/e2e.mjs
//
// Serves the web build (MKDD_WWW, default build/web_wasm/web-netweb) from the lobby
// server itself, opens Alice (host) and Bob, creates and joins a room through the
// page's panel, sets profiles and race settings, starts the session (?netnoboot=1:
// the game is not booted, only the disc header is read for the game ID), then:
//  - exchanges messages both ways on both channels through the JS API (mesh.send),
//  - sends through the wasm bridge (port_net_send) and pulls on the other side
//    (port_net_recv), reads port_net_session_json / port_net_host_time,
//  - measures RTT, host-clock agreement and delivery of 600 unreliable 56-byte
//    packets at 30 Hz-like pacing.
// Simulated network on both pages: NET_LAG (ms), NET_JITTER (ms), NET_LOSS (0..1).
// Ports: MKDD_NET_PORT (server, default 18610), Chrome debug 19610/19611.
import { spawn } from 'node:child_process';
import { mkdir, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createServer, loadConfig } from '../server/server.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../../../..');
const www = path.resolve(process.env.MKDD_WWW || path.join(root, 'build/web_wasm/web-netweb'));
const out = path.resolve(process.env.MKDD_NET_OUT || path.join(root, 'build/web_wasm/net-e2e'));
const iso = process.env.MKDD_ISO;
const port = Number(process.env.MKDD_NET_PORT || 18610);
if (!iso) { console.error('set MKDD_ISO to your disc image (only its 6-byte header is read)'); process.exit(2); }
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
    await delay(100);
  }
  throw new Error(`timed out waiting for ${what}${last ? ' (' + last.message + ')' : ''}`);
}

async function launch(name, debugPort) {
  const b = spawn(process.env.CHROME_BIN || 'google-chrome', [
    '--headless=new', '--no-sandbox', '--disable-dev-shm-usage', '--enable-unsafe-swiftshader', '--use-angle=swiftshader',
    '--disable-background-timer-throttling', '--disable-renderer-backgrounding', '--disable-backgrounding-occluded-windows',
    '--disable-features=WebRtcHideLocalIpsWithMdns',
    `--remote-debugging-port=${debugPort}`, `--user-data-dir=${path.join(out, 'profile-' + name)}`, '--window-size=1000,900', 'about:blank',
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

const q = new URLSearchParams({ netserver: `ws://127.0.0.1:${port}/ws`, netnoboot: '1' });
for (const [k, e] of [['netlag', 'NET_LAG'], ['netjitter', 'NET_JITTER'], ['netloss', 'NET_LOSS']]) if (process.env[e]) q.set(k, process.env[e]);
async function open(peer, extra) {
  await peer.send('Page.navigate', { url: `http://127.0.0.1:${port}/?${q}&${new URLSearchParams(extra)}` });
  await waitFor(() => peer.evaluate("document.readyState === 'complete' && !!window.mkddNetUi"), peer.name + ' page');
  const doc = await peer.send('DOM.getDocument');
  const { nodeId } = await peer.send('DOM.querySelector', { nodeId: doc.root.nodeId, selector: '#pick' });
  await peer.send('DOM.setFileInputFiles', { nodeId, files: [path.resolve(iso)] });
}

const results = {};
const check = (name, ok, detail = '') => { results[name] = ok; console.log(`${ok ? 'ok  ' : 'FAIL'} ${name}${detail ? ': ' + detail : ''}`); };
const t0 = Date.now();
const A = await launch('alice', 19610), B = await launch('bob', 19611);
try {
  await open(A, { netname: 'Alice', nethost: '1' });
  const code = await waitFor(() => A.evaluate('mkddNetUi.lobby && mkddNetUi.lobby.room && mkddNetUi.lobby.room.code'), 'room code');
  await open(B, { netname: 'Bob', netroom: code });
  await waitFor(() => A.evaluate('mkddNetUi.lobby.room.members.length === 2 && mkddNetUi.mesh.allConnected()'), 'WebRTC between the peers');
  check('room + WebRTC', true, `room ${code}, connected after ${((Date.now() - t0) / 1000).toFixed(1)} s`);

  // profiles and host settings through the panel
  await B.evaluate("(() => { const s = document.getElementById('mpKart'); s.value = '9'; s.dispatchEvent(new Event('change')); return true; })()");
  await A.evaluate("(() => { const s = document.getElementById('mpCourse'); s.value = String(0x2d); s.dispatchEvent(new Event('change')); return true; })()");
  await waitFor(() => B.evaluate('mkddNetUi.lobby.room.settings.course === 0x2d'), 'race settings on Bob');
  await B.evaluate("document.getElementById('mpReady').click(); true");
  await waitFor(() => A.evaluate("!document.getElementById('mpStart').disabled"), 'start enabled');
  await delay(1500); // clock samples
  await A.evaluate("document.getElementById('mpStart').click(); true");
  await waitFor(() => B.evaluate('!!mkddNetUi.mesh.session'), 'session on Bob');
  const session = await B.evaluate('JSON.parse(mkddNetUi.mesh.attachModule ? Module.netBridge.sessionJson() : "null")');
  check('session start', session && session.settings.course === 0x2d && session.members.find((m) => m.name === 'Bob').profile.kart === 9,
        JSON.stringify(session));

  // JS API: both channels, both directions
  const setup = `window.__got = []; mkddNetUi.mesh.on('message', (p, kind, d) => __got.push([p.name, kind, [...d]])); true`;
  await A.evaluate(setup); await B.evaluate(setup);
  await A.evaluate("mkddNetUi.mesh.send('all', MkddNet.KIND.RELIABLE, new Uint8Array([0x40, 1, 2, 3])); mkddNetUi.mesh.send('all', MkddNet.KIND.UNRELIABLE, new Uint8Array([0x01, 5])); true");
  await B.evaluate("mkddNetUi.mesh.send('all', MkddNet.KIND.RELIABLE, new Uint8Array([0x41]));  true");
  const gotB = await waitFor(() => B.evaluate('__got.length >= (MkddNet && 1) && __got.some((m) => m[1] === 1) ? __got : null'), 'Bob receives');
  const gotA = await waitFor(() => A.evaluate('__got.length ? __got : null'), 'Alice receives');
  check('JS messages both ways', gotB.some((m) => m[0] === 'Alice' && m[1] === 1 && m[2].join() === '64,1,2,3') && gotA.some((m) => m[0] === 'Bob' && m[2][0] === 0x41),
        `Bob got ${JSON.stringify(gotB)}, Alice got ${JSON.stringify(gotA)}`);

  // wasm bridge: Alice's C side sends, Bob's C side pulls
  await A.evaluate(`(() => { const p = _malloc(4); HEAPU8.set([0x42, 0xde, 0xad, 0x01], p);
    window.__sent = Module._port_net_send(-1, 1, p, 4); _free(p); return true; })()`);
  const pulled = await waitFor(() => B.evaluate(`(() => {
    const buf = _malloc(256), from = _malloc(4), kind = _malloc(4);
    let res = null;
    for (let n; (n = Module._port_net_recv(from, kind, buf, 256)) > 0;) {
      const k = HEAP32[kind >> 2], bytes = [...HEAPU8.subarray(buf, buf + n)];
      if (k === 1 && bytes[0] === 0x42) res = { from: HEAP32[from >> 2], bytes };
    }
    _free(buf); _free(from); _free(kind);
    return res;
  })()`), 'bridge message on Bob');
  const ids = await A.evaluate('[Module._port_net_self(), Module._port_net_host(), Module._port_net_active()]');
  check('wasm bridge send/recv', pulled.from === ids[0] && pulled.bytes.join() === '66,222,173,1', JSON.stringify(pulled));
  check('bridge info', ids[0] === ids[1] && ids[2] === 1, `Alice self/host/active ${ids}`);
  const sj = await B.evaluate("(() => { const p = _malloc(4096); const n = Module._port_net_session_json(p, 4096); const s = UTF8ToString(p); _free(p); return [n, s]; })()");
  check('port_net_session_json', sj[0] > 0 && JSON.parse(sj[1]).code === code, sj[1].slice(0, 120));

  // host clock agreement: sample both at nearly the same moment through CDP
  const ht = [];
  for (let i = 0; i < 5; i++) {
    const [a, b] = await Promise.all([A.evaluate('Module._port_net_host_time()'), B.evaluate('Module._port_net_host_time()')]);
    ht.push(b - a);
    await delay(100);
  }
  const rtt = await B.evaluate('Module._port_net_rtt(Module._port_net_host())');
  ht.sort((x, y) => x - y);
  check('host clock agreement', Math.abs(ht[2]) < 20 + (Number(process.env.NET_JITTER) || 0), `median Bob-Alice host time ${ht[2].toFixed(1)} ms, rtt ${rtt} ms`);

  // a 30 Hz stream of 56-byte state packets for 600 packets (20 s at 30 Hz, sent faster here)
  await B.evaluate("window.__stream = 0; mkddNetUi.mesh.on('message', (p, k, d) => { if (k === 0 && d[0] === 0x01 && d.length === 56) __stream++; }); true");
  await A.evaluate(`(async () => { for (let i = 0; i < 600; i++) { const b = new Uint8Array(56); b[0] = 1; b[2] = i & 255; b[3] = i >> 8;
    mkddNetUi.mesh.send('all', 0, b); if (i % 10 === 9) await new Promise((r) => setTimeout(r, 33)); } return true; })()`);
  await delay(1000 + 4 * (Number(process.env.NET_LAG) || 0));
  const streamed = await B.evaluate('__stream');
  const loss = Number(process.env.NET_LOSS) || 0;
  check('unreliable stream', streamed > 600 * (1 - loss) * 0.85, `${streamed}/600 delivered (simulated loss ${loss * 100}%)`);
  const stats = { alice: await A.evaluate('mkddNetUi.mesh.stats()'), bob: await B.evaluate('mkddNetUi.mesh.stats()') };
  await writeFile(path.join(out, 'summary.json'), JSON.stringify({ results, stats, session, sim: Object.fromEntries(q) }, null, 1));
} catch (e) {
  check('run', false, e.message);
}
for (const p of [A, B]) {
  const log = await p.evaluate("document.getElementById('log').textContent").catch(() => '');
  await writeFile(path.join(out, p.name + '.log'), log + '\n' + p.errors.join('\n'));
  if (p.errors.length) console.log(p.name, p.errors.join('\n'));
}
const ok = Object.values(results).every(Boolean);
console.log(`\n${ok ? 'PASS' : 'FAIL'} (${((Date.now() - t0) / 1000).toFixed(1)} s); wrote ${out}`);
for (const b of browsers) b.kill();
await srv.close();
process.exit(ok ? 0 : 1);
