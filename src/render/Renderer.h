#pragma once

#include <vector>

#include <raylib.h>

#include "core/OccupancyGridMap.h"
#include "core/Types.h"
#include "core/WorldGrid.h"

namespace slam::render {

struct PixelRay {
  Vector2 start{};
  Vector2 end{};
  bool hit = false;
};

struct Palette {
  static constexpr Color kBackground{0, 0, 0, 255};
  static constexpr Color kWorldObstacle{150, 150, 150, 255};
  static constexpr Color kMapObstacle{80, 80, 80, 255};
  static constexpr Color kLaser{255, 0, 0, 255};
  static constexpr Color kHit{0, 255, 0, 255};
  static constexpr Color kRobot{0, 220, 0, 255};
  static constexpr Color kText{0, 255, 0, 255};
};

void DrawWorld(const core::WorldGrid& world, int cellSize, int offsetX);
void DrawMap(const core::OccupancyGridMap& map, int cellSize, int offsetX);
std::vector<PixelRay> ScanSamplesToPixels(
    const core::RobotPose& pose,
    const std::vector<core::ScanSample>& scan,
    int cellSize,
    int offsetX);
std::vector<Vector2> UpdateHitPointHistory(
    const std::vector<Vector2>& history,
    const std::vector<Vector2>& currentHits,
    bool accumulate);

}  // namespace slam::render
