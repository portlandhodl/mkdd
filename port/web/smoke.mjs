// Headless Chrome smoke test for the web port.
//
//   MKDD_ISO=/path/to/GM4E01.iso node port/web/smoke.mjs [seconds] [query]
// Serves build/port/web, picks the disc image through the page's file input
// (the file stays local), presses Start, and after `seconds` (default 40) writes
// the page log and a screenshot to build/port/smoke/. Query defaults to "fast=1".
// MKDD_SHOTS="20,35,50" also saves canvas captures at those seconds
// (shot-<s>.png); MKDD_SMOKE_OUT overrides the output directory.
import { spawn } from "node:child_process";
import { mkdir, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
const www = path.join(root, "build/port/web");
const out = process.env.MKDD_SMOKE_OUT ? path.resolve(process.env.MKDD_SMOKE_OUT) : path.join(root, "build/port/smoke");
const iso = process.env.MKDD_ISO;
const seconds = Number(process.argv[2] || 40);
const query = process.argv[3] ?? "fast=1";
if (!iso) {
  console.error("set MKDD_ISO to your disc image");
  process.exit(2);
}
await mkdir(out, { recursive: true });

const httpPort = 18_537, debugPort = 19_537;
const server = spawn("python3", ["-m", "http.server", String(httpPort), "--bind", "127.0.0.1", "--directory", www], { stdio: "ignore" });
const browser = spawn(process.env.CHROME_BIN || "google-chrome", [
  "--headless=new", "--no-sandbox", "--disable-dev-shm-usage",
  "--enable-unsafe-swiftshader", "--use-angle=swiftshader",
  `--remote-debugging-port=${debugPort}`, `--user-data-dir=${path.join(out, "chrome-profile")}`,
  "--window-size=1100,1000", "about:blank",
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
const consoleLines = [];
ws.onmessage = (ev) => {
  const m = JSON.parse(String(ev.data));
  if (m.id && pending.has(m.id)) {
    const p = pending.get(m.id); pending.delete(m.id);
    m.error ? p.reject(new Error(m.error.message)) : p.resolve(m.result);
  } else if (m.method === "Runtime.exceptionThrown") {
    consoleLines.push("EXCEPTION " + (m.params.exceptionDetails.exception?.description || m.params.exceptionDetails.text));
  }
};
const send = (method, params = {}) => new Promise((resolve, reject) => {
  const id = nextId++; pending.set(id, { resolve, reject });
  ws.send(JSON.stringify({ id, method, params }));
});
const evaluate = async (expr) => (await send("Runtime.evaluate", { expression: expr, awaitPromise: true, returnByValue: true })).result.value;

await send("Page.enable");
await send("Runtime.enable");
await send("DOM.enable");
await send("Page.navigate", { url: `http://127.0.0.1:${httpPort}/?${query}` });
await waitFor(() => evaluate("typeof Module !== 'undefined' && !!Module.calledRun === false && document.readyState === 'complete'"), "page");
await waitFor(() => evaluate("typeof Module.callMain === 'function'"), "runtime");

const doc = await send("DOM.getDocument");
const { nodeId } = await send("DOM.querySelector", { nodeId: doc.root.nodeId, selector: "#pick" });
await send("DOM.setFileInputFiles", { nodeId, files: [path.resolve(iso)] });
await waitFor(() => evaluate("!document.getElementById('start').disabled"), "start button");
await evaluate("document.getElementById('start').click(); true");

const shots = (process.env.MKDD_SHOTS || "").split(",").filter(Boolean).map(Number).sort((a, b) => a - b);
const t0 = Date.now();
while (Date.now() - t0 < seconds * 1000) {
  await delay(250);
  while (shots.length && Date.now() - t0 >= shots[0] * 1000) {
    const s = shots.shift();
    const clip = await evaluate("(() => { const r = document.getElementById('screen').getBoundingClientRect(); return { x: r.left + scrollX, y: r.top + scrollY, width: r.width, height: r.height, scale: 1 }; })()");
    const img = await send("Page.captureScreenshot", { format: "png", clip });
    await writeFile(path.join(out, `shot-${s}.png`), Buffer.from(img.data, "base64"));
  }
}

const log = await evaluate("document.getElementById('log').textContent");
await writeFile(path.join(out, "page.log"), log + "\n" + consoleLines.join("\n"));
const shot = await send("Page.captureScreenshot", { format: "png" });
await writeFile(path.join(out, "screen.png"), Buffer.from(shot.data, "base64"));
console.log(log.split("\n").filter((l) => !l.startsWith("[sdk] stub")).slice(-25).join("\n"));
if (consoleLines.length) console.log(consoleLines.join("\n"));
console.log(`\nwrote ${path.join(out, "page.log")} and screen.png`);
ws.close();
cleanup();
process.exit(0);
