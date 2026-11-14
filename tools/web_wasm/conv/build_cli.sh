#!/bin/sh
# Build the native converter CLI: build/web_wasm/mkdd-conv
set -e
cd "$(dirname "$0")/../../.."
mkdir -p build/web_wasm
c++ -std=c++17 -O2 -g -Wall -Wno-unused-function -o build/web_wasm/mkdd-conv \
    $(ls tools/web_wasm/conv/*.cpp | grep -v capi.cpp)
