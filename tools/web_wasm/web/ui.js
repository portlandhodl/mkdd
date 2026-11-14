// Page behaviour for the themed UI (theme.css): drawers, the "pit radio" log
// console, disc drag-and-drop and the disc card, race-start lights while the game
// boots, the controller diagram, small extras (button blips, confetti, FPS chip).
// Purely presentational: the game is driven by index.html's inline script,
// input.js, audio.js and (when present) net.js + netui.js.
(() => {
  'use strict';
  const $ = (id) => document.getElementById(id);
  const page = window.mkddPage;
  const body = document.body;
  const startBtn = $('start'), pick = $('pick'), logEl = $('log');
  const reduced = matchMedia('(prefers-reduced-motion: reduce)');

  // -- preferences (per browser) --
  const PREFS_KEY = 'mkdd-ui-v1';
  const prefs = { sfx: false, motion: !reduced.matches, fps: true, legend: 'kb' };
  try { Object.assign(prefs, JSON.parse(localStorage.getItem(PREFS_KEY) || '{}')); } catch (e) {}
  const savePrefs = () => { try { localStorage.setItem(PREFS_KEY, JSON.stringify(prefs)); } catch (e) {} };
  const calm = () => !prefs.motion || reduced.matches;
  function applyPrefs() {
    body.classList.toggle('calm', !prefs.motion);
    $('uiSfx').checked = prefs.sfx;
    $('uiMotion').checked = prefs.motion;
    $('uiFpsOn').checked = prefs.fps;
  }
  $('uiSfx').addEventListener('change', (e) => { prefs.sfx = e.target.checked; savePrefs(); if (prefs.sfx) blip(880); });
  $('uiMotion').addEventListener('change', (e) => { prefs.motion = e.target.checked; savePrefs(); applyPrefs(); });
  $('uiFpsOn').addEventListener('change', (e) => { prefs.fps = e.target.checked; savePrefs(); });
  applyPrefs();

  // -- button blips: a few ms of a synthesized square wave, off by default --
  let sfx = null;
  function blip(freq = 660) {
    if (!prefs.sfx) return;
    try {
      sfx = sfx || new (window.AudioContext || window.webkitAudioContext)();
      const t = sfx.currentTime, o = sfx.createOscillator(), g = sfx.createGain();
      o.type = 'square';
      o.frequency.setValueAtTime(freq, t);
      o.frequency.exponentialRampToValueAtTime(freq * 1.5, t + 0.05);
      g.gain.setValueAtTime(0.0001, t);
      g.gain.exponentialRampToValueAtTime(0.05, t + 0.005);
      g.gain.exponentialRampToValueAtTime(0.0001, t + 0.09);
      o.connect(g).connect(sfx.destination);
      o.start(t); o.stop(t + 0.1);
    } catch (e) {}
  }
  addEventListener('pointerdown', (e) => {
    const b = e.target.closest && e.target.closest('button:not([disabled]), label.btn, .seg button');
    if (b) blip(b.classList.contains('b-green') ? 990 : b.classList.contains('b-red') ? 440 : 660);
  });
  // A button clicked with the mouse gives the focus back, so the game keys
  // (Space, Enter, arrows) never land on it.
  addEventListener('click', (e) => {
    if (!e.detail) return;
    const b = e.target.closest && e.target.closest('button, label.btn, summary');
    if (b) setTimeout(() => { if (document.activeElement === b) b.blur(); }, 0);
  });
  // Labels that open a file picker work from the keyboard too.
  for (const l of document.querySelectorAll('label[data-file]'))
    l.addEventListener('keydown', (e) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); l.click(); } });

  // -- toasts and confetti --
  function toast(text, kind = '') {
    const t = document.createElement('div');
    t.className = 'toast ' + kind; t.textContent = text;
    $('uiToasts').append(t);
    setTimeout(() => t.remove(), 3500);
  }
  function confetti() {
    if (calm()) return;
    const c = document.createElement('canvas');
    c.className = 'confetti';
    const dpr = Math.min(2, devicePixelRatio || 1);
    c.width = innerWidth * dpr; c.height = innerHeight * dpr;
    body.append(c);
    const g = c.getContext('2d');
    const colors = ['#35c94f', '#ee3b3b', '#ffc61a', '#c3c5d3', '#8a6cff', '#ffffff'];
    const parts = Array.from({ length: 140 }, (_, i) => ({
      x: c.width * (i % 2 ? 0.15 : 0.85), y: c.height * 0.75,
      vx: (i % 2 ? 1 : -1) * (4 + Math.random() * 10) * dpr, vy: -(10 + Math.random() * 14) * dpr,
      s: (6 + Math.random() * 8) * dpr, r: Math.random() * 6, vr: (Math.random() - 0.5) * 0.4,
      c: colors[i % colors.length], flag: i % 7 === 0,
    }));
    const t0 = performance.now();
    (function frame(now) {
      const t = (now - t0) / 1000;
      g.clearRect(0, 0, c.width, c.height);
      for (const p of parts) {
        p.vy += 0.45 * dpr; p.vx *= 0.99; p.x += p.vx; p.y += p.vy; p.r += p.vr;
        g.save(); g.translate(p.x, p.y); g.rotate(p.r); g.globalAlpha = Math.max(0, 1 - Math.max(0, t - 1.8));
        if (p.flag) { // a little checkered flag
          const q = p.s / 2;
          for (let k = 0; k < 4; k++) { g.fillStyle = (k === 0 || k === 3) ? '#fff' : '#111'; g.fillRect((k % 2) * q - q, (k >> 1) * q - q, q, q); }
        } else { g.fillStyle = p.c; g.fillRect(-p.s / 2, -p.s / 4, p.s, p.s / 2); }
        g.restore();
      }
      if (t < 2.8) requestAnimationFrame(frame); else c.remove();
    })(t0);
  }

  // -- drawers (settings, multiplayer) and the log console --
  const panels = [...document.querySelectorAll('.drawer, .console')];
  const openers = (p) => document.querySelectorAll(`[data-drawer="${p.id}"], [aria-controls="${p.id}"]`);
  function setOpen(p, open) {
    if (!p) return;
    if (open) for (const q of panels) if (q !== p && q.classList.contains('open') && !(q.classList.contains('console') || p.classList.contains('console'))) setOpen(q, false);
    p.classList.toggle('open', open);
    for (const b of openers(p)) b.setAttribute('aria-expanded', String(open));
    body.classList.toggle('drawer-open', panels.some((q) => q.classList.contains('drawer') && q.classList.contains('open')));
    if (p.id === 'uiConsole' && open) { unread = 0; showUnread(); logEl.scrollTop = logEl.scrollHeight; }
  }
  for (const b of document.querySelectorAll('[data-drawer]'))
    b.addEventListener('click', () => { const p = $(b.dataset.drawer); setOpen(p, !p.classList.contains('open')); });
  $('uiLogBtn').addEventListener('click', () => { const p = $('uiConsole'); setOpen(p, !p.classList.contains('open')); });
  for (const b of document.querySelectorAll('[data-close]')) b.addEventListener('click', () => setOpen(b.closest('.drawer, .console'), false));
  $('uiScrim').addEventListener('click', () => { for (const p of panels) if (p.classList.contains('drawer')) setOpen(p, false); });
  addEventListener('keydown', (e) => {
    if (e.key !== 'Escape' || document.fullscreenElement) return;
    const open = panels.filter((p) => p.classList.contains('open'));
    if (open.length) setOpen(open[open.length - 1], false);
  });
  for (const tab of document.querySelectorAll('[data-tab]'))
    tab.addEventListener('click', () => {
      for (const t of tab.parentNode.children) {
        t.setAttribute('aria-selected', String(t === tab));
        $(t.dataset.tab).hidden = t !== tab;
      }
    });
  $('uiLogCopy').addEventListener('click', () => {
    navigator.clipboard && navigator.clipboard.writeText(logEl.textContent).then(() => toast('Log copied'), () => {});
  });

  // -- graphics options: two-way choices become switches or segmented buttons --
  const enhanced = [];
  for (const sel of document.querySelectorAll('#gfx select')) {
    const opts = [...sel.options];
    if (opts.length !== 2) continue;
    let ctl;
    if (opts.map((o) => o.text).join('/') === 'Off/On' || opts.map((o) => o.text).join('/') === 'On/Off') {
      ctl = document.createElement('input');
      ctl.type = 'checkbox'; ctl.className = 'switch';
      ctl.setAttribute('aria-label', sel.closest('label').firstElementChild.textContent);
      const on = opts.find((o) => o.text === 'On').value, off = opts.find((o) => o.text === 'Off').value;
      ctl.addEventListener('change', () => { sel.value = ctl.checked ? on : off; sel.dispatchEvent(new Event('change')); });
      ctl.sync = () => { ctl.checked = sel.value === on; };
    } else {
      ctl = document.createElement('span');
      ctl.className = 'seg';
      for (const o of opts) {
        const b = document.createElement('button');
        b.type = 'button'; b.textContent = o.text.replace(/ \(.*\)$/, ''); b.title = o.text; b.dataset.v = o.value;
        b.addEventListener('click', (e) => { e.preventDefault(); sel.value = o.value; sel.dispatchEvent(new Event('change')); });
        ctl.append(b);
      }
      ctl.sync = () => { for (const b of ctl.children) b.setAttribute('aria-pressed', String(b.dataset.v === sel.value)); };
    }
    sel.classList.add('enhanced');
    sel.after(ctl);
    enhanced.push(ctl);
  }
  const syncGfx = () => setTimeout(() => { for (const c of enhanced) c.sync(); }, 0);
  $('gfx').addEventListener('change', syncGfx);
  $('gfx').addEventListener('click', syncGfx);
  syncGfx();

  // -- disc: drag and drop, the disc card --
  const isDisc = (f) => /\.(iso|gcm)$/i.test(f.name);
  const isCard = (f) => /\.(raw|bin)$/i.test(f.name);
  let dragDepth = 0;
  const hasFiles = (e) => e.dataTransfer && [...e.dataTransfer.types].includes('Files');
  addEventListener('dragenter', (e) => { if (!hasFiles(e)) return; dragDepth++; body.classList.add('dragging'); e.preventDefault(); });
  addEventListener('dragover', (e) => { if (hasFiles(e)) { e.preventDefault(); e.dataTransfer.dropEffect = 'copy'; } });
  addEventListener('dragleave', () => { if (--dragDepth <= 0) { dragDepth = 0; body.classList.remove('dragging'); } });
  addEventListener('drop', (e) => {
    if (!hasFiles(e)) return;
    e.preventDefault();
    dragDepth = 0; body.classList.remove('dragging');
    const f = e.dataTransfer.files[0];
    if (!f) return;
    const into = isDisc(f) ? pick : isCard(f) ? $('cardImport') : null;
    if (!into) { toast('That is not a disc image (.iso / .gcm) or a memory card (.raw)', 'bad'); return; }
    if (into === pick && page && page.started()) { toast('The game is already running: reload the page to change the disc', 'bad'); return; }
    const dt = new DataTransfer();
    dt.items.add(f);
    into.files = dt.files;
    into.dispatchEvent(new Event('change', { bubbles: true }));
  });

  const ascii = (b) => String.fromCharCode(...b).replace(/\0.*$/s, '').trim();
  async function showDisc(f) {
    body.classList.toggle('has-disc', !!f);
    $('uiOnboard').hidden = !!f;
    $('uiDiscCard').hidden = !f;
    if (!f) return;
    $('uiDiscFile').textContent = f.name;
    $('uiDiscSize').textContent = (f.size / 1048576).toFixed(0) + ' MB';
    $('uiDiscTitle').textContent = f.name.replace(/\.[^.]+$/, '');
    $('uiDiscId').textContent = '';
    $('uiDiscEyebrow').textContent = 'Disc ready';
    try {
      const h = new Uint8Array(await f.slice(0, 0x60).arrayBuffer());
      const magic = ((h[0x1c] << 24) | (h[0x1d] << 16) | (h[0x1e] << 8) | h[0x1f]) >>> 0;
      if (magic === 0xc2339f3d) {
        const id = ascii(h.subarray(0, 6));
        if (id !== $('uiDiscTitle').textContent) $('uiDiscId').textContent = id;
        $('uiDiscTitle').title = ascii(h.subarray(0x20, 0x60));
      } else $('uiDiscEyebrow').textContent = 'Unrecognised disc image';
    } catch (e) {}
  }
  if (page && page.onDisc) page.onDisc(showDisc);
  else pick.addEventListener('change', () => showDisc(pick.files[0]));
  $('uiPlay').addEventListener('click', () => { if (!startBtn.disabled) startBtn.click(); });

  // -- race-start lights while the game boots --
  const attract = $('uiAttract'), loader = $('uiLoader'), lamps = [...loader.querySelectorAll('.lamp')];
  const STEPS = [['Reading your disc…', 'Spinning up the drive'], ['Warming up the engine…', 'Loading the game'],
                 ['Lining up on the grid…', 'Graphics and sound']];
  let stage = -1, want = -1, stepTimer = 0, booted = false, failTimer = 0;
  function setStep(n) {
    // the renderer is up: the picture follows shortly
    if (n === 2 && want < 2) setTimeout(() => setStep(3), 1500);
    want = Math.max(want, n);
    if (!stepTimer) advance();
  }
  function advance() {
    stepTimer = 0;
    if (stage >= want || booted) return;
    stage++;
    if (stage < 3) {
      lamps[stage].classList.add('on');
      $('uiLoaderCap').textContent = STEPS[stage][0];
      $('uiLoaderSub').textContent = STEPS[stage][1];
      blip(440);
      stepTimer = setTimeout(advance, calm() ? 50 : 650);
    } else go();
  }
  function go() {
    booted = true;
    clearTimeout(failTimer);
    loader.classList.add('go');
    $('uiLoaderCap').textContent = 'GO!';
    $('uiLoaderSub').textContent = '';
    blip(990);
    setTimeout(() => { attract.classList.add('out'); setTimeout(() => { attract.hidden = true; }, 520); }, calm() ? 200 : 700);
  }
  function bootStarted() {
    $('uiOnboard').hidden = $('uiDiscCard').hidden = true;
    loader.hidden = false;
    setStep(0);
    failTimer = setTimeout(() => setStep(3), 25000);
  }
  function stopped() {
    body.classList.add('stopped');
    body.classList.remove('racing');
    if (!booted && !loader.hidden) {
      loader.classList.add('fail');
      $('uiLoaderCap').textContent = 'Stopped';
      $('uiLoaderSub').textContent = 'Open the pit radio (log) for details';
      clearTimeout(stepTimer); clearTimeout(failTimer); booted = true;
    }
    toast('The game stopped: see the pit radio (log)', 'bad');
  }

  // -- log console: unread errors badge, plus page events read from the log --
  let unread = 0;
  function showUnread() {
    const b = $('uiLogBadge');
    b.hidden = !unread;
    b.textContent = unread > 99 ? '99+' : String(unread);
  }
  const ERR = /ABORT|EXCEPTION|Uncaught|\bERROR\b|\berror:|failed/i;
  new MutationObserver((records) => {
    for (const r of records) for (const n of r.addedNodes) {
      const text = n.textContent || '';
      for (const line of text.split('\n')) {
        if (!line) continue;
        if (ERR.test(line) && !$('uiConsole').classList.contains('open')) unread++;
        if (line.startsWith('ABORT')) stopped();
        else if (line.startsWith('[dvd]')) setStep(1);
        else if (line.startsWith('[webgl]') || line.startsWith('JAU_JAIInitializer')) setStep(2);
        else if (line.startsWith('[vi]') || line.startsWith('Demo ') || line.startsWith('Race ')) setStep(3);
        else if (line.startsWith('memory card: imported')) { toast('Save imported!', 'ok'); confetti(); }
        else if (line.startsWith('memory card: ')) toast(line.slice(13).replace(/^./, (c) => c.toUpperCase()), 'bad');
      }
    }
    showUnread();
  }).observe(logEl, { childList: true });
  // the FIFO log download link is inserted next to the log
  new MutationObserver((records) => {
    for (const r of records) for (const n of r.addedNodes) if (n.tagName === 'A') { toast(n.textContent, 'ok'); setOpen($('uiConsole'), true); }
  }).observe($('uiConsole'), { childList: true });

  // -- page state: started / ready / the solo start button --
  let wasStarted = false;
  function poll() {
    const started = !!(page && page.started());
    if (started && !wasStarted) { wasStarted = true; body.classList.add('racing'); bootStarted(); }
    const play = $('uiPlay');
    play.disabled = startBtn.disabled;
    play.hidden = startBtn.hidden;
    $('uiDiscHint').textContent = startBtn.hidden ? 'You are in a multiplayer room: the host starts the race.'
      : !(window.Module && Module.portReady) ? 'Warming up the engine…' : 'Keyboard or gamepad: see the controls below.';
  }
  setInterval(poll, 250);
  poll();

  // -- top bar chips: FPS while running, gamepad --
  let frames = 0, last = performance.now();
  (function fps(now) {
    requestAnimationFrame(fps);
    const chip = $('uiFps');
    if (!prefs.fps || !body.classList.contains('racing') || body.classList.contains('stopped')) { chip.hidden = true; return; }
    frames++;
    if (now - last >= 1000) {
      const v = Math.round(frames * 1000 / (now - last));
      frames = 0; last = now;
      chip.hidden = false;
      chip.firstElementChild.textContent = String(v);
      chip.classList.toggle('warn', v < 50 && v >= 28);
      chip.classList.toggle('bad', v < 28);
    }
  })(last);
  const padChip = () => { $('uiPad').hidden = ![...(navigator.getGamepads ? navigator.getGamepads() : [])].some((p) => p && p.connected); };
  addEventListener('gamepadconnected', (e) => { padChip(); toast('Gamepad connected: ' + e.gamepad.id.replace(/\s*\(.*$/, ''), 'ok'); setLegend('gp'); });
  addEventListener('gamepaddisconnected', padChip);

  // -- controller diagram: keyboard / gamepad labels, keys light up --
  function setLegend(mode) {
    prefs.legend = mode; savePrefs();
    for (const b of document.querySelectorAll('[data-legend]')) b.setAttribute('aria-pressed', String(b.dataset.legend === mode));
    for (const t of document.querySelectorAll('.pad .cap .k')) t.textContent = mode === 'gp' ? t.dataset.gp : t.dataset.kb;
  }
  for (const b of document.querySelectorAll('[data-legend]')) b.addEventListener('click', () => setLegend(b.dataset.legend));
  setLegend(prefs.legend === 'gp' ? 'gp' : 'kb');
  const HIT = { KeyX: 'A', KeyZ: 'B', KeyC: 'X', KeyV: 'Y', Space: 'Z', Enter: 'START', KeyQ: 'L', KeyW: 'R', Digit1: 'D', Digit2: 'D',
                ArrowLeft: 'S', ArrowRight: 'S', ArrowUp: 'S', ArrowDown: 'S', KeyI: 'C', KeyJ: 'C', KeyK: 'C', KeyL: 'C' };
  const lit = (code, on) => {
    const id = HIT[code];
    if (!id) return;
    for (const el of document.querySelectorAll(`.pad [data-hit="${id}"]`)) el.classList.toggle('down', on);
    for (const el of document.querySelectorAll(`.pad [data-cap="${id}"]`)) el.classList.toggle('lit', on);
  };
  addEventListener('keydown', (e) => lit(e.code, true));
  addEventListener('keyup', (e) => lit(e.code, false));
  addEventListener('blur', () => { for (const el of document.querySelectorAll('.pad .down, .pad .lit')) el.classList.remove('down', 'lit'); });

  // -- multiplayer (only with net.js + netui.js and the #mp block) --
  const mp = $('mp');
  if (mp && window.mkddNetUi) {
    $('uiMpBtn').hidden = false;
    if (page.params.has('netserver')) setOpen($('mpDrawer'), true);
    const code = $('mpRoomCode');
    const chips = document.createElement('span');
    chips.className = 'code-chips';
    chips.setAttribute('aria-hidden', 'true');
    code.after(chips);
    code.classList.add('chipped');
    new MutationObserver(() => {
      if (chips.dataset.code === code.textContent) return;
      chips.dataset.code = code.textContent;
      chips.textContent = '';
      for (const ch of code.textContent) { const i = document.createElement('i'); i.textContent = ch; chips.append(i); }
    }).observe(code, { childList: true, characterData: true, subtree: true });
    $('mpCopy').addEventListener('click', () => { if (code.textContent) toast('Room code ' + code.textContent + ' copied', 'ok'); });
    const summary = $('mpSummary'), badge = $('uiMpBadge');
    new MutationObserver(() => {
      const m = /(\d+) player/.exec(summary.textContent);
      badge.hidden = !m;
      if (m) badge.textContent = m[1];
    }).observe(summary, { childList: true, characterData: true, subtree: true });
    // the session starts when netui shows the in-game HUD: count down, wave the flags
    const hud = $('netHud');
    new MutationObserver(() => {
      if (hud.hidden || hud.dataset.counted) return;
      hud.dataset.counted = '1';
      setOpen($('mpDrawer'), false);
      countdown();
    }).observe(hud, { attributes: true, attributeFilter: ['hidden'] });
  }
  function countdown() {
    const box = $('uiCountdown');
    box.hidden = false;
    const seq = calm() ? ['GO!'] : ['3', '2', '1', 'GO!'];
    let i = 0;
    (function next() {
      if (i >= seq.length) { box.hidden = true; box.textContent = ''; return; }
      const s = document.createElement('span');
      s.textContent = seq[i];
      if (seq[i] === 'GO!') { s.className = 'go'; confetti(); blip(990); } else blip(520);
      box.textContent = ''; box.append(s);
      i++;
      setTimeout(next, 800);
    })();
  }
})();
