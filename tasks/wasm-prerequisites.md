# WASM Prerequisites (Blocking)

1. Activate emsdk (`emsdk_env`) and verify `emcc`, `em++`, `emcmake` are available.
2. Ensure raylib is built or provided for Emscripten/WebAssembly target.
3. Do not reuse desktop/macOS raylib artifacts for WASM builds.
4. Verify CMake/Make links against WASM-compatible raylib before packaging.
5. Treat this checklist as blocking before Chrome runtime validation.
