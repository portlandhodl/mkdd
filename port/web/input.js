// Controller input for the web port: keyboard and the Gamepad API -> GameCube
// controller port 1, sent to the game every animation frame (port_pad_set).
(() => {
  // PADStatus.button bits (include/dolphin/pad.h)
  const B = { LEFT: 0x0001, RIGHT: 0x0002, DOWN: 0x0004, UP: 0x0008, Z: 0x0010, R: 0x0020, L: 0x0040,
              A: 0x0100, B: 0x0200, X: 0x0400, Y: 0x0800, START: 0x1000 };
  const STICK = 80; // full deflection on a real controller is about +/-80

  // Keyboard layout: arrows steer, X accelerates (A), Z brakes (B), Q/W are the
  // L/R triggers (drift), Space uses an item (Z), C/V are X/Y, Enter is Start,
  // I/J/K/L move the C-stick.
  const keyButtons = { KeyX: B.A, KeyZ: B.B, KeyC: B.X, KeyV: B.Y, Space: B.Z, Enter: B.START,
                       KeyQ: B.L, KeyW: B.R, Digit1: B.LEFT, Digit2: B.RIGHT };
  const down = new Set();
  const handled = new Set([...Object.keys(keyButtons), 'ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown',
                           'KeyI', 'KeyJ', 'KeyK', 'KeyL']);
  addEventListener('keydown', (e) => { if (handled.has(e.code)) { down.add(e.code); e.preventDefault(); } });
  addEventListener('keyup', (e) => { down.delete(e.code); });
  addEventListener('blur', () => down.clear());

  const clampS8 = (v) => Math.max(-128, Math.min(127, Math.round(v)));
  const axis = (neg, pos) => (down.has(pos) ? 1 : 0) - (down.has(neg) ? 1 : 0);

  function readKeyboard() {
    let button = 0;
    for (const [code, bit] of Object.entries(keyButtons)) if (down.has(code)) button |= bit;
    return {
      button,
      sx: axis('ArrowLeft', 'ArrowRight') * STICK, sy: axis('ArrowDown', 'ArrowUp') * STICK,
      cx: axis('KeyJ', 'KeyL') * STICK, cy: axis('KeyK', 'KeyI') * STICK,
      tl: down.has('KeyQ') ? 255 : 0, tr: down.has('KeyW') ? 255 : 0,
    };
  }

  // Standard-mapping gamepads (Xbox/PlayStation layout and most GC adapters).
  function readGamepad() {
    const pads = navigator.getGamepads ? navigator.getGamepads() : [];
    for (const p of pads) {
      if (!p || !p.connected) continue;
      const b = (i) => p.buttons[i] && p.buttons[i].pressed;
      const v = (i) => (p.buttons[i] ? p.buttons[i].value : 0);
      const dz = (x) => (Math.abs(x) < 0.15 ? 0 : x);
      let button = 0;
      if (b(0)) button |= B.A;
      if (b(1)) button |= B.B;
      if (b(2)) button |= B.X;
      if (b(3)) button |= B.Y;
      if (b(5)) button |= B.Z;
      if (b(9)) button |= B.START;
      if (b(12)) button |= B.UP;
      if (b(13)) button |= B.DOWN;
      if (b(14)) button |= B.LEFT;
      if (b(15)) button |= B.RIGHT;
      const tl = v(6), tr = v(7);
      if (tl > 0.8 || b(4)) button |= B.L;
      if (tr > 0.8) button |= B.R;
      return {
        button,
        sx: dz(p.axes[0] || 0) * STICK, sy: -dz(p.axes[1] || 0) * STICK,
        cx: dz(p.axes[2] || 0) * STICK, cy: -dz(p.axes[3] || 0) * STICK,
        tl: Math.round((b(4) ? 1 : tl) * 255), tr: Math.round(tr * 255),
      };
    }
    return null;
  }

  function merge(a, g) {
    if (!g) return a;
    const pick = (x, y) => (Math.abs(y) > Math.abs(x) ? y : x);
    return { button: a.button | g.button, sx: pick(a.sx, g.sx), sy: pick(a.sy, g.sy), cx: pick(a.cx, g.cx),
             cy: pick(a.cy, g.cy), tl: Math.max(a.tl, g.tl), tr: Math.max(a.tr, g.tr) };
  }

  let send = null;
  function frame() {
    if (!send && typeof Module !== 'undefined' && Module.calledRun && Module.cwrap) {
      send = Module.cwrap('port_pad_set', null, ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']);
    }
    if (send) {
      const s = merge(readKeyboard(), readGamepad());
      send(0, s.button, clampS8(s.sx), clampS8(s.sy), clampS8(s.cx), clampS8(s.cy), s.tl, s.tr);
    }
    requestAnimationFrame(frame);
  }
  requestAnimationFrame(frame);
})();
