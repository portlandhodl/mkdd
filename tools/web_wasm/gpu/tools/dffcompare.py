#!/usr/bin/env python3
"""Render a Dolphin FIFO log (.dff) with Dolphin and with the port's WebGL2 renderer
and compare the XFB images frame by frame.

    tools/web_wasm/gpu/tools/dffplay.sh                    # build the player once
    python3 tools/web_wasm/gpu/tools/dffcompare.py log.dff outdir [--backend Vulkan] [--swiftshader]

Writes outdir/dolphin-NN.png (Dolphin's XFB copies, before presentation),
outdir/ours-NNNNN.ppm, outdir/cmp-NNNNN.png (ours | Dolphin | |diff| x4) and prints
per-frame metrics. Dolphin runs headless (dolphin-emu-nogui) with its own user
directory under outdir, so the normal Dolphin configuration is untouched; it uses
Dolphin's defaults otherwise (copy filter disabled, EFB/XFB copies to texture).
Our side runs in headless Chrome on the GPU (ANGLE/Vulkan) unless --swiftshader.

Recording a log: MKDD_DFF=<file> MKDD_DFF_FRAMES=<start>:<count> (node or browser,
?dff=start:count in the page); see tools/web_wasm/gpu/gpu_fifolog.c.
"""
import argparse, glob, os, shutil, subprocess, sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../.."))

DOLPHIN_INI = """[Core]
GFXBackend = {backend}
CPUThread = False
[FifoPlayer]
LoopReplay = False
[Interface]
ConfirmStop = False
[Analytics]
PermissionAsked = True
Enabled = False
"""
GFX_INI = """[Settings]
InternalResolution = 1
DumpXFBTarget = True
ShaderCompilationMode = 0
WaitForShadersBeforeStarting = True
[Hacks]
EFBToTextureEnable = True
XFBToTextureEnable = True
ImmediateXFBEnable = True
"""


def run_dolphin(dff, out, backend, exe):
    user = os.path.join(out, "dolphin-user")
    shutil.rmtree(os.path.join(user, "Dump"), ignore_errors=True)
    os.makedirs(os.path.join(user, "Config"), exist_ok=True)
    open(os.path.join(user, "Config", "Dolphin.ini"), "w").write(DOLPHIN_INI.format(backend=backend))
    open(os.path.join(user, "Config", "GFX.ini"), "w").write(GFX_INI)
    with open(os.path.join(out, "dolphin.log"), "w") as log:
        subprocess.run([exe, "-p", "headless", "-u", user, "-e", os.path.abspath(dff)], stdout=log, stderr=log, timeout=300)
    dumps = glob.glob(os.path.join(user, "Dump", "Textures", "xfb*.png"))
    dumps.sort(key=lambda p: int(os.path.basename(p).split("_")[1][1:]))
    for f in glob.glob(os.path.join(out, "dolphin-*.png")):
        os.remove(f)
    for i, f in enumerate(dumps):
        shutil.copy(f, os.path.join(out, "dolphin-%02d.png" % i))
    return len(dumps)


def run_ours(dff, out, swiftshader):
    env = dict(os.environ)
    if not swiftshader:
        env.setdefault("CHROME_FLAGS", "--use-angle=vulkan --enable-features=Vulkan --ignore-gpu-blocklist --enable-gpu")
    for f in glob.glob(os.path.join(out, "ours-*.ppm")):
        os.remove(f)
    r = subprocess.run(["node", os.path.join(ROOT, "tools/web_wasm/gpu/tools/dffplay.mjs"), dff, out], env=env,
                       capture_output=True, text=True, timeout=300)
    print(r.stdout.strip().splitlines()[0] if r.stdout.strip() else r.stderr)


def compare(out):
    import numpy as np
    from PIL import Image
    ours = sorted(glob.glob(os.path.join(out, "ours-*.ppm")))
    dol = sorted(glob.glob(os.path.join(out, "dolphin-*.png")))
    for o, p in zip(ours, dol):
        a = np.asarray(Image.open(o).convert("RGB")).astype(np.int16)
        b = np.asarray(Image.open(p).convert("RGB")).astype(np.int16)
        h, w = min(a.shape[0], b.shape[0]), min(a.shape[1], b.shape[1])
        a, b = a[:h, :w], b[:h, :w]
        d = np.abs(a - b)
        mse = float(((a - b) ** 2).mean())
        psnr = 10 * np.log10(255 ** 2 / mse) if mse else 99.0
        tag = os.path.basename(o)[5:-4]
        print(f"frame {tag}: mean |diff| {d.mean():.2f}, pixels off by >24: {(d.max(2) > 24).mean() * 100:.1f}%, PSNR {psnr:.1f} dB")
        side = np.concatenate([a, b, np.clip(d * 4, 0, 255)], axis=1).astype(np.uint8)
        Image.fromarray(side).save(os.path.join(out, f"cmp-{tag}.png"))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dff")
    ap.add_argument("out")
    ap.add_argument("--backend", default="Vulkan", help='Dolphin video backend ("Software Renderer" works too)')
    ap.add_argument("--dolphin", default="dolphin-emu-nogui")
    ap.add_argument("--swiftshader", action="store_true", help="render ours with SwiftShader instead of the GPU")
    ap.add_argument("--skip-dolphin", action="store_true", help="reuse outdir/dolphin-*.png")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    if not a.skip_dolphin:
        print(f"Dolphin: {run_dolphin(a.dff, a.out, a.backend, a.dolphin)} XFB copies")
    run_ours(a.dff, a.out, a.swiftshader)
    compare(a.out)


if __name__ == "__main__":
    sys.exit(main())
