/**
 * @file SimulatedLidar.cpp
 * @brief Ray-march lidar simulation implementation.
 */

#include "core/SimulatedLidar.h"

#include <cmath>
#include <stdexcept>

namespace slam::core {

/**
 * @brief Construct a lidar model with fixed scan parameters.
 */
SimulatedLidar::SimulatedLidar(double maxRange, int beamCount, double stepSize)
    : maxRange_(maxRange), beamCount_(beamCount), stepSize_(stepSize) {
  if (maxRange <= 0.0 || beamCount <= 0 || stepSize <= 0.0) {
    throw std::invalid_argument("SimulatedLidar parameters must be positive");
  }
}

/**
 * @brief Execute a full 360-degree scan.
 * @param world Ground-truth world grid.
 * @param pose Robot pose.
 * @return Beam samples containing distance and hit state.
 */
std::vector<ScanSample> SimulatedLidar::Scan(const WorldGrid& world, const RobotPose& pose) const {
  std::vector<ScanSample> samples;
  samples.reserve(static_cast<std::size_t>(beamCount_));

  constexpr double kTwoPi = 6.28318530717958647692;
  for (int beamIndex = 0; beamIndex < beamCount_; ++beamIndex) {
    const double relativeAngle = (kTwoPi * static_cast<double>(beamIndex)) / static_cast<double>(beamCount_);
    const double absoluteAngle = pose.theta + relativeAngle;
    const auto [distance, hit] = CastBeam(world, pose, absoluteAngle);
    samples.push_back(ScanSample{
        .relativeAngle = relativeAngle,
        .distance = distance,
        .hit = hit,
    });
  }
  return samples;
}

/**
 * @brief Cast one beam by ray-marching through the world.
 * @param world Ground-truth world grid.
 * @param pose Robot pose.
 * @param angle Absolute beam angle in radians.
 * @return Pair of measured distance and hit flag.
 */
std::pair<double, bool> SimulatedLidar::CastBeam(
    const WorldGrid& world, const RobotPose& pose, double angle) const {
  double distance = stepSize_;
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
