# WASM Input/Audio Fix Verification (2026-02-20)

## Scope
- Resolve WASM keyboard hold degradation.
- Resolve post-control-click drag lock (`RESET`, `WORLD`, `GREEN`).
- Resolve WASM sound playback failure.
- Validate with executable e2e interactions (not load-only smoke).

## Changes Applied
- `src/app/SlamApp.cpp`
  - Replaced stateful drag lock with per-frame drag decision (`leftDown && !control`).
  - Prioritized keyboard motion path when key intent exists.
  - Added WASM debug state export (`window.__slamDebug`) for pose/input/audio observability.
  - Added early sound-asset presence detection and safer audio-init trigger conditions.
- `src/app/SlamApp.h`
  - Updated debug method signature and audio/asset state members.
- `src/app/AssetPaths.cpp`
  - Added WASM-first absolute asset path candidate (`/assets/...`).
- `CMakeLists.txt`
  - Added Emscripten runtime export for audio path compatibility:
    - `-sEXPORTED_RUNTIME_METHODS=['HEAPF32']`
- `scripts/wasm_e2e_smoke.sh`
  - Added keyboard hold interaction checks.
  - Added key-release verification.
  - Added VFS asset presence verification.
  - Added post-input audio readiness verification.
  - Added control click/tap then mouse/touch drag continuity verification.
  - Added canvas coordinate-to-viewport transform for reliable pointer actions.

## Validation Commands and Results
1. `cmake --build build-release -j`
   - PASS
2. `ctest --test-dir build-release --output-on-failure`
   - PASS (9/9)
3. `./scripts/build_wasm.sh`
   - PASS
4. `./scripts/wasm_e2e_smoke.sh`
   - PASS
   - Output: `[OK] wasm e2e smoke passed (canvas, runtime, VFS assets, keyboard hold, mouse/touch drag-after-controls, and audio checks).`

## Error Traceability
- Detailed error history and root-cause/fix mapping is tracked in:
  - `tasks/reports/error-report.md`
