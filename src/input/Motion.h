#pragma once

#include "core/Types.h"
#include "core/WorldGrid.h"

/**
 * @file Motion.h
 * @brief Keyboard and pointer motion helpers for robot control.
 */

namespace slam::input {

/**
 * @brief Compute next pose from directional key states.
 */
core::RobotPose HandleMotion(
    const core::RobotPose& pose,
    double speed,
    bool upPressed,
    bool downPressed,
    bool leftPressed,
    bool rightPressed);

/**
 * @brief Move the pose toward a dragged target while respecting obstacles.
 */
core::RobotPose ApplyMouseDragToPose(
    const core::RobotPose& pose,
    int targetX,
    int targetY,
    const core::WorldGrid& world);

}  // namespace slam::input
