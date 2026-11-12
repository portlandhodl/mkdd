const byId = (id) => document.getElementById(id);
const dom = {
  app: byId("app"),
  workspace: byId("workspace"),
  openInput: byId("open-file-input"),
  relatedInput: byId("related-file-input"),
  replaceInput: byId("replace-asset-input"),
  libraryInput: byId("library-file-input"),
  openButton: byId("open-file-button"),
  relatedButton: byId("related-file-button"),
  addObjectButton: byId("add-object-button"),
  duplicateButton: byId("duplicate-button"),
  deleteButton: byId("delete-button"),
  focusButton: byId("focus-button"),
  floorButton: byId("floor-button"),
  undoButton: byId("undo-button"),
  redoButton: byId("redo-button"),
  exportMenuRoot: byId("export-menu-root"),
  exportMenuButton: byId("export-bol-menu-button"),
  exportMenu: byId("export-bol-menu"),
  exportBolButton: byId("export-bol-button"),
  exportOriginalButton: byId("export-original-bol-button"),
  exportArchiveButton: byId("export-archive-button"),
  helpButton: byId("help-button"),
  helpOverlay: byId("help-overlay"),
  helpClose: byId("help-close"),
  emptyOpenButton: byId("empty-open-button"),
  emptyState: byId("empty-state"),
  emptyKicker: byId("empty-kicker"),
  emptyTitle: byId("empty-title"),
  emptyDescription: byId("empty-description"),
  engineBadge: byId("engine-badge"),
  engineLabel: byId("engine-label"),
  outlinerFileName: byId("outliner-file-name"),
  documentKind: byId("document-kind"),
  courseSummary: byId("course-summary"),
  archiveCount: byId("archive-count"),
  entityCount: byId("entity-count"),
  fileList: byId("file-list"),
  entityList: byId("entity-list"),
  outlinerSearch: byId("outliner-search"),
  placeList: byId("place-list"),
  placeSearch: byId("place-search"),
  placeModelsOnly: byId("place-models-only"),
  placeShowInternal: byId("place-show-internal"),
  libraryList: byId("library-list"),
  addLibraryButton: byId("add-library-button"),
  layerList: byId("layer-list"),
  showButton: byId("show-button"),
  showMenu: byId("show-menu"),
  showMenuRoot: byId("show-menu-root"),
  gameViewButton: byId("game-view-button"),
  snapToggle: byId("snap-toggle"),
  snapTranslate: byId("snap-translate"),
  snapRotate: byId("snap-rotate"),
  shadingButton: byId("shading-button"),
  shadingLabel: byId("shading-label"),
  perspectiveButton: byId("perspective-button"),
  topButton: byId("top-button"),
  cameraSpeed: byId("camera-speed"),
  cameraSpeedLabel: byId("camera-speed-label"),
  viewportWrap: byId("viewport-wrap"),
  viewport: byId("viewport"),
  viewportTip: byId("viewport-tip"),
  viewportFileName: byId("viewport-file-name"),
  viewportDirty: byId("viewport-dirty-mark"),
  frameButton: byId("frame-button"),
  dropOverlay: byId("drop-overlay"),
  detailsSearch: byId("details-search"),
  selectionEmpty: byId("selection-empty"),
  selectionContent: byId("selection-content"),
  selectionTitle: byId("selection-title"),
  selectionKind: byId("selection-kind"),
  selectionIndex: byId("selection-index"),
  selectionSwatch: byId("selection-swatch"),
  selectionFields: byId("selection-fields"),
  rawDataDetails: byId("raw-data-details"),
  rawHex: byId("raw-hex"),
  headerSection: byId("course-header-section"),
  headerFields: byId("header-fields"),
  assetSection: byId("asset-section"),
  assetName: byId("asset-name"),
  assetMetadata: byId("asset-metadata"),
  assetInfo: byId("asset-info"),
  texturePreview: byId("texture-preview"),
  textureEmpty: byId("texture-empty"),
  replaceAssetButton: byId("replace-asset-button"),
  downloadAssetButton: byId("download-asset-button"),
  issuesDrawer: byId("issues-drawer"),
  issuesList: byId("issues-list"),
  issuesToggle: byId("issues-toggle"),
  issuesClose: byId("issues-close-button"),
  issuesCount: byId("issues-count"),
  issuesDrawerCount: byId("issues-drawer-count"),
  statusMessage: byId("status-message"),
  cursorPosition: byId("cursor-position"),
  toolReadout: byId("tool-readout"),
  transferStatus: byId("transfer-status"),
  transferLabel: byId("transfer-label"),
  transferProgress: byId("transfer-progress"),
  dirtyIndicator: byId("dirty-indicator"),
  dirtyLabel: byId("dirty-label"),
  statusFileName: byId("status-file-name"),
  noticeRegion: byId("notice-region"),
};

const COURSE_EXTENSIONS = new Set(["bol", "arc", "szs"]);
const RELATED_EXTENSIONS = new Set(["bco", "bti", "bmd", "bdl", "arc", "szs"]);
const HELPER_LAYERS = ["grid", "objects", "routes", "checkpoints", "centerline", "starts", "areas", "cameras", "respawns", "collision"];
const KIND_INFO = {
  object: { label: "Objects", color: "#5ba7ff" },
  start: { label: "Start Points", color: "#38f266" },
  checkpoint: { label: "Checkpoints", color: "#ff9e1f" },
  checkpoint_group: { label: "Checkpoint Groups", color: "#cc7d18" },
  path: { label: "Routes", color: "#7aadff" },
  route_point: { label: "Route Points", color: "#7aadff" },
  centerline: { label: "Centerline", color: "#1fd1eb" },
  respawn: { label: "Respawns", color: "#ff523d" },
  area: { label: "Areas", color: "#3da3f0" },
  camera: { label: "Cameras", color: "#d661f0" },
  light: { label: "Lights", color: "#ffe680" },
  mg: { label: "Minigame", color: "#a0a0a0" },
};
const CATEGORY_COLORS = {
  Items: "#ffc738", "Track Feature": "#59b8ff", Scenery: "#6bdb73", Hazard: "#ff614d", Traffic: "#fa943d",
  Effects: "#cc8cff", Generator: "#9e9efa", Gameplay: "#4de6d9", Water: "#4d99ff", Audio: "#f280bf",
};
const TOOL_LABELS = { select: "Select", translate: "Move", rotate: "Rotate", scale: "Scale" };

// The document and all edits live in Rust. This cache is only the latest snapshot.
let editor = null;
let currentSnapshot = null;
let engineReady = false;
let busy = false;
let fieldId = 0;
let renderRequest = 0;
let resizeRequest = 0;
let textureRequest = 0;
let dragDepth = 0;
let pointerGesture = null;
let tool = "translate";
let gameViewStash = null;
const collapsedGroups = new Set(["checkpoint_group", "light", "mg", "path"]);
const collapsedFieldGroups = new Set(["Advanced"]);
const keysDown = new Set();
let flyFrame = 0;
let lastFlyTime = 0;
let cursorTimer = 0;
let lastCursorEvent = null;
let lastEntitiesKey = "";
let lastPlaceKey = "";

function makeElement(tag, className, text) {
  const element = document.createElement(tag);
  if (className) element.className = className;
  if (text !== undefined && text !== null) element.textContent = String(text);
  return element;
}

function makeIcon(name) {
  const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  svg.setAttribute("class", "icon");
  svg.setAttribute("aria-hidden", "true");
  const use = document.createElementNS("http://www.w3.org/2000/svg", "use");
  use.setAttribute("href", `#icon-${name}`);
  svg.append(use);
  return svg;
}

const asArray = (value) => Array.isArray(value) ? value : [];
const isObject = (value) => value !== null && typeof value === "object" && !Array.isArray(value);
const hex4 = (value) => `0x${Number(value).toString(16).padStart(4, "0")}`;

function hasDocument() {
  return Boolean(currentSnapshot && (
    currentSnapshot.file_name ||
    asArray(currentSnapshot.entities).length ||
    asArray(currentSnapshot.archive?.files).length
  ));
}

function documentName() {
  return String(currentSnapshot?.file_name || "Untitled course");
}

function extensionOf(name) {
  const parts = String(name || "").toLowerCase().split(".");
  return parts.length > 1 ? parts.pop() : "";
}

function humanize(value) {
  const words = String(value || "").replace(/[_-]+/g, " ").replace(/([a-z])([A-Z])/g, "$1 $2").trim();
  return words ? words.charAt(0).toUpperCase() + words.slice(1) : "Value";
}

