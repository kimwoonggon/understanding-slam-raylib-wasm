# Understanding SLAM with Raylib C++ (+ WASM Plan)

This repository migrates `ref2` (pygame SLAM simulation) to **Raylib C++** first, then to **WebAssembly (Chrome)**.

## 1. Project Goal

1. Port pygame SLAM behavior to native C++/Raylib with parity.
2. Harden behavior with deterministic tests.
3. Build and run the same project as WebAssembly in Chrome.

Reference repositories:
- `ref1`: pygame -> Raylib migration pattern reference
- `ref2`: source pygame SLAM behavior/spec reference

## 2. Current Runtime Features (Native)

- 2D grid world simulation with obstacles
- Simulated lidar ray casting
- Occupancy-grid integration
- Rendering palette aligned with ref2 style
  - black background
  - red laser rays
  - green hit points
- Controls
  - move: `W/A/S/D` or arrow keys
  - mouse drag: hold left mouse to reposition robot (wall collision respected)
  - reset map: `I`
  - toggle world/map: `M`
  - hit mode live/accumulate: `G`
  - lock/hide mouse cursor: `P` (browser pointer-lock aware)
- Asset loading
  - world: `assets/maze.png` if present, otherwise demo world fallback
  - sound: `assets/sounds/scan_loop.wav`, `assets/sounds/collision_beep.wav` (if present)

## 3. Prerequisites

## Native C++/Raylib

- CMake 3.20+
- C++20 compiler (AppleClang/Clang/GCC)
- raylib (desktop build)

macOS (Homebrew example):
```bash
brew install raylib
```

## WASM (Emscripten)

- emsdk activated (`emcc`, `emcmake` available)
- **raylib built for Emscripten/WebAssembly**
  - desktop raylib binary cannot be reused for WASM
- optional env var:
  - `RAYLIB_WASM_ROOT=/path/to/raylib-wasm-install`
  - if omitted, scripts auto-detect local `.toolchains/raylib-wasm` when present

## 4. Native Build and Run

## Configure + Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## Run GUI app

```bash
./build/slam-raylib
```

## Headless runtime smoke (E2E)

Runs end-to-end pipeline without opening a window:
```bash
SLAM_HEADLESS_STEPS=120 ./build/slam-raylib
```

Exit code `0` means smoke passed.

## 5. Testing

All tests:
```bash
ctest --test-dir build --output-on-failure
```

Individual targets:
```bash
ctest --test-dir build -R slam-core-tests --output-on-failure
ctest --test-dir build -R slam-motion-tests --output-on-failure
ctest --test-dir build -R slam-ui-tests --output-on-failure
ctest --test-dir build -R slam-render-tests --output-on-failure
ctest --test-dir build -R slam-world-loader-tests --output-on-failure
ctest --test-dir build -R slam-audio-tests --output-on-failure
ctest --test-dir build -R slam-native-e2e --output-on-failure
```

Test coverage currently includes:
- core SLAM math/model behavior
- motion and drag collision behavior
- UI geometry and reset triggers
- rendering coordinate conversion + hit-history mode
- image-based world loading threshold behavior
- audio loop/cooldown controller behavior
- native headless E2E smoke

## Differential E2E: `ref2` pygame vs Raylib C++

This runs both engines with the same WASD input sequence and compares per-frame pixel-change behavior.
Both sides use the same world-grid file for deterministic map parity:
- `tests/data/world_grid_120x80.txt`

Build trace binary:
```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j --target slam-diff-trace
```

Run differential compare:
```bash
uv run --project ../understanding-slam-raylib-wasm-ref2 \
  python scripts/e2e_compare_ref2_vs_raylib.py \
  --cpp-trace-exe build-release/slam-diff-trace \
  --ref2-root ../understanding-slam-raylib-wasm-ref2 \
  --world-grid tests/data/world_grid_120x80.txt
```

Default pass criteria:
- pose absolute tolerance: `1e-6`
- per-frame changed-pixel diff tolerance: `40`

Inputs are loaded from:
- `tests/data/e2e_wasd_sequence.txt`

## Triad E2E (WASM + Native + ref2)

Build wasm/node trace:
```bash
./scripts/build_wasm_diff_trace.sh
```

Run triad compare:
```bash
uv run --project ../understanding-slam-raylib-wasm-ref2 \
  python scripts/e2e_compare_ref2_vs_raylib.py \
  --cpp-trace-exe build-release/slam-diff-trace \
  --ref2-root ../understanding-slam-raylib-wasm-ref2 \
  --world-grid tests/data/world_grid_120x80.txt \
  --wasm-trace-js build-wasm-trace/slam-diff-trace.js
```

Full triad pipeline + report generation:
```bash
./scripts/run_triad_e2e.sh
```

