#!/usr/bin/env python3
"""Compile one translation unit to a temp object and diff functions.

Usage: python tools/fntry.py <src-file> <symbol> [symbol...]
       python tools/fntry.py <src-file>            # compile only (syntax check)

Prints MATCH/DIFF per symbol against the original (target) object.
Does not touch the shared build directory state (safe for parallel use).
"""
import os
import re
import shlex
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fndiff import disasm


def find_build(src):
    lines = open("build.ninja").read().splitlines()
    i = 0
    while i < len(lines):
        l = lines[i]
        if l.startswith("build "):
            start = i
            buf = l
            while buf.rstrip().endswith("$"):
                buf = buf.rstrip()[:-1] + " " + lines[i + 1].strip()
                i += 1
            head, rest = buf.split(":", 1)
            rtoks = rest.split()
            rule = rtoks[0]
            inputs = [t for t in rtoks[1:] if t not in ("|", "||")]
            if inputs and inputs[0] == src:
                vars_ = {}
                j = start + 1
                while j < len(lines) and lines[j - 1].rstrip().endswith("$"):
                    j += 1
                while j < len(lines) and lines[j].startswith("  "):
                    m = re.match(r"\s*(\w+)\s*=\s*(.*)", lines[j])
                    if m:
                        val = m.group(2)
                        while val.rstrip().endswith("$"):
                            j += 1
                            val = val.rstrip()[:-1] + " " + lines[j].strip()
                        vars_[m.group(1)] = val.strip()
                    j += 1
                return rule, vars_
        i += 1
    return None, None


def main():
    src = sys.argv[1]
    symbols = sys.argv[2:]
    rule, vars_ = find_build(src)
    if rule is None:
        print(f"no build rule for {src}", file=sys.stderr)
        sys.exit(1)
    pre = ["build/tools/wibo"]
    if rule in ("mwcc_sjis", "mwcc_sjis_extab"):
        pre.append("build/tools/sjiswrap.exe")
    cc = pre + [f"build/compilers/{vars_['mw_version']}/mwcceppc.exe"] + shlex.split(vars_["cflags"]) + ["-c", src, "-o"]
    # wait: sjiswrap needs the compiler as its argument
    fd, out = tempfile.mkstemp(suffix=".o", prefix="fntry_")
    os.close(fd)
    cmd = cc + [out]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.stdout.strip():
        print(r.stdout)
    if r.returncode != 0:
        print(r.stderr)
        print("COMPILE FAILED")
        os.unlink(out)
        sys.exit(1)
    # derive original object path
    unit = src
    for pfx in ("src/", "libs/"):
        if unit.startswith(pfx):
            unit = unit[len(pfx):]
    unit = re.sub(r"\.(cpp|c|cp|cc)$", "", unit)
    orig = f"build/MarioClub_us/obj/{unit}.o"
    if not os.path.exists(orig):
        print(f"orig object missing: {orig}", file=sys.stderr)
    for sym in symbols:
        try:
            a = disasm(out, sym)
            b = disasm(orig, sym)
        except SystemExit:
            print(f"{sym}: symbol missing")
            continue
        if a == b:
            print(f"{sym}: MATCH ({len(a)} instrs)")
        else:
            import difflib
            print(f"{sym}: DIFF ours={len(a)} orig={len(b)}")
            for line in difflib.unified_diff(b, a, "orig", "ours", lineterm="", n=3):
                print(line)
    os.unlink(out)


if __name__ == "__main__":
    main()
