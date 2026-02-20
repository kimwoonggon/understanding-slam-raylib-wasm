# ref1 Migration Patterns Reusable for SLAM Port

## Goal

Extract reusable engineering patterns from `ref1` (pygame -> Raylib C++ success case) and map them to the `ref2` SLAM migration plan.

## Observed Patterns in ref1

## Pattern 1: Thin Entry Point + Central Config

What ref1 does:
1. `src/main.cpp` only parses runtime options and constructs `app::Config`.
2. Real logic starts in `app::App`.

Reuse for SLAM:
1. Keep `main.cpp` minimal.
2. Put SLAM tunables (world size, lidar beams/range, cell size, debug flags, audio toggles) into one config struct.
3. Avoid scattering constants across render/input files.

## Pattern 2: App Orchestrator with Split Responsibilities

What ref1 does:
1. `App` is split across focused compilation units:
- `App.Init.cpp`
- `App.Physics.cpp`
- `App.Render.cpp`
- `App.Ai.cpp`
2. Run loop order is stable: input -> update -> draw.

Reuse for SLAM:
1. Implement `SlamApp` orchestration class.
2. Split into `Init`, `Input`, `Update`, `Render`, and optional `Wasm` adapter units.
3. Keep event handling out of core SLAM math.

## Pattern 3: Domain Logic Decoupled from Rendering Backend

What ref1 does:
1. `game::Game` owns gameplay state transitions and physics independent from draw code.
2. Collision math and data structures are in dedicated domain modules.

Reuse for SLAM:
1. Keep `WorldGrid`, `SimulatedLidar`, and `OccupancyGridMap` backend-agnostic.
2. Ensure these modules compile and test without Raylib window creation.
3. Treat rendering as a projection layer over domain state.

## Pattern 4: RAII Wrappers for Library Resources

What ref1 does:
1. Wraps `Texture2D`, `Sound`, `RenderTexture2D`, `Shader`, `Image` in move-only RAII types.
2. Eliminates manual unload leaks and ownership ambiguity.

Reuse for SLAM:
1. Build small RAII wrappers for textures, sounds, render targets, and optional shader resources.
2. Keep wrappers non-copyable and move-safe.
3. Store validity checks inside wrappers.

## Pattern 5: Explicit Initialization Stages

What ref1 does:
1. Separates asset loading, collision mask creation, game creation, and AI pipeline init.
2. Logs each stage and fallback behavior.

Reuse for SLAM:
1. Stage init as:
- window/audio context
- world load (image/demo fallback)
- SLAM map/lidar/robot init
- UI control layout
- optional WASM-specific setup
2. Fail gracefully where possible (for example missing audio assets).

## Pattern 6: Feature Fallback Instead of Hard Fail

What ref1 does:
1. Uses fallback path when GPU preprocess is unavailable.
2. Keeps app running with reduced capability.

Reuse for SLAM:
1. Keep app running if sounds are missing.
2. Keep optional debug overlays behind runtime flags.
3. For WASM, provide single-thread fallback when thread prerequisites are missing.

## Pattern 7: Lightweight Deterministic Test Harness

What ref1 does:
1. Tests domain and logic behavior in `tests/test_main.cpp` without full UI automation.
2. Validates config defaults, state transitions, physics bounds, and model fallback behavior.

Reuse for SLAM:
1. Port Python tests into C++ logic tests first (core/movement/modes/audio state machine).
2. Add deterministic assertions for ray endpoints and occupancy updates.
3. Keep rendering checks focused on coordinate conversion and palette contracts.

## Pattern 8: Build Matrix Through Make Targets and Flags

What ref1 does:
1. Exposes multiple build/run modes via Make targets and env flags.
2. Keeps operational entrypoints simple for iterative work.

Reuse for SLAM:
1. Define clear targets early:
- `native-debug`
- `native-release`
- `wasm-debug`
- `wasm-release`
2. Keep runtime mode flags explicit and documented.

## Pattern Mapping to SLAM Units

From `tasks/ref2-architecture-scope.md`:

1. Unit A (Domain Core): apply Patterns 3 and 7.
2. Unit B (World Builder): apply Patterns 5 and 6.
3. Unit C (Input/Motion): apply Patterns 2 and 7.
4. Unit D (Rendering): apply Patterns 2 and 4.
5. Unit E (UI Controls): apply Patterns 2 and 5.
6. Unit F (Audio): apply Patterns 4 and 6.
7. Unit G (App Orchestrator): apply Patterns 1, 2, and 5.
8. Unit H (Parity Harness): apply Pattern 7.

## Guardrails to Keep

1. Never let render/input code mutate core map state in hidden ways.
2. Keep update order deterministic (`input -> update -> render`).
3. Prefer compile-time ownership guarantees (RAII) over manual cleanup.
4. Preserve behavior-first migration: match Python semantics before optimization.