function formatBytes(value) {
  const bytes = Number(value);
  if (!Number.isFinite(bytes) || bytes < 0) return "Unknown size";
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(bytes < 10240 ? 1 : 0)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

function formatNumber(value) {
  const number = Number(value);
  if (!Number.isFinite(number)) return String(value ?? "");
  return Number.isInteger(number) ? String(number) : String(Math.round(number * 1000) / 1000);
}

function indexValue(value, fallback = 0) {
  const index = Number(value ?? fallback);
  if (!Number.isFinite(index)) throw new Error("The editor returned an invalid selection index.");
  return index;
}

function setStatus(message) {
  dom.statusMessage.textContent = String(message || "Ready");
  dom.statusMessage.title = String(message || "Ready");
}

function setEngineState(state, label) {
  dom.engineBadge.dataset.state = state;
  dom.engineLabel.textContent = label;
}

function friendlyError(error) {
  const raw = typeof error === "string" ? error : error?.message;
  const message = String(raw || "The editor could not complete that action.")
    .replace(/^Error:\s*/i, "")
    .replace(/^RuntimeError:\s*/i, "")
    .trim();
  if (!message || message === "unreachable") {
    return "The Rust editor reported an unexpected error. Check that the file is valid and try again.";
  }
  return message.length > 600 ? `${message.slice(0, 597)}…` : message;
}

function showNotice(title, error, severity = "error") {
  const notice = makeElement("section", "notice");
  notice.dataset.severity = severity;
  notice.setAttribute("role", severity === "error" ? "alert" : "status");
  notice.append(makeIcon(severity === "error" ? "alert" : "info"));
  const copy = makeElement("div", "notice-copy");
  copy.append(makeElement("strong", "", title));
  copy.append(makeElement("p", "", typeof error === "string" ? error : friendlyError(error)));
  const close = makeElement("button", "icon-button");
  close.type = "button";
  close.setAttribute("aria-label", "Dismiss message");
  close.append(makeIcon("close"));
  close.addEventListener("click", () => notice.remove());
  notice.append(copy, close);
  dom.noticeRegion.prepend(notice);
  if (severity !== "error") window.setTimeout(() => notice.remove(), 7000);
  while (dom.noticeRegion.children.length > 3) dom.noticeRegion.lastElementChild.remove();
}

function showTip(message, duration = 1400) {
  dom.viewportTip.textContent = message;
  dom.viewportTip.hidden = false;
  clearTimeout(showTip.timer);
  showTip.timer = setTimeout(() => {
    dom.viewportTip.hidden = true;
  }, duration);
}

function parseSnapshot(value) {
  const parsed = typeof value === "string" ? JSON.parse(value) : value;
  if (!isObject(parsed)) throw new Error("The editor returned an invalid course snapshot.");
  return parsed;
}

function refreshSnapshot(value) {
  currentSnapshot = parseSnapshot(value === undefined ? editor.snapshot() : value);
  renderSnapshot(currentSnapshot);
  return currentSnapshot;
}

function scheduleRender() {
  if (!engineReady || renderRequest) return;
  renderRequest = requestAnimationFrame(() => {
    renderRequest = 0;
    if (!engineReady) return;
    try {
      editor.render();
    } catch (error) {
      showNotice("Viewport rendering failed", error);
    }
  });
}

function scheduleResize() {
  if (!engineReady || resizeRequest) return;
  resizeRequest = requestAnimationFrame(() => {
    resizeRequest = 0;
    if (!engineReady) return;
    try {
      editor.resize();
      editor.render();
    } catch (error) {
      showNotice("Viewport resizing failed", error);
    }
  });
}

function selection() {
  const value = currentSnapshot?.selection;
  return isObject(value) && value.kind !== undefined && value.index !== undefined && value.index !== null ? value : null;
}

function updateCommands() {
  const ready = engineReady && !busy;
  const open = hasDocument();
  const selected = selection();
  const hasObjectSelection = selected?.kind === "object";
  const assetIndex = currentSnapshot?.selected_asset?.index;
  const hasCourse = Boolean(currentSnapshot?.course);
  const hasArchive = asArray(currentSnapshot?.archive?.files).some((file) => !file.loose);

  dom.openButton.disabled = !ready;
  dom.emptyOpenButton.disabled = !ready;
  dom.relatedButton.disabled = !ready || !open;
  dom.addObjectButton.disabled = !ready || !hasCourse;
  dom.duplicateButton.disabled = !ready || !hasObjectSelection;
  dom.deleteButton.disabled = !ready || !hasObjectSelection;
  dom.focusButton.disabled = !ready || !selected;
  dom.floorButton.disabled = !ready || !selected;
  dom.undoButton.disabled = !ready || !open || !currentSnapshot?.can_undo;
  dom.redoButton.disabled = !ready || !open || !currentSnapshot?.can_redo;
  dom.exportMenuButton.disabled = !ready || !hasCourse;
  dom.exportArchiveButton.disabled = !ready || !hasCourse || !hasArchive;
  dom.frameButton.disabled = !ready || !open;
  dom.replaceAssetButton.disabled = !ready || assetIndex === undefined || assetIndex === null;
  dom.downloadAssetButton.disabled = !ready || assetIndex === undefined || assetIndex === null;
  dom.selectionFields.inert = busy;
  dom.headerFields.inert = busy;
  for (const checkbox of dom.layerList.querySelectorAll("input")) checkbox.disabled = !ready || !open;
  if (!ready) closeExportMenu();
}

async function runAction(pendingMessage, action, options = {}) {
  if (!engineReady || busy) return undefined;
  const { refresh = "required", render = true, success, errorTitle = "Action failed" } = options;
  busy = true;
  setEngineState("busy", "Working");
  setStatus(pendingMessage);
  updateCommands();
  try {
    const result = await action();
    if (refresh === "required") refreshSnapshot(result);
    if (refresh === "optional" && (typeof result === "string" || isObject(result))) refreshSnapshot(result);
    if (render) scheduleRender();
    const resolvedSuccess = typeof success === "function" ? success(result) : success;
    setStatus(resolvedSuccess || currentSnapshot?.status || "Done");
    return result;
  } catch (error) {
    console.error(pendingMessage, error);
    if (currentSnapshot) renderSnapshot(currentSnapshot);
    showNotice(errorTitle, error);
    setStatus(`${errorTitle}.${hasDocument() ? " Your open course is still available." : ""}`);
    return undefined;
  } finally {
    busy = false;
    setEngineState(engineReady ? "ready" : "error", engineReady ? "Ready" : "Unavailable");
    updateCommands();
  }
}

/** Synchronous editor call for high-frequency interaction (no busy state). */
function quick(method, ...args) {
  if (!engineReady || busy) return undefined;
  try {
    return editor[method](...args);
  } catch (error) {
    showNotice("The viewport action failed", error);
    return undefined;
  }
}

function readFileBytes(file, label) {
  if (!file.size) return Promise.reject(new Error(`${file.name} is empty. Choose a valid course or asset file.`));
  dom.transferStatus.hidden = false;
  dom.transferLabel.textContent = label || `Reading ${file.name}`;
  dom.transferProgress.removeAttribute("value");
  setStatus(label || `Reading ${file.name}…`);
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onprogress = (event) => {
      if (event.lengthComputable) dom.transferProgress.value = Math.min(100, Math.round((event.loaded / event.total) * 100));
    };
    reader.onerror = () => reject(new Error(`${file.name} could not be read. Check the file and try again.`));
    reader.onabort = () => reject(new Error(`Reading ${file.name} was cancelled.`));
    reader.onload = () => {
      if (!(reader.result instanceof ArrayBuffer)) {
        reject(new Error(`${file.name} could not be read as binary data.`));
        return;
      }
      resolve(new Uint8Array(reader.result));
    };
    reader.readAsArrayBuffer(file);
  }).finally(() => {
    dom.transferStatus.hidden = true;
  });
}

// ---------- Model library persistence (IndexedDB, local to this browser) ----------

const libraryStore = {
  open() {
    if (!("indexedDB" in window)) return Promise.resolve(null);
    if (!this.promise) {
      this.promise = new Promise((resolve) => {
        const request = indexedDB.open("double-dash-studio", 1);
        request.onupgradeneeded = () => request.result.createObjectStore("libraries", { keyPath: "name" });
        request.onsuccess = () => resolve(request.result);
        request.onerror = () => resolve(null);
      });
    }
    return this.promise;
  },
  async transaction(mode, work) {
    const db = await this.open();
    if (!db) return undefined;
    return new Promise((resolve) => {
      const tx = db.transaction("libraries", mode);
      const request = work(tx.objectStore("libraries"));
      tx.oncomplete = () => resolve(request?.result);
      tx.onerror = () => resolve(undefined);
    });
  },
  save(name, bytes) {
    return this.transaction("readwrite", (store) => store.put({ name, bytes }));
  },
  remove(name) {
    return this.transaction("readwrite", (store) => store.delete(name));
  },
  all() {
    return this.transaction("readonly", (store) => store.getAll());
  },
};

async function restoreLibraries() {
  const stored = asArray(await libraryStore.all());
  for (const entry of stored) {
    try {
      refreshSnapshot(editor.add_library(entry.name, entry.bytes));
    } catch (error) {
      console.warn("Could not restore model library", entry.name, error);
    }
  }
  return stored.length;
}

function libraryNames() {
  return new Set(asArray(currentSnapshot?.library?.archives).map((archive) => archive.name));
}

async function addLibraries(files) {
  for (const file of files) {
    let bytes = null;
    const result = await runAction(`Adding ${file.name} to the model library…`, async () => {
      bytes = await readFileBytes(file, `Reading ${file.name}`);
      return editor.add_library(file.name, bytes);
    }, { errorTitle: `Couldn’t add ${file.name}` });
    if (result !== undefined && bytes) await libraryStore.save(file.name, bytes);
  }
}

// ---------- Opening files ----------

function confirmDirtyReplacement() {
  if (!currentSnapshot?.dirty) return true;
  return window.confirm("This course has unsaved changes. Opening another course will discard them. Continue?");
}

async function openCourse(file) {
  if (!engineReady || busy) return;
  if (!COURSE_EXTENSIONS.has(extensionOf(file.name))) {
    showNotice("Unsupported course file", `“${file.name}” is not a BOL or ARC file. Choose a Mario Kart Double Dash course file.`);
    return;
  }
  if (!confirmDirtyReplacement()) return;
  await runAction(`Opening ${file.name}…`, async () => {
    const bytes = await readFileBytes(file, `Reading ${file.name}`);
    return editor.open_file(file.name, bytes);
  }, {
    success: () => {
      const view = currentSnapshot?.view || {};
      const models = Number(view.object_models || 0);
      return `Opened ${file.name} · ${models} object model${models === 1 ? "" : "s"}${view.sky ? " · sky" : ""}. Hold right mouse and use WASD to fly.`;
    },
    errorTitle: `Couldn’t open ${file.name}`,
  });
  setTool(tool);
}

async function loadRelatedFiles(files) {
  if (!hasDocument()) {
    showNotice("Open a course first", "Related BCO, BTI, BMD, and ARC files need an open course. Open a BOL or ARC before adding assets.");
    return;
  }
  let completed = 0;
  for (const file of files) {
    if (!RELATED_EXTENSIONS.has(extensionOf(file.name))) {
      showNotice("Unsupported related file", `“${file.name}” was skipped. Related files must use BCO, BTI, BMD, or ARC.`, "warning");
      continue;
    }
    if (busy) break;
    let bytes = null;
    const before = libraryNames();
    const result = await runAction(`Adding ${file.name}…`, async () => {
      bytes = await readFileBytes(file, `Reading ${file.name}`);
      return editor.load_related(file.name, bytes);
    }, { errorTitle: `Couldn’t add ${file.name}` });
    if (result === undefined) continue;
    completed++;
    // Extra archives become model libraries; remember them for next time.
    if (bytes && libraryNames().has(file.name) && !before.has(file.name)) await libraryStore.save(file.name, bytes);
  }
  if (completed > 1) setStatus(`Added ${completed} related files.`);
}

