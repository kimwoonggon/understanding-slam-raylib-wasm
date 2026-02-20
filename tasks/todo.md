# Todo

- [x] Scope `ref2` pygame architecture and identify migration units.
- [x] Extract relevant migration patterns from `ref1` worktree.
- [x] Define Raylib C++ module map and build baseline executable.
- [ ] Port SLAM simulation logic with parity checks.
- [x] Add and run native C++ tests for critical behavior.
- [ ] Prepare Emscripten build path for WASM target (include raylib WebAssembly-compatible build/link verification).
- [ ] Validate Chrome execution and document deployment/runtime constraints.

# Review

- Outcome: Native Raylib C++ baseline is modularized and test-backed (core, motion, ui, render, world-loader, audio, native headless e2e). WASM build/e2e scripts and prerequisites are documented.
- Evidence: `ctest --test-dir build --output-on-failure` passes 7/7 tests. `scripts/check_wasm_prereqs.sh` enforces emsdk + wasm-raylib prerequisites.
- Risks/Follow-ups: WASM build/Chrome validation is still blocked in this environment because `emcc`/`emcmake` are not installed yet.
