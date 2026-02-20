# Raylib C++ Module Map (Baseline)

## Objective

Define the initial C++ module layout for migrating `ref2` to Raylib, with clear boundaries aligned to the migration units.

## Build and Entry

1. `CMakeLists.txt`
- Builds `slam-raylib` baseline executable.
- Builds `slam-core-tests` deterministic core test target.
2. `src/main.cpp`
- Thin entrypoint.
- Creates default config and runs `SlamApp`.

## Application Layer

1. `src/app/Config.h`
- Central runtime configuration for screen, world, lidar, and motion defaults.
2. `src/app/SlamApp.h`
- App orchestration interface and private runtime state.
3. `src/app/SlamApp.cpp`
- Frame loop orchestration (`input -> scan/update -> draw`).
- Keyboard/mouse movement, mode toggles, map reset.
- Demo-world bootstrap and hit-history behavior.

## Domain Core Layer

1. `src/core/Types.h`
- Shared domain types and occupancy constants.
2. `src/core/WorldGrid.h/.cpp`
- Obstacle grid, bounds checks, wall/rectangle helpers.
3. `src/core/SimulatedLidar.h/.cpp`
- 360-degree raymarch scan and hit-distance computation.
4. `src/core/OccupancyGridMap.h/.cpp`
- Scan integration into occupancy map using Bresenham ray tracing.

## Rendering Layer

1. `src/render/Renderer.h/.cpp`
- World/map grid rasterization.
- Scan-sample to pixel-ray conversion.
- Shared color palette constants.

## Testing Layer (TDD + cpp-testing)

1. `tests/core_tests.cpp`
- Core deterministic behavior checks:
  - lidar wall distance
  - occupancy map free/occupied integration
  - border-wall construction
  - map reset semantics

## Mapping to Migration Units

1. Unit A (Domain Core): `src/core/*`
2. Unit B (World Builder): currently in `SlamApp::InitializeDemoWorld`, later isolate file loader module
3. Unit C (Input/Motion): currently in `SlamApp::HandleInput`, later split dedicated motion module
4. Unit D (Rendering): `src/render/*`
5. Unit E (UI Controls): placeholder in app text UI; later split UI controls module
6. Unit F (Audio): not yet implemented in C++ baseline
7. Unit G (Orchestration): `src/app/SlamApp.*`
8. Unit H (Parity Tests): `tests/core_tests.cpp` baseline, to be expanded