function selectEntity(kind, index, { focus = false } = {}) {
  closeExportMenu();
  void runAction("Updating selection…", () => {
    const snapshot = editor.select(kind, index);
    if (focus) editor.focus_selected();
    return snapshot;
  }, {
    success: focus ? "Focused the selection." : undefined,
    errorTitle: "Couldn’t select this item",
  });
}

// ---------- Rendering the snapshot ----------

function renderFileIdentity(snapshot) {
  const open = hasDocument();
  const name = open ? documentName() : "No course open";
  const files = asArray(snapshot?.archive?.files);
  const courseName = snapshot?.course?.course_name;
  dom.outlinerFileName.textContent = name;
  dom.outlinerFileName.title = name;
  dom.documentKind.textContent = open
    ? [courseName, files.length ? `${files.length} files` : "Standalone BOL", snapshot?.course_model?.loaded ? "textured model" : "no model", snapshot?.collision ? "collision" : null].filter(Boolean).join(" · ")
    : "Open a course ARC or BOL";
  dom.viewportFileName.textContent = open ? name : "No course";
  dom.viewportDirty.hidden = !snapshot?.dirty;
  dom.statusFileName.textContent = open ? name : "No course";
  dom.statusFileName.title = open ? name : "No course open";
  dom.dirtyIndicator.dataset.dirty = String(Boolean(snapshot?.dirty));
  dom.dirtyLabel.textContent = snapshot?.dirty ? "Unsaved" : open ? "Saved" : "No file";
  dom.emptyState.hidden = open;
  document.title = open ? `${snapshot?.dirty ? "• " : ""}${name} — Double Dash Studio` : "Double Dash Studio";
}

function makeTreeEmpty(message) {
  return makeElement("li", "tree-empty", message);
}

function renderFiles() {
  const files = asArray(currentSnapshot?.archive?.files);
  const selectedFile = currentSnapshot?.archive?.selected_file;
  dom.archiveCount.textContent = String(files.length);
  dom.fileList.replaceChildren();
  if (!files.length) {
    dom.fileList.append(makeTreeEmpty(hasDocument() ? "Standalone BOL. Add related resources to build an archive." : "Open a BOL or ARC to inspect its files."));
  } else {
    const fragment = document.createDocumentFragment();
    files.forEach((file, position) => {
      const index = indexValue(file?.index, position);
      const item = makeElement("li");
      const button = makeElement("button", "tree-row files-row");
      button.type = "button";
      button.setAttribute("aria-pressed", String(Number(selectedFile) === index));
      button.title = `Select ${file?.path || `file ${index}`}`;
      button.append(makeIcon(file?.compressed ? "archive" : "file"));
      const copy = makeElement("span", "tree-row-copy");
      copy.append(makeElement("span", "tree-row-label", file?.path || file?.name || `File ${index + 1}`));
      const meta = makeElement("span", "tree-row-meta");
      meta.append(makeElement("span", "kind-tag", file?.kind || extensionOf(file?.path) || "data"));
      meta.append(makeElement("span", "", formatBytes(file?.size)));
      if (file?.compressed) meta.append(makeElement("span", "compressed-tag", "packed"));
      copy.append(meta);
      button.append(copy);
      button.addEventListener("click", () => selectEntity("file", index));
      item.append(button);
      fragment.append(item);
    });
    dom.fileList.append(fragment);
  }

  const libraries = asArray(currentSnapshot?.library?.archives);
  const courseModels = Number(currentSnapshot?.library?.course_models || 0);
  dom.libraryList.replaceChildren();
  if (courseModels) {
    const item = makeElement("li");
    item.append(makeIcon("archive"), makeElement("span", "", "Course archive"), makeElement("small", "", `${courseModels} models`));
    dom.libraryList.append(item);
  }
  for (const library of libraries) {
    const item = makeElement("li");
    const remove = makeElement("button", "icon-button");
    remove.type = "button";
    remove.title = `Remove ${library.name}`;
    remove.setAttribute("aria-label", `Remove ${library.name}`);
    remove.append(makeIcon("close"));
    remove.addEventListener("click", () => {
      void runAction(`Removing ${library.name}…`, () => editor.remove_library(library.name), { errorTitle: "Couldn’t remove the library" })
        .then(() => libraryStore.remove(library.name));
    });
    item.append(makeIcon("library"), makeElement("span", "", library.name), makeElement("small", "", `${library.models} models`), remove);
    dom.libraryList.append(item);
  }
  if (!courseModels && !libraries.length) dom.libraryList.append(makeElement("li", "library-empty", "No model libraries yet."));
}

function renderEntities() {
  const entities = asArray(currentSnapshot?.entities);
  const selected = selection();
  const query = dom.outlinerSearch.value.trim().toLowerCase();
  const labels = entities.map((entity) => `${entity.kind}${entity.index}${entity.label}${entity.has_model ? 1 : 0}`).join("|");
  const key = JSON.stringify([query, selected?.kind, selected?.index, labels]);
  dom.entityCount.textContent = String(entities.length);
  if (key === lastEntitiesKey) return;
  lastEntitiesKey = key;
  dom.entityList.replaceChildren();
  if (!entities.length) {
    dom.entityList.append(makeTreeEmpty(hasDocument() ? "No editable entities were found. Use Place to add one." : "Course objects and routes will appear here."));
    return;
  }
  const groups = new Map();
  for (const [position, entity] of entities.entries()) {
    const label = String(entity?.label || `${humanize(entity?.kind)} ${position}`);
    if (query && !`${label} ${entity?.kind} ${entity?.category || ""} #${entity?.index}`.toLowerCase().includes(query)) continue;
    const group = String(entity?.kind || "object");
    if (!groups.has(group)) groups.set(group, []);
    groups.get(group).push({ ...entity, label, position });
  }
  const order = Object.keys(KIND_INFO);
  const rank = (group) => (order.includes(group) ? order.indexOf(group) : order.length);
  const fragment = document.createDocumentFragment();
  for (const group of [...groups.keys()].sort((a, b) => rank(a) - rank(b))) {
    const rows = groups.get(group);
    const info = KIND_INFO[group] || { label: humanize(group), color: "#999" };
    const item = makeElement("li");
    const details = makeElement("details", "tree-group");
    details.open = Boolean(query) || !collapsedGroups.has(group) || (selected && selected.kind === group);
    const summary = makeElement("summary");
    const dot = makeElement("span", "group-dot");
    dot.style.background = info.color;
    summary.append(makeIcon("chevron"), dot, makeElement("span", "", info.label), makeElement("span", "count-badge", rows.length));
    details.append(summary);
    details.addEventListener("toggle", () => {
      if (details.open) collapsedGroups.delete(group);
      else collapsedGroups.add(group);
    });
    const list = makeElement("ul");
    for (const entity of rows) {
      const kind = String(entity.kind);
      const index = indexValue(entity.index, entity.position);
      const isSelected = selected && String(selected.kind) === kind && Number(selected.index) === index;
      const row = makeElement("li");
      const button = makeElement("button", "tree-row");
      button.type = "button";
      button.setAttribute("aria-pressed", String(Boolean(isSelected)));
      button.title = `${entity.label} — click to select, double-click to focus`;
      const rowDot = makeElement("span", "row-dot");
      rowDot.style.background = CATEGORY_COLORS[entity.category] || info.color;
      const copy = makeElement("span", "tree-row-copy");
      copy.append(makeElement("span", "tree-row-label", entity.label));
      const meta = makeElement("span", "tree-row-meta");
      if (entity.has_model) meta.append(makeElement("span", "model-tag", "3D"));
      meta.append(makeElement("span", "", `#${index}`));
      copy.append(meta);
      button.append(rowDot, copy);
      button.addEventListener("click", () => selectEntity(kind, index));
      button.addEventListener("dblclick", () => selectEntity(kind, index, { focus: true }));
      row.append(button);
      list.append(row);
    }
    details.append(list);
    item.append(details);
    fragment.append(item);
  }
  if (!groups.size) fragment.append(makeTreeEmpty(`Nothing matches “${query}”.`));
  dom.entityList.append(fragment);
  dom.entityList.querySelector('.tree-row[aria-pressed="true"]')?.scrollIntoView({ block: "nearest" });
}

function renderPlaceList() {
  const catalog = asArray(currentSnapshot?.course?.object_catalog);
  const query = dom.placeSearch.value.trim().toLowerCase();
  const modelsOnly = dom.placeModelsOnly.checked;
  const showInternal = dom.placeShowInternal.checked;
  const key = JSON.stringify([catalog.length, query, modelsOnly, showInternal, catalog.filter((object) => object.model).length]);
  if (key === lastPlaceKey) return;
  lastPlaceKey = key;
  dom.placeList.replaceChildren();
  if (!catalog.length) {
    dom.placeList.append(makeTreeEmpty("Open a course to browse placeable objects."));
    return;
  }
  const items = catalog.filter((object) => (showInternal || object.placeable)
    && (!modelsOnly || object.model)
    && (!query || `${object.name} ${object.class_name} ${object.category} ${hex4(object.id)}`.toLowerCase().includes(query)));
  items.sort((a, b) => String(a.category).localeCompare(String(b.category))
    || Number(Boolean(b.model)) - Number(Boolean(a.model))
    || String(a.name).localeCompare(String(b.name)));
  const fragment = document.createDocumentFragment();
  let category = null;
  for (const object of items) {
    if (object.category !== category) {
      category = object.category;
      fragment.append(makeElement("li", "place-category", category));
    }
    const row = makeElement("li");
    const button = makeElement("button", "tree-row");
    button.type = "button";
    button.draggable = true;
    button.title = `${object.name} (${object.class_name}, ${hex4(object.id)})${object.model ? ` · model ${object.model}` : " · no model loaded"}\nDrag into the viewport or double-click to place.`;
    const dot = makeElement("span", "row-dot");
    dot.style.background = CATEGORY_COLORS[object.category] || "#999";
    const copy = makeElement("span", "tree-row-copy");
    copy.append(makeElement("span", "tree-row-label", object.name));
    const meta = makeElement("span", "tree-row-meta");
    if (object.model) meta.append(makeElement("span", "model-tag", "3D"));
    meta.append(makeElement("span", "", hex4(object.id)));
    copy.append(meta);
    button.append(dot, copy);
    button.addEventListener("dragstart", (event) => {
      event.dataTransfer.setData("application/x-dd-object", String(object.id));
      event.dataTransfer.setData("text/plain", object.name);
      event.dataTransfer.effectAllowed = "copy";
    });
    button.addEventListener("dblclick", () => placeObject(object.id));
    row.append(button);
    fragment.append(row);
  }
  if (!items.length) fragment.append(makeTreeEmpty("No objects match these filters."));
  dom.placeList.append(fragment);
}

