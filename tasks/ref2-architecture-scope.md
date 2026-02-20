# ref2 Architecture Scope and Migration Units

## Goal

Define the pygame project (`ref2`) as migration units for a Raylib C++ rewrite, while preserving behavior validated by existing tests.

## Source Snapshot

1. Entry point: `src/slam_understanding/app.py`
2. Core simulation: `src/slam_understanding/core.py`
3. Rendering utilities: `src/slam_understanding/rendering.py`
4. Movement/input helpers: `src/slam_understanding/movement.py`
5. UI controls: `src/slam_understanding/ui_controls.py`
6. World construction: `src/slam_understanding/world_loader.py`
7. Audio runtime control: `src/slam_understanding/audio.py`
8. Behavior spec: `tests/*.py`

## Runtime Data Flow (Current pygame)

1. App initializes window, controls, sound controller, world, SLAM map, lidar, and robot pose.
2. Event loop handles quit, reset, mode toggles, and mouse drag.
3. Keyboard or drag updates pose with obstacle collision constraints.
4. Lidar scan is generated from world + pose.
5. Occupancy map integrates current scan.
6. Render pass draws either world mode or map mode, then overlays rays/hit points/UI.
7. Audio reacts to movement and collision states.

## Functional Contracts to Preserve

1. Core math and map model:
- `WorldGrid`, `SimulatedLidar`, `OccupancyGridMap`, Bresenham scan integration.
2. Visual semantics:
- Black background, red laser rays, green hit points.
3. Mode semantics:
- Default world visibility is OFF.
- Green hit mode toggles between live-only and accumulate.
4. Input semantics:
- `WASD`/arrow movement.
- Left-drag robot repositioning; cannot cross walls.
5. Reset semantics:
- Reset clears reconstructed map only.
6. World loading:
- Use `assets/maze.png` when present, otherwise fallback demo world.
7. Audio semantics:
- Scan loop starts/stops with movement; collision uses cooldown.

## Migration Units (C++ Raylib)

## Unit A: Domain Core (No rendering/input/audio)

Includes:
1. `RobotPose`, `ScanSample` structures.
2. `WorldGrid` obstacle matrix and helpers.
3. `SimulatedLidar` scan raymarch.
4. `OccupancyGridMap` integration logic.
5. Bresenham helper.

Why first:
1. Pure deterministic logic.
2. Most testable and reusable for native + WASM.

## Unit B: World Builder

Includes:
1. Demo world generation.
2. Maze image-based obstacle generation (dark pixels -> obstacle).

Notes:
1. Replace `pygame.image.load` with C++ image loading path compatible with Raylib.

## Unit C: Input and Motion

Includes:
1. Keyboard movement vector + heading update.
2. Mouse-to-grid conversion.
3. Drag path probing with wall stop.

Notes:
1. Keep panel-offset conversion behavior aligned with tests.

## Unit D: Rendering

Includes:
1. Occupancy/world grid drawing.
2. Scan ray conversion to pixel-space.
3. Hit history update (live vs accumulate).
4. Robot indicator and palette mapping.

Notes:
1. Single-panel overlay layout is current expected behavior.

## Unit E: UI Controls and Modes

Includes:
1. Rect generation for reset/world/accumulate controls.
2. Button hit testing.
3. Event-to-action mapping (`I`, `M`, `G`, mouse click).

## Unit F: Audio Controller

Includes:
1. Looped movement sound state machine.
2. Collision one-shot with cooldown.
3. Safe no-audio fallback path.

## Unit G: Application Orchestrator

Includes:
1. Initialization and resource wiring.
2. Frame loop scheduling and update order.
3. Integration of Units A-F.

## Unit H: Test Parity Harness

Includes:
1. Deterministic unit tests for Units A-C and F.
2. Pixel/coordinate conversion assertions for rendering helpers.
3. Mode/default behavior assertions.

## Suggested Implementation Order

1. Unit A (core domain)
2. Unit B (world builder)
3. Unit C (input/motion)
4. Unit D (rendering helpers)
5. Unit E (UI controls/modes)
6. Unit F (audio controller)
7. Unit G (app orchestration)
8. Unit H (test parity hardening)

## Main Migration Risks

1. Event-loop model mismatch during pygame -> raylib port.
2. Pixel/grid rounding differences affecting drag and ray endpoints.
3. Image-loading threshold differences for obstacle extraction.
4. Audio behavior drift (loop/cooldown) across library APIs.
5. Hidden coupling in UI placement assumptions vs dynamic layout.

## Acceptance for This Scope Step

1. Modules are decomposed into explicit migration units.
2. Test-backed contracts are listed before implementation.
3. Sequencing and risks are documented for next step (`ref1` pattern extraction).
