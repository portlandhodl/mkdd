#!/bin/sh
# Build the native converter CLI: build/port/mkdd-conv
set -e
cd "$(dirname "$0")/../.."
mkdir -p build/port
c++ -std=c++17 -O2 -g -Wall -Wno-unused-function -o build/port/mkdd-conv \
    $(ls port/conv/*.cpp | grep -v capi.cpp)