function placeObject(id, x, y) {
  void runAction("Placing object…", () => (x === undefined ? editor.add_object_of(id) : editor.add_object_at(id, x, y)), {
    errorTitle: "Couldn’t place the object",
  });
}

function renderCourseSummary() {
  const counts = asArray(currentSnapshot?.course?.counts).filter((count) => Number(count?.count) > 0);
  dom.courseSummary.replaceChildren();
  dom.courseSummary.hidden = !counts.length;
  for (const count of counts) {
    const chip = makeElement("span", "summary-chip");
    chip.append(makeElement("strong", "", count?.count ?? 0));
    chip.append(document.createTextNode(count?.label || humanize(count?.kind || "items")));
    dom.courseSummary.append(chip);
  }
}

function renderLayers(snapshot) {
  const rows = asArray(snapshot?.layers);
  dom.layerList.replaceChildren();
  rows.forEach((layer, index) => {
    const label = makeElement("label", "layer-row");
    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.id = `layer-${index}`;
    checkbox.dataset.layerName = layer.name;
    checkbox.checked = Boolean(layer.visible);
    checkbox.setAttribute("aria-label", `Show ${layer.label}`);
    checkbox.addEventListener("change", () => setLayer(layer.name, checkbox.checked, layer.label));
    label.append(checkbox, makeElement("span", "", layer.label));
    dom.layerList.append(label);
  });
}

function setLayer(name, visible, label = humanize(name)) {
  gameViewStash = null;
  void runAction(`${visible ? "Showing" : "Hiding"} ${label}…`, () => editor.set_layer(name, visible), {
    errorTitle: "Couldn’t change the viewport layer",
  });
}

function toggleGameView() {
  if (!hasDocument()) return;
  const layers = asArray(currentSnapshot?.layers);
  void runAction("Toggling game view…", () => {
    let snapshot;
    if (gameViewStash) {
      for (const [name, visible] of gameViewStash) snapshot = editor.set_layer(name, visible);
      gameViewStash = null;
    } else {
      gameViewStash = layers.filter((layer) => HELPER_LAYERS.includes(layer.name)).map((layer) => [layer.name, layer.visible]);
      for (const [name] of gameViewStash) snapshot = editor.set_layer(name, name === "objects");
    }
    return snapshot ?? editor.snapshot();
  }, { success: () => (gameViewStash ? "Game view: editor helpers hidden (G to restore)." : "Editor helpers restored.") });
}

// ---------- Details panel ----------

function normalizeFields(fields) {
  if (Array.isArray(fields)) return fields.map((field, index) => (isObject(field) ? field : { key: String(index), label: String(index), value: field }));
  if (isObject(fields)) return Object.entries(fields).map(([key, field]) => (isObject(field) ? { key, ...field } : { key, label: humanize(key), value: field }));
  return [];
}

function isNumericField(field) {
  const type = String(field.type || "").toLowerCase();
  return type.includes("int") || ["number", "float", "double", "f32", "f64", "u8", "u16", "u32", "i8", "i16", "i32", "usize"].includes(type)
    || (!type && typeof field.value === "number");
}

function isBooleanField(field) {
  return ["bool", "boolean", "checkbox", "switch", "toggle"].includes(String(field.type || "").toLowerCase())
    || (!field.type && typeof field.value === "boolean");
}

function normalizeOptions(field) {
  let raw = field.options;
  if (raw === "object_catalog") {
    raw = asArray(currentSnapshot?.course?.object_catalog).map((object) => ({
      value: object.id,
      label: `${object.name}${object.model ? " ◆" : ""} — ${hex4(object.id)}`,
    }));
  }
  if (!Array.isArray(raw) && isObject(raw)) raw = Object.entries(raw).map(([value, label]) => ({ value, label: isObject(label) ? label.label ?? label.name : label }));
  if (!Array.isArray(raw)) return null;
  return raw.map((option, index) => {
    if (!isObject(option)) return { value: option, label: String(option) };
    const value = option.value ?? option.id ?? option.key ?? option.index ?? option.name ?? option.label ?? index;
    return { value, label: String(option.label ?? option.name ?? value) };
  });
}

function inputValueForDisplay(value, multiline = false) {
  if (value === undefined || value === null) return "";
  if (typeof value === "object") return JSON.stringify(value, null, multiline ? 2 : 0);
  return String(value);
}

function commitField(context, key, label, value) {
  void runAction(`Saving ${label}…`, () => {
    if (context.type === "header") return editor.edit_header(key, value);
    return editor.edit(context.kind, context.index, key, value);
  }, { success: `${label} updated.`, errorTitle: `Couldn’t update ${label}` });
}

function createField(field, context) {
  const key = String(field.key ?? field.id ?? field.name ?? `field_${fieldId}`);
  const labelText = String(field.label || humanize(key));
  const id = `inspector-field-${fieldId++}`;
  const numeric = isNumericField(field);
  const boolean = isBooleanField(field);
  const options = normalizeOptions(field);
  const fieldType = String(field.type || "").toLowerCase();
  const multiline = fieldType === "textarea" || fieldType === "multiline";
  const wrapper = makeElement("div", `field${boolean ? " checkbox-field" : ""}`);
  wrapper.dataset.search = `${labelText} ${key}`.toLowerCase();
  const label = makeElement("label", boolean ? "field-label" : "", labelText);
  label.htmlFor = id;

  if (boolean) {
    const copy = makeElement("div");
    copy.append(label);
    if (field.description) copy.append(makeElement("span", "field-description", field.description));
    const control = makeElement("span", "checkbox-control");
    const input = document.createElement("input");
    input.type = "checkbox";
    input.id = id;
    input.checked = Boolean(field.value);
    input.disabled = Boolean(field.disabled || field.readonly || field.read_only);
    const track = makeElement("span", "switch-track");
    track.setAttribute("aria-hidden", "true");
    control.append(input, track);
    wrapper.append(copy, control);
    input.addEventListener("change", () => commitField(context, key, labelText, input.checked));
    return wrapper;
  }

  wrapper.append(label);
  let input;
  let readValue;
  if (options) {
    input = document.createElement("select");
    if (!options.some((option) => String(option.value) === String(field.value)) && field.value !== undefined && field.value !== null) {
      options.unshift({ value: field.value, label: `${String(field.value)} (current)` });
    }
    options.forEach((option, index) => {
      const element = document.createElement("option");
      element.textContent = option.label;
      element.value = String(index);
      element.selected = String(option.value) === String(field.value);
      input.append(element);
    });
    readValue = () => options[input.selectedIndex]?.value;
  } else if (multiline) {
    input = document.createElement("textarea");
    input.value = inputValueForDisplay(field.value, true);
    readValue = () => parseTextValue(input, field);
  } else {
    input = document.createElement("input");
    input.type = numeric ? "number" : "text";
    input.value = numeric ? formatNumber(field.value) : inputValueForDisplay(field.value);
    if (numeric) {
      input.step = field.step === undefined ? "any" : String(field.step);
      input.inputMode = "decimal";
      readValue = () => {
        const value = input.valueAsNumber;
        if (!Number.isFinite(value)) throw new Error(`“${labelText}” needs a valid number.`);
        return value;
      };
    } else {
      readValue = () => parseTextValue(input, field);
    }
  }
  input.id = id;
  input.disabled = Boolean(field.disabled);
  if (field.readonly || field.read_only) input.readOnly = true;
  if (field.min !== undefined) input.min = String(field.min);
  if (field.max !== undefined) input.max = String(field.max);
  if (field.description && !options) input.title = String(field.description);
  input.addEventListener("change", () => {
    if (busy || !engineReady) return;
    input.setCustomValidity("");
    let value;
    try {
      value = readValue();
    } catch (error) {
      input.setCustomValidity(friendlyError(error));
      input.reportValidity();
      return;
    }
    commitField(context, key, labelText, value);
  });
  wrapper.append(input);
  if (field.description) wrapper.append(makeElement("span", "field-description", field.description));
  return wrapper;
}

function parseTextValue(input, field) {
  const expectsJson = String(field.type || "").toLowerCase() === "json" || (field.value !== null && typeof field.value === "object");
  if (!expectsJson) return input.value;
  try {
    return JSON.parse(input.value);
  } catch {
    throw new Error(`“${field.label || humanize(field.key)}” must contain valid JSON.`);
  }
}

const VECTOR_KEY = /^([a-z_0-9]+)\.(x|y|z|w|yaw|pitch|roll)$/;
const ROTATION_AXES = { yaw: ["Y", "y"], pitch: ["P", "x"], roll: ["R", "z"] };

