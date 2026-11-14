#!/bin/sh
# Build the FIFO log player (tools/web_wasm/gpu/tools/dffplay.c) into build/web_wasm/dffplay/.
#   source ~/emsdk/emsdk_env.sh && tools/web_wasm/gpu/tools/dffplay.sh
set -e
ROOT=$(cd "$(dirname "$0")/../../../.." && pwd)
OUT="$ROOT/build/web_wasm/dffplay${DFFPLAY_SUFFIX:-}"
mkdir -p "$OUT/obj"
cd "$ROOT"
python3 tools/web_wasm/tools/patches.py apply >/dev/null  # patched decomp headers
CFLAGS="-O2 -g1 -fsigned-char -fshort-wchar -w -DMKDD_PORT=1 -DREGION_US=1 -I build/web_wasm/tree/include -I tools/web_wasm/include -I tools/web_wasm/sdk -I tools/web_wasm/gpu"
for f in fifo gpu_vtx gpu_fifolog gpu_dump; do
  emcc -std=gnu11 $CFLAGS -Wno-typedef-redefinition -fno-strict-aliasing -c tools/web_wasm/gpu/$f.c -o "$OUT/obj/$f.o"
done
emcc -std=gnu11 $CFLAGS -c tools/web_wasm/gpu/tools/dffplay.c -o "$OUT/obj/dffplay.o"
for f in webgl_backend webgl_tev webgl_texture texdec; do
  em++ -std=c++17 -O2 -g1 -fno-exceptions -fno-rtti -I tools/web_wasm/gpu -c tools/web_wasm/gpu/$f.cpp -o "$OUT/obj/$f.o"
done
# The player's own memory starts at 64 MB so the log's addresses below are free.
em++ "$OUT"/obj/*.o -O2 -sGLOBAL_BASE=67108864 -sINITIAL_MEMORY=402653184 -sALLOW_MEMORY_GROWTH=1 \
  -sENVIRONMENT=web -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sEXIT_RUNTIME=0 \
  -sEXPORTED_FUNCTIONS=_main,_malloc,_free,_dffplay -sEXPORTED_RUNTIME_METHODS=FS,stringToNewUTF8,HEAPU8 \
  -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$stringToNewUTF8,$UTF8ToString' -o "$OUT/dffplay.js"
cp tools/web_wasm/gpu/tools/dffplay.html "$OUT/index.html"
echo "built $OUT"
