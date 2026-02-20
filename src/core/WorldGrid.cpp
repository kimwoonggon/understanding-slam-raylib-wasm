#include "core/WorldGrid.h"

#include <algorithm>
#include <stdexcept>

namespace slam::core {

WorldGrid::WorldGrid(int width, int height)
    : width_(width),
      height_(height),
      obstacles_(static_cast<std::size_t>(std::max(width, 0) * std::max(height, 0)), 0U) {
  if (width <= 0 || height <= 0) {
    throw std::invalid_argument("WorldGrid dimensions must be positive");
  }
}

WorldGrid WorldGrid::WithBorderWalls(int width, int height) {
  WorldGrid world(width, height);
  for (int x = 0; x < width; ++x) {
    world.SetObstacle(x, 0);
    world.SetObstacle(x, height - 1);
  }
  for (int y = 0; y < height; ++y) {
    world.SetObstacle(0, y);
    world.SetObstacle(width - 1, y);
  }
  return world;
}

bool WorldGrid::InBounds(int x, int y) const {
  return x >= 0 && x < width_ && y >= 0 && y < height_;
}

void WorldGrid::SetObstacle(int x, int y) {
  if (!InBounds(x, y)) {
    return;
  }
  obstacles_[static_cast<std::size_t>(Index(x, y))] = 1U;
}

void WorldGrid::AddRectangle(int x, int y, int width, int height) {
  const int xStart = std::max(0, x);
  const int yStart = std::max(0, y);
  const int xEnd = std::min(width_, x + width);
  const int yEnd = std::min(height_, y + height);

  for (int row = yStart; row < yEnd; ++row) {
    for (int col = xStart; col < xEnd; ++col) {
      SetObstacle(col, row);
    }
  }
}

bool WorldGrid::IsObstacle(int x, int y) const {
  if (!InBounds(x, y)) {
    return true;
  }
  return obstacles_[static_cast<std::size_t>(Index(x, y))] != 0U;
}

int WorldGrid::Index(int x, int y) const {
  return y * width_ + x;
}

}  // namespace slam::core
