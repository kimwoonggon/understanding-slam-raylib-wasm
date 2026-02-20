# Edge-Case Playbook (C++ WASM)

Use this file as symptom-first triage. Validate one hypothesis at a time.

## 1. Loader and Hosting Failures

| Symptom | Likely Cause | Actions |
|---|---|---|
| `TypeError: WebAssembly.instantiate(): expected magic word` | Wrong file served (HTML or error page instead of wasm) | Check Network response body and status for wasm URL; fix routing/static asset path |
| Streaming compile path not used | Wrong MIME type for `.wasm` | Serve with `application/wasm`; verify response headers |
| Works locally, fails in production CDN | CORS/header mismatch | Validate `Access-Control-*`, COOP/COEP, and cache behavior |
| App fails only on `file://` | Browser security model | Serve through local HTTP server and retest |

## 2. Symbol and Linking Failures

| Symptom | Likely Cause | Actions |
|---|---|---|
| `undefined symbol` at link or runtime | Missing library or stripped symbol | Enable strict undefined-symbol checks; inspect imports/exports with `wasm-objdump -x` |
| JS cannot call expected C++ function | Export not preserved | Add `EMSCRIPTEN_KEEPALIVE` and include `_<name>` in `EXPORTED_FUNCTIONS` |
| Build succeeds, runtime traps at first call | ABI mismatch at boundary | Ensure `extern "C"` and argument type compatibility with binding layer |
| Side module load fails | MAIN/SIDE module mismatch | Align module flags and ABI expectations between artifacts |

## 3. Memory and Stack Failures

| Symptom | Likely Cause | Actions |
|---|---|---|
| `memory access out of bounds` | Heap growth assumptions wrong or bug in indexing | Rebuild debug profile; check bounds; review memory settings and growth |
| Random crash under load | Stack exhaustion | Increase `STACK_SIZE` with evidence; reduce recursion and large stack locals |
| Large-scene startup failure | `INITIAL_MEMORY` too low | Raise initial memory or enable growth; profile real high-water mark |
| Severe frame spikes later in session | Repeated growth/realloc pressure | Tune initial/max memory and allocation patterns |

## 4. Threading and Worker Failures

| Symptom | Likely Cause | Actions |
|---|---|---|
| `pthread_create failed` | Missing thread support or browser isolation | Confirm `-pthread`, worker setup, and COOP/COEP headers |
| Works single-threaded only | SharedArrayBuffer unavailable | Verify `crossOriginIsolated` and third-party resource compatibility |
| Worker script 404 after bundling | Asset path rewrite | Pin worker URL strategy in bundler/deploy pipeline |
| Deadlock-like freeze | Main-thread blocking or lock misuse | Audit lock ordering; avoid blocking calls on browser main thread |

## 5. Async and Event-Loop Failures

| Symptom | Likely Cause | Actions |
|---|---|---|
| UI freezes during blocking call | Desktop blocking model in browser thread | Move work to worker or Asyncify-required boundaries |
| Logic breaks after enabling Asyncify | Over-broad async transformation | Restrict Asyncify scope and retest |
| Frame pacing unstable | Wrong main loop integration | Use browser-managed loop API and cap per-frame workload |

## 6. Filesystem and Asset Failures

| Symptom | Likely Cause | Actions |
|---|---|---|
| Save data disappears after refresh | Using MEMFS only | Mount IDBFS and call `FS.syncfs` load/save paths explicitly |
| Asset load path fails in deploy subdir | Hard-coded absolute paths | Normalize runtime path handling relative to deployment root |
| Works in Node, fails in browser | Filesystem backend mismatch | Separate Node-backed and browser-backed FS assumptions |

## 7. Numeric and Interop Failures

| Symptom | Likely Cause | Actions |
|---|---|---|
| 64-bit values become inaccurate in JS | i64 conversion mismatch | Use BigInt-safe boundary handling or split representation |
| Struct/class interop returns garbage | Binding layout mismatch | Recheck embind signatures and ownership/lifetime policy |
| Callback crashes after some time | Captured object lifetime bug | Validate ownership and callback deregistration strategy |

## 8. Graphics and Platform Porting Failures

| Symptom | Likely Cause | Actions |
|---|---|---|
| Desktop render loop ports poorly to web | Event-loop model mismatch | Refactor to frame callback model and non-blocking I/O |
| Input/audio timing differs from desktop | Browser policy and scheduling differences | Re-test focus/gesture-triggered APIs and timing assumptions |
| Unexpected GPU behavior across browsers | Driver/browser variance | Reproduce on multiple engines; reduce undefined GL usage |

## 9. Release-Only Regressions

| Symptom | Likely Cause | Actions |
|---|---|---|
| Debug works, release fails | UB exposed by optimization | Run UBSan/ASan paths, narrow optimization level, isolate offending unit |
| Release artifact too large | Excess exports/debug/runtime features | Trim exports, disable unused systems, run `wasm-opt -Oz` |
| Startup slower after size pass | Over-aggressive transform tradeoff | Compare `-O3` vs `-Oz`; choose based on measured user impact |

## 10. Triage Order (Always Follow)

1. Confirm network/headers/loader correctness.
2. Confirm import/export and symbol contract.
3. Confirm memory/stack/thread settings.
4. Confirm async/event-loop model.
5. Confirm filesystem and asset paths.
6. Confirm optimization did not hide UB.
