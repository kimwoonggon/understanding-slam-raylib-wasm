#pragma once

#include <vector>

#include <raylib.h>

#include "core/OccupancyGridMap.h"
#include "core/Types.h"
#include "core/WorldGrid.h"

/**
 * @file Renderer.h
 * @brief Rendering helpers for world/map/lidar overlays.
 */

namespace slam::render {

/**
 * @brief Pixel-space representation of one lidar beam.
 */
struct PixelRay {
  /// Pixel start position.
  Vector2 start{};
  /// Pixel end position.
  Vector2 end{};
  /// True when the beam hit an obstacle.
  bool hit = false;
};

/**
 * @brief Color palette constants used by the app.
 */
struct Palette {
  static constexpr Color kBackground{0, 0, 0, 255};
  static constexpr Color kWorldObstacle{150, 150, 150, 255};
  static constexpr Color kMapObstacle{80, 80, 80, 255};
  static constexpr Color kLaser{255, 0, 0, 255};
  static constexpr Color kHit{0, 255, 0, 255};
  static constexpr Color kRobot{0, 220, 0, 255};
  static constexpr Color kText{0, 255, 0, 255};
};

/**
 * @brief Draw the ground-truth world obstacle grid.
 */
void DrawWorld(const core::WorldGrid& world, int cellSize, int offsetX);
/**
 * @brief Draw the reconstructed occupancy map.
 */
void DrawMap(const core::OccupancyGridMap& map, int cellSize, int offsetX);
/**
 * @brief Convert scan samples to pixel-space rays.
 */
std::vector<PixelRay> ScanSamplesToPixels(
    const core::RobotPose& pose,
    const std::vector<core::ScanSample>& scan,
    int cellSize,
    int offsetX);
/**
 * @brief Update green-hit history in live or accumulate mode.
 */
std::vector<Vector2> UpdateHitPointHistory(
    const std::vector<Vector2>& history,
    const std::vector<Vector2>& currentHits,
    bool accumulate);

}  // namespace slam::render
