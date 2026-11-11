#!/usr/bin/env python3
"""Decompilation workflow helper for this repository.

One entry point for the repetitive parts of matching a decompilation unit:
finding work, reading target assembly, compiling one TU in isolation, scoring a
function diff, inspecting symbols/callers, and detecting regressions.

Every command accepts --json and emits a JSON object, so the same code powers
both humans and tools/decomp_mcp.py.

Examples:
    python tools/decomp.py next --n 15
    python tools/decomp.py funcs main/Sato/stParticle
    python tools/decomp.py asm main/Sato/stParticle SomeSymbol
    python tools/decomp.py diff src/Sato/stParticle.cpp SomeSymbol
    python tools/decomp.py diff-unit src/Sato/stParticle.cpp
    python tools/decomp.py asm --orig main/Kameda/Race2D getGoalRankPos__...
    python tools/decomp.py regress
"""

import argparse
import difflib
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from fndiff import disasm, function_bounds, objdump_code  # noqa: E402
from fntry import find_build  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJDUMP = "/opt/devkitpro/devkitPPC/bin/powerpc-eabi-objdump"
NM = "/opt/devkitpro/devkitPPC/bin/powerpc-eabi-nm"
OBJDIFF_JSON = os.path.join(ROOT, "objdiff.json")
DEFAULT_VERSION = "MarioClub_us"


# ---------------------------------------------------------------------------
# Project data
# ---------------------------------------------------------------------------


class Data:
    def __init__(self, version: str):
        self.version = version
        self.build = os.path.join(ROOT, "build", version)
        self.report_path = os.path.join(self.build, "report.json")
        self.baseline_path = os.path.join(self.build, "baseline.json")
        self.map_path = os.path.join(
            ROOT, "orig", version, "files", "debugInfoM.MAP"
        )
        with open(OBJDIFF_JSON, encoding="utf-8") as f:
            self.units = json.load(f)["units"]
        self.report = None
        if os.path.exists(self.report_path):
            with open(self.report_path, encoding="utf-8") as f:
                self.report = json.load(f)
        self.by_name = {u["name"]: u for u in self.units}
        self.by_source = {}
        for u in self.units:
            sp = u.get("metadata", {}).get("source_path")
            if sp:
                self.by_source[norm(sp)] = u
        self.report_units = {}
        if self.report:
            self.report_units = {u["name"]: u for u in self.report["units"]}

    def resolve_unit(self, arg: str) -> dict:
        arg = norm(arg)
        u = self.by_name.get(arg) or self.by_source.get(arg)
        if u:
            return u
        if arg.startswith("src/") or arg.startswith("libs/"):
            u = self.by_source.get(arg)
            if u:
                return u
        cands = [u for u in self.units if u["name"].endswith(arg)]
        cands += [u for u in self.units if arg in (u.get("metadata", {}).get("source_path") or "")]
        uniq = {u["name"]: u for u in cands}
        if len(uniq) == 1:
            return next(iter(uniq.values()))
        if len(uniq) > 1:
            names = "\n  ".join(sorted(uniq))
            raise SystemExit(f"ambiguous unit '{arg}', candidates:\n  {names}")
        raise SystemExit(f"unit not found: {arg}")

    def source_for(self, arg: str) -> tuple[dict, str]:
        u = self.resolve_unit(arg)
        src = u.get("metadata", {}).get("source_path")
        if not src:
            raise SystemExit(f"unit has no source path: {u['name']}")
        return u, src

    def unit_report(self, name: str) -> dict:
        return self.report_units.get(name, {})

    def functions(self, name: str) -> list:
        return self.unit_report(name).get("functions", [])


def norm(p: str) -> str:
    return p.replace("\\", "/").lstrip("./")


def out(obj, as_json: bool, text_fn=None):
    if as_json:
        json.dump(obj, sys.stdout, indent=2, sort_keys=False)
        sys.stdout.write("\n")
    elif text_fn:
        text_fn(obj)
    else:
        print(json.dumps(obj, indent=2))


