// End-to-end test of starting an online session the way a player does it, including the ways
// it used to fail silently: a window whose game was already running ignored the session start,
// a window without a disc could not take part, a host could press GO before everyone was ready.
//
//   MKDD_ISO=/path/GM4E01.iso node tools/web_wasm/net/test/e2e-start.mjs
//
// Two headless Chrome instances (Alice, Bob) against the lobby server, driven with mouse clicks
// and typed text through the page (no ?netserver / ?nethost automation):
//  1. no disc: Bob connects without a disc image -> a notice asks for one, nothing connects
//  2. Play pressed before joining: Bob starts his game solo, then joins Alice's room -> refused
//     with a notice and a "Reload to play online" button, which reloads into the room
//  3. in a room the solo Play / Start buttons are gone, with a hint
//  4. Bob not ready: GO is disabled with the reason; a start forced past the button is refused
//     by the server and the reason is shown
//  5. a member whose game runs (or who has no disc) blocks GO and cannot be Ready
//  6. a member that cannot boot when the session starts (as an older page would): it shows why,
//     the host is told and ends the session, both reload into the room (host changes hands)
//  7. the real start: both games boot, the drawer closes, the boot lights / countdown show, the
//     keyboard focus is on the game
//  8. the server logged the room events
// Screenshots of each notice go to MKDD_NET_OUT (default build/web_wasm/net-start).
// Env: MKDD_WWW (default build/web_wasm/web-netweb), MKDD_NET_PORT (server, 18630),
// MKDD_NET_DEBUG_PORT (Chrome debug, 19630 and +1), CHROME_BIN.
import { spawn } from 'node:child_process';
import { mkdir, rm, writeFile } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createServer, loadConfig } from '../server/server.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../../../..');
const www = path.resolve(process.env.MKDD_WWW || path.join(root, 'build/web_wasm/web-netweb'));
const out = path.resolve(process.env.MKDD_NET_OUT || path.join(root, 'build/web_wasm/net-start'));
const iso = process.env.MKDD_ISO;
const port = Number(process.env.MKDD_NET_PORT || 18630);
const debugPort = Number(process.env.MKDD_NET_DEBUG_PORT || 19630);
if (!iso) { console.error('set MKDD_ISO to your disc image'); process.exit(2); }
await mkdir(out, { recursive: true });
// fresh browser profiles: no remembered disc, name or server
for (const n of ['alice', 'bob']) await rm(path.join(out, 'profile-' + n), { recursive: true, force: true });

const serverLog = [];
const srv = createServer(loadConfig({ PORT: String(port), HOST: '127.0.0.1', STATIC_DIR: www, STUN_URLS: '' }), { log: (l) => serverLog.push(l) });
await srv.listen();
const delay = (ms) => new Promise((r) => setTimeout(r, ms));
const browsers = [];
process.on('exit', () => { for (const b of browsers) b.kill(); });

async function waitFor(check, what, timeout = 30_000) {
  const t0 = Date.now();
  let last;
  while (Date.now() - t0 < timeout) {
    try { const v = await check(); if (v) return v; } catch (e) { last = e; }
    await delay(150);
  }
  throw new Error(`timed out waiting for ${what}${last ? ' (' + last.message + ')' : ''}`);
}

