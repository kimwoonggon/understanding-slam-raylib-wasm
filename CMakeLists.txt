cmake_minimum_required(VERSION 3.20)

project(understanding_slam_raylib_wasm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(SLAM_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

set(SLAM_SOURCES
  src/main.cpp
  src/app/AssetPaths.cpp
  src/app/HeadlessSmoke.cpp
  src/app/SlamApp.cpp
  src/core/WorldGrid.cpp
  src/core/SimulatedLidar.cpp
  src/core/OccupancyGridMap.cpp
  src/audio/SoundController.cpp
  src/input/Motion.cpp
  src/render/Renderer.cpp
  src/ui/UiControls.cpp
  src/world/WorldLoader.cpp
)

add_executable(slam-raylib ${SLAM_SOURCES})

target_include_directories(slam-raylib PRIVATE src)
target_compile_options(slam-raylib PRIVATE -Wall -Wextra -Wpedantic)

find_package(raylib QUIET)

if(TARGET raylib)
  set(SLAM_RAYLIB_TARGET raylib)
elseif(TARGET raylib::raylib)
  set(SLAM_RAYLIB_TARGET raylib::raylib)
else()
  find_path(RAYLIB_INCLUDE_DIR raylib.h
    PATHS
      /opt/homebrew/include
      /usr/local/include
  )
  find_library(RAYLIB_LIBRARY raylib
    PATHS
      /opt/homebrew/lib
      /usr/local/lib
  )

  if(NOT RAYLIB_INCLUDE_DIR OR NOT RAYLIB_LIBRARY)
    message(FATAL_ERROR "raylib not found. Install raylib or provide a CMake package.")
  endif()

  add_library(raylib UNKNOWN IMPORTED)
  set_target_properties(raylib PROPERTIES
    IMPORTED_LOCATION "${RAYLIB_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${RAYLIB_INCLUDE_DIR}"
  )
  set(SLAM_RAYLIB_TARGET raylib)
endif()

target_link_libraries(slam-raylib PRIVATE ${SLAM_RAYLIB_TARGET})

if(EMSCRIPTEN)
  set_target_properties(slam-raylib PROPERTIES SUFFIX ".html")
  target_link_options(slam-raylib PRIVATE
    "SHELL:-sUSE_GLFW=3"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
    "SHELL:-sASSERTIONS=1"
    "SHELL:-sASYNCIFY"
    "SHELL:-sWASM=1"
    "SHELL:-sFORCE_FILESYSTEM=1"
    "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets@/assets"
  )
endif()

option(SLAM_BUILD_TESTS "Build SLAM core tests" ON)

if(SLAM_BUILD_TESTS)
  enable_testing()

  add_executable(slam-core-tests
    tests/core_tests.cpp
    src/core/WorldGrid.cpp
    src/core/SimulatedLidar.cpp
    src/core/OccupancyGridMap.cpp
  )
  target_include_directories(slam-core-tests PRIVATE src)
  target_compile_options(slam-core-tests PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slam-core-tests COMMAND slam-core-tests)

  add_executable(slam-motion-tests
    tests/motion_tests.cpp
    src/core/WorldGrid.cpp
    src/input/Motion.cpp
  )
  target_include_directories(slam-motion-tests PRIVATE src)
  target_compile_options(slam-motion-tests PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slam-motion-tests COMMAND slam-motion-tests)

  add_executable(slam-ui-tests
    tests/ui_tests.cpp
    src/ui/UiControls.cpp
  )
  target_include_directories(slam-ui-tests PRIVATE src)
  target_compile_options(slam-ui-tests PRIVATE -Wall -Wextra -Wpedantic)
  target_link_libraries(slam-ui-tests PRIVATE ${SLAM_RAYLIB_TARGET})
  add_test(NAME slam-ui-tests COMMAND slam-ui-tests)

  add_executable(slam-render-tests
    tests/render_tests.cpp
    src/render/Renderer.cpp
    src/core/OccupancyGridMap.cpp
  )
  target_include_directories(slam-render-tests PRIVATE src)
  target_compile_options(slam-render-tests PRIVATE -Wall -Wextra -Wpedantic)
  target_link_libraries(slam-render-tests PRIVATE ${SLAM_RAYLIB_TARGET})
  add_test(NAME slam-render-tests COMMAND slam-render-tests)

  add_executable(slam-world-loader-tests
    tests/world_loader_tests.cpp
    src/app/AssetPaths.cpp
    src/core/WorldGrid.cpp
    src/world/WorldLoader.cpp
  )
  target_include_directories(slam-world-loader-tests PRIVATE src)
  target_compile_options(slam-world-loader-tests PRIVATE -Wall -Wextra -Wpedantic)
  target_link_libraries(slam-world-loader-tests PRIVATE ${SLAM_RAYLIB_TARGET})
  add_test(NAME slam-world-loader-tests COMMAND slam-world-loader-tests)

  add_executable(slam-audio-tests
    tests/audio_tests.cpp
    src/audio/SoundController.cpp
  )
  target_include_directories(slam-audio-tests PRIVATE src)
  target_compile_options(slam-audio-tests PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slam-audio-tests COMMAND slam-audio-tests)

  add_executable(slam-asset-paths-tests
    tests/asset_paths_tests.cpp
    src/app/AssetPaths.cpp
  )
  target_include_directories(slam-asset-paths-tests PRIVATE src)
  target_compile_options(slam-asset-paths-tests PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slam-asset-paths-tests COMMAND slam-asset-paths-tests)

  add_executable(slam-config-tests
    tests/config_tests.cpp
  )
  target_include_directories(slam-config-tests PRIVATE src)
  target_compile_options(slam-config-tests PRIVATE -Wall -Wextra -Wpedantic)
  add_test(NAME slam-config-tests COMMAND slam-config-tests)

  if(NOT EMSCRIPTEN)
    add_executable(slam-static-server
      src/tools/StaticFileServer.cpp
    )
    target_compile_options(slam-static-server PRIVATE -Wall -Wextra -Wpedantic)

    add_executable(slam-diff-trace
      src/tools/DifferentialTrace.cpp
      src/app/AssetPaths.cpp
      src/core/WorldGrid.cpp
      src/core/SimulatedLidar.cpp
      src/core/OccupancyGridMap.cpp
      src/input/Motion.cpp
    )
    target_include_directories(slam-diff-trace PRIVATE src)
    target_compile_options(slam-diff-trace PRIVATE -Wall -Wextra -Wpedantic)

    add_test(
      NAME slam-native-e2e
      COMMAND ${CMAKE_COMMAND} -E env SLAM_HEADLESS_STEPS=120 $<TARGET_FILE:slam-raylib>
    )
  endif()
endif()
