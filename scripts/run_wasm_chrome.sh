#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-8080}"
BUILD_RELEASE_DIR="${BUILD_RELEASE_DIR:-${ROOT_DIR}/build-release}"
WASM_BUILD_DIR="${WASM_BUILD_DIR:-${ROOT_DIR}/build-wasm}"
SKIP_SETUP=0

usage() {
  cat <<'USAGE'
Usage: ./scripts/run_wasm_chrome.sh [options]

Options:
  --host <host>            Server host (default: 127.0.0.1)
  --port <port>            Server port (default: 8080)
  --skip-setup             Skip setup_wasm_env.sh
  -h, --help               Show help

This script runs:
1) setup_wasm_env.sh (unless --skip-setup)
2) build_wasm.sh
3) build native slam-static-server target
4) serve build-wasm/ for Chrome access
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host)
      HOST="$2"
      shift 2
      ;;
    --port)
      PORT="$2"
      shift 2
      ;;
    --skip-setup)
      SKIP_SETUP=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "[ERROR] Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

if [[ "${SKIP_SETUP}" -eq 0 ]]; then
  "${ROOT_DIR}/scripts/setup_wasm_env.sh"
fi

"${ROOT_DIR}/scripts/build_wasm.sh"

echo "[INFO] Building native static server target"
cmake -S "${ROOT_DIR}" -B "${BUILD_RELEASE_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_RELEASE_DIR}" --target slam-static-server -j

echo "[OK] WASM build ready at ${WASM_BUILD_DIR}/slam-raylib.html"
echo "[INFO] Open in Chrome: http://${HOST}:${PORT}/slam-raylib.html"
echo "[INFO] Starting static server (Ctrl+C to stop)"
exec "${BUILD_RELEASE_DIR}/slam-static-server" --root "${WASM_BUILD_DIR}" --host "${HOST}" --port "${PORT}"
