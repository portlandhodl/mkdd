#!/usr/bin/env python3
"""Build the WebAssembly port: game + JSystem (decomp sources) and the SDK layer.

Decomp sources are compiled from build/web_wasm/tree, the decomp tree with
tools/web_wasm/patches applied (see patches.py); it is refreshed first.

    source ~/emsdk/emsdk_env.sh
    python tools/web_wasm/tools/build.py [--target node|web] [--jobs N] [-O2]
Outputs build/web_wasm/<target>/mkdd.{js,wasm}. Incremental via clang depfiles.
"""
import argparse, concurrent.futures as cf, glob, os, shutil, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import patches  # noqa: E402
import scan  # noqa: E402

ROOT = scan.ROOT
TREE = scan.TREE
# __FILE__ (assert messages) keeps naming the decomp path, not the patched copy
REWRITE = os.path.join(ROOT, "build", "web_wasm", "rewrite")
PREFIX_MAP = [f"-fmacro-prefix-map={TREE}/={ROOT}/", f"-fmacro-prefix-map={REWRITE}/={ROOT}/",
              f"-fmacro-prefix-map={os.path.relpath(TREE, ROOT)}/="]
EMSDK = os.environ.get("EMSDK", os.path.expanduser("~/emsdk"))
CLANGXX = os.path.join(EMSDK, "upstream", "bin", "clang++")
EMCC = shutil.which("emcc") or os.path.join(EMSDK, "upstream", "emscripten", "emcc")
EMXX = shutil.which("em++") or os.path.join(EMSDK, "upstream", "emscripten", "em++")

SDK_FLAGS = ["-std=gnu11", "-fsigned-char", "-fshort-wchar", "-Wno-typedef-redefinition", "-I", "build/web_wasm/tree/include", "-I", "tools/web_wasm/include", "-I", "tools/web_wasm/sdk", "-I", "tools/web_wasm/gpu",
             "-DMKDD_PORT=1", "-DREGION_US=1", "-Wno-incompatible-library-redeclaration",
             "-fno-strict-aliasing"]
SDK_CXX_FLAGS = ["-std=gnu++03", "-fsigned-char", "-fshort-wchar", "-fno-exceptions", "-fno-rtti", "-nostdinc", "-nostdinc++",
                 "-I", "tools/web_wasm/include", "-I", "tools/web_wasm/sdk", "-I", "build/web_wasm/tree/include", "-I", "build/web_wasm/tree/libs/PowerPC_EABI_Support/include",
                 "-I", "build/MarioClub_us/include", "-DREGION_US=1", "-DDEBUG=1", "-DMKDD_PORT=1",
                 "-include", "tools/web_wasm/include/mkdd_port.h", "-fms-extensions", "-fms-compatibility",
                 "-fdelayed-template-parsing", "-Wno-error=extraneous-template-head", "-w"]

def rewrite_long_chars(text):
    """MWCC packs 5-8 character constants ('T_tx1') into a 64-bit value, first
    character most significant; clang keeps only 4. Replace them with the value.
    Strings and comments are left alone."""
    out, i, n, changed = [], 0, len(text), False
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i); j = n if j < 0 else j
            out.append(text[i:j]); i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2); j = n if j < 0 else j + 2
            out.append(text[i:j]); i = j
        elif c == '"':
            j = i + 1
            while j < n and text[j] != '"':
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1]); i = j + 1
        elif c == "'":
            j = i + 1
            while j < n and text[j] != "'":
                j += 2 if text[j] == "\\" else 1
            body = text[i + 1:j]
            if 5 <= len(body) <= 8 and "\\" not in body:
                v = 0
                for ch in body.encode("latin-1"):
                    v = v << 8 | ch
                out.append(f"((long long)0x{v:X}LL)"); changed = True
            else:
                out.append(text[i:j + 1])
            i = j + 1
        else:
            j = i
            while j < n and text[j] not in "/\"'":
                j += 1
            if j == i:
                j = i + 1
            out.append(text[i:j]); i = j
    return "".join(out), changed

def stale(src, obj, dep):
    if not os.path.exists(obj) or not os.path.exists(dep):
        return True
    t = os.path.getmtime(obj)
    if os.path.getmtime(src) > t:
        return True
    deps = open(dep).read().replace("\\\n", " ").split(":", 1)[1].split()
    return any(not os.path.exists(os.path.join(ROOT, d)) or os.path.getmtime(os.path.join(ROOT, d)) > t for d in deps)

