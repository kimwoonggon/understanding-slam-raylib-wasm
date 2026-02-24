#!/usr/bin/env bash
# shellcheck shell=bash

if [[ "${SLAM_EMSDK_HELPER_LOADED:-0}" == "1" ]]; then
  return 0
fi
SLAM_EMSDK_HELPER_LOADED=1

slam_tools_available() {
  local tool=""
  for tool in "$@"; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
      return 1
    fi
  done
  return 0
}

slam_auto_activate_emsdk() {
  local root_dir="${1:-}"
  shift || true
  local required_tools=("$@")
  local -a emsdk_candidates=()
  local emsdk_candidate=""

  if [[ "${#required_tools[@]}" -eq 0 ]]; then
    required_tools=("emcc")
  fi

  if slam_tools_available "${required_tools[@]}"; then
    return 0
  fi

  if [[ -n "${EMSDK:-}" ]]; then
    emsdk_candidates+=("${EMSDK}")
  fi
  if [[ -n "${EMSDK_ROOT:-}" ]]; then
    emsdk_candidates+=("${EMSDK_ROOT}")
  fi

  if [[ -n "${root_dir}" ]]; then
    emsdk_candidates+=(
      "${root_dir}/.third_party/emsdk"
      "${root_dir}/.toolchains/emsdk"
    )
  fi

  emsdk_candidates+=(
    "${HOME}/emsdk"
    "/opt/emsdk"
    "/usr/local/emsdk"
    "/mnt/c/emsdk"
    "/mnt/d/emsdk"
  )

  for emsdk_candidate in "${emsdk_candidates[@]}"; do
    if [[ ! -f "${emsdk_candidate}/emsdk_env.sh" ]]; then
      continue
    fi

    # shellcheck disable=SC1090
    source "${emsdk_candidate}/emsdk_env.sh" >/dev/null
    if slam_tools_available "${required_tools[@]}"; then
      export EMSDK="${emsdk_candidate}"
      return 0
    fi
  done

  return 1
}
