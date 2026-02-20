#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-wasm}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

"${ROOT_DIR}/scripts/check_wasm_prereqs.sh"

CMAKE_ARGS=(
  -S "${ROOT_DIR}"
  -B "${BUILD_DIR}"
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
  -DSLAM_BUILD_TESTS=OFF
)

if [[ -n "${RAYLIB_WASM_ROOT:-}" ]]; then
  CMAKE_ARGS+=(
    -DRAYLIB_INCLUDE_DIR="${RAYLIB_WASM_ROOT}/include"
    -DRAYLIB_LIBRARY="${RAYLIB_WASM_ROOT}/lib/libraylib.a"
  )
fi

echo "[INFO] Configuring WASM build in ${BUILD_DIR}"
emcmake cmake "${CMAKE_ARGS[@]}"

echo "[INFO] Building WASM target"
cmake --build "${BUILD_DIR}" -j

echo "[OK] WASM build complete."
echo "[INFO] Output: ${BUILD_DIR}/slam-raylib.html"
echo "[INFO] Serve with: python3 -m http.server --directory ${BUILD_DIR} 8080"
