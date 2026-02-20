/**
 * @file main.cpp
 * @brief Application entrypoint selecting interactive or headless execution.
 */

#include "app/Config.h"
#include "app/HeadlessSmoke.h"
#include "app/SlamApp.h"

#include <cstdlib>

int main() {
  const slam::app::AppConfig config = slam::app::AppConfig::Default();

  const char* headlessSteps = std::getenv("SLAM_HEADLESS_STEPS");
  if (headlessSteps != nullptr) {
    const int steps = std::atoi(headlessSteps);
    if (steps > 0) {
      return slam::app::RunHeadlessSmoke(config, steps);
    }
  }

  slam::app::SlamApp app(config);
  return app.Run();
}
