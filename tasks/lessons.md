# Lessons

## 2026-02-20

- Correction: Parallelizable work should aggressively use subagent-style parallel execution.
- Rule to prevent recurrence: For file discovery, multi-file reads, and independent verification commands, default to parallel tool execution and split tasks into independent lanes before coding.
- Applied in: C++ Raylib baseline setup (module-map extraction, file generation checks, and build verification lanes).
- Correction: Implementation must explicitly follow `test-driven-development` and `cpp-testing` skills.
- Rule to prevent recurrence: Start each C++ feature with failing tests, verify RED state, implement minimal GREEN change, then refactor and re-run full test target with CTest/gtest filters.
- Applied in: Core SLAM C++ baseline (`slam-core-tests`) before completing app/runtime wiring.
- Correction: Runtime-critical assets (map/audio) must be copied and wired early, not deferred.
- Rule to prevent recurrence: During baseline setup, create `assets/` immediately and verify required runtime files (`maze.png`, sound files) are present before claiming runnable status.
- Applied in: Raylib baseline initialization and asset bootstrap.
- Correction: Execute migration in strict TDD, one file at a time.
- Rule to prevent recurrence: For each new module, run Red (add failing test), Green (minimal implementation), and Refactor before touching the next file.
- Applied in: Upcoming SLAM parity port steps starting with input/motion extraction.
- Correction: TDD granularity can be function-level, not only file-level.
- Rule to prevent recurrence: Prefer the smallest meaningful behavior unit (function or small interaction) for Red→Green cycles to keep momentum and reduce batch size.
- Applied in: Ongoing SLAM port (motion, UI, rendering, audio behavior slices).
- Correction: Raylib WASM requires a WebAssembly-compatible raylib build/toolchain; desktop raylib binaries are not interchangeable.
- Rule to prevent recurrence: Before WASM packaging, explicitly validate emsdk activation and raylib web build linkage (or Emscripten-ready raylib package) as a blocking prerequisite.
- Applied in: WASM phase planning and upcoming Chrome execution pipeline.
- Correction: README must contain detailed run/build/test/debug/WASM instructions, similar in depth to ref2.
- Rule to prevent recurrence: After each major milestone, update README with exact commands and troubleshooting notes before marking the step complete.
- Applied in: Native+WASM baseline documentation for this repository.
