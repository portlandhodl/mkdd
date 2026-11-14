// Render the WebGL2 backend test page in headless Chrome (SwiftShader) and
// screenshot it: node port/gpu/tools/webgl_test.mjs
// Usage: node port/gpu/tools/webgl_test.mjs [scene 1|2]
// Writes build/port/webgl_test/shot<scene>.png and log<scene>.txt.
import { spawn } from "node:child_process";
import { writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const scene = process.argv[2] || "1";
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../..");
const www = path.join(root, "build/port/webgl_test");
const httpPort = 18_538, debugPort = 19_538;
const server = spawn("python3", ["-m", "http.server", String(httpPort), "--bind", "127.0.0.1", "--directory", www], { stdio: "ignore" });
const browser = spawn(process.env.CHROME_BIN || "google-chrome", [
  "--headless=new", "--no-sandbox", "--disable-dev-shm-usage",
  "--enable-unsafe-swiftshader", "--use-angle=swiftshader",
  `--remote-debugging-port=${debugPort}`, `--user-data-dir=${path.join(www, "chrome-profile")}`,
  "--window-size=700,800", "about:blank",
], { stdio: "ignore" });
const cleanup = () => { browser.kill(); server.kill(); };
process.on("exit", cleanup);
const delay = (ms) => new Promise((r) => setTimeout(r, ms));
async function waitFor(check, what, timeout = 20_000) {
  const t0 = Date.now();
  while (Date.now() - t0 < timeout) {
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
const extra = [];
ws.onmessage = (ev) => {
  const m = JSON.parse(String(ev.data));
  if (m.id && pending.has(m.id)) {
    const p = pending.get(m.id); pending.delete(m.id);
    m.error ? p.reject(new Error(m.error.message)) : p.resolve(m.result);
  } else if (m.method === "Runtime.exceptionThrown") {
    extra.push("EXCEPTION " + (m.params.exceptionDetails.exception?.description || m.params.exceptionDetails.text));
  } else if (m.method === "Runtime.consoleAPICalled") {
    extra.push("console: " + m.params.args.map((a) => a.value ?? a.description).join(" "));
  }
};
const send = (method, params = {}) => new Promise((resolve, reject) => {
  const id = nextId++; pending.set(id, { resolve, reject });
  ws.send(JSON.stringify({ id, method, params }));
});
const evaluate = async (expr) => (await send("Runtime.evaluate", { expression: expr, returnByValue: true })).result.value;
await send("Page.enable");
await send("Runtime.enable");
await send("Page.navigate", { url: `http://127.0.0.1:${httpPort}/?scene=${scene}` });
let log = "";
try {
  await waitFor(async () => { log = await evaluate("document.getElementById('log').textContent"); return /WEBGL_TEST_(DONE|FAIL)/.test(log); }, "test", 30_000);
} catch (e) { extra.push(String(e)); log = await evaluate("document.getElementById('log').textContent"); }
await delay(500);
const shot = await send("Page.captureScreenshot", { format: "png", clip: { x: 0, y: 0, width: 640, height: 480, scale: 1 } });
await writeFile(path.join(www, `shot${scene}.png`), Buffer.from(shot.data, "base64"));
await writeFile(path.join(www, `log${scene}.txt`), log + "\n" + extra.join("\n"));
console.log(log + (extra.length ? "\n" + extra.join("\n") : ""));
console.log(`wrote ${path.join(www, `shot${scene}.png`)}`);
ws.close();
cleanup();
process.exit(0);
