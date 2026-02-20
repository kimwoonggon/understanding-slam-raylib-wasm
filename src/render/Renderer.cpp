#include "render/Renderer.h"

#include <algorithm>
#include <cmath>

namespace slam::render {

void DrawWorld(const core::WorldGrid& world, int cellSize, int offsetX) {
  const auto& obstacles = world.ObstacleData();
  for (int y = 0; y < world.Height(); ++y) {
    for (int x = 0; x < world.Width(); ++x) {
      const int index = y * world.Width() + x;
      const Color color = obstacles[static_cast<std::size_t>(index)] != 0U
                              ? Palette::kWorldObstacle
                              : Palette::kBackground;
      DrawRectangle(offsetX + x * cellSize, y * cellSize, cellSize, cellSize, color);
    }
  }
}

void DrawMap(const core::OccupancyGridMap& map, int cellSize, int offsetX) {
  for (int y = 0; y < map.Height(); ++y) {
    for (int x = 0; x < map.Width(); ++x) {
      const auto value = map.ValueAt(x, y);
      const Color color = (value == core::kOccupied) ? Palette::kMapObstacle : Palette::kBackground;
      DrawRectangle(offsetX + x * cellSize, y * cellSize, cellSize, cellSize, color);
    }
  }
}

std::vector<PixelRay> ScanSamplesToPixels(
    const core::RobotPose& pose,
    const std::vector<core::ScanSample>& scan,
    int cellSize,
    int offsetX) {
  const Vector2 origin{
      static_cast<float>(static_cast<int>(pose.x * static_cast<double>(cellSize)) + offsetX),
      static_cast<float>(static_cast<int>(pose.y * static_cast<double>(cellSize)))};
  std::vector<PixelRay> output;
  output.reserve(scan.size());

  for (const core::ScanSample& sample : scan) {
    const double angle = pose.theta + sample.relativeAngle;
    const int endX = static_cast<int>((pose.x + std::cos(angle) * sample.distance) * static_cast<double>(cellSize)) + offsetX;
    const int endY = static_cast<int>((pose.y + std::sin(angle) * sample.distance) * static_cast<double>(cellSize));
    output.push_back(PixelRay{
        .start = origin,
        .end = {static_cast<float>(endX), static_cast<float>(endY)},
        .hit = sample.hit,
    });
  }

  return output;
}

std::vector<Vector2> UpdateHitPointHistory(
    const std::vector<Vector2>& history,
    const std::vector<Vector2>& currentHits,
    bool accumulate) {
  if (!accumulate) {
    return currentHits;
  }

  std::vector<Vector2> merged = history;
  for (const Vector2& point : currentHits) {
    const bool exists = std::any_of(
        merged.begin(), merged.end(), [&](const Vector2& existing) {
          return existing.x == point.x && existing.y == point.y;
        });
    if (!exists) {
      merged.push_back(point);
    }
  }
  return merged;
}

}  // namespace slam::render