/** One XYZ row with labels you can drag to scrub the value, like Unreal's details panel. */
function createVectorRow(base, components, context) {
  const first = components[0].field;
  const labelText = base === "rotation" ? "Rotation" : String(first.label || humanize(base)).replace(/\s+[xyzw]$/i, "");
  const row = makeElement("div", "vector-field");
  row.dataset.search = `${labelText} ${base}`.toLowerCase();
  const label = makeElement("label", "", labelText);
  const inputs = makeElement("div", "vector-inputs");
  inputs.style.setProperty("--components", String(components.length));
  const marker = Number(currentSnapshot?.view?.marker || 40);
  const step = base === "rotation" ? 0.5 : base === "scale" ? 0.01 : Math.max(0.5, marker / 25);
  for (const { axis, field } of components) {
    const box = makeElement("div", "axis-input");
    const [letter, color] = ROTATION_AXES[axis] || [axis.toUpperCase(), axis];
    const handle = makeElement("span", "", letter);
    handle.dataset.axis = color;
    handle.title = `Drag to change ${field.label || field.key} (Shift ×10, Alt ×0.1)`;
    const input = document.createElement("input");
    input.type = "number";
    input.step = "any";
    input.value = formatNumber(field.value);
    input.disabled = Boolean(field.disabled);
    input.setAttribute("aria-label", String(field.label || field.key));
    const key = String(field.key);
    const name = String(field.label || key);
    input.addEventListener("change", () => {
      const value = input.valueAsNumber;
      if (Number.isFinite(value)) commitField(context, key, name, value);
    });
    handle.addEventListener("pointerdown", (event) => {
      if (input.disabled || busy) return;
      event.preventDefault();
      handle.setPointerCapture(event.pointerId);
      const start = Number(field.value) || 0;
      const originX = event.clientX;
      let value = start;
      const move = (moveEvent) => {
        const scale = moveEvent.shiftKey ? 10 : moveEvent.altKey ? 0.1 : 1;
        value = start + (moveEvent.clientX - originX) * step * scale;
        input.value = formatNumber(value);
      };
      const up = () => {
        handle.removeEventListener("pointermove", move);
        handle.removeEventListener("pointerup", up);
        handle.removeEventListener("pointercancel", up);
        if (value !== start) commitField(context, key, name, value);
      };
      handle.addEventListener("pointermove", move);
      handle.addEventListener("pointerup", up);
      handle.addEventListener("pointercancel", up);
    });
    box.append(handle, input);
    inputs.append(box);
  }
  row.append(label, inputs);
  return row;
}

function renderFieldGroups(container, fields, context) {
  container.replaceChildren();
  const groups = new Map();
  for (const field of normalizeFields(fields)) {
    const group = String(field.group || "Properties");
    if (!groups.has(group)) groups.set(group, []);
    groups.get(group).push(field);
  }
  const order = ["Transform", "Properties", "Parameters", "Advanced"];
  const fragment = document.createDocumentFragment();
  for (const group of [...groups.keys()].sort((a, b) => order.indexOf(a) - order.indexOf(b))) {
    const details = makeElement("details", "field-group");
    details.open = !collapsedFieldGroups.has(group);
    details.addEventListener("toggle", () => {
      if (details.open) collapsedFieldGroups.delete(group);
      else collapsedFieldGroups.add(group);
    });
    const summary = makeElement("summary", "", group);
    summary.append(makeIcon("chevron"));
    const list = makeElement("div", "field-list");
    const fieldsInGroup = groups.get(group);
    const vectors = new Map();
    for (const field of fieldsInGroup) {
      const match = VECTOR_KEY.exec(String(field.key));
      if (match && match[1] !== "params") {
        if (!vectors.has(match[1])) vectors.set(match[1], []);
        vectors.get(match[1]).push({ axis: match[2], field });
      }
    }
    const emitted = new Set();
    for (const field of fieldsInGroup) {
      const match = VECTOR_KEY.exec(String(field.key));
      if (match && vectors.get(match[1])?.length > 1) {
        if (!emitted.has(match[1])) {
          emitted.add(match[1]);
          list.append(createVectorRow(match[1], vectors.get(match[1]), context));
        }
        continue;
      }
      list.append(createField(field, context));
    }
    details.append(summary, list);
    fragment.append(details);
  }
  container.append(fragment);
  applyDetailsFilter();
}

function applyDetailsFilter() {
  const query = dom.detailsSearch.value.trim().toLowerCase();
  for (const element of document.querySelectorAll("#right-panel [data-search]")) {
    element.dataset.filtered = String(Boolean(query) && !element.dataset.search.includes(query));
  }
  if (query) for (const group of document.querySelectorAll("#right-panel details.field-group, #course-header-section")) group.open = true;
}

function renderAsset(asset) {
  textureRequest++;
  const available = isObject(asset) && asset.index !== undefined && asset.index !== null;
  dom.assetSection.hidden = !available;
  dom.texturePreview.hidden = true;
  dom.textureEmpty.hidden = false;
  dom.assetInfo.textContent = "";
  if (!available) {
    dom.replaceAssetButton.onclick = null;
    dom.downloadAssetButton.onclick = null;
    return;
  }
  const index = indexValue(asset.index);
  const archiveFile = asArray(currentSnapshot?.archive?.files).find((file, position) => Number(file?.index ?? position) === index);
  const name = String(asset.name || archiveFile?.path || `Asset ${index}`);
  const dimensions = asset.width && asset.height ? `${asset.width} × ${asset.height}` : "";
  dom.assetName.textContent = name;
  dom.assetName.title = name;
  dom.assetMetadata.textContent = [dimensions, asset.format].filter(Boolean).join(" · ") || `Archive index ${index}`;
  if (asset.info !== undefined && asset.info !== null) dom.assetInfo.textContent = typeof asset.info === "string" ? asset.info : JSON.stringify(asset.info);
  dom.textureEmpty.replaceChildren(makeIcon("object"), makeElement("span", "", "Loading texture preview…"));
  dom.replaceInput.dataset.assetIndex = String(index);
  dom.replaceAssetButton.onclick = () => {
    if (!busy && engineReady) dom.replaceInput.click();
  };
  dom.downloadAssetButton.onclick = () => {
    void runAction(`Preparing ${name}…`, () => editor.asset_data(index), { refresh: false, render: false, errorTitle: `Couldn’t download ${name}` })
      .then((bytes) => {
        if (bytes !== undefined) downloadBytes(bytes, assetFileName(name));
      });
  };
  updateTexturePreview(index, textureRequest);
}

