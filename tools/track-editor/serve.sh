#!/usr/bin/env bash
set -euo pipefail

CRATE_DIR="$(dirname "$(realpath "$0")")"
"$CRATE_DIR/build.sh"
exec python3 -m http.server "${PORT:-8000}" \
  --bind "${HOST:-127.0.0.1}" \
  --directory "$CRATE_DIR/www"
