#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "app/Config.h"

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

void TestConfigDefaultsMatchRef2() {
  const auto config = slam::app::AppConfig::Default();
  ASSERT_TRUE(config.world.width == 120, "world width must be 120");
  ASSERT_TRUE(config.world.height == 80, "world height must be 80");
  ASSERT_TRUE(config.screen.worldCellSize == 8, "cell size must be 8");
  ASSERT_TRUE(config.screen.fps == 60, "fps must be 60");
  ASSERT_TRUE(config.world.showWorldByDefault == false, "world visibility default must be OFF");
  ASSERT_TRUE(config.lidar.maxRange == 30.0F, "lidar max range must be 30");
  ASSERT_TRUE(config.lidar.beamCount == 72, "lidar beam count must be 72");
  ASSERT_TRUE(config.lidar.stepSize == 1.0F, "lidar step size must be 1.0");
}

}  // namespace

int main() {
  const std::vector<TestResult> results = {
      Run("Config defaults parity", TestConfigDefaultsMatchRef2),
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
