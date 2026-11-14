// Render Dolphin FIFO logs with the port's WebGL2 backend in headless Chrome.
//
//   port/gpu/tools/dffplay.sh                       (build build/port/dffplay)
//   node port/gpu/tools/dffplay.mjs <log.dff> <outdir> [first] [last]
// Writes <outdir>/ours-<frame>.ppm for every XFB copy of frames first..last
// (default: all) and <outdir>/play.log. DFFPLAY_WWW overrides the build
// directory, CHROME_FLAGS the GPU flags (default: SwiftShader), DFFPLAY_ENV a JSON
// object of port environment switches (e.g. {"MKDD_WGL_PICK": "253116"}).
import { spawn } from "node:child_process";
import { createServer } from "node:http";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../..");
const www = path.resolve(process.env.DFFPLAY_WWW || path.join(root, "build/port/dffplay"));
const [dffArg, outArg, firstArg, lastArg] = process.argv.slice(2);
if (!dffArg || !outArg) {
  console.error("usage: node dffplay.mjs <log.dff> <outdir> [first] [last]");
  process.exit(2);
}
const out = path.resolve(outArg);
await mkdir(out, { recursive: true });
const first = Number(firstArg ?? 0), last = Number(lastArg ?? -1);

const types = { ".html": "text/html", ".js": "text/javascript", ".wasm": "application/wasm" };
const server = createServer(async (req, res) => {
  try {
    const url = new URL(req.url, "http://x");
    const file = url.pathname === "/log.dff" ? path.resolve(dffArg)
      : path.join(www, url.pathname === "/" ? "index.html" : path.normalize(url.pathname).replace(/^\/+/, ""));
    const data = await readFile(file);
    res.writeHead(200, { "content-type": types[path.extname(file)] || "application/octet-stream" });
    res.end(data);
  } catch {
    res.writeHead(404); res.end();
  }
});
await new Promise((r) => server.listen(0, "127.0.0.1", r));
const httpPort = server.address().port, debugPort = httpPort + 1;
const flags = (process.env.CHROME_FLAGS || "--enable-unsafe-swiftshader --use-angle=swiftshader").split(" ").filter(Boolean);
const browser = spawn(process.env.CHROME_BIN || "google-chrome", [
  "--headless=new", "--no-sandbox", "--disable-dev-shm-usage", ...flags,
  `--remote-debugging-port=${debugPort}`, `--user-data-dir=${path.join(out, "chrome-profile")}`,
  "--window-size=800,700", "about:blank",
], { stdio: "ignore" });
const cleanup = () => { browser.kill(); server.close(); };
process.on("exit", cleanup);

const delay = (ms) => new Promise((r) => setTimeout(r, ms));
async function waitFor(check, what, ms = 30_000) {
  const t0 = Date.now();
  while (Date.now() - t0 < ms) {
    try { const v = await check(); if (v) return v; } catch {}
    await delay(150);
  }
  throw new Error(`timed out waiting for ${what}`);
}
const target = await waitFor(async () => {
  const list = await (await fetch(`http://127.0.0.1:${debugPort}/json/list`)).json();
  return list.find((t) => t.type === "page");
}, "chrome");
const ws = new WebSocket(target.webSocketDebuggerUrl);
await new Promise((r, j) => { ws.onopen = r; ws.onerror = j; });
let nextId = 1;
const pending = new Map();
ws.onmessage = (ev) => {
  const m = JSON.parse(String(ev.data));
  if (m.id && pending.has(m.id)) {
    const p = pending.get(m.id); pending.delete(m.id);
    m.error ? p.reject(new Error(m.error.message)) : p.resolve(m.result);
  }
};
const send = (method, params = {}) => new Promise((resolve, reject) => {
  const id = nextId++; pending.set(id, { resolve, reject });
  ws.send(JSON.stringify({ id, method, params }));
});
const evaluate = async (expr) => {
  const r = await send("Runtime.evaluate", { expression: expr, awaitPromise: true, returnByValue: true });
  if (r.exceptionDetails) throw new Error(r.exceptionDetails.exception?.description || r.exceptionDetails.text);
  return r.result.value;
};

await send("Runtime.enable");
await send("Page.navigate", { url: `http://127.0.0.1:${httpPort}/` });
await waitFor(() => evaluate("typeof Module !== 'undefined' && Module.ready === true"), "player");
await evaluate(`Object.assign(Module.env, ${process.env.DFFPLAY_ENV || "{}"}); true`);
const t0 = Date.now();
const n = await evaluate(`(async () => {
  const b = new Uint8Array(await (await fetch('/log.dff')).arrayBuffer());
  return playDff(b, ${first}, ${last});
})()`);
const renderer = await evaluate(`(() => { const gl = document.createElement('canvas').getContext('webgl2');
  const e = gl && gl.getExtension('WEBGL_debug_renderer_info'); return e ? gl.getParameter(e.UNMASKED_RENDERER_WEBGL) : '?'; })()`);
const files = await evaluate("Module.FS.readdir('/out').filter(f => f.endsWith('.ppm'))");
for (const f of files) {
  const b64 = await evaluate(`(() => { const d = Module.FS.readFile('/out/${f}'); let s = '';
    for (let i = 0; i < d.length; i += 32768) s += String.fromCharCode.apply(null, d.subarray(i, i + 32768)); return btoa(s); })()`);
  await writeFile(path.join(out, f.replace(/^cap\./, "")), Buffer.from(b64, "base64"));
}
const log = await evaluate("document.getElementById('log').textContent");
await writeFile(path.join(out, "play.log"), log);
console.log(`played ${n} frames in ${((Date.now() - t0) / 1000).toFixed(1)} s, ${files.length} captures in ${out} (${renderer})`);
if (log.trim()) console.log(log.split("\n").slice(-10).join("\n"));
ws.close();
cleanup();
process.exit(0);
