#!/usr/bin/env python3
"""Strict per-function comparison of two PowerPC ELF relocatable objects.

The loose diff in fndiff.py compares normalised objdump text: it drops branch
targets, hex immediates and every relocation, so a function can "match" while
branching elsewhere, calling a different function, or loading a different
float literal. This module compares the raw instruction words instead (with
only the relocated bit-fields masked) and resolves every relocation:

- named symbols must have the same name and addend (``$1234`` / ``@class$12``
  numbering is ignored);
- anonymous data (``@1234`` literals, ``...rodata.0+0x10``, switch tables,
  string pools) is compared by the bytes it points at, recursively following
  relocations inside that data (jump-table entries resolve to
  ``function+offset``), so pool numbering and ordering do not matter;
- branches compare their effective target relative to the function start,
  whether it is encoded locally or through a relocation.

A reference that differs only in naming (an anonymous ``@123`` vs a named
local such as ``sDummy`` with identical bytes, a static that moved into an
anonymous namespace, a dtk auto-name like ``lbl_80001234``/``fn_80001234``)
is reported as a *note*, not an issue.

Usage: python tools/strictdiff.py <ours.o> <target.o> <symbol>...
"""

import os
import re
import struct
import sys

SHT_SYMTAB = 2
SHT_RELA = 4
SHT_NOBITS = 8
SHF_EXECINSTR = 4
STB_LOCAL = 0
STT_SECTION = 3
STT_FUNC = 2

R_NAMES = {
    1: "ADDR32", 2: "ADDR24", 3: "ADDR16", 4: "ADDR16_LO", 5: "ADDR16_HI",
    6: "ADDR16_HA", 7: "ADDR14", 10: "REL24", 11: "REL14", 26: "REL32",
    109: "EMB_SDA21",
}
R_MASK = {
    1: 0xFFFFFFFF, 2: 0x03FFFFFC, 3: 0xFFFF, 4: 0xFFFF, 5: 0xFFFF,
    6: 0xFFFF, 7: 0xFFFC, 8: 0xFFFC, 9: 0xFFFC, 10: 0x03FFFFFC,
    11: 0xFFFC, 12: 0xFFFC, 13: 0xFFFC, 26: 0xFFFFFFFF, 109: 0x001FFFFF,
}
R_BRANCH = (10, 11)

# Names that carry no meaning across compilations; compare what they refer to.
_ANON_RE = re.compile(
    r"^(@\d+|\.\.\.\w+\.\d+|@stringBase\d*|\$\$\d+|@etb_[0-9A-Fa-f]+"
    r"|@eti_[0-9A-Fa-f]+)$")
# dtk-generated placeholder names in the target: any name on our side is fine.
_AUTO_RE = re.compile(
    r"^(lbl_|fn_|func_|jumptable_|gap_|switch_|data_|bss_)[0-9A-Fa-f]{8}(_\w+)?$")
_LOCAL_SUFFIX_RE = re.compile(r"\$\d+$")
_CLASS_RE = re.compile(r"\d*@class\$\d+")


def norm_name(name):
    name = _CLASS_RE.sub("@class$", name)
    return _LOCAL_SUFFIX_RE.sub("", name)


