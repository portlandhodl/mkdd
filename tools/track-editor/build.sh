#!/usr/bin/env bash
set -euo pipefail

CRATE_DIR="$(dirname "$(realpath "$0")")"
PROJECT_DIR="$(realpath "$CRATE_DIR/../..")"
export CARGO_TARGET_DIR="${CARGO_TARGET_DIR:-$PROJECT_DIR/build/track-editor/target}"
export RUSTUP_TOOLCHAIN="${RUSTUP_TOOLCHAIN:-stable}"

cargo test --manifest-path "$CRATE_DIR/Cargo.toml"
wasm-pack build "$CRATE_DIR" \
  --target web \
  --release \
  --out-dir www/pkg \
  --out-name track_editor
