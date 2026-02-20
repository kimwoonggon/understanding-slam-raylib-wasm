#include "core/SimulatedLidar.h"

#include <cmath>
#include <stdexcept>

namespace slam::core {

SimulatedLidar::SimulatedLidar(float maxRange, int beamCount, float stepSize)
    : maxRange_(maxRange), beamCount_(beamCount), stepSize_(stepSize) {
  if (maxRange <= 0.0F || beamCount <= 0 || stepSize <= 0.0F) {
    throw std::invalid_argument("SimulatedLidar parameters must be positive");
  }
}

std::vector<ScanSample> SimulatedLidar::Scan(const WorldGrid& world, const RobotPose& pose) const {
  std::vector<ScanSample> samples;
  samples.reserve(static_cast<std::size_t>(beamCount_));

  constexpr float kTwoPi = 6.28318530717958647692F;
  for (int beamIndex = 0; beamIndex < beamCount_; ++beamIndex) {
    const float relativeAngle = (kTwoPi * static_cast<float>(beamIndex)) / static_cast<float>(beamCount_);
    const float absoluteAngle = pose.theta + relativeAngle;
    const auto [distance, hit] = CastBeam(world, pose, absoluteAngle);
    samples.push_back(ScanSample{
        .relativeAngle = relativeAngle,
        .distance = distance,
        .hit = hit,
    });
  }
  return samples;
}

std::pair<float, bool> SimulatedLidar::CastBeam(
    const WorldGrid& world, const RobotPose& pose, float angle) const {
  float distance = stepSize_;
  while (distance <= maxRange_) {
    const int x = static_cast<int>(pose.x + std::cos(angle) * distance);
    const int y = static_cast<int>(pose.y + std::sin(angle) * distance);
    if (world.IsObstacle(x, y)) {
      return {distance, true};
    }
    distance += stepSize_;
  }
  return {maxRange_, false};
}

}  // namespace slam::core
