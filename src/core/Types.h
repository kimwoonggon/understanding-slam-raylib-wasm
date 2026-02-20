#pragma once

#include <cstdint>

namespace slam::core {

constexpr std::int16_t kUnknown = -1;
constexpr std::int16_t kFree = 0;
constexpr std::int16_t kOccupied = 100;

struct RobotPose {
  float x = 0.0F;
  float y = 0.0F;
  float theta = 0.0F;
};

struct ScanSample {
  float relativeAngle = 0.0F;
  float distance = 0.0F;
  bool hit = false;
};

}  // namespace slam::core
