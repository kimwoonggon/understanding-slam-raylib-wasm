#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EMSDK_DIR="${EMSDK_DIR:-${EMSDK:-$HOME/emsdk}}"
EMSDK_VERSION="${EMSDK_VERSION:-}"
RAYLIB_REF="${RAYLIB_REF:-5.5}"
SETUP_RAYLIB=1

usage() {
  cat <<'USAGE'
Usage: ./scripts/setup_wasm_env.sh [options]

Options:
  --emsdk-dir <path>       Set emsdk install path (default: $EMSDK or ~/emsdk)
  --emsdk-version <ver>    Force emsdk version (default: auto)
  --skip-raylib            Skip raylib-wasm bootstrap
  --raylib-ref <tag>       raylib git ref used by setup_raylib_wasm.sh (default: 5.5)
  -h, --help               Show help

Default behavior:
1) Install/activate emsdk automatically.
2) If Python < 3.10, use emsdk 3.1.74 for compatibility.
3) Build/install raylib-wasm under .toolchains/raylib-wasm.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --emsdk-dir)
      EMSDK_DIR="$2"
      shift 2
      ;;
    --emsdk-version)
      EMSDK_VERSION="$2"
      shift 2
      ;;
    --skip-raylib)
      SETUP_RAYLIB=0
      shift
      ;;
    --raylib-ref)
      RAYLIB_REF="$2"
      shift 2
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

pick_python_cmd() {
  if command -v python3 >/dev/null 2>&1; then
    echo "python3"
    return 0
  fi
  if command -v python >/dev/null 2>&1; then
    echo "python"
    return 0
  fi
  return 1
}

detect_default_emsdk_version() {
  local py_cmd=""
  local py_major=""
  local py_minor=""
  py_cmd="$(pick_python_cmd || true)"

  # If python is not available, fall back to a conservative version.
  if [[ -z "${py_cmd}" ]]; then
    echo "3.1.74"
    return 0
  fi

  read -r py_major py_minor < <("${py_cmd}" -c 'import sys; print(sys.version_info.major, sys.version_info.minor)')
  if [[ "${py_major}" -gt 3 ]] || ([[ "${py_major}" -eq 3 ]] && [[ "${py_minor}" -ge 10 ]]); then
    echo "latest"
  else
    echo "3.1.74"
  fi
}

if [[ -z "${EMSDK_VERSION}" ]]; then
  EMSDK_VERSION="$(detect_default_emsdk_version)"
fi

mkdir -p "$(dirname "${EMSDK_DIR}")"
if [[ ! -d "${EMSDK_DIR}/.git" ]]; then
  echo "[INFO] Cloning emsdk into ${EMSDK_DIR}"
  git clone https://github.com/emscripten-core/emsdk.git "${EMSDK_DIR}"
else
  echo "[INFO] Reusing emsdk at ${EMSDK_DIR}"
fi

export EMSDK="${EMSDK_DIR}"
export EMSDK_QUIET=1

echo "[INFO] Installing emsdk ${EMSDK_VERSION}"
"${EMSDK_DIR}/emsdk" install "${EMSDK_VERSION}"
echo "[INFO] Activating emsdk ${EMSDK_VERSION}"
"${EMSDK_DIR}/emsdk" activate "${EMSDK_VERSION}"

# shellcheck disable=SC1091
source "${ROOT_DIR}/scripts/ensure_emsdk_env.sh"
slam_auto_activate_emsdk "${ROOT_DIR}" emcc emcmake || true

if ! command -v emcc >/dev/null 2>&1 || ! command -v emcmake >/dev/null 2>&1; then
  echo "[ERROR] emsdk activation completed but emcc/emcmake are still unavailable." >&2
  exit 1
fi

echo "[OK] emsdk ready: EMSDK=${EMSDK_DIR}"

if [[ "${SETUP_RAYLIB}" -eq 1 ]]; then
  echo "[INFO] Bootstrapping raylib-wasm"
  RAYLIB_REF="${RAYLIB_REF}" "${ROOT_DIR}/scripts/setup_raylib_wasm.sh"
  echo "[OK] raylib-wasm ready: ${ROOT_DIR}/.toolchains/raylib-wasm"
else
  echo "[INFO] Skipping raylib-wasm bootstrap (--skip-raylib)"
fi

cat <<EOF
[DONE] WASM environment setup complete.
Recommended persistent settings:
  export EMSDK=${EMSDK_DIR}
  export RAYLIB_WASM_ROOT=${ROOT_DIR}/.toolchains/raylib-wasm
EOF