Generated reports:
- `tasks/reports/e2e-triad-*.md`
- detailed logs under `tasks/reports/logs/*`

## 6. Debugging Guide

## Debug build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## Run with LLDB (macOS)

```bash
lldb ./build/slam-raylib
(lldb) run
```

## Investigate a specific failing test

```bash
ctest --test-dir build -R slam-render-tests --output-on-failure
```

## 7. WebAssembly Build and Run

## Step A: Check prerequisites

```bash
./scripts/check_wasm_prereqs.sh
```

If raylib-wasm is not prepared yet:
```bash
./scripts/setup_raylib_wasm.sh
```

## Step B: Build wasm artifact

```bash
./scripts/build_wasm.sh
```

Expected output artifact:
- `build-wasm/slam-raylib.html`
- related `.js`, `.wasm`, and preloaded assets

## Step C: Serve and open in Chrome

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target slam-static-server -j
./build-release/slam-static-server --root build-wasm --port 8080
```

Open:
- `http://localhost:8080/slam-raylib.html`

Phone/LAN access:
```bash
./build-release/slam-static-server --root build-wasm --host 0.0.0.0 --port 8080
```
Then open `http://<your-computer-ip>:8080/slam-raylib.html` from phone on same network.

## WASM UI Customization

Primary file:
- `web/emshell.html`

Where to modify:
- "Powered by Emscripten" replacement text:
  - `web/emshell.html` in `#status` content
- Canvas size policy:
  - `web/emshell.html` CSS variables `--canvas-width`, `--canvas-height`
  - `web/emshell.html` JS flag `AUTO_RESIZE_CANVAS`
- Mouse lock/hide behavior:
  - runtime hotkey logic in `src/app/SlamApp.cpp` (`KEY_P`)
  - pointer-lock request/release bridge in the same file under `#ifdef EMSCRIPTEN`

## Step D: WASM browser E2E smoke (headless)

```bash
chmod +x scripts/wasm_e2e_smoke.sh
./scripts/wasm_e2e_smoke.sh
```

This verifies:
1. page loads in headless Chromium
2. canvas is present
3. WASM VFS contains map/sound assets
4. keyboard hold works (`W/A/S/D`) and key-release state recovers
5. drag continues to work after clicking/tapping `RESET`, `WORLD`, `GREEN` controls
6. audio runtime initializes without browser-side aborts
7. no runtime/page console errors are emitted

## Important WASM Notes

1. Use emsdk shell/environment for all wasm commands.
2. Ensure raylib is wasm-compatible (do not link desktop `.dylib/.a` built for macOS target).
3. Verify server serves `.wasm` as `application/wasm`.
4. Run from HTTP(S), not `file://`.

## 8. Repository Structure

```text
.
├── CMakeLists.txt
├── README.md
├── assets/
│   ├── maze.png
│   └── sounds/
├── scripts/
│   ├── check_wasm_prereqs.sh
│   ├── build_wasm.sh
│   ├── build_wasm_diff_trace.sh
│   ├── setup_raylib_wasm.sh
│   ├── wasm_e2e_smoke.sh
│   ├── e2e_compare_ref2_vs_raylib.py
│   └── run_triad_e2e.sh
├── src/
│   ├── app/
│   ├── audio/
│   ├── core/
│   ├── input/
│   ├── render/
│   ├── tools/
│   ├── ui/
│   └── world/
├── web/
│   └── emshell.html
├── tests/
│   └── data/
└── tasks/
    └── reports/
```

## 9. Troubleshooting

## `emcc` not found

Activate emsdk first:
```bash
source /path/to/emsdk/emsdk_env.sh
```

## raylib not found during native build

Install desktop raylib and retry configure.

## raylib not found during wasm build

Set wasm raylib location:
```bash
export RAYLIB_WASM_ROOT=/path/to/raylib-wasm-install
```
or bootstrap and auto-detect:
```bash
./scripts/setup_raylib_wasm.sh
```

## App runs but no map/sound

Verify files exist:
- `assets/maze.png`
- `assets/sounds/scan_loop.wav`
- `assets/sounds/collision_beep.wav`

## 10. Engineering Workflow

- TDD-first for each behavior slice (Red -> Green -> Refactor)
- Deterministic C++ tests via CTest
- Native E2E smoke in headless mode
- WASM prerequisites treated as blocking gate before browser validation

## 11. Clean Artifacts

Remove local build artifacts:
```bash
rm -rf build build-* build-wasm build-wasm-trace
```

Remove local wasm toolchains/cache cloned by scripts:
```bash
rm -rf .toolchains .third_party
```

Remove local node artifacts:
```bash
rm -rf node_modules package-lock.json package.json
```
