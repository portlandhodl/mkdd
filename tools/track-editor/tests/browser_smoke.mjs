import { spawn } from "node:child_process";
import { mkdir, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const crate = fileURLToPath(new URL("../", import.meta.url));
const www = path.join(crate, "www");
const archive = process.env.MKDD_TEST_ARC;
const output = process.env.MKDD_EDITOR_TEST_OUT || path.join(crate, "build");
const chrome = process.env.CHROME_BIN || "google-chrome";
if (!archive) {
  console.error("Set MKDD_TEST_ARC to a user-owned course ARC before running the browser smoke test.");
  process.exit(2);
}

await mkdir(output, { recursive: true });
const httpPort = 18_437;
const debugPort = 19_437;
const server = spawn("python3", ["-m", "http.server", String(httpPort), "--bind", "127.0.0.1", "--directory", www], {
  stdio: "ignore",
});
const browser = spawn(chrome, [
  "--headless=new",
  "--no-sandbox",
  "--disable-dev-shm-usage",
  "--enable-unsafe-swiftshader",
  "--use-angle=swiftshader",
  `--remote-debugging-port=${debugPort}`,
  `--user-data-dir=${path.join(output, "chrome-profile")}`,
  "about:blank",
], { stdio: "ignore" });

const delay = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
async function waitFor(check, description, timeout = 20_000) {
  const start = Date.now();
  let lastError;
  while (Date.now() - start < timeout) {
    try {
      const value = await check();
      if (value) return value;
    } catch (error) {
      lastError = error;
    }
    await delay(100);
  }
  throw new Error(`Timed out waiting for ${description}${lastError ? `: ${lastError}` : ""}`);
}

class Cdp {
  constructor(url) {
    this.socket = new WebSocket(url);
    this.nextId = 1;
    this.pending = new Map();
    this.errors = [];
  }
  async open() {
    await new Promise((resolve, reject) => {
      this.socket.addEventListener("open", resolve, { once: true });
      this.socket.addEventListener("error", reject, { once: true });
    });
    this.socket.addEventListener("message", (event) => {
      const message = JSON.parse(String(event.data));
      if (message.id) {
        const pending = this.pending.get(message.id);
        if (!pending) return;
        this.pending.delete(message.id);
        if (message.error) pending.reject(new Error(`${pending.method}: ${message.error.message}`));
        else pending.resolve(message.result);
      } else if (message.method === "Runtime.exceptionThrown") {
        this.errors.push(message.params.exceptionDetails);
      } else if (message.method === "Log.entryAdded" && message.params.entry.level === "error") {
        this.errors.push(message.params.entry);
      } else if (message.method === "Page.javascriptDialogOpening") {
        void this.send("Page.handleJavaScriptDialog", { accept: true }).catch(() => {});
      }
    });
  }
  send(method, params = {}) {
    const id = this.nextId++;
    return new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject, method });
      this.socket.send(JSON.stringify({ id, method, params }));
    });
  }
  async evaluate(expression) {
    const response = await this.send("Runtime.evaluate", {
      expression,
      awaitPromise: true,
      returnByValue: true,
      userGesture: true,
    });
    if (response.exceptionDetails) {
      throw new Error(response.exceptionDetails.exception?.description || response.exceptionDetails.text);
    }
    return response.result?.value;
  }
}

