/**
 * @file HeadlessSmoke.cpp
 * @brief Deterministic headless simulation loop for smoke validation.
 */

#include "app/HeadlessSmoke.h"

#include <cmath>

#include "core/OccupancyGridMap.h"
#include "core/SimulatedLidar.h"
#include "core/Types.h"
#include "world/WorldLoader.h"

namespace slam::app {

/**
 * @brief Run a deterministic headless simulation for smoke validation.
 * @param config Runtime configuration.
 * @param steps Number of simulation steps.
 * @return 0 on success; non-zero if map integration evidence is insufficient.
 */
int RunHeadlessSmoke(const AppConfig& config, int steps) {
  if (steps <= 0) {
    return 0;
  }

  core::WorldGrid world = world::BuildDemoWorld(config.world.width, config.world.height);
  core::OccupancyGridMap map(config.world.width, config.world.height);
  core::SimulatedLidar lidar(config.lidar.maxRange, config.lidar.beamCount, config.lidar.stepSize);
  core::RobotPose pose{10.0, 10.0, 0.0};

  for (int i = 0; i < steps; ++i) {
    const auto scan = lidar.Scan(world, pose);
    map.IntegrateScan(pose, scan);

    const double phase = static_cast<double>(i % 4);
    const double vx = (phase < 2.0) ? 0.5 : -0.5;
    const double vy = (phase == 1.0 || phase == 2.0) ? 0.5 : -0.5;
    const core::RobotPose candidate{
        .x = pose.x + vx,
        .y = pose.y + vy,
        .theta = std::atan2(vy, vx),
    };
    if (!world.IsObstacle(static_cast<int>(candidate.x), static_cast<int>(candidate.y))) {
      pose = candidate;
    }
  }

  bool hasOccupied = false;
  bool hasFree = false;
  for (const auto value : map.Data()) {
    hasOccupied = hasOccupied || (value == core::kOccupied);
    hasFree = hasFree || (value == core::kFree);
    if (hasOccupied && hasFree) {
      return 0;
    }
  }
  return 1;
}

}  // namespace slam::app