async function launch(name, dp) {
  const b = spawn(process.env.CHROME_BIN || 'google-chrome', [
    '--headless=new', '--no-sandbox', '--disable-dev-shm-usage', '--enable-unsafe-swiftshader', '--use-angle=swiftshader',
    '--disable-background-timer-throttling', '--disable-renderer-backgrounding', '--disable-backgrounding-occluded-windows',
    '--disable-features=WebRtcHideLocalIpsWithMdns', '--autoplay-policy=no-user-gesture-required',
    `--remote-debugging-port=${dp}`, `--user-data-dir=${path.join(out, 'profile-' + name)}`, '--window-size=1280,900', 'about:blank',
  ], { stdio: 'ignore' });
  browsers.push(b);
  const target = await waitFor(async () => (await (await fetch(`http://127.0.0.1:${dp}/json/list`)).json()).find((t) => t.type === 'page'), 'chrome ' + name);
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
  const p = { name, send, evaluate, errors };
  // a real mouse click in the middle of the element (scrolled into view first)
  p.click = async (sel) => {
    const r = await waitFor(() => evaluate(`(() => { const e = document.querySelector(${JSON.stringify(sel)}); if (!e) return null;
      e.scrollIntoView({ block: 'center', behavior: 'instant' }); const r = e.getBoundingClientRect();
      const x = r.left + r.width / 2, y = r.top + r.height / 2;
      return r.width > 0 && document.elementFromPoint(x, y) && e.contains(document.elementFromPoint(x, y)) ? { x, y } : null; })()`), `${name}: ${sel} clickable`, 20_000);
    for (const type of ['mouseMoved', 'mousePressed', 'mouseReleased'])
      await send('Input.dispatchMouseEvent', { type, x: r.x, y: r.y, button: 'left', clickCount: 1 });
  };
  p.type = async (sel, text) => {
    await evaluate(`(() => { const e = document.querySelector(${JSON.stringify(sel)}); e.focus(); e.select(); return true; })()`);
    await send('Input.insertText', { text });
    await evaluate(`document.querySelector(${JSON.stringify(sel)}).dispatchEvent(new Event('change')), true`);
  };
  p.shot = async (label) => {
    const img = await send('Page.captureScreenshot', { format: 'png' });
    const file = path.join(out, `${label}-${name}.png`);
    await writeFile(file, Buffer.from(img.data, 'base64'));
    return file;
  };
  p.ui = () => evaluate(`(() => { const $ = (id) => document.getElementById(id), vis = (id) => !$(id).hidden && !$(id).closest('[hidden]');
    const L = mkddNetUi.lobby;
    return { alert: vis('mpAlert') ? $('mpAlertText').textContent : '', alertButtons: [...$('mpAlertButtons').children].map((b) => b.textContent),
             overlay: vis('netMsg') ? $('netMsgText').textContent : '', overlayButtons: [...$('netMsgButtons').children].map((b) => b.textContent),
             status: $('mpStatus').textContent, started: mkddPage.started(), connected: !!(L && L.id), room: L && L.room && L.room.code,
             members: L && L.room ? L.room.members.length : 0, roomState: L && L.room && L.room.state,
             session: !!(mkddNetUi.mesh && mkddNetUi.mesh.session), host: !!(L && L.isHost),
             go: { hidden: $('mpStart').hidden, disabled: $('mpStart').disabled, title: $('mpStart').title },
             ready: { disabled: $('mpReady').disabled, checked: $('mpReady').checked },
             play: { visible: vis('uiPlay'), startVisible: vis('start'), hint: $('uiDiscHint').textContent },
             blockedRows: [...document.querySelectorAll('#mpMembers tr.blocked td:nth-child(2)')].map((t) => t.textContent),
             drawer: $('mpDrawer').classList.contains('open'), hud: vis('netHud'), boot: $('netHud').dataset.boot || '',
             loader: vis('uiLoader'), countdown: vis('uiCountdown') ? $('uiCountdown').textContent : '',
             focus: document.activeElement && (document.activeElement.id || document.activeElement.tagName),
             phase: window.Module && Module.netPhase || '', search: location.search }; })()`);
  return p;
}

const results = {};
const check = (name, ok, detail = '') => { results[name] = !!ok; console.log(`${ok ? 'ok  ' : 'FAIL'} ${name}${detail ? ': ' + detail : ''}`); };
const t0 = Date.now();
const A = await launch('alice', debugPort), B = await launch('bob', debugPort + 1);
const base = `http://127.0.0.1:${port}/?env=MKDD_GFX_SCALE%3D1&env=MKDD_GFX_MSAA%3D0&env=MKDD_GFX_SHADER_WARM%3D0&netpageboot=1&netauto=0`;

async function load(p, url = base) {
  await p.send('Page.navigate', { url });
  await waitFor(() => p.evaluate("document.readyState === 'complete' && !!window.mkddNetUi && !!(window.Module && Module.portReady)"), p.name + ' page', 60_000);
}
async function chooseDisc(p) {
  const doc = await p.send('DOM.getDocument');
  const { nodeId } = await p.send('DOM.querySelector', { nodeId: doc.root.nodeId, selector: '#pick' });
  await p.send('DOM.setFileInputFiles', { nodeId, files: [path.resolve(iso)] });
  await waitFor(() => p.evaluate('!!mkddPage.getDisc()'), p.name + ' disc');
}
async function openDrawer(p) {
  if (!(await p.evaluate("document.getElementById('mpDrawer').classList.contains('open')"))) await p.click('#uiMpBtn');
  // (the drawer slides in; a page busy running the game takes a while)
  await waitFor(() => p.evaluate("Math.abs(document.getElementById('mpDrawer').getBoundingClientRect().right - document.documentElement.clientWidth) < 2"), p.name + ' drawer', 20_000);
  await delay(200);
}
async function connect(p, name) {
  await openDrawer(p);
  if (await p.evaluate("!document.getElementById('mpName').disabled")) await p.type('#mpName', name);
  await p.click('#mpConnect');
}
const pick = (p, id, v) => p.evaluate(`(() => { const s = document.getElementById('${id}'); s.value = '${v}'; s.dispatchEvent(new Event('change')); return true; })()`);

