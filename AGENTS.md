# mkdd decomp agent notes

Mario Kart Double Dash!! matching decompilation (doldecomp/mkdd fork).
Target version: `MarioClub_us` (debug). Build with `ninja`. Diff with
`objdiff.json` / `build/tools/objdiff-cli`.

## Tools

Prefer the `mkdd-decomp` MCP tools (or `python tools/decomp.py <cmd>`) over
ad-hoc shell. They wrap the project's data sources and keep compiles isolated.

- `decomp_next` / `decomp_units` / `decomp_functions` — pick work
- `decomp_asm` — target assembly with relocations (`--orig` is the default)
- `decomp_diff` / `decomp_diff_unit` — compile one TU in isolation, score diffs
- `decomp_check`, `decomp_context`, `decomp_symbols`, `decomp_callers`,
  `decomp_callees`, `decomp_regress`, `decomp_progress`
- `decomp_port` — search sibling decomp checkouts under `references/` for an
  implementation to adapt

CLI flags such as `--json` go before the subcommand. To use `decomp_port`,
clone sibling projects into `references/` (gitignored), e.g.
`git clone --depth 1 https://github.com/doldecomp/sms references/sms`.

## Match loop

Work in small batches (1–5 functions), not whole units. Keep the TU compiling
after each function.

1. Pick one function (`decomp_next` or `decomp_functions <unit>`). Prefer small,
   untouched functions in nearly-finished units and portable jsys/sdk code.
2. Read the target with `decomp_asm <unit> <symbol>`. Read the current source,
   headers, and callers/callees before writing.
3. Implement in the unit's `.cpp`; keep declarations and field offsets in the
   matching headers. Match the original construction order, types, and
   inlining so the compiler emits the same code.
4. `decomp_diff <unit> <symbol>` until MATCH.
5. `decomp_diff_unit <unit>` and make sure the whole TU compiles and nothing
   regressed.
6. Run the symbol-order validator for changed units:
   `python tools/validate-symbol-order.py -u <unit>`
7. `python tools/decomp.py regress` (or `ninja changes`) must be clean.

## Rules

- Never edit `config/<version>/symbols.txt` or `splits.txt` unless the unit
  genuinely needs it; explain why in the commit/PR.
- Do not touch compiler flags to force a match without checking other functions
  in the same TU with `decomp_diff_unit`.
- Keep decompiled code style consistent with the surrounding file. No new
  comments unless they explain a compiler quirk.
- Build artifacts under `build/` are not committed. `report.json` is stale
  until `python tools/decomp.py report` or a build refresh.
- `decomp diff` exits 1 on a mismatch; `decomp regress` exits 1 on regressions.
- Data matching counts too: prefer declaring real types/initializers over
  casts to opaque bytes.

## Track Studio (`tools/track-editor`)

Standalone offline Rust/WASM/WebGL2 course and asset editor. It is independent of
Ninja and the matching build; `tools/track-editor/serve.sh` tests, builds into
`www/pkg`, and serves on `127.0.0.1:8000` (`PORT`/`HOST` can override). Cargo
artifacts default to `build/track-editor/target`; generated WASM and proprietary
assets are not committed.

- BOL `0015`: all sections, field edits, object add/duplicate/delete, undo/redo,
  validation, and raw-preserving nonstructural export. Structural export is
  canonical; unmodified export is byte-identical.
- BCO `0003`: collision rendering and inspection, not collision writing.
- BMD: course, sky, and object geometry with full GX materials: color
  channels/lighting, all texgens and texture-matrix modes, multi-stage TEV
  (generated GLSL in `src/gx_shader.rs`), alpha test, blend and depth modes.
  Not implemented: indirect texturing, BTK/BRK/BCK animation, billboards,
  BMD writing.
- Objects draw their real models, resolved by ID through `catalog::model_names`
  (from `getBmdFileName()` in the decomp) against the course ARC and any model
  library ARCs (e.g. `MRAM.arc`, persisted in the browser's IndexedDB).
- Viewport: Unreal-style fly camera (RMB + WASD), translate/rotate/scale gizmo
  with snapping and single-step undo, raycast placement, drop-to-floor,
  logarithmic depth. Press `?` in the editor for all controls.
- `cargo run --example inspect_arc -- [--survey|--objects|--dump] ...` reports
  model decode coverage, unresolved object models, and per-mesh GX state.
- RARC/Yaz0/Yay0: archive browsing, extraction, file replacement, and raw RARC
  export with the current BOL injected. Modified exports decompress entries.
- BTI: RGBA preview for direct/indexed/CMPR formats.

Run native tests and optional user-owned corpus tests with:

```sh
cargo test --manifest-path tools/track-editor/Cargo.toml
MKDD_TEST_COURSE_DIR=/absolute/extracted/course/dir \
  cargo test --manifest-path tools/track-editor/Cargo.toml --test real_files
```

For the real Chrome CDP smoke test, set `MKDD_TEST_ARC`, optionally
`MKDD_TEST_BOL`, and `MKDD_EDITOR_TEST_OUT` to ignored output directories, then
run `node tools/track-editor/tests/browser_smoke.mjs`.
