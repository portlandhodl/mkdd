#!/bin/sh
# Build the synthetic WebGL2 backend test into build/web_wasm/webgl_test/.
#   source ~/emsdk/emsdk_env.sh && tools/web_wasm/gpu/tools/webgl_test.sh
set -e
ROOT=$(cd "$(dirname "$0")/../../../.." && pwd)
OUT="$ROOT/build/web_wasm/webgl_test"
mkdir -p "$OUT"
cd "$ROOT"
em++ -std=c++17 -O2 -fno-exceptions -fno-rtti -Wall -I tools/web_wasm/gpu \
  tools/web_wasm/gpu/tools/webgl_test.cpp tools/web_wasm/gpu/webgl_backend.cpp tools/web_wasm/gpu/webgl_tev.cpp \
  tools/web_wasm/gpu/webgl_texture.cpp tools/web_wasm/gpu/texdec.cpp \
  -sENVIRONMENT=web -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sALLOW_MEMORY_GROWTH=1 \
  -sEXIT_RUNTIME=0 -o "$OUT/webgl_test.js"
cp tools/web_wasm/gpu/tools/webgl_test.html "$OUT/index.html"
echo "built $OUT"
