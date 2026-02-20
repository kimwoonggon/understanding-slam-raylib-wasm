#pragma once

#include <utility>
#include <vector>

#include "core/Types.h"
#include "core/WorldGrid.h"

namespace slam::core {

class SimulatedLidar {
 public:
  SimulatedLidar(float maxRange, int beamCount, float stepSize);

  std::vector<ScanSample> Scan(const WorldGrid& world, const RobotPose& pose) const;

 private:
  std::pair<float, bool> CastBeam(const WorldGrid& world, const RobotPose& pose, float angle) const;

  float maxRange_ = 0.0F;
  int beamCount_ = 0;
  float stepSize_ = 0.0F;
};

}  // namespace slam::core
