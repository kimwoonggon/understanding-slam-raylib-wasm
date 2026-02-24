#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="${OUT_DIR:-${ROOT_DIR}/build-wasm-trace}"
OUT_JS="${OUT_JS:-${OUT_DIR}/slam-diff-trace.js}"

# shellcheck disable=SC1091
source "${ROOT_DIR}/scripts/ensure_emsdk_env.sh"
slam_auto_activate_emsdk "${ROOT_DIR}" em++ || true

if ! command -v em++ >/dev/null 2>&1; then
  cat >&2 <<'MSG'
[ERROR] em++ not found. Install emsdk or expose it via EMSDK/EMSDK_ROOT.
Expected emsdk_env.sh in one of:
  $EMSDK
  $EMSDK_ROOT
  ./.third_party/emsdk
  ./.toolchains/emsdk
  ~/emsdk
  /opt/emsdk
  /usr/local/emsdk
  /mnt/c/emsdk
  /mnt/d/emsdk
MSG
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