def job(kind, src, outdir, opt):
    rel = os.path.relpath(src, TREE if src.startswith(TREE + os.sep) else ROOT)
    orig_src = src
    obj = os.path.join(outdir, rel + ".o")
    dep = obj + ".d"
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    if kind == "game":
        flags = [f for f in scan.FLAGS if f not in ("-fsyntax-only",)]
        cmd = [CLANGXX, *flags, "-fshort-wchar", "-fno-strict-return", opt, "-fno-delete-null-pointer-checks", "-fno-strict-aliasing", "-g1"]
        text, changed = rewrite_long_chars(open(src, encoding="latin-1").read())
        if changed:
            rw = os.path.join(REWRITE, rel)
            os.makedirs(os.path.dirname(rw), exist_ok=True)
            if not os.path.exists(rw) or open(rw, encoding="latin-1").read() != text:
                open(rw, "w", encoding="latin-1").write(text)
            cmd += ["-iquote", os.path.dirname(src)]
            src = rw
    elif kind == "gpucxx":
        cmd = [EMXX, "-std=c++17", "-fno-exceptions", "-fno-rtti", "-I", "build/web_wasm/tree/include", "-I", "tools/web_wasm/include", "-I", "tools/web_wasm/sdk",
               "-I", "tools/web_wasm/gpu", "-DMKDD_PORT=1", "-fshort-wchar", opt, "-g1"]
    elif kind == "conv":
        cmd = [EMXX, "-std=c++17", opt, "-g1"]
    elif kind == "gx":
        cmd = [EMCC, *SDK_FLAGS, "-include", "tools/web_wasm/include/mkdd_port.h", opt, "-g1"]
    elif kind == "sdkcxx":
        cmd = [CLANGXX, "--target=wasm32-unknown-emscripten", *SDK_CXX_FLAGS, opt, "-g1"]
    else:
        cmd = [EMCC, *SDK_FLAGS, opt, "-g1"]
    full = [*cmd, *PREFIX_MAP, "-MMD", "-MF", dep, "-c", "-o", obj, src]
    cmdfile = obj + ".cmd"
    if not stale(orig_src, obj, dep) and os.path.exists(cmdfile) and open(cmdfile).read() == " ".join(full):
        return rel, obj, None
    r = subprocess.run(full, cwd=ROOT, capture_output=True, text=True)
    if r.returncode == 0:
        open(cmdfile, "w").write(" ".join(full))
    return rel, obj if r.returncode == 0 else None, r.stderr if r.returncode else None

LINK_COMMON = [
    "-sASYNCIFY=1", "-sASYNCIFY_STACK_SIZE=1048576", "-sSTACK_SIZE=1048576",
    "-sINITIAL_MEMORY=268435456", "-sALLOW_MEMORY_GROWTH=1", "-sGLOBAL_BASE=65536",
    "-sERROR_ON_UNDEFINED_SYMBOLS=1", "-sASSERTIONS=1", "-sMAX_WEBGL_VERSION=2",
    "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap", "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$stringToNewUTF8,$UTF8ToString", "-g2",
]
LINK_TARGET = {
    "node": ["-sNODERAWFS=1", "-sENVIRONMENT=node", "-sEXIT_RUNTIME=1"],
    "web": ["-sENVIRONMENT=web", "-sINVOKE_RUN=0", "-sEXIT_RUNTIME=0", "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2", "-sEXPORTED_RUNTIME_METHODS=callMain,ccall,cwrap,FS"],
}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--target", choices=sorted(LINK_TARGET), default="node")
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    ap.add_argument("-O", dest="opt", default="2")
    ap.add_argument("--tag", default="", help="separate object/output directories (parallel builds)")
    a = ap.parse_args()
    try:
        patches.materialise()
    except patches.PatchError as e:
        sys.exit(f"patch set: {e}")
    opt = f"-O{a.opt}"
    suffix = f"-{a.tag}" if a.tag else ""
    outdir = os.path.join(ROOT, "build", "web_wasm", f"obj-O{a.opt}{suffix}")
    jobs = [("game", s) for s in scan.sources()]
    jobs += [("sdk", s) for s in sorted(glob.glob(os.path.join(ROOT, "tools/web_wasm/sdk/*.c")))]
    jobs += [("sdk", s) for s in sorted(glob.glob(os.path.join(TREE, "libs/dolphin/gd/*.c")))]
    jobs += [("gx", s) for s in sorted(glob.glob(os.path.join(TREE, "libs/dolphin/gx/*.c")))]
    jobs += [("sdkcxx", s) for s in sorted(glob.glob(os.path.join(ROOT, "tools/web_wasm/sdk/*.cpp")))]
    jobs += [("sdk", s) for s in sorted(glob.glob(os.path.join(ROOT, "tools/web_wasm/net/*.c")))]
    jobs += [("sdkcxx", s) for s in sorted(glob.glob(os.path.join(ROOT, "tools/web_wasm/net/*.cpp")))]
    jobs += [("conv", s) for s in sorted(glob.glob(os.path.join(ROOT, "tools/web_wasm/conv/*.cpp"))) if not s.endswith("cli.cpp")]
    jobs += [("sdk", s) for s in sorted(glob.glob(os.path.join(ROOT, "tools/web_wasm/gpu/*.c")))]
    jobs += [("gpucxx", s) for s in sorted(glob.glob(os.path.join(ROOT, "tools/web_wasm/gpu/*.cpp")))]
    objs, errors = [], []
    with cf.ThreadPoolExecutor(a.jobs) as ex:
        for rel, obj, err in ex.map(lambda j: job(j[0], j[1], outdir, opt), jobs):
            if obj:
                objs.append(obj)
            else:
                errors.append((rel, err))
    for rel, err in errors:
        print(f"--- {rel}\n{err}", file=sys.stderr)
    if errors:
        sys.exit(f"{len(errors)} compile failures")
    out = os.path.join(ROOT, "build", "web_wasm", a.target + suffix)
    os.makedirs(out, exist_ok=True)
    rsp = os.path.join(out, "objs.rsp")
    open(rsp, "w").write("\n".join(objs))
    cmd = [EMXX, f"@{rsp}", opt, *LINK_COMMON, *LINK_TARGET[a.target], "-o", os.path.join(out, "mkdd.js")]
    r = subprocess.run(cmd, cwd=ROOT)
    if r.returncode == 0 and a.target == "web":
        for f in glob.glob(os.path.join(ROOT, "tools", "web_wasm", "web", "*")):
            if os.path.isfile(f) and f.endswith((".html", ".css", ".js", ".ico", ".png")):
                shutil.copy(f, out)
    sys.exit(r.returncode)

if __name__ == "__main__":
    main()
