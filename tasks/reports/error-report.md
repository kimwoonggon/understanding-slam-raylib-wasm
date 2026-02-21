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

## 2026-02-21

### 5) WASM telemetry toggle false-negative (`failed to enable accumulate mode`)
- Symptom:
  - `scripts/wasm_fps_telemetry.sh` initially failed with `failed to enable accumulate mode`.
- Root cause:
  - Telemetry script used `page.keyboard.press('g')` (very short key pulse), while app logic checks `IsKeyPressed(KEY_G)` per frame; short pulses could be missed in headless timing.
- Fix:
  - Switched telemetry toggle to reliable key hold sequence:
    - `keyboard.down('g')` -> wait -> `keyboard.up('g')`
  - Added retry/wait loop for `accumulateHits` debug-state confirmation.
- Verification:
  - `SAMPLE_SECONDS=20 ./scripts/wasm_fps_telemetry.sh` passed.
  - Output included FPS/hit metrics:
    - `wasm_fps samples=200 min=42.00 p50=55.00 p95=56.00 avg=54.98 max=56.00`
    - `wasm_hits samples=200 start=96 end=4478 max=4478 growth=4382`
