#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/OccupancyGridMap.h"
#include "core/SimulatedLidar.h"
#include "core/Types.h"
#include "core/WorldGrid.h"

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

void TestLidarDetectsExpectedWallDistance() {
  slam::core::WorldGrid world(20, 20);
  world.SetObstacle(8, 5);
  slam::core::SimulatedLidar lidar(10.0F, 4, 1.0F);
  slam::core::RobotPose pose{5.0F, 5.0F, 0.0F};

  const std::vector<slam::core::ScanSample> scan = lidar.Scan(world, pose);

  ASSERT_TRUE(std::fabs(scan[0].distance - 3.0F) < 1e-6F, "forward beam distance must be 3.0");
  ASSERT_TRUE(scan[0].hit, "forward beam must hit obstacle");
}

void TestOccupancyGridMarksFreeAndHitCells() {
  slam::core::OccupancyGridMap map(20, 20);
  slam::core::RobotPose pose{5.0F, 5.0F, 0.0F};
  std::vector<slam::core::ScanSample> scan{
      slam::core::ScanSample{.relativeAngle = 0.0F, .distance = 3.0F, .hit = true}};
  map.IntegrateScan(pose, scan);

  ASSERT_TRUE(map.ValueAt(6, 5) == slam::core::kFree, "cell (6,5) must be free");
  ASSERT_TRUE(map.ValueAt(7, 5) == slam::core::kFree, "cell (7,5) must be free");
  ASSERT_TRUE(map.ValueAt(8, 5) == slam::core::kOccupied, "cell (8,5) must be occupied");
}

void TestWorldBuilderAddsBorderWalls() {
  const slam::core::WorldGrid world = slam::core::WorldGrid::WithBorderWalls(12, 10);
  for (int x = 0; x < 12; ++x) {
    ASSERT_TRUE(world.IsObstacle(x, 0), "top border must be obstacle");
    ASSERT_TRUE(world.IsObstacle(x, 9), "bottom border must be obstacle");
  }
  for (int y = 0; y < 10; ++y) {
    ASSERT_TRUE(world.IsObstacle(0, y), "left border must be obstacle");
    ASSERT_TRUE(world.IsObstacle(11, y), "right border must be obstacle");
  }
}

void TestResetClearsMapToUnknown() {
  slam::core::OccupancyGridMap map(20, 20);
  slam::core::RobotPose pose{5.0F, 5.0F, 0.0F};
  map.IntegrateScan(
      pose, {slam::core::ScanSample{.relativeAngle = 0.0F, .distance = 3.0F, .hit = true}});
  ASSERT_TRUE(map.ValueAt(8, 5) == slam::core::kOccupied, "precondition: occupied after integration");

  map.Reset();
  ASSERT_TRUE(map.ValueAt(8, 5) == slam::core::kUnknown, "reset must clear to unknown");
}

}  // namespace

int main() {
  const std::vector<TestResult> results = {
      Run("Lidar wall distance", TestLidarDetectsExpectedWallDistance),
      Run("Occupancy integration", TestOccupancyGridMarksFreeAndHitCells),
      Run("World border walls", TestWorldBuilderAddsBorderWalls),
      Run("Map reset", TestResetClearsMapToUnknown),
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
