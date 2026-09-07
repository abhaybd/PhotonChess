#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v emcmake >/dev/null 2>&1; then
	echo "emcmake not found. Activate the Emscripten SDK in this shell, e.g.:" >&2
	echo "  source ~/emsdk/emsdk_env.sh" >&2
	exit 1
fi

mkdir -p "${ROOT}/web/public/engine"

emcmake cmake -S "${ROOT}" -B "${ROOT}/build-wasm" -DCMAKE_BUILD_TYPE=Release
cmake --build "${ROOT}/build-wasm" -j

cd "${ROOT}/web"
if [[ -f package-lock.json ]]; then
	npm ci
else
	npm install
fi
npm run build
