#!/usr/bin/env python3
"""Build the WebAssembly port: game + JSystem (decomp sources) and the SDK layer.

    source ~/emsdk/emsdk_env.sh
    python port/tools/build.py [--target node|web] [--jobs N] [-O2]
Outputs build/port/<target>/mkdd.{js,wasm}. Incremental via clang depfiles.
"""
import argparse, concurrent.futures as cf, glob, os, shutil, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import scan  # noqa: E402

ROOT = scan.ROOT
EMSDK = os.environ.get("EMSDK", os.path.expanduser("~/emsdk"))
CLANGXX = os.path.join(EMSDK, "upstream", "bin", "clang++")
EMCC = shutil.which("emcc") or os.path.join(EMSDK, "upstream", "emscripten", "emcc")
EMXX = shutil.which("em++") or os.path.join(EMSDK, "upstream", "emscripten", "em++")

SDK_FLAGS = ["-std=gnu11", "-fsigned-char", "-fshort-wchar", "-Wno-typedef-redefinition", "-I", "include", "-I", "port/include", "-I", "port/sdk",
             "-DMKDD_PORT=1", "-DREGION_US=1", "-Wno-incompatible-library-redeclaration",
             "-fno-strict-aliasing"]
SDK_CXX_FLAGS = ["-std=gnu++03", "-fsigned-char", "-fshort-wchar", "-fno-exceptions", "-fno-rtti", "-nostdinc", "-nostdinc++",
                 "-I", "port/include", "-I", "port/sdk", "-I", "include", "-I", "libs/PowerPC_EABI_Support/include",
                 "-I", "build/MarioClub_us/include", "-DREGION_US=1", "-DDEBUG=1", "-DMKDD_PORT=1",
                 "-include", "port/include/mkdd_port.h", "-fms-extensions", "-fms-compatibility",
                 "-fdelayed-template-parsing", "-Wno-error=extraneous-template-head", "-w"]

def stale(src, obj, dep):
    if not os.path.exists(obj) or not os.path.exists(dep):
        return True
    t = os.path.getmtime(obj)
    deps = open(dep).read().replace("\\\n", " ").split(":", 1)[1].split()
    return any(not os.path.exists(os.path.join(ROOT, d)) or os.path.getmtime(os.path.join(ROOT, d)) > t for d in deps)

def job(kind, src, outdir, opt):
    rel = os.path.relpath(src, ROOT)
    obj = os.path.join(outdir, rel + ".o")
    dep = obj + ".d"
    if not stale(src, obj, dep):
        return rel, obj, None
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    if kind == "game":
        flags = [f for f in scan.FLAGS if f not in ("-fsyntax-only",)]
        cmd = [CLANGXX, *flags, "-fshort-wchar", opt, "-fno-delete-null-pointer-checks", "-fno-strict-aliasing", "-g1"]
    elif kind == "conv":
        cmd = [EMXX, "-std=c++17", opt, "-g1"]
    elif kind == "sdkcxx":
        cmd = [CLANGXX, "--target=wasm32-unknown-emscripten", *SDK_CXX_FLAGS, opt, "-g1"]
    else:
        cmd = [EMCC, *SDK_FLAGS, opt, "-g1"]
    r = subprocess.run([*cmd, "-MMD", "-MF", dep, "-c", "-o", obj, src], cwd=ROOT, capture_output=True, text=True)
    return rel, obj if r.returncode == 0 else None, r.stderr if r.returncode else None

LINK_COMMON = [
    "-sASYNCIFY=1", "-sASYNCIFY_STACK_SIZE=1048576", "-sSTACK_SIZE=1048576",
    "-sINITIAL_MEMORY=268435456", "-sALLOW_MEMORY_GROWTH=1", "-sGLOBAL_BASE=65536",
    "-sERROR_ON_UNDEFINED_SYMBOLS=1", "-sASSERTIONS=1", "-sEXIT_RUNTIME=1",
    "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap", "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$stringToNewUTF8", "-g2",
]
LINK_TARGET = {
    "node": ["-sNODERAWFS=1", "-sENVIRONMENT=node"],
    "web": ["-sENVIRONMENT=web", "-sFORCE_FILESYSTEM=1"],
}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--target", choices=sorted(LINK_TARGET), default="node")
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    ap.add_argument("-O", dest="opt", default="2")
    a = ap.parse_args()
    opt = f"-O{a.opt}"
    outdir = os.path.join(ROOT, "build", "port", f"obj-O{a.opt}")
    jobs = [("game", s) for s in scan.sources()]
    jobs += [("sdk", s) for s in sorted(glob.glob(os.path.join(ROOT, "port/sdk/*.c")))]
    jobs += [("sdk", s) for s in sorted(glob.glob(os.path.join(ROOT, "libs/dolphin/gd/*.c")))]
    jobs += [("sdkcxx", s) for s in sorted(glob.glob(os.path.join(ROOT, "port/sdk/*.cpp")))]
    jobs += [("conv", s) for s in sorted(glob.glob(os.path.join(ROOT, "port/conv/*.cpp"))) if not s.endswith("cli.cpp")]
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
    out = os.path.join(ROOT, "build", "port", a.target)
    os.makedirs(out, exist_ok=True)
    rsp = os.path.join(out, "objs.rsp")
    open(rsp, "w").write("\n".join(objs))
    cmd = [EMXX, f"@{rsp}", opt, *LINK_COMMON, *LINK_TARGET[a.target], "-o", os.path.join(out, "mkdd.js")]
    r = subprocess.run(cmd, cwd=ROOT)
    sys.exit(r.returncode)

if __name__ == "__main__":
    main()
