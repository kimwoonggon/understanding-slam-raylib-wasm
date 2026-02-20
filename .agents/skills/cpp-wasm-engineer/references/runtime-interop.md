# Runtime and Interop (Browser/Worker/Node/WASI)

## Table of Contents

1. Pick Interop Layer Deliberately
2. Main Loop Contract
3. Async Boundary Rules
4. Filesystem Modes
5. Threading Contract
6. Loader and MIME Rules
7. Numeric Boundary: i64 and JS
8. Environment Scoping
9. Dynamic Loading and Modules
10. Runtime Verification Checklist

## 1. Pick Interop Layer Deliberately

1. Use `ccall/cwrap` for small C-style entrypoints.
2. Use Embind for rich C++ object APIs with lifecycle mapping.
3. Use `EM_JS`/`EM_ASM` for tight, low-overhead JS bridge points.
4. Keep one interop style per module unless there is a hard reason to mix.

## 2. Main Loop Contract

1. Do not run desktop-style infinite loops in the browser main thread.
2. Use Emscripten main-loop APIs so rendering cooperates with browser scheduling.
3. Keep each frame deterministic and bounded.

Pattern:

```cpp
#include <emscripten.h>

void frame() {
  // update + render
}

int main() {
  emscripten_set_main_loop(frame, 0, 1);
  return 0;
}
```

## 3. Async Boundary Rules

1. Assume C++ code is synchronous by default.
2. Use `-sASYNCIFY` only when blocking semantics are required across async JS APIs.
3. Restrict Asyncify scope where possible; it increases code size and runtime cost.
4. Re-test performance after enabling Asyncify.

## 4. Filesystem Modes

1. `MEMFS`: ephemeral, in-memory default.
2. `IDBFS`: persistent browser storage with explicit `syncfs`.
3. `NODEFS`: node-backed filesystem bridge for Node.js use cases.
4. `WasmFS`: modern filesystem implementation for multithreaded scenarios.

IDBFS persistence requires explicit sync:

```javascript
FS.mkdir('/persist');
FS.mount(IDBFS, {}, '/persist');
FS.syncfs(true, (err) => {
  if (err) console.error(err);
});
```

## 5. Threading Contract

1. Compile and link with `-pthread`.
2. Configure worker strategy (`-sPTHREAD_POOL_SIZE=...`) as needed.
3. Require cross-origin isolation for `SharedArrayBuffer` in browsers.
4. Confirm hosting sends:
   - `Cross-Origin-Opener-Policy: same-origin`
   - `Cross-Origin-Embedder-Policy: require-corp`

Common optional pattern:

```bash
-pthread -sPTHREAD_POOL_SIZE=4 -sPROXY_TO_PTHREAD=1
```

## 6. Loader and MIME Rules

1. Serve `.wasm` with `application/wasm` for streaming compilation.
2. Use HTTP(S), not `file://`, for realistic browser behavior.
3. Validate CORS if wasm/js/assets are on different origins.
4. Ensure JS loader points to the deployed wasm path.

## 7. Numeric Boundary: i64 and JS

1. Treat 64-bit values crossing JS boundaries as a special case.
2. Use BigInt-capable bindings or explicit split/high-low handling.
3. Avoid silent truncation in JS number conversions.

## 8. Environment Scoping

Use `-sENVIRONMENT=` to narrow deployment runtime when appropriate.
Examples:

```bash
-sENVIRONMENT=web,worker
-sENVIRONMENT=node
```

## 9. Dynamic Loading and Modules

1. Keep module options (`MODULARIZE`, `EXPORT_ES6`, etc.) consistent with bundler strategy.
2. Audit startup contract between generated JS wrapper and host code.
3. Validate worker script paths and asset URLs after bundling.

## 10. Runtime Verification Checklist

1. Confirm frame loop runs at expected cadence.
2. Confirm thread workers actually spawn.
3. Confirm persistent files survive reload.
4. Confirm JS <-> C++ calls preserve expected types.
5. Confirm wasm loads via correct MIME and origin policy.
6. Confirm keyboard hold remains active during press and resets after keyup.
7. Confirm UI control click/tap does not lock drag input (`mouse` and `touch` paths).
8. In browser automation, map canvas-internal coordinates through `getBoundingClientRect()` before pointer/touch injection.
9. Confirm audio initializes from a real user gesture and no runtime-method export errors appear in console.
