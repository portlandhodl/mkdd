#!/bin/sh
# Build the web target and serve it locally: http://127.0.0.1:${PORT:-8090}/
set -e
cd "$(dirname "$0")/../../.."
. "${EMSDK:-$HOME/emsdk}/emsdk_env.sh" > /dev/null 2>&1
python3 -I tools/web_wasm/tools/build.py --target web
exec python3 -m http.server "${PORT:-8090}" --bind "${HOST:-127.0.0.1}" --directory build/web_wasm/web
