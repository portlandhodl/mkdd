#!/usr/bin/env python3
"""Host patch set for the WebAssembly port.

The decomp tree (src/, libs/, include/) holds only matching-decomp code. Every
change the host build needs lives in tools/web_wasm/patches/*.patch (unified
diffs, -p1 from the repository root, applied in the order of `series`). Each
decomp file is owned by at most one patch. The build compiles a materialised
copy, build/web_wasm/tree/{src,libs,include} = decomp tree + patches.

    python tools/web_wasm/tools/patches.py apply [--force]   materialise the tree
    python tools/web_wasm/tools/patches.py check             patches apply cleanly?
    python tools/web_wasm/tools/patches.py list              patch -> files
    python tools/web_wasm/tools/patches.py new NAME FILE...  start a patch owning FILEs
    python tools/web_wasm/tools/patches.py refresh PATCH...  regenerate from tree edits

Workflow: `apply`, edit the owned files under build/web_wasm/tree/, then
`refresh <patch>` (use `new` first for files no patch owns yet). `apply`
refuses to overwrite tree files edited since it last wrote them.
"""
import argparse, difflib, json, os, re, shutil, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
PATCH_DIR = os.path.join(ROOT, "tools", "web_wasm", "patches")
SERIES = os.path.join(PATCH_DIR, "series")
TREE = os.path.join(ROOT, "build", "web_wasm", "tree")
MANIFEST = os.path.join(TREE, ".manifest.json")
DECOMP_DIRS = ("src", "libs", "include")
ENC = "latin-1"  # byte-transparent; sources mix ASCII, UTF-8 and Shift-JIS


class PatchError(Exception):
    pass


class Patch:
    def __init__(self, name):
        self.name = name
        self.header = ""
        self.files = {}  # rel path -> list of hunks; hunk = (old_start, old_lines, new_lines)
        self.new_files = set()

    @property
    def path(self):
        return os.path.join(PATCH_DIR, self.name)


def read_text(path):
    with open(path, encoding=ENC, newline="") as f:
        return f.read()


def write_text(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding=ENC, newline="") as f:
        f.write(text)


def strip_prefix(p):
    p = p.split("\t", 1)[0].strip()
    if p == "/dev/null":
        return None
    if not p.startswith(("a/", "b/")):
        raise ValueError(f"path without a/ or b/ prefix: {p}")
    return p[2:]


HUNK_RE = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")


def parse_patch(name):
    patch = Patch(name)
    lines = read_text(os.path.join(PATCH_DIR, name)).splitlines(True)
    i, n, header = 0, len(lines), []
    while i < n and not (lines[i].startswith("--- ") and i + 1 < n and lines[i + 1].startswith("+++ ")):
        if not lines[i].startswith(("diff --git ", "index ")):
            header.append(lines[i])
        i += 1
    patch.header = "".join(header)
    while i < n:
        line = lines[i]
        if line.startswith(("diff ", "index ", "new file mode", "deleted file mode")):
            i += 1
            continue
        if not line.startswith("--- "):
            raise PatchError(f"{name}:{i + 1}: expected '--- ', got {line.rstrip()!r}")
        try:
            old, new = strip_prefix(line[4:]), strip_prefix(lines[i + 1][4:])
        except ValueError as e:
            raise PatchError(f"{name}:{i + 1}: {e}")
        if new is None:
            raise PatchError(f"{name}:{i + 1}: deleting files is not supported")
        if old is not None and old != new:
            raise PatchError(f"{name}:{i + 1}: renames are not supported ({old} -> {new})")
        if not new.startswith(tuple(d + "/" for d in DECOMP_DIRS)):
            raise PatchError(f"{name}:{i + 1}: {new} is outside {', '.join(DECOMP_DIRS)}")
        if old is None:
            patch.new_files.add(new)
        hunks = patch.files.setdefault(new, [])
        i += 2
        while i < n and lines[i].startswith("@@"):
            m = HUNK_RE.match(lines[i])
            if not m:
                raise PatchError(f"{name}:{i + 1}: bad hunk header")
            ocount = int(m.group(2)) if m.group(2) is not None else 1
            ncount = int(m.group(4)) if m.group(4) is not None else 1
            ostart = int(m.group(1))
            i += 1
            old_l, new_l, last = [], [], None
            while i < n and (ocount > 0 or ncount > 0 or lines[i].startswith("\\")):
                l = lines[i]
                if l.startswith("\\"):  # "\ No newline at end of file" for the previous line
                    for side in last:
                        side[-1] = side[-1].rstrip("\n")
                elif l[:1] == " " or l in ("\n", "\r\n"):
                    body = l[1:] if l[:1] == " " else l
                    old_l.append(body); new_l.append(body); last = (old_l, new_l)
                    ocount -= 1; ncount -= 1
                elif l[:1] == "-":
                    old_l.append(l[1:]); last = (old_l,); ocount -= 1
                elif l[:1] == "+":
                    new_l.append(l[1:]); last = (new_l,); ncount -= 1
                else:
                    raise PatchError(f"{name}:{i + 1}: unexpected line in hunk: {l.rstrip()!r}")
                i += 1
            if ocount or ncount:
                raise PatchError(f"{name}: truncated hunk for {new}")
            hunks.append((ostart, old_l, new_l))
    return patch


