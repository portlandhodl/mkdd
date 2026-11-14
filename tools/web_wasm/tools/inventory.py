#!/usr/bin/env python3
"""Inventory file formats on an extracted disc, looking inside RARC archives.

    python tools/web_wasm/tools/inventory.py build/web_wasm/disc/files [--list EXT]
Prints counts per extension with first-4-byte magics and total size.
"""
import collections, os, struct, sys

def yaz0(src):
    size = struct.unpack(">I", src[4:8])[0]
    dst = bytearray(); i = 16
    while len(dst) < size:
        flags = src[i]; i += 1
        for b in range(8):
            if len(dst) >= size: break
            if flags & (0x80 >> b):
                dst.append(src[i]); i += 1
            else:
                b1, b2 = src[i], src[i + 1]; i += 2
                dist = ((b1 & 0xF) << 8 | b2) + 1
                n = b1 >> 4
                if n == 0: n = src[i] + 0x12; i += 1
                else: n += 2
                for _ in range(n): dst.append(dst[-dist])
    return bytes(dst)

def rarc_files(data):
    hl, fdo = struct.unpack(">II", data[8:16])
    info = hl
    nn, no, nf, fo, sl, so = struct.unpack(">IIIIII", data[info:info + 24])
    base = info
    strs = data[base + so:base + so + sl]
    for i in range(nf):
        e = base + fo + i * 20
        fid, h, flag, doff, size = struct.unpack(">HHIII", data[e:e + 16])
        name_off = flag & 0xFFFFFF; attr = flag >> 24
        name = strs[name_off:strs.index(b"\0", name_off)].decode("shift_jis", "replace")
        if attr & 2 or name in (".", ".."): continue
        start = hl + fdo + doff
        yield name, data[start:start + size]

def walk(name, data, stats, listing, path):
    if data[:4] == b"Yaz0":
        data = yaz0(data)
    ext = os.path.splitext(name)[1].lower() or "(none)"
    if data[:4] == b"RARC":
        for n, d in rarc_files(data):
            walk(n, d, stats, listing, path + "/" + n)
        ext = ext + "[rarc]"
    s = stats[ext]; s[0] += 1; s[1] += len(data); s[2][data[:4]] += 1
    listing[ext].append(path)

def main():
    root = sys.argv[1]
    want = sys.argv[3].lower() if len(sys.argv) > 3 and sys.argv[2] == "--list" else None
    stats = collections.defaultdict(lambda: [0, 0, collections.Counter()])
    listing = collections.defaultdict(list)
    for dp, _, fs in os.walk(root):
        for f in fs:
            p = os.path.join(dp, f)
            walk(f, open(p, "rb").read(), stats, listing, os.path.relpath(p, root))
    if want:
        print("\n".join(listing[want])); return
    for ext, (n, size, magics) in sorted(stats.items(), key=lambda kv: -kv[1][0]):
        print(f"{ext:14} {n:5d} {size/1e6:8.2f}MB  " + " ".join(f"{m!r}x{c}" for m, c in magics.most_common(4)))

if __name__ == "__main__":
    main()