class Elf:
    def __init__(self, path):
        with open(path, "rb") as f:
            data = f.read()
        if data[:4] != b"\x7fELF" or data[4] != 1:
            raise ValueError(f"not an ELF32 object: {path}")
        self.path = path
        self.data = data
        e = self.e = ">" if data[5] == 2 else "<"
        shoff, = struct.unpack_from(e + "I", data, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from(e + "HHH", data, 0x2E)
        self.sections = []
        for i in range(shnum):
            (name, typ, flags, _addr, off, size, link, info, _al,
             entsize) = struct.unpack_from(e + "10I", data, shoff + i * shentsize)
            self.sections.append({"name_off": name, "type": typ,
                                  "flags": flags, "offset": off, "size": size,
                                  "link": link, "info": info,
                                  "entsize": entsize})
        strsec = self.sections[shstrndx]
        for s in self.sections:
            s["name"] = self._str(strsec, s["name_off"])
        self.symbols = []
        for s in self.sections:
            if s["type"] != SHT_SYMTAB:
                continue
            strtab = self.sections[s["link"]]
            for j in range(s["size"] // 16):
                nm, val, sz, info, _oth, shndx = struct.unpack_from(
                    e + "IIIBBH", data, s["offset"] + j * 16)
                self.symbols.append({
                    "name": self._str(strtab, nm), "value": val, "size": sz,
                    "bind": info >> 4, "type": info & 0xF, "shndx": shndx,
                })
            break
        # target section index -> {word offset: (type, sym index, addend)}
        self.relocs = {}
        for s in self.sections:
            if s["type"] != SHT_RELA:
                continue
            tbl = self.relocs.setdefault(s["info"], {})
            for j in range(s["size"] // 12):
                off, info, add = struct.unpack_from(
                    e + "IIi", data, s["offset"] + j * 12)
                # 16-bit field relocations sit at insn+2 (mwcc) or insn+0
                # (dtk); key every relocation by its word.
                tbl[off & ~3] = (info & 0xFF, info >> 8, add)
        self.by_section = {}
        for sym in self.symbols:
            if 0 < sym["shndx"] < len(self.sections) and sym["type"] != STT_SECTION:
                self.by_section.setdefault(sym["shndx"], []).append(sym)
        for lst in self.by_section.values():
            lst.sort(key=lambda s: (s["value"], -s["size"]))

    def _str(self, sec, off):
        base = sec["offset"] + off
        end = self.data.find(b"\0", base)
        return self.data[base:end].decode("latin-1")

    def defined(self, sym):
        return 0 < sym["shndx"] < len(self.sections)

    def section_bytes(self, shndx, off, size):
        s = self.sections[shndx]
        if s["type"] == SHT_NOBITS:
            return None
        return self.data[s["offset"] + off:s["offset"] + off + size]

    def word(self, shndx, off):
        return struct.unpack_from(self.e + "I", self.section_bytes(shndx, off, 4))[0]

    def func(self, name):
        cands = [s for s in self.symbols if s["name"] == name
                 and self.defined(s)
                 and self.sections[s["shndx"]]["flags"] & SHF_EXECINSTR]
        if not cands:
            return None
        return max(cands, key=lambda s: s["size"])

    def covering(self, shndx, addr, want_func=False):
        """Innermost symbol covering addr, preferring sized and named ones."""
        best = None
        for s in self.by_section.get(shndx, []):
            if s["value"] > addr:
                break
            if want_func and s["type"] != STT_FUNC:
                continue
            if addr < s["value"] + s["size"] or (s["size"] == 0 and s["value"] == addr):
                key = (s["value"], s["size"] > 0, -s["size"], not is_anon(s))
                if best is None or key > best[0]:
                    best = (key, s)
        return best[1] if best else None

    def extent(self, shndx, addr):
        """(end, exact) of the data object starting at/containing addr.

        exact is True when a sized symbol starts exactly at addr."""
        cov = self.covering(shndx, addr)
        if cov is not None and cov["size"] > 0:
            return cov["value"] + cov["size"], cov["value"] == addr
        nxt = [s["value"] for s in self.by_section.get(shndx, [])
               if s["value"] > addr]
        return (min(nxt) if nxt else self.sections[shndx]["size"]), False


_ELF_CACHE = {}


def load(path):
    st = os.stat(path)
    key = (path, st.st_mtime_ns, st.st_size)
    e = _ELF_CACHE.get(key)
    if e is None:
        if len(_ELF_CACHE) > 16:
            _ELF_CACHE.clear()
        e = _ELF_CACHE[key] = Elf(path)
    return e


def is_anon(sym):
    return (sym["type"] == STT_SECTION or not sym["name"]
            or bool(_ANON_RE.match(sym["name"])))


def _printable(b):
    return all(32 <= c < 127 or c in (9, 10, 13) for c in b)


def fmt_bytes(b, sec_name=""):
    if b is None:
        return "(bss)"
    if len(b) == 4 and ("data2" in sec_name or "rodata" in sec_name):
        f, = struct.unpack(">f", b)
        return f"{f!r} (0x{b.hex()})"
    if len(b) == 8 and ("data2" in sec_name or "rodata" in sec_name):
        d, = struct.unpack(">d", b)
        return f"{d!r} (0x{b.hex()})"
    s = b.split(b"\0", 1)[0]
    if len(s) >= 2 and _printable(s) and all(c == 0 for c in b[len(s):]):
        t = s.decode("ascii")
        return repr(t if len(t) <= 48 else t[:48] + "...")
    h = b.hex()
    return "0x" + (h if len(h) <= 32 else h[:32] + f"...({len(b)} bytes)")


class Ref:
    """A resolved relocation target."""
    __slots__ = ("kind", "name", "add", "shndx", "addr", "sec", "auto",
                 "label", "glob")

    def __init__(self, kind, name=None, add=0, shndx=None, addr=None,
                 sec=None, auto=False, label=""):
        # kind: 'name' (undefined or code symbol), 'obj' (defined data
        # object; name None when anonymous), 'bss' (defined nobits)
        self.kind, self.name, self.add = kind, name, add
        self.shndx, self.addr, self.sec = shndx, addr, sec
        self.auto, self.label = auto, label
        self.glob = False

    def desc(self):
        if self.name:
            return norm_name(self.name) + (f"+0x{self.add:x}" if self.add else "")
        return self.label or f"anonymous {self.sec}"


def _sda_base(sec):
    if sec in (".sdata2", ".sbss2"):
        return "r2"
    if sec in (".sdata", ".sbss"):
        return "r13"
    return sec.lstrip(".").split(".")[0].replace("bss", "data")


def _base_sec(name):
    return re.sub(r"\.\d+$", "", name)


def resolve(elf, sym_idx, add):
    sym = elf.symbols[sym_idx]
    label = (sym["name"] or elf.sections[sym["shndx"]]["name"]
             if elf.defined(sym) else sym["name"])
    label += f"+0x{add:x}" if add else ""
    if not elf.defined(sym):
        return Ref("name", sym["name"], add, label=label,
                   auto=bool(_AUTO_RE.match(sym["name"])))
    shndx = sym["shndx"]
    sec = elf.sections[shndx]
    addr = sym["value"] + add
    if sec["flags"] & SHF_EXECINSTR:
        f = sym if (sym["type"] == STT_FUNC and not is_anon(sym)
                    and sym["value"] <= addr < sym["value"] + max(sym["size"], 1)) \
            else elf.covering(shndx, addr, want_func=True)
        if f is not None:
            return Ref("name", f["name"], addr - f["value"], shndx, addr,
                       auto=bool(_AUTO_RE.match(f["name"])), label=label)
        return Ref("name", None, addr, shndx, addr, sec=sec["name"], label=label)
    named = sym if not is_anon(sym) else None
    off = add
    if named is None:
        cov = elf.covering(shndx, addr)
        if cov is not None and not is_anon(cov):
            named, off = cov, addr - cov["value"]
    kind = "bss" if sec["type"] == SHT_NOBITS else "obj"
    if named is not None:
        r = Ref(kind, named["name"], off, shndx, addr, _base_sec(sec["name"]),
                auto=bool(_AUTO_RE.match(named["name"])), label=label)
        r.glob = named["bind"] != STB_LOCAL
        return r
    return Ref(kind, None, 0, shndx, addr, _base_sec(sec["name"]), label=label)


class Comparer:
    def __init__(self, ours, target):
        self.o = load(ours)
        self.t = load(target)
        self._memo = {}

    # -- data ---------------------------------------------------------------

    def data_diff(self, ro, rt, depth=0):
        """Compare the bytes of two defined data references. None if equal."""
        key = (ro.shndx, ro.addr, rt.shndx, rt.addr)
        if key in self._memo:
            return self._memo[key]
        self._memo[key] = None  # cycle guard
        r = self._data_diff(ro, rt, depth)
        self._memo[key] = r
        return r

    def _data_diff(self, ro, rt, depth):
        if ro.sec != rt.sec:
            return f"data in {rt.sec} vs ours {ro.sec}"
        if ro.kind == "bss" or rt.kind == "bss":
            if ro.kind != rt.kind:
                return "bss vs initialised data"
            oe, ox = self.o.extent(ro.shndx, ro.addr)
            te, tx = self.t.extent(rt.shndx, rt.addr)
            if ox and tx and oe - ro.addr != te - rt.addr:
                return f"size {te - rt.addr:#x} vs ours {oe - ro.addr:#x}"
            return None
        oe, ox = self.o.extent(ro.shndx, ro.addr)
        te, tx = self.t.extent(rt.shndx, rt.addr)
        no, nt = oe - ro.addr, te - rt.addr
        n = min(no, nt)
        ob = bytearray(self.o.section_bytes(ro.shndx, ro.addr, max(n, 0)) or b"")
        tb = bytearray(self.t.section_bytes(rt.shndx, rt.addr, max(n, 0)) or b"")
        orel = self.o.relocs.get(ro.shndx, {})
        trel = self.t.relocs.get(rt.shndx, {})
        oms = {off - ro.addr: v for off, v in orel.items()
               if ro.addr <= off < ro.addr + n}
        tms = {off - rt.addr: v for off, v in trel.items()
               if rt.addr <= off < rt.addr + n}
        for off, (typ, _, _) in list(oms.items()) + list(tms.items()):
            if off + 4 <= n:
                m = R_MASK.get(typ, 0xFFFFFFFF)
                for b_ in (ob, tb):
                    w, = struct.unpack_from(">I", b_, off)
                    struct.pack_into(">I", b_, off, w & ~m & 0xFFFFFFFF)
        if ob != tb or (ox and tx and no != nt):
            to = self.t.section_bytes(rt.shndx, rt.addr, nt if tx else n)
            oo = self.o.section_bytes(ro.shndx, ro.addr, no if ox else n)
            return f"{fmt_bytes(to, rt.sec)} vs ours {fmt_bytes(oo, ro.sec)}"
        if set(oms) != set(tms):
            return (f"pointer layout differs (target relocs at "
                    f"{[hex(x) for x in sorted(tms)[:6]]} vs ours "
                    f"{[hex(x) for x in sorted(oms)[:6]]})")
        for off in sorted(oms):
            ot, osym, oadd = oms[off]
            tt, tsym, tadd = tms[off]
            if ot != tt:
                return f"+0x{off:x}: relocation type differs"
            r = self.ref_diff(resolve(self.o, osym, oadd),
                              resolve(self.t, tsym, tadd), depth + 1)
            if r and r[0] == "issue":
                return f"[+0x{off:x}] {r[1]}"
        return None

    def ref_diff(self, ro, rt, depth=0):
        """Compare two resolved references.

        Returns None (same), ('note', msg) for naming-only differences, or
        ('issue', msg)."""
        same_name = (ro.name is not None and rt.name is not None
                     and norm_name(ro.name) == norm_name(rt.name))
        if same_name and ro.add == rt.add:
            if ro.kind != "name" and rt.kind != "name" and ro.sec != rt.sec:
                msg = f"{rt.desc()} in {rt.sec} vs ours {ro.sec}"
                if _sda_base(ro.sec) != _sda_base(rt.sec):
                    return ("issue", msg)
                return ("note", msg)
            return None
        if rt.auto and ro.name is not None and ro.add == rt.add:
            return ("note", f"{rt.name} is ours {ro.name}")
        if ro.kind == "name" or rt.kind == "name":
            if ro.kind != rt.kind:
                return ("issue", f"{rt.desc()} vs ours {ro.desc()}")
            if ro.name is None or rt.name is None:
                if ro.name is None and rt.name is None and ro.add == rt.add:
                    return None
                return ("issue", f"{rt.desc()} vs ours {ro.desc()}")
            if same_name:
                if ro.add == rt.add:
                    return None
                return ("issue", f"{rt.desc()} vs ours {ro.desc()}")
            if rt.auto and ro.add == rt.add:
                return ("note", f"{rt.name} is ours {ro.name}")
            return ("issue", f"{rt.desc()} vs ours {ro.desc()}")
        # both defined data (obj/bss)
        if same_name and ro.add == rt.add:
            # Named objects are data-matched on their own (objdiff); only
            # anonymous ones are this function's concern.
            return None
        if depth > 6:
            return None
        why = self.data_diff(ro, rt, depth)
        if why:
            if ro.name is None and rt.name is None:
                return ("issue", f"{rt.desc()}: {why}")
            return ("issue", f"{rt.desc()} vs ours {ro.desc()}: {why}")
        if ro.name is None and rt.name is None:
            return None
        if rt.name is not None and rt.glob and not rt.auto:
            return ("issue", f"{rt.desc()} vs ours {ro.desc()} (same bytes)")
        return ("note", f"{rt.desc()} is ours {ro.desc()} (same bytes)")

    # -- functions ----------------------------------------------------------

    def _branch_dest(self, elf, func, shndx, i, word, rel):
        """Effective branch destination: ('self', off) or a Ref."""
        if rel is not None and rel[0] in R_BRANCH:
            r = resolve(elf, rel[1], rel[2])
            if r.shndx == shndx and r.addr is not None and \
                    func["value"] <= r.addr < func["value"] + func["size"]:
                return ("self", r.addr - func["value"])
            return r
        op = word >> 26
        if op == 18:
            disp = _sext(word & 0x03FFFFFC, 26)
        else:
            disp = _sext(word & 0xFFFC, 16)
        if word & 2:  # absolute
            return ("abs", disp)
        return ("self", i * 4 + disp)

    def _track_pool(self, i, w, ro, rt, bases, issues, add):
        """Follow pool-base registers and compare strings reached by offset.

        Only strings are checked (printable, NUL-terminated in the target):
        they are what pools are addressed by offset for (assert file names,
        format strings), and a mis-tracked register can then at worst compare
        two equal byte runs."""
        op = w >> 26
        rd, ra = (w >> 21) & 31, (w >> 16) & 31
        if op == 14 and ro is not None and rt is not None and \
                ro[0] == rt[0] == 4:  # addi rD, rA, sym@l
            refo = resolve(self.o, ro[1], ro[2])
            reft = resolve(self.t, rt[1], rt[2])
            if refo.kind == reft.kind == "obj" and refo.addr is not None \
                    and reft.addr is not None:
                bases[rd] = (refo, reft)
            else:
                bases.pop(rd, None)
            return
        if op == 14 and ro is None and rt is None and ra in bases and ra != 0:
            imm = _sext(w & 0xFFFF, 16)
            if imm:
                bo, bt = bases[ra]
                tb = self.t.section_bytes(bt.shndx, bt.addr + imm, 256) or b""
                end = tb.find(b"\0")
                if end >= 2 and all(32 <= c < 127 or c in (9, 10)
                                    for c in tb[:end]):
                    ob = self.o.section_bytes(bo.shndx, bo.addr + imm,
                                              end + 1) or b""
                    if ob != tb[:end + 1]:
                        msg = (f"string at pool+0x{imm:x}: "
                               f"{tb[:end].decode('latin-1')!r} vs ours "
                               f"{ob.split(b'\0')[0].decode('latin-1')!r}")
                        add(issues, "literal", i, msg, dedupe=msg)
            if rd != ra:
                bases.pop(rd, None)
            return
        if op == 18 and w & 1:  # bl: volatile registers are clobbered
            for r in range(0, 13):
                bases.pop(r, None)
            return
        if op in (16, 18, 19):
            return  # branches (bctrl/blrl clobber via the bl rule only)
        if 36 <= op <= 45 or op == 47 or op in (52, 53, 54, 55):
            return  # stores
        if op in (48, 49, 50, 51, 59, 63):
            if op in (49, 51):  # lfsu/lfdu update rA
                bases.pop(ra, None)
            return  # FPR destination
        bases.pop(rd, None)
        if op in (24, 25, 26, 27, 28, 29, 21, 20, 23, 31, 30):
            bases.pop(ra, None)  # logical/rotate ops write rA

    def compare(self, symbol, loose_lines=None):
        """Return (issues, notes): lists of {kind, offset, message}."""
        fo = self.o.func(symbol)
        ft = self.t.func(symbol)
        if fo is None or ft is None:
            return [{"kind": "missing", "offset": 0,
                     "message": "symbol missing in "
                     + ("ours" if fo is None else "target")}], []
        so, st_ = fo["shndx"], ft["shndx"]
        no = _trim(self.o, so, fo["value"], fo["size"])
        nt = _trim(self.t, st_, ft["value"], ft["size"])
        issues, notes = [], []
        seen = set()

        def add(lst, kind, i, msg, dedupe=None):
            if dedupe is not None:
                if dedupe in seen:
                    return
                seen.add(dedupe)
            text = loose_lines[i] if loose_lines and i < len(loose_lines) else ""
            lst.append({"kind": kind, "offset": i * 4,
                        "message": f"+0x{i * 4:x} {text}: {msg}".replace(" :", ":")})

        if no != nt:
            issues.append({"kind": "size", "offset": min(no, nt) * 4,
                           "message": f"size {nt * 4:#x} vs ours {no * 4:#x}"})
        orel = self.o.relocs.get(so, {})
        trel = self.t.relocs.get(st_, {})
        # GPR -> (ours Ref, target Ref): registers holding the address of an
        # anonymous data pool (``addi rD, rA, pool@l``). Later ``addi rX,
        # rD, imm`` reach a string inside the pool by a plain immediate, so
        # the relocation alone only proves the pool *base* matches.
        bases = {}
        for i in range(min(no, nt)):
            wo = self.o.word(so, fo["value"] + i * 4)
            wt = self.t.word(st_, ft["value"] + i * 4)
            ro = orel.get(fo["value"] + i * 4)
            rt = trel.get(ft["value"] + i * 4)
            op = wt >> 26
            if wo == wt:
                self._track_pool(i, wt, ro, rt, bases, issues, add)
            else:
                bases.clear()
            if op in (16, 18) and op == wo >> 26 and \
                    ((ro is None or ro[0] in R_BRANCH)
                     and (rt is None or rt[0] in R_BRANCH)):
                fmask = 0x03FFFFFC if op == 18 else 0xFFFC
                if (wo & ~fmask) != (wt & ~fmask):
                    add(issues, "immediate", i, f"word {wt:08x} vs ours {wo:08x}")
                    continue
                do = self._branch_dest(self.o, fo, so, i, wo, ro)
                dt = self._branch_dest(self.t, ft, st_, i, wt, rt)
                if isinstance(do, tuple) or isinstance(dt, tuple):
                    if do != dt:
                        add(issues, "branch", i, "branch target "
                            f"{_bdesc(dt)} vs ours {_bdesc(do)}")
                    continue
                r = self.ref_diff(do, dt)
                if r:
                    add(issues if r[0] == "issue" else notes,
                        "call" if r[0] == "issue" else "name", i, r[1],
                        dedupe=r[1])
                continue
            if (ro is None) != (rt is None) or (ro and ro[0] != rt[0]):
                add(issues, "reloc", i, "relocation "
                    f"{_rdesc(rt, self.t)} vs ours {_rdesc(ro, self.o)}")
                continue
            mask = R_MASK.get(ro[0], 0xFFFFFFFF) if ro else 0
            if (wo & ~mask) != (wt & ~mask):
                add(issues, "immediate", i, f"word {wt:08x} vs ours {wo:08x}")
                continue
            if ro is None:
                continue
            refo = resolve(self.o, ro[1], ro[2])
            reft = resolve(self.t, rt[1], rt[2])
            r = self.ref_diff(refo, reft)
            if not r:
                continue
            if r[0] == "note":
                add(notes, "name", i, r[1], dedupe=r[1])
                continue
            kind = "reloc"
            if reft.kind == "obj" and refo.kind == "obj":
                kind = "data"
                if reft.sec in (".sdata2", ".rodata", ".sbss2") or \
                        reft.sec.startswith(".rodata"):
                    kind = "literal"
                if _is_jumptable(self.t, reft, symbol):
                    kind = "jumptable"
            add(issues, kind, i, r[1], dedupe=r[1])
        return issues, notes


def _is_jumptable(elf, ref, func_name):
    """A table whose first pointer targets the referencing function."""
    if ref.shndx is None or ref.addr is None:
        return False
    end, _ = elf.extent(ref.shndx, ref.addr)
    rel = elf.relocs.get(ref.shndx, {})
    offs = sorted(o for o in rel if ref.addr <= o < end)
    if not offs:
        return False
    t, s, a = rel[offs[0]]
    r = resolve(elf, s, a)
    return r.kind == "name" and r.name == func_name


def _trim(elf, shndx, start, size):
    n = size // 4
    rel = elf.relocs.get(shndx, {})
    while n > 0 and elf.word(shndx, start + (n - 1) * 4) == 0 \
            and (start + (n - 1) * 4) not in rel:
        n -= 1
    return n


def _bdesc(d):
    if isinstance(d, tuple):
        return f"+0x{d[1]:x}" if d[0] == "self" else f"abs 0x{d[1]:x}"
    return d.desc()


def _rdesc(r, elf):
    if r is None:
        return "none"
    return f"{R_NAMES.get(r[0], r[0])} {resolve(elf, r[1], r[2]).label}"


def _sext(v, bits):
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


_COMPARERS = {}


def comparer(ours, target):
    """Cached Comparer for an (ours, target) pair; reloads changed files."""
    key = (ours, target)
    c = _COMPARERS.get(key)
    if c is None or c.o is not load(ours) or c.t is not load(target):
        c = _COMPARERS[key] = Comparer(ours, target)
    return c


def compare_symbol(ours, target, symbol, loose_lines=None):
    """(issues, notes) for symbol; issues empty means a strict match."""
    return comparer(ours, target).compare(symbol, loose_lines)


def summarize(issues, limit=3):
    """One-line reason for a strict-only failure."""
    parts = []
    for i in issues[:limit]:
        parts.append(f"[{i['kind']}] {i['message']}")
    more = len(issues) - limit
    if more > 0:
        parts.append(f"(+{more} more)")
    return "; ".join(parts)


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        sys.exit(2)
    ours, target = sys.argv[1], sys.argv[2]
    bad = 0
    for sym in sys.argv[3:]:
        issues, notes = compare_symbol(ours, target, sym)
        for n in notes:
            print(f"{sym}: note {n['message']}")
        if not issues:
            print(f"{sym}: STRICT MATCH")
            continue
        bad += 1
        print(f"{sym}: {len(issues)} strict issue(s)")
        for i in issues[:20]:
            print(f"  [{i['kind']}] {i['message']}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
