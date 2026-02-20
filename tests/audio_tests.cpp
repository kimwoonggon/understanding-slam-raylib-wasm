#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "audio/SoundController.h"

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

class DummySound : public slam::audio::SoundLike {
 public:
  void Play(int loops) override {
    playCalls.push_back(loops);
  }
  void Stop() override {
    stopCalls += 1;
  }

  std::vector<int> playCalls;
  int stopCalls = 0;
};

TestResult Run(const std::string& name, const std::function<void()>& fn) {
  try {
    fn();
    return {name, true, ""};
  } catch (const std::exception& ex) {
    return {name, false, ex.what()};
  }
}

void TestScanLoopStartsOnceAndStops() {
  DummySound scan;
  DummySound collision;
  slam::audio::SoundController ctrl(
      &scan, &collision, true, []() { return 0.0; }, 0.2);

  ctrl.UpdateScan(true);
  ctrl.UpdateScan(true);
  ctrl.UpdateScan(false);

  ASSERT_TRUE(scan.playCalls.size() == 1U, "scan loop must start once");
  ASSERT_TRUE(scan.playCalls[0] == -1, "scan loop must use looped playback");
  ASSERT_TRUE(scan.stopCalls == 1, "scan loop must stop once");
}

void TestCollisionSoundUsesCooldown() {
  DummySound scan;
  DummySound collision;
  double now = 0.0;
  slam::audio::SoundController ctrl(
      &scan, &collision, true, [&]() { return now; }, 0.2);

  ctrl.PlayCollision();
  ctrl.PlayCollision();
  now = 0.25;
  ctrl.PlayCollision();

  ASSERT_TRUE(collision.playCalls.size() == 2U, "collision sound should respect cooldown");
  ASSERT_TRUE(collision.playCalls[0] == 0 && collision.playCalls[1] == 0, "collision sound should be one-shot");
}

}  // namespace

int main() {
  const std::vector<TestResult> results = {
      Run("Scan loop start/stop", TestScanLoopStartsOnceAndStops),
      Run("Collision cooldown", TestCollisionSoundUsesCooldown),
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
