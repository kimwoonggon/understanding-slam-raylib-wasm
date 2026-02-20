#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Try to auto-activate emsdk when this shell does not already expose emcc/emcmake.
if ! command -v emcc >/dev/null 2>&1 || ! command -v emcmake >/dev/null 2>&1; then
  EMSDK_CANDIDATE="${EMSDK:-$HOME/emsdk}"
  if [[ -f "${EMSDK_CANDIDATE}/emsdk_env.sh" ]]; then
    # shellcheck disable=SC1090
    source "${EMSDK_CANDIDATE}/emsdk_env.sh" >/dev/null
  fi
fi

# Auto-detect local raylib-wasm install if env var is not set.
if [[ -z "${RAYLIB_WASM_ROOT:-}" ]]; then
  LOCAL_RAYLIB_WASM_ROOT="${ROOT_DIR}/.toolchains/raylib-wasm"
  if [[ -f "${LOCAL_RAYLIB_WASM_ROOT}/include/raylib.h" && -f "${LOCAL_RAYLIB_WASM_ROOT}/lib/libraylib.a" ]]; then
    export RAYLIB_WASM_ROOT="${LOCAL_RAYLIB_WASM_ROOT}"
  fi
fi

missing=0

if ! command -v emcc >/dev/null 2>&1; then
  echo "[ERROR] emcc not found. Activate emsdk first (source emsdk_env.sh)." >&2
  missing=1
fi

if ! command -v emcmake >/dev/null 2>&1; then
  echo "[ERROR] emcmake not found. Activate emsdk first (source emsdk_env.sh)." >&2
  missing=1
fi

if [[ "${missing}" -ne 0 ]]; then
  exit 1
fi

if [[ -n "${RAYLIB_WASM_ROOT:-}" ]]; then
  if [[ ! -f "${RAYLIB_WASM_ROOT}/include/raylib.h" ]]; then
    echo "[ERROR] RAYLIB_WASM_ROOT/include/raylib.h not found: ${RAYLIB_WASM_ROOT}" >&2
    exit 1
  fi
  if [[ ! -f "${RAYLIB_WASM_ROOT}/lib/libraylib.a" ]]; then
    echo "[ERROR] RAYLIB_WASM_ROOT/lib/libraylib.a not found: ${RAYLIB_WASM_ROOT}" >&2
    exit 1
  fi
  echo "[OK] Using raylib wasm from RAYLIB_WASM_ROOT=${RAYLIB_WASM_ROOT}"
else
  if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists raylib; then
    echo "[OK] pkg-config raylib found (ensure this is wasm-compatible in emsdk shell)."
  else
    cat >&2 <<'MSG'
[ERROR] Could not verify a WebAssembly-compatible raylib.
Set RAYLIB_WASM_ROOT to a raylib build compiled for Emscripten:
  export RAYLIB_WASM_ROOT=/path/to/raylib-wasm-install
or bootstrap one in this repo:
  ./scripts/setup_raylib_wasm.sh
MSG
    exit 1
  fi
fi

if [[ ! -d "${ROOT_DIR}/assets" ]]; then
  echo "[ERROR] assets directory missing. Ensure map/sound assets are present." >&2
  exit 1
fi

echo "[OK] WASM prerequisites validated."
