# Error Report

## 2026-02-20

### 1) WASM audio runtime abort (`HEAPF32` not exported)
- Symptom:
  - Browser console error spam: `Aborted('HEAPF32' was not exported. add it to EXPORTED_RUNTIME_METHODS ...)`
  - User-facing impact: sound not playing in WASM.
- Root cause:
  - Emscripten runtime method export set did not include `HEAPF32`, but raylib/miniaudio web path required it.
- Fix:
  - Updated `CMakeLists.txt` Emscripten link options:
    - `-sEXPORTED_RUNTIME_METHODS=['HEAPF32']`
- Verification:
  - `./scripts/build_wasm.sh` succeeded.
  - `./scripts/wasm_e2e_smoke.sh` passed with audio checks enabled.

### 2) WASM e2e debug-state wait timeout
- Symptom:
  - `page.waitForFunction(... __slamDebug ...)` timed out.
- Root cause:
  - Build and e2e were run in parallel in one step, causing unstable timing/artifact race.
- Fix:
  - Enforced sequential verification flow:
    1. `./scripts/build_wasm.sh`
    2. `./scripts/wasm_e2e_smoke.sh`
- Verification:
  - Sequential run now consistently passes.

### 3) Drag e2e false-negative (`Baseline drag movement too small`)
- Symptom:
  - e2e reported `Baseline drag movement too small: {"delta":0}` despite app being interactive.
- Root cause:
  - Test used raw internal canvas coordinates as viewport coordinates (missed canvas offset/scaling).
- Fix:
  - Added canvas bounding-box based coordinate transform in `scripts/wasm_e2e_smoke.sh`.
- Verification:
  - Drag checks now pass, including post-control click drag checks.

### 4) Local server bind failures (`Address already in use`)
- Symptom:
  - `bind() failed: Address already in use`, intermittent connection refused in ad-hoc checks.
- Root cause:
  - Prior background server process still listening on test port.
- Fix:
  - Pre-test port cleanup added to test execution flow.
- Verification:
  - Stable server startup during final verification runs.
