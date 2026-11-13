#!/bin/sh
# Build the synthetic WebGL2 backend test into build/port/webgl_test/.
#   source ~/emsdk/emsdk_env.sh && port/gpu/tools/webgl_test.sh
set -e
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
OUT="$ROOT/build/port/webgl_test"
mkdir -p "$OUT"
cd "$ROOT"
em++ -std=c++17 -O2 -fno-exceptions -fno-rtti -Wall -I port/gpu \
  port/gpu/tools/webgl_test.cpp port/gpu/webgl_backend.cpp port/gpu/webgl_tev.cpp \
  port/gpu/webgl_texture.cpp port/gpu/texdec.cpp \
  -sENVIRONMENT=web -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sALLOW_MEMORY_GROWTH=1 \
  -sEXIT_RUNTIME=0 -o "$OUT/webgl_test.js"
cp port/gpu/tools/webgl_test.html "$OUT/index.html"
echo "built $OUT"
