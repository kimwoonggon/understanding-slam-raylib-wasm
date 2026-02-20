#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/Types.h"
#include "core/WorldGrid.h"
#include "input/Motion.h"

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

void TestHandleMotionUpdatesPoseAndHeading() {
  const slam::core::RobotPose pose{3.0F, 3.0F, 0.0F};
  const slam::core::RobotPose updated =
      slam::input::HandleMotion(pose, 0.5F, false, false, false, true);

  ASSERT_TRUE(std::fabs(updated.x - 3.5F) < 1e-6F, "x must move right by speed");
  ASSERT_TRUE(std::fabs(updated.y - 3.0F) < 1e-6F, "y must remain unchanged");
  ASSERT_TRUE(std::fabs(updated.theta - 0.0F) < 1e-6F, "heading must point right");
}

void TestApplyMouseDragMovesPoseToWorldGridCell() {
  const slam::core::WorldGrid world = slam::core::WorldGrid::WithBorderWalls(20, 20);
  const slam::core::RobotPose pose{3.0F, 3.0F, 0.0F};

  const slam::core::RobotPose updated =
      slam::input::ApplyMouseDragToPose(pose, 10, 5, world);

  ASSERT_TRUE(std::fabs(updated.x - 10.0F) < 1e-6F, "drag must move to target x");
  ASSERT_TRUE(std::fabs(updated.y - 5.0F) < 1e-6F, "drag must move to target y");
}

void TestApplyMouseDragDoesNotMoveIntoObstacle() {
  slam::core::WorldGrid world = slam::core::WorldGrid::WithBorderWalls(20, 20);
  world.SetObstacle(10, 5);
  const slam::core::RobotPose pose{3.0F, 3.0F, 0.0F};

  const slam::core::RobotPose updated =
      slam::input::ApplyMouseDragToPose(pose, 10, 5, world);

  ASSERT_TRUE(std::fabs(updated.x - 9.0F) < 1e-6F, "drag must stop before obstacle x");
  ASSERT_TRUE(std::fabs(updated.y - 5.0F) < 1e-6F, "drag must stop at reachable y");
}

void TestMouseDragDoesNotCrossWallBarrier() {
  slam::core::WorldGrid world(20, 20);
  world.AddRectangle(5, 0, 1, 20);
  const slam::core::RobotPose pose{3.0F, 10.0F, 0.0F};

  const slam::core::RobotPose updated =
      slam::input::ApplyMouseDragToPose(pose, 8, 10, world);

  ASSERT_TRUE(std::fabs(updated.x - 4.0F) < 1e-6F, "drag must stop at last free x before wall");
  ASSERT_TRUE(std::fabs(updated.y - 10.0F) < 1e-6F, "drag must stay on same y");
}

}  // namespace

int main() {
  const std::vector<TestResult> results = {
      Run("HandleMotion heading", TestHandleMotionUpdatesPoseAndHeading),
      Run("Drag move to cell", TestApplyMouseDragMovesPoseToWorldGridCell),
      Run("Drag obstacle stop", TestApplyMouseDragDoesNotMoveIntoObstacle),
      Run("Drag wall barrier", TestMouseDragDoesNotCrossWallBarrier),
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
