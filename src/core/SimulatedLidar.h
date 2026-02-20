#pragma once

#include <utility>
#include <vector>

#include "core/Types.h"
#include "core/WorldGrid.h"

namespace slam::core {

class SimulatedLidar {
 public:
  SimulatedLidar(double maxRange, int beamCount, double stepSize);

  std::vector<ScanSample> Scan(const WorldGrid& world, const RobotPose& pose) const;

 private:
  std::pair<double, bool> CastBeam(const WorldGrid& world, const RobotPose& pose, double angle) const;

  double maxRange_ = 0.0;
  int beamCount_ = 0;
  double stepSize_ = 0.0;
};

}  // namespace slam::core