function assetFileName(name) {
  const clean = String(name || "asset").split(/[\\/]/).pop().replace(/[<>:"/\\|?*\x00-\x1F]/g, "_");
  return /\.[a-z0-9]+$/i.test(clean) ? clean : `${clean}.bti`;
}

function updateTexturePreview(index, request) {
  const unavailable = (message) => {
    if (request !== textureRequest) return;
    dom.texturePreview.hidden = true;
    dom.textureEmpty.hidden = false;
    dom.textureEmpty.replaceChildren(makeIcon("object"), makeElement("span", "", message));
  };
  try {
    const width = Number(editor.texture_width(index));
    const height = Number(editor.texture_height(index));
    const rgba = editor.texture_rgba(index);
    if (request !== textureRequest) return;
    if (!width || !height || !rgba || rgba.length < width * height * 4) {
      unavailable("Texture data isn’t in a supported RGBA format");
      return;
    }
    const canvas = dom.texturePreview;
    canvas.width = width;
    canvas.height = height;
    const context = canvas.getContext("2d", { alpha: true });
    const image = context.createImageData(width, height);
    image.data.set(new Uint8ClampedArray(rgba.buffer, rgba.byteOffset, width * height * 4));
    context.putImageData(image, 0, 0);
    dom.textureEmpty.hidden = true;
    canvas.hidden = false;
  } catch {
    unavailable("No texture preview for this asset");
  }
}

function renderInspector() {
  const selected = currentSnapshot?.selection;
  const fields = normalizeFields(selected?.fields);
  const hasSelection = isObject(selected) && Boolean(selected.title || selected.kind || fields.length || selected.raw_hex);
  dom.selectionEmpty.hidden = hasSelection;
  dom.selectionContent.hidden = !hasSelection;
  dom.selectionEmpty.querySelector("h3").textContent = hasDocument() ? "Nothing selected" : "No course loaded";
  dom.selectionEmpty.querySelector("p").textContent = hasDocument()
    ? "Click something in the viewport or outliner. Drag new objects in from the Place tab."
    : "Open a course ARC or BOL, then select an object to edit its properties.";
  if (hasSelection) {
    const kind = String(selected.kind || "");
    const entity = asArray(currentSnapshot?.entities).find((item) => item.kind === kind && Number(item.index) === Number(selected.index));
    dom.selectionTitle.textContent = selected.title || selected.label || humanize(kind || "Selection");
    dom.selectionKind.textContent = entity?.category ? `${humanize(kind)} · ${entity.category}` : humanize(kind || "Selection");
    dom.selectionIndex.textContent = selected.index === undefined || selected.index === null ? "" : `#${selected.index}`;
    dom.selectionSwatch.style.background = CATEGORY_COLORS[entity?.category] || KIND_INFO[kind]?.color || "var(--accent)";
    if (selected.kind !== undefined && selected.index !== undefined && selected.index !== null) {
      renderFieldGroups(dom.selectionFields, fields, { type: "selection", kind: selected.kind, index: indexValue(selected.index) });
    } else {
      dom.selectionFields.replaceChildren();
    }
    dom.rawDataDetails.hidden = !selected.raw_hex;
    dom.rawHex.textContent = selected.raw_hex ? String(selected.raw_hex) : "";
  } else {
    dom.selectionFields.replaceChildren();
    dom.rawDataDetails.hidden = true;
  }
  const header = normalizeFields(currentSnapshot?.course?.header);
  dom.headerSection.hidden = !hasDocument() || !header.length;
  dom.headerFields.replaceChildren();
  for (const field of header) dom.headerFields.append(createField(field, { type: "header" }));
  renderAsset(currentSnapshot?.selected_asset);
}

function renderIssues() {
  const issues = asArray(currentSnapshot?.issues);
  dom.issuesCount.textContent = String(issues.length);
  dom.issuesDrawerCount.textContent = String(issues.length);
  const errors = issues.some((issue) => String(issue?.severity).toLowerCase() === "error");
  dom.issuesToggle.classList.toggle("has-errors", errors);
  dom.issuesToggle.classList.toggle("has-warnings", !errors && issues.some((issue) => String(issue?.severity).toLowerCase() === "warning"));
  dom.issuesList.replaceChildren();
  if (!issues.length) {
    dom.issuesList.append(makeElement("li", "issues-empty", "No issues reported for this course."));
    return;
  }
  const fragment = document.createDocumentFragment();
  for (const issue of issues) {
    const severity = ["error", "warning", "info"].includes(String(issue?.severity).toLowerCase()) ? String(issue.severity).toLowerCase() : "info";
    const row = makeElement("li", "issue-row");
    row.dataset.severity = severity;
    row.append(makeIcon(severity === "info" ? "info" : "alert"), makeElement("span", "issue-severity", severity), makeElement("span", "", issue?.message || "Unspecified course issue"));
    if (issue?.kind && issue.index !== undefined && issue.index !== null && KIND_INFO[issue.kind]) {
      row.style.cursor = "pointer";
      row.title = "Select and focus";
      row.addEventListener("click", () => selectEntity(issue.kind, issue.index, { focus: true }));
    }
    fragment.append(row);
  }
  dom.issuesList.append(fragment);
}

function renderViewState() {
  const view = currentSnapshot?.view;
  if (!view) return;
  const snap = view.snap || {};
  dom.snapToggle.setAttribute("aria-pressed", String(Boolean(snap.enabled)));
  if (snap.translate) dom.snapTranslate.value = String(snap.translate);
  if (snap.rotate) dom.snapRotate.value = String(snap.rotate);
  dom.shadingLabel.textContent = view.shading === "simple" ? "Simple" : "Lit";
  if (asArray(view.shader_failures).length && !renderViewState.warned) {
    renderViewState.warned = true;
    showNotice("Some materials use the simple shader", `${view.shader_failures.length} GX shader(s) failed to compile on this GPU: ${view.shader_failures[0]}`, "warning");
  }
}

function renderSnapshot(snapshot) {
  renderFileIdentity(snapshot);
  renderFiles();
  renderEntities();
  renderPlaceList();
  renderCourseSummary();
  renderLayers(snapshot);
  renderInspector();
  renderIssues();
  renderViewState();
  updateCommands();
}

function toggleIssues(force) {
  const open = force === undefined ? dom.issuesDrawer.hidden : Boolean(force);
  dom.issuesDrawer.hidden = !open;
  dom.issuesToggle.setAttribute("aria-expanded", String(open));
}

// ---------- Export ----------

function binaryBytes(value) {
  if (value instanceof Uint8Array) return value;
  if (value instanceof ArrayBuffer) return new Uint8Array(value);
  if (ArrayBuffer.isView(value)) return new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
  if (Array.isArray(value)) return new Uint8Array(value);
  throw new Error("The editor did not return binary file data for this export.");
}

function exportFileName(extension, original = false) {
  const originalName = documentName().split(/[\\/]/).pop();
  const stem = (originalName.replace(/\.(bol|arc|szs)$/i, "") || "course").replace(/[<>:"/\\|?*\x00-\x1F]/g, "_");
  return `${stem}${original ? ".original" : ""}.${extension}`;
}

function downloadBytes(data, name) {
  const blob = new Blob([binaryBytes(data)], { type: "application/octet-stream" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = name;
  link.style.display = "none";
  document.body.append(link);
  link.click();
  link.remove();
  window.setTimeout(() => URL.revokeObjectURL(url), 1000);
  setStatus(`Downloaded ${name}`);
}

async function exportDocument(method, extension, original = false) {
  if (!engineReady || busy || !hasDocument()) return;
  closeExportMenu(true);
  const name = exportFileName(extension, original);
  const bytes = await runAction(`Preparing ${name}…`, () => editor[method](), { refresh: false, render: false, errorTitle: "Export failed" });
  if (bytes === undefined) return;
  try {
    downloadBytes(bytes, name);
  } catch (error) {
    showNotice("Export failed", error);
  }
}

function openExportMenu(focusFirst = false) {
  if (dom.exportMenuButton.disabled || !dom.exportMenu.hidden) return;
  dom.exportMenu.hidden = false;
  dom.exportMenuButton.setAttribute("aria-expanded", "true");
  if (focusFirst) dom.exportBolButton.focus();
}

function closeExportMenu(returnFocus = false) {
  const wasOpen = !dom.exportMenu.hidden;
  dom.exportMenu.hidden = true;
  dom.exportMenuButton.setAttribute("aria-expanded", "false");
  if (returnFocus && wasOpen) dom.exportMenuButton.focus();
}

// ---------- Viewport interaction ----------

function isTypingTarget(target) {
  return target instanceof Element && Boolean(target.closest("input:not([type=checkbox]):not([type=range]), textarea, select, [contenteditable='true']"));
}

function canvasPoint(event) {
  const rect = dom.viewport.getBoundingClientRect();
  return { x: event.clientX - rect.left, y: event.clientY - rect.top };
}

function frameAll() {
  if (!hasDocument() || !engineReady) return;
  quick("frame_all");
  scheduleRender();
  setStatus("Framed the whole course.");
}

function focusSelection() {
  if (!hasDocument() || !engineReady) return;
  quick("focus_selected");
  scheduleRender();
  setStatus(selection() ? "Focused the selection." : "Framed the whole course.");
}

function setTool(next) {
  tool = next;
  for (const button of document.querySelectorAll("[data-tool]")) button.setAttribute("aria-pressed", String(button.dataset.tool === tool));
  dom.toolReadout.textContent = TOOL_LABELS[tool];
  if (engineReady) {
    try {
      currentSnapshot = parseSnapshot(editor.set_gizmo_mode(tool));
    } catch (error) {
      showNotice("Couldn’t change the tool", error);
    }
  }
  scheduleRender();
}

function viewportPick(point) {
  if (!engineReady || busy || !hasDocument()) return;
  void runAction("Selecting…", () => editor.pick(point.x, point.y), {
    refresh: "optional",
    success: () => (selection() ? `Selected ${currentSnapshot?.selection?.title || "item"}.` : "Selection cleared."),
    errorTitle: "Viewport selection failed",
  });
}

function speedMultiplier() {
  return 2 ** (Number(dom.cameraSpeed.value) - 4);
}

function flyLoop(time) {
  flyFrame = 0;
  const flying = pointerGesture?.mode === "fly";
  const arrows = ["arrowup", "arrowdown", "arrowleft", "arrowright"].some((key) => keysDown.has(key));
  if (!flying && !arrows) return;
  const dt = Math.min(0.1, lastFlyTime ? (time - lastFlyTime) / 1000 : 0.016);
  lastFlyTime = time;
  let forward = 0;
  let right = 0;
  let up = 0;
  if (flying) {
    if (keysDown.has("w")) forward += 1;
    if (keysDown.has("s")) forward -= 1;
    if (keysDown.has("d")) right += 1;
    if (keysDown.has("a")) right -= 1;
    if (keysDown.has("e")) up += 1;
    if (keysDown.has("q")) up -= 1;
  }
  if (keysDown.has("arrowup")) forward += 1;
  if (keysDown.has("arrowdown")) forward -= 1;
  if (keysDown.has("arrowright")) right += 1;
  if (keysDown.has("arrowleft")) right -= 1;
  if (forward || right || up) {
    const radius = Number(currentSnapshot?.view?.scene_radius) || 5000;
    const speed = radius * 0.15 * speedMultiplier() * (keysDown.has("shift") ? 3 : 1) * dt;
    editor.fly(forward * speed, right * speed, up * speed);
    scheduleRender();
  }
  flyFrame = requestAnimationFrame(flyLoop);
}

function ensureFlyLoop() {
  if (!flyFrame) {
    lastFlyTime = 0;
    flyFrame = requestAnimationFrame(flyLoop);
  }
}

function updateCursorReadout() {
  cursorTimer = 0;
  if (!lastCursorEvent || !engineReady || !hasDocument() || pointerGesture) return;
  const point = canvasPoint(lastCursorEvent);
  const position = editor.world_at(point.x, point.y);
  dom.cursorPosition.textContent = position
    ? `X ${Math.round(position[0])}  Y ${Math.round(position[1])}  Z ${Math.round(position[2])}`
    : "—";
}

function endPointerGesture(event, cancelled = false) {
  const gesture = pointerGesture;
  if (!gesture || (event && gesture.id !== event.pointerId)) return;
  pointerGesture = null;
  dom.viewport.removeAttribute("data-navigation");
  if (event && dom.viewport.hasPointerCapture(event.pointerId)) dom.viewport.releasePointerCapture(event.pointerId);
  if (gesture.mode === "gizmo") {
    const result = cancelled ? quick("cancel_drag") : quick("pointer_up");
    if (typeof result === "string") refreshSnapshot(result);
    setStatus(currentSnapshot?.status || "Done");
    scheduleRender();
    return;
  }
  if (gesture.mode === "orbit" && gesture.button === 0 && !gesture.moved && !cancelled && event?.type === "pointerup") {
    viewportPick(canvasPoint(event));
  }
}

function bindViewport() {
  const canvas = dom.viewport;
  canvas.addEventListener("contextmenu", (event) => event.preventDefault());
  canvas.addEventListener("pointerdown", (event) => {
    if (!engineReady || busy || !hasDocument() || pointerGesture) return;
    event.preventDefault();
    canvas.focus({ preventScroll: true });
    const point = canvasPoint(event);
    let mode;
    if (event.button === 2) mode = "fly";
    else if (event.button === 1 || (event.button === 0 && event.shiftKey)) mode = "pan";
    else if (event.button === 0 && event.altKey) mode = "orbit";
    else if (event.button === 0) mode = selection() && editor.pointer_down(point.x, point.y) ? "gizmo" : "orbit";
    else return;
    pointerGesture = { id: event.pointerId, mode, button: event.button, x: event.clientX, y: event.clientY, moved: false };
    canvas.setPointerCapture(event.pointerId);
    canvas.dataset.navigation = mode;
    if (mode === "fly") ensureFlyLoop();
    if (mode === "gizmo") scheduleRender();
  });
  canvas.addEventListener("pointermove", (event) => {
    const gesture = pointerGesture;
    if (!gesture) {
      if (!engineReady || busy || !hasDocument()) return;
      lastCursorEvent = event;
      if (!cursorTimer) cursorTimer = setTimeout(updateCursorReadout, 90);
      if (selection()) {
        const point = canvasPoint(event);
        if (quick("pointer_move", point.x, point.y, false)) {
          canvas.dataset.hover = editor.gizmo_hover() ? "gizmo" : "";
          scheduleRender();
        }
      }
      return;
    }
    if (gesture.id !== event.pointerId) return;
    const dx = event.clientX - gesture.x;
    const dy = event.clientY - gesture.y;
    gesture.x = event.clientX;
    gesture.y = event.clientY;
    gesture.moved = gesture.moved || Math.abs(dx) + Math.abs(dy) > 2;
    if (gesture.mode === "orbit" && gesture.moved) quick("orbit", dx, dy);
    else if (gesture.mode === "pan") quick("pan", dx, dy);
    else if (gesture.mode === "fly") quick("look", dx, dy);
    else if (gesture.mode === "gizmo") {
      const point = canvasPoint(event);
      quick("pointer_move", point.x, point.y, event.ctrlKey || event.metaKey);
    }
    scheduleRender();
  });
  canvas.addEventListener("pointerup", (event) => endPointerGesture(event));
  canvas.addEventListener("pointercancel", (event) => endPointerGesture(event, true));
  canvas.addEventListener("lostpointercapture", (event) => endPointerGesture(event));
  canvas.addEventListener("pointerleave", () => {
    dom.cursorPosition.textContent = "—";
  });
  canvas.addEventListener("wheel", (event) => {
    if (!engineReady || busy || !hasDocument()) return;
    event.preventDefault();
    let delta = event.deltaY;
    if (event.deltaMode === WheelEvent.DOM_DELTA_LINE) delta *= 16;
    if (event.deltaMode === WheelEvent.DOM_DELTA_PAGE) delta *= Math.max(1, canvas.clientHeight);
    if (pointerGesture?.mode === "fly") {
      const value = Math.min(8, Math.max(1, Number(dom.cameraSpeed.value) + (delta < 0 ? 1 : -1)));
      dom.cameraSpeed.value = String(value);
      dom.cameraSpeedLabel.textContent = String(value);
      showTip(`Camera speed ${value}`);
      return;
    }
    const point = canvasPoint(event);
    quick("zoom_at", delta, point.x, point.y);
    scheduleRender();
  }, { passive: false });

  // Dropping objects dragged from the Place panel.
  canvas.addEventListener("dragover", (event) => {
    if (!Array.from(event.dataTransfer?.types || []).includes("application/x-dd-object")) return;
    event.preventDefault();
    event.dataTransfer.dropEffect = "copy";
    dom.viewportWrap.classList.add("drop-target");
  });
  canvas.addEventListener("dragleave", () => dom.viewportWrap.classList.remove("drop-target"));
  canvas.addEventListener("drop", (event) => {
    const id = event.dataTransfer?.getData("application/x-dd-object");
    dom.viewportWrap.classList.remove("drop-target");
    if (!id) return;
    event.preventDefault();
    event.stopPropagation();
    const point = canvasPoint(event);
    placeObject(Number(id), point.x, point.y);
  });
}

function isFileDrag(event) {
  return Array.from(event.dataTransfer?.types || []).includes("Files");
}

async function handleDroppedFiles(fileList) {
  const files = Array.from(fileList);
  if (!files.length || !engineReady || busy) return;
  dragDepth = 0;
  dom.dropOverlay.hidden = true;
  const supported = files.filter((file) => COURSE_EXTENSIONS.has(extensionOf(file.name)) || RELATED_EXTENSIONS.has(extensionOf(file.name)));
  if (supported.length !== files.length) showNotice("Some files were skipped", "Only BOL, ARC, SZS, BCO, BTI, and BMD files can be imported.", "warning");
  if (!hasDocument() || (supported.length === 1 && extensionOf(supported[0].name) === "bol")) {
    const courseIndex = supported.findIndex((file) => COURSE_EXTENSIONS.has(extensionOf(file.name)));
    if (courseIndex >= 0) {
      const [course] = supported.splice(courseIndex, 1);
      await openCourse(course);
    }
  }
  const related = supported.filter((file) => RELATED_EXTENSIONS.has(extensionOf(file.name)));
  if (related.length && hasDocument() && !busy) await loadRelatedFiles(related);
}

function bindFilesAndDrops() {
  const chooseCourse = () => {
    closeExportMenu();
    if (engineReady && !busy) {
      dom.openInput.value = "";
      dom.openInput.click();
    }
  };
  dom.openButton.addEventListener("click", chooseCourse);
  dom.emptyOpenButton.addEventListener("click", chooseCourse);
  dom.relatedButton.addEventListener("click", () => {
    if (engineReady && !busy && hasDocument()) {
      dom.relatedInput.value = "";
      dom.relatedInput.click();
    }
  });
  dom.addLibraryButton.addEventListener("click", () => {
    if (engineReady && !busy) {
      dom.libraryInput.value = "";
      dom.libraryInput.click();
    }
  });
  dom.openInput.addEventListener("change", () => {
    const [file] = dom.openInput.files || [];
    dom.openInput.value = "";
    if (file) void openCourse(file);
  });
  dom.relatedInput.addEventListener("change", () => {
    const files = Array.from(dom.relatedInput.files || []);
    dom.relatedInput.value = "";
    if (files.length) void loadRelatedFiles(files);
  });
  dom.libraryInput.addEventListener("change", () => {
    const files = Array.from(dom.libraryInput.files || []);
    dom.libraryInput.value = "";
    if (files.length) void addLibraries(files);
  });
  dom.replaceInput.addEventListener("change", () => {
    const [file] = dom.replaceInput.files || [];
    const index = Number(dom.replaceInput.dataset.assetIndex);
    dom.replaceInput.value = "";
    if (!file || !Number.isFinite(index)) return;
    void runAction(`Replacing the asset with ${file.name}…`, async () => editor.replace_asset(index, await readFileBytes(file, `Reading ${file.name}`)), {
      success: `Replaced the selected asset with ${file.name}.`,
      errorTitle: `Couldn’t replace the asset with ${file.name}`,
    });
  });

  dom.app.addEventListener("dragenter", (event) => {
    if (!isFileDrag(event) || !engineReady || busy) return;
    event.preventDefault();
    dragDepth++;
    dom.dropOverlay.hidden = false;
  });
  dom.app.addEventListener("dragover", (event) => {
    if (!isFileDrag(event) || !engineReady || busy) return;
    event.preventDefault();
    event.dataTransfer.dropEffect = "copy";
    dom.dropOverlay.hidden = false;
  });
  dom.app.addEventListener("dragleave", (event) => {
    if (!isFileDrag(event)) return;
    dragDepth = Math.max(0, dragDepth - 1);
    if (!dragDepth) dom.dropOverlay.hidden = true;
  });
  dom.app.addEventListener("drop", (event) => {
    if (!isFileDrag(event)) return;
    event.preventDefault();
    dragDepth = 0;
    dom.dropOverlay.hidden = true;
    void handleDroppedFiles(event.dataTransfer.files);
  });
  window.addEventListener("dragover", (event) => {
    if (isFileDrag(event) && !dom.app.contains(event.target)) event.preventDefault();
  });
  window.addEventListener("drop", (event) => {
    if (isFileDrag(event)) event.preventDefault();
  });
}

function bindMenus() {
  dom.exportMenuButton.addEventListener("click", () => {
    if (dom.exportMenu.hidden) openExportMenu(true);
    else closeExportMenu();
  });
  dom.exportMenu.addEventListener("keydown", (event) => {
    const items = [dom.exportBolButton, dom.exportOriginalButton];
    const current = items.indexOf(document.activeElement);
    if (event.key === "Escape") {
      event.preventDefault();
      closeExportMenu(true);
    } else if (event.key === "ArrowDown" || event.key === "ArrowUp") {
      event.preventDefault();
      items[(current + (event.key === "ArrowDown" ? 1 : -1) + items.length) % items.length].focus();
    }
  });
  dom.showButton.addEventListener("click", () => {
    const open = dom.showMenu.hidden;
    dom.showMenu.hidden = !open;
    dom.showButton.setAttribute("aria-expanded", String(open));
  });
  document.addEventListener("pointerdown", (event) => {
    if (!dom.exportMenuRoot.contains(event.target)) closeExportMenu();
    if (!dom.showMenuRoot.contains(event.target)) {
      dom.showMenu.hidden = true;
      dom.showButton.setAttribute("aria-expanded", "false");
    }
  });
  dom.gameViewButton.addEventListener("click", toggleGameView);
}

function bindPanels() {
  for (const tab of document.querySelectorAll(".panel-tab")) {
    tab.addEventListener("click", () => {
      for (const other of document.querySelectorAll(".panel-tab")) {
        const selected = other === tab;
        other.setAttribute("aria-selected", String(selected));
        byId(other.getAttribute("aria-controls")).hidden = !selected;
      }
    });
  }
  dom.outlinerSearch.addEventListener("input", renderEntities);
  for (const input of [dom.placeSearch, dom.placeModelsOnly, dom.placeShowInternal]) input.addEventListener("input", renderPlaceList);
  dom.detailsSearch.addEventListener("input", applyDetailsFilter);

  // Column resizing, remembered across sessions.
  let saved = {};
  try {
    saved = JSON.parse(localStorage.getItem("dds-panels") || "{}");
  } catch {
    saved = {};
  }
  if (saved.left) dom.workspace.style.setProperty("--left-width", `${saved.left}px`);
  if (saved.right) dom.workspace.style.setProperty("--right-width", `${saved.right}px`);
  for (const handle of document.querySelectorAll(".panel-resizer")) {
    handle.addEventListener("pointerdown", (event) => {
      event.preventDefault();
      handle.setPointerCapture(event.pointerId);
      handle.dataset.active = "true";
      const side = handle.dataset.resize;
      const rect = dom.workspace.getBoundingClientRect();
      const move = (moveEvent) => {
        const width = side === "left" ? moveEvent.clientX - rect.left : rect.right - moveEvent.clientX;
        const clamped = Math.round(Math.min(560, Math.max(200, width)));
        dom.workspace.style.setProperty(`--${side}-width`, `${clamped}px`);
        saved[side] = clamped;
        scheduleResize();
      };
      const up = () => {
        handle.dataset.active = "false";
        handle.removeEventListener("pointermove", move);
        handle.removeEventListener("pointerup", up);
        localStorage.setItem("dds-panels", JSON.stringify(saved));
      };
      handle.addEventListener("pointermove", move);
      handle.addEventListener("pointerup", up);
    });
  }
}

function toggleShading() {
  if (!engineReady) return;
  const next = currentSnapshot?.view?.shading === "simple" ? "material" : "simple";
  try {
    refreshSnapshot(editor.set_shading(next));
    setStatus(currentSnapshot.status);
  } catch (error) {
    showNotice("Couldn’t change shading", error);
  }
  scheduleRender();
}

function bindViewportToolbar() {
  for (const button of document.querySelectorAll("[data-tool]")) button.addEventListener("click", () => setTool(button.dataset.tool));
  const applySnap = (enabled) => {
    if (!engineReady) return;
    try {
      refreshSnapshot(editor.set_snap(enabled, Number(dom.snapTranslate.value), Number(dom.snapRotate.value)));
      setStatus(currentSnapshot.status);
    } catch (error) {
      showNotice("Couldn’t change snapping", error);
    }
  };
  dom.snapToggle.addEventListener("click", () => applySnap(dom.snapToggle.getAttribute("aria-pressed") !== "true"));
  dom.snapTranslate.addEventListener("change", () => applySnap(true));
  dom.snapRotate.addEventListener("change", () => applySnap(true));
  dom.shadingButton.addEventListener("click", toggleShading);
  dom.topButton.addEventListener("click", () => {
    quick("view_top");
    scheduleRender();
  });
  dom.perspectiveButton.addEventListener("click", () => {
    const info = JSON.parse(quick("camera_info") || "{}");
    quick("set_camera_angles", info.yaw ?? 45, 35);
    scheduleRender();
  });
  dom.cameraSpeed.addEventListener("input", () => {
    dom.cameraSpeedLabel.textContent = dom.cameraSpeed.value;
  });
}

function bindCommands() {
  dom.addObjectButton.addEventListener("click", () => placeObject(1));
  dom.duplicateButton.addEventListener("click", () => void runAction("Duplicating…", () => editor.duplicate_selected(), { errorTitle: "Couldn’t duplicate the selection" }));
  dom.deleteButton.addEventListener("click", () => void runAction("Deleting…", () => editor.delete_selected(), { success: "Deleted. Undo is available.", errorTitle: "Couldn’t delete the selection" }));
  dom.focusButton.addEventListener("click", focusSelection);
  dom.floorButton.addEventListener("click", () => void runAction("Dropping to the floor…", () => editor.drop_to_floor(), { errorTitle: "Couldn’t drop to the floor" }));
  dom.undoButton.addEventListener("click", () => void runAction("Undoing…", () => editor.undo(), { success: "Undid the last edit.", errorTitle: "Undo failed" }));
  dom.redoButton.addEventListener("click", () => void runAction("Redoing…", () => editor.redo(), { success: "Redid the edit.", errorTitle: "Redo failed" }));
  dom.exportBolButton.addEventListener("click", () => void exportDocument("export_bol", "bol"));
  dom.exportOriginalButton.addEventListener("click", () => void exportDocument("export_original_bol", "bol", true));
  dom.exportArchiveButton.addEventListener("click", () => void exportDocument("export_archive", "arc"));
  dom.frameButton.addEventListener("click", focusSelection);
  dom.issuesToggle.addEventListener("click", () => toggleIssues());
  dom.issuesClose.addEventListener("click", () => toggleIssues(false));
  const toggleHelp = (force) => {
    dom.helpOverlay.hidden = force === undefined ? !dom.helpOverlay.hidden : !force;
    if (!dom.helpOverlay.hidden) dom.helpClose.focus();
  };
  dom.helpButton.addEventListener("click", () => toggleHelp());
  dom.helpClose.addEventListener("click", () => toggleHelp(false));
  dom.helpOverlay.addEventListener("click", (event) => {
    if (event.target === dom.helpOverlay) toggleHelp(false);
  });

  document.addEventListener("keydown", (event) => {
    const key = event.key.toLowerCase();
    if (event.shiftKey) keysDown.add("shift");
    if (event.defaultPrevented || isTypingTarget(event.target)) return;
    if (!dom.helpOverlay.hidden) {
      if (key === "escape" || key === "?") toggleHelp(false);
      return;
    }
    if (key === "?" || key === "f1") {
      event.preventDefault();
      toggleHelp(true);
      return;
    }
    if (!engineReady) return;
    const modifier = event.ctrlKey || event.metaKey;
    keysDown.add(key);
    const flyKey = pointerGesture?.mode === "fly" && ["w", "a", "s", "d", "q", "e"].includes(key);
    if (!modifier && (flyKey || key.startsWith("arrow"))) {
      if (hasDocument()) {
        event.preventDefault();
        ensureFlyLoop();
      }
      return;
    }
    if (busy) return;
    if (modifier && key === "z") {
      event.preventDefault();
      (event.shiftKey ? dom.redoButton : dom.undoButton).click();
    } else if (modifier && key === "y") {
      event.preventDefault();
      dom.redoButton.click();
    } else if (modifier && key === "d") {
      event.preventDefault();
      if (!dom.duplicateButton.disabled) dom.duplicateButton.click();
    } else if (modifier && key === "s") {
      event.preventDefault();
      if (!dom.exportArchiveButton.disabled) dom.exportArchiveButton.click();
      else if (!dom.exportMenuButton.disabled) void exportDocument("export_bol", "bol");
    } else if (modifier && key === "o") {
      event.preventDefault();
      dom.openButton.click();
    } else if (modifier || event.altKey) {
      return;
    } else if (key === "escape") {
      if (pointerGesture?.mode === "gizmo") endPointerGesture(null, true);
      else if (selection()) selectEntity("none", 0);
    } else if (key === "f") {
      event.preventDefault();
      focusSelection();
    } else if (key === "home") {
      event.preventDefault();
      frameAll();
    } else if (key === "q") setTool("select");
    else if (key === "w") setTool("translate");
    else if (key === "e") setTool("rotate");
    else if (key === "r") setTool("scale");
    else if (key === " " && hasDocument()) {
      event.preventDefault();
      const order = ["translate", "rotate", "scale"];
      setTool(order[(order.indexOf(tool) + 1) % order.length]);
      showTip(`${TOOL_LABELS[tool]} tool`);
    } else if (key === "t") {
      quick("view_top");
      scheduleRender();
    } else if (key === "g") toggleGameView();
    else if (key === "l") toggleShading();
    else if (key === "end") {
      event.preventDefault();
      if (!dom.floorButton.disabled) dom.floorButton.click();
    } else if (key === "delete" || key === "backspace") {
      if (!dom.deleteButton.disabled) {
        event.preventDefault();
        dom.deleteButton.click();
      }
    }
  });
  document.addEventListener("keyup", (event) => {
    keysDown.delete(event.key.toLowerCase());
    if (!event.shiftKey) keysDown.delete("shift");
  });
  window.addEventListener("blur", () => keysDown.clear());
  window.addEventListener("beforeunload", (event) => {
    if (!currentSnapshot?.dirty) return;
    event.preventDefault();
    event.returnValue = "";
  });
}

function initializeEmptyView() {
  renderFileIdentity(null);
  renderFiles();
  renderEntities();
  renderIssues();
  dom.emptyTitle.textContent = "Starting Double Dash Studio…";
  dom.emptyDescription.textContent = "Loading the offline Rust and WebAssembly editor engine. Your files stay on this machine.";
  setEngineState("loading", "Starting");
  setTool(tool);
  updateCommands();
}

async function bootstrap() {
  initializeEmptyView();
  bindFilesAndDrops();
  bindCommands();
  bindMenus();
  bindPanels();
  bindViewportToolbar();
  bindViewport();
  if (typeof ResizeObserver === "function") new ResizeObserver(scheduleResize).observe(dom.viewportWrap);
  window.addEventListener("resize", scheduleResize);

  try {
    const module = await import("./pkg/track_editor.js");
    await module.default();
    editor = new module.Editor("viewport");
    // Exposed for automated browser tests and console debugging.
    window.__ddStudio = editor;
    engineReady = true;
    setEngineState("ready", "Ready");
    refreshSnapshot();
    const restored = await restoreLibraries();
    dom.emptyTitle.textContent = "Your next lap starts here.";
    dom.emptyDescription.textContent = "Open a course ARC to edit it in a fully textured 3D view — entirely on your machine.";
    setStatus(restored
      ? `Ready. Restored ${restored} model librar${restored === 1 ? "y" : "ies"}. Open a course ARC or BOL to begin.`
      : "Ready. Open a course ARC or BOL to begin.");
    setTool(tool);
    updateCommands();
    scheduleResize();
  } catch (error) {
    console.error("Double Dash Studio initialization failed", error);
    engineReady = false;
    editor = null;
    setEngineState("error", "Unavailable");
    const guidance = "The local WebAssembly package could not be loaded. Run tools/track-editor/serve.sh to build www/pkg and serve this directory over HTTP.";
    dom.emptyKicker.textContent = "EDITOR ENGINE UNAVAILABLE";
    dom.emptyTitle.textContent = "The studio couldn’t start.";
    dom.emptyDescription.textContent = guidance;
    setStatus("Editor engine unavailable. Check the local pkg build and browser console.");
    showNotice("Couldn’t start Double Dash Studio", `${guidance} ${friendlyError(error)}`);
    updateCommands();
  }
}

void bootstrap();
