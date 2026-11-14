#!/bin/sh
# Build the FIFO log player (port/gpu/tools/dffplay.c) into build/port/dffplay/.
#   source ~/emsdk/emsdk_env.sh && port/gpu/tools/dffplay.sh
set -e
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
OUT="$ROOT/build/port/dffplay${DFFPLAY_SUFFIX:-}"
mkdir -p "$OUT/obj"
cd "$ROOT"
CFLAGS="-O2 -g1 -fsigned-char -fshort-wchar -w -DMKDD_PORT=1 -DREGION_US=1 -I include -I port/include -I port/sdk -I port/gpu"
for f in fifo gpu_vtx gpu_fifolog gpu_dump; do
  emcc -std=gnu11 $CFLAGS -Wno-typedef-redefinition -fno-strict-aliasing -c port/gpu/$f.c -o "$OUT/obj/$f.o"
done
emcc -std=gnu11 $CFLAGS -c port/gpu/tools/dffplay.c -o "$OUT/obj/dffplay.o"
for f in webgl_backend webgl_tev webgl_texture texdec; do
  em++ -std=c++17 -O2 -g1 -fno-exceptions -fno-rtti -I port/gpu -c port/gpu/$f.cpp -o "$OUT/obj/$f.o"
done
# The player's own memory starts at 64 MB so the log's addresses below are free.
em++ "$OUT"/obj/*.o -O2 -sGLOBAL_BASE=67108864 -sINITIAL_MEMORY=402653184 -sALLOW_MEMORY_GROWTH=1 \
  -sENVIRONMENT=web -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sEXIT_RUNTIME=0 \
  -sEXPORTED_FUNCTIONS=_main,_malloc,_free,_dffplay -sEXPORTED_RUNTIME_METHODS=FS,stringToNewUTF8,HEAPU8 \
  -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$stringToNewUTF8,$UTF8ToString' -o "$OUT/dffplay.js"
cp port/gpu/tools/dffplay.html "$OUT/index.html"
echo "built $OUT"
