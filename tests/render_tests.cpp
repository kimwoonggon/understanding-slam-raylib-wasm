#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/Types.h"
#include "render/Renderer.h"

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

void TestPaletteUsesReferenceColors() {
  ASSERT_TRUE(slam::render::Palette::kBackground.r == 0 && slam::render::Palette::kBackground.g == 0 &&
                  slam::render::Palette::kBackground.b == 0,
              "background must be black");
  ASSERT_TRUE(
      slam::render::Palette::kLaser.r == 255 && slam::render::Palette::kLaser.g == 0 &&
          slam::render::Palette::kLaser.b == 0,
      "laser must be red");
  ASSERT_TRUE(
      slam::render::Palette::kHit.r == 0 && slam::render::Palette::kHit.g == 255 &&
          slam::render::Palette::kHit.b == 0,
      "hit must be green");
}

void TestScanSamplesToPixelsReturnsExpectedEndpoints() {
  constexpr double kPi = 3.14159265358979323846;
  const slam::core::RobotPose pose{5.0, 5.0, 0.0};
  const std::vector<slam::core::ScanSample> scan = {
      {.relativeAngle = 0.0, .distance = 3.0, .hit = true},
      {.relativeAngle = kPi / 2.0, .distance = 2.0, .hit = true},
  };

  const std::vector<slam::render::PixelRay> rays =
      slam::render::ScanSamplesToPixels(pose, scan, 8, 0);

  ASSERT_TRUE(rays.size() == 2U, "must produce one ray per sample");
  ASSERT_TRUE(
      std::fabs(rays[0].start.x - 40.0F) < 1e-6F && std::fabs(rays[0].start.y - 40.0F) < 1e-6F,
      "ray0 start mismatch");
  ASSERT_TRUE(
      std::fabs(rays[0].end.x - 64.0F) < 1e-6F && std::fabs(rays[0].end.y - 40.0F) < 1e-6F,
      "ray0 end mismatch");
  ASSERT_TRUE(
      std::fabs(rays[1].end.x - 40.0F) < 1e-6F && std::fabs(rays[1].end.y - 56.0F) < 1e-6F,
      "ray1 end mismatch");
}

void TestUpdateHitPointHistoryAccumulatesOrReplaces() {
  const std::vector<Vector2> history = {{1.0F, 1.0F}};
  const std::vector<Vector2> current = {{2.0F, 2.0F}, {3.0F, 3.0F}};

  const auto replaced = slam::render::UpdateHitPointHistory(history, current, false);
  const auto accumulated = slam::render::UpdateHitPointHistory(history, current, true);

  ASSERT_TRUE(replaced.size() == 2U, "replace mode must return current hits only");
  ASSERT_TRUE(accumulated.size() == 3U, "accumulate mode must merge history + current");
}

void TestTryMarkHitPixelDeduplicatesByPixelIndex() {
  const int width = 8;
  const int height = 6;
  std::vector<unsigned char> occupancy(static_cast<std::size_t>(width * height), 0U);

  ASSERT_TRUE(
      slam::render::TryMarkHitPixel(occupancy, width, height, Vector2{3.0F, 4.0F}),
      "first mark in-bounds must be inserted");
  ASSERT_TRUE(
      !slam::render::TryMarkHitPixel(occupancy, width, height, Vector2{3.0F, 4.0F}),
      "second mark for same pixel must be deduplicated");
  ASSERT_TRUE(
      !slam::render::TryMarkHitPixel(occupancy, width, height, Vector2{-1.0F, 0.0F}),
      "out-of-bounds point must be rejected");
  ASSERT_TRUE(
      !slam::render::TryMarkHitPixel(occupancy, width, height, Vector2{8.0F, 0.0F}),
      "right-edge out-of-bounds point must be rejected");
}

}  // namespace

int main() {
  const std::vector<TestResult> results = {
      Run("Palette reference colors", TestPaletteUsesReferenceColors),
      Run("Scan endpoints", TestScanSamplesToPixelsReturnsExpectedEndpoints),
      Run("Hit history mode", TestUpdateHitPointHistoryAccumulatesOrReplaces),
      Run("Hit pixel dedup", TestTryMarkHitPixelDeduplicatesByPixelIndex),
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
