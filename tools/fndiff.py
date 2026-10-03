#!/usr/bin/env python3
"""Diff a single function between our compiled object and the orig object.

Usage: python tools/fndiff.py <objfile> <symbol> [orig-obj]
Compares normalized PPC disassembly of <symbol> in our object (src/...)
against the orig object (obj/... by default, derived from path).
"""
import subprocess
import sys
import re

OBJDUMP = "/opt/devkitpro/devkitPPC/bin/powerpc-eabi-objdump"


def disasm(objfile, symbol):
    out = subprocess.run(
        [OBJDUMP, "-dr", "-j", ".text", objfile],
        capture_output=True, text=True, check=True).stdout
    lines = out.splitlines()
    # find function start
    start = None
    for i, line in enumerate(lines):
        m = re.match(r"^([0-9a-f]+) <(.+)>:$", line)
        if m and m.group(2) == symbol:
            start = i
            break
    if start is None:
        print(f"symbol {symbol} not found in {objfile}", file=sys.stderr)
        sys.exit(1)
    end = len(lines)
    for i in range(start + 1, len(lines)):
        if re.match(r"^[0-9a-f]+ <.+>:$", lines[i]):
            end = i
            break
    body = lines[start + 1:end]
    # normalize: strip address prefix, keep mnemonic + operands
    norm = []
    for line in body:
        m = re.match(r"^\s*[0-9a-f]+:\s+((?:[0-9a-f]{2,8}\s+)+)\s*(\S.*?)\s*$", line)
        if not m:
            continue
        instr = m.group(2)
        # strip branch target addresses and symbol+offset annotations
        instr = re.sub(r"\s*[0-9a-f]+ <[^>]*>", "", instr)
        instr = re.sub(r"\s*<[^>]*>", "", instr)
        instr = re.sub(r"\b0x[0-9a-f]+\b", "", instr)
        norm.append(instr.strip())
    return norm


def main():
    ours = sys.argv[1]
    symbol = sys.argv[2]
    if len(sys.argv) > 3:
        orig = sys.argv[3]
    else:
        orig = ours.replace("/src/", "/obj/")
    a = disasm(ours, symbol)
    b = disasm(orig, symbol)
    if a == b:
        print(f"{symbol}: MATCH ({len(a)} instrs)")
        return
    import difflib
    print(f"{symbol}: DIFF ours={len(a)} orig={len(b)}")
    for line in difflib.unified_diff(b, a, "orig", "ours", lineterm="", n=3):
        print(line)


if __name__ == "__main__":
    main()
