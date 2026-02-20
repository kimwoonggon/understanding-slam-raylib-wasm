# Debugging, Profiling, and Optimization

## 1. Use Fixed Build Profiles

### Debug profile

```bash
-O0 -gsource-map -sASSERTIONS=2 -sSAFE_HEAP=1 -sSTACK_OVERFLOW_CHECK=2
```

### Release profile

```bash
-O3 -DNDEBUG -sASSERTIONS=0
```

### Size profile

```bash
-Oz -flto -DNDEBUG
```

Never mix random debug and release flags in a single profile.

## 2. Inspect Binary Structure Early

```bash
wasm-objdump -h app.wasm
wasm-objdump -x app.wasm
wasm-objdump -d app.wasm | sed -n '1,120p'
```

Use these to confirm sections, imports, exports, and suspicious instructions (`unreachable`).

## 3. Keep Symbol Visibility Intentional

1. Export only what host code needs.
2. Remove stale exports to reduce attack surface and binary size.
3. Verify exports after each significant link change.

## 4. Use Sanitizers for Native Repro First

1. Reproduce memory or UB issues natively when possible.
2. Run ASan/UBSan builds to isolate root causes before wasm-specific tuning.
3. Use Emscripten sanitizer support when issue reproduces only in wasm.

## 5. Source Maps and Browser Debugging

1. Use `-gsource-map` for browser-readable source mapping.
2. Keep source map paths stable across CI and deploy environments.
3. Verify source map loading in DevTools Network + Sources tabs.

## 6. Size Optimization Sequence

1. Remove unnecessary exports and runtime methods.
2. Disable unused subsystems (for example filesystem if not used).
3. Switch to size-focused flags (`-Oz`, LTO).
4. Run `wasm-opt` and compare size/runtime.
5. Re-check functionality after each optimization step.

Example:

```bash
wasm-opt -Oz app.wasm -o app.opt.wasm
```

## 7. Performance Optimization Sequence

1. Measure startup, frame time, and hot paths first.
2. Use `-O3` or profile-guided strategy where supported.
3. Reduce cross-boundary JS<->WASM chatter in tight loops.
4. Revisit memory-growth policy to avoid repeated realloc spikes.
5. Verify gains with before/after numbers.

## 8. Common Anti-Patterns

1. Tuning `INITIAL_MEMORY` blindly without profiling.
2. Enabling Asyncify globally when only a small call tree needs it.
3. Exporting many debug helpers in production.
4. Ignoring warnings about undefined symbols until runtime.
5. Optimizing size first when runtime is already unstable.

## 9. Evidence to Capture in PR/Report

1. Build command and toolchain versions.
2. Artifact sizes (`.wasm`, `.js`, compressed size if relevant).
3. Startup and steady-state performance numbers.
4. Symbol/export diff before and after.
5. Remaining known limitations.
