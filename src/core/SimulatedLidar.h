#pragma once

#include <utility>
#include <vector>

#include "core/Types.h"
#include "core/WorldGrid.h"

/**
 * @file SimulatedLidar.h
 * @brief Lidar beam simulation on a grid world.
 */

namespace slam::core {

/**
 * @brief Performs ray-march lidar scans over a WorldGrid.
 */
class SimulatedLidar {
 public:
  /**
   * @brief Construct a lidar model.
   * @param maxRange Maximum sensing range in grid units.
   * @param beamCount Number of beams per 360-degree scan.
   * @param stepSize Step size for beam marching.
   */
  SimulatedLidar(double maxRange, int beamCount, double stepSize);

  /**
   * @brief Run a full scan from the given robot pose.
   * @param world Ground-truth world.
   * @param pose Robot pose.
   * @return Per-beam measurements.
   */
  std::vector<ScanSample> Scan(const WorldGrid& world, const RobotPose& pose) const;

 private:
  std::pair<double, bool> CastBeam(const WorldGrid& world, const RobotPose& pose, double angle) const;

  double maxRange_ = 0.0;
  int beamCount_ = 0;
  double stepSize_ = 0.0;
};

}  // namespace slam::core
