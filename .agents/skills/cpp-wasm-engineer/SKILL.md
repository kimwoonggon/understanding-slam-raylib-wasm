---
name: cpp-wasm-engineer
description: Build, debug, and optimize C++ WebAssembly targets with Emscripten (browser/worker/node) and WASI runtimes. Use when handling C/C++ to WASM compilation, CMake/Make migration, JS interop, threading, filesystem persistence, size/performance tuning, raylib/SDL/web render loop issues, and hard runtime failures such as traps, undefined symbols, MIME/CORS misconfiguration, or memory/thread edge cases.
---

# C++ WASM Engineer

Drive C++-first WebAssembly delivery with deterministic diagnosis, minimal-risk build settings, and explicit runtime constraints.

## Skill Routing

1. Use this skill for C++/C, CMake, Make, Emscripten, WASI, raylib/SDL ports, and browser runtime debugging.
2. Keep `discover-wasm` as a gateway only; treat this skill as the implementation authority for C++.
3. Route Rust smart-contract specific tasks to `multiversx-wasm-debug`.

## Core Workflow

1. Classify target first.
Choose one: `browser`, `worker`, `node`, `wasi`, or `hybrid`.
2. Pin the toolchain and entrypoints.
Capture exact compiler, linker, and exported symbol list before edits.
3. Build a minimal reproducible artifact.
Reduce to the smallest command or target that still fails.
4. Resolve runtime contract mismatches.
Check environment assumptions: threads, filesystem, MIME, CORS, JS glue, and memory model.
5. Verify with evidence.
Run target-specific smoke tests plus size and symbol checks before closing.

## Target Decision Matrix

| Requirement | Prefer | Reason |
|---|---|---|
| DOM/WebGL/audio/canvas integration | Emscripten | Provides JS glue and browser APIs |
| Portable server/runtime execution without browser APIs | WASI | Lean runtime contract |
| Same core library in browser and backend | Hybrid (Emscripten + WASI) | Share core, split platform adapters |
| Existing desktop CMake app | Emscripten first | Lowest migration friction |
| Strict startup and binary-size budget | Either, benchmark both | Tradeoffs depend on JS glue + libc usage |

## Hard Rules

1. Keep all build flags explicit and version-controlled.
2. Export only required symbols and runtime methods.
3. Fail fast on undefined symbols while iterating.
4. Keep debug and release presets separate; never blend flags ad hoc.
5. Treat browser headers and hosting config as part of the build contract.
6. Document every non-default flag with the symptom it solves.
7. For interactive bug fixes, require executable e2e evidence (keyboard + control-click/tap + drag + audio) before closure.
8. Record each encountered error with symptom/root-cause/fix/evidence in project error reports.

## Load References On Demand

1. Read `references/toolchain-build.md` for CMake/Make/direct builds, export policy, and Emscripten vs WASI setup.
2. Read `references/runtime-interop.md` for JS bridge, event loop, async, filesystem, and pthread runtime constraints.
3. Read `references/debugging-optimization.md` for debug presets, sanitizers, wasm inspection, and size/perf tuning.
4. Read `references/edge-cases.md` for symptom-driven triage and high-frequency failure patterns.
5. Read `references/source-index.md` to trace official sources used by this skill.

## Execution Templates

### Template A: Migrate desktop CMake app to browser WASM

1. Configure with `emcmake cmake`.
2. Build with `cmake --build`.
3. Replace blocking desktop loop with browser-compatible main loop.
4. Declare exports and runtime methods explicitly.
5. Validate in served HTTP context, not `file://`.

### Template B: Fix runtime trap quickly

1. Rebuild with debug preset.
2. Inspect wasm sections and symbols.
3. Map trap to one of: memory, symbol export/import, threading, async boundary, or JS interop contract.
4. Apply one fix at a time; retest after each.
5. Re-run release preset and compare size/perf regression.

### Template C: Prepare production artifact

1. Switch to release preset and run wasm optimizer pass.
2. Audit exports, assets, and filesystem usage.
3. Verify hosting headers for threads and MIME.
4. Run smoke tests in target browsers/runtimes.
5. Record artifact size, startup time, and known constraints.

## Done Criteria

1. Build is reproducible with committed commands.
2. Runtime behavior is validated in intended environment.
3. Edge-case checks for threads, memory, and loader headers are complete.
4. Debug/release presets are documented and separate.
5. Remaining limitations are explicit.