def pct(matched, total):
    total = float(total or 0)
    return 100.0 * float(matched or 0) / total if total else 0.0


# ---------------------------------------------------------------------------
# Compilation (single TU, isolated from the shared build dir)
# ---------------------------------------------------------------------------


def compile_unit(src: str) -> tuple[str, str]:
    """Compile src to a temp object. Returns (temp_obj, orig_obj)."""
    rule, vars_ = find_build(src)
    if rule is None:
        raise SystemExit(f"no build rule for {src}")
    pre = [os.path.join(ROOT, "build/tools/wibo")]
    if rule in ("mwcc_sjis", "mwcc_sjis_extab"):
        pre.append(os.path.join(ROOT, "build/tools/sjiswrap.exe"))
    cc = pre + [
        os.path.join(ROOT, "build/compilers", vars_.get("mw_version", "GC/2.6"), "mwcceppc.exe")
    ] + shlex.split(vars_.get("cflags", "")) + ["-c", src, "-o"]
    fd, tmp = tempfile.mkstemp(suffix=".o", prefix="decomp_")
    os.close(fd)
    if rule == "as":
        cmd = [
            os.path.join(ROOT, "build/binutils/powerpc-eabi-as")
        ] + shlex.split(vars_.get("asflags", "")) + ["-o", tmp, src]
        r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
        if r.returncode == 0:
            subprocess.run(
                [os.path.join(ROOT, "build/tools/dtk"), "elf", "fixup", tmp, tmp],
                cwd=ROOT, capture_output=True, text=True,
            )
    else:
        r = subprocess.run(cc + [tmp], cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        try:
            os.unlink(tmp)
        except OSError:
            pass
        raise SystemExit("COMPILE FAILED\n" + r.stdout + r.stderr)
    unit = src
    for pfx in ("src/", "libs/"):
        if unit.startswith(pfx):
            unit = unit[len(pfx):]
    unit = re.sub(r"\.(cpp|c|cp|cc)$", "", unit)
    orig = os.path.join(ROOT, "build", CURRENT_VERSION, "obj", unit + ".o")
    with open(OBJDIFF_JSON, encoding="utf-8") as f:
        for u in json.load(f).get("units", []):
            if norm(u.get("metadata", {}).get("source_path") or "") == norm(src):
                orig = os.path.join(ROOT, u["target_path"])
                break
    return tmp, orig


def obj_symbols(obj: str) -> list:
    r = subprocess.run(
        [NM, "--defined-only", obj], capture_output=True, text=True
    )
    syms = []
    for line in r.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[-2] in ("t", "T", "w", "W"):
            syms.append(parts[-1])
    return syms


def suggest(symbol: str, candidates: list, n=5) -> list:
    return difflib.get_close_matches(symbol, candidates, n=n, cutoff=0.45)


def raw_disasm(obj: str, symbol: str) -> list:
    lines = objdump_code(obj, check=False)
    bounds = function_bounds(lines, symbol)
    if bounds is None:
        raise SystemExit(f"symbol not found: {symbol}")
    start, end = bounds
    return lines[start:end]


# ---------------------------------------------------------------------------
# Diff classification
# ---------------------------------------------------------------------------


def classify(orig_lines: list, our_lines: list) -> list:
    """Summarize instruction-level differences without being too noisy."""
    hints = []
    sm = difflib.SequenceMatcher(a=orig_lines, b=our_lines)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            continue
        if tag == "replace":
            for a, b in zip(orig_lines[i1:i2], our_lines[j1:j2]):
                ao = a.split()[0] if a.split() else ""
                bo = b.split()[0] if b.split() else ""
                if ao != bo:
                    hints.append(f"opcode {ao} -> {bo}")
                elif a != b:
                    hints.append(f"operand {a.strip()} -> {b.strip()}")
            if abs((i2 - i1) - (j2 - j1)) > 0:
                hints.append(f"block length {i2 - i1} -> {j2 - j1}")
        elif tag == "delete":
            hints.append(f"missing {i2 - i1} instr(s): " + "; ".join(orig_lines[i1:i2][:3]))
        elif tag == "insert":
            hints.append(f"extra {j2 - j1} instr(s): " + "; ".join(our_lines[j1:j2][:3]))
    seen = []
    for h in hints:
        if h not in seen:
            seen.append(h)
    return seen[:40]


def diff_symbol(our_obj: str, orig_obj: str, symbol: str) -> dict:
    try:
        a = disasm(our_obj, symbol)
        b = disasm(orig_obj, symbol)
    except SystemExit:
        return {"symbol": symbol, "error": "not disassemblable",
                "match": False, "score": 0.0, "our_instructions": 0,
                "target_instructions": 0, "hints": []}
    if a == b:
        return {"symbol": symbol, "match": True, "score": 100.0,
                "our_instructions": len(a), "target_instructions": len(b),
                "hints": []}
    ratio = difflib.SequenceMatcher(a=b, b=a).ratio() * 100.0
    udiff = list(
        difflib.unified_diff(b, a, "target", "ours", lineterm="", n=2)
    )
    return {
        "symbol": symbol,
        "match": False,
        "score": round(ratio, 3),
        "our_instructions": len(a),
        "target_instructions": len(b),
        "hints": classify(b, a),
        "diff": udiff[:400],
    }


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------


def cmd_progress(d: Data, args):
    if not d.report:
        raise SystemExit(f"no report: {d.report_path}")
    m = d.report["measures"]
    data = {
        "version": d.version,
        "report_age_seconds": int(time.time() - os.path.getmtime(d.report_path)),
        "code": {
            "matched": int(m["matched_code"]),
            "total": int(m["total_code"]),
            "percent": round(float(m["matched_code_percent"]), 3),
        },
        "functions": {
            "matched": int(m["matched_functions"]),
            "total": int(m["total_functions"]),
            "percent": round(float(m["matched_functions_percent"]), 3),
        },
        "data": {
            "matched": int(m["matched_data"]),
            "total": int(m["total_data"]),
            "percent": round(float(m["matched_data_percent"]), 3),
        },
        "units": {
            "complete": int(m["complete_units"]),
            "total": int(m["total_units"]),
        },
        "categories": [],
    }
    for cat in d.report.get("categories", []):
        cm = cat.get("measures", {})
        data["categories"].append({
            "id": cat.get("id"),
            "code_percent": round(float(cm.get("matched_code_percent", 0)), 3),
            "function_percent": round(float(cm.get("matched_functions_percent", 0)), 3),
            "data_percent": round(float(cm.get("matched_data_percent", 0)), 3),
        })
    out(data, args.json, lambda x: print(
        f"{x['version']} code {x['code']['matched']}/{x['code']['total']} "
        f"({x['code']['percent']:.2f}%) functions "
        f"{x['functions']['matched']}/{x['functions']['total']} "
        f"({x['functions']['percent']:.2f}%) data {x['data']['percent']:.2f}% "
        f"units {x['units']['complete']}/{x['units']['total']}"))


def cmd_units(d: Data, args):
    rows = []
    for u in d.units:
        m = d.unit_report(u["name"]).get("measures", {})
        if not m:
            continue
        total = int(m.get("total_code", 0))
        matched = int(m.get("matched_code", 0))
        if total == 0:
            continue
        if args.unmatched and matched >= total:
            continue
        rows.append({
            "unit": u["name"],
            "source": u.get("metadata", {}).get("source_path"),
            "category": (u.get("metadata", {}).get("progress_categories") or [""])[0],
            "matched_code": matched,
            "total_code": total,
            "matched_percent": round(pct(matched, total), 2),
            "matched_functions": int(m.get("matched_functions", 0)),
            "total_functions": int(m.get("total_functions", 0)),
            "complete": bool(u.get("metadata", {}).get("complete")),
        })
    key = {
        "bytes": lambda r: r["total_code"] - r["matched_code"],
        "pct": lambda r: r["matched_percent"],
        "funcs": lambda r: r["total_functions"] - r["matched_functions"],
    }[args.sort]
    rows.sort(key=key, reverse=not args.ascending)
    if args.category:
        rows = [r for r in rows if r["category"] == args.category]
    rows = rows[: args.limit]
    out(rows, args.json, lambda rs: [print(
        f"{r['matched_code']:7d}/{r['total_code']:7d} {r['matched_percent']:6.2f}% "
        f"f {r['matched_functions']:3d}/{r['total_functions']:3d} {r['unit']}")
        for r in rs])


def cmd_funcs(d: Data, args):
    u = d.resolve_unit(args.unit)
    rows = []
    for f in d.functions(u["name"]):
        p = float(f.get("fuzzy_match_percent", 0))
        if args.unmatched and p >= 99.999:
            continue
        rows.append({
            "symbol": f["name"],
            "demangled": f.get("metadata", {}).get("demangled_name", ""),
            "size": int(f.get("size", 0)),
            "score": round(p, 3),
            "address": f.get("metadata", {}).get("virtual_address"),
        })
    rows.sort(key=lambda r: (r["score"], -r["size"]))
    rows = rows[: args.limit]
    out(rows, args.json, lambda rs: [print(
        f"{r['score']:7.2f}% {r['size']:6d} {r['symbol']}  {r['demangled']}")
        for r in rs])


def next_candidates(d: Data, mode: str) -> list:
    candidates = []
    for u in d.units:
        ur = d.unit_report(u["name"])
        m = ur.get("measures", {})
        if not m:
            continue
        total = int(m.get("total_code", 0))
        matched = int(m.get("matched_code", 0))
        if total == 0 or matched >= total:
            continue
        cat = (u.get("metadata", {}).get("progress_categories") or [""])[0]
        unit_pct = pct(matched, total)
        for f in ur.get("functions", []):
            score = float(f.get("fuzzy_match_percent", 0))
            if score >= 99.999:
                continue
            size = int(f.get("size", 0))
            if size <= 0 or size > 1200:
                continue
            # reward small, untouched functions; reward finishing units
            value = 1.0 / (size ** 0.55)
            value *= 1.0 + (unit_pct / 100.0) * 1.5
            if cat in ("jsys", "sdk", "msl"):
                value *= 1.8
            if mode == "finish" and unit_pct < 55:
                value *= 0.35
            if mode == "sweep" and unit_pct > 35:
                value *= 0.5
            if score > 0:
                value *= 1.15
            candidates.append({
                "unit": u["name"],
                "source": u.get("metadata", {}).get("source_path"),
                "category": cat,
                "symbol": f["name"],
                "demangled": f.get("metadata", {}).get("demangled_name", ""),
                "size": size,
                "score": round(score, 2),
                "unit_percent": round(unit_pct, 2),
                "value": round(value, 5),
            })
    candidates.sort(key=lambda r: r["value"], reverse=True)
    return candidates


def cmd_next(d: Data, args):
    rows = next_candidates(d, args.mode)[: args.n]
    out(rows, args.json, lambda rs: [print(
        f"{r['value']:.3f} | {r['unit_percent']:6.2f}% unit | {r['size']:5d}B "
        f"{r['symbol']}  ({r['unit']})") for r in rs])


def cmd_info(d: Data, args):
    u = d.resolve_unit(args.unit)
    ur = d.unit_report(u["name"])
    m = ur.get("measures", {})
    sections = {}
    sections_path = os.path.join(ROOT, "config", d.version, "splits.txt")
    source = u.get("metadata", {}).get("source_path", "")
    rel_stem = os.path.splitext(source)[0]
    unit_stem = os.path.splitext(u["name"].split("/", 1)[-1])[0]
    key_stems = {rel_stem, unit_stem}
    for pfx in ("src/", "libs/"):
        if rel_stem.startswith(pfx):
            key_stems.add(rel_stem[len(pfx):])
    if os.path.exists(sections_path):
        current = None
        for line in open(sections_path, encoding="utf-8", errors="replace"):
            if line and not line.startswith(("\t", " ")) and line.strip().endswith(":"):
                current = line.strip()[:-1]
            elif current and (os.path.splitext(current)[0] in key_stems
                              or os.path.splitext(current)[0].endswith(
                                  "/" + os.path.basename(rel_stem))):
                s = re.match(r"^\s+(\S+)\s+start:0x([0-9A-Fa-f]+)\s+end:0x([0-9A-Fa-f]+)", line)
                if s:
                    sections[s.group(1)] = [s.group(2), s.group(3)]
    data = {
        "unit": u["name"],
        "source": u.get("metadata", {}).get("source_path"),
        "target_object": u.get("target_path"),
        "category": (u.get("metadata", {}).get("progress_categories") or [""])[0],
        "complete": bool(u.get("metadata", {}).get("complete")),
        "matched_code": int(m.get("matched_code", 0)),
        "total_code": int(m.get("total_code", 0)),
        "matched_percent": round(pct(m.get("matched_code", 0), m.get("total_code", 0)), 3),
        "matched_functions": int(m.get("matched_functions", 0)),
        "total_functions": int(m.get("total_functions", 0)),
        "sections": sections,
    }
    out(data, args.json, lambda x: print(json.dumps(x, indent=2)))


def cmd_asm(d: Data, args):
    u = d.resolve_unit(args.unit)
    obj = u["target_path"]
    if args.ours:
        obj = u["base_path"]
    if not os.path.exists(obj):
        raise SystemExit(f"object missing: {obj}")
    try:
        lines = raw_disasm(obj, args.symbol)
    except SystemExit:
        syms = obj_symbols(obj)
        raise SystemExit(
            f"symbol not found: {args.symbol}\n"
            + "did you mean: " + ", ".join(suggest(args.symbol, syms))
        )
    text = "\n".join(lines)
    if args.json:
        out({"unit": u["name"], "symbol": args.symbol, "object": obj,
             "asm": lines}, True)
    else:
        print(text)


def cmd_symbols(d: Data, args):
    q = args.query.lower()
    hits = []
    sym_path = os.path.join(ROOT, "config", d.version, "symbols.txt")
    if os.path.exists(sym_path):
        for line in open(sym_path, encoding="utf-8", errors="replace"):
            if q in line.lower():
                hits.append({"name": line.split("=")[0].strip(), "location": "symbols.txt"})
    for u in d.units:
        for f in d.functions(u["name"]):
            if q in f["name"].lower() or q in f.get("metadata", {}).get("demangled_name", "").lower():
                hits.append({"name": f["name"],
                             "demangled": f.get("metadata", {}).get("demangled_name", ""),
                             "unit": u["name"], "size": int(f.get("size", 0)),
                             "score": round(float(f.get("fuzzy_match_percent", 0)), 2)})
    hits = hits[: args.limit]
    out(hits, args.json, lambda hs: [print(
        f"{h.get('unit', ''):35s} {h.get('name', '')}  {h.get('demangled', '')}")
        for h in hs])


def load_call_tree(path: str) -> tuple[dict, dict]:
    callers, callees = {}, {}
    if not os.path.exists(path):
        return callers, callees
    stack = {}
    rx = re.compile(r"^\s*(\d+)\]\s+(\S+)\s+\((?:func|object|notype),\w+\)")
    for line in open(path, encoding="utf-8", errors="replace"):
        if line.strip().endswith("section layout"):
            break
        m = rx.match(line)
        if not m:
            continue
        depth, name = int(m.group(1)), m.group(2)
        stack[depth] = name
        parent = stack.get(depth - 1)
        if parent and parent != name:
            callers.setdefault(name, [])
            if parent not in callers[name]:
                callers[name].append(parent)
            callees.setdefault(parent, [])
            if name not in callees[parent]:
                callees[parent].append(name)
    return callers, callees


def cmd_callgraph(d: Data, args):
    callers, callees = load_call_tree(d.map_path)
    if args.direction == "callers":
        rows = callers.get(args.symbol, [])
    else:
        rows = callees.get(args.symbol, [])
    out({"symbol": args.symbol, args.direction: rows}, args.json,
        lambda x: [print(r) for r in x[args.direction]])


def cmd_diff(d: Data, args):
    u, src = d.source_for(args.unit)
    if not os.path.exists(os.path.join(ROOT, src)):
        raise SystemExit(f"source missing: {src}")
    tmp, orig_obj = compile_unit(src)
    try:
        if not os.path.exists(orig_obj):
            raise SystemExit(f"target object missing: {orig_obj}")
        results = []
        for sym in args.symbols:
            if not obj_symbols(tmp) or sym not in obj_symbols(tmp):
                results.append({"symbol": sym, "error": "not emitted by this TU",
                                "suggestions": suggest(sym, obj_symbols(tmp))})
                continue
            results.append(diff_symbol(tmp, orig_obj, sym))
    finally:
        try:
            os.unlink(tmp)
        except OSError:
            pass
    ok = all(r.get("match") for r in results)

    def text(x):
        for r in x["results"]:
            if "error" in r:
                print(f"{r['symbol']}: {r['error']}  suggestions: {', '.join(r['suggestions'])}")
                continue
            if r["match"]:
                print(f"{r['symbol']}: MATCH ({r['target_instructions']} instrs)")
                continue
            print(f"{r['symbol']}: DIFF score={r['score']:.2f}% "
                  f"ours={r['our_instructions']} target={r['target_instructions']}")
            for h in r["hints"][:12]:
                print(f"  ! {h}")
            if args.diff_text:
                print("\n".join(r["diff"]))
    out({"source": src, "all_match": ok, "results": results}, args.json, text)
    if not ok:
        sys.exit(1)


def cmd_diff_unit(d: Data, args):
    u, src = d.source_for(args.unit)
    tmp, orig_obj = compile_unit(src)
    try:
        if not os.path.exists(orig_obj):
            raise SystemExit(f"target object missing: {orig_obj}")
        target = set(obj_symbols(orig_obj))
        ours = set(obj_symbols(tmp))
        results = []
        for sym in sorted(target & ours):
            results.append(diff_symbol(tmp, orig_obj, sym))
    finally:
        try:
            os.unlink(tmp)
        except OSError:
            pass
    results.sort(key=lambda r: r["score"])
    matched = sum(1 for r in results if r["match"])
    exact = sum(1 for r in results if r["match"])
    total = len(results)
    data = {
        "source": src,
        "functions": total,
        "exact_matches": exact,
        "missing_from_ours": sorted(target - ours)[:50],
        "extra_in_ours": sorted(ours - target)[:50],
        "avg_score": round(sum(r["score"] for r in results) / total, 2) if total else 0.0,
        "worst": [r for r in results if not r["match"]][: args.worst],
    }
    out(data, args.json, lambda x: (
        print(f"{x['source']}: {x['exact_matches']}/{x['functions']} exact, "
              f"avg {x['avg_score']:.2f}%, {len(x['missing_from_ours'])} missing"),
        [print(f"  {r['score']:6.2f}% {r['symbol']}") for r in x["worst"]]))


def cmd_check(d: Data, args):
    _, src = d.source_for(args.unit)
    tmp, orig_obj = compile_unit(src)
    os.unlink(tmp)
    out({"source": src, "compiled": True, "target_object": orig_obj}, args.json,
        lambda x: print(f"compiled OK: {x['source']}"))


def cmd_context(d: Data, args):
    _, src = d.source_for(args.unit)
    rule, vars_ = find_build(src)
    toks = shlex.split(vars_.get("cflags", "")) if vars_ else []
    cmd = [sys.executable, os.path.join(ROOT, "tools/decompctx.py"), src]
    for i, t in enumerate(toks):
        if t in ("-i", "-I") and i + 1 < len(toks):
            cmd += ["-I", toks[i + 1]]
        elif t.startswith("-D"):
            cmd += ["-D", t[2:]]
    fd, ctx = tempfile.mkstemp(suffix=".c", prefix="decompctx_")
    os.close(fd)
    cmd += ["-o", ctx]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit(r.stdout + r.stderr)
    text = open(ctx, encoding="utf-8", errors="replace").read()
    os.unlink(ctx)
    out({"source": src, "context": text}, args.json, lambda x: print(x["context"]))


def cmd_port(d: Data, args):
    refs = os.path.join(ROOT, args.refs)
    if not os.path.isdir(refs):
        raise SystemExit(
            f"reference dir missing: {refs}\n"
            "clone sibling decomp projects there (Petari, sms, pikmin2, prime, tp)"
        )
    syms = [args.query]
    for u in d.units:
        for f in d.functions(u["name"]):
            if f["name"] == args.query:
                dm = f.get("metadata", {}).get("demangled_name", "")
                base = re.sub(r"\(.*\)$", "", dm)
                if "::" in base:
                    syms.append(base.split("::")[-1] + "(")
                    syms.append(base)
    try:
        r = subprocess.run(["rg", "-l", "-F", "-e", *syms, refs],
                           capture_output=True, text=True, timeout=60)
        files = [f for f in r.stdout.splitlines() if f]
    except (FileNotFoundError, subprocess.TimeoutExpired):
        files = []
        for dirpath, _, names in os.walk(refs):
            for n in names:
                if n.endswith((".c", ".cc", ".cpp", ".h", ".hpp")):
                    p = os.path.join(dirpath, n)
                    try:
                        body = open(p, encoding="utf-8", errors="ignore").read()
                    except OSError:
                        continue
                    if any(s in body for s in syms):
                        files.append(p)
                    if len(files) >= args.limit:
                        break
            if len(files) >= args.limit:
                break
    hits = []
    for path in files[: args.limit]:
        try:
            lines = open(path, encoding="utf-8", errors="ignore").read().splitlines()
        except OSError:
            continue
        for i, line in enumerate(lines):
            if any(s in line for s in syms):
                hits.append({"file": os.path.relpath(path, ROOT), "line": i + 1,
                             "text": line.strip()})
                break
    out({"query": args.query, "siblings": syms, "hits": hits}, args.json,
        lambda x: [print(f"{h['file']}:{h['line']}: {h['text']}") for h in x["hits"]]
        or print("no hits"))


def cmd_report(d: Data, args):
    cmd = [os.path.join(ROOT, "build/tools/objdiff-cli"), "report", "generate",
           "-p", ROOT, "-o", d.report_path, "-f", "json"]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit(r.stdout + r.stderr)
    with open(d.report_path, encoding="utf-8") as f:
        d.report = json.load(f)
    d.report_units = {u["name"]: u for u in d.report["units"]}
    print(f"wrote {d.report_path}", file=sys.stderr)
    cmd_progress(d, args)


def cmd_regress(d: Data, args):
    if not os.path.exists(d.baseline_path):
        raise SystemExit("no baseline; run `ninja baseline` first")
    tmp = os.path.join(d.build, "report_regress.json")
    cmd = [os.path.join(ROOT, "build/tools/objdiff-cli"), "report", "generate",
           "-p", ROOT, "-o", tmp, "-f", "json"]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        raise SystemExit(r.stdout + r.stderr)
    r = subprocess.run(
        [os.path.join(ROOT, "build/tools/objdiff-cli"), "report", "changes",
         "--format", "json-pretty", d.baseline_path, tmp],
        cwd=ROOT, capture_output=True, text=True,
    )
    out({"returncode": r.returncode, "output": r.stdout + r.stderr},
        args.json, lambda x: print(x["output"]))
    if r.returncode != 0:
        sys.exit(1)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def build_parser(version_holder):
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--version", default=DEFAULT_VERSION)
    p.add_argument("--json", action="store_true", help="machine-readable output")
    p.add_argument("--root", action="store_true", help="run from repo root")
    sub = p.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("progress", help="overall progress")
    s.set_defaults(fn=cmd_progress)

    s = sub.add_parser("units", help="list units")
    s.add_argument("--unmatched", action="store_true")
    s.add_argument("--category")
    s.add_argument("--sort", choices=["bytes", "pct", "funcs"], default="bytes")
    s.add_argument("--ascending", action="store_true")
    s.add_argument("--limit", type=int, default=50)
    s.set_defaults(fn=cmd_units)

    s = sub.add_parser("funcs", help="list functions in a unit")
    s.add_argument("unit")
    s.add_argument("--unmatched", action="store_true")
    s.add_argument("--limit", type=int, default=200)
    s.set_defaults(fn=cmd_funcs)

    s = sub.add_parser("next", help="suggest functions to work on")
    s.add_argument("--n", type=int, default=20)
    s.add_argument("--mode", choices=["mixed", "finish", "sweep", "portable"],
                   default="mixed")
    s.set_defaults(fn=cmd_next)

    s = sub.add_parser("info", help="unit metadata")
    s.add_argument("unit")
    s.set_defaults(fn=cmd_info)

    s = sub.add_parser("asm", help="target (or ours) assembly for a symbol")
    s.add_argument("unit")
    s.add_argument("symbol")
    s.add_argument("--ours", action="store_true")
    s.set_defaults(fn=cmd_asm)

    s = sub.add_parser("symbols", help="search symbols and functions")
    s.add_argument("query")
    s.add_argument("--limit", type=int, default=100)
    s.set_defaults(fn=cmd_symbols)

    s = sub.add_parser("callers", help="find callers of a symbol (link map)")
    s.add_argument("symbol")
    s.set_defaults(fn=cmd_callgraph, direction="callers")

    s = sub.add_parser("callees", help="find callees of a symbol (link map)")
    s.add_argument("symbol")
    s.set_defaults(fn=cmd_callgraph, direction="callees")

    s = sub.add_parser("diff", help="compile one TU and diff symbols")
    s.add_argument("unit")
    s.add_argument("symbols", nargs="+")
    s.add_argument("--diff-text", action="store_true")
    s.set_defaults(fn=cmd_diff)

    s = sub.add_parser("diff-unit", help="compile one TU and summarize all diffs")
    s.add_argument("unit")
    s.add_argument("--worst", type=int, default=20)
    s.set_defaults(fn=cmd_diff_unit)

    s = sub.add_parser("check", help="compile one TU only")
    s.add_argument("unit")
    s.set_defaults(fn=cmd_check)

    s = sub.add_parser("context", help="decomp.me context for a unit")
    s.add_argument("unit")
    s.set_defaults(fn=cmd_context)

    s = sub.add_parser("port", help="search sibling decomp projects for a symbol")
    s.add_argument("query")
    s.add_argument("--refs", default="references")
    s.add_argument("--limit", type=int, default=30)
    s.set_defaults(fn=cmd_port)

    s = sub.add_parser("report", help="regenerate report.json")
    s.set_defaults(fn=cmd_report)

    s = sub.add_parser("regress", help="fail on match regressions vs baseline")
    s.set_defaults(fn=cmd_regress)
    return p


CURRENT_VERSION = DEFAULT_VERSION


def main():
    global CURRENT_VERSION
    args = build_parser(None).parse_args()
    CURRENT_VERSION = args.version
    if args.version == "Release_eu" and not os.path.isdir(
            os.path.join(ROOT, "build", args.version)):
        print("warning: Release_eu build dir missing, using MarioClub_us", file=sys.stderr)
        CURRENT_VERSION = args.version = DEFAULT_VERSION
    d = Data(args.version)
    args.fn(d, args)


if __name__ == "__main__":
    main()
