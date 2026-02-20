# Toolchain and Build (C++ -> WASM)

## Table of Contents

1. Choose Runtime Contract First
2. Verify Toolchain Quickly
3. Emscripten Build Patterns
4. Baseline Build Presets
5. Export Policy
6. Memory and Stack Defaults to Revisit
7. Dynamic Linking (Advanced)
8. WASI Build Pattern
9. raylib/Web Render-Loop Notes
10. Build Review Checklist

## 1. Choose Runtime Contract First

1. Choose `Emscripten` for browser-facing apps using DOM/WebGL/audio/input.
2. Choose `WASI` for runtime/server workloads that do not need browser APIs.
3. Split platform adapters when shipping both.

## 2. Verify Toolchain Quickly

```bash
emcc -v
em++ -v
emcmake --version
wasm-objdump --version
wasm-opt --version
```

For WASI:

```bash
clang++ --version
echo "$WASI_SDK_PATH"
```

## 3. Emscripten Build Patterns

### CMake

```bash
emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-wasm -j
```

### Make

```bash
emmake make -j
```

### Direct compile/link

```bash
em++ src/main.cpp -std=c++20 -O2 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sEXPORTED_FUNCTIONS='["_main"]' \
  -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -o dist/app.js
```

## 4. Baseline Build Presets

### Debug preset

```bash
-O0 -gsource-map -sASSERTIONS=2 -sSAFE_HEAP=1 -sSTACK_OVERFLOW_CHECK=2
```

### Release preset

```bash
-O3 -DNDEBUG -sASSERTIONS=0
```

### Size-first release preset

```bash
-Oz -flto -DNDEBUG
```

## 5. Export Policy

1. Keep C ABI exports inside `extern "C"` blocks when called from JS by name.
2. Use `EMSCRIPTEN_KEEPALIVE` for functions that may be removed by dead-code elimination.
3. Pass only needed exports via `-sEXPORTED_FUNCTIONS`.
4. Remember export names in `EXPORTED_FUNCTIONS` are prefixed with `_`.

Minimal example:

```cpp
#include <emscripten/emscripten.h>

extern "C" {
EMSCRIPTEN_KEEPALIVE int add(int a, int b) { return a + b; }
}
```

```bash
em++ add.cpp -O2 \
  -sEXPORTED_FUNCTIONS='["_add"]' \
  -sEXPORTED_RUNTIME_METHODS='["ccall"]' \
  -o add.js
```

## 6. Memory and Stack Defaults to Revisit

1. Start with `-sALLOW_MEMORY_GROWTH=1` for uncertain workloads.
2. Pin `-sINITIAL_MEMORY=<bytes>` once memory profile is known.
3. Set `-sMAXIMUM_MEMORY=<bytes>` when host policy requires a cap.
4. Raise `-sSTACK_SIZE=<bytes>` only when stack evidence requires it.

## 7. Dynamic Linking (Advanced)

1. Use `-sMAIN_MODULE=1` for the main wasm.
2. Use `-sSIDE_MODULE=1` for loadable side modules.
3. Keep ABI and link flags aligned between main and side modules.
4. Audit imports/exports with `wasm-objdump -x` before runtime tests.

## 8. WASI Build Pattern

```bash
clang++ --target=wasm32-wasi \
  --sysroot "$WASI_SDK_PATH/share/wasi-sysroot" \
  -O2 -std=c++20 src/main.cpp -o app.wasm
```

Optional explicit export:

```bash
-Wl,--export=run
```

## 9. raylib/Web Render-Loop Notes

1. Replace desktop infinite loops with browser-compatible frame callbacks.
2. Keep per-frame work bounded; browsers schedule frames, not your `while(true)`.
3. Validate input/audio/resource paths under hosted HTTP, not local file paths.

## 10. Build Review Checklist

1. Confirm exact command lines are reproducible.
2. Confirm target runtime (`web/worker/node/wasi`) is explicit.
3. Confirm exported functions/runtime methods are minimal.
4. Confirm debug and release presets are not mixed.
5. Confirm `.wasm` and loader assets are deployed together.
