#!/usr/bin/env python3
"""Syntax-check the patched decomp sources (build/web_wasm/tree) with clang for
wasm32 and summarize the errors.

    python tools/web_wasm/tools/scan.py [--filter SUBSTR] [--top N] [--files]
"""
import argparse, collections, concurrent.futures as cf, glob, os, re, subprocess

import patches

ROOT = patches.ROOT
TREE = patches.TREE  # decomp tree + tools/web_wasm/patches (patches.materialise)
FLAGS = [
    "--target=wasm32-unknown-emscripten", "-fsyntax-only", "-nostdinc", "-nostdinc++",
    "-std=gnu++03", "-fsigned-char", "-w", "-ferror-limit=0", "-fno-exceptions", "-fno-rtti",
    "-I", "tools/web_wasm/include", "-I", "build/web_wasm/tree/include", "-I", "build/web_wasm/tree/libs/PowerPC_EABI_Support/include",
    "-I", "build/MarioClub_us/include",
    "-DREGION_US=1", "-DDEBUG=1", "-DHIO_SCREENSHOT=1", "-DBUILD_VERSION=0", "-DVERSION_MarioClub_us",
    "-DMKDD_PORT=1", "-include", "tools/web_wasm/include/mkdd_port.h",
    "-fms-extensions", "-fms-compatibility", "-fdelayed-template-parsing", "-Wno-error=extraneous-template-head",
]

def sources():
    out = []
    for d in ("src", "libs/JSystem"):
        out += glob.glob(os.path.join(TREE, d, "**", "*.cpp"), recursive=True)
    return sorted(out)

def check(path):
    r = subprocess.run(["clang++", *FLAGS, path], cwd=ROOT, capture_output=True, text=True)
    errs = [l for l in r.stderr.splitlines() if ": error:" in l or ": fatal error:" in l]
    return path, errs

def norm(msg):
    msg = re.split(r": (?:fatal )?error: ", msg, maxsplit=1)[1]
    msg = re.sub(r"'[^']*'", "'X'", msg)
    return msg

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--filter", default="")
    ap.add_argument("--top", type=int, default=40)
    ap.add_argument("--files", action="store_true")
    a = ap.parse_args()
    patches.materialise()
    files = [f for f in sources() if a.filter in f]
    kinds, where, per_file = collections.Counter(), collections.Counter(), {}
    with cf.ThreadPoolExecutor(os.cpu_count()) as ex:
        for path, errs in ex.map(check, files):
            per_file[os.path.relpath(path, ROOT)] = errs
            for e in errs:
                kinds[norm(e)] += 1
                where[e.split(":", 1)[0]] += 1
    clean = sum(1 for e in per_file.values() if not e)
    print(f"{clean}/{len(per_file)} files clean, {sum(kinds.values())} errors")
    print("\n-- top error kinds --")
    for k, n in kinds.most_common(a.top):
        print(f"{n:6d}  {k}")
    print("\n-- top error locations (file) --")
    for k, n in where.most_common(a.top):
        print(f"{n:6d}  {k}")
    if a.files:
        for f, e in sorted(per_file.items(), key=lambda kv: len(kv[1])):
            print(f"{len(e):5d} {f}")

if __name__ == "__main__":
    main()
