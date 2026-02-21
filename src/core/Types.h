#pragma once

#include <cstdint>

/**
 * @file Types.h
 * @brief Shared SLAM domain types and occupancy constants.
 */

namespace slam::core {

/// Unknown occupancy state for map cells.
constexpr std::int16_t kUnknown = -1;
/// Free occupancy state for map cells.
constexpr std::int16_t kFree = 0;
/// Occupied occupancy state for map cells.
constexpr std::int16_t kOccupied = 100;

/**
 * @brief Robot pose in world-grid coordinates.
 */
struct RobotPose {
  /// X position in grid units.
  double x = 0.0;
  /// Y position in grid units.
  double y = 0.0;
  /// Heading in radians.
  double theta = 0.0;
};

/**
 * @brief One lidar beam measurement relative to robot pose.
 */
struct ScanSample {
  /// Beam angle relative to the robot heading in radians.
  double relativeAngle = 0.0;
  /// Distance measured along the beam in grid units.
  double distance = 0.0;
  /// True when the beam terminates on an obstacle.
  bool hit = false;
};

}  // namespace slam::core
