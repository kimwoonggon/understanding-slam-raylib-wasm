#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-wasm}"
BUILD_TYPE="${BUILD_TYPE:-Release}"

# shellcheck disable=SC1091
source "${ROOT_DIR}/scripts/ensure_emsdk_env.sh"
slam_auto_activate_emsdk "${ROOT_DIR}" emcc emcmake || true

# Auto-detect local raylib-wasm install if env var is not set.
if [[ -z "${RAYLIB_WASM_ROOT:-}" ]]; then
  LOCAL_RAYLIB_WASM_ROOT="${ROOT_DIR}/.toolchains/raylib-wasm"
  if [[ -f "${LOCAL_RAYLIB_WASM_ROOT}/include/raylib.h" && -f "${LOCAL_RAYLIB_WASM_ROOT}/lib/libraylib.a" ]]; then
    export RAYLIB_WASM_ROOT="${LOCAL_RAYLIB_WASM_ROOT}"
  fi
fi

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
echo "[INFO] Serve with C++ static server:"
echo "  cmake -S ${ROOT_DIR} -B ${ROOT_DIR}/build-release -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build ${ROOT_DIR}/build-release --target slam-static-server -j"
echo "  ${ROOT_DIR}/build-release/slam-static-server --root ${BUILD_DIR} --port 8080"
