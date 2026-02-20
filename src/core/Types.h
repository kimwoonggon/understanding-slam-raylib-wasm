#pragma once

#include <cstdint>

namespace slam::core {

constexpr std::int16_t kUnknown = -1;
constexpr std::int16_t kFree = 0;
constexpr std::int16_t kOccupied = 100;

struct RobotPose {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

struct ScanSample {
  double relativeAngle = 0.0;
  double distance = 0.0;
  bool hit = false;
};

}  // namespace slam::core
