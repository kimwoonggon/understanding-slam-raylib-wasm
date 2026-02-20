/**
 * @file Motion.cpp
 * @brief Motion update helpers for keyboard and drag interactions.
 */

#include "input/Motion.h"

#include <algorithm>
#include <cmath>

namespace slam::input {

core::RobotPose HandleMotion(
    const core::RobotPose& pose,
    double speed,
    bool upPressed,
    bool downPressed,
    bool leftPressed,
    bool rightPressed) {
  double vx = 0.0;
  double vy = 0.0;

  if (upPressed) {
    vy -= speed;
  }
  if (downPressed) {
    vy += speed;
  }
  if (leftPressed) {
    vx -= speed;
  }
  if (rightPressed) {
    vx += speed;
  }

  if (vx == 0.0 && vy == 0.0) {
    return pose;
  }

  return core::RobotPose{
      .x = pose.x + vx,
      .y = pose.y + vy,
      .theta = std::atan2(vy, vx),
  };
}

core::RobotPose ApplyMouseDragToPose(
    const core::RobotPose& pose, int targetX, int targetY, const core::WorldGrid& world) {
  const int startX = static_cast<int>(pose.x);
  const int startY = static_cast<int>(pose.y);
  const int maxDelta = std::max(std::abs(targetX - startX), std::abs(targetY - startY));
  if (maxDelta == 0) {
    return pose;
  }

  int lastFreeX = startX;
  int lastFreeY = startY;
  for (int step = 1; step <= maxDelta; ++step) {
    const double t = static_cast<double>(step) / static_cast<double>(maxDelta);
    const int probeX = static_cast<int>(std::round(static_cast<double>(startX) + static_cast<double>(targetX - startX) * t));
    const int probeY = static_cast<int>(std::round(static_cast<double>(startY) + static_cast<double>(targetY - startY) * t));
    if (world.IsObstacle(probeX, probeY)) {
      break;
    }
    lastFreeX = probeX;
    lastFreeY = probeY;
  }

  return core::RobotPose{
      .x = static_cast<double>(lastFreeX),
      .y = static_cast<double>(lastFreeY),
      .theta = pose.theta,
  };
}

}  // namespace slam::input