let cdp;
try {
  await waitFor(() => fetch(`http://127.0.0.1:${debugPort}/json/version`).then((r) => r.ok), "Chrome DevTools");
  const pageUrl = `http://127.0.0.1:${httpPort}/`;
  const target = await fetch(`http://127.0.0.1:${debugPort}/json/new?${encodeURIComponent(pageUrl)}`, { method: "PUT" }).then((r) => r.json());
  cdp = new Cdp(target.webSocketDebuggerUrl);
  await cdp.open();
  await cdp.send("Page.enable");
  await cdp.send("Runtime.enable");
  await cdp.send("Log.enable");
  await cdp.send("Emulation.setDeviceMetricsOverride", {
    width: 1600,
    height: 1000,
    deviceScaleFactor: 1,
    mobile: false,
  });
  await waitFor(() => cdp.evaluate(`document.getElementById("engine-label")?.textContent === "Ready"`), "Rust/WASM startup");
  await cdp.evaluate(`document.getElementById("open-file-input").removeAttribute("hidden")`);
  const { root } = await cdp.send("DOM.getDocument");
  const { nodeId } = await cdp.send("DOM.querySelector", { nodeId: root.nodeId, selector: "#open-file-input" });
  await cdp.send("DOM.setFileInputFiles", { nodeId, files: [path.resolve(archive)] });
  await waitFor(() => cdp.evaluate(`document.getElementById("empty-state")?.hidden && Number(document.getElementById("entity-count")?.textContent) > 0`), "course import and entity tree", 30_000);
  const opened = await cdp.evaluate(`({
    file: document.getElementById("outliner-file-name").textContent,
    entities: Number(document.getElementById("entity-count").textContent),
    files: Number(document.getElementById("archive-count").textContent),
    modelLayer: [...document.querySelectorAll("#layer-list label")].some((label) => label.textContent.toLowerCase().includes("course model")),
    canvasWidth: document.getElementById("viewport").width,
    canvasHeight: document.getElementById("viewport").height
  })`);
  if (!opened.entities || !opened.files || !opened.canvasWidth || !opened.canvasHeight || !opened.modelLayer) {
    throw new Error(`Course smoke state is incomplete: ${JSON.stringify(opened)}`);
  }
  console.log(`Opened ${opened.file}: ${opened.entities} entities, ${opened.files} files`);
  for (const expected of [false, true]) {
    await cdp.evaluate(`(() => {
      const label = [...document.querySelectorAll("#layer-list label")].find((row) => row.textContent.toLowerCase().includes("course model"));
      const checkbox = label?.querySelector("input");
      if (!checkbox || checkbox.checked === ${expected}) return false;
      checkbox.click();
      return true;
    })()`);
    await delay(150);
  }

  await cdp.evaluate(`document.querySelector("#entity-list button").click()`);
  await waitFor(() => cdp.evaluate(`!document.getElementById("selection-content")?.hidden`), "entity inspector");
  await waitFor(() => cdp.evaluate(`document.getElementById("engine-label").textContent === "Ready"`), "selection completion");
  await cdp.evaluate(`document.getElementById("add-object-button").click()`);
  await waitFor((previous) => cdp.evaluate(`Number(document.getElementById("entity-count").textContent)`) .then((count) => count === opened.entities + 1), "object insertion");
  const dirty = await cdp.evaluate(`document.getElementById("dirty-label").textContent`);
  if (dirty !== "Unsaved") throw new Error(`Expected unsaved state, got ${dirty}`);
  await waitFor(() => cdp.evaluate(`document.getElementById("engine-label").textContent === "Ready"`), "insert completion");
  await cdp.evaluate(`document.getElementById("undo-button").click()`);
  await waitFor((previous) => cdp.evaluate(`Number(document.getElementById("entity-count").textContent)`).then((count) => count === opened.entities), "undo");
  await waitFor(() => cdp.evaluate(`document.getElementById("engine-label").textContent === "Ready"`), "undo completion");
  await cdp.evaluate(`document.getElementById("redo-button").click()`);
  await waitFor((previous) => cdp.evaluate(`Number(document.getElementById("entity-count").textContent)`).then((count) => count === opened.entities + 1), "redo");

  await waitFor(() => cdp.evaluate(`document.getElementById("engine-label").textContent === "Ready"`), "history completion");
  const selectedMap = await cdp.evaluate(`(() => {
    const row = [...document.querySelectorAll("#file-list button")].find((button) => button.title.toLowerCase().includes("map.bti"));
    if (!row) return false;
    row.click();
    return true;
  })()`);
  if (selectedMap) {
    await waitFor(() => cdp.evaluate(`(() => {
      const canvas = document.getElementById("texture-preview");
      return !canvas.hidden && canvas.width > 0 && canvas.height > 0;
    })()`), "BTI texture preview");
  }

  console.log("Object add/undo/redo and texture preview passed");
  await cdp.evaluate(`document.getElementById("frame-button").click()`);
  await delay(300);
  const { data } = await cdp.send("Page.captureScreenshot", { format: "png", captureBeyondViewport: false });
  const screenshot = path.join(output, "browser-smoke.png");
  await writeFile(screenshot, Buffer.from(data, "base64"));

  let attached;
  if (process.env.MKDD_TEST_BOL) {
    console.log("Testing standalone BOL plus related ARC attachment");
    await cdp.send("Page.reload", { ignoreCache: true });
    await delay(250);
    await waitFor(() => cdp.evaluate(`document.getElementById("engine-label")?.textContent === "Ready"`), "standalone BOL startup");
    const { root: freshRoot } = await cdp.send("DOM.getDocument");
    const { nodeId: freshInput } = await cdp.send("DOM.querySelector", { nodeId: freshRoot.nodeId, selector: "#open-file-input" });
    await cdp.send("DOM.setFileInputFiles", { nodeId: freshInput, files: [path.resolve(process.env.MKDD_TEST_BOL)] });
    await waitFor(() => cdp.evaluate(`document.getElementById("empty-state")?.hidden && Number(document.getElementById("entity-count")?.textContent) === ${opened.entities} && Number(document.getElementById("archive-count")?.textContent) === 0`), "standalone BOL import", 30_000);
    console.log("Standalone BOL loaded");
    const { root: relatedRoot } = await cdp.send("DOM.getDocument");
    const { nodeId: relatedInput } = await cdp.send("DOM.querySelector", { nodeId: relatedRoot.nodeId, selector: "#related-file-input" });
    await cdp.send("DOM.setFileInputFiles", { nodeId: relatedInput, files: [path.resolve(archive)] });
    await waitFor(() => cdp.evaluate(`Number(document.getElementById("archive-count").textContent) === ${opened.files} && document.getElementById("engine-label").textContent === "Ready"`), "related ARC attachment", 30_000);
    attached = await cdp.evaluate(`({
      fileName: document.getElementById("outliner-file-name").textContent,
      entities: Number(document.getElementById("entity-count").textContent),
      files: Number(document.getElementById("archive-count").textContent)
    })`);
    if (attached.entities !== opened.entities || attached.files !== opened.files || !attached.fileName.toLowerCase().endsWith(".bol")) {
      throw new Error(`Attaching ARC changed the active course incorrectly: ${JSON.stringify(attached)}`);
    }
  }
  if (cdp.errors.length) throw new Error(`Browser errors: ${JSON.stringify(cdp.errors)}`);
  console.log(JSON.stringify({ ...opened, selectedMap, attached, screenshot, result: "PASS" }, null, 2));
} finally {
  if (cdp?.socket?.readyState === WebSocket.OPEN) cdp.socket.close();
  browser.kill("SIGTERM");
  server.kill("SIGTERM");
}
