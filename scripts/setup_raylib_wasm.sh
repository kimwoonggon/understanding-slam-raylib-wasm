#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
THIRD_PARTY_DIR="${ROOT_DIR}/.third_party"
RAYLIB_SRC_DIR="${RAYLIB_SRC_DIR:-${THIRD_PARTY_DIR}/raylib}"
RAYLIB_BUILD_DIR="${RAYLIB_BUILD_DIR:-${RAYLIB_SRC_DIR}/build-wasm}"
RAYLIB_WASM_ROOT="${RAYLIB_WASM_ROOT:-${ROOT_DIR}/.toolchains/raylib-wasm}"
RAYLIB_REF="${RAYLIB_REF:-5.5}"

# Try to auto-activate emsdk when this shell does not already expose emcc/emcmake.
if ! command -v emcc >/dev/null 2>&1 || ! command -v emcmake >/dev/null 2>&1; then
  EMSDK_CANDIDATE="${EMSDK:-$HOME/emsdk}"
  if [[ -f "${EMSDK_CANDIDATE}/emsdk_env.sh" ]]; then
    # shellcheck disable=SC1090
    source "${EMSDK_CANDIDATE}/emsdk_env.sh" >/dev/null
  fi
fi

if ! command -v emcc >/dev/null 2>&1 || ! command -v emcmake >/dev/null 2>&1; then
  echo "[ERROR] emcc/emcmake not available. Install/activate emsdk first." >&2
  exit 1
fi

mkdir -p "${THIRD_PARTY_DIR}"
if [[ ! -d "${RAYLIB_SRC_DIR}/.git" ]]; then
  echo "[INFO] Cloning raylib into ${RAYLIB_SRC_DIR}"
  git clone --depth 1 --branch "${RAYLIB_REF}" https://github.com/raysan5/raylib.git "${RAYLIB_SRC_DIR}"
else
  echo "[INFO] Reusing existing raylib source at ${RAYLIB_SRC_DIR}"
fi

echo "[INFO] Configuring raylib for WebAssembly"
emcmake cmake \
  -S "${RAYLIB_SRC_DIR}" \
  -B "${RAYLIB_BUILD_DIR}" \
  -DPLATFORM=Web \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_GAMES=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${RAYLIB_WASM_ROOT}"

echo "[INFO] Building/installing raylib wasm"
cmake --build "${RAYLIB_BUILD_DIR}" -j
cmake --install "${RAYLIB_BUILD_DIR}"

echo "[OK] raylib wasm installed."
echo "[INFO] Export this before wasm builds:"
echo "export RAYLIB_WASM_ROOT=${RAYLIB_WASM_ROOT}"
