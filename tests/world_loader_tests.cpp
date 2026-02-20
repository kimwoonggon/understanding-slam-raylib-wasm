#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <raylib.h>

#include "app/AssetPaths.h"
#include "world/WorldLoader.h"

namespace {

struct TestResult {
  std::string name;
  bool passed = false;
  std::string message;
};

#define ASSERT_TRUE(cond, msg) \
  do {                         \
    if (!(cond)) {             \
      throw std::runtime_error(msg); \
    }                          \
  } while (false)

TestResult Run(const std::string& name, const std::function<void()>& fn) {
  try {
    fn();
    return {name, true, ""};
  } catch (const std::exception& ex) {
    return {name, false, ex.what()};
  }
}

void TestBuildWorldFromImageMarksDarkPixelsAsObstacles() {
  const std::filesystem::path tempPath = std::filesystem::path("tmp_world_loader_test.png");

  Image image = GenImageColor(3, 2, WHITE);
  ImageDrawPixel(&image, 1, 0, BLACK);
  ImageDrawPixel(&image, 2, 1, Color{5, 5, 5, 255});
  ExportImage(image, tempPath.string().c_str());
  UnloadImage(image);

  const slam::core::WorldGrid world =
      slam::world::BuildWorldFromImage(tempPath.string(), 3, 2);

  ASSERT_TRUE(world.IsObstacle(1, 0), "black pixel must map to obstacle");
  ASSERT_TRUE(world.IsObstacle(2, 1), "dark pixel must map to obstacle");
  ASSERT_TRUE(!world.IsObstacle(0, 0), "bright pixel must stay free");

  std::error_code ec;
  std::filesystem::remove(tempPath, ec);
}

void TestBuildDemoWorldAddsBorderWalls() {
  const slam::core::WorldGrid world = slam::world::BuildDemoWorld(12, 10);
  for (int x = 0; x < 12; ++x) {
    ASSERT_TRUE(world.IsObstacle(x, 0), "top border must be obstacle");
    ASSERT_TRUE(world.IsObstacle(x, 9), "bottom border must be obstacle");
  }
}

void TestMazeImageLoadingMatchesPixelThresholdRule() {
  const std::string mazePath = slam::app::ResolveAssetPath("assets/maze.png");
  ASSERT_TRUE(FileExists(mazePath.c_str()), "maze.png must be discoverable from current working directory");

  const slam::core::WorldGrid world = slam::world::BuildWorldFromImage(mazePath, 120, 80);

  Image image = LoadImage(mazePath.c_str());
  ASSERT_TRUE(image.data != nullptr, "maze image must load");
  if (image.width != 120 || image.height != 80) {
    ImageResize(&image, 120, 80);
  }
  Color* pixels = LoadImageColors(image);
  ASSERT_TRUE(pixels != nullptr, "maze image pixels must be readable");

  for (int y = 0; y < 80; ++y) {
    for (int x = 0; x < 120; ++x) {
      const Color c = pixels[y * 120 + x];
      const bool expectedObstacle = c.r < 32 && c.g < 32 && c.b < 32;
      const bool actualObstacle = world.IsObstacle(x, y);
      if (expectedObstacle != actualObstacle) {
        UnloadImageColors(pixels);
        UnloadImage(image);
        throw std::runtime_error("world/image obstacle mismatch at (" + std::to_string(x) + "," + std::to_string(y) + ")");
      }
    }
  }

  UnloadImageColors(pixels);
  UnloadImage(image);
}

}  // namespace

int main() {
  const std::vector<TestResult> results = {
      Run("Image dark-pixel mapping", TestBuildWorldFromImageMarksDarkPixelsAsObstacles),
      Run("Demo world border walls", TestBuildDemoWorldAddsBorderWalls),
      Run("Maze image threshold parity", TestMazeImageLoadingMatchesPixelThresholdRule),
  };

  int failed = 0;
  for (const TestResult& result : results) {
    if (result.passed) {
      std::cout << "[PASS] " << result.name << '\n';
    } else {
      ++failed;
      std::cout << "[FAIL] " << result.name << " :: " << result.message << '\n';
    }
  }
  std::cout << "Total: " << results.size() << ", Failed: " << failed << '\n';
  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
