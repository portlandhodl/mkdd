#!/usr/bin/env python3
"""Compile decomp sources to wasm32 objects and list external symbols they need.

    python tools/web_wasm/tools/surface.py [--jobs N]
Writes build/web_wasm/obj/... and prints undefined symbols grouped by prefix.
"""
import argparse, collections, concurrent.futures as cf, os, re, shutil, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import scan  # noqa: E402

ROOT = scan.ROOT
OBJ = os.path.join(ROOT, "build", "web_wasm", "obj")
NM = shutil.which("llvm-nm") or shutil.which("llvm-nm-21")

def compile_one(src):
    rel = os.path.relpath(src, ROOT)
    out = os.path.join(OBJ, rel + ".o")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    flags = [f for f in scan.FLAGS if f != "-fsyntax-only"]
    r = subprocess.run(["clang++", *flags, "-c", "-O1", "-o", out, src], cwd=ROOT, capture_output=True, text=True)
    return rel, out if r.returncode == 0 else None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    a = ap.parse_args()
    scan.patches.materialise()
    objs, failed = [], []
    with cf.ThreadPoolExecutor(a.jobs) as ex:
        for rel, out in ex.map(compile_one, scan.sources()):
            (objs if out else failed).append(out or rel)
    defined, undefined = set(), collections.Counter()
    for o in objs:
        r = subprocess.run([NM, "--defined-only", "-j", o], capture_output=True, text=True)
        defined.update(r.stdout.split())
        r = subprocess.run([NM, "--undefined-only", "-j", o], capture_output=True, text=True)
        undefined.update(r.stdout.split())
    need = {s: n for s, n in undefined.items() if s not in defined}
    groups = collections.defaultdict(list)
    for s in need:
        m = re.match(r"_?_?([A-Z][A-Za-z]{1,5}?)(?=[A-Z_])", s.lstrip("_"))
        groups[m.group(1) if m else s[:6]].append(s)
    print(f"{len(objs)} objects compiled, {len(failed)} failed; {len(need)} external symbols")
    for g, syms in sorted(groups.items(), key=lambda kv: -len(kv[1])):
        print(f"{len(syms):4d} {g}: {' '.join(sorted(syms)[:12])}{' ...' if len(syms) > 12 else ''}")
    with open(os.path.join(ROOT, "build", "web_wasm", "undefined.txt"), "w") as f:
        f.write("\n".join(sorted(need)) + "\n")

if __name__ == "__main__":
    main()
