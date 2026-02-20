#pragma once

#include "core/Types.h"
#include "core/WorldGrid.h"

namespace slam::input {

core::RobotPose HandleMotion(
    const core::RobotPose& pose,
    float speed,
    bool upPressed,
    bool downPressed,
    bool leftPressed,
    bool rightPressed);

core::RobotPose ApplyMouseDragToPose(
    const core::RobotPose& pose,
    int targetX,
    int targetY,
    const core::WorldGrid& world);

}  // namespace slam::input