try {
  // fresh profiles: no remembered disc
  await load(A); await load(B);
  await chooseDisc(A);

  // 1. no disc
  await connect(B, 'Bob');
  let b = await waitFor(async () => { const u = await B.ui(); return u.alert ? u : null; }, 'Bob: no-disc notice');
  check('no disc: Connect explains, does not connect', /Choose your disc image/.test(b.alert) && !b.connected, b.alert);
  await B.shot('1-nodisc');

  // Alice hosts
  await connect(A, 'Alice');
  await waitFor(async () => (await A.ui()).connected, 'Alice connected');
  await A.click('#mpCreate');
  const code = await waitFor(async () => (await A.ui()).room, 'room code');
  check('room created', true, code);

  // 2. Play pressed before joining
  await chooseDisc(B);
  await connect(B, 'Bob');
  await waitFor(async () => (await B.ui()).connected, 'Bob connected');
  b = await B.ui();
  check('Play available before joining a room', b.play.visible, b.play.hint);
  await B.click('#mpDrawer [data-close]'); // the user closes the drawer to see Play
  await B.click('#uiPlay');
  await waitFor(async () => (await B.ui()).started, 'Bob game started');
  await openDrawer(B);
  await B.type('#mpCode', code);
  await B.click('#mpJoin');
  b = await waitFor(async () => { const u = await B.ui(); return u.alert ? u : null; }, 'Bob: refused notice');
  check('game running: join refused with a reload button', /already running/.test(b.alert) && b.alertButtons.includes('Reload to play online') &&
        /already running/.test(b.overlay) && !b.room, `${b.alert} [${b.alertButtons}]`);
  check('game running: the room did not change', (await A.ui()).members === 1);
  await B.shot('2-running-refused');
  await B.click('#mpAlertButtons button');
  await waitFor(async () => { const u = await B.ui(); return !u.started && u.room === code; }, 'Bob reloaded into the room', 60_000);
  await waitFor(async () => (await A.ui()).members === 2, 'Alice sees Bob');
  b = await B.ui();
  check('reload rejoins the room, URL cleaned', b.room === code && !/netroom/.test(b.search), b.search);

  // 3. solo start hidden in a room (ui.js mirrors the buttons every 250 ms)
  b = await waitFor(async () => { const u = await B.ui(); return !u.play.visible && /host starts the race/.test(u.play.hint) ? u : null; }, 'Bob: hint', 5000).catch(() => B.ui());
  check('in a room: no solo Play / Start, with a hint', !b.play.visible && !b.play.startVisible && /host starts the race/.test(b.play.hint), b.play.hint);
  check('started solo is refused in a room', !(await B.evaluate('mkddPage.startGame(null).then(() => mkddPage.started())')));

  // 4. Bob not ready: GO disabled with the reason, a forced start is refused and explained
  await waitFor(async () => /Waiting for Bob/.test((await A.ui()).go.title), 'GO reason');
  let a = await A.ui();
  check('guest not ready: GO disabled with the reason', a.go.disabled && /Waiting for Bob to be ready/.test(a.go.title) && /Waiting for Bob/.test(a.status), a.go.title);
  await A.evaluate('mkddNetUi.lobby.start(), true');
  a = await waitFor(async () => { const u = await A.ui(); return /Cannot start/.test(u.alert) ? u : null; }, 'Alice: start refused notice');
  check('forced start refused, reason shown', /not ready: Bob/.test(a.alert) && a.roomState === 'lobby', a.alert);
  await A.shot('4-not-ready');

  // 5. a member whose game runs blocks GO and cannot be Ready
  await B.evaluate('window.__started = mkddPage.started; mkddPage.started = () => true; mkddNetUi.render(); true');
  a = await waitFor(async () => { const u = await A.ui(); return /Bob \(game already running/.test(u.go.title) ? u : null; }, 'GO blocked by Bob');
  b = await B.ui();
  check('member with a running game blocks GO', a.go.disabled && a.blockedRows.some((n) => n.startsWith('Bob')), a.go.title);
  check('member with a running game cannot be Ready', b.ready.disabled && !b.ready.checked && /You cannot race/.test(b.status), b.status);
  await A.shot('5-blocked'); await B.shot('5-blocked');
  await B.evaluate('mkddPage.started = window.__started; mkddNetUi.render(); true');
  await waitFor(async () => !(await B.ui()).ready.disabled, 'Bob can be ready again');

  // 6. a member that cannot boot at the start (an older page that does not report its state)
  await B.click('#mpReady');
  await waitFor(async () => !(await A.ui()).go.disabled, 'GO enabled');
  await B.evaluate(`(() => { const L = mkddNetUi.lobby, orig = L.setStatus.bind(L);
    L.setStatus = (s) => { if (s.boot) orig(s); }; L.setReady = () => {}; mkddPage.started = () => true; return true; })()`);
  await delay(1500); // clock samples
  await A.click('#mpStart');
  b = await waitFor(async () => { const u = await B.ui(); return /cannot join it/.test(u.overlay) ? u : null; }, 'Bob: cannot-join overlay');
  check('cannot boot: overlay + drawer notice, no countdown', /already running/.test(b.overlay) && b.overlayButtons.includes('Reload to play online') &&
        /cannot join it/.test(b.alert) && b.boot === 'fail' && !b.countdown && !b.drawer, b.overlay);
  a = await waitFor(async () => { const u = await A.ui(); return /Bob cannot join the race/.test(u.overlay) ? u : null; }, 'Alice told about Bob');
  check('host told, offered End session', a.overlayButtons.includes('End session') && /already running/.test(a.overlay), a.overlay);
  await B.shot('6-cannot-boot'); await A.shot('6-host-told');
  await A.click('#netMsgButtons button');
  b = await waitFor(async () => { const u = await B.ui(); return /ended the session/.test(u.overlay) ? u : null; }, 'Bob: session ended');
  check('session ended: everyone offered a reload', /rejoin room/.test(b.overlay) && b.overlayButtons.includes('Reload'), b.overlay);
  await B.shot('6-ended');
  await B.click('#netMsgButtons button');
  await waitFor(async () => { const u = await B.ui(); return !u.started && u.room === code && u.members === 2; }, 'Bob back in the room', 60_000);
  await waitFor(async () => /ended the session/.test((await A.ui()).overlay), 'Alice: session ended');
  await A.click('#netMsgButtons button');
  await waitFor(async () => { const u = await A.ui(); return !u.started && u.room === code && u.members === 2; }, 'Alice back in the room', 60_000);
  b = await waitFor(async () => { const u = await B.ui(); return u.host ? u : null; }, 'Bob hosts');
  check('both reloaded into the room; the host changed hands', b.host && b.roomState === 'lobby');

  // 7. the real start: Bob hosts now, Alice is ready
  await pick(B, 'mpCourse', 0x24);
  await waitFor(async () => (await A.evaluate('mkddNetUi.lobby.room.settings.course')) === 0x24, 'settings on Alice');
  await openDrawer(A);
  await A.click('#mpReady');
  await waitFor(async () => !(await B.ui()).go.disabled, 'GO enabled');
  await delay(1500);
  await B.click('#mpStart');
  const seen = { A: {}, B: {} };
  await waitFor(async () => {
    for (const [k, p] of [['A', A], ['B', B]]) {
      const u = await p.ui();
      if (u.countdown) seen[k].countdown = u.countdown;
      if (u.loader) seen[k].loader = true;
      if (!u.drawer && u.session) seen[k].closed = true;
      if (u.focus === 'screen') seen[k].focus = true;
      if (u.phase) seen[k].phase = u.phase;
      if (u.overlay) seen[k].overlay = u.overlay;
    }
    return seen.A.phase && seen.B.phase;
  }, 'both games loading the race', 180_000);
  for (const [k, p] of [['A', A], ['B', B]])
    check(`${p.name}: GO is visible (drawer closed, countdown / boot lights, focus on the game, game booting)`,
          seen[k].closed && (seen[k].countdown || seen[k].loader) && seen[k].focus && seen[k].phase && !seen[k].overlay, JSON.stringify(seen[k]));
  await A.shot('7-go'); await B.shot('7-go');

  // 8. server log
  const mine = serverLog.filter((l) => l.includes(code));
  const want = [/created by #\d+ "Alice"/, /"Bob" refused|"Bob" joined/, /start refused: not ready: Bob/, /session started by #\d+ "Alice"/,
                /"Bob" could not start the game: the game was already running/, /session ended by #\d+ "Alice"/, /"Alice" is the host now|"Bob" is the host now/,
                /session started by #\d+ "Bob"/];
  const missing = want.filter((re) => !mine.some((l) => re.test(l)));
  check('server logged the room events', !missing.length, missing.length ? 'missing ' + missing.join(' ') + '\n' + mine.join('\n') : `${mine.length} lines`);
} catch (e) {
  check('run', false, e.message);
}
for (const p of [A, B]) {
  const log = await p.evaluate("document.getElementById('log').textContent").catch(() => '');
  await writeFile(path.join(out, p.name + '.log'), log + '\n' + p.errors.join('\n'));
  if (p.errors.length) console.log(p.name, p.errors.slice(0, 5).join('\n'));
}
await writeFile(path.join(out, 'server.log'), serverLog.join('\n') + '\n');
const ok = Object.values(results).every(Boolean) && !A.errors.length && !B.errors.length;
console.log(`\n${ok ? 'PASS' : 'FAIL'} (${((Date.now() - t0) / 1000).toFixed(1)} s); wrote ${out}`);
for (const br of browsers) br.kill();
await srv.close();
process.exit(ok ? 0 : 1);
