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

## Step B: Build wasm artifact

```bash
./scripts/build_wasm.sh
```

Expected output artifact:
- `build-wasm/slam-raylib.html`
- related `.js`, `.wasm`, and preloaded assets

## Step C: Serve and open in Chrome

```bash
python3 -m http.server --directory build-wasm 8080
```

Open:
- `http://localhost:8080/slam-raylib.html`

## Step D: WASM browser E2E smoke (headless)

```bash
chmod +x scripts/wasm_e2e_smoke.sh
./scripts/wasm_e2e_smoke.sh
```

This verifies:
1. page loads in headless Chromium
2. canvas is present
3. no runtime/page console errors are emitted

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
│   └── build_wasm.sh
├── src/
│   ├── app/
│   ├── audio/
│   ├── core/
│   ├── input/
│   ├── render/
│   ├── ui/
│   └── world/
├── tests/
└── tasks/
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
