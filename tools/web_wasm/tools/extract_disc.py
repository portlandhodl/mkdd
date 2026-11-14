#!/usr/bin/env python3
"""Extract a GameCube disc image (GCM/ISO) into a directory tree for the port.

    python port/tools/extract_disc.py orig/MarioClub_us/GM4E01.iso build/port/disc
Writes <out>/files/... (the disc filesystem) and <out>/sys/{boot.bin,bi2.bin,main.dol}.
"""
import os, struct, sys

def main(iso, out):
    with open(iso, "rb") as f:
        def rd(off, n):
            f.seek(off); return f.read(n)
        hdr = rd(0, 0x440)
        dol_off, fst_off, fst_size = struct.unpack(">III", hdr[0x420:0x42C])
        os.makedirs(os.path.join(out, "sys"), exist_ok=True)
        open(os.path.join(out, "sys", "boot.bin"), "wb").write(hdr)
        open(os.path.join(out, "sys", "bi2.bin"), "wb").write(rd(0x440, 0x2000))
        dh = rd(dol_off, 0x100)
        offs = struct.unpack(">18I", dh[0:0x48]); sizes = struct.unpack(">18I", dh[0x90:0xD8])
        open(os.path.join(out, "sys", "main.dol"), "wb").write(rd(dol_off, max(o + s for o, s in zip(offs, sizes))))
        fst = rd(fst_off, fst_size)
        n = struct.unpack(">I", fst[8:12])[0]
        names = fst[n * 12:]
        def name(i):
            o = struct.unpack(">I", fst[i * 12:i * 12 + 4])[0] & 0xFFFFFF
            return names[o:names.index(b"\0", o)].decode("shift_jis")
        stack = [(n, os.path.join(out, "files"))]
        os.makedirs(stack[0][1], exist_ok=True)
        count = 0
        for i in range(1, n):
            while i >= stack[-1][0]:
                stack.pop()
            w0, a, b = struct.unpack(">III", fst[i * 12:i * 12 + 12])
            path = os.path.join(stack[-1][1], name(i))
            if w0 >> 24:
                os.makedirs(path, exist_ok=True)
                stack.append((b, path))
            else:
                with open(path, "wb") as o:
                    f.seek(a); left = b
                    while left:
                        chunk = f.read(min(left, 1 << 20)); o.write(chunk); left -= len(chunk)
                count += 1
        print(f"extracted {count} files to {out}")

if __name__ == "__main__":
    main(*sys.argv[1:3])
