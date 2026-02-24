#!/usr/bin/env bash
set -euo pipefail

PORT="${PORT:-8080}"
PAGE_PATH="${PAGE_PATH:-/slam-raylib.html}"

usage() {
  cat <<'USAGE'
Usage: ./scripts/print_access_ip.sh [--port <port>] [--path </page>]

Examples:
  ./scripts/print_access_ip.sh
  ./scripts/print_access_ip.sh --port 9000
  ./scripts/print_access_ip.sh --port 8080 --path /slam-raylib.html
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --port)
      PORT="$2"
      shift 2
      ;;
    --path)
      PAGE_PATH="$2"
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

is_wsl() {
  grep -qi microsoft /proc/version 2>/dev/null || grep -qi microsoft /proc/sys/kernel/osrelease 2>/dev/null
}

first_linux_ipv4() {
  local ip=""
  for ip in $(hostname -I 2>/dev/null); do
    if [[ "${ip}" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]] && [[ "${ip}" != 127.* ]]; then
      echo "${ip}"
      return 0
    fi
  done
  return 1
}

first_windows_ipv4_from_wsl() {
  local ps="/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"
  local win_ip=""
  if [[ ! -x "${ps}" ]]; then
    return 1
  fi

  win_ip="$(
    "${ps}" -NoProfile -Command \
      "Get-NetIPConfiguration | Where-Object { \$_.IPv4Address -ne \$null -and \$_.NetAdapter.Status -eq 'Up' -and \$_.IPv4DefaultGateway -ne \$null -and \$_.InterfaceAlias -notlike 'vEthernet*' } | Select-Object -ExpandProperty IPv4Address | Select-Object -ExpandProperty IPAddress -First 1" \
      2>/dev/null | tr -d '\r' | head -n 1
  )"

  if [[ "${win_ip}" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "${win_ip}"
    return 0
  fi
  return 1
}

linux_ip="$(first_linux_ipv4 || true)"

echo "Environment: $(is_wsl && echo WSL || echo Linux)"
if [[ -n "${linux_ip}" ]]; then
  echo "Linux IP: ${linux_ip}"
fi

if is_wsl; then
  win_ip="$(first_windows_ipv4_from_wsl || true)"
  if [[ -n "${win_ip}" ]]; then
    echo "Windows LAN IP: ${win_ip}"
    echo "Open from iPad: http://${win_ip}:${PORT}${PAGE_PATH}"
  else
    echo "[WARN] Could not auto-detect Windows LAN IP from WSL."
    echo "Run 'ipconfig' in Windows and use that Wi-Fi IPv4."
  fi
else
  if [[ -n "${linux_ip}" ]]; then
    echo "Open from iPad: http://${linux_ip}:${PORT}${PAGE_PATH}"
  else
    echo "[WARN] Could not detect Linux IPv4. Check network interface status."
  fi
fi
