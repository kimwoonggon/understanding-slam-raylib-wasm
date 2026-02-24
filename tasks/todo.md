# Todo

- [x] Scope `ref2` pygame architecture and identify migration units.
- [x] Extract relevant migration patterns from `ref1` worktree.
- [x] Define Raylib C++ module map and build baseline executable.
- [ ] Port SLAM simulation logic with parity checks.
- [x] Add and run native C++ tests for critical behavior.
- [ ] Prepare Emscripten build path for WASM target (include raylib WebAssembly-compatible build/link verification).
- [ ] Validate Chrome execution and document deployment/runtime constraints.
- [x] Investigate frame-drop root cause when green dots accumulate (`feat/why-framedrops`).
- [x] Verify hypothesis with runnable evidence (profiling/instrumentation).
- [x] Document root cause and mitigation options.
- [x] Implement accumulation optimization (O(1) dedup + render-texture caching).
- [x] Record before/after performance status with reproducible measurements.
- [x] Run WASM Chrome smoke validation after FPS optimization.
- [x] Expose WASM debug telemetry fields for FPS/hit accumulation.
- [x] Add and run long-session WASM FPS telemetry script (accumulate mode).
- [x] Fix native Debug build break (`<numbers>` header + raylib static link deps) and verify `cmake --build build -j` passes.
- [x] Improve WASM scripts to auto-discover/activate emsdk without manual sourcing; update README commands accordingly.
- [x] Add one-command WASM environment bootstrap (emsdk + raylib-wasm) and one-command local Chrome run flow; document both in README.
- [x] Add a dedicated markdown guide explaining Linux IP vs Windows LAN IP vs WSL networking path for iPad access.

# Review

- Outcome: Native Raylib C++ baseline is modularized and test-backed (core, motion, ui, render, world-loader, audio, native headless e2e). WASM build/e2e scripts and prerequisites are documented.
- Evidence: `ctest --test-dir build-debug --output-on-failure` passes 9/9 tests. `./scripts/build_wasm.sh` succeeded. `./scripts/wasm_e2e_smoke.sh` passed with canvas/runtime/VFS/input/drag/audio checks. `./scripts/wasm_fps_telemetry.sh` captured long-session FPS/hit telemetry.
- Risks/Follow-ups: Root-cause and optimization are validated for native benchmarks and WASM functional smoke, including long accumulate-session telemetry.

## FPS Root-Cause Record (feat/why-framedrops)

- Root code path:
  - Pre-fix `SlamApp::UpdateScan` merged hits into `hitHistory_` every frame in accumulate mode.
  - Pre-fix `SlamApp::DrawFrame` drew every stored hit each frame (`DrawCircleV` over full `hitHistory_`).
  - Pre-fix `render::UpdateHitPointHistory` copied full history (`std::vector<Vector2> merged = history`) on each merge call.
  - Pre-fix `render::UpdateHitPointHistory` performed linear duplicate lookup (`std::any_of`) for each current hit.
- Root cause:
  - `hitHistory_` grows over time in accumulate mode, so frame work scales with history size.
  - Rendering cost grows roughly linearly with hit count (all dots drawn every frame).
  - Merge cost adds extra CPU overhead from repeated full-history copy + linear duplicate checks.
- Measured evidence:
  - Draw benchmark (`map + rays + hits`) average frame time:
    - `map+rays`: ~1.36 ms/frame
    - `+5k hits`: ~7.32 ms/frame
    - `+20k hits`: ~24.63 ms/frame
    - `+40k hits`: ~47.39 ms/frame
  - At 60 FPS budget (16.67 ms/frame), hit history above ~20k causes expected FPS collapse.

## FPS Optimization Status (feat/why-framedrops)

- Before:
  - `map+rays`: ~1.36 ms/frame
  - `map+rays+5k hits`: ~7.32 ms/frame
  - `map+rays+20k hits`: ~24.63 ms/frame
  - `map+rays+40k hits`: ~47.39 ms/frame
- After:
  - `map+rays+5k hits (layer)`: ~1.47 ms/frame
  - `map+rays+20k hits (layer)`: ~1.45 ms/frame
  - `map+rays+40k hits (layer)`: ~1.46 ms/frame
  - Dedup insert cost (`TryMarkHitPixel`) remained ~0.0003 ms/call from 1k to 40k prefilled points.

## WASM FPS Telemetry (feat/why-framedrops)

- Command: `SAMPLE_SECONDS=20 ./scripts/wasm_fps_telemetry.sh`
- Session metrics:
  - `wasm_fps`: `samples=200 min=42.00 p50=55.00 p95=56.00 avg=54.98 max=56.00`
  - `wasm_hits`: `samples=200 start=96 end=4478 max=4478 growth=4382`
- Interpretation:
  - In 20-second accumulate session, hit-history grows substantially while FPS remains near target in this headless Chromium environment.
