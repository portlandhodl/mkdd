# WebAssembly port

Runs the decompiled game in a browser (WebGL2 + WebAudio) or headless under
node. Independent of `ninja` and the matching build.

| Path | Contents |
| --- | --- |
| `sdk/` | GameCube SDK replacement (OS, DVD, VI/PAD, AI/DSP mixer, card) |
| `gpu/` | emulated command processor, software T&L, WebGL2 renderer, tools |
| `conv/` | disc-format converters to host byte order (also a native CLI) |
| `include/` | force-included `mkdd_port.h` (intrinsics, `port_be16/32`), GX FIFO shim |
| `patches/` | host patch set for decomp files, applied in `series` order |
| `tools/` | `build.py`, `patches.py`, disc extraction, stub generation |
| `net/` | online multiplayer: state-sync netcode design (`NETCODE.md`) and remote-kart spike (`netkart.cpp`) |
| `web/` | page (`index.html`, `theme.css`, `ui.js`, OFL fonts in `fonts/`), input, audio, `serve.sh`, headless smoke test |

## Build and run

```sh
source ~/emsdk/emsdk_env.sh
python tools/web_wasm/tools/build.py --target node      # build/web_wasm/node/mkdd.js
MKDD_DISC=/path/GM4E01.iso node build/web_wasm/node/mkdd.js
tools/web_wasm/web/serve.sh                             # web build, serves build/web_wasm/web
MKDD_ISO=/path/GM4E01.iso node tools/web_wasm/web/smoke.mjs
```

`--tag NAME` builds into separate `obj-*-NAME` / `<target>-NAME` directories.

Online play (lobby server, WebRTC transport, `port_net_*` bridge for the game):
`net/README.md`.

## Patch set

`src/`, `libs/` and `include/` hold only matching-decomp code. Anything the
host build needs changed in a decomp file (byte order, bitfield order, Gekko
asm, clang vs MWCC language differences, behaviour that relies on PowerPC
register leftovers...) is a patch in `patches/`, never an edit in the decomp
tree. Each decomp file belongs to at most one patch; the header of each patch
says why it exists.

`build.py` first materialises `build/web_wasm/tree/{src,libs,include}` =
decomp tree + patches (only changed files are rewritten, so incremental builds
stay incremental) and compiles decomp sources from there.

```sh
P="python tools/web_wasm/tools/patches.py"
$P check                    # every patch applies to the current decomp tree
$P list                     # patch -> files
$P apply                    # materialise build/web_wasm/tree
# change a file a patch already owns:
$EDITOR build/web_wasm/tree/src/Foo.cpp && $P refresh 0013-missing-returns
# change a file no patch owns yet:
$P new my-topic src/Foo.cpp -m "Why the host needs this." && $EDITOR build/web_wasm/tree/src/Foo.cpp && $P refresh my-topic
```

`apply` (and therefore `build.py`) refuses to overwrite tree files that were
edited since it last wrote them; `refresh` them or discard with `apply --force`.
After an upstream merge or new matching work, run `check`; if a hunk no longer
applies, apply the rest, fix the file in the tree and `refresh` that patch.
