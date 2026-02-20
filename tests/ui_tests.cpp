#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ui/UiControls.h"

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

void TestCreateUiControlsPlacesButtonsOnMapPanel() {
  const slam::ui::UiControls controls = slam::ui::CreateUiControls(960, 640);
  ASSERT_TRUE(controls.reset.x >= 960.0F, "reset button must be on map panel");
  ASSERT_TRUE(controls.toggleWorld.x >= 960.0F, "toggle button must be on map panel");
  ASSERT_TRUE(controls.accumulate.x >= 960.0F, "accumulate button must be on map panel");
}

void TestCreateUiControlsForWindowPinsButtonsToRightEdge() {
  const slam::ui::UiControls controls = slam::ui::CreateUiControlsForWindow(960, 640);
  ASSERT_TRUE(std::fabs(controls.reset.x - 790.0F) < 1e-6F, "reset must align to right edge");
  ASSERT_TRUE(std::fabs(controls.toggleWorld.x - 790.0F) < 1e-6F, "toggle must align to right edge");
  ASSERT_TRUE(std::fabs(controls.accumulate.x - 790.0F) < 1e-6F, "accumulate must align to right edge");
}

void TestButtonRectsAreOnMapPanel() {
  const Rectangle toggle = slam::ui::CreateToggleWorldButtonRect(960, 640);
  const Rectangle acc = slam::ui::CreateAccumulateButtonRect(960, 640);
  ASSERT_TRUE(toggle.x >= 960.0F, "toggle rect must be on map panel");
  ASSERT_TRUE(acc.x >= 960.0F, "accumulate rect must be on map panel");
}

void TestIsResetButtonClickDetectsInsidePoint() {
  const Rectangle rect{10.0F, 20.0F, 120.0F, 36.0F};
  ASSERT_TRUE(slam::ui::IsResetButtonClick(Vector2{50.0F, 40.0F}, rect), "inside point must return true");
  ASSERT_TRUE(!slam::ui::IsResetButtonClick(Vector2{5.0F, 5.0F}, rect), "outside point must return false");
}

void TestShouldResetFromInputs() {
  const Rectangle rect{10.0F, 20.0F, 120.0F, 36.0F};
  ASSERT_TRUE(slam::ui::ShouldResetFromInputs(true, false, Vector2{0.0F, 0.0F}, rect), "I key should reset");
  ASSERT_TRUE(
      slam::ui::ShouldResetFromInputs(false, true, Vector2{15.0F, 25.0F}, rect),
      "left click on button should reset");
  ASSERT_TRUE(
      !slam::ui::ShouldResetFromInputs(false, true, Vector2{0.0F, 0.0F}, rect),
      "left click outside button should not reset");
}

}  // namespace

int main() {
  const std::vector<TestResult> results = {
      Run("CreateUiControls placement", TestCreateUiControlsPlacesButtonsOnMapPanel),
      Run("CreateUiControlsForWindow placement", TestCreateUiControlsForWindowPinsButtonsToRightEdge),
      Run("Button rect placement", TestButtonRectsAreOnMapPanel),
      Run("Reset click detection", TestIsResetButtonClickDetectsInsidePoint),
      Run("Reset trigger inputs", TestShouldResetFromInputs),
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