def series_names():
    if not os.path.exists(SERIES):
        return []
    out = []
    for line in read_text(SERIES).splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            out.append(line)
    return out


def load_patches():
    patches, owner = [], {}
    for name in series_names():
        if not os.path.exists(os.path.join(PATCH_DIR, name)):
            raise PatchError(f"series lists {name}, which does not exist")
        p = parse_patch(name)
        for f in p.files:
            if f in owner:
                raise PatchError(f"{f} is owned by both {owner[f]} and {name}; each file belongs to one patch")
            owner[f] = name
        patches.append(p)
    return patches, owner


def apply_hunks(patch, rel, text, hunks, offsets=None):
    lines = text.splitlines(True)
    out, pos, delta = [], 0, 0
    for k, (ostart, old_l, new_l) in enumerate(hunks, 1):
        want = max(ostart - 1 + delta, 0) if old_l else ostart + delta
        found = None
        for d in range(0, len(lines) + 1):
            for at in ((want + d, want - d) if d else (want,)):
                if at < pos or at + len(old_l) > len(lines):
                    continue
                if lines[at:at + len(old_l)] == old_l:
                    found = at
                    break
            if found is not None:
                break
        if found is None:
            raise PatchError(f"{patch.name}: {rel}: hunk #{k} (@@ -{ostart} @@) does not apply")
        if found != want and offsets is not None:
            offsets.append(f"{patch.name}: {rel}: hunk #{k} applies with offset {found - want:+d}")
        delta += found - want
        out.extend(lines[pos:found])
        out.extend(new_l)
        pos = found + len(old_l)
    out.extend(lines[pos:])
    return "".join(out)


def patched_files(patches, offsets=None):
    """rel path -> patched text, for every file a patch owns."""
    result = {}
    for p in patches:
        for rel, hunks in p.files.items():
            src = os.path.join(ROOT, rel)
            if rel in p.new_files:
                if os.path.exists(src):
                    raise PatchError(f"{p.name}: creates {rel}, which already exists in the decomp tree")
                base = ""
            elif not os.path.exists(src):
                raise PatchError(f"{p.name}: {rel} does not exist in the decomp tree")
            else:
                base = read_text(src)
            result[rel] = apply_hunks(p, rel, base, hunks, offsets)
    return result


def _stat(path):
    st = os.stat(path)
    return [st.st_size, st.st_mtime_ns]


def load_manifest():
    try:
        with open(MANIFEST) as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def save_manifest(m):
    os.makedirs(TREE, exist_ok=True)
    tmp = MANIFEST + ".tmp"
    with open(tmp, "w") as f:
        json.dump(m, f, separators=(",", ":"), sort_keys=True)
    os.replace(tmp, MANIFEST)


