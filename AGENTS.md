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
