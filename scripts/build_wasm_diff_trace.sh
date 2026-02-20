#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/build-wasm-trace}"
OUT_JS="${OUT_JS:-${OUT_DIR}/slam-diff-trace.js}"

# Try to auto-activate emsdk when this shell does not already expose em++.
if ! command -v em++ >/dev/null 2>&1; then
  EMSDK_CANDIDATE="${EMSDK:-$HOME/emsdk}"
  if [[ -f "${EMSDK_CANDIDATE}/emsdk_env.sh" ]]; then
    # shellcheck disable=SC1090
    source "${EMSDK_CANDIDATE}/emsdk_env.sh" >/dev/null
  fi
fi

if ! command -v em++ >/dev/null 2>&1; then
  echo "[ERROR] em++ not found. Activate emsdk first (or install it)." >&2
  exit 1
fi

mkdir -p "${OUT_DIR}"

echo "[INFO] Building wasm/node differential trace"
em++ \
  -std=c++20 \
  -O2 \
  -I "${ROOT_DIR}/src" \
  -DSLAM_PROJECT_ROOT="\"${ROOT_DIR}\"" \
  "${ROOT_DIR}/src/tools/DifferentialTrace.cpp" \
  "${ROOT_DIR}/src/app/AssetPaths.cpp" \
  "${ROOT_DIR}/src/core/WorldGrid.cpp" \
  "${ROOT_DIR}/src/core/SimulatedLidar.cpp" \
  "${ROOT_DIR}/src/core/OccupancyGridMap.cpp" \
  "${ROOT_DIR}/src/input/Motion.cpp" \
  -sWASM=1 \
  -sENVIRONMENT=node \
  -sNODERAWFS=1 \
  -sEXIT_RUNTIME=1 \
  -sASSERTIONS=1 \
  -sALLOW_MEMORY_GROWTH=1 \
  -o "${OUT_JS}"

echo "[OK] wasm/node trace built: ${OUT_JS}"