def decomp_files():
    for d in DECOMP_DIRS:
        for dirpath, dirnames, filenames in os.walk(os.path.join(ROOT, d)):
            dirnames.sort()
            for fn in sorted(filenames):
                yield os.path.relpath(os.path.join(dirpath, fn), ROOT)


def edited_tree_files(manifest, only=None):
    edited = []
    for rel, rec in manifest.items():
        if only is not None and rel not in only:
            continue
        t = os.path.join(TREE, rel)
        if os.path.exists(t) and _stat(t) != rec["tree"]:
            edited.append(rel)
    return sorted(edited)


def materialise(force=False, quiet=False):
    """Bring build/web_wasm/tree up to date. Only files whose content changes are
    rewritten, so object files of unchanged sources are not rebuilt."""
    patches, owner = load_patches()
    owned = patched_files(patches)
    manifest = load_manifest()
    edited = edited_tree_files(manifest)
    if edited and not force:
        listing = "\n  ".join(f"{r}  ({owner[r] if r in owner else 'no patch: use `new`'})" for r in edited[:20])
        raise PatchError("files under build/web_wasm/tree were edited since the last apply:\n  " + listing +
                         "\nkeep them with `patches.py refresh <patch>` (or `new`), or discard them with `apply --force`")
    new_manifest, written = {}, 0
    rels = set(decomp_files()) | set(owned)
    for rel in sorted(rels):
        dst = os.path.join(TREE, rel)
        src = os.path.join(ROOT, rel)
        rec = manifest.get(rel)
        if rel in owned:
            text = owned[rel]
            if not (os.path.exists(dst) and read_text(dst) == text):
                write_text(dst, text)
                written += 1
            new_manifest[rel] = {"src": None, "tree": _stat(dst)}
        else:
            sst = _stat(src)
            if not (rec and rec["src"] == sst and os.path.exists(dst) and _stat(dst) == rec["tree"]):
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                shutil.copy2(src, dst)
                written += 1
            new_manifest[rel] = {"src": sst, "tree": _stat(dst)}
    removed = 0
    for d in DECOMP_DIRS:
        for dirpath, dirnames, filenames in os.walk(os.path.join(TREE, d), topdown=False):
            for fn in filenames:
                rel = os.path.relpath(os.path.join(dirpath, fn), TREE)
                if rel not in rels:
                    os.remove(os.path.join(dirpath, fn))
                    removed += 1
            if not os.listdir(dirpath):
                os.rmdir(dirpath)
    save_manifest(new_manifest)
    if not quiet and (written or removed):
        print(f"patches: {len(patches)} patches, {len(owned)} patched files; tree: {written} written, {removed} removed")
    return written


def find_patch(name):
    names = series_names()
    if name in names:
        return name
    hits = [n for n in names if n == name + ".patch" or re.sub(r"^\d+-|\.patch$", "", n) == name or n.startswith(name + "-")]
    if len(hits) != 1:
        raise PatchError(f"no single patch matches {name!r} (series: {', '.join(names)})")
    return hits[0]


def render_patch(patch, rels):
    out = [patch.header if patch.header.endswith("\n\n") or not patch.header else patch.header.rstrip("\n") + "\n\n"]
    for rel in rels:
        src = os.path.join(ROOT, rel)
        base = read_text(src).splitlines(True) if rel not in patch.new_files else []
        tree = read_text(os.path.join(TREE, rel)).splitlines(True)
        a = "/dev/null" if rel in patch.new_files else "a/" + rel
        diff = list(difflib.unified_diff(base, tree, a, "b/" + rel, n=3))
        if not diff:
            diff = [f"--- {a}\n", f"+++ b/{rel}\n"]
        for l in diff:
            out.append(l if l.endswith("\n") else l + "\n\\ No newline at end of file\n")
    return "".join(out)


def cmd_apply(a):
    materialise(force=a.force)
    print(f"tree: {os.path.relpath(TREE, ROOT)}")


def cmd_check(a):
    try:
        patches, owner = load_patches()
        offsets = []
        patched_files(patches, offsets)
    except PatchError as e:
        print(f"FAIL: {e}", file=sys.stderr)
        return 1
    for o in offsets:
        print(f"note: {o} (run refresh to update line numbers)")
    print(f"OK: {len(patches)} patches, {len(owner)} files apply cleanly to the decomp tree")
    return 0


