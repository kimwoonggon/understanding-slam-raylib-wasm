#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REF2_ROOT="${REF2_ROOT:-${ROOT_DIR}/../understanding-slam-raylib-wasm-ref2}"
TIMESTAMP="$(date +%Y%m%d-%H%M%S)"
REPORT_DIR="${ROOT_DIR}/tasks/reports"
LOG_DIR="${REPORT_DIR}/logs/${TIMESTAMP}"
REPORT_FILE="${REPORT_DIR}/e2e-triad-${TIMESTAMP}.md"

mkdir -p "${LOG_DIR}"

cat > "${REPORT_FILE}" <<EOF
# E2E Triad Report (${TIMESTAMP})

- Project: \`${ROOT_DIR}\`
- ref2: \`${REF2_ROOT}\`
- Input sequence: \`tests/data/e2e_wasd_sequence.txt\`
- World grid: \`tests/data/world_grid_120x80.txt\`

## Step Results

| Step | Status | Log |
|---|---|---|
EOF

run_step() {
  local name="$1"
  local cmd="$2"
  local log_file="${LOG_DIR}/${name}.log"
  echo "[RUN] ${name}"
  set +e
  bash -lc "cd '${ROOT_DIR}' && ${cmd}" > "${log_file}" 2>&1
  local rc=$?
  set -e
  if [[ ${rc} -eq 0 ]]; then
    echo "| ${name} | PASS | \`${log_file}\` |" >> "${REPORT_FILE}"
    echo "[OK] ${name}"
  else
    echo "| ${name} | FAIL (${rc}) | \`${log_file}\` |" >> "${REPORT_FILE}"
    echo "[FAIL] ${name}. See ${log_file}" >&2
    tail -n 60 "${log_file}" >&2 || true
    return ${rc}
  fi
}

run_step "native_build_and_ctest" \
  "cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release && cmake --build build-release -j 8 && ctest --test-dir build-release --output-on-failure"

run_step "setup_raylib_wasm" \
  "./scripts/setup_raylib_wasm.sh"

run_step "build_wasm_app" \
  "./scripts/build_wasm.sh"

run_step "wasm_e2e_smoke" \
  "./scripts/wasm_e2e_smoke.sh"

run_step "build_wasm_diff_trace" \
  "./scripts/build_wasm_diff_trace.sh"

run_step "triad_compare" \
  "uv run --project '${REF2_ROOT}' python '${ROOT_DIR}/scripts/e2e_compare_ref2_vs_raylib.py' --cpp-trace-exe '${ROOT_DIR}/build-release/slam-diff-trace' --ref2-root '${REF2_ROOT}' --world-grid '${ROOT_DIR}/tests/data/world_grid_120x80.txt' --wasm-trace-js '${ROOT_DIR}/build-wasm-trace/slam-diff-trace.js'"

{
  echo
  echo "## Triad Compare Output"
  echo
  echo '```text'
  cat "${LOG_DIR}/triad_compare.log"
  echo '```'
} >> "${REPORT_FILE}"

echo
echo "[OK] Triad E2E pipeline complete."
echo "[INFO] Report: ${REPORT_FILE}"
