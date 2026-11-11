#!/usr/bin/env python3
"""Diff a single function between our compiled object and the orig object.

Usage: python tools/fndiff.py <objfile> <symbol> [orig-obj]
Compares normalized PPC disassembly of <symbol> in our object (src/...)
against the orig object (obj/... by default, derived from path).
"""
import os
import re
import struct
import subprocess
import sys
import tempfile

OBJDUMP = "/opt/devkitpro/devkitPPC/bin/powerpc-eabi-objdump"

SHT_PROGBITS = 1
SHF_EXECINSTR = 4


def _uniquify_code_sections(objfile):
    """Return a temp copy of objfile whose code sections have unique names.

    MWCC emits one section per weak function group, all named .text at VMA 0.
    objdump conflates same-named sections and attaches labels from the wrong
    one, so symbols outside the first .text vanish. Renaming each duplicate
    code section to .text.<index> fixes symbol lookup without moving
    addresses. Returns None when the object has at most one code section per
    name (the ordinary case, left untouched).
    """
    with open(objfile, "rb") as f:
        data = bytearray(f.read())
    if data[:4] != b"\x7fELF" or data[4] != 1:
        return None
    e = ">" if data[5] == 2 else "<"
    shoff, = struct.unpack_from(e + "I", data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from(e + "HHH", data, 0x2E)
    if not shoff or not shnum or shstrndx >= shnum:
        return None
    hdrs = [list(struct.unpack_from(e + "10I", data, shoff + i * shentsize))
            for i in range(shnum)]
    str_off, str_size = hdrs[shstrndx][4], hdrs[shstrndx][5]
    strtab = bytes(data[str_off:str_off + str_size])

    def name(h):
        end = strtab.find(b"\0", h[0])
        return strtab[h[0]:end if end >= 0 else len(strtab)]

    code = [i for i, h in enumerate(hdrs)
            if h[1] == SHT_PROGBITS and h[2] & SHF_EXECINSTR]
    names = [name(hdrs[i]) for i in code]
    if len(set(names)) == len(names):
        return None
    for i in code:
        h = hdrs[i]
        n = name(h)
        h[0] = len(strtab)
        strtab += n + b"." + str(i).encode() + b"\0"
    hdrs[shstrndx][4] = len(data)
    hdrs[shstrndx][5] = len(strtab)
    data += strtab
    for i, h in enumerate(hdrs):
        struct.pack_into(e + "10I", data, shoff + i * shentsize, *h)
    fd, tmp = tempfile.mkstemp(suffix=".o", prefix="fndiff_")
    with os.fdopen(fd, "wb") as f:
        f.write(data)
    return tmp


def objdump_code(objfile, check=True):
    """objdump -dr of every code section, as a list of lines."""
    tmp = _uniquify_code_sections(objfile)
    try:
        if tmp is None:
            cmd = [OBJDUMP, "-dr", "-j", ".text", objfile]
        else:
            cmd = [OBJDUMP, "-dr", tmp]
        out = subprocess.run(cmd, capture_output=True, text=True,
                             check=check).stdout
    finally:
        if tmp is not None:
            os.unlink(tmp)
    return out.splitlines()


def function_bounds(lines, symbol):
    """(start, end) line indices of symbol's block, or None if absent."""
    start = None
    for i, line in enumerate(lines):
        m = re.match(r"^([0-9a-f]+) <(.+)>:$", line)
        if m and m.group(2) == symbol:
            start = i
            break
    if start is None:
        return None
    end = len(lines)
    for i in range(start + 1, len(lines)):
        if (re.match(r"^[0-9a-f]+ <.+>:$", lines[i])
                or lines[i].startswith("Disassembly of section ")):
            end = i
            break
    return start, end


def disasm(objfile, symbol):
    lines = objdump_code(objfile)
    bounds = function_bounds(lines, symbol)
    if bounds is None:
        print(f"symbol {symbol} not found in {objfile}", file=sys.stderr)
        sys.exit(1)
    start, end = bounds
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