def cmd_list(a):
    patches, _ = load_patches()
    for p in patches:
        title = p.header.strip().split("\n", 1)[0] if p.header.strip() else ""
        print(f"{p.name}  ({len(p.files)} files)  {title}")
        for rel in p.files:
            print(f"    {rel}")


def cmd_new(a):
    names = series_names()
    patches, owner = load_patches()
    slug = re.sub(r"[^A-Za-z0-9_.-]+", "-", a.name).strip("-")
    nums = [int(n[:4]) for n in names if n[:4].isdigit()]
    fname = f"{(max(nums) + 1) if nums else 1:04d}-{slug}.patch"
    rels = []
    for f in a.files:
        rel = os.path.relpath(os.path.abspath(f), ROOT)
        if rel.startswith(os.path.relpath(TREE, ROOT) + os.sep):
            rel = os.path.relpath(os.path.abspath(f), TREE)
        if rel in owner:
            raise PatchError(f"{rel} already belongs to {owner[rel]}; edit it there and refresh that patch")
        if not os.path.exists(os.path.join(ROOT, rel)):
            raise PatchError(f"{rel} is not a decomp file")
        rels.append(rel)
    header = (a.message.strip() + "\n") if a.message else "TODO: why the host build needs this (byte order, clang vs MWCC, ...).\n"
    write_text(os.path.join(PATCH_DIR, fname), header + "\n" + "".join(f"--- a/{r}\n+++ b/{r}\n" for r in rels))
    write_text(SERIES, "".join(n + "\n" for n in names + [fname]))
    materialise(quiet=True)
    print(f"created {os.path.relpath(os.path.join(PATCH_DIR, fname), ROOT)}; edit these files, then run refresh {fname}:")
    for r in rels:
        print(f"    {os.path.relpath(os.path.join(TREE, r), ROOT)}")


def cmd_refresh(a):
    names = series_names() if a.all else [find_patch(n) for n in a.patches]
    manifest = load_manifest()
    for name in names:
        p = parse_patch(name)
        for rel in p.files:
            if not os.path.exists(os.path.join(TREE, rel)):
                raise PatchError(f"{rel} is missing from the tree; run apply first")
        text = render_patch(p, list(p.files))
        changed = text != read_text(p.path)
        write_text(p.path, text)
        for rel in p.files:
            if rel in manifest:
                manifest[rel]["tree"] = _stat(os.path.join(TREE, rel))
        print(f"{'refreshed' if changed else 'unchanged'}: {name}")
    save_manifest(manifest)
    # the refreshed patches must reproduce the tree exactly
    patches, _ = load_patches()
    owned = patched_files(patches)
    for name in names:
        for rel in parse_patch(name).files:
            if owned[rel] != read_text(os.path.join(TREE, rel)):
                raise PatchError(f"{name}: refreshed patch does not reproduce {rel}")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("apply", help="materialise build/web_wasm/tree")
    s.add_argument("--force", action="store_true", help="discard edits made in the tree")
    s.set_defaults(fn=cmd_apply)
    sub.add_parser("check", help="verify every patch applies to the decomp tree").set_defaults(fn=cmd_check)
    sub.add_parser("list", help="show which patch owns which files").set_defaults(fn=cmd_list)
    s = sub.add_parser("new", help="start a patch owning FILES")
    s.add_argument("name")
    s.add_argument("files", nargs="+")
    s.add_argument("-m", "--message", help="header comment: why the host needs the change")
    s.set_defaults(fn=cmd_new)
    s = sub.add_parser("refresh", help="regenerate patches from their files in the tree")
    s.add_argument("patches", nargs="*")
    s.add_argument("--all", action="store_true")
    s.set_defaults(fn=cmd_refresh)
    a = ap.parse_args(argv)
    if a.cmd == "refresh" and not a.all and not a.patches:
        ap.error("refresh needs patch names or --all")
    try:
        return a.fn(a) or 0
    except PatchError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
